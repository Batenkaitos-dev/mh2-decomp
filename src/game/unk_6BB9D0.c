/*
 * Game overlay code at 0x6BB9D0-0x6C7AE0, not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "em.h"
#include "objhead.h"
#include "objattach.h"
#include "player.h"
#include "stageobj.h"
#include "sprite.h"
#include "capsule.h"

/* prototypes */
s32 Player_HasMealEffect(Player *pl, s16 kind);
void Player_AddHp(Player *p, s16 n);
void Player_AddMaxHp(Player *p, s16 n, u8 temporary);
void QSync_SendPlayer(Player *p, s32 kind, u16 mode);
extern char *gHornSongTexts[];
s32 sprintf(char *, const char *, ...);
u8 Notice_PushText(char *s);
extern ArrowParam gArrowParams[];
extern s32 Eff06_PlayerFlags[];
extern void *Eff06_MotionRef;
extern HitShape *Eff06_Vals088[];
void Eff06_InitWeapon(Eff06Buf *b, Player *pl);
void Eff06_InitSkills(Eff06Buf *b, Player *pl);
void Eff06_CalcPower(ObjAttach *o);
void Eff06_ApplyKind(ObjAttach *o);
f32 Eff06_BitsToFrac(u16 arg0, s32 arg1);
f32 Math_Atan2(f32 y, f32 x);
f32 Math_Sqrtf(f32 x);
void Eff06_PlaySe(ObjAttach *o, s16 kind);
extern void *Eff00_MotionRef;
extern HitShape *Eff00_Vals088[];
s16 Player_GetWeaponStat(Player *p, u8 kind);
void BowgunShot_SetKind(ObjAttach *o, u8 a, u8 b);
s32 CamCtl_IsAiming(Player *pl);
void StageObj_K12_CreateOwned(s32 owner, s16 v, s32 k);
void StageObj_K12_CreateOnEm(Player *em, s16 v, s32 k, u8 c);
void Eff06_BoneLaunchPos(f32 *out, Player *p, s32 *ang, s16 bone);
void StageObj_K0F_CreateNoOwner(f32 *pos, u8 k, u8 c, u16 v, f32 scale);
void StageObj_K29_CreateAt(f32 *src, u8 k, u8 c, f32 scale);
void Eff06_Create(Player *em, u8 kind, s16 arg2, f32 scale);
extern s16 Gunlance_State01Motions[][2];
extern s16 D_00897022[][2];
void StageObj_K29_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);
s32 func_364AE0(ObjAttach *o, u16 x);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
u16 Rand_Next(s32 stream);
void Vec_RotateX(f32 *v, f32 angle);
extern s16 Eff03_Models[];
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Draw_ApplyStates(u32);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
extern s8 Eff03_NoDefault[];
extern f32 Eff03_DefaultTblA[][2];
extern f32 Eff03_DefaultTblB[][2];
extern f32 Eff03_DefaultTblC[][2];
void Eff09_Create(Player *owner, f32 *pos, s32 kind);
void StageObj_K0E_CreateOwned(f32 *pos, s16 k, f32 scale, void *owner);
StageObj *StageObj_K0F_CreateAt(f32 *pos, s32 k, s32 c, f32 scale);
void StageObj_K0C_CreateFromAttach(ObjAttach *oa, s32 k, u8 c);
void StageObj_K0C_Create(f32 *pos, s16 v, s32 k, u8 c);
void StageObj_K29_CreateAtDefault(f32 *src, u8 k, f32 scale);
void Effect0D_SpawnAt(f32 *pos, s32 a, f32 scale);
void Effect02_SpawnRandomAngle(u16 a, u16 b, u8 type, f32 *pos);
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
void Vec_RotateY(f32 *v, f32 angle);
void CamShake_ReqB(void *obj, s32 i);
void Bowgun_FireLoaded(Player *em, s32 arg1);
u8 Pouch_Add(Player *p, u16 item, s16 n);
u16 Bowgun_NextAmmoSlot(Player *p, u16 slot, u8 dir);
void Bowgun_LoadAmmo(Player *p, s32 a);
void StageObj_K12_CreateGunlanceShell(Player *em, s16 i);
void StageObj_Gun_CreateShell(void *owner, u8 lvl);
void StageObj_Gun_CreateType4(void *owner);
void StageObj_Gun_CreateType6(void *owner);
void Player_ClearPartOverrides(Player *em);
extern f32 Eff03_ScaleKeysA[];
extern f32 Eff03_ScaleKeysB[];
extern u8 Eff03_ColorKeysA[];
extern u8 Eff03_ColorKeysB[];
extern s16 Eff03_PartModels[];
extern f32 gCamMatrix[];
extern f32 gCamYawMtx[];
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Gs_SetState(u8 reg, u32 v);
void Vec_RotateSelf(f32 *v, f32 *m);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_Identity(f32 *m);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
EmBone *Em_GetBone(void *obj, s16 bone);
static void Bow_SetAimLayers(Player *em, u16 motA, u16 motB);
void Eff03_Update(ObjAttach *o);
extern s8 Eff03_KindOn[];
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Player_FaceStick(Player *em);
void Bow_ClearCoatingSlot(Player *p);
void Bow_SetCoatingSlot(Player *p);
s32 Gunlance_CheckInput(Player *p, s32 caller);
void Bow_UpdateAim(Player *em, s32 arg1);
u8 Player_StickMoveLevel(Player *, s32);
void Player_SetFlag(void *obj, u32 flag);
s32 Bow_CheckInput(Player *em, s32 arg1);
extern u8 HuntingHorn_SongNotes[];
void HuntingHorn_PlaySong(Player *em, u8 song, u8 quiet);
void Eff03_SetupType2(ObjAttach *o);
void Eff03_SetupType1(ObjAttach *o);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Eff03_SetupDefault(ObjAttach *o);
void Eff03_Draw(DrawSlot *d);
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s32 i);
void Eff03_DrawPart(DrawSlot *d);
typedef struct AttachPart { s16 slot; s16 idx; DrawSlot *node; s16 unk08; s16 unk0A; s16 unk0C; s16 unk0E; } AttachPart;
void Eff06_Update(ObjAttach *o);
void Eff00_Update(ObjAttach *o);
void Eff02_Update(ObjAttach *o);
void Eff05_Update(ObjAttach *o);
u8 Player_InCurrentArea(void *obj);
ObjAttach *ObjAttach_Alloc(s32 n);
void Eff01_Update(ObjAttach *o);
void Eff04_Update(ObjAttach *o);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Obj_InCurrentArea(void *obj);
s32 Side_Match(u8 a, u8 b, u8 none);
typedef struct AttachSprite { s16 unk0; s16 kind; void *draw; s16 frame; s16 time; u8 pad[4]; } AttachSprite;
void Eff03_Fall(ObjAttach *o);
s32 Eff03_BounceStep(ObjAttach *o);
extern s8 Eff03_KindParams[];
void Eff03_RunDefault(ObjAttach *o);
void Eff03_RunType1(ObjAttach *o);
void Eff03_RunType2(ObjAttach *o);
extern HitShape *Eff04_Vals088[];
extern void *Eff04_MotionRef;
void Eff02_DrawNop(ObjAttach *o);
void Eff05_DrawNop(ObjAttach *o);
extern HitShape *Eff02_Vals088[];
extern void *Eff02_MotionRef;
extern HitShape *Eff05_Vals088[];
extern void *Eff05_MotionRef;
f32 Math_Absf(f32 x);
extern HitShape *Eff01_Vals088[];
extern void *Eff01_MotionRef;
void Eff00_AttachTransform(ObjAttach *o, Player *p);
void HuntingHorn_CheckSongs(Player *em);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
void Eff09_CreateAt(f32 *pos, s32 kind, u16 area, u8 arg3);
void Eff03_InitParts(ObjAttach *o);
void StageObj_K10_CreateAt(f32 *pos, u8 k, s16 v, u8 c, f32 scale);
f32 AreaGrid2_GetFloorYFiltered(f32 *);
struct MoveObj;
void MoveObj_AddVelocity(struct MoveObj *);
s32 Motion_PassedFrame(void *, s32 layer, f32 frame);
void ObjAttach_Free(ObjAttach *p);
void Player_AddSharpness(Player *p, s16 n);
s32 Player_IsLocal(void *);
void Player_RequestState(Player *p, u16 state, u8 sub, u16 flags);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_SetMoveMode(Player *p, u16 mode, s16 flag1, s16 flag2);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_UpdateMotionTime(Player *p, s16 step);
void Player_UseStamina(Player *p, s32 n);
void HuntingHorn_State00(Player *, s32, s32);
void HuntingHorn_State09(Player *, s32);
void HuntingHorn_State10(Player *, s32);
void HuntingHorn_State11(Player *, s32);
void HuntingHorn_State12(Player *, s32);
void HuntingHorn_State24(Player *, s32);
void HuntingHorn_State15(Player *, s32);
void HuntingHorn_State20(Player *, s32);
void HuntingHorn_CheckInput(Player *, u32);
u8 HuntingHorn_AttackNote(Player *);
void Gunlance_State00(Player *, s32);
void Gunlance_State01(Player *, s32);
void Gunlance_State02(Player *, s32);
void Gunlance_State03(Player *, s32);
void Gunlance_State04(Player *, s32);
void Gunlance_State06(Player *, s32);
void Gunlance_State07(Player *, s32);
void Bow_State00(Player *, s32);
void Bow_State01(Player *, u32);
void Bow_State03(Player *, s32);
void Bow_State05(Player *, s32);
void Bow_State06(Player *, s32);
void Bow_State08(Player *, s32);
void Bow_State17(Player *, s32);
void Eff00_Create(Player *, u8);
void Eff00_CreateArrow(Player *, s8);
void Eff00_Setup(ObjAttach *o);
void Eff00_Run(ObjAttach *o);
void Eff00_End(ObjAttach *o);
void Eff00_Free(ObjAttach *o);
void Eff01_Setup(ObjAttach *o);
void Eff01_Run(ObjAttach *o);
void Eff01_End(ObjAttach *o);
void Eff01_Free(ObjAttach *o);
void Eff02_Setup(ObjAttach *o);
void Eff02_Run(ObjAttach *o);
void Eff02_End(ObjAttach *o);
void Eff02_Free(ObjAttach *o);
void Eff03_Setup(ObjAttach *o);
void Eff03_Run(ObjAttach *o);
void Eff03_EndImpact(ObjAttach *o);
void Eff03_Hit(ObjAttach *o);
void Eff03_FreeDrawSlots(ObjAttach *o);
void Eff03_End(ObjAttach *o);
void Eff03_Free(ObjAttach *o);
void Eff04_Setup(ObjAttach *o);
void Eff04_Run(ObjAttach *o);
void Eff04_End(ObjAttach *o);
void Eff04_Free(ObjAttach *o);
void Eff05_Setup(ObjAttach *o);
void Eff05_Run(ObjAttach *o);
void Eff05_End(ObjAttach *o);
void Eff05_Free(ObjAttach *o);
void EffGunlance38_Create(Player *, s8);
void StageObj_Gun_CreateReload(void *owner, u8 k);
s32 Motion_ReachedFrame(void *, s32, f32);
void CamShake_ReqA(void *obj, s32 i);
s32 Game_IsOnline(void);
s32 Quest_PhaseIsOver(s32);
void DrawSlot_Free(s32);
void DrawSlotB_Free(s32);
void Player_BeginState(Player *p, s32 action, s32 a2, s32 a3);
void MoveObj_Integrate(struct MoveObj *);
void Player_TickWeaponTimer(Player *p);
extern u8 gBladeSpecialMarks;
s32 Player_HasSkill(Player *p, u8 skill);
s32 Player_TestSetFlags(void *, s32 mask);
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
extern HitShape *Eff03_Vals088[];
extern void *Eff03_MotionRef;
extern s8 Eff03_Motions[];
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
s32 Motion_InFrameRange(void *, s32, f32, f32);
void Vib_StartLocal(void *port, s32 i);
s32 HuntingHorn_CheckHit(Player *);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Vec_Copy(f32 *out, f32 *in);
void Player_SetEvadeFrames(Player *);
void Player_ResetAim(Player *p);
void Player_AddStamina(Player *p, s16 n);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
/* The hunting horn's attacks (HuntingHorn_State15's variants): the note, the motions and the attack id. */
typedef struct HornAttack {
    s16 note;       /* 0x0: the note played (0-3) */
    s16 unk2;       /* 0x2: 0: no wind-up motion */
    s16 motion;     /* 0x4 */
    s16 unk6;       /* 0x6 */
    s16 motion2;    /* 0x8: after the wind-up */
    s16 unkA;       /* 0xA */
    s16 unkC;       /* 0xC */
    u8 attack;      /* 0xE: Eff00_Create */
    u8 unkF;
} HornAttack;
extern HornAttack HuntingHorn_Attacks[];
extern s16 HuntingHorn_NoteFrames[];     /* per note: the frame of its sound effect, 0: none */
/* end of prototypes */

void HuntingHorn_State09(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->weaponOut = 1;
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 50;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        switch (variant) {
        case 0:
        case 1:
        case 2:
            p->step = 2;
            if (variant == 1) {
                Player_SetMotionPair(p, 0x579, 0xA, 0x28);
            } else {
                Player_SetMotionPair(p, 0x579, 4, 0);
            }
            if (variant == 2) {
                p->motionSpeed = 0.77f;
            }
            Eff00_Create(p, 0x51);
            break;
        case 3:
            p->step++;
            Player_SetMotionPair(p, 0x3EE, 2, 0);
            break;
        case 4:
            p->step++;
            Player_SetMotionPair(p, 0x596, 4, 0);
            break;
        }
        Player_UpdateMotionTime(p, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            if (variant == 3) {
                Player_SetMotionPair(p, 0x579, 0, 0x1A);
                Player_UpdateMotionTime(p, 0);
                Eff00_Create(p, 0x63);
                break;
            }
            Player_SetMotionPair(p, 0x579, 0, 0xA);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x51);
        }
        break;
    case 2:
        if (HuntingHorn_CheckHit(p) != 1) {
            if (p->motion[0].playing == 0) {
                Player_BeginState(p, 0, 6, 0);
                break;
            }
            if (Player_IsLocal(p) == 1) {
                if (Motion_InFrameRange(p, 0, 40.0f, 60.0f) != 0 && p->stateVar0 == 0 && (p->input.pressed & 0x80)) {
                    p->stateVar0 = 1;
                }
                if (Motion_ReachedFrame(p, 0, 60.0f) != 0 && p->stateVar0 != 0) {
                    Player_RequestState(p, 1, 0x16, 4);
                    break;
                }
                if (Motion_InFrameRange(p, 0, 80.0f, 104.0f) != 0) {
                    HuntingHorn_CheckInput(p, 0);
                }
            }
        }
        break;
    }
}

void HuntingHorn_State10(Player *p, s32 unused)
{
    u8 step;

    step = p->step;
    switch (step) {
    case 0:
        p->step = (u8) (step + 1);
        p->stateVar0 = 0U;
        p->stateVar1 = 0;
        Player_SetMotionPair(p, 0x57A, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Eff00_Create(p, 0x52U);
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0x32;
        break;
    case 1:
        if (HuntingHorn_CheckHit(p) != 1) {
            if (p->motion[0].playing == 0) {
                Player_BeginState(p, 0, 6, 0);
                break;
            }
            if (Player_IsLocal(p) == 1) {
                if ((Motion_InFrameRange(p, 0, 42.0f, 62.0f) != 0) && (p->stateVar0 == 0) && (p->input.pressed & 0x80)) {
                    p->stateVar0 = 1U;
                }
                if ((Motion_ReachedFrame(p, 0, 62.0f) != 0) && (p->stateVar0 != 0)) {
                    Player_RequestState(p, 1U, 0x24U, 4);
                    break;
                }
                if (Motion_InFrameRange(p, 0, 80.0f, 104.0f) != 0) {
                    HuntingHorn_CheckInput(p, 1U);
                }
            }
        } else {
        }
        break;
    }
}

void HuntingHorn_State11(Player *p, s32 unused)
{
    u8 step;

    step = p->step;
    switch (step) {
    case 0:
        p->step = (u8) (step + 1);
        p->stateVar0 = 0U;
        p->stateVar1 = 0;
        Player_SetMotionPair(p, 0x57C, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Eff00_Create(p, 0x53U);
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0x1E;
        break;
    case 1:
        if (HuntingHorn_CheckHit(p) != 1) {
            if (p->motion[0].playing == 0) {
                Player_BeginState(p, 0, 6, 0);
                break;
            }
            if (Player_IsLocal(p) == 1) {
                if ((Motion_InFrameRange(p, 0, 32.0f, 52.0f) != 0) && (p->stateVar0 == 0) && (p->input.pressed & 0x80)) {
                    p->stateVar0 = 1U;
                }
                if (Motion_InFrameRange(p, 0, 32.0f, 52.0f) != 0) {
                    if (p->stateVar0 != 0) {
                        Player_RequestState(p, 1U, 0x27U, 4);
                        break;
                    }
                    HuntingHorn_CheckInput(p, 2U);
                }
            }
        } else {
        }
        break;
    }
}

void HuntingHorn_State12(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        Player_SetMotionPair(p, 0x588, 4, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 30;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x57C, 0, 0xC);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x54);
        }
        break;
    case 2:
        if (HuntingHorn_CheckHit(p) != 1) {
            if (p->motion[0].playing == 0) {
                Player_BeginState(p, 0, 6, 0);
                break;
            }
            if (Player_IsLocal(p) == 1) {
                if (variant == 0) {
                    if (Motion_InFrameRange(p, 0, 32.0f, 52.0f) != 0 && p->stateVar0 == 0 && (p->input.pressed & 0x80)) {
                        p->stateVar0 = 1;
                    }
                    if (Motion_InFrameRange(p, 0, 32.0f, 52.0f) != 0) {
                        if (p->stateVar0 != 0) {
                            Player_RequestState(p, 1, 0x27, 4);
                            break;
                        }
                        HuntingHorn_CheckInput(p, 3);
                        break;
                    }
                } else if (Motion_InFrameRange(p, 0, 32.0f, 52.0f) != 0) {
                    HuntingHorn_CheckInput(p, 6);
                }
            }
        }
        break;
    }
}

void HuntingHorn_State24(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x595, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x579, 0, 0x44);
            break;
        }
        if (p->stateVar0 != 0) {
            if (Motion_ReachedFrame(p, 0, 32.0f) != 0) {
                Player_SetState(p, 1, 0x25, 0xC);
                break;
            }
        } else {
            if (Motion_InFrameRange(p, 0, 2.0f, 30.0f) != 0 && p->stateVar0 == 0 && p->itemSelect == 0 && (p->input.stickDirPressed & 0x3C)) {
                p->stateVar0 = 1;
            }
            break;
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void HuntingHorn_State14(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->weaponOut = 1;
        Player_SetMoveMode(p, 0, 1, 0);
        if (variant == 0) {
            p->step = 2;
            Player_SetMotionPair(p, 0x584, 2, 0);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x5EU);
            break;
        }
        Player_SetMotionPair(p, 0x597, 4, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x584, 0, 0x26);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x5EU);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 1U, 5U, 4);
        }
        break;
    }
}

