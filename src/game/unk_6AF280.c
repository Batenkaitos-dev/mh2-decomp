/*
 * The drawn weapons' states, first part: the great sword, the bowguns (heavy
 * and light share them), the hammer, the lance, the sword and shield, the
 * dual swords, the long sword, and the hunting horn's play stance (the rest
 * of the horn, the gunlance and the bow are in unk_6BB9D0.c).
 *
 * Player_RunWeaponState (unk_70A290.c) runs the drawn weapon's
 * <Weapon>_RunState, which maps the player's state in group PL_GROUP_WEAPON
 * (the per-weapon enums of include/player.h) to a handler and a variant.
 * A handler steps through pl->step: it plays its motions
 * (Player_SetMotionPair; motion ids are plain numbers), makes the attack's
 * hit object (PlayerAttack_Create with an attack id, unk_6BB9D0.c), checks the hit
 * (<Weapon>_CheckHit: a bounce, the hit slow) and in its input window reads
 * the next input (<Weapon>_CheckInput); then it chains another state or
 * returns to idle (Player_ReturnToIdle). <Weapon>_AttackInput reads the drawn
 * idle's input, <Weapon>_DrawAttack starts the attack that draws on the move.
 * The attack input is a flick of the right stick (input.stickDirPressed
 * 0x3C), its direction in input.rightStickAngle; the roll button is
 * input.pressed PAD_CROSS, and input.held PAD_R1 the weapon's hold special
 * (Player_HoldSpecialInput: the guard, the charge, the demon mode...).
 */
#include "common.h"
#include "em.h"
#include "pad.h"
#include "player.h"

/* prototypes */
extern u16 HuntingHorn_StanceMotions[];
extern HornMotion HuntingHorn_StanceEntryMotions[];
extern FrameRange HuntingHorn_Note0WalkFrames[];
extern FrameRange HuntingHorn_Note1WalkFrames[];
extern FrameRange HuntingHorn_Note2WalkFrames[];
void Player_StickTurnMove(Player *, s16, f32);
void HuntingHorn_PlayInput(Player *, s32);
void HuntingHorn_AddNote(Player *, u8);
void GreatSword_SetChargeAttack(Player *em);
extern MotionBlend GreatSword_OverheadBlends[];
void StageObj_K00_Create(void *owner, u8 k, f32 *pos, u8 c);
void GreatSword_CheckInput(Player *em, s32 arg1);
extern GunMotion Bowgun_FireMotions[];
/* Byte 2 of each 4-byte row of sub_main's gAmmoLevels (one row per ammo type):
 * the rapid-fire shots after the first. A linker alias (game_aliases.ld), which
 * the match needs: the code indexes from it, not from gAmmoLevels. */
extern u8 gAmmoRapidShots[][4];
void CamShake_ReqB(void *obj, s32 i);
void Bowgun_FireShot(Player *em, s32 arg1, s32 arg2, f32 scale);
u8 Pouch_Add(Player *p, u16 item, s16 n);
void StageObj_Gun_CreateReload(void *owner, u8 k);
f32 Math_Cos(f32 x);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void DualSwords_ComboInput(Player *em, s32 a, s32 b);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
u8 Bowgun_ReloadLevel(Player *em, u8 k);
void Player_ReturnToIdleKeepParts(Player *em, s32 action, s32 b, s32 c);
s32 Player_HasSkill(Player *p, u8 skill);
u8 GreatSword_ChargeLevel(Player *p);
u8 Player_StickMoveLevel(Player *, s32);
void Player_FaceStick(Player *em);
s16 Player_IsState(void *obj, u8 state, u8 sub);
s16 Bowgun_RecoilLevel(Player *p, u8 k);
s32 Motion_PassedFrame(void *, s32 layer, f32 frame);
s32 Player_IsLocal(void *);
void Player_RequestState(Player *p, u16 state, u8 sub, u16 flags);
void Player_SetFlag(void *obj, u32 flag);
void Player_SetMotionPair(Player *, s32, s32, s32); /* unit's own declaration: the last parameter s32 (the definition has s16); with the definition's type GreatSword_StateSweepLeft by 94 of 281, GreatSword_StateSweepRight by 141 of 316, GreatSword_StateOverhead by 293 of 560 words differ */
void Player_SetMoveMode(Player *, u16 mode, s16 flag1, s16 flag2);
void Player_SetState(Player *, u8, u8, s32); /* unit's own declaration: (u8, u8, s32) (the definition has u16, u8, u16); with the definition's type Player_TurnAndSetState by 3 of 21 words differ */
u32 Player_TestFlag(void *obj, u32 flag);
void Player_UpdateMotionTime(Player *p, s16 step);
void Player_UseStamina(Player *p, s32 n);
void GreatSword_StateSweepRight(Player *, s32);
void GreatSword_StateOverhead(Player *, u32);
void GreatSword_StateBounce(Player *, s32);
void GreatSword_StateCharge(Player *, u32);
void GreatSword_StateChargeSlash(Player *, s32);
void Bowgun_StateFire(Player *, s32, s32);
void Bowgun_StateReload(Player *, s32);
void Bowgun_StateDrawReload(Player *, s32);
void Bowgun_StateDryFire(Player *, s32);
void Hammer_StateDownAttack(Player *, s32);
void Hammer_StateCombo1(Player *, s32);
void Hammer_StateCombo2(Player *, s32);
void Hammer_StateCharge(Player *, s32);
void Hammer_StateChargeSpin(Player *, s32);
void Hammer_StateChargeRelease(Player *, s32);
void Hammer_StateChargeRelease2(Player *, s32);
void Hammer_StateFinisher(Player *, s32);
void Hammer_StateBounce(Player *, s32);
void Hammer_CheckInput(Player *em, s32 arg1);
void Hammer_CheckChargeInput(Player *);
void Lance_StateThrust(Player *, s32);
void Lance_StateUpperThrust(Player *, s32);
void Lance_StateCharge(Player *, s32);
void Lance_StateChargeFinish(Player *, s32);
void Lance_StateBounce(Player *, s32);
void Lance_CheckInput(Player *, s32);
s32 SwordShield_CheckHit(Player *);
void SwordShield_StateCombo(Player *, s32);
void SwordShield_StateRisingSlash(Player *, s32);
void SwordShield_StateBounce(Player *, s32);
void SwordShield_CheckInput(Player *em, s32 arg1);
s32 DualSwords_CheckHit(Player *);
void DualSwords_StateCombo(Player *, s32);
void DualSwords_StateSideSlash(Player *em, s32 arg1);
void DualSwords_StateRisingSlash(Player *, s32);
void DualSwords_StateDemonToggle(Player *, s32);
void DualSwords_StateDemonSideSlash(Player *, s32);
void DualSwords_StateBounce(Player *, s32);
void DualSwords_CheckInput(Player *, s32);
void LongSword_StateSlash(Player *, s32, s32);
void LongSword_StateSlash2(Player *, s32);
void LongSword_StateFadeSlash(Player *, u32);
void LongSword_StateBounce(Player *, s32);
void LongSword_StateThrust(Player *, s32);
void LongSword_StateSpirit1(Player *, u32);
void LongSword_StateSpiritThrust(Player *, s32);
void LongSword_StateSpirit2(Player *, s32);
void LongSword_StateSpiritRising(Player *, s32);
void LongSword_StateRisingSlash(Player *, s32);
void LongSword_StateSpirit3(Player *, s32);
void LongSword_StateExtraSlash(Player *, s32);
void PlayerAttack_Create(Player *, u8);
s32 Motion_ReachedFrame(void *, s32, f32);
void CamShake_ReqA(void *obj, s32 i);
s32 Game_IsOnline(void);
s32 Quest_PhaseIsOver(s32);
void Player_ReturnToIdle(Player *p, s32 action, s32 a2, s32 a3);
void Player_TickWeaponTimer(Player *p);
s32 LongSword_GaugeAtLeast(Player *p, s16 n);
void GreatSword_StateSweepLeft(Player *, s32);
void PlayerAttack_CreateDual(Player *, s32, s32);
extern s16 DualSwords_SideSlashMotions[];
s32 Motion_InFrameRange(void *, s32, f32, f32);
void Vib_StartLocal(void *port, s32 i);
s32 LongSword_ReadAttackKey(Player *em);
void LongSword_EvadeInput(Player *em);
u8 LongSword_ComboInput(Player *, u32, u8, u8);
void Player_TurnAndSetState(Player *em, u8 group, u8 state, s32 mode, u8 turn);
struct StageObj *StageObj_K05_Create(void *owner, s32 arg1, s32 kind);
void StageObj_K29_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);
void LongSword_AddGauge(Player *p, s16 n);
/* The light bowguns' rapid-fire ammo (Bowgun_RapidFireMask): a gun's id and a
 * mask with a bit per ammo type; id 999 ends it. A linker alias into sub_main's
 * data (game_aliases.ld). */
extern GunAmmoMask gRapidFireAmmo[];
/* end of prototypes */

/* unk_6AF280's .rodata (0x8DA570-0x8DCB50) */
#pragma rodata on
#pragma explicit_zero_data on
char D_008DA570[0x10] = {
#include "game/008DA570.inc"
};
#pragma explicit_zero_data reset
char D_008DA580[0x20] = {
#include "game/008DA580.inc"
};
char D_008DA5A0[0x20] = {
#include "game/008DA5A0.inc"
};
char D_008DA5C0[0x30] = {
#include "game/008DA5C0.inc"
};
char D_008DA5F0[0x30] = {
#include "game/008DA5F0.inc"
};
char D_008DA620[0x30] = {
#include "game/008DA620.inc"
};
char D_008DA650[0x20] = {
#include "game/008DA650.inc"
};
char D_008DA670[0x20] = {
#include "game/008DA670.inc"
};
char D_008DA690[0x30] = {
#include "game/008DA690.inc"
};
char D_008DA6C0[0x20] = {
#include "game/008DA6C0.inc"
};
char D_008DA6E0[0x20] = {
#include "game/008DA6E0.inc"
};
char D_008DA700[0x20] = {
#include "game/008DA700.inc"
};
char D_008DA720[0x30] = {
#include "game/008DA720.inc"
};
char D_008DA750[0x20] = {
#include "game/008DA750.inc"
};
char D_008DA770[0x20] = {
#include "game/008DA770.inc"
};
char D_008DA790[0x20] = {
#include "game/008DA790.inc"
};
char D_008DA7B0[0x20] = {
#include "game/008DA7B0.inc"
};
char D_008DA7D0[0x10] = {
#include "game/008DA7D0.inc"
};
char D_008DA7E0[0x30] = {
#include "game/008DA7E0.inc"
};
char D_008DA810[0x20] = {
#include "game/008DA810.inc"
};
char D_008DA830[0x30] = {
#include "game/008DA830.inc"
};
char D_008DA860[0x30] = {
#include "game/008DA860.inc"
};
char D_008DA890[0x20] = {
#include "game/008DA890.inc"
};
char D_008DA8B0[0x30] = {
#include "game/008DA8B0.inc"
};
char D_008DA8E0[0x20] = {
#include "game/008DA8E0.inc"
};
char D_008DA900[0x30] = {
#include "game/008DA900.inc"
};
char D_008DA930[0x20] = {
#include "game/008DA930.inc"
};
char D_008DA950[0x30] = {
#include "game/008DA950.inc"
};
char D_008DA980[0x30] = {
#include "game/008DA980.inc"
};
char D_008DA9B0[0x30] = {
#include "game/008DA9B0.inc"
};
char D_008DA9E0[0x20] = {
#include "game/008DA9E0.inc"
};
char D_008DAA00[0x30] = {
#include "game/008DAA00.inc"
};
char D_008DAA30[0x20] = {
#include "game/008DAA30.inc"
};
char D_008DAA50[0x20] = {
#include "game/008DAA50.inc"
};
char D_008DAA70[0x20] = {
#include "game/008DAA70.inc"
};
char D_008DAA90[0x30] = {
#include "game/008DAA90.inc"
};
char D_008DAAC0[0x20] = {
#include "game/008DAAC0.inc"
};
char D_008DAAE0[0x30] = {
#include "game/008DAAE0.inc"
};
char D_008DAB10[0x20] = {
#include "game/008DAB10.inc"
};
char D_008DAB30[0x20] = {
#include "game/008DAB30.inc"
};
char D_008DAB50[0x20] = {
#include "game/008DAB50.inc"
};
char D_008DAB70[0x30] = {
#include "game/008DAB70.inc"
};
char D_008DABA0[0x30] = {
#include "game/008DABA0.inc"
};
char D_008DABD0[0x20] = {
#include "game/008DABD0.inc"
};
char D_008DABF0[0x30] = {
#include "game/008DABF0.inc"
};
char D_008DAC20[0x20] = {
#include "game/008DAC20.inc"
};
char D_008DAC40[0x30] = {
#include "game/008DAC40.inc"
};
char D_008DAC70[0x30] = {
#include "game/008DAC70.inc"
};
char D_008DACA0[0x20] = {
#include "game/008DACA0.inc"
};
char D_008DACC0[0x10] = {
#include "game/008DACC0.inc"
};
char D_008DACD0[0x30] = {
#include "game/008DACD0.inc"
};
char D_008DAD00[0x20] = {
#include "game/008DAD00.inc"
};
char D_008DAD20[0x30] = {
#include "game/008DAD20.inc"
};
char D_008DAD50[0x20] = {
#include "game/008DAD50.inc"
};
char D_008DAD70[0x20] = {
#include "game/008DAD70.inc"
};
char D_008DAD90[0x20] = {
#include "game/008DAD90.inc"
};
char D_008DADB0[0x30] = {
#include "game/008DADB0.inc"
};
char D_008DADE0[0x20] = {
#include "game/008DADE0.inc"
};
char D_008DAE00[0x20] = {
#include "game/008DAE00.inc"
};
char D_008DAE20[0x30] = {
#include "game/008DAE20.inc"
};
char D_008DAE50[0x20] = {
#include "game/008DAE50.inc"
};
char D_008DAE70[0x30] = {
#include "game/008DAE70.inc"
};
char D_008DAEA0[0x20] = {
#include "game/008DAEA0.inc"
};
char D_008DAEC0[0x30] = {
#include "game/008DAEC0.inc"
};
char D_008DAEF0[0x20] = {
#include "game/008DAEF0.inc"
};
char D_008DAF10[0x20] = {
#include "game/008DAF10.inc"
};
char D_008DAF30[0x10] = {
#include "game/008DAF30.inc"
};
char D_008DAF40[0x40] = {
#include "game/008DAF40.inc"
};
char D_008DAF80[0x30] = {
#include "game/008DAF80.inc"
};
char D_008DAFB0[64] = {
#include "game/008DAFB0.inc"
};
char D_008DAFF0[0x20] = {
#include "game/008DAFF0.inc"
};
char D_008DB010[0x20] = {
#include "game/008DB010.inc"
};
char D_008DB030[0x28] = {
#include "game/008DB030.inc"
};
char D_008DB058[0xC] = {
#include "game/008DB058.inc"
};
char D_008DB070[0x20] = {
#include "game/008DB070.inc"
};
char D_008DB090[0x30] = {
#include "game/008DB090.inc"
};
char D_008DB0C0[0x30] = {
#include "game/008DB0C0.inc"
};
char D_008DB0F0[0x20] = {
#include "game/008DB0F0.inc"
};
char D_008DB110[0x20] = {
#include "game/008DB110.inc"
};
char D_008DB130[0x20] = {
#include "game/008DB130.inc"
};
char D_008DB150[0x20] = {
#include "game/008DB150.inc"
};
char D_008DB170[0x28] = {
#include "game/008DB170.inc"
};
char D_008DB198[12] = {
#include "game/008DB198.inc"
};
char D_008DB1B0[0x20] = {
#include "game/008DB1B0.inc"
};
char D_008DB1D0[0x30] = {
#include "game/008DB1D0.inc"
};
char D_008DB200[0x20] = {
#include "game/008DB200.inc"
};
char D_008DB220[0x20] = {
#include "game/008DB220.inc"
};
char D_008DB240[0x20] = {
#include "game/008DB240.inc"
};
char D_008DB260[0x20] = {
#include "game/008DB260.inc"
};
char D_008DB280[0x20] = {
#include "game/008DB280.inc"
};
char D_008DB2A0[0x20] = {
#include "game/008DB2A0.inc"
};
char D_008DB2C0[0x20] = {
#include "game/008DB2C0.inc"
};
char D_008DB2E0[0x30] = {
#include "game/008DB2E0.inc"
};
char D_008DB310[0x20] = {
#include "game/008DB310.inc"
};
char D_008DB330[0x20] = {
#include "game/008DB330.inc"
};
char D_008DB350[0x10] = {
#include "game/008DB350.inc"
};
char D_008DB360[0x30] = {
#include "game/008DB360.inc"
};
char D_008DB390[0x20] = {
#include "game/008DB390.inc"
};
char D_008DB3B0[0x20] = {
#include "game/008DB3B0.inc"
};
char D_008DB3D0[0x20] = {
#include "game/008DB3D0.inc"
};
char D_008DB3F0[0x30] = {
#include "game/008DB3F0.inc"
};
char D_008DB420[0x20] = {
#include "game/008DB420.inc"
};
char D_008DB440[0x20] = {
#include "game/008DB440.inc"
};
char D_008DB460[0x20] = {
#include "game/008DB460.inc"
};
char D_008DB480[0x30] = {
#include "game/008DB480.inc"
};
char D_008DB4B0[0x20] = {
#include "game/008DB4B0.inc"
};
char D_008DB4D0[0x20] = {
#include "game/008DB4D0.inc"
};
char D_008DB4F0[0x20] = {
#include "game/008DB4F0.inc"
};
char D_008DB510[0x20] = {
#include "game/008DB510.inc"
};
char D_008DB530[0x20] = {
#include "game/008DB530.inc"
};
char D_008DB550[0x20] = {
#include "game/008DB550.inc"
};
char D_008DB570[0x30] = {
#include "game/008DB570.inc"
};
char D_008DB5A0[0x20] = {
#include "game/008DB5A0.inc"
};
char D_008DB5C0[0x20] = {
#include "game/008DB5C0.inc"
};
char D_008DB5E0[48] = {
#include "game/008DB5E0.inc"
};
char D_008DB610[0x20] = {
#include "game/008DB610.inc"
};
char D_008DB630[0x30] = {
#include "game/008DB630.inc"
};
char D_008DB660[0x30] = {
#include "game/008DB660.inc"
};
char D_008DB690[0x30] = {
#include "game/008DB690.inc"
};
char D_008DB6C0[0x20] = {
#include "game/008DB6C0.inc"
};
char D_008DB6E0[0x20] = {
#include "game/008DB6E0.inc"
};
char D_008DB700[0x20] = {
#include "game/008DB700.inc"
};
char D_008DB720[0x20] = {
#include "game/008DB720.inc"
};
char D_008DB740[0x10] = {
#include "game/008DB740.inc"
};
char D_008DB750[0x30] = {
#include "game/008DB750.inc"
};
char D_008DB780[0x20] = {
#include "game/008DB780.inc"
};
char D_008DB7A0[0x20] = {
#include "game/008DB7A0.inc"
};
char D_008DB7C0[0x20] = {
#include "game/008DB7C0.inc"
};
char D_008DB7E0[0x30] = {
#include "game/008DB7E0.inc"
};
char D_008DB810[0x20] = {
#include "game/008DB810.inc"
};
char D_008DB830[0x30] = {
#include "game/008DB830.inc"
};
char D_008DB860[0x30] = {
#include "game/008DB860.inc"
};
char D_008DB890[0x28] = {
#include "game/008DB890.inc"
};
char D_008DB8B8[12] = {
#include "game/008DB8B8.inc"
};
char D_008DB8D0[0x20] = {
#include "game/008DB8D0.inc"
};
char D_008DB8F0[0x30] = {
#include "game/008DB8F0.inc"
};
char D_008DB920[0x20] = {
#include "game/008DB920.inc"
};
char D_008DB940[0x10] = {
#include "game/008DB940.inc"
};
char D_008DB950[0x30] = {
#include "game/008DB950.inc"
};
char D_008DB980[0x30] = {
#include "game/008DB980.inc"
};
char D_008DB9B0[0x20] = {
#include "game/008DB9B0.inc"
};
char D_008DB9D0[0x30] = {
#include "game/008DB9D0.inc"
};
char D_008DBA00[0x30] = {
#include "game/008DBA00.inc"
};
char D_008DBA30[0x20] = {
#include "game/008DBA30.inc"
};
char D_008DBA50[0x20] = {
#include "game/008DBA50.inc"
};
char D_008DBA70[0x30] = {
#include "game/008DBA70.inc"
};
char D_008DBAA0[0x30] = {
#include "game/008DBAA0.inc"
};
char D_008DBAD0[0x30] = {
#include "game/008DBAD0.inc"
};
char D_008DBB00[0x20] = {
#include "game/008DBB00.inc"
};
char D_008DBB20[0x30] = {
#include "game/008DBB20.inc"
};
char D_008DBB50[0x30] = {
#include "game/008DBB50.inc"
};
char D_008DBB80[0x20] = {
#include "game/008DBB80.inc"
};
char D_008DBBA0[0x30] = {
#include "game/008DBBA0.inc"
};
char D_008DBBD0[0x30] = {
#include "game/008DBBD0.inc"
};
char D_008DBC00[0x20] = {
#include "game/008DBC00.inc"
};
char D_008DBC20[0x30] = {
#include "game/008DBC20.inc"
};
char D_008DBC50[0x30] = {
#include "game/008DBC50.inc"
};
char D_008DBC80[0x30] = {
#include "game/008DBC80.inc"
};
char D_008DBCB0[0x30] = {
#include "game/008DBCB0.inc"
};
char D_008DBCE0[0x30] = {
#include "game/008DBCE0.inc"
};
char D_008DBD10[0x20] = {
#include "game/008DBD10.inc"
};
char D_008DBD30[0x30] = {
#include "game/008DBD30.inc"
};
char D_008DBD60[0x20] = {
#include "game/008DBD60.inc"
};
char D_008DBD80[0x20] = {
#include "game/008DBD80.inc"
};
char D_008DBDA0[64] = {
#include "game/008DBDA0.inc"
};
char D_008DBDE0[0x30] = {
#include "game/008DBDE0.inc"
};
char D_008DBE10[0x20] = {
#include "game/008DBE10.inc"
};
char D_008DBE30[0x20] = {
#include "game/008DBE30.inc"
};
char D_008DBE50[0x20] = {
#include "game/008DBE50.inc"
};
char D_008DBE70[0x30] = {
#include "game/008DBE70.inc"
};
char D_008DBEA0[0x20] = {
#include "game/008DBEA0.inc"
};
char D_008DBEC0[0x30] = {
#include "game/008DBEC0.inc"
};
char D_008DBEF0[0x28] = {
#include "game/008DBEF0.inc"
};
char D_008DBF18[12] = {
#include "game/008DBF18.inc"
};
char D_008DBF30[0x30] = {
#include "game/008DBF30.inc"
};
char D_008DBF60[0x20] = {
#include "game/008DBF60.inc"
};
char D_008DBF80[0x30] = {
#include "game/008DBF80.inc"
};
char D_008DBFB0[0x30] = {
#include "game/008DBFB0.inc"
};
char D_008DBFE0[0x30] = {
#include "game/008DBFE0.inc"
};
char D_008DC010[0x28] = {
#include "game/008DC010.inc"
};
char D_008DC038[12] = {
#include "game/008DC038.inc"
};
char D_008DC050[0x30] = {
#include "game/008DC050.inc"
};
char D_008DC080[0x30] = {
#include "game/008DC080.inc"
};
char D_008DC0B0[0x20] = {
#include "game/008DC0B0.inc"
};
char D_008DC0D0[0x30] = {
#include "game/008DC0D0.inc"
};
char D_008DC100[0x30] = {
#include "game/008DC100.inc"
};
char D_008DC130[0x30] = {
#include "game/008DC130.inc"
};
char D_008DC160[0x30] = {
#include "game/008DC160.inc"
};
char D_008DC190[0x20] = {
#include "game/008DC190.inc"
};
char D_008DC1B0[0x20] = {
#include "game/008DC1B0.inc"
};
char D_008DC1D0[0x30] = {
#include "game/008DC1D0.inc"
};
char D_008DC200[0x20] = {
#include "game/008DC200.inc"
};
char D_008DC220[0x20] = {
#include "game/008DC220.inc"
};
char D_008DC240[0x30] = {
#include "game/008DC240.inc"
};
char D_008DC270[0x20] = {
#include "game/008DC270.inc"
};
char D_008DC290[0x20] = {
#include "game/008DC290.inc"
};
char D_008DC2B0[64] = {
#include "game/008DC2B0.inc"
};
char D_008DC2F0[0x20] = {
#include "game/008DC2F0.inc"
};
char D_008DC310[0x30] = {
#include "game/008DC310.inc"
};
char D_008DC340[0x30] = {
#include "game/008DC340.inc"
};
char D_008DC370[0x20] = {
#include "game/008DC370.inc"
};
char D_008DC390[0x30] = {
#include "game/008DC390.inc"
};
char D_008DC3C0[0x20] = {
#include "game/008DC3C0.inc"
};
char D_008DC3E0[0x20] = {
#include "game/008DC3E0.inc"
};
char D_008DC400[0x40] = {
#include "game/008DC400.inc"
};
char D_008DC440[0x30] = {
#include "game/008DC440.inc"
};
char D_008DC470[0x20] = {
#include "game/008DC470.inc"
};
char D_008DC490[0x20] = {
#include "game/008DC490.inc"
};
char D_008DC4B0[0x20] = {
#include "game/008DC4B0.inc"
};
char D_008DC4D0[0x20] = {
#include "game/008DC4D0.inc"
};
char D_008DC4F0[0x30] = {
#include "game/008DC4F0.inc"
};
char D_008DC520[0x30] = {
#include "game/008DC520.inc"
};
char D_008DC550[0x20] = {
#include "game/008DC550.inc"
};
char D_008DC570[0x20] = {
#include "game/008DC570.inc"
};
char D_008DC590[0x30] = {
#include "game/008DC590.inc"
};
char D_008DC5C0[0x30] = {
#include "game/008DC5C0.inc"
};
char D_008DC5F0[0x20] = {
#include "game/008DC5F0.inc"
};
char D_008DC610[0x30] = {
#include "game/008DC610.inc"
};
char D_008DC640[0x30] = {
#include "game/008DC640.inc"
};
char D_008DC670[0x30] = {
#include "game/008DC670.inc"
};
char D_008DC6A0[0x20] = {
#include "game/008DC6A0.inc"
};
char D_008DC6C0[0x30] = {
#include "game/008DC6C0.inc"
};
char D_008DC6F0[0x20] = {
#include "game/008DC6F0.inc"
};
char D_008DC710[0x20] = {
#include "game/008DC710.inc"
};
char D_008DC730[0x20] = {
#include "game/008DC730.inc"
};
char D_008DC750[0x30] = {
#include "game/008DC750.inc"
};
char D_008DC780[0x30] = {
#include "game/008DC780.inc"
};
char D_008DC7B0[0x20] = {
#include "game/008DC7B0.inc"
};
char D_008DC7D0[0x20] = {
#include "game/008DC7D0.inc"
};
char D_008DC7F0[0x20] = {
#include "game/008DC7F0.inc"
};
char D_008DC810[0x30] = {
#include "game/008DC810.inc"
};
char D_008DC840[0x20] = {
#include "game/008DC840.inc"
};
char D_008DC860[0x20] = {
#include "game/008DC860.inc"
};
char D_008DC880[0x30] = {
#include "game/008DC880.inc"
};
char D_008DC8B0[0x30] = {
#include "game/008DC8B0.inc"
};
char D_008DC8E0[0x20] = {
#include "game/008DC8E0.inc"
};
char D_008DC900[0x20] = {
#include "game/008DC900.inc"
};
char D_008DC920[0x30] = {
#include "game/008DC920.inc"
};
char D_008DC950[0x20] = {
#include "game/008DC950.inc"
};
char D_008DC970[0x30] = {
#include "game/008DC970.inc"
};
char D_008DC9A0[0x20] = {
#include "game/008DC9A0.inc"
};
char D_008DC9C0[0x30] = {
#include "game/008DC9C0.inc"
};
char D_008DC9F0[0x20] = {
#include "game/008DC9F0.inc"
};
char D_008DCA10[0x20] = {
#include "game/008DCA10.inc"
};
char D_008DCA30[0x20] = {
#include "game/008DCA30.inc"
};
char D_008DCA50[0x20] = {
#include "game/008DCA50.inc"
};
char D_008DCA70[0x20] = {
#include "game/008DCA70.inc"
};
char D_008DCA90[0x20] = {
#include "game/008DCA90.inc"
};
char D_008DCAB0[0x20] = {
#include "game/008DCAB0.inc"
};
char D_008DCAD0[0x10] = {
#include "game/008DCAD0.inc"
};
char D_008DCAE0[0x20] = {
#include "game/008DCAE0.inc"
};
char D_008DCB00[0x20] = {
#include "game/008DCB00.inc"
};
char D_008DCB20[0x20] = {
#include "game/008DCB20.inc"
};
char D_008DCB40[0x10] = {
#include "game/008DCB40.inc"
};
#pragma rodata off

