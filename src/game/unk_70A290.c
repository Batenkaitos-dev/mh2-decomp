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
s32 func_3648F0(void);
s32 Item_UsableOutsideCamp(void);
s32 Item_CanUseSmallBarrelBomb(void);
s32 func_364A70(void);
s32 Scene_FewPlaced3_Unk(void);
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
void Player_State0_01(Player *em, s32 variant);
void Player_State0_27(Player *em, s32 arg1);
void Player_State0_28(Player *player, s32 arg1);
u32 Player_TestFlag(void *obj, u32 flag);
void Player_TryDrawWeapon(Player *em, u8 mode);
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
extern s16 Player_State0_01_NextByWeapon[];
s32 Player_HasMealEffect(Player *pl, s16 kind);
void Player_GatherStep(Player *p);
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
void Hammer_Draw(Player *em);
void HuntingHorn_Draw(Player *em, s32 a);
void DualSwords_Draw(Player *em, s32 a);
void LongSword_Draw(Player *p);
void Bow_Draw(Player *em);
void Player_ResetAim(Player *p);
void Hammer_EvadeCharge(Player *em);
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
void SeqA_Start(s32 id);
s32 Versus_BothDefeated(void);
void Effect2C_Spawn(s16);
typedef struct WeaponMotion { s16 motion; s16 blend; s16 flag; s16 next; } WeaponMotion;
extern WeaponMotion Player_State0_04_Motions0[];
extern WeaponMotion Player_State0_04_Motions4[];
extern WeaponMotion Player_State0_04_Motions2[];
extern WeaponMotion Player_State0_04_Motions5[];
void Player_UseItem(Player *em, u8 mode);
s32 Player_PouchHasSelectable(Player *p);
u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Save_ClearEventFlag(s32 n);
s32 func_3A46B0(u8 *ids);
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
void Player_SetMotionPair(Player *, s32, s32, s32); /* unit's own declaration: the last parameter s32 (the definition has s16); with the definition's type Player_State0_55 by 418 of 469 words differ */
void Player_SetMoveMode(Player *, u16 mode, s16 flag1, s16 flag2);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Quest_DeliverFromPouch(Player *pl, s32 mode);
s32 Save_TestEventFlag(s32);
void Lance_GatherAttack(Player *p, s32 arg1);
void SwordShield_GatherAttack(Player *p);
s32 Gunlance_GatherInput(Player *p, s32 arg1);
u8 Player_StickMoveLevel(Player *, s32);
s32 Player_TrySideStep(Player *, s16);
s32 Player_IdleInput(Player *);
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
    Player *p;
    s8 i;

    em->turnStep = 0;
    i = em->targetNo;
    if (i == -1) {
        em->targetAngle = em->stateRand;
    } else {
        p = &gPlayers[i];
        em->target = p;
        Em_GetTargetPos(em, (u8) em->targetNo, pos);
        Area_ToWorld((s32) p->area, pos, pos2);
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
    n = func_3A46B0(ids);
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
    Player *p = &gPlayers[gScene.localPlayer];
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
                Jingle_Play(p, ev->data->jingleDay);
            } else {
                Jingle_Play(p, ev->data->jingleNight);
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
                    Jingle_Play(p, 0xDD);
                    break;
                case 5:
                    Jingle_Play(p, 0xDD);
                    break;
                case 6:
                    Jingle_Play(p, 0xDD);
                    break;
                case 7:
                    Jingle_Play(p, 0xDD);
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
            SeqA_Start(6);
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

static void Player_SetAimLayers(Player *p, u16 motA, u16 motB)
{

    if (p->aimWeight == 0) {
        p->partAnimOn[0] = 0;
        p->partAnimOn[1] = 0;
        return;
    }
    p->partAnimOn[0] = 1;
    p->partAnimOn[1] = 1;
    if (p->aimWeight > 0) {
        if (motA == 0x581) {
            p->partAnimMotion[0] = 0x580;
            p->partAnimOn[0] = 0;
        } else {
            p->partAnimMotion[0] = motA;
        }
        p->partAnimMotion[1] = motA + 100;
        p->partAnimWeight[0] = p->aimWeight;
        p->partAnimWeight[1] = p->aimWeight;
    } else {
        if (motB == 0x582) {
            p->partAnimMotion[0] = 0x580;
            p->partAnimOn[0] = 0;
        } else {
            p->partAnimMotion[0] = motB;
        }
        p->partAnimMotion[1] = motB + 100;
        p->partAnimWeight[0] = -p->aimWeight;
        p->partAnimWeight[1] = -p->aimWeight;
    }
    if (p->partAnimWeight[0] > 100) {
        p->partAnimWeight[0] = 100;
    }
    if (p->partAnimWeight[1] > 100) {
        p->partAnimWeight[1] = 100;
    }
}

static void Bowgun_AddAimWeight(Player *p, s8 delta)
{

    p->aimWeight += delta;
    if (p->aimWeight >= 100) {
        p->aimWeight = 100;
    }
    if (p->aimWeight < -99) {
        p->aimWeight = -100;
    }
}

void Player_UpdateEnvironment(Player *p)
{
    f32 v[3];
    s16 n;
    s16 k;
    u8 f;
    u16 iv;

    if (p->stateGroup == 3) {
        return;
    }
    p->inPoisonArea = 0;
    if (p->poison > 0) {
        if (--p->poisonTimer <= 0) {
            p->poison--;
            if (Player_HasSkill(p, 0xA) == 1) {
                k = 60;
            } else if (Player_HasSkill(p, 0xC) == 1) {
                k = 15;
            } else {
                k = 30;
            }
            p->poisonTimer = k;
            if (Player_IsLocal(p) == 1) {
                v[0] = 0.0f;
                v[1] = 20.0f;
                v[2] = 0.0f;
                Effect06_SpawnAt(p, 3, 0x14, v, 1.0f);
            }
            if (Player_Damage(p, 1, 0) == 1) {
                return;
            }
        }
    }
    if (Player_IsLocal(p) == 0 && (p->statusIcons & 1)) {
        k = Player_HasSkill(p, 0xA) ? 60 : 30;
        if (gScene.frame % k == 0) {
            v[0] = 0.0f;
            v[1] = 20.0f;
            v[2] = 0.0f;
            Effect06_SpawnAt(p, 3, 0x14, v, 1.0f);
        }
    }
    if (Player_IsLocal(p) == 1) {
        n = Player_CountSpecialItems(p);
        if (n > 0 && gScene.frame % 30 == 0) {
            if (Player_Damage(p, n, 0) == 1) {
                return;
            }
        }
    }
    if (Player_IsLocal(p) != 1) {
        return;
    }
    k = Area_Temperature(p->area);
    switch (k) {
    case 1:
    case 3:
        if (p->coolDrinkTimer != 0) {
            break;
        }
        if (k == 1 && (Player_HasSkill(p, 0x85) == 1 || Player_HasSkill(p, 0x86) == 1)) {
            break;
        }
        if (k == 3 && Player_HasSkill(p, 0x86) == 1) {
            break;
        }
        if (--p->envTimer > 0) {
            break;
        }
        if (k == 1) {
            p->envTimer = 90;
            if (Player_HasSkill(p, 0x87) == 1) {
                p->envTimer = 60;
            } else if (Player_HasSkill(p, 0x88) == 1) {
                p->envTimer = 45;
            } else if (Player_HasSkill(p, 0x84) == 1) {
                p->envTimer = 180;
            }
        } else {
            p->envTimer = 60;
            if (Player_HasSkill(p, 0x87) == 1) {
                p->envTimer = 40;
            } else if (Player_HasSkill(p, 0x88) == 1) {
                p->envTimer = 30;
            } else if (Player_HasSkill(p, 0x84) == 1) {
                p->envTimer = 120;
            } else if (Player_HasSkill(p, 0x85) == 1) {
                p->envTimer = 180;
            }
        }
        if (Player_Damage(p, 1, 0) == 1) {
            return;
        }
        break;
    case 2:
    case 4:
        break;
    }
    if (p->stateGroup == 2) {
        switch (p->state) {
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
    if (p->stateGroup == 3) {
        return;
    }
    if (p->stateGroup == 4) {
        return;
    }
    if (p->action == 2) {
        return;
    }
    if (p->noHitTimer > 0) {
        return;
    }
    f = p->unk669;
    if (f == 0 && Player_NearEm40(p) == 1) {
        f = 2;
    }
    StageObj_K29_TickType6(p, f);
    switch (f) {
    case 0:
        break;
    case 1:
        if (Player_HasSkill(p, 0xB) == 0) {
            p->inPoisonArea = 1;
            if (p->poison < 10) {
                p->poison = 10;
            }
        }
        break;
    case 2:
        p->inPoisonArea = 1;
        iv = 8;
        if (Player_HasSkill(p, 0xBF) == 1) {
            iv = 12;
        } else if (Player_HasSkill(p, 0xC0) == 1) {
            iv = 24;
        } else if (Player_HasSkill(p, 0xC1) == 1) {
            iv = 6;
        } else if (Player_HasSkill(p, 0xC2) == 1) {
            iv = 4;
        }
        if (gScene.frame % iv == 0) {
            if (Player_Damage(p, 1, 0) == 1) {
                return;
            }
        }
        break;
    }
}

s32 Player_PouchHasSelectable(Player *p)
{
    s16 i;

    for (i = 0; i < 20; i++) {
        if (p->pouch[i].item != 0 && p->pouch[i].count > 0 && (gItemData[p->pouch[i].item].flags & 1)) {
            return 1;
        }
    }
    return 0;
}

void Player_UpdateItemSelect(Player *p)
{
    u16 b;

    if (Player_IsLocal(p) != 0) {
        if (Player_IsState(p, 2, 0x13) == 0 && (u32) (p->stateGroup - 3) > 1 && Quest_PhaseIsOver(1) != 1) {
            if (p->input.held2 & 4) {
                if (p->itemSelect == 0) {
                    Sound_PlaySeFlat(0, 0x11, 0);
                }
                p->itemSelect = 1;
                if (Player_PouchHasSelectable(p) != 0) {
                    b = p->input.pressed2;
                    if (b & 0x20) {
                        Sound_PlaySeFlat(0, 0x12, 0);
                        p->pouchCursor = Pouch_NextSelectable(p, p->pouchCursor, 0);
                        p->pouchHudFlags |= 2;
                    } else if (b & 0x200) {
                        Sound_PlaySeFlat(0, 0x12, 0);
                        p->pouchCursor = Pouch_NextSelectable(p, p->pouchCursor, 1);
                        p->pouchHudFlags |= 4;
                    }
                }
            } else if (p->itemSelect != 0) {
                Sound_PlaySeFlat(0, 0x14, 0);
                p->itemSelect = 0;
            }
            return;
        }
        p->itemSelect = 0;
    }
}

static s32 Player_AmmoTypeChanged(Player *p)
{
    if ((p->isGunner != 1) && ((u16) p->ammoSlot == 0xFF)) {
        return 0;
    }
    if (p->input.held2 & 4) {
        return 0;
    }
    if ((p->ammoMenuOpen == 0) && (p->savedAmmoType != p->ammoType) && (p->weaponOut != 0)) {
        return 1;
    }
    return 0;
}

s32 Player_BowgunAmmoSwitch(Player *p)
{

    if (Player_IsLocal(p) == 0) {
        return 0;
    }
    if (p->isGunner != 1) {
        return 0;
    }
    if (Player_IsState(p, 2, 0x13) != 0) {
        return 0;
    }
    if (p->stateGroup == 3 || p->stateGroup == 4) {
        return 0;
    }
    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    if (p->unk816 != 0) {
        return 0;
    }
    if (p->stateGroup == 1) {
        if (p->weaponType == 10) {
            switch (p->state) {
            case 0:
            case 14:
                if (p->step < 2) {
                    return 0;
                }
                break;
            case 3:
            case 4:
            case 13:
                return 0;
            }
        } else {
            switch (p->state) {
            case 0:
            case 4:
            case 5:
                if (p->step < 2) {
                    return 0;
                }
                break;
            case 8:
                if (p->step < 3) {
                    return 0;
                }
                break;
            }
        }
    }
    if (p->ammoMenuOpen == 0) {
        p->ammoMenuOpen = 1;
        p->savedAmmoType = p->ammoType;
        p->savedAmmoCount = p->ammoCount;
        p->savedClipSize = p->clipSize;
        p->savedRoundsLoaded = p->roundsLoaded;
        p->savedCoatingSlot = p->coatingSlot;
    }
    if (p->input.held2 & 4) {
        if (p->input.pressed2 & 0x40) {
            if (p->ammoSlot == 0xFF) {
                p->ammoSlot = Bowgun_NextAmmoSlot(p, 0, 3);
            } else {
                p->ammoSlot = Bowgun_NextAmmoSlot(p, p->ammoSlot, 1);
            }
            Bowgun_LoadAmmo(p, 0);
            if (p->ammoSlot != 0xFF) {
                p->pouchHudFlags |= 8;
                Sound_PlaySeFlat(0, 0x12, 0);
            }
        } else if (p->input.pressed2 & 0x100) {
            if (p->ammoSlot == 0xFF) {
                p->ammoSlot = Bowgun_NextAmmoSlot(p, 0, 2);
            } else {
                p->ammoSlot = Bowgun_NextAmmoSlot(p, p->ammoSlot, 0);
            }
            Bowgun_LoadAmmo(p, 0);
            if (p->ammoSlot != 0xFF) {
                p->pouchHudFlags |= 0x10;
                Sound_PlaySeFlat(0, 0x12, 0);
            }
        }
    } else if (p->ammoMenuOpen != 0) {
        p->ammoMenuOpen = 0;
        if (p->savedAmmoType != p->ammoType) {
            Bowgun_LoadAmmo(p, 0);
        } else {
            p->ammoCount = p->savedAmmoCount;
            p->clipSize = p->savedClipSize;
            p->roundsLoaded = p->savedRoundsLoaded;
        }
    }
    if (p->savedAmmoType == p->ammoType) {
        p->ammoCount = p->savedAmmoCount;
        p->clipSize = p->savedClipSize;
        p->roundsLoaded = p->savedRoundsLoaded;
        p->coatingSlot = p->savedCoatingSlot;
    }
    return 0;
}

u8 Player_PushNotice3(s32 arg0)
{
    return Notice_Push(1, 3, arg0);
}

static s32 Player_PickStatusState(Player *p)
{
    if (p->status2Buildup > 0) {
        return 1;
    }
    if (p->status4Buildup > 0) {
        return 2;
    }
    return 0;
}

s32 Player_GetStickYaw(Player *p, s32 i)
{
    s32 yaw = Camera_GetYaw() & 0xFFFF;

    return ((&p->input.stickAngle)[i] + yaw) & 0xFFFF;
}

void Player_FaceStick(Player *p)
{
    if (CamCtl_IsTurningBehind() != 1) {
        *(s16 *)&p->targetYaw = (s16) (p->input.stickAngle + (Camera_GetYaw() & 0xFFFF));
    }
}

u8 Player_StickMoveLevel(Player *p, s32 unused)
{
    u8 lvl = 0;
    u16 v;

    switch (p->weaponType) {
    case 1:
    case 5:
    case 10:
        if (p->weaponOut != 0 && p->aimMode == 1) {
            return 0;
        }
        break;
    case 0:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
    case 8:
    case 9:
        break;
    }
    v = p->input.stickTilt;
    if (v >= 120) {
        lvl = 3;
    } else if (v >= 85) {
        lvl = 1;
    } else if (v >= 40) {
        lvl = 1;
    }
    if (p->action == 0 && p->weaponOut == 0 && (p->input.held & 0x10) && lvl != 0
        && (Player_IsState(p, 0, 0x24) == 0 || p->dashDelay == 0)) {
        return 5;
    }
    return lvl;
}

void Player_UpdateInBattle(Player *p)
{
    s16 i;
    u8 bit;
    Em *m;
    s32 t;

    bit = 1 << p->index;
    p->prevInBattle = p->inBattle;
    p->inBattle = 0;
    for (i = 0, m = gEms; i < 20; i++, m++) {
        if (m->area == p->area && m->active != 0 && m->visible != 0 && m->startsBattle != 0 && m->stateGroup != 5
            && (m->noticed & bit) && m->combat == 1) {
            m->combatPlayers |= bit;
            p->inBattle = 1;
            p->target = m;
            p->targetAngle = (u16) (Vec_Heading(&m->posX, p->pos) + 0x4000);
            t = m->type;
            if (t == 0x1B || t == 0x1C || t == 0x1F) {
                p->unk73A_combat = 1;
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
s16 Player_CheckRideSlip(Player *p)
{
    Em *m;
    s16 i;
    f32 len;
    f32 lim;

    for (m = gEms, i = 0; i < 20; i++, m++) {
        if (p->rideEm == m->slotNo && m->type != 0x37) {
            return 0;
        }
    }
    len = Vec_Length(p->rideDelta);
    if (p->stateGroup == 0 && p->state == 0xC) {
        lim = 10.0f;
    } else if (p->stateGroup == 0 && p->state == 0x16) {
        lim = 4.0f;
    } else {
        lim = 7.0f;
    }
    if (!(len < lim)) {
        if (p->stateGroup == 0 && p->state == 0x16) {
            return 1;
        }
        Player_SetState(p, 0, 0x16, 0);
        return 1;
    }
    return 0;
}

void Player_UseDashStamina(Player *p)
{
    if (Player_HasSkill(p, 0xB6) == 1) {
        Player_AddStamina(p, -8);
        return;
    }
    if (Player_HasSkill(p, 0xB7) == 1) {
        Player_AddStamina(p, -0x12);
        return;
    }
    Player_AddStamina(p, -0xF);
}

s32 Player_TrySideStep(Player *p, s16 noTurn)
{
    u16 s = p->input.stickDir;

    if (s & 0x2000) {
        if (p->input.held & 0x10) {
            if (Player_IsState(p, 0, 0x77) == 0) {
                Player_RequestState(p, 0, 0x77, 0xC);
                if (noTurn == 0) {
                    Player_UseDashStamina(p);
                }
            }
        } else if (Player_IsState(p, 0, 0x28) == 0) {
            Player_RequestState(p, 0, 0x28, 0xC);
        }
        return 1;
    }
    if (s & 0x1000) {
        if (p->input.held & 0x10) {
            if (Player_IsState(p, 0, 0x78) == 0) {
                Player_RequestState(p, 0, 0x78, 0xC);
                if (noTurn == 0) {
                    Player_UseDashStamina(p);
                }
            }
        } else if (Player_IsState(p, 0, 0x29) == 0) {
            Player_RequestState(p, 0, 0x29, 0xC);
        }
        return 1;
    }
    if (p->input.pressed & 0x40) {
        Player_RequestState(p, 0, 0x30, 0);
        return 1;
    }
    return 0;
}

void Player_StartSpotAction(Player *p, s32 mode)
{
    s8 kind;
    u16 angle;
    f32 pos[3];
    f32 d[3];
    f32 v[3];
    s32 ang[3];
    f32 m[16];

    switch (Player_FindActionSpot((Player *)p, (f32 *)pos, &angle, (u8 *)&kind)) {
    case 0:
    case 2:
    case 6:
    case 8:
    default:
        if ((u8) Player_NearEmKind10(p)) {
            Player_RequestState(p, 0, 0x5B, 0);
        }
        break;
    case 1:
        Vec_Copy(p->moveTo, pos);
        Player_StartMoveTo(p, 0x14);
        p->targetYaw = angle;
        Player_RequestState(p, 0, 0x4C, 0);
        break;
    case 3:
        if (p->itemSelect == 0) {
            Player_RequestState(p, 0, 0x57, 0);
        }
        break;
    case 4:
        p->targetYaw = angle;
        Player_RequestState(p, 0, 0x5C, 0);
        break;
    case 5:
        if (mode == 0) {
            Player_RequestState(p, 0, 0xC, 0);
            break;
        }
        Vec_Copy(p->moveTo, pos);
        ang[0] = 0;
        ang[1] = angle;
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, m);
        Vec_Set(v, 120.0f * gSpotLineupOffsets[p->index], 0.0f, 0.0f);
        Vec_Rotate(d, v, m);
        p->moveTo[0] += d[0];
        p->moveTo[1] += d[1];
        p->moveTo[2] += d[2];
        p->targetYaw = angle;
        Player_StartMoveTo(p, 0x14);
        Player_RequestState(p, 0, 0x33, 0);
        break;
    case 9:
        if (gScene.countdownOn == 0) {
            p->targetYaw = Vec_Heading(&p->pos[0], pos) - 0x4000;
            Player_RequestState(p, 0, 0x56, 0);
        }
        break;
    case 7:
        if (Quest_PhaseIsOver(0) != 1 && p->itemSelect == 0) {
            Player_RequestState(p, 0, 0x7B, 0);
        }
        break;
    }
}

void Player_TryDrawWeapon(Player *p, u8 mode)
{

    if (p->input.held & 0x80) {
        if (mode == 0 || mode == 2) {
            switch (p->weaponType) {
            case 0:
                if (mode == 2 && (p->carry & 0xF)) {
                    p->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(p, 4);
                }
                Player_RequestState(p, 2, 3, 0);
                break;
            case 3:
            case 4:
            case 9:
                if (mode == 2 && (p->carry & 0xF)) {
                    p->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(p, 4);
                }
                Player_RequestState(p, 2, 0x17, 0);
                break;
            case 6:
                if (mode == 2 && (p->carry & 0xF)) {
                    p->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(p, 4);
                }
                if (p->demonMode == 0) {
                    DualSwords_Draw(p, 0);
                }
                break;
            case 2:
                if (mode == 2 && (p->carry & 0xF)) {
                    p->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(p, 4);
                }
                Hammer_Draw(p);
                break;
            case 8:
                if (mode == 2 && (p->carry & 0xF)) {
                    p->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(p, 4);
                }
                HuntingHorn_Draw(p, 1);
                break;
            case 7:
                if (mode == 2 && (p->carry & 0xF)) {
                    p->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(p, 4);
                }
                LongSword_Draw(p);
                break;
            case 10:
                if (mode == 2 && (p->carry & 0xF)) {
                    p->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(p, 4);
                }
                Bow_Draw(p);
                break;
            }
        } else {
            switch (p->weaponType) {
            case 0:
            case 3:
            case 4:
            case 9:
                Player_RequestState(p, 2, 3, 0);
                break;
            case 2:
                Hammer_Draw(p);
                break;
            case 8:
                HuntingHorn_Draw(p, 0);
                break;
            case 6:
                if (p->demonMode == 0) {
                    DualSwords_Draw(p, 0);
                } else {
                    DualSwords_Draw(p, 1);
                }
                break;
            }
        }
    }
}

void Player_TryGather(Player *p, s32 a)
{
    f32 pos[3];
    u16 kind;
    u16 id;
    u16 r;

    if ((u8) Player_NearEmKind10(p) && (u8) a != 2) {
        return;
    }
    if (Player_GetGatherPoint(p, &kind, pos) == 0xFFFF) {
        goto other;
    }
    switch (kind) {
    case 0:
        if ((u8) a == 2) {
            goto other;
        }
        if ((u8) a == 0) {
            Player_SetState(p, 0, 0x47, 0);
        } else {
            Player_SetState(p, 0, 0x5D, 0);
        }
        break;
    case 3:
        if ((u8) a == 2) {
            switch (p->pouch[p->pouchCursor].item) {
            case 150:
            case 151:
            case 152:
                p->targetYaw = Vec_Heading(&p->pos[0], pos) - 0x4000;
                p->useItem = p->pouch[p->pouchCursor].item;
                Player_SetState(p, 0, 0x51, 0);
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
            id = p->pouch[p->pouchCursor].item;
            switch (id) {
            case 153:
            case 154:
            case 155:
                p->useItem = id;
                Player_SetState(p, 0, 0x52, 0);
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
        switch (p->pouch[p->pouchCursor].item) {
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
        r = MinePoint_FindNearPlayer(p);
        if (r != 0xFFFF) {
            if ((u8) a == 0) {
                switch (r) {
                default:
                    Player_SetState(p, 0, 0x7C, 0);
                    break;
                case 5:
                    Player_SetState(p, 0, 0x75, 0);
                    break;
                case 2:
                    Player_SetState(p, 0, 0x4A, 0);
                    break;
                }
            } else {
                switch (r) {
                default:
                    Player_SetState(p, 0, 0x7D, 0);
                    break;
                case 5:
                    Player_SetState(p, 0, 0x76, 0);
                    break;
                case 2:
                    Player_SetState(p, 0, 0x60, 0);
                    break;
                }
            }
        }
    }
}

s32 Player_IdleInput(Player *p)
{

    if (Player_IsLocal(p) == 0 && Game_IsOnline() == 1) {
        return 0;
    }
    switch (gScene.questPhase) {
    case 4:
        if (p->unk7E5 == 0) {
            Player_RequestState(p, 4, 2, 0);
            return 0;
        }
        break;
    case 6:
    case 5:
        Player_RequestState(p, 4, 3, 0);
        return 0;
    case 7:
        Player_RequestState(p, 4, 6, 0);
        return 0;
    case 8:
        Player_RequestState(p, 4, 5, 0);
        return 0;
    }
    p->freeState = 1;
    switch ((u8) Player_StickMoveLevel(p, 0)) {
    case 1:
    case 2:
        Player_FaceStick(p);
        if (Player_IsState(p, 0, 0x3B) == 0) {
            if (Player_IsState(p, 0, 0x3D) != 0) {
                Player_RequestState(p, 0, 0x3C, 0);
            } else {
                Player_SetState(p, 0, 0x3B, 0);
            }
        }
        break;
    case 3:
        Player_FaceStick(p);
        if (Player_IsState(p, 0, 0x3D) == 0) {
            Player_SetState(p, 0, 0x3D, 0);
        }
        break;
    case 0:
    default:
        if ((Player_IsState(p, 0, 0x3B) != 0 || Player_IsState(p, 0, 0x3D) != 0) && p->timer == 0) {
            Player_RequestState(p, 0, 0x3C, 0);
        }
        break;
    }
    if ((p->input.pressed & 0x40) && p->unk668 == 0) {
        Player_RequestState(p, 0, 0x39, 0);
    }
    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    if (p->actRequest != 0) {
        Player_StartAct(p);
    }
    if (p->unk668 == 0) {
        if (p->input.stickDirPressed & 0x3C) {
            Player_SetState(p, 0, 4, 0);
        }
        Player_TryDrawWeapon(p, 0);
    }
    if ((p->input.pressed & 0x200) && p->itemUseDelay == 0 && p->itemSelect == 0) {
        Player_TryGather(p, 2);
        Player_UseItem(p, 0);
        p->pouchHudFlags |= 1;
    }
    if (p->input.pressed & 0x20) {
        if (Player_TakeOfferedItem(p) == 0) {
            Player_StartSpotAction(p, 0);
            Player_TryGather(p, 1);
        }
    } else if (p->offerRequest != 0) {
        p->freeState = 0;
        p->offerStarted = 1;
        Player_RequestState(p, 0, 0x67, 0x10);
    }
    return 0;
}

/* The weapons input checks (the states cancel into another action), for the local player or a human one
 * when input is allowed. */
void Player_WeaponAttackInput(Player *p, s32 arg)
{

    if ((Player_IsLocal(p) != 0 || Game_IsOnline() != 1) && p->itemSelect == 0 && Quest_PhaseIsOver(1) != 1) {
        switch (p->weaponType) {
        case 0:
            GreatSword_AttackInput(p);
            break;
        case 7:
            LongSword_AttackInput(p);
            break;
        case 1:
        case 5:
            Bowgun_AttackInput(p);
            break;
        case 10:
            Bow_AttackInput(p, arg);
            break;
        case 2:
            Hammer_AttackInput(p);
            break;
        case 8:
            HuntingHorn_AttackInput(p);
            break;
        case 3:
            Lance_AttackInput(p);
            break;
        case 9:
            Gunlance_AttackInput(p);
            break;
        case 4:
            SwordShield_AttackInput(p);
            break;
        case 6:
            DualSwords_AttackInput(p);
            return;
        }
    }
}

void Player_GatherStep(Player *p)
{
    u8 temp_a0;

    if (((Player_IsLocal(p) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1)) {
        temp_a0 = p->weaponType;
        switch (temp_a0) {
        case 3:
            if ((Player_StickMoveLevel(p, 0) & 0xFF) && (p->motionId[0] != 0x3F3)) {
                Player_SetState(p, 0U, 7U, 4);
            }
            if (p->input.pressed & 1) {
                Lance_GatherAttack(p, 0);
            }
            if (p->input.stickDirPressed & 0x3C) {
                Lance_GatherAttack(p, 1);
            }
            break;
        case 9:
            if ((Gunlance_GatherInput(p, 0) == 0) && (Player_StickMoveLevel(p, 0) & 0xFF) && (p->motionId[0] != 0x3F3)) {
                Player_SetState(p, 0U, 7U, 4);
            }
            break;
        case 4:
            if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B)) {
                Player_SetState(p, 0U, 0x1CU, 4);
            }
            if (p->input.stickDirPressed & 0x20) {
                SwordShield_GatherAttack(p);
            }
            break;
        case 0:
        case 7:
            if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B)) {
                Player_SetState(p, 0U, 0x1CU, 4);
            }
            break;
        }
    }
}

s8 Player_TurnAlongWall(Player *p, s16 side)
{
    s16 i;
    u16 flags;
    u16 ang;

    if ((u8)Obj_InCurrentArea(p) == 0) {
        return 0;
    }
    if (p->wallHitDirs != 0) {
        for (i = 0; i < 10; i++) {
            flags = gEmWallContacts[p->index][i].flags;
            if (flags == 0) {
                break;
            }
            ang = Math_Heading(gEmWallContacts[p->index][i].pos[0], gEmWallContacts[p->index][i].pos[2], 0.0f, 0.0f);
            if ((flags & 2) && (p->wallHitDirs & 0xE0000007)) {
                if (side == 0) {
                    p->rot[1] = (u16) (ang - 0x4000);
                } else {
                    p->rot[1] = (u16) (ang + 0x4000);
                }
                p->targetYaw = p->rot[1];
                return 1;
            }
            if (flags != 0) {
                if (side == 1) {
                    p->rot[1] = (u16) (ang + 0x4000);
                    p->targetYaw = p->rot[1];
                }
                return 2;
            }
        }
    }
    return 0;
}

s32 Player_TurnAlongWallSide(Player *p, s16 side)
{
    s16 i;
    u16 flags;
    u16 ang;

    if ((u8)Obj_InCurrentArea(p) == 0) {
        return 0;
    }
    if (p->wallHitDirs & (side == 0 ? 0xE0000007 : 0x3E000)) {
        for (i = 0; i < 10; i++) {
            flags = gEmWallContacts[p->index][i].flags;
            if (flags == 0) {
                break;
            }
            ang = Math_Heading(gEmWallContacts[p->index][i].pos[0], gEmWallContacts[p->index][i].pos[2], 0.0f, 0.0f);
            if ((flags & 2) && side == 0) {
                p->rot[1] = (u16) (ang - 0x4000);
                p->targetYaw = p->rot[1];
                return 1;
            }
            if (side == 1) {
                p->rot[1] = (u16) (ang + 0x4000);
                p->targetYaw = p->rot[1];
                return 2;
            }
        }
    }
    return 0;
}

u8 Player_CheckClimb(Player *p)
{
    f32 dist;
    u8 ret = 0;

    switch (Player_TurnAlongWall(p, 0)) {
    case 0:
        break;
    case 1:
        Player_SetState(p, 0, 0x2B, 0xC);
        ret = 1;
        break;
    }
    if (Player_FloorAheadAbove(p, &dist, 55.0f, 60.0f, 120.0f) != 0) {
        p->targetYaw = p->rot[1];
        Player_RequestState(p, 0, 0x1E, 0xC);
        ret = 1;
    } else if (Player_FloorAheadAbove(p, &dist, 55.0f, 180.0f, 30.0f) != 0) {
        p->targetYaw = p->rot[1];
        Player_RequestState(p, 0, 0x1B, 0xC);
        ret = 1;
    } else if (Player_FloorAheadAbove(p, &dist, 55.0f, 210.0f, 90.0f) != 0) {
        p->targetYaw = p->rot[1];
        Player_RequestState(p, 0, 0x15, 0xC);
        ret = 1;
    }
    return ret;
}

/* Uses the selected pouch item (see config/symbols/game.txt). */
void Player_UseItem(Player *p, u8 mode)
{
    f32 v[3];
    u16 ang;
    u8 k;

    if (Pouch_CursorCount(p) <= 0) {
        Sound_PlaySeFlat(0, 0x15, 0);
        if (mode == 1) {
            Player_BeginState(p, 0, 4, 0);
        }
        return;
    }
    if (mode == 2) {
        switch (p->pouch[p->pouchCursor].item) {
        case 0x61:
            if (func_3648F0() == 1) {
                p->useItem = p->pouch[p->pouchCursor].item;
                Player_RequestState(p, 0, 0x6F, 0xC);
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
    switch (p->pouch[p->pouchCursor].item) {
    case 0x4C:
    case 0x4B:
        p->useItem = p->pouch[p->pouchCursor].item;
        if (!Player_TestFlag(p, 0x2000600)) {
            Player_RequestState(p, 0, 0xB, 0xC);
        } else {
            Player_RequestState(p, 0, 0xE, 0xC);
        }
        break;
    case 0x4D:
    case 0x2BA:
    case 0x4F:
        p->useItem = p->pouch[p->pouchCursor].item;
        Player_RequestState(p, 0, 0xB, 0xC);
        break;
    case 0x4E:
        if (Item_UsableOutsideCamp() == 1) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x72, 0xC);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
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
        p->useItem = p->pouch[p->pouchCursor].item;
        Player_RequestState(p, 0, 0xE, 0xC);
        break;
    case 0x50:
    case 0x45:
        if (Quest_IsVersus() == 0) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0xE, 0xC);
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
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    case 0x56:
        if (func_364A70() == 1) {
            goto drink;
        }
        Sound_PlaySeFlat(0, 0x15, 0);
        if (mode == 1) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    case 0x28:
    case 0x2C:
    case 0x2E:
    case 0x2D:
        if (Scene_FewPlaced3_Unk() == 1) {
        drink:
            if (p->action == 0) {
                p->useItem = p->pouch[p->pouchCursor].item;
                Player_RequestState(p, 0, 0x19, 0xC);
            } else if (p->action == 1) {
                p->useItem = p->pouch[p->pouchCursor].item;
                Player_RequestState(p, 0, 0x59, 0xC);
            }
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
            }
        }
        break;
    case 0x5D:
    case 0x5E:
    case 0x29A:
        if (Item_CanPlaceLargeBarrelBomb() == 1) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x6F, 0xC);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
            }
        }
        break;
    case 0x62:
        if (Item_CanPlaceKind1() == 1) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x6F, 0xC);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
            }
        }
        break;
    case 0x55:
    case 0x296:
        if (Item_CanPlaceKind2() == 1) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x2E, 0xC);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
            }
        }
        break;
    case 0x54:
    case 0x299:
        if (Player_GetAreaAmbModel(p) >= 0) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x2E, 0xC);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
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
        if (SpecialMine_IsFree(p) == 1) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x2E, 0xC);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
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
        if (Player_FindActionSpot((Player *)p, (f32 *)v, &ang, (u8 *)&k) == 2) {
            p->useItem = p->pouch[p->pouchCursor].item;
            p->targetYaw = ang;
            Player_RequestState(p, 0, 0x4F, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
            }
        }
        break;
    case 0x2A6:
        if (Player_FindActionSpot((Player *)p, (f32 *)v, &ang, (u8 *)&k) == 0xB) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x2E, 0xC);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
            }
        }
        break;
    case 0x29B:
        if (Player_FindActionSpot((Player *)p, (f32 *)v, &ang, (u8 *)&k) == 6) {
            p->moveTo[0] = v[0];
            p->moveTo[1] = v[1];
            p->moveTo[2] = v[2];
            p->targetYaw = ang;
            Player_StartMoveTo(p, 0x14);
            p->stateRand = k;
            Player_RequestState(p, 0, 0x36, 0x20);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
            }
        }
        break;
    case 0x2B7:
        if (Player_FindActionSpot((Player *)p, (f32 *)v, &ang, (u8 *)&k) == 0xA) {
            p->targetYaw = Vec_Heading(p->pos, v) - 0x4000;
            p->stateRand = k;
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x79, 0x20);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
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
        if (func_00723BC0((Em *)p) == 0) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x58, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
            }
        }
        break;
    case 0x294:
    case 0x9C:
    case 0x9D:
    case 0x9F:
    case 0x9E:
    case 0xA0:
        if (Pouch_HasItem28(p) == 1) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_SetState(p, 0, 0x4C, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
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
        p->useItem = p->pouch[p->pouchCursor].item;
        Player_SetState(p, 0, 0x4C, 0);
        break;
    case 0xAC:
    case 0x295:
    case 0xEA:
    case 0xF2:
        if (p->isGunner != 1) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x61, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
            }
        }
        break;
    case 0xAF:
        p->useItem = p->pouch[p->pouchCursor].item;
        Player_SetState(p, 0, 0x5E, 0xC);
        break;
    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
        p->useItem = p->pouch[p->pouchCursor].item;
        Player_SetState(p, 0, 0x62, 0xC);
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
        if (func_00723BC0((Em *)p) == 0) {
            p->useItem = p->pouch[p->pouchCursor].item;
            Player_RequestState(p, 0, 0x5A, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
            }
        }
        break;
    case 0xAD:
        if (Player_LocalAreaOk_Unk() == 1) {
            if (p->action == 1) {
                Player_RequestState(p, 0, 0x66, 0);
            } else {
                Player_RequestState(p, 0, 0x65, 0);
            }
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(p, 0, 4, 0);
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
        p->useItem = p->pouch[p->pouchCursor].item;
        Player_RequestState(p, 0, 0x73, 0);
        break;
    default:
        Sound_PlaySeFlat(0, 0x15, 0);
        if (mode == 1) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

extern s32 gPouchFullTakeText;
s32 Player_TakeOfferedItem(Player *p)
{
    s16 i;
    Player *o;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (p->offerNoticeTimer != 0) {
        return 0;
    }
    o = gPlayers;
    for (i = 0; i < gScene.playerCount; i++, o++) {
        if (i != p->index && o->active != 0 && o->area == p->area && Player_IsState(o, 0, 0x67) != 0
            && o->offerTo == p->index && Vec_Distance(p->pos, o->pos) <= 300.0f && gItemData[o->offerItem].rarity < 3) {
            if (Pouch_Count(p, o->offerItem) == 0) {
                if (Pouch_HasFreeSlot(p) != 0) {
                    p->offerItem = o->offerItem;
                    p->offerCount = o->offerCount;
                    if (Player_IsLocal(p) == 1) {
                        QSync_SendPlayer(p, 7, o->index);
                    }
                    Player_RequestState(p, 0, 0x68, 0);
                    return 1;
                }
                Notice_PushItem((char *)gPouchFullTakeText);
                p->offerNoticeTimer = 0x5A;
            } else if (Pouch_Room(p, o->offerItem) >= o->offerCount) {
                p->offerItem = o->offerItem;
                p->offerCount = o->offerCount;
                if (Player_IsLocal(p) == 1) {
                    QSync_SendPlayer(p, 7, o->index);
                }
                Player_RequestState(p, 0, 0x68, 0);
                return 1;
            } else {
                Notice_Push(1, 0xF, o->offerItem);
                p->offerNoticeTimer = 0x5A;
            }
            return 0;
        }
    }
    return 0;
}

void Player_IdleActions(Player *p, s32 arg)
{
    Em *m = gEms;
    s16 i;
    u8 k;
    u8 r;
    u8 drew;
    u16 d;

    if (Game_IsOnline() == 0) {
        if (Player_IsLocal(p) == 0) {
            return;
        }
    } else if (Player_IsLocal(p) == 0) {
        return;
    }
    drew = 0;
    if (gScene.questPhase == 4 && p->unk7E5 == 0) {
        Player_RequestState(p, 4, 2, 0);
        return;
    }
    p->freeState = 1;
    if (p->rideState != 0) {
        k = p->rideEm;
        for (i = 0; i < 20; i++, m++) {
            if (k == m->slotNo && (m->type == 7 || m->type == 0x32 || m->type == 0x3A || m->type == 0x48 ||
                                   m->type == 0x37)) {
                if (m->quakeTimer != 0) {
                    Player_SetState(p, 2, 7, 0);
                    return;
                }
                break;
            }
        }
        if ((u8) Player_StickMoveLevel(p, 1) > 0 && p->state != 0x12) {
            Player_FaceStick(p);
            Player_SetState(p, 0, 0x12, 0);
        }
        Player_CheckRideSlip(p);
        if (Quest_PhaseIsOver(1) == 1) {
            return;
        }
        if (p->input.pressed & 0x20) {
            if (Player_TakeOfferedItem(p) == 0) {
                Player_StartSpotAction(p, 0);
                Player_TryGather(p, 0);
            }
        }
        if ((p->input.pressed & 0x200) && p->itemUseDelay == 0 && p->itemSelect == 0) {
            Player_UseItem(p, 2);
            p->pouchHudFlags |= 1;
        }
    } else {
        if (gScene.questPhase == 6 || gScene.questPhase == 5) {
            Player_RequestState(p, 4, 3, 0);
            return;
        }
        if (gScene.questPhase == 7) {
            Player_RequestState(p, 4, 6, 0);
            return;
        }
        if (gScene.questPhase == 8) {
            Player_RequestState(p, 4, 5, 0);
            return;
        }
        if (p->weaponOut != 0) {
            r = Player_StickMoveLevel(p, 1);
            switch (r) {
            case 0:
                break;
            case 1:
            case 2:
            case 3:
                if (p->state != 0x24 && p->state != 3 && p->state != 0x74 && p->unk668 == 0) {
                    Player_SetState(p, 0, 3, 0);
                }
                break;
            case 4:
                break;
            }
            if (r != 0 && p->unk668 != 0 && p->state != 0x3B) {
                Player_SetState(p, 0, 0x3B, 0);
            }
            if (Quest_PhaseIsOver(1) == 1) {
                return;
            }
            if (p->actRequest != 0) {
                Player_StartAct(p);
            }
            switch (p->weaponType) {
            case 4:
                if ((p->input.pressed & 0x20) || ((p->input.pressed & 0x10) && p->input.stickTilt >= 0x28)) {
                    if (Player_TestFlag(p, 0x1200) == 0) {
                        Player_SetState(p, 0, 5, 0);
                    } else {
                        Player_SetState(p, 0, 0xA, 0);
                    }
                } else if ((p->input.pressed & 0x200) && p->itemUseDelay == 0 && p->itemSelect == 0) {
                    Player_TryGather(p, 2);
                    Player_UseItem(p, 0);
                    p->pouchHudFlags |= 1;
                }
                break;
            default:
                if ((p->input.pressed & 0x220) || ((p->input.pressed & 0x10) && p->input.stickTilt >= 0x28)) {
                    if (Player_TestFlag(p, 0x1200) == 0) {
                        Player_SetState(p, 0, 5, 0);
                    } else {
                        Player_SetState(p, 0, 0xA, 0);
                    }
                    p->pouchHudFlags |= 1;
                }
                break;
            }
            Player_TryDrawWeapon(p, 1);
            if ((Player_AmmoTypeChanged(p) & 0xFF) == 0) {
                Player_WeaponAttackInput(p, arg);
            }
            if (p->offerRequest != 0) {
                p->freeState = 0;
                p->offerStarted = 1;
                Player_RequestState(p, 0, 0x67, 0x10);
            }
            if (Player_Status5Full(p) & 0xFF) {
                p->freeState = 0;
                Player_RequestState(p, 2, 0x13, 0);
                return;
            }
            if (Player_HasMashBuildup(p) & 0xFF) {
                p->freeState = 0;
                Player_RequestState(p, 2, 0x1B, 0);
            }
        } else {
            r = Player_StickMoveLevel(p, 0);
            switch (r) {
            case 0:
                break;
            case 1:
                if (p->state != 0x49 && p->state != 0x24 && p->unk668 == 0) {
                    Player_FaceStick(p);
                    Player_SetState(p, 0, 0x49, 0);
                }
                break;
            case 2:
                if (p->state != 2 && p->state != 0x24 && p->unk668 == 0) {
                    Player_FaceStick(p);
                    Player_SetState(p, 0, 2, 0);
                }
                break;
            case 3:
                if (p->state != 1 && p->state != 0x3F && p->state != 0x13 && p->state != 0x24 &&
                    p->state != 0x1F && p->unk668 == 0) {
                    Player_FaceStick(p);
                    d = p->targetYaw + 0x10000 - p->rot[1];
                    if (d >= 0x6000U && d < 0xA001U) {
                        Player_SetState(p, 0, 0x3F, 0xC);
                    } else {
                        Player_SetState(p, 0, 1, 0);
                    }
                }
                break;
            case 4:
                break;
            case 5:
                if (Player_TestFlag(p, 0x200) == 0) {
                    if (p->inBattle != 0 && (u16) (p->targetAngle - p->rot[1] + 0x2000) > 0x4000) {
                        Player_SetState(p, 0, 0x20, 0);
                    } else {
                        Player_SetState(p, 0, 0x1F, 0);
                    }
                    drew = 1;
                }
                break;
            }
            if (p->actRequest != 0) {
                Player_StartAct(p);
            }
            if (r != 0 && p->unk668 != 0 && p->state != 0x3B) {
                Player_SetState(p, 0, 0x3B, 0);
            }
            if (Quest_PhaseIsOver(1) == 1) {
                return;
            }
            if (p->input.pressed & 0x40) {
                if (p->input.stickTilt >= 0x55) {
                    if (p->stamina >= 0x4B) {
                        Player_SetState(p, 0, 0x1C, 4);
                    }
                } else {
                    Player_SetState(p, 0, 8, 0);
                }
            }
            if ((p->input.pressed & 0x200) && p->itemUseDelay == 0 && p->itemSelect == 0) {
                Player_TryGather(p, 2);
                Player_UseItem(p, 0);
                p->pouchHudFlags |= 1;
            }
            Player_TryDrawWeapon(p, 0);
            if ((drew == 0 ? p->input.pressed : p->input.held) & 0x20) {
                Player_CheckClimb(p);
            }
            if (p->input.stickDirPressed & 0x3C) {
                if (Player_TestFlag(p, 0x2000600) == 0) {
                    if (p->weaponType == 1 || p->weaponType == 5) {
                        Bowgun_DrawAttack(p, 0);
                    } else {
                        Player_SetState(p, 0, 4, 0);
                    }
                } else {
                    switch (p->weaponType) {
                    case 0:
                    default:
                        GreatSword_DrawAttack(p);
                        break;
                    case 1:
                    case 5:
                        Bowgun_DrawAttack(p, 1);
                        break;
                    case 10:
                        Bow_DrawAttack(p);
                        break;
                    case 4:
                        SwordShield_DrawAttack(p);
                        break;
                    case 6:
                        DualSwords_DrawAttack(p);
                        break;
                    case 3:
                        Lance_DrawAttack(p);
                        break;
                    case 9:
                        Gunlance_DrawAttack(p);
                        break;
                    case 2:
                        Hammer_DrawAttack(p);
                        break;
                    case 8:
                        HuntingHorn_DrawAttack(p);
                        break;
                    case 7:
                        LongSword_DrawAttack(p);
                        break;
                    }
                }
            }
            if (p->input.pressed & 1) {
                Player_SetState(p, 1, 0x63, 4);
            }
            if (p->input.pressed & 0x20) {
                if (Player_TakeOfferedItem(p) == 0) {
                    Player_StartSpotAction(p, 0);
                    Player_TryGather(p, 0);
                }
            } else if (p->offerRequest != 0) {
                p->freeState = 0;
                p->offerStarted = 1;
                Player_RequestState(p, 0, 0x67, 0x10);
            }
            if ((Player_EnteredBattle((Em *) p) & 0xFF) == 1) {
                p->freeState = 0;
                Player_SetState(p, 0, 0x3E, 0);
                CamCtl_SetTargetObj((u8 *) p);
            }
            if (Player_Status5Full(p) & 0xFF) {
                p->freeState = 0;
                Player_RequestState(p, 2, 0x13, 0);
                return;
            }
            if (Player_HasMashBuildup(p) & 0xFF) {
                p->freeState = 0;
                Player_RequestState(p, 2, 0x1B, 0);
            }
        }
    }
}

void Player_BowgunAim(Player *p)
{
    s8 s;
    s8 a;
    u16 da;

    if (Player_IsLocal(p) == 0) {
        return;
    }
    if (p->isGunner != 1) {
        return;
    }
    if (p->motionId[0] != 0x3E9) {
        return;
    }
    if (p->aimLockTimer != 0 || p->aimMode != 0) {
        p->aimLockTimer = 10;
    }
    Player_SetAimLayers(p, 0x3ED, 0x3EE);
    if (p->aimLockTimer == 0 && p->aimMode == 0) {
        if (p->input.held & 4) {
            return;
        }
        if (!(p->input.held & 8)) {
            return;
        }
    }
    if (p->input.held & 0x3000) {
        a = 5;
        if (p->aimMode != 0 && gScene.aimReverse != 0) {
            a *= -1;
        }
        if (p->input.held & 0x2000) {
            Bowgun_AddAimWeight(p, a);
        } else {
            Bowgun_AddAimWeight(p, (s8) -a);
        }
        p->aimLockTimer = 5;
    }
    if (p->input.held & 0x800) {
        p->aimLockTimer = 10;
        if (p->aimMode != 0 && gScene.aimReverse == 2) {
            p->targetYaw -= 0x240;
        } else {
            p->targetYaw += 0x240;
        }
    }
    if (p->input.held & 0x400) {
        p->aimLockTimer = 10;
        if (p->aimMode != 0 && gScene.aimReverse == 2) {
            p->targetYaw += 0x240;
        } else {
            p->targetYaw -= 0x240;
        }
    }
    Player_SetAimLayers(p, 0x3ED, 0x3EE);
    if (p->aimLockTimer != 0 || p->aimMode != 0) {
        if (!(p->input.stickDir & 0x3C00) || p->input.stickTilt < 0x28) {
            return;
        }
        s = 1;
        da = 0x4C;
        if (p->input.stickTilt >= 0x78) {
            s = 8;
            da = 0x200;
        } else if (p->input.stickTilt >= 0x5A) {
            s = 4;
            da = 0xC0;
        } else if (p->input.stickTilt >= 0x3C) {
            s = 2;
            da = 0x80;
        }
        if (p->input.stickDir & 0x3000) {
            if (gScene.aimReverse != 0) {
                s = -s;
            }
            s = (s8) s;
            if (p->input.stickDir & 0x2000) {
                Bowgun_AddAimWeight(p, s);
            } else {
                Bowgun_AddAimWeight(p, (s8) -s);
            }
            p->aimLockTimer = 5;
            Player_SetAimLayers(p, 0x3ED, 0x3EE);
        }
        if (p->input.stickDir & 0x800) {
            p->aimLockTimer = 10;
            if (gScene.aimReverse == 2) {
                p->targetYaw -= da;
            } else {
                p->targetYaw += da;
            }
        }
        if (p->input.stickDir & 0x400) {
            p->aimLockTimer = 10;
            if (gScene.aimReverse == 2) {
                p->targetYaw += da;
            } else {
                p->targetYaw -= da;
            }
        }
    }
    Player_SetAimLayers(p, 0x3ED, 0x3EE);
}

static void Player_AddAimPitch(Player *p, s16 d)
{
    p->aimPitch += d;
    if (d >= 0) {
        if (p->aimPitch >= 0x2000) {
            p->aimPitch = 0x2000;
        }
    } else {
        if (p->aimPitch <= -0x2000) {
            p->aimPitch = -0x2000;
        }
    }
}

static void Player_AddAimPitchPct(Player *p, s8 d)
{
    f32 f;
    s8 v;

    p->aimPitchPct += d;
    if (p->aimPitchPct > 100) {
        p->aimPitchPct = 100;
    }
    if (p->aimPitchPct < -100) {
        p->aimPitchPct = -100;
    }
    v = p->aimPitchPct;
    f = v / 100.0f;
    if (v >= 0) {
        p->aimPitch = 0.5f + (65536.0f * (45.0f * f)) / 360.0f;
        return;
    }
    p->aimPitch = 0.5f + (65536.0f * (35.0f * f)) / 360.0f;
}

void Player_UpdateLean(Player *p, s32 mot)
{
    s8 s;
    s8 a;
    u16 da;
    u16 d;

    if (Player_IsLocal(p) == 0) {
        return;
    }
    if (p->motionId[0] != (u16) mot) {
        return;
    }
    if (p->aimLockTimer != 0 || p->aimMode != 0) {
        p->aimLockTimer = 10;
    }
    if (p->aimLockTimer == 0 && p->aimMode == 0) {
        if (p->input.held & 4) {
            return;
        }
        if (!(p->input.held & 8)) {
            return;
        }
    }
    if (p->input.held & 0x3000) {
        a = 4;
        if (p->aimMode != 0 && gScene.aimReverse != 0) {
            a *= -1;
        }
        if (p->input.held & 0x2000) {
            if ((u16) mot != 0x19E) {
                Player_AddAimPitch(p, a << 8);
            } else {
                Player_AddAimPitchPct(p, a);
            }
        } else {
            if ((u16) mot != 0x19E) {
                Player_AddAimPitch(p, -a << 8);
            } else {
                Player_AddAimPitchPct(p, (s8) -a);
            }
        }
        p->aimLockTimer = 5;
    }
    if (p->input.held & 0x800) {
        p->aimLockTimer = 10;
        if (gScene.aimReverse == 2) {
            p->targetYaw -= 0x180;
        } else {
            p->targetYaw += 0x180;
        }
    }
    if (p->input.held & 0x400) {
        p->aimLockTimer = 10;
        if (gScene.aimReverse == 2) {
            p->targetYaw += 0x180;
        } else {
            p->targetYaw -= 0x180;
        }
    }
    if (p->aimLockTimer != 0 || p->aimMode != 0) {
        if ((p->input.stickDir & 0x3C00) && p->input.stickTilt >= 0x28) {
            s = 1;
            da = 0x4C;
            if (p->input.stickTilt >= 0x78) {
                s = 8;
                da = 0x200;
            } else if (p->input.stickTilt >= 0x5A) {
                s = 4;
                da = 0xC0;
            } else if (p->input.stickTilt >= 0x3C) {
                s = 2;
                da = 0x80;
            }
            if (p->input.stickDir & 0x3000) {
                if (gScene.aimReverse != 0) {
                    s = -s;
                }
                s = (s8) s;
                if (p->input.stickDir & 0x2000) {
                    if ((u16) mot != 0x19E) {
                        Player_AddAimPitch(p, s << 8);
                    } else {
                        Player_AddAimPitchPct(p, s);
                    }
                } else {
                    if ((u16) mot != 0x19E) {
                        Player_AddAimPitch(p, -s << 8);
                    } else {
                        Player_AddAimPitchPct(p, (s8) -s);
                    }
                }
                p->aimLockTimer = 5;
            }
            if (p->input.stickDir & 0x800) {
                p->aimLockTimer = 10;
                if (gScene.aimReverse == 2) {
                    p->targetYaw -= da;
                } else {
                    p->targetYaw += da;
                }
            }
            if (p->input.stickDir & 0x400) {
                p->aimLockTimer = 10;
                if (gScene.aimReverse == 2) {
                    p->targetYaw += da;
                } else {
                    p->targetYaw -= da;
                }
            }
        }
    }
    if ((u16) mot == 0x19E) {
        d = p->targetYaw - p->yawCenter;
        if (d <= 0x8000U && d > 0x4000U) {
            p->targetYaw = p->yawCenter + 0x4000;
        } else if (d >= 0x8000U && d < 0xC000U) {
            p->targetYaw = p->yawCenter - 0x4000;
        }
    }
}

void Player_State0_00(Player *p, s32 unused)
{
    u16 motion = p->motionId[0];

    switch (motion) {
    case 1:
    case 0x3F:
        if (p->staminaMax < 76) {
            Player_SetMotionPair(p, 0x193, 6, 0);
        } else if (p->motionTime >= 240 && p->gender == 1 && motion != 0x3F) {
            Player_SetMotionPair(p, 0x3F, 2, 0);
        }
        break;
    case 0x15:
        if (p->motion[0].playing == 0) {
            Player_SetMotionPair(p, 5, 4, 0x90);
        }
        break;
    case 5:
        if (!(gScene.frame & 0x1F)) {
            Effect02_SpawnOnOwner(p, 8, 0x14, 0.1f);
        }
        break;
    case 0x193:
        if (p->staminaMax > 75) {
            Player_SetMotionPair(p, 1, 4, 0);
        }
        break;
    case 0x3E9:
        if (p->weaponOut != 0 && p->isGunner == 1 && p->weaponType != 10) {
            Player_SetAimLayers(p, 0x3ED, 0x3EE);
        }
        break;
    }
    switch (p->weaponType) {
    case 1:
    case 5:
        Player_BowgunAim(p);
        break;
    }
    Player_IdleActions(p, 0);
}

void Player_State0_01(Player *p, s32 variant)
{
    f32 v[3];
    f32 d[3];
    u8 r;
    u8 t;

    if (p->step == 0) {
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 10;
        p->timer = 10;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x1000);
        switch (variant) {
        case 0:
            Player_SetMotionPair(p, 3, 6, 0);
            Player_SetFlag(p, 0x2000000);
            break;
        case 1:
            Player_SetMotionPair(p, 2, 4, 0);
            break;
        case 2:
        case 9:
            switch (p->weaponType) {
            case 0:
            case 7:
            default:
                Player_SetMotionPair(p, 0x3EC, 4, 0);
                break;
            case 3:
            case 9:
                Player_SetMotionPair(p, 0x3EC, 4, 0);
                break;
            case 2:
            case 8:
                Player_SetMotionPair(p, 0x3EC, 8, 0);
                break;
            case 1:
            case 4:
            case 5:
            case 6:
            case 10:
                Player_SetMotionPair(p, 0x3EC, 6, 0);
                break;
            }
            break;
        case 3:
            Player_SetMotionPair(p, 10, 6, 0);
            break;
        case 4:
            break;
        case 5:
            Player_SetMotionPair(p, 10, 2, 0);
            p->action = 1;
            break;
        case 6:
            p->dashDelay = 30;
            if (p->motionId[0] != 3) {
                Player_SetMotionPair(p, 3, 0, 0x26);
            }
            Player_SetFlag(p, 0x2000000);
            break;
        case 7:
            Player_SetMotionPair(p, 0x31, 6, 0);
            break;
        case 8:
            Player_SetFlag(p, 0x8000);
            Player_SetMotionPair(p, 0x3F3, 4, 0);
            break;
        }
        p->motionTime = 0;
    }
    if (Player_IsLocal(p) == 0 && Game_IsOnline() == 1) {
        return;
    }
    r = Player_StickMoveLevel(p, p->weaponOut) & 0xFF;
    if (p->stateVar1 != 0) {
        p->stateVar1--;
    }
    if (p->timer > 0) {
        p->timer--;
    }
    t = r;
    if (t == 0 && p->timer == 0) {
        if (p->motionId[0] == 0x3EC) {
            if (variant == 9) {
                Player_SetState(p, 1, 7, 4);
            } else {
                Player_BeginState(p, 0, Player_State0_01_NextByWeapon[p->weaponType], 0);
            }
        } else if (p->motionId[0] == 3 || p->motionId[0] == 4) {
            if (p->stateVar1 == 0) {
                p->freeState = 1;
                Player_SetState(p, 0, 0x2C, 0);
            } else {
                Player_BeginState(p, 0, 4, 0);
            }
        } else if (p->motionId[0] == 0x3F3) {
            Player_RequestState(p, 2, 0xC, 0);
        } else {
            Player_BeginState(p, 0, 4, 0);
        }
        return;
    }
    switch (p->motionId[0]) {
    case 2:
        if (Motion_PassedFrame(p, 0, 108.0f)) {
            p->stateVar0++;
            if (p->stateVar0 >= 5) {
                p->stateVar0 = 0;
                Player_SetMotionPair(p, 0x1D, 2, 0);
            }
        }
        break;
    case 3:
        if (t < 3) {
            p->freeState = 1;
            Player_SetState(p, 0, 0x2C, 0);
            return;
        }
        break;
    case 0x1D:
        if (p->motion[0].playing == 0) {
            Player_SetMotionPair(p, 2, 6, 0x24);
        }
        break;
    case 0x3F3:
        if (!(p->input.held & 0x80)) {
            Player_RequestState(p, 2, 0xC, 0);
        }
        break;
    }
    if (p->unk668 != 0 && (variant != 5 || variant != 8)) {
        Player_SetState(p, 0, 0x3B, 0);
    }
    if (t != 0) {
        Player_FaceStick(p);
    }
    if (Player_IsLocal(p) == 0) {
        return;
    }
    if (variant == 2 && p->weaponType == 8 && Player_IsLocal(p) == 1 && p->motionId[0] == 0x3EC &&
        Player_HasMealEffect(p, 0) == 1 && p->mealEffect[0] > 0) {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 8.0f;
        Vec_Rotate(d, v, p->rotMtx);
        p->pos[0] += d[0];
        p->pos[2] += d[2];
    }
    if (variant == 2 && p->weaponType == 6 && Player_IsLocal(p) == 1 && p->motionId[0] == 0x3EC &&
        p->demonMode != 0) {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 2.0f;
        Vec_Rotate(d, v, p->rotMtx);
        p->pos[0] += d[0];
        p->pos[2] += d[2];
    }
    if (variant != 8) {
        if (variant == 9) {
            Player_IdleActions(p, 1);
        } else {
            Player_IdleActions(p, 0);
        }
    } else {
        Player_GatherStep(p);
    }
}