void HuntingHorn_State15(Player *p, s32 variant)
{
    HornAttack *a = &HuntingHorn_Attacks[variant];
    s16 frame;

    switch (p->step) {
    case 0:
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        Player_PlaySe(p, 0x1E, 0, &p->pos[0], 1, 0);
        Player_SetMotionPair(p, a->motion, a->unk6, 0);
        if (a->unk2 == 0) {
            p->step = 2;
            Eff00_Create(p, a->attack);
        } else {
            p->step++;
        }
        p->noFlinchTimer = 125;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, a->motion2, a->unkC, a->unkA);
            Eff00_Create(p, a->attack);
        }
        break;
    case 2:
        if (HuntingHorn_CheckHit(p) != 1) {
            if (p->motion[0].playing == 0) {
                switch (a->note) {
                case 0:
                    Player_RequestState(p, 1, 4, 6);
                    break;
                case 1:
                    Player_RequestState(p, 1, 5, 6);
                    break;
                case 2:
                    Player_RequestState(p, 1, 6, 6);
                    break;
                case 3:
                    Player_RequestState(p, 1, 7, 6);
                    break;
                }
            } else {
                frame = HuntingHorn_NoteFrames[a->note];
                if (frame != 0 && Motion_PassedFrame(p, 0, frame) != 0) {
                    if (p->stateVar0 == 0) {
                        p->stateVar0++;
                        Vib_StartLocal(p, 2);
                        CamShake_ReqA(p, 0);
                    }
                }
                if (Player_IsLocal(p) == 1) {
                    switch (p->motionId[0]) {
                    case 0x580:
                        if (Motion_InFrameRange(p, 0, 112.0f, 132.0f) != 0) {
                            HuntingHorn_CheckInput(p, 4);
                        }
                        break;
                    case 0x581:
                        if (Motion_InFrameRange(p, 0, 128.0f, 148.0f) != 0) {
                            HuntingHorn_CheckInput(p, 4);
                        }
                        break;
                    case 0x582:
                        if (Motion_InFrameRange(p, 0, 108.0f, 128.0f) != 0) {
                            HuntingHorn_CheckInput(p, 4);
                        }
                        break;
                    case 0x583:
                        if (Motion_InFrameRange(p, 0, 92.0f, 112.0f) != 0) {
                            HuntingHorn_CheckInput(p, 4);
                        }
                        break;
                    }
                }
            }
        }
        break;
    }
}

void HuntingHorn_State19(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->stateVar0 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        switch (variant) {
        case 0:
            p->step = 2;
            Player_SetMotionPair(p, 0x585, 2, 0);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x5FU);
            break;
        case 1:
            p->step++;
            Player_SetMotionPair(p, 0x586, 2, 0);
            break;
        case 2:
            p->step++;
            Player_SetMotionPair(p, 0x587, 2, 0);
            break;
        }
        p->weaponOut = 1;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x585, 0, 0x1A);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x5FU);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if ((Player_IsLocal(p) == 1) && (Motion_ReachedFrame(p, 0, 66.0f) != 0)) {
            HuntingHorn_CheckInput(p, 5U);
        }
        break;
    }
}

void HuntingHorn_State20(Player *p, s32 unused)
{
    u8 step;

    step = p->step;
    switch (step) {
    case 0:
        p->step = (u8) (step + 1);
        Player_SetMoveMode(p, 0, 1, 0);
        Player_SetMotionPair(p, 0x57B, 2, 0);
        Player_UpdateMotionTime(p, 0);
        Eff00_Create(p, 0x55U);
        p->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (Motion_PassedFrame(p, 0, 84.0f) != 0) {
            Eff00_Create(p, 0x56U);
        }
        if (Motion_PassedFrame(p, 0, 86.0f) != 0) {
            Vib_StartLocal(p, 2);
            CamShake_ReqA(p, 2);
        }
        if ((Player_IsLocal(p) == 1) && (Motion_InFrameRange(p, 0, 110.0f, 154.0f) != 0)) {
            HuntingHorn_CheckInput(p, 4U);
        }
        break;
    }
}

void HuntingHorn_RunState(Player *p)
{
    switch (p->state) {
    case 0:
        HuntingHorn_State00(p, 0, 0);
        break;
    case 1:
        HuntingHorn_State00(p, 1, 0);
        break;
    case 2:
        HuntingHorn_State00(p, 2, 0);
        break;
    case 3:
        HuntingHorn_State00(p, 3, 0);
        break;
    case 4:
        HuntingHorn_State00(p, 0, 1);
        break;
    case 5:
        HuntingHorn_State00(p, 1, 1);
        break;
    case 6:
        HuntingHorn_State00(p, 2, 1);
        break;
    case 7:
        HuntingHorn_State00(p, 3, 1);
        break;
    case 9:
        HuntingHorn_State09(p, 0);
        break;
    case 10:
        HuntingHorn_State10(p, 0);
        break;
    case 11:
        HuntingHorn_State11(p, 0);
        break;
    case 12:
        HuntingHorn_State12(p, 0);
        break;
    case 13:
        HuntingHorn_State09(p, 1);
        break;
    case 14:
        HuntingHorn_State14(p, 0);
        break;
    case 15:
        HuntingHorn_State15(p, 0);
        break;
    case 16:
        HuntingHorn_State15(p, 1);
        break;
    case 17:
        HuntingHorn_State15(p, 2);
        break;
    case 18:
        HuntingHorn_State15(p, 3);
        break;
    case 19:
        HuntingHorn_State19(p, 0);
        break;
    case 20:
        HuntingHorn_State20(p, 0);
        break;
    case 21:
        HuntingHorn_State09(p, 2);
        break;
    case 22:
        HuntingHorn_State00(p, 3, 2);
        break;
    case 23:
        HuntingHorn_State15(p, 4);
        break;
    case 24:
        HuntingHorn_State24(p, 0);
        break;
    case 25:
        HuntingHorn_State09(p, 3);
        break;
    case 26:
        HuntingHorn_State19(p, 1);
        break;
    case 27:
        HuntingHorn_State19(p, 2);
        break;
    case 28:
        HuntingHorn_State15(p, 5);
        break;
    case 29:
        HuntingHorn_State15(p, 6);
        break;
    case 30:
        HuntingHorn_State15(p, 7);
        break;
    case 31:
        HuntingHorn_State15(p, 8);
        break;
    case 32:
        HuntingHorn_State15(p, 9);
        break;
    case 33:
        HuntingHorn_State15(p, 0xA);
        break;
    case 34:
        HuntingHorn_State15(p, 0xB);
        break;
    case 35:
        HuntingHorn_State12(p, 1);
        break;
    case 36:
        HuntingHorn_State00(p, 3, 3);
        break;
    case 37:
        HuntingHorn_State09(p, 4);
        break;
    case 38:
        HuntingHorn_State14(p, 1);
        break;
    case 39:
        HuntingHorn_State00(p, 3, 4);
        
    default:
        break;
    }
}

s32 HuntingHorn_AttackInput(Player *p)
{
    s32 dir;

    if (p->input.stickDirPressed & 0x3C) {
        dir = (p->input.prevStickAngle + 0x1C72) & 0xFFFF;
        if (dir < 0x471D) {
            Player_RequestState(p, 1U, 0xAU, 4);
        } else if (dir < 0x8001) {
            Player_RequestState(p, 1U, 0x14U, 4);
        } else if (dir < 0xC71D) {
            Player_RequestState(p, 1U, 9U, 4);
        } else {
            Player_RequestState(p, 1U, 0xBU, 4);
        }
        return 1;
    }
    if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B)) {
        Player_RequestState(p, 0U, 0x1CU, 4);
        return 1;
    }
    return 0;
}

void HuntingHorn_CheckInput(Player *p, u32 variant)
{
    s32 dir;

    if ((Player_IsLocal(p) != 0 || Game_IsOnline() != 1) && Quest_PhaseIsOver(1) != 1 && p->itemSelect == 0) {
        if ((p->input.pressed & 0x40) && p->stamina >= 0x4B && variant != 5) {
            if (p->input.stickDir & 0x3C00) {
                dir = (u16) (p->input.stickAngle + 0x4000);
                if (dir < 0x5C72) {
                    Player_RequestState(p, 0, 0x31, 4);
                } else if (dir < 0xA38E) {
                    Player_RequestState(p, 0, 0x1C, 4);
                } else {
                    Player_RequestState(p, 0, 0x32, 4);
                }
            } else {
                Player_RequestState(p, 0, 0x1C, 4);
            }
            return;
        }
        if (variant == 4 || variant == 6) {
            return;
        }
        if (p->input.stickDirPressed & 0x3C) {
            dir = (u16) (p->input.prevStickAngle + 0x1C72);
            if (dir < 0x471D) {
                if (variant == 0 || variant - 2 <= 1 || variant == 5) {
                    Player_RequestState(p, 1, 0xA, 4);
                }
            } else if (dir < 0x8001) {
                if (variant <= 3 || variant == 5) {
                    Player_RequestState(p, 1, 0x14, 4);
                }
            } else if (dir < 0xC71D) {
                if (variant - 1 <= 2 || variant == 5) {
                    Player_RequestState(p, 1, 9, 4);
                }
            } else if (variant == 2 || variant == 5) {
                Player_RequestState(p, 1, 0xC, 4);
            } else if (variant == 3) {
                Player_RequestState(p, 1, 0x23, 4);
                return;
            }
        }
    }
}

void HuntingHorn_IdleInput(Player *p, s32 arg1)
{
    s32 dir;
    s32 dir2;

    if (((Player_IsLocal(p) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1) && (p->itemSelect == 0)) {
        if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B)) {
            if (p->input.stickDir & 0x3C00) {
                dir = (p->input.stickAngle + 0x4000) & 0xFFFF;
                if (dir < 0x5C72) {
                    Player_RequestState(p, 0U, 0x31U, 4);
                } else if (dir < 0xA38E) {
                    Player_RequestState(p, 0U, 0x1CU, 4);
                } else {
                    Player_RequestState(p, 0U, 0x32U, 4);
                }
            } else {
                Player_RequestState(p, 0U, 0x1CU, 4);
            }
        }
        if (p->input.stickDirPressed & 0x3C) {
            dir2 = (p->input.prevStickAngle + 0x2000) & 0xFFFF;
            if (dir2 < 0x4001) {
                switch (arg1) {
                case 0:
                    Player_RequestState(p, 1U, 0x1CU, 4);
                    break;
                case 3:
                    Player_RequestState(p, 1U, 0x20U, 4);
                    break;
                default:
                    Player_RequestState(p, 1U, 0x10U, 4);
                    break;
                }
            } else if (dir2 < 0x8001) {
                switch (arg1) {
                case 0:
                    Player_RequestState(p, 1U, 0x17U, 4);
                    break;
                case 3:
                    Player_RequestState(p, 1U, 0x1FU, 4);
                    break;
                default:
                    Player_RequestState(p, 1U, 0xFU, 4);
                    break;
                }
            } else if (dir2 < 0xC001) {
                switch (arg1) {
                case 0:
                    Player_RequestState(p, 1U, 0x1DU, 4);
                    break;
                case 3:
                    Player_RequestState(p, 1U, 0x21U, 4);
                    break;
                default:
                    Player_RequestState(p, 1U, 0x11U, 4);
                    break;
                }
            } else {
                switch (arg1) {
                case 0:
                    Player_RequestState(p, 1U, 0x1EU, 4);
                    break;
                case 3:
                    Player_RequestState(p, 1U, 0x22U, 4);
                    break;
                default:
                    Player_RequestState(p, 1U, 0x12U, 4);
                    break;
                }
            }
        }
    }
}

void HuntingHorn_DrawAttack(Player *p)
{
    Player_RequestState(p, 0x1, 0x19, 0x4);
}

void HuntingHorn_Draw(Player *p, s32 arg1)
{
    if (arg1 == 0) {
        Player_RequestState((void *) p, 1U, 0xEU, 4);
        return;
    }
    Player_RequestState((void *) p, 1U, 0x26U, 4);
}

u8 HuntingHorn_AttackNote(Player *p)
{
    u8 ret = 0;

    if (p->stateGroup != 1) {
        return 0;
    }
    switch (p->state) {
    case 0:
    case 4:
        ret = 2;
        break;
    case 1:
    case 2:
    case 5:
    case 6:
        ret = 1;
        break;
    case 3:
    case 7:
    case 22:
    case 36:
    case 39:
        ret = 3;
        break;
    }
    switch (p->motionId[0]) {
    case 0x584:
        if (Motion_ReachedFrame(p, 0, 72.0f) == 0 || p->motion[0].blend != 0) {
            ret = 0;
        }
        break;
    case 0x593:
    case 0x594:
    case 0x598:
        ret = 0;
        break;
    }
    return ret;
}

void HuntingHorn_ShowSongMessage(Player *p, u8 song)
{
    char buf[0x100];

    if (Player_IsLocal(p)) {
        sprintf(buf, "%s%s", gHornSongTexts[0], gHornSongTexts[song]);
        Notice_PushText(buf);
    }
}

/* two empty strings after "%s%s" whose users were dropped */
const u8 D_008DD028[8] = {0};