/*
 * The left sweep, GSWORD_SWEEP_LEFT and its chained forms. variant: 0 from
 * idle; 1 after the overhead or the charged slash; 2 after the right sweep; 3
 * and 4 as 1 and 2, turned 45 degrees left first. 1-4 play a link motion
 * before the sweep. A bounce goes to GSWORD_BOUNCE; frames 72-98 are the
 * combo window (GreatSword_CheckInput caller 1).
 */
void GreatSword_StateSweepLeft(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->unk417 = 1;
        pl->stateVar1 = 0;
        pl->noFlinchTimer = 0x3C;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, 0x57E, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 2);
            Player_SetMoveMode(pl, 0, 1, 0);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                Player_UpdateMotionTime(pl, 0);
                switch (variant) {
                case 1:
                    Player_SetMotionPair(pl, 0x589, 4, 0);
                    break;
                case 2:
                    Player_SetMotionPair(pl, 0x58D, 4, 0);
                    break;
                case 3:
                case 4:
                    pl->rot[1] = (u16) (pl->rot[1] + 0x2000);
                    pl->targetYaw = pl->rot[1];
                    if (variant == 3) {
                        Player_SetMotionPair(pl, 0x58F, 0, 0);
                    } else {
                        Player_SetMotionPair(pl, 0x593, 0, 0);
                    }
                    break;
                }
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    switch (variant) {
                    case 2:
                        Player_SetMotionPair(pl, 0x57E, 2, 0x2C);
                        break;
                    case 4:
                        Player_SetMotionPair(pl, 0x57E, 0, 0x2C);
                        break;
                    default:
                        Player_SetMotionPair(pl, 0x57E, 2, 0x1A);
                        break;
                    }
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 2);
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        } else if (pl->hitPending != 0 && pl->stateVar1 == 0) {
            pl->stateVar1 = 1;
            CamShake_ReqA(pl, 0);
            switch (pl->hitKind) {
            case 0:
                StageObj_K05_Create(pl, 0xF, 0);
                break;
            case 1:
                pl->hitStop = 0;
                pl->hitStopAdd = 0;
                pl->unk417 = 0;
                Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_BOUNCE, 0);
                break;
            case 2:
            case 3:
                pl->step++;
                Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
                pl->weaponSeMute = 2;
                CamShake_ReqA(pl, 2);
                break;
            }
        } else if (Motion_ReachedFrame(pl, 0, 72.0f) != 0 && Motion_ReachedFrame(pl, 0, 98.0f) == 0) {
            GreatSword_CheckInput(pl, 1);
        }
        break;
    case 2:
        if (pl->hitSlowTimer <= 0) {
            pl->step++;
            StageObj_K05_Create(pl, 0x1C, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        } else if (Motion_ReachedFrame(pl, 0, 78.0f) != 0 && Motion_ReachedFrame(pl, 0, 104.0f) == 0) {
            GreatSword_CheckInput(pl, 1);
        }
        break;
    }
}

/*
 * The right sweep, GSWORD_SWEEP_RIGHT and its chained forms, the mirror of
 * GreatSword_StateSweepLeft (variants likewise; 2 is after the left sweep).
 * When nothing was hit, a ground impact at frame 106. Window: frames 116-192
 * (caller 2).
 */
void GreatSword_StateSweepRight(Player *pl, s32 variant)
{
    f32 point[4];
    f32 out[4];

    switch (pl->step) {
    case 0:
        pl->unk417 = 1;
        pl->stateVar1 = 0;
        pl->noFlinchTimer = 0x3C;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, 0x57F, 4, 0);
            pl->motionTime = 0;
            PlayerAttack_Create(pl, 1);
            Player_SetMoveMode(pl, 0, 1, 0);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                Player_UpdateMotionTime(pl, 0);
                switch (variant) {
                case 1:
                    Player_SetMotionPair(pl, 0x588, 4, 0);
                    break;
                case 2:
                    Player_SetMotionPair(pl, 0x58B, 4, 0);
                    break;
                case 3:
                case 4:
                    pl->rot[1] = (u16) (pl->rot[1] - 0x2000);
                    pl->targetYaw = pl->rot[1];
                    if (variant == 3) {
                        Player_SetMotionPair(pl, 0x58E, 0, 0);
                    } else {
                        Player_SetMotionPair(pl, 0x591, 0, 0);
                    }
                    break;
                }
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    if (variant == 1) {
                        Player_SetMotionPair(pl, 0x57F, 2, 0x40);
                    } else {
                        Player_SetMotionPair(pl, 0x57F, 0, 0x42);
                    }
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 1);
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        } else if (pl->hitPending != 0 && pl->stateVar1 == 0) {
            pl->stateVar1 = 1;
            CamShake_ReqA(pl, 0);
            switch (pl->hitKind) {
            case 0:
                StageObj_K05_Create(pl, 0xF, 0);
                break;
            case 1:
                pl->hitStop = 0;
                pl->hitStopAdd = 0;
                pl->unk417 = 0;
                Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_BOUNCE, 0);
                break;
            case 2:
            case 3:
                pl->step++;
                Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
                pl->weaponSeMute = 2;
                CamShake_ReqA(pl, 2);
                break;
            }
        } else {
            if (pl->stateVar1 == 0 && Motion_PassedFrame(pl, 0, 106.0f) != 0) {
                point[0] = 30.0f;
                point[1] = 0.0f;
                point[2] = -150.0f;
                Vec_Rotate(out, point, pl->rotMtx);
                point[0] = pl->pos[0] + out[0];
                point[1] = 15.0f + (pl->pos[1] + out[1]);
                point[2] = pl->pos[2] + out[2];
                StageObj_K00_Create(pl, 0, point, 1);
                Vib_StartLocal(pl, 2);
                CamShake_ReqA(pl, 0);
            }
            if (Motion_ReachedFrame(pl, 0, 116.0f) != 0 && Motion_ReachedFrame(pl, 0, 192.0f) == 0) {
                GreatSword_CheckInput(pl, 2);
            }
        }
        break;
    case 2:
        if (pl->hitSlowTimer <= 0) {
            pl->step++;
            StageObj_K05_Create(pl, 0x1C, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        } else if (Motion_ReachedFrame(pl, 0, 116.0f) != 0 && Motion_ReachedFrame(pl, 0, 192.0f) == 0) {
            GreatSword_CheckInput(pl, 2);
        }
        break;
    }
}

/*
 * The overhead slash, GSWORD_OVERHEAD and its forms. variant: 0 from idle, 1
 * GSWORD_DRAW_OVERHEAD, 2 after the right sweep, 3 after the left, 4 after
 * the bounce, 5 and 6 as 3 and 2 turned. The slash motion's blend and start
 * come from GreatSword_OverheadBlends[variant]. Window: frames 48-102 (caller
 * 3).
 */
void GreatSword_StateOverhead(Player *pl, u32 variant)
{
    f32 point[4];
    f32 out[4];

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0x3C;
        pl->weaponOut = 1;
        pl->unk417 = 1;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, 0x579, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            break;
        case 1:
            pl->step++;
            Player_SetMotionPair(pl, 0x582, 2, 0);
            pl->unk417 = 0;
            break;
        case 2:
            Player_SetMotionPair(pl, 0x58C, 4, 0);
            break;
        case 3:
            Player_SetMotionPair(pl, 0x58A, 4, 0);
            break;
        case 4:
            pl->step++;
            Player_SetMotionPair(pl, 0x57D, 4, 0);
            break;
        case 5:
            pl->rot[1] += 0x2000;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, 0x590, 0, 0);
            break;
        case 6:
            pl->rot[1] -= 0x2000;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, 0x592, 0, 0);
            break;
        }
        break;
    case 1:
        switch (variant) {
        case 0:
        case 1:
        case 4:
            pl->step++;
            break;
        case 2:
            if (pl->motion[0].playing == 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x579, 2, 0x32);
            }
            break;
        case 3:
            if (Motion_ReachedFrame(pl, 0, 24.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x595, 2, 0);
            }
            break;
        case 5:
            if (Motion_ReachedFrame(pl, 0, 26.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x596, 2, 0);
            }
            break;
        case 6:
            if (pl->motion[0].playing == 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x579, 0, 0x34);
            }
            break;
        }
        break;
    case 2:
        switch (variant) {
        case 0:
        case 2:
        case 6:
            if (Motion_ReachedFrame(pl, 0, 58.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x594, GreatSword_OverheadBlends[variant].blend, GreatSword_OverheadBlends[variant].start);
                Player_UpdateMotionTime(pl, 0);
                PlayerAttack_Create(pl, 3);
            }
            break;
        case 1:
        case 3:
        case 4:
        case 5:
            if (pl->motion[0].playing == 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x594, GreatSword_OverheadBlends[variant].blend, GreatSword_OverheadBlends[variant].start);
                Player_UpdateMotionTime(pl, 0);
                PlayerAttack_Create(pl, 3);
            }
            break;
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        } else if (pl->hitPending != 0 && pl->stateVar1 == 0) {
            pl->stateVar1 = 1;
            CamShake_ReqA(pl, 0);
            switch (pl->hitKind) {
            case 0:
                StageObj_K05_Create(pl, 0xF, 0);
                break;
            case 1:
                pl->hitStop = 0;
                pl->hitStopAdd = 0;
                Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_BOUNCE, 0);
                break;
            case 2:
            case 3:
                pl->step++;
                Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
                pl->weaponSeMute = 2;
                CamShake_ReqA(pl, 1);
                break;
            }
        } else {
            if (pl->stateVar1 == 0 && Motion_PassedFrame(pl, 0, 42.0f) != 0) {
                point[0] = 0.0f;
                point[1] = 0.0f;
                point[2] = 150.0f;
                Vec_Rotate(out, point, pl->rotMtx);
                point[0] = pl->pos[0] + out[0];
                point[1] = 15.0f + (pl->pos[1] + out[1]);
                point[2] = pl->pos[2] + out[2];
                StageObj_K00_Create(pl, 0, point, 1);
                Vib_StartLocal(pl, 2);
                CamShake_ReqA(pl, 0);
            }
            if (Motion_ReachedFrame(pl, 0, 48.0f) != 0 && Motion_ReachedFrame(pl, 0, 102.0f) == 0) {
                GreatSword_CheckInput(pl, 3);
            }
        }
        break;
    case 4:
        if (pl->hitSlowTimer <= 0) {
            pl->step++;
            StageObj_K05_Create(pl, 0x1C, 0);
        }
        break;
    case 5:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        } else if (pl->hitPending != 0 && pl->stateVar1 == 0) {
            pl->stateVar1 = 1;
            CamShake_ReqA(pl, 0);
            switch (pl->hitKind) {
            case 0:
                StageObj_K05_Create(pl, 0xF, 0);
                break;
            case 1:
                pl->hitStop = 0;
                pl->hitStopAdd = 0;
                Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_BOUNCE, 0);
                break;
            case 2:
            case 3:
                pl->step++;
                Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
                pl->weaponSeMute = 2;
                CamShake_ReqA(pl, 1);
                break;
            }
        } else {
            if (pl->stateVar1 == 0 && Motion_PassedFrame(pl, 0, 42.0f) != 0) {
                point[0] = 0.0f;
                point[1] = 0.0f;
                point[2] = 150.0f;
                Vec_Rotate(out, point, pl->rotMtx);
                point[0] = pl->pos[0] + out[0];
                point[1] = 15.0f + (pl->pos[1] + out[1]);
                point[2] = pl->pos[2] + out[2];
                StageObj_K00_Create(pl, 0, point, 1);
                Vib_StartLocal(pl, 2);
                CamShake_ReqA(pl, 0);
            }
            if (Motion_ReachedFrame(pl, 0, 48.0f) != 0 && Motion_ReachedFrame(pl, 0, 102.0f) == 0) {
                GreatSword_CheckInput(pl, 3);
            }
        }
        break;
    case 6:
        if (pl->hitSlowTimer <= 0) {
            pl->step++;
            StageObj_K05_Create(pl, 0x1C, 0);
        }
        break;
    case 7:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        } else if (Motion_ReachedFrame(pl, 0, 48.0f) != 0 && Motion_ReachedFrame(pl, 0, 102.0f) == 0) {
            GreatSword_CheckInput(pl, 3);
        }
        break;
    }
}

/*
 * GSWORD_BOUNCE: the attack bounced off. A flick in frames 2-30 queues
 * GSWORD_BOUNCE_OVERHEAD at frame 18; else the state ends in idle.
 */
void GreatSword_StateBounce(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x57C, 0, 0);
        break;
    case 1:
        if (pl->stateVar0 != 0) {
            if (Motion_ReachedFrame(pl, 0, 18.0f) != 0) {
                Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_BOUNCE_OVERHEAD, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        } else if ((Motion_InFrameRange(pl, 0, 2.0f, 30.0f) != 0) && (pl->stateVar0 == 0) && (pl->itemSelect == 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            pl->stateVar0 = 1;
            pl->stateVar1 = 0;
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 0xA, 0);
        }
        break;
    }
}

/* GSWORD_STRIKE: one motion with hit object 4, then idle. */
void GreatSword_StateStrike(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x583, 0, 0);
        PlayerAttack_Create(pl, 4U);
        Player_SetMoveMode(pl, 0, 1, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * Returns the charge level from weaponTimer: 0 under 40 frames, then one more
 * per 20 frames, 3 at most.
 */
u8 GreatSword_ChargeLevel(Player *pl)
{
    s16 timer = pl->weaponTimer;
    u8 level;

    if (timer < 40) {
        return 0;
    }
    level = (s16) (timer - 40) / 20 + 1;
    if (level > 3) {
        level = 3;
    }
    return level;
}

/*
 * The charge, GSWORD_CHARGE and its forms. variant: 0 from idle, 1
 * GSWORD_DRAW_CHARGE, 2 after the right sweep, 3 after the left, 4 no lead-
 * in, 5 and 6 as 3 and 2 turned. For the local player, the charge motion runs
 * at half speed and weaponTimer counts while the stick is held down (90
 * frames at most); then chargeLevel is set and the charged slash requested
 * (after 3 and 5 their own forms).
 */
void GreatSword_StateCharge(Player *pl, u32 variant)
{
    f32 fromFrame;
    f32 toFrame;

    pl->noFlinchTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->weaponOut = 1;
        pl->unk417 = 1;
        pl->weaponTimer = 0;
        pl->timer = 90;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, 0x579, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x597, 2, 0);
            break;
        case 2:
            Player_SetMotionPair(pl, 0x58C, 4, 0);
            break;
        case 3:
            pl->step++;
            Player_SetMotionPair(pl, 0x58A, 4, 0);
            break;
        case 5:
            pl->step++;
            pl->rot[1] += 0x2000;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, 0x590, 0, 0);
            break;
        case 6:
            pl->rot[1] -= 0x2000;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, 0x592, 0, 0);
            break;
        }
        break;
    case 1:
        switch (variant) {
        default:
            /* the game reloads the step here (the cases reuse the switch value): volatile gives the reload */
            (*(volatile u8 *) &pl->step)++;
            break;
        case 1:
            if (pl->motion[0].playing == 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x579, 0, 0x38);
            }
            break;
        case 2:
            if (pl->motion[0].playing == 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x579, 2, 0x32);
            }
            break;
        case 6:
            if (pl->motion[0].playing == 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x579, 0, 0x34);
            }
            break;
        }
        break;
    case 2:
        if (Player_IsLocal(pl) != 0) {
            if (variant == 3) {
                toFrame = 24.0f;
                fromFrame = 0.0f;
            } else if (variant == 5) {
                toFrame = 26.0f;
                fromFrame = 2.0f;
            } else {
                toFrame = 58.0f;
                fromFrame = 34.0f;
            }
            if (Motion_ReachedFrame(pl, 0, fromFrame) != 0) {
                if (pl->stateVar1 == 0) {
                    pl->stateVar1++;
                    StageObj_K29_CreateOnPlayerDefault(pl, 8, 1.0f);
                }
                if (!(pl->input.stickDir & PAD_RSTICK_DOWN)) {
                    pl->stateVar0 = 1;
                }
                if (pl->stateVar0 == 0) {
                    pl->motionSpeed = 0.5f;
                    pl->weaponTimer++;
                    if (--pl->timer <= 0) {
                        pl->stateVar0 = 1;
                    }
                } else {
                    pl->motionSpeed = 1.0f;
                }
            }
            if (Motion_ReachedFrame(pl, 0, toFrame) != 0 && pl->stateVar0 == 1) {
                pl->chargeLevel = GreatSword_ChargeLevel(pl);
                pl->motionSpeed = 1.0f;
                switch (variant) {
                case 3:
                    Player_RequestState(pl, PL_GROUP_WEAPON, GSWORD_CHARGE_SLASH_AFTER_LEFT, PL_SET_SNAP_POS);
                    break;
                case 5:
                    Player_RequestState(pl, PL_GROUP_WEAPON, GSWORD_CHARGE_SLASH_AFTER_LEFT_TURN, PL_SET_SNAP_POS);
                    break;
                default:
                    Player_RequestState(pl, PL_GROUP_WEAPON, GSWORD_CHARGE_SLASH, PL_SET_SNAP_POS);
                    break;
                }
            }
        }
        break;
    }
}

/*
 * Sets the charged slash's attack by chargeLevel 0-3: attackPercent 100, 110,
 * 120 or 130, and hit object 3, 0x5B, 0x5C or 0x5D.
 */
void GreatSword_SetChargeAttack(Player *pl)
{

    switch (pl->chargeLevel) {
    case 0:
    default:
        pl->attackPercent = 100;
        PlayerAttack_Create(pl, 3);
        break;
    case 1:
        pl->attackPercent = 110;
        PlayerAttack_Create(pl, 0x5B);
        break;
    case 2:
        pl->attackPercent = 120;
        PlayerAttack_Create(pl, 0x5C);
        break;
    case 3:
        pl->attackPercent = 130;
        PlayerAttack_Create(pl, 0x5D);
        break;
    }
}

/*
 * The charged slash, GSWORD_CHARGE_SLASH and its forms. variant: 0 direct, 1
 * and 2 after the left sweep's link motions. Level 2 and up adds an effect;
 * level 3 replaces the ground impact. Window: frames 48-102 (caller 4).
 */
void GreatSword_StateChargeSlash(Player *pl, s32 variant)
{
    f32 point[4];
    f32 out[4];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0x3C;
        pl->unk417 = 1;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x594, 2, 0);
            Player_UpdateMotionTime(pl, 0);
            GreatSword_SetChargeAttack(pl);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x595, 0, 0);
            Player_UpdateMotionTime(pl, 0);
            break;
        case 2:
            Player_SetMotionPair(pl, 0x596, 0, 0);
            Player_UpdateMotionTime(pl, 0);
            break;
        }
        if (pl->chargeLevel >= 2) {
            StageObj_K29_CreateOnPlayerDefault(pl, 4, 1.0f);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_UpdateMotionTime(pl, 0);
            if (variant == 1) {
                Player_SetMotionPair(pl, 0x594, 2, 8);
            } else {
                Player_SetMotionPair(pl, 0x594, 0, 0xA);
            }
            GreatSword_SetChargeAttack(pl);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        } else if (pl->hitPending != 0 && pl->stateVar1 == 0) {
            pl->stateVar1 = 1;
            CamShake_ReqA(pl, 0);
            switch (pl->hitKind) {
            case 0:
                StageObj_K05_Create(pl, 0xF, 0);
                break;
            case 1:
                pl->hitStop = 0;
                pl->hitStopAdd = 0;
                Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_BOUNCE, 0);
                break;
            case 2:
            case 3:
                pl->step++;
                Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
                pl->weaponSeMute = 2;
                CamShake_ReqA(pl, 1);
                break;
            }
        } else {
            if (pl->stateVar1 == 0 && Motion_PassedFrame(pl, 0, 42.0f) != 0) {
                point[0] = 0.0f;
                point[1] = 0.0f;
                point[2] = 150.0f;
                Vec_Rotate(out, point, pl->rotMtx);
                point[0] = pl->pos[0] + out[0];
                point[1] = 15.0f + (pl->pos[1] + out[1]);
                point[2] = pl->pos[2] + out[2];
                if (pl->chargeLevel == 3) {
                    StageObj_K29_CreateOnPlayerDefault(pl, 0xA, 1.0f);
                } else {
                    StageObj_K00_Create(pl, 0, point, 1);
                }
                Vib_StartLocal(pl, 2);
                CamShake_ReqA(pl, 0);
            }
            if (Motion_ReachedFrame(pl, 0, 48.0f) != 0 && Motion_ReachedFrame(pl, 0, 102.0f) == 0) {
                GreatSword_CheckInput(pl, 4);
            }
        }
        break;
    case 3:
        if (pl->hitSlowTimer <= 0) {
            pl->step++;
            StageObj_K05_Create(pl, 0x1C, 0);
        }
        break;
    case 4:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        } else if (pl->hitPending != 0 && pl->stateVar1 == 0) {
            pl->stateVar1 = 1;
            CamShake_ReqA(pl, 0);
            switch (pl->hitKind) {
            case 0:
                StageObj_K05_Create(pl, 0xF, 0);
                break;
            case 1:
                pl->hitStop = 0;
                pl->hitStopAdd = 0;
                Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_BOUNCE, 0);
                break;
            case 2:
            case 3:
                pl->step++;
                Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
                pl->weaponSeMute = 2;
                CamShake_ReqA(pl, 1);
                break;
            }
        } else {
            if (pl->stateVar1 == 0 && Motion_PassedFrame(pl, 0, 42.0f) != 0) {
                point[0] = 0.0f;
                point[1] = 0.0f;
                point[2] = 150.0f;
                Vec_Rotate(out, point, pl->rotMtx);
                point[0] = pl->pos[0] + out[0];
                point[1] = 15.0f + (pl->pos[1] + out[1]);
                point[2] = pl->pos[2] + out[2];
                StageObj_K00_Create(pl, 0, point, 1);
                Vib_StartLocal(pl, 2);
                CamShake_ReqA(pl, 0);
            }
            if (Motion_ReachedFrame(pl, 0, 48.0f) != 0 && Motion_ReachedFrame(pl, 0, 102.0f) == 0) {
                GreatSword_CheckInput(pl, 4);
            }
        }
        break;
    case 5:
        if (pl->hitSlowTimer <= 0) {
            pl->step++;
            StageObj_K05_Create(pl, 0x1C, 0);
        }
        break;
    case 6:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        } else if (Motion_ReachedFrame(pl, 0, 48.0f) != 0 && Motion_ReachedFrame(pl, 0, 102.0f) == 0) {
            GreatSword_CheckInput(pl, 4);
        }
        break;
    }
}

/*
 * Runs the great sword's PL_GROUP_WEAPON state: maps the state to its handler
 * and variant.
 */
