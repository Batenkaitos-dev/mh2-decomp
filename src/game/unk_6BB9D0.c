/*
 * The drawn weapons' states, second part (the first is unk_6AF280.c, which
 * says how they work): the hunting horn (its swings, its play stances, the
 * notes and the songs), the gunlance and the bow. Then the attack objects
 * made by these states and by the monsters: families of ObjAttach
 * (include/objattach.h: enum ObjAttachKind), each with its Create, Update
 * (switching on ObjAttach.state, enum ObjAttachState), Setup, Run, End and
 * Free. An attack's type is a row of its family's Rows table (the hit's
 * delay, length, power, kind and element, copied over the object), and its
 * hit shapes a list of its HitShapes table:
 * - PlayerAttack: a player's weapon hit, made by the weapon states;
 * - RathalosAttack, FatalisAttack, LaoShanAttack: those monsters' body
 *   attacks (em_rathalos.c, em_laoshan.c, and stage objects);
 * - EmContactAttack: Kirin's, Melynx's, Anteka's and Mosswine's attacks,
 *   which report what they touch to their owner;
 * - ItemAttack: an item a player throws or puts down (enum ItemAttackMode);
 * - BowgunShot: the bowgun's shot (Bowgun_FireShot) and its pellets; its
 *   flight is in unk_6C7AE0.c. The bow's arrows are BowArrow (unk_6D3B50.c).
 */
#include "common.h"
#include "em.h"
#include "objhead.h"
#include "objattach.h"
#include "pad.h"
#include "player.h"
#include "stageobj.h"
#include "sprite.h"
#include "capsule.h"
#include "render_state.h"
#include "qsync.h"
#include "item.h"

/* prototypes */
s32 Player_HasMealEffect(Player *pl, s16 kind);
void Player_AddHp(Player *p, s16 n);
void Player_AddMaxHp(Player *p, s16 n, u8 temporary);
void QSync_SendPlayer(Player *p, s32 kind, u16 mode);
extern char *gHornSongTexts[];
s32 sprintf(char *, const char *, ...);
u8 Notice_PushText(char *s);
extern BowgunShotParam gBowgunShotParams[];
extern s32 BowgunShot_PlayerFlags[];
extern void *BowgunShot_RowsPtr;
extern HitShape *BowgunShot_HitShapes[];
void BowgunShot_InitWeapon(BowgunShotBuf *b, Player *pl);
void BowgunShot_InitSkills(BowgunShotBuf *b, Player *pl);
void BowgunShot_CalcPower(ObjAttach *o);
void BowgunShot_ApplyKind(ObjAttach *o);
f32 BowgunShot_BitsToFrac(u16 arg0, s32 arg1);
f32 Math_Atan2(f32 y, f32 x);
f32 Math_Sqrtf(f32 x);
void BowgunShot_PlaySe(ObjAttach *o, s16 kind);
extern void *PlayerAttack_RowsPtr;
extern HitShape *PlayerAttack_HitShapes[];
s16 Player_GetWeaponStat(Player *p, u8 kind);
void BowArrow_ApplyCoating(ObjAttach *o, u8 a, u8 b);
s32 CamCtl_IsAiming(Player *pl);
void StageObj_K12_CreateOwned(s32 owner, s16 v, s32 k);
void StageObj_K12_CreateOnEm(Player *em, s16 v, s32 k, u8 c);
void BowgunShot_BoneLaunchPos(f32 *out, Player *p, s32 *ang, s16 bone);
void StageObj_K0F_CreateNoOwner(f32 *pos, u8 k, u8 c, u16 v, f32 scale);
void StageObj_K29_CreateAt(f32 *src, u8 k, u8 c, f32 scale);
void BowgunShot_Create(Player *em, u8 kind, s16 arg2, f32 scale);
extern s16 Gunlance_ShellMotions[][2];
extern s16 D_00897022[][2];
void StageObj_K29_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);
s32 func_364AE0(ObjAttach *o, u16 x);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
u16 Rand_Next(s32 stream);
void Vec_RotateX(f32 *v, f32 angle);
extern s16 ItemAttack_Models[];
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Draw_ApplyStates(u32);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
extern s8 ItemAttack_HitsInFlight[];
extern f32 ItemAttack_ThrowSpeedsAlt[][2];
extern f32 ItemAttack_ThrowSpeeds[][2];
extern f32 ItemAttack_LobSpeeds[][2];
void Explosion_Create(Player *owner, f32 *pos, s32 kind);
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
void Bow_FireArrows(Player *em, s32 arg1);
u8 Pouch_Add(Player *p, u16 item, s16 n);
u16 Bowgun_NextAmmoSlot(Player *p, u16 slot, u8 dir);
void Bowgun_LoadAmmo(Player *p, s32 a);
void StageObj_K12_CreateGunlanceShell(Player *em, s16 i);
void StageObj_Gun_CreateShell(void *owner, u8 lvl);
void StageObj_Gun_CreateType4(void *owner);
void StageObj_Gun_CreateType6(void *owner);
void Player_ClearPartOverrides(Player *em);
extern f32 ItemAttack_ScaleKeysA[];
extern f32 ItemAttack_ScaleKeysB[];
extern u8 ItemAttack_ColorKeysA[];
extern u8 ItemAttack_ColorKeysB[];
extern s16 ItemAttack_PartModels[];
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
void ItemAttack_Update(ObjAttach *o);
extern s8 ItemAttack_HasSide[];
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
void ItemAttack_SetupHopping(ObjAttach *o);
void ItemAttack_SetupPlaced(ObjAttach *o);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void ItemAttack_SetupThrown(ObjAttach *o);
void ItemAttack_Draw(DrawSlot *d);
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s32 i);
void ItemAttack_DrawPart(DrawSlot *d);
/* ItemAttack's two parts (kind 0x16): its buffer's 16-byte blocks, each with
 * its own draw slot (ItemAttack_InitParts, ItemAttack_AnimParts,
 * ItemAttack_DrawPart). */
typedef struct AttachPart {
    s16 slot;       /* the draw slot (DrawSlotB_Alloc), -1: none */
    s16 idx;        /* 0 the low part, 1 the high one (+1060) */
    DrawSlot *node; /* DrawSlotB_Get(slot), NULL: none */
    s16 frame;      /* the animation frame, 1-4 (scale keys, the texture's quarter) */
    s16 time;       /* frames since the start (colour keys) */
    s16 unk0C;
    s16 unk0E;
} AttachPart;
void BowgunShot_Update(ObjAttach *o);
void PlayerAttack_Update(ObjAttach *o);
void EmContactAttack_Update(ObjAttach *o);
void LaoShanAttack_Update(ObjAttach *o);
u8 Player_InCurrentArea(void *obj);
ObjAttach *ObjAttach_Alloc(s32 n);
void RathalosAttack_Update(ObjAttach *o);
void FatalisAttack_Update(ObjAttach *o);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Obj_InCurrentArea(void *obj);
s32 Side_Match(u8 a, u8 b, u8 none);
void ItemAttack_FlyBack(ObjAttach *o);
s32 ItemAttack_BounceOff(ObjAttach *o);
extern s8 ItemAttack_Modes[];
void ItemAttack_RunThrown(ObjAttach *o);
void ItemAttack_RunPlaced(ObjAttach *o);
void ItemAttack_RunHopping(ObjAttach *o);
extern HitShape *FatalisAttack_HitShapes[];
extern void *FatalisAttack_RowsPtr;
void EmContactAttack_DrawNop(ObjAttach *o);
void LaoShanAttack_DrawNop(ObjAttach *o);
extern HitShape *EmContactAttack_HitShapes[];
extern void *EmContactAttack_RowsPtr;
extern HitShape *LaoShanAttack_HitShapes[];
extern void *LaoShanAttack_RowsPtr;
f32 Math_Absf(f32 x);
extern HitShape *RathalosAttack_HitShapes[];
extern void *RathalosAttack_RowsPtr;
void PlayerAttack_PlaceOnWeapon(ObjAttach *o, Player *p);
void HuntingHorn_CheckSongs(Player *em);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
void Explosion_CreateAt(f32 *pos, s32 kind, u16 area, u8 arg3);
void ItemAttack_InitParts(ObjAttach *o);
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
void HuntingHorn_StatePlay(Player *, s32, s32);
void HuntingHorn_StateSwingLeft(Player *, s32);
void HuntingHorn_StateSwingRight(Player *, s32);
void HuntingHorn_StateSwingDown(Player *, s32);
void HuntingHorn_StateSwingDownNext(Player *, s32);
void HuntingHorn_StateBounce(Player *, s32);
void HuntingHorn_StatePlayAttack(Player *, s32);
void HuntingHorn_StateSlam(Player *, s32);
void HuntingHorn_CheckInput(Player *, u32);
u8 HuntingHorn_PlayingNote(Player *);
void Gunlance_StateThrust(Player *, s32);
void Gunlance_StateShell(Player *, s32);
void Gunlance_StateGuardThrust(Player *, s32);
void Gunlance_StateWyvernFire(Player *, s32);
void Gunlance_StateUpperThrust(Player *, s32);
void Gunlance_StateLunge(Player *, s32);
void Gunlance_StateBounce(Player *, s32);
void Bow_StateFire(Player *, s32);
void Bow_StateCharge(Player *, u32);
void Bow_StateStab(Player *, s32);
void Bow_StateAim(Player *, s32);
void Bow_StateChargeWalk(Player *, s32);
void Bow_StateEvade(Player *, s32);
void Bow_StateLoadCoating(Player *, s32);
void PlayerAttack_Create(Player *, u8);
void PlayerAttack_CreateBowStab(Player *, s8);
void PlayerAttack_Setup(ObjAttach *o);
void PlayerAttack_Run(ObjAttach *o);
void PlayerAttack_End(ObjAttach *o);
void PlayerAttack_Free(ObjAttach *o);
void RathalosAttack_Setup(ObjAttach *o);
void RathalosAttack_Run(ObjAttach *o);
void RathalosAttack_End(ObjAttach *o);
void RathalosAttack_Free(ObjAttach *o);
void EmContactAttack_Setup(ObjAttach *o);
void EmContactAttack_Run(ObjAttach *o);
void EmContactAttack_End(ObjAttach *o);
void EmContactAttack_Free(ObjAttach *o);
void ItemAttack_Setup(ObjAttach *o);
void ItemAttack_Run(ObjAttach *o);
void ItemAttack_OnHit(ObjAttach *o);
void ItemAttack_Deflect(ObjAttach *o);
void ItemAttack_FreeDrawSlots(ObjAttach *o);
void ItemAttack_End(ObjAttach *o);
void ItemAttack_Free(ObjAttach *o);
void FatalisAttack_Setup(ObjAttach *o);
void FatalisAttack_Run(ObjAttach *o);
void FatalisAttack_End(ObjAttach *o);
void FatalisAttack_Free(ObjAttach *o);
void LaoShanAttack_Setup(ObjAttach *o);
void LaoShanAttack_Run(ObjAttach *o);
void LaoShanAttack_End(ObjAttach *o);
void LaoShanAttack_Free(ObjAttach *o);
void GunlanceBlast_Create(Player *, s8);
void StageObj_Gun_CreateReload(void *owner, u8 k);
s32 Motion_ReachedFrame(void *, s32, f32);
void CamShake_ReqA(void *obj, s32 i);
s32 Game_IsOnline(void);
s32 Quest_PhaseIsOver(s32);
void DrawSlot_Free(s32);
void DrawSlotB_Free(s32);
void Player_ReturnToIdle(Player *p, s32 action, s32 a2, s32 a3);
void MoveObj_Integrate(struct MoveObj *);
void Player_TickWeaponTimer(Player *p);
extern u8 gBladeSpecialMarks;
s32 Player_HasSkill(Player *p, u8 skill);
s32 Player_TestSetFlags(void *, s32 mask);
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
extern HitShape *ItemAttack_HitShapes[];
extern void *ItemAttack_RowsPtr;
extern s8 ItemAttack_AttackRow[];
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
extern HornAttack HuntingHorn_Attacks[];
extern s16 HuntingHorn_NoteFrames[];     /* per note: the frame of its sound effect, 0: none */
/* end of prototypes */

/*
 * The left swing. variant: 0 HORN_SWING_LEFT, 1 HORN_SWING_LEFT_LATE, 2
 * HORN_SWING_LEFT_SLOW, 3 HORN_DRAW_ATTACK, 4 HORN_BOUNCE_SWING.
 * input.pressed PAD_R1 in frames 40-60 enters HORN_STANCE_DOWN_AFTER_LEFT.
 */
void HuntingHorn_StateSwingLeft(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->weaponOut = 1;
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 50;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        switch (variant) {
        case 0:
        case 1:
        case 2:
            pl->step = 2;
            if (variant == 1) {
                Player_SetMotionPair(pl, 0x579, 0xA, 0x28);
            } else {
                Player_SetMotionPair(pl, 0x579, 4, 0);
            }
            if (variant == 2) {
                pl->motionSpeed = 0.77f;
            }
            PlayerAttack_Create(pl, 0x51);
            break;
        case 3:
            pl->step++;
            Player_SetMotionPair(pl, 0x3EE, 2, 0);
            break;
        case 4:
            pl->step++;
            Player_SetMotionPair(pl, 0x596, 4, 0);
            break;
        }
        Player_UpdateMotionTime(pl, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            if (variant == 3) {
                Player_SetMotionPair(pl, 0x579, 0, 0x1A);
                Player_UpdateMotionTime(pl, 0);
                PlayerAttack_Create(pl, 0x63);
                break;
            }
            Player_SetMotionPair(pl, 0x579, 0, 0xA);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x51);
        }
        break;
    case 2:
        if (HuntingHorn_CheckHit(pl) != 1) {
            if (pl->motion[0].playing == 0) {
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            }
            if (Player_IsLocal(pl) == 1) {
                if (Motion_InFrameRange(pl, 0, 40.0f, 60.0f) != 0 && pl->stateVar0 == 0 && (pl->input.pressed & PAD_R1)) {
                    pl->stateVar0 = 1;
                }
                if (Motion_ReachedFrame(pl, 0, 60.0f) != 0 && pl->stateVar0 != 0) {
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_STANCE_DOWN_AFTER_LEFT, PL_SET_SNAP_POS);
                    break;
                }
                if (Motion_InFrameRange(pl, 0, 80.0f, 104.0f) != 0) {
                    HuntingHorn_CheckInput(pl, 0);
                }
            }
        }
        break;
    }
}

/*
 * HORN_SWING_RIGHT: input.pressed PAD_R1 in frames 42-62 enters
 * HORN_STANCE_DOWN_AFTER_RIGHT; the combo window is frames 80-104.
 */
void HuntingHorn_StateSwingRight(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->stateVar0 = 0U;
        pl->stateVar1 = 0;
        Player_SetMotionPair(pl, 0x57A, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 0x52U);
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0x32;
        break;
    case 1:
        if (HuntingHorn_CheckHit(pl) != 1) {
            if (pl->motion[0].playing == 0) {
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            }
            if (Player_IsLocal(pl) == 1) {
                if ((Motion_InFrameRange(pl, 0, 42.0f, 62.0f) != 0) && (pl->stateVar0 == 0) && (pl->input.pressed & PAD_R1)) {
                    pl->stateVar0 = 1U;
                }
                if ((Motion_ReachedFrame(pl, 0, 62.0f) != 0) && (pl->stateVar0 != 0)) {
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_STANCE_DOWN_AFTER_RIGHT, PL_SET_SNAP_POS);
                    break;
                }
                if (Motion_InFrameRange(pl, 0, 80.0f, 104.0f) != 0) {
                    HuntingHorn_CheckInput(pl, 1U);
                }
            }
        } else {
        }
        break;
    }
}

/*
 * HORN_SWING_DOWN: input.pressed PAD_R1 in frames 32-52 enters
 * HORN_STANCE_DOWN_AFTER_DOWN; else the combo window.
 */
void HuntingHorn_StateSwingDown(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->stateVar0 = 0U;
        pl->stateVar1 = 0;
        Player_SetMotionPair(pl, 0x57C, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 0x53U);
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0x1E;
        break;
    case 1:
        if (HuntingHorn_CheckHit(pl) != 1) {
            if (pl->motion[0].playing == 0) {
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            }
            if (Player_IsLocal(pl) == 1) {
                if ((Motion_InFrameRange(pl, 0, 32.0f, 52.0f) != 0) && (pl->stateVar0 == 0) && (pl->input.pressed & PAD_R1)) {
                    pl->stateVar0 = 1U;
                }
                if (Motion_InFrameRange(pl, 0, 32.0f, 52.0f) != 0) {
                    if (pl->stateVar0 != 0) {
                        Player_RequestState(pl, PL_GROUP_WEAPON, HORN_STANCE_DOWN_AFTER_DOWN, PL_SET_SNAP_POS);
                        break;
                    }
                    HuntingHorn_CheckInput(pl, 2U);
                }
            }
        } else {
        }
        break;
    }
}

/*
 * The next down swings, after a wind-up. variant: 0 HORN_SWING_DOWN_2 (the
 * play button enters HORN_STANCE_DOWN_AFTER_DOWN), 1 HORN_SWING_DOWN_3 (only
 * the evade).
 */
void HuntingHorn_StateSwingDownNext(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetMotionPair(pl, 0x588, 4, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 30;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x57C, 0, 0xC);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x54);
        }
        break;
    case 2:
        if (HuntingHorn_CheckHit(pl) != 1) {
            if (pl->motion[0].playing == 0) {
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            }
            if (Player_IsLocal(pl) == 1) {
                if (variant == 0) {
                    if (Motion_InFrameRange(pl, 0, 32.0f, 52.0f) != 0 && pl->stateVar0 == 0 && (pl->input.pressed & PAD_R1)) {
                        pl->stateVar0 = 1;
                    }
                    if (Motion_InFrameRange(pl, 0, 32.0f, 52.0f) != 0) {
                        if (pl->stateVar0 != 0) {
                            Player_RequestState(pl, PL_GROUP_WEAPON, HORN_STANCE_DOWN_AFTER_DOWN, PL_SET_SNAP_POS);
                            break;
                        }
                        HuntingHorn_CheckInput(pl, 3);
                        break;
                    }
                } else if (Motion_InFrameRange(pl, 0, 32.0f, 52.0f) != 0) {
                    HuntingHorn_CheckInput(pl, 6);
                }
            }
        }
        break;
    }
}

/*
 * HORN_BOUNCE: a flick in frames 2-30 queues HORN_BOUNCE_SWING at frame 32.
 */
void HuntingHorn_StateBounce(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x595, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x579, 0, 0x44);
            break;
        }
        if (pl->stateVar0 != 0) {
            if (Motion_ReachedFrame(pl, 0, 32.0f) != 0) {
                Player_SetState(pl, PL_GROUP_WEAPON, HORN_BOUNCE_SWING, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
        } else {
            if (Motion_InFrameRange(pl, 0, 2.0f, 30.0f) != 0 && pl->stateVar0 == 0 && pl->itemSelect == 0 && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
                pl->stateVar0 = 1;
            }
            break;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * Raises the horn into HORN_STANCE_RIGHT. variant: 0 HORN_PLAY_START, 1
 * HORN_DRAW_PLAY_START (the draw first).
 */
void HuntingHorn_StatePlayStart(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->weaponOut = 1;
        Player_SetMoveMode(pl, 0, 1, 0);
        if (variant == 0) {
            pl->step = 2;
            Player_SetMotionPair(pl, 0x584, 2, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x5EU);
            break;
        }
        Player_SetMotionPair(pl, 0x597, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x584, 0, 0x26);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x5EU);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_WEAPON, HORN_STANCE_RIGHT, PL_SET_SNAP_POS);
        }
        break;
    }
}

/*
 * A stance attack, row `variant` of HuntingHorn_Attacks (by the stance's note
 * and the flick's direction); it ends in the stance the row names. Each
 * strike motion has its evade window.
 */
void HuntingHorn_StatePlayAttack(Player *pl, s32 variant)
{
    HornAttack *a = &HuntingHorn_Attacks[variant];
    s16 frame;

    switch (pl->step) {
    case 0:
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        Player_PlaySe(pl, 0x1E, 0, &pl->pos[0], 1, 0);
        Player_SetMotionPair(pl, a->motion, a->unk6, 0);
        if (a->unk2 == 0) {
            pl->step = 2;
            PlayerAttack_Create(pl, a->attack);
        } else {
            pl->step++;
        }
        pl->noFlinchTimer = 125;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, a->motion2, a->unkC, a->unkA);
            PlayerAttack_Create(pl, a->attack);
        }
        break;
    case 2:
        if (HuntingHorn_CheckHit(pl) != 1) {
            if (pl->motion[0].playing == 0) {
                switch (a->note) {
                case 0:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_STANCE_UP, PL_SET_NO_SYNC | PL_SET_SNAP_POS);
                    break;
                case 1:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_STANCE_RIGHT, PL_SET_NO_SYNC | PL_SET_SNAP_POS);
                    break;
                case 2:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_STANCE_LEFT, PL_SET_NO_SYNC | PL_SET_SNAP_POS);
                    break;
                case 3:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_STANCE_DOWN, PL_SET_NO_SYNC | PL_SET_SNAP_POS);
                    break;
                }
            } else {
                frame = HuntingHorn_NoteFrames[a->note];
                if (frame != 0 && Motion_PassedFrame(pl, 0, frame) != 0) {
                    if (pl->stateVar0 == 0) {
                        pl->stateVar0++;
                        Vib_StartLocal(pl, 2);
                        CamShake_ReqA(pl, 0);
                    }
                }
                if (Player_IsLocal(pl) == 1) {
                    switch (pl->motionId[0]) {
                    case 0x580:
                        if (Motion_InFrameRange(pl, 0, 112.0f, 132.0f) != 0) {
                            HuntingHorn_CheckInput(pl, 4);
                        }
                        break;
                    case 0x581:
                        if (Motion_InFrameRange(pl, 0, 128.0f, 148.0f) != 0) {
                            HuntingHorn_CheckInput(pl, 4);
                        }
                        break;
                    case 0x582:
                        if (Motion_InFrameRange(pl, 0, 108.0f, 128.0f) != 0) {
                            HuntingHorn_CheckInput(pl, 4);
                        }
                        break;
                    case 0x583:
                        if (Motion_InFrameRange(pl, 0, 92.0f, 112.0f) != 0) {
                            HuntingHorn_CheckInput(pl, 4);
                        }
                        break;
                    }
                }
            }
        }
        break;
    }
}

/*
 * Leaves the stance with a swing, then idle; attacks from frame 66. variant:
 * 0 HORN_PLAY_END, 1 HORN_PLAY_END_NOTE1, 2 HORN_PLAY_END_NOTE2 (a wind-up
 * first).
 */