void HuntingHorn_PlaySong(Player *p, u8 song, u8 quiet)
{
    Player *pl;
    s16 i;
    s16 n;
    u8 a;
    u8 b;
    u8 k;

    switch (song) {
    case 0:
    case 1:
        if (Player_HasMealEffect(p, 0) == 1) {
            p->mealEffect[0] = 2;
            p->mealEffectTimer[0] += 0x708;
            if (p->mealEffectTimer[0] >= 0xE10) {
                p->mealEffectTimer[0] = 0xE10;
            }
            HuntingHorn_ShowSongMessage(p, 2);
        } else {
            p->mealEffect[0] = 1;
            p->mealEffectTimer[0] = 0xE10;
            HuntingHorn_ShowSongMessage(p, 1);
        }
        break;
    case 2:
    case 3:
        if (Player_HasMealEffect(p, 1) == 1) {
            if (song == 2) {
                if (p->mealEffect[1] <= 2) {
                    p->mealEffect[1] = 2;
                    p->mealEffectTimer[1] += 0x708;
                    if (p->mealEffectTimer[1] >= 0xA8C) {
                        p->mealEffectTimer[1] = 0xA8C;
                    }
                    HuntingHorn_ShowSongMessage(p, 4);
                }
            } else if (p->mealEffect[1] >= 3) {
                p->mealEffect[1] = 4;
                p->mealEffectTimer[1] += 0x384;
                if (p->mealEffectTimer[1] >= 0x708) {
                    p->mealEffectTimer[1] = 0x708;
                }
                HuntingHorn_ShowSongMessage(p, 6);
            } else {
                p->mealEffect[1] = 3;
                p->mealEffectTimer[1] = 0x708;
                HuntingHorn_ShowSongMessage(p, 5);
            }
        } else if (song == 2) {
            p->mealEffect[1] = 1;
            p->mealEffectTimer[1] = 0xA8C;
            HuntingHorn_ShowSongMessage(p, 3);
        } else {
            p->mealEffect[1] = 3;
            p->mealEffectTimer[1] = 0x708;
            HuntingHorn_ShowSongMessage(p, 5);
        }
        break;
    case 4:
    case 5:
        if (Player_HasMealEffect(p, 2) == 1) {
            if (song == 4) {
                if (p->mealEffect[2] <= 2) {
                    p->mealEffect[2] = 2;
                    p->mealEffectTimer[2] += 0x708;
                    if (p->mealEffectTimer[2] >= 0xA8C) {
                        p->mealEffectTimer[2] = 0xA8C;
                    }
                    HuntingHorn_ShowSongMessage(p, 8);
                }
            } else if (p->mealEffect[2] >= 3) {
                p->mealEffect[2] = 4;
                p->mealEffectTimer[2] += 0x384;
                if (p->mealEffectTimer[2] >= 0x708) {
                    p->mealEffectTimer[2] = 0x708;
                }
                HuntingHorn_ShowSongMessage(p, 0xA);
            } else {
                p->mealEffect[2] = 3;
                p->mealEffectTimer[2] = 0x708;
                HuntingHorn_ShowSongMessage(p, 9);
            }
        } else if (song == 4) {
            p->mealEffect[2] = 1;
            p->mealEffectTimer[2] = 0xA8C;
            HuntingHorn_ShowSongMessage(p, 7);
        } else {
            p->mealEffect[2] = 3;
            p->mealEffectTimer[2] = 0x708;
            HuntingHorn_ShowSongMessage(p, 9);
        }
        break;
    case 6:
    case 7:
    case 8:
        switch (song) {
        case 6:
        default:
            a = 10;
            HuntingHorn_ShowSongMessage(p, 0xB);
            break;
        case 7:
            a = 20;
            HuntingHorn_ShowSongMessage(p, 0xD);
            break;
        case 8:
            a = 30;
            HuntingHorn_ShowSongMessage(p, 0xF);
            break;
        }
        if (Player_HasMealEffect(p, 3) == 1) {
            k = p->mealEffect[3];
            if (k == a) {
                p->mealEffectTimer[3] += 0x708;
                if (p->mealEffectTimer[3] >= 0xE10) {
                    p->mealEffectTimer[3] = 0xE10;
                }
            } else if (a > k) {
                Player_AddMaxHp(p, a - k, 1);
                Player_AddHp(p, a - p->mealEffect[3]);
                p->mealEffect[3] = a;
                p->mealEffectTimer[3] = 0xE10;
            }
        } else {
            n = a;
            Player_AddMaxHp(p, n, 1);
            Player_AddHp(p, n);
            p->mealEffect[3] = a;
            p->mealEffectTimer[3] = 0xE10;
        }
        break;
    case 9:
    case 10:
    case 11:
    case 12:
        switch (song) {
        case 9:
        default:
        case 10:
            a = 1;
            b = 2;
            break;
        case 11:
            a = 2;
            b = 3;
            break;
        case 12:
            a = 3;
            b = a;
            break;
        }
        if (Player_HasMealEffect(p, 4) == 1) {
            k = p->mealEffect[4];
            if (k == a || k == b) {
                if (k <= 2) {
                    p->mealEffect[4] = b;
                    p->mealEffectTimer[4] = 0x708;
                } else {
                    p->mealEffect[4] = b;
                    p->mealEffectTimer[4] += 0x708;
                    if (p->mealEffectTimer[4] >= 0xE10) {
                        p->mealEffectTimer[4] = 0xE10;
                    }
                }
            } else if (a > k) {
                p->mealEffect[4] = a;
                p->mealEffectTimer[4] = 0xE10;
            } else {
                break;
            }
        } else {
            p->mealEffect[4] = a;
            p->mealEffectTimer[4] = 0xE10;
        }
        switch (p->mealEffect[4]) {
        case 1:
        default:
            HuntingHorn_ShowSongMessage(p, 0x11);
            break;
        case 2:
            HuntingHorn_ShowSongMessage(p, 0x13);
            break;
        case 3:
            HuntingHorn_ShowSongMessage(p, 0x15);
            break;
        }
        break;
    case 13:
    case 14:
        p->mealEffect[5] = 1;
        if (Player_HasMealEffect(p, 5) == 1) {
            p->mealEffectTimer[5] += 0x384;
            if (song == 13) {
                if (p->mealEffectTimer[5] >= 0x708) {
                    p->mealEffectTimer[5] = 0x708;
                }
            } else if (p->mealEffectTimer[5] >= 0xE10) {
                p->mealEffectTimer[5] = 0xE10;
            }
            HuntingHorn_ShowSongMessage(p, 0x34);
        } else if (song == 13) {
            p->mealEffectTimer[5] = 0x708;
            HuntingHorn_ShowSongMessage(p, 0x17);
        } else {
            p->mealEffectTimer[5] = 0xE10;
            HuntingHorn_ShowSongMessage(p, 0x19);
        }
        break;
    case 15:
    case 16:
        if (Rand_Next(1) % 3 != 0) {
            p->poison = 0;
            HuntingHorn_ShowSongMessage(p, 0x33);
        } else {
            Player_AddHp(p, 10);
            HuntingHorn_ShowSongMessage(p, 0x1B);
        }
        break;
    case 17:
        if (Rand_Next(1) % 3 != 0) {
            p->poison = 0;
            HuntingHorn_ShowSongMessage(p, 0x33);
        } else {
            Player_AddHp(p, 20);
            HuntingHorn_ShowSongMessage(p, 0x1D);
        }
        break;
    case 18:
    case 19:
        if (Player_HasMealEffect(p, 6) == 1) {
            switch (p->mealEffect[6]) {
            case 1:
            default:
            case 2:
                if (song == 18) {
                    p->mealEffect[6] = 2;
                    HuntingHorn_ShowSongMessage(p, 0x20);
                } else {
                    p->mealEffect[6] = 3;
                    HuntingHorn_ShowSongMessage(p, 0x21);
                }
                break;
            case 3:
            case 4:
                if (song == 19) {
                    p->mealEffect[6] = 4;
                    HuntingHorn_ShowSongMessage(p, 0x22);
                } else {
                    p->mealEffect[6] = 1;
                    HuntingHorn_ShowSongMessage(p, 0x1F);
                }
                break;
            }
        } else if (song == 18) {
            p->mealEffect[6] = 1;
            HuntingHorn_ShowSongMessage(p, 0x1F);
        } else {
            p->mealEffect[6] = 3;
            HuntingHorn_ShowSongMessage(p, 0x21);
        }
        p->mealEffectTimer[6] = 0x708;
        break;
    case 20:
    case 21:
        if (Player_HasMealEffect(p, 7) == 1) {
            switch (p->mealEffect[7]) {
            case 1:
            default:
            case 2:
                if (song == 20) {
                    p->mealEffect[7] = 2;
                    HuntingHorn_ShowSongMessage(p, 0x24);
                } else {
                    p->mealEffect[7] = 3;
                    HuntingHorn_ShowSongMessage(p, 0x25);
                }
                break;
            case 3:
            case 4:
                if (song == 21) {
                    p->mealEffect[7] = 4;
                    HuntingHorn_ShowSongMessage(p, 0x26);
                } else {
                    p->mealEffect[7] = 1;
                    HuntingHorn_ShowSongMessage(p, 0x23);
                }
                break;
            }
        } else if (song == 20) {
            p->mealEffect[7] = 1;
            HuntingHorn_ShowSongMessage(p, 0x23);
        } else {
            p->mealEffect[7] = 3;
            HuntingHorn_ShowSongMessage(p, 0x25);
        }
        p->mealEffectTimer[7] = 0x708;
        break;
    case 22:
    case 23:
        if (Player_HasMealEffect(p, 8) == 1) {
            switch (p->mealEffect[8]) {
            case 1:
            default:
            case 2:
                if (song == 22) {
                    p->mealEffect[8] = 2;
                    HuntingHorn_ShowSongMessage(p, 0x28);
                } else {
                    p->mealEffect[8] = 3;
                    HuntingHorn_ShowSongMessage(p, 0x29);
                }
                break;
            case 3:
            case 4:
                if (song == 23) {
                    p->mealEffect[8] = 4;
                    HuntingHorn_ShowSongMessage(p, 0x2A);
                } else {
                    p->mealEffect[8] = 1;
                    HuntingHorn_ShowSongMessage(p, 0x27);
                }
                break;
            }
        } else if (song == 22) {
            p->mealEffect[8] = 1;
            HuntingHorn_ShowSongMessage(p, 0x27);
        } else {
            p->mealEffect[8] = 3;
            HuntingHorn_ShowSongMessage(p, 0x29);
        }
        p->mealEffectTimer[8] = 0x708;
        break;
    case 24:
    case 25:
        if (Player_HasMealEffect(p, 9) == 1) {
            switch (p->mealEffect[9]) {
            case 1:
            default:
            case 2:
                if (song == 24) {
                    p->mealEffect[9] = 2;
                    HuntingHorn_ShowSongMessage(p, 0x2C);
                } else {
                    p->mealEffect[9] = 3;
                    HuntingHorn_ShowSongMessage(p, 0x2D);
                }
                break;
            case 3:
            case 4:
                if (song == 25) {
                    p->mealEffect[9] = 4;
                    HuntingHorn_ShowSongMessage(p, 0x2E);
                } else {
                    p->mealEffect[9] = 1;
                    HuntingHorn_ShowSongMessage(p, 0x2B);
                }
                break;
            }
        } else if (song == 24) {
            p->mealEffect[9] = 1;
            HuntingHorn_ShowSongMessage(p, 0x2B);
        } else {
            p->mealEffect[9] = 3;
            HuntingHorn_ShowSongMessage(p, 0x2D);
        }
        p->mealEffectTimer[9] = 0x708;
        break;
    case 26:
    case 27:
    case 28:
    case 29:
        if (Player_HasMealEffect(p, 10) == 1) {
            switch (p->mealEffect[10]) {
            case 1:
            default:
            case 2:
                if (song == 26 || song == 27) {
                    p->mealEffect[10] = 2;
                    HuntingHorn_ShowSongMessage(p, 0x30);
                } else {
                    p->mealEffect[10] = 3;
                    HuntingHorn_ShowSongMessage(p, 0x31);
                }
                break;
            case 3:
            case 4:
                if (song == 28 || song == 29) {
                    p->mealEffect[10] = 4;
                    HuntingHorn_ShowSongMessage(p, 0x32);
                } else {
                    p->mealEffect[10] = 1;
                    HuntingHorn_ShowSongMessage(p, 0x2F);
                }
                break;
            }
        } else if (song == 26 || song == 27) {
            p->mealEffect[10] = 1;
            HuntingHorn_ShowSongMessage(p, 0x2F);
        } else {
            p->mealEffect[10] = 3;
            HuntingHorn_ShowSongMessage(p, 0x31);
        }
        p->mealEffectTimer[10] = 0x708;
        break;
    }
    if (quiet == 0) {
        QSync_SendPlayer(p, 10, 0);
    }
    if (Player_IsLocal(p) == 1) {
        switch (song) {
        case 0:
        case 1:
            Effect06_SpawnAtBone(p, 0, 2, 10, 4.0f);
            break;
        default:
            pl = gPlayers;
            i = 0;
            if (i < gScene.playerCount) {
                do {
                    if (pl->active != 0 && pl->visible != 0 && (u8)Obj_InCurrentArea(pl) != 0
                        && (Game_IsOnline() != 1 || gScene.inQuest[i] == 1) && pl->areaMoveState == 0
                        && Side_Match(p->side, pl->side, 0) == 1) {
                        switch (pl->stateGroup) {
                        case 3:
                        case 4:
                            break;
                        default:
                            Effect06_SpawnAtBone(pl, 0, 2, 10, 4.0f);
                            break;
                        }
                    }
                    i++;
                    pl++;
                } while (i < gScene.playerCount);
            }
            break;
        }
    }
}

void HuntingHorn_SongFxAllies(Player *p)
{
    s16 i;
    Player *pl;
    SceneState *sc;

    i = 0;
    if (i < gScene.playerCount) {
        pl = gPlayers;
        sc = &gScene;
        do {
            if (pl->active != 0 && pl->visible != 0 && (u8)Obj_InCurrentArea(pl) != 0
                && (Game_IsOnline() != 1 || sc->inQuest[0] == 1) && pl->areaMoveState == 0
                && (i == p->index || Side_Match(p->side, pl->side, 0) == 1)) {
                switch (pl->stateGroup) {
                case 3:
                case 4:
                    break;
                default:
                    Effect06_SpawnAtBone(pl, 0, 2, 10, 4.0f);
                    break;
                }
            }
            i++;
            pl++;
            sc = (SceneState *) ((u8 *) sc + 1);
        } while (i < gScene.playerCount);
    }
}

void HuntingHorn_SongFxSelf(Player *p)
{
    switch (p->stateGroup) {
    case 3:
    case 4:
        break;
    default:
        Effect06_SpawnAtBone(p, 0, 2, 0xA, 4.0f);
        break;
    }
}

void HuntingHorn_CheckSongs(Player *p)
{
    s16 i;
    s16 steps;
    s16 matched;
    u8 last;
    u8 *note;
    u8 c;
    u8 ok;

    for (i = 0; i < 30; i++) {
        last = p->hornNoteSlot;
        matched = 0;
        steps = 0;
        /* the songs: 30 entries of 4 notes (0: any; 0x80: any but this one) */
        note = HuntingHorn_SongNotes + (u32) (i * 4);
        do {
            c = *note;
            ok = 0;
            if (c == 0) {
                ok = 1;
            } else if (c & 0x80) {
                if ((c & 0x7F) != p->hornNotes[last]) {
                    ok = 1;
                }
            } else {
                if ((c & 0x7F) == p->hornNotes[last]) {
                    ok = 1;
                }
            }
            if (ok == 1) {
                matched++;
                note++;
                if (matched >= 4) {
                    p->hornSong = i;
                    HuntingHorn_PlaySong(p, p->hornSong, 0);
                    break;
                }
            } else if (matched > 0 || steps == 0) {
                break;
            }
            if (last == 0) {
                last = 7;
            } else {
                last--;
            }
            steps++;
        } while (steps < 8);
    }
}

void HuntingHorn_AddNote(Player *p, u8 note)
{

    p->hornNoteSlot++;
    if (p->hornNoteSlot >= 8) {
        p->hornNoteSlot = 0;
    }
    p->hornNotes[p->hornNoteSlot] = (&gBladeSpecialMarks)[p->bladeSpecial * 3 + note];
    Sound_PlaySeFlat(0, 0x22, 0);
    HuntingHorn_CheckSongs(p);
}

void HuntingHorn_ClearNotes(Player *p)
{
    p->hornNoteSlot = 0;
    p->hornNotes[0] = 0;
    p->hornNotes[1] = 0;
    p->hornNotes[2] = 0;
    p->hornNotes[3] = 0;
    p->hornNotes[4] = 0;
    p->hornNotes[5] = 0;
    p->hornNotes[6] = 0;
    p->hornNotes[7] = 0;
}

s32 HuntingHorn_AttackStateKind(Player *p)
{
    if (Player_IsLocal(p) == 0) {
        return 0;
    }
    if (p->stateGroup != 1) {
        return 0;
    }
    switch (p->state) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 0x16:
    case 0x24:
    case 0x27:
        return (p->stateVar0 == 0) ? 1 : 2;
    }
    return 0;
}

s16 HuntingHorn_StateKind(Player *p)
{
    s16 r;

    r = HuntingHorn_AttackStateKind(p);
    if (r != 0) {
        return r;
    }
    if (p->stateGroup != 1) {
        return 0;
    }
    switch (p->state) {
    case 0xF:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x17:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
    case 0x21:
    case 0x22:
        return 3;
    }
    return 0;
}

u8 HuntingHorn_AttackNoteColour(Player *p)
{
    s32 note;

    note = HuntingHorn_AttackNote(p) & 0xFF;
    if (note > 0) {
        note = (note - 1) & 0xFF;
    }
    return *((p->bladeSpecial * 3) + (&gBladeSpecialMarks + (note & 0xFF)));
}

static s32 Gunlance_CanShell(Player *p)
{
    switch (p->sharpnessLevel) {
    case 0:
        return 0;
    default:
        return 1;
    }
}

s32 Gunlance_CheckHit(Player *p)
{
    if ((p->hitPending != 0) && (p->hitKind == 1)) {
        CamShake_ReqA(p, 0);
        p->hitStop = 0;
        p->hitStopAdd = 0;
        Player_RequestState(p, 1U, 7U, 0);
        return 1;
    }
    return 0;
}

void Gunlance_RequestState2_0C(void *em)
{
    Player_RequestState(em, 0x2, 0xC, 0);
}

void Gunlance_RequestState2_26(void *em)
{
    Player_RequestState(em, 0x2, 0x26, 0);
}

void Gunlance_State00(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        switch (variant) {
        case 0:
            p->step = 2;
            Player_SetMotionPair(p, 0x57D, 4, 0);
            Eff00_Create(p, 0x3F);
            break;
        case 1:
            p->step++;
            Player_SetMotionPair(p, 0x3F2, 2, 0);
            p->weaponOut = 1;
            Eff00_Create(p, 0x41);
            break;
        case 2:
        case 3:
            p->step = 2;
            Player_SetMotionPair(p, 0x57D, 6, 0xA);
            if (variant == 3) {
                Eff00_Create(p, 0x42);
            } else {
                Eff00_Create(p, 0x3F);
            }
            break;
        case 4:
            p->step = 2;
            Player_SetMotionPair(p, 0x57D, 4, 0xA);
            Eff00_Create(p, 0x3F);
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x57D, 0, 0x22);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (Gunlance_CheckHit(p) != 1 && Player_IsLocal(p) == 1 && Motion_InFrameRange(p, 0, 36.0f, 62.0f) != 0) {
            if (p->input.pressed & 1) {
                if (p->roundsLoaded == 0 || Gunlance_CanShell(p) == 0) {
                    Player_SetState(p, 1, 0xE, 0xC);
                } else {
                    Player_SetState(p, 1, 1, 4);
                }
            } else {
                Gunlance_CheckInput(p, 0);
            }
        }
        break;
    }
}

void Gunlance_ShellEffect(Player *p)
{
    s32 kind;

    kind = (s32) p->bladeSpecial % 3;
    switch (kind) {
    case 0:
        EffGunlance38_Create(p, 1);
        break;
    case 1:
        EffGunlance38_Create(p, 2);
        break;
    case 2:
        EffGunlance38_Create(p, 3);
        break;
    }
}

void Gunlance_ShellSharpness(Player *p)
{
    s32 kind;

    kind = (s32) p->bladeSpecial % 3;
    switch (kind) {
    case 0:
        Player_AddSharpness(p, -3);
        break;
    case 1:
        Player_AddSharpness(p, -3);
        break;
    case 2:
        Player_AddSharpness(p, -4);
        break;
    }
}

void Gunlance_State01(Player *p, s32 variant)
{
    f32 lo;
    f32 hi;
    u16 st;

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        p->roundsLoaded--;
        Player_SetMoveMode(p, 0, 1, 0);
        Player_SetMotionPair(p, Gunlance_State01Motions[variant][0], D_00897022[variant][0], 0);
        if (variant == 4) {
            p->step = 2;
            break;
        }
        if (variant <= 2) {
        fire:
            p->step = 2;
            Gunlance_ShellEffect(p);
            Gunlance_ShellSharpness(p);
            p->timer = 0;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            switch (variant) {
            case 3:
                Player_SetMotionPair(p, 0x579, 0, 0);
                break;
            case 4:
                Player_SetMotionPair(p, 0x579, 0, 0);
                break;
            case 5:
                Player_SetMotionPair(p, 0x57A, 0, 0);
                break;
            case 6:
                Player_SetMotionPair(p, 0x57B, 0, 0);
                break;
            }
            goto fire;
        }
        break;
    case 2:
        if (variant == 4 && p->stateVar1 == 0 && p->motion[0].blend == 0) {
            p->stateVar1++;
            Gunlance_ShellEffect(p);
            Gunlance_ShellSharpness(p);
            p->timer = 0;
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (Motion_PassedFrame(p, 0, 2.0f) != 0 && (u8)Obj_InCurrentArea(p) == 1) {
            Vib_StartLocal(p, 1);
            StageObj_K12_CreateGunlanceShell(p, variant);
            switch (p->bladeSpecial % 3) {
            case 0:
                StageObj_Gun_CreateShell(p, 0);
                break;
            case 1:
                StageObj_Gun_CreateShell(p, 2);
                break;
            case 2:
                StageObj_Gun_CreateShell(p, 1);
                break;
            }
        }
        switch (variant) {
        default:
        case 0:
        case 3:
        case 4:
            lo = 32.0f;
            hi = 62.0f;
            st = 0x18;
            break;
        case 1:
        case 5:
            lo = 28.0f;
            hi = 58.0f;
            st = 0x19;
            break;
        case 2:
        case 6:
            lo = 32.0f;
            st = 0x1A;
            hi = 62.0f;
            break;
        }
        if (Player_IsLocal(p) == 1) {
            if (Motion_InFrameRange(p, 0, lo, hi) != 0 && (p->input.pressed & 1)) {
                if (p->roundsLoaded == 0 || Gunlance_CanShell(p) == 0) {
                    Player_RequestState(p, 1, 0xE, 0xC);
                } else {
                    Player_RequestState(p, 1, st, 0xC);
                }
            } else if (Motion_ReachedFrame(p, 0, 42.0f) != 0 && (p->input.stickDirPressed & 0x3C)
                       && ((p->input.prevStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
                switch (variant) {
                case 0:
                case 3:
                case 4:
                    Player_SetState(p, 1, 0, 0xC);
                    break;
                case 1:
                case 5:
                    Player_SetState(p, 1, 2, 0xC);
                    break;
                case 2:
                case 6:
                    Player_SetState(p, 1, 2, 0xC);
                    break;
                }
            }
        }
        break;
    }
}

void Gunlance_State02(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        Player_SetMoveMode(p, 0, 1, 0);
        switch (variant) {
        default:
            p->step = 2;
            Player_SetMotionPair(p, 0x588, 2, 0);
            Eff00_Create(p, 0x43);
            break;
        case 3:
        case 4:
            p->step++;
            Player_SetMotionPair(p, 0x58D, 4, 0);
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x588, 0, 8);
            Eff00_Create(p, 0x43);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Gunlance_RequestState2_0C(p);
            break;
        }
        if (Gunlance_CheckHit(p) != 1 && Player_IsLocal(p) == 1 && Motion_InFrameRange(p, 0, 36.0f, 50.0f) != 0) {
            if (p->input.pressed & 1) {
                if (p->roundsLoaded == 0 || Gunlance_CanShell(p) == 0) {
                    Player_SetState(p, 1, 0xE, 0xC);
                } else {
                    Player_SetState(p, 1, 0xC, 4);
                }
            } else {
                Gunlance_CheckInput(p, 0);
            }
        }
        break;
    }
}