void GreatSword_RunState(Player *pl)
{
    switch (pl->state) {
    case GSWORD_SWEEP_LEFT:
        GreatSword_StateSweepLeft(pl, 0);
        break;
    case GSWORD_SWEEP_RIGHT:
        GreatSword_StateSweepRight(pl, 0);
        break;
    case GSWORD_OVERHEAD:
        GreatSword_StateOverhead(pl, 0U);
        break;
    case GSWORD_BOUNCE:
        GreatSword_StateBounce(pl, 0);
        break;
    case GSWORD_STRIKE:
        GreatSword_StateStrike(pl, 0);
        break;
    case GSWORD_SWEEP_LEFT_AFTER_OVERHEAD:
        GreatSword_StateSweepLeft(pl, 1);
        break;
    case GSWORD_SWEEP_LEFT_AFTER_RIGHT:
        GreatSword_StateSweepLeft(pl, 2);
        break;
    case GSWORD_SWEEP_LEFT_AFTER_OVERHEAD_TURN:
        GreatSword_StateSweepLeft(pl, 3);
        break;
    case GSWORD_SWEEP_LEFT_AFTER_RIGHT_TURN:
        GreatSword_StateSweepLeft(pl, 4);
        break;
    case GSWORD_SWEEP_RIGHT_AFTER_OVERHEAD:
        GreatSword_StateSweepRight(pl, 1);
        break;
    case GSWORD_SWEEP_RIGHT_AFTER_LEFT:
        GreatSword_StateSweepRight(pl, 2);
        break;
    case GSWORD_SWEEP_RIGHT_AFTER_OVERHEAD_TURN:
        GreatSword_StateSweepRight(pl, 3);
        break;
    case GSWORD_SWEEP_RIGHT_AFTER_LEFT_TURN:
        GreatSword_StateSweepRight(pl, 4);
        break;
    case GSWORD_DRAW_OVERHEAD:
        GreatSword_StateOverhead(pl, 1U);
        break;
    case GSWORD_OVERHEAD_AFTER_RIGHT:
        GreatSword_StateOverhead(pl, 2U);
        break;
    case GSWORD_OVERHEAD_AFTER_LEFT:
        GreatSword_StateOverhead(pl, 3U);
        break;
    case GSWORD_BOUNCE_OVERHEAD:
        GreatSword_StateOverhead(pl, 4U);
        break;
    case GSWORD_OVERHEAD_AFTER_LEFT_TURN:
        GreatSword_StateOverhead(pl, 5U);
        break;
    case GSWORD_OVERHEAD_AFTER_RIGHT_TURN:
        GreatSword_StateOverhead(pl, 6U);
        break;
    case GSWORD_CHARGE:
        GreatSword_StateCharge(pl, 0U);
        break;
    case GSWORD_DRAW_CHARGE:
        GreatSword_StateCharge(pl, 1U);
        break;
    case GSWORD_CHARGE_AFTER_RIGHT:
        GreatSword_StateCharge(pl, 2U);
        break;
    case GSWORD_CHARGE_AFTER_LEFT:
        GreatSword_StateCharge(pl, 3U);
        break;
    case GSWORD_CHARGE_NO_LEAD_IN:
        GreatSword_StateCharge(pl, 4U);
        break;
    case GSWORD_CHARGE_AFTER_LEFT_TURN:
        GreatSword_StateCharge(pl, 5U);
        break;
    case GSWORD_CHARGE_AFTER_RIGHT_TURN:
        GreatSword_StateCharge(pl, 6U);
        break;
    case GSWORD_CHARGE_SLASH:
        GreatSword_StateChargeSlash(pl, 0);
        break;
    case GSWORD_CHARGE_SLASH_AFTER_LEFT:
        GreatSword_StateChargeSlash(pl, 1);
        break;
    case GSWORD_CHARGE_SLASH_AFTER_LEFT_TURN:
        GreatSword_StateChargeSlash(pl, 2);
        
    default:
        break;
    }
}

/*
 * Returns the direction of a flick of the attack stick, from
 * input.rightStickAngle: 0 none, 1 left, 2 right, 3 up, 4 down.
 */
static s32 GreatSword_StickAttackDir(Player *pl)
{
    s32 dir;
    s32 ret;

    if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
        dir = (pl->input.rightStickAngle + 0x2AAB) & 0xFFFF;
        if (dir < 0x471C) {
            return 2;
        }
        if (dir < 0x8E3A) {
            return 3;
        }
        if (dir < 0xD556) {
            ret = 1;
        } else {
            ret = 4;
        }
        return ret;
    }
    return 0;
}

/*
 * The drawn idle's input: a flick starts the left or right sweep, the
 * overhead or the charge; input.pressed PAD_R3 the strike; the roll button rolls.
 * Returns 1 when it started a state.
 */
s32 GreatSword_AttackInput(Player *pl)
{

    switch (GreatSword_StickAttackDir(pl)) {
    case 0:
        break;
    case 1:
        Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_LEFT, PL_SET_SNAP_POS);
        return 1;
    case 2:
        Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_RIGHT, PL_SET_SNAP_POS);
        return 1;
    case 3:
        Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_OVERHEAD, PL_SET_SNAP_POS);
        return 1;
    case 4:
        Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_CHARGE, PL_SET_SNAP_POS);
        return 1;
    }
    if (pl->input.pressed & PAD_R3) {
        Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_STRIKE, PL_SET_SNAP_POS);
        return 1;
    }
    if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
        return 1;
    }
    return 0;
}

/*
 * An attack window's input. caller is the running attack (1 the left sweep, 2
 * the right, 3 the overhead, 4 the charged slash). The roll button evades by
 * the left stick; a flick another way starts the chained attack, its "_TURN"
 * form when the left stick points that way.
 */
void GreatSword_CheckInput(Player *pl, s32 caller)
{
    s32 k;
    s32 dir;

    if ((Player_IsLocal(pl) != 0 || Game_IsOnline() != 1) && Quest_PhaseIsOver(1) != 1 && pl->itemSelect == 0) {
        if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (u16) (pl->input.stickAngle + 0x2AAB);
                if (dir < 0x5C72) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_RIGHT, PL_SET_SNAP_POS);
                } else if (dir < 0xA38E) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_LEFT, PL_SET_SNAP_POS);
                }
            } else {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            }
        }
        k = GreatSword_StickAttackDir(pl);
        if (k == 0 || k == caller) {
            return;
        }
        switch (k) {
        case 1:
            switch (caller) {
            case 3:
                if (pl->input.stickDir & PAD_LEFT) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_LEFT_AFTER_OVERHEAD_TURN, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_LEFT_AFTER_OVERHEAD, PL_SET_SNAP_POS);
                }
                break;
            case 2:
                if (pl->input.stickDir & PAD_LEFT) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_LEFT_AFTER_RIGHT_TURN, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_LEFT_AFTER_RIGHT, PL_SET_SNAP_POS);
                }
                break;
            case 4:
                if (pl->input.stickDir & PAD_LEFT) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_LEFT_AFTER_OVERHEAD_TURN, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_LEFT_AFTER_OVERHEAD, PL_SET_SNAP_POS);
                }
                break;
            }
            break;
        case 2:
            switch (caller) {
            case 3:
                if (pl->input.stickDir & PAD_RIGHT) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_RIGHT_AFTER_OVERHEAD_TURN, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_RIGHT_AFTER_OVERHEAD, PL_SET_SNAP_POS);
                }
                break;
            case 1:
                if (pl->input.stickDir & PAD_RIGHT) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_RIGHT_AFTER_LEFT_TURN, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_RIGHT_AFTER_LEFT, PL_SET_SNAP_POS);
                }
                break;
            case 4:
                if (pl->input.stickDir & PAD_RIGHT) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_RIGHT_AFTER_OVERHEAD_TURN, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_SWEEP_RIGHT_AFTER_OVERHEAD, PL_SET_SNAP_POS);
                }
                break;
            }
            break;
        case 3:
            switch (caller) {
            case 2:
                if (pl->input.stickDir & PAD_RIGHT) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_OVERHEAD_AFTER_RIGHT_TURN, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_OVERHEAD_AFTER_RIGHT, PL_SET_SNAP_POS);
                }
                break;
            case 1:
                if (pl->input.stickDir & PAD_LEFT) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_OVERHEAD_AFTER_LEFT_TURN, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_OVERHEAD_AFTER_LEFT, PL_SET_SNAP_POS);
                }
                break;
            case 4:
                break;
            }
            break;
        case 4:
            switch (caller) {
            case 1:
                if (pl->input.stickDir & PAD_LEFT) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_CHARGE_AFTER_LEFT_TURN, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_CHARGE_AFTER_LEFT, PL_SET_SNAP_POS);
                }
                break;
            case 2:
                if (pl->input.stickDir & PAD_RIGHT) {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_CHARGE_AFTER_RIGHT_TURN, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_CHARGE_AFTER_RIGHT, PL_SET_SNAP_POS);
                }
                break;
            case 3:
                break;
            }
            break;
        }
    }
}

/*
 * The attack that draws on the move: GSWORD_DRAW_CHARGE with the stick held
 * down, else GSWORD_DRAW_OVERHEAD.
 */
void GreatSword_DrawAttack(Player *pl)
{
    if (pl->input.stickDir & PAD_RSTICK_DOWN) {
        Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_DRAW_CHARGE, PL_SET_SNAP_POS);
        return;
    }
    Player_SetState(pl, PL_GROUP_WEAPON, GSWORD_DRAW_OVERHEAD, PL_SET_SNAP_POS);
}

/*
 * Returns the light bowgun's rapid-fire ammo mask (a bit per ammo type, from
 * gRapidFireAmmo by the gun's id); 0 for a heavy bowgun.
 */
static u32 Bowgun_RapidFireMask(Player *pl)
{
    GunAmmoMask *row = gRapidFireAmmo;

    if (pl->weaponType != WEAPON_LIGHT_BOWGUN) {
        return 0;
    }
    for (; row->id != 999; row++) {
        if (row->id == pl->equip[6].id) {
            return row->mask;
        }
    }
    return 0;
}

/* Returns 1 when the loaded ammo type fires rapidly from this gun. */
s32 Bowgun_CanRapidFire(Player *pl)
{
    u32 mask = Bowgun_RapidFireMask(pl);

    if (mask == 0) {
        return 0;
    }
    if (mask & (1 << pl->ammoType)) {
        return 1;
    }
    return 0;
}

/*
 * Shows the aim in the two part-motion layers: motA when aimWeight is
 * positive, motB when negative, weighted |aimWeight| (at most 100); off at 0.
 */
static void Bowgun_SetAimLayers(Player *pl, u16 motA, u16 motB)
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

/*
 * The shot, BOWGUN_FIRE_RECOIL_0/1/2 and BOWGUN_RAPID_FIRE. row: the row of
 * Bowgun_FireMotions (1 recoil level 0, 0 level 1, 2 level 2); variant 1 is
 * rapid fire. Fires at frame 8 and spends the round (kept with skill 0x33,
 * except in rapid fire) and the pouch item; rapid fire shoots again from
 * frame 34, gAmmoRapidShots[ammo] more times.
 */
void Bowgun_StateFire(Player *pl, s32 row, s32 variant)
{
    s32 k;

    if (pl->aimLockTimer != 0) {
        pl->aimLockTimer = 5;
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        if (variant == 0) {
            pl->stateVar1 = 0;
        } else {
            pl->stateVar1 = gAmmoRapidShots[pl->ammoType][0];
        }
        Player_SetMoveMode(pl, 0, 1, 0);
        Bowgun_SetAimLayers(pl, Bowgun_FireMotions[row].aimA, Bowgun_FireMotions[row].aimB);
        Player_SetMotionPair(pl, Bowgun_FireMotions[row].mot, 0, 0);
        break;
    case 1:
    case 2:
        if (Motion_PassedFrame(pl, 1, 8.0f) != 0 && pl->step == 1) {
            pl->step++;
            Vib_StartLocal(pl, 0);
            CamShake_ReqB(pl, 1);
            if (variant == 0) {
                Bowgun_FireShot(pl, 0, 0xE, 1.0f);
            } else {
                Bowgun_FireShot(pl, 0, 0xE, 0.5f);
            }
            if (pl->stateVar0 <= 0) {
                pl->stateVar0++;
                if (Player_HasSkill(pl, 0x33) == 0 || variant == 1) {
                    pl->roundsLoaded--;
                }
                if (Quest_PhaseIsOver(1) == 0) {
                    Pouch_Add(pl, pl->pouch[pl->ammoSlot].item, -1);
                }
                if (pl->ammoCount == 0) {
                    pl->roundsLoaded = 0;
                }
                pl->savedAmmoCount = pl->ammoCount;
                pl->savedClipSize = pl->clipSize;
                pl->savedRoundsLoaded = pl->roundsLoaded;
            }
        }
        if (pl->aimMode == 0 && Motion_PassedFrame(pl, 0, 28.0f) != 0) {
            StageObj_Gun_CreateReload(pl, 2);
        }
        if (pl->motion[0].playing == 0) {
            Bowgun_SetAimLayers(pl, 0x3ED, 0x3EE);
            Player_ReturnToIdleKeepParts(pl, 0, 0, 0);
        } else if (Motion_ReachedFrame(pl, 0, 34.0f) != 0 && pl->stateVar1 > 0) {
            pl->step = 1;
            pl->stateVar1--;
            k = pl->stateVar1 > 0;
            if (pl->stateRand & 0x8000) {
                pl->stateRand = (pl->stateRand << 1) | 1;
            } else {
                pl->stateRand = pl->stateRand << 1;
            }
            Bowgun_SetAimLayers(pl, Bowgun_FireMotions[k].aimA, Bowgun_FireMotions[k].aimB);
            Player_SetMotionPair(pl, Bowgun_FireMotions[k].mot, 0, 0);
        }
        break;
    }
}

/*
 * BOWGUN_RELOAD: fills the clip from the pouch at once (at most ammoCount),
 * the motion's speed by Bowgun_ReloadLevel.
 */
void Bowgun_StateReload(Player *pl, s32 unused)
{

    if (pl->aimLockTimer != 0) {
        pl->aimLockTimer = 5;
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        Bowgun_SetAimLayers(pl, 0x586, 0x587);
        Player_SetMotionPair(pl, 0x57C, 0, 0);
        pl->roundsLoaded = pl->clipSize;
        if (pl->ammoCount < pl->roundsLoaded) {
            pl->roundsLoaded = pl->ammoCount;
        }
        Player_SetMoveMode(pl, 0, 0, 0);
        switch (Bowgun_ReloadLevel(pl, pl->ammoType)) {
        case 0:
            pl->motionSpeed = 1.25f;
            break;
        case 1:
        default:
            pl->motionSpeed = 1.0f;
            break;
        case 2:
            pl->motionSpeed = 0.75f;
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdleKeepParts(pl, 0, 0, 0);
        }
        break;
    }
}

/*
 * Draws, then reloads as Bowgun_StateReload. variant: 0 BOWGUN_DRAW_RELOAD, 1
 * BOWGUN_DRAW_RELOAD_MOVING.
 */
void Bowgun_StateDrawReload(Player *pl, s32 variant)
{

    if (pl->aimLockTimer != 0) {
        pl->aimLockTimer = 5;
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->weaponOut = 1;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x58E, 0, 0);
        } else {
            Player_SetMotionPair(pl, 0x58F, 0, 0);
        }
        pl->roundsLoaded = pl->clipSize;
        if (pl->ammoCount < pl->roundsLoaded) {
            pl->roundsLoaded = pl->ammoCount;
        }
        pl->stateVar0 = Bowgun_ReloadLevel(pl, pl->ammoType);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Bowgun_SetAimLayers(pl, 0x586, 0x587);
            Player_SetMotionPair(pl, 0x57C, 0, 0x1E);
            switch (pl->stateVar0) {
            case 0:
                pl->motionSpeed = 1.25f;
                break;
            case 1:
            default:
                pl->motionSpeed = 1.0f;
                break;
            case 2:
                pl->motionSpeed = 0.75f;
                break;
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdleKeepParts(pl, 0, 0, 0);
        }
        break;
    }
}

/* BOWGUN_DRY_FIRE: one motion, then idle with the aim layers kept. */
void Bowgun_StateDryFire(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Bowgun_SetAimLayers(pl, 0x588, 0x589);
        Player_SetMotionPair(pl, 0x584, 0, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdleKeepParts(pl, 0, 0, 0);
        }
        break;
    }
}

/* BOWGUN_MELEE: one motion with hit object 0x1D, then idle. */
void Bowgun_StateMelee(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x585, 4, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        PlayerAttack_Create(pl, 0x1DU);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/* Runs the bowguns' PL_GROUP_WEAPON state (heavy and light share them). */
void Bowgun_RunState(Player *pl)
{
    switch (pl->state) {
    case BOWGUN_FIRE_RECOIL_0:
        Bowgun_StateFire(pl, 1, 0);
        break;
    case BOWGUN_RELOAD:
        Bowgun_StateReload(pl, 0);
        break;
    case BOWGUN_DRY_FIRE:
        Bowgun_StateDryFire(pl, 0);
        break;
    case BOWGUN_MELEE:
        Bowgun_StateMelee(pl, 0);
        break;
    case BOWGUN_FIRE_RECOIL_1:
        Bowgun_StateFire(pl, 0, 0);
        break;
    case BOWGUN_FIRE_RECOIL_2:
        Bowgun_StateFire(pl, 2, 0);
        break;
    case BOWGUN_DRAW_RELOAD:
        Bowgun_StateDrawReload(pl, 0);
        break;
    case BOWGUN_DRAW_RELOAD_MOVING:
        Bowgun_StateDrawReload(pl, 1);
        break;
    case BOWGUN_RAPID_FIRE:
        Bowgun_StateFire(pl, 1, 1);
        
    default:
        break;
    }
}

/*
 * The drawn input: the roll button rolls; input.pressed PAD_R3 fires (rapid, by
 * recoil level, or dry); a flick down reloads when the clip isn't full; a
 * flick up is the melee outside the aim mode. Sets bowgunReady while loaded.
 * Returns 1 when it started a state.
 */
s32 Bowgun_AttackInput(Player *pl)
{

    if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
        return 1;
    }
    if (pl->roundsLoaded == 0) {
        if (pl->input.pressed & PAD_R3) {
            Player_SetState(pl, PL_GROUP_WEAPON, BOWGUN_DRY_FIRE, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
            return 1;
        }
    } else {
        pl->bowgunReady = 1;
        if (pl->input.pressed & PAD_R3) {
            if (Bowgun_CanRapidFire(pl) == 1) {
                Player_SetState(pl, PL_GROUP_WEAPON, BOWGUN_RAPID_FIRE, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
                return 1;
            }
            switch (Bowgun_RecoilLevel(pl, pl->ammoType)) {
            case 0:
                Player_SetState(pl, PL_GROUP_WEAPON, BOWGUN_FIRE_RECOIL_0, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
                break;
            case 1:
            default:
                Player_SetState(pl, PL_GROUP_WEAPON, BOWGUN_FIRE_RECOIL_1, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
                break;
            case 2:
                Player_SetState(pl, PL_GROUP_WEAPON, BOWGUN_FIRE_RECOIL_2, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
                break;
            }
            return 1;
        }
    }
    if (pl->roundsLoaded == 0 || (pl->roundsLoaded < pl->clipSize && pl->roundsLoaded < pl->ammoCount)) {
        if ((pl->input.stickDirPressed & PAD_RSTICK_DOWN) && pl->ammoCount > 0) {
            Player_RequestState(pl, PL_GROUP_WEAPON, BOWGUN_RELOAD, PL_SET_KEEP_PARTS | PL_SET_SNAP_POS);
            return 1;
        }
    }
    if ((pl->input.stickDirPressed & PAD_RSTICK_UP) && pl->aimLockTimer == 0 && pl->aimMode == 0) {
        Player_SetState(pl, PL_GROUP_WEAPON, BOWGUN_MELEE, PL_SET_SNAP_POS);
        return 1;
    }
    return 0;
}

/*
 * The draw with the stick held down and a clip to fill reloads
 * (BOWGUN_DRAW_RELOAD, _MOVING for a light bowgun on the move); else
 * PL_ACT_DRAW, or PL_ACT_DRAW_MOVING. moving: the caller's moving flags.
 */
void Bowgun_DrawAttack(Player *pl, s32 moving)
{
    u8 rounds;

    if (pl->input.stickDir & PAD_RSTICK_DOWN) {
        rounds = pl->roundsLoaded;
        if (rounds != 0) {
            if (((s32) rounds < (s32) pl->clipSize) && ((s32) rounds < pl->ammoCount)) {
                goto block_5;
            }
            goto block_13;
        }
block_5:
        if (pl->ammoCount > 0) {
            if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                Player_SetState(pl, PL_GROUP_WEAPON, BOWGUN_DRAW_RELOAD, PL_SET_SNAP_POS);
                return;
            }
            if (!(moving & 0xFF)) {
                Player_SetState(pl, PL_GROUP_WEAPON, BOWGUN_DRAW_RELOAD, PL_SET_SNAP_POS);
                return;
            }
            Player_SetState(pl, PL_GROUP_WEAPON, BOWGUN_DRAW_RELOAD_MOVING, PL_SET_SNAP_POS);
            return;
        }
        goto block_13;
    }
block_13:
    if ((pl->weaponType == WEAPON_HEAVY_BOWGUN) || !(moving & 0xFF)) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DRAW, 0);
        return;
    }
    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DRAW_MOVING, 0);
}

/*
 * The attack's hit, tracked in stateVar1. A bounce (hitKind 1) requests
 * HAMMER_BOUNCE and returns 1; kinds 2 and 3 step the motion back 2 frames
 * for the hit slow. Returns 0 otherwise.
 */
s32 Hammer_CheckHit(Player *pl)
{

    switch (pl->stateVar1) {
    case 0:
        if (pl->hitPending != 0) {
            CamShake_ReqA(pl, 0);
            switch (pl->hitKind) {
            case 0:
                pl->stateVar1++;
                break;
            case 1:
                pl->hitStop = 0;
                pl->hitStopAdd = 0;
                Player_RequestState(pl, PL_GROUP_WEAPON, HAMMER_BOUNCE, 0);
                return 1;
            case 2:
            case 3:
                pl->stateVar1++;
                Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
                pl->weaponSeMute = 2;
                CamShake_ReqA(pl, 1);
                break;
            }
        }
        break;
    case 1:
        if (pl->hitSlowTimer <= 0) {
            pl->stateVar1++;
        }
        break;
    case 2:
        break;
    }
    return 0;
}

/*
 * HAMMER_DOWN_ATTACK (hit object 6). In frames 50-80 a flick another way
 * chains HAMMER_COMBO_1_AFTER_DOWN; the charge button held at frame 72
 * charges (HAMMER_CHARGE_AFTER_ATTACK).
 */
void Hammer_StateDownAttack(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMotionPair(pl, 0x579, 4, 0);
        PlayerAttack_Create(pl, 6U);
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (Hammer_CheckHit(pl) != 1) {
            if (pl->motion[0].playing == 0) {
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            }
            if (Player_IsLocal(pl) == 1) {
                if (Motion_InFrameRange(pl, 0, 50.0f, 80.0f) != 0) {
                    Hammer_CheckInput(pl, 1);
                }
                if ((Motion_ReachedFrame(pl, 0, 72.0f) != 0) && (pl->input.held & PAD_R1)) {
                    Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_AFTER_ATTACK, 0);
                }
            }
        } else {
        }
        break;
    }
}

/*
 * The combo's first swing (hit object 7). variant: 0 HAMMER_COMBO_1, 1
 * HAMMER_COMBO_1_AFTER_DOWN (a link motion first). A flick in frames 30-88
 * queues HAMMER_COMBO_2 at frame 70; evade window frames 80-120.
 */
void Hammer_StateCombo1(Player *pl, s32 variant)
{
    u8 stateVar0;

    switch (pl->step) {
    case 0:
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->timer = 0;
        pl->stateVar1 = 0;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, 0x57A, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            Player_SetMoveMode(pl, 0, 1, 0);
            PlayerAttack_Create(pl, 7U);
            pl->noFlinchTimer = 0x1B;
            break;
        case 1:
            stateVar0 = pl->stateVar0;
            switch (stateVar0) {
            case 0:
                pl->stateVar0 = stateVar0 + 1;
                Player_UpdateMotionTime(pl, 0);
                Player_SetMotionPair(pl, 0x581, 4, 0);
                Player_SetMoveMode(pl, 0, 1, 0);
                pl->weaponOut = 1;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 0x57A, 0, 0x10);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 7U);
                }
                break;
            }
        }
        break;
    case 1:
        if (Hammer_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 58.0f) != 0) {
                Vib_StartLocal(pl, 2);
                CamShake_ReqA(pl, 2);
            }
            if ((Player_IsLocal(pl) == 1) && (Motion_InFrameRange(pl, 0, 30.0f, 88.0f) != 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY) && (((pl->input.rightStickAngle + 0x1555) & 0xFFFF) < 0xAAAC)) {
                pl->timer = 1;
            }
            if (pl->motion[0].playing == 0) {
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            }
            if (pl->hitSlowTimer == 0) {
                if (Motion_InFrameRange(pl, 0, 80.0f, 120.0f) != 0) {
                    Hammer_CheckInput(pl, 2);
                    break;
                }
                if ((Motion_ReachedFrame(pl, 0, 70.0f) != 0) && (pl->timer != 0)) {
                    Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_COMBO_2, PL_SET_SNAP_POS);
                }
            }
        }
        break;
    }
}

/*
 * HAMMER_COMBO_2 (hit object 0x21). A flick in frames 30-60 queues
 * HAMMER_COMBO_3 at the motion's end; else a recovery with the evade window.
 * variant is always 0.
 */
void Hammer_StateCombo2(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMotionPair(pl, 0x584, 2, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0;
        PlayerAttack_Create(pl, 0x21U);
        pl->timer = 0;
        break;
    case 1:
        if (Hammer_CheckHit(pl) != 1) {
            if (pl->motion[0].playing == 0) {
                if ((Player_IsLocal(pl) == 1) && (pl->timer != 0) && (variant == 0)) {
                    Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_COMBO_3, PL_SET_SNAP_POS);
                    break;
                }
                pl->step++;
                Player_SetMotionPair(pl, 0x57A, 4, 0x4A);
                break;
            }
            if ((Player_IsLocal(pl) == 1) && (Motion_InFrameRange(pl, 0, 30.0f, 60.0f) != 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY) && (((pl->input.rightStickAngle + 0x1555) & 0xFFFF) < 0xAAAC)) {
                pl->timer = 1;
            }
            if (Motion_PassedFrame(pl, 0, 52.0f) != 0) {
                Vib_StartLocal(pl, 2);
                CamShake_ReqA(pl, 2);
                break;
            }
        } else {
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (Motion_InFrameRange(pl, 0, 80.0f, 120.0f) != 0) {
            Hammer_CheckInput(pl, 2);
        }
        break;
    }
}