void HuntingHorn_StatePlayEnd(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->stateVar0 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x585, 2, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x5FU);
            break;
        case 1:
            pl->step++;
            Player_SetMotionPair(pl, 0x586, 2, 0);
            break;
        case 2:
            pl->step++;
            Player_SetMotionPair(pl, 0x587, 2, 0);
            break;
        }
        pl->weaponOut = 1;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x585, 0, 0x1A);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x5FU);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if ((Player_IsLocal(pl) == 1) && (Motion_ReachedFrame(pl, 0, 66.0f) != 0)) {
            HuntingHorn_CheckInput(pl, 5U);
        }
        break;
    }
}

/*
 * HORN_SLAM: two hits (the second at frame 84), the camera shaken at frame
 * 86; only the evade follows.
 */
void HuntingHorn_StateSlam(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Player_SetMoveMode(pl, 0, 1, 0);
        Player_SetMotionPair(pl, 0x57B, 2, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 0x55U);
        pl->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 84.0f) != 0) {
            PlayerAttack_Create(pl, 0x56U);
        }
        if (Motion_PassedFrame(pl, 0, 86.0f) != 0) {
            Vib_StartLocal(pl, 2);
            CamShake_ReqA(pl, 2);
        }
        if ((Player_IsLocal(pl) == 1) && (Motion_InFrameRange(pl, 0, 110.0f, 154.0f) != 0)) {
            HuntingHorn_CheckInput(pl, 4U);
        }
        break;
    }
}

/*
 * Runs the hunting horn's PL_GROUP_WEAPON state: maps the state to its
 * handler and variant (8 has no case).
 */
void HuntingHorn_RunState(Player *pl)
{
    switch (pl->state) {
    case HORN_STANCE_UP_BLEND:
        HuntingHorn_StatePlay(pl, 0, 0);
        break;
    case HORN_STANCE_RIGHT_BLEND:
        HuntingHorn_StatePlay(pl, 1, 0);
        break;
    case HORN_STANCE_LEFT_BLEND:
        HuntingHorn_StatePlay(pl, 2, 0);
        break;
    case HORN_STANCE_DOWN_BLEND:
        HuntingHorn_StatePlay(pl, 3, 0);
        break;
    case HORN_STANCE_UP:
        HuntingHorn_StatePlay(pl, 0, 1);
        break;
    case HORN_STANCE_RIGHT:
        HuntingHorn_StatePlay(pl, 1, 1);
        break;
    case HORN_STANCE_LEFT:
        HuntingHorn_StatePlay(pl, 2, 1);
        break;
    case HORN_STANCE_DOWN:
        HuntingHorn_StatePlay(pl, 3, 1);
        break;
    case HORN_SWING_LEFT:
        HuntingHorn_StateSwingLeft(pl, 0);
        break;
    case HORN_SWING_RIGHT:
        HuntingHorn_StateSwingRight(pl, 0);
        break;
    case HORN_SWING_DOWN:
        HuntingHorn_StateSwingDown(pl, 0);
        break;
    case HORN_SWING_DOWN_2:
        HuntingHorn_StateSwingDownNext(pl, 0);
        break;
    case HORN_SWING_LEFT_LATE:
        HuntingHorn_StateSwingLeft(pl, 1);
        break;
    case HORN_PLAY_START:
        HuntingHorn_StatePlayStart(pl, 0);
        break;
    case HORN_NOTE0_ATTACK_UP:
        HuntingHorn_StatePlayAttack(pl, 0);
        break;
    case HORN_NOTE0_ATTACK_RIGHT:
        HuntingHorn_StatePlayAttack(pl, 1);
        break;
    case HORN_NOTE0_ATTACK_LEFT:
        HuntingHorn_StatePlayAttack(pl, 2);
        break;
    case HORN_NOTE0_ATTACK_DOWN:
        HuntingHorn_StatePlayAttack(pl, 3);
        break;
    case HORN_PLAY_END:
        HuntingHorn_StatePlayEnd(pl, 0);
        break;
    case HORN_SLAM:
        HuntingHorn_StateSlam(pl, 0);
        break;
    case HORN_SWING_LEFT_SLOW:
        HuntingHorn_StateSwingLeft(pl, 2);
        break;
    case HORN_STANCE_DOWN_AFTER_LEFT:
        HuntingHorn_StatePlay(pl, 3, 2);
        break;
    case HORN_NOTE1_ATTACK_UP:
        HuntingHorn_StatePlayAttack(pl, 4);
        break;
    case HORN_BOUNCE:
        HuntingHorn_StateBounce(pl, 0);
        break;
    case HORN_DRAW_ATTACK:
        HuntingHorn_StateSwingLeft(pl, 3);
        break;
    case HORN_PLAY_END_NOTE1:
        HuntingHorn_StatePlayEnd(pl, 1);
        break;
    case HORN_PLAY_END_NOTE2:
        HuntingHorn_StatePlayEnd(pl, 2);
        break;
    case HORN_NOTE1_ATTACK_RIGHT:
        HuntingHorn_StatePlayAttack(pl, 5);
        break;
    case HORN_NOTE1_ATTACK_LEFT:
        HuntingHorn_StatePlayAttack(pl, 6);
        break;
    case HORN_NOTE1_ATTACK_DOWN:
        HuntingHorn_StatePlayAttack(pl, 7);
        break;
    case HORN_NOTE2_ATTACK_UP:
        HuntingHorn_StatePlayAttack(pl, 8);
        break;
    case HORN_NOTE2_ATTACK_RIGHT:
        HuntingHorn_StatePlayAttack(pl, 9);
        break;
    case HORN_NOTE2_ATTACK_LEFT:
        HuntingHorn_StatePlayAttack(pl, 0xA);
        break;
    case HORN_NOTE2_ATTACK_DOWN:
        HuntingHorn_StatePlayAttack(pl, 0xB);
        break;
    case HORN_SWING_DOWN_3:
        HuntingHorn_StateSwingDownNext(pl, 1);
        break;
    case HORN_STANCE_DOWN_AFTER_RIGHT:
        HuntingHorn_StatePlay(pl, 3, 3);
        break;
    case HORN_BOUNCE_SWING:
        HuntingHorn_StateSwingLeft(pl, 4);
        break;
    case HORN_DRAW_PLAY_START:
        HuntingHorn_StatePlayStart(pl, 1);
        break;
    case HORN_STANCE_DOWN_AFTER_DOWN:
        HuntingHorn_StatePlay(pl, 3, 4);
        
    default:
        break;
    }
}

/*
 * The drawn idle's input: a flick swings right, left or down, or slams (up);
 * the roll button rolls. Returns 1 when it started a state.
 */
s32 HuntingHorn_AttackInput(Player *pl)
{
    s32 dir;

    if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
        dir = (pl->input.rightStickAngle + 0x1C72) & 0xFFFF;
        if (dir < 0x471D) {
            Player_RequestState(pl, PL_GROUP_WEAPON, HORN_SWING_RIGHT, PL_SET_SNAP_POS);
        } else if (dir < 0x8001) {
            Player_RequestState(pl, PL_GROUP_WEAPON, HORN_SLAM, PL_SET_SNAP_POS);
        } else if (dir < 0xC71D) {
            Player_RequestState(pl, PL_GROUP_WEAPON, HORN_SWING_LEFT, PL_SET_SNAP_POS);
        } else {
            Player_RequestState(pl, PL_GROUP_WEAPON, HORN_SWING_DOWN, PL_SET_SNAP_POS);
        }
        return 1;
    }
    if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B)) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
        return 1;
    }
    return 0;
}

/*
 * An attack window's input. caller names the attack: 0 the left swing, 1 the
 * right, 2 the down, 3 the second down, 4 a stance attack or the slam (the
 * evade only), 5 the play end (attacks, no evade), 6 the third down (the
 * evade only).
 */
void HuntingHorn_CheckInput(Player *pl, u32 caller)
{
    s32 dir;

    if ((Player_IsLocal(pl) != 0 || Game_IsOnline() != 1) && Quest_PhaseIsOver(1) != 1 && pl->itemSelect == 0) {
        if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B && caller != 5) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (u16) (pl->input.stickAngle + 0x4000);
                if (dir < 0x5C72) {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_RIGHT, PL_SET_SNAP_POS);
                } else if (dir < 0xA38E) {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                } else {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_LEFT, PL_SET_SNAP_POS);
                }
            } else {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            }
            return;
        }
        if (caller == 4 || caller == 6) {
            return;
        }
        if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
            dir = (u16) (pl->input.rightStickAngle + 0x1C72);
            if (dir < 0x471D) {
                if (caller == 0 || caller - 2 <= 1 || caller == 5) {
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_SWING_RIGHT, PL_SET_SNAP_POS);
                }
            } else if (dir < 0x8001) {
                if (caller <= 3 || caller == 5) {
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_SLAM, PL_SET_SNAP_POS);
                }
            } else if (dir < 0xC71D) {
                if (caller - 1 <= 2 || caller == 5) {
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_SWING_LEFT, PL_SET_SNAP_POS);
                }
            } else if (caller == 2 || caller == 5) {
                Player_RequestState(pl, PL_GROUP_WEAPON, HORN_SWING_DOWN_2, PL_SET_SNAP_POS);
            } else if (caller == 3) {
                Player_RequestState(pl, PL_GROUP_WEAPON, HORN_SWING_DOWN_3, PL_SET_SNAP_POS);
                return;
            }
        }
    }
}

/*
 * A stance's input: the evade, or a flick: the stance attack for the stance
 * and the flick's direction.
 */
void HuntingHorn_PlayInput(Player *pl, s32 stance)
{
    s32 dir;
    s32 dir2;

    if (((Player_IsLocal(pl) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1) && (pl->itemSelect == 0)) {
        if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B)) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (pl->input.stickAngle + 0x4000) & 0xFFFF;
                if (dir < 0x5C72) {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_RIGHT, PL_SET_SNAP_POS);
                } else if (dir < 0xA38E) {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                } else {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_LEFT, PL_SET_SNAP_POS);
                }
            } else {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            }
        }
        if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
            dir2 = (pl->input.rightStickAngle + 0x2000) & 0xFFFF;
            if (dir2 < 0x4001) {
                switch (stance) {
                case 0:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE1_ATTACK_RIGHT, PL_SET_SNAP_POS);
                    break;
                case 3:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE2_ATTACK_RIGHT, PL_SET_SNAP_POS);
                    break;
                default:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE0_ATTACK_RIGHT, PL_SET_SNAP_POS);
                    break;
                }
            } else if (dir2 < 0x8001) {
                switch (stance) {
                case 0:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE1_ATTACK_UP, PL_SET_SNAP_POS);
                    break;
                case 3:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE2_ATTACK_UP, PL_SET_SNAP_POS);
                    break;
                default:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE0_ATTACK_UP, PL_SET_SNAP_POS);
                    break;
                }
            } else if (dir2 < 0xC001) {
                switch (stance) {
                case 0:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE1_ATTACK_LEFT, PL_SET_SNAP_POS);
                    break;
                case 3:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE2_ATTACK_LEFT, PL_SET_SNAP_POS);
                    break;
                default:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE0_ATTACK_LEFT, PL_SET_SNAP_POS);
                    break;
                }
            } else {
                switch (stance) {
                case 0:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE1_ATTACK_DOWN, PL_SET_SNAP_POS);
                    break;
                case 3:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE2_ATTACK_DOWN, PL_SET_SNAP_POS);
                    break;
                default:
                    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_NOTE0_ATTACK_DOWN, PL_SET_SNAP_POS);
                    break;
                }
            }
        }
    }
}

/* The attack that draws on the move: HORN_DRAW_ATTACK. */
void HuntingHorn_DrawAttack(Player *pl)
{
    Player_RequestState(pl, PL_GROUP_WEAPON, HORN_DRAW_ATTACK, PL_SET_SNAP_POS);
}

/*
 * input.held PAD_R1 (Player_HoldSpecialInput): HORN_PLAY_START, or
 * HORN_DRAW_PLAY_START when sheathed.
 */
void HuntingHorn_StartPlaying(Player *pl, s32 sheathed)
{
    if (sheathed == 0) {
        Player_RequestState((void *) pl, PL_GROUP_WEAPON, HORN_PLAY_START, PL_SET_SNAP_POS);
        return;
    }
    Player_RequestState((void *) pl, PL_GROUP_WEAPON, HORN_DRAW_PLAY_START, PL_SET_SNAP_POS);
}

/*
 * Returns the current stance's note plus 1, or 0 outside the stances and
 * during their entry motions.
 */
u8 HuntingHorn_PlayingNote(Player *pl)
{
    u8 ret = 0;

    if (pl->stateGroup != PL_GROUP_WEAPON) {
        return 0;
    }
    switch (pl->state) {
    case HORN_STANCE_UP_BLEND:
    case HORN_STANCE_UP:
        ret = 2;
        break;
    case HORN_STANCE_RIGHT_BLEND:
    case HORN_STANCE_LEFT_BLEND:
    case HORN_STANCE_RIGHT:
    case HORN_STANCE_LEFT:
        ret = 1;
        break;
    case HORN_STANCE_DOWN_BLEND:
    case HORN_STANCE_DOWN:
    case HORN_STANCE_DOWN_AFTER_LEFT:
    case HORN_STANCE_DOWN_AFTER_RIGHT:
    case HORN_STANCE_DOWN_AFTER_DOWN:
        ret = 3;
        break;
    }
    switch (pl->motionId[0]) {
    case 0x584:
        if (Motion_ReachedFrame(pl, 0, 72.0f) == 0 || pl->motion[0].blend != 0) {
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

/*
 * For the local player: shows the song's notice, text entry 0 of
 * gHornSongTexts followed by entry `song`.
 */
void HuntingHorn_ShowSongMessage(Player *pl, u8 song)
{
    char buf[0x100];

    if (Player_IsLocal(pl)) {
        sprintf(buf, "%s%s", gHornSongTexts[0], gHornSongTexts[song]);
        Notice_PushText(buf);
    }
}

/* two empty strings after "%s%s" whose users were dropped */
const u8 D_008DD028[8] = {0};

/*
 * Applies a song's effect through the meal effect slots (buffs, max HP, roar
 * and stamina protection, cures, resistances) and shows its message. quiet 0
 * sends it to the others (QSync_SendPlayer kind 10); the receivers apply it
 * with quiet 1.
 */
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
        QSync_SendPlayer(p, QSYNC_PL_POS_SONG, 0);
    }
    if (Player_IsLocal(p) == 1) {
        switch (song) {
        case 0:
        case 1:
            Effect06_SpawnAtBone(p, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
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
                            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
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

/*
 * Shows the song's effect on every player of the same side in the area (and
 * pl), except those fainted or in an event.
 */
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
                    Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
                    break;
                }
            }
            i++;
            pl++;
            sc = (SceneState *) ((u8 *) sc + 1);
        } while (i < gScene.playerCount);
    }
}

/* Shows the song's effect on pl, unless fainted or in an event. */
void HuntingHorn_SongFxSelf(Player *pl)
{
    switch (pl->stateGroup) {
    case 3:
    case 4:
        break;
    default:
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
        break;
    }
}

/*
 * Matches the last four notes, newest first, against the 30 songs of
 * HuntingHorn_SongNotes (0 matches any note, 0x80 | c any note but c), and
 * plays every song that matches.
 */
void HuntingHorn_CheckSongs(Player *pl)
{
    s16 i;
    s16 steps;
    s16 matched;
    u8 last;
    u8 *note;
    u8 c;
    u8 ok;

    for (i = 0; i < 30; i++) {
        last = pl->hornNoteSlot;
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
                if ((c & 0x7F) != pl->hornNotes[last]) {
                    ok = 1;
                }
            } else {
                if ((c & 0x7F) == pl->hornNotes[last]) {
                    ok = 1;
                }
            }
            if (ok == 1) {
                matched++;
                note++;
                if (matched >= 4) {
                    pl->hornSong = i;
                    HuntingHorn_PlaySong(pl, pl->hornSong, 0);
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

/*
 * Adds a note: its colour (gBladeSpecialMarks by the horn) goes into the
 * 8-slot ring hornNotes, a sound plays, and the songs are checked.
 */
void HuntingHorn_AddNote(Player *pl, u8 note)
{

    pl->hornNoteSlot++;
    if (pl->hornNoteSlot >= 8) {
        pl->hornNoteSlot = 0;
    }
    pl->hornNotes[pl->hornNoteSlot] = (&gBladeSpecialMarks)[pl->bladeSpecial * 3 + note];
    Sound_PlaySeFlat(0, 0x22, 0);
    HuntingHorn_CheckSongs(pl);
}

/* Empties the note ring. */
void HuntingHorn_ClearNotes(Player *pl)
{
    pl->hornNoteSlot = 0;
    pl->hornNotes[0] = 0;
    pl->hornNotes[1] = 0;
    pl->hornNotes[2] = 0;
    pl->hornNotes[3] = 0;
    pl->hornNotes[4] = 0;
    pl->hornNotes[5] = 0;
    pl->hornNotes[6] = 0;
    pl->hornNotes[7] = 0;
}

/*
 * For the local player in a stance: 1 before its first note, 2 after; else 0.
 */
s32 HuntingHorn_PlayStanceKind(Player *pl)
{
    if (Player_IsLocal(pl) == 0) {
        return 0;
    }
    if (pl->stateGroup != PL_GROUP_WEAPON) {
        return 0;
    }
    switch (pl->state) {
    case HORN_STANCE_UP_BLEND:
    case HORN_STANCE_RIGHT_BLEND:
    case HORN_STANCE_LEFT_BLEND:
    case HORN_STANCE_DOWN_BLEND:
    case HORN_STANCE_UP:
    case HORN_STANCE_RIGHT:
    case HORN_STANCE_LEFT:
    case HORN_STANCE_DOWN:
    case HORN_STANCE_DOWN_AFTER_LEFT:
    case HORN_STANCE_DOWN_AFTER_RIGHT:
    case HORN_STANCE_DOWN_AFTER_DOWN:
        return (pl->stateVar0 == 0) ? 1 : 2;
    }
    return 0;
}

/*
 * HuntingHorn_PlayStanceKind, or 3 in a stance attack, else 0 (0 clears the
 * notes: Player_Update).
 */
s16 HuntingHorn_PlayKind(Player *pl)
{
    s16 r;

    r = HuntingHorn_PlayStanceKind(pl);
    if (r != 0) {
        return r;
    }
    if (pl->stateGroup != PL_GROUP_WEAPON) {
        return 0;
    }
    switch (pl->state) {
    case HORN_NOTE0_ATTACK_UP:
    case HORN_NOTE0_ATTACK_RIGHT:
    case HORN_NOTE0_ATTACK_LEFT:
    case HORN_NOTE0_ATTACK_DOWN:
    case HORN_NOTE1_ATTACK_UP:
    case HORN_NOTE1_ATTACK_RIGHT:
    case HORN_NOTE1_ATTACK_LEFT:
    case HORN_NOTE1_ATTACK_DOWN:
    case HORN_NOTE2_ATTACK_UP:
    case HORN_NOTE2_ATTACK_RIGHT:
    case HORN_NOTE2_ATTACK_LEFT:
    case HORN_NOTE2_ATTACK_DOWN:
        return 3;
    }
    return 0;
}

/*
 * Returns the colour of HuntingHorn_PlayingNote's note on this horn (the
 * HUD's note object).
 */
u8 HuntingHorn_PlayingNoteColour(Player *pl)
{
    s32 note;

    note = HuntingHorn_PlayingNote(pl) & 0xFF;
    if (note > 0) {
        note = (note - 1) & 0xFF;
    }
    return *((pl->bladeSpecial * 3) + (&gBladeSpecialMarks + (note & 0xFF)));
}

/* Returns 1 when the gunlance can shell: its sharpness above level 0. */
static s32 Gunlance_CanShell(Player *pl)
{
    switch (pl->sharpnessLevel) {
    case 0:
        return 0;
    default:
        return 1;
    }
}

/*
 * When the attack bounced off (hitKind 1): shakes the camera, clears the hit
 * stop, requests GLANCE_BOUNCE and returns 1; else 0.
 */
s32 Gunlance_CheckHit(Player *pl)
{
    if ((pl->hitPending != 0) && (pl->hitKind == 1)) {
        CamShake_ReqA(pl, 0);
        pl->hitStop = 0;
        pl->hitStopAdd = 0;
        Player_RequestState(pl, PL_GROUP_WEAPON, GLANCE_BOUNCE, 0);
        return 1;
    }
    return 0;
}

/* Requests the held guard, PL_REACT_GUARD_HOLD. */
void Gunlance_ReturnToGuard(void *pl)
{
    Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
}

/*
 * Requests PL_REACT_GUARD_HOLD_ALT, the guard after
 * GLANCE_WYVERN_FIRE_COOLING.
 */
void Gunlance_ReturnToGuardAlt(void *pl)
{
    Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD_ALT, 0);
}

/*
 * The thrusts. variant: 0 GLANCE_THRUST, 1 GLANCE_DRAW_THRUST (the draw
 * first), 2 GLANCE_THRUST_2, 3 GLANCE_THRUST_3, 4 GLANCE_BOUNCE_THRUST. In
 * the window the shell button shells and a flick chains
 * (Gunlance_CheckInput); it ends in idle.
 */
void Gunlance_StateThrust(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57D, 4, 0);
            PlayerAttack_Create(pl, 0x3F);
            break;
        case 1:
            pl->step++;
            Player_SetMotionPair(pl, 0x3F2, 2, 0);
            pl->weaponOut = 1;
            PlayerAttack_Create(pl, 0x41);
            break;
        case 2:
        case 3:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57D, 6, 0xA);
            if (variant == 3) {
                PlayerAttack_Create(pl, 0x42);
            } else {
                PlayerAttack_Create(pl, 0x3F);
            }
            break;
        case 4:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57D, 4, 0xA);
            PlayerAttack_Create(pl, 0x3F);
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x57D, 0, 0x22);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (Gunlance_CheckHit(pl) != 1 && Player_IsLocal(pl) == 1 && Motion_InFrameRange(pl, 0, 36.0f, 62.0f) != 0) {
            if (pl->input.pressed & PAD_R3) {
                if (pl->roundsLoaded == 0 || Gunlance_CanShell(pl) == 0) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_SHELL_EMPTY, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_SHELL_AFTER_THRUST, PL_SET_SNAP_POS);
                }
            } else {
                Gunlance_CheckInput(pl, 0);
            }
        }
        break;
    }
}

/*
 * Creates the shell's blast effect (GunlanceBlast), kind 1-3 by the shelling
 * type, bladeSpecial % 3.
 */
void Gunlance_ShellEffect(Player *pl)
{
    s32 kind;

    kind = (s32) pl->bladeSpecial % 3;
    switch (kind) {
    case 0:
        GunlanceBlast_Create(pl, 1);
        break;
    case 1:
        GunlanceBlast_Create(pl, 2);
        break;
    case 2:
        GunlanceBlast_Create(pl, 3);
        break;
    }
}