static void Gunlance_SetAimLayers(Player *p, u16 motA, u16 motB)
{

    if (p->aimWeight == 0) {
        p->partAnimOn[0] = 0;
        p->partAnimOn[1] = 0;
        return;
    }
    p->partAnimOn[0] = 1;
    p->partAnimOn[1] = 1;
    if (p->aimWeight > 0) {
        p->partAnimMotion[0] = motA;
        p->partAnimMotion[1] = motA + 100;
        p->partAnimWeight[0] = p->aimWeight;
        p->partAnimWeight[1] = p->aimWeight;
    } else {
        p->partAnimMotion[0] = motB;
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

void Gunlance_AddAimWeight(Player *p, s32 delta)
{

    p->aimWeight = p->aimWeight + delta;
    if (p->aimWeight >= 100) {
        p->aimWeight = 100;
    }
    if (p->aimWeight < -99) {
        p->aimWeight = -100;
    }
}

void Gunlance_State03(Player *p, s32 variant)
{
    u16 h;

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        Player_SetMotionPair(p, 0x57F, 4, 0);
        p->timer = 0;
        p->noFlinchTimer = 2;
        break;
    case 1:
        p->noFlinchTimer = 2;
        if (p->motion[0].playing == 0) {
            p->step++;
            Gunlance_SetAimLayers(p, 0x58B, 0x58C);
            Player_SetMotionPair(p, 0x58A, 0, 0);
            break;
        }
        if (Motion_ReachedFrame(p, 0, 42.0f) != 0 && p->stateVar0 == 0) {
            p->stateVar0++;
            StageObj_Gun_CreateType4(p);
        }
        break;
    case 2:
        p->noFlinchTimer = 2;
        p->timer++;
        if (p->timer >= 50) {
            p->step++;
            p->rot[1] += (u16) (s32) (0.5f + 65536.0f * (-0.45f * p->aimWeight) / 360.0f);
            p->targetYaw = p->rot[1];
            Player_ClearPartOverrides(p);
            Player_SetMotionPair(p, 0x580, 0, 0);
            p->noFlinchTimer = 10;
            break;
        }
        if (Player_IsLocal(p) == 1) {
            h = p->input.stickDir;
            if (h & 0x800) {
                Gunlance_AddAimWeight(p, -5);
            } else if (h & 0x400) {
                Gunlance_AddAimWeight(p, 5);
            }
            Gunlance_SetAimLayers(p, 0x58B, 0x58C);
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (Motion_PassedFrame(p, 0, 2.0f) != 0) {
            StageObj_K12_CreateGunlanceShell(p, 3);
            StageObj_Gun_CreateShell(p, 3);
            EffGunlance38_Create(p, 4);
            Player_AddSharpness(p, -10);
            Vib_StartLocal(p, 2);
            CamShake_ReqA(p, 2);
            p->weaponTimer = 0x546;
        }
        if (Motion_PassedFrame(p, 0, 44.0f) != 0) {
            StageObj_Gun_CreateType6(p);
        }
        break;
    }
}

void Gunlance_State04(Player *p, s32 unused)
{

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        Player_SetMotionPair(p, 0x57E, 4, 0xA);
        Eff00_Create(p, 0x40);
        p->noFlinchTimer = 0x19;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (Gunlance_CheckHit(p) != 1 && Player_IsLocal(p) == 1 && Motion_InFrameRange(p, 0, 54.0f, 80.0f) != 0) {
            if (p->input.pressed & 1) {
                if (p->roundsLoaded == 0 || Gunlance_CanShell(p) == 0) {
                    Player_SetState(p, 1, 0xE, 0xC);
                } else {
                    Player_SetState(p, 1, 0xD, 4);
                }
            } else {
                Gunlance_CheckInput(p, 0);
            }
        }
        break;
    }
}

void Gunlance_State05(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 1, 0);
        if (variant == 0) {
            Player_SetMotionPair(p, 0x57C, 2, 0);
        } else {
            Player_SetMotionPair(p, 0x57C, 2, 6);
        }
        p->roundsLoaded = (u8) p->clipSize;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (Motion_PassedFrame(p, 0, 44.0f) != 0) {
            StageObj_Gun_CreateReload(p, 3);
        }
        break;
    }
}

void Gunlance_State06(Player *p, s32 variant)
{
    f32 in[3];
    f32 out[3];

    switch (p->step) {
    case 0:
        p->noFlinchTimer = 0x28;
        p->weaponOut = 1;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        switch (variant) {
        case 0:
            p->step = 2;
            Player_SetMotionPair(p, 0x581, 4, 0xA);
            Eff00_Create(p, 0x46);
            break;
        case 1:
            p->step++;
            Player_SetMotionPair(p, 0x3F2, 2, 0);
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x581, 0, 0x2A);
            Eff00_Create(p, 0x46);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (Gunlance_CheckHit(p) != 1) {
            if (p->motionId[0] == 0x581 && Motion_ReachedFrame(p, 0, 44.0f) == 0) {
                in[0] = 0.0f;
                in[1] = 0.0f;
                in[2] = 4.0f;
                Vec_Rotate((f32 *) out, (f32 *) in, (f32 *) p->rotMtx);
                p->pos[0] += out[0];
                p->pos[2] += out[2];
            }
            if (Player_IsLocal(p) == 1 && Motion_InFrameRange(p, 0, 78.0f, 98.0f) != 0) {
                if (p->input.pressed & 1) {
                    if (p->roundsLoaded == 0 || Gunlance_CanShell(p) == 0) {
                        Player_SetState(p, 1, 0xE, 0xC);
                    } else {
                        Player_SetState(p, 1, 0xC, 4);
                    }
                } else {
                    Gunlance_CheckInput(p, 0);
                }
            }
        }
        break;
    }
}

void Gunlance_State07(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x589, 0, 0);
        CamShake_ReqA(p, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        break;
    case 1:
        if (p->stateVar0 != 0) {
            if (Motion_ReachedFrame(p, 0, 42.0f) != 0) {
                Player_SetState(p, 1, 0xB, 0xC);
            }
        } else if (Motion_InFrameRange(p, 0, 10.0f, 40.0f) != 0 && p->stateVar0 == 0 && p->itemSelect == 0 && (p->input.stickDirPressed & 0x3C)) {
            p->stateVar0 = 1;
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Gunlance_State14(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x582, 4, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            if (variant == 0) {
                Player_BeginState(p, 0, 4, 0);
                break;
            }
            Gunlance_RequestState2_26(p);
        } else {
        }
        break;
    }
}

void Gunlance_RunState(Player *p)
{
    switch (p->state) {
    case 0:
        Gunlance_State00(p, 0);
        break;
    case 1:
        Gunlance_State01(p, 0);
        break;
    case 2:
        Gunlance_State02(p, 0);
        break;
    case 3:
        Gunlance_State03(p, 0);
        break;
    case 4:
        Gunlance_State04(p, 0);
        break;
    case 5:
        Gunlance_State05(p, 0);
        break;
    case 6:
        Gunlance_State06(p, 0);
        break;
    case 7:
        Gunlance_State07(p, 0);
        break;
    case 8:
        Gunlance_State00(p, 1);
        break;
    case 9:
        Gunlance_State00(p, 2);
        break;
    case 10:
        Gunlance_State00(p, 3);
        break;
    case 11:
        Gunlance_State00(p, 4);
        break;
    case 12:
        Gunlance_State01(p, 1);
        break;
    case 13:
        Gunlance_State01(p, 2);
        break;
    case 14:
        Gunlance_State14(p, 0);
        break;
    case 15:
        Gunlance_State02(p, 1);
        break;
    case 16:
        Gunlance_State02(p, 2);
        break;
    case 17:
        Gunlance_State04(p, 1);
        break;
    case 18:
        Gunlance_State04(p, 2);
        break;
    case 19:
        Gunlance_State14(p, 1);
        break;
    case 20:
        Gunlance_State06(p, 1);
        break;
    case 21:
        Gunlance_State02(p, 3);
        break;
    case 22:
        Gunlance_State02(p, 4);
        break;
    case 23:
        Gunlance_State01(p, 3);
        break;
    case 24:
        Gunlance_State01(p, 4);
        break;
    case 25:
        Gunlance_State01(p, 5);
        break;
    case 26:
        Gunlance_State01(p, 6);
        break;
    case 27:
        Gunlance_State05(p, 1);
        
    default:
        break;
    }
}

s32 Gunlance_AttackInput(Player *p)
{

    if ((p->input.pressed & 0x40) && p->stamina >= 0x4B) {
        Player_SetState(p, 0, 0x6D, 4);
    }
    if (p->input.pressed & 1) {
        if (p->roundsLoaded == 0 || Gunlance_CanShell(p) == 0) {
            Player_SetState(p, 1, 0xE, 0xC);
        } else {
            Player_SetState(p, 1, 0x17, 0xC);
        }
    }
    if (p->input.stickDirPressed & 0x3C) {
        if (((p->input.prevStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
            if (p->input.stickTilt >= 0x78) {
                Player_SetState(p, 1, 6, 0xC);
            } else {
                Player_SetState(p, 1, 0, 0xC);
            }
        } else {
            Player_SetState(p, 1, 4, 0xC);
        }
    }
    return 0;
}

s32 Gunlance_CheckInput(Player *p, s32 caller)
{
    s32 dir;
    u8 state;
    u8 state2;

    if ((Player_IsLocal(p) == 0) && (Game_IsOnline() == 1)) {
        return 0;
    }
    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    if (p->itemSelect != 0) {
        return 0;
    }
    if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B)) {
        if (p->input.stickDir & 0x3C00) {
            dir = (p->input.stickAngle + 0x2000) & 0xFFFF;
            if (dir < 0x6000) {
                Player_SetState(p, 0U, 0x6CU, 4);
                return 1;
            }
            if ((dir >= 0x6000) && (dir < 0xC001)) {
                Player_SetState(p, 0U, 0x6BU, 4);
                return 1;
            }
            Player_SetState(p, 0U, 0x6EU, 4);
            return 1;
        }
        Player_SetState(p, 0U, 0x6DU, 4);
        return 1;
    }
    if ((p->input.stickDirPressed & 0x3C) && (caller != 3)) {
        if (((p->input.prevStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
            state = p->state;
            switch (state) {
            case 2:
                Player_SetState(p, 1U, 0xFU, 4);
                break;
            case 4:
                Player_SetState(p, 1U, 0x15U, 4);
                break;
            case 15:
            case 21:
                Player_SetState(p, 1U, 0x10U, 4);
                break;
            case 17:
                Player_SetState(p, 1U, 0x16U, 4);
                break;
            case 0:
            case 8:
            case 11:
                Player_SetState(p, 1U, 9U, 4);
                break;
            case 9:
                Player_SetState(p, 1U, 0xAU, 4);
                break;
            case 6:
            case 20:
                Player_SetState(p, 1U, 9U, 4);
                break;
            }
        } else {
            state2 = p->state;
            switch (state2) {
            case 0:
            case 8:
            case 11:
            case 2:
                Player_SetState(p, 1U, 0x11U, 4);
                break;
            case 15:
            case 21:
            case 9:
                Player_SetState(p, 1U, 0x12U, 4);
                break;
            }
        }
    }
    return 0;
}

s32 Gunlance_GatherInput(Player *p, s32 unused)
{
    u16 btn;

    if (p->input.pressed & 1) {
        if (p->weaponTimer == 0) {
            Player_SetState(p, 1U, 3U, 0xC);
        } else {
            Player_SetState(p, 1U, 0x13U, 0xC);
        }
        return 1;
    }
    btn = p->input.stickDirPressed;
    if (btn & 0x20) {
        Player_SetState(p, 1U, 2U, 0xC);
        return 1;
    }
    if ((btn & 0x10) && ((s32) p->roundsLoaded < (s32) p->clipSize)) {
        Player_SetState(p, 1U, 5U, 0xC);
        return 1;
    }
    return 0;
}

void Gunlance_DrawAttack(Player *p)
{
    Player_SetState(p, 0x1, 0x14, 0x4);
}

s32 Bow_ArcVisible(void *obj, u16 force)
{
    Player *p = obj;

    if (p->stateGroup != 1) {
        return 0;
    }
    switch (p->state) {
    case 0:
    case 0xE:
        return 0;
    case 1:
    case 9:
    case 0xB:
    case 0xF:
    case 0x13:
    case 0x14:
        if (force == 0 && Player_IsLocal(p) == 1 && !(p->input.held & 0x80)) {
            return 0;
        }
        if (p->motionId[0] == 0x57D) {
            if (Motion_ReachedFrame(p, 0, 30.0f) != 0) {
                return 1;
            }
            return 0;
        }
        return 0;
    case 5:
    case 7:
        if (force == 0 && Player_IsLocal(p) == 1) {
            if (!(p->input.held & 0x80)) {
                return 0;
            }
            if (p->motion[0].blend == 1) {
                return 0;
            }
        }
        return 1;
    case 6:
        return force != 0;
    }
    return 0;
}

static void Bow_UpdateAmmoType(Player *player)
{
    u8 slot = player->coatingSlot;

    if (slot == 0xFF) {
        player->ammoType = 0;
    } else {
        player->ammoType = gItemData[player->pouch[slot].item].ammoKind;
    }
}

u8 Bow_ChargeLevel(Player *p)
{
    s16 t = p->weaponTimer;
    u8 lvl;
    u8 max;

    if (Player_HasSkill(p, 0x33) == 1) {
        t = 1.1764705f * t;
    }
    lvl = t / 30;
    if (Player_HasSkill(p, 0xB8) == 1) {
        max = 3;
    } else {
        max = 2;
    }
    if (lvl > max) {
        lvl = max;
    }
    return lvl;
}

static void Bow_AddAimTilt(Player *p, s8 d)
{

    p->aimWeight += d;
    if (p->aimWeight >= 100) {
        p->aimWeight = 100;
    }
    if (p->aimWeight < -99) {
        p->aimWeight = -100;
    }
}

void Bow_DrainStamina(Player *p)
{
    Player_TickWeaponTimer(p);
    Player_UseStamina(p, -1);
}

void Bow_UpdateAim(Player *p, s32 arg1)
{
    u16 h;
    s8 d;

    if (Player_IsLocal(p) == 0) {
        return;
    }
    if (p->aimLockTimer != 0 || p->aimMode != 0) {
        p->aimLockTimer = 10;
    }
    if (p->aimLockTimer == 0 && p->aimMode == 0 && (p->input.held & 4)) {
        return;
    }
    h = p->input.held;
    if (!(h & 0x80)) {
        return;
    }
    if (h & 0x3000) {
        d = 5;
        if (p->aimMode != 0 && gScene.aimReverse != 0) {
            d *= -1;
        }
        if (h & 0x2000) {
            Bow_AddAimTilt(p, d);
        } else {
            Bow_AddAimTilt(p, (s8) -d);
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
    switch (arg1) {
    case 0:
        Bow_SetAimLayers(p, 0x57A, 0x57B);
        break;
    case 1:
        Bow_SetAimLayers(p, 0x57E, 0x57F);
        break;
    case 2:
        Bow_SetAimLayers(p, 0x58B, 0x58C);
        break;
    }
}

static void Bow_SetAimLayers(Player *p, u16 motA, u16 motB)
{

    if (p->aimWeight == 0) {
        p->partAnimOn[0] = 0;
        p->partAnimOn[1] = 0;
        return;
    }
    p->partAnimOn[0] = 1;
    p->partAnimOn[1] = 1;
    if (p->aimWeight > 0) {
        p->partAnimMotion[0] = motA;
        p->partAnimMotion[1] = motA + 100;
        p->partAnimWeight[0] = p->aimWeight;
        p->partAnimWeight[1] = p->aimWeight;
    } else {
        p->partAnimMotion[0] = motB;
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

s32 Bow_CheckHit(Player *p)
{
    if ((p->hitPending != 0) && (p->hitKind == 1)) {
        CamShake_ReqA(p, 0);
        p->hitStop = 0;
        p->hitStopAdd = 0;
        Player_RequestState(p, 1U, 0x10U, 0);
        return 1;
    }
    return 0;
}

void Bow_State00(Player *p, s32 variant)
{
    u8 slot;
    f32 f;

    if (p->aimLockTimer != 0) {
        p->aimLockTimer = 5;
    }
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0xFF;
        Player_SetMoveMode(p, 0, 1, 0);
        if (variant == 0) {
            Bow_SetAimLayers(p, 0x581, 0x582);
            Player_SetMotionPair(p, 0x580, 0, 0);
        } else {
            Bow_SetAimLayers(p, 0x588, 0x589);
            Player_SetMotionPair(p, 0x587, 0, 0);
        }
        Vib_StartLocal(p, 0);
        CamShake_ReqB(p, 1);
        p->chargeLevel = Bow_ChargeLevel(p);
        Bowgun_FireLoaded(p, 0);
        p->weaponTimer = 0;
        if (Player_HasSkill(p, 0x33) == 0) {
            p->roundsLoaded--;
        }
        if (p->coatingSlot != 0xFF) {
            if (Player_IsLocal(p) == 1) {
                slot = p->coatingSlot;
                if (Quest_PhaseIsOver(1) == 0) {
                    Pouch_Add(p, p->pouch[p->coatingSlot].item, -1);
                }
                if (slot == p->ammoSlot && p->pouch[p->ammoSlot].count <= 0) {
                    p->ammoSlot = Bowgun_NextAmmoSlot(p, 0x14, 2);
                    p->ammoCount = p->pouch[p->ammoSlot].count;
                    Bowgun_LoadAmmo(p, 0);
                }
            }
            p->savedAmmoCount = p->ammoCount;
            p->savedClipSize = p->clipSize;
            p->savedRoundsLoaded = p->roundsLoaded;
            p->savedCoatingSlot = p->coatingSlot;
        }
        break;
    case 1:
    case 2:
        if (Motion_PassedFrame(p, 1, 8.0f) != 0 && p->step == 1) {
            p->step++;
        }
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 1, 7, 5);
            break;
        }
        if (variant == 0) {
            f = 36.0f;
        } else {
            f = 22.0f;
        }
        if (Motion_ReachedFrame(p, 0, f) != 0) {
            if ((p->input.pressed & 0x40) && p->stamina >= 0x4B) {
                Player_RequestState(p, 1, 8, 4);
                break;
            }
            if (p->input.stickDir & 0x10) {
                Player_RequestState(p, 1, 0xF, 5);
            }
        }
        break;
    }
}

void Bow_State01(Player *p, u32 variant)
{

    if (p->aimLockTimer != 0) {
        p->aimLockTimer = 5;
    }
    p->weaponOut = 1;
    switch (p->step) {
    case 0:
        switch (variant) {
        case 0:
        case 1:
        case 2:
            p->step = 2;
            Bow_SetAimLayers(p, 0x57E, 0x57F);
            if (variant == 0 || variant == 2) {
                Player_SetMotionPair(p, 0x57D, 0, 0);
                p->weaponTimer = 0;
                p->timer = 0;
                StageObj_K29_CreateOnPlayerDefault(p, 2, 1.0f);
            } else {
                p->timer = 30;
                Player_SetMotionPair(p, 0x57D, 0, 0x78);
                Bow_DrainStamina(p);
            }
            break;
        case 3:
            p->step++;
            Bow_SetAimLayers(p, 0x58B, 0x58C);
            Player_SetMotionPair(p, 0x58A, 0, 0);
            p->weaponTimer = 0;
            p->timer = 0;
            StageObj_K29_CreateOnPlayerDefault(p, 2, 1.0f);
            break;
        case 4:
            p->step++;
            Player_SetMotionPair(p, 0x3F5, 4, 0);
            p->weaponTimer = 0;
            p->timer = 0;
            StageObj_K29_CreateOnPlayerDefault(p, 2, 1.0f);
            break;
        case 5:
            p->step++;
            p->timer = 0;
            Player_SetMotionPair(p, 0x3F4, 0, 0);
            p->weaponTimer = 0;
            StageObj_K29_CreateOnPlayerDefault(p, 2, 1.0f);
            break;
        }
        p->weaponOut = 1;
        p->roundsLoaded = p->clipSize;
        if (p->roundsLoaded > p->ammoCount) {
            p->roundsLoaded = p->ammoCount;
        }
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        Bow_DrainStamina(p);
        if (p->motion[0].playing == 0) {
            p->step++;
            Bow_SetAimLayers(p, 0x57E, 0x57F);
            switch (variant) {
            case 5:
            default:
                Player_SetMotionPair(p, 0x57D, 0, 0x1E);
                break;
            case 3:
                Player_SetMotionPair(p, 0x57D, 0, 0x28);
                break;
            case 4:
                Player_SetMotionPair(p, 0x57D, 0, 0xE);
                break;
            }
        }
        break;
    case 2:
        Bow_DrainStamina(p);
        if (Player_IsLocal(p) == 1 && Motion_ReachedFrame(p, 0, 40.0f) != 0) {
            if (!(p->input.stickDir & 0x10) || p->stamina <= 0) {
                if (Bow_ChargeLevel(p) == 0) {
                    Bow_UpdateAmmoType(p);
                    Player_RequestState(p, 1, 0xE, 5);
                } else {
                    Bow_UpdateAmmoType(p);
                    Player_RequestState(p, 1, 0, 5);
                }
            } else {
                Bow_UpdateAim(p, 1);
                if (p->timer > 0) {
                    p->timer--;
                }
                if (p->timer <= 0) {
                    Bow_CheckInput(p, 0);
                }
            }
        }
        break;
    }
}

void Bow_State02(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x584, 0, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 0, 0);
        }
        break;
    }
}