void Player_State0_04(Player *p, u32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        switch (variant) {
        case 0:
            p->weaponOut = Player_State0_04_Motions0[p->weaponType].flag;
            Player_SetMotionPair(p, Player_State0_04_Motions0[p->weaponType].motion, Player_State0_04_Motions0[p->weaponType].blend, 0);
            break;
        case 1:
        case 3:
        case 4:
            p->weaponOut = Player_State0_04_Motions4[p->weaponType].flag;
            Player_SetMotionPair(p, Player_State0_04_Motions4[p->weaponType].motion, Player_State0_04_Motions4[p->weaponType].blend, 0);
            break;
        case 2:
            Player_SetFlag(p, 0x1000);
            p->weaponOut = Player_State0_04_Motions2[p->weaponType].flag;
            if (p->weaponType == 8) {
                Player_SetMotionPair(p, Player_State0_04_Motions2[p->weaponType].motion, Player_State0_04_Motions2[p->weaponType].blend, 0xA);
            } else {
                Player_SetMotionPair(p, Player_State0_04_Motions2[p->weaponType].motion, Player_State0_04_Motions2[p->weaponType].blend, 0);
            }
            break;
        case 5:
            p->weaponOut = Player_State0_04_Motions5[p->weaponType].flag;
            Player_SetMotionPair(p, Player_State0_04_Motions5[p->weaponType].motion, Player_State0_04_Motions5[p->weaponType].blend, 0);
            break;
        }
        if (p->carry & 0xF) {
            p->carry &= 0xF0;
            StageObj_K0C_CreateAtPlayer(p, 4);
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            switch (variant) {
            case 0:
                Player_BeginState(p, 0, Player_State0_04_Motions0[p->weaponType].next, 0);
                break;
            case 1:
                Player_BeginState(p, 0, Player_State0_04_Motions4[p->weaponType].next, 0);
                break;
            case 2:
                Player_BeginState(p, 0, Player_State0_04_Motions2[p->weaponType].next, 0);
                break;
            case 3:
                Player_RequestState(p, 0, 8, 2);
                break;
            case 4:
                if (Player_IsLocal(p) == 1) {
                    Player_UseItem(p, 1);
                    p->pouchHudFlags |= 1;
                    break;
                }
                Player_BeginState(p, 0, Player_State0_04_Motions0[p->weaponType].next, 0);
                break;
            case 5:
                Player_BeginState(p, 0, Player_State0_04_Motions5[p->weaponType].next, 0);
                break;
            }
        }
        break;
    }
}

void Player_State0_06(Player *p, s32 variant)
{
    f32 v[3];
    f32 d[3];
    f32 t;

    p->timer++;
    switch (p->step) {
    case 0:
        p->step++;
        p->action = 2;
        p->aimLockTimer = 0;
        Player_ResetAim(p);
        switch (variant) {
        default:
            Player_SetMotionPair(p, 0xF, 0, 0);
            p->vel[0] = 0.0f;
            p->vel[2] = 20.0f;
            p->vel[1] = 18.0f;
            p->accel[0] = 0.0f;
            p->accel[2] = -(p->vel[2] / 80.0f);
            Player_StartJump(p, 0x13);
            if (p->rideState) {
                p->rideState = 2;
            }
            break;
        case 1:
            Player_SetMotionPair(p, 0xF, 8, 0x20);
            p->rideState = 0;
            p->vel[0] = 0.0f;
            p->vel[2] = 8.0f;
            p->vel[1] = -10.0f;
            p->accel[2] = 0.0f;
            p->accel[0] = 0.0f;
            p->accel[1] = -0.72727275f;
            break;
        }
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 2, 0, 0);
        p->timer = 0;
        p->stateVar1 = 0;
        break;
    case 1:
        p->vel[0] += p->accel[0];
        p->vel[1] += p->accel[1];
        p->vel[2] += p->accel[2];
        if (p->vel[0] < 0.0f) {
            p->vel[0] = 0.0f;
        }
        if (p->vel[2] < 0.0f) {
            p->vel[2] = 0.0f;
        }
        v[0] = p->vel[0];
        v[1] = p->vel[1];
        v[2] = p->vel[2];
        Vec_Rotate(d, v, p->rotMtx);
        p->pos[0] += d[0];
        p->pos[1] += d[1];
        p->pos[2] += d[2];
        if (p->vel[1] < 0.0f) {
            if (Player_FloorAheadAbove(p, &t, 55.0f, 180.0f, 30.0f) && p->hp > 0 && p->unk893 < 21) {
                p->targetYaw = p->rot[1];
                Player_RequestState(p, 0, 0x1B, 4);
                break;
            }
            if (p->floorY < p->pos[1]) {
                break;
            }
            p->pos[1] = p->floorY;
            p->action = 0;
            p->rideState = 0;
            if (p->hp <= 0 && Player_IsLocal(p) == 1) {
                Player_Die(p);
                break;
            }
            if (p->timer >= 30) {
                Player_RequestState(p, 0, 0x26, 0);
                break;
            }
            p->step++;
            if (p->weaponOut == 0 && (p->input.held & 0x10) && (Player_StickMoveLevel(p, 0) & 0xFF)) {
                p->stateVar1 = 1;
                Player_SetMotionPair(p, 0x10, 0, 0);
            } else {
                p->stateVar1 = 0;
                Player_SetMotionPair(p, 0x3E, 0, 0);
            }
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            if (p->stateVar1 == 0) {
                Player_BeginState(p, 0, 8, 0);
            } else if (p->inBattle == 0) {
                Player_RequestState(p, 0, 0x46, 0);
            } else {
                Player_RequestState(p, 0, 0x20, 0);
            }
        }
        break;
    }
}

void Player_State0_15(Player *p, s32 variant)
{
    f32 y;
    f32 v[3];
    f32 d[3];

    p->timer++;
    switch (p->step) {
    case 0:
        switch (variant) {
        case 0:
        default:
            p->step++;
            Player_SetMotionPair(p, 0xB, 2, 0);
            Player_SetMoveMode(p, 0, 0, 0);
            break;
        case 2:
            p->pos[1] -= 195.0f;
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -30.0f;
            Vec_Rotate(d, v, (f32 *) p->worldMtx);
            p->pos[0] += d[0];
            p->pos[2] += d[2];
        case 1:
            p->step = 2;
            Player_SetMoveMode(p, 2, 0, 0);
            Player_StopMove(p);
            p->vel[1] = -9.0f;
            p->accel[1] = -8.0f / 11.0f;
            if (variant == 1) {
                Player_SetMotionPair(p, 0xC, 4, 0x16);
            } else {
                Player_SetMotionPair(p, 0xC, 0, 0x16);
            }
            break;
        }
        Player_ResetAim(p);
        Player_UpdateMotionTime(p, 0);
        p->timer = 0;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_StopMove(p);
            p->vel[1] = 2.0f * (15.0f * (p->weaponOut ? 0.8f : 1.0f));
            Player_StartJump(p, 0x14);
            Player_SetMotionPair(p, 0xC, 2, 0);
            p->action = 2;
            if (p->rideState != 0) {
                p->rideState = 2;
            }
            Player_UpdateMotionTime(p, 0);
            p->timer = 0;
        }
        break;
    case 2:
        Player_MoveAccel(p);
        if (p->vel[1] < 0.0f) {
            if (variant != 2 && Player_FloorAheadAbove(p, &y, 55.0f, 180.0f, 30.0f) && p->hp > 0) {
                p->targetYaw = p->rot[1];
                Player_RequestState(p, 0, 0x1B, 0xC);
                break;
            }
            if (!(p->floorY < p->pos[1])) {
                p->pos[1] = p->floorY;
                p->action = 0;
                p->rideState = 0;
                if (p->hp <= 0 && Player_IsLocal(p) == 1) {
                    Player_Die(p);
                    break;
                }
                if (p->timer >= 30) {
                    Player_SetState(p, 0, 0x26, 0);
                    break;
                }
                p->step++;
                Player_SetMotionPair(p, 0xD, 0, 0);
            }
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 3, 0);
        }
        break;
    }
}

void Player_State0_08(Player *p, s32 variant)
{
    u8 carry;

    p->freeState = 1;
    switch (p->step) {
    case 0:
        switch (variant) {
        case 0:
            p->step++;
            p->motionTime = 0;
            Player_SetMotionPair(p, 7, 2, 0);
            Player_SetMoveMode(p, 0, 0, 0);
            p->weaponOut = 0;
            carry = p->carry;
            if (carry & 0xF) {
                p->carry = carry & 0xF0;
                StageObj_K0C_CreateAtPlayer(p, 4);
            }
            break;
        case 1:
            p->step = 2;
            if (p->motionId[0] == 0x19C) {
                Player_SetMotionPair(p, 8, 8, 0);
            } else {
                Player_SetMotionPair(p, 8, 2, 0);
            }
            Player_UpdateMotionTime(p, 0);
            Player_SetMoveMode(p, 0x8001, 0, 0);
            p->weaponOut = 0;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 8, 6, 0);
            Player_SetMoveMode(p, 0x8001, 0, 0);
            Player_UpdateMotionTime(p, 0);
        }
        break;
    case 2:
        Player_IdleInput(p);
        break;
    }
}

//---
void Player_State0_0C(Player *p, s32 unused)
{

    p->timer--;
    if (p->timer <= 0) {
        p->timer = 0;
    }
    switch (p->step) {
    case 0:
        p->step++;
        p->timer = 30;
        Player_SetMotionPair(p, 1, 4, 0);
        p->deliveryMenu = 0xFF;
        p->menuDelay = 30;
        p->menuCursor = 0;
        p->menuPage = 0;
        break;
    case 1:
        if ((s32) p->motionTime >= 0xF0 && p->gender == 1 && p->motionId[0] != 0x3F) {
            Player_SetMotionPair(p, 0x3F, 2, 0);
        }
        if (Player_IsLocal(p) == 1 && p->timer <= 0) {
            if (Quest_PhaseIsOver(1) == 1 || p->deliveryMenu == 0) {
                Player_BeginState(p, 0, 2, 0);
            } else if (p->deliveryMenu == 1) {
                p->deliveryMenu = 0;
                switch (p->menuCursor) {
                case 0:
                    Player_StartSpotAction(p, 1);
                    break;
                case 1:
                    Quest_Start();
                    Player_BeginState(p, 0, 2, 0);
                    break;
                case 2:
                    Quest_Abort();
                    Player_BeginState(p, 0, 2, 0);
                    break;
                }
            }
        }
        break;
    }
}

void Player_State0_0D(Player *p, s32 unused)
{
    Player_BeginState(p, 0, 0x2, 0);
}

void Player_State0_0B(Player *p, s32 variant)
{
    s8 kind;
    u8 nouse;
    u8 k;

    switch (p->step) {
    case 0:
        p->step++;
        if (variant == 0) {
            Player_SetMotionPair(p, 0x191, 2, 0);
        } else {
            Player_SetMotionPair(p, 0x1A1, 2, 0);
        }
        p->motionTime = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (Motion_PassedFrame(p, 0, variant ? 20.0f : 24.0f)) {
            kind = 0;
            if (p->prevStateGroup == 0) {
                if (p->prevState == 1) {
                    kind = 2;
                }
                if (p->prevState == 2) {
                    kind = 1;
                }
            }
            nouse = 0;
            switch (p->useItem) {
            case 0x4C:
                Eff03_Create(p, 0, kind);
                break;
            case 0x4E:
                Eff03_Create(p, 7, kind);
                break;
            case 0x48:
                Eff03_Create(p, 1, kind);
                break;
            case 0x297:
                Eff03_Create(p, 1, kind);
                break;
            case 0x4B:
                Eff03_Create(p, 5, kind);
                break;
            case 0x4D:
                Eff03_Create(p, 0xb, kind);
                break;
            case 0x47:
                Eff03_Create(p, 6, kind);
                break;
            case 0xD7:
                Eff03_Create(p, 8, kind);
                break;
            case 0x49:
                Eff03_Create(p, 9, kind);
                break;
            case 0x50:
                Eff03_Create(p, 0xa, kind);
                break;
            case 0x41:
                Eff03_Create(p, 0xc, kind);
                break;
            case 0x42:
                Eff03_Create(p, 0xd, kind);
                break;
            case 0x43:
                Eff03_Create(p, 0xe, kind);
                break;
            case 0x44:
                Eff03_Create(p, 0xf, kind);
                break;
            case 0x45:
                Eff03_Create(p, 0x15, kind);
                break;
            case 0x2BA:
                Eff03_Create(p, 0x11, kind);
                break;
            case 0x2BB:
                Eff03_Create(p, 0x12, kind);
                break;
            case 0x2BC:
                Eff03_Create(p, 0x13, kind);
                break;
            case 0x4F:
                Eff03_Create(p, 0x17, kind);
                break;
            case 0x40:
                if (p->prevStateGroup == 0 && p->prevState == 0x49) {
                    kind = 1;
                }
                if (p->stateRand % 8) {
                    nouse = 1;
                }
                if (Player_HasSkill(p, 0x2D) == 1 && !(Rand_Next(1) & 7)) {
                    nouse = 1;
                }
                if (nouse == 0 && Player_IsLocal(p) == 1) {
                    Notice_Push(1, 5, 0x40);
                }
                Eff28_Create((Em *) p, kind, nouse);
                break;
            }
            if (Quest_PhaseIsOver(1) == 0 && nouse == 0) {
                Pouch_Add(p, p->useItem, -1);
            }
        }
        if (p->motion[0].playing == 0) {
            switch (variant) {
            case 0:
                Player_BeginState(p, 0, 0xA, 0);
                break;
            case 1:
                Player_BeginState(p, 0, 2, 0);
                break;
            case 2:
                if (Player_IsLocal(p) == 1 && Quest_PhaseIsOver(1) == 0) {
                    k = gScene.campArea;
                    p->areaMoveState = 1;
                    p->startPos[0] = gStartPos[k][0];
                    p->startPos[1] = D_43F0A4[k][0];
                    p->startPos[2] = D_43F0A8[k][0];
                    p->startAngle = gStartAngle[k];
                    p->nextArea = k;
                    Player_AddSpawnOffset(p, p->startPos, p->startAngle);
                    p->noHitTimer = 5;
                    QSync_SendPlayer(p, 5, 0);
                } else {
                    Player_BeginState(p, 0, 2, 0);
                }
                break;
            }
        }
        break;
    case 2:
        break;
    }
}