/*
 * The charge, standing. variant: 0 HAMMER_CHARGE, 1 HAMMER_CHARGE_HOLD (keeps
 * the charge time), 2 HAMMER_CHARGE_AFTER_ATTACK, 3 HAMMER_CHARGE_RESUME.
 * weaponTimer counts, stamina drains 1 a frame, and Hammer_CheckChargeInput
 * walks or releases.
 */
void Hammer_StateCharge(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->weaponOut = 1;
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->timer = 0;
        switch (variant) {
        case 0:
        case 3:
            pl->step = 2;
            if (variant == 3) {
                Player_SetMotionPair(pl, 0x3EE, 6, 0xC);
            } else {
                Player_SetMotionPair(pl, 0x3EE, 4, 0);
            }
            pl->weaponTimer = 0;
            Effect06_SpawnAtBone(pl, 6, 0, 2, 4.0f);
            pl->noFlinchTimer = 0x3C;
            break;
        case 1:
            pl->step = 3;
            Player_SetMotionPair(pl, 0x3EF, 8, 0);
            goto charge;
        case 2:
            pl->step = 1;
            Player_SetMotionPair(pl, 0x582, 4, 0);
            pl->weaponTimer = 0;
            pl->noFlinchTimer = 0x3C;
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x3EE, 0, 0xE);
            pl->weaponTimer = 0;
            Effect06_SpawnAtBone(pl, 6, 0, 2, 4.0f);
            pl->noFlinchTimer = 0x3C;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x3EF, 4, 0);
            pl->noFlinchTimer = 1;
        }
        break;
    case 3:
    charge:
        Player_TickWeaponTimer(pl);
        if (Player_IsLocal(pl) == 1) {
            pl->timer++;
            if (pl->timer >= 2) {
                Player_UseStamina(pl, -1);
            }
        }
        Hammer_CheckChargeInput(pl);
        break;
    }
}

/*
 * HAMMER_CHARGE_WALK: charges while walking; the moving flags make the
 * release a moving one.
 */
void Hammer_StateChargeWalk(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x3ED, 8, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        Player_SetFlag(pl, 0x02001000U);
        pl->timer = 0;
        
    case 1:
        Player_TickWeaponTimer(pl);
        if (Player_IsLocal(pl) == 1) {
            pl->timer++;
            if (pl->timer >= 2) {
                Player_UseStamina(pl, -1);
            }
        }
        Hammer_CheckChargeInput(pl);
        break;
    }
}

/*
 * HAMMER_CHARGE_SPIN: chargeLevel + 2 turns (5 at most), steered by the left
 * stick. A flick up ends it in HAMMER_CHARGE_RELEASE_1_MOVING (turns 1-2) or
 * HAMMER_SPIN_FINISH (turns 3-5); out of turns, a last strike.
 */
void Hammer_StateChargeSpin(Player *pl, s32 unused)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x57B, 6, 0);
        pl->stateVar0 = pl->chargeLevel + 2;
        if (pl->stateVar0 >= 5) {
            pl->stateVar0 = 5;
        }
        pl->stateVar1 = 0;
        pl->timer = 0;
        pl->targetYaw = pl->rot[1];
        PlayerAttack_Create(pl, 0xC);
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0x3C;
        break;
    case 1:
        pl->noFlinchTimer = 2;
        if (Motion_PassedFrame(pl, 0, 44.0f) != 0) {
            pl->timer++;
            if (--pl->stateVar0 <= 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x57C, 0, 0);
                pl->stateRand += 3;
                PlayerAttack_Create(pl, 0xF);
                break;
            }
            switch (pl->timer) {
            case 0:
                break;
            case 1:
            case 2:
                if (pl->stateVar1 != 0) {
                    if (pl->input.stickDir & PAD_LEFT) {
                        pl->rot[1] += 0x2000;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->input.stickDir & PAD_RIGHT) {
                        pl->rot[1] -= 0x2000;
                        pl->targetYaw = pl->rot[1];
                    }
                    Player_RequestState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_RELEASE_1_MOVING, PL_SET_SNAP_POS);
                }
                break;
            case 3:
            case 4:
            case 5:
                if (pl->stateVar1 != 0) {
                    if (pl->input.stickDir & PAD_LEFT) {
                        pl->rot[1] += 0x2000;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->input.stickDir & PAD_RIGHT) {
                        pl->rot[1] -= 0x2000;
                        pl->targetYaw = pl->rot[1];
                    }
                    Player_RequestState(pl, PL_GROUP_WEAPON, HAMMER_SPIN_FINISH, PL_SET_SNAP_POS);
                }
                break;
            }
        } else {
            if (Player_IsLocal(pl) == 1) {
                if (pl->input.stickDir & PAD_LEFT) {
                    pl->targetYaw += 0xC0;
                } else if (pl->input.stickDir & PAD_RIGHT) {
                    pl->targetYaw -= 0xC0;
                }
                if (pl->input.stickDirPressed & PAD_RSTICK_UP) {
                    pl->stateVar1++;
                }
            }
            if (Motion_PassedFrame(pl, 0, 26.0f) != 0) {
                PlayerAttack_Create(pl, 0xD);
            }
            if (Motion_PassedFrame(pl, 0, 52.0f) != 0) {
                PlayerAttack_Create(pl, 0xE);
            }
        }
        break;
    case 2:
        pl->noFlinchTimer = 2;
        if (Motion_PassedFrame(pl, 0, 38.0f) != 0) {
            Vib_StartLocal(pl, 2);
            CamShake_ReqA(pl, 1);
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * The releases. variant: 0 HAMMER_CHARGE_RELEASE_1 (two hits), 1 _RELEASE_0,
 * 2 _RELEASE_0_MOVING, 3 _RELEASE_1_MOVING, 4 HAMMER_BOUNCE_SWING. The moving
 * forms push forward for 30 frames; all but the first stop at frame 30. The
 * charge button held charges again.
 */
void Hammer_StateChargeRelease(Player *pl, s32 variant)
{
    f32 offset[4];
    f32 out[4];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        if (variant == 4) {
            Player_SetMotionPair(pl, 0x57D, 2, 0);
        } else {
            Player_SetMotionPair(pl, 0x57D, 4, 0);
        }
        Player_SetMoveMode(pl, 0, 1, 0);
        PlayerAttack_Create(pl, 8);
        pl->noFlinchTimer = 0x3C;
        break;
    case 1:
        if ((Motion_ReachedFrame(pl, 0, 30.0f) == 0) && (variant == 2 || variant == 3)) {
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 18.0f * Math_Cos((6.2831855f * (90.0f * (pl->motion[0].frame / 24.0f))) / 360.0f);
            Vec_Rotate(out, offset, pl->rotMtx);
            pl->pos[0] += out[0];
            pl->pos[2] += out[2];
        }
        if (variant == 0 || variant == 3) {
            if (Motion_PassedFrame(pl, 0, 60.0f) != 0) {
                PlayerAttack_Create(pl, 9);
            }
            if (pl->motion[0].playing == 0) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (Player_IsLocal(pl) == 1) {
                if ((Motion_ReachedFrame(pl, 0, 80.0f) != 0) && (pl->input.held & PAD_R1)) {
                    Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_RESUME, 0);
                } else if (Motion_ReachedFrame(pl, 0, 88.0f) != 0) {
                    Hammer_CheckInput(pl, 0);
                }
            }
        } else if ((Hammer_CheckHit(pl) != 1) && (Motion_ReachedFrame(pl, 0, 30.0f) != 0)) {
            pl->step++;
            Player_SetMotionPair(pl, 0x57F, 0, 0);
        }
        break;
    case 2:
        if (Hammer_CheckHit(pl) != 1) {
            if (pl->motion[0].playing == 0) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (Player_IsLocal(pl) == 1) {
                if ((Motion_ReachedFrame(pl, 0, 46.0f) != 0) && (pl->input.held & PAD_R1)) {
                    Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_AFTER_ATTACK, 0);
                } else if (Motion_InFrameRange(pl, 0, 32.0f, 46.0f) != 0) {
                    Hammer_CheckInput(pl, 0);
                }
            }
        }
        break;
    }
}

/*
 * HAMMER_CHARGE_RELEASE_2: two hits (0xA, then 0xB at frame 74), an impact at
 * frame 80; evade window frames 84-170.
 */
void Hammer_StateChargeRelease2(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x57E, 2, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        PlayerAttack_Create(pl, 0xAU);
        pl->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 74.0f) != 0) {
            PlayerAttack_Create(pl, 0xBU);
        }
        if (Motion_PassedFrame(pl, 0, 80.0f) != 0) {
            Vib_StartLocal(pl, 2);
            CamShake_ReqA(pl, 2);
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (Motion_InFrameRange(pl, 0, 84.0f, 170.0f) != 0) {
            Hammer_CheckInput(pl, 0);
        }
        break;
    }
}

/*
 * HAMMER_DRAW_ATTACK: draws into a swing (hit object 0x22 at frame 56). The
 * charge button held at frame 80 charges again; evade from frame 88.
 */
void Hammer_StateDrawAttack(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 1, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, 0x3F2, 2, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->weaponOut = 1;
        pl->unk417 = 1;
        pl->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x57D, 2, 0x2E);
            Player_UpdateMotionTime(pl, 0);
            pl->unk417 = 1;
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 56.0f) != 0) {
            PlayerAttack_Create(pl, 0x22U);
            pl->step++;
        }
        
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (Player_IsLocal(pl) == 1) {
            if ((Motion_ReachedFrame(pl, 0, 80.0f) != 0) && (pl->input.held & PAD_R1)) {
                Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_RESUME, 0);
                break;
            }
            if (Motion_ReachedFrame(pl, 0, 88.0f) != 0) {
                Hammer_CheckInput(pl, 0);
            }
        }
        break;
    }
}

/*
 * The finishing swing (hit object 0x20). variant: 0 HAMMER_SPIN_FINISH, 1
 * HAMMER_COMBO_3 (a lead-in motion first). Evade window frames 102-132.
 */
void Hammer_StateFinisher(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0x1C;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x583, 0, 0);
            PlayerAttack_Create(pl, 0x20U);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x585, 0, 0);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x583, 0, 0x20);
            PlayerAttack_Create(pl, 0x20U);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 40.0f) != 0) {
            Vib_StartLocal(pl, 2);
            CamShake_ReqA(pl, 1);
        }
        if (Motion_InFrameRange(pl, 0, 102.0f, 132.0f) != 0) {
            Hammer_CheckInput(pl, 0);
        }
        break;
    }
}

/*
 * HAMMER_BOUNCE: a flick in frames 2-30 queues HAMMER_BOUNCE_SWING at frame
 * 32; else idle.
 */
void Hammer_StateBounce(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x580, 0, 0);
        break;
    case 1:
        if (pl->stateVar0 != 0) {
            if (Motion_ReachedFrame(pl, 0, 32.0f) != 0) {
                Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_BOUNCE_SWING, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        } else if ((Motion_InFrameRange(pl, 0, 2.0f, 30.0f) != 0) && (pl->stateVar0 == 0) && (pl->itemSelect == 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            pl->stateVar0 = 1;
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 0xA, 0);
        }
        break;
    }
}

/*
 * Runs the hammer's PL_GROUP_WEAPON state: maps the state to its handler and
 * variant.
 */
void Hammer_RunState(Player *pl)
{
    switch (pl->state) {
    case HAMMER_DOWN_ATTACK:
        Hammer_StateDownAttack(pl, 0);
        break;
    case HAMMER_COMBO_1:
        Hammer_StateCombo1(pl, 0);
        break;
    case HAMMER_COMBO_2:
        Hammer_StateCombo2(pl, 0);
        break;
    case HAMMER_CHARGE:
        Hammer_StateCharge(pl, 0);
        break;
    case HAMMER_CHARGE_WALK:
        Hammer_StateChargeWalk(pl, 0);
        break;
    case HAMMER_CHARGE_SPIN:
        Hammer_StateChargeSpin(pl, 0);
        break;
    case HAMMER_CHARGE_RELEASE_1:
        Hammer_StateChargeRelease(pl, 0);
        break;
    case HAMMER_CHARGE_RELEASE_2:
        Hammer_StateChargeRelease2(pl, 0);
        break;
    case HAMMER_DRAW_ATTACK:
        Hammer_StateDrawAttack(pl, 0);
        break;
    case HAMMER_SPIN_FINISH:
        Hammer_StateFinisher(pl, 0);
        break;
    case HAMMER_BOUNCE:
        Hammer_StateBounce(pl, 0);
        break;
    case HAMMER_COMBO_1_AFTER_DOWN:
        Hammer_StateCombo1(pl, 1);
        break;
    case HAMMER_CHARGE_HOLD:
        Hammer_StateCharge(pl, 1);
        break;
    case HAMMER_CHARGE_AFTER_ATTACK:
        Hammer_StateCharge(pl, 2);
        break;
    case HAMMER_CHARGE_RESUME:
        Hammer_StateCharge(pl, 3);
        break;
    case HAMMER_CHARGE_RELEASE_0:
        Hammer_StateChargeRelease(pl, 1);
        break;
    case HAMMER_CHARGE_RELEASE_0_MOVING:
        Hammer_StateChargeRelease(pl, 2);
        break;
    case HAMMER_CHARGE_RELEASE_1_MOVING:
        Hammer_StateChargeRelease(pl, 3);
        break;
    case HAMMER_BOUNCE_SWING:
        Hammer_StateChargeRelease(pl, 4);
        break;
    case HAMMER_COMBO_3:
        Hammer_StateFinisher(pl, 1);
        
    default:
        break;
    }
}

/*
 * The drawn idle's input: a flick down starts HAMMER_DOWN_ATTACK, any other
 * way HAMMER_COMBO_1; the roll button rolls. Returns 1 when it started a
 * state.
 */
s32 Hammer_AttackInput(Player *pl)
{
    if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
        if (((pl->input.rightStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
            Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_COMBO_1, PL_SET_SNAP_POS);
            return 1;
        }
        Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_DOWN_ATTACK, PL_SET_SNAP_POS);
        return 1;
    }
    if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B)) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
        return 1;
    }
    return 0;
}

/*
 * An attack window's input: the roll button evades by the left stick; with
 * caller 1 (the down attack) a flick another way chains
 * HAMMER_COMBO_1_AFTER_DOWN.
 */
void Hammer_CheckInput(Player *pl, s32 caller)
{
    s32 dir;

    if (((Player_IsLocal(pl) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1) && (pl->itemSelect == 0)) {
        if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B)) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (pl->input.stickAngle + 0x4000) & 0xFFFF;
                if (dir < 0x5C72) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_RIGHT, PL_SET_SNAP_POS);
                } else if (dir < 0xA38E) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_LEFT, PL_SET_SNAP_POS);
                }
            } else {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            }
        }
        if ((pl->input.stickDirPressed & PL_RSTICK_ANY) && (caller == 1) && (((pl->input.rightStickAngle + 0x1555) & 0xFFFF) < 0xAAAC)) {
            Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_COMBO_1_AFTER_DOWN, PL_SET_SNAP_POS);
        }
    }
}

/* Returns the charge level, weaponTimer / 30. */
u8 Hammer_ChargeLevel(Player *pl)
{
    return pl->weaponTimer / 30;
}

/*
 * Requests the release for the charge level, standing or moving (player flag
 * 0x1000): level 0 HAMMER_CHARGE_RELEASE_0(_MOVING), level 1 _RELEASE_1
 * (_MOVING), level 2 and up _RELEASE_2 standing or HAMMER_CHARGE_SPIN moving
 * (which sets chargeLevel).
 */
void Hammer_ReleaseCharge(Player *pl)
{
    s32 level;

    level = Hammer_ChargeLevel(pl) & 0xFF;
    switch (level) {
    case 0:
        if (Player_TestFlag(pl, 0x1000U) != 0) {
            Player_RequestState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_RELEASE_0_MOVING, PL_SET_SNAP_POS);
            break;
        }
        Player_RequestState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_RELEASE_0, PL_SET_SNAP_POS);
        break;
    case 1:
        if (Player_TestFlag(pl, 0x1000U) != 0) {
            Player_RequestState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_RELEASE_1_MOVING, PL_SET_SNAP_POS);
            break;
        }
        Player_RequestState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_RELEASE_1, PL_SET_SNAP_POS);
        break;
    default:
    case 2:
        if (Player_TestFlag(pl, 0x1000U) != 0) {
            pl->chargeLevel = Hammer_ChargeLevel(pl);
            Player_RequestState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_SPIN, PL_SET_SNAP_POS);
            break;
        }
        Player_RequestState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_RELEASE_2, PL_SET_SNAP_POS);
        break;
    }
}

/*
 * The local player's input while charging: the roll button rolls; while the
 * charge button is held with stamina, the left stick walks
 * (HAMMER_CHARGE_WALK) or stands (HAMMER_CHARGE_HOLD) and a flick up releases
 * early; letting go releases.
 */
void Hammer_CheckChargeInput(Player *pl)
{

    if (Player_IsLocal(pl) != 0) {
        if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            return;
        }
        if ((pl->input.held & PAD_R1) && pl->stamina > 0) {
            if (Player_StickMoveLevel(pl, 0) != 0) {
                if (Player_IsState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_WALK) == 0) {
                    Player_FaceStick(pl);
                    Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_WALK, 0);
                } else {
                    Player_FaceStick(pl);
                }
            } else if (pl->stateGroup == PL_GROUP_WEAPON) {
                switch (pl->state) {
                case HAMMER_CHARGE:
                case HAMMER_CHARGE_HOLD:
                case HAMMER_CHARGE_AFTER_ATTACK:
                case HAMMER_CHARGE_RESUME:
                    break;
                default:
                    Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_HOLD, 0);
                    break;
                }
            } else {
                Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE_HOLD, 0);
            }
            if ((pl->input.stickDirPressed & PAD_RSTICK_UP) && pl->itemSelect == 0) {
                Hammer_ReleaseCharge(pl);
            }
        } else {
            Hammer_ReleaseCharge(pl);
        }
    }
}

/* The attack that draws on the move: HAMMER_DRAW_ATTACK. */
void Hammer_DrawAttack(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_DRAW_ATTACK, PL_SET_SNAP_POS);
}

/*
 * Charges from a roll (frame 40 with the charge button held): HAMMER_CHARGE.
 */
void Hammer_ChargeFromRoll(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE, PL_SET_SNAP_POS);
}

/* The charge button held (Player_HoldSpecialInput): HAMMER_CHARGE. */
void Hammer_StartCharge(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, HAMMER_CHARGE, PL_SET_SNAP_POS);
}

/*
 * When the thrust bounced off (hitKind 1): shakes the camera, clears the hit
 * stop, requests LANCE_BOUNCE and returns 1; else 0.
 */
s32 Lance_CheckHit(Player *pl)
{
    if ((pl->hitPending != 0) && (pl->hitKind == 1)) {
        CamShake_ReqA(pl, 0);
        pl->hitStop = 0;
        pl->hitStopAdd = 0;
        Player_RequestState(pl, PL_GROUP_WEAPON, LANCE_BOUNCE, 0);
        return 1;
    }
    return 0;
}

/*
 * The thrusts. variant: 0 LANCE_THRUST, 1 LANCE_DRAW_THRUST (the draw motion
 * first), 2 LANCE_THRUST_2, 3 LANCE_THRUST_3, 4 LANCE_BOUNCE_THRUST. Frames
 * 36-62 run Lance_CheckInput, and a flick chains the next thrust (down an
 * upper thrust); LANCE_THRUST_3 chains none.
 */
void Lance_StateThrust(Player *pl, s32 variant)
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
            PlayerAttack_Create(pl, 0x10);
            break;
        case 1:
            pl->step++;
            Player_SetMotionPair(pl, 0x3F2, 2, 0);
            pl->weaponOut = 1;
            PlayerAttack_Create(pl, 0x14);
            break;
        case 2:
        case 3:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57D, 6, 0xA);
            if (variant == 3) {
                PlayerAttack_Create(pl, 0x24);
                break;
            }
            PlayerAttack_Create(pl, 0x10);
            break;
        case 4:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57D, 4, 0xA);
            PlayerAttack_Create(pl, 0x10);
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
        if (Lance_CheckHit(pl) != 1 && Player_IsLocal(pl) == 1 && Motion_InFrameRange(pl, 0, 36.0f, 62.0f) != 0) {
            Lance_CheckInput(pl, 0);
            if ((pl->input.stickDirPressed & PL_RSTICK_ANY) && variant != 3) {
                if (((pl->input.rightStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
                    if (variant == 2) {
                        Player_SetState(pl, PL_GROUP_WEAPON, LANCE_THRUST_3, PL_SET_SNAP_POS);
                        break;
                    }
                    Player_SetState(pl, PL_GROUP_WEAPON, LANCE_THRUST_2, PL_SET_SNAP_POS);
                    break;
                }
                if (variant == 2) {
                    Player_SetState(pl, PL_GROUP_WEAPON, LANCE_UPPER_THRUST_3, PL_SET_SNAP_POS);
                    break;
                }
                Player_SetState(pl, PL_GROUP_WEAPON, LANCE_UPPER_THRUST_2, PL_SET_SNAP_POS);
            }
        }
        break;
    }
}

/*
 * The upper thrusts (likely). variant: 0 LANCE_UPPER_THRUST, 1 _2, 2 _3; they
 * chain as Lance_StateThrust's.
 */
void Lance_StateUpperThrust(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        switch (variant) {
        default:
            Player_SetMotionPair(pl, 0x57E, 4, 0);
            PlayerAttack_Create(pl, 0x1C);
            break;
        case 1:
        case 2:
            Player_SetMotionPair(pl, 0x57E, -6, 0xA);
            if (variant == 2) {
                PlayerAttack_Create(pl, 0x23);
                break;
            }
            PlayerAttack_Create(pl, 0x1C);
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (Lance_CheckHit(pl) != 1 && Player_IsLocal(pl) == 1 && Motion_InFrameRange(pl, 0, 36.0f, 62.0f) != 0) {
            Lance_CheckInput(pl, 0);
            if ((pl->input.stickDirPressed & PL_RSTICK_ANY) && variant != 2) {
                if (((pl->input.rightStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
                    if (variant == 1) {
                        Player_SetState(pl, PL_GROUP_WEAPON, LANCE_THRUST_3, PL_SET_SNAP_POS);
                        break;
                    }
                    Player_SetState(pl, PL_GROUP_WEAPON, LANCE_THRUST_2, PL_SET_SNAP_POS);
                    break;
                }
                if (variant == 1) {
                    Player_SetState(pl, PL_GROUP_WEAPON, LANCE_UPPER_THRUST_3, PL_SET_SNAP_POS);
                    break;
                }
                Player_SetState(pl, PL_GROUP_WEAPON, LANCE_UPPER_THRUST_2, PL_SET_SNAP_POS);
            }
        }
        break;
    }
}

/*
 * LANCE_GUARD_THRUST: frames 18-28 run Lance_CheckInput; it ends back in the
 * held guard (PL_REACT_GUARD_HOLD).
 */
void Lance_StateGuardThrust(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x588, 2, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        PlayerAttack_Create(pl, 0x12U);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
            break;
        }
        if ((Lance_CheckHit(pl) != 1) && (Motion_ReachedFrame(pl, 0, 18.0f) != 0) && (Motion_ReachedFrame(pl, 0, 28.0f) == 0)) {
            Lance_CheckInput(pl, 0);
        }
        break;
    }
}

/*
 * The charge. variant: 0 LANCE_CHARGE (a start motion), 1
 * LANCE_CHARGE_RESUME. Stamina drains; the left stick steers; the charge
 * button or a flick down stops it and a flick up finishes it. A hit reacts by
 * sharpness (level 0 a knockdown, 1 LANCE_CHARGE_HIT_STOP, 2
 * LANCE_CHARGE_HIT; skills 0x24/0x25 skip this); a wall gives
 * LANCE_CHARGE_WALL.
 */
void Lance_StateCharge(Player *pl, s32 variant)
{
    u8 level;

    pl->noFlinchTimer = 2;
    switch (pl->step) {
    case 0:
        if (variant == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x579, 4, 0);
        } else {
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57A, 0, 6);
            PlayerAttack_Create(pl, 0x11);
        }
        pl->hitPending = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x57A, 0, 4);
            PlayerAttack_Create(pl, 0x11);
        }
        break;
    case 2:
        if (Player_IsLocal(pl) == 1) {
            if (pl->input.stickDir & PAD_LEFT) {
                pl->targetYaw += 0x80;
            }
            if (pl->input.stickDir & PAD_RIGHT) {
                pl->targetYaw -= 0x80;
            }
        }
        Player_UseStamina(pl, -2);
        if (pl->stamina <= 0) {
            Player_SetState(pl, PL_GROUP_WEAPON, LANCE_CHARGE_STOP, PL_SET_SNAP_POS);
            break;
        }
        if (Player_IsLocal(pl) == 1) {
            if ((pl->input.stickDirPressed & PAD_RSTICK_DOWN) || (pl->input.pressed & PAD_R3)) {
                Player_SetState(pl, PL_GROUP_WEAPON, LANCE_CHARGE_STOP, PL_SET_SNAP_POS);
            } else if (pl->input.stickDirPressed & PAD_RSTICK_UP) {
                Player_SetState(pl, PL_GROUP_WEAPON, LANCE_CHARGE_FINISH, PL_SET_SNAP_POS);
            }
        }
        if (pl->hitPending != 0) {
            if (Player_HasSkill(pl, 0x24) == 0 && Player_HasSkill(pl, 0x25) == 0) {
                if (pl->hitPending & 0x80) {
                    level = 0;
                } else {
                    level = pl->sharpnessLevel;
                }
                switch (level) {
                case 0:
                    pl->hitStop = 0;
                    pl->hitStopAdd = 0;
                    Player_SetState(pl, PL_GROUP_REACT, PL_REACT_KNOCKDOWN_FRONT, 0);
                    break;
                case 2:
                    pl->hitStop = 0;
                    pl->hitStopAdd = 0;
                    Player_RequestState(pl, PL_GROUP_WEAPON, LANCE_CHARGE_HIT, 0);
                    break;
                case 1:
                    pl->hitStop = 0;
                    pl->hitStopAdd = 0;
                    Player_RequestState(pl, PL_GROUP_WEAPON, LANCE_CHARGE_HIT_STOP, 0);
                    break;
                }
            }
        } else if (pl->wallHitDirs & 0xE0000007) {
            Player_RequestState(pl, PL_GROUP_WEAPON, LANCE_CHARGE_WALL, 0);
        }
        break;
    }
}