void Bow_State03(Player *p, s32 variant)
{
    f32 in[3];
    f32 out[3];
    u8 x;

    switch (p->step) {
    case 0:
        p->stateVar0 = 0xFF;
        if (variant == 0) {
            p->step = 2;
            Player_SetMotionPair(p, 0x585, 4, 0);
            Eff00_CreateArrow(p, 0x3C);
        } else {
            p->step++;
            Player_SetMotionPair(p, 0x3F6, 2, 0);
        }
        Player_SetMoveMode(p, 0, 1, 0);
        p->weaponOut = 1;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_UpdateMotionTime(p, 0);
            Player_SetMotionPair(p, 0x585, 4, 0x18);
            Eff00_CreateArrow(p, 0x3C);
        }
        break;
    case 2:
        if (Bow_CheckHit(p) == 1) {
            break;
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (p->stateVar0 == 0xFF) {
            if (Motion_InFrameRange(p, 0, 34.0f, 64.0f) != 0) {
                if ((p->input.stickDirPressed & 0x20) && p->aimLockTimer == 0 && p->aimMode == 0) {
                    p->stateVar0 = 4;
                } else if (p->input.pressed & 0x40) {
                    p->stateVar0 = 0x12;
                } else if (p->input.stickDir & 0x10) {
                    p->stateVar0 = 0x13;
                }
            }
        } else if (Motion_ReachedFrame(p, 0, 44.0f) != 0) {
            x = p->stateVar0;
            if (x == 0x12) {
                if (p->stamina >= 0x4B) {
                    Player_RequestState(p, 1, 0x12, 4);
                }
            } else if (x == 4) {
                Bow_UpdateAmmoType(p);
                Player_RequestState(p, 1, 4, 4);
            } else if (x == 0x13) {
                Player_RequestState(p, 1, 0x13, 4);
            }
            break;
        }
        if (variant == 1 && Motion_ReachedFrame(p, 0, 50.0f) == 0) {
            in[0] = 0.0f;
            in[1] = 0.0f;
            in[2] = 3.0f;
            Vec_Rotate((f32 *) out, (f32 *) in, (f32 *) p->rotMtx);
            p->pos[0] += out[0];
            p->pos[2] += out[2];
        }
        break;
    }
}

void Bow_State04(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        Player_SetMotionPair(p, 0x586, 4, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        Eff00_CreateArrow(p, 0x3D);
        break;
    case 1:
        if ((Bow_CheckHit(p) != 1) && (p->motion[0].playing == 0)) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Bow_State05(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->timer = 20;
        if (variant == 0) {
            Player_SetMotionPair(p, 0x579, 4, 0);
        } else {
            Player_SetMotionPair(p, 0x579, 0, 0);
        }
        Bow_SetAimLayers(p, 0x57A, 0x57B);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (--p->timer <= 0) {
            p->step++;
        }
        if (p->motion[0].blend == 0 && Player_IsLocal(p) == 1) {
            Bow_UpdateAim(p, 0);
            if (variant == 1) {
                Bow_CheckInput(p, 1);
            } else {
                Bow_CheckInput(p, 2);
            }
        }
        break;
    case 2:
        if (Player_IsLocal(p) == 1) {
            if (!(p->input.held & 0x80) && p->aimMode == 0) {
                Player_BeginState(p, 0, 0xA, 0);
            } else {
                Bow_UpdateAim(p, 0);
                Bow_CheckInput(p, 1);
            }
        }
        break;
    }
}

void Bow_State06(Player *p, s32 unused)
{

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x3F1, 4, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetFlag(p, 0x02001000);
        p->timer = 0;
        Bow_DrainStamina(p);
        break;
    case 1:
        Bow_DrainStamina(p);
        if (Player_IsLocal(p) == 1) {
            if (Player_StickMoveLevel(p, 0) == 0) {
                Player_RequestState(p, 1, 9, 4);
            } else if (!(p->input.stickDir & 0x10) || p->stamina <= 0) {
                if (Bow_ChargeLevel(p) == 0) {
                    Bow_UpdateAmmoType(p);
                    Player_RequestState(p, 1, 0xE, 4);
                } else {
                    Bow_UpdateAmmoType(p);
                    Player_RequestState(p, 1, 0, 4);
                }
            } else {
                Bow_CheckInput(p, 0);
            }
        }
        break;
    }
}

void Bow_State08(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->motionTime = 0;
        Player_SetEvadeFrames(p);
        p->timer = 0;
        Player_ResetAim(p);
        p->noFlinchTimer = 0;
        switch (variant) {
        case 0:
            p->step = 2;
            Player_SetMotionPair(p, 0x3EE, 2, 0);
            break;
        case 1:
            p->step = 2;
            Player_SetMotionPair(p, 0x3EE, 0, 6);
            break;
        case 2:
            Player_SetMotionPair(p, 0x3F3, 2, 0);
            break;
        }
        Player_SetMoveMode(p, 0, 0, 0);
        Player_AddStamina(p, -75);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x3EE, 0, 0xE);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            if (p->input.held & 0x80) {
                Player_RequestState(p, 1, 5, 4);
                break;
            }
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (Player_IsLocal(p) == 1) {
            if (Motion_InFrameRange(p, 0, 32.0f, 42.0f) != 0 && (p->input.pressed & 0x40) && p->stateVar0 == 0) {
                p->stateVar0 = 1;
            }
            if (Motion_PassedFrame(p, 0, 42.0f) != 0 && p->stateVar0 != 0 && p->stamina >= 75) {
                Player_RequestState(p, 1, 10, 4);
            }
        }
        break;
    }
}

void Bow_State12(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->motionTime = 0;
        p->weaponOut = 1;
        p->timer = 0;
        Player_SetMotionPair(p, 0x3F2, 4, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 1U, 7U, 0);
        }
        break;
    }
}

void Bow_State16(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x583, 0, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Bow_State17(Player *p, s32 unused)
{
    u16 k;

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x57C, 4, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        p->motionSpeed = 1.0f;
        if (Player_HasSkill(p, 0x2E) == 1) {
            p->motionSpeed = 1.17647052f;
        } else if (Player_HasSkill(p, 0x2F) == 1) {
            p->motionSpeed = 1.33333337f;
        } else if (Player_HasSkill(p, 0x30) == 1) {
            p->motionSpeed = 1.53846157f;
        } else if (Player_HasSkill(p, 0x31) == 1) {
            p->motionSpeed = 0.909090877f;
        } else if (Player_HasSkill(p, 0x32) == 1) {
            p->motionSpeed = 0.714285731f;
        }
        k = p->pouch[p->ammoSlot].item;
        if (k == 0x87 || k == 0) {
            Bow_ClearCoatingSlot(p);
        } else if (p->coatingSlot != 0xFF) {
            Bow_ClearCoatingSlot(p);
            p->ammoType = 0;
        } else {
            Bow_SetCoatingSlot(p);
            p->ammoType = gItemData[p->pouch[p->coatingSlot].item].ammoKind;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Bow_RunState(Player *p)
{
    switch (p->state) {
    case 0:
        Bow_State00(p, 0);
        break;
    case 1:
        Bow_State01(p, 0U);
        break;
    case 2:
        Bow_State02(p, 0);
        break;
    case 3:
        Bow_State03(p, 0);
        break;
    case 4:
        Bow_State04(p, 0);
        break;
    case 5:
        Bow_State05(p, 0);
        break;
    case 6:
        Bow_State06(p, 0);
        break;
    case 7:
        Bow_State05(p, 1);
        break;
    case 8:
        Bow_State08(p, 0);
        break;
    case 9:
        Bow_State01(p, 1U);
        break;
    case 10:
        Bow_State08(p, 1);
        break;
    case 11:
        Bow_State01(p, 2U);
        break;
    case 12:
        Bow_State12(p, 0);
        break;
    case 13:
        Bow_State03(p, 1);
        break;
    case 14:
        Bow_State00(p, 1);
        break;
    case 15:
        Bow_State01(p, 3U);
        break;
    case 16:
        Bow_State16(p, 0);
        break;
    case 17:
        Bow_State17(p, 0);
        break;
    case 18:
        Bow_State08(p, 2);
        break;
    case 19:
        Bow_State01(p, 4U);
        break;
    case 20:
        Bow_State01(p, 5U);
        
    default:
        break;
    }
}

s32 Bow_AttackInput(Player *p, s32 arg)
{
    u8 f;

    if (p->stamina >= 0x4B && (p->input.pressed & 0x40)) {
        if (p->input.stickDir & 0x3C00) {
            Player_RequestState(p, 0, 0x1C, 4);
        } else {
            Player_RequestState(p, 1, 8, 4);
        }
        return 1;
    }
    if (arg == 0) {
        if (p->input.held & 0x80) {
            Player_RequestState(p, 1, 5, 4);
            return 1;
        }
    } else if (p->input.pressed & 0x80) {
        Player_RequestState(p, 1, 5, 4);
        return 1;
    }
    f = p->aimMode;
    if (f != 0) {
        Player_RequestState(p, 1, 7, 4);
        return 1;
    }
    if (p->input.stickDir & 0x10) {
        Player_RequestState(p, 1, 0xB, 5);
        return 1;
    }
    if ((p->input.stickDirPressed & 0x20) && p->aimLockTimer == 0 && f == 0) {
        Bow_UpdateAmmoType(p);
        Player_RequestState(p, 1, 3, 4);
        return 1;
    }
    if ((p->input.pressed & 1) && p->pouch[p->ammoSlot].item != 0x87 && p->coatingSlot == 0xFF) {
        Player_RequestState(p, 1, 0x11, 4);
        return 1;
    }
    return 0;
}

s32 Bow_CheckInput(Player *p, s32 caller)
{

    if (Player_StickMoveLevel(p, 0) != 0) {
        if (caller == 0) {
            if (Player_IsState(p, 1, 6) == 0) {
                Player_FaceStick(p);
                Player_RequestState(p, 1, 6, 0);
            } else {
                Player_FaceStick(p);
            }
        } else if (caller == 1) {
            Player_RequestState(p, 0, 0x74, 0);
        }
    }
    if (p->stamina >= 0x4B && (p->input.pressed & 0x40)) {
        if (p->input.stickDir & 0x3C00) {
            Player_RequestState(p, 0, 0x1C, 4);
        } else {
            Player_RequestState(p, 1, 8, 4);
        }
        return 1;
    }
    if (caller == 0) {
        return 0;
    }
    if (p->input.stickDir & 0x10) {
        Player_RequestState(p, 1, 1, 5);
        return 1;
    }
    if ((p->input.stickDirPressed & 0x20) && p->aimLockTimer == 0 && p->aimMode == 0) {
        Bow_UpdateAmmoType(p);
        Player_RequestState(p, 1, 3, 4);
        return 1;
    }
    if ((p->input.pressed & 1) && p->pouch[p->ammoSlot].item != 0x87 && p->coatingSlot == 0xFF) {
        Player_RequestState(p, 1, 0x11, 4);
        return 1;
    }
    return 0;
}

void Bow_DrawAttack(Player *p)
{
    if (p->input.stickDir & 0x10) {
        Player_RequestState(p, 1U, 0x14U, 5);
        return;
    }
    Bow_UpdateAmmoType(p);
    Player_RequestState(p, 1U, 0xDU, 4);
}

void Bow_Draw(Player *p)
{
    Player_RequestState(p, 0x1, 0xC, 0x4);
}

void Eff00_Create(Player *p, u8 kind)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = 0;
        o->type = kind;
        o->update = Eff00_Update;
        o->idx = p->index;
        o->isEm = p->isEm;
        o->ownerMotion = p->motionId[0];
        o->owner = p;
        o->yaw = p->rot[1];
        o->randBits = p->stateRand;
        o->stateVar0 = 0;
        o->stateVar1 = 0;
        o->rotX = p->rot[0];
        o->rotY = p->rot[1];
        o->rotZ = p->rot[2];
        Vec_Copy(o->pos, &p->pos[0]);
        o->side = *(u8 *) &p->side;
        o->unk080 = 0;
        p->hitPending = 0;
        p->attackHit800 = 0;
    }
}

void Eff00_CreateDual(Player *p, s32 kind, s32 arg2)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = 0;
        o->type = kind;
        o->update = Eff00_Update;
        o->idx = p->index;
        o->isEm = p->isEm;
        o->ownerMotion = p->motionId[0];
        o->owner = p;
        o->yaw = p->rot[1];
        o->randBits = p->stateRand;
        o->stateVar0 = 0;
        o->stateVar1 = arg2;
        o->rotX = p->rot[0];
        o->rotY = p->rot[1];
        o->rotZ = p->rot[2];
        Vec_Copy(o->pos, &p->pos[0]);
        o->side = *(u8 *) &p->side;
        o->unk080 = 0;
        p->hitPending = 0;
    }
}

void Eff00_CreateArrow(Player *p, s8 kind)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = 0;
        o->type = kind;
        o->update = Eff00_Update;
        o->idx = p->index;
        o->isEm = p->isEm;
        o->ownerMotion = p->motionId[0];
        o->owner = p;
        o->yaw = p->rot[1];
        o->randBits = p->stateRand;
        o->stateVar1 = 0;
        o->rotX = p->rot[0];
        o->rotY = p->rot[1];
        o->rotZ = p->rot[2];
        Vec_Copy(o->pos, &p->pos[0]);
        o->side = *(u8 *) &p->side;
        o->stateVar0 = p->ammoType;
        p->hitPending = 0;
    }
}

void Eff00_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff00_Setup(o);
        break;
    case 1:
        Eff00_Run(o);
        break;
    case 2:
        Eff00_End(o);
        break;
    case 3:
        o->state = 1;
        (o)->hitFrames += 2;
        Eff00_Run(o);
        break;
    case 4:
        Eff00_End(o);
        break;
    case 5:
        Eff00_End(o);
        break;
    case 9:
        Eff00_End(o);
        break;
    case 7:
        Eff00_End(o);
        break;
    case 6:
        Eff00_Free(o);
        
    default:
        break;
    }
}

void Eff00_AttachTransform(ObjAttach *o, Player *p)
{
    f32 *iy;
    f32 *iz;
    f32 *oy;
    f32 *oz;
    f32 m0[16];
    f32 m1[16];
    f32 in[3];
    f32 out[3];

    Mtx_Copy(m0, (f32 *)Em_GetBone(p, 0xE));
    Mtx_Identity(m1);
    Mtx_SetRotXYZ(m1, 0.0628318489f, -0.541052103f, -3.04734492f);
    Mtx_Concat3x3(m1, m0);
    in[0] = 0.0f;
    iy = &in[1];
    *iy = 0.0f;
    iz = &in[2];
    *iz = o->hitList->a[2];
    Vec_Rotate(out, in, m1);
    Mtx_GetTrans(o->pos2, m0);
    oy = &out[1];
    oz = &out[2];
    o->pos2[0] += out[0];
    o->pos2[1] += *oy;
    o->pos2[2] += *oz;
    in[0] = 0.0f;
    *iy = 0.0f;
    *iz = o->hitList->b[2];
    Vec_Rotate(out, in, m1);
    Mtx_GetTrans(o->pos, m0);
    o->pos[0] += out[0];
    o->pos[1] += *oy;
    o->pos[2] += *oz;
}

void Eff00_Setup(ObjAttach *o)
{
    Player *p = &gPlayers[o->idx];
    s16 v;
    s32 k;
    f32 in[3];
    f32 out[3];
    u8 f;

    f = 0;
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) p, o->type, &Eff00_MotionRef);
    if (o->stateVar1 & 1) {
        switch (o->hitListNo) {
        case 9:
            o->hitListNo = 10;
            break;
        case 10:
            o->hitListNo = 9;
            break;
        }
    }
    o->hitList = Eff00_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = p->area;
    if (o->hitList->bone == 0x7E) {
        Eff00_AttachTransform(o, p);
    }
    switch (o->type) {
    case 0xB:
    case 0x56:
        in[0] = 0.0f;
        in[1] = 0.0f;
        in[2] = 200.0f;
        Vec_Rotate((f32 *) out, (f32 *) in, (f32 *) p->worldMtx);
        o->pos[0] += out[0];
        o->pos[1] += out[1];
        o->pos[2] += out[2];
        o->aux = 4;
        Player_TestSetFlags(o, 0x60);
        f = 1;
        break;
    case 0x3E:
        in[0] = 117.0f;
        in[1] = 21.0f;
        in[2] = 187.0f;
        Vec_Rotate(out, in, (f32 *)Em_GetBone(p, 0xE));
        Mtx_GetTrans(o->pos, Em_GetBone(p, 0xE)->world);
        f = 1;
        o->pos[0] += out[0];
        o->pos[1] += out[1];
        o->pos[2] += out[2];
        break;
    case 4:
    case 5:
    case 0x1D:
    case 0x60:
    case 0x61:
        Player_TestSetFlags(o, 0x20);
        break;
    case 0x3C:
    case 0x3D:
        Player_TestSetFlags(o, 0x60);
        BowgunShot_SetKind(o, o->stateVar0, 1);
        break;
    default:
        Player_TestSetFlags(o, 0x60);
        f = 1;
        break;
    }
    if (f) {
        v = Player_GetWeaponStat(p, 0);
        if (v > 0) {
            o->elementFlags |= 0x10;
            o->elementPower = v;
        }
        v = Player_GetWeaponStat(p, 1);
        if (v > 0) {
            o->elementFlags |= 0x20;
            o->elementPower = v;
        }
        v = Player_GetWeaponStat(p, 2);
        if (v > 0) {
            o->elementFlags |= 0x40;
            o->elementPower = v;
        }
        v = Player_GetWeaponStat(p, 3);
        if (v > 0) {
            o->elementFlags |= 0x80;
            o->elementPower = v;
        }
        v = Player_GetWeaponStat(p, 7);
        if (v > 0) {
            o->elementFlags |= 0x200;
            o->elementPower = v;
        }
        if (p->stateRand % 3 == 0) {
            v = Player_GetWeaponStat(p, 4);
            if (v > 0) {
                o->elementFlags |= 2;
                o->elementPower = v;
            }
            v = Player_GetWeaponStat(p, 5);
            if (v > 0) {
                o->elementFlags |= 4;
                o->elementPower = v;
            }
            v = Player_GetWeaponStat(p, 6);
            if (v > 0) {
                o->elementFlags |= 1;
                o->elementPower = v;
            }
        }
    }
    o->hitFx &= 0x3F;
    v = Player_GetWeaponStat(p, 8);
    if (v != 0 && p->sharpnessLevel >= 3) {
        if (v < 0) {
            if (p->stateRand % 100 < -v) {

                o->power >>= 1;
                o->hitFx |= 0x40;
            }
        } else if (v > p->stateRand % 100) {
            o->power = (s32) (1.25f * o->power);
            o->hitFx |= 0x80;
        }
    }
    if (p->weaponType == 6 && p->demonMode != 0) {
        o->unk075[0] = 0;
        o->unk075[1] = 0x12;
        o->power = 1.35f * o->power;
    }
    if (o->hitDelay != 0) {
        k = Math_Absf(p->motionBlend[0] % 100);
        o->hitDelay += k - (s32) (Math_Absf((u16) p->motionFrame[0]) / 2.0f) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
}