void Player_State0_11(Player *p, s32 variant)
{
    f32 y;
    s32 ang[3];
    f32 v[3];
    f32 pos[3];
    f32 d[3];
    f32 m[16];

    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        p->targetYaw = p->rot[1];
        Player_SetMotionPair(p, 0x11, 0, 0);
        p->timer = 30;
        Player_SetMoveMode(p, 0, 0, 0);
        if (variant == 1) {
            ang[0] = 0;
            ang[1] = p->rot[1];
            ang[2] = 0;
            Mtx_FromAnglesXYZ(ang, m);
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 55.0f;
            Vec_Rotate(d, v, m);
            pos[0] = p->pos[0] + d[0];
            pos[1] = p->pos[1] + d[1];
            pos[2] = p->pos[2] + d[2];
            if (Em_GetFloorYAbove((Em *) p, pos, &y) == 1) {
                p->pos[1] = y;
            }
            v[2] = 55.0f;
            Vec_Rotate(d, v, (f32 *) p->worldMtx);
            p->pos[0] += d[0];
            p->pos[1] += d[1];
            p->pos[2] += d[2];
        }
        if (Player_IsState(p, 0, 0x1B) == 1 && p->prevStateGroup == 0 && p->prevState == 9) {
            p->unk893++;
        }
        break;
    case 1:
        if (p->timer != 0) {
            p->timer--;
            if (p->timer <= 0) {
                p->timer = 0;
            }
        }
        if (p->motion[0].playing == 0 || (p->input.pressed & 0x20) || (p->input.stickDir & 0x2000)) {
            Player_RequestState(p, 0, 0x3A, 0xC);
        } else if (Player_IsLocal(p) == 1 && (p->input.stickDirPressed & 0x1000) && p->timer == 0) {
            Player_RequestState(p, 0, 0x38, 4);
        }
        break;
    }
}

void Player_State0_16(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x1A, 6, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (Player_CheckRideSlip(p) == 0) {
            Player_BeginState(p, 0, 8, 0);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
        }
        break;
    }
}

void Player_State0_14(Player *p, s32 variant)
{
    s32 timer;

    switch (p->step) {
    case 0:
        p->motionTime = 0;
        p->timer = 0;
        switch (variant) {
        case 0:
            p->step++;
            Player_SetMotionPair(p, 8, 6, 0);
            Player_SetMoveMode(p, 0, 0, 0);
            break;
        case 1:
            p->step = 3;
            Player_SetMoveMode(p, 1, 0, 0);
            Player_SetMotionPair(p, 0x199, 0, 0x14);
            break;
        }
        Player_UpdateMotionTime(p, 0);
        if (Player_IsLocal(p) == 1) {
            Item_UseTool(p);
        }
        break;
    case 1:
        timer = p->timer + 1;
        p->timer = timer;
        if (timer >= 0xE) {
            p->step++;
            Player_SetMotionPair(p, 9, 0, 0);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 2, 0);
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 0U, 0x1DU, 2);
        }
        break;
    }
}

void Player_State0_19(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        p->timer = 0;
        Player_SetMotionPair(p, 7, 2, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            if (Player_IsLocal(p) == 1) {
                Player_RequestState(p, 0U, 0x18U, 0xC);
                break;
            }
            p->step++;
            Player_SetMotionPair(p, 8, 6, 0);
        } else {
        case 2:
            break;
        }
        break;
    }
}

void Player_State0_1C(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->motionTime = 0;
        Player_SetEvadeFrames(p);
        p->timer = 0;
        Player_ResetAim(p);
        p->noFlinchTimer = 0;
        p->stateVar0 = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_AddStamina(p, -75);
        Player_PlaySe(p, 30, 0, p->pos, 1, 0);
        if (p->carry & 0xF) {
            p->carry &= 0xF0;
            StageObj_K0C_CreateAtPlayer(p, 4);
        }
        if (variant == 0) {
            p->step = 2;
            if ((p->weaponType == 2 || p->weaponType == 8) && p->weaponOut != 0) {
                Player_SetMotionPair(p, 0x3F3, 2, 0);
            } else {
                Player_SetMotionPair(p, 0x1C, 2, 0);
            }
        } else {
            p->step++;
            Player_SetMotionPair(p, 0x16, 0, 0);
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x1C, 0, 6);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
        } else if (Player_IsLocal(p) == 1 && variant == 0 && p->weaponOut != 0) {
            switch (p->weaponType) {
            case 4:
            case 6:
                if ((p->input.stickDirPressed & 0x20) && Motion_InFrameRange(p, 0, 10.0f, 44.0f) && p->stateVar0 == 0) {
                    p->stateVar0 = 1;
                    if (p->input.stickDir & 0x800) {
                        p->stateVar0 = 2;
                    }
                    if (p->input.stickDir & 0x400) {
                        p->stateVar0 = 3;
                    }
                }
                if (Motion_ReachedFrame(p, 0, 44.0f) && p->stateVar0 != 0) {
                    if ((p->stateVar0 & 0xFF) == 2) {
                        p->rot[1] += 0x2000;
                        p->targetYaw = p->rot[1];
                    }
                    if (p->stateVar0 == 3) {
                        p->rot[1] -= 0x2000;
                        p->targetYaw = p->rot[1];
                    }
                    if (p->weaponType == 4) {
                        SwordShield_EvadeAttack(p);
                    } else {
                        DualSwords_EvadeAttack(p);
                    }
                }
                break;
            case 2:
                if (Motion_ReachedFrame(p, 0, 40.0f) && (p->input.held & 0x80)) {
                    Hammer_EvadeCharge(p);
                }
                break;
            }
        }
        break;
    }
}

//---
void Player_State0_1E(Player *p, s32 unused)
{
    f32 v[3];
    f32 pos[4];
    f32 d[4];
    f32 y;
    f32 *vz;
    f32 *dy;
    f32 *dz;

    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMotionPair(p, 0x12, 0, 0x58);
        p->targetYaw = p->rot[1];
        v[0] = 0.0f;
        v[1] = 0.0f;
        vz = &v[2];
        *vz = 55.0f;
        Vec_Rotate(d, v, (f32 *) p->worldMtx);
        dy = &d[1];
        dz = &d[2];
        pos[0] = p->pos[0] + d[0];
        pos[1] = p->pos[1] + *dy;
        pos[2] = p->pos[2] + *dz;
        if (Em_GetFloorYAbove((Em *) p, pos, &y) == 1) {
            p->pos[1] = y;
        }
        *vz = 55.0f;
        Vec_Rotate(d, v, (f32 *) p->worldMtx);
        p->pos[0] += d[0];
        p->pos[1] += *dy;
        p->pos[2] += *dz;
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State0_1F(Player *p, s32 variant)
{
    u8 r;
    f32 v[3];
    f32 d[3];

    p->freeState = 1;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x600);
        p->motionTime = 0;
        p->dashDelay = 30;
        switch (variant) {
        case 0:
            if (p->motionId[0] != 3) {
                Player_SetMotionPair(p, 3, 0, 0);
            }
            break;
        case 1:
            Player_SetMotionPair(p, 3, 4, 30);
            break;
        }
        p->motion[0].step = 2.5f;
        p->motion[1].step = 2.5f;
        p->targetYaw = p->rot[1];
        break;
    case 1:
        p->motion[0].step = 2.5f;
        p->motion[1].step = 2.5f;
        Player_UseStamina(p, -1);
        if (p->stamina < 0x4C) {
            Player_RequestState(p, 0, 0x22, 0);
            break;
        }
        if ((p->input.pressed & 0x40) && p->stamina >= 0x4B && Quest_PhaseIsOver(1) == 0) {
            p->targetYaw = p->rot[1];
            Player_SetState(p, 0, 0x1C, 4);
            break;
        }
        if (p->input.stickTilt >= 0x28) {
            Player_FaceStick(p);
        }
        if (Player_FloorAheadBelow(p, 0, 85.0f, -60.0f)) {
            p->targetYaw = p->rot[1];
            Player_RequestState(p, 0, 6, 0);
            break;
        }
        if (p->inBattle != 0 && (u16) (p->targetAngle - p->rot[1] + 0x2000) > 0x4000) {
            Player_SetState(p, 0, 0x20, 0);
            break;
        }
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 5.0f;
        Vec_Rotate(d, v, (f32 *) p->rotMtx);
        p->pos[0] += d[0];
        p->pos[2] += d[2];
        if (Player_IsLocal(p) != 0) {
            r = Player_StickMoveLevel(p, 0);
            if (r != 5 && (Player_IsLocal(p) == 1 || Game_IsOnline() == 0)) {
                if (Player_IsState(p, 0, 0x22) == 0 || p->stamina > 0) {
                    if (r == 3) {
                        if (p->dashDelay == 0) {
                            Player_SetState(p, 0, 0x24, 0);
                        } else {
                            Player_IdleActions(p, 0);
                        }
                    } else {
                        p->freeState = 1;
                        Player_SetState(p, 0, 0x2C, 0);
                    }
                } else {
                    Player_SetState(p, 0, 0x21, 0);
                }
            } else {
                Player_IdleActions(p, 0);
            }
            if (p->unk668 != 0) {
                Player_SetState(p, 0, 0x3B, 0);
            }
        }
        break;
    }
}

void Player_State0_20(Player *p, s32 variant)
{
    u8 r;
    f32 v[3];
    f32 d[3];

    p->freeState = 1;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x600);
        p->motionTime = 0;
        switch (variant) {
        case 0:
            Player_SetMotionPair(p, 0x26, 0, 0);
            break;
        case 1:
            Player_SetMotionPair(p, 0x27, 0, 0);
            break;
        case 2:
            Player_SetMotionPair(p, 0x26, 4, 0);
            break;
        }
        p->targetYaw = p->rot[1];
        break;
    case 1:
        if (variant != 1) {
            Player_UseStamina(p, -2);
            if (p->stamina < 0x4C) {
                Player_RequestState(p, 0, 0x22, 0);
                break;
            }
            if (p->inBattle == 0 || (u16) (p->targetAngle - p->rot[1] + 0x2000) <= 0x4000) {
                Player_SetState(p, 0, 0x46, 0);
                break;
            }
        } else {
            Player_UseStamina(p, -1);
        }
        if ((p->input.pressed & 0x40) && variant != 1 && p->stamina >= 0x4B) {
            p->targetYaw = p->rot[1];
            Player_SetState(p, 0, 0x44, 4);
            break;
        }
        if (p->input.stickTilt >= 0x28) {
            Player_FaceStick(p);
        }
        if (Player_FloorAheadBelow(p, 0, 85.0f, -60.0f) && variant != 1) {
            p->targetYaw = p->rot[1];
            Player_RequestState(p, 0, 6, 0);
            break;
        }
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 1.5f;
        Vec_Rotate(d, v, (f32 *) p->rotMtx);
        p->pos[0] += d[0];
        p->pos[2] += d[2];
        if (Player_IsLocal(p) != 0) {
            r = Player_StickMoveLevel(p, 0);
            if (r != 5 && (Player_IsLocal(p) == 1 || Game_IsOnline() == 0)) {
                if (Player_IsState(p, 0, 0x22) == 0 || p->stamina > 0) {
                    if (r == 3 && variant != 1) {
                        Player_SetState(p, 0, 0x24, 0);
                    } else {
                        p->freeState = 1;
                        Player_SetState(p, 0, 0x2C, 0);
                    }
                } else {
                    Player_SetState(p, 0, 0x21, 0);
                }
            } else {
                Player_IdleActions(p, 0);
            }
            if (p->unk668 != 0) {
                Player_SetState(p, 0, 0x3B, 0);
            }
        }
        break;
    }
}

void Player_State0_21(Player *p, s32 variant)
{
    Player_AddStamina(p, 4);
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        p->weaponOut = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(p, 5, 4, 0);
            p->step = 2;
            p->timer = 0x96;
            break;
        }
        Player_SetMotionPair(p, 0x15, 8, 0);
        p->timer = 0xA;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 5, 4, 0x90);
        }
        break;
    case 2:
        if (--p->timer <= 0) {
            p->step++;
            Player_SetMotionPair(p, 6, 4, 0);
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_AddStamina(p, (s32) p->staminaMax);
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State0_25(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x27, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 5, 0);
        }
        break;
    }
}

void Player_State0_26(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0x8000, 0, 0);
        p->rideState = 0;
        Player_SetMotionPair(p, 0x13, 2, 0);
        Vib_StartLocal(p, 1);
        CamShake_ReqA(p, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
        }
        break;
    }
}

void Player_State0_27(Player *p, s32 unused)
{
    u8 step;

    step = p->step;
    if (step == 0) {
        p->step = step + 1;
        Player_SetFlag(p, 0x20000U);
        p->stateVar0 = 0;
        if (p->motionId[0] == 0x22) {
            if (Motion_PassedFrame(p, 0, 46.0f) != 0) {
                Player_SetMotionPair(p, 0x22, 0, 0x2E);
            } else {
                Player_SetMotionPair(p, 0x22, 0, 0x5A);
            }
        } else if (Motion_PassedFrame(p, 0, 0.0f) != 0) {
            Player_SetMotionPair(p, 0x23, 0, 0);
        } else {
            Player_SetMotionPair(p, 0x23, 0, 0x2C);
        }
    } else {
        p->stateVar0 = 1;
    }
    p->noPushTimer = 2;
    Player_SetMoveMode(p, 0x8002, 0, 0);
    Player_StepBack(p, 1);
    Player_TrySideStep(p, 0);
}

void Player_State0_28(Player *player, s32 variant)
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
        if (!(Player_TrySideStep(player, 1) & 0xFF)) {
            Player_RequestState((void *) player, 0U, 0x27U, 0);
        } else if (variant == 1) {
            Player_UseDashStamina(player);
        }
    }
    if (player->stamina <= 0) {
        Player_RequestState((void *) player, 0U, 0x30U, 0);
        return;
    }
    if (Player_FloorAheadAbove((void *) player, &h, 55.0f, 180.0f, 30.0f) != 0) {
        player->targetYaw = (s16) player->rot[1];
        Player_RequestState((void *) player, 0U, 0x1BU, 0);
    }
    Player_StepBack(player, 1);
}

void Player_State0_29(Player *p, s32 variant)
{
    u16 motion;

    p->noPushTimer = 2;
    if (variant == 0) {
        p->motion[0].step = 1.6f;
        p->motion[1].step = 1.6f;
    } else {
        p->motion[0].step = 2.6f;
        p->motion[1].step = 2.6f;
    }
    if (p->step == 0) {
        p->step++;
        Player_SetMoveMode(p, 2, 0, 0);
        p->rideState = 0;
        motion = p->motionId[0];
        if (motion != 0x23) {
            if (motion == 0x22) {
                if (Motion_ReachedFrame(p, 0, 86.0f) != 0) {
                    Player_SetMotionPair(p, 0x23, 0, 0x2C);
                } else {
                    Player_SetMotionPair(p, 0x23, 0, 0);
                }
            } else {
                Player_SetMotionPair(p, 0x23, 4, 0);
            }
        }
    } else if (Motion_PassedFrame(p, 0, 0.0f) != 0 || Motion_PassedFrame(p, 0, 44.0f) != 0) {
        if (Player_IsLocal(p) == 1) {
            if (!(Player_TrySideStep(p, 1) & 0xFF)) {
                Player_RequestState(p, 0, 0x27, 0);
            } else if (variant == 1) {
                Player_UseDashStamina(p);
            }
        }
    }
    if (p->stamina <= 0) {
        Player_RequestState(p, 0, 0x30, 0);
        return;
    }
    if (p->pos[1] <= p->floorY) {
        Player_RequestState(p, 0, 0x2A, 0);
    }
    Player_StepBack(p, 1);
}

void Player_State0_2A(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x24, 6, 0);
        Player_StepBack(p, 1);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        Player_StepBack(p, 1);
        break;
    }
}

void Player_State0_2B(Player *p, s32 unused)
{
    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 2, 0, 0);
        p->rideState = 0;
        Player_TurnAlongWallSide(p, 0);
        Player_SetMotionPair(p, 0x22, 4, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(p, 0, 90.0f) != 0) {
            Player_RequestState(p, 0U, 0x27U, 0);
        }
        break;
    }
    Player_StepBack(p, 1);
}

void Player_State0_2C(Player *p, s32 unused)
{
    p->freeState = 1;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x1E, 4, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 0xA, 0);
        }
        break;
    }
}

void Player_State0_2E(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x195, -0xE, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(p, 0, 130.0f) != 0) {
            p->step++;
            Player_SetMotionPair(p, 0x19A, 4, 0);
        }
        break;
    case 2:
        if (Motion_ReachedFrame(p, 0, 60.0f) != 0) {
            if (Player_IsLocal(p) == 1) {
                Player_RequestState(p, 0U, 0x2FU, 0xC);
                break;
            }
            p->step++;
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 0xA, 0);
        }
        break;
    }
}

void Player_State0_2F(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x19A, 0, 0x3C);
        if (Player_IsLocal(p) == 1) {
            if (p->useItem == 0x2A6) {
                Quest_DeliverFromPouch(p, 1);
                break;
            }
            Item_UseMisc(p);
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x195, -4, 0xCE);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 0xA, 0);
        }
        break;
    }
}

void Player_State0_31(Player *p, s32 variant)
{
    u8 dir;

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->noFlinchTimer = 0;
        Player_SetEvadeFrames(p);
        Player_SetMoveMode(p, 0, 0, 0);
        Player_AddStamina(p, -75);
        Player_PlaySe(p, 0x1E, 0, &p->pos[0], 1, 0);
        p->motionTime = 0;
        if (variant == 0) {
            if (p->weaponType == 7) {
                Player_SetMotionPair(p, 0x3EF, 2, 0);
                break;
            }
            Player_SetMotionPair(p, 0x3F5, 2, 0);
            break;
        }
        if (p->weaponType == 7) {
            Player_SetMotionPair(p, 0x3EE, 2, 0);
            break;
        }
        Player_SetMotionPair(p, 0x3F4, 2, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
            break;
        }
        if (Player_IsLocal(p) == 1 && p->weaponOut != 0) {
            switch (p->weaponType) {
            case 4:
            case 6:
                if ((p->input.stickDirPressed & 0x20) && Motion_InFrameRange(p, 0, 10.0f, 42.0f) != 0 && p->stateVar0 == 0) {
                    p->stateVar0 = 1;
                    if (p->input.stickDir & 0x800) {
                        p->stateVar0 = 2;
                    }
                    if (p->input.stickDir & 0x400) {
                        p->stateVar0 = 3;
                    }
                }
                if (Motion_ReachedFrame(p, 0, 42.0f) != 0 && (dir = p->stateVar0)) {
                    if (dir == 2) {
                        p->rot[1] += 0x2000;
                        p->targetYaw = p->rot[1];
                    }
                    if (p->stateVar0 == 3) {
                        p->rot[1] -= 0x2000;
                        p->targetYaw = p->rot[1];
                    }
                    if (p->weaponType == 4) {
                        SwordShield_EvadeAttack(p);
                        break;
                    }
                    DualSwords_EvadeAttack(p);
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
s16 Player_State0_01_NextByWeapon[16] = {
#include "game/008C3AF0.inc"
};
WeaponMotion Player_State0_04_Motions0[12] = {
#include "game/008C3B10.inc"
};
WeaponMotion Player_State0_04_Motions4[12] = {
#include "game/008C3B70.inc"
};
WeaponMotion Player_State0_04_Motions2[12] = {
#include "game/008C3BD0.inc"
};
WeaponMotion Player_State0_04_Motions5[12] = {
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
void func_3A1F30(void);
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
void Player_GatherStep(Player *p);
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
extern s8 Player_State2_13_Need[];
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
void Player_SetMotionPair(Player *, s32, s32, s32); /* unit's own declaration: the last parameter s32 (the definition has s16); with the definition's type Player_State0_55 by 418 of 469 words differ */
void Player_SetMoveMode(Player *, u16 mode, s16 flag1, s16 flag2);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_StartIdleMotion(Player *p, s32 a, s32 b);
void Quest_StartCountdown(Player *p);
void Eff00_Create(Player *, u8);
void Eff22_CreateForPlayer(Player *, u8 kind);
s32 Player_IdleInput(Player *);
void Player_UpdateLean(Player *, s32);
void Player_State0_00(Player *, s32);
void Player_State0_04(Player *, u32);
void Player_State0_06(Player *, s32);
void Player_State0_15(Player *, s32);
void Player_State0_08(Player *, s32);
void Player_State0_0C(Player *, s32);
void Player_State0_0D(Player *em, s32 arg1);
void Player_State0_0B(Player *, s32);
void Player_State0_11(Player *, s32);
void Player_State0_16(Player *, s32);
void Player_State0_14(Player *, s32);
void Player_State0_19(Player *em, s32 arg1);
void Player_State0_1C(Player *, s32);
void Player_State0_1E(Player *, s32);
void Player_State0_1F(Player *, s32);
void Player_State0_20(Player *, s32);
void Player_State0_21(Player *em, s32 arg1);
void Player_State0_25(Player *em, s32 arg1);
void Player_State0_26(Player *em, s32 arg1);
void Player_State0_29(Player *, s32);
void Player_State0_2A(Player *em, s32 arg1);
void Player_State0_2B(Player *, s32);
void Player_State0_2C(Player *, s32);
void Player_State0_2E(Player *, s32);
void Player_State0_2F(Player *, s32);
void Player_State0_31(Player *, s32);
void Player_State0_34(Player *, s32);
void Player_State0_36(Player *, s32);
void Player_State0_48(Player *, s32);
void Player_State0_3C(Player *, s32);
void Player_StepBack(Player *, s32);
void Player_UpdateWallSidle(Player *em);
void Player_State0_44(Player *, s32);
void Player_State0_47(Player *, u32);
void Player_State0_5D(Player *, s32);
void Player_UpdateFishingInput(Player *);
void Player_State0_53(Player *, s32);
void Player_State0_54(Player *em, s32 arg1);
void Player_State0_55(Player *, s32);
void Player_State0_5B(Player *, s32);
void Player_State0_5E(Player *, s32);
void Player_State0_5F(Player *, s32);
void Player_State0_61(Player *, s32);
void Player_State0_6B(Player *, s32);
void Player_State0_73(Player *, s32);
void Player_State3_00(Player *, s32);
void Player_State4_00(Player *, s32);
void Player_State4_05(Player *em, s32 arg1);
void Player_UpdateCarryKind(Player *);
void Player_UpdateCarryInput(Player *, s32);
void Player_State5_01(Player *, s32);
void Player_State5_02(Player *, s32);
void Player_State5_03(Player *, s32);
void Player_State5_05(Player *, s32);
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
extern s16 Player_State6_09_MotionStates[];
u16 Rand_Next(s32 stream);
u16 Player_FindActionSpot(Player *, f32 *, u16 *arg, u8 *);
s16 Pouch_Count(Player *p, u16 item);
s16 Player_IsState(void *obj, u8 state, u8 sub);
u8 Item_CarryKind(u16);
/* end of prototypes */

void Player_State0_33(Player *p, s32 unused)
{
    p->noPushTimer = 2;
    p->fieldCollide = 0;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        p->motionTime = 0;
        Player_SetMotionPair(p, 0x1AA, -6, 0);
        p->poison = 0;
        p->mashBuildup = 0;
        p->mashLeft = 0;
        p->status7Timer = 0;
        p->status6Timer = 0;
        p->status5Buildup = 0;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 0U, 0x34U, 0);
        }
        break;
    }
}

void Player_State0_34(Player *p, s32 variant)
{
    f32 v[3];

    p->noPushTimer = 2;
    p->fieldCollide = 0;
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        p->motionTime = 0;
        p->timer = 120;
        Player_SetMotionPair(p, 0x1AB, 0, 0);
        Effect06_SpawnAtBone(p, 0, 0, 10, 4.0f);
        break;
    case 1:
        if (--p->timer <= 0 && Player_IsLocal(p) == 1) {
            Player_RequestState(p, 0, 0x35, 0);
            break;
        }
        if (p->motionId[0] == 0x1AB && !(Rand_Next(1) & 0x7F)) {
            Player_SetMotionPair(p, 0x1AC, 4, 0);
        } else if (p->motionId[0] == 0x1AC && p->motion[0].playing == 0) {
            Player_SetMotionPair(p, 0x1AB, 4, 0);
        }
        v[0] = 0.0f;
        v[1] = 20.0f;
        v[2] = 0.0f;
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(p, 4, 20, v, 0.8f);
        }
        Player_AddHp(p, 1);
        break;
    }
}

void Player_State0_35(Player *p, s32 unused)
{
    p->noPushTimer = 2;
    p->fieldCollide = 0;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        p->motionTime = 0;
        p->rot[1] = (p->rot[1] + 0x7FFF + 1) & 0xFFFF;
        p->targetYaw = (u16) p->rot[1];
        Player_SetMotionPair(p, 0x1AD, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State0_36(Player *p, s32 variant)
{
    f32 pos[3];
    u16 angle;
    s8 kind;

    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        p->weaponOut = 0;
        if (variant == 0) {
            Player_SetMotionPair(p, 0x19E, 4, 0);
            p->aimPitch = 0;
            p->aimPitchPct = 0;
        } else {
            Player_SetMotionPair(p, 0x19E, 0, 0x42);
        }
        Player_UpdateMotionTime(p, 0);
        p->timer = 60;
        p->stateVar0 = 30;
        if (Player_FindActionSpot((Player *)p, (f32 *)pos, &angle, (u8 *)&kind) == 6) {
            p->yawCenter = angle;
            break;
        }
        p->yawCenter = p->targetYaw;
        break;
    case 1:
        if (p->timer > 0) {
            p->timer--;
        }
        if (p->stateVar0 > 0) {
            p->stateVar0--;
        }
        if (Player_IsLocal(p) == 1) {
            if (Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(p, 0, 6, 0);
                break;
            }
            Player_UpdateLean(p, 0x19E);
            if (p->stateVar0 == 0 && (p->input.pressed & 0x21) && Pouch_Count(p, 0x29B) != 0) {
                Player_RequestState(p, 0, 0x48, 0x4C);
                break;
            }
            if (p->timer == 0 && (p->input.pressed2 & 0x40) && p->itemSelect == 0) {
                Player_BeginState(p, 0, 6, 0);
            }
        }
        break;
    }
}

void Player_State0_48(Player *p, s32 variant)
{

    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x19E, 0, 0x42);
        Player_UpdateMotionTime(p, 0);
        p->timer = 60;
        if (Player_IsLocal(p) != 1 || Pouch_Count(p, 0x29B) != 0) {
            Pouch_Add(p, 0x29B, -1);
            Vib_StartLocal(p, 2);
            Eff22_CreateForPlayer(p, 0);
        }
        break;
    case 1:
        p->timer -= 1;
        if (p->timer == 0) {
            if (Pouch_Count(p, 0x29B) > 0) {
                Player_RequestState(p, 0, 0x7A, 0xC);
                break;
            }
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State0_37(Player *p, s32 unused)
{
    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0xD3, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Vib_StartLocal(p, 3);
        CamShake_ReqA(p, 5);
        Eff22_CreateForPlayer(p, 1U);
        break;
    case 1:
        if (Motion_PassedFrame(p, 0, p->motion[0].frameCount) != 0) {
            p->step++;
            Player_SetMotionPair(p, 0xD4, 2, 0);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State0_39(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->motionTime = 0;
        Player_SetMoveMode(p, 1, 0, 0);
        Player_SetMotionPair(p, 9, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State0_3A(Player *p, s32 unused)
{
    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->motionTime = 0;
        Player_SetMotionPair(p, 0x12, 4, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State0_3B(Player *p, s32 unused)
{
    s32 timer;

    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMotionPair(p, 0xA, 8, 0);
        Player_SetMoveMode(p, 1, 0, 0);
        p->weaponOut = 0;
        Player_SetFlag(p, 0x1000U);
        p->freeState = 1;
        p->timer = 0xF;
        break;
    case 1:
        timer = p->timer;
        if (timer != 0) {
            p->timer = timer - 1;
        }
        if (Player_IsLocal(p) == 1) {
            if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B) && (p->unk668 == 0)) {
                Player_SetState(p, 0U, 0x2DU, 4);
                break;
            }
            Player_IdleInput(p);
        } else {
        }
        break;
    }
}

void Player_State0_3C(Player *p, s32 variant)
{
    s16 blend;

    p->timer++;
    p->freeState = 1;
    switch (p->step) {
    case 0:
        p->step++;
        if (p->motionId[0] == 0x25) {
            blend = 12;
        } else {
            blend = 8;
        }
        Player_SetMoveMode(p, 0x8001, 0, 0);
        Player_SetMotionPair(p, 8, blend, 0);
        p->timer = 0;
        break;
    case 1:
        if (p->timer >= 12) {
            Player_SetState(p, 0, 0x1D, 0);
        }
        break;
    }
}

void Player_State0_3D(Player *p, s32 unused)
{
    s32 timer;

    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMotionPair(p, 0x25, 6, 0);
        Player_SetMoveMode(p, 1, 0, 0);
        Player_SetFlag(p, 0x02000000U);
        p->freeState = 1;
        p->timer = 0xF;
        break;
    case 1:
        timer = p->timer;
        if (timer != 0) {
            p->timer = timer - 1;
        }
        if (Player_IsLocal(p) == 1) {
            if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B) && (p->unk668 == 0)) {
                Player_SetState(p, 0U, 0x2DU, 4);
                break;
            }
            Player_IdleInput(p);
        } else {
        }
        break;
    }
}

void Player_State0_3E(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMotionPair(p, 0x29, 2, 0);
        Player_SetMoveMode(p, 0x8000, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
        }
        break;
    }
}

void Player_State0_3F(Player *p, s32 unused)
{
    p->freeState = 1;
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        p->rot[1] = (s32) p->targetYaw;
        Player_SetMotionPair(p, 0x2A, 0, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x1000);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 0U, 0x24U, 0);
        }
        break;
    }
}

/* moves the player 3 units back along its heading (forward when fwd is set) */
void Player_StepBack(Player *p, s32 fwd)
{
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    f32 m[16];

    ang[0] = 0;
    ang[2] = 0;
    ang[1] = p->rot[1];
    Mtx_FromAnglesXYZ(ang, m);
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = -3.0f;
    if ((s16) fwd) {
        v[2] *= -1.0f;
    }
    Vec_Rotate(d, v, m);
    p->pos[0] += d[0];
    p->pos[2] += d[2];
}

void Player_UpdateWallSidle(Player *p)
{
    s32 d;

    if (Player_IsLocal(p) != 0) {
        switch (Player_StickMoveLevel(p, 0)) {
        case 1:
        case 2:
        case 3:
            d = (u16) ((u16) Player_GetStickYaw(p, 0) - *(u16 *) &p->rot[1]);
            if (d >= 0x2000 && d <= 0x6000) {
                if (Player_IsState(p, 0, 0x42) == 0) {
                    Player_SetState(p, 0, 0x42, 0);
                }
            } else if (d >= 0xA000 && d <= 0xE000) {
                if (Player_IsState(p, 0, 0x41) == 0) {
                    Player_SetState(p, 0, 0x41, 0);
                }
            } else if (Player_IsState(p, 0, 0x40) == 0 && Player_IsState(p, 0, 0x43) == 0) {
                Player_SetState(p, 0, 0x43, 0);
            }
            break;
        default:
            if (Player_IsState(p, 0, 0x40) == 0 && Player_IsState(p, 0, 0x43) == 0) {
                Player_SetState(p, 0, 0x43, 0);
            }
            break;
        }
        if ((p->input.pressed & 0x40) || !(Player_TurnAlongWallSide(p, 1) & 0xFF)) {
            p->freeState = 1;
            Player_RequestState(p, 0, 0x2C, 0);
        }
    }
}

void Player_State0_40(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        if (variant == 0) {
            Player_SetMotionPair(p, 0x2B, 2, 0);
        } else {
            Player_SetMotionPair(p, 0x2B, 2, 0x1E);
        }
        Player_SetMoveMode(p, 0x8000, 0, 0);
        Player_StepBack(p, 0);
        break;
    case 1:
        Player_StepBack(p, 0);
        if (Motion_ReachedFrame(p, 0, 30.0f) != 0) {
            Player_UpdateWallSidle(p);
        }
        break;
    }
}

void Player_State0_41(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        if (variant == 0) {
            Player_SetMotionPair(p, 0x2C, 2, 0);
        } else {
            Player_SetMotionPair(p, 0x2D, 2, 0);
        }
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x1000);
        Player_StepBack(p, 0);
        break;
    case 1:
        Player_StepBack(p, 0);
        Player_UpdateWallSidle(p);
        break;
    }
}

void Player_State0_44(Player *p, s32 unused)
{
    f32 v[3];
    f32 d[3];

    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMotionPair(p, 0x2E, 4, 0);
        Player_SetMoveMode(p, 2, 0, 0);
        Player_AddStamina(p, -75);
        Player_PlaySe(p, 0x1E, 0, p->pos, 1, 0);
        p->noHitTimer = 3;
        break;
    case 1:
        p->noHitTimer = 3;
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x2F, 0, 0);
            p->pos[1] += 100.0f;
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 16.0f;
            Vec_Rotate(d, v, (f32 *) p->worldMtx);
            p->vel[0] = d[0];
            p->vel[1] = 3.0f;
            p->vel[2] = d[2];
            Player_ClearAccel(p);
            Player_StartJump(p, 8);
        }
        break;
    case 2:
        p->noHitTimer = 3;
        Player_MoveAccel(p);
        if (p->vel[1] <= 0.0f && p->pos[1] <= p->floorY) {
            p->step++;
            p->action = 0;
            p->pos[1] = p->floorY;
            Player_SetMotionPair(p, 0x30, 0, 0);
            Vib_StartLocal(p, 1);
            CamShake_ReqA(p, 0);
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            p->noHitTimer = 0;
            if (p->input.held & 0x10) {
                Player_RequestState(p, 0, 0x45, 0);
            } else {
                p->freeState = 1;
                Player_RequestState(p, 0, 0x2C, 0);
            }
        } else if (Motion_ReachedFrame(p, 0, 30.0f) == 0) {
            p->noHitTimer = 2;
        }
        break;
    }
}

extern s32 gNothingMoreText;
/* gathering at the player's spot (kind 0: a gather point, 1-3: a mining point); stat: 1, 2 count quest stats 0xE, 0xF. 1: an item was taken */
s32 Player_GatherAtSpot(Player *p, u16 kind, u16 stat)
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
        item = Gather_AtPoint(p);
        break;
    case 1:
    case 2:
    case 3:
        item = Gather_AtMinePoint(p);
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
            Jingle_Play(p, 0xA);
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
        r = Pouch_GiveItem(p, item, p->gotItem, 3);
        switch (r) {
        case 0:
        case 1:
        case 2:
            if (Item_CarryKind(item) & 0xFF) {
                p->stateVar1 = 1;
                MsgEvent_Check(0x10);
                if (item == 0x2A3) {
                    Notice_Push(1, 0x14, item);
                }
            }
            sel = p->pouchCursor;
            id = p->pouch[sel].item;
            if (id != 0 && !(gItemData[id].flags & 1)) {
                p->pouchCursor = Pouch_NextSelectable(p, sel, 0);
            }
            return 1;
        }
    }
    return 0;
}