/* Takes the shell's sharpness cost, 3, 3 or 4 by the shelling type. */
void Gunlance_ShellSharpness(Player *pl)
{
    s32 kind;

    kind = (s32) pl->bladeSpecial % 3;
    switch (kind) {
    case 0:
        Player_AddSharpness(pl, -3);
        break;
    case 1:
        Player_AddSharpness(pl, -3);
        break;
    case 2:
        Player_AddSharpness(pl, -4);
        break;
    }
}

/*
 * A shell: a round fired, its sharpness taken, the shell objects made.
 * variant: 0 GLANCE_SHELL_AFTER_THRUST, 1 _AFTER_GUARD_THRUST, 2
 * _AFTER_UPPER, 3 GLANCE_SHELL, 4-6 the repeats (GLANCE_SHELL_REPEAT,
 * _REPEAT_GUARD, _REPEAT_UPPER); the motion and blend from
 * Gunlance_ShellMotions. The window shells again or thrusts.
 */
void Gunlance_StateShell(Player *pl, s32 variant)
{
    f32 lo;
    f32 hi;
    u16 st;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        pl->roundsLoaded--;
        Player_SetMoveMode(pl, 0, 1, 0);
        Player_SetMotionPair(pl, Gunlance_ShellMotions[variant][0], D_00897022[variant][0], 0);
        if (variant == 4) {
            pl->step = 2;
            break;
        }
        if (variant <= 2) {
        fire:
            pl->step = 2;
            Gunlance_ShellEffect(pl);
            Gunlance_ShellSharpness(pl);
            pl->timer = 0;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            switch (variant) {
            case 3:
                Player_SetMotionPair(pl, 0x579, 0, 0);
                break;
            case 4:
                Player_SetMotionPair(pl, 0x579, 0, 0);
                break;
            case 5:
                Player_SetMotionPair(pl, 0x57A, 0, 0);
                break;
            case 6:
                Player_SetMotionPair(pl, 0x57B, 0, 0);
                break;
            }
            goto fire;
        }
        break;
    case 2:
        if (variant == 4 && pl->stateVar1 == 0 && pl->motion[0].blend == 0) {
            pl->stateVar1++;
            Gunlance_ShellEffect(pl);
            Gunlance_ShellSharpness(pl);
            pl->timer = 0;
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 2.0f) != 0 && (u8)Obj_InCurrentArea(pl) == 1) {
            Vib_StartLocal(pl, 1);
            StageObj_K12_CreateGunlanceShell(pl, variant);
            switch (pl->bladeSpecial % 3) {
            case 0:
                StageObj_Gun_CreateShell(pl, 0);
                break;
            case 1:
                StageObj_Gun_CreateShell(pl, 2);
                break;
            case 2:
                StageObj_Gun_CreateShell(pl, 1);
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
        if (Player_IsLocal(pl) == 1) {
            if (Motion_InFrameRange(pl, 0, lo, hi) != 0 && (pl->input.pressed & PAD_R3)) {
                if (pl->roundsLoaded == 0 || Gunlance_CanShell(pl) == 0) {
                    Player_RequestState(pl, PL_GROUP_WEAPON, GLANCE_SHELL_EMPTY, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                } else {
                    Player_RequestState(pl, 1, st, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                }
            } else if (Motion_ReachedFrame(pl, 0, 42.0f) != 0 && (pl->input.stickDirPressed & PL_RSTICK_ANY)
                       && ((pl->input.rightStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
                switch (variant) {
                case 0:
                case 3:
                case 4:
                    Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                    break;
                case 1:
                case 5:
                    Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_GUARD_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                    break;
                case 2:
                case 6:
                    Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_GUARD_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                    break;
                }
            }
        }
        break;
    }
}

/*
 * The guard thrusts. variant: 0 GLANCE_GUARD_THRUST, 1 _2, 2 _3, 3 and 4
 * GLANCE_UPPER_TO_GUARD_THRUST and _2 (a lead-in first). It ends in the held
 * guard.
 */
void Gunlance_StateGuardThrust(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        Player_SetMoveMode(pl, 0, 1, 0);
        switch (variant) {
        default:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x588, 2, 0);
            PlayerAttack_Create(pl, 0x43);
            break;
        case 3:
        case 4:
            pl->step++;
            Player_SetMotionPair(pl, 0x58D, 4, 0);
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x588, 0, 8);
            PlayerAttack_Create(pl, 0x43);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Gunlance_ReturnToGuard(pl);
            break;
        }
        if (Gunlance_CheckHit(pl) != 1 && Player_IsLocal(pl) == 1 && Motion_InFrameRange(pl, 0, 36.0f, 50.0f) != 0) {
            if (pl->input.pressed & PAD_R3) {
                if (pl->roundsLoaded == 0 || Gunlance_CanShell(pl) == 0) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_SHELL_EMPTY, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_SHELL_AFTER_GUARD_THRUST, PL_SET_SNAP_POS);
                }
            } else {
                Gunlance_CheckInput(pl, 0);
            }
        }
        break;
    }
}

/*
 * Shows the aim in the two part-motion layers: motA when aimWeight is
 * positive, motB when negative, weighted |aimWeight|; off at 0.
 */
static void Gunlance_SetAimLayers(Player *pl, u16 motA, u16 motB)
{

    if (pl->aimWeight == 0) {
        pl->partAnimOn[0] = 0;
        pl->partAnimOn[1] = 0;
        return;
    }
    pl->partAnimOn[0] = 1;
    pl->partAnimOn[1] = 1;
    if (pl->aimWeight > 0) {
        pl->partAnimMotion[0] = motA;
        pl->partAnimMotion[1] = motA + 100;
        pl->partAnimWeight[0] = pl->aimWeight;
        pl->partAnimWeight[1] = pl->aimWeight;
    } else {
        pl->partAnimMotion[0] = motB;
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

/* Adds delta to aimWeight, kept within -100..100. */
void Gunlance_AddAimWeight(Player *pl, s32 delta)
{

    pl->aimWeight = pl->aimWeight + delta;
    if (pl->aimWeight >= 100) {
        pl->aimWeight = 100;
    }
    if (pl->aimWeight < -99) {
        pl->aimWeight = -100;
    }
}

/*
 * GLANCE_WYVERN_FIRE (likely): a 50-frame charge that the left stick aims
 * sideways, then the blast: sharpness -10 and a 0x546-frame cooldown
 * (weaponTimer).
 */
void Gunlance_StateWyvernFire(Player *pl, s32 variant)
{
    u16 h;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        Player_SetMotionPair(pl, 0x57F, 4, 0);
        pl->timer = 0;
        pl->noFlinchTimer = 2;
        break;
    case 1:
        pl->noFlinchTimer = 2;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Gunlance_SetAimLayers(pl, 0x58B, 0x58C);
            Player_SetMotionPair(pl, 0x58A, 0, 0);
            break;
        }
        if (Motion_ReachedFrame(pl, 0, 42.0f) != 0 && pl->stateVar0 == 0) {
            pl->stateVar0++;
            StageObj_Gun_CreateType4(pl);
        }
        break;
    case 2:
        pl->noFlinchTimer = 2;
        pl->timer++;
        if (pl->timer >= 50) {
            pl->step++;
            pl->rot[1] += (u16) (s32) (0.5f + 65536.0f * (-0.45f * pl->aimWeight) / 360.0f);
            pl->targetYaw = pl->rot[1];
            Player_ClearPartOverrides(pl);
            Player_SetMotionPair(pl, 0x580, 0, 0);
            pl->noFlinchTimer = 10;
            break;
        }
        if (Player_IsLocal(pl) == 1) {
            h = pl->input.stickDir;
            if (h & 0x800) {
                Gunlance_AddAimWeight(pl, -5);
            } else if (h & 0x400) {
                Gunlance_AddAimWeight(pl, 5);
            }
            Gunlance_SetAimLayers(pl, 0x58B, 0x58C);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 2.0f) != 0) {
            StageObj_K12_CreateGunlanceShell(pl, 3);
            StageObj_Gun_CreateShell(pl, 3);
            GunlanceBlast_Create(pl, 4);
            Player_AddSharpness(pl, -10);
            Vib_StartLocal(pl, 2);
            CamShake_ReqA(pl, 2);
            pl->weaponTimer = 0x546;
        }
        if (Motion_PassedFrame(pl, 0, 44.0f) != 0) {
            StageObj_Gun_CreateType6(pl);
        }
        break;
    }
}

/*
 * The upper thrusts (likely): GLANCE_UPPER_THRUST, _2 and _3 behave the same.
 * The window shells or chains.
 */
void Gunlance_StateUpperThrust(Player *pl, s32 unused)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        Player_SetMotionPair(pl, 0x57E, 4, 0xA);
        PlayerAttack_Create(pl, 0x40);
        pl->noFlinchTimer = 0x19;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (Gunlance_CheckHit(pl) != 1 && Player_IsLocal(pl) == 1 && Motion_InFrameRange(pl, 0, 54.0f, 80.0f) != 0) {
            if (pl->input.pressed & PAD_R3) {
                if (pl->roundsLoaded == 0 || Gunlance_CanShell(pl) == 0) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_SHELL_EMPTY, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_SHELL_AFTER_UPPER, PL_SET_SNAP_POS);
                }
            } else {
                Gunlance_CheckInput(pl, 0);
            }
        }
        break;
    }
}

/*
 * Fills the clip. variant: 0 GLANCE_RELOAD from the guard, 1
 * GLANCE_HOP_RELOAD after a hop (from frame 6).
 */
void Gunlance_StateReload(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 1, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x57C, 2, 0);
        } else {
            Player_SetMotionPair(pl, 0x57C, 2, 6);
        }
        pl->roundsLoaded = (u8) pl->clipSize;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 44.0f) != 0) {
            StageObj_Gun_CreateReload(pl, 3);
        }
        break;
    }
}

/*
 * The lunge, moving forward until frame 44. variant: 0 GLANCE_LUNGE, 1
 * GLANCE_DRAW_LUNGE (the draw first).
 */
void Gunlance_StateLunge(Player *pl, s32 variant)
{
    f32 in[3];
    f32 out[3];

    switch (pl->step) {
    case 0:
        pl->noFlinchTimer = 0x28;
        pl->weaponOut = 1;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x581, 4, 0xA);
            PlayerAttack_Create(pl, 0x46);
            break;
        case 1:
            pl->step++;
            Player_SetMotionPair(pl, 0x3F2, 2, 0);
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x581, 0, 0x2A);
            PlayerAttack_Create(pl, 0x46);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (Gunlance_CheckHit(pl) != 1) {
            if (pl->motionId[0] == 0x581 && Motion_ReachedFrame(pl, 0, 44.0f) == 0) {
                in[0] = 0.0f;
                in[1] = 0.0f;
                in[2] = 4.0f;
                Vec_Rotate((f32 *) out, (f32 *) in, (f32 *) pl->rotMtx);
                pl->pos[0] += out[0];
                pl->pos[2] += out[2];
            }
            if (Player_IsLocal(pl) == 1 && Motion_InFrameRange(pl, 0, 78.0f, 98.0f) != 0) {
                if (pl->input.pressed & PAD_R3) {
                    if (pl->roundsLoaded == 0 || Gunlance_CanShell(pl) == 0) {
                        Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_SHELL_EMPTY, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                    } else {
                        Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_SHELL_AFTER_GUARD_THRUST, PL_SET_SNAP_POS);
                    }
                } else {
                    Gunlance_CheckInput(pl, 0);
                }
            }
        }
        break;
    }
}

/*
 * GLANCE_BOUNCE: a flick in frames 10-40 queues GLANCE_BOUNCE_THRUST at frame
 * 42; else idle.
 */
void Gunlance_StateBounce(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x589, 0, 0);
        CamShake_ReqA(pl, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        break;
    case 1:
        if (pl->stateVar0 != 0) {
            if (Motion_ReachedFrame(pl, 0, 42.0f) != 0) {
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_BOUNCE_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        } else if (Motion_InFrameRange(pl, 0, 10.0f, 40.0f) != 0 && pl->stateVar0 == 0 && pl->itemSelect == 0 && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            pl->stateVar0 = 1;
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * The empty click. variant: 0 GLANCE_SHELL_EMPTY, then idle; 1
 * GLANCE_WYVERN_FIRE_COOLING, then the guard.
 */
void Gunlance_StateShellEmpty(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x582, 4, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (variant == 0) {
                Player_ReturnToIdle(pl, 0, 4, 0);
                break;
            }
            Gunlance_ReturnToGuardAlt(pl);
        } else {
        }
        break;
    }
}

/*
 * Runs the gunlance's PL_GROUP_WEAPON state: maps the state to its handler
 * and variant.
 */
void Gunlance_RunState(Player *pl)
{
    switch (pl->state) {
    case GLANCE_THRUST:
        Gunlance_StateThrust(pl, 0);
        break;
    case GLANCE_SHELL_AFTER_THRUST:
        Gunlance_StateShell(pl, 0);
        break;
    case GLANCE_GUARD_THRUST:
        Gunlance_StateGuardThrust(pl, 0);
        break;
    case GLANCE_WYVERN_FIRE:
        Gunlance_StateWyvernFire(pl, 0);
        break;
    case GLANCE_UPPER_THRUST:
        Gunlance_StateUpperThrust(pl, 0);
        break;
    case GLANCE_RELOAD:
        Gunlance_StateReload(pl, 0);
        break;
    case GLANCE_LUNGE:
        Gunlance_StateLunge(pl, 0);
        break;
    case GLANCE_BOUNCE:
        Gunlance_StateBounce(pl, 0);
        break;
    case GLANCE_DRAW_THRUST:
        Gunlance_StateThrust(pl, 1);
        break;
    case GLANCE_THRUST_2:
        Gunlance_StateThrust(pl, 2);
        break;
    case GLANCE_THRUST_3:
        Gunlance_StateThrust(pl, 3);
        break;
    case GLANCE_BOUNCE_THRUST:
        Gunlance_StateThrust(pl, 4);
        break;
    case GLANCE_SHELL_AFTER_GUARD_THRUST:
        Gunlance_StateShell(pl, 1);
        break;
    case GLANCE_SHELL_AFTER_UPPER:
        Gunlance_StateShell(pl, 2);
        break;
    case GLANCE_SHELL_EMPTY:
        Gunlance_StateShellEmpty(pl, 0);
        break;
    case GLANCE_GUARD_THRUST_2:
        Gunlance_StateGuardThrust(pl, 1);
        break;
    case GLANCE_GUARD_THRUST_3:
        Gunlance_StateGuardThrust(pl, 2);
        break;
    case GLANCE_UPPER_THRUST_2:
        Gunlance_StateUpperThrust(pl, 1);
        break;
    case GLANCE_UPPER_THRUST_3:
        Gunlance_StateUpperThrust(pl, 2);
        break;
    case GLANCE_WYVERN_FIRE_COOLING:
        Gunlance_StateShellEmpty(pl, 1);
        break;
    case GLANCE_DRAW_LUNGE:
        Gunlance_StateLunge(pl, 1);
        break;
    case GLANCE_UPPER_TO_GUARD_THRUST:
        Gunlance_StateGuardThrust(pl, 3);
        break;
    case GLANCE_UPPER_TO_GUARD_THRUST_2:
        Gunlance_StateGuardThrust(pl, 4);
        break;
    case GLANCE_SHELL:
        Gunlance_StateShell(pl, 3);
        break;
    case GLANCE_SHELL_REPEAT:
        Gunlance_StateShell(pl, 4);
        break;
    case GLANCE_SHELL_REPEAT_GUARD:
        Gunlance_StateShell(pl, 5);
        break;
    case GLANCE_SHELL_REPEAT_UPPER:
        Gunlance_StateShell(pl, 6);
        break;
    case GLANCE_HOP_RELOAD:
        Gunlance_StateReload(pl, 1);
        
    default:
        break;
    }
}

/*
 * The drawn idle's input: the roll button hops back, the shell button shells,
 * a flick thrusts, lunges or upper-thrusts. The tests have no else, so two
 * can set a state in one frame. Returns 0.
 */
s32 Gunlance_AttackInput(Player *pl)
{

    if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_BACK, PL_SET_SNAP_POS);
    }
    if (pl->input.pressed & PAD_R3) {
        if (pl->roundsLoaded == 0 || Gunlance_CanShell(pl) == 0) {
            Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_SHELL_EMPTY, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_SHELL, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
    }
    if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
        if (((pl->input.rightStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
            if (pl->input.stickTilt >= 0x78) {
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_LUNGE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            } else {
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        } else {
            Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_UPPER_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
    }
    return 0;
}

/*
 * An attack window's input: the roll button hops (returns 1); a flick chains
 * the thrusts by the current state (returning 0 all the same). caller 3 skips
 * the chains (seen: every caller passes 0).
 */
s32 Gunlance_CheckInput(Player *pl, s32 caller)
{
    s32 dir;
    u8 state;
    u8 state2;

    if ((Player_IsLocal(pl) == 0) && (Game_IsOnline() == 1)) {
        return 0;
    }
    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    if (pl->itemSelect != 0) {
        return 0;
    }
    if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B)) {
        if (pl->input.stickDir & PAD_DPAD) {
            dir = (pl->input.stickAngle + 0x2000) & 0xFFFF;
            if (dir < 0x6000) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_SIDE_B, PL_SET_SNAP_POS);
                return 1;
            }
            if ((dir >= 0x6000) && (dir < 0xC001)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_SIDE_A, PL_SET_SNAP_POS);
                return 1;
            }
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_BACK_LONG, PL_SET_SNAP_POS);
            return 1;
        }
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_BACK, PL_SET_SNAP_POS);
        return 1;
    }
    if ((pl->input.stickDirPressed & PL_RSTICK_ANY) && (caller != 3)) {
        if (((pl->input.rightStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
            state = pl->state;
            switch (state) {
            case GLANCE_GUARD_THRUST:
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_GUARD_THRUST_2, PL_SET_SNAP_POS);
                break;
            case GLANCE_UPPER_THRUST:
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_UPPER_TO_GUARD_THRUST, PL_SET_SNAP_POS);
                break;
            case GLANCE_GUARD_THRUST_2:
            case GLANCE_UPPER_TO_GUARD_THRUST:
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_GUARD_THRUST_3, PL_SET_SNAP_POS);
                break;
            case GLANCE_UPPER_THRUST_2:
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_UPPER_TO_GUARD_THRUST_2, PL_SET_SNAP_POS);
                break;
            case GLANCE_THRUST:
            case GLANCE_DRAW_THRUST:
            case GLANCE_BOUNCE_THRUST:
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_THRUST_2, PL_SET_SNAP_POS);
                break;
            case GLANCE_THRUST_2:
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_THRUST_3, PL_SET_SNAP_POS);
                break;
            case GLANCE_LUNGE:
            case GLANCE_DRAW_LUNGE:
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_THRUST_2, PL_SET_SNAP_POS);
                break;
            }
        } else {
            state2 = pl->state;
            switch (state2) {
            case GLANCE_THRUST:
            case GLANCE_DRAW_THRUST:
            case GLANCE_BOUNCE_THRUST:
            case GLANCE_GUARD_THRUST:
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_UPPER_THRUST_2, PL_SET_SNAP_POS);
                break;
            case GLANCE_GUARD_THRUST_2:
            case GLANCE_UPPER_TO_GUARD_THRUST:
            case GLANCE_THRUST_2:
                Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_UPPER_THRUST_3, PL_SET_SNAP_POS);
                break;
            }
        }
    }
    return 0;
}

/*
 * The guard's input (Player_GuardInput): the shell button is the wyvern fire
 * (GLANCE_WYVERN_FIRE_COOLING during its cooldown), a flick up the guard
 * thrust, down the reload when the clip isn't full. Returns 1 when it started
 * a state.
 */
s32 Gunlance_GuardInput(Player *pl, s32 unused)
{
    u16 btn;

    if (pl->input.pressed & PAD_R3) {
        if (pl->weaponTimer == 0) {
            Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_WYVERN_FIRE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_WYVERN_FIRE_COOLING, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
        return 1;
    }
    btn = pl->input.stickDirPressed;
    if (btn & 0x20) {
        Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_GUARD_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        return 1;
    }
    if ((btn & 0x10) && ((s32) pl->roundsLoaded < (s32) pl->clipSize)) {
        Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_RELOAD, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        return 1;
    }
    return 0;
}

/* The attack that draws on the move: GLANCE_DRAW_LUNGE. */
void Gunlance_DrawAttack(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, GLANCE_DRAW_LUNGE, PL_SET_SNAP_POS);
}

/*
 * Returns 1 when the bow's aim arc shows: in the aim states while the aim
 * button is held, in the charge states once the charge motion passes frame
 * 30. ignoreButton (1 from the charge effect) skips the button test, as a
 * remote player does.
 */
s32 Bow_ArcVisible(void *obj, u16 ignoreButton)
{
    Player *pl = obj;

    if (pl->stateGroup != PL_GROUP_WEAPON) {
        return 0;
    }
    switch (pl->state) {
    case BOW_FIRE:
    case BOW_FIRE_QUICK:
        return 0;
    case BOW_CHARGE:
    case BOW_CHARGE_RESUME:
    case BOW_CHARGE_FROM_IDLE:
    case BOW_CHARGE_AFTER_SHOT:
    case BOW_CHARGE_AFTER_STAB:
    case BOW_DRAW_CHARGE:
        if (ignoreButton == 0 && Player_IsLocal(pl) == 1 && !(pl->input.held & PAD_R1)) {
            return 0;
        }
        if (pl->motionId[0] == 0x57D) {
            if (Motion_ReachedFrame(pl, 0, 30.0f) != 0) {
                return 1;
            }
            return 0;
        }
        return 0;
    case BOW_AIM:
    case BOW_AIM_RESUME:
        if (ignoreButton == 0 && Player_IsLocal(pl) == 1) {
            if (!(pl->input.held & PAD_R1)) {
                return 0;
            }
            if (pl->motion[0].blend == 1) {
                return 0;
            }
        }
        return 1;
    case BOW_CHARGE_WALK:
        return ignoreButton != 0;
    }
    return 0;
}

/* Sets ammoType from the loaded coating's item (0 with no coating). */
static void Bow_UpdateAmmoType(Player *player)
{
    u8 slot = player->coatingSlot;

    if (slot == 0xFF) {
        player->ammoType = 0;
    } else {
        player->ammoType = gItemData[player->pouch[slot].item].ammoKind;
    }
}

/*
 * Returns the charge level: weaponTimer / 30 (faster with skill 0x33), 2 at
 * most (3 with skill 0xB8).
 */