/*
 * The charge's end, then idle. variant: 0 LANCE_CHARGE_STOP, 1
 * LANCE_CHARGE_HIT_STOP.
 */
void Lance_StateChargeStop(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        pl->noFlinchTimer = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x57B, 2, 0);
            break;
        }
        Player_SetMotionPair(pl, 0x583, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * variant 0 LANCE_CHARGE_HIT: stamina drains, then LANCE_CHARGE_RESUME, or
 * LANCE_CHARGE_STOP after the charge button or a flick down; variant 1
 * LANCE_CHARGE_WALL: always LANCE_CHARGE_STOP.
 */
void Lance_StateChargeHit(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->hitStop = 0;
        pl->hitStopAdd = 0;
        Player_SetMotionPair(pl, 0x57C, 0, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0;
        break;
    case 1:
        Player_UseStamina(pl, -2);
        if (pl->motion[0].playing == 0) {
            if ((variant == 0) && (pl->stateVar0 == 0)) {
                Player_RequestState(pl, PL_GROUP_WEAPON, LANCE_CHARGE_RESUME, 0);
                break;
            }
            Player_RequestState(pl, PL_GROUP_WEAPON, LANCE_CHARGE_STOP, 0);
            break;
        }
        if ((pl->input.stickDirPressed & PAD_RSTICK_DOWN) || (pl->input.pressed & PAD_R3)) {
            pl->stateVar0 = 1;
        }
        break;
    }
}

/*
 * LANCE_CHARGE_FINISH: the charge's last thrust, without flinching for 60
 * frames. Frames 72-102 run Lance_CheckInput.
 */
void Lance_StateChargeFinish(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x584, 2, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0x3C;
        PlayerAttack_Create(pl, 0x1FU);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->noFlinchTimer = 0;
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (Motion_InFrameRange(pl, 0, 72.0f, 102.0f) != 0) {
            Lance_CheckInput(pl, 0);
        }
        break;
    }
}

/*
 * LANCE_BOUNCE: a flick in frames 10-40 queues LANCE_BOUNCE_THRUST at frame
 * 42.
 */
void Lance_StateBounce(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x589, 0, 0);
        CamShake_ReqA(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->stateVar0 != 0) {
            if (Motion_ReachedFrame(pl, 0, 42.0f) != 0) {
                Player_SetState(pl, PL_GROUP_WEAPON, LANCE_BOUNCE_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        } else if ((Motion_InFrameRange(pl, 0, 10.0f, 40.0f) != 0) && (pl->stateVar0 == 0) && (pl->itemSelect == 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            pl->stateVar0 = 1;
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * Runs the lance's PL_GROUP_WEAPON state: maps the state to its handler and
 * variant.
 */
void Lance_RunState(Player *pl)
{
    switch (pl->state) {
    case LANCE_THRUST:
        Lance_StateThrust(pl, 0);
        break;
    case LANCE_UPPER_THRUST:
        Lance_StateUpperThrust(pl, 0);
        break;
    case LANCE_GUARD_THRUST:
        Lance_StateGuardThrust(pl, 0);
        break;
    case LANCE_CHARGE:
        Lance_StateCharge(pl, 0);
        break;
    case LANCE_CHARGE_STOP:
        Lance_StateChargeStop(pl, 0);
        break;
    case LANCE_CHARGE_HIT:
        Lance_StateChargeHit(pl, 0);
        break;
    case LANCE_CHARGE_FINISH:
        Lance_StateChargeFinish(pl, 0);
        break;
    case LANCE_BOUNCE:
        Lance_StateBounce(pl, 0);
        break;
    case LANCE_DRAW_THRUST:
        Lance_StateThrust(pl, 1);
        break;
    case LANCE_THRUST_2:
        Lance_StateThrust(pl, 2);
        break;
    case LANCE_THRUST_3:
        Lance_StateThrust(pl, 3);
        break;
    case LANCE_BOUNCE_THRUST:
        Lance_StateThrust(pl, 4);
        break;
    case LANCE_UPPER_THRUST_2:
        Lance_StateUpperThrust(pl, 1);
        break;
    case LANCE_UPPER_THRUST_3:
        Lance_StateUpperThrust(pl, 2);
        break;
    case LANCE_CHARGE_RESUME:
        Lance_StateCharge(pl, 1);
        break;
    case LANCE_CHARGE_HIT_STOP:
        Lance_StateChargeStop(pl, 1);
        break;
    case LANCE_CHARGE_WALL:
        Lance_StateChargeHit(pl, 1);
        
    default:
        break;
    }
}

/*
 * The drawn idle's input: the roll button hops back (75 stamina), the charge
 * button charges, a flick thrusts (down an upper thrust). Returns 0.
 */
s32 Lance_AttackInput(Player *pl)
{

    if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_BACK, PL_SET_SNAP_POS);
    }
    if (pl->input.pressed & PAD_R3) {
        Player_SetState(pl, PL_GROUP_WEAPON, LANCE_CHARGE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
    }
    if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
        if ((u16)(pl->input.rightStickAngle + 0x1555) < 0xAAAC) {
            Player_SetState(pl, PL_GROUP_WEAPON, LANCE_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Player_SetState(pl, PL_GROUP_WEAPON, LANCE_UPPER_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
    }
    return 0;
}

/*
 * An attack window's input (the local player, no item selector open): the
 * roll button hops (the left stick picks the side, or the long back hop), the
 * charge button charges. variant is not used (always 0).
 */
void Lance_CheckInput(Player *pl, s32 variant)
{
    s32 dir;

    if ((Player_IsLocal(pl) != 0 || Game_IsOnline() != 1) && Quest_PhaseIsOver(1) != 1 && pl->itemSelect == 0) {
        if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (u16) (pl->input.stickAngle + 0x2000);
                if (dir < 0x6000) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_SIDE_B, PL_SET_SNAP_POS);
                } else if (dir >= 0x6000 && dir < 0xC001) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_SIDE_A, PL_SET_SNAP_POS);
                } else {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_BACK_LONG, PL_SET_SNAP_POS);
                }
            } else {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_BACK, PL_SET_SNAP_POS);
            }
        }
        if (pl->input.pressed & PAD_R3) {
            Player_SetState(pl, PL_GROUP_WEAPON, LANCE_CHARGE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
    }
}

/*
 * From the guard (Player_GuardInput): thrust 0 charges (LANCE_CHARGE), 1
 * thrusts (LANCE_GUARD_THRUST).
 */
void Lance_GuardAttack(Player *pl, s32 thrust)
{
    if (thrust == 0) {
        Player_SetState(pl, PL_GROUP_WEAPON, LANCE_CHARGE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        return;
    }
    Player_SetState(pl, PL_GROUP_WEAPON, LANCE_GUARD_THRUST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
}

/* The attack that draws on the move: LANCE_DRAW_THRUST. */
void Lance_DrawAttack(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, LANCE_DRAW_THRUST, PL_SET_SNAP_POS);
}

/*
 * The attack's first hit, once per state (stateVar1): a bounce requests
 * SNS_BOUNCE and returns 1; kinds 2 and 3 step the motion back 2 frames and
 * mute the weapon sound. Returns 0 otherwise.
 */
s32 SwordShield_CheckHit(Player *pl)
{

    if (pl->hitPending != 0 && pl->stateVar1 == 0) {
        pl->stateVar1 = 1;
        switch (pl->hitKind) {
        case 0:
            break;
        case 1:
            pl->hitStopAdd = pl->hitStop = 0;
            Player_SetState(pl, PL_GROUP_WEAPON, SNS_BOUNCE, 0);
            return 1;
        case 2:
        case 3:
            Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
            pl->weaponSeMute = 2;
            break;
        }
    }
    return 0;
}

/*
 * The flick-up combo. variant: 0 SNS_COMBO_1, 1 SNS_COMBO_2, 2 SNS_COMBO_3, 3
 * SNS_COMBO_1_LINKED. A queued flick chains: up the next slash (none after
 * the third), sideways SNS_SIDE_SLASH; else the evade window.
 */
void SwordShield_StateCombo(Player *pl, s32 variant)
{
    f32 fromFrame;
    f32 toFrame;
    s32 dir;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, 0x579, 4, 0);
            PlayerAttack_Create(pl, 0x15);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x57A, 2, 0);
            PlayerAttack_Create(pl, 0x19);
            break;
        case 2:
            Player_SetMotionPair(pl, 0x57B, 2, 0);
            PlayerAttack_Create(pl, 0x1A);
            break;
        case 3:
            Player_SetMotionPair(pl, 0x579, 2, 0xA);
            PlayerAttack_Create(pl, 0x15);
            StageObj_K05_Create(pl, 8, 1);
            break;
        }
        break;
    case 1:
        if (pl->stateVar0 == 0) {
            fromFrame = 10.0f;
            toFrame = 50.0f;
            if (variant == 2) {
                fromFrame = 30.0f;
                toFrame = 70.0f;
            }
            if (Motion_ReachedFrame(pl, 0, fromFrame) != 0 && Motion_ReachedFrame(pl, 0, toFrame) == 0 && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
                dir = (u16) (pl->input.rightStickAngle + 0x2AAB);
                if (dir < 0x4001) {
                    pl->stateVar0 = 2;
                } else if (dir < 0x9556) {
                    pl->stateVar0 = 1;
                } else if (dir < 0xD556) {
                    pl->stateVar0 = 2;
                }
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (SwordShield_CheckHit(pl) == 1) {
            break;
        }
        switch (variant) {
        case 0:
        case 3:
        case 4:
            if (Motion_ReachedFrame(pl, 0, 24.0f) != 0) {
                if (pl->stateVar0 != 0) {
                    if (pl->stateVar0 == 1) {
                        Player_SetState(pl, PL_GROUP_WEAPON, SNS_COMBO_2, PL_SET_SNAP_POS);
                    } else {
                        Player_SetState(pl, PL_GROUP_WEAPON, SNS_SIDE_SLASH, PL_SET_SNAP_POS);
                    }
                } else if (Motion_ReachedFrame(pl, 0, 50.0f) == 0) {
                    SwordShield_CheckInput(pl, 0);
                }
            }
            break;
        case 1:
            if (Motion_ReachedFrame(pl, 0, 20.0f) != 0) {
                if (pl->stateVar0 != 0) {
                    if (pl->stateVar0 == 1) {
                        Player_SetState(pl, PL_GROUP_WEAPON, SNS_COMBO_3, PL_SET_SNAP_POS);
                    } else {
                        Player_SetState(pl, PL_GROUP_WEAPON, SNS_SIDE_SLASH, PL_SET_SNAP_POS);
                    }
                } else if (Motion_ReachedFrame(pl, 0, 50.0f) == 0) {
                    SwordShield_CheckInput(pl, 0);
                }
            }
            break;
        case 2:
            if (Motion_ReachedFrame(pl, 0, 64.0f) != 0) {
                if (pl->stateVar0 == 2) {
                    Player_SetState(pl, PL_GROUP_WEAPON, SNS_SIDE_SLASH, PL_SET_SNAP_POS);
                } else if (Motion_ReachedFrame(pl, 0, 92.0f) == 0) {
                    SwordShield_CheckInput(pl, 0);
                }
            }
            break;
        }
        if (Motion_PassedFrame(pl, 0, 52.0f) != 0 && variant == 2) {
            PlayerAttack_Create(pl, 0x1B);
        }
        break;
    }
}

/*
 * The jump slash (likely). variant: 0 SNS_JUMP_SLASH, 1 SNS_DRAW_JUMP_SLASH,
 * 2 SNS_BOUNCE_JUMP_SLASH. A flick up in frames 20-42 queues SNS_RISING_SLASH
 * at frame 34.
 */
void SwordShield_StateJumpSlash(Player *pl, s32 variant)
{
    s32 dir;

    switch (pl->step) {
    case 0:
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57C, 2, 0);
            PlayerAttack_Create(pl, 0x16U);
            pl->noFlinchTimer = 0x64;
            break;
        case 1:
            pl->step++;
            pl->weaponOut = 1;
            Player_SetMotionPair(pl, 0x3F2, 2, 0);
            pl->noFlinchTimer = 0x64;
            break;
        case 2:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57C, 4, 0);
            PlayerAttack_Create(pl, 0x16U);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x57C, 0, 8);
            PlayerAttack_Create(pl, 0x16U);
        }
        break;
    case 2:
        if ((pl->stateVar0 == 0) && (Motion_ReachedFrame(pl, 0, 20.0f) != 0) && (Motion_ReachedFrame(pl, 0, 42.0f) == 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            dir = (pl->input.rightStickAngle + 0x2AAB) & 0xFFFF;
            if ((dir > 16384) && (dir < 0x9556)) {
                pl->stateVar0 = 1;
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if ((SwordShield_CheckHit(pl) != 1) && (Motion_ReachedFrame(pl, 0, 34.0f) != 0)) {
            if (pl->stateVar0 != 0) {
                Player_SetState(pl, PL_GROUP_WEAPON, SNS_RISING_SLASH, PL_SET_SNAP_POS);
                break;
            }
            if (Motion_ReachedFrame(pl, 0, 42.0f) == 0) {
                SwordShield_CheckInput(pl, 0);
            }
        }
        break;
    }
}

/* SNS_SIDE_SLASH: no attack chains; the evade from frame 40. */
void SwordShield_StateSideSlash(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMotionPair(pl, 0x57D, 4, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        PlayerAttack_Create(pl, 0x17U);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if ((SwordShield_CheckHit(pl) != 1) && (Motion_ReachedFrame(pl, 0, 20.0f) != 0) && (Motion_ReachedFrame(pl, 0, 40.0f) != 0)) {
            SwordShield_CheckInput(pl, 0);
        }
        break;
    }
}

/* SNS_GUARD_SLASH: slashes from the guard, back to PL_REACT_GUARD_HOLD. */
void SwordShield_StateGuardSlash(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMotionPair(pl, 0x588, 4, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        PlayerAttack_Create(pl, 0x18U);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
            break;
        }
        SwordShield_CheckHit(pl);
        break;
    }
}

/*
 * The rising slash (likely). variant: 0 SNS_RISING_SLASH, 1
 * SNS_EVADE_RISING_SLASH, 2 SNS_SHIELD_BASH (hit object 0x64 at frame 18,
 * then the rising slash's motion). At frame 26 a flick up chains
 * SNS_COMBO_1_LINKED, sideways SNS_SIDE_SLASH.
 */
void SwordShield_StateRisingSlash(Player *pl, s32 variant)
{
    s32 dir;
    u8 stateVar0;
    u8 attackMade;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57E, 2, 0);
            PlayerAttack_Create(pl, 0x1EU);
            break;
        case 1:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57E, 2, 8);
            PlayerAttack_Create(pl, 0x1EU);
            break;
        case 2:
            Player_SetMotionPair(pl, 0x589, 2, 0);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->stateVar0 = 0;
            Player_SetMotionPair(pl, 0x57E, 0, 0x10);
            break;
        }
        if (Motion_ReachedFrame(pl, 0, 18.0f) != 0) {
            attackMade = pl->stateVar0;
            if (attackMade == 0) {
                pl->stateVar0 = attackMade + 1;
                PlayerAttack_Create(pl, 0x64U);
                break;
            }
        }
        break;
    case 2:
        if ((pl->stateVar0 == 0) && (Motion_ReachedFrame(pl, 0, 26.0f) != 0) && (Motion_ReachedFrame(pl, 0, 38.0f) == 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            dir = (u16) (pl->input.rightStickAngle + 0x2AAB);
            if (dir <= 0x4000) {
                pl->stateVar0 = 2;
            } else if (dir < 0x9556) {
                pl->stateVar0 = 1;
            } else if (dir < 0xD556) {
                pl->stateVar0 = 2;
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if ((SwordShield_CheckHit(pl) != 1) && (Motion_ReachedFrame(pl, 0, 26.0f) != 0)) {
            stateVar0 = pl->stateVar0;
            if (stateVar0 == 1) {
                Player_SetState(pl, PL_GROUP_WEAPON, SNS_COMBO_1_LINKED, PL_SET_SNAP_POS);
                break;
            }
            if (stateVar0 == 2) {
                Player_SetState(pl, PL_GROUP_WEAPON, SNS_SIDE_SLASH, PL_SET_SNAP_POS);
                break;
            }
            if (Motion_ReachedFrame(pl, 0, 38.0f) == 0) {
                SwordShield_CheckInput(pl, 0);
            }
        }
        break;
    }
}

/*
 * SNS_BOUNCE: a flick in frames 14-40 queues SNS_BOUNCE_JUMP_SLASH at frame
 * 32.
 */
void SwordShield_StateBounce(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x580, 4, 0);
        CamShake_ReqA(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->stateVar0 != 0) {
            if (Motion_ReachedFrame(pl, 0, 32.0f) != 0) {
                Player_RequestState(pl, PL_GROUP_WEAPON, SNS_BOUNCE_JUMP_SLASH, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        } else if ((Motion_InFrameRange(pl, 0, 14.0f, 40.0f) != 0) && (pl->stateVar0 == 0) && (pl->itemSelect == 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            pl->stateVar0 = 1;
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * Runs the sword and shield's PL_GROUP_WEAPON state: maps the state to its
 * handler and variant.
 */
void SwordShield_RunState(Player *pl)
{
    switch (pl->state) {
    case SNS_COMBO_1:
        SwordShield_StateCombo(pl, 0);
        break;
    case SNS_JUMP_SLASH:
        SwordShield_StateJumpSlash(pl, 0);
        break;
    case SNS_SIDE_SLASH:
        SwordShield_StateSideSlash(pl, 0);
        break;
    case SNS_GUARD_SLASH:
        SwordShield_StateGuardSlash(pl, 0);
        break;
    case SNS_RISING_SLASH:
        SwordShield_StateRisingSlash(pl, 0);
        break;
    case SNS_BOUNCE:
        SwordShield_StateBounce(pl, 0);
        break;
    case SNS_COMBO_2:
        SwordShield_StateCombo(pl, 1);
        break;
    case SNS_COMBO_3:
        SwordShield_StateCombo(pl, 2);
        break;
    case SNS_COMBO_1_LINKED:
        SwordShield_StateCombo(pl, 3);
        break;
    case SNS_DRAW_JUMP_SLASH:
        SwordShield_StateJumpSlash(pl, 1);
        break;
    case SNS_BOUNCE_JUMP_SLASH:
        SwordShield_StateJumpSlash(pl, 2);
        break;
    case SNS_EVADE_RISING_SLASH:
        SwordShield_StateRisingSlash(pl, 1);
        break;
    case SNS_SHIELD_BASH:
        SwordShield_StateRisingSlash(pl, 2);
        
    default:
        break;
    }
}

/*
 * The drawn idle's input: input.pressed PAD_R3 the shield bash; a flick up the
 * combo, sideways the side slash, down the jump slash; the roll button rolls.
 * Returns 1 when it started a state.
 */
s32 SwordShield_AttackInput(Player *pl)
{
    s32 dir;
    u16 btn;

    btn = pl->input.pressed;
    if (btn & 1) {
        Player_SetState(pl, PL_GROUP_WEAPON, SNS_SHIELD_BASH, PL_SET_SNAP_POS);
        return 1;
    }
    if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
        dir = (pl->input.rightStickAngle + 0x2AAB) & 0xFFFF;
        if (dir < 0x4001) {
            Player_SetState(pl, PL_GROUP_WEAPON, SNS_SIDE_SLASH, PL_SET_SNAP_POS);
            return 1;
        }
        if (dir < 0x9556) {
            Player_SetState(pl, PL_GROUP_WEAPON, SNS_COMBO_1, PL_SET_SNAP_POS);
            return 1;
        }
        if (dir < 0xD556) {
            Player_SetState(pl, PL_GROUP_WEAPON, SNS_SIDE_SLASH, PL_SET_SNAP_POS);
            return 1;
        }
        Player_SetState(pl, PL_GROUP_WEAPON, SNS_JUMP_SLASH, PL_SET_SNAP_POS);
        return 1;
    }
    if ((btn & 0x40) && (pl->stamina >= 0x4B)) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
        return 1;
    }
    return 0;
}

/*
 * An attack window's input: the roll button (75 stamina) evades right or left
 * or rolls, by the left stick.
 */
void SwordShield_CheckInput(Player *pl, s32 unused)
{
    s32 dir;

    if (((Player_IsLocal(pl) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1) && (pl->itemSelect == 0) && (pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B)) {
        if (pl->input.stickDir & PAD_DPAD) {
            dir = (pl->input.stickAngle + 0x4000) & 0xFFFF;
            if (dir < 0x5C72) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_RIGHT, PL_SET_SNAP_POS);
                return;
            }
            if (dir < 0xA38E) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                return;
            }
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_LEFT, PL_SET_SNAP_POS);
            return;
        }
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
    }
}

/* The attack that draws on the move: SNS_DRAW_JUMP_SLASH. */
void SwordShield_DrawAttack(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, SNS_DRAW_JUMP_SLASH, PL_SET_SNAP_POS);
}

/* A flick up from the guard (Player_GuardInput): SNS_GUARD_SLASH. */
void SwordShield_GuardSlash(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, SNS_GUARD_SLASH, PL_SET_SNAP_POS);
}

/* A flick up in a roll or an evade: SNS_EVADE_RISING_SLASH. */
void SwordShield_EvadeAttack(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, SNS_EVADE_RISING_SLASH, PL_SET_SNAP_POS);
}

/* As SwordShield_CheckHit; a bounce requests DSWORDS_BOUNCE. */
s32 DualSwords_CheckHit(Player *pl)
{

    if (pl->hitPending != 0 && pl->stateVar1 == 0) {
        pl->stateVar1 = 1;
        switch (pl->hitKind) {
        case 0:
            break;
        case 1:
            pl->hitStopAdd = pl->hitStop = 0;
            Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_BOUNCE, 0);
            return 1;
        case 2:
        case 3:
            Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
            pl->weaponSeMute = 2;
            break;
        }
    }
    return 0;
}

/*
 * The flick-up combo. variant: 0 DSWORDS_COMBO_1, 1 _COMBO_2, 2 _COMBO_3, 3
 * DSWORDS_COMBO_1_LINKED. A queued flick chains: up the next slash, the other
 * ways DualSwords_ComboInput (linked).
 */
void DualSwords_StateCombo(Player *pl, s32 variant)
{
    f32 fromFrame;
    f32 toFrame;
    u16 angle;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, 0x579, 2, 0);
            PlayerAttack_Create(pl, 0x25);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x57A, 2, 0);
            PlayerAttack_Create(pl, 0x26);
            break;
        case 2:
            Player_SetMotionPair(pl, 0x57B, 2, 0);
            PlayerAttack_Create(pl, 0x28);
            break;
        case 3:
            Player_SetMotionPair(pl, 0x579, 2, 10);
            PlayerAttack_Create(pl, 0x25);
            StageObj_K05_Create(pl, 8, 1);
            break;
        }
        break;
    case 1:
        if (pl->stateVar0 == 0) {
            fromFrame = 10.0f;
            toFrame = 50.0f;
            if (variant == 2) {
                fromFrame = 60.0f;
                toFrame = 120.0f;
            }
            if (Motion_ReachedFrame(pl, 0, fromFrame) != 0 && Motion_ReachedFrame(pl, 0, toFrame) == 0
                && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
                angle = pl->input.rightStickAngle + 0x2AAB;
                if (angle <= 0x4000) {
                    pl->stateVar0 = 2;
                } else if (angle <= 0x9555) {
                    pl->stateVar0 = 1;
                } else if (angle <= 0xD555) {
                    pl->stateVar0 = 3;
                } else {
                    pl->stateVar0 = 4;
                }
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (DualSwords_CheckHit(pl) == 1) {
            break;
        }
        switch (variant) {
        case 0:
        case 3:
            if (Motion_ReachedFrame(pl, 0, 22.0f) != 0) {
                switch (pl->stateVar0) {
                case 1:
                    Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_COMBO_2, PL_SET_SNAP_POS);
                    break;
                case 2:
                    DualSwords_ComboInput(pl, 0, 1);
                    break;
                case 3:
                    DualSwords_ComboInput(pl, 1, 1);
                    break;
                case 4:
                    DualSwords_ComboInput(pl, 3, 1);
                    break;
                default:
                    if (Motion_ReachedFrame(pl, 0, 50.0f) == 0) {
                        DualSwords_CheckInput(pl, 0);
                    }
                    break;
                }
            }
            break;
        case 1:
            if (Motion_ReachedFrame(pl, 0, 28.0f) != 0) {
                switch (pl->stateVar0) {
                case 1:
                    Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_COMBO_3, PL_SET_SNAP_POS);
                    break;
                case 2:
                    DualSwords_ComboInput(pl, 0, 1);
                    break;
                case 3:
                    DualSwords_ComboInput(pl, 1, 1);
                    break;
                case 4:
                    DualSwords_ComboInput(pl, 3, 1);
                    break;
                default:
                    if (Motion_ReachedFrame(pl, 0, 50.0f) == 0) {
                        DualSwords_CheckInput(pl, 0);
                    }
                    break;
                }
            }
            if (Motion_PassedFrame(pl, 0, 18.0f) != 0) {
                PlayerAttack_Create(pl, 0x27);
            }
            break;
        case 2:
            if (Motion_ReachedFrame(pl, 0, 88.0f) != 0) {
                switch (pl->stateVar0) {
                case 2:
                    DualSwords_ComboInput(pl, 0, 1);
                    break;
                case 3:
                    DualSwords_ComboInput(pl, 1, 1);
                    break;
                case 4:
                    DualSwords_ComboInput(pl, 3, 1);
                    break;
                default:
                    if (Motion_ReachedFrame(pl, 0, 120.0f) == 0) {
                        DualSwords_CheckInput(pl, 0);
                    }
                    break;
                }
            }
            if (Motion_PassedFrame(pl, 0, 58.0f) != 0) {
                PlayerAttack_Create(pl, 0x29);
            }
            if (Motion_PassedFrame(pl, 0, 70.0f) != 0) {
                PlayerAttack_Create(pl, 0x2A);
            }
            break;
        }
        break;
    }
}

/*
 * The side slashes, three hits; the motion and its start from
 * DualSwords_SideSlashMotions[variant]. variant: 0 right, 1 left, 2 right
 * linked, 3 left linked. Evade and demon window from frame 50.
 */