void Player_State0_47(Player *p, u32 variant)
{
    u16 n;

    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        p->timer = 0;
        p->motionTime = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 0x3C;
        switch (variant) {
        case 0:
        case 1:
        case 6:
        case 7:
            Player_SetMotionPair(p, 0x195, -0xE, 0);
            if (Player_HasSkill(p, 0x97) == 1) {
                p->motion[0].step = 2.6f;
                p->motion[1].step = 2.6f;
            }
            break;
        case 2:
            if (Player_IsLocal(p) == 1) {
                Notice_Push(0, 0xA, 0);
            }
            Player_SetMotionPair(p, 0x195, -0xE, 0);
            if (Player_HasSkill(p, 0x97) == 1) {
                p->motion[0].step = 2.6f;
                p->motion[1].step = 2.6f;
            }
            break;
        case 3:
            Player_SetMotionPair(p, 0x1A3, -6, 0);
            break;
        case 4:
            Player_SetMotionPair(p, 0x19F, 0, 0);
            if (Player_HasSkill(p, 0x97) == 1) {
                p->timer = 0x28;
                p->motion[0].step = 2.8f;
                p->motion[1].step = 2.8f;
            }
            break;
        case 5:
            Player_SetMotionPair(p, 0x1A2, 0, 0);
            if (Player_HasSkill(p, 0x97) == 1) {
                p->timer = 0x28;
                p->motion[0].step = 2.8f;
                p->motion[1].step = 2.8f;
            }
            break;
        }
        break;
    case 1:
        if (Player_IsLocal(p) == 1) {
            if (p->timer > 0) {
                p->timer--;
            }
            if (p->timer == 0 && p->stateVar0 == 0 && Quest_PhaseIsOver(1) == 0) {
                n = 1;
                p->stateVar0 = 1;
                switch (variant) {
                case 4:
                case 5:
                    switch (p->useItem) {
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
                    if (Player_HasSkill(p, 0x9A) == 1 && (Rand_Next(1) & 3) == 0) {
                        n = n / 2U + 1;
                    }
                    if (Player_HasSkill(p, 0x9B) == 1 && (Rand_Next(1) & 1) == 0) {
                        n = n / 2U + 1;
                    }
                    if (Rand_Next(1) % n == 0
                        && (Player_HasSkill(p, 0x98) != 1 || (Rand_Next(1) & 3))
                        && (Player_HasSkill(p, 0x99) != 1 || (Rand_Next(1) & 1))) {
                        Notice_Push(1, 0xE, p->useItem);
                        Pouch_Add(p, p->useItem, -1);
                    }
                default:
                    switch (variant) {
                    default:
                        Player_GatherAtSpot(p, 0, 0);
                        break;
                    case 4:
                        Player_GatherAtSpot(p, 0, 1);
                        break;
                    case 5:
                        Player_GatherAtSpot(p, 0, 2);
                        break;
                    }
                    break;
                case 6:
                    Player_GatherAtSpot(p, 2, 0);
                    break;
                case 7:
                    Player_GatherAtSpot(p, 3, 0);
                    break;
                case 2:
                    Quest_GiveRewards(p);
                    break;
                case 3:
                    Player_GatherAtSpot(p, 1, 0);
                    break;
                }
                break;
            }
        }
        if (p->motion[0].playing == 0) {
            if (p->stateVar1 == 0) {
                switch (variant) {
                case 0:
                case 1:
                case 2:
                case 6:
                case 7:
                    Player_BeginState(p, 0, 0xA, 0);
                    break;
                case 3:
                case 5:
                    Player_BeginState(p, 0, 6, 0);
                    break;
                default:
                    Player_BeginState(p, 0, 4, 0);
                    break;
                }
            } else {
                Player_UpdateCarryKind(p);
                Player_RequestState(p, 5, 9, 0);
            }
        } else if (Motion_PassedFrame(p, 0, 100.0f) != 0 && variant == 3 && Player_HasSkill(p, 0x97) == 1) {
            Player_SetMotionPair(p, 0x1A3, -8, 0xE0);
        }
        break;
    }
}

void Player_State0_5D(Player *p, s32 variant)
{
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    p->noPushTimer = 2;
    p->timer++;
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        p->timer = 0;
        p->motionTime = 0;
        Player_SetMoveMode(p, 1, 0, 0);
        switch (variant) {
        case 0:
        case 2:
        case 3:
            Player_SetMotionPair(p, 0x19B, -6, 0);
            break;
        case 1:
            Player_SetMotionPair(p, 0x1A6, -6, 0);
            break;
        }
        if (Player_HasSkill(p, 0x97) == 1) {
            p->motion[0].step = 2.6f;
            p->motion[1].step = 2.6f;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            switch (variant) {
            case 0:
            case 2:
            case 3:
                Player_SetMotionPair(p, 0x195, -4, 0x24);
                break;
            case 1:
                Player_SetMotionPair(p, 0x1A3, 0, 0x44);
                break;
            }
        }
        break;
    case 2:
        if ((Player_IsLocal(p) == 1) && (p->timer >= 0x3C) && (p->stateVar0 == 0) && (Quest_PhaseIsOver(1) == 0)) {
            p->stateVar0 = 1;
            Player_GatherAtSpot(p, variant & 0xFFFF, 0);
        }
        switch (variant) {
        case 0:
        case 2:
        case 3:
            if (Motion_ReachedFrame(p, 0, 200.0f) != 0) {
                p->step++;
                Player_SetMotionPair(p, 0x19C, -4, 0);
            }
            break;
        case 1:
            if (Motion_ReachedFrame(p, 0, 294.0f) != 0) {
                p->step++;
                Player_SetMotionPair(p, 0x1A7, -2, 0);
            }
            break;
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            if (p->stateVar1 == 0) {
                Player_RequestState(p, 0U, 0x1DU, 2);
                break;
            }
            Player_UpdateCarryKind(p);
            Player_RequestState(p, 5U, 9U, 0);
        }
        break;
    }
}

void Player_State0_4C(Player *p, s32 unused)
{
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        p->weaponOut = 0;
        p->cookTimer = 0;
        p->cookResult = 0;
        Player_SetMotionPair(p, 0x320, -4, 0);
        CamCtl_TurnToObj_Unk((u8 *) p);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 0U, 0x55U, 2);
        }
        break;
    }
}

void Player_State0_4B(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(p, 0x321, 0, 0);
            break;
        }
        Player_SetMotionPair(p, 0x322, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_UpdateFishingInput(Player *p)
{

    if (Player_IsLocal(p) != 0) {
        if (p->stateGroup != 0) {
            Player_BeginState(p, 0, 4, 0);
            return;
        }
        if (Quest_PhaseIsOver(1) == 1) {
            Player_BeginState(p, 0, 4, 0);
            return;
        }
        switch (p->state) {
        case 0x4E:
        case 0x4F:
        case 0x53:
            break;
        case 0x50:
            if (p->input.pressed & 0x20) {
                if (p->fishTimer > 0) {
                    Player_RequestState(p, 0, 0x53, 0);
                } else if (p->baited == 1) {
                    Player_RequestState(p, 0, 0x64, 0);
                } else {
                    Player_RequestState(p, 0, 0x54, 0);
                }
            }
            break;
        case 0x51:
        case 0x52:
            if (p->input.pressed & 0x20) {
                Player_RequestState(p, 0, 0x53, 0);
            }
            break;
        }
    }
}

void Player_State0_4E(Player *p, s32 variant)
{
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x80000U);
        p->weaponOut = 0;
        p->timer = 0;
        p->stateVar1 = 0;
        if (variant == 0) {
            Player_SetMotionPair(p, 0x324, 0, 0);
            break;
        }
        Player_SetMotionPair(p, 0x326, 2, 0);
        break;
    case 1:
        if (((s8) p->fishTimer != 0) && (p->stateVar1 == 0)) {
            p->stateVar1 = 1;
        }
        Player_UpdateFishingInput(p);
        break;
    }
}

void Player_State0_4F(Player *p, s32 unused)
{
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    p->noPushTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x80000);
        p->weaponOut = 0;
        p->fishTimer = 0;
        p->gotItemId = 0;
        p->baited = 1;
        Player_SetMotionPair(p, 0x325, -2, 0);
        break;
    case 1:
        if (Motion_PassedFrame(p, 0, 104.0f) != 0) {
            if (p->useItem == 0x91) {
                Lure_Spawn(p, 1);
            } else {
                Lure_Spawn(p, 0);
            }
        }
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 0U, 0x50U, 0);
        }
        break;
    }
}

void Player_State0_53(Player *p, s32 variant)
{
    Em *e = gEms;
    s16 i;

    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    p->noPushTimer = 2;
    p->noHitTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        p->msgTimer = 0;
        p->motionTime = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x80000);
        p->weaponOut = 0;
        for (i = 0; i < 20; i++, e++) {
            if (e->active != 0 && e->area == p->area && e->hooked != 0 && e->hookedBy == p->index) {
                p->stateVar0 = 1;
            }
        }
        Player_SetMotionPair(p, 0x327, 2, 0);
        break;
    case 1:
        if (p->stateVar0 == 0) {
            if (Motion_PassedFrame(p, 0, 118.0f) != 0) {
                p->step++;
                Player_SetMotionPair(p, 0x328, 2, 0);
                break;
            }
            for (i = 0; i < 20; i++, e++) {
                if (e->area == p->area && e->hooked != 0 && e->hookedBy == p->index) {
                    p->stateVar0 = 1;
                }
            }
            break;
        }
        if (Motion_PassedFrame(p, 0, p->motion[0].frameCount) != 0) {
            p->stateVar1++;
            if (p->stateVar1 >= 2) {
                p->step = 3;
                Player_SetMotionPair(p, 0x32A, 4, 0);
                if (Quest_PhaseIsOver(1) == 0) {
                    Pouch_Add(p, p->useItem, -1);
                }
            }
        }
        break;
    case 2:
        if (Motion_PassedFrame(p, 0, 120.0f) != 0 && Player_IsLocal(p) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(1, 0xB, p->gotItemId);
            Jingle_Play(p, 7);
            Pouch_Add(p, p->useItem, -1);
            i = p->gotItem;
            if ((s16) Pouch_GiveItem(p, p->gotItemId, i, 0) == 3) {
                Player_PushNotice3(p->gotItemId);
            }
            QuestStats_Add(0xD, 1);
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 2, 0);
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0xD8, 2, 0x24);
        }
        break;
    case 4:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State0_54(Player *p, s32 variant)
{
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x80000U);
        p->weaponOut = 0;
        Player_SetMotionPair(p, 0x329, 2, 0);
        break;
    case 1:
        if ((Motion_PassedFrame(p, 0, 120.0f) != 0) && (Player_IsLocal(p) == 1) && (Quest_PhaseIsOver(1) == 0)) {
            if (variant == 0) {
                Jingle_Play(p, 0xA);
                Pouch_Add(p, (s32) p->useItem, -1);
                Notice_Push(0, 3, 0);
            } else {
                Notice_Push(0, 0xD, 0);
            }
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 2, 0);
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

void Player_State0_55(Player *p, s32 unused)
{
    SpitMeat *m;
    s16 done;
    s16 doneLen;
    s16 gourmet;
    s16 gourmetLen;
    s16 limit;
    s32 j;
    s16 n;
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    p->noPushTimer = 2;
    m = &gSpitMeats[SpitMeat_Kind(p->useItem)];
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        p->weaponOut = 0;
        Player_SetMotionPair(p, 0x323, 4, Rand_Next(1) & 0x3F);
        p->cookTimer = 0;
        p->cookResult = 0;
        Jingle_Play(p, m->jingle);
        break;
    case 1:
        p->cookTimer++;
        done = m->done;
        doneLen = m->doneLen;
        gourmet = m->gourmet;
        gourmetLen = m->gourmetLen;
        if (Player_HasSkill(p, 0xC6) == 1) {
            done -= 2;
            doneLen += 4;
            if (gourmet > 0) {
                gourmet -= 1;
                gourmetLen += 2;
            }
        } else if (Player_HasSkill(p, 0xC7) == 1) {
            done -= 4;
            doneLen += 8;
            if (gourmet > 0) {
                gourmet -= 2;
                gourmetLen += 4;
            }
        } else if (Player_HasSkill(p, 0xC8) == 1) {
            doneLen /= 2;
            if (gourmet > 0) {
                gourmet /= 2;
                gourmetLen /= 2;
            }
        }
        if (p->cookTimer >= done + doneLen) {
            p->cookResult = 3;
        } else if (p->cookTimer >= done) {
            if (gourmet > 0 && p->cookTimer >= done + gourmet && p->cookTimer < gourmetLen + (done + gourmet)) {
                p->cookResult = 4;
            } else {
                p->cookResult = 2;
            }
        } else if (p->cookTimer >= m->rare) {
            p->cookResult = 1;
        } else {
            p->cookResult = 0;
        }
        if (SpitMeat_Kind(p->useItem) == 2) {
            limit = 180;
        } else {
            limit = 330;
        }
        if (Player_IsLocal(p) == 1 && ((p->input.pressed & 0x20) || p->cookTimer >= limit)) {
            Bgm_StopEvent(p);
            switch (p->cookResult) {
            case 0:
                if (Quest_PhaseIsOver(1) == 0) {
                    Notice_Push(0, 0xB, 0);
                }
                Player_RequestState(p, 0, 0x4B, 0);
                break;
            case 1:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (m->raw == 0) {
                        Pouch_Add(p, 0x28, -1);
                    } else {
                        Pouch_Add(p, p->useItem, -1);
                        Pouch_Add(p, 0xA2, 1);
                    }
                    Notice_Push(1, 9, m->itemRare);
                    Jingle_Play(p, 0xE);
                    n = p->gotItem;
                    if ((s16) Pouch_GiveItem(p, m->itemRare, n, 0) == 3) {
                        Player_PushNotice3(m->itemRare);
                    }
                }
                Player_RequestState(p, 0, 0x4B, 0);
                break;
            case 2:
            case 4:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (m->raw == 0) {
                        Pouch_Add(p, 0x28, -1);
                    } else {
                        Pouch_Add(p, p->useItem, -1);
                        Pouch_Add(p, 0xA2, 1);
                    }
                    if (p->cookResult == 4) {
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
                    Jingle_Play(p, j);
                    if (p->cookResult == 4) {
                        Notice_Push(1, 9, m->itemGourmet);
                        n = p->gotItem;
                        if ((s16) Pouch_GiveItem(p, m->itemGourmet, n, 0) == 3) {
                            Player_PushNotice3(m->itemGourmet);
                        }
                    } else {
                        Notice_Push(1, 9, m->itemDone);
                        n = p->gotItem;
                        if ((s16) Pouch_GiveItem(p, m->itemDone, n, 0) == 3) {
                            Player_PushNotice3(m->itemDone);
                        }
                    }
                }
                Player_RequestState(p, 0, 0x4D, 0);
                break;
            case 3:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (m->raw == 0) {
                        Pouch_Add(p, 0x28, -1);
                    } else {
                        Pouch_Add(p, p->useItem, -1);
                        Pouch_Add(p, 0xA2, 1);
                    }
                    Notice_Push(1, 0xA, m->itemBurnt);
                    Jingle_Play(p, 0x10);
                    n = p->gotItem;
                    if ((s16) Pouch_GiveItem(p, m->itemBurnt, n, 0) == 3) {
                        Player_PushNotice3(m->itemBurnt);
                    }
                }
                Player_RequestState(p, 0, 0x4B, 0);
                break;
            }
        }
        break;
    }
}

void Player_State0_56(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x19F, 0, 0);
        if (variant == 0) {
            Quest_StartCountdown((Player *) p);
            break;
        }
        Eff22_CreateForPlayer(p, 7U);
        Pouch_Add(p, (s32) p->useItem, -1);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State0_57(Player *p, s32 variant)
{
    u8 var_v0;

    p->noPushTimer = 2;
    p->noHitTimer = 5;
    switch (p->step) {
    case 0:
        p->step++;
        if (p->motionId[0] != 1) {
            Player_SetMotionPair(p, 1, 6, 0);
        }
        if (variant == 0) {
            p->supplyBoxOpen = 1;
        } else {
            p->deliveryOpen = 1;
            p->menuCursor = 0;
            p->menuPage = 0;
        }
        p->menuDelay = 0;
        if (Player_IsLocal(p) == 1) {
            Sound_PlaySeFlat(1, 0x8E, 0);
        }
        break;
    case 1:
        if (variant == 0) {
            var_v0 = p->supplyBoxOpen;
        } else {
            var_v0 = p->deliveryOpen;
        }
        if (var_v0 & 0xFF) {
            if (Quest_PhaseIsOver(1) == 1) {
                goto block_17;
            }
        } else {
block_17:
            p->noHitTimer = 0;
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State0_58(Player *p, s32 variant)
{
    u8 stateVar0;

    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        switch (variant) {
        case 0:
            Player_SetMotionPair(p, 0x196, 4, 0);
            break;
        case 1:
            Player_SetMotionPair(p, 0x1A0, 4, 0);
            break;
        }
        p->motionTime = 0;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            if (Player_IsLocal(p) == 1) {
                Player_RequestState(p, 0U, 0x71U, 0);
                break;
            }
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (variant == 1) {
            if ((Player_HasSkill(p, 0xBD) == 1) && (Motion_PassedFrame(p, 0, 62.0f) != 0)) {
                stateVar0 = p->stateVar0;
                if ((s32) stateVar0 < 4) {
                    p->stateVar0 = stateVar0 + 1;
                    p->stateVar1 = 1;
                    Player_SetMotionPair(p, 0x1A0, 4, 0x34);
                }
            }
            if ((Motion_ReachedFrame(p, 0, 38.0f) != 0) && (Motion_ReachedFrame(p, 0, 180.0f) == 0) && (Player_HasSkill(p, 0xBC) == 1)) {
                p->motion[0].step = 3.6f;
                p->motion[1].step = 3.6f;
                break;
            }
            p->motion[0].step = 2.0f;
            p->motion[1].step = 2.0f;
        } else {
        }
        break;
    }
}

void Player_State0_71(Player *p, s32 unused)
{
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        if (Player_IsLocal(p) == 1) {
            Item_Use(p);
        }
        p->motionTime = 0;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State0_59(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 1, 0, 0);
        Player_SetMotionPair(p, 0x199, 2, 0);
        p->motionTime = 0;
        break;
    case 1:
        if (Motion_PassedFrame(p, 0, 20.0f) != 0) {
            if (Player_IsLocal(p) == 1) {
                Player_RequestState(p, 0U, 0x14U, 0xC);
                break;
            }
            p->step++;
            Player_SetMotionPair(p, 8, 6, 0);
        } else {
        case 2:
            break;
        }
        break;
    }
}

static void Player_StartTradeTimer(Player *p)
{
    if (p->tradeCooldown <= 0) {
        p->tradeCooldown = 9000;
    }
}

void Player_State0_5B(Player *p, s32 variant)
{

    p->noHitTimer = 3;
    switch (p->step) {
    case 0:
        p->step++;
        if (p->motionId[0] != 1) {
            Player_SetMotionPair(p, 1, -6, 0);
        }
        p->tradeMsgTimer = 60;
        if (p->tradeCount < 7) {
            p->tradeCount++;
            if (p->tradeCount >= 4 && (Rand_Next(1) & 1)) {
                p->tradeCount = 7;
                Player_StartTradeTimer(p);
            }
        } else {
            p->tradeCount = 7;
            Player_StartTradeTimer(p);
        }
        p->timer = 60;
        break;
    case 1:
        if (p->tradeMsgTimer == 0) {
            Player_BeginState(p, 0, 2, 0);
        }
        break;
    }
}

void Player_State0_5E(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(p, 0x1A4, 4, 0);
            Player_SetFlag(p, 0x10000);
        } else {
            Player_SetMotionPair(p, 0x1A8, 4, 0);
        }
        p->motionTime = 0;
        break;
    case 1:
        switch (variant) {
        case 0:
            if (p->stateVar0 >= 3 || (Motion_ReachedFrame(p, 0, 44.0f) != 0 && (p->input.pressed & 0x40))) {
                Player_RequestState(p, 0, 0x5F, 0);
            } else if (Motion_PassedFrame(p, 0, p->motion[0].frameCount) != 0) {
                p->stateVar0++;
            }
            break;
        case 1:
            if (Motion_PassedFrame(p, 0, p->motion[0].frameCount) != 0) {
                p->stateVar0++;
                if (p->stateVar0 >= 2) {
                    Player_RequestState(p, 0, 0x63, 0);
                }
            }
            break;
        }
        break;
    }
}

void Player_State0_5F(Player *p, s32 variant)
{
    u16 chance = 1;

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        if (variant == 0) {
            chance = 5;
            Player_SetMotionPair(p, 0x1A5, 2, 0);
        } else {
            Player_SetMotionPair(p, 0x1A9, 4, 0);
            if (Player_IsLocal(p) == 1) {
                Item_UseTeamPowder(p);
            }
            switch (p->useItem) {
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
        if (Player_IsLocal(p) == 1 && Quest_PhaseIsOver(1) == 0) {
            if (Player_HasSkill(p, 0x9A) == 1 && !(Rand_Next(1) & 3)) {
                chance = chance / 2 + 1;
            }
            if (Player_HasSkill(p, 0x9B) == 1 && !(Rand_Next(1) & 1)) {
                chance = chance / 2 + 1;
            }
            if (Rand_Next(1) % chance == 0) {
                if (Player_HasSkill(p, 0x98) != 1 || (Rand_Next(1) & 3)) {
                    if ((Player_HasSkill(p, 0x99) != 1 || (Rand_Next(1) & 1)) && (p->weaponType != 8 || (Rand_Next(1) & 3))) {
                        Notice_Push(1, 0xE, p->useItem);
                        Pouch_Add(p, p->useItem, -1);
                    }
                }
            }
        }
        p->motionTime = 0;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State0_61(Player *p, s32 unused)
{
    s16 n;

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        p->weaponOut = 0;
        Player_SetMotionPair(p, 0x19D, 4, 0);
        p->motionTime = 0;
        break;
    case 1:
        if (Player_HasSkill(p, 0x26) == 1 && Motion_PassedFrame(p, 0, 98.0f) != 0) {
            Player_SetMotionPair(p, 0x19D, 0, 0xEE);
        }
        if (Motion_PassedFrame(p, 0, 260.0f) != 0 && Quest_PhaseIsOver(1) == 0) {
            Pouch_Add(p, p->useItem, -1);
            switch (p->useItem) {
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
            if (Player_HasSkill(p, 0x27) == 1) {
                n = n / 2;
            }
            Player_AddSharpness(p, n);
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State0_65(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 1, 0, 0);
        p->aimMode = 1;
        p->weaponOut = 0;
        if (variant == 0) {
            Player_SetMotionPair(p, 7, 2, 0);
            break;
        }
        p->step++;
        Player_SetMotionPair(p, 8, 2, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 8, 6, 0);
            Player_UpdateLean(p, 8);
            break;
        }
        Player_UpdateLean(p, 7);
        break;
    case 2:
        Player_UpdateLean(p, 8);
        if (Player_IsLocal(p) != 0) {
            if (!(p->input.pressed & 0x40)) {
                if (Quest_PhaseIsOver(1) == 1) {
                    goto block_15;
                }
            } else {
block_15:
                Player_ResetAim(p);
                Player_RequestState(p, 0U, 0x1DU, 0);
            }
        }
        break;
    }
}

void Player_State0_67(Player *p, s32 unused)
{
    p->offerStarted = 1;
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    Player_ResetAim(p);
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x3C, 4, 0);
        p->offerState = 0U;
        break;
    case 1:
        if ((Player_IsLocal(p) == 1) && ((p->input.pressed & 0x40) || (p->offerState != 0))) {
            Player_RequestState(p, 0U, 0x69U, 0);
        }
        break;
    }
}

void Player_State0_68(Player *p, s32 unused)
{
    s32 timer;

    p->offerStarted = 1;
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    Player_ResetAim(p);
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_UpdateMotionTime(p, 0);
        if (p->motionId[0] != 1) {
            Player_SetMotionPair(p, 1, 6, 0);
        }
        p->offerState = 0U;
        p->timer = 0;
        break;
    case 1:
        timer = p->timer + 1;
        p->timer = timer;
        if ((timer >= 0x259) || (p->offerState != 0)) {
            if (p->offerState == 1) {
                Player_RequestState(p, 0U, 0x6AU, 0);
                break;
            }
            Player_BeginState(p, 0, 2, 0);
        } else {
        }
        break;
    }
}

void Player_State0_69(Player *p, s32 unused)
{
    p->offerStarted = 1;
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    Player_ResetAim(p);
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMotionPair(p, 0x3D, 4, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State0_6A(Player *p, s32 unused)
{
    p->offerStarted = 1;
    ItemGet_Reserve((s32) p, (u16 *) &p->gotItem);
    Player_ResetAim(p);
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMotionPair(p, 0x280, 4, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State0_6B(Player *p, s32 variant)
{
    f32 v[3];
    f32 d[3];
    u16 a;
    u16 b;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetEvadeFrames(p);
        Player_AddStamina(p, -75);
        Player_PlaySe(p, 0x1E, 0, p->pos, 1, 0);
        p->stateVar1 = 0xFF;
        switch (variant) {
        case 0:
            Player_SetMotionPair(p, 0x3F6, 2, 0);
            break;
        case 1:
            Player_SetMotionPair(p, 0x3F7, 2, 0);
            break;
        case 2:
        case 3:
            Player_SetMotionPair(p, 0x3F8, 2, 0);
            break;
        }
        break;
    case 1:
        switch (variant) {
        case 0:
        case 1:
            if (p->weaponType == 9 && Motion_ReachedFrame(p, 0, 20.0f) == 0) {
                if (variant == 0) {
                    v[0] = 9.0f;
                } else {
                    v[0] = -9.0f;
                }
                v[1] = 0.0f;
                v[2] = 0.0f;
                Vec_Rotate(d, v, p->rotMtx);
                p->pos[0] += d[0];
                p->pos[2] += d[2];
            }
            break;
        case 3:
            if (Motion_ReachedFrame(p, 0, 28.0f) == 0) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = -8.0f;
                Vec_Rotate(d, v, p->rotMtx);
                p->pos[0] += d[0];
                p->pos[2] += d[2];
            }
            break;
        }
        if (Motion_ReachedFrame(p, 0, p->motion[0].frameCount - 30.0f) != 0 && Player_IsLocal(p) == 1) {
            if (p->input.stickDirPressed & 0x3C) {
                a = p->input.prevStickAngle + 0x1555;
                switch (p->weaponType) {
                case 3:
                    if (a < 0xAAAC) {
                        p->stateVar1 = 0;
                    } else {
                        p->stateVar1 = 1;
                    }
                    break;
                case 9:
                    if (a < 0xAAAC) {
                        p->stateVar1 = 0;
                    } else if (p->roundsLoaded < p->clipSize) {
                        p->stateVar1 = 0x1B;
                    } else {
                        p->stateVar1 = 4;
                    }
                    break;
                }
            }
            if (Motion_ReachedFrame(p, 0, p->motion[0].frameCount - 10.0f) != 0 && p->stateVar1 != 0xFF) {
                if (p->input.stickDir & 0x3C00) {
                    b = p->input.stickAngle - 0x1555;
                    if (b >= 0xAAAB) {
                        if (p->weaponType == 9) {
                            p->targetYaw -= 0x1555;
                        } else {
                            p->targetYaw -= 0x1000;
                        }
                    } else if (b >= 0x5555) {
                        if (p->weaponType == 9) {
                            p->targetYaw += 0x1555;
                        } else {
                            p->targetYaw += 0x1000;
                        }
                    }
                }
                Player_SetState(p, 1, p->stateVar1, 0xC);
            }
        }
        if (p->motion[0].playing == 0) {
            if (variant == 2) {
                Player_BeginState(p, 0, 6, 0);
            } else {
                Player_BeginState(p, 0, 8, 0);
            }
        }
        break;
    }
}

void Player_State0_6F(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        p->timer = 0;
        Player_SetMotionPair(p, 0x1B0, 6, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(p, 0, 78.0f) != 0) {
            if (Player_IsLocal(p) == 1) {
                Player_RequestState(p, 0U, 0x70U, 0xC);
                break;
            }
            p->step++;
        }
        break;
    case 2:
        Player_SetMotionPair(p, 0x1B0, 0, 0x4E);
        break;
    }
}

void Player_State0_70(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->motionTime = 0;
        p->timer = 0;
        Player_SetMotionPair(p, 0x1B0, 0, 0x4E);
        Player_SetMoveMode(p, 0, 0, 0);
        Player_UpdateMotionTime(p, 0);
        if (Player_IsLocal(p) == 1) {
            Item_UsePlaced(p);
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
        }
        break;
    }
}

void Player_State0_73(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 1;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x1A4, 4, 0);
        Player_SetFlag(p, 0x10000);
        p->motionTime = 0;
        switch (p->useItem) {
        case 0x2B0:
        case 0x2B1:
        case 0x2B4:
        case 0x2B5:
            p->stateVar0 = 2;
            p->stateVar1 = 16;
            break;
        case 0x2B2:
            p->stateVar0 = 1;
            p->stateVar1 = 12;
            break;
        case 0x2B3:
        case 0x2B6:
            p->stateVar0 = 3;
            p->stateVar1 = 8;
            break;
        }
        break;
    case 1:
        if (p->stateVar0 <= 0) {
            p->step++;
            Player_SetMotionPair(p, 0x1A5, 2, 0);
            if (Player_IsLocal(p) == 1 && Quest_PhaseIsOver(1) == 0 && Rand_Next(1) % p->stateVar1 == 0) {
                if (Player_HasSkill(p, 0x98) == 1) {
                    if (!(Rand_Next(1) & 3)) {
                        break;
                    }
                }
                if (Player_HasSkill(p, 0x99) == 1) {
                    if (!(Rand_Next(1) & 1)) {
                        break;
                    }
                }
                if (p->weaponType == 8) {
                    if (!(Rand_Next(1) & 3)) {
                        break;
                    }
                }
                Notice_Push(1, 0xE, p->useItem);
                Pouch_Add(p, p->useItem, -1);
            }
        } else if (Motion_PassedFrame(p, 0, p->motion[0].frameCount) != 0) {
            p->stateVar0--;
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_RunGroup0(Player *p)
{
    switch (p->state) {
    case 0x0:
        Player_State0_00(p, 0);
        break;
    case 0x1:
        Player_State0_01(p, 0U);
        break;
    case 0x2:
        Player_State0_01(p, 1U);
        break;
    case 0x3:
        Player_State0_01(p, 2U);
        break;
    case 0x4:
        Player_State0_04(p, 0U);
        break;
    case 0x5:
        Player_State0_04(p, 1U);
        break;
    case 0x6:
        Player_State0_06(p, 0);
        break;
    case 0x7:
        Player_State0_01(p, 8U);
        break;
    case 0x8:
        Player_State0_08(p, 0);
        break;
    case 0x9:
        Player_State0_06(p, 1);
        break;
    case 0xA:
        Player_State0_04(p, 2U);
        break;
    case 0xB:
        Player_State0_0B(p, 1);
        break;
    case 0xC:
        Player_State0_0C(p, 0);
        break;
    case 0xD:
        Player_State0_0D(p, 0);
        break;
    case 0xE:
        Player_State0_0B(p, 0);
        break;
    case 0xF:
        Player_State0_04(p, 3U);
        break;
    case 0x10:
        Player_State0_04(p, 4U);
        break;
    case 0x11:
        Player_State0_11(p, 0);
        break;
    case 0x12:
        Player_State0_01(p, 3U);
        break;
    case 0x13:
        Player_State0_01(p, 4U);
        break;
    case 0x14:
        Player_State0_14(p, 1);
        break;
    case 0x15:
        Player_State0_15(p, 0);
        break;
    case 0x16:
        Player_State0_16(p, 0);
        break;
    case 0x17:
        Player_State0_04(p, 5U);
        break;
    case 0x18:
        Player_State0_14(p, 0);
        break;
    case 0x19:
        Player_State0_19(p, 0);
        break;
    case 0x1A:
        Player_State0_01(p, 5U);
        break;
    case 0x1B:
        Player_State0_11(p, 1);
        break;
    case 0x1C:
        Player_State0_1C(p, 0);
        break;
    case 0x1D:
        Player_State0_08(p, 1);
        break;
    case 0x1E:
        Player_State0_1E(p, 0);
        break;
    case 0x1F:
        Player_State0_1F(p, 0);
        break;
    case 0x20:
        Player_State0_20(p, 0);
        break;
    case 0x21:
        Player_State0_21(p, 0);
        break;
    case 0x22:
        Player_State0_20(p, 1);
        break;
    case 0x24:
        Player_State0_01(p, 6U);
        break;
    case 0x25:
        Player_State0_25(p, 0);
        break;
    case 0x26:
        Player_State0_26(p, 0);
        break;
    case 0x27:
        Player_State0_27(p, 0);
        break;
    case 0x28:
        Player_State0_28(p, 0);
        break;
    case 0x29:
        Player_State0_29(p, 0);
        break;
    case 0x2A:
        Player_State0_2A(p, 0);
        break;
    case 0x2B:
        Player_State0_2B(p, 0);
        break;
    case 0x2C:
        Player_State0_2C(p, 0);
        break;
    case 0x2D:
        Player_State0_1C(p, 1);
        break;
    case 0x2E:
        Player_State0_2E(p, 0);
        break;
    case 0x2F:
        Player_State0_2F(p, 0);
        break;
    case 0x30:
        Player_State0_15(p, 1);
        break;
    case 0x31:
        Player_State0_31(p, 0);
        break;
    case 0x32:
        Player_State0_31(p, 1);
        break;
    case 0x33:
        Player_State0_33(p, 0);
        break;
    case 0x34:
        Player_State0_34(p, 0);
        break;
    case 0x35:
        Player_State0_35(p, 0);
        break;
    case 0x36:
        Player_State0_36(p, 0);
        break;
    case 0x37:
        Player_State0_37(p, 0);
        break;
    case 0x38:
        Player_State0_15(p, 2);
        break;
    case 0x39:
        Player_State0_39(p, 0);
        break;
    case 0x3A:
        Player_State0_3A(p, 0);
        break;
    case 0x3B:
        Player_State0_3B(p, 0);
        break;
    case 0x3C:
        Player_State0_3C(p, 0);
        break;
    case 0x3D:
        Player_State0_3D(p, 0);
        break;
    case 0x3E:
        Player_State0_3E(p, 0);
        break;
    case 0x3F:
        Player_State0_3F(p, 0);
        break;
    case 0x40:
        Player_State0_40(p, 0);
        break;
    case 0x41:
        Player_State0_41(p, 0);
        break;
    case 0x42:
        Player_State0_41(p, 1);
        break;
    case 0x43:
        Player_State0_40(p, 1);
        break;
    case 0x44:
        Player_State0_44(p, 0);
        break;
    case 0x45:
        Player_State0_20(p, 2);
        break;
    case 0x46:
        Player_State0_1F(p, 1);
        break;
    case 0x47:
        Player_State0_47(p, 0U);
        break;
    case 0x48:
        Player_State0_48(p, 0);
        break;
    case 0x49:
        Player_State0_01(p, 7U);
        break;
    case 0x4A:
        Player_State0_47(p, 3U);
        break;
    case 0x4B:
        Player_State0_4B(p, 1);
        break;
    case 0x4C:
        Player_State0_4C(p, 0);
        break;
    case 0x4D:
        Player_State0_4B(p, 0);
        break;
    case 0x4E:
        Player_State0_4E(p, 0);
        break;
    case 0x4F:
        Player_State0_4F(p, 0);
        break;
    case 0x50:
        Player_State0_4E(p, 1);
        break;
    case 0x51:
        Player_State0_47(p, 4U);
        break;
    case 0x52:
        Player_State0_47(p, 5U);
        break;
    case 0x53:
        Player_State0_53(p, 0);
        break;
    case 0x54:
        Player_State0_54(p, 0);
        break;
    case 0x55:
        Player_State0_55(p, 0);
        break;
    case 0x56:
        Player_State0_56(p, 0);
        break;
    case 0x57:
        Player_State0_57(p, 0);
        break;
    case 0x58:
        Player_State0_58(p, 0);
        break;
    case 0x59:
        Player_State0_59(p, 0);
        break;
    case 0x5A:
        Player_State0_58(p, 1);
        break;
    case 0x5B:
        Player_State0_5B(p, 0);
        break;
    case 0x5C:
        Player_State0_47(p, 2U);
        break;
    case 0x5D:
        Player_State0_5D(p, 0);
        break;
    case 0x5E:
        Player_State0_5E(p, 0);
        break;
    case 0x5F:
        Player_State0_5F(p, 0);
        break;
    case 0x60:
        Player_State0_5D(p, 1);
        break;
    case 0x61:
        Player_State0_61(p, 0);
        break;
    case 0x62:
        Player_State0_5E(p, 1);
        break;
    case 0x63:
        Player_State0_5F(p, 1);
        break;
    case 0x64:
        Player_State0_54(p, 1);
        break;
    case 0x65:
        Player_State0_65(p, 0);
        break;
    case 0x66:
        Player_State0_65(p, 1);
        break;
    case 0x67:
        Player_State0_67(p, 0);
        break;
    case 0x68:
        Player_State0_68(p, 0);
        break;
    case 0x69:
        Player_State0_69(p, 0);
        break;
    case 0x6A:
        Player_State0_6A(p, 0);
        break;
    case 0x6B:
        Player_State0_6B(p, 0);
        break;
    case 0x6C:
        Player_State0_6B(p, 1);
        break;
    case 0x6D:
        Player_State0_6B(p, 2);
        break;
    case 0x6E:
        Player_State0_6B(p, 3);
        break;
    case 0x6F:
        Player_State0_6F(p, 0);
        break;
    case 0x70:
        Player_State0_70(p, 0);
        break;
    case 0x71:
        Player_State0_71(p, 0);
        break;
    case 0x72:
        Player_State0_0B(p, 2);
        break;
    case 0x73:
        Player_State0_73(p, 0);
        break;
    case 0x74:
        Player_State0_01(p, 9U);
        break;
    case 0x75:
        Player_State0_47(p, 6U);
        break;
    case 0x76:
        Player_State0_5D(p, 2);
        break;
    case 0x77:
        Player_State0_28(p, 1);
        break;
    case 0x78:
        Player_State0_29(p, 1);
        break;
    case 0x79:
        Player_State0_56(p, 1);
        break;
    case 0x7A:
        Player_State0_36(p, 1);
        break;
    case 0x7B:
        Player_State0_57(p, 1);
        break;
    case 0x7C:
        Player_State0_47(p, 7U);
        break;
    case 0x7D:
        Player_State0_5D(p, 3);
        break;
    default:
        Player_BeginState(p, 0, 4, 0);
        break;
    }
}

void Player_WeaponState63(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x32, 4, 0);
        Eff00_Create(p, 5U);
        Player_SetMoveMode(p, 0, 1, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_RunWeaponState(Player *p)
{
    p->unk310 = 0;
    if (p->state == 0x63) {
        Player_WeaponState63(p, 0);
    } else {
        switch (p->weaponType) {
        case 0:
            GreatSword_RunState(p);
            return;
        case 7:
            LongSword_RunState(p);
            return;
        case 1:
        case 5:
            Bowgun_RunState(p);
            return;
        case 10:
            Bow_RunState(p);
            return;
        case 2:
            Hammer_RunState(p);
            return;
        case 8:
            HuntingHorn_RunState(p);
            return;
        case 3:
            Lance_RunState(p);
            return;
        case 9:
            Gunlance_RunState(p);
            return;
        case 4:
            SwordShield_RunState(p);
            return;
        case 6:
            DualSwords_RunState(p);
            return;
        }
    }
}

void Player_State2_00(Player *p)
{
    p->noHitTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        Player_UpdateMotionTime(p, 0);
        Player_SetMotionPair(p, 0xCA, 0, 0);
        Player_SetMoveMode(p, 0, 0, 1);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0xD8, 6, 0x22);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            p->noHitTimer = 6;
        }
        break;
    }
}