u8 Bow_ChargeLevel(Player *pl)
{
    s16 t = pl->weaponTimer;
    u8 lvl;
    u8 max;

    if (Player_HasSkill(pl, 0x33) == 1) {
        t = 1.1764705f * t;
    }
    lvl = t / 30;
    if (Player_HasSkill(pl, 0xB8) == 1) {
        max = 3;
    } else {
        max = 2;
    }
    if (lvl > max) {
        lvl = max;
    }
    return lvl;
}

/* Adds delta to aimWeight, kept within -100..100. */
static void Bow_AddAimWeight(Player *pl, s8 d)
{

    pl->aimWeight += d;
    if (pl->aimWeight >= 100) {
        pl->aimWeight = 100;
    }
    if (pl->aimWeight < -99) {
        pl->aimWeight = -100;
    }
}

/* One frame of the charge: Player_TickWeaponTimer, and 1 stamina. */
void Bow_TickCharge(Player *pl)
{
    Player_TickWeaponTimer(pl);
    Player_UseStamina(pl, -1);
}

/*
 * The local player's aim while the aim button is held: input.held
 * 0x2000/0x1000 tilt aimWeight (gScene.aimReverse reverses it), 0x800/0x400
 * turn the heading; then the aim layers. layers picks their motions: 0 the
 * aim stance's, 1 the charge's.
 */
void Bow_UpdateAim(Player *pl, s32 layers)
{
    u16 h;
    s8 d;

    if (Player_IsLocal(pl) == 0) {
        return;
    }
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        pl->aimLockTimer = 10;
    }
    if (pl->aimLockTimer == 0 && pl->aimMode == 0 && (pl->input.held & PAD_L2)) {
        return;
    }
    h = pl->input.held;
    if (!(h & 0x80)) {
        return;
    }
    if (h & 0x3000) {
        d = 5;
        if (pl->aimMode != 0 && gScene.aimReverse != 0) {
            d *= -1;
        }
        if (h & 0x2000) {
            Bow_AddAimWeight(pl, d);
        } else {
            Bow_AddAimWeight(pl, (s8) -d);
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
    switch (layers) {
    case 0:
        Bow_SetAimLayers(pl, 0x57A, 0x57B);
        break;
    case 1:
        Bow_SetAimLayers(pl, 0x57E, 0x57F);
        break;
    case 2:
        Bow_SetAimLayers(pl, 0x58B, 0x58C);
        break;
    }
}

/* As Gunlance_SetAimLayers, for the bow. */
static void Bow_SetAimLayers(Player *pl, u16 motA, u16 motB)
{

    if (pl->aimWeight == 0) {
        pl->partAnimOn[0] = 0;
        pl->partAnimOn[1] = 0;
        return;
    }
    pl->partAnimOn[0] = 1;
    pl->partAnimOn[1] = 1;
    if (pl->aimWeight > 0) {
        pl->partAnimMotion[0] = motA;
        pl->partAnimMotion[1] = motA + 100;
        pl->partAnimWeight[0] = pl->aimWeight;
        pl->partAnimWeight[1] = pl->aimWeight;
    } else {
        pl->partAnimMotion[0] = motB;
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

/*
 * When the stab bounced off (hitKind 1): requests BOW_BOUNCE and returns 1;
 * else 0.
 */
s32 Bow_CheckHit(Player *pl)
{
    if ((pl->hitPending != 0) && (pl->hitKind == 1)) {
        CamShake_ReqA(pl, 0);
        pl->hitStop = 0;
        pl->hitStopAdd = 0;
        Player_RequestState(pl, PL_GROUP_WEAPON, BOW_BOUNCE, 0);
        return 1;
    }
    return 0;
}

/*
 * The shot (Bow_FireArrows): chargeLevel set, a coating used up. variant:
 * 0 BOW_FIRE (charge level 1 or more), 1 BOW_FIRE_QUICK (level 0). The window
 * evades or charges again; it ends in BOW_AIM_RESUME.
 */
void Bow_StateFire(Player *pl, s32 variant)
{
    u8 slot;
    f32 f;

    if (pl->aimLockTimer != 0) {
        pl->aimLockTimer = 5;
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0xFF;
        Player_SetMoveMode(pl, 0, 1, 0);
        if (variant == 0) {
            Bow_SetAimLayers(pl, 0x581, 0x582);
            Player_SetMotionPair(pl, 0x580, 0, 0);
        } else {
            Bow_SetAimLayers(pl, 0x588, 0x589);
            Player_SetMotionPair(pl, 0x587, 0, 0);
        }
        Vib_StartLocal(pl, 0);
        CamShake_ReqB(pl, 1);
        pl->chargeLevel = Bow_ChargeLevel(pl);
        Bow_FireArrows(pl, 0);
        pl->weaponTimer = 0;
        if (Player_HasSkill(pl, 0x33) == 0) {
            pl->roundsLoaded--;
        }
        if (pl->coatingSlot != 0xFF) {
            if (Player_IsLocal(pl) == 1) {
                slot = pl->coatingSlot;
                if (Quest_PhaseIsOver(1) == 0) {
                    Pouch_Add(pl, pl->pouch[pl->coatingSlot].item, -1);
                }
                if (slot == pl->ammoSlot && pl->pouch[pl->ammoSlot].count <= 0) {
                    pl->ammoSlot = Bowgun_NextAmmoSlot(pl, 0x14, 2);
                    pl->ammoCount = pl->pouch[pl->ammoSlot].count;
                    Bowgun_LoadAmmo(pl, 0);
                }
            }
            pl->savedAmmoCount = pl->ammoCount;
            pl->savedClipSize = pl->clipSize;
            pl->savedRoundsLoaded = pl->roundsLoaded;
            pl->savedCoatingSlot = pl->coatingSlot;
        }
        break;
    case 1:
    case 2:
        if (Motion_PassedFrame(pl, 1, 8.0f) != 0 && pl->step == 1) {
            pl->step++;
        }
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_WEAPON, BOW_AIM_RESUME, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
            break;
        }
        if (variant == 0) {
            f = 36.0f;
        } else {
            f = 22.0f;
        }
        if (Motion_ReachedFrame(pl, 0, f) != 0) {
            if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B) {
                Player_RequestState(pl, PL_GROUP_WEAPON, BOW_EVADE, PL_SET_SNAP_POS);
                break;
            }
            if (pl->input.stickDir & PAD_RSTICK_DOWN) {
                Player_RequestState(pl, PL_GROUP_WEAPON, BOW_CHARGE_AFTER_SHOT, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
            }
        }
        break;
    }
}

/*
 * The charge. variant: 0 BOW_CHARGE, 1 BOW_CHARGE_RESUME (keeps the time), 2
 * BOW_CHARGE_FROM_IDLE, 3 BOW_CHARGE_AFTER_SHOT, 4 BOW_CHARGE_AFTER_STAB, 5
 * BOW_DRAW_CHARGE. Past frame 40, letting go of the charge (or running out of
 * stamina) shoots.
 */
void Bow_StateCharge(Player *pl, u32 variant)
{

    if (pl->aimLockTimer != 0) {
        pl->aimLockTimer = 5;
    }
    pl->weaponOut = 1;
    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
        case 1:
        case 2:
            pl->step = 2;
            Bow_SetAimLayers(pl, 0x57E, 0x57F);
            if (variant == 0 || variant == 2) {
                Player_SetMotionPair(pl, 0x57D, 0, 0);
                pl->weaponTimer = 0;
                pl->timer = 0;
                StageObj_K29_CreateOnPlayerDefault(pl, 2, 1.0f);
            } else {
                pl->timer = 30;
                Player_SetMotionPair(pl, 0x57D, 0, 0x78);
                Bow_TickCharge(pl);
            }
            break;
        case 3:
            pl->step++;
            Bow_SetAimLayers(pl, 0x58B, 0x58C);
            Player_SetMotionPair(pl, 0x58A, 0, 0);
            pl->weaponTimer = 0;
            pl->timer = 0;
            StageObj_K29_CreateOnPlayerDefault(pl, 2, 1.0f);
            break;
        case 4:
            pl->step++;
            Player_SetMotionPair(pl, 0x3F5, 4, 0);
            pl->weaponTimer = 0;
            pl->timer = 0;
            StageObj_K29_CreateOnPlayerDefault(pl, 2, 1.0f);
            break;
        case 5:
            pl->step++;
            pl->timer = 0;
            Player_SetMotionPair(pl, 0x3F4, 0, 0);
            pl->weaponTimer = 0;
            StageObj_K29_CreateOnPlayerDefault(pl, 2, 1.0f);
            break;
        }
        pl->weaponOut = 1;
        pl->roundsLoaded = pl->clipSize;
        if (pl->roundsLoaded > pl->ammoCount) {
            pl->roundsLoaded = pl->ammoCount;
        }
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        Bow_TickCharge(pl);
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Bow_SetAimLayers(pl, 0x57E, 0x57F);
            switch (variant) {
            case 5:
            default:
                Player_SetMotionPair(pl, 0x57D, 0, 0x1E);
                break;
            case 3:
                Player_SetMotionPair(pl, 0x57D, 0, 0x28);
                break;
            case 4:
                Player_SetMotionPair(pl, 0x57D, 0, 0xE);
                break;
            }
        }
        break;
    case 2:
        Bow_TickCharge(pl);
        if (Player_IsLocal(pl) == 1 && Motion_ReachedFrame(pl, 0, 40.0f) != 0) {
            if (!(pl->input.stickDir & PAD_RSTICK_DOWN) || pl->stamina <= 0) {
                if (Bow_ChargeLevel(pl) == 0) {
                    Bow_UpdateAmmoType(pl);
                    Player_RequestState(pl, PL_GROUP_WEAPON, BOW_FIRE_QUICK, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
                } else {
                    Bow_UpdateAmmoType(pl);
                    Player_RequestState(pl, PL_GROUP_WEAPON, BOW_FIRE, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
                }
            } else {
                Bow_UpdateAim(pl, 1);
                if (pl->timer > 0) {
                    pl->timer--;
                }
                if (pl->timer <= 0) {
                    Bow_CheckInput(pl, 0);
                }
            }
        }
        break;
    }
}

/* BOW_TO_IDLE: one motion, then idle. */
void Bow_StateToIdle(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x584, 0, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 0, 0);
        }
        break;
    }
}

/*
 * The stab. variant: 0 BOW_STAB, 1 BOW_DRAW_STAB (the draw first, moving
 * forward). Frames 34-64 queue BOW_STAB_2, BOW_STAB_EVADE or
 * BOW_CHARGE_AFTER_STAB, started at frame 44.
 */
void Bow_StateStab(Player *pl, s32 variant)
{
    f32 in[3];
    f32 out[3];
    u8 x;

    switch (pl->step) {
    case 0:
        pl->stateVar0 = 0xFF;
        if (variant == 0) {
            pl->step = 2;
            Player_SetMotionPair(pl, 0x585, 4, 0);
            PlayerAttack_CreateBowStab(pl, 0x3C);
        } else {
            pl->step++;
            Player_SetMotionPair(pl, 0x3F6, 2, 0);
        }
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->weaponOut = 1;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_UpdateMotionTime(pl, 0);
            Player_SetMotionPair(pl, 0x585, 4, 0x18);
            PlayerAttack_CreateBowStab(pl, 0x3C);
        }
        break;
    case 2:
        if (Bow_CheckHit(pl) == 1) {
            break;
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (pl->stateVar0 == 0xFF) {
            if (Motion_InFrameRange(pl, 0, 34.0f, 64.0f) != 0) {
                if ((pl->input.stickDirPressed & PAD_RSTICK_UP) && pl->aimLockTimer == 0 && pl->aimMode == 0) {
                    pl->stateVar0 = 4;
                } else if (pl->input.pressed & PAD_CROSS) {
                    pl->stateVar0 = 0x12;
                } else if (pl->input.stickDir & PAD_RSTICK_DOWN) {
                    pl->stateVar0 = 0x13;
                }
            }
        } else if (Motion_ReachedFrame(pl, 0, 44.0f) != 0) {
            x = pl->stateVar0;
            if (x == 0x12) {
                if (pl->stamina >= 0x4B) {
                    Player_RequestState(pl, PL_GROUP_WEAPON, BOW_STAB_EVADE, PL_SET_SNAP_POS);
                }
            } else if (x == 4) {
                Bow_UpdateAmmoType(pl);
                Player_RequestState(pl, PL_GROUP_WEAPON, BOW_STAB_2, PL_SET_SNAP_POS);
            } else if (x == 0x13) {
                Player_RequestState(pl, PL_GROUP_WEAPON, BOW_CHARGE_AFTER_STAB, PL_SET_SNAP_POS);
            }
            break;
        }
        if (variant == 1 && Motion_ReachedFrame(pl, 0, 50.0f) == 0) {
            in[0] = 0.0f;
            in[1] = 0.0f;
            in[2] = 3.0f;
            Vec_Rotate((f32 *) out, (f32 *) in, (f32 *) pl->rotMtx);
            pl->pos[0] += out[0];
            pl->pos[2] += out[2];
        }
        break;
    }
}

/* BOW_STAB_2, then idle. */
void Bow_StateStab2(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMotionPair(pl, 0x586, 4, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        PlayerAttack_CreateBowStab(pl, 0x3D);
        break;
    case 1:
        if ((Bow_CheckHit(pl) != 1) && (pl->motion[0].playing == 0)) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * The aim stance. variant: 0 BOW_AIM (its first 20 frames read the input as
 * caller 2), 1 BOW_AIM_RESUME. Letting go of the aim button returns to idle.
 */
void Bow_StateAim(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->timer = 20;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x579, 4, 0);
        } else {
            Player_SetMotionPair(pl, 0x579, 0, 0);
        }
        Bow_SetAimLayers(pl, 0x57A, 0x57B);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (--pl->timer <= 0) {
            pl->step++;
        }
        if (pl->motion[0].blend == 0 && Player_IsLocal(pl) == 1) {
            Bow_UpdateAim(pl, 0);
            if (variant == 1) {
                Bow_CheckInput(pl, 1);
            } else {
                Bow_CheckInput(pl, 2);
            }
        }
        break;
    case 2:
        if (Player_IsLocal(pl) == 1) {
            if (!(pl->input.held & PAD_R1) && pl->aimMode == 0) {
                Player_ReturnToIdle(pl, 0, 0xA, 0);
            } else {
                Bow_UpdateAim(pl, 0);
                Bow_CheckInput(pl, 1);
            }
        }
        break;
    }
}

/*
 * BOW_CHARGE_WALK: the charge goes on while walking; the stick back to the
 * middle gives BOW_CHARGE_RESUME; letting go shoots.
 */
void Bow_StateChargeWalk(Player *pl, s32 unused)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x3F1, 4, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x02001000);
        pl->timer = 0;
        Bow_TickCharge(pl);
        break;
    case 1:
        Bow_TickCharge(pl);
        if (Player_IsLocal(pl) == 1) {
            if (Player_StickMoveLevel(pl, 0) == 0) {
                Player_RequestState(pl, PL_GROUP_WEAPON, BOW_CHARGE_RESUME, PL_SET_SNAP_POS);
            } else if (!(pl->input.stickDir & PAD_RSTICK_DOWN) || pl->stamina <= 0) {
                if (Bow_ChargeLevel(pl) == 0) {
                    Bow_UpdateAmmoType(pl);
                    Player_RequestState(pl, PL_GROUP_WEAPON, BOW_FIRE_QUICK, PL_SET_SNAP_POS);
                } else {
                    Bow_UpdateAmmoType(pl);
                    Player_RequestState(pl, PL_GROUP_WEAPON, BOW_FIRE, PL_SET_SNAP_POS);
                }
            } else {
                Bow_CheckInput(pl, 0);
            }
        }
        break;
    }
}

/*
 * The evade (75 stamina). variant: 0 BOW_EVADE, 1 BOW_EVADE_2, 2
 * BOW_STAB_EVADE. It ends in BOW_AIM while the aim button is held.
 */
void Bow_StateEvade(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->motionTime = 0;
        Player_SetEvadeFrames(pl);
        pl->timer = 0;
        Player_ResetAim(pl);
        pl->noFlinchTimer = 0;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x3EE, 2, 0);
            break;
        case 1:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x3EE, 0, 6);
            break;
        case 2:
            Player_SetMotionPair(pl, 0x3F3, 2, 0);
            break;
        }
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_AddStamina(pl, -75);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x3EE, 0, 0xE);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            if (pl->input.held & PAD_R1) {
                Player_RequestState(pl, PL_GROUP_WEAPON, BOW_AIM, PL_SET_SNAP_POS);
                break;
            }
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (Player_IsLocal(pl) == 1) {
            if (Motion_InFrameRange(pl, 0, 32.0f, 42.0f) != 0 && (pl->input.pressed & PAD_CROSS) && pl->stateVar0 == 0) {
                pl->stateVar0 = 1;
            }
            if (Motion_PassedFrame(pl, 0, 42.0f) != 0 && pl->stateVar0 != 0 && pl->stamina >= 75) {
                Player_RequestState(pl, PL_GROUP_WEAPON, BOW_EVADE_2, PL_SET_SNAP_POS);
            }
        }
        break;
    }
}

/* BOW_DRAW_AIM: draws the bow, then BOW_AIM_RESUME. */
void Bow_StateDrawAim(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->motionTime = 0;
        pl->weaponOut = 1;
        pl->timer = 0;
        Player_SetMotionPair(pl, 0x3F2, 4, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_WEAPON, BOW_AIM_RESUME, 0);
        }
        break;
    }
}

/* BOW_BOUNCE, then idle. */
void Bow_StateBounce(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x583, 0, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * BOW_LOAD_COATING: loads the ammo slot's coating (or clears it), the
 * motion's speed by the reload skills 0x2E-0x32.
 */
void Bow_StateLoadCoating(Player *pl, s32 unused)
{
    u16 k;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x57C, 4, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->motionSpeed = 1.0f;
        if (Player_HasSkill(pl, 0x2E) == 1) {
            pl->motionSpeed = 1.17647052f;
        } else if (Player_HasSkill(pl, 0x2F) == 1) {
            pl->motionSpeed = 1.33333337f;
        } else if (Player_HasSkill(pl, 0x30) == 1) {
            pl->motionSpeed = 1.53846157f;
        } else if (Player_HasSkill(pl, 0x31) == 1) {
            pl->motionSpeed = 0.909090877f;
        } else if (Player_HasSkill(pl, 0x32) == 1) {
            pl->motionSpeed = 0.714285731f;
        }
        k = pl->pouch[pl->ammoSlot].item;
        if (k == 0x87 || k == 0) {
            Bow_ClearCoatingSlot(pl);
        } else if (pl->coatingSlot != 0xFF) {
            Bow_ClearCoatingSlot(pl);
            pl->ammoType = 0;
        } else {
            Bow_SetCoatingSlot(pl);
            pl->ammoType = gItemData[pl->pouch[pl->coatingSlot].item].ammoKind;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * Runs the bow's PL_GROUP_WEAPON state: maps the state to its handler and
 * variant.
 */
void Bow_RunState(Player *pl)
{
    switch (pl->state) {
    case BOW_FIRE:
        Bow_StateFire(pl, 0);
        break;
    case BOW_CHARGE:
        Bow_StateCharge(pl, 0U);
        break;
    case BOW_TO_IDLE:
        Bow_StateToIdle(pl, 0);
        break;
    case BOW_STAB:
        Bow_StateStab(pl, 0);
        break;
    case BOW_STAB_2:
        Bow_StateStab2(pl, 0);
        break;
    case BOW_AIM:
        Bow_StateAim(pl, 0);
        break;
    case BOW_CHARGE_WALK:
        Bow_StateChargeWalk(pl, 0);
        break;
    case BOW_AIM_RESUME:
        Bow_StateAim(pl, 1);
        break;
    case BOW_EVADE:
        Bow_StateEvade(pl, 0);
        break;
    case BOW_CHARGE_RESUME:
        Bow_StateCharge(pl, 1U);
        break;
    case BOW_EVADE_2:
        Bow_StateEvade(pl, 1);
        break;
    case BOW_CHARGE_FROM_IDLE:
        Bow_StateCharge(pl, 2U);
        break;
    case BOW_DRAW_AIM:
        Bow_StateDrawAim(pl, 0);
        break;
    case BOW_DRAW_STAB:
        Bow_StateStab(pl, 1);
        break;
    case BOW_FIRE_QUICK:
        Bow_StateFire(pl, 1);
        break;
    case BOW_CHARGE_AFTER_SHOT:
        Bow_StateCharge(pl, 3U);
        break;
    case BOW_BOUNCE:
        Bow_StateBounce(pl, 0);
        break;
    case BOW_LOAD_COATING:
        Bow_StateLoadCoating(pl, 0);
        break;
    case BOW_STAB_EVADE:
        Bow_StateEvade(pl, 2);
        break;
    case BOW_CHARGE_AFTER_STAB:
        Bow_StateCharge(pl, 4U);
        break;
    case BOW_DRAW_CHARGE:
        Bow_StateCharge(pl, 5U);
        
    default:
        break;
    }
}

/*
 * The drawn idle's input: the evade or the roll, the aim (needPress 0: holding
 * the aim button is enough, else it must be pressed), BOW_AIM_RESUME
 * in the aim mode, the charge, the stab, the coating. Returns 1 when it
 * started a state.
 */
s32 Bow_AttackInput(Player *pl, s32 needPress)
{
    u8 f;

    if (pl->stamina >= 0x4B && (pl->input.pressed & PAD_CROSS)) {
        if (pl->input.stickDir & PAD_DPAD) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
        } else {
            Player_RequestState(pl, PL_GROUP_WEAPON, BOW_EVADE, PL_SET_SNAP_POS);
        }
        return 1;
    }
    if (needPress == 0) {
        if (pl->input.held & PAD_R1) {
            Player_RequestState(pl, PL_GROUP_WEAPON, BOW_AIM, PL_SET_SNAP_POS);
            return 1;
        }
    } else if (pl->input.pressed & PAD_R1) {
        Player_RequestState(pl, PL_GROUP_WEAPON, BOW_AIM, PL_SET_SNAP_POS);
        return 1;
    }
    f = pl->aimMode;
    if (f != 0) {
        Player_RequestState(pl, PL_GROUP_WEAPON, BOW_AIM_RESUME, PL_SET_SNAP_POS);
        return 1;
    }
    if (pl->input.stickDir & PAD_RSTICK_DOWN) {
        Player_RequestState(pl, PL_GROUP_WEAPON, BOW_CHARGE_FROM_IDLE, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
        return 1;
    }
    if ((pl->input.stickDirPressed & PAD_RSTICK_UP) && pl->aimLockTimer == 0 && f == 0) {
        Bow_UpdateAmmoType(pl);
        Player_RequestState(pl, PL_GROUP_WEAPON, BOW_STAB, PL_SET_SNAP_POS);
        return 1;
    }
    if ((pl->input.pressed & PAD_R3) && pl->pouch[pl->ammoSlot].item != 0x87 && pl->coatingSlot == 0xFF) {
        Player_RequestState(pl, PL_GROUP_WEAPON, BOW_LOAD_COATING, PL_SET_SNAP_POS);
        return 1;
    }
    return 0;
}

/*
 * An aim or charge state's input. caller: 0 charging (the stick walks or
 * turns), 1 the aim stance (the stick walks: PL_ACT_BOW_WALK), 2 the aim
 * stance's first frames (the stick ignored). Returns 1 when it started a
 * state.
 */
s32 Bow_CheckInput(Player *pl, s32 caller)
{

    if (Player_StickMoveLevel(pl, 0) != 0) {
        if (caller == 0) {
            if (Player_IsState(pl, PL_GROUP_WEAPON, BOW_CHARGE_WALK) == 0) {
                Player_FaceStick(pl);
                Player_RequestState(pl, PL_GROUP_WEAPON, BOW_CHARGE_WALK, 0);
            } else {
                Player_FaceStick(pl);
            }
        } else if (caller == 1) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BOW_WALK, 0);
        }
    }
    if (pl->stamina >= 0x4B && (pl->input.pressed & PAD_CROSS)) {
        if (pl->input.stickDir & PAD_DPAD) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
        } else {
            Player_RequestState(pl, PL_GROUP_WEAPON, BOW_EVADE, PL_SET_SNAP_POS);
        }
        return 1;
    }
    if (caller == 0) {
        return 0;
    }
    if (pl->input.stickDir & PAD_RSTICK_DOWN) {
        Player_RequestState(pl, PL_GROUP_WEAPON, BOW_CHARGE, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
        return 1;
    }
    if ((pl->input.stickDirPressed & PAD_RSTICK_UP) && pl->aimLockTimer == 0 && pl->aimMode == 0) {
        Bow_UpdateAmmoType(pl);
        Player_RequestState(pl, PL_GROUP_WEAPON, BOW_STAB, PL_SET_SNAP_POS);
        return 1;
    }
    if ((pl->input.pressed & PAD_R3) && pl->pouch[pl->ammoSlot].item != 0x87 && pl->coatingSlot == 0xFF) {
        Player_RequestState(pl, PL_GROUP_WEAPON, BOW_LOAD_COATING, PL_SET_SNAP_POS);
        return 1;
    }
    return 0;
}

/*
 * The attack that draws on the move: BOW_DRAW_CHARGE with the charge held,
 * else BOW_DRAW_STAB.
 */
void Bow_DrawAttack(Player *pl)
{
    if (pl->input.stickDir & PAD_RSTICK_DOWN) {
        Player_RequestState(pl, PL_GROUP_WEAPON, BOW_DRAW_CHARGE, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
        return;
    }
    Bow_UpdateAmmoType(pl);
    Player_RequestState(pl, PL_GROUP_WEAPON, BOW_DRAW_STAB, PL_SET_SNAP_POS);
}

/* The aim button held while sheathed: BOW_DRAW_AIM. */
void Bow_DrawToAim(Player *pl)
{
    Player_RequestState(pl, PL_GROUP_WEAPON, BOW_DRAW_AIM, PL_SET_SNAP_POS);
}

/*
 * Makes a PlayerAttack for pl: an ObjAttach that follows pl and hits with the
 * row attackId of PlayerAttack_Rows from pl's current motion. Clears pl's
 * hitPending and contactHit for the new attack.
 */
void PlayerAttack_Create(Player *pl, u8 attackId)
{
    ObjAttach *obj;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_PLAYER_ATTACK;
        obj->type = attackId;
        obj->update = PlayerAttack_Update;
        obj->idx = pl->index;
        obj->isEm = pl->isEm;
        obj->ownerMotion = pl->motionId[0];
        obj->owner = pl;
        obj->yaw = pl->rot[1];
        obj->randBits = pl->stateRand;
        obj->stateVar0 = 0;
        obj->stateVar1 = 0;
        obj->rotX = pl->rot[0];
        obj->rotY = pl->rot[1];
        obj->rotZ = pl->rot[2];
        Vec_Copy(obj->pos, &pl->pos[0]);
        obj->side = *(u8 *) &pl->side;
        obj->unk080 = 0;
        pl->hitPending = 0;
        pl->contactHit = 0;
    }
}

/*
 * PlayerAttack_Create for the dual swords: mirrored (bit 0) swaps the hit
 * shape lists 9 and 10, the two blades (PlayerAttack_Setup). Leaves
 * contactHit alone.
 */
void PlayerAttack_CreateDual(Player *pl, s32 attackId, s32 mirrored)
{
    ObjAttach *obj;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_PLAYER_ATTACK;
        obj->type = attackId;
        obj->update = PlayerAttack_Update;
        obj->idx = pl->index;
        obj->isEm = pl->isEm;
        obj->ownerMotion = pl->motionId[0];
        obj->owner = pl;
        obj->yaw = pl->rot[1];
        obj->randBits = pl->stateRand;
        obj->stateVar0 = 0;
        obj->stateVar1 = mirrored;
        obj->rotX = pl->rot[0];
        obj->rotY = pl->rot[1];
        obj->rotZ = pl->rot[2];
        Vec_Copy(obj->pos, &pl->pos[0]);
        obj->side = *(u8 *) &pl->side;
        obj->unk080 = 0;
        pl->hitPending = 0;
    }
}