void DualSwords_StateSideSlash(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMotionPair(pl, DualSwords_SideSlashMotions[variant * 2], 4, DualSwords_SideSlashMotions[variant * 2 + 1]);
        Player_SetMoveMode(pl, 0, 1, 0);
        PlayerAttack_CreateDual(pl, 0x2D, variant & 0xFF);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (DualSwords_CheckHit(pl) != 1) {
            if (Motion_ReachedFrame(pl, 0, 50.0f) != 0) {
                DualSwords_CheckInput(pl, 0);
            }
            if (Motion_PassedFrame(pl, 0, 24.0f) != 0) {
                PlayerAttack_CreateDual(pl, 0x2E, variant & 0xFF);
            }
            if (Motion_PassedFrame(pl, 0, 34.0f) != 0) {
                PlayerAttack_CreateDual(pl, 0x2F, variant & 0xFF);
            }
        }
        break;
    }
}

/*
 * The jump slash (likely). variant: 0 DSWORDS_JUMP_SLASH, 1
 * DSWORDS_DRAW_JUMP_SLASH, 2 DSWORDS_BOUNCE_JUMP_SLASH. A flick up in frames
 * 20-42 queues DSWORDS_RISING_SLASH at frame 42; timer marks the extra hit
 * made.
 */
void DualSwords_StateJumpSlash(Player *pl, s32 variant)
{
    s32 dir;
    s32 timer;

    switch (pl->step) {
    case 0:
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        pl->timer = 0;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57C, 2, 0);
            PlayerAttack_Create(pl, 0x2BU);
            break;
        case 1:
            pl->step++;
            pl->weaponOut = 1;
            Player_SetMotionPair(pl, 0x585, 4, 0);
            break;
        case 2:
            pl->step = 2;
            Player_SetMotionPair(pl, 0x57C, 4, 0);
            PlayerAttack_Create(pl, 0x2BU);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x57C, 2, 0x1C);
            PlayerAttack_Create(pl, 0x37U);
        }
        break;
    case 2:
        if ((pl->stateVar0 == 0) && (Motion_ReachedFrame(pl, 0, 20.0f) != 0) && (Motion_ReachedFrame(pl, 0, 42.0f) == 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            dir = (pl->input.rightStickAngle + 0x2AAB) & 0xFFFF;
            if ((dir > 16384) && (dir < 0x9556)) {
                pl->stateVar0 = 1;
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (DualSwords_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 36.0f) != 0) {
                timer = pl->timer;
                if (timer == 0) {
                    pl->timer = timer + 1;
                    PlayerAttack_Create(pl, 0x38U);
                }
            }
            if (Motion_ReachedFrame(pl, 0, 42.0f) != 0) {
                if (pl->stateVar0 != 0) {
                    Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_RISING_SLASH, PL_SET_SNAP_POS);
                    break;
                }
                if (Motion_ReachedFrame(pl, 0, 60.0f) == 0) {
                    DualSwords_CheckInput(pl, 0);
                }
            }
        }
        break;
    }
}

/*
 * The rising slash (likely). variant: 0 DSWORDS_RISING_SLASH, 1
 * DSWORDS_EVADE_RISING_SLASH. At frame 26 a flick up chains
 * DSWORDS_COMBO_1_LINKED, sideways the side slashes.
 */
void DualSwords_StateRisingSlash(Player *pl, s32 variant)
{
    u16 angle;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->stateVar0 = 0;
        Player_SetMotionPair(pl, 0x57E, 2, variant ? 4 : 0);
        PlayerAttack_Create(pl, 0x2C);
        break;
    case 1:
        if (pl->stateVar0 == 0 && Motion_ReachedFrame(pl, 0, 26.0f) != 0 && Motion_ReachedFrame(pl, 0, 38.0f) == 0
            && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            angle = pl->input.rightStickAngle + 0x2AAB;
            if (angle <= 0x4000) {
                pl->stateVar0 = 2;
            } else if (angle < 0x9556) {
                pl->stateVar0 = 1;
            } else if (angle < 0xD556) {
                pl->stateVar0 = 3;
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (DualSwords_CheckHit(pl) == 1) {
            break;
        }
        if (Motion_ReachedFrame(pl, 0, 26.0f) == 0) {
            break;
        }
        switch (pl->stateVar0) {
        case 1:
            Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_COMBO_1_LINKED, PL_SET_SNAP_POS);
            break;
        case 2:
            DualSwords_ComboInput(pl, 0, 0);
            break;
        case 3:
            DualSwords_ComboInput(pl, 1, 0);
            break;
        default:
            if (Motion_ReachedFrame(pl, 0, 38.0f) == 0) {
                DualSwords_CheckInput(pl, 0);
            }
            break;
        }
        break;
    }
}

/*
 * Enters or leaves the demon mode. variant: 0 DSWORDS_DEMON_START, 1
 * DSWORDS_DEMON_END, 2 DSWORDS_DEMON_START_LINKED. While entering it doesn't
 * flinch, and a flick from frame 60 starts the jump slash.
 */
void DualSwords_StateDemonToggle(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->weaponOut = 1;
        switch (variant) {
        case 0:
        default:
        case 2:
            pl->demonMode = 1;
            pl->noFlinchTimer = 2;
            if (variant == 0) {
                Player_SetMotionPair(pl, 0x3FA, 4, 0);
                break;
            }
            Player_SetMotionPair(pl, 0x3FA, 4, 0xA);
            break;
        case 1:
            pl->demonMode = 0;
            Player_SetMotionPair(pl, 0x3FB, 4, 0);
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->noFlinchTimer = 0;
            Player_ReturnToIdle(pl, 0, 2, 0);
            break;
        }
        if (variant != 1) {
            pl->noFlinchTimer = 2;
            if (Player_IsLocal(pl) == 1 && Motion_ReachedFrame(pl, 0, 60.0f) != 0 && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
                Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_JUMP_SLASH, PL_SET_SNAP_POS);
            }
        }
        break;
    }
}

/*
 * The demon mode's side slashes in two parts, six hits. variant: 0 right, 1
 * left. Input before frame 22 of the second part (stateVar0) acts at frame
 * 22: up the jump slash, the evades, down the dance.
 */
void DualSwords_StateDemonSideSlash(Player *pl, s32 variant)
{
    u16 angle;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0xFF;
        pl->stateVar1 = 0;
        pl->timer = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x583, 4, 0);
        } else {
            Player_SetMotionPair(pl, 0x57D, 4, 0);
        }
        PlayerAttack_CreateDual(pl, 0x2D, variant & 0xFF);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 40.0f) != 0) {
            pl->step++;
            if (variant == 0) {
                Player_SetMotionPair(pl, 0x582, 0, 0);
            } else {
                Player_SetMotionPair(pl, 0x581, 0, 0);
            }
            PlayerAttack_CreateDual(pl, 0x34, variant & 0xFF);
            pl->stateVar1 = 0;
            break;
        }
        if (DualSwords_CheckHit(pl) == 1) {
            break;
        }
        if (Motion_PassedFrame(pl, 0, 24.0f) != 0) {
            PlayerAttack_CreateDual(pl, 0x2E, variant & 0xFF);
        }
        if (Motion_PassedFrame(pl, 0, 34.0f) != 0) {
            PlayerAttack_CreateDual(pl, 0x2F, variant & 0xFF);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (DualSwords_CheckHit(pl) == 1) {
            break;
        }
        if (Motion_PassedFrame(pl, 0, 28.0f) != 0) {
            PlayerAttack_CreateDual(pl, 0x35, variant & 0xFF);
        }
        if (Motion_PassedFrame(pl, 0, 38.0f) != 0) {
            PlayerAttack_CreateDual(pl, 0x36, variant & 0xFF);
        }
        if (Player_IsLocal(pl) != 1) {
            break;
        }
        if (pl->stateVar0 == 0xFF) {
            if (Motion_ReachedFrame(pl, 0, 22.0f) == 0) {
                if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
                    angle = pl->input.rightStickAngle + 0x2AAB;
                    if (angle > 0x4000 && angle < 0x9556) {
                        pl->stateVar0 = 1;
                        if (pl->input.stickDir & PAD_LEFT) {
                            pl->timer = 1;
                        } else if (pl->input.stickDir & PAD_RIGHT) {
                            pl->timer = 2;
                        }
                    } else if (angle > 0xD555) {
                        pl->stateVar0 = 6;
                    }
                }
                if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B) {
                    if (pl->input.stickDir & PAD_DPAD) {
                        angle = pl->input.stickAngle + 0x4000;
                        if (angle < 0x5C72) {
                            pl->stateVar0 = 2;
                        } else if (angle < 0xA38E) {
                            pl->stateVar0 = 3;
                        } else {
                            pl->stateVar0 = 4;
                        }
                    } else {
                        pl->stateVar0 = 5;
                    }
                }
            }
        } else if (Motion_PassedFrame(pl, 0, 22.0f) != 0) {
            switch (pl->stateVar0) {
            case 1:
                switch (pl->timer) {
                case 1:
                    pl->rot[1] += 0x2000;
                    pl->targetYaw = pl->rot[1];
                    break;
                case 2:
                    pl->rot[1] -= 0x2000;
                    pl->targetYaw = pl->rot[1];
                    break;
                }
                Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_JUMP_SLASH, PL_SET_SNAP_POS);
                break;
            case 2:
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_RIGHT, PL_SET_SNAP_POS);
                break;
            case 3:
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                break;
            case 4:
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_LEFT, PL_SET_SNAP_POS);
                break;
            case 5:
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                break;
            case 6:
                DualSwords_ComboInput(pl, 3, 1);
                break;
            }
        }
        break;
    }
}

/*
 * DSWORDS_DEMON_DANCE: ten hits. At frame 58 below sharpness level 2 it stops
 * (DSWORDS_DANCE_STOP); the evade from frame 132.
 */
void DualSwords_StateDemonDance(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x57F, 2, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        pl->noFlinchTimer = 0x1C;
        PlayerAttack_Create(pl, 0x30U);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if ((Motion_PassedFrame(pl, 0, 58.0f) != 0) && ((s32) pl->sharpnessLevel < 2)) {
            Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_DANCE_STOP, PL_SET_SNAP_POS);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 20.0f) != 0) {
            PlayerAttack_Create(pl, 0x31U);
        }
        if (Motion_PassedFrame(pl, 0, 26.0f) != 0) {
            PlayerAttack_Create(pl, 0x32U);
        }
        if (Motion_PassedFrame(pl, 0, 50.0f) != 0) {
            PlayerAttack_Create(pl, 0x32U);
        }
        if (Motion_PassedFrame(pl, 0, 54.0f) != 0) {
            PlayerAttack_Create(pl, 0x31U);
        }
        if (Motion_PassedFrame(pl, 0, 66.0f) != 0) {
            PlayerAttack_Create(pl, 0x31U);
        }
        if (Motion_PassedFrame(pl, 0, 70.0f) != 0) {
            PlayerAttack_Create(pl, 0x32U);
        }
        if (Motion_PassedFrame(pl, 0, 82.0f) != 0) {
            PlayerAttack_Create(pl, 0x32U);
        }
        if (Motion_PassedFrame(pl, 0, 88.0f) != 0) {
            PlayerAttack_Create(pl, 0x31U);
        }
        if (Motion_PassedFrame(pl, 0, 118.0f) != 0) {
            PlayerAttack_Create(pl, 0x33U);
        }
        if ((Player_IsLocal(pl) == 1) && (Motion_ReachedFrame(pl, 0, 132.0f) != 0)) {
            DualSwords_CheckInput(pl, 1);
        }
        break;
    }
}

/* DSWORDS_DANCE_STOP: its motion, then idle. */
void DualSwords_StateDanceStop(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x584, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * DSWORDS_BOUNCE: a flick in frames 14-40 queues DSWORDS_BOUNCE_JUMP_SLASH at
 * frame 32.
 */
void DualSwords_StateBounce(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x580, 4, 0);
        CamShake_ReqA(pl, 0);
        Player_SetMoveMode(pl, 0, 1, 0);
        break;
    case 1:
        if (pl->stateVar0 != 0) {
            if (Motion_ReachedFrame(pl, 0, 32.0f) != 0) {
                Player_RequestState(pl, PL_GROUP_WEAPON, DSWORDS_BOUNCE_JUMP_SLASH, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        } else if ((Motion_InFrameRange(pl, 0, 14.0f, 40.0f) != 0) && (pl->stateVar0 == 0) && (pl->itemSelect == 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            pl->stateVar0 = 1;
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * Runs the dual swords' PL_GROUP_WEAPON state: maps the state to its handler
 * and variant.
 */
void DualSwords_RunState(Player *pl)
{
    switch (pl->state) {
    case DSWORDS_COMBO_1:
        DualSwords_StateCombo(pl, 0);
        break;
    case DSWORDS_SIDE_SLASH_RIGHT:
        DualSwords_StateSideSlash(pl, 0);
        break;
    case DSWORDS_JUMP_SLASH:
        DualSwords_StateJumpSlash(pl, 0);
        break;
    case DSWORDS_RISING_SLASH:
        DualSwords_StateRisingSlash(pl, 0);
        break;
    case DSWORDS_DEMON_START:
        DualSwords_StateDemonToggle(pl, 0);
        break;
    case DSWORDS_DEMON_SIDE_RIGHT:
        DualSwords_StateDemonSideSlash(pl, 0);
        break;
    case DSWORDS_DEMON_DANCE:
        DualSwords_StateDemonDance(pl, 0);
        break;
    case DSWORDS_DANCE_STOP:
        DualSwords_StateDanceStop(pl, 0);
        break;
    case DSWORDS_BOUNCE:
        DualSwords_StateBounce(pl, 0);
        break;
    case DSWORDS_COMBO_2:
        DualSwords_StateCombo(pl, 1);
        break;
    case DSWORDS_COMBO_3:
        DualSwords_StateCombo(pl, 2);
        break;
    case DSWORDS_COMBO_1_LINKED:
        DualSwords_StateCombo(pl, 3);
        break;
    case DSWORDS_SIDE_SLASH_LEFT:
        DualSwords_StateSideSlash(pl, 1);
        break;
    case DSWORDS_SIDE_SLASH_RIGHT_LINKED:
        DualSwords_StateSideSlash(pl, 2);
        break;
    case DSWORDS_SIDE_SLASH_LEFT_LINKED:
        DualSwords_StateSideSlash(pl, 3);
        break;
    case DSWORDS_DRAW_JUMP_SLASH:
        DualSwords_StateJumpSlash(pl, 1);
        break;
    case DSWORDS_BOUNCE_JUMP_SLASH:
        DualSwords_StateJumpSlash(pl, 2);
        break;
    case DSWORDS_EVADE_RISING_SLASH:
        DualSwords_StateRisingSlash(pl, 1);
        break;
    case DSWORDS_DEMON_END:
        DualSwords_StateDemonToggle(pl, 1);
        break;
    case DSWORDS_DEMON_START_LINKED:
        DualSwords_StateDemonToggle(pl, 2);
        break;
    case DSWORDS_DEMON_SIDE_LEFT:
        DualSwords_StateDemonSideSlash(pl, 1);
        
    default:
        break;
    }
}

/*
 * Starts the attack of a flick's sector: sector 0 right, 1 left, 2 down from
 * idle, 3 down within a combo (nothing outside the demon mode); linked picks
 * the side slashes' linked forms. In the demon mode it starts the demon side
 * slashes or the dance.
 */
void DualSwords_ComboInput(Player *pl, s32 sector, s32 linked)
{
    switch (sector) {
    case 0:
        if ((s8) pl->demonMode != 0) {
            Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_DEMON_SIDE_RIGHT, PL_SET_SNAP_POS);
            break;
        }
        if (linked == 0) {
            Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_SIDE_SLASH_RIGHT, PL_SET_SNAP_POS);
            break;
        }
        Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_SIDE_SLASH_RIGHT_LINKED, PL_SET_SNAP_POS);
        break;
    case 1:
        if ((s8) pl->demonMode != 0) {
            Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_DEMON_SIDE_LEFT, PL_SET_SNAP_POS);
            break;
        }
        if (linked == 0) {
            Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_SIDE_SLASH_LEFT, PL_SET_SNAP_POS);
            break;
        }
        Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_SIDE_SLASH_LEFT_LINKED, PL_SET_SNAP_POS);
        break;
    case 2:
    case 3:
        if ((s8) pl->demonMode != 0) {
            Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_DEMON_DANCE, PL_SET_SNAP_POS);
            break;
        }
        if (sector == 2) {
            Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_JUMP_SLASH, PL_SET_SNAP_POS);
        }
        break;
    }
}

/*
 * The drawn idle's input: a flick up the combo, the other ways
 * DualSwords_ComboInput; the roll button rolls. Returns 1 when it started a
 * state.
 */
s32 DualSwords_AttackInput(Player *pl)
{
    s32 dir;

    if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
        dir = (pl->input.rightStickAngle + 0x2AAB) & 0xFFFF;
        if (dir < 0x4001) {
            DualSwords_ComboInput(pl, 0, 0);
            return 1;
        }
        if (dir < 0x9556) {
            Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_COMBO_1, PL_SET_SNAP_POS);
            return 1;
        }
        if (dir < 0xD556) {
            DualSwords_ComboInput(pl, 1, 0);
            return 1;
        }
        DualSwords_ComboInput(pl, 2, 0);
        return 1;
    }
    if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B)) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
        return 1;
    }
    return 0;
}

/*
 * An attack window's input: the evade as the sword and shield's; with variant
 * 0, input.pressed PAD_R1 also toggles the demon mode.
 */
void DualSwords_CheckInput(Player *pl, s32 variant)
{
    u16 pressed;
    s32 dir;

    if ((Player_IsLocal(pl) != 0 || Game_IsOnline() != 1) && Quest_PhaseIsOver(1) != 1 && pl->itemSelect == 0) {
        pressed = pl->input.pressed;
        if ((pressed & PAD_CROSS) && pl->stamina >= 75) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (pl->input.stickAngle + 0x4000) & 0xFFFF;
                if (dir < 0x5C72) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_RIGHT, PL_SET_SNAP_POS);
                    return;
                }
                if (dir < 0xA38E) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                    return;
                }
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_LEFT, PL_SET_SNAP_POS);
                return;
            }
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            return;
        }
        if (variant == 0 && (pressed & PAD_R1)) {
            if (pl->demonMode == 0) {
                Player_RequestState(pl, PL_GROUP_WEAPON, DSWORDS_DEMON_START_LINKED, 0);
                return;
            }
            Player_RequestState(pl, PL_GROUP_WEAPON, DSWORDS_DEMON_END, 0);
            return;
        }
    }
}

/* The attack that draws on the move: DSWORDS_DRAW_JUMP_SLASH. */
void DualSwords_DrawAttack(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_DRAW_JUMP_SLASH, PL_SET_SNAP_POS);
}

/* A flick up in a roll or an evade: DSWORDS_EVADE_RISING_SLASH. */
void DualSwords_EvadeAttack(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, DSWORDS_EVADE_RISING_SLASH, PL_SET_SNAP_POS);
}

/*
 * input.held PAD_R1 (Player_HoldSpecialInput): end 0 enters the demon mode, 1
 * leaves it.
 */
void DualSwords_DemonToggle(Player *pl, s32 end)
{
    if (end == 0) {
        Player_SetState((void *) pl, PL_GROUP_WEAPON, DSWORDS_DEMON_START, PL_SET_SNAP_POS);
        return;
    }
    Player_SetState((void *) pl, PL_GROUP_WEAPON, DSWORDS_DEMON_END, PL_SET_SNAP_POS);
}

/*
 * The attack's first hit, once (timer): a bounce requests LSWORD_BOUNCE and
 * returns 1; kinds 2 and 3 step the motion back 2 frames and mute the weapon
 * sound. Returns 0 otherwise.
 */
s32 LongSword_CheckHit(Player *pl)
{

    if (pl->hitPending != 0 && *(s32 *) &pl->timer == 0) {
        *(s32 *) &pl->timer = 1;
        switch (pl->hitKind) {
        case 0:
            break;
        case 1:
            pl->hitStopAdd = pl->hitStop = 0;
            Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_BOUNCE, 0);
            return 1;
        case 2:
        case 3:
            Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
            pl->weaponSeMute = 2;
            break;
        }
    }
    return 0;
}

/*
 * The slash. variant: the lead-in (0 none, 1 a wind-up, 2 from the bounce, 3
 * the draw); stepForward 1 steps forward in frames 40-56. A key read in
 * frames 44-94 chains at frame 80; evade in frames 80-164.
 */
void LongSword_StateSlash(Player *pl, s32 variant, s32 stepForward)
{
    f32 offset[4];
    f32 out[4];

    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
            pl->unk417 = 1;
            pl->weaponOut = 1;
            pl->fxMade = 0;
            pl->step++;
            pl->stateVar1 = 0xFF;
            pl->timer = 0;
            Player_SetMoveMode(pl, 0, 1, 0);
            Player_SetMotionPair(pl, 0x579, 4, 0xA);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x39);
            pl->noFlinchTimer = 0x28;
            break;
        case 1:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                pl->fxMade = 0;
                Player_SetMoveMode(pl, 0, 1, 0);
                Player_SetMotionPair(pl, 0x584, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 0x28;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 0x579, 2, 0x40);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 0x39);
                    pl->stateVar1 = 0xFF;
                    pl->timer = 0;
                }
                break;
            }
            break;
        case 2:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                pl->unk417 = 1;
                pl->fxMade = 0;
                pl->weaponOut = 1;
                Player_SetMoveMode(pl, 0, 1, 0);
                Player_SetMotionPair(pl, 0x586, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 0x28;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 0x579, 2, 0x26);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 0x39);
                    pl->stateVar1 = 0xFF;
                    pl->timer = 0;
                }
                break;
            }
            break;
        case 3:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                pl->unk417 = 1;
                pl->fxMade = 0;
                pl->weaponOut = 1;
                Player_SetMoveMode(pl, 0, 1, 0);
                Player_SetMotionPair(pl, 0x587, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 0x28;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 0x579, 0, 0x30);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 0x39);
                    pl->stateVar1 = 0xFF;
                    pl->timer = 0;
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        } else if (LongSword_CheckHit(pl) != 1) {
            if ((Motion_ReachedFrame(pl, 0, 48.0f) != 0) && (pl->fxMade == 0)) {
                pl->fxMade = 1;
                StageObj_K05_Create(pl, 9, 5);
            }
            if ((pl->stateVar1 == 0xFF) && (Motion_InFrameRange(pl, 0, 44.0f, 94.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 1, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if ((Motion_InFrameRange(pl, 0, 40.0f, 56.0f) != 0) && (stepForward != 0)) {
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = 7.5f;
                Vec_Rotate(out, offset, pl->rotMtx);
                pl->pos[0] += out[0];
                pl->pos[2] += out[2];
            }
            if ((Motion_ReachedFrame(pl, 0, 80.0f) != 0) && (pl->stateVar1 != 0xFF)) {
                LongSword_ComboInput(pl, 1, pl->stateVar1, 0);
            } else if (Motion_InFrameRange(pl, 0, 80.0f, 164.0f) != 0) {
                LongSword_EvadeInput(pl);
            }
        }
        break;
    }
}

/*
 * The second slash. variant 1: LSWORD_SLASH_2_AFTER_RISING (a wind-up first).
 */
void LongSword_StateSlash2(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
            pl->step++;
            pl->stateVar1 = 0xFF;
            pl->timer = 0;
            pl->unk417 = 1;
            Player_SetMotionPair(pl, 0x57A, 4, 0xA);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x3A);
            pl->noFlinchTimer = 0x46;
            Player_SetMoveMode(pl, 0, 1, 0);
            break;
        case 1:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                Player_SetMoveMode(pl, 0, 1, 0);
                Player_SetMotionPair(pl, 0x58C, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 0x28;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 0x57A, 0, 0x18);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 0x3A);
                    pl->stateVar1 = 0xFF;
                    pl->timer = 0;
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        } else if (LongSword_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 48.0f) != 0) {
                StageObj_K05_Create(pl, 8, 5);
            }
            if ((pl->stateVar1 == 0xFF) && (Motion_InFrameRange(pl, 0, 44.0f, 104.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 2, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if ((Motion_ReachedFrame(pl, 0, 80.0f) != 0) && (pl->stateVar1 != 0xFF)) {
                LongSword_ComboInput(pl, 2, pl->stateVar1, 0);
            } else if (Motion_InFrameRange(pl, 0, 80.0f, 164.0f) != 0) {
                LongSword_EvadeInput(pl);
            }
        }
        break;
    }
}

/*
 * The fade slash (likely a backward slash). variant 0-6: the lead-in motion
 * for the attack it follows. No attack follows; only the evade.
 */
void LongSword_StateFadeSlash(Player *pl, u32 variant)
{
    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
            pl->unk417 = 1;
            pl->step++;
            pl->stateVar1 = 0xFF;
            pl->timer = 0;
            Player_SetMoveMode(pl, 0, 1, 0);
            Player_SetMotionPair(pl, 0x57B, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x3B);
            pl->noFlinchTimer = 0x28;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            switch (pl->stateVar0) {
            case 0:
                pl->unk417 = 1;
                pl->stateVar0++;
                switch (variant) {
                case 1:
                case 2:
                    Player_SetMotionPair(pl, 0x582, 2, 0);
                    break;
                case 3:
                    Player_SetMotionPair(pl, 0x589, 2, 0);
                    break;
                case 4:
                    Player_SetMotionPair(pl, 0x58A, 2, 0);
                    break;
                case 5:
                    Player_SetMotionPair(pl, 0x58D, 4, 0);
                    break;
                case 6:
                    Player_SetMotionPair(pl, 0x58E, 2, 0);
                    break;
                }
                Player_UpdateMotionTime(pl, 0);
                Player_SetMoveMode(pl, 0, 1, 0);
                pl->noFlinchTimer = 0x28;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    pl->stateVar1 = 0xFF;
                    pl->timer = 0;
                    switch (variant) {
                    case 1:
                    case 2:
                        Player_SetMotionPair(pl, 0x57B, 2, 0x12);
                        break;
                    case 3:
                        Player_SetMotionPair(pl, 0x57B, 0, 0x14);
                        break;
                    case 4:
                        Player_SetMotionPair(pl, 0x57B, 0, 0x14);
                        break;
                    case 5:
                        Player_SetMotionPair(pl, 0x57B, 0, 0x10);
                        break;
                    case 6:
                        Player_SetMotionPair(pl, 0x57B, 0, 0xC);
                        break;
                    }
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 0x3B);
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        } else if (LongSword_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 22.0f) != 0) {
                StageObj_K05_Create(pl, 0x11, 5);
            }
            if ((pl->stateVar1 == 0xFF) && (Motion_InFrameRange(pl, 0, 28.0f, 68.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 3, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if ((Motion_ReachedFrame(pl, 0, 68.0f) != 0) && (pl->stateVar1 != 0xFF)) {
                LongSword_ComboInput(pl, 3, pl->stateVar1, 0);
            } else if (Motion_InFrameRange(pl, 0, 80.0f, 128.0f) != 0) {
                LongSword_EvadeInput(pl);
            }
        }
        break;
    }
}