void Player_State2_01(Player *p, s32 variant)
{
    f32 v[3];
    f32 d[3];

    switch (p->step) {
    case 0:
        p->step++;
        if (variant == 0) {
            Player_SetMotionPair(p, 0x586, 2, 0);
        } else if (p->weaponType == 1) {
            Player_SetMotionPair(p, 0x58B, 0, 0);
        } else {
            Player_SetMotionPair(p, 0x587, 0, 0);
        }
        Player_UpdateMotionTime(p, 0);
        Sound_PlayPlayerSe(p, 0x67, 0, p->pos, 1, 0);
        Player_SetMoveMode(p, 0, 0, 1);
        Vib_StartLocal(p, 1);
        Player_SetFlag(p, 2);
        Player_SetFlag(p, 0x8000);
        p->weaponOut = 1;
        break;
    case 1:
        v[2] = 0.0f;
        if (variant == 0) {
            if (Motion_ReachedFrame(p, 0, 30.0f) == 0) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                switch (p->weaponType) {
                case 3:
                case 9:
                case 1:
                    v[2] = -15.0f * Math_Cos(6.2831855f * (90.0f * (p->motion[0].frame / 32.0f)) / 360.0f);
                    break;
                case 0:
                case 7:
                case 4:
                    v[2] = -20.0f * Math_Cos(6.2831855f * (90.0f * (p->motion[0].frame / 32.0f)) / 360.0f);
                    break;
                }
                Vec_Rotate(d, v, p->rotMtx);
                p->pos[0] += d[0];
                p->pos[2] += d[2];
            }
        } else {
            v[0] = 0.0f;
            v[1] = 0.0f;
            switch (p->weaponType) {
            case 3:
            case 9:
            case 1:
                if (Motion_ReachedFrame(p, 0, 90.0f) == 0) {
                    v[2] = -16.0f * Math_Cos(6.2831855f * (90.0f * (p->motion[0].frame / 104.0f)) / 360.0f);
                }
                break;
            case 0:
            case 7:
                if (Motion_ReachedFrame(p, 0, 104.0f) == 0) {
                    v[2] = -22.0f * Math_Cos(6.2831855f * (90.0f * (p->motion[0].frame / 104.0f)) / 360.0f);
                }
                break;
            case 4:
                if (Motion_ReachedFrame(p, 0, 94.0f) == 0) {
                    v[2] = -18.0f * Math_Cos(6.2831855f * (90.0f * (p->motion[0].frame / 94.0f)) / 360.0f);
                }
                break;
            }
            Vec_Rotate(d, v, p->rotMtx);
            p->pos[0] += d[0];
            p->pos[2] += d[2];
        }
        if (p->motion[0].playing == 0) {
            switch (p->weaponType) {
            case 3:
            case 9:
                Player_RequestState(p, 2, 0xC, 0);
                break;
            case 4:
                Player_BeginState(p, 0, 6, 0);
                break;
            case 0:
            case 7:
            case 1:
            default:
                Player_BeginState(p, 0, 0xA, 0);
                break;
            }
        }
        break;
    }
}

void Player_State2_02(Player *p)
{
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    f32 m[16];
    f32 y;

    p->noHitTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->demonMode = 0;
        Player_SetMotionPair(p, 0xCD, 0, 0x10);
        Em_GetBonePos(p, 2, v);
        if (p->pos[1] < v[1]) {
            p->pos[1] = v[1];
        }
        Player_SetMoveMode(p, 2, 0, 1);
        p->rideState = 0;
        Player_ClearAccel(p);
        ang[0] = 0;
        ang[1] = p->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, m);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = -11.17f;
        Vec_Rotate(d, v, m);
        p->vel[0] = 2.0f * d[0];
        p->vel[2] = 2.0f * d[2];
        p->vel[1] = 6.0f;
        Player_StartJump(p, 0xC);
        p->vel[1] = 0.0f;
        p->accel[1] = -1.0f / 3.0f;
        p->timer = 16;
        break;
    case 1:
        if (p->motion[0].playing == 0 && p->motionId[0] == 0xCD) {
            Player_SetMotionPair(p, 0xD6, 0, 0);
        }
        Player_MoveAccel(p);
        if (p->timer > 0) {
            p->timer--;
            y = 63.0f + p->floorY;
            if (p->pos[1] <= y) {
                p->pos[1] = y;
            }
        } else if (p->vel[1] <= 0.0f && p->pos[1] <= 63.0f + p->floorY) {
            p->step++;
            p->pos[1] = p->floorY;
            Sound_PlayPlayerSe(p, 0x42, 0, p->pos, 1, 0);
            Sound_PlayArmorRattle(p, 3);
            Player_SetMotionPair(p, 0xCE, 0, 0);
            p->action = 3;
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            if (p->hp > 0) {
                p->step++;
                Player_SetMotionPair(p, 0xCF, 2, 0);
            } else if (Player_IsLocal(p) == 1) {
                Player_RequestStateSynced(p, 3, 1, 0);
            } else {
                p->step = 4;
                Player_SetFlag(p, 0x20000);
            }
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            p->noHitTimer = 6;
        }
        break;
    case 4:
        break;
    }
}

void Player_State2_03(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        if (variant == 0) {
            switch (p->weaponType) {
            case 0:
            default:
            case 7:
                Player_SetMotionPair(p, 0x584, 3, 0);
                break;
            case 4:
                Player_SetMotionPair(p, 0x3ED, 2, 0);
                break;
            case 3:
            case 9:
                Player_SetMotionPair(p, 0x3ED, 2, 0);
                break;
            }
        } else {
            switch (p->weaponType) {
            case 0:
            default:
            case 7:
                Player_SetMotionPair(p, 0x584, 6, 0x28);
                break;
            case 4:
            case 3:
                Player_SetMotionPair(p, 0x3EE, 2, 0);
                break;
            case 9:
                if (variant == 1) {
                    Player_SetMotionPair(p, 0x3EE, 2, 0);
                } else {
                    Player_SetMotionPair(p, 0x3ED, 4, 4);
                }
                break;
            case 1:
                Player_SetMotionPair(p, 0x58A, 2, 0);
                break;
            }
        }
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x8000);
        p->weaponOut = 1;
        break;
    case 1:
        if (Player_IsLocal(p) == 1 || Game_IsOnline() == 0) {
            if (p->motionTime > 18 && !(p->input.held & 0x80)) {
                Player_RequestState(p, 2, 8, 0);
                break;
            }
        }
        switch (p->weaponType) {
        case 3:
        case 9:
        case 4:
            if (p->motionId[0] == 0x3ED && p->motion[0].playing == 0) {
                Player_SetMotionPair(p, 0x3EE, 2, 0);
            }
            if (p->motionTime > 18) {
                Player_GatherStep(p);
            }
            break;
        case 0:
        case 7:
            if (p->motionTime > 18) {
                Player_GatherStep(p);
            }
            break;
        }
        break;
    }
}

void Player_State2_04(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        Player_SetMotionPair(p, 0xE3, 0, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 1);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            p->noHitTimer = 6;
            break;
        }
        if (Motion_ReachedFrame(p, 0, 40.0f) == 0 && p->motionTime % 7 == 0) {
            Effect0D_SpawnForPlayer(p, 5, 9);
            Effect0D_SpawnForPlayer(p, 8, 9);
        }
        if (Motion_ReachedFrame(p, 0, 64.0f) == 0) {
            p->noHitTimer = 2;
        }
        break;
    }
}

void Player_State2_05(Player *p)
{
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    f32 m[16];
    f32 y;

    p->noHitTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->demonMode = 0;
        Player_SetMotionPair(p, 0xD0, 0, 0);
        Em_GetBonePos(p, 2, v);
        if (p->pos[1] < v[1]) {
            p->pos[1] = v[1];
        }
        p->targetYaw = p->rot[1];
        Player_SetMoveMode(p, 2, 0, 1);
        p->rideState = 0;
        Player_ClearAccel(p);
        ang[0] = 0;
        ang[1] = (u16) p->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, m);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 11.5f;
        Vec_Rotate(d, v, m);
        p->vel[0] = 2.0f * d[0];
        p->vel[2] = 2.0f * d[2];
        p->vel[1] = 0.0f;
        p->accel[1] = -0.25f;
        p->timer = 16;
        break;
    case 1:
        if (p->motion[0].playing == 0 && p->motionId[0] == 0xD0) {
            Player_SetMotionPair(p, 0xDC, 0, 0);
        }
        Player_MoveAccel(p);
        if (p->timer > 0) {
            p->timer--;
            y = 63.0f + p->floorY;
            if (p->pos[1] <= y) {
                p->pos[1] = y;
            }
        } else if (p->vel[1] <= 0.0f && p->pos[1] <= 63.0f + p->floorY) {
            p->step++;
            p->pos[1] = p->floorY;
            Sound_PlayPlayerSe(p, 0x42, 0, p->pos, 1, 0);
            Sound_PlayArmorRattle(p, 3);
            Player_SetMotionPair(p, 0xD1, 0, 0);
            p->action = 3;
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            if (p->hp > 0) {
                p->step++;
                Player_SetMotionPair(p, 0xD8, 0, 0);
            } else if (Player_IsLocal(p) == 1) {
                Player_RequestStateSynced(p, 3, 2, 0);
            } else {
                p->step = 4;
                Player_SetFlag(p, 0x20000);
            }
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
            p->noHitTimer = 6;
        }
        break;
    case 4:
        break;
    }
}

void Player_State2_06(Player *p)
{
    p->noHitTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        Player_UpdateMotionTime(p, 0);
        Player_SetMotionPair(p, 0xCC, 0, 0);
        Player_SetMoveMode(p, 0, 0, 1);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0xCF, 6, 0x3A);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            p->noHitTimer = 6;
        }
        break;
    }
}

void Player_State2_07(Player *p, s32 variant)
{
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    f32 m[16];
    f32 z;

    p->noHitTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->demonMode = 0;
        if (variant == 0) {
            Player_SetMotionPair(p, 0xD5, 0, 0);
        } else {
            Player_SetMotionPair(p, 0xDB, 0, 0);
        }
        Em_GetBonePos(p, 2, v);
        if (p->pos[1] < v[1]) {
            p->pos[1] = v[1];
        }
        Player_SetMoveMode(p, 2, 0, 1);
        p->rideState = 0;
        Player_ClearAccel(p);
        ang[0] = 0;
        ang[1] = p->rot[1];
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
        p->vel[0] = 2.0f * d[0];
        p->vel[2] = 2.0f * d[2];
        p->vel[1] = 20.0f;
        p->accel[1] = -8.0f / 11.0f;
        break;
    case 1:
        if (p->motion[0].playing == 0 && (p->motionId[0] == 0xD5 || p->motionId[0] == 0xDB)) {
            if (variant == 0) {
                Player_SetMotionPair(p, 0xD6, 0, 0);
            } else {
                Player_SetMotionPair(p, 0xDC, 0, 0);
            }
        }
        Player_MoveAccel(p);
        if (p->vel[1] <= 0.0f && p->pos[1] <= 63.0f + p->floorY) {
            p->step++;
            Sound_PlayPlayerSe(p, 0x42, 0, p->pos, 1, 0);
            Sound_PlayArmorRattle(p, 3);
            Player_SetMotionPair(p, 0xD7, 0, 0);
            p->pos[1] = p->floorY;
            p->action = 3;
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            p->step++;
            p->rot[1] = (p->rot[1] + 0x8000) & 0xFFFF;
            p->targetYaw = p->rot[1];
            Player_SetMotionPair(p, 0xD8, 0, 0);
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
            p->noHitTimer = 6;
        }
        break;
    }
}

void Player_State2_08(Player *p)
{
    switch (p->step) {
    case 0:
        p->step++;
        switch (p->weaponType) {
        case 0:
        default:
        case 7:
            Player_SetMotionPair(p, 0x586, 2, 0);
            break;
        case 4:
        case 3:
        case 9:
            Player_SetMotionPair(p, 0x3EF, 2, 0);
            break;
        case 1:
            Player_SetMotionPair(p, 0x58D, 2, 0);
            break;
        }
        Player_SetFlag(p, 0x8000);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 2, 0);
        }
        break;
    }
}

void Player_State2_09(Player *p, s32 variant)
{
    f32 v[3];
    s32 ang[3];
    f32 m[16];
    f32 z;

    switch (p->step) {
    case 0:
        p->step++;
        Player_UpdateMotionTime(p, 0);
        if (p->weaponOut != 0) {
            Player_SetMotionPair(p, 0x4BB, 0, 0);
        } else {
            Player_SetMotionPair(p, 0xC9, 0, 0);
        }
        Player_SetMoveMode(p, 0, 0, 1);
        ang[0] = 0;
        ang[2] = 0;
        ang[1] = p->targetYaw;
        Mtx_FromAnglesXYZ(ang, m);
        v[0] = 0.0f;
        v[1] = 0.0f;
        if (variant != 0) {
            z = 4.0f;
        } else {
            z = -4.0f;
        }
        v[2] = z;
        Vec_Rotate(&p->vel[0], v, m);
        break;
    case 1:
        if (Motion_ReachedFrame(p, 0, 16.0f) == 0) {
            Player_Move(p);
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State2_0B(Player *p)
{
    switch (p->step) {
    case 0:
        p->step++;
        if (p->weaponType == 1) {
            Player_SetMotionPair(p, 0x58A, 0, 0);
        } else {
            Player_SetMotionPair(p, 0x585, 0, 0);
        }
        Player_UpdateMotionTime(p, 0);
        Sound_PlayPlayerSe(p, 0x67, 0, &p->pos[0], 1, 0);
        Player_SetMoveMode(p, 0, 0, 1);
        Player_SetFlag(p, 0x8000U);
        Vib_StartLocal(p, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            if (p->weaponType == 1) {
                Player_BeginState(p, 0, 6, 0);
                break;
            }
            Player_RequestState(p, 2U, 0xCU, 0);
        } else {
        }
        break;
    }
}

void Player_State2_0F(Player *p, s32 variant)
{
    f32 v[3];
    f32 d[3];

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        Player_SetMotionPair(p, 0xDA, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 1);
        break;
    case 1:
        if (Motion_ReachedFrame(p, 0, 28.0f) != 0 && variant == 1) {
            p->step = 3;
            Player_SetMotionPair(p, 0xE2, 6, 0x14);
        } else if (Motion_ReachedFrame(p, 0, 40.0f) == 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -18.0f * Math_Cos(6.2831855f * (90.0f * (p->motion[0].frame / 42.0f)) / 360.0f);
            Vec_Rotate(d, v, p->rotMtx);
            p->pos[0] += d[0];
            p->pos[2] += d[2];
            if (p->motionTime % 7 == 0) {
                Effect0D_SpawnForPlayer(p, 5, 9);
                Effect0D_SpawnForPlayer(p, 8, 9);
            }
        } else {
            p->step++;
        }
        break;
    case 2:
        if (Motion_ReachedFrame(p, 0, 44.0f) != 0) {
            p->stateVar0++;
            if (variant == 1 || (variant == 0 && p->stateVar0 >= 2)) {
                p->step++;
                Player_SetMotionPair(p, 0xE2, 2, 0);
            }
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 0xC, 0);
        }
        break;
    }
}

void Player_State2_10(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        if (variant == 0) {
            p->stateVar1 = 4;
        } else {
            p->stateVar1 = 1;
        }
        Player_SetMotionPair(p, 0xD3, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 1);
        p->timer = 0;
        break;
    case 1:
        if (Motion_PassedFrame(p, 0, p->motion[0].frameCount) != 0) {
            p->stateVar0++;
            if ((s32) p->stateVar0 >= (s32) p->stateVar1) {
                p->step++;
                Player_SetMotionPair(p, 0xD4, 2, 0);
                break;
            }
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 0xC, 0);
        }
        break;
    }
}

void Player_State2_11(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_UpdateMotionTime(p, 0);
        if (p->weaponOut != 0) {
            Player_SetMotionPair(p, 0x4BB, 0, 0);
        } else {
            Player_SetMotionPair(p, 0xC9, 0, 0);
        }
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

static void Player_ClearStatus5(Player *p)
{
    p->status5Delay = 0;
    p->status5Buildup = 0;
    p->itemUseDelay = 0x1E;
}

void Player_State2_13(Player *p, s32 unused)
{
    s8 need;

    switch (p->step) {
    case 0:
        p->step++;
        Player_UpdateMotionTime(p, 0);
        p->weaponOut = 0;
        p->noFlinchTimer = 0;
        Player_SetMotionPair(p, 0x1A, 4, 0);
        Player_SetMoveMode(p, 0, 0, 1);
        p->timer = 300;
        p->stateVar0 = 0;
        p->stateVar1 = 5;
        p->status5Delay = 0;
        p->status5Buildup = 0;
        Player_ClearMash(p);
        p->targetYaw = p->rot[1];
        StageObj_Stun_CreatePlayer(p, 0);
        break;
    case 1:
        if (--p->timer <= 0) {
            p->stateVar0 = 0;
            p->stateVar1--;
            p->timer = 300;
        } else if ((p->input.pressed & 0x3FF) || (p->input.stickDirPressed & 0x3C3C)) {
            p->stateVar0++;
            need = Player_State2_13_Need[p->stateVar1];
            if (Player_HasSkill(p, 9) == 1) {
                need *= 2;
            }
            if (p->stateVar0 >= need) {
                p->stateVar0 = 0;
                p->stateVar1--;
                p->timer = 300;
            }
        }
        if (p->stateVar1 == 0) {
            Player_ClearStatus5(p);
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State2_14(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        p->weaponOut = 0;
        Player_SetMotionPair(p, 0x197, 4, 0);
        p->motionTime = 0;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State2_15(Player *p, s32 variant)
{
    f32 v[3];

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 0, 1);
        p->weaponOut = 0;
        p->status2Buildup = 0;
        Player_ClearStatus5(p);
        Player_ClearMash(p);
        Player_SetMotionPair(p, 0xE1, 4, 0);
        p->motionTime = 0;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            p->timer = 240;
            if (Player_HasSkill(p, 4) == 1) {
                p->timer /= 2;
            }
            if (Player_HasSkill(p, 6) == 1) {
                p->timer *= 2;
            }
            Player_SetMotionPair(p, 0xD2, 0, 0x9A);
        }
        break;
    case 2:
        if (p->stateVar1 > 0) {
            p->stateVar1--;
        }
        if (((p->input.pressed & 0x3FF) || (p->input.stickDirPressed & 0x3C3C)) && p->stateVar1 == 0) {
            p->stateVar1 = 3;
            p->timer--;
        }
        if (--p->timer <= 0) {
            p->step++;
            Player_SetMotionPair(p, 0xCF, 0, 0);
            break;
        }
        v[0] = 0.0f;
        v[1] = 20.0f;
        v[2] = 0.0f;
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(p, 4, 20, v, 0.8f);
        }
        Player_AddStamina(p, 2);
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State2_16(Player *p, s32 variant)
{

    p->stateVar0++;
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 0, 1);
        p->weaponOut = 0;
        p->status4Buildup = 0;
        p->status2Buildup = 0;
        Player_ClearStatus5(p);
        Player_ClearMash(p);
        if (variant == 0) {
            Player_SetMotionPair(p, 0xE0, 4, 0);
            p->motionTime = 0;
            p->stateVar0 = 0;
            Effect06_SpawnAtBone(p, 5, 0, 2, 0.75f);
            p->recoverTimer = 150;
            if (Player_HasSkill(p, 1) == 1) {
                p->recoverTimer /= 2;
            }
            if (Player_HasSkill(p, 3) == 1) {
                p->recoverTimer *= 2;
            }
            p->noHitTimer = 2;
            break;
        }
        p->step = 2;
        if (p->recoverTimer <= 0) {
            p->recoverTimer = 0;
        }
        Player_SetMotionPair(p, 0xD2, 2, 0x96);
        break;
    case 1:
        p->noHitTimer = 2;
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0xD2, 2, 0x96);
            p->noHitTimer = 0;
        }
        if (p->step < 3 && !(p->stateVar0 & 0x1F)) {
            Effect06_SpawnAtBone(p, 5, 0, 2, 0.75f);
        }
        break;
    case 2:
        if (p->stateVar1 > 0) {
            p->stateVar1--;
        }
        if (((p->input.pressed & 0x3FF) || (p->input.stickDirPressed & 0x3C3C)) && p->stateVar1 == 0) {
            p->stateVar1 = 3;
            p->recoverTimer--;
        }
        if (--p->recoverTimer <= 0) {
            p->step++;
            Player_SetMotionPair(p, 0xCF, 0, 0);
            p->noHitTimer = 2;
            break;
        }
        if (p->motionId[0] == 0xD2 && Motion_PassedFrame(p, 0, 170.0f) != 0 && !(Rand_Next(1) & 0x3F)) {
            Player_SetMotionPair(p, 0xDF, 0, 0);
        } else if (p->motionId[0] == 0xDF && p->motion[0].playing == 0) {
            Player_SetMotionPair(p, 0xD2, 4, 0xAA);
        }
        if (p->step < 3 && !(p->stateVar0 & 0x1F)) {
            Effect06_SpawnAtBone(p, 5, 0, 2, 0.75f);
        }
        break;
    case 3:
        p->noHitTimer = 2;
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            p->noHitTimer = 6;
        }
        break;
    }
}

void Player_State2_17(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x3FA, 2, 1);
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x8000);
        p->weaponOut = 1;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 2U, 0xCU, 0);
        }
        break;
    }
}

void Player_UpdateMashEscape(Player *p, s32 caller)
{
    s16 n;

    if (Player_IsLocal(p) != 0) {
        if (p->mashTimer == 0) {
            if ((p->input.pressed & 0x3FF) || (p->input.stickDirPressed & 0x3C3C)) {
                if (p->mashLeft > 0) {
                    n = 10;
                    if (Player_HasSkill(p, 0x10) == 1) {
                        n *= 2;
                    }
                    if (Player_HasSkill(p, 0x12) == 1) {
                        n /= 2;
                    }
                    p->mashLeft -= n;
                    if (p->mashLeft < 0) {
                        p->mashLeft = 0;
                    }
                    p->mashTimer = 1;
                }
            }
        } else {
            p->mashTimer--;
        }
        if (p->mashLeft <= 0) {
            Player_ClearMash(p);
            p->statusIcons &= 0xFFBF;
            Player_RequestState(p, 2, 0x1C, 0);
            return;
        }
        if (p->input.stickTilt >= 40) {
            Player_FaceStick(p);
            if ((p->input.held & 0x10) && p->dashDelay == 0) {
                if (Player_IsState(p, 2, 0x20) == 0) {
                    Player_RequestState(p, 2, 0x20, 0);
                }
            } else if (Player_IsState(p, 2, 0x1F) == 0 && p->dashDelay == 0) {
                Player_RequestState(p, 2, 0x1F, 0);
            }
        } else if (Player_TestFlag(p, 0x2001200) != 0) {
            if ((s16) caller != 1 || p->timer == 0) {
                Player_RequestState(p, 2, 0x1E, 0);
            }
        }
    }
}

static void Player_ClearMash(Player *p)
{
    p->mashLeft = 0;
    p->mashBuildup = 0;
    p->mashTimer = 0;
}

void Player_State2_1B(Player *p, s32 variant)
{
    if (p->motionId[0] != 0x33) {
        Player_SetMotionPair(p, 0x33, 4, 0);
    }
    switch (p->step) {
    case 0:
        p->step++;
        p->weaponOut = 0;
        if (variant == 0) {
            p->status4Buildup = 0;
            p->status2Buildup = 0;
            Player_ClearStatus5(p);
            Player_ClearMash(p);
            p->mashLeft = 900;
            p->noFlinchTimer = 0;
            p->noFlinchItemTimer = 0;
            p->timer = 15;
        } else {
            p->timer = 30;
        }
        Player_SetMoveMode(p, 0, 0, 1);
        p->weaponOut = 0;
    case 1:
        if (--p->timer <= 0) {
            p->step++;
        }
        break;
    case 2:
        if (Player_IsLocal(p) == 1) {
            Player_UpdateMashEscape(p, 0);
        }
        break;
    }
}

void Player_State2_1C(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->weaponOut = 0;
        Player_SetMoveMode(p, 0, 0, 1);
        p->weaponOut = 0;
        Player_SetMotionPair(p, 0x197, 6, 0);
        StageObj_K1B_Create(0, (s16 *) p);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State2_1D(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->weaponOut = 0;
        Player_SetMotionPair(p, 0x1A, 6, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 1);
        if (variant == 0) {
            p->timer = 0x28;
            break;
        }
        p->timer = 0x50;
        break;
    case 1:
        if (--p->timer <= 0) {
            Player_BeginState(p, 0, 0xC, 0);
        }
        break;
    }
}

void Player_State2_1F(Player *p, s32 unused)
{
    f32 v[3];
    f32 d[3];

    switch (p->step) {
    case 0:
        p->step++;
        p->timer = 0x14;
        p->dashDelay = 0x1E;
        Player_SetMotionPair(p, 0x34, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x1000);
        break;
    case 1:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 2.0f;
        Vec_Rotate(d, v, p->rotMtx);
        p->pos[0] += d[0];
        p->pos[2] += d[2];
        if (p->timer > 0) {
            p->timer--;
        }
        Player_UpdateMashEscape(p, 1);
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        StageObj_K1B_Create(1, (s16 *) p);
    }
}

void Player_State2_20(Player *p, s32 unused)
{
    f32 v[3];
    f32 d[3];

    switch (p->step) {
    case 0:
        p->step++;
        if (p->gender == 0) {
            Player_SetMotionPair(p, 0x35, 4, 0);
        } else {
            Player_SetMotionPair(p, 0x3B, 4, 0);
        }
        p->timer = 0x14;
        p->dashDelay = 0x1E;
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x2000200);
        break;
    case 1:
        if (Player_FloorAheadBelow(p, 0, 85.0f, -60.0f) != 0) {
            p->targetYaw = p->rot[1];
            Player_RequestState(p, 2, 0x21, 0);
            return;
        }
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 5.0f;
        Vec_Rotate(d, v, p->rotMtx);
        p->pos[0] += d[0];
        p->pos[2] += d[2];
        if (p->timer > 0) {
            p->timer--;
        }
        Player_UpdateMashEscape(p, 1);
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        StageObj_K1B_Create(1, (s16 *) p);
    }
}

void Player_State2_21(Player *p, s32 variant)
{
    f32 v[3];
    f32 d[3];

    p->timer++;
    switch (p->step) {
    case 0:
        p->step++;
        p->action = 2;
        switch (variant) {
        case 0:
        default:
            Player_SetMotionPair(p, 0x38, 0, 0);
            p->vel[0] = 0.0f;
            p->vel[2] = 20.0f;
            p->vel[1] = 18.0f;
            p->accel[0] = 0.0f;
            p->accel[2] = -(p->vel[2] / 80.0f);
            Player_StartJump(p, 0x13);
            break;
        case 1:
            Player_SetMotionPair(p, 0x38, 8, 0x20);
            p->rideState = 0;
            p->vel[0] = 0.0f;
            p->vel[2] = 8.0f;
            p->vel[1] = -10.0f;
            p->accel[2] = 0.0f;
            p->accel[0] = 0.0f;
            p->accel[1] = -8.0f / 11.0f;
            break;
        }
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 2, 0, 0);
        p->timer = 0;
        p->stateVar1 = 0;
        break;
    case 1:
        p->vel[0] += p->accel[0];
        p->vel[1] += p->accel[1];
        p->vel[2] += p->accel[2];
        if (p->vel[0] < 0.0f) {
            p->vel[0] = 0.0f;
        }
        if (p->vel[2] < 0.0f) {
            p->vel[2] = 0.0f;
        }
        v[0] = p->vel[0];
        v[1] = p->vel[1];
        v[2] = p->vel[2];
        Vec_Rotate(d, v, p->rotMtx);
        p->pos[0] += d[0];
        p->pos[1] += d[1];
        p->pos[2] += d[2];
        if (p->vel[1] < 0.0f) {
            p->timer++;
            if (!(p->floorY < p->pos[1])) {
                p->pos[1] = p->floorY;
                p->action = 0;
                p->rideState = 0;
                if (p->timer >= 50) {
                    StageObj_K1B_Create(0, (s16 *) p);
                    Player_ClearMash(p);
                    Player_RequestState(p, 0, 0x26, 0);
                } else if (p->timer >= 30) {
                    Player_RequestState(p, 2, 0x22, 2);
                } else {
                    Player_RequestState(p, 2, 0x23, 2);
                }
            }
        }
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        StageObj_K1B_Create(1, (s16 *) p);
    }
}

void Player_State2_22(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        if (variant == 0) {
            Player_SetMotionPair(p, 0x39, 0, 0);
        } else {
            Player_SetMotionPair(p, 0x3A, 0, 0);
        }
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 2U, 0x1EU, 2);
        }
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        StageObj_K1B_Create(1, (s16 *) p);
    }
}

void Player_State2_27(Player *p, s32 variant)
{
    f32 v[3];
    f32 d[3];
    f32 t[3];
    s32 ang[3];
    f32 mtx[16];
    Em *m;
    s16 i;
    s16 found;
    switch (p->step) {
    case 0:
        p->step++;
        p->demonMode = 0;
        Player_SetMotionPair(p, 0xD5, 0, 0);
        p->noHitTimer = 10;
        Em_GetBonePos(p, 2, v);
        if (p->pos[1] < v[1]) {
            p->pos[1] = v[1];
        }
        Player_SetMoveMode(p, 2, 0, 1);
        p->rideState = 0;
        Player_ClearAccel(p);
        found = 0;
        m = gEms;
        for (i = 0; i < 20; i++, m++) {
            if (m->active && m->visible && (Player_InCurrentArea(m) & 0xFF) && m->type == 0x3A) {
                found = 1;
                Em_BonePos(m, 0x24, t);
                p->rot[1] = Vec_Heading(p->pos, t) + 0x4000;
                p->targetYaw = p->rot[1];
            }
        }
        ang[0] = 0;
        ang[1] = p->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, mtx);
        if (found == 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -7.0f;
            Vec_Rotate(d, v, mtx);
            p->vel[0] = 2.0f * d[0];
            p->vel[2] = 2.0f * d[2];
            p->vel[1] = 20.0f;
            p->accel[1] = -8.0f / 11.0f;
        } else {
            d[0] = (t[0] - p->pos[0]) / 50.0f;
            d[1] = (t[1] - p->pos[1]) / 50.0f;
            d[2] = (t[2] - p->pos[2]) / 50.0f;
            p->vel[0] = d[0];
            p->vel[2] = d[2];
            if (d[1] <= 15.0f) {
                d[1] = 15.0f;
            }
            p->vel[1] = 2.0f * d[1];
            p->accel[1] = -8.0f / 11.0f;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0 && (p->motionId[0] == 0xD5 || p->motionId[0] == 0xDB)) {
            if (variant == 0) {
                Player_SetMotionPair(p, 0xD6, 0, 0);
            } else {
                Player_SetMotionPair(p, 0xDC, 0, 0);
            }
        }
        Player_MoveAccel(p);
        if (p->vel[1] <= 0.0f && p->pos[1] <= 63.0f + p->floorY) {
            p->noHitTimer = 2;
            p->step++;
            Sound_PlayPlayerSe(p, 0x42, 0, p->pos, 1, 0);
            Sound_PlayArmorRattle(p, 3);
            Player_SetMotionPair(p, 0xD7, 0, 0);
            p->pos[1] = p->floorY;
            p->action = 3;
        }
        break;
    case 2:
        p->noHitTimer = 2;
        if (p->motion[0].playing == 0) {
            p->step++;
            p->rot[1] = (p->rot[1] + 0x8000) & 0xFFFF;
            p->targetYaw = p->rot[1];
            Player_SetMotionPair(p, 0xD8, 0, 0);
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
            p->noHitTimer = 6;
        }
        break;
    }
}

void Player_RunGroup2(Player *p)
{
    u8 state;
    u8 carry;

    Player_ResetAim(p);
    p->supplyBoxOpen = 0;
    p->deliveryMenu = 0;
    p->deliveryOpen = 0;
    p->tradeMsgTimer = 0;
    p->noFlinchTimer = 0;
    Player_ClearFlag(p, 0x80000);
    state = p->state;
    if ((state != 0x11) && (state != 0x12)) {
        carry = p->carry;
        if (carry & 0xF) {
            p->carry = carry & 0xF0;
            StageObj_K0C_CreateAtPlayer(p, 4);
        }
    }
    switch (p->state) {
    case 0:
        Player_State2_00(p);
        break;
    case 1:
        Player_State2_01(p, 0);
        break;
    case 2:
        Player_State2_02(p);
        break;
    case 3:
        Player_State2_03(p, 0);
        break;
    case 4:
        Player_State2_04(p, 0);
        break;
    case 5:
        Player_State2_05(p);
        break;
    case 6:
        Player_State2_06(p);
        break;
    case 7:
        Player_State2_07(p, 0);
        break;
    case 8:
        Player_State2_08(p);
        break;
    case 9:
        Player_State2_09(p, 0);
        break;
    case 10:
        Player_State2_01(p, 1);
        break;
    case 11:
        Player_State2_0B(p);
        break;
    case 12:
        Player_State2_03(p, 1);
        break;
    case 13:
        Player_State2_09(p, 1);
        break;
    case 14:
        Player_State2_07(p, 1);
        break;
    case 15:
        Player_State2_0F(p, 0);
        break;
    case 16:
        Player_State2_10(p, 0);
        break;
    case 17:
        Player_State2_11(p, 0);
        break;
    case 18:
        Player_State2_11(p, 1);
        break;
    case 19:
        Player_State2_13(p, 0);
        break;
    case 20:
        Player_State2_14(p, 0);
        break;
    case 21:
        Player_State2_15(p, 0);
        break;
    case 22:
        Player_State2_16(p, 0);
        break;
    case 23:
        Player_State2_17(p, 0);
        break;
    case 24:
        Player_State2_16(p, 1);
        break;
    case 25:
        Player_State2_10(p, 1);
        break;
    case 26:
        Player_State2_0F(p, 1);
        break;
    case 27:
        Player_State2_1B(p, 0);
        break;
    case 28:
        Player_State2_1C(p, 0);
        break;
    case 29:
        Player_State2_1D(p, 0);
        break;
    case 30:
        Player_State2_1B(p, 1);
        break;
    case 31:
        Player_State2_1F(p, 0);
        break;
    case 32:
        Player_State2_20(p, 0);
        break;
    case 33:
        Player_State2_21(p, 0);
        break;
    case 34:
        Player_State2_22(p, 0);
        break;
    case 35:
        Player_State2_22(p, 1);
        break;
    case 36:
        Player_State2_21(p, 1);
        break;
    case 37:
        Player_State2_1D(p, 1);
        break;
    case 38:
        Player_State2_03(p, 2);
        break;
    case 39:
        Player_State2_27(p, 0);
        break;
    default:
        Player_BeginState(p, 0, 4, 0);
        break;
    }
}