/*
 * PlayerAttack_Create for the bow's close stab (attack ids 0x3C, 0x3D): it
 * carries the loaded coating, pl->ammoType, which PlayerAttack_Setup applies
 * (BowArrow_ApplyCoating).
 */
void PlayerAttack_CreateBowStab(Player *pl, s8 attackId)
{
    ObjAttach *obj;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_PLAYER_ATTACK;
        obj->type = attackId;
        obj->update = PlayerAttack_Update;
        obj->idx = pl->index;
        obj->isEm = pl->isEm;
        obj->ownerMotion = pl->motionId[0];
        obj->owner = pl;
        obj->yaw = pl->rot[1];
        obj->randBits = pl->stateRand;
        obj->stateVar1 = 0;
        obj->rotX = pl->rot[0];
        obj->rotY = pl->rot[1];
        obj->rotZ = pl->rot[2];
        Vec_Copy(obj->pos, &pl->pos[0]);
        obj->side = *(u8 *) &pl->side;
        obj->stateVar0 = pl->ammoType;
        pl->hitPending = 0;
    }
}

/*
 * The PlayerAttack's per-frame update, by ObjAttach.state. A hit (ATTACH_HIT)
 * keeps the window open two more frames and runs on.
 */
void PlayerAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        PlayerAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        PlayerAttack_Run(obj);
        break;
    case ATTACH_END:
        PlayerAttack_End(obj);
        break;
    case ATTACH_HIT:
        obj->state = ATTACH_RUN;
        (obj)->hitFrames += 2;
        PlayerAttack_Run(obj);
        break;
    case 4:
        PlayerAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        PlayerAttack_End(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        PlayerAttack_End(obj);
        break;
    case 7:
        PlayerAttack_End(obj);
        break;
    case ATTACH_FREE:
        PlayerAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Puts the attack's capsule on the weapon: pos and pos2 are the hit shape's
 * two z offsets (a[2], b[2]) along bone 0xE, turned by a fixed rotation. Used
 * for hit shapes whose bone is 0x7E.
 */
void PlayerAttack_PlaceOnWeapon(ObjAttach *obj, Player *pl)
{
    f32 *iy;
    f32 *iz;
    f32 *oy;
    f32 *oz;
    f32 m0[16];
    f32 m1[16];
    f32 in[3];
    f32 out[3];

    Mtx_Copy(m0, (f32 *)Em_GetBone(pl, 0xE));
    Mtx_Identity(m1);
    Mtx_SetRotXYZ(m1, 0.0628318489f, -0.541052103f, -3.04734492f);
    Mtx_Concat3x3(m1, m0);
    in[0] = 0.0f;
    iy = &in[1];
    *iy = 0.0f;
    iz = &in[2];
    *iz = obj->hitList->a[2];
    Vec_Rotate(out, in, m1);
    Mtx_GetTrans(obj->pos2, m0);
    oy = &out[1];
    oz = &out[2];
    obj->pos2[0] += out[0];
    obj->pos2[1] += *oy;
    obj->pos2[2] += *oz;
    in[0] = 0.0f;
    *iy = 0.0f;
    *iz = obj->hitList->b[2];
    Vec_Rotate(out, in, m1);
    Mtx_GetTrans(obj->pos, m0);
    obj->pos[0] += out[0];
    obj->pos[1] += *oy;
    obj->pos[2] += *oz;
}

/*
 * Sets the attack up from its row (MoveObj_StartMotionOwned copies it over
 * hitDelay..unk075): the hit shapes, the attack flags, the weapon's element
 * (and, one hit in three, its status) and the affinity's roll (a critical hit
 * 1.25 times the power, a weak one half; only from sharpness level 3). The
 * dual swords' demon mode adds 35 % power. hitDelay is counted from the
 * motion's blend and current frame.
 */
void PlayerAttack_Setup(ObjAttach *obj)
{
    Player *pl = &gPlayers[obj->idx];
    s16 stat;
    s32 blendFrames;
    f32 in[3];
    f32 out[3];
    u8 takesElement;

    takesElement = 0;
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    MoveObj_StartMotionOwned((struct MoveObj *)obj, (s32) pl, obj->type, &PlayerAttack_RowsPtr);
    if (obj->stateVar1 & 1) {
        switch (obj->hitListNo) {
        case 9:
            obj->hitListNo = 10;
            break;
        case 10:
            obj->hitListNo = 9;
            break;
        }
    }
    obj->hitList = PlayerAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->area = pl->area;
    if (obj->hitList->bone == 0x7E) {
        PlayerAttack_PlaceOnWeapon(obj, pl);
    }
    switch (obj->type) {
    case 0xB:
    case 0x56:
        in[0] = 0.0f;
        in[1] = 0.0f;
        in[2] = 200.0f;
        Vec_Rotate((f32 *) out, (f32 *) in, (f32 *) pl->worldMtx);
        obj->pos[0] += out[0];
        obj->pos[1] += out[1];
        obj->pos[2] += out[2];
        obj->aux = 4;
        Player_TestSetFlags(obj, ATKF_OWNER_CONTACT | ATKF_WEAPON);
        takesElement = 1;
        break;
    case 0x3E:
        in[0] = 117.0f;
        in[1] = 21.0f;
        in[2] = 187.0f;
        Vec_Rotate(out, in, (f32 *)Em_GetBone(pl, 0xE));
        Mtx_GetTrans(obj->pos, Em_GetBone(pl, 0xE)->world);
        takesElement = 1;
        obj->pos[0] += out[0];
        obj->pos[1] += out[1];
        obj->pos[2] += out[2];
        break;
    case 4:
    case 5:
    case 0x1D:
    case 0x60:
    case 0x61:
        Player_TestSetFlags(obj, ATKF_OWNER_CONTACT);
        break;
    case 0x3C:
    case 0x3D:
        Player_TestSetFlags(obj, ATKF_OWNER_CONTACT | ATKF_WEAPON);
        BowArrow_ApplyCoating(obj, obj->stateVar0, 1);
        break;
    default:
        Player_TestSetFlags(obj, ATKF_OWNER_CONTACT | ATKF_WEAPON);
        takesElement = 1;
        break;
    }
    if (takesElement) {
        stat = Player_GetWeaponStat(pl, WSTAT_FIRE);
        if (stat > 0) {
            obj->elementFlags |= ELEMF_FIRE;
            obj->elementPower = stat;
        }
        stat = Player_GetWeaponStat(pl, WSTAT_WATER);
        if (stat > 0) {
            obj->elementFlags |= ELEMF_WATER;
            obj->elementPower = stat;
        }
        stat = Player_GetWeaponStat(pl, WSTAT_THUNDER);
        if (stat > 0) {
            obj->elementFlags |= ELEMF_THUNDER;
            obj->elementPower = stat;
        }
        stat = Player_GetWeaponStat(pl, WSTAT_ICE);
        if (stat > 0) {
            obj->elementFlags |= ELEMF_ICE;
            obj->elementPower = stat;
        }
        stat = Player_GetWeaponStat(pl, WSTAT_DRAGON);
        if (stat > 0) {
            obj->elementFlags |= ELEMF_DRAGON;
            obj->elementPower = stat;
        }
        if (pl->stateRand % 3 == 0) {
            stat = Player_GetWeaponStat(pl, WSTAT_POISON);
            if (stat > 0) {
                obj->elementFlags |= ELEMF_POISON;
                obj->elementPower = stat;
            }
            stat = Player_GetWeaponStat(pl, WSTAT_STATUS_TYPE2);
            if (stat > 0) {
                obj->elementFlags |= ELEMF_EM_STATUS4;
                obj->elementPower = stat;
            }
            stat = Player_GetWeaponStat(pl, WSTAT_STATUS_TYPE3);
            if (stat > 0) {
                obj->elementFlags |= ELEMF_EM_STATUS2;
                obj->elementPower = stat;
            }
        }
    }
    obj->hitFx &= HITFX_KIND_MASK;
    stat = Player_GetWeaponStat(pl, WSTAT_AFFINITY);
    if (stat != 0 && pl->sharpnessLevel >= 3) {
        if (stat < 0) {
            if (pl->stateRand % 100 < -stat) {

                obj->power >>= 1;
                obj->hitFx |= HITFX_WEAK;
            }
        } else if (stat > pl->stateRand % 100) {
            obj->power = (s32) (1.25f * obj->power);
            obj->hitFx |= HITFX_CRITICAL;
        }
    }
    if (pl->weaponType == WEAPON_DUAL_SWORDS && pl->demonMode != 0) {
        obj->unk075[0] = 0;
        obj->unk075[1] = 0x12;
        obj->power = 1.35f * obj->power;
    }
    if (obj->hitDelay != 0) {
        blendFrames = Math_Absf(pl->motionBlend[0] % 100);
        obj->hitDelay += blendFrames - (s32) (Math_Absf((u16) pl->motionFrame[0]) / 2.0f) - 1;
        if ((s8) obj->hitDelay < 0) {
            obj->hitDelay = 0;
        }
    }
}

/*
 * Keeps the hit window open while the player is in hit stop or slowed after a
 * hit.
 */
static void PlayerAttack_PauseInHitStop(ObjAttach *obj)
{
    Player *pl;

    pl = &gPlayers[obj->idx];
    if ((pl->hitStop != 0) || (((Player *) pl)->hitSlowTimer != 0)) {
        (obj)->hitFrames++;
    }
}

/*
 * Follows the weapon and ends the attack when the player's motion changes
 * (attack 8 also lasts through motion 0x57F, 0x64 through 0x57E, 0x13 holds
 * its window open for the whole motion); attacks 0xB and 0x56 last four
 * frames, 0x60 and 0x61 until their window closes.
 */
void PlayerAttack_Run(ObjAttach *obj)
{
    Player *pl = &gPlayers[obj->idx];

    if (obj->hitList->bone == 0x7E && obj->type != 0x44) {
        PlayerAttack_PlaceOnWeapon(obj, pl);
    }
    PlayerAttack_PauseInHitStop(obj);
    switch (obj->type) {
    case 8:
        if (pl->motionId[0] == obj->ownerMotion || pl->motionId[0] == 0x57F) {
            return;
        }
        goto end;
    case 0x64:
        if (pl->motionId[0] == obj->ownerMotion || pl->motionId[0] == 0x57E) {
            return;
        }
        goto end;
    case 0xB:
    case 0x56:
        if (--obj->aux > 0) {
            return;
        }
        goto end;
    case 0x13:
        if (pl->motionId[0] == obj->ownerMotion) {
            obj->hitFrames = 99;
            return;
        }
        goto end;
    case 0x60:
    case 0x61:
        if (obj->hitFrames <= 0) {
            goto end;
        }
        break;
    default:
        if (pl->motionId[0] != obj->ownerMotion) {
        end:
            obj->hitPhase = 0;
            obj->state = ATTACH_END;
        }
        break;
    }
}

/* Hides the attack and frees it on the next update. */
void PlayerAttack_End(ObjAttach *obj)
{
    obj->state = ATTACH_FREE;
    obj->visible = 0;
}

/* Returns the object to the pool. */
void PlayerAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

/*
 * Makes a RathalosAttack for em with the row attackId of RathalosAttack_Rows,
 * if a player is in em's area. Clears em's hitPending and contactHit.
 */
void RathalosAttack_Create(Em *em, s8 attackId)
{
    ObjAttach *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((obj = ObjAttach_Alloc(0)) != NULL) {
            obj->kind = ATTACH_RATHALOS_ATTACK;
            obj->type = attackId;
            obj->update = RathalosAttack_Update;
            obj->idx = em->index;
            obj->isEm = em->isEm;
            obj->ownerMotion = em->motionId[0];
            obj->owner = em;
            obj->yaw = em->rotY;
            obj->rotX = em->rotX;
            obj->rotY = em->rotY;
            obj->rotZ = em->rotZ;
            obj->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/*
 * RathalosAttack_Create at the stage object src's position, for src's owner
 * (and without the area check): the attack starts at step 1.
 */
void RathalosAttack_CreateOnObj(StageObj *src, s8 attackId)
{
    ObjAttach *obj;
    Em *em;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        em = src->owner;
        obj->kind = ATTACH_RATHALOS_ATTACK;
        obj->type = attackId;
        obj->update = RathalosAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = em->rotX;
        obj->rotY = em->rotY;
        obj->rotZ = em->rotZ;
        obj->side = *(u8 *) &em->side;
        obj->pos[0] = src->pos[0];
        obj->pos[1] = src->pos[1];
        obj->pos[2] = src->pos[2];
        em->hitPending = 0;
        em->contactHit = 0;
        obj->step = 1;
    }
}

/*
 * RathalosAttack_Create at pos (without the area check): the attack starts at
 * step 1.
 */
void RathalosAttack_CreateAt(Em *em, f32 *pos, s8 attackId)
{
    ObjAttach *obj;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_RATHALOS_ATTACK;
        obj->type = attackId;
        obj->update = RathalosAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = em->rotX;
        obj->rotY = em->rotY;
        obj->rotZ = em->rotZ;
        obj->side = *(u8 *) &em->side;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
        em->hitPending = 0;
        em->contactHit = 0;
        obj->step = 1;
    }
}

/* The RathalosAttack's per-frame update, by ObjAttach.state. */
void RathalosAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        RathalosAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        RathalosAttack_Run(obj);
        break;
    case ATTACH_END:
        RathalosAttack_End(obj);
        break;
    case ATTACH_HIT:
        RathalosAttack_Run(obj);
        break;
    case 4:
        RathalosAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        RathalosAttack_End(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        RathalosAttack_End(obj);
        break;
    case 7:
        RathalosAttack_End(obj);
        break;
    case ATTACH_FREE:
        RathalosAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT;
 * hitDelay is counted from the monster's motion.
 */
void RathalosAttack_Setup(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    Player_TestSetFlags(obj, ATKF_OWNER_CONTACT);
    MoveObj_StartMotionOwned((struct MoveObj *)obj, (s32) em, obj->type, &RathalosAttack_RowsPtr);
    obj->hitList = RathalosAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->area = em->area;
    if (obj->hitDelay != 0) {
        obj->hitDelay += (s32) Math_Absf((s16) em->motionBlend[0] % 100) - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) obj->hitDelay < 0) {
            obj->hitDelay = 0;
        }
    }
    obj->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes;
 * attacks 0x18 and 0x22 keep their window open.
 */
void RathalosAttack_Run(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    switch (obj->type) {
    case 0x18:
    case 0x22:
        obj->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != obj->ownerMotion || obj->hitPhase == 0) {
        obj->hitPhase = 0;
        obj->state = ATTACH_END;
    }
    if (obj->drawSlotBuf != NULL) {
        Vec_Copy(obj->drawSlotBuf->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void RathalosAttack_End(ObjAttach *obj)
{
    obj->hitPhase = 0;
    obj->state = ATTACH_FREE;
    obj->visible = 0;
    if (obj->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) obj->drawSlot);
    }
}

void RathalosAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

/*
 * Makes an EmContactAttack for em with the row attackId of
 * EmContactAttack_Rows, if a player is in em's area; returns it, or NULL.
 */
ObjAttach *EmContactAttack_Create(Em *em, s8 attackId)
{
    ObjAttach *obj;

    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return NULL;
    }
    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_EM_CONTACT_ATTACK;
        obj->type = attackId;
        obj->update = EmContactAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = em->rotX;
        obj->rotY = em->rotY;
        obj->rotZ = em->rotZ;
        obj->side = *(u8 *) &em->side;
        em->hitPending = 0;
        em->contactHit = 0;
    }
    return obj;
}

/* The EmContactAttack's per-frame update, by ObjAttach.state. */
void EmContactAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        EmContactAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        EmContactAttack_Run(obj);
        break;
    case ATTACH_END:
        EmContactAttack_End(obj);
        break;
    case ATTACH_HIT:
        EmContactAttack_Run(obj);
        break;
    case 4:
        EmContactAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        EmContactAttack_End(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        EmContactAttack_End(obj);
        break;
    case 7:
        EmContactAttack_End(obj);
        break;
    case ATTACH_FREE:
        EmContactAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from row attackId + 1: the hit shapes,
 * ATKF_OWNER_CONTACT and ATKF_REPORT_CONTACT (the owner learns what it
 * touched); hitDelay is counted from the monster's motion.
 */
void EmContactAttack_Setup(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    obj->state++;
    obj->visible = 1;
    obj->draw = EmContactAttack_DrawNop;
    Player_TestSetFlags(obj, ATKF_OWNER_CONTACT | ATKF_REPORT_CONTACT);
    obj->wasHit = 0;
    obj->area = em->area;
    MoveObj_StartMotionOwned((struct MoveObj *)obj, (s32) em, obj->type + 1, &EmContactAttack_RowsPtr);
    obj->hitList = EmContactAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    if (obj->hitDelay != 0) {
        obj->hitDelay += (s32) Math_Absf((s16) em->motionBlend[0] % 100) - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) obj->hitDelay < 0) {
            obj->hitDelay = 0;
        }
    }
    obj->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes, the hit window closes or
 * the monster's task leaves its run phase; some attacks keep their window
 * open.
 */