static void Eff00_PauseInHitStop(ObjAttach *o)
{
    Player *p;

    p = &gPlayers[o->idx];
    if ((p->hitStop != 0) || (((Player *) p)->hitSlowTimer != 0)) {
        (o)->hitFrames++;
    }
}

void Eff00_Run(ObjAttach *o)
{
    Player *p = &gPlayers[o->idx];

    if (o->hitList->bone == 0x7E && o->type != 0x44) {
        Eff00_AttachTransform(o, p);
    }
    Eff00_PauseInHitStop(o);
    switch (o->type) {
    case 8:
        if (p->motionId[0] == o->ownerMotion || p->motionId[0] == 0x57F) {
            return;
        }
        goto end;
    case 0x64:
        if (p->motionId[0] == o->ownerMotion || p->motionId[0] == 0x57E) {
            return;
        }
        goto end;
    case 0xB:
    case 0x56:
        if (--o->aux > 0) {
            return;
        }
        goto end;
    case 0x13:
        if (p->motionId[0] == o->ownerMotion) {
            o->hitFrames = 99;
            return;
        }
        goto end;
    case 0x60:
    case 0x61:
        if (o->hitFrames <= 0) {
            goto end;
        }
        break;
    default:
        if (p->motionId[0] != o->ownerMotion) {
        end:
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    }
}

void Eff00_End(ObjAttach *o)
{
    o->state = 6;
    o->visible = 0;
}

void Eff00_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Eff01_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 1;
            o->type = kind;
            o->update = Eff01_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void Eff01_CreateOnObj(StageObj *s, s8 kind)
{
    ObjAttach *o;
    Em *em;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        em = s->owner;
        o->kind = 1;
        o->type = kind;
        o->update = Eff01_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->ownerMotion = em->motionId[0];
        o->owner = em;
        o->yaw = em->rotY;
        o->rotX = em->rotX;
        o->rotY = em->rotY;
        o->rotZ = em->rotZ;
        o->side = *(u8 *) &em->side;
        o->pos[0] = s->pos[0];
        o->pos[1] = s->pos[1];
        o->pos[2] = s->pos[2];
        em->hitPending = 0;
        em->attackHit800 = 0;
        o->step = 1;
    }
}

void Eff01_CreateAt(Em *em, f32 *pos, s8 kind)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = 1;
        o->type = kind;
        o->update = Eff01_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->ownerMotion = em->motionId[0];
        o->owner = em;
        o->yaw = em->rotY;
        o->rotX = em->rotX;
        o->rotY = em->rotY;
        o->rotZ = em->rotZ;
        o->side = *(u8 *) &em->side;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        em->hitPending = 0;
        em->attackHit800 = 0;
        o->step = 1;
    }
}

void Eff01_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff01_Setup(o);
        break;
    case 1:
        Eff01_Run(o);
        break;
    case 2:
        Eff01_End(o);
        break;
    case 3:
        Eff01_Run(o);
        break;
    case 4:
        Eff01_End(o);
        break;
    case 5:
        Eff01_End(o);
        break;
    case 9:
        Eff01_End(o);
        break;
    case 7:
        Eff01_End(o);
        break;
    case 6:
        Eff01_Free(o);
        
    default:
        break;
    }
}

void Eff01_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &Eff01_MotionRef);
    o->hitList = Eff01_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        o->hitDelay += (s32) Math_Absf((s16) m->motionBlend[0] % 100) - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void Eff01_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 0x18:
    case 0x22:
        o->hitFrames = 99;
        break;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void Eff01_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void Eff01_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

ObjAttach *Eff02_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return NULL;
    }
    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = 2;
        o->type = kind;
        o->update = Eff02_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->ownerMotion = em->motionId[0];
        o->owner = em;
        o->yaw = em->rotY;
        o->rotX = em->rotX;
        o->rotY = em->rotY;
        o->rotZ = em->rotZ;
        o->side = *(u8 *) &em->side;
        em->hitPending = 0;
        em->attackHit800 = 0;
    }
    return o;
}

void Eff02_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff02_Setup(o);
        break;
    case 1:
        Eff02_Run(o);
        break;
    case 2:
        Eff02_End(o);
        break;
    case 3:
        Eff02_Run(o);
        break;
    case 4:
        Eff02_End(o);
        break;
    case 5:
        Eff02_End(o);
        break;
    case 9:
        Eff02_End(o);
        break;
    case 7:
        Eff02_End(o);
        break;
    case 6:
        Eff02_Free(o);
        
    default:
        break;
    }
}

void Eff02_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    o->state++;
    o->visible = 1;
    o->draw = Eff02_DrawNop;
    Player_TestSetFlags(o, 0x820);
    o->wasHit = 0;
    o->area = m->area;
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type + 1, &Eff02_MotionRef);
    o->hitList = Eff02_Vals088[o->hitListNo];
    o->hurtList = 0;
    if (o->hitDelay != 0) {
        o->hitDelay += (s32) Math_Absf((s16) m->motionBlend[0] % 100) - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void Eff02_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 2:
    case 3:
    case 4:
    case 7:
    case 8:
    case 9:
    case 0xA:
    case 0xE:
    case 0xF:
    case 0x10:
    case 0x16:
        o->hitFrames = 99;
        break;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0 || m->taskPhase >= 2) {
        o->hitPhase = 0;
        o->state = 2;
    } else if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void Eff02_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void Eff02_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Eff02_DrawNop(ObjAttach *o)
{
}

void Eff03_Create(void *obj, s32 kind, s8 arg2)
{
    Player *p = obj;
    ObjHead *e = obj;
    s16 n;
    ObjAttach *o;
    s8 *t = &Eff03_KindParams[kind];

    if (*t == 1 || *t == 2 || (u8)Obj_InCurrentArea(p) != 0) {
        switch (kind) {
        case 0x16:
            n = 1;
            break;
        default:
            n = 0;
            break;
        }
        if ((o = ObjAttach_Alloc(n)) != NULL) {
            o->kind = 3;
            o->type = kind;
            o->stateVar1 = arg2;
            o->update = Eff03_Update;
            o->idx = e->index;
            o->isEm = e->isEm;
            o->ownerMotion = 0;
            o->owner = obj;
            o->yaw = e->rot[1];
            o->rotX = 0;
            o->rotY = e->rot[1];
            o->rotZ = 0;
            o->area = e->area;
            if (Eff03_KindOn[kind] != 0) {
                o->side = e->side;
            }
            switch (*t) {
            case 1:
            case 2:
                o->pos[0] = e->pos[0];
                o->pos[1] = 21.0f + e->pos[1];
                o->pos[2] = e->pos[2];
                o->keep = 1;
                break;
            default:
                o->pos[0] = ((f32 *) p->partRecs[18])[28];
                o->pos[1] = ((f32 *) p->partRecs[18])[29];
                o->pos[2] = ((f32 *) p->partRecs[18])[30];
                o->keep = 0;
                break;
            }
            if (o->type == 0x16 && gScene.area == o->area) {
                Sound_PlaySeAtPosChecked(1, 0x91, 0, o->pos, 1, 0);
            }
            o->velX = 0;
            o->velY = 0;
            o->velZ = 0;
            o->homingTurn = 0;
            o->gravity = 0;
            o->homingSpeed = 0;
            o->randBits = e->stateRand;
        }
    }
}

void Eff03_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff03_Setup(o);
        break;
    case 1:
        Eff03_Run(o);
        break;
    case 2:
        Eff03_End(o);
        break;
    case 3:
        Eff03_EndImpact(o);
        break;
    case 4:
        Eff03_End(o);
        break;
    case 5:
        Eff03_Hit(o);
        break;
    case 9:
        Eff03_Hit(o);
        break;
    case 7:
        Eff03_End(o);
        break;
    case 6:
        Eff03_Free(o);
        
    default:
        break;
    }
}

void Eff03_Setup(ObjAttach *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    switch (Eff03_KindParams[o->type]) {
    case 1:
        Eff03_SetupType1(o);
        break;
    case 2:
        Eff03_SetupType2(o);
        break;
    default:
        Eff03_SetupDefault(o);
        break;
    }
    Vec_Copy(o->pos2, o->pos);
    if (o->keep == 1) {
        o->drawSlot = DrawSlotB_Alloc();
        if (o->drawSlot != -1) {
            o->drawSlotBuf = DrawSlotB_Get(o->drawSlot);
        }
    } else {
        o->drawSlot = DrawSlot_Alloc();
        if (o->drawSlot != -1) {
            o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        }
    }
    if (o->drawSlot != -1) {
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = Eff03_Draw;
        Eff03_Run(o);
    } else {
        ObjAttach_Free(o);
    }
}

void Eff03_SetupDefault(ObjAttach *o)
{
    Player *owner = o->owner;
    f32 (*tbl)[2];
    f32 *pz;
    f32 in[3];
    f32 out[3];
    f32 f;

    switch (o->type) {
    case 0x12:
        Player_TestSetFlags(o, 0x400);
        break;
    case 8:
        Player_TestSetFlags(o, 0x10);
        break;
    default:
        Player_TestSetFlags(o, 0);
        break;
    }
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) owner, Eff03_Motions[o->type], &Eff03_MotionRef);
    o->hitList = Eff03_Vals088[o->hitListNo];
    o->hurtList = 0;
    if (Eff03_NoDefault[o->type] == 0) {
        o->hitPhase = 0;
    }
    switch (o->type) {
    default:
        if (owner->motionId[0] == 0x1A1) {
            tbl = Eff03_DefaultTblA;
        } else {
            tbl = Eff03_DefaultTblB;
        }
        o->gravity = -0.8f;
        break;
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
    case 0x15:
        tbl = Eff03_DefaultTblC;
        o->gravity = -0.04f;
        break;
    }
    in[0] = 0.0f;
    in[1] = 0.0f;
    pz = &in[2];
    *pz = tbl[o->stateVar1][0];
    o->stateVar0 = 0;
    if (Player_HasSkill(owner, 0x2D) == 1) {
        *pz *= 1.5f;
        o->stateVar0 |= 2;
    }
    if (Player_HasSkill(owner, 0x47) == 1) {
        o->stateVar0 |= 4;
        if (o->elementPower > 0) {
            f = 1.125f * o->elementPower;
            if (f > 255.0f) {
                f = 255.0f;
            }
            o->elementPower = f;
        }
    }
    Vec_Rotate((f32 *) out, (f32 *) in, (f32 *) owner->worldMtx);
    o->velX = 2.0f * out[0];
    o->velY = 2.0f * tbl[o->stateVar1][1];
    o->velZ = 2.0f * out[2];
    o->homingTurn = 0;
    o->homingSpeed = 0;
    o->stateVar1 = 0;
}

void Eff03_InitParts(ObjAttach *o)
{
    AttachPart *e = (AttachPart *) o->buf;
    s16 i;

    for (i = 0; i < 2; i++, e++) {
        e->idx = i;
        e->unk08 = 0;
        e->unk0A = 0;
        e->slot = DrawSlotB_Alloc();
        e->unk0C = 0;
        if (e->slot != -1) {
            e->node = DrawSlotB_Get(e->slot);
            e->node->owner = o;
            e->node->part = i;
            e->node->draw = Eff03_DrawPart;
            Vec_Copy(e->node->pos, o->pos);
            switch (e->idx) {
            case 0:
                e->node->pos[1] += 5.0f;
                break;
            case 1:
                e->node->pos[1] += 1060.0f;
                break;
            }
        } else {
            e->node = NULL;
        }
    }
}

void Eff03_SetupType1(ObjAttach *o)
{
    void *owner = o->owner;

    Player_TestSetFlags(o, 0);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) owner, Eff03_Motions[o->type], &Eff03_MotionRef);
    o->hitList = 0;
    o->hurtList = 0;
    o->hitPhase = 0;
    if (Player_HasSkill(owner, 0x48) == 1) {
        o->stateVar0 = 1;
    } else {
        o->stateVar0 = 0;
    }
    o->stateVar1 = 0;
    o->aux = 0x4B;
    switch (o->type) {
    case 0x16:
        Eff03_InitParts(o);
        break;
    default:
        if (o->area == gScene.area) {
            StageObj_K10_CreateAt(o->pos, 0xD, 0x4B, 0xA, 1.0f);
        }
        break;
    }
}

void Eff03_SetupType2(ObjAttach *o)
{
    s32 p;

    p = (s32) o->owner;
    Player_TestSetFlags(o, 0);
    MoveObj_StartMotionOwned((struct MoveObj *)o, p, Eff03_Motions[o->type], &Eff03_MotionRef);
    o->hitList = Eff03_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->hitPhase = 0;
    if (Player_HasSkill((Player *) p, 0x48) == 1) {
        o->stateVar0 = 1;
    } else {
        o->stateVar0 = 0;
    }
    o->stateVar1 = 0;
    o->aux = 0;
}

void Eff03_Run(ObjAttach *o)
{

    Vec_Copy(o->pos2, o->pos);
    if (o->stateVar1 == 0xFF) {
        Eff03_Fall(o);
    } else if (o->stateVar1 == 0xFE) {
        if (Eff03_BounceStep(o) & 0xFF) {
            o->state = 2;
            return;
        }
    } else {
        switch (Eff03_KindParams[o->type]) {
        default:
            Eff03_RunDefault(o);
            break;
        case 1:
            Eff03_RunType1(o);
            break;
        case 2:
            Eff03_RunType2(o);
            break;
        }
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void Eff03_RunDefault(ObjAttach *o)
{
    Player *owner = o->owner;
    f32 y;
    s16 lim;

    MoveObj_Integrate((struct MoveObj *)o);
    if (o->type == 1) {
        o->hitFrames = 0;
    } else {
        o->hitFrames = 0x63;
    }
    y = AreaGrid2_GetFloorYFiltered(o->pos);
    o->ownerMotion++;
    switch (o->type) {
    case 1:
        lim = (o->stateVar0 & 2) ? 8 : 13;
        if (lim <= o->ownerMotion) {
            if (o->side != 0) {
                Eff09_Create(o->owner, o->pos, 0x1B);
            }
            if ((u8)Obj_InCurrentArea(owner) != 0) {
                StageObj_K0E_CreateOwned(o->pos, 4, 1.0f, owner);
                Sound_PlaySeAtPosChecked(1, 0x47, 0, o->pos, 0xF, 0);
            }
            o->state = 2;
            return;
        }
        break;
    case 9:
        if (o->ownerMotion >= 16) {
            Eff09_Create(o->owner, o->pos, 0xD);
            if (o->area == gScene.area) {
                StageObj_K0F_CreateAt(o->pos, 4, 0, 0.5f);
            }
            o->state = 2;
            return;
        }
        break;
    case 12:
    case 13:
    case 14:
    case 15:
    case 21:
        switch (o->stateVar1) {
        case 0:
            if (o->ownerMotion > 15) {
                o->stateVar1++;
                o->gravity -= 1.0f;
            }
            break;
        case 1:
            break;
        }
        break;
    }
    if (o->pos[1] <= y) {
        if (o->pos[1] > y - 100.0f) {
            o->pos[1] = y;
        }
        switch (o->type) {
        case 0:
            StageObj_K0C_CreateFromAttach(o, 1, 0);
            break;
        case 5:
            StageObj_K0C_CreateFromAttach(o, 1, 1);
            break;
        case 7:
            if (o->area == gScene.area) {
                StageObj_K0C_Create(o->pos, 0, 5, 2);
            }
            break;
        case 11:
            if (o->area == gScene.area) {
                StageObj_K0C_Create(o->pos, 0, 5, 3);
                Eff09_Create(o->owner, o->pos, 0x13);
            }
            break;
        case 17:
            if (o->area == gScene.area) {
                StageObj_K0C_Create(o->pos, 0, 5, 4);
                Eff09_Create(o->owner, o->pos, 0x1A);
            }
            break;
        case 23:
            if (o->area == gScene.area) {
                StageObj_K0C_Create(o->pos, 0, 5, 5);
                Eff09_Create(o->owner, o->pos, 0x24);
            }
            break;
        }
        o->hitPhase = 0;
        o->visible = 0;
        o->state = 2;
    }
}

void Eff03_AnimSprites(ObjAttach *o)
{
    AttachSprite *e = (AttachSprite *) (o)->buf;
    s16 i;

    for (i = 0; i < 2; i++, e++) {
        e->frame++;
        e->time++;
        switch (e->kind) {
        case 0:
            if (e->frame > 4) {
                e->frame = 1;
            }
            if (e->time > 16) {
                e->frame = 1;
            }
            break;
        case 1:
            if (e->frame > 4) {
                e->frame = 1;
            }
            if (e->time > 0x41) {
                e->time = 0x41;
            }
            break;
        }
        if (e->frame > 0 && e->time > 0 && e->draw != NULL) {
            OT_AddByDepth(gDrawOtSorted, e->draw, 0x40, 0);
        }
    }
}

void Eff03_RunType1(ObjAttach *o)
{
    s16 n;

    if (++o->ownerMotion >= 0x4B) {
        if (o->area == gScene.area) {
            switch (o->type) {
            case 0x10:
                o->visible ^= 1;
                StageObj_K0E_CreateScaled(o->pos, 0, 1.5f);
                Eff09_CreateAt(o->pos, 0x14, o->area, o->stateVar0);
                break;
            case 0x16:
                StageObj_K29_CreateAtDefault(o->pos, 0xD, 1.5f);
                StageObj_K29_CreateAtDefault(o->pos, 0xE, 1.5f);
                Eff09_CreateAt(o->pos, 0x21, o->area, 0);
                Sound_PlaySeAtPosChecked(1, 0x93, 0, o->pos, 1, 0);
                break;
            default:
                o->visible ^= 1;
                StageObj_K0E_CreateScaled(o->pos, 0, 1.0f);
                Eff09_CreateAt(o->pos, 0, o->area, o->stateVar0);
                break;
            }
        }
        o->state = 2;
        return;
    }
    switch (o->type) {
    default:
    case 4:
    case 0x10:
        n = o->ownerMotion;
        if (n >= 0x37) {
            o->visible ^= 1;
            break;
        }
        if (o->stateVar1 == 0) {
            if (0.8f * o->aux > (f32) (0x37 - n)) {
                o->visible ^= 1;
                if (0x37 - o->ownerMotion > 2) {
                    o->stateVar1 = 2;
                }
            }
        } else if (--o->stateVar1 == 0) {
            o->visible ^= 1;
            o->aux = 0x37 - o->ownerMotion;
        }
        if (o->area == gScene.area) {
            if (o->ownerMotion == 1) {
                Sound_PlaySeAtPosChecked(1, 0x24, 0, o->pos, 1, 0);
            }
            n = o->ownerMotion;
            if (n != 0 && n % 15 == 0) {
                Sound_PlaySeAtPosChecked(1, 0x25, 0, o->pos, 1, 0);
            }
            if (o->ownerMotion & 1) {
                Effect02_SpawnRandomAngle(0xC000, 0, 2, o->pos);
            }
        }
        break;
    case 0x16:
        if (o->ownerMotion == 0x3E && gScene.area == o->area) {
            Sound_PlaySeAtPosChecked(1, 0x92, 0, o->pos, 1, 0);
        }
        Eff03_AnimSprites(o);
        break;
    }
}

void Eff03_RunType2(ObjAttach *o)
{
    f32 v[3];
    u16 a;
    u16 b;
    s16 n;

    o->ownerMotion++;
    MoveObj_Integrate((struct MoveObj *)o);
    switch (o->stateVar1) {
    case 0:
        if (o->ownerMotion >= 0x1A) {
            o->stateVar1++;
            o->ownerMotion = 0;
            o->velY = 50.0f;
            o->gravity = -0.2f;
            if (o->type == 0x18) {
                o->velZ = 30.0f;
                Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
            }
            if (o->area == gScene.area) {
                v[0] = o->pos[0];
                v[1] = o->pos[1] - 20.0f;
                v[2] = o->pos[2];
                Effect0D_SpawnAt(v, 7, 0.5f);
            }
        }
        break;
    case 1:
        if (o->ownerMotion >= 7) {
            o->stateVar1++;
            o->hitPhase = 1;
        }
    case 2:
        o->rotX += 0x2000;
        o->rotY += 0x2000;
        if (o->area == gScene.area) {
            n = o->ownerMotion;
            if (n != 0 && n % 15 == 0) {
                Sound_PlaySeAtPosChecked(1, 0x25, 0, o->pos, 1, 0);
            }
            if (o->ownerMotion & 1) {
                Vec_DirToAngles(&o->velX, (s16 *) &a, (s16 *) &b);
                Effect02_SpawnRandomAngle(a + 0x8000, b, 2, o->pos);
            }
        }
        if (o->ownerMotion >= 0x12) {
            if (o->area == gScene.area) {
                StageObj_K0E_CreateScaled(o->pos, 0, 1.0f);
                Eff09_CreateAt(o->pos, 0, o->area, o->stateVar0);
            }
            o->state = 2;
        }
        break;
    }
    if (o->ownerMotion == 1 && o->area == gScene.area) {
        Sound_PlaySeAtPosChecked(1, 0x24, 0, o->pos, 1, 0);
    }
}

void Eff03_EndImpact(ObjAttach *o)
{

    switch (o->type) {
    case 20:
        if (o->area == gScene.area) {
            StageObj_K0E_CreateScaled(o->pos, 0, 1.0f);
            Eff09_CreateAt(o->pos, 0, o->area, o->stateVar0);
        }
        break;
    case 24:
        if (o->area == gScene.area) {
            StageObj_K0E_CreateScaled(o->pos, 0, 1.0f);
            Eff09_CreateAt(o->pos, 0, o->area, o->stateVar0);
        }
        break;
    }
    Eff03_End(o);
}

void Eff03_Hit(ObjAttach *o)
{
    Em *owner = o->owner;
    Em *t = o->targetEm;
    f32 v[3];
    f32 d[3];
    u16 a;
    u16 b;

    o->step = 0;
    o->stateVar0 = 0;
    o->ownerMotion = 0;
    o->side = 0;
    if (func_364AE0(o, o->randBits) == 0) {
        o->stateVar1 = 0xFE;
        o->hitPhase = 0;
        Vec_Diff(d, o->pos2, o->pos);
        Vec_DirToAngles(d, (s16 *) &a, (s16 *) &b);
        a += -(Rand_Next(1) & 0x7FF) - 0x200;
        b += (Rand_Next(1) & 0x1FFF) - 0x1000;
        o->velX = 0.0f;
        o->velY = Rand_Next(1) & 7;
        o->velZ = 7.0f + (Rand_Next(1) & 3);
        o->homingTurn = 0.0f;
        o->gravity = -1.5f - (Rand_Next(1) & 0x1F) / 32.0f;
        o->homingSpeed = 0.0f;
        Vec_RotateX(&o->velX, (6.2831855f * a) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * b) / 65536.0f);
    } else {
        o->stateVar1 = 0xFF;
        o->idx = t->index;
        o->isEm = t->isEm;
        o->owner = t;
        Player_TestSetFlags(o, 0);
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) t, 0xC, &Eff03_MotionRef);
        o->hitList = Eff03_Vals088[o->hitListNo];
        o->hurtList = 0;
        Vec_Copy(v, &owner->posX);
        v[1] += 150.0f;
        Vec_Diff(d, v, o->pos);
        Vec_DirToAngles(d, (s16 *) &a, (s16 *) &b);
        o->rotX = a;
        o->rotY = b;
        o->yaw = o->rotY;
        o->velX = 0.0f;
        o->velY = 0.0f;
        o->velZ = 70.0f;
        Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        o->homingTurn = 0.0f;
        o->gravity = -0.4f;
        o->homingSpeed = 0.0f;
        Vec_Copy(o->pos2, o->pos);
        if (gScene.area == o->area) {
            Sound_PlaySeAtPosChecked(1, 0x96, 0, o->pos, 6, 0);
        }
    }
    o->state = 1;
    Eff03_Run(o);
}