extern s32 gRewardZeroText;
extern s32 gFaintedText;
extern s32 gRewardZeroText2;
extern s32 gNoMoreReviveText;
void Player_State3_00(Player *p, s32 variant)
{
    s8 f;
    s8 r;
    u8 k;

    p->noPushTimer = 10;
    Player_ResetAim(p);
    p->supplyBoxOpen = 0;
    p->deliveryMenu = 0;
    p->deliveryOpen = 0;
    p->tradeMsgTimer = 0;
    Player_ClearFlag(p, 0x80000);
    if (p->carry & 0xF) {
        p->carry &= 0xF0;
        StageObj_K0C_CreateAtPlayer(p, 4);
    }
    switch (p->step) {
    case 0:
        p->step++;
        switch (variant) {
        case 0:
            Player_SetMotionPair(p, 0xD2, 0, 0);
            break;
        case 1:
            Player_SetMotionPair(p, 0xD2, 0, 0x8C);
            break;
        case 2:
            Player_SetMotionPair(p, 0xD1, 0, 0x8C);
            Player_SetFlag(p, 0x20000);
            break;
        }
        Vib_StartLocal(p, 3);
        Player_UpdateMotionTime(p, 0);
        Player_PlaySeAtSelf((u8 *) p, 0x22);
        Player_ClearFlag(p, 2);
        Player_ClearFlag(p, 0x8000);
        p->weaponOut = 0;
        switch (*(u8 *) &p->stateRand) {
        default:
            f = 0;
            break;
        case 1:
            f = 1;
            break;
        }
        if (p->side == 0) {
            r = Quest_PlayerFainted(p, f);
            if (r != 0 && r != 3) {
                p->step++;
            }
        } else {
            r = 2;
            Versus_MonsterDown((Em *) p);
        }
        if (p->faints <= 98) {
            p->faints++;
        } else {
            p->faints = 99;
        }
        if (Player_IsLocal(p) == 0) {
            Notice_Push(3, 0, p->index);
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
            if (p->step <= 1) {
                func_3A1F30();
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
        if (p->side != 0) {
            p->step = 3;
            p->timer = 0;
        }
        if (p->motionId[0] == 0xD2 && Motion_PassedFrame(p, 0, 164.0f) != 0 && (Rand_Next(1) & 0x3F) == 0) {
            Player_SetMotionPair(p, 0xDF, 0, 0);
        } else if (p->motionId[0] == 0xDF && p->motion[0].playing == 0) {
            Player_SetMotionPair(p, 0xD2, 4, 0xA4);
        }
        if (Quest_PhaseIsOver(2) == 1) {
            p->step++;
            break;
        }
        if (p->motionTime == 300 && Player_IsLocal(p) == 1) {
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
        if (p->motionTime == 600 && Player_IsLocal(p) == 1) {
            switch (gScene.area) {
            case 0x7C:
                k = 0x54;
                p->areaMoveState = 3;
                break;
            case 0x7E:
                k = 0x7D;
                p->areaMoveState = 3;
                break;
            case 0x8A:
                k = 0x81;
                p->areaMoveState = 3;
                break;
            default:
                k = gScene.campArea;
                p->areaMoveState = 2;
                break;
            }
            p->startPos[0] = gStartPos[k][0];
            p->startPos[1] = D_43F0A4[k][0];
            p->startPos[2] = D_43F0A8[k][0];
            p->startAngle = gStartAngle[k];
            p->nextArea = k;
            QSync_SendPlayer(p, 4, 0);
        }
        break;
    case 2:
        break;
    case 3:
        if (p->motionId[0] == 0xD2 && Motion_PassedFrame(p, 0, 164.0f) != 0 && (Rand_Next(1) & 0x3F) == 0) {
            Player_SetMotionPair(p, 0xDF, 0, 0);
        } else if (p->motionId[0] == 0xDF && p->motion[0].playing == 0) {
            Player_SetMotionPair(p, 0xD2, 4, 0xA4);
        }
        if (Player_IsLocal(p) == 1) {
            if (++p->timer >= 450) {
                CamReq_Cancel();
                p->hpMax = (s16)Player_GetHpSkillBonus(p) + 100;
                if (p->hpMax > 150) {
                    p->hpMax = 150;
                }
                p->hp = p->hpMax;
                p->hpRecoverable = p->hpMax;
                p->hpMaxBase = p->hpMax;
                p->staminaMax = 300;
                p->staminaTimer = 0x2A30;
                p->stamina = p->staminaMax;
                Player_ClearBuffs(p);
                p->weaponSeMute = 0;
                Player_RequestState(p, 4, 4, 2);
            }
        }
        break;
    }
}

void Player_RunGroup3(Player *p)
{
    p->supplyBoxOpen = 0;
    p->deliveryMenu = 0;
    p->deliveryOpen = 0;
    p->tradeMsgTimer = 0;
    switch (p->state) {
    case 0:
        Player_State3_00(p, 0);
        break;
    case 1:
        Player_State3_00(p, 1);
        break;
    case 2:
        Player_State3_00(p, 2);
        break;
    default:
        Player_BeginState(p, 0, 4, 0);
        break;
    }
}

void Player_State4_00(Player *p, s32 variant)
{
    Em *m;
    s16 i;
    s16 kind;

    p->noPushTimer = 10;
    p->noHitTimer = 10;
    p->visible = 1;
    switch (p->step) {
    case 0:
        p->step++;
        kind = -1;
        p->stateVar0 = 0;
        for (i = 0; i < 4; i++) {
            if (gScene.emKind[i] == 0x12 && (m = EmSlot_Alloc()) != NULL) {
                m->modelSlot = i;
                m->type = gScene.emKind[i];
                m->area = gScene.area;
                m->variant = 0;
                m->variant = 0xFF;
                m->cartPlayer = p->index;
                GmEm_Task(m);
                m->posX = p->pos[0];
                m->posY = p->pos[1];
                m->posZ = p->pos[2];
                m->rotY = p->rot[1];
                p->cart = m;
                kind = gScene.emKind[i];
                break;
            }
        }
        if (kind != 0x12) {
            Player_BeginState(p, 0, 4, 0);
        } else {
            p->stateVar1 = 0;
            Player_SetMotionPair(p, 0xDD, 0, 0);
        }
        if (Player_IsLocal(p) == 1) {
            CamReq_Mode01();
        }
        break;
    case 1:
        m = p->cart;
        if (m == NULL) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        Em_BonePos(m, 0x1D, &p->pos[0]);
        if (m->motionId[0] == 0x3EB) {
            Player_RequestState(p, 4, 1, 2);
        }
        p->rot[1] = m->rotY;
        p->targetYaw = p->rot[1];
        break;
    }
}

void Player_State4_01(Player *p, s32 unused)
{
    f32 floorY;

    p->noPushTimer = 0xA;
    p->noHitTimer = 0xA;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0xDE, 0, 0);
        p->action = 2;
        break;
    case 1:
        if (Motion_ReachedFrame(p, 0, 28.0f) != 0) {
            p->pos[1] -= 4.0f;
        }
        floorY = p->floorY;
        if (p->pos[1] <= floorY) {
            p->pos[1] = floorY;
        }
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0xD1, 0, 0x46);
            p->action = 0;
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0xD8, 0, 0);
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Player_State4_02(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->unk7E5 = 1;
        Player_SetMoveMode(p, 0, 0, 0);
        p->weaponOut = 0;
        Player_SetMotionPair(p, 0x197, 4, 0);
        p->motionTime = 0;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State4_03(Player *p, s32 unused)
{
    u8 carry;

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        p->weaponOut = 0;
        Player_SetMotionPair(p, 0x26F, 6, 0);
        p->motionTime = 0;
        carry = p->carry;
        if (carry & 0xF) {
            p->carry = carry & 0xF0;
            StageObj_K0C_CreateAtPlayer(p, 4);
        }
        if (Player_IsLocal(p) == 1) {
            CamReq_Mode0A();
        }
        
    case 1:
        break;
    }
}

void Player_State4_04(Player *p, s32 unused)
{
    p->noHitTimer = 4;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        p->weaponOut = 0;
        if (p->motionId[0] == 0xD1) {
            Player_SetMotionPair(p, 0xD8, 0, 0);
        } else {
            Player_SetMotionPair(p, 0xCF, 0, 0);
        }
        p->motionTime = 0;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->noHitTimer = 0x96;
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State4_05(Player *p, s32 unused)
{
    u8 carry;

    p->noHitTimer = 0xA;
    p->noPushTimer = 0xA;
    Player_ResetAim(p);
    p->supplyBoxOpen = 0;
    p->deliveryMenu = 0;
    p->deliveryOpen = 0;
    p->tradeMsgTimer = 0;
    Player_ClearFlag(p, 0x80000);
    p->pos[1] = p->floorY;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0xD2, 0, 0);
        Player_UpdateMotionTime(p, 0);
        Player_PlaySeAtSelf((u8 *) p, 0x22);
        Player_ClearFlag(p, 2);
        Player_ClearFlag(p, 0x8000);
        p->weaponOut = 0;
        p->timer = 0x12C;
        carry = p->carry;
        if (carry & 0xF) {
            p->carry = carry & 0xF0;
            StageObj_K0C_CreateAtPlayer(p, 4);
        }
        if (Game_IsOnline() != 1) {
            return;
        }
        return;
    case 1:
        if (--p->timer <= 0) {
            p->step++;
            return;
        }
        break;
    case 2:
        p->active = 0;
        p->visible = 0;
        break;
    }
}

void Player_State4_06(Player *p, s32 unused)
{
    u8 carry;

    p->noHitTimer = 0xA;
    p->noPushTimer = 0xA;
    Player_ResetAim(p);
    p->supplyBoxOpen = 0;
    p->deliveryMenu = 0;
    p->deliveryOpen = 0;
    p->tradeMsgTimer = 0;
    Player_ClearFlag(p, 0x80000);
    p->pos[1] = p->floorY;
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x15, 0, 0);
        Player_ClearFlag(p, 2);
        Player_ClearFlag(p, 0x8000);
        p->weaponOut = 0;
        p->timer = 0x12C;
        carry = p->carry;
        if (carry & 0xF) {
            p->carry = carry & 0xF0;
            StageObj_K0C_CreateAtPlayer(p, 4);
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 5, 4, 0x90);
        }
        break;
    case 2:
        if (--p->timer <= 0) {
            p->step++;
        }
        break;
    case 3:
        p->active = 0;
        p->visible = 0;
        break;
    }
}

void Player_RunGroup4(Player *p)
{
    p->supplyBoxOpen = 0;
    p->deliveryMenu = 0;
    p->tradeMsgTimer = 0;
    p->deliveryOpen = 0;
    switch (p->state) {
    case 0:
        Player_State4_00(p, 0);
        break;
    case 1:
        Player_State4_01(p, 0);
        break;
    case 2:
        Player_State4_02(p, 0);
        break;
    case 3:
        Player_State4_03(p, 0);
        break;
    case 4:
        Player_State4_04(p, 0);
        break;
    case 5:
        Player_State4_05(p, 0);
        break;
    case 6:
        Player_State4_06(p, 0);
        break;
    default:
        Player_BeginState(p, 0, 4, 0);
        break;
    }
}

void Player_UpdateCarryKind(Player *p)
{
    u16 slot;

    if (Player_IsLocal(p) == 1 && Player_IsState(p, 5, 10) == 0 && Player_IsState(p, 5, 11) == 0) {
        slot = Pouch_FindCarryItem(p);
        if (slot != 0xFFFF) {
            p->carry = ((Item_CarryKind(slot) & 0xFF) - 1) * 0x10;
        } else {
            p->carry = 0;
        }
    }
    p->carry = (p->carry & 0xF0) | 4;
}

void Player_UpdateCarryInput(Player *p, s32 caller)
{
    f32 buf[3];
    ItemStack st;
    u16 ang;
    u8 k;
    s16 i;

    if (Player_IsLocal(p) != 0) {
        switch (gScene.questPhase) {
        case 6:
        case 5:
            Player_RequestState(p, 4, 3, 0);
            break;
        case 7:
            Player_RequestState(p, 4, 6, 0);
            break;
        case 8:
            Player_RequestState(p, 4, 5, 0);
            break;
        default:
            if ((u16) Pouch_FindCarryItem(p) == 0xFFFF) {
                Player_RequestState(p, 5, 3, 0);
                break;
            }
            if (p->input.stickTilt >= 40) {
                Player_FaceStick(p);
                if ((p->input.held & 0x10) && p->dashDelay == 0) {
                    if (Player_IsState(p, 5, 2) == 0) {
                        Player_RequestState(p, 5, 2, 0);
                    }
                } else if (Player_IsState(p, 5, 1) == 0 && p->dashDelay == 0) {
                    Player_RequestState(p, 5, 1, 0);
                }
            } else if (Player_TestFlag(p, 0x2001200) != 0) {
                if ((s16) caller != 1 || p->timer == 0) {
                    Player_RequestState(p, 5, 0, 0);
                }
            }
            if (Quest_PhaseIsOver(1) == 1) {
                break;
            }
            if (p->input.pressed & 0x40) {
                if (p->input.stickTilt >= 85) {
                    if (p->stamina >= 75) {
                        Player_RequestState(p, 0, 0x1C, 4);
                    }
                } else {
                    Player_RequestState(p, 0, 8, 0);
                }
                return;
            }
            if (p->input.stickDirPressed & 0x3C) {
                Player_RequestState(p, 0, 4, 0);
                break;
            }
            if ((p->input.pressed & 0x200) && p->itemSelect == 0) {
                Player_RequestState(p, 5, 3, 0);
            }
            Player_TryDrawWeapon(p, 2);
            if (p->input.pressed & 0x20) {
                switch (Player_FindActionSpot((Player *)p, (f32 *)buf, &ang, (u8 *)&k)) {
                case 7:
                    if (Quest_PhaseIsOver(0) == 1) {
                        break;
                    }
                    for (i = 0; i < 3; i++) {
                        if (Quest_GetDeliveryTarget(p, i, &st) == 2 && st.item == (u16) Pouch_FindCarryItem(p)) {
                            Quest_DeliverItem(p, st.item, 1);
                        }
                    }
                    if ((u16) Pouch_FindCarryItem(p) == 0xFFFF) {
                        Player_RequestState(p, 5, 0xA, 0);
                    }
                    break;
                case 8:
                    if ((u16) Pouch_FindCarryItem(p) == 0x29C) {
                        p->carry &= 0xF0;
                        Pouch_Add(p, 0x29C, -10);
                        p->targetYaw = ang;
                        p->stateRand = k;
                        Player_RequestState(p, 0, 0x37, 0x20);
                    }
                    break;
                default:
                    if (Player_NearEmKind10(p) & 0xFF) {
                        Player_RequestState(p, 5, 0xB, 0);
                        return;
                    }
                    break;
                }
            }
            break;
        }
    }
}

void Player_State5_00(Player *p, s32 variant)
{
    s32 timer;

    Player_UpdateCarryKind(p);
    switch (p->step) {
    case 0:
        p->step++;
        p->timer = 0x1E;
        p->dashDelay = 0;
        
    case 1:
        if (p->motionId[0] != 0x33) {
            Player_SetMotionPair(p, 0x33, 4, 0);
        }
        if (variant == 1) {
            timer = p->timer;
            if (timer <= 0) {
                Player_UpdateCarryInput(p, 0);
                break;
            }
            p->timer = timer - 1;
            break;
        }
        Player_UpdateCarryInput(p, 0);
        break;
    }
}

void Player_State5_01(Player *p, s32 unused)
{
    f32 v[3];
    f32 d[3];

    Player_UpdateCarryKind(p);
    switch (p->step) {
    case 0:
        p->step++;
        p->timer = 0x14;
        p->dashDelay = 0x1E;
        Player_SetMotionPair(p, 0x34, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x1000);
        break;
    case 1:
        if (func_3B5440((Em *) p) != 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 2.0f;
            Vec_Rotate(d, v, p->rotMtx);
            p->pos[0] += d[0];
            p->pos[2] += d[2];
        }
        if (p->timer > 0) {
            p->timer--;
        }
        Player_UpdateCarryInput(p, 1);
        break;
    }
}

void Player_State5_02(Player *p, s32 unused)
{
    f32 v[3];
    f32 d[3];

    Player_UpdateCarryKind(p);
    switch (p->step) {
    case 0:
        p->step++;
        if (p->gender == 0) {
            Player_SetMotionPair(p, 0x35, 4, 0);
        } else {
            Player_SetMotionPair(p, 0x3B, 4, 0);
        }
        p->timer = 0x14;
        p->dashDelay = 0x1E;
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x2000200);
        break;
    case 1:
        if (Player_FloorAheadBelow(p, 0, 85.0f, -60.0f) != 0) {
            p->targetYaw = p->rot[1];
            Player_RequestState(p, 5, 5, 0);
            break;
        }
        Player_UseStamina(p, -1);
        if (p->stamina <= 0) {
            Player_RequestState(p, 5, 4, 0);
            break;
        }
        if (func_3B5440((Em *) p) != 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 5.0f;
            Vec_Rotate(d, v, p->rotMtx);
            p->pos[0] += d[0];
            p->pos[2] += d[2];
        }
        if (p->timer > 0) {
            p->timer--;
        }
        Player_UpdateCarryInput(p, 1);
        break;
    }
}

void Player_State5_03(Player *p, s32 variant)
{
    u16 item;

    switch (p->step) {
    case 0:
        p->step++;
        if (variant == 1) {
            Player_SetMotionPair(p, 0x37, 4, 0);
        } else {
            Player_SetMotionPair(p, 0x36, 4, 0);
        }
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (Motion_PassedFrame(p, 0, variant == 1 ? 4 : 0x72) != 0) {
            p->step++;
            p->carry &= 0xF0;
            if (variant != 2) {
                StageObj_K0C_CreateAtPlayer(p, 4);
            }
            if (variant == 0 && Player_IsLocal(p) == 1) {
                item = Pouch_FindCarryItem(p);
                if (item != 0xFFFF) {
                    Notice_Push(1, 0xC, item);
                    Pouch_Add(p, item, -100);
                }
            }
            break;
        }
        Player_UpdateCarryKind(p);
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
        }
        break;
    }
}

void Player_State5_05(Player *p, s32 variant)
{
    f32 v[3];
    f32 d[3];
    s16 hi;
    s16 lo;

    Player_UpdateCarryKind(p);
    p->timer++;
    switch (p->step) {
    case 0:
        p->step++;
        p->action = 2;
        switch (variant) {
        case 0:
        default:
            Player_SetMotionPair(p, 0x38, 0, 0);
            p->vel[0] = 0.0f;
            p->vel[2] = 20.0f;
            p->vel[1] = 18.0f;
            p->accel[0] = 0.0f;
            p->accel[2] = -(p->vel[2] / 80.0f);
            Player_StartJump(p, 0x13);
            break;
        case 1:
            Player_SetMotionPair(p, 0x38, 8, 0x20);
            p->rideState = 0;
            p->vel[0] = 0.0f;
            p->vel[2] = 8.0f;
            p->vel[1] = -10.0f;
            p->accel[2] = 0.0f;
            p->accel[0] = 0.0f;
            p->accel[1] = -8.0f / 11.0f;
            break;
        }
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 2, 0, 0);
        p->timer = 0;
        p->stateVar1 = 0;
        break;
    case 1:
        p->vel[0] += p->accel[0];
        p->vel[1] += p->accel[1];
        p->vel[2] += p->accel[2];
        if (p->vel[0] < 0.0f) {
            p->vel[0] = 0.0f;
        }
        if (p->vel[2] < 0.0f) {
            p->vel[2] = 0.0f;
        }
        v[0] = p->vel[0];
        v[1] = p->vel[1];
        v[2] = p->vel[2];
        Vec_Rotate(d, v, p->rotMtx);
        p->pos[0] += d[0];
        p->pos[1] += d[1];
        p->pos[2] += d[2];
        if (p->vel[1] < 0.0f) {
            p->timer++;
            if (!(p->floorY < p->pos[1])) {
                p->pos[1] = p->floorY;
                p->action = 0;
                p->rideState = 0;
                if (func_3B5440((Em *) p) == 1) {
                    hi = 60;
                    lo = 40;
                } else {
                    hi = 50;
                    lo = 30;
                }
                if (p->timer >= hi) {
                    p->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(p, 4);
                    Player_RequestState(p, 0, 0x26, 0);
                } else if (p->timer >= lo) {
                    Player_RequestState(p, 5, 7, 2);
                } else {
                    Player_RequestState(p, 5, 8, 2);
                }
            }
        }
        break;
    }
}

void Player_State5_07(Player *p, s32 variant)
{
    Player_UpdateCarryKind(p);
    switch (p->step) {
    case 0:
        p->step++;
        if (variant == 0) {
            Player_SetMotionPair(p, 0x39, 0, 0);
        } else {
            Player_SetMotionPair(p, 0x3A, 0, 0);
        }
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 5U, 0U, 2);
        }
        break;
    }
}

void Player_State5_0B(Player *p, s32 unused)
{
    p->noHitTimer = 3;
    switch (p->step) {
    case 0:
        p->step++;
        Player_UpdateCarryKind(p);
        if (p->motionId[0] != 0x33) {
            Player_SetMotionPair(p, 0x33, 4, 0);
        }
        p->tradeMsgTimer = 0x3C;
        p->timer = 0x3C;
        break;
    case 1:
        Player_UpdateCarryKind(p);
        if ((Pouch_FindCarryItem(p) & 0xFFFF) == 0xFFFF) {
            p->step++;
            Player_SetMotionPair(p, 0x36, 4, 0);
        }
        break;
    case 2:
        if (Motion_PassedFrame(p, 0, 114.0f) != 0) {
            p->step++;
            p->carry &= 0xF0;
            break;
        }
        Player_UpdateCarryKind(p);
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_StartIdleMotion(p, 8, 0);
        }
        break;
    case 4:
        if (p->tradeMsgTimer == 0) {
            Player_BeginState(p, 0, 2, 0);
        }
        break;
    }
}

void Player_RunGroup5(Player *p)
{
    p->weaponOut = 0;
    switch (p->state) {
    case 0:
        Player_State5_00(p, 0);
        break;
    case 1:
        Player_State5_01(p, 0);
        break;
    case 2:
        Player_State5_02(p, 0);
        break;
    case 3:
        Player_State5_03(p, 0);
        break;
    case 4:
        Player_State5_03(p, 1);
        break;
    case 5:
        Player_State5_05(p, 0);
        break;
    case 6:
        Player_State5_05(p, 1);
        break;
    case 7:
        Player_State5_07(p, 0);
        break;
    case 8:
        Player_State5_07(p, 1);
        break;
    case 9:
        Player_State5_00(p, 1);
        break;
    case 10:
        Player_State5_03(p, 2);
        break;
    case 11:
        Player_State5_0B(p, 0);
        break;
    default:
        Player_BeginState(p, 0, 4, 0);
        break;
    }
}

void Player_State6_00(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0xD2, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(p) == 1) {
            if (((s32) p->input.stickTilt >= 0x28) || (Quest_PhaseIsOver(1) == 1)) {
                Player_RequestState(p, 6U, 1U, 0);
                break;
            }
            if (p->actRequest != 0) {
                Player_StartAct(p);
            }
        } else {
        }
        break;
    }
}

void Player_State6_01(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0xCF, 0, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 0x3C;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State6_02(Player *p, s32 variant)
{
    f32 pos[3];
    s32 d;

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x26C, 6, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 60;
        break;
    case 1:
        if (p->timer > 0) {
            p->timer--;
        }
        if (Player_IsLocal(p) == 1 && p->timer == 0) {
            if (p->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(p, 0, 4, 0);
            } else if (p->actRequest != 0) {
                Player_StartAct(p);
            } else if (func_379E40() == 1 && p->unk8FE == 0) {
                func_379E50(pos, p->area);
                d = (u16) (p->rot[1] - (u16) (Vec_Heading(pos, p->pos) + 0x4000));
                if (d <= 0x1800 || d >= 0xE800) {
                    p->unk8FE = 1;
                    p->actDetectTimer = 300;
                }
            }
        }
        break;
    }
}

void Player_State6_03(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x26F, 6, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(p) == 1 && Motion_ReachedFrame(p, 0, 90.0f) != 0) {
            if (p->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_RequestState(p, 6, 4, 0);
            } else if (p->actRequest != 0) {
                Player_StartAct(p);
            }
        }
        break;
    }
}

void Player_State6_04(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x270, 6, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetFlag(p, 0x20000);
            Player_SetMotionPair(p, 0xCF, -4, 0x3C);
        }
        break;
    case 2:
        Player_SetFlag(p, 0x20000);
        if (p->motion[0].playing == 0) {
            Player_ClearFlag(p, 0x20000);
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Player_State6_05(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x274, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 60;
        break;
    case 1:
        if (p->timer > 0) {
            p->timer--;
        }
        if (Motion_PassedFrame(p, 0, 48.0f) != 0) {
            Eff00_Create(p, 0x60);
        }
        if (Motion_PassedFrame(p, 0, 68.0f) != 0) {
            Eff00_Create(p, 0x61);
        }
        if (Motion_PassedFrame(p, 0, 168.0f) != 0) {
            Eff00_Create(p, 0x61);
        }
        if (Motion_PassedFrame(p, 0, 190.0f) != 0) {
            Eff00_Create(p, 0x60);
        }
        if (Player_IsLocal(p) == 1 && p->timer == 0) {
            if (p->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(p, 0, 0xA, 0);
            } else if (p->actRequest != 0) {
                Player_StartAct(p);
            }
        }
        break;
    }
}

void Player_State6_06(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x278, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 0x3C;
        break;
    case 1:
        if (p->timer > 0) {
            p->timer--;
        }
        if (Player_IsLocal(p) == 1 && p->timer == 0) {
            if (p->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(p, 0, 0xA, 0);
            } else if (p->actRequest != 0) {
                Player_StartAct(p);
            }
        }
        break;
    }
}

void Player_State6_07(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x26E, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 0x3C;
        break;
    case 1:
        if (p->timer > 0) {
            p->timer--;
        }
        if (Player_IsLocal(p) == 1 && p->timer == 0) {
            if (p->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(p, 0, 0x4, 0);
            } else if (p->actRequest != 0) {
                Player_StartAct(p);
            }
        }
        break;
    }
}

void Player_State6_08(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x273, 6, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 0x3C;
        break;
    case 1:
        if (p->timer > 0) {
            p->timer--;
        }
        if (Player_IsLocal(p) == 1 && p->timer == 0) {
            if (p->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(p, 0, 4, 0);
            } else if (p->actRequest != 0) {
                Player_StartAct(p);
            }
        }
        break;
    }
}

void Player_State6_09(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, Player_State6_09_MotionStates[variant * 2], 4, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, Player_State6_09_MotionStates[variant * 2 + 1], 0);
        }
        break;
    }
}

void Player_State6_0A(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x271, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 0x3C;
        break;
    case 1:
        if (p->timer > 0) {
            p->timer--;
        }
        if (Player_IsLocal(p) == 1 && p->timer == 0) {
            if (p->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(p, 0, 0x4, 0);
            } else if (p->actRequest != 0) {
                Player_StartAct(p);
            }
        }
        break;
    }
}

void Player_State6_0B(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x276, 0x10, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 0x3C;
        break;
    case 1:
        if (p->timer > 0) {
            p->timer--;
        }
        if (Player_IsLocal(p) == 1 && p->timer == 0) {
            if (p->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(p, 0, 0xA, 0);
            } else if (p->actRequest != 0) {
                Player_StartAct(p);
            }
        }
        break;
    }
}

void Player_State6_0C(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x272, 6, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 0x3C;
        break;
    case 1:
        if (p->timer > 0) {
            p->timer--;
        }
        if (Player_IsLocal(p) == 1 && p->timer == 0) {
            if (p->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(p, 0, 4, 0);
            } else if (p->actRequest != 0) {
                Player_StartAct(p);
            }
        }
        break;
    }
}

void Player_State6_10(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x279, 6, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->timer = 0x3C;
        break;
    case 1:
        if (p->timer > 0) {
            p->timer--;
        }
        if (Player_IsLocal(p) == 1 && p->timer == 0) {
            if (p->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(p, 0, 4, 0);
            } else if (p->actRequest != 0) {
                Player_StartAct(p);
            }
        }
        break;
    }
}