/*
 * LSWORD_BOUNCE: a flick in frames 2-30 queues LSWORD_BOUNCE_SLASH at frame
 * 22; else idle.
 */
void LongSword_StateBounce(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x585, 0, 0);
        break;
    case 1:
        if (pl->stateVar0 != 0) {
            if (Motion_ReachedFrame(pl, 0, 22.0f) != 0) {
                Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_BOUNCE_SLASH, PL_SET_SNAP_POS);
            }
        } else if ((Motion_InFrameRange(pl, 0, 2.0f, 30.0f) != 0) && (pl->stateVar0 == 0) && (pl->itemSelect == 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            pl->stateVar0 = 1;
            pl->stateVar1 = 0;
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 0xA, 0);
        }
        break;
    }
}

/* LSWORD_THRUST: a key read in frames 28-100 chains at frame 38. */
void LongSword_StateThrust(Player *pl, s32 unused)
{
    u8 stick;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0xFF;
        pl->timer = 0;
        pl->unk417 = 1;
        Player_SetMotionPair(pl, 0x57D, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 0x48U);
        pl->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (LongSword_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 18.0f) != 0) {
                StageObj_K05_Create(pl, 9, 5);
            }
            if (pl->motion[0].playing == 0) {
                Player_ReturnToIdle(pl, 0, 4, 0);
                break;
            }
            if ((pl->stateVar1 == 0xFF) && (Motion_InFrameRange(pl, 0, 28.0f, 100.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 0xBU, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(pl, 0, 38.0f) != 0) {
                stick = pl->stateVar1;
                if (stick != 0xFF) {
                    LongSword_ComboInput(pl, 0xBU, stick, 0);
                    break;
                }
            }
            if (Motion_InFrameRange(pl, 0, 38.0f, 160.0f) != 0) {
                LongSword_EvadeInput(pl);
            }
        } else {
        }
        break;
    }
}

/*
 * The first spirit slash. The even variants spend the gauge (15; 20 for 2 and
 * 4) and don't bounce; the odd ones are the "_EMPTY" forms. 2 and 3 follow a
 * rising slash, 4 and 5 are the draw's.
 */
void LongSword_StateSpirit1(Player *pl, u32 variant)
{
    u8 attackKey;

    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
        case 1:
            pl->unk417 = 1;
            pl->weaponOut = 1;
            pl->fxMade = 0;
            pl->step++;
            pl->stateVar1 = 0xFF;
            pl->timer = 0;
            Player_SetMoveMode(pl, 0, 1, 0);
            Player_SetMotionPair(pl, 0x57C, 4, 8);
            Player_UpdateMotionTime(pl, 0);
            if (variant == 0) {
                PlayerAttack_Create(pl, 0x45);
                LongSword_AddGauge(pl, -0xF);
                StageObj_K29_CreateOnPlayerDefault(pl, 0, 1.0f);
            } else {
                PlayerAttack_Create(pl, 0x4F);
            }
            pl->noFlinchTimer = 0x23;
            break;
        case 2:
        case 3:
        case 4:
        case 5:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                pl->fxMade = 0;
                Player_SetMoveMode(pl, 0, 1, 0);
                if (variant == 2 || variant == 3) {
                    Player_SetMotionPair(pl, 0x591, 2, 0);
                } else {
                    Player_SetMotionPair(pl, 0x592, 4, 0);
                }
                Player_UpdateMotionTime(pl, 0);
                if (variant == 2 || variant == 4) {
                    LongSword_AddGauge(pl, -0x14);
                    StageObj_K29_CreateOnPlayerDefault(pl, 0, 1.0f);
                }
                pl->noFlinchTimer = 0x23;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    if (variant == 2 || variant == 3) {
                        Player_SetMotionPair(pl, 0x57C, 0, 0x1E);
                    } else {
                        Player_SetMotionPair(pl, 0x57C, 0, 0x2C);
                    }
                    Player_UpdateMotionTime(pl, 0);
                    if (variant == 2 || variant == 4) {
                        PlayerAttack_Create(pl, 0x45);
                    } else {
                        PlayerAttack_Create(pl, 0x4F);
                    }
                    pl->stateVar1 = 0xFF;
                    pl->timer = 0;
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (variant == 1 || variant == 3 || variant == 5) {
            if (LongSword_CheckHit(pl) == 1) {
                break;
            }
        }
        if (Motion_ReachedFrame(pl, 0, 40.0f) != 0 && pl->fxMade == 0) {
            pl->fxMade = 1;
            if (variant == 0 || variant == 2 || variant == 4) {
                StageObj_K05_Create(pl, 6, 6);
            } else {
                StageObj_K05_Create(pl, 6, 5);
            }
        }
        if (pl->stateVar1 == 0xFF && Motion_InFrameRange(pl, 0, 46.0f, 100.0f) != 0) {
            attackKey = LongSword_ReadAttackKey(pl);
            if (variant == 0 || variant == 2 || variant == 4) {
                pl->stateVar1 = LongSword_ComboInput(pl, 4, attackKey, 1);
            } else {
                pl->stateVar1 = LongSword_ComboInput(pl, 0xD, attackKey, 1);
            }
        }
        if (Motion_ReachedFrame(pl, 0, 56.0f) != 0) {
            attackKey = pl->stateVar1;
            if (attackKey != 0xFF) {
                if (variant == 0 || variant == 2 || variant == 4) {
                    LongSword_ComboInput(pl, 4, attackKey, 0);
                    break;
                }
                LongSword_ComboInput(pl, 0xD, attackKey, 0);
                break;
            }
        }
        if (Motion_InFrameRange(pl, 0, 56.0f, 160.0f) != 0) {
            LongSword_EvadeInput(pl);
        }
        break;
    }
}

/* LSWORD_SPIRIT_THRUST: the thrust after the first spirit slash. */
void LongSword_StateSpiritThrust(Player *pl, s32 unused)
{
    u8 stick;

    switch (pl->step) {
    case 0:
        pl->unk417 = 1;
        pl->step++;
        pl->stateVar1 = 0xFF;
        pl->timer = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        Player_SetMotionPair(pl, 0x58F, 2, 0);
        Player_UpdateMotionTime(pl, 0);
        pl->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x57D, 0, 0x10);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x47U);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (LongSword_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 18.0f) != 0) {
                StageObj_K05_Create(pl, 9, 5);
            }
            if ((pl->stateVar1 == 0xFF) && (Motion_InFrameRange(pl, 0, 28.0f, 100.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 5U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(pl, 0, 38.0f) != 0) {
                stick = pl->stateVar1;
                if (stick != 0xFF) {
                    LongSword_ComboInput(pl, 5U, stick, 0);
                    break;
                }
            }
            if (Motion_InFrameRange(pl, 0, 38.0f, 160.0f) != 0) {
                LongSword_EvadeInput(pl);
            }
        }
        break;
    }
}

/*
 * The second spirit slash (gauge 20; no bounce). variant 1:
 * LSWORD_SPIRIT_2_AFTER_THRUST.
 */
void LongSword_StateSpirit2(Player *pl, s32 variant)
{
    u8 stick;
    u8 stateVar0;

    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
            pl->unk417 = 1;
            pl->step++;
            pl->stateVar1 = 0xFF;
            pl->timer = 0;
            Player_SetMoveMode(pl, 0, 1, 0);
            Player_SetMotionPair(pl, 0x57E, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x49U);
            LongSword_AddGauge(pl, -0x14);
            StageObj_K29_CreateOnPlayerDefault(pl, 0U, 1.0f);
            pl->noFlinchTimer = 0x3C;
            break;
        case 1:
            stateVar0 = pl->stateVar0;
            switch (stateVar0) {
            case 0:
                pl->stateVar0 = stateVar0 + 1;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                Player_SetMoveMode(pl, 0, 1, 0);
                Player_SetMotionPair(pl, 0x590, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                LongSword_AddGauge(pl, -0x14);
                StageObj_K29_CreateOnPlayerDefault(pl, 0U, 1.0f);
                pl->noFlinchTimer = 0x3C;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 0x57E, 0, 0x18);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 0x49U);
                    pl->stateVar1 = 0xFF;
                    pl->timer = 0;
                }
                break;
            }
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 28.0f) != 0) {
            StageObj_K05_Create(pl, 8, 6);
        }
        if ((pl->stateVar1 == 0xFF) && (Motion_InFrameRange(pl, 0, 34.0f, 100.0f) != 0)) {
            pl->stateVar1 = LongSword_ComboInput(pl, 6U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
        }
        if (Motion_ReachedFrame(pl, 0, 44.0f) != 0) {
            stick = pl->stateVar1;
            if (stick != 0xFF) {
                LongSword_ComboInput(pl, 6U, stick, 0);
                break;
            }
        }
        if (Motion_InFrameRange(pl, 0, 62.0f, 160.0f) != 0) {
            LongSword_EvadeInput(pl);
        }
        break;
    }
}

/* LSWORD_SPIRIT_RISING: the rising slash after the second spirit slash. */
void LongSword_StateSpiritRising(Player *pl, s32 unused)
{
    u8 stick;

    switch (pl->step) {
    case 0:
        pl->unk417 = 1;
        pl->step++;
        pl->stateVar1 = 0xFF;
        pl->timer = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        Player_SetMotionPair(pl, 0x57F, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 0x4AU);
        pl->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (LongSword_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 22.0f) != 0) {
                StageObj_K05_Create(pl, 5, 5);
            }
            if ((pl->stateVar1 == 0xFF) && (Motion_InFrameRange(pl, 0, 32.0f, 100.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 7U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(pl, 0, 42.0f) != 0) {
                stick = pl->stateVar1;
                if (stick != 0xFF) {
                    LongSword_ComboInput(pl, 7U, stick, 0);
                    break;
                }
            }
            if (Motion_InFrameRange(pl, 0, 42.0f, 160.0f) != 0) {
                LongSword_EvadeInput(pl);
            }
        } else {
        }
        break;
    }
}

/* The rising slash (likely). variant 1: LSWORD_RISING_AFTER_THRUST. */
void LongSword_StateRisingSlash(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
            pl->step++;
            pl->stateVar1 = 0xFF;
            pl->timer = 0;
            pl->unk417 = 1;
            Player_SetMotionPair(pl, 0x57F, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x4B);
            break;
        case 1:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                Player_SetMoveMode(pl, 0, 1, 0);
                Player_SetMotionPair(pl, 0x588, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 0x28;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 0x57F, 0, 0x14);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 0x4B);
                    pl->stateVar1 = 0xFF;
                    pl->timer = 0;
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        } else if (LongSword_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 22.0f) != 0) {
                StageObj_K05_Create(pl, 5, 5);
            }
            if ((pl->stateVar1 == 0xFF) && (Motion_InFrameRange(pl, 0, 32.0f, 100.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 0xC, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if ((Motion_ReachedFrame(pl, 0, 42.0f) != 0) && (pl->stateVar1 != 0xFF)) {
                LongSword_ComboInput(pl, 0xC, pl->stateVar1, 0);
            } else if (Motion_InFrameRange(pl, 0, 42.0f, 160.0f) != 0) {
                LongSword_EvadeInput(pl);
            }
        }
        break;
    }
}

/*
 * The third spirit slash (gauge 25), three hits. variant 1:
 * LSWORD_SPIRIT_3_AFTER_RISING. Only the fade slash follows.
 */
void LongSword_StateSpirit3(Player *pl, s32 variant)
{
    u8 stick;
    u8 stateVar0;

    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
            pl->unk417 = 1;
            pl->step++;
            pl->stateVar0 = 0;
            pl->stateVar1 = 0xFF;
            pl->timer = 0;
            Player_SetMoveMode(pl, 0, 1, 0);
            Player_SetMotionPair(pl, 0x580, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 0x4CU);
            LongSword_AddGauge(pl, -0x19);
            StageObj_K29_CreateOnPlayerDefault(pl, 0U, 1.0f);
            pl->noFlinchTimer = 0x3C;
            break;
        case 1:
            stateVar0 = pl->stateVar0;
            switch (stateVar0) {
            case 0:
                pl->stateVar0 = stateVar0 + 1;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                Player_SetMoveMode(pl, 0, 1, 0);
                Player_SetMotionPair(pl, 0x58B, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 0x3C;
                LongSword_AddGauge(pl, -0x19);
                StageObj_K29_CreateOnPlayerDefault(pl, 0U, 1.0f);
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    pl->stateVar0 = 0;
                    Player_SetMotionPair(pl, 0x580, 0, 0xA);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 0x4CU);
                    pl->stateVar1 = 0xFF;
                    pl->timer = 0;
                }
                break;
            }
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 14.0f) != 0) {
            StageObj_K05_Create(pl, 6, 6);
        }
        if (Motion_PassedFrame(pl, 0, 34.0f) != 0) {
            StageObj_K05_Create(pl, 6, 6);
        }
        if (Motion_PassedFrame(pl, 0, 84.0f) != 0) {
            StageObj_K05_Create(pl, 8, 6);
        }
        if ((Motion_PassedFrame(pl, 0, 34.0f) != 0) && (pl->stateVar0 != 1)) {
            pl->stateVar0 = 1;
            PlayerAttack_Create(pl, 0x4DU);
        }
        if ((Motion_PassedFrame(pl, 0, 88.0f) != 0) && (pl->stateVar0 != 2)) {
            pl->stateVar0 = 2;
            PlayerAttack_Create(pl, 0x50U);
        }
        if ((pl->stateVar1 == 0xFF) && (Motion_InFrameRange(pl, 0, 100.0f, 160.0f) != 0)) {
            pl->stateVar1 = LongSword_ComboInput(pl, 8U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
        }
        if (Motion_ReachedFrame(pl, 0, 110.0f) != 0) {
            stick = pl->stateVar1;
            if (stick != 0xFF) {
                LongSword_ComboInput(pl, 8U, stick, 0);
                break;
            }
        }
        if (Motion_InFrameRange(pl, 0, 110.0f, 160.0f) != 0) {
            LongSword_EvadeInput(pl);
        }
        break;
    }
}

/* LSWORD_EXTRA_SLASH: only the fade slash follows (no entry seen). */
void LongSword_StateExtraSlash(Player *pl, s32 unused)
{
    u8 stick;

    switch (pl->step) {
    case 0:
        pl->unk417 = 1;
        pl->step++;
        pl->stateVar1 = 0xFF;
        pl->timer = 0;
        Player_SetMoveMode(pl, 0, 1, 0);
        Player_SetMotionPair(pl, 0x581, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 0x4EU);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (LongSword_CheckHit(pl) != 1) {
            if ((pl->stateVar1 == 0xFF) && (Motion_InFrameRange(pl, 0, 40.0f, 100.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 9U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(pl, 0, 60.0f) != 0) {
                stick = pl->stateVar1;
                if (stick != 0xFF) {
                    LongSword_ComboInput(pl, 9U, stick, 0);
                    break;
                }
            }
            if (Motion_InFrameRange(pl, 0, 90.0f, 160.0f) != 0) {
                LongSword_EvadeInput(pl);
            }
        } else {
        }
        break;
    }
}

/*
 * Runs the long sword's PL_GROUP_WEAPON state: maps the state to its handler
 * and variant.
 */
void LongSword_RunState(Player *pl)
{
    switch (pl->state) {
    case LSWORD_SLASH:
        LongSword_StateSlash(pl, 0, 0);
        break;
    case LSWORD_SLASH_2:
        LongSword_StateSlash2(pl, 0);
        break;
    case LSWORD_FADE_SLASH:
        LongSword_StateFadeSlash(pl, 0U);
        break;
    case LSWORD_BOUNCE:
        LongSword_StateBounce(pl, 0);
        break;
    case LSWORD_THRUST:
        LongSword_StateThrust(pl, 0);
        break;
    case LSWORD_FADE_AFTER_SLASH:
        LongSword_StateFadeSlash(pl, 1U);
        break;
    case LSWORD_FADE_AFTER_SLASH_2:
        LongSword_StateFadeSlash(pl, 2U);
        break;
    case LSWORD_SPIRIT_1:
        LongSword_StateSpirit1(pl, 0U);
        break;
    case LSWORD_SPIRIT_THRUST:
        LongSword_StateSpiritThrust(pl, 0);
        break;
    case LSWORD_SPIRIT_2:
        LongSword_StateSpirit2(pl, 0);
        break;
    case LSWORD_SPIRIT_RISING:
        LongSword_StateSpiritRising(pl, 0);
        break;
    case LSWORD_SLASH_ALT:
        LongSword_StateSlash(pl, 1, 0);
        break;
    case LSWORD_STEP_SLASH:
        LongSword_StateSlash(pl, 0, 1);
        break;
    case LSWORD_STEP_SLASH_ALT:
        LongSword_StateSlash(pl, 1, 1);
        break;
    case LSWORD_RISING_SLASH:
        LongSword_StateRisingSlash(pl, 0);
        break;
    case LSWORD_SPIRIT_3:
        LongSword_StateSpirit3(pl, 0);
        break;
    case LSWORD_EXTRA_SLASH:
        LongSword_StateExtraSlash(pl, 0);
        break;
    case LSWORD_BOUNCE_SLASH:
        LongSword_StateSlash(pl, 2, 0);
        break;
    case LSWORD_SPIRIT_1_EMPTY:
        LongSword_StateSpirit1(pl, 1U);
        break;
    case LSWORD_DRAW_SLASH:
        LongSword_StateSlash(pl, 3, 0);
        break;
    case LSWORD_RISING_AFTER_THRUST:
        LongSword_StateRisingSlash(pl, 1);
        break;
    case LSWORD_FADE_AFTER_THRUST:
        LongSword_StateFadeSlash(pl, 3U);
        break;
    case LSWORD_FADE_AFTER_RISING:
        LongSword_StateFadeSlash(pl, 4U);
        break;
    case LSWORD_SPIRIT_3_AFTER_RISING:
        LongSword_StateSpirit3(pl, 1);
        break;
    case LSWORD_SLASH_2_AFTER_RISING:
        LongSword_StateSlash2(pl, 1);
        break;
    case LSWORD_FADE_AFTER_SPIRIT_1:
        LongSword_StateFadeSlash(pl, 5U);
        break;
    case LSWORD_FADE_AFTER_SPIRIT_2:
        LongSword_StateFadeSlash(pl, 6U);
        break;
    case LSWORD_SPIRIT_2_AFTER_THRUST:
        LongSword_StateSpirit2(pl, 1);
        break;
    case LSWORD_SPIRIT_1_AFTER_RISING:
        LongSword_StateSpirit1(pl, 2U);
        break;
    case LSWORD_SPIRIT_1_AFTER_RISING_EMPTY:
        LongSword_StateSpirit1(pl, 3U);
        break;
    case LSWORD_DRAW_SPIRIT_1:
        LongSword_StateSpirit1(pl, 4U);
        break;
    case LSWORD_DRAW_SPIRIT_1_EMPTY:
        LongSword_StateSpirit1(pl, 5U);
        
    default:
        break;
    }
}

/*
 * The drawn idle's input: key 1 the slash (LSWORD_STEP_SLASH with the left
 * stick held), key 3 the fade slash, key 4 the first spirit slash (or its
 * empty form), input.pressed PAD_R3 the thrust, the roll button rolls. Returns 1
 * when it started a state.
 */
s32 LongSword_AttackInput(Player *pl)
{
    u16 pressed;

    switch (LongSword_ReadAttackKey(pl) & 0xFF) {
    case 0:
        break;
    case 1:
        if (pl->input.stickDir & PAD_DPAD) {
            Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_STEP_SLASH, PL_SET_SNAP_POS);
        } else {
            Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_SLASH, PL_SET_SNAP_POS);
        }
        return 1;
    case 3:
        Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_FADE_SLASH, PL_SET_SNAP_POS);
        return 1;
    case 4:
        if (LongSword_GaugeAtLeast(pl, 0xF) == 1) {
            Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_1, PL_SET_SNAP_POS);
        } else {
            Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_1_EMPTY, PL_SET_SNAP_POS);
        }
        return 1;
    }
    pressed = pl->input.pressed;
    if (pressed & PAD_R3) {
        Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_THRUST, PL_SET_SNAP_POS);
        return 1;
    }
    if ((pressed & PAD_CROSS) && pl->stamina >= 75) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
        return 1;
    }
    return 0;
}

/*
 * Returns the key pressed: 1 a flick to the attack stick's upper half, 3 to
 * its lower half, 4 input.pressed PAD_R1, 10 input.pressed PAD_R3, 0 none.
 */
s32 LongSword_ReadAttackKey(Player *pl)
{
    u16 btn;

    if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
        if (pl->input.rightStickAngle < 0x8001) {
            return 1;
        }
        return 3;
    }
    btn = pl->input.pressed;
    if (btn & 0x80) {
        return 4;
    }
    return !(btn & 1) ? 0 : 0xA;
}

/* The roll button (75 stamina): a roll, or a side evade by the left stick. */
void LongSword_EvadeInput(Player *pl)
{
    s32 dir;

    if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B)) {
        if (pl->input.stickDir & PAD_DPAD) {
            dir = (pl->input.stickAngle + 0x4000) & 0xFFFF;
            if (dir < 0x5C72) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_RIGHT, PL_SET_SNAP_POS);
                return;
            }
            if (dir < 0xA38E) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                return;
            }
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_EVADE_LEFT, PL_SET_SNAP_POS);
            return;
        }
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
    }
}

/*
 * The follow-up of combo row `row` for key `key`. With peek 1, returns the
 * key when the row has a follow-up for it, else 0xFF; with peek 0,
 * starts it (key 4 checks the spirit gauge first).
 */