void Eff03_Fall(ObjAttach *o)
{
    o->hitFrames = 99;
    if (++o->ownerMotion < 0) {
        MoveObj_AddVelocity((struct MoveObj *)o);
    } else {
        MoveObj_Integrate((struct MoveObj *)o);
    }
    if (o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
        o->hitFrames = 0;
        o->state = 2;
    }
}

s32 Eff03_BounceStep(ObjAttach *o)
{
    s16 t;

    o->hitPhase = 0;
    t = o->ownerMotion + 1;
    o->ownerMotion = t;
    if (t > 30) {
        return 1;
    }
    MoveObj_Integrate((struct MoveObj *)o);
    o->rotX = (s32) (o->rotX - 0x400);
    o->rotY = (s32) (o->rotY - 0x400);
    return 0;
}

void Eff03_FreeDrawSlots(ObjAttach *o)
{
    u8 *p = o->buf;
    s16 i;

    for (i = 0; i < 2; i++) {
        if (*(s32 *) (p + 4) != 0) {
            DrawSlotB_Free(*(s16 *) p);
        }
        p += 0x10;
    }
}

void Eff03_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        if (o->keep == 1) {
            DrawSlotB_Free((s32) o->drawSlot);
        } else {
            DrawSlot_Free((s32) o->drawSlot);
        }
    }
    switch (o->type) {
    case 0x16:
        Eff03_FreeDrawSlots(o);
        break;
    }
}

void Eff03_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Eff03_DrawPart(DrawSlot *n)
{
    f32 m[16];
    f32 t[16];
    f32 scale[3];
    f32 rot[3];
    u32 col;
    ObjAttach *o = n->owner;
    AttachPart *e = (AttachPart *) o->buf + n->part;
    ModelMat *tex;
    Model *stage = gCommonModels[0];
    ModelMatSet *mdl;
    u8 k;

    if (gScene.area == o->area && o->visible != 0 && e->unk08 > 0) {
        tex = stage->mats;
        switch (e->idx) {
        case 0:
            Key_LerpVec2(Eff03_ScaleKeysA, scale, e->unk08);
            Color_EvalKeys((s32 *) Eff03_ColorKeysA, e->unk0A, &col);
            k = 1;
            break;
        case 1:
            Key_LerpVec2(Eff03_ScaleKeysB, scale, e->unk08);
            Color_EvalKeys((s32 *) Eff03_ColorKeysB, e->unk0A, &col);
            Mtx_Translation(t, 0.25f * (e->unk08 & 1), 0.0625f * ((e->unk08 >> 1) & 1), 0.0f);
            Gs_SetState(0x19, (s32) t);
            k = 2;
            break;
        default:
            return;
        }
        Mtx_FromScaleRotTrans(scale, rot, n->pos, 0, m);
        mdl = &stage->matSets[Eff03_PartModels[e->idx]];
        switch (k) {
        case 0:
            break;
        case 1:
            Vec_RotateSelf(m, gCamMatrix);
            break;
        case 2:
            Vec_RotateSelf(m, gCamYawMtx);
            break;
        }
        Gs_SetState(0x6C, 0);
        Effect_DrawModel(mdl, m, col, 2, tex);
        Gs_SetState(0x6C, 1);
    }
}

void Eff03_Draw(DrawSlot *n)
{
    f32 m[16];
    Model *stage = gCommonModels[0];
    ObjAttach *o = n->owner;
    ModelMat *mats;
    ModelMatSet *mdl;
    ModelMat *mat;
    s32 i;

    if (o->area == gScene.area && stage != NULL && stage->active != 0) {
        mats = stage->mats;
        Mtx_Translation(m, n->pos[0], n->pos[1], n->pos[2]);
        switch (Eff03_KindParams[o->type]) {
        case 0:
            switch (o->type) {
            case 0xC:
            case 0xD:
            case 0xE:
            case 0xF:
            case 0x15:
                Mtx_RotateXYZ(m, 0.0f, (6.2831855f * (u16) o->yaw) / 65536.0f, 0.0f);
                break;
            default:
                Mtx_RotateXYZ(m, (6.2831855f * ((o->ownerMotion << 11) & 0xFFFF)) / 65536.0f, 0.0f, 0.0f);
                break;
            }
            break;
        case 1:
            Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
            break;
        case 2:
            Mtx_RotateX(m, (6.2831855f * o->rotX) / 65536.0f);
            Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
            break;
        }
        mdl = stage->matSets + Eff03_Models[o->type];
        Gs_SetState(0x1A, (s32) m);
        if (mdl != NULL && mdl->id != -1) {
            for (i = 0; i < mdl->count; i++) {
                mat = &mats[mdl->mat[i]];
                if (i == 1 && Eff03_Models[o->type] == 0x37) {
                    mat->alpha = 0.0f;
                }
                mat->rgb[0] = 1.0f;
                mat->rgb[1] = 1.0f;
                mat->rgb[2] = 1.0f;
                Gs_SetState((u8) (i + 0x3A), (s32) mat);
            }
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void Eff04_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 4;
            o->type = kind;
            o->update = Eff04_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void Eff04_CreateOnObj(StageObj *s, s8 kind)
{
    ObjAttach *o;
    Em *em;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        em = s->owner;
        o->kind = 4;
        o->type = kind;
        o->update = Eff04_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->ownerMotion = em->motionId[0];
        o->owner = em;
        o->yaw = em->rotY;
        o->rotX = em->rotX;
        o->rotY = em->rotY;
        o->rotZ = em->rotZ;
        o->pos[0] = s->pos[0];
        o->pos[1] = s->pos[1];
        o->pos[2] = s->pos[2];
        em->hitPending = 0;
        em->attackHit800 = 0;
        o->step = 1;
    }
}

void Eff04_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff04_Setup(o);
        break;
    case 1:
        Eff04_Run(o);
        break;
    case 2:
        Eff04_End(o);
        break;
    case 3:
        Eff04_Run(o);
        break;
    case 4:
        Eff04_End(o);
        break;
    case 5:
        Eff04_End(o);
        break;
    case 9:
        Eff04_End(o);
        break;
    case 7:
        Eff04_End(o);
        break;
    case 6:
        Eff04_Free(o);
        
    default:
        break;
    }
}

void Eff04_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;
    s32 b;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &Eff04_MotionRef);
    o->hitList = Eff04_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            a = Math_Absf((s16) m->motionBlend[1] % 100);
            b = Math_Absf(m->motionFrame[1]);
        } else {
            a = Math_Absf((s16) m->motionBlend[0] % 100);
            b = Math_Absf(m->motionFrame[0]);
        }
        o->hitDelay += a - b - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void Eff04_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 2:
        o->hitFrames = 0x63;
        break;
    }
    if (o->type == 0x1E) {
        switch (o->stateVar0) {
        case 0:
            if (m->motionId[0] == 0x40C) {
                o->stateVar0++;
                o->hitFrames = 0x3C;
            } else {
                o->hitFrames = 0x63;
            }
            break;
        case 1:
            if (m->motionId[0] != 0x40C || o->hitPhase == 0) {
                o->hitPhase = 0;
                o->state = 2;
            }
            break;
        }
        if (m->stateGroup != 3 || ((u32) (m->state - 0xC) > 1 && m->state != 0x14) || m->stateGroup == 4 || m->stateGroup == 5) {
            o->hitPhase = 0;
            o->state = 2;
        }
    } else if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20, 0);
    }
}

void Eff04_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void Eff04_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Eff05_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 5;
            o->type = kind;
            o->update = Eff05_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
            o->step = 0;
            o->hitSpan = 0;
        }
    }
}

void Eff05_CreateEx(Em *em, s32 kind, s32 arg2)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 5;
            o->type = kind;
            o->update = Eff05_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
            o->step = 0;
            o->hitSpan = arg2;
        }
    }
}

void Eff05_CreateAt(Em *em, f32 *pos, s8 kind)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = 5;
        o->type = kind;
        o->update = Eff05_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->ownerMotion = em->motionId[0];
        o->owner = em;
        o->yaw = em->rotY;
        o->rotX = 0;
        o->rotY = 0;
        o->rotZ = 0;
        o->side = *(u8 *) &em->side;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        em->hitPending = 0;
        em->attackHit800 = 0;
        o->step = 1;
        o->hitSpan = 0;
    }
}

void Eff05_CreateOnObj(StageObj *s, s8 kind)
{
    ObjAttach *o;
    Em *em;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        em = s->owner;
        o->kind = 5;
        o->type = kind;
        o->update = Eff05_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->ownerMotion = em->motionId[0];
        o->owner = em;
        o->yaw = em->rotY;
        o->rotX = em->rotX;
        o->rotY = em->rotY;
        o->rotZ = em->rotZ;
        o->pos[0] = s->pos[0];
        o->pos[1] = s->pos[1];
        o->pos[2] = s->pos[2];
        em->hitPending = 0;
        em->attackHit800 = 0;
        o->step = 1;
        o->hitSpan = 0;
    }
}

void Eff05_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff05_Setup(o);
        break;
    case 1:
        Eff05_Run(o);
        break;
    case 2:
        Eff05_End(o);
        break;
    case 3:
        Eff05_End(o);
        break;
    case 4:
        Eff05_End(o);
        break;
    case 5:
        Eff05_End(o);
        break;
    case 9:
        Eff05_End(o);
        break;
    case 7:
        Eff05_End(o);
        break;
    case 6:
        Eff05_Free(o);
        
    default:
        break;
    }
}

void Eff05_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    o->state++;
    o->visible = 1;
    o->draw = Eff05_DrawNop;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &Eff05_MotionRef);
    o->hitList = Eff05_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->type == 0x47) {
        o->hitFrames = o->hitSpan / 2;
    }
    if (o->hitDelay != 0) {
        o->hitDelay += (s32) Math_Absf((s16) m->motionBlend[0] % 100) - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void Eff05_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void Eff05_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void Eff05_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Eff05_DrawNop(ObjAttach *o)
{
}

void Bowgun_FireShot(Player *p, s32 ammo, s32 arg2, f32 scale)
{
    f32 v[3];
    s32 ang[3];
    u8 k;

    if ((ammo & 0x80) == 0) {
        k = p->ammoType;
    } else {
        k = ammo & 0x7F;
    }
    if ((p->index != gScene.localPlayer || CamCtl_IsAiming(p) == 0) && (u8)Obj_InCurrentArea(p) == 1) {
        switch (k) {
        default:
            StageObj_K12_CreateOwned((s32) p, arg2, 0);
            StageObj_K12_CreateOwned((s32) p, arg2, 1);
            StageObj_K12_CreateOwned((s32) p, arg2, 2);
            break;
        case 20:
        case 21:
            StageObj_K12_CreateOnEm(p, arg2, 5, 0);
            StageObj_K12_CreateOnEm(p, arg2, 6, 0);
            break;
        case 26:
        case 27:
        case 28:
            StageObj_K12_CreateOnEm(p, arg2, 5, 3);
            StageObj_K12_CreateOnEm(p, arg2, 6, 3);
            break;
        case 22:
        case 23:
            StageObj_K12_CreateOnEm(p, arg2, 5, 1);
            StageObj_K12_CreateOnEm(p, arg2, 6, 1);
            break;
        case 24:
        case 25:
            StageObj_K12_CreateOnEm(p, arg2, 5, 2);
            StageObj_K12_CreateOnEm(p, arg2, 6, 2);
            break;
        case 29:
            StageObj_K12_CreateOnEm(p, arg2, 5, 8);
            StageObj_K12_CreateOnEm(p, arg2, 6, 8);
            break;
        case 30:
            StageObj_K12_CreateOnEm(p, arg2, 5, 6);
            StageObj_K12_CreateOnEm(p, arg2, 6, 6);
            break;
        case 31:
            StageObj_K12_CreateOnEm(p, arg2, 5, 7);
            StageObj_K12_CreateOnEm(p, arg2, 6, 7);
            break;
        case 19:
            Eff06_BoneLaunchPos(v, p, ang, arg2);
            StageObj_K12_CreateOnEm(p, arg2, 5, 9);
            StageObj_K0F_CreateNoOwner(v, 8, 1, (u16) p->rot[1], 0.25f);
            break;
        case 17:
            Eff06_BoneLaunchPos(v, p, ang, arg2);
            StageObj_K12_CreateOnEm(p, arg2, 5, 0xA);
            StageObj_K0F_CreateNoOwner(v, 9, 1, (u16) p->rot[1], 0.25f);
            break;
        case 16:
            Eff06_BoneLaunchPos(v, p, ang, arg2);
            StageObj_K12_CreateOnEm(p, arg2, 5, 0xB);
            StageObj_K0F_CreateNoOwner(v, 2, 1, (u16) p->rot[1], 0.4f);
            break;
        case 15:
            StageObj_K12_CreateOnEm(p, arg2, 5, 0xC);
            StageObj_K12_CreateOnEm(p, arg2, 6, 0xC);
            break;
        case 18:
            Eff06_BoneLaunchPos(v, p, ang, arg2);
            StageObj_K12_CreateOnEm(p, arg2, 5, 0xD);
            StageObj_K29_CreateAt(v, 0xC, 1, 0.4f);
            break;
        }
    }
    Eff06_Create(p, p->ammoType, arg2, scale);
}

void Eff06_Create(Player *p, u8 kind, s16 arg2, f32 scale)
{
    ObjAttach *o;
    Eff06Buf *b;

    if ((o = ObjAttach_Alloc(1)) != NULL) {
        b = (Eff06Buf *) o->buf;
        o->kind = 6;
        o->type = kind;
        o->aux = arg2;
        o->update = Eff06_Update;
        o->idx = p->index;
        o->isEm = p->isEm;
        o->owner = p;
        o->yaw = p->rot[1];
        o->randBits = p->stateRand;
        o->stateVar1 = 0;
        o->area = p->area;
        o->side = *(u8 *) &p->side;
        b->scale = scale;
    }
}

void Eff06_Scatter(ObjAttach *o, u8 *p, f32 f)
{
    s16 i;
    u8 kind = p[0];
    s32 m;
    s16 n = *(s16 *) (p + 2);
    ObjAttach *c;
    u8 *b;

    Eff06_PlaySe(o, 0);
    i = 0;
    if (i < n) {
        m = 1;
        do {
            if ((c = ObjAttach_Alloc(1)) != NULL) {
                b = o->buf;
                c->kind = 6;
                c->type = kind;
                c->aux = o->aux;
                c->update = Eff06_Update;
                c->idx = ((Em *) o->owner)->index;
                c->isEm = ((Em *) o->owner)->isEm;
                c->owner = o->owner;
                c->yaw = *(u16 *) &o->yaw;
                c->rotX = o->rotX;
                c->rotY = o->rotY;
                c->pos[0] = o->pos[0];
                c->pos[1] = o->pos[1];
                c->pos[2] = o->pos[2];
                c->velX = o->velX;
                c->velY = o->velY;
                c->velZ = o->velZ;
                c->randBits = o->randBits * m;
                c->stateVar1 = i;
                c->area = o->area;
                c->side = o->side;
                /* the source's buffer, not the new object's */
                *(f32 *) (b + 0x1C) = f;
            }
            i++;
            m += 2;
        } while (i < n);
    }
}

extern f32 D_0045EB04[];
extern f32 D_0045EB24[];
extern f32 D_006AEE04[];
void Eff06_Init(ObjAttach *o)
{
    f32 m[16];
    Eff06Buf *buf = (Eff06Buf *) o->buf;
    ArrowParam *e = &gArrowParams[o->type];
    Player *pl = o->owner;
    u8 *t;
    u8 fl;
    s16 v;
    f32 a;
    f32 b;
    f32 c;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->ownerMotion = 0;
    Player_TestSetFlags(o, Eff06_PlayerFlags[o->type]);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) pl, o->type + 1, &Eff06_MotionRef);
    buf->crit = 0;
    if (o->type != 0x20) {
        Eff06_InitWeapon(buf, pl);
        Eff06_InitSkills(buf, pl);
        v = Player_GetWeaponStat(pl, 8);
        if (v < 0) {
            if (o->randBits % 100 < -v) {
                buf->crit = 2;
            }
        } else if (v > o->randBits % 100) {
            buf->crit = 1;
        }
        Eff06_CalcPower(o);
        t = buf->levelRow;
        fl = buf->level;
        a = *(f32 *) (t + 4);
        b = *(f32 *) (t + 8);
        c = *(f32 *) (t + 0xC);
        if (fl & 0x10) {
            a += D_006AEE04[0];
            b += D_006AEE04[1];
            c += D_006AEE04[2];
        }
        if (fl & 0x20) {
            if (buf->unk27 == 0) {
                a += D_0045EB24[0];
                b += D_0045EB24[1];
                c += D_0045EB24[2];
            } else {
                a += D_0045EB04[0];
                b += D_0045EB04[1];
                c += D_0045EB04[2];
            }
        }
        fl = buf->skills;
        a *= e->speed;
        b *= e->speedRand;
        c *= e->gravityRand;
        if (fl & 0x10) {
            b *= 2.0f;
            c *= 2.0f;
        }
        if (fl & 0x20) {
            b *= 0.5f;
            c *= 0.5f;
        }
    } else {
        a = e->speed;
        b = e->speedRand;
        c = e->gravityRand;
    }
    buf->power = o->power;
    o->hitList = Eff06_Vals088[o->hitListNo];
    o->hurtList = 0;
    o->step = 0;
    o->stateVar0 = 0;
    buf->bounced = 0;
    buf->stage = 0;
    buf->flyStep = 0;
    buf->unk03 = 0;
    o->homingTurn = 0.0f;
    o->homingSpeed = 0.0f;
    switch (e->kind) {
    case 4:
    default:
    case 0:
    case 1:
        if (e->model == 0x62) {
            buf->bounced = 1;
            o->rotZ = Rand_Next(1);
            if (e->kind == 4) {
                o->hitPhase = 0;
            }
        }
        Eff06_BoneLaunchPos(o->pos, pl, &o->rotX, o->aux);
        o->velX = 0.0f;
        o->velY = 0.0f;
        o->velZ = a + b * Eff06_BitsToFrac(o->randBits, 0x10);
        Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        o->gravity = -(e->gravity + c * Eff06_BitsToFrac(o->randBits >> 1, 0xF));
        break;
    case 2:
        Mtx_Copy(m, (f32 *)Em_GetBone(pl, o->aux));
        o->velX = a + b * Eff06_BitsToFrac(o->randBits >> 2, 0xE);
        if ((o->randBits >> 3) & 1) {
            o->velX = -o->velX;
        }
        o->velY = 2.0f * (a + b * Eff06_BitsToFrac(o->randBits >> 4, 0xC));
        o->velZ = a + b * Eff06_BitsToFrac(o->randBits >> 5, 0xB);
        if ((o->randBits >> 6) & 1) {
            o->velZ = -o->velZ;
        }
        o->rotX = (u16) (s32) (0.5f + 65536.0f * Math_Atan2(o->velY, Math_Sqrtf(o->velX * o->velX + o->velZ * o->velZ)) / 6.2831855f);
        o->rotY = (u16) (s32) (0.5f + 65536.0f * Math_Atan2(o->velX, o->velZ) / 6.2831855f) + 0x8000;
        o->gravity = -(e->gravity + c * Eff06_BitsToFrac(o->randBits >> 7, 9));
        break;
    }
    Vec_Copy(o->pos2, o->pos);
    Vec_Copy(buf->drawPos, o->pos);
    Eff06_PlaySe(o, 1);
    if (e->life == 0xFF) {
        buf->phase = 2;
    } else {
        buf->phase = 0;
    }
    buf->unk17 = 0;
    if (e->powerTbl != -1) {
        Eff06_ApplyKind(o);
    }
}