void EmContactAttack_Run(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    switch (obj->type) {
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
        obj->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != obj->ownerMotion || obj->hitPhase == 0 || em->taskPhase >= 2) {
        obj->hitPhase = 0;
        obj->state = ATTACH_END;
    } else if (obj->drawSlotBuf != NULL) {
        Vec_Copy(obj->drawSlotBuf->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void EmContactAttack_End(ObjAttach *obj)
{
    obj->hitPhase = 0;
    obj->state = ATTACH_FREE;
    obj->visible = 0;
    if (obj->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) obj->drawSlot);
    }
}

void EmContactAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

void EmContactAttack_DrawNop(ObjAttach *obj)
{
}

/*
 * Makes an ItemAttack: the item type (a row of ItemAttack_Modes and the other
 * ItemAttack tables) thrown or put down by owner, a player. throwRow picks
 * the throw's speeds (ItemAttack_SetupThrown). A thrown item starts at the
 * hand (part record 18), a placed one at the feet; nothing is made for a
 * thrown item outside the current area. Item 0x16 takes the second pool and
 * plays its sound.
 */
void ItemAttack_Create(void *owner, s32 type, s8 throwRow)
{
    Player *pl = owner;
    ObjHead *head = owner;
    s16 pool;
    ObjAttach *item;
    s8 *mode = &ItemAttack_Modes[type];

    if (*mode == ITEM_PLACED || *mode == ITEM_HOPPING || (u8)Obj_InCurrentArea(pl) != 0) {
        switch (type) {
        case 0x16:
            pool = 1;
            break;
        default:
            pool = 0;
            break;
        }
        if ((item = ObjAttach_Alloc(pool)) != NULL) {
            item->kind = ATTACH_ITEM_ATTACK;
            item->type = type;
            item->stateVar1 = throwRow;
            item->update = ItemAttack_Update;
            item->idx = head->index;
            item->isEm = head->isEm;
            item->ownerMotion = 0;
            item->owner = owner;
            item->yaw = head->rot[1];
            item->rotX = 0;
            item->rotY = head->rot[1];
            item->rotZ = 0;
            item->area = head->area;
            if (ItemAttack_HasSide[type] != 0) {
                item->side = head->side;
            }
            switch (*mode) {
            case ITEM_PLACED:
            case ITEM_HOPPING:
                item->pos[0] = head->pos[0];
                item->pos[1] = 21.0f + head->pos[1];
                item->pos[2] = head->pos[2];
                item->keep = 1;
                break;
            default:
                item->pos[0] = ((f32 *) pl->partRecs[18])[28];
                item->pos[1] = ((f32 *) pl->partRecs[18])[29];
                item->pos[2] = ((f32 *) pl->partRecs[18])[30];
                item->keep = 0;
                break;
            }
            if (item->type == 0x16 && gScene.area == item->area) {
                Sound_PlaySeAtPosChecked(1, 0x91, 0, item->pos, 1, 0);
            }
            item->velX = 0;
            item->velY = 0;
            item->velZ = 0;
            item->homingTurn = 0;
            item->gravity = 0;
            item->homingSpeed = 0;
            item->randBits = head->stateRand;
        }
    }
}

/*
 * The ItemAttack's per-frame update, by ObjAttach.state: a hit bursts items
 * 20 and 24 (ItemAttack_OnHit); a deflected item bounces off or flies back
 * (ItemAttack_Deflect).
 */
void ItemAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        ItemAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        ItemAttack_Run(obj);
        break;
    case ATTACH_END:
        ItemAttack_End(obj);
        break;
    case ATTACH_HIT:
        ItemAttack_OnHit(obj);
        break;
    case 4:
        ItemAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        ItemAttack_Deflect(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        ItemAttack_Deflect(obj);
        break;
    case 7:
        ItemAttack_End(obj);
        break;
    case ATTACH_FREE:
        ItemAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Sets the item up by its mode, then takes a draw slot (the second pool for a
 * placed item) and runs the first frame; frees the object when no slot is
 * left.
 */
void ItemAttack_Setup(ObjAttach *obj)
{
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    switch (ItemAttack_Modes[obj->type]) {
    case ITEM_PLACED:
        ItemAttack_SetupPlaced(obj);
        break;
    case ITEM_HOPPING:
        ItemAttack_SetupHopping(obj);
        break;
    default:
        ItemAttack_SetupThrown(obj);
        break;
    }
    Vec_Copy(obj->pos2, obj->pos);
    if (obj->keep == 1) {
        obj->drawSlot = DrawSlotB_Alloc();
        if (obj->drawSlot != -1) {
            obj->drawSlotBuf = DrawSlotB_Get(obj->drawSlot);
        }
    } else {
        obj->drawSlot = DrawSlot_Alloc();
        if (obj->drawSlot != -1) {
            obj->drawSlotBuf = DrawSlot_Get(obj->drawSlot);
        }
    }
    if (obj->drawSlot != -1) {
        obj->drawSlotBuf->owner = obj;
        obj->drawSlotBuf->draw = ItemAttack_Draw;
        ItemAttack_Run(obj);
    } else {
        ObjAttach_Free(obj);
    }
}

/*
 * A thrown item: its hit row, its flags (0x12 hits no player, 8 is deflected
 * by wind), and its velocity from the throw's speeds (ItemAttack_LobSpeeds
 * for 0xC-0xF and 0x15, the other table when the thrower's motion is 0x1A1).
 * Skill 0x2D throws 1.5 times as far, skill 0x47 adds an eighth to the
 * element power; stateVar0 keeps both as bits 2 and 4.
 */
void ItemAttack_SetupThrown(ObjAttach *obj)
{
    Player *owner = obj->owner;
    f32 (*speeds)[2];
    f32 *forward;
    f32 in[3];
    f32 out[3];
    f32 power;

    switch (obj->type) {
    case 0x12:
        Player_TestSetFlags(obj, ATKF_NO_PLAYER);
        break;
    case 8:
        Player_TestSetFlags(obj, ATKF_WIND_DEFLECTS);
        break;
    default:
        Player_TestSetFlags(obj, 0);
        break;
    }
    MoveObj_StartMotionOwned((struct MoveObj *)obj, (s32) owner, ItemAttack_AttackRow[obj->type], &ItemAttack_RowsPtr);
    obj->hitList = ItemAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    if (ItemAttack_HitsInFlight[obj->type] == 0) {
        obj->hitPhase = 0;
    }
    switch (obj->type) {
    default:
        if (owner->motionId[0] == 0x1A1) {
            speeds = ItemAttack_ThrowSpeedsAlt;
        } else {
            speeds = ItemAttack_ThrowSpeeds;
        }
        obj->gravity = -0.8f;
        break;
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
    case 0x15:
        speeds = ItemAttack_LobSpeeds;
        obj->gravity = -0.04f;
        break;
    }
    in[0] = 0.0f;
    in[1] = 0.0f;
    forward = &in[2];
    *forward = speeds[obj->stateVar1][0];
    obj->stateVar0 = 0;
    if (Player_HasSkill(owner, 0x2D) == 1) {
        *forward *= 1.5f;
        obj->stateVar0 |= 2;
    }
    if (Player_HasSkill(owner, 0x47) == 1) {
        obj->stateVar0 |= 4;
        if (obj->elementPower > 0) {
            power = 1.125f * obj->elementPower;
            if (power > 255.0f) {
                power = 255.0f;
            }
            obj->elementPower = power;
        }
    }
    Vec_Rotate((f32 *) out, (f32 *) in, (f32 *) owner->worldMtx);
    obj->velX = 2.0f * out[0];
    obj->velY = 2.0f * speeds[obj->stateVar1][1];
    obj->velZ = 2.0f * out[2];
    obj->homingTurn = 0;
    obj->homingSpeed = 0;
    obj->stateVar1 = 0;
}

/*
 * Item 0x16's two parts: a draw slot each from the second pool, at 5 and 1060
 * above the item.
 */
void ItemAttack_InitParts(ObjAttach *obj)
{
    AttachPart *part = (AttachPart *) obj->buf;
    s16 i;

    for (i = 0; i < 2; i++, part++) {
        part->idx = i;
        part->frame = 0;
        part->time = 0;
        part->slot = DrawSlotB_Alloc();
        part->unk0C = 0;
        if (part->slot != -1) {
            part->node = DrawSlotB_Get(part->slot);
            part->node->owner = obj;
            part->node->part = i;
            part->node->draw = ItemAttack_DrawPart;
            Vec_Copy(part->node->pos, obj->pos);
            switch (part->idx) {
            case 0:
                part->node->pos[1] += 5.0f;
                break;
            case 1:
                part->node->pos[1] += 1060.0f;
                break;
            }
        } else {
            part->node = NULL;
        }
    }
}

/*
 * A placed item: no hit of its own; aux starts at 75 (the fuse's blink).
 * Skill 0x48 sets stateVar0 1 (passed to the burst). Item 0x16 makes its
 * parts, the others a ground effect.
 */
void ItemAttack_SetupPlaced(ObjAttach *obj)
{
    void *owner = obj->owner;

    Player_TestSetFlags(obj, 0);
    MoveObj_StartMotionOwned((struct MoveObj *)obj, (s32) owner, ItemAttack_AttackRow[obj->type], &ItemAttack_RowsPtr);
    obj->hitList = 0;
    obj->hurtList = 0;
    obj->hitPhase = 0;
    if (Player_HasSkill(owner, SKILL_COMBINE_SURE) == 1) {
        obj->stateVar0 = 1;
    } else {
        obj->stateVar0 = 0;
    }
    obj->stateVar1 = 0;
    obj->aux = 0x4B;
    switch (obj->type) {
    case 0x16:
        ItemAttack_InitParts(obj);
        break;
    default:
        if (obj->area == gScene.area) {
            StageObj_K10_CreateAt(obj->pos, 0xD, 0x4B, 0xA, 1.0f);
        }
        break;
    }
}

/*
 * A hopping item: its hit row, no hit until it jumps; SKILL_COMBINE_SURE sets
 * stateVar0 1 (passed to the burst).
 */
void ItemAttack_SetupHopping(ObjAttach *obj)
{
    s32 owner;

    owner = (s32) obj->owner;
    Player_TestSetFlags(obj, 0);
    MoveObj_StartMotionOwned((struct MoveObj *)obj, owner, ItemAttack_AttackRow[obj->type], &ItemAttack_RowsPtr);
    obj->hitList = ItemAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->hitPhase = 0;
    if (Player_HasSkill((Player *) owner, SKILL_COMBINE_SURE) == 1) {
        obj->stateVar0 = 1;
    } else {
        obj->stateVar0 = 0;
    }
    obj->stateVar1 = 0;
    obj->aux = 0;
}

/*
 * Moves the item: flying back (stateVar1 0xFF), bouncing off (0xFE, ended
 * after 30 frames) or by its mode; then draws it at its position.
 */
void ItemAttack_Run(ObjAttach *obj)
{

    Vec_Copy(obj->pos2, obj->pos);
    if (obj->stateVar1 == 0xFF) {
        ItemAttack_FlyBack(obj);
    } else if (obj->stateVar1 == 0xFE) {
        if (ItemAttack_BounceOff(obj) & 0xFF) {
            obj->state = ATTACH_END;
            return;
        }
    } else {
        switch (ItemAttack_Modes[obj->type]) {
        default:
            ItemAttack_RunThrown(obj);
            break;
        case ITEM_PLACED:
            ItemAttack_RunPlaced(obj);
            break;
        case ITEM_HOPPING:
            ItemAttack_RunHopping(obj);
            break;
        }
    }
    if (obj->drawSlotBuf != NULL) {
        Vec_Copy(obj->drawSlotBuf->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->drawSlotBuf, 0x20U, 0);
    }
}

/*
 * A thrown item flies; ownerMotion counts its frames. Item 1 bursts after 13
 * frames (8 with skill 0x2D), item 9 after 16; items 0xC-0xF and 0x15 fall
 * faster after 15 frames. On the floor it ends, leaving its mark (items 0, 5,
 * 7, 11, 17 and 23 make a stage object, and 11, 17 and 23 their burst).
 */
void ItemAttack_RunThrown(ObjAttach *obj)
{
    Player *owner = obj->owner;
    f32 floorY;
    s16 fuseFrames;

    MoveObj_Integrate((struct MoveObj *)obj);
    if (obj->type == 1) {
        obj->hitFrames = 0;
    } else {
        obj->hitFrames = 0x63;
    }
    floorY = AreaGrid2_GetFloorYFiltered(obj->pos);
    obj->ownerMotion++;
    switch (obj->type) {
    case 1:
        fuseFrames = (obj->stateVar0 & 2) ? 8 : 13;
        if (fuseFrames <= obj->ownerMotion) {
            if (obj->side != 0) {
                Explosion_Create(obj->owner, obj->pos, 0x1B);
            }
            if ((u8)Obj_InCurrentArea(owner) != 0) {
                StageObj_K0E_CreateOwned(obj->pos, 4, 1.0f, owner);
                Sound_PlaySeAtPosChecked(1, 0x47, 0, obj->pos, 0xF, 0);
            }
            obj->state = ATTACH_END;
            return;
        }
        break;
    case 9:
        if (obj->ownerMotion >= 16) {
            Explosion_Create(obj->owner, obj->pos, 0xD);
            if (obj->area == gScene.area) {
                StageObj_K0F_CreateAt(obj->pos, 4, 0, 0.5f);
            }
            obj->state = ATTACH_END;
            return;
        }
        break;
    case 12:
    case 13:
    case 14:
    case 15:
    case 21:
        switch (obj->stateVar1) {
        case 0:
            if (obj->ownerMotion > 15) {
                obj->stateVar1++;
                obj->gravity -= 1.0f;
            }
            break;
        case 1:
            break;
        }
        break;
    }
    if (obj->pos[1] <= floorY) {
        if (obj->pos[1] > floorY - 100.0f) {
            obj->pos[1] = floorY;
        }
        switch (obj->type) {
        case 0:
            StageObj_K0C_CreateFromAttach(obj, 1, 0);
            break;
        case 5:
            StageObj_K0C_CreateFromAttach(obj, 1, 1);
            break;
        case 7:
            if (obj->area == gScene.area) {
                StageObj_K0C_Create(obj->pos, 0, 5, 2);
            }
            break;
        case 11:
            if (obj->area == gScene.area) {
                StageObj_K0C_Create(obj->pos, 0, 5, 3);
                Explosion_Create(obj->owner, obj->pos, 0x13);
            }
            break;
        case 17:
            if (obj->area == gScene.area) {
                StageObj_K0C_Create(obj->pos, 0, 5, 4);
                Explosion_Create(obj->owner, obj->pos, 0x1A);
            }
            break;
        case 23:
            if (obj->area == gScene.area) {
                StageObj_K0C_Create(obj->pos, 0, 5, 5);
                Explosion_Create(obj->owner, obj->pos, 0x24);
            }
            break;
        }
        obj->hitPhase = 0;
        obj->visible = 0;
        obj->state = ATTACH_END;
    }
}

/*
 * Steps item 0x16's two parts: frames 1-4 in a loop, the low part restarting
 * after 16 frames, the high one holding at 65; draws them.
 */
void ItemAttack_AnimParts(ObjAttach *obj)
{
    AttachPart *part = (AttachPart *) obj->buf;
    s16 i;

    for (i = 0; i < 2; i++, part++) {
        part->frame++;
        part->time++;
        switch (part->idx) {
        case 0:
            if (part->frame > 4) {
                part->frame = 1;
            }
            if (part->time > 16) {
                part->frame = 1;
            }
            break;
        case 1:
            if (part->frame > 4) {
                part->frame = 1;
            }
            if (part->time > 0x41) {
                part->time = 0x41;
            }
            break;
        }
        if (part->frame > 0 && part->time > 0 && part->node != NULL) {
            OT_AddByDepth(gDrawOtSorted, part->node, 0x40, 0);
        }
    }
}

/*
 * A placed item's fuse: it bursts after 75 frames (item 0x10 a larger burst,
 * 0x16 its own effects). Until then it ticks every 15 frames, smokes, and
 * from frame 55 blinks faster (aux, scaled by 0.8 each blink, is the gap).
 */
void ItemAttack_RunPlaced(ObjAttach *obj)
{
    s16 frames;

    if (++obj->ownerMotion >= 0x4B) {
        if (obj->area == gScene.area) {
            switch (obj->type) {
            case 0x10:
                obj->visible ^= 1;
                StageObj_K0E_CreateScaled(obj->pos, 0, 1.5f);
                Explosion_CreateAt(obj->pos, 0x14, obj->area, obj->stateVar0);
                break;
            case 0x16:
                StageObj_K29_CreateAtDefault(obj->pos, 0xD, 1.5f);
                StageObj_K29_CreateAtDefault(obj->pos, 0xE, 1.5f);
                Explosion_CreateAt(obj->pos, 0x21, obj->area, 0);
                Sound_PlaySeAtPosChecked(1, 0x93, 0, obj->pos, 1, 0);
                break;
            default:
                obj->visible ^= 1;
                StageObj_K0E_CreateScaled(obj->pos, 0, 1.0f);
                Explosion_CreateAt(obj->pos, 0, obj->area, obj->stateVar0);
                break;
            }
        }
        obj->state = ATTACH_END;
        return;
    }
    switch (obj->type) {
    default:
    case 4:
    case 0x10:
        frames = obj->ownerMotion;
        if (frames >= 0x37) {
            obj->visible ^= 1;
            break;
        }
        if (obj->stateVar1 == 0) {
            if (0.8f * obj->aux > (f32) (0x37 - frames)) {
                obj->visible ^= 1;
                if (0x37 - obj->ownerMotion > 2) {
                    obj->stateVar1 = 2;
                }
            }
        } else if (--obj->stateVar1 == 0) {
            obj->visible ^= 1;
            obj->aux = 0x37 - obj->ownerMotion;
        }
        if (obj->area == gScene.area) {
            if (obj->ownerMotion == 1) {
                Sound_PlaySeAtPosChecked(1, 0x24, 0, obj->pos, 1, 0);
            }
            frames = obj->ownerMotion;
            if (frames != 0 && frames % 15 == 0) {
                Sound_PlaySeAtPosChecked(1, 0x25, 0, obj->pos, 1, 0);
            }
            if (obj->ownerMotion & 1) {
                Effect02_SpawnRandomAngle(0xC000, 0, 2, obj->pos);
            }
        }
        break;
    case 0x16:
        if (obj->ownerMotion == 0x3E && gScene.area == obj->area) {
            Sound_PlaySeAtPosChecked(1, 0x92, 0, obj->pos, 1, 0);
        }
        ItemAttack_AnimParts(obj);
        break;
    }
}

/*
 * A hopping item: after 26 frames it jumps (item 0x18 also forward along its
 * yaw), opens its hit 7 frames later, spins, and bursts 18 frames after the
 * jump.
 */
void ItemAttack_RunHopping(ObjAttach *obj)
{
    f32 effectPos[3];
    u16 yaw;
    u16 pitch;
    s16 frames;

    obj->ownerMotion++;
    MoveObj_Integrate((struct MoveObj *)obj);
    switch (obj->stateVar1) {
    case 0:
        if (obj->ownerMotion >= 0x1A) {
            obj->stateVar1++;
            obj->ownerMotion = 0;
            obj->velY = 50.0f;
            obj->gravity = -0.2f;
            if (obj->type == 0x18) {
                obj->velZ = 30.0f;
                Vec_RotateY(&obj->velX, (6.2831855f * obj->rotY) / 65536.0f);
            }
            if (obj->area == gScene.area) {
                effectPos[0] = obj->pos[0];
                effectPos[1] = obj->pos[1] - 20.0f;
                effectPos[2] = obj->pos[2];
                Effect0D_SpawnAt(effectPos, 7, 0.5f);
            }
        }
        break;
    case 1:
        if (obj->ownerMotion >= 7) {
            obj->stateVar1++;
            obj->hitPhase = 1;
        }
    case 2:
        obj->rotX += 0x2000;
        obj->rotY += 0x2000;
        if (obj->area == gScene.area) {
            frames = obj->ownerMotion;
            if (frames != 0 && frames % 15 == 0) {
                Sound_PlaySeAtPosChecked(1, 0x25, 0, obj->pos, 1, 0);
            }
            if (obj->ownerMotion & 1) {
                Vec_DirToAngles(&obj->velX, (s16 *) &yaw, (s16 *) &pitch);
                Effect02_SpawnRandomAngle(yaw + 0x8000, pitch, 2, obj->pos);
            }
        }
        if (obj->ownerMotion >= 0x12) {
            if (obj->area == gScene.area) {
                StageObj_K0E_CreateScaled(obj->pos, 0, 1.0f);
                Explosion_CreateAt(obj->pos, 0, obj->area, obj->stateVar0);
            }
            obj->state = ATTACH_END;
        }
        break;
    }
    if (obj->ownerMotion == 1 && obj->area == gScene.area) {
        Sound_PlaySeAtPosChecked(1, 0x24, 0, obj->pos, 1, 0);
    }
}

/* A hit: items 20 and 24 burst where they are; every item then ends. */
void ItemAttack_OnHit(ObjAttach *obj)
{

    switch (obj->type) {
    case 20:
        if (obj->area == gScene.area) {
            StageObj_K0E_CreateScaled(obj->pos, 0, 1.0f);
            Explosion_CreateAt(obj->pos, 0, obj->area, obj->stateVar0);
        }
        break;
    case 24:
        if (obj->area == gScene.area) {
            StageObj_K0E_CreateScaled(obj->pos, 0, 1.0f);
            Explosion_CreateAt(obj->pos, 0, obj->area, obj->stateVar0);
        }
        break;
    }
    ItemAttack_End(obj);
}

/*
 * A deflected item (ATTACH_DEFLECTED, ATTACH_DEFLECTED_AURA). Either it
 * bounces off at a random angle (stateVar1 0xFE), or, when func_364AE0 says
 * so, the monster in targetEm bats it back: it becomes that monster's attack
 * (row 0xC) flying at its thrower's chest (stateVar1 0xFF).
 */
void ItemAttack_Deflect(ObjAttach *obj)
{
    Em *owner = obj->owner;
    Em *batter = obj->targetEm;
    f32 aim[3];
    f32 dir[3];
    u16 yaw;
    u16 pitch;

    obj->step = 0;
    obj->stateVar0 = 0;
    obj->ownerMotion = 0;
    obj->side = 0;
    if (func_364AE0(obj, obj->randBits) == 0) {
        obj->stateVar1 = 0xFE;
        obj->hitPhase = 0;
        Vec_Diff(dir, obj->pos2, obj->pos);
        Vec_DirToAngles(dir, (s16 *) &yaw, (s16 *) &pitch);
        yaw += -(Rand_Next(1) & 0x7FF) - 0x200;
        pitch += (Rand_Next(1) & 0x1FFF) - 0x1000;
        obj->velX = 0.0f;
        obj->velY = Rand_Next(1) & 7;
        obj->velZ = 7.0f + (Rand_Next(1) & 3);
        obj->homingTurn = 0.0f;
        obj->gravity = -1.5f - (Rand_Next(1) & 0x1F) / 32.0f;
        obj->homingSpeed = 0.0f;
        Vec_RotateX(&obj->velX, (6.2831855f * yaw) / 65536.0f);
        Vec_RotateY(&obj->velX, (6.2831855f * pitch) / 65536.0f);
    } else {
        obj->stateVar1 = 0xFF;
        obj->idx = batter->index;
        obj->isEm = batter->isEm;
        obj->owner = batter;
        Player_TestSetFlags(obj, 0);
        MoveObj_StartMotionOwned((struct MoveObj *)obj, (s32) batter, 0xC, &ItemAttack_RowsPtr);
        obj->hitList = ItemAttack_HitShapes[obj->hitListNo];
        obj->hurtList = 0;
        Vec_Copy(aim, &owner->posX);
        aim[1] += 150.0f;
        Vec_Diff(dir, aim, obj->pos);
        Vec_DirToAngles(dir, (s16 *) &yaw, (s16 *) &pitch);
        obj->rotX = yaw;
        obj->rotY = pitch;
        obj->yaw = obj->rotY;
        obj->velX = 0.0f;
        obj->velY = 0.0f;
        obj->velZ = 70.0f;
        Vec_RotateX(&obj->velX, (6.2831855f * obj->rotX) / 65536.0f);
        Vec_RotateY(&obj->velX, (6.2831855f * obj->rotY) / 65536.0f);
        obj->homingTurn = 0.0f;
        obj->gravity = -0.4f;
        obj->homingSpeed = 0.0f;
        Vec_Copy(obj->pos2, obj->pos);
        if (gScene.area == obj->area) {
            Sound_PlaySeAtPosChecked(1, 0x96, 0, obj->pos, 6, 0);
        }
    }
    obj->state = ATTACH_RUN;
    ItemAttack_Run(obj);
}

/*
 * A batted-back item flies with its window open until it reaches the floor.
 */
void ItemAttack_FlyBack(ObjAttach *obj)
{
    obj->hitFrames = 99;
    if (++obj->ownerMotion < 0) {
        MoveObj_AddVelocity((struct MoveObj *)obj);
    } else {
        MoveObj_Integrate((struct MoveObj *)obj);
    }
    if (obj->pos[1] <= AreaGrid2_GetFloorYFiltered(obj->pos)) {
        obj->hitFrames = 0;
        obj->state = ATTACH_END;
    }
}

/*
 * A bouncing item tumbles for 30 frames without a hit; returns 1 when it is
 * over.
 */
s32 ItemAttack_BounceOff(ObjAttach *obj)
{
    s16 frames;

    obj->hitPhase = 0;
    frames = obj->ownerMotion + 1;
    obj->ownerMotion = frames;
    if (frames > 30) {
        return 1;
    }
    MoveObj_Integrate((struct MoveObj *)obj);
    obj->rotX = (s32) (obj->rotX - 0x400);
    obj->rotY = (s32) (obj->rotY - 0x400);
    return 0;
}

/* Frees item 0x16's two parts' draw slots. */
void ItemAttack_FreeDrawSlots(ObjAttach *obj)
{
    AttachPart *part = (AttachPart *) obj->buf;
    s16 i;

    for (i = 0; i < 2; i++) {
        if (part->node != NULL) {
            DrawSlotB_Free(part->slot);
        }
        part++;
    }
}

/*
 * Hides the item, frees its draw slot (and item 0x16's parts) and frees it on
 * the next update.
 */
void ItemAttack_End(ObjAttach *obj)
{
    obj->hitPhase = 0;
    obj->state = ATTACH_FREE;
    obj->visible = 0;
    if (obj->drawSlotBuf != NULL) {
        if (obj->keep == 1) {
            DrawSlotB_Free((s32) obj->drawSlot);
        } else {
            DrawSlot_Free((s32) obj->drawSlot);
        }
    }
    switch (obj->type) {
    case 0x16:
        ItemAttack_FreeDrawSlots(obj);
        break;
    }
}

void ItemAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

/*
 * Draws one of item 0x16's parts while it is shown in the current area: part
 * 0 faces the camera, part 1 turns with the camera's yaw and steps through
 * the four quarters of its texture; both are scaled and coloured by their
 * keys.
 */
void ItemAttack_DrawPart(DrawSlot *slot)
{
    f32 mtx[16];
    f32 uvMtx[16];
    f32 scale[3];
    f32 rot[3];
    u32 col;
    ObjAttach *obj = slot->owner;
    AttachPart *part = (AttachPart *) obj->buf + slot->part;
    ModelMat *tex;
    Model *stage = gCommonModels[0];
    ModelMatSet *mdl;
    u8 facing;

    if (gScene.area == obj->area && obj->visible != 0 && part->frame > 0) {
        tex = stage->mats;
        switch (part->idx) {
        case 0:
            Key_LerpVec2(ItemAttack_ScaleKeysA, scale, part->frame);
            Color_EvalKeys((s32 *) ItemAttack_ColorKeysA, part->time, &col);
            facing = 1;
            break;
        case 1:
            Key_LerpVec2(ItemAttack_ScaleKeysB, scale, part->frame);
            Color_EvalKeys((s32 *) ItemAttack_ColorKeysB, part->time, &col);
            Mtx_Translation(uvMtx, 0.25f * (part->frame & 1), 0.0625f * ((part->frame >> 1) & 1), 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            facing = 2;
            break;
        default:
            return;
        }
        Mtx_FromScaleRotTrans(scale, rot, slot->pos, 0, mtx);
        mdl = &stage->matSets[ItemAttack_PartModels[part->idx]];
        switch (facing) {
        case 0:
            break;
        case 1:
            Vec_RotateSelf(mtx, gCamMatrix);
            break;
        case 2:
            Vec_RotateSelf(mtx, gCamYawMtx);
            break;
        }
        Gs_SetState(0x6C, 0);
        Effect_DrawModel(mdl, mtx, col, 2, tex);
        Gs_SetState(0x6C, 1);
    }
}

/*
 * Draws the item's model (ItemAttack_Models[type]) at its position: a thrown
 * item tumbles (0xC-0xF and 0x15 keep their yaw), a placed one turns by its
 * yaw, a hopping one by its spin.
 */
void ItemAttack_Draw(DrawSlot *slot)
{
    f32 mtx[16];
    Model *stage = gCommonModels[0];
    ObjAttach *obj = slot->owner;
    ModelMat *mats;
    ModelMatSet *mdl;
    ModelMat *mat;
    s32 i;

    if (obj->area == gScene.area && stage != NULL && stage->active != 0) {
        mats = stage->mats;
        Mtx_Translation(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        switch (ItemAttack_Modes[obj->type]) {
        case ITEM_THROWN:
            switch (obj->type) {
            case 0xC:
            case 0xD:
            case 0xE:
            case 0xF:
            case 0x15:
                Mtx_RotateXYZ(mtx, 0.0f, (6.2831855f * (u16) obj->yaw) / 65536.0f, 0.0f);
                break;
            default:
                Mtx_RotateXYZ(mtx, (6.2831855f * ((obj->ownerMotion << 11) & 0xFFFF)) / 65536.0f, 0.0f, 0.0f);
                break;
            }
            break;
        case ITEM_PLACED:
            Mtx_RotateY(mtx, (6.2831855f * obj->rotY) / 65536.0f);
            break;
        case ITEM_HOPPING:
            Mtx_RotateX(mtx, (6.2831855f * obj->rotX) / 65536.0f);
            Mtx_RotateY(mtx, (6.2831855f * obj->rotY) / 65536.0f);
            break;
        }
        mdl = stage->matSets + ItemAttack_Models[obj->type];
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        if (mdl != NULL && mdl->id != -1) {
            for (i = 0; i < mdl->count; i++) {
                mat = &mats[mdl->mat[i]];
                if (i == 1 && ItemAttack_Models[obj->type] == 0x37) {
                    mat->alpha = 0.0f;
                }
                mat->rgb[0] = 1.0f;
                mat->rgb[1] = 1.0f;
                mat->rgb[2] = 1.0f;
                Gs_SetState((u8) RS_MATERIAL(i), (s32) mat);
            }
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Makes a FatalisAttack for em with the row attackId of FatalisAttack_Rows,
 * if a player is in em's area.
 */
void FatalisAttack_Create(Em *em, s8 attackId)
{
    ObjAttach *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((obj = ObjAttach_Alloc(0)) != NULL) {
            obj->kind = ATTACH_FATALIS_ATTACK;
            obj->type = attackId;
            obj->update = FatalisAttack_Update;
            obj->idx = em->index;
            obj->isEm = em->isEm;
            obj->ownerMotion = em->motionId[0];
            obj->owner = em;
            obj->yaw = em->rotY;
            obj->rotX = em->rotX;
            obj->rotY = em->rotY;
            obj->rotZ = em->rotZ;
            obj->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/*
 * FatalisAttack_Create at the stage object src's position, for src's owner
 * (without the area check).
 */
void FatalisAttack_CreateOnObj(StageObj *src, s8 attackId)
{
    ObjAttach *obj;
    Em *em;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        em = src->owner;
        obj->kind = ATTACH_FATALIS_ATTACK;
        obj->type = attackId;
        obj->update = FatalisAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = em->rotX;
        obj->rotY = em->rotY;
        obj->rotZ = em->rotZ;
        obj->pos[0] = src->pos[0];
        obj->pos[1] = src->pos[1];
        obj->pos[2] = src->pos[2];
        em->hitPending = 0;
        em->contactHit = 0;
        obj->step = 1;
    }
}

/* The FatalisAttack's per-frame update, by ObjAttach.state. */
void FatalisAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        FatalisAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        FatalisAttack_Run(obj);
        break;
    case ATTACH_END:
        FatalisAttack_End(obj);
        break;
    case ATTACH_HIT:
        FatalisAttack_Run(obj);
        break;
    case 4:
        FatalisAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        FatalisAttack_End(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        FatalisAttack_End(obj);
        break;
    case 7:
        FatalisAttack_End(obj);
        break;
    case ATTACH_FREE:
        FatalisAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row; attack 3 counts hitDelay from the
 * monster's second motion layer.
 */
void FatalisAttack_Setup(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];
    s32 blendFrames;
    s32 motionFrame;

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    Player_TestSetFlags(obj, ATKF_OWNER_CONTACT);
    MoveObj_StartMotionOwned((struct MoveObj *)obj, (s32) em, obj->type, &FatalisAttack_RowsPtr);
    obj->hitList = FatalisAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->area = em->area;
    if (obj->hitDelay != 0) {
        if (obj->type == 3) {
            blendFrames = Math_Absf((s16) em->motionBlend[1] % 100);
            motionFrame = Math_Absf(em->motionFrame[1]);
        } else {
            blendFrames = Math_Absf((s16) em->motionBlend[0] % 100);
            motionFrame = Math_Absf(em->motionFrame[0]);
        }
        obj->hitDelay += blendFrames - motionFrame - 1;
        if ((s8) obj->hitDelay < 0) {
            obj->hitDelay = 0;
        }
    }
    obj->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the window closes.
 * Attack 0x1E lasts while the monster's state is group 3's 0xC, 0xD or 0x14:
 * its window opens for 60 frames once the motion is 0x40C.
 */
void FatalisAttack_Run(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    switch (obj->type) {
    case 2:
        obj->hitFrames = 0x63;
        break;
    }
    if (obj->type == 0x1E) {
        switch (obj->stateVar0) {
        case 0:
            if (em->motionId[0] == 0x40C) {
                obj->stateVar0++;
                obj->hitFrames = 0x3C;
            } else {
                obj->hitFrames = 0x63;
            }
            break;
        case 1:
            if (em->motionId[0] != 0x40C || obj->hitPhase == 0) {
                obj->hitPhase = 0;
                obj->state = ATTACH_END;
            }
            break;
        }
        if (em->stateGroup != 3 || ((u32) (em->state - 0xC) > 1 && em->state != 0x14) || em->stateGroup == 4 || em->stateGroup == 5) {
            obj->hitPhase = 0;
            obj->state = ATTACH_END;
        }
    } else if (em->motionId[0] != obj->ownerMotion || obj->hitPhase == 0) {
        obj->hitPhase = 0;
        obj->state = ATTACH_END;
    }
    if (obj->drawSlotBuf != NULL) {
        Vec_Copy(obj->drawSlotBuf->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->drawSlotBuf, 0x20, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void FatalisAttack_End(ObjAttach *obj)
{
    obj->hitPhase = 0;
    obj->state = ATTACH_FREE;
    obj->visible = 0;
    if (obj->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) obj->drawSlot);
    }
}

void FatalisAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

/*
 * Makes a LaoShanAttack for em with the row attackId of LaoShanAttack_Rows,
 * if a player is in em's area.
 */
void LaoShanAttack_Create(Em *em, s8 attackId)
{
    ObjAttach *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((obj = ObjAttach_Alloc(0)) != NULL) {
            obj->kind = ATTACH_LAOSHAN_ATTACK;
            obj->type = attackId;
            obj->update = LaoShanAttack_Update;
            obj->idx = em->index;
            obj->isEm = em->isEm;
            obj->ownerMotion = em->motionId[0];
            obj->owner = em;
            obj->yaw = em->rotY;
            obj->rotX = em->rotX;
            obj->rotY = em->rotY;
            obj->rotZ = em->rotZ;
            obj->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
            obj->step = 0;
            obj->hitSpan = 0;
        }
    }
}

/*
 * LaoShanAttack_Create with a hit window of span frames (halved for attack
 * 0x47, the only user).
 */
void LaoShanAttack_CreateSpan(Em *em, s32 attackId, s32 span)
{
    ObjAttach *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((obj = ObjAttach_Alloc(0)) != NULL) {
            obj->kind = ATTACH_LAOSHAN_ATTACK;
            obj->type = attackId;
            obj->update = LaoShanAttack_Update;
            obj->idx = em->index;
            obj->isEm = em->isEm;
            obj->ownerMotion = em->motionId[0];
            obj->owner = em;
            obj->yaw = em->rotY;
            obj->rotX = em->rotX;
            obj->rotY = em->rotY;
            obj->rotZ = em->rotZ;
            obj->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
            obj->step = 0;
            obj->hitSpan = span;
        }
    }
}

/* LaoShanAttack_Create at pos (without the area check). */
void LaoShanAttack_CreateAt(Em *em, f32 *pos, s8 attackId)
{
    ObjAttach *obj;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_LAOSHAN_ATTACK;
        obj->type = attackId;
        obj->update = LaoShanAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = 0;
        obj->rotY = 0;
        obj->rotZ = 0;
        obj->side = *(u8 *) &em->side;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
        em->hitPending = 0;
        em->contactHit = 0;
        obj->step = 1;
        obj->hitSpan = 0;
    }
}

/*
 * LaoShanAttack_Create at the stage object src's position, for src's owner
 * (without the area check).
 */
void LaoShanAttack_CreateOnObj(StageObj *src, s8 attackId)
{
    ObjAttach *obj;
    Em *em;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        em = src->owner;
        obj->kind = ATTACH_LAOSHAN_ATTACK;
        obj->type = attackId;
        obj->update = LaoShanAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = em->rotX;
        obj->rotY = em->rotY;
        obj->rotZ = em->rotZ;
        obj->pos[0] = src->pos[0];
        obj->pos[1] = src->pos[1];
        obj->pos[2] = src->pos[2];
        em->hitPending = 0;
        em->contactHit = 0;
        obj->step = 1;
        obj->hitSpan = 0;
    }
}

/* The LaoShanAttack's per-frame update, by ObjAttach.state. */
void LaoShanAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        LaoShanAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        LaoShanAttack_Run(obj);
        break;
    case ATTACH_END:
        LaoShanAttack_End(obj);
        break;
    case ATTACH_HIT:
        LaoShanAttack_End(obj);
        break;
    case 4:
        LaoShanAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        LaoShanAttack_End(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        LaoShanAttack_End(obj);
        break;
    case 7:
        LaoShanAttack_End(obj);
        break;
    case ATTACH_FREE:
        LaoShanAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row; hitDelay is counted from the monster's
 * motion.
 */
void LaoShanAttack_Setup(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    obj->state++;
    obj->visible = 1;
    obj->draw = LaoShanAttack_DrawNop;
    Player_TestSetFlags(obj, ATKF_OWNER_CONTACT);
    MoveObj_StartMotionOwned((struct MoveObj *)obj, (s32) em, obj->type, &LaoShanAttack_RowsPtr);
    obj->hitList = LaoShanAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->area = em->area;
    if (obj->type == 0x47) {
        obj->hitFrames = obj->hitSpan / 2;
    }
    if (obj->hitDelay != 0) {
        obj->hitDelay += (s32) Math_Absf((s16) em->motionBlend[0] % 100) - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) obj->hitDelay < 0) {
            obj->hitDelay = 0;
        }
    }
    obj->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes.
 */
void LaoShanAttack_Run(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    if (em->motionId[0] != obj->ownerMotion || obj->hitPhase == 0) {
        obj->hitPhase = 0;
        obj->state = ATTACH_END;
    }
    if (obj->drawSlotBuf != NULL) {
        Vec_Copy(obj->drawSlotBuf->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void LaoShanAttack_End(ObjAttach *obj)
{
    obj->hitPhase = 0;
    obj->state = ATTACH_FREE;
    obj->visible = 0;
    if (obj->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) obj->drawSlot);
    }
}

void LaoShanAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

void LaoShanAttack_DrawNop(ObjAttach *obj)
{
}

/*
 * The bowgun's shot: the muzzle's flash and smoke by the ammunition kind
 * (ammo with bit 0x80 names it, else pl->ammoType; not for the local player
 * while aiming), then the shot itself (BowgunShot_Create with pl->ammoType).
 * bone is the gun bone the effects start from.
 */
void Bowgun_FireShot(Player *pl, s32 ammo, s32 bone, f32 scale)
{
    f32 launchPos[3];
    s32 launchAngles[3];
    u8 kind;

    if ((ammo & 0x80) == 0) {
        kind = pl->ammoType;
    } else {
        kind = ammo & 0x7F;
    }
    if ((pl->index != gScene.localPlayer || CamCtl_IsAiming(pl) == 0) && (u8)Obj_InCurrentArea(pl) == 1) {
        switch (kind) {
        default:
            StageObj_K12_CreateOwned((s32) pl, bone, 0);
            StageObj_K12_CreateOwned((s32) pl, bone, 1);
            StageObj_K12_CreateOwned((s32) pl, bone, 2);
            break;
        case 20:
        case 21:
            StageObj_K12_CreateOnEm(pl, bone, 5, 0);
            StageObj_K12_CreateOnEm(pl, bone, 6, 0);
            break;
        case 26:
        case 27:
        case 28:
            StageObj_K12_CreateOnEm(pl, bone, 5, 3);
            StageObj_K12_CreateOnEm(pl, bone, 6, 3);
            break;
        case 22:
        case 23:
            StageObj_K12_CreateOnEm(pl, bone, 5, 1);
            StageObj_K12_CreateOnEm(pl, bone, 6, 1);
            break;
        case 24:
        case 25:
            StageObj_K12_CreateOnEm(pl, bone, 5, 2);
            StageObj_K12_CreateOnEm(pl, bone, 6, 2);
            break;
        case 29:
            StageObj_K12_CreateOnEm(pl, bone, 5, 8);
            StageObj_K12_CreateOnEm(pl, bone, 6, 8);
            break;
        case 30:
            StageObj_K12_CreateOnEm(pl, bone, 5, 6);
            StageObj_K12_CreateOnEm(pl, bone, 6, 6);
            break;
        case 31:
            StageObj_K12_CreateOnEm(pl, bone, 5, 7);
            StageObj_K12_CreateOnEm(pl, bone, 6, 7);
            break;
        case 19:
            BowgunShot_BoneLaunchPos(launchPos, pl, launchAngles, bone);
            StageObj_K12_CreateOnEm(pl, bone, 5, 9);
            StageObj_K0F_CreateNoOwner(launchPos, 8, 1, (u16) pl->rot[1], 0.25f);
            break;
        case 17:
            BowgunShot_BoneLaunchPos(launchPos, pl, launchAngles, bone);
            StageObj_K12_CreateOnEm(pl, bone, 5, 0xA);
            StageObj_K0F_CreateNoOwner(launchPos, 9, 1, (u16) pl->rot[1], 0.25f);
            break;
        case 16:
            BowgunShot_BoneLaunchPos(launchPos, pl, launchAngles, bone);
            StageObj_K12_CreateOnEm(pl, bone, 5, 0xB);
            StageObj_K0F_CreateNoOwner(launchPos, 2, 1, (u16) pl->rot[1], 0.4f);
            break;
        case 15:
            StageObj_K12_CreateOnEm(pl, bone, 5, 0xC);
            StageObj_K12_CreateOnEm(pl, bone, 6, 0xC);
            break;
        case 18:
            BowgunShot_BoneLaunchPos(launchPos, pl, launchAngles, bone);
            StageObj_K12_CreateOnEm(pl, bone, 5, 0xD);
            StageObj_K29_CreateAt(launchPos, 0xC, 1, 0.4f);
            break;
        }
    }
    BowgunShot_Create(pl, pl->ammoType, bone, scale);
}

/*
 * Makes a BowgunShot of ammunition kind (a row of gBowgunShotParams) for pl;
 * bone goes to aux, the gun bone it starts from; scale is the shot's power
 * scale.
 */
void BowgunShot_Create(Player *pl, u8 kind, s16 bone, f32 scale)
{
    ObjAttach *obj;
    BowgunShotBuf *buf;

    if ((obj = ObjAttach_Alloc(1)) != NULL) {
        buf = (BowgunShotBuf *) obj->buf;
        obj->kind = ATTACH_BOWGUN_SHOT;
        obj->type = kind;
        obj->aux = bone;
        obj->update = BowgunShot_Update;
        obj->idx = pl->index;
        obj->isEm = pl->isEm;
        obj->owner = pl;
        obj->yaw = pl->rot[1];
        obj->randBits = pl->stateRand;
        obj->stateVar1 = 0;
        obj->area = pl->area;
        obj->side = *(u8 *) &pl->side;
        buf->scale = scale;
    }
}

/*
 * Splits a shot into pellets (the scatter shot): params gives their kind and
 * count; each copies the shot's flight with its own random bits, and power
 * goes to the source's buffer (the source's, not the new pellet's).
 */
void BowgunShot_Scatter(ObjAttach *obj, u8 *params, f32 power)
{
    s16 i;
    u8 kind = params[0];
    s32 randMul;
    s16 count = *(s16 *) (params + 2);
    ObjAttach *pellet;
    BowgunShotBuf *buf;

    BowgunShot_PlaySe(obj, 0);
    i = 0;
    if (i < count) {
        randMul = 1;
        do {
            if ((pellet = ObjAttach_Alloc(1)) != NULL) {
                buf = (BowgunShotBuf *) obj->buf;
                pellet->kind = ATTACH_BOWGUN_SHOT;
                pellet->type = kind;
                pellet->aux = obj->aux;
                pellet->update = BowgunShot_Update;
                pellet->idx = ((Em *) obj->owner)->index;
                pellet->isEm = ((Em *) obj->owner)->isEm;
                pellet->owner = obj->owner;
                pellet->yaw = *(u16 *) &obj->yaw;
                pellet->rotX = obj->rotX;
                pellet->rotY = obj->rotY;
                pellet->pos[0] = obj->pos[0];
                pellet->pos[1] = obj->pos[1];
                pellet->pos[2] = obj->pos[2];
                pellet->velX = obj->velX;
                pellet->velY = obj->velY;
                pellet->velZ = obj->velZ;
                pellet->randBits = obj->randBits * randMul;
                pellet->stateVar1 = i;
                pellet->area = obj->area;
                pellet->side = obj->side;
                /* the source's buffer, not the new object's */
                buf->scale = power;
            }
            i++;
            randMul += 2;
        } while (i < count);
    }
}

extern f32 D_0045EB04[];
extern f32 D_0045EB24[];
extern f32 D_006AEE04[];
/*
 * Sets a shot up from its row of gBowgunShotParams: the weapon's level and
 * skills, the affinity's roll (crit 1 or a weak 2), the power, and the launch
 * speed (the level row's, plus the level bits' extras).
 */
void BowgunShot_Init(ObjAttach *obj)
{
    f32 mtx[16];
    BowgunShotBuf *buf = (BowgunShotBuf *) obj->buf;
    BowgunShotParam *param = &gBowgunShotParams[obj->type];
    Player *pl = obj->owner;
    u8 *levelRow;
    u8 bits;
    s16 affinity;
    f32 speed;
    f32 speedRand;
    f32 gravityRand;

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->ownerMotion = 0;
    Player_TestSetFlags(obj, BowgunShot_PlayerFlags[obj->type]);
    MoveObj_StartMotionOwned((struct MoveObj *)obj, (s32) pl, obj->type + 1, &BowgunShot_RowsPtr);
    buf->crit = 0;
    if (obj->type != 0x20) {
        BowgunShot_InitWeapon(buf, pl);
        BowgunShot_InitSkills(buf, pl);
        affinity = Player_GetWeaponStat(pl, WSTAT_AFFINITY);
        if (affinity < 0) {
            if (obj->randBits % 100 < -affinity) {
                buf->crit = 2;
            }
        } else if (affinity > obj->randBits % 100) {
            buf->crit = 1;
        }
        BowgunShot_CalcPower(obj);
        levelRow = buf->levelRow;
        bits = buf->level;
        speed = *(f32 *) (levelRow + 4);
        speedRand = *(f32 *) (levelRow + 8);
        gravityRand = *(f32 *) (levelRow + 0xC);
        if (bits & 0x10) {
            speed += D_006AEE04[0];
            speedRand += D_006AEE04[1];
            gravityRand += D_006AEE04[2];
        }
        if (bits & 0x20) {
            if (buf->unk27 == 0) {
                speed += D_0045EB24[0];
                speedRand += D_0045EB24[1];
                gravityRand += D_0045EB24[2];
            } else {
                speed += D_0045EB04[0];
                speedRand += D_0045EB04[1];
                gravityRand += D_0045EB04[2];
            }
        }
        bits = buf->skills;
        speed *= param->speed;
        speedRand *= param->speedRand;
        gravityRand *= param->gravityRand;
        if (bits & 0x10) {
            speedRand *= 2.0f;
            gravityRand *= 2.0f;
        }
        if (bits & 0x20) {
            speedRand *= 0.5f;
            gravityRand *= 0.5f;
        }
    } else {
        speed = param->speed;
        speedRand = param->speedRand;
        gravityRand = param->gravityRand;
    }
    buf->power = obj->power;
    obj->hitList = BowgunShot_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->step = 0;
    obj->stateVar0 = 0;
    buf->bounced = 0;
    buf->stage = 0;
    buf->flyStep = 0;
    buf->unk03 = 0;
    obj->homingTurn = 0.0f;
    obj->homingSpeed = 0.0f;
    switch (param->kind) {
    case 4:
    default:
    case 0:
    case 1:
        if (param->model == 0x62) {
            buf->bounced = 1;
            obj->rotZ = Rand_Next(1);
            if (param->kind == 4) {
                obj->hitPhase = 0;
            }
        }
        BowgunShot_BoneLaunchPos(obj->pos, pl, &obj->rotX, obj->aux);
        obj->velX = 0.0f;
        obj->velY = 0.0f;
        obj->velZ = speed + speedRand * BowgunShot_BitsToFrac(obj->randBits, 0x10);
        Vec_RotateX(&obj->velX, (6.2831855f * obj->rotX) / 65536.0f);
        Vec_RotateY(&obj->velX, (6.2831855f * obj->rotY) / 65536.0f);
        obj->gravity = -(param->gravity + gravityRand * BowgunShot_BitsToFrac(obj->randBits >> 1, 0xF));
        break;
    case 2:
        Mtx_Copy(mtx, (f32 *)Em_GetBone(pl, obj->aux));
        obj->velX = speed + speedRand * BowgunShot_BitsToFrac(obj->randBits >> 2, 0xE);
        if ((obj->randBits >> 3) & 1) {
            obj->velX = -obj->velX;
        }
        obj->velY = 2.0f * (speed + speedRand * BowgunShot_BitsToFrac(obj->randBits >> 4, 0xC));
        obj->velZ = speed + speedRand * BowgunShot_BitsToFrac(obj->randBits >> 5, 0xB);
        if ((obj->randBits >> 6) & 1) {
            obj->velZ = -obj->velZ;
        }
        obj->rotX = (u16) (s32) (0.5f + 65536.0f * Math_Atan2(obj->velY, Math_Sqrtf(obj->velX * obj->velX + obj->velZ * obj->velZ)) / 6.2831855f);
        obj->rotY = (u16) (s32) (0.5f + 65536.0f * Math_Atan2(obj->velX, obj->velZ) / 6.2831855f) + 0x8000;
        obj->gravity = -(param->gravity + gravityRand * BowgunShot_BitsToFrac(obj->randBits >> 7, 9));
        break;
    }
    Vec_Copy(obj->pos2, obj->pos);
    Vec_Copy(buf->drawPos, obj->pos);
    BowgunShot_PlaySe(obj, 1);
    if (param->life == 0xFF) {
        buf->phase = 2;
    } else {
        buf->phase = 0;
    }
    buf->unk17 = 0;
    if (param->powerTbl != -1) {
        BowgunShot_ApplyKind(obj);
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
s16 Gunlance_ShellMotions[8][2] = {
#include "game/00897020.inc"
};
HitShape PlayerAttack_HitShapes_0[4] = {
#include "game/00897040.inc"
};
HitShape PlayerAttack_HitShapes_1[3] = {
#include "game/008970E0.inc"
};
HitShape PlayerAttack_HitShapes_2[3] = {
#include "game/00897160.inc"
};
HitShape PlayerAttack_HitShapes_3[2] = {
#include "game/008971E0.inc"
};
HitShape PlayerAttack_HitShapes_4[2] = {
#include "game/00897230.inc"
};
HitShape PlayerAttack_HitShapes_5[2] = {
#include "game/00897280.inc"
};
HitShape PlayerAttack_HitShapes_6[2] = {
#include "game/008972D0.inc"
};
HitShape PlayerAttack_HitShapes_7[2] = {
#include "game/00897320.inc"
};
HitShape PlayerAttack_HitShapes_8[2] = {
#include "game/00897370.inc"
};
HitShape PlayerAttack_HitShapes_9[2] = {
#include "game/008973C0.inc"
};
HitShape PlayerAttack_HitShapes_10[2] = {
#include "game/00897410.inc"
};
HitShape PlayerAttack_HitShapes_11[3] = {
#include "game/00897460.inc"
};
HitShape PlayerAttack_HitShapes_12[2] = {
#include "game/008974E0.inc"
};
HitShape PlayerAttack_HitShapes_13[2] = {
#include "game/00897530.inc"
};
HitShape PlayerAttack_HitShapes_14[2] = {
#include "game/00897580.inc"
};
HitShape PlayerAttack_HitShapes_15[2] = {
#include "game/008975D0.inc"
};
HitShape PlayerAttack_HitShapes_16[2] = {
#include "game/00897620.inc"
};
HitShape PlayerAttack_HitShapes_17[2] = {
#include "game/00897670.inc"
};
HitShape PlayerAttack_HitShapes_18[2] = {
#include "game/008976C0.inc"
};
HitShape PlayerAttack_HitShapes_19[2] = {
#include "game/00897710.inc"
};
HitShape *PlayerAttack_HitShapes[20] = {
    PlayerAttack_HitShapes_0, PlayerAttack_HitShapes_1, PlayerAttack_HitShapes_2, PlayerAttack_HitShapes_3,
    PlayerAttack_HitShapes_4, PlayerAttack_HitShapes_5, PlayerAttack_HitShapes_6, PlayerAttack_HitShapes_7,
    PlayerAttack_HitShapes_8, PlayerAttack_HitShapes_9, PlayerAttack_HitShapes_10, PlayerAttack_HitShapes_11,
    PlayerAttack_HitShapes_12, PlayerAttack_HitShapes_13, PlayerAttack_HitShapes_14, PlayerAttack_HitShapes_15,
    PlayerAttack_HitShapes_16, PlayerAttack_HitShapes_17, PlayerAttack_HitShapes_18, PlayerAttack_HitShapes_19,
};
u32 PlayerAttack_Rows[606] = {
#include "game/008977B0.inc"
};
void *PlayerAttack_RowsPtr = (void *)PlayerAttack_Rows;
HitShape RathalosAttack_HitShapes_0[2] = {
#include "game/00898130.inc"
};
HitShape RathalosAttack_HitShapes_1[3] = {
#include "game/00898180.inc"
};
HitShape RathalosAttack_HitShapes_2[3] = {
#include "game/00898200.inc"
};
HitShape RathalosAttack_HitShapes_3[3] = {
#include "game/00898280.inc"
};
HitShape RathalosAttack_HitShapes_4[4] = {
#include "game/00898300.inc"
};
HitShape RathalosAttack_HitShapes_5[2] = {
#include "game/008983A0.inc"
};
HitShape RathalosAttack_HitShapes_6[2] = {
#include "game/008983F0.inc"
};
HitShape RathalosAttack_HitShapes_7[2] = {
#include "game/00898440.inc"
};
HitShape RathalosAttack_HitShapes_8[3] = {
#include "game/00898490.inc"
};
HitShape RathalosAttack_HitShapes_9[3] = {
#include "game/00898510.inc"
};
HitShape RathalosAttack_HitShapes_10[2] = {
#include "game/00898590.inc"
};
HitShape RathalosAttack_HitShapes_11[2] = {
#include "game/008985E0.inc"
};
HitShape RathalosAttack_HitShapes_12[2] = {
#include "game/00898630.inc"
};
HitShape RathalosAttack_HitShapes_13[2] = {
#include "game/00898680.inc"
};
HitShape RathalosAttack_HitShapes_14[4] = {
#include "game/008986D0.inc"
};
HitShape RathalosAttack_HitShapes_15[2] = {
#include "game/00898770.inc"
};
HitShape RathalosAttack_HitShapes_16[6] = {
#include "game/008987C0.inc"
};
HitShape *RathalosAttack_HitShapes[20] = {
    RathalosAttack_HitShapes_0, RathalosAttack_HitShapes_1, RathalosAttack_HitShapes_2, RathalosAttack_HitShapes_3,
    RathalosAttack_HitShapes_4, RathalosAttack_HitShapes_5, RathalosAttack_HitShapes_6, RathalosAttack_HitShapes_7,
    RathalosAttack_HitShapes_8, RathalosAttack_HitShapes_9, RathalosAttack_HitShapes_10, RathalosAttack_HitShapes_11,
    RathalosAttack_HitShapes_12, RathalosAttack_HitShapes_13, RathalosAttack_HitShapes_14, RathalosAttack_HitShapes_15,
    RathalosAttack_HitShapes_16, 0, 0, 0,
};
u32 RathalosAttack_Rows[294] = {
#include "game/00898900.inc"
};
void *RathalosAttack_RowsPtr = (void *)RathalosAttack_Rows;
HitShape EmContactAttack_HitShapes_0[2] = {
#include "game/00898DA0.inc"
};
HitShape EmContactAttack_HitShapes_1[2] = {
#include "game/00898DF0.inc"
};
HitShape EmContactAttack_HitShapes_2[2] = {
#include "game/00898E40.inc"
};
HitShape EmContactAttack_HitShapes_3[2] = {
#include "game/00898E90.inc"
};
HitShape EmContactAttack_HitShapes_4[2] = {
#include "game/00898EE0.inc"
};
HitShape EmContactAttack_HitShapes_5[2] = {
#include "game/00898F30.inc"
};
HitShape EmContactAttack_HitShapes_6[2] = {
#include "game/00898F80.inc"
};
HitShape EmContactAttack_HitShapes_7[2] = {
#include "game/00898FD0.inc"
};
HitShape *EmContactAttack_HitShapes[8] = {
    EmContactAttack_HitShapes_0, EmContactAttack_HitShapes_1, EmContactAttack_HitShapes_2, EmContactAttack_HitShapes_3,
    EmContactAttack_HitShapes_4, EmContactAttack_HitShapes_5, EmContactAttack_HitShapes_6, EmContactAttack_HitShapes_7,
};
u32 EmContactAttack_Rows[144] = {
#include "game/00899040.inc"
};
void *EmContactAttack_RowsPtr = (void *)EmContactAttack_Rows;
HitShape ItemAttack_HitShapes_0[2] = {
#include "game/00899290.inc"
};
HitShape ItemAttack_HitShapes_1[2] = {
#include "game/008992E0.inc"
};
HitShape ItemAttack_HitShapes_2[2] = {
#include "game/00899330.inc"
};
HitShape *ItemAttack_HitShapes[4] = {
    ItemAttack_HitShapes_0, ItemAttack_HitShapes_1, ItemAttack_HitShapes_2, 0,
};
u32 ItemAttack_Rows[84] = {
#include "game/00899390.inc"
};
void *ItemAttack_RowsPtr = (void *)ItemAttack_Rows;
s8 ItemAttack_AttackRow[32] = {
#include "game/008994F0.inc"
};
s8 ItemAttack_HitsInFlight[32] = {
#include "game/00899510.inc"
};
s8 ItemAttack_HasSide[32] = {
#include "game/00899530.inc"
};
s8 ItemAttack_Modes[32] = {
#include "game/00899550.inc"
};
s16 ItemAttack_Models[32] = {
#include "game/00899570.inc"
};
f32 ItemAttack_ScaleKeysA[12] = {
#include "game/008995B0.inc"
};
f32 ItemAttack_ScaleKeysB[8] = {
#include "game/008995E0.inc"
};
u8 ItemAttack_ColorKeysA[32] = {
#include "game/00899600.inc"
};
u8 ItemAttack_ColorKeysB[48] = {
#include "game/00899620.inc"
};
s16 ItemAttack_PartModels[8] = {
#include "game/00899650.inc"
};
f32 ItemAttack_ThrowSpeedsAlt[4][2] = {
#include "game/00899660.inc"
};
f32 ItemAttack_ThrowSpeeds[4][2] = {
#include "game/00899680.inc"
};
f32 ItemAttack_LobSpeeds[4][2] = {
#include "game/008996A0.inc"
};
HitShape FatalisAttack_HitShapes_0[2] = {
#include "game/008996C0.inc"
};
HitShape FatalisAttack_HitShapes_1[2] = {
#include "game/00899710.inc"
};
HitShape FatalisAttack_HitShapes_2[5] = {
#include "game/00899760.inc"
};
HitShape FatalisAttack_HitShapes_3[2] = {
#include "game/00899830.inc"
};
HitShape FatalisAttack_HitShapes_4[2] = {
#include "game/00899880.inc"
};
HitShape FatalisAttack_HitShapes_5[2] = {
#include "game/008998D0.inc"
};
HitShape FatalisAttack_HitShapes_6[2] = {
#include "game/00899920.inc"
};
HitShape FatalisAttack_HitShapes_7[2] = {
#include "game/00899970.inc"
};
HitShape FatalisAttack_HitShapes_8[2] = {
#include "game/008999C0.inc"
};
HitShape FatalisAttack_HitShapes_9[6] = {
#include "game/00899A10.inc"
};
HitShape FatalisAttack_HitShapes_10[2] = {
#include "game/00899B00.inc"
};
HitShape FatalisAttack_HitShapes_11[3] = {
#include "game/00899B50.inc"
};
HitShape FatalisAttack_HitShapes_12[2] = {
#include "game/00899BD0.inc"
};
HitShape FatalisAttack_HitShapes_13[8] = {
#include "game/00899C20.inc"
};
HitShape FatalisAttack_HitShapes_14[3] = {
#include "game/00899D60.inc"
};
HitShape FatalisAttack_HitShapes_15[3] = {
#include "game/00899DE0.inc"
};
HitShape FatalisAttack_HitShapes_16[2] = {
#include "game/00899E60.inc"
};
HitShape FatalisAttack_HitShapes_17[2] = {
#include "game/00899EB0.inc"
};
HitShape FatalisAttack_HitShapes_18[3] = {
#include "game/00899F00.inc"
};
HitShape *FatalisAttack_HitShapes[20] = {
    FatalisAttack_HitShapes_0, FatalisAttack_HitShapes_1, FatalisAttack_HitShapes_2, FatalisAttack_HitShapes_3,
    FatalisAttack_HitShapes_4, FatalisAttack_HitShapes_5, FatalisAttack_HitShapes_6, FatalisAttack_HitShapes_7,
    FatalisAttack_HitShapes_8, FatalisAttack_HitShapes_9, FatalisAttack_HitShapes_10, FatalisAttack_HitShapes_11,
    FatalisAttack_HitShapes_12, FatalisAttack_HitShapes_13, FatalisAttack_HitShapes_14, FatalisAttack_HitShapes_15,
    FatalisAttack_HitShapes_16, FatalisAttack_HitShapes_17, FatalisAttack_HitShapes_18, 0,
};
u32 FatalisAttack_Rows[216] = {
#include "game/00899FD0.inc"
};
void *FatalisAttack_RowsPtr = (void *)FatalisAttack_Rows;
HitShape LaoShanAttack_HitShapes_0[2] = {
#include "game/0089A340.inc"
};
HitShape LaoShanAttack_HitShapes_1[2] = {
#include "game/0089A390.inc"
};
HitShape LaoShanAttack_HitShapes_2[3] = {
#include "game/0089A3E0.inc"
};
HitShape LaoShanAttack_HitShapes_3[3] = {
#include "game/0089A460.inc"
};
HitShape LaoShanAttack_HitShapes_4[2] = {
#include "game/0089A4E0.inc"
};
HitShape LaoShanAttack_HitShapes_5[4] = {
#include "game/0089A530.inc"
};
HitShape LaoShanAttack_HitShapes_6[3] = {
#include "game/0089A5D0.inc"
};
HitShape LaoShanAttack_HitShapes_7[5] = {
#include "game/0089A650.inc"
};
HitShape LaoShanAttack_HitShapes_8[3] = {
#include "game/0089A720.inc"
};
HitShape LaoShanAttack_HitShapes_9[4] = {
#include "game/0089A7A0.inc"
};
HitShape LaoShanAttack_HitShapes_10[5] = {
#include "game/0089A840.inc"
};
HitShape LaoShanAttack_HitShapes_11[3] = {
#include "game/0089A910.inc"
};
HitShape LaoShanAttack_HitShapes_12[2] = {
#include "game/0089A990.inc"
};
HitShape LaoShanAttack_HitShapes_13[2] = {
#include "game/0089A9E0.inc"
};
HitShape LaoShanAttack_HitShapes_14[2] = {
#include "game/0089AA30.inc"
};
HitShape LaoShanAttack_HitShapes_15[2] = {
#include "game/0089AA80.inc"
};
HitShape LaoShanAttack_HitShapes_16[2] = {
#include "game/0089AAD0.inc"
};
HitShape LaoShanAttack_HitShapes_17[2] = {
#include "game/0089AB20.inc"
};
HitShape LaoShanAttack_HitShapes_18[2] = {
#include "game/0089AB70.inc"
};
HitShape LaoShanAttack_HitShapes_19[2] = {
#include "game/0089ABC0.inc"
};
HitShape *LaoShanAttack_HitShapes[20] = {
    LaoShanAttack_HitShapes_0, LaoShanAttack_HitShapes_1, LaoShanAttack_HitShapes_2, LaoShanAttack_HitShapes_3,
    LaoShanAttack_HitShapes_4, LaoShanAttack_HitShapes_5, LaoShanAttack_HitShapes_6, LaoShanAttack_HitShapes_7,
    LaoShanAttack_HitShapes_8, LaoShanAttack_HitShapes_9, LaoShanAttack_HitShapes_10, LaoShanAttack_HitShapes_11,
    LaoShanAttack_HitShapes_12, LaoShanAttack_HitShapes_13, LaoShanAttack_HitShapes_14, LaoShanAttack_HitShapes_15,
    LaoShanAttack_HitShapes_16, LaoShanAttack_HitShapes_17, LaoShanAttack_HitShapes_18, LaoShanAttack_HitShapes_19,
};
u32 LaoShanAttack_Rows[528] = {
#include "game/0089AC60.inc"
};
void *LaoShanAttack_RowsPtr = (void *)LaoShanAttack_Rows;
HitShape BowgunShot_HitShapes_0[2] = {
#include "game/0089B4B0.inc"
};
HitShape BowgunShot_HitShapes_1[2] = {
#include "game/0089B500.inc"
};
HitShape BowgunShot_HitShapes_2[2] = {
#include "game/0089B550.inc"
};
HitShape BowgunShot_HitShapes_3[2] = {
#include "game/0089B5A0.inc"
};
HitShape BowgunShot_HitShapes_4[2] = {
#include "game/0089B5F0.inc"
};
HitShape BowgunShot_HitShapes_5[2] = {
#include "game/0089B640.inc"
};
HitShape BowgunShot_HitShapes_6[2] = {
#include "game/0089B690.inc"
};
HitShape *BowgunShot_HitShapes[8] = {
    BowgunShot_HitShapes_0, BowgunShot_HitShapes_1, BowgunShot_HitShapes_2, BowgunShot_HitShapes_3,
    BowgunShot_HitShapes_4, BowgunShot_HitShapes_5, BowgunShot_HitShapes_6, 0,
};
u32 BowgunShot_Rows[210] = {
#include "game/0089B700.inc"
};
void *BowgunShot_RowsPtr = (void *)BowgunShot_Rows;
BowgunShotParam gBowgunShotParams[33] = {
#include "game/0089BA50.inc"
};
s32 BowgunShot_PlayerFlags[34] = {
#include "game/0089BF00.inc"
};