u8 LongSword_ComboInput(Player *pl, u32 row, u8 key, u8 peek)
{
    switch (key) {
    case 1:
        switch (row) {
        case 1:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SLASH_2, 4, 0);
            } else {
                return 1;
            }
            break;
        case 4:
        case 13:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_THRUST, 4, 1);
            } else {
                return 1;
            }
            break;
        case 6:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_RISING, 4, 1);
            } else {
                return 1;
            }
            break;
        case 12:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SLASH_2_AFTER_RISING, 4, 1);
            } else {
                return 1;
            }
            break;
        case 11:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_RISING_AFTER_THRUST, 4, 1);
            } else {
                return 1;
            }
            break;
        default:
            return 0xFF;
        }
        break;
    case 3:
        switch (row) {
        case 1:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_FADE_AFTER_SLASH, 4, 0);
            } else {
                return 3;
            }
            break;
        case 2:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_FADE_AFTER_SLASH_2, 4, 0);
            } else {
                return 3;
            }
            break;
        case 4:
        case 13:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_FADE_AFTER_SPIRIT_1, 4, 0);
            } else {
                return 3;
            }
            break;
        case 6:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_FADE_AFTER_SPIRIT_2, 4, 0);
            } else {
                return 3;
            }
            break;
        case 8:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_FADE_AFTER_SLASH, 4, 0);
            } else {
                return 3;
            }
            break;
        case 9:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_FADE_SLASH, 4, 0);
            } else {
                return 3;
            }
            break;
        case 5:
        case 11:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_FADE_AFTER_THRUST, 4, 0);
            } else {
                return 3;
            }
            break;
        case 7:
        case 12:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_FADE_AFTER_RISING, 4, 0);
            } else {
                return 3;
            }
            break;
        default:
            return 0xFF;
        }
        break;
    case 4:
        switch (row) {
        case 1:
        case 2:
        case 11:
            if (peek == 0) {
                if (LongSword_GaugeAtLeast(pl, 0xF) == 1) {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_1, 4, 1);
                } else {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_1_EMPTY, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 12:
            if (peek == 0) {
                if (LongSword_GaugeAtLeast(pl, 0xF) == 1) {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_1_AFTER_RISING, 4, 1);
                } else {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_1_AFTER_RISING_EMPTY, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 4:
            if (peek == 0) {
                if (LongSword_GaugeAtLeast(pl, 0x14) == 1) {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_2, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 5:
            if (peek == 0) {
                if (LongSword_GaugeAtLeast(pl, 0x14) == 1) {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_2_AFTER_THRUST, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 6:
            if (peek == 0) {
                if (LongSword_GaugeAtLeast(pl, 0x19) == 1) {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_3, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 7:
            if (peek == 0) {
                if (LongSword_GaugeAtLeast(pl, 0x19) == 1) {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_3_AFTER_RISING, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        default:
            return 0xFF;
        }
        break;
    case 10:
        switch (row) {
        case 1:
        case 2:
            if (peek == 0) {
                Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_THRUST, 4, 1);
            } else {
                return 10;
            }
            break;
        default:
            return 0xFF;
        }
        break;
    }
    return 0xFF;
}

/* The attack that draws on the move: LSWORD_DRAW_SLASH. */
void LongSword_DrawAttack(Player *pl)
{
    Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_DRAW_SLASH, PL_SET_SNAP_POS);
}

/*
 * input.held PAD_R1 while sheathed: LSWORD_DRAW_SPIRIT_1, or its empty form
 * with the gauge below 15.
 */
void LongSword_SpiritDraw(Player *pl)
{
    if (LongSword_GaugeAtLeast(pl, 0xF) == 1) {
        Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_DRAW_SPIRIT_1, PL_SET_SNAP_POS);
        return;
    }
    Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_DRAW_SPIRIT_1_EMPTY, PL_SET_SNAP_POS);
}

/*
 * Sets the state; with turn 1, first turns targetYaw 0x1000 toward a sideways
 * left stick.
 */
void Player_TurnAndSetState(Player *pl, u8 group, u8 state, s32 mode, u8 turn)
{
    if (turn == 1) {
        if (pl->input.stickDir & PAD_LEFT) {
            pl->targetYaw += 0x1000;
        } else if (pl->input.stickDir & PAD_RIGHT) {
            pl->targetYaw -= 0x1000;
        }
    }
    Player_SetState(pl, group, state, mode);
}

/*
 * The attack's hit, tracked in stateVar1: a bounce requests HORN_BOUNCE and
 * returns 1; kinds 2 and 3 step the motion back 2 frames and shake the
 * camera. Returns 0 otherwise.
 */
s32 HuntingHorn_CheckHit(Player *pl)
{

    switch (pl->stateVar1) {
    case 0:
        if (pl->hitPending != 0) {
            CamShake_ReqA(pl, 0);
            switch (pl->hitKind) {
            case 0:
                pl->stateVar1++;
                break;
            case 1:
                pl->stateVar1++;
                pl->hitStop = 0;
                pl->hitStopAdd = 0;
                Player_RequestState(pl, PL_GROUP_WEAPON, HORN_BOUNCE, 0);
                return 1;
            case 2:
            case 3:
                pl->stateVar1++;
                Player_SetMotionPair(pl, pl->motionId[0], 0, (s16) (pl->motion[0].frame - 2.0f));
                pl->weaponSeMute = 2;
                CamShake_ReqA(pl, 1);
                break;
            }
        }
        break;
    case 1:
        if (pl->hitSlowTimer <= 0) {
            pl->stateVar1++;
        }
        break;
    case 2:
        break;
    }
    return 0;
}

u16 Camera_GetYaw(void);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
/* While the stick is pushed: turns the heading toward the stick's direction
 * (input.stickAngle, relative to the camera's yaw) by at most maxTurn, then
 * moves dist along the new heading. */
void Player_StickTurnMove(Player *pl, s16 maxTurn, f32 dist)
{
    f32 mtx[16];
    f32 step[3];
    f32 out[3];
    s32 ang[3];
    u16 target;
    s32 delta;

    if (Player_StickMoveLevel(pl, 0) > 0) {
        target = pl->input.stickAngle + Camera_GetYaw();
        delta = (target - pl->rot[1]) & 0xFFFF;
        if ((u32) (delta + maxTurn) < (u32) (maxTurn * 2)) {
            pl->rot[1] = target;
        } else if (delta < 0x8000U) {
            pl->rot[1] += maxTurn;
        } else {
            pl->rot[1] -= maxTurn;
        }
        pl->targetYaw = pl->rot[1];
        ang[0] = 0;
        ang[2] = 0;
        ang[1] = pl->rot[1];
        Mtx_FromAnglesXYZ(ang, mtx);
        step[0] = 0.0f;
        step[2] = dist;
        step[1] = 0.0f;
        Vec_Rotate(out, step, mtx);
        pl->pos[0] += out[0];
        pl->pos[2] += out[2];
    }
}

/*
 * A play stance: stance 0-3 (up, right, left, down) loops its motion and adds
 * its note every 60 frames. variant: 0 and 1 the blend (4 or 0), 2-4 the
 * entry motions from a swing. input.pressed PAD_R1 leaves (the play end), a
 * flick starts the stance attack; the left stick turns and walks.
 */
void HuntingHorn_StatePlay(Player *pl, s32 stance, s32 variant)
{
    s16 i;
    FrameRange *note0Range;
    FrameRange *note1Range;
    s16 j;
    FrameRange *note2Range;
    s16 k;

    if (pl->timer > 0) {
        pl->timer--;
    }
    if (pl->stateVar1 > 0) {
        pl->stateVar1--;
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->weaponOut = 1;
        pl->timer = 10;
        pl->stateVar0 = 0;
        pl->stateVar1 = 40;
        switch (variant) {
        case 0:
            pl->step = 3;
            Player_SetMotionPair(pl, HuntingHorn_StanceMotions[stance], 4, 0);
            break;
        case 1:
            pl->step = 3;
            Player_SetMotionPair(pl, HuntingHorn_StanceMotions[stance], 0, 0);
            break;
        case 2:
        case 3:
            pl->step = 2;
            Player_SetMotionPair(pl, HuntingHorn_StanceEntryMotions[variant].mot, HuntingHorn_StanceEntryMotions[variant].blend, 0);
            break;
        case 4:
            Player_SetMotionPair(pl, HuntingHorn_StanceEntryMotions[variant].mot, HuntingHorn_StanceEntryMotions[variant].blend, 0);
            break;
        }
        Player_SetMoveMode(pl, 0, 1, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x594, 0, 8);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, HuntingHorn_StanceEntryMotions[variant].mot2, HuntingHorn_StanceEntryMotions[variant].blend2, HuntingHorn_StanceEntryMotions[variant].start2);
        }
        break;
    case 3:
        if (Player_IsLocal(pl) == 1) {
            if (pl->stateVar1 == 0) {
                pl->stateVar0++;
                pl->stateVar1 = 60;
                StageObj_K29_CreateOnPlayerDefault(pl, 0xB, 1.0f);
                switch (pl->motionId[0]) {
                case 0x57D:
                default:
                    HuntingHorn_AddNote(pl, 0);
                    break;
                case 0x57E:
                    HuntingHorn_AddNote(pl, 1);
                    break;
                case 0x57F:
                    HuntingHorn_AddNote(pl, 2);
                    break;
                }
            }
            if (pl->timer == 0) {
                if (pl->input.pressed & PAD_R1) {
                    switch (pl->motionId[0]) {
                    case 0x57D:
                    default:
                        Player_RequestState(pl, PL_GROUP_WEAPON, HORN_PLAY_END, PL_SET_SNAP_POS);
                        break;
                    case 0x57E:
                        Player_RequestState(pl, PL_GROUP_WEAPON, HORN_PLAY_END_NOTE1, PL_SET_SNAP_POS);
                        break;
                    case 0x57F:
                        Player_RequestState(pl, PL_GROUP_WEAPON, HORN_PLAY_END_NOTE2, PL_SET_SNAP_POS);
                        break;
                    }
                } else {
                    switch (pl->motionId[0]) {
                    case 0x57D:
                        for (i = 0, note0Range = HuntingHorn_Note0WalkFrames; i < 3; i++, note0Range++) {
                            if (Motion_InFrameRange(pl, 0, note0Range->from, note0Range->to) != 0) {
                                Player_StickTurnMove(pl, 0x180, 3.0f);
                            }
                        }
                        break;
                    case 0x57E:
                        for (j = 0, note1Range = HuntingHorn_Note1WalkFrames; j < 3; j++, note1Range++) {
                            if (Motion_InFrameRange(pl, 0, note1Range->from, note1Range->to) != 0) {
                                Player_StickTurnMove(pl, 0x180, 1.5f);
                            }
                        }
                        break;
                    case 0x57F:
                        for (k = 0, note2Range = HuntingHorn_Note2WalkFrames; k < 2; k++, note2Range++) {
                            if (Motion_InFrameRange(pl, 0, note2Range->from, note2Range->to) != 0) {
                                Player_StickTurnMove(pl, 0x180, 1.5f);
                            }
                        }
                        break;
                    }
                    HuntingHorn_PlayInput(pl, stance);
                }
            }
        }
        break;
    }
}

/* unk_6AF280's .data (0x895300-0x896ED0) */
extern char D_008DA300[];
extern char D_008DA320[];
extern char D_008DA340[];
extern char D_008DA370[];
extern char D_008DA3A0[];
extern char D_008DA3D0[];
extern char D_008DA3F0[];
extern char D_008DA410[];
extern char D_008DA430[];
extern char D_008DA460[];
extern char D_008DA490[];
extern char D_008DA4B0[];
extern char D_008DA4D0[];
extern char D_008DA500[];
extern char D_008DA520[];
extern char D_008DA550[];
extern char D_008DAFD8[];
extern char D_008DB5F8[];
extern char D_008DBDC8[];
extern u32 D_00895D80[];
extern u32 D_008962B0[];
extern u32 D_00896A40[];
extern u32 D_00896C10[];
extern u32 D_00896D80[];
u32 D_00895300[4] = {
#include "game/00895300.inc"
};
u32 D_00895310[8] = {
#include "game/00895310.inc"
};
u32 D_00895330[4] = {
#include "game/00895330.inc"
};
u32 D_00895340[12] = {
#include "game/00895340.inc"
};
u32 D_00895370[12] = {
#include "game/00895370.inc"
};
u32 D_008953A0[4] = {
#include "game/008953A0.inc"
};
u32 D_008953B0[12] = {
#include "game/008953B0.inc"
};
u32 D_008953E0[12] = {
#include "game/008953E0.inc"
};
u32 D_00895410[12] = {
#include "game/00895410.inc"
};
u32 D_00895440[8] = {
#include "game/00895440.inc"
};
u32 D_00895460[12] = {
#include "game/00895460.inc"
};
u32 D_00895490[8] = {
#include "game/00895490.inc"
};
u32 D_008954B0[12] = {
#include "game/008954B0.inc"
};
u32 D_008954E0[12] = {
#include "game/008954E0.inc"
};
u32 D_00895510[8] = {
#include "game/00895510.inc"
};
u32 D_00895530[12] = {
#include "game/00895530.inc"
};
u32 D_00895560[16] = {
#include "game/00895560.inc"
};
u32 D_008955A0[4] = {
#include "game/008955A0.inc"
};
u32 D_008955B0[8] = {
#include "game/008955B0.inc"
};
u32 D_008955D0[16] = {
#include "game/008955D0.inc"
};
u32 D_00895610[12] = {
#include "game/00895610.inc"
};
u32 D_00895640[12] = {
#include "game/00895640.inc"
};
u32 D_00895670[8] = {
#include "game/00895670.inc"
};
u32 D_00895690[12] = {
#include "game/00895690.inc"
};
u32 D_008956C0[8] = {
#include "game/008956C0.inc"
};
u32 D_008956E0[20] = {
#include "game/008956E0.inc"
};
u32 D_00895730[16] = {
#include "game/00895730.inc"
};
u32 D_00895770[8] = {
#include "game/00895770.inc"
};
u32 D_00895790[8] = {
#include "game/00895790.inc"
};
u32 D_008957B0[16] = {
#include "game/008957B0.inc"
};
u32 D_008957F0[8] = {
#include "game/008957F0.inc"
};
u32 D_00895810[16] = {
#include "game/00895810.inc"
};
u32 D_00895850[16] = {
#include "game/00895850.inc"
};
u32 D_00895890[16] = {
#include "game/00895890.inc"
};
u32 D_008958D0[16] = {
#include "game/008958D0.inc"
};
u32 D_00895910[12] = {
#include "game/00895910.inc"
};
u32 D_00895940[16] = {
#include "game/00895940.inc"
};
u32 D_00895980[8] = {
#include "game/00895980.inc"
};
u32 D_008959A0[16] = {
#include "game/008959A0.inc"
};
u32 D_008959E0[8] = {
#include "game/008959E0.inc"
};
u32 D_00895A00[8] = {
#include "game/00895A00.inc"
};
u32 D_00895A20[12] = {
#include "game/00895A20.inc"
};
u32 D_00895A50[16] = {
#include "game/00895A50.inc"
};
u32 D_00895A90[8] = {
#include "game/00895A90.inc"
};
u32 D_00895AB0[12] = {
#include "game/00895AB0.inc"
};
u32 D_00895AE0[4] = {
#include "game/00895AE0.inc"
};
u32 D_00895AF0[12] = {
#include "game/00895AF0.inc"
};
u32 D_00895B20[6] = {
#include "game/00895B20.inc"
};
u32 D_00895B38[2] = {
#include "game/00895B38.inc"
};
u32 D_00895B40[4] = {
#include "game/00895B40.inc"
};
u32 D_00895B50[6] = {
#include "game/00895B50.inc"
};
u32 D_00895B68[2] = {
#include "game/00895B68.inc"
};
u32 D_00895B70[8] = {
#include "game/00895B70.inc"
};
u32 D_00895B90[8] = {
#include "game/00895B90.inc"
};
u32 D_00895BB0[8] = {
#include "game/00895BB0.inc"
};
u32 D_00895BD0[12] = {
#include "game/00895BD0.inc"
};
u32 D_00895C00[8] = {
#include "game/00895C00.inc"
};
u32 D_00895C20[4] = {
#include "game/00895C20.inc"
};
u32 D_00895C30[8] = {
#include "game/00895C30.inc"
};
u32 D_00895C50[8] = {
#include "game/00895C50.inc"
};
u32 D_00895C70[8] = {
#include "game/00895C70.inc"
};
u32 D_00895C90[8] = {
#include "game/00895C90.inc"
};
u32 D_00895CB0[8] = {
#include "game/00895CB0.inc"
};
u32 D_00895CD0[12] = {
#include "game/00895CD0.inc"
};
u32 D_00895D00[16] = {
#include "game/00895D00.inc"
};
u32 D_00895D40[8] = {
#include "game/00895D40.inc"
};
u32 D_00895D60[8] = {
#include "game/00895D60.inc"
};
u32 D_00895D80[76] = {
#include "game/00895D80.inc"
};
u32 D_00895EB0[16] = {
#include "game/00895EB0.inc"
};
u32 D_00895EF0[16] = {
#include "game/00895EF0.inc"
};
u32 D_00895F30[16] = {
#include "game/00895F30.inc"
};
u32 D_00895F70[16] = {
#include "game/00895F70.inc"
};
u32 D_00895FB0[16] = {
#include "game/00895FB0.inc"
};
u32 D_00895FF0[16] = {
#include "game/00895FF0.inc"
};
u32 D_00896030[16] = {
#include "game/00896030.inc"
};
u32 D_00896070[16] = {
#include "game/00896070.inc"
};
u32 D_008960B0[16] = {
#include "game/008960B0.inc"
};
u32 D_008960F0[16] = {
#include "game/008960F0.inc"
};
u32 D_00896130[16] = {
#include "game/00896130.inc"
};
u32 D_00896170[16] = {
#include "game/00896170.inc"
};
u32 D_008961B0[16] = {
#include "game/008961B0.inc"
};
u32 D_008961F0[16] = {
#include "game/008961F0.inc"
};
u32 D_00896230[16] = {
#include "game/00896230.inc"
};
u32 D_00896270[16] = {
#include "game/00896270.inc"
};
u32 D_008962B0[56] = {
#include "game/008962B0.inc"
};
char *D_00896390[16] = {
    D_008DA300, D_008DA320, D_008DA340, D_008DA370,
    D_008DA3A0, D_008DA3D0, D_008DA3F0, D_008DA410,
    D_008DA430, D_008DA460, D_008DA490, D_008DA4B0,
    D_008DA4D0, NULL, NULL, NULL,
};
char *D_008963D0[16] = {
    D_008DA300, D_008DA500, D_008DA520, D_008DA550,
    D_008DA570, D_008DA580, D_008DA5A0, D_008DA5C0,
    D_008DA5F0, D_008DA570, D_008DA620, D_008DA650,
    D_008DA670, NULL, NULL, NULL,
};
char *D_00896410[16] = {
    D_008DA690, D_008DA6C0, D_008DA6E0, D_008DA700,
    D_008DA720, D_008DA570, D_008DA750, D_008DA770,
    D_008DA790, D_008DA7B0, D_008DA7D0, D_008DA7E0,
    D_008DA810, NULL, NULL, NULL,
};
char *D_00896450[16] = {
    D_008DA830, D_008DA860, D_008DA890, D_008DA8B0,
    D_008DA8E0, D_008DA900, D_008DA930, D_008DA570,
    D_008DA950, D_008DA980, D_008DA9B0, D_008DA9E0,
    D_008DA720, NULL, NULL, NULL,
};
char *D_00896490[16] = {
    D_008DAA00, D_008DAA30, D_008DAA50, D_008DAA70,
    D_008DA570, D_008DAA90, D_008DAAC0, D_008DA570,
    D_008DAAE0, D_008DAB10, D_008DAB30, D_008DAB50,
    NULL, NULL, NULL, NULL,
};
char *D_008964D0[16] = {
    D_008DAB70, D_008DABA0, D_008DABD0, D_008DABF0,
    D_008DAC20, D_008DA570, D_008DAC40, D_008DAC70,
    D_008DACA0, D_008DACC0, D_008DA570, D_008DACD0,
    D_008DAD00, NULL, NULL, NULL,
};
char *D_00896510[16] = {
    D_008DAD20, D_008DAD50, D_008DAD70, D_008DAD90,
    D_008DA570, D_008DADB0, D_008DADE0, D_008DAE00,
    D_008DA570, D_008DAE20, D_008DAE50, D_008DAE70,
    D_008DAEA0, NULL, NULL, NULL,
};
char *D_00896550[16] = {
    D_008DAEC0, D_008DAEF0, D_008DAF10, D_008DAF30,
    D_008DA570, D_008DAF40, D_008DAF80, D_008DAFB0,
    D_008DAFD8, D_008DAFF0, D_008DB010, D_008DB030,
    D_008DB058, NULL, NULL, NULL,
};
char *D_00896590[16] = {
    D_008DB070, D_008DB090, D_008DA570, D_008DB0C0,
    D_008DB0F0, D_008DB110, D_008DA570, D_008DB130,
    D_008DB150, D_008DA570, D_008DB170, D_008DB198,
    D_008DA570, NULL, NULL, NULL,
};
char *D_008965D0[16] = {
    D_008DB1B0, D_008DB1D0, D_008DB200, D_008DA570,
    D_008DB220, D_008DB240, D_008DA570, D_008DB260,
    D_008DB280, D_008DA570, D_008DB2A0, D_008DB2C0,
    D_008DB2E0, NULL, NULL, NULL,
};
char *D_00896610[16] = {
    D_008DB310, D_008DB330, D_008DA570, D_008DB350,
    D_008DB360, D_008DA570, D_008DB390, D_008DB3B0,
    D_008DA570, D_008DB3D0, D_008DB3F0, D_008DB420,
    NULL, NULL, NULL, NULL,
};
char *D_00896650[16] = {
    D_008DB310, D_008DB440, D_008DA570, D_008DB350,
    D_008DB460, D_008DB480, D_008DA570, D_008DB4B0,
    D_008DB4D0, D_008DB4F0, D_008DA570, D_008DB510,
    D_008DB530, NULL, NULL, NULL,
};
char *D_00896690[16] = {
    D_008DB310, D_008DB550, D_008DA570, D_008DB350,
    D_008DB570, D_008DA570, D_008DB5A0, D_008DB5C0,
    D_008DA570, D_008DB5E0, D_008DB5F8, D_008DB610,
    D_008DA570, NULL, NULL, NULL,
};
char *D_008966D0[16] = {
    D_008DB630, D_008DB660, D_008DA570, D_008DB690,
    D_008DB6C0, D_008DB6E0, D_008DA570, D_008DB700,
    D_008DB720, D_008DB740, D_008DB750, D_008DB780,
    D_008DB7A0, NULL, NULL, NULL,
};
char *D_00896710[16] = {
    D_008DB7C0, D_008DB7E0, D_008DB810, D_008DB830,
    D_008DB860, D_008DB890, D_008DB8B8, D_008DA570,
    D_008DB8D0, D_008DB8F0, D_008DB920, D_008DB940,
    D_008DB950, NULL, NULL, NULL,
};
char *D_00896750[16] = {
    D_008DB7C0, D_008DB980, D_008DB9B0, D_008DA570,
    D_008DB9D0, D_008DBA00, D_008DBA30, D_008DBA50,
    D_008DA570, D_008DBA70, D_008DBAA0, D_008DBAD0,
    D_008DBB00, NULL, NULL, NULL,
};
char *D_00896790[16] = {
    D_008DBB20, D_008DBB50, D_008DB9B0, D_008DBB80,
    D_008DBBA0, D_008DBBD0, D_008DBC00, D_008DBC20,
    D_008DBC50, D_008DBC80, D_008DBCB0, D_008DBCE0,
    D_008DBD10, NULL, NULL, NULL,
};
char *D_008967D0[16] = {
    D_008DB7C0, D_008DBD30, D_008DB9B0, D_008DA570,
    D_008DBD60, D_008DBD80, D_008DBDA0, D_008DBDC8,
    D_008DA570, D_008DBDE0, D_008DBE10, D_008DBE30,
    D_008DBE50, NULL, NULL, NULL,
};
char *D_00896810[16] = {
    D_008DB7C0, D_008DBE70, D_008DB9B0, D_008DA570,
    D_008DBEA0, D_008DBEC0, D_008DBEF0, D_008DA570,
    D_008DBF18, D_008DBF30, D_008DBF60, D_008DBF80,
    D_008DBFB0, NULL, NULL, NULL,
};
char *D_00896850[16] = {
    D_008DB7C0, D_008DBFE0, D_008DB9B0, D_008DA570,
    D_008DC010, D_008DC038, D_008DC050, D_008DC080,
    D_008DC0B0, D_008DC0D0, D_008DA570, D_008DC100,
    D_008DC130, NULL, NULL, NULL,
};
char *D_00896890[16] = {
    D_008DB7C0, D_008DC160, D_008DB9B0, D_008DA570,
    D_008DC190, D_008DC1B0, D_008DC1D0, D_008DC200,
    D_008DC220, D_008DA570, D_008DC240, D_008DC270,
    D_008DC290, NULL, NULL, NULL,
};
u32 D_008968D0[16] = {
#include "game/008968D0.inc"
};
char *D_00896910[16] = {
    D_008DC470, D_008DC490, D_008DC4B0, D_008DC4D0,
    D_008DC4F0, D_008DA570, D_008DC520, D_008DC550,
    D_008DC570, D_008DA570, D_008DC590, D_008DC5C0,
    D_008DC5F0, NULL, NULL, NULL,
};
char *D_00896950[16] = {
    D_008DC610, D_008DC640, D_008DA570, D_008DB350,
    D_008DC670, D_008DA570, D_008DC6A0, D_008DC6C0,
    D_008DC6F0, D_008DA570, D_008DC710, D_008DC730,
    D_008DC750, NULL, NULL, NULL,
};
char *D_00896990[16] = {
    D_008DC780, D_008DC7B0, D_008DA570, D_008DB350,
    D_008DC7D0, D_008DA570, D_008DC7F0, D_008DC810,
    D_008DC840, D_008DA570, D_008DC710, D_008DC730,
    D_008DC860, NULL, NULL, NULL,
};
char *D_008969D0[16] = {
    D_008DC880, D_008DA570, D_008DB350, D_008DC8B0,
    D_008DA570, D_008DC8E0, D_008DA570, D_008DC900,
    D_008DC920, D_008DC950, D_008DA570, D_008DC970,
    D_008DC9A0, NULL, NULL, NULL,
};
char *D_00896A10[12] = {
    D_008DC880, D_008DA570, D_008DB350, D_008DC9C0,
    D_008DA570, D_008DB5A0, D_008DC9F0, D_008DA570,
    D_008DC710, D_008DC730, D_008DCA10, NULL,
};
u32 D_00896A40[60] = {
#include "game/00896A40.inc"
};
u32 D_00896B30[2] = {
#include "game/00896B30.inc"
};
u32 D_00896B38[2] = {
#include "game/00896B38.inc"
};
u32 D_00896B40[2] = {
#include "game/00896B40.inc"
};
u32 D_00896B48[2] = {
#include "game/00896B48.inc"
};
u32 D_00896B50[2] = {
#include "game/00896B50.inc"
};
u32 D_00896B58[2] = {
#include "game/00896B58.inc"
};
u32 D_00896B60[2] = {
#include "game/00896B60.inc"
};
u32 D_00896B68[2] = {
#include "game/00896B68.inc"
};
u32 D_00896B70[2] = {
#include "game/00896B70.inc"
};
u32 D_00896B78[2] = {
#include "game/00896B78.inc"
};
u32 D_00896B80[2] = {
#include "game/00896B80.inc"
};
u32 D_00896B88[2] = {
#include "game/00896B88.inc"
};
u32 D_00896B90[4] = {
#include "game/00896B90.inc"
};
u32 D_00896BA0[12] = {
#include "game/00896BA0.inc"
};
u32 D_00896BD0[8] = {
#include "game/00896BD0.inc"
};
u32 D_00896BF0[6] = {
#include "game/00896BF0.inc"
};
u32 D_00896C08[2] = {
#include "game/00896C08.inc"
};
u32 D_00896C10[24] = {
#include "game/00896C10.inc"
};
u32 D_00896C70[4] = {
#include "game/00896C70.inc"
};
u32 D_00896C80[4] = {
#include "game/00896C80.inc"
};
u32 D_00896C90[4] = {
#include "game/00896C90.inc"
};
u32 D_00896CA0[4] = {
#include "game/00896CA0.inc"
};
u32 D_00896CB0[4] = {
#include "game/00896CB0.inc"
};
u32 D_00896CC0[4] = {
#include "game/00896CC0.inc"
};
u32 D_00896CD0[4] = {
#include "game/00896CD0.inc"
};
u32 D_00896CE0[4] = {
#include "game/00896CE0.inc"
};
u32 D_00896CF0[4] = {
#include "game/00896CF0.inc"
};
u32 D_00896D00[4] = {
#include "game/00896D00.inc"
};
u32 D_00896D10[4] = {
#include "game/00896D10.inc"
};
u32 D_00896D20[4] = {
#include "game/00896D20.inc"
};
u32 D_00896D30[4] = {
#include "game/00896D30.inc"
};
u32 D_00896D40[4] = {
#include "game/00896D40.inc"
};
u32 D_00896D50[4] = {
#include "game/00896D50.inc"
};
u32 D_00896D60[4] = {
#include "game/00896D60.inc"
};
u32 D_00896D70[4] = {
#include "game/00896D70.inc"
};
u32 D_00896D80[24] = {
#include "game/00896D80.inc"
};
MotionBlend GreatSword_OverheadBlends[8] = {
#include "game/00896DE0.inc"
};
GunMotion Bowgun_FireMotions[5] = {
#include "game/00896E00.inc"
};
s16 DualSwords_SideSlashMotions[8] = {
#include "game/00896E20.inc"
};
u16 HuntingHorn_StanceMotions[8] = {
#include "game/00896E30.inc"
};
HornMotion HuntingHorn_StanceEntryMotions[6] = {
#include "game/00896E40.inc"
};
FrameRange HuntingHorn_Note0WalkFrames[4] = {
#include "game/00896E80.inc"
};
FrameRange HuntingHorn_Note1WalkFrames[4] = {
#include "game/00896EA0.inc"
};
FrameRange HuntingHorn_Note2WalkFrames[2] = {
#include "game/00896EC0.inc"
};