/* unk_6BB9D0's .data (0x896ED0-0x89BF88) */
s16 HuntingHorn_NoteFrames[8] = {
#include "game/00896ED0.inc"
};
HornAttack HuntingHorn_Attacks[12] = {
#include "game/00896EE0.inc"
};
u8 HuntingHorn_SongNotes[128] = {
#include "game/00896FA0.inc"
};
s16 Gunlance_State01Motions[8][2] = {
#include "game/00897020.inc"
};
HitShape Eff00_Vals088_0[4] = {
#include "game/00897040.inc"
};
HitShape Eff00_Vals088_1[3] = {
#include "game/008970E0.inc"
};
HitShape Eff00_Vals088_2[3] = {
#include "game/00897160.inc"
};
HitShape Eff00_Vals088_3[2] = {
#include "game/008971E0.inc"
};
HitShape Eff00_Vals088_4[2] = {
#include "game/00897230.inc"
};
HitShape Eff00_Vals088_5[2] = {
#include "game/00897280.inc"
};
HitShape Eff00_Vals088_6[2] = {
#include "game/008972D0.inc"
};
HitShape Eff00_Vals088_7[2] = {
#include "game/00897320.inc"
};
HitShape Eff00_Vals088_8[2] = {
#include "game/00897370.inc"
};
HitShape Eff00_Vals088_9[2] = {
#include "game/008973C0.inc"
};
HitShape Eff00_Vals088_10[2] = {
#include "game/00897410.inc"
};
HitShape Eff00_Vals088_11[3] = {
#include "game/00897460.inc"
};
HitShape Eff00_Vals088_12[2] = {
#include "game/008974E0.inc"
};
HitShape Eff00_Vals088_13[2] = {
#include "game/00897530.inc"
};
HitShape Eff00_Vals088_14[2] = {
#include "game/00897580.inc"
};
HitShape Eff00_Vals088_15[2] = {
#include "game/008975D0.inc"
};
HitShape Eff00_Vals088_16[2] = {
#include "game/00897620.inc"
};
HitShape Eff00_Vals088_17[2] = {
#include "game/00897670.inc"
};
HitShape Eff00_Vals088_18[2] = {
#include "game/008976C0.inc"
};
HitShape Eff00_Vals088_19[2] = {
#include "game/00897710.inc"
};
HitShape *Eff00_Vals088[20] = {
    Eff00_Vals088_0, Eff00_Vals088_1, Eff00_Vals088_2, Eff00_Vals088_3,
    Eff00_Vals088_4, Eff00_Vals088_5, Eff00_Vals088_6, Eff00_Vals088_7,
    Eff00_Vals088_8, Eff00_Vals088_9, Eff00_Vals088_10, Eff00_Vals088_11,
    Eff00_Vals088_12, Eff00_Vals088_13, Eff00_Vals088_14, Eff00_Vals088_15,
    Eff00_Vals088_16, Eff00_Vals088_17, Eff00_Vals088_18, Eff00_Vals088_19,
};
u32 D_008977B0[606] = {
#include "game/008977B0.inc"
};
void *Eff00_MotionRef = (void *)D_008977B0;
HitShape Eff01_Vals088_0[2] = {
#include "game/00898130.inc"
};
HitShape Eff01_Vals088_1[3] = {
#include "game/00898180.inc"
};
HitShape Eff01_Vals088_2[3] = {
#include "game/00898200.inc"
};
HitShape Eff01_Vals088_3[3] = {
#include "game/00898280.inc"
};
HitShape Eff01_Vals088_4[4] = {
#include "game/00898300.inc"
};
HitShape Eff01_Vals088_5[2] = {
#include "game/008983A0.inc"
};
HitShape Eff01_Vals088_6[2] = {
#include "game/008983F0.inc"
};
HitShape Eff01_Vals088_7[2] = {
#include "game/00898440.inc"
};
HitShape Eff01_Vals088_8[3] = {
#include "game/00898490.inc"
};
HitShape Eff01_Vals088_9[3] = {
#include "game/00898510.inc"
};
HitShape Eff01_Vals088_10[2] = {
#include "game/00898590.inc"
};
HitShape Eff01_Vals088_11[2] = {
#include "game/008985E0.inc"
};
HitShape Eff01_Vals088_12[2] = {
#include "game/00898630.inc"
};
HitShape Eff01_Vals088_13[2] = {
#include "game/00898680.inc"
};
HitShape Eff01_Vals088_14[4] = {
#include "game/008986D0.inc"
};
HitShape Eff01_Vals088_15[2] = {
#include "game/00898770.inc"
};
HitShape Eff01_Vals088_16[6] = {
#include "game/008987C0.inc"
};
HitShape *Eff01_Vals088[20] = {
    Eff01_Vals088_0, Eff01_Vals088_1, Eff01_Vals088_2, Eff01_Vals088_3,
    Eff01_Vals088_4, Eff01_Vals088_5, Eff01_Vals088_6, Eff01_Vals088_7,
    Eff01_Vals088_8, Eff01_Vals088_9, Eff01_Vals088_10, Eff01_Vals088_11,
    Eff01_Vals088_12, Eff01_Vals088_13, Eff01_Vals088_14, Eff01_Vals088_15,
    Eff01_Vals088_16, 0, 0, 0,
};
u32 D_00898900[294] = {
#include "game/00898900.inc"
};
void *Eff01_MotionRef = (void *)D_00898900;
HitShape Eff02_Vals088_0[2] = {
#include "game/00898DA0.inc"
};
HitShape Eff02_Vals088_1[2] = {
#include "game/00898DF0.inc"
};
HitShape Eff02_Vals088_2[2] = {
#include "game/00898E40.inc"
};
HitShape Eff02_Vals088_3[2] = {
#include "game/00898E90.inc"
};
HitShape Eff02_Vals088_4[2] = {
#include "game/00898EE0.inc"
};
HitShape Eff02_Vals088_5[2] = {
#include "game/00898F30.inc"
};
HitShape Eff02_Vals088_6[2] = {
#include "game/00898F80.inc"
};
HitShape Eff02_Vals088_7[2] = {
#include "game/00898FD0.inc"
};
HitShape *Eff02_Vals088[8] = {
    Eff02_Vals088_0, Eff02_Vals088_1, Eff02_Vals088_2, Eff02_Vals088_3,
    Eff02_Vals088_4, Eff02_Vals088_5, Eff02_Vals088_6, Eff02_Vals088_7,
};
u32 D_00899040[144] = {
#include "game/00899040.inc"
};
void *Eff02_MotionRef = (void *)D_00899040;
HitShape Eff03_Vals088_0[2] = {
#include "game/00899290.inc"
};
HitShape Eff03_Vals088_1[2] = {
#include "game/008992E0.inc"
};
HitShape Eff03_Vals088_2[2] = {
#include "game/00899330.inc"
};
HitShape *Eff03_Vals088[4] = {
    Eff03_Vals088_0, Eff03_Vals088_1, Eff03_Vals088_2, 0,
};
u32 D_00899390[84] = {
#include "game/00899390.inc"
};
void *Eff03_MotionRef = (void *)D_00899390;
s8 Eff03_Motions[32] = {
#include "game/008994F0.inc"
};
s8 Eff03_NoDefault[32] = {
#include "game/00899510.inc"
};
s8 Eff03_KindOn[32] = {
#include "game/00899530.inc"
};
s8 Eff03_KindParams[32] = {
#include "game/00899550.inc"
};
s16 Eff03_Models[32] = {
#include "game/00899570.inc"
};
f32 Eff03_ScaleKeysA[12] = {
#include "game/008995B0.inc"
};
f32 Eff03_ScaleKeysB[8] = {
#include "game/008995E0.inc"
};
u8 Eff03_ColorKeysA[32] = {
#include "game/00899600.inc"
};
u8 Eff03_ColorKeysB[48] = {
#include "game/00899620.inc"
};
s16 Eff03_PartModels[8] = {
#include "game/00899650.inc"
};
f32 Eff03_DefaultTblA[4][2] = {
#include "game/00899660.inc"
};
f32 Eff03_DefaultTblB[4][2] = {
#include "game/00899680.inc"
};
f32 Eff03_DefaultTblC[4][2] = {
#include "game/008996A0.inc"
};
HitShape Eff04_Vals088_0[2] = {
#include "game/008996C0.inc"
};
HitShape Eff04_Vals088_1[2] = {
#include "game/00899710.inc"
};
HitShape Eff04_Vals088_2[5] = {
#include "game/00899760.inc"
};
HitShape Eff04_Vals088_3[2] = {
#include "game/00899830.inc"
};
HitShape Eff04_Vals088_4[2] = {
#include "game/00899880.inc"
};
HitShape Eff04_Vals088_5[2] = {
#include "game/008998D0.inc"
};
HitShape Eff04_Vals088_6[2] = {
#include "game/00899920.inc"
};
HitShape Eff04_Vals088_7[2] = {
#include "game/00899970.inc"
};
HitShape Eff04_Vals088_8[2] = {
#include "game/008999C0.inc"
};
HitShape Eff04_Vals088_9[6] = {
#include "game/00899A10.inc"
};
HitShape Eff04_Vals088_10[2] = {
#include "game/00899B00.inc"
};
HitShape Eff04_Vals088_11[3] = {
#include "game/00899B50.inc"
};
HitShape Eff04_Vals088_12[2] = {
#include "game/00899BD0.inc"
};
HitShape Eff04_Vals088_13[8] = {
#include "game/00899C20.inc"
};
HitShape Eff04_Vals088_14[3] = {
#include "game/00899D60.inc"
};
HitShape Eff04_Vals088_15[3] = {
#include "game/00899DE0.inc"
};
HitShape Eff04_Vals088_16[2] = {
#include "game/00899E60.inc"
};
HitShape Eff04_Vals088_17[2] = {
#include "game/00899EB0.inc"
};
HitShape Eff04_Vals088_18[3] = {
#include "game/00899F00.inc"
};
HitShape *Eff04_Vals088[20] = {
    Eff04_Vals088_0, Eff04_Vals088_1, Eff04_Vals088_2, Eff04_Vals088_3,
    Eff04_Vals088_4, Eff04_Vals088_5, Eff04_Vals088_6, Eff04_Vals088_7,
    Eff04_Vals088_8, Eff04_Vals088_9, Eff04_Vals088_10, Eff04_Vals088_11,
    Eff04_Vals088_12, Eff04_Vals088_13, Eff04_Vals088_14, Eff04_Vals088_15,
    Eff04_Vals088_16, Eff04_Vals088_17, Eff04_Vals088_18, 0,
};
u32 D_00899FD0[216] = {
#include "game/00899FD0.inc"
};
void *Eff04_MotionRef = (void *)D_00899FD0;
HitShape Eff05_Vals088_0[2] = {
#include "game/0089A340.inc"
};
HitShape Eff05_Vals088_1[2] = {
#include "game/0089A390.inc"
};
HitShape Eff05_Vals088_2[3] = {
#include "game/0089A3E0.inc"
};
HitShape Eff05_Vals088_3[3] = {
#include "game/0089A460.inc"
};
HitShape Eff05_Vals088_4[2] = {
#include "game/0089A4E0.inc"
};
HitShape Eff05_Vals088_5[4] = {
#include "game/0089A530.inc"
};
HitShape Eff05_Vals088_6[3] = {
#include "game/0089A5D0.inc"
};
HitShape Eff05_Vals088_7[5] = {
#include "game/0089A650.inc"
};
HitShape Eff05_Vals088_8[3] = {
#include "game/0089A720.inc"
};
HitShape Eff05_Vals088_9[4] = {
#include "game/0089A7A0.inc"
};
HitShape Eff05_Vals088_10[5] = {
#include "game/0089A840.inc"
};
HitShape Eff05_Vals088_11[3] = {
#include "game/0089A910.inc"
};
HitShape Eff05_Vals088_12[2] = {
#include "game/0089A990.inc"
};
HitShape Eff05_Vals088_13[2] = {
#include "game/0089A9E0.inc"
};
HitShape Eff05_Vals088_14[2] = {
#include "game/0089AA30.inc"
};
HitShape Eff05_Vals088_15[2] = {
#include "game/0089AA80.inc"
};
HitShape Eff05_Vals088_16[2] = {
#include "game/0089AAD0.inc"
};
HitShape Eff05_Vals088_17[2] = {
#include "game/0089AB20.inc"
};
HitShape Eff05_Vals088_18[2] = {
#include "game/0089AB70.inc"
};
HitShape Eff05_Vals088_19[2] = {
#include "game/0089ABC0.inc"
};
HitShape *Eff05_Vals088[20] = {
    Eff05_Vals088_0, Eff05_Vals088_1, Eff05_Vals088_2, Eff05_Vals088_3,
    Eff05_Vals088_4, Eff05_Vals088_5, Eff05_Vals088_6, Eff05_Vals088_7,
    Eff05_Vals088_8, Eff05_Vals088_9, Eff05_Vals088_10, Eff05_Vals088_11,
    Eff05_Vals088_12, Eff05_Vals088_13, Eff05_Vals088_14, Eff05_Vals088_15,
    Eff05_Vals088_16, Eff05_Vals088_17, Eff05_Vals088_18, Eff05_Vals088_19,
};
u32 D_0089AC60[528] = {
#include "game/0089AC60.inc"
};
void *Eff05_MotionRef = (void *)D_0089AC60;
HitShape Eff06_Vals088_0[2] = {
#include "game/0089B4B0.inc"
};
HitShape Eff06_Vals088_1[2] = {
#include "game/0089B500.inc"
};
HitShape Eff06_Vals088_2[2] = {
#include "game/0089B550.inc"
};
HitShape Eff06_Vals088_3[2] = {
#include "game/0089B5A0.inc"
};
HitShape Eff06_Vals088_4[2] = {
#include "game/0089B5F0.inc"
};
HitShape Eff06_Vals088_5[2] = {
#include "game/0089B640.inc"
};
HitShape Eff06_Vals088_6[2] = {
#include "game/0089B690.inc"
};
HitShape *Eff06_Vals088[8] = {
    Eff06_Vals088_0, Eff06_Vals088_1, Eff06_Vals088_2, Eff06_Vals088_3,
    Eff06_Vals088_4, Eff06_Vals088_5, Eff06_Vals088_6, 0,
};
u32 D_0089B700[210] = {
#include "game/0089B700.inc"
};
void *Eff06_MotionRef = (void *)D_0089B700;
ArrowParam gArrowParams[33] = {
#include "game/0089BA50.inc"
};
s32 Eff06_PlayerFlags[34] = {
#include "game/0089BF00.inc"
};