/* The 0x716360 part's .data (0x8C3C90-0x8C3DE0) */
SpitMeat gSpitMeats[13] = {
#include "game/008C3C90.inc"
};
s8 Player_State2_13_Need[8] = {
#include "game/008C3DC8.inc"
};
s16 Player_State6_09_MotionStates[8] = {
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
void Em_SetRow638(Em *em, u8 idx);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_CheckKindRequest(Em *em);
void Em_Add7EC(Em *em, s32 n);
void Em_Add7F0(Em *em, s32 n);
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
void *EmScript_GetTable30(Em *em, u8 i);
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
void *EmScript_GetTable08b(Em *em, u8 i);
void Em_RestartScript(Em *em);
s32 Effect_ExistsKind16(void *pl);
void *EmScript_GetTable14(Em *em, u8 i);
void *EmScript_GetTable18(Em *em, u8 i);
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
void *EmScript_GetTable08(Em *obj, u8 i);
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
void Player_RunGroup0(Player *);
void Player_RunWeaponState(Player *);
void Player_RunGroup2(Player *);
void Player_RunGroup3(Player *em);
void Player_RunGroup4(Player *);
void Player_RunGroup5(Player *);
void Player_State6_00(Player *em, s32 arg1);
void Player_State6_01(Player *em, s32 arg1);
void Player_State6_02(Player *, s32);
void Player_State6_03(Player *, s32);
void Player_State6_04(Player *em, s32 arg1);
void Player_State6_05(Player *, s32);
void Player_State6_06(Player *, s32);
void Player_State6_07(Player *, s32);
void Player_State6_08(Player *, s32);
void Player_State6_09(Player *, s32);
void Player_State6_0A(Player *, s32);
void Player_State6_0B(Player *, s32);
void Player_State6_0C(Player *, s32);
void Player_State6_10(Player *, s32);
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

void Player_RunGroup6(Player *p)
{
    u8 state;

    Player_ResetAim(p);
    state = p->state;
    switch (state) {
    case 0:
        Player_State6_00(p, 0);
        break;
    case 1:
        Player_State6_01(p, 0);
        break;
    case 2:
        Player_State6_02(p, 0);
        break;
    case 3:
        Player_State6_03(p, 0);
        break;
    case 4:
        Player_State6_04(p, 0);
        break;
    case 5:
        Player_State6_05(p, 0);
        break;
    case 6:
        Player_State6_06(p, 0);
        break;
    case 7:
        Player_State6_07(p, 0);
        break;
    case 8:
        Player_State6_08(p, 0);
        break;
    case 9:
        Player_State6_09(p, 0);
        break;
    case 10:
        Player_State6_0A(p, 0);
        break;
    case 11:
        Player_State6_0B(p, 0);
        break;
    case 12:
        Player_State6_0C(p, 0);
        break;
    case 13:
        Player_State6_09(p, 1);
        break;
    case 14:
        Player_State6_06(p, 0);
        break;
    case 15:
        Player_State6_09(p, 3);
        break;
    case 16:
        Player_State6_10(p, 0);
        break;
    default:
        Player_BeginState(p, 0, 4, 0);
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

void Player_RunState(Player *p)
{
    p->freeState = 0;
    p->fieldCollide = 1;
    switch (p->stateGroup) {
    case 0:
        Player_RunGroup0(p);
        break;
    case 1:
        Player_RunWeaponState(p);
        break;
    case 2:
        Player_RunGroup2(p);
        break;
    case 3:
        Player_RunGroup3(p);
        break;
    case 4:
        Player_RunGroup4(p);
        break;
    case 5:
        Player_RunGroup5(p);
        break;
    case 6:
        Player_RunGroup6(p);
        
    default:
        break;
    }
}

void Player_CheckKushalaSeen(Player *p)
{
    Em *m;
    s16 i;

    if (Player_IsLocal(p) == 0 || Game_IsOnline() == 1) {
        return;
    }
    m = gEms;
    for (i = 0; i < 20; i++) {
        if (m->active != 0 && m->visible != 0 && m->type == 0x36 && m->area == p->area && Save_TestEventFlag(0x81) == 0) {
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
s16 HuntingHorn_StateKind(Player *);
extern u16 gFrameTick; /* sub_main: the frame counter */

s32 CamCtl_GetPlayerFade(f32 *out);
void Player_RecordActionSpot(Player *p);
void Player_UpdateAreaPos(void *obj);
void Player_UpdateNearMonster(Player *p);
s32 Em_FindFloorTri(Em *em, f32 *out, s32 apply);
void HuntingHorn_ClearNotes(struct Player *em);
void Player_ReactToHit(struct Player *em);
/* A player's per-frame update (Game_UpdatePlayers). */
void Player_Update(Player *p)
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

    if (p->active == 0) {
        return;
    }
    Player_CheckKushalaSeen(p);
    Player_UpdateInBattle(p);
    Player_TickTimers(p);
    p->pouchHudFlags = 0;
    p->offerStarted = 0;
    if (p->supplyBoxOpen == 0 || p->deliveryMenu != 0) {
        Player_UpdateItemSelect(p);
        Player_BowgunAmmoSwitch(p);
    }
    ((Em *) p)->prevPosX = p->pos[0];
    ((Em *) p)->prevPosY = p->pos[1];
    ((Em *) p)->prevPosZ = p->pos[2];
    p->unk58C = -1;
    p->unk58E = -1;
    p->unk3F8 = 0;
    p->bowgunReady = 0;
    p->freeState = 0;
    if (Player_IsLocal(p) == 1) {
        if (Player_IsState(p, 0, 9) == 0 && Player_IsState(p, 0, 0x1B) == 0) {
            p->unk893 = 0;
        }
        Player_CheckTeamComboLost(p);
    } else {
        p->unk893 = 0;
    }
    Player_ReactToHit((struct Player *)p);
    p->prevFreeState = 0;
    switch ((u8) Player_PickStatusState(p)) {
    case 1:
        Player_SetState(p, 2, 0x15, 0);
        break;
    case 2:
        if (Player_IsState(p, 2, 0x16) == 0 && Player_IsState(p, 2, 0x18) == 0) {
            Player_SetState(p, 2, 0x16, 0);
        }
        break;
    }
    Player_CalcAttack(p);
    Player_CalcDefense(p);
    Player_CalcResists(p);
    if (p->hitStop == 0) {
        Player_UpdateEnvironment(p);
        Player_RunState(p);
        if (p->stateRequest != 0) {
            Player_RunState(p);
            p->stateRequest = 0;
        }
        Player_UpdateAutoTurn(p);
        Player_FindLookTarget(p);
        if (p->stateGroup != 4) {
            Player_UpdateMoveTo(p);
        }
    }
    Player_CheckPointMessages((u8 *) p);
    p->prevFreeState = p->freeState;
    if (p->weaponOut == 0) {
        p->demonMode = 0;
    }
    Player_UpdateStatusIcons(p);
    if (p->statusIcons & 0x400) {
        StageObj_K29_TickType9(p);
    }
    if (p->statusIcons & 0x800) {
        StageObj_K29_TickType9Late(p);
    }
    p->actRequest = 0;
    p->offerRequest = 0;
    Player_UpdateHitStop(p);
    if (Player_IsState(p, 0, 0x27) == 0 || (Player_IsState(p, 0, 0x27) != 0 && p->stateVar0 == 0)) {
        Player_UpdateMotionStep(p);
    }
    Player_UpdateHitTimers(p);
    (*(void (**)(Player *)) ((u8 *) p->handlers + 0xC))(p);
    if (Player_IsState(p, 0, 0x1B) == 0 || Player_IsState(p, 0, 0x1E) == 0) {
        if ((u8) Obj_InCurrentArea(p)) {
            Em_CollideField((struct Em *)p, 0);
        }
    }
    if (p->stateGroup == 0 && (p->state == 1 || p->state == 2 || p->state == 3 || p->state == 0x13) && (u8) Obj_InCurrentArea(p)) {
        Em_FindFloorTri((Em *)p, buf, 1);
    }
    Player_RecordActionSpot(p);
    if (Em_GetFloorInfo((struct Em *)p, p->pos, (struct TriHdr *)(&(p)->floorKind), &floorY, (s32 *)(&(p)->surfaceY)) == 1 && Player_IsState(p, 4, 0) == 0) {
        p->floorY = floorY;
    }
    if ((u8) Obj_InCurrentArea(p)) {
        func_330DE0(p);
    }
    if (p->action != 2 && p->rideState == 0 && Player_IsState(p, 0, 0x3A) == 0 && p->stateGroup != 4 && (u8) Obj_InCurrentArea(p)) {
        fy = p->floorY;
        y = p->pos[1];
        if (y < fy) {
            p->pos[1] = fy;
            p->rideState = 0;
        } else if (y >= fy) {
            if (y - fy < 30.0f) {
                p->pos[1] = fy;
                p->rideState = 0;
            } else if (p->hp > 0) {
            g = p->stateGroup;
            if (g != 5) {
                if (g == 2) {
                switch (p->state) {
                case 27:
                case 30:
                case 31:
                case 32:
                case 33:
                case 34:
                case 35:
                    Player_SetState(p, 2, 0x24, 0);
                    break;
                default:
                    Player_SetState(p, 0, 9, 0);
                    break;
                }
            } else {
                Player_SetState(p, 0, 9, 0);
            }
            } else {
                Player_SetState(p, 5, 6, 0);
            }
            }
        }
    }
    p->unk417 = 1;
    Player_UpdateAreaPos(p);
    g = p->stateGroup;
    if (g != 2 && g != 3) {
        if (p->weaponOut && (p->weaponType == 1 || p->weaponType == 5) && (p->input.pressed & 0x80)) {
            if (g == 0) {
                switch (p->state) {
                case 0x1C:
                case 0x67:
                case 0x68:
                case 0x69:
                case 0x6A:
                    break;
                case 0x1B:
                case 0x3A:
                    p->aimMode = 0;
                    break;
                default:
                    p->aimMode ^= 1;
                    break;
                }
            } else if (g != 6 && Player_IsState(p, 1, 3) == 0) {
                p->aimMode ^= 1;
            }
            if (p->action == 2) {
                p->aimMode = 0;
            }
            if (Player_LocalAreaOk_Unk() == 0) {
                if ((u8) Obj_InCurrentArea(p) == 1) {
                    Sound_PlaySeFlat(0, 0x15, 0);
                }
                Player_ResetAim(p);
            }
        } else if (p->weaponOut == 0) {
            if (Player_IsState(p, 0, 0x36) != 0 || Player_IsState(p, 0, 0x7A) != 0 || Player_IsState(p, 0, 0x48) != 0) {
                if (p->input.pressed & 0x80) {
                    p->aimMode ^= 1;
                }
            } else if (Player_IsState(p, 0, 0x65) != 0 || Player_IsState(p, 0, 0x66) != 0) {
                p->aimMode = 1;
            } else {
                Player_ResetAim(p);
            }
        } else if (p->isGunner != 1) {
            if (Player_IsState(p, 0, 0x65) != 0 || Player_IsState(p, 0, 0x66) != 0) {
                p->aimMode = 1;
            } else {
                Player_ResetAim(p);
            }
        }
    }
    if (p->aimMode == 0) {
        p->aimLockTimer = 0;
    }
    if (p->weaponType == 8 && HuntingHorn_StateKind(p) == 0) {
        HuntingHorn_ClearNotes((struct Player *)p);
    }
    if (p->area == gScene.area) {
        Player_UpdateNearMonster(p);
        *(f32 *) ((u8 *) p->drawSlot + 8) = p->pos[0];
        *(f32 *) ((u8 *) p->drawSlot + 0xC) = p->pos[1];
        *(f32 *) ((u8 *) p->drawSlot + 0x10) = p->pos[2];
        if (Player_IsLocal(p) == 1) {
            p->nearCamera = 0;
        } else {
            pos[0] = p->pos[0];
            pos[1] = p->pos[1] + 80.0f;
            pos[2] = p->pos[2];
            if (Camera_Distance(pos) <= 200.0f) {
                p->nearCamera = 1;
            } else {
                p->nearCamera = 0;
            }
        }
        if (Player_IsLocal(p) == 0 && Player_TestFlag(p, 0x02001600) != 0) {
            p->rot[1] = (u16) (Vec_Heading(p->pos, p->prevPos) + 0x4000);
        }
        CamCtl_GetPlayerFade(&d);
        clamp = Player_InSceneEvent() == 1;
        if (p->nearCamera == 0 && !(d < 1.0f)) {
            OT_AddByDepth(gDrawOt32, p->drawSlot, 0x20, clamp);
        } else {
            OT_AddByDepth(gDrawOtSorted, p->drawSlot, 0x40, clamp);
        }
        if (p->syncTimer != 0) {
            (p->syncTimer)--;
        }
        n = *(s16 *)&gQSyncSendPeriod;
        if (p->syncTimer == 0 && gFrameTick % n == 0) {
            QSync_SendPlayer(p, 2, 0);
        }
    }
}

/* turns the monster (unkA4) toward unk00E by a step that depends on its state, and swings the
 * body angle at +0x69C (a u16) with it */
void Player_UpdateAutoTurn(Player *p)
{
    s32 step;
    u32 d;
    u16 turn;
    s32 target;
    s32 dir;

    if (p->areaMoveState != 0) {
        return;
    }
    if (p->weaponOut != 0 && p->stateGroup == 0 && (
        (u32) (p->state - 1) <= 1
        || p->state == 12)) {
        step = 0x71C;
    } else if (Player_IsState(p, 0, 1) != 0 || Player_IsState(p, 0, 0x24) != 0) {
        step = 0x71C;
    } else if (Player_IsState(p, 0, 0x1F) != 0) {
        step = 0x5B0;
    } else if (Player_IsState(p, 0, 0x3F) != 0) {
        step = 0;
    } else {
        step = 0xFA4;
    }
turn:
    dir = p->rot[1];
    target = p->targetYaw;
    d = (u16) (target - (u16) dir);
    switch (p->motionId[0]) {
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
        p->rot[1] = target;
        p->unk2B8 = 0;
        if (p->lean + 0x300 <= 0x600) {
            p->lean = 0;
        } else if (p->lean < 0x8000) {
            p->lean -= 0x300;
        } else {
            p->lean += 0x300;
        }
    } else if (d < 0x8000U) {
        p->rot[1] = (u16) (dir + step);
        p->lean -= turn;
    } else {
        p->rot[1] = (u16) (dir - step);
        p->lean += turn;
    }
    if (p->lean <= 0xF600 && p->lean >= 0xA00) {
        if (p->lean >= 0x8000) {
            p->lean = 0xF600;
        } else {
            p->lean = 0xA00;
        }
    }
}

void Player_FindLookTarget(Player *p)
{
    EmHeadView *h = (EmHeadView *) p;
    u32 i;
    u32 best;
    Em *e;
    f32 min;
    f32 d;
    u16 a;

    best = 0xFF;
    h->found = 0;
    min = 1500.0f;
    e = gEms;
    if (p->motionId[0] == 0x26) {
        for (i = 0; i < 20; i++, e++) {
            if (e->active != 0 && e->visible != 0) {
                d = Vec_Distance(&p->pos[0], &e->posX);
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
        a = (u16) (Vec_Heading(&gEms[best].posX, &p->pos[0]) + 0x4000) + 0x10000 - p->rot[1];
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

void Player_CheckPointMessages(u8 *p)
{
    f32 pos[3];
    u16 sp2C;
    s8 sp2F;
    s32 spot;

    if (Player_IsLocal(p) != 0) {
        spot = Player_FindActionSpot((Player *)p, (f32 *)pos, &sp2C, (u8 *)&sp2F) & 0xFFFF;
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

s16 Player_CountSpecialItems(Player *p)
{
    s16 n = 0;

    if (Pouch_Count(p, 0x2A4) > 0 || ItemGet_Result() == 0x2A4) {
        n++;
    }
    if (Pouch_Count(p, 0x2A6) > 0 || ItemGet_Result() == 0x2A6) {
        n++;
    }
    if (Pouch_Count(p, 0x2A3) > 0) {
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
    Player *p;
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
    for (p = gPlayers, i = 0; i < 4; i++, p++) {
        if (p->active == 0) {
            return;
        }
        have[i] = 0xFF;
        n = 0;
        if (func_3BCCB0(&p->equip[2], p->gender) == 1) {
            n++;
        }
        if (func_3BCCB0(&p->equip[3], p->gender) == 1) {
            n++;
        }
        if (func_3BCCB0(&p->equip[4], p->gender) == 1) {
            n++;
        }
        if (func_3BCCB0(&p->equip[5], p->gender) == 1) {
            n++;
        }
        if (func_3BCCB0(&p->equip[0], p->gender) == 1) {
            n++;
        }
        if (n == 0) {
            return;
        }
        have[i] = p->style;
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

void Player_CheckTeamComboLost(Player *p)
{
    s16 i;

    if (p->mealSkill != 0) {
        for (i = 0; i < gScene.playerCount; i++) {
            if (gScene.inQuest[i] != 1) {
                p->mealSkill = 0;
                p->mealSkillGroup = 0;
                Notice_Push(0, 0x1E, 0);
                return;
            }
        }
    }
}

void Player_SetEvadeFrames(Player *p)
{
    s16 v = 6;

    if (Player_HasSkill(p, 0xAF) == 1) {
        v += 2;
    } else if (Player_HasSkill(p, 0xB0) == 1) {
        v += 4;
    }
    p->noHitTimer = v;
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
                        Em_SetRow638(em, 0);
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

static void EmScript_SetVar95E(Em *em, u8 *p, s32 v)
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
    u8 *p;
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
    if ((p = EmScript_PickTrigger(em)) == NULL) {
        p = EmScript_GetPos(em);
    }
    while (run) {
        n++;
        if (n >= 1000) {
            EmScript_SetVar95E(em, p, 2);
        }
        if (em->scriptAbort != 0) {
            Em_RestartScript(em);
            break;
        }
        if (p == NULL) {
            em->scriptAbort = 0;
            Em_RestartScript(em);
            break;
        }
        switch (*p) {
        case 0x01:
            p = EmScript_Op01(em, p + 1);
            break;
        case 0x02:
            p = EmScript_Op02(em, p + 1);
            break;
        case 0x03:
            p = EmScript_Op03(em, p + 1);
            break;
        case 0x04:
            em->scriptNo = 0;
            p = (u8 *) Em_StartScript(em);
            break;
        case 0x05:
            p++;
            a = *p++;
            b = *p++;
            em->routedStateArg = *p++;
            if (em->remote == 0) {
                Em_ChangeStateRouted(em, a, b, 1);
            }
            EmScript_SetPos(em, p);
            run = 0;
            break;
        case 0x06:
            p = EmScript_Op06(em, p + 1);
            break;
        case 0x07:
            p = (u8 *) EmScript_Start(em, p + 1);
            break;
        case 0x08:
            p = EmScript_Op08(em, p + 1);
            break;
        case 0x1F:
            p = EmScript_Op1F(em, p + 1);
            break;
        case 0x09:
            p = EmScript_Op09(em, p + 1);
            break;
        case 0x0A:
            p = EmScript_Op0A(em, p + 1);
            break;
        case 0x0B:
            p = EmScript_Op0B(em, p + 1);
            break;
        case 0x0C:
            p = EmScript_Op0C(em, p + 1);
            break;
        case 0x0D:
            p = EmScript_Op0D(em, p + 1);
            break;
        case 0x0E:
            p = EmScript_Op0E(em, p + 1);
            break;
        case 0x0F:
            p = EmScript_Op0F(em, p + 1);
            break;
        case 0x10:
            p = EmScript_Op10(em, p + 1);
            break;
        case 0x11:
            p = EmScript_Op11(em, p + 1);
            break;
        case 0x12:
            p = EmScript_Op12(em, p + 1);
            break;
        case 0x13:
            p = EmScript_Op13(em, p + 1);
            break;
        case 0x14:
            p = EmScript_Op14(em, p + 1);
            break;
        case 0x15:
            p = EmScript_Op15(em, p + 1);
            break;
        case 0x16:
            p = EmScript_Call(em, p + 1);
            break;
        case 0x17:
            p = EmScript_Op17(em, p + 1);
            break;
        case 0x18:
            p = EmScript_Op18(em, p + 1);
            break;
        case 0x19:
            p = EmScript_Op19(em, p + 1);
            break;
        case 0x1A:
            p = EmScript_Op1A(em, p + 1);
            break;
        case 0x1B:
            p = EmScript_Op1B(em, p + 1);
            break;
        case 0x1C:
            p = EmScript_Op1C(em, p + 1);
            break;
        case 0x1D:
            p = EmScript_Op1D(em, p + 1);
            break;
        case 0x1E:
            p = EmScript_Op1E(em, p + 1);
            break;
        case 0x20:
            p = EmScript_Op20(em, p + 1);
            break;
        case 0x21:
            p = EmScript_Op21(em, p + 1);
            break;
        case 0x22:
            p = EmScript_Op22(em, p + 1);
            break;
        case 0x36:
            p = EmScript_Op36(em, p + 1);
            break;
        case 0x23:
            p = EmScript_Op23(em, p + 1);
            break;
        case 0x24:
            p = EmScript_Op24(em, p + 1);
            break;
        case 0x25:
            p = EmScript_Op25(em, p + 1);
            break;
        case 0x26:
            p = EmScript_Op26(em, p + 1);
            break;
        case 0x27:
            p = EmScript_Op27(em, p + 1);
            break;
        case 0x28:
            p = EmScript_Op28(em, p + 1);
            break;
        case 0x29:
            p = EmScript_Op29(em, p + 1);
            break;
        case 0x2A:
            p = EmScript_Op2A(em, p + 1);
            break;
        case 0x2B:
            p = EmScript_Op2B(em, p + 1);
            break;
        case 0x2C:
            p = EmScript_Op2C(em, p + 1);
            break;
        case 0x70:
            p = EmScript_Op70(em, p + 1);
            break;
        case 0x2D:
            p = EmScript_Op2D(em, p + 1);
            break;
        case 0x2E:
            p = EmScript_Op2E(em, p + 1);
            break;
        case 0x2F:
            p = EmScript_Op2F(em, p + 1);
            break;
        case 0x30:
            p = EmScript_Op30(em, p + 1);
            break;
        case 0x31:
            p = EmScript_Op31(em, p + 1);
            break;
        case 0x32:
            p = EmScript_Op32(em, p + 1);
            break;
        case 0x33:
            p = EmScript_Op33(em, p + 1);
            break;
        case 0x34:
            p = EmScript_Op34(em, p + 1);
            break;
        case 0x35:
            p = EmScript_Op35(em, p + 1);
            break;
        case 0x37:
            p = EmScript_Op37(em, p + 1);
            break;
        case 0x38:
            p = EmScript_Op38(em, p + 1);
            break;
        case 0x39:
            p = EmScript_Op39(em, p + 1);
            break;
        case 0x3A:
            p = EmScript_Op3A(em, p + 1);
            break;
        case 0x3B:
            p = EmScript_Op3B(em, p + 1);
            break;
        case 0x3C:
            p = EmScript_Op3C(em, p + 1);
            break;
        case 0x3D:
            p = EmScript_Op3D(em, p + 1);
            break;
        case 0x3E:
            p = EmScript_Op3E(em, p + 1);
            break;
        case 0x3F:
            p = EmScript_Op3F(em, p + 1);
            break;
        case 0x40:
            p = EmScript_Op40(em, p + 1);
            break;
        case 0x41:
            p = EmScript_Op41(em, p + 1);
            break;
        case 0x42:
            p = EmScript_Op42(em, p + 1);
            break;
        case 0x69:
            p = EmScript_Op69(em, p + 1);
            break;
        case 0x44:
            p = EmScript_Op44(em, p + 1);
            break;
        case 0x45:
            p = EmScript_Op45(em, p + 1);
            break;
        case 0x46:
            p = EmScript_Op46(em, p + 1);
            break;
        case 0x47:
            p = EmScript_Op47(em, p + 1);
            break;
        case 0x48:
            p = EmScript_Op48(em, p + 1);
            break;
        case 0x49:
            p = EmScript_Op49(em, p + 1);
            break;
        case 0x4A:
            p = EmScript_Op4A(em, p + 1);
            break;
        case 0x4B:
            p = EmScript_Op4B(em, p + 1);
            break;
        case 0x4C:
            p = EmScript_Op4C(em, p + 1);
            break;
        case 0x4D:
            p = EmScript_Op4D(em, p + 1);
            break;
        case 0x4E:
            p = EmScript_Op4E(em, p + 1);
            break;
        case 0x4F:
            p = EmScript_Op4F(em, p + 1);
            break;
        case 0x50:
            p = EmScript_Op50(em, p + 1);
            break;
        case 0x51:
            p = EmScript_Op51(em, p + 1);
            break;
        case 0x52:
            p = EmScript_Op52(em, p + 1);
            break;
        case 0x53:
            p = EmScript_Op53(em, p + 1);
            break;
        case 0x54:
            p = EmScript_Op54(em, p + 1);
            break;
        case 0x55:
            p = EmScript_Op55(em, p + 1);
            break;
        case 0x56:
            p = EmScript_Op56(em, p + 1);
            break;
        case 0x57:
            p = EmScript_Op57(em, p + 1);
            break;
        case 0x58:
            p = EmScript_Op58(em, p + 1);
            break;
        case 0x59:
            p = EmScript_Op59(em, p + 1);
            break;
        case 0x5A:
            p = EmScript_Op5A(em, p + 1);
            break;
        case 0x5B:
            p = EmScript_Op5B(em, p + 1);
            break;
        case 0x5C:
            p = EmScript_Op5C(em, p + 1);
            break;
        case 0x5D:
            p = EmScript_Op5D(em, p + 1);
            break;
        case 0x5E:
            p = EmScript_Op5E(em, p + 1);
            break;
        case 0x5F:
            p = EmScript_Op5F(em, p + 1);
            break;
        case 0x60:
            p = EmScript_Op60(em, p + 1);
            break;
        case 0x61:
            p = EmScript_Op61(em, p + 1);
            break;
        case 0x62:
            p = EmScript_Op62(em, p + 1);
            break;
        case 0x63:
            p = EmScript_Op63(em, p + 1);
            break;
        case 0x64:
            p = EmScript_Op64(em, p + 1);
            break;
        case 0x65:
            p = EmScript_Op65(em, p + 1);
            break;
        case 0x66:
            p = EmScript_Op66(em, p + 1);
            break;
        case 0x67:
            p = EmScript_Op67(em, p + 1);
            break;
        case 0x68:
            p = (u8 *) EmScript_Op68(em, (s32) (p + 1));
            break;
        case 0x80:
            p = EmScript_Op80(em, p + 1);
            break;
        case 0x81:
            p = EmScript_Op81(em, p + 1);
            break;
        case 0x82:
            p = EmScript_Op82(em, p + 1);
            break;
        case 0x83:
            p = EmScript_Op83(em, p + 1);
            break;
        case 0x84:
        case 0x7B:
            EmScript_Op7B(em);
            p++;
            break;
        case 0xFF:
            p = EmScript_OpFF(em, p + 1);
            break;
        case 0x90:
            p = EmScript_Op90(em, p + 1);
            break;
        case 0x91:
            p = EmScript_Op91(em, p + 1);
            break;
        case 0x99:
            p = EmScript_Op99(em, p + 1);
            break;
        case 0x92:
            p = (u8 *) EmScript_Op92(self, (s32) (p + 1));
            break;
        case 0x93:
            p = (u8 *) EmScript_Op93(self, (s32) (p + 1));
            break;
        case 0x94:
            p = EmScript_Op94(em, p + 1);
            break;
        case 0x71:
            p = EmScript_Op71(em, p + 1);
            break;
        case 0x72:
            p = EmScript_Op72(em, p + 1);
            break;
        case 0x73:
            p = EmScript_Op73(em, p + 1);
            break;
        case 0x74:
            p = EmScript_Op74(em, p + 1);
            break;
        case 0x75:
            p = EmScript_Op75(em, p + 1);
            break;
        case 0x76:
            p = EmScript_Op76(em, p + 1);
            break;
        case 0x77:
            p = EmScript_Op77(em, p + 1);
            break;
        case 0x78:
            p = EmScript_Op78(em, p + 1);
            break;
        case 0x79:
            p = EmScript_Op79(em, p + 1);
            break;
        case 0x7A:
            p = EmScript_Op7A(em, p + 1);
            break;
        case 0x7C:
            p = EmScript_Op7C(em, p + 1);
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

u8 *EmScript_Op01(Em *em, u8 *p)
{
    s32 i;
    s32 n;

    switch (*p) {
    case 0:
        p++;
        for (i = 0, n = 0; i < gScene.playerCount; i++) {
            if (em->sensedPlayers & (1 << i)) {
                n++;
            }
        }
        if (n == 0) {
            for (;;) {
                if (p[0] == 1 && p[1] == 1) {
                    break;
                }
                if (p[0] == 1 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 1, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 1 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p + 1, 1);
        break;
    case 2:
        p++;
        break;
    }
    return p;
}

u8 *EmScript_Op02(Em *em, u8 *p)
{
    switch (*p) {
    case 0:
        p++;
        if (em->noticed == 0) {
            *(s8 *) &em->scriptPlayer = -1;
            for (;;) {
                if (p[0] == 2 && p[1] == 1) {
                    break;
                }
                if (p[0] == 2 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 2, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 2 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p + 1, 2);
        break;
    case 2:
        p++;
        break;
    }
    return p;
}

u8 *EmScript_Op03(Em *em, u8 *p)
{
    s32 ok;

    switch (*p) {
    case 0:
        p++;
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
                if (p[0] == 3 && p[1] == 1) {
                    break;
                }
                if (p[0] == 3 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 3, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 3 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
            break;
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p + 1, 3);
        break;
    case 2:
        p++;
        break;
    }
    return p;
}

/* Script command: start script *p (Em_StartScript). */
s32 EmScript_Start(Em *em, u8 *p)
{
    em->scriptNo = *p;
    return Em_StartScript(em);
}

u8 *EmScript_Op08(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->stance != 0) {
            for (;;) {
                if (p[0] == 8 && p[1] == 1) {
                    break;
                }
                if (p[0] == 8 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 8, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 8 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 8);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op1F(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->stance != 1) {
            for (;;) {
                if (p[0] == 0x1F && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x1F && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x1F, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x1F && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x1F);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op09(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->stance != 2) {
            for (;;) {
                if (p[0] == 9 && p[1] == 1) {
                    break;
                }
                if (p[0] == 9 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 9, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 9 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 9);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op0A(Em *em, u8 *p)
{
    em->hitReactMode = *p;
    return p + 1;
}

u8 *EmScript_Op0B(Em *em, u8 *p)
{
    u8 v;

    switch (*p++) {
    case 0:
        v = *p++;
        em->scriptCombat = em->combat;
        if (em->combat != v) {
            for (;;) {
                if (p[0] == 0xB && p[1] == 1) {
                    break;
                }
                if (p[0] == 0xB && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0xB, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0xB && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0xB);
        break;
    case 2:
        break;
    }
    return p;
}

/* Script op 0x0C: k and v are the two bytes at p; sets the Em byte k picks to v (1: blind,
 * 2: unk814, 3: senseDelay, 4: unk78E[0], 5/8/9: work, 6/7: work bytes 0x44/0x45; 5-9 only for
 * some types and when not remote); k == 0 sets all of them (k 1-10). w is set inside the type
 * checks of cases 6 and 7: the loop hoists &em->work from there into its preheader (between the
 * compare constants 55 and 45) and keeps 0x44/0x45 as the store offsets.
 */
u8 *EmScript_Op0C(Em *em, u8 *p)
{
    u8 k;
    u8 v;
    s32 i;
    s32 n;
    u8 *w;

    k = *p++;
    v = *p++;
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
    return p;
}

/* Script op 0x0D: clears the Em byte the byte at p picks (the fields of EmScript_Op0C; 10:
 * unk882 = 0xFFFF); 0 clears all of them. w is set as in EmScript_Op0C.
 */
u8 *EmScript_Op0D(Em *em, u8 *p)
{
    u8 k;
    s32 i;
    s32 n;
    u8 *w;

    k = p[0];
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
    return p + 1;
}

u8 *EmScript_Op0E(Em *em, u8 *p)
{
    u16 id;

    switch (*p++) {
    case 0:
        id = p[0] << 8;
        id |= p[1];
        p += 2;
        if (em->area != Area_IdForCurrentTime(id)) {
            for (;;) {
                if (p[0] == 0xE && p[1] == 1) {
                    break;
                }
                if (p[0] == 0xE && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0xE, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0xE && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0xE);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op0F(Em *em, u8 *p)
{
    u8 *q;
    u8 n;
    s16 *e;

    em->loopIndex = 0;
    em->loopMode = p[0];
    n = p[1];
    em->loopPointTable = *(p += 2);
    em->loopScript = p[1];
    em->loopEndScript = *(p += 2);
    EmScript_SetPos(em, p + 1);
    em->scriptFlags |= 1;
    switch (em->loopMode) {
    case 0:
    case 1:
    case 2:
        e = (s16 *)Em_FindAreaEntry(em, em->tables[1], em->area);
        if (e == NULL) {
            em->scriptFlags &= 0xFE;
            return p + 1;
        }
        if (em->loopMode != 1 && e[1] < n) {
            n = e[1];
        }
        em->loopCount = n;
        em->areaPointCount = e[1];
        em->loopScriptPos = EmScript_GetTable08(em, em->loopScript);
        break;
    }
    return em->loopScriptPos;
}

//---
u8 *EmScript_Op10(Em *em, u8 *p)
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
            em->loopPoint = *(u8 *)EmScript_GetTable08b(em, em->loopPointTable);
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
    return p;
}

u8 *EmScript_Op11(Em *em, u8 *p)
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
    return p;
}

u8 *EmScript_Op12(Em *em, u8 *p)
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
    return p;
}

u8 *EmScript_Op13(Em *em, u8 *p)
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
    return p;
}

u8 *EmScript_Op14(Em *em, u8 *p)
{
    s32 ok;
    u8 k;
    u16 a;
    u16 d;
    u16 t;
    Player *pl;
    f32 pa[4];
    f32 pb[4];

    switch (*p++) {
    case 0:
        k = *p++;
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
            if (p[0] == 0x14 && p[1] == 1) {
                p = EmScript_NextCmd(em, p);
                if (p[0] == 0x14 && p[1] == 2) {
                    p = EmScript_NextCmd(em, p);
                } else {
                    p = EmScript_SkipBlock(em, p, 0x14, 2);
                    p = EmScript_NextCmd(em, p);
                }
            }
        } else {
            for (;;) {
                if (p[0] == 0x14 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x14 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x14, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x14 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x14);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op15(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u16 v;
    u8 ok;
    u16 d;
    u16 e;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = p[0] << 8;
                v |= p[1];
                p += 2;
                d = Area_DayId(v);
                e = Area_DayId(em->area);
                if (e == d) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x15, 3);
                if (p[1] == 2 || p[1] == 3) {
                    p += 2;
                    break;
                }
                p += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p += 2;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x15, 3);
            if (p[0] == 0x15 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Call(Em *em, u8 *p)
{
    em->subScriptNo = *p;
    em->subScriptReturn = p + 1;
    p = EmScript_SubScript(em, em->subScriptNo);
    em->scriptPos = p;
    return p;
}

/* Command 0x17 (4 argument bytes), once while +0x880 is -1: two pairs of bytes into +0x79A/+0x880 and
 * +0x79B/+0x7A0; a zero +0x880 becomes -1 again. The last step is two increments (`p += 2` is folded
 * into each return). */
u8 *EmScript_Op17(Em *em, u8 *p)
{
    if (em->areaSeqLen != -1) {
        return p + 4;
    }
    em->areaSeqTable = p[0];
    em->areaSeqLen = p[1];
    em->areaSeqScript = *(p += 2);
    em->areaSeqMode = p[1];
    em->areaSeqPos = 0;
    p++;
    p++;
    if (em->areaSeqLen == 0) {
        em->areaSeqLen = -1;
        return p;
    }
    return p;
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
        v = ((u16 *)EmScript_GetTable14(em, em->areaSeqTable))[em->areaSeqPos];
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
    p = EmScript_GetTable18(em, em->areaSeqScript);
    em->scriptPos = p;
    return p;
}

u8 *EmScript_Op19(Em *em, u8 *p)
{
    u16 dest;

    em->goalKind = 3;
    dest = *(u16 *) &em->destArea;
    em->goalNo = (s16) dest;
    em->goalArg = (s16) dest;
    return p;
}

/* Script op 0x1A: walks the route toward area (the two bytes at p): up to 20 steps of
 * Em_PickRoutePoint's pick from the areas Em_IdSetLookup/Em_IdSetHas list, until an area passes
 * Em_CanEnterArea or the target is reached; the result goes to Em +0x986, +0x77C and +0x77A.
 * `area = area | *p++` (not `|=`) gives the game's stack slot for area. */
u8 *EmScript_Op1A(Em *em, u8 *p)
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

    area = *p++ << 8;
    area = area | *p++;
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
    return p;
}

u8 *EmScript_Op1B(Em *em, u8 *p)
{
    u8 v;

    switch (*p++) {
    case 0:
        v = *p++;
        if (v != em->requested) {
            for (;;) {
                if (p[0] == 0x1B && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x1B && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x1B, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x1B && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x1B);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op1C(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;
    u8 x;

    switch (*p++) {
    case 0:
        x = em->hp * 100 / em->hpMax;
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = *p++;
                if (v > x) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x1C, 3);
                if (p[1] == 2 || p[1] == 3) {
                    p += 2;
                    break;
                }
                p += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x1C, 3);
            if (p[0] == 0x1C && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op1D(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = *p++;
                if (em->requestId == v) {
                    break;
                }
                if (v < em->requestId) {
                    p = EmScript_SkipBlock(em, p, 0x1D, 3);
                    if (p[1] == 2 || p[1] == 3) {
                        p += 2;
                        break;
                    }
                    p += 2;
                } else {
                    ok = 1;
                    do {
                        p = EmScript_SkipBlock(em, p, 0x1D, 3);
                        if ((p[0] == 0x1D && p[1] == 2) || (p[0] == 0x1D && p[1] == 3)) {
                            ok = 0;
                        }
                        p = EmScript_NextCmd(em, p);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x1D, 3);
            if (p[0] == 0x1D && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op1E(Em *em, u8 *p)
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
    return p;
}

u8 *EmScript_Op20(Em *em, u8 *p)
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

    switch (*p++) {
    case 0:
        n = p[0];
        p += 3;
        if (em->goalKind == 1 && *(u16 *) &em->goalArg == 0 && (s16) (k = *(u16 *) &em->goalNo) != -1) {
            Em_GetTargetPos(em, k, a);
            pl = &gPlayers[k];
            Area_ToWorld(pl->area, a, b);
            ang = Math_AngleXZ(em->mapPos, b) - em->rotY;
        } else {
            ok = 1;
            do {
                p = EmScript_SkipBlock(em, p, 0x20, 3);
                if ((p[0] == 0x20 && p[1] == 2) || (p[0] == 0x20 && p[1] == 3)) {
                    ok = 0;
                }
                p = EmScript_NextCmd(em, p);
            } while (ok);
            break;
        }
        i = 0;
        if (i < n) {
            do {
                v = *p++ << 8;
                if (v >= ang) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x20, 3);
                if (p[1] == 2 || p[1] == 3) {
                    p += 2;
                    break;
                }
                p += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x20, 3);
            if (p[0] == 0x20 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op21(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->need2Gauge > (s32) (0.3f * em->need2Max)) {
            for (;;) {
                if (p[0] == 0x21 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x21 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x21, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x21 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x21);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op22(Em *em, u8 *p)
{
    u8 v;
    f32 lim;

    switch (*p++) {
    case 0:
        lim = 100.0f * *p++;
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= lim)) {
            for (;;) {
                if (p[0] == 0x22 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x22 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x22, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x22 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x22);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op36(Em *em, u8 *p)
{
    u8 v;
    f32 lim;

    switch (*p++) {
    case 0:
        lim = 100.0f * *p++;
        if (!(Vec_Distance(&em->posX, em->goal) <= lim)) {
            for (;;) {
                if (p[0] == 0x36 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x36 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x36, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x36 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x36);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op23(Em *em, u8 *p)
{
    u8 n;
    u8 v;
    u8 found;
    s32 i;
    s32 j;
    u8 ok;
    Em *e;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = *p++;
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
                p = EmScript_SkipBlock(em, p, 0x23, 3);
                if (p[1] == 2 || p[1] == 3) {
                    p += 2;
                    break;
                }
                p += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x23, 3);
            if (p[0] == 0x23 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op24(Em *em, u8 *p)
{

    switch (*p++) {
    case 0:
        em->repeatCount = *p++;
        em->repeatPos = p;
        if (em->repeatCount <= 0) {
            for (;;) {
                if (p[0] == 0x24 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x24, 1);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x24 && p[1] == 1) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        if (--em->repeatCount > 0) {
            p = em->repeatPos;
        }
        break;
    }
    return p;
}

u8 *EmScript_Op25(Em *em, u8 *p)
{
    em->repeatCount = 0;
    return p;
}

u8 *EmScript_Op26(Em *em, u8 *p)
{
    em->noAlert = *p;
    return p + 1;
}

u8 *EmScript_Op27(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = *p++;
                if (em->variant == v) {
                    break;
                }
                if (v < em->variant) {
                    p = EmScript_SkipBlock(em, p, 0x27, 3);
                    if (p[1] == 2 || p[1] == 3) {
                        p += 2;
                        break;
                    }
                    p += 2;
                } else {
                    ok = 1;
                    do {
                        p = EmScript_SkipBlock(em, p, 0x27, 3);
                        if ((p[0] == 0x27 && p[1] == 2) || (p[0] == 0x27 && p[1] == 3)) {
                            ok = 0;
                        }
                        p = EmScript_NextCmd(em, p);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x27, 3);
            if (p[0] == 0x27 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op75(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = *p++;
                if (em->spawnParam == v) {
                    break;
                }
                if (v < em->spawnParam) {
                    p = EmScript_SkipBlock(em, p, 0x75, 3);
                    if (p[1] == 2 || p[1] == 3) {
                        p += 2;
                        break;
                    }
                    p += 2;
                } else {
                    ok = 1;
                    do {
                        p = EmScript_SkipBlock(em, p, 0x75, 3);
                        if ((p[0] == 0x75 && p[1] == 2) || (p[0] == 0x75 && p[1] == 3)) {
                            ok = 0;
                        }
                        p = EmScript_NextCmd(em, p);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x75, 3);
            if (p[0] == 0x75 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op76(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;
    u8 x;

    switch (*p++) {
    case 0:
        x = Quest_GetSeason();
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = *p++;
                if (x == v) {
                    break;
                }
                if (v < x) {
                    p = EmScript_SkipBlock(em, p, 0x76, 3);
                    if (p[1] == 2 || p[1] == 3) {
                        p += 2;
                        break;
                    }
                    p += 2;
                } else {
                    ok = 1;
                    do {
                        p = EmScript_SkipBlock(em, p, 0x76, 3);
                        if ((p[0] == 0x76 && p[1] == 2) || (p[0] == 0x76 && p[1] == 3)) {
                            ok = 0;
                        }
                        p = EmScript_NextCmd(em, p);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x76, 3);
            if (p[0] == 0x76 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op7A(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;
    u8 x;

    switch (*p++) {
    case 0:
        x = Area_BaseId(gScene.map);
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = Area_BaseId(*p++);
                if (x == v) {
                    break;
                }
                if (v < x) {
                    p = EmScript_SkipBlock(em, p, 0x7A, 3);
                    if (p[1] == 2 || p[1] == 3) {
                        p += 2;
                        break;
                    }
                    p += 2;
                } else {
                    ok = 1;
                    do {
                        p = EmScript_SkipBlock(em, p, 0x7A, 3);
                        if ((p[0] == 0x7A && p[1] == 2) || (p[0] == 0x7A && p[1] == 3)) {
                            ok = 0;
                        }
                        p = EmScript_NextCmd(em, p);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x7A, 3);
            if (p[0] == 0x7A && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op79(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;
    u8 x;
    s32 k;
    u8 (*fn)(Em *, u8);

    switch (*p++) {
    case 0:
        fn = em->handlers->unk10;
        n = *p++;
        k = *p++;
        if (fn != NULL) {
            x = fn(em, k);
        } else {
            x = 0;
        }
        p += 2;
        i = 0;
        if (i < n) {
            do {
                v = *p++;
                if (x == v) {
                    break;
                }
                if (v < x) {
                    p = EmScript_SkipBlock(em, p, 0x79, 3);
                    if (p[1] == 2 || p[1] == 3) {
                        p += 2;
                        break;
                    }
                    p += 2;
                } else {
                    ok = 1;
                    do {
                        p = EmScript_SkipBlock(em, p, 0x79, 3);
                        if ((p[0] == 0x79 && p[1] == 2) || (p[0] == 0x79 && p[1] == 3)) {
                            ok = 0;
                        }
                        p = EmScript_NextCmd(em, p);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x79, 3);
            if (p[0] == 0x79 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op28(Em *em, u8 *p)
{
    Player *pl = gPlayers;
    s32 i;
    u8 ok;

    switch (*p++) {
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
                if (p[0] == 0x28 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x28 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x28, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x28 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x28);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op29(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->areaTimer1 > 0) {
            for (;;) {
                if (p[0] == 0x29 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x29 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x29, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x29 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x29);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op2A(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->areaTimer2 > 0) {
            for (;;) {
                if (p[0] == 0x2A && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x2A && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x2A, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x2A && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x2A);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op2B(Em *em, u8 *p)
{
    u8 k;
    u8 v;
    u8 ne;

    switch (*p++) {
    case 0:
        k = p[0];
        v = p[1];
        p += 2;
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
            if (p[0] == 0x2B && p[1] == 1) {
                break;
            }
            if (p[0] == 0x2B && p[1] == 2) {
                break;
            }
            p = EmScript_SkipBlock(em, p, 0x2B, 2);
        }
        p = EmScript_NextCmd(em, p);
        if (p[0] == 0x2B && p[1] == 2) {
            p = EmScript_NextCmd(em, p);
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x2B);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op2C(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 ok;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                if (Em_KindMatches(em->type, *p++) != 0) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x2C, 3);
                if (p[1] == 2 || p[1] == 3) {
                    p += 2;
                    break;
                }
                p += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x2C, 3);
            if (p[0] == 0x2C && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op70(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = *p++;
                if (em->type == v) {
                    break;
                }
                if (v < em->type) {
                    p = EmScript_SkipBlock(em, p, 0x70, 3);
                    if (p[1] == 2 || p[1] == 3) {
                        p += 2;
                        break;
                    }
                    p += 2;
                } else {
                    ok = 1;
                    do {
                        p = EmScript_SkipBlock(em, p, 0x70, 3);
                        if ((p[0] == 0x70 && p[1] == 2) || (p[0] == 0x70 && p[1] == 3)) {
                            ok = 0;
                        }
                        p = EmScript_NextCmd(em, p);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x70, 3);
            if (p[0] == 0x70 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op2D(Em *em, u8 *p)
{
    em->goalKind = 7;
    em->goalArg = (s16) em->actionGoal[1];
    em->goalNo = (s16) em->actionGoal[2];
    return p;
}

u8 *EmScript_Op2E(Em *em, u8 *p)
{
    Em_SetRow638(em, *p);
    return p + 1;
}

u8 *EmScript_Op2F(Em *em, u8 *p)
{
    s8 i;
    u8 found;

    switch (*p++) {
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
                if (p[0] == 0x2F && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x2F && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x2F, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x2F && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x2F);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op30(Em *em, u8 *p)
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
    return p;
}

u8 *EmScript_Op31(Em *em, u8 *p)
{
    em->goalKind = 8;
    return p;
}

u8 *EmScript_Op32(Em *em, u8 *p)
{
    u8 v;

    switch (*p++) {
    case 0:
        v = *p++;
        if (em->hitReactMode != v) {
            for (;;) {
                if (p[0] == 0x32 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x32 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x32, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x32 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x32);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op33(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = *p++;
                if (em->hitReactMode == v) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x33, 3);
                if (p[1] == 2 || p[1] == 3) {
                    p += 2;
                    break;
                }
                p += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x33, 3);
            if (p[0] == 0x33 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op34(Em *em, u8 *p)
{
    u8 g;
    u8 s;

    switch (*p++) {
    case 0:
        g = p[0];
        s = p[1];
        p += 2;
        if (em->stateGroup != g || em->state != s) {
            for (;;) {
                if (p[0] == 0x34 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x34 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x34, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x34 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x34);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op35(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->rage == 0) {
            for (;;) {
                if (p[0] == 0x35 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x35 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x35, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x35 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x35);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op37(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (!(gQuest.flags & 0x200000)) {
            for (;;) {
                if (p[0] == 0x37 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x37 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x37, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x37 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x37);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op38(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (!func_330E80(em)) {
            for (;;) {
                if (p[0] == 0x38 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x38 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x38, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x38 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x38);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op39(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->blindTimer == 0) {
            for (;;) {
                if (p[0] == 0x39 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x39 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x39, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x39 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x39);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op3A(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->senseDelay != 0) {
            for (;;) {
                if (p[0] == 0x3A && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x3A && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x3A, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x3A && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x3A);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op3B(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->link == NULL) {
            for (;;) {
                if (p[0] == 0x3B && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x3B && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x3B, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x3B && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x3B);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op3C(Em *em, u8 *p)
{
    u8 ok;

    switch (*p++) {
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
                if (p[0] == 0x3C && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x3C && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x3C, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x3C && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x3C);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op3D(Em *em, u8 *p)
{
    u8 ok;
    u16 id;

    switch (*p++) {
    case 0:
        id = p[0] << 8;
        id |= p[1];
        p += 2;
        if (id == 0xFFFF) {
            ok = 1;
        } else {
            if (em->prevArea == Area_IdForCurrentTime(id)) {
                break;
            }
            ok = 1;
        }
        while (ok) {
            if (p[0] == 0x3D && p[1] == 1) {
                break;
            }
            if (p[0] == 0x3D && p[1] == 2) {
                break;
            }
            p = EmScript_SkipBlock(em, p, 0x3D, 2);
        }
        p = EmScript_NextCmd(em, p);
        if (p[0] == 0x3D && p[1] == 2) {
            p = EmScript_NextCmd(em, p);
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x3D);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op3E(Em *em, u8 *p)
{
    u16 x = em->prevArea;
    u8 n;
    s32 i;
    u16 v;
    u8 ok;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = p[0] << 8;
                v |= p[1];
                p += 2;
                if (x == Area_IdForCurrentTime(v) && x != 0xFFFF) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x3E, 3);
                if (p[1] == 2 || p[1] == 3) {
                    p += 2;
                    break;
                }
                p += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p += 2;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x3E, 3);
            if (p[0] == 0x3E && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

/* Script command 0x3F: move to area p[0..1] (big-endian) by a route. Picks the route to that
 * area (the one starting nearest when the area has no id at this time), then the point to start
 * at: outside the scene's area the nearest flagged one, inside it the first flagged one in line
 * of sight (AreaHit_Raycast) at height p[2] * 10. */
u8 *EmScript_Op3F(Em *em, u8 *p)
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
    area = p[0] << 8;
    area |= p[1];
    em->routeHeight = *(p += 2);
    h = 10.0f * em->routeHeight;
    if (em->area == Area_IdForCurrentTime(area)) {
        return p + 1;
    }
    set = (EmRouteSet *) Em_FindAreaEntry(em, em->tables[6], em->area);
    r = set->routes;
    if (r == NULL) {
        return p + 1;
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
            return p + 1;
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
    em->routeResume = p + 1;
    em->routePointScript = pt->unk12;
    return EmScript_GetTable30(em, em->routePointScript);
}

u8 *EmScript_Op40(Em *em, u8 *p)
{
    if (*p == 0) {
        Em_SetCombat(em, 0, 0);
    } else {
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
    }
    return p + 1;
}

u8 *EmScript_Op41(Em *em, u8 *p)
{
    u8 temp_a1;
    u8 type;

    type = em->type;
    temp_a1 = *p;
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
    return p + 1;
}

u8 *EmScript_Op42(Em *em, u8 *p)
{
    s32 ok;
    u8 k;
    u16 a;
    u16 d;

    switch (*p++) {
    case 0:
        Em_ComputeGoal(em, em->goal);
        k = *p++;
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
            if (p[0] == 0x42 && p[1] == 1) {
                p = EmScript_NextCmd(em, p);
                if (p[0] == 0x42 && p[1] == 2) {
                    p = EmScript_NextCmd(em, p);
                } else {
                    p = EmScript_SkipBlock(em, p, 0x42, 2);
                    p = EmScript_NextCmd(em, p);
                }
            }
        } else {
            for (;;) {
                if (p[0] == 0x42 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x42 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x42, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x42 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x42);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op69(Em *em, u8 *p)
{
    s32 ok;
    u8 k;
    u16 a;
    u16 d;

    switch (*p++) {
    case 0:
        Em_ComputeGoal(em, em->goal);
        k = *p++;
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
            if (p[0] == 0x69 && p[1] == 1) {
                p = EmScript_NextCmd(em, p);
                if (p[0] == 0x69 && p[1] == 2) {
                    p = EmScript_NextCmd(em, p);
                } else {
                    p = EmScript_SkipBlock(em, p, 0x69, 2);
                    p = EmScript_NextCmd(em, p);
                }
            }
        } else {
            for (;;) {
                if (p[0] == 0x69 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x69 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x69, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x69 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x69);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op44(Em *em, u8 *p)
{
    u8 ok;

    switch (*p++) {
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
            if (p[0] == 0x44 && p[1] == 1) {
                break;
            }
            if (p[0] == 0x44 && p[1] == 2) {
                break;
            }
            p = EmScript_SkipBlock(em, p, 0x44, 2);
        }
        p = EmScript_NextCmd(em, p);
        if (p[0] == 0x44 && p[1] == 2) {
            p = EmScript_NextCmd(em, p);
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x44);
        break;
    case 2:
        break;
    }
    return p;
}

/* Command 0x45 (if/else/end): true when exactly one player is in the area and it passes flag 0x80000 and
 * Effect_ExistsKind16; the monster then targets it (+0x798). */
u8 *EmScript_Op45(Em *em, u8 *p)
{
    Player *pl = gPlayers;
    u8 list[4];
    u8 ok;
    s8 i;
    s8 n;
    s8 cnt;
    u8 *w;

    switch (*p++) {
    case 0:
        for (i = 0, n = 0, cnt = 0, ok = 0, w = list; i < 4; i++, pl++) {
            if (pl->active != 0 && Obj_SameArea(em, pl) != 0) {
                cnt++;
                if (cnt > 1) {
                    ok = 0;
                    break;
                }
                if (Player_TestFlag(pl, 0x80000) != 0 && Effect_ExistsKind16(pl) == 1) {
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
                if (p[0] == 0x45 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x45 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x45, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x45 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x45);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op46(Em *em, u8 *p)
{
    u8 g;
    u8 s;

    switch (*p++) {
    case 0:
        g = p[0];
        s = p[1];
        p += 2;
        if (Player_IsState(&gPlayers[(s8) em->scriptPlayer & 0xF], g, s) == 0) {
            for (;;) {
                if (p[0] == 0x46 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x46 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x46, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x46 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x46);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op47(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->hooked != 0) {
            em->hooked = 0;
        } else {
            for (;;) {
                if (p[0] == 0x47 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x47 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x47, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x47 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x47);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op48(Em *em, u8 *p)
{
    em->scriptDelay = *p;
    return p + 1;
}

u8 *EmScript_Op49(Em *em, u8 *p)
{
    s32 v = *p;

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
    return p + 1;
}

u8 *EmScript_Op4A(Em *em, u8 *p)
{
    s8 k;

    switch (*p++) {
    case 0:
        k = em->scriptPlayer;
        if (!(em->seen & (1 << (k & 0xF))) || k == -1) {
            for (;;) {
                if (p[0] == 0x4A && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x4A && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x4A, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x4A && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x4A);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op4B(Em *em, u8 *p)
{
    s8 idx;

    idx = (s8) em->scriptPlayer;
    if (idx != -1) {
        em->alertBuildup[idx & 0xF] = 0;
    }
    return p;
}

u8 *EmScript_Op4C(Em *em, u8 *p)
{
    s8 idx;

    idx = (s8) em->scriptPlayer;
    if (idx != -1) {
        em->noticeCount[idx & 0xF] = 0;
    }
    return p;
}

u8 *EmScript_Op4D(Em *em, u8 *p)
{
    Em_ComputeGoal(em, em->goal);
    return p;
}

u8 *EmScript_Op4E(Em *em, u8 *p)
{
    switch (*p) {
    case 0:
        Em_Add7EC(em, em->need2Max / 2);
        break;
    }
    return p + 1;
}

u8 *EmScript_Op4F(Em *em, u8 *p)
{
    switch (*p) {
    case 0:
        Em_Add7F0(em, em->need1Max / 2);
        break;
    }
    return p + 1;
}

u8 *EmScript_Op50(Em *em, u8 *p)
{
    switch (*p) {
    case 0:
        Em_Add7F4(em, em->need3Max / 2);
        break;
    }
    return p + 1;
}

u8 *EmScript_Op51(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (em->stance != 4) {
            for (;;) {
                if (p[0] == 0x51 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x51 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x51, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x51 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x51);
        break;
    case 2:
        break;
    }
    return p;
}

/* Script command 0x52: picks the target player into +0x798 among those it is angry with (+0x7E3)
 * and can reach: anger (+0x86C) 50000 or more first, then 30000, by the largest +0x818; else the
 * angriest (none if all are 0); -1 when none. */
u8 *EmScript_Op52(Em *em, u8 *p)
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
    return p;
}

u8 *EmScript_Op53(Em *em, u8 *p)
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
            return p;
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
    return p;
}

u8 *EmScript_Op54(Em *em, u8 *p)
{
    s8 k;
    Player *pl;

    switch (*p++) {
    case 0:
        k = em->scriptPlayer;
        if (k == -1 || Obj_SameArea(em, pl = &gPlayers[k]) == 0 || pl->active == 0) {
            for (;;) {
                if (p[0] == 0x54 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x54 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x54, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x54 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x54);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op55(Em *em, u8 *p)
{
    s8 k;

    switch (*p++) {
    case 0:
        k = em->scriptPlayer;
        if (k == -1 || em->noticeCount[k] < 30000) {
            for (;;) {
                if (p[0] == 0x55 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x55 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x55, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x55 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x55);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op56(Em *em, u8 *p)
{
    u16 day;

    switch (*p++) {
    case 0:
        day = p[0] << 8;
        day |= p[1];
        p += 2;
        if (gQuest.stage != day) {
            for (;;) {
                if (p[0] == 0x56 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x56 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x56, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x56 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x56);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op57(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u16 v;
    u8 ok;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = p[0] << 8;
                v |= p[1];
                p += 2;
                if (em->questKey == v) {
                    break;
                }
                if (v < em->questKey) {
                    p = EmScript_SkipBlock(em, p, 0x57, 3);
                    if (p[1] == 2 || p[1] == 3) {
                        p += 2;
                        break;
                    }
                    p += 2;
                } else {
                    ok = 1;
                    do {
                        p = EmScript_SkipBlock(em, p, 0x57, 3);
                        if ((p[0] == 0x57 && p[1] == 2) || (p[0] == 0x57 && p[1] == 3)) {
                            ok = 0;
                        }
                        p = EmScript_NextCmd(em, p);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p += 2;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x57, 3);
            if (p[0] == 0x57 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op58(Em *em, u8 *p)
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
    return p;
}

u8 *EmScript_Op59(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if ((s8) em->ceilingTimer == 0) {
            for (;;) {
                if (p[0] == 0x59 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x59 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x59, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x59 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x59);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op5A(Em *em, u8 *p)
{
    u8 v;
    s8 k;

    switch (*p++) {
    case 0:
        v = *p++;
        if (em->curGoalKind == 0xB || em->curGoalKind == 1) {
            k = em->scriptPlayer;
            if (k != -1 && v == ((Player *) ((u8 *) gPlayers + (k & 0xF) * sizeof(Player)))->floorAttr) {
                break;
            }
        }
        for (;;) {
            if (p[0] == 0x5A && p[1] == 1) {
                break;
            }
            if (p[0] == 0x5A && p[1] == 2) {
                break;
            }
            p = EmScript_SkipBlock(em, p, 0x5A, 2);
        }
        p = EmScript_NextCmd(em, p);
        if (p[0] == 0x5A && p[1] == 2) {
            p = EmScript_NextCmd(em, p);
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x5A);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op5B(Em *em, u8 *p)
{
    u8 v = *p;
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
    return p + 1;
}

u8 *EmScript_Op5C(Em *em, u8 *p)
{
    f32 a;
    f32 b;

    switch (*p++) {
    case 0:
        if (em->area != gScene.area || AreaGrid2_GetCeiling(&em->posX, &a, (struct AreaTri *) &b) == 0) {
            for (;;) {
                if (p[0] == 0x5C && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x5C && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x5C, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x5C && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x5C);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op5D(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        Em_ComputeGoal(em, em->goal);
        if (em->goal[0] == 0.0f || em->goal[1] == 0.0f || em->goal[2] == 0.0f) {
            for (;;) {
                if (p[0] == 0x5D && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x5D && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x5D, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x5D && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x5D);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op5E(Em *em, u8 *p)
{
    u8 v;

    switch (*p++) {
    case 0:
        v = *p++;
        if (*((u8 *) em + 0x662) != v) {
            for (;;) {
                if (p[0] == 0x5E && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x5E && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x5E, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x5E && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x5E);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op5F(Em *em, u8 *p)
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
            return p;
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
    return p;
}

u8 *EmScript_Op60(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (gScene.unk20A == 0) {
            for (;;) {
                if (p[0] == 0x60 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x60 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x60, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x60 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x60);
        break;
    case 2:
        break;
    }
    return p;
}

struct EmIdPair;
void Em_SetAreaValue8A0(Em *em, struct EmIdPair *pair);
u8 *EmScript_Op61(Em *em, u8 *p)
{
    Em_SetAreaValue8A0(em, (struct EmIdPair *)gEmAreaValues8A0[em->type]);
    return p;
}

//---
/* Command 0x62 (a branch on the target player, 0..3 then 0xFF): picks branch 1 (runs on), 2 or 3 from the
 * target player +0x662 and the monster +0x662, skipping to it; no target: branch 3. As in the game, go is
 * only set on that path (the others read it uninitialized). */
u8 *EmScript_Op62(Em *em, u8 *p)
{
    Player *pl;
    s8 k;
    u8 r;
    u8 go;
    u8 want;

    switch (*p++) {
    case 0:
        k = em->scriptPlayer;
        p += 2;
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
            p = EmScript_SkipBlock(em, p, 0x62, 0xFF);
            if ((p[0] == 0x62 && p[1] == want) || (p[0] == 0x62 && p[1] == 0xFF)) {
                go = 0;
            }
            p = EmScript_NextCmd(em, p);
        }
        break;
    case 1:
    case 2:
    case 3:
        for (;;) {
            p = EmScript_SkipBlock(em, p, 0x62, 0xFF);
            if (p[1] == 0xFF) {
                p += 2;
                break;
            }
            p += 2;
        }
        break;
    case 0xFF:
        break;
    }
    return p;
}

u8 *EmScript_Op63(Em *em, u8 *p)
{
    u8 v;

    switch (*p++) {
    case 0:
        v = *p++;
        if (!(em->variant & v)) {
            for (;;) {
                if (p[0] == 0x63 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x63 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x63, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x63 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x63);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op77(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (Daytime_Get() & 0xFF) {
            for (;;) {
                if (p[0] == 0x77 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x77 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x77, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x77 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x77);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op7C(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if ((u32) ((Area_BaseId(gScene.map) & 0xFF) - 9) > 1) {
            for (;;) {
                if (p[0] == 0x7C && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x7C && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x7C, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x7C && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x7C);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op78(Em *em, u8 *p)
{
    s32 ok;
    u16 lo;
    u16 hi;
    u16 d;
    u16 t;
    Player *pl;
    f32 pa[4];
    f32 pb[4];

    switch (*p++) {
    case 0:
        lo = p[0];
        hi = p[1];
        ok = 0;
        p += 2;
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
            if (p[0] == 0x78 && p[1] == 1) {
                p = EmScript_NextCmd(em, p);
                if (p[0] == 0x78 && p[1] == 2) {
                    p = EmScript_NextCmd(em, p);
                } else {
                    p = EmScript_SkipBlock(em, p, 0x78, 2);
                    p = EmScript_NextCmd(em, p);
                }
            }
        } else {
            for (;;) {
                if (p[0] == 0x78 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x78 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x78, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x78 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x78);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op64(Em *em, u8 *p)
{
    u8 v;
    s8 k;

    switch (*p++) {
    case 0:
        v = *p++;
        k = em->scriptPlayer;
        if (k == -1 || em->noticeCount[k] < gEmNoticeThresholds[v]) {
            for (;;) {
                if (p[0] == 0x64 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x64 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x64, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x64 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x64);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op65(Em *em, u8 *p)
{
    em->noticeCount[0] = 0;
    em->noticeCount[1] = 0;
    em->noticeCount[2] = 0;
    em->noticeCount[3] = 0;
    return p;
}

u8 *EmScript_Op66(Em *em, u8 *p)
{
    u8 ok = 1;
    Player *pl;
    s8 i;

    switch (*p++) {
    case 0:
        pl = gPlayers;
        for (i = 0; i < gScene.playerCount; i++, pl++) {
            if (pl->rideState != 0) {
                ok = 0;
            }
        }
        if (ok) {
            while (ok) {
                if (p[0] == 0x66 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x66 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x66, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x66 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x66);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op67(Em *em, u8 *p)
{
    s32 ok;

    switch (*p++) {
    case 0:
        ok = em->remote != 0;
        if (ok) {
            while (ok) {
                if (p[0] == 0x67 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x67 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x67, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x67 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x67);
        break;
    case 2:
        break;
    }
    return p;
}

s32 EmScript_Op68(Em *em, s32 p)
{
    Em_RestartScript(em);
    return p;
}

u8 *EmScript_Op80(Em *em, u8 *p)
{
    u16 i;
    u8 sum;
    u16 r;
    u8 n;
    u8 w;

    switch (*p) {
    case 0:
        n = p[1];
        r = em->stateRand & 0x1F;
        sum = 0;
        i = 0;
        p += 2;
        for (; i < n; i++) {
            p = EmScript_SkipBlock(em, p, 0x80, 0xFF);
            w = p[2];
            p += 3;
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
        p += 2;
        for (;;) {
            p = EmScript_SkipBlock(em, p, 0x80, 0xFF);
            if (p[1] == 0xFF) {
                p += 2;
                break;
            }
            p += 3;
        }
        break;
    case 0xFF:
        p++;
        break;
    }
    return p;
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

u8 *EmScript_Op06(Em *em, u8 *p)
{
    s32 i;
    Em *m;

    em->goalKind = p[0];
    switch (em->goalKind) {
    case 0:
        em->goalKind = 0;
        em->goalArg = p[1];
        em->goalNo = *(p += 2);
        p++;
        break;
    case 1:
        em->goalKind = 1;
        em->goalArg = p[1];
        em->goalNo = *(p += 2);
        p++;
        break;
    case 2:
    case 4:
    case 5:
    case 6:
        em->goalArg = p[1];
        em->goalNo = *(p += 2);
        p++;
        break;
    case 3:
        em->goalArg = p[1];
        em->goalNo = *(p += 2);
        em->goalNo = *(u16 *) &em->goalNo << 8;
        *(u16 *) &em->goalNo |= p[1];
        p += 2;
        em->goalNo = Area_IdForCurrentTime(*(u16 *) &em->goalNo);
        em->goalArg = *(u16 *) &em->goalNo;
        em->nextArea = *(u16 *) &em->goalNo;
        *(u16 *) &em->destArea = 0xFFFF;
        break;
    case 0xA:
        em->goalKind = 3;
        em->goalArg = p[1];
        em->goalNo = *(p += 2);
        p++;
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
        em->goalArg = p[1];
        em->goalNo = *(p += 2);
        p++;
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
        em->goalArg = p[1];
        em->goalNo = *(p += 2);
        p++;
        break;
    }
    return p;
}

/* Command 0x83 (a multi-way branch on distance, 0..5 then 0xFF): picks the first limit (+0x764) the
 * distance to the target reaches and runs that branch; -1: no target, the last one. */
u8 *EmScript_Op83(Em *em, u8 *p)
{
    s8 k;
    f32 d;
    u8 n;
    s32 i;
    s32 j;
    s32 off;

    switch (*p++) {
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
        n = *p++;
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
            p = EmScript_NextCmd(em, p);
            p = EmScript_SkipBlock(em, p, 0x83, 0xFF);
        }
        p = EmScript_NextCmd(em, p);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        for (;;) {
            p = EmScript_SkipBlock(em, p, 0x83, 0xFF);
            if (p[1] == 0xFF) {
                p += 2;
                break;
            }
            p += 2;
        }
        break;
    case 0xFF:
        break;
    }
    return p;
}

u8 *EmScript_OpFF(Em *em, u8 *p)
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

    switch (*p) {
    case 0:
        em->scriptNo = 0;
        p = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case 1:
        em->scriptCallDepth = 0;
        p = em->scriptCallReturn;
        break;
    case 2:
        em->scriptCallDepth = 1;
        p = em->scriptJumpReturn;
        break;
    case 3:
        p = em->subScriptReturn;
        break;
    case 4:
    case 5:
        set = (EmRouteSet *) Em_FindAreaEntry(em, em->tables[6], em->area);
        routes = set->routes;
        if (routes == NULL) {
            p = em->routeResume;
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
                p = EmScript_GetTable30(em, em->routePointScript);
            } else {
                em->scriptNo = 0;
                p = (u8 *) Em_StartScript(em);
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
            p = EmScript_GetTable30(em, em->routePointScript);
            em->scriptPos = p;
        } else if (em->routePoint == 0) {
        last:
            if (*p == 4 && (s16) routes[em->curGoalArg].area != -1 && em->large != 0) {
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
            p = em->routeResume;
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
            p = EmScript_GetTable30(em, em->routePointScript);
        }
        break;
    case 0xF5:
        em->scriptNo = 0;
        p = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case 0xF6:
        em->scriptNo = 0;
        p = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case 0xF7:
        em->scriptNo = 0;
        p = (u8 *) Em_StartScript(em);
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
            p = EmScript_GetTable08(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            p = (u8 *) Em_StartScript(em);
            Em_RequestSync(em, 2);
        }
        break;
    case 0xF9:
        em->scriptFlags &= 0xFB;
        if (em->scriptFlags & 1) {
            p = EmScript_GetTable08(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            p = (u8 *) Em_StartScript(em);
            Em_RequestSync(em, 2);
        }
        break;
    case 0xFA:
        Em_StartScript(em);
        em->scriptFlags &= 0xFD;
        Em_SetRow638(em, 0);
        if (em->scriptFlags & 1) {
            p = EmScript_GetTable08(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            p = (u8 *) Em_StartScript(em);
            Em_RequestSync(em, 2);
        }
        break;
    case 0xFB:
        em->areaSeqActive = 0;
        p = em->areaSeqNext;
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
        p = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case 0xFD:
        em->scriptFlags &= 0xEF;
        if (em->scriptFlags & 1) {
            p = EmScript_GetTable08(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            p = (u8 *) Em_StartScript(em);
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
                em->loopPoint = ((u8 *)EmScript_GetTable08b(em, em->loopPointTable))[(s8) em->loopIndex];
                break;
            case 2:
                em->loopPoint++;
                break;
            }
        }
        p = EmScript_GetTable08(em, em->loopEndScript);
        break;
    case 0xFF:
        if ((s8) em->loopIndex < em->loopCount) {
            p = EmScript_GetTable08(em, em->loopScript);
        } else {
            em->scriptFlags &= 0xFE;
            p = EmScript_GetPos(em);
        }
        break;
    }
    em->scriptPos = p;
    return p;
}

u8 *EmScript_Op90(Em *em, u8 *p)
{
    f32 *v = (f32 *) (gEmGoalPoints[em->type] + *p * sizeof(f32[3]));

    em->posX = v[0];
    em->posY = v[1];
    em->posZ = v[2];
    em->prevPosX = em->posX;
    em->prevPosY = em->posY;
    em->prevPosZ = em->posZ;
    return p + 1;
}

void * EmScript_Op91(Em *em, u8 *p)
{
    em->rotY = Math_AngleXZ((f32 *) &em->posX, (f32 *) (gEmGoalPoints[em->type] + (*p * 0xC))) & 0xFFFF;
    return p + 1;
}

void * EmScript_Op99(Em *em, u8 *p)
{
    em->rotY = *p << 8;
    return p + 1;
}

s32 EmScript_Op92(s32 unused, s32 p)
{
    return p;
}

s32 EmScript_Op93(s32 unused, s32 p)
{
    return p;
}

u8 *EmScript_Op94(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = *p++;
                if (D_008F5AB8 == v) {
                    break;
                }
                if (v < D_008F5AB8) {
                    p = EmScript_SkipBlock(em, p, 0x94, 3);
                    if (p[1] == 2 || p[1] == 3) {
                        p += 2;
                        break;
                    }
                    p += 2;
                } else {
                    ok = 1;
                    do {
                        p = EmScript_SkipBlock(em, p, 0x94, 3);
                        if ((p[0] == 0x94 && p[1] == 2) || (p[0] == 0x94 && p[1] == 3)) {
                            ok = 0;
                        }
                        p = EmScript_NextCmd(em, p);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x94, 3);
            if (p[0] == 0x94 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op71(Em *em, u8 *p)
{
    u8 v;

    switch (*p++) {
    case 0:
        v = *p++;
        if (em->level < v) {
            for (;;) {
                if (p[0] == 0x71 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x71 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x71, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x71 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x71);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op72(Em *em, u8 *p)
{
    switch (*p++) {
    case 0:
        if (Quest_IsVersus() != 1) {
            for (;;) {
                if (p[0] == 0x72 && p[1] == 1) {
                    break;
                }
                if (p[0] == 0x72 && p[1] == 2) {
                    break;
                }
                p = EmScript_SkipBlock(em, p, 0x72, 2);
            }
            p = EmScript_NextCmd(em, p);
            if (p[0] == 0x72 && p[1] == 2) {
                p = EmScript_NextCmd(em, p);
            }
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x72);
        break;
    case 2:
        break;
    }
    return p;
}

u8 *EmScript_Op73(Em *em, u8 *p)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;
    u8 x;

    switch (*p++) {
    case 0:
        n = p[0];
        i = 0;
        p += 3;
        if (i < n) {
            do {
                v = *p++;
                x = em->itemUseKind & ~0x80;
                if (x == v) {
                    em->itemUseKind &= 0x80;
                    break;
                }
                if (v < x) {
                    p = EmScript_SkipBlock(em, p, 0x73, 3);
                    if (p[1] == 2 || p[1] == 3) {
                        p += 2;
                        break;
                    }
                    p += 2;
                } else {
                    ok = 1;
                    do {
                        p = EmScript_SkipBlock(em, p, 0x73, 3);
                        if ((p[0] == 0x73 && p[1] == 2) || (p[0] == 0x73 && p[1] == 3)) {
                            ok = 0;
                        }
                        p = EmScript_NextCmd(em, p);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case 1:
        p++;
    case 2:
        ok = 1;
        do {
            p = EmScript_SkipBlock(em, p, 0x73, 3);
            if (p[0] == 0x73 && p[1] == 3) {
                ok = 0;
            }
            p = EmScript_NextCmd(em, p);
        } while (ok);
        break;
    case 3:
        break;
    }
    return p;
}

u8 *EmScript_Op74(Em *em, u8 *p)
{
    u8 v;
    u8 n;

    switch (*p++) {
    case 0:
        v = *p++;
        if (Quest_IsVersus() == 1 && (n = em->itemUseKind & ~0x80) > v) {
            if (Versus_AddGauge(em, n) == 1) {
                break;
            }
            em->itemUseKind &= 0x80;
        }
        for (;;) {
            if (p[0] == 0x74 && p[1] == 1) {
                break;
            }
            if (p[0] == 0x74 && p[1] == 2) {
                break;
            }
            p = EmScript_SkipBlock(em, p, 0x74, 2);
        }
        p = EmScript_NextCmd(em, p);
        if (p[0] == 0x74 && p[1] == 2) {
            p = EmScript_NextCmd(em, p);
        }
        break;
    case 1:
        p = EmScript_SkipToEnd(em, p, 0x74);
        break;
    case 2:
        break;
    }
    return p;
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

u8 *EmScript_SkipBlock(Em *em, u8 *p, u8 op, u8 end)
{
    for (;;) {
        while (p[0] != op) {
            p = EmScript_NextCmd(em, p);
        }
        if (p[0] != op || p[1] != 0) {
            break;
        }
        p = EmScript_NextCmd(em, p);
        for (;;) {
            if (p[0] == op && p[1] == end) {
                p = EmScript_NextCmd(em, p);
                break;
            }
            if (p[0] == op) {
                p = EmScript_NextCmd(em, p);
            }
            p = EmScript_SkipBlock(em, p, op, end);
        }
    }
    return p;
}

/* Skips the command at p (its arguments, and a sub-command where it has one); an unknown command
 * stops the script (EmScript_SetVar95E with 4). The game keeps p in a1 across that call: EmScript_SetVar95E
 * is static and defined earlier in this file (catalogue "Arguments kept or reloaded across calls"). */
u8 *EmScript_NextCmd(Em *em, u8 *p)
{
    switch (*p) {
    case 1:
        p += 2;
        break;
    case 2:
        p += 2;
        break;
    case 3:
        p += 2;
        break;
    case 4:
        p += 1;
        break;
    case 5:
        p += 4;
        break;
    case 6:
        if (p[1] == 3) {
            p += 5;
        } else {
            p += 4;
        }
        break;
    case 7:
        p += 2;
        break;
    case 8:
        p += 2;
        break;
    case 0x1F:
        p += 2;
        break;
    case 9:
        p += 2;
        break;
    case 0x0A:
        p += 2;
        break;
    case 0x0B:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x0C:
        p += 3;
        break;
    case 0x0D:
        p += 2;
        break;
    case 0x0E:
        p += 1;
        switch (*p) {
        case 0:
            p += 3;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x0F:
        p += 6;
        break;
    case 0x10:
        p += 1;
        break;
    case 0x11:
        p += 1;
        break;
    case 0x12:
        p += 1;
        break;
    case 0x13:
        p += 1;
        break;
    case 0x14:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x15:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 2;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x16:
        p += 2;
        break;
    case 0x17:
        p += 5;
        break;
    case 0x18:
        p += 1;
        break;
    case 0x19:
        p += 1;
        break;
    case 0x1A:
        p += 3;
        break;
    case 0x1B:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x1C:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x1D:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x1E:
        p += 1;
        break;
    case 0x20:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x21:
        p += 1;
        switch (*p) {
        case 0:
            p += 1;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x22:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x36:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x23:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x24:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
            p += 1;
            break;
        }
        break;
    case 0x25:
        p += 1;
        break;
    case 0x26:
        p += 2;
        break;
    case 0x27:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x28:
        p += 2;
        break;
    case 0x29:
        p += 2;
        break;
    case 0x2A:
        p += 2;
        break;
    case 0x2B:
        p += 1;
        switch (*p) {
        case 0:
            p += 3;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x2C:
    case 0x70:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x2D:
        p += 1;
        break;
    case 0x2E:
        p += 2;
        break;
    case 0x2F:
        p += 2;
        break;
    case 0x30:
        p += 1;
        break;
    case 0x31:
        p += 1;
        break;
    case 0x32:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x33:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x34:
        p += 1;
        switch (*p) {
        case 0:
            p += 3;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x35:
        p += 2;
        break;
    case 0x37:
        p += 2;
        break;
    case 0x38:
        p += 2;
        break;
    case 0x39:
        p += 2;
        break;
    case 0x3A:
        p += 2;
        break;
    case 0x3B:
        p += 2;
        break;
    case 0x3C:
        p += 2;
        break;
    case 0x3D:
        p += 1;
        switch (*p) {
        case 0:
            p += 3;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x3E:
        p += 1;
        switch (*p) {
        case 0:
            p += 3;
        case 1:
            p += 3;
            break;
        case 2:
        case 3:
            p += 1;
            break;
        }
        break;
    case 0x3F:
        p += 4;
        break;
    case 0x40:
        p += 2;
        break;
    case 0x41:
        p += 2;
        break;
    case 0x42:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x69:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x44:
        p += 2;
        break;
    case 0x45:
        p += 2;
        break;
    case 0x46:
        p += 1;
        switch (*p) {
        case 0:
            p += 3;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x47:
        p += 2;
        break;
    case 0x48:
        p += 2;
        break;
    case 0x49:
        p += 2;
        break;
    case 0x4A:
        p += 2;
        break;
    case 0x4B:
        p += 1;
        break;
    case 0x4C:
        p += 1;
        break;
    case 0x4D:
        p += 1;
        break;
    case 0x4E:
    case 0x4F:
    case 0x50:
        p += 2;
        break;
    case 0x51:
        p += 2;
        break;
    case 0x52:
        p += 1;
        break;
    case 0x53:
        p += 1;
        break;
    case 0x54:
        p += 2;
        break;
    case 0x55:
        p += 2;
        break;
    case 0x56:
        p += 1;
        switch (*p) {
        case 0:
            p += 3;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x57:
        p += 1;
        switch (*p) {
        case 0:
            p += 3;
        case 1:
            p += 3;
            break;
        case 2:
        case 3:
            p += 1;
            break;
        }
        break;
    case 0x58:
        p += 1;
        break;
    case 0x59:
        p += 2;
        break;
    case 0x5A:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x5B:
        p += 2;
        break;
    case 0x5C:
        p += 2;
        break;
    case 0x5D:
        p += 2;
        break;
    case 0x5E:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x5F:
        p += 1;
        break;
    case 0x60:
        p += 2;
        break;
    case 0x61:
        p += 1;
        break;
    case 0x62:
        p += 2;
        break;
    case 0x63:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x64:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x65:
        p += 1;
        break;
    case 0x66:
        p += 2;
        break;
    case 0x67:
        p += 2;
        break;
    case 0x68:
        p += 1;
        break;
    case 0x80:
        p += 1;
        switch (*p) {
        case 0:
            p += 3;
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
            p += 2;
            break;
        case 0xFF:
            p += 1;
            break;
        }
        break;
    case 0x81:
        p += 2;
        break;
    case 0x82:
        p += 3;
        break;
    case 0x83:
        p += 1;
        switch (*p) {
        case 0:
            p += 4;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 0xFF:
            p += 1;
            break;
        }
        break;
    case 0x84:
    case 0x7B:
        p += 1;
        break;
    case 0xFF:
        p += 2;
        break;
    case 0x90:
        p += 2;
        break;
    case 0x91:
    case 0x99:
        p += 2;
        break;
    case 0x92:
    case 0x93:
        p += 1;
        break;
    case 0x94:
        p += 1;
        switch (*p++) {
        case 0:
            p += 2;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x71:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x72:
        p += 2;
        break;
    case 0x73:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x74:
        p += 1;
        switch (*p) {
        case 0:
            p += 2;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x75:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x76:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x77:
        p += 2;
        break;
    case 0x78:
        p += 1;
        switch (*p) {
        case 0:
            p += 3;
            break;
        case 1:
        case 2:
            p += 1;
            break;
        }
        break;
    case 0x79:
        p += 1;
        switch (*p++) {
        case 0:
            p += 4;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x7A:
        p += 1;
        switch (*p++) {
        case 0:
            p += 3;
        case 1:
            p += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case 0x7C:
        p += 2;
        break;
    default:
        EmScript_SetVar95E(em, p, 4);
        break;
    }
    return p;
}

u8 *EmScript_SkipToEnd(Em *em, u8 *p, u8 op)
{
    Em *e;

    if (p[0] == op && p[1] == 2) {
        p = EmScript_NextCmd(em, p);
    } else {
        e = em; /* a copy for the loop (found by the permuter): with em itself MWCC lays the loop out differently */
        for (;;) {
            if (p[0] == op && p[1] == 2) {
                p = EmScript_NextCmd(e, p);
                break;
            }
            p = EmScript_SkipBlock(e, p, op, 2);
        }
    }
    return p;
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

u8 *EmScript_PickWeighted(Em *em, u8 *p)
{
    u16 n;
    u16 i;
    u8 sum;
    u16 r;
    u8 w;

    n = *(u16 *) (p + 4);
    sum = 0;
    r = em->stateRand & 0x1F;
    p += 6;
    for (i = 0; i < n; i++) {
        w = p[4];
        p += 6;
        if (!w) {
            p += 2;
            continue;
        }
        if (w == 0xFF) {
            break;
        }
        sum += w;
        if (r < sum) {
            break;
        }
        p += 2;
    }
    return p;
}

/* The 0x722400 part's .data (0x8C3DE0-0x8C3E50) */
ArmorCombo gArmorCombos[14] = {
#include "game/008C3DE0.inc"
};
s32 gEmNoticeThresholds[3] = {
#include "game/008C3E38.inc"
};
