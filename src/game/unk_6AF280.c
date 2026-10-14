/*
 * Game overlay code at 0x6AF280-0x6BB9D0 (starts with GreatSword_State00), not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "em.h"
#include "player.h"

/* prototypes */
typedef struct HornMotion { s16 mot; s16 blend; s16 mot2; s16 blend2; s16 start2; } HornMotion;
typedef struct FrameRange { f32 from; f32 to; } FrameRange;
extern u16 HuntingHorn_State00_Motions[];
extern HornMotion HuntingHorn_State00_VarMotions[];
extern FrameRange HuntingHorn_State00_Fx57D[];
extern FrameRange HuntingHorn_State00_Fx57E[];
extern FrameRange HuntingHorn_State00_Fx57F[];
void Player_StickTurnMove(Player *, s16, f32);
void HuntingHorn_IdleInput(Player *, s32);
void HuntingHorn_AddNote(Player *, u8);
void GreatSword_SetChargeAttack(Player *em);
typedef struct MotionBlend { s16 blend; s16 start; } MotionBlend;
extern MotionBlend GreatSword_State02_Motions[];
void StageObj_K00_Create(void *owner, u8 k, f32 *pos, u8 c);
void GreatSword_CheckInput(Player *em, s32 arg1);
typedef struct GunMotion { s16 mot; u16 aimA; u16 aimB; } GunMotion;
extern GunMotion Bowgun_State00_AimLayers[];
extern u8 D_45E0E2[][4]; /* +2 of a 4-byte row per ammo type in main's data (gAmmoLevels) */
void CamShake_ReqB(void *obj, s32 i);
void Bowgun_FireShot(Player *em, s32 arg1, s32 arg2, f32 scale);
u8 Pouch_Add(Player *p, u16 item, s16 n);
void StageObj_Gun_CreateReload(void *owner, u8 k);
f32 Math_Cos(f32 x);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void DualSwords_ComboInput(Player *em, s32 a, s32 b);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
u8 Bowgun_ReloadLevel(Player *em, u8 k);
void Player_BeginStateKeepParts(Player *em, s32 action, s32 b, s32 c);
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
void Player_SetMotionPair(Player *, s32, s32, s32); /* unit's own declaration: the last parameter s32 (the definition has s16); with the definition's type GreatSword_State00 by 94 of 281, GreatSword_State01 by 141 of 316, GreatSword_State02 by 293 of 560 words differ */
void Player_SetMoveMode(Player *, u16 mode, s16 flag1, s16 flag2);
void Player_SetState(Player *, u8, u8, s32); /* unit's own declaration: (u8, u8, s32) (the definition has u16, u8, u16); with the definition's type Player_TurnAndSetState by 3 of 21 words differ */
u32 Player_TestFlag(void *obj, u32 flag);
void Player_UpdateMotionTime(Player *p, s16 step);
void Player_UseStamina(Player *p, s32 n);
void GreatSword_State01(Player *, s32);
void GreatSword_State02(Player *, u32);
void GreatSword_State03(Player *, s32);
void GreatSword_State19(Player *, u32);
void GreatSword_State26(Player *, s32);
void Bowgun_State00(Player *, s32, s32);
void Bowgun_State01(Player *, s32);
void Bowgun_State06(Player *, s32);
void Bowgun_State02(Player *, s32);
void Hammer_State00(Player *, s32);
void Hammer_State01(Player *, s32);
void Hammer_State02(Player *, s32);
void Hammer_State03(Player *, s32);
void Hammer_State05(Player *, s32);
void Hammer_State06(Player *, s32);
void Hammer_State07(Player *, s32);
void Hammer_State09(Player *, s32);
void Hammer_State10(Player *, s32);
void Hammer_CheckInput(Player *em, s32 arg1);
void Hammer_CheckChargeInput(Player *);
void Lance_State00(Player *, s32);
void Lance_State01(Player *, s32);
void Lance_State03(Player *, s32);
void Lance_State06(Player *, s32);
void Lance_State07(Player *, s32);
void Lance_CheckInput(Player *, s32);
s32 SwordShield_CheckHit(Player *);
void SwordShield_State00(Player *, s32);
void SwordShield_State04(Player *, s32);
void SwordShield_State05(Player *, s32);
void SwordShield_CheckInput(Player *em, s32 arg1);
s32 DualSwords_CheckHit(Player *);
void DualSwords_State00(Player *, s32);
void DualSwords_State01(Player *em, s32 arg1);
void DualSwords_State03(Player *, s32);
void DualSwords_State04(Player *, s32);
void DualSwords_State05(Player *, s32);
void DualSwords_State08(Player *, s32);
void DualSwords_CheckInput(Player *, s32);
void LongSword_State00(Player *, s32, s32);
void LongSword_State01(Player *, s32);
void LongSword_State02(Player *, u32);
void LongSword_State03(Player *, s32);
void LongSword_State04(Player *, s32);
void LongSword_State07(Player *, u32);
void LongSword_State08(Player *, s32);
void LongSword_State09(Player *, s32);
void LongSword_State10(Player *, s32);
void LongSword_State14(Player *, s32);
void LongSword_State15(Player *, s32);
void LongSword_State16(Player *, s32);
void Eff00_Create(Player *, u8);
s32 Motion_ReachedFrame(void *, s32, f32);
void CamShake_ReqA(void *obj, s32 i);
s32 Game_IsOnline(void);
s32 Quest_PhaseIsOver(s32);
void Player_BeginState(Player *p, s32 action, s32 a2, s32 a3);
void Player_TickWeaponTimer(Player *p);
s32 LongSword_GaugeAtLeast(Player *p, s16 n);
void GreatSword_State00(Player *, s32);
void Eff00_CreateDual(Player *, s32, s32);
extern s16 DualSwords_State01_Motions[];
s32 Motion_InFrameRange(void *, s32, f32, f32);
void Vib_StartLocal(void *port, s32 i);
s32 LongSword_StickAction(Player *em);
void LongSword_CheckInput(Player *em);
u8 LongSword_ComboInput(Player *, u32, u8, u8);
void Player_TurnAndSetState(Player *em, u8 group, u8 state, s32 mode, u8 turn);
struct StageObj *StageObj_K05_Create(void *owner, s32 arg1, s32 kind);
void StageObj_K29_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);
void LongSword_AddGauge(Player *p, s16 n);
typedef struct GunAmmoMask { u16 id; u32 mask; } GunAmmoMask;
extern GunAmmoMask D_45EE20[];
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

void GreatSword_State00(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->unk417 = 1;
        p->stateVar1 = 0;
        p->noFlinchTimer = 0x3C;
        switch (variant) {
        case 0:
            p->step++;
            Player_SetMotionPair(p, 0x57E, 4, 0);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 2);
            Player_SetMoveMode(p, 0, 1, 0);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            switch (p->stateVar0) {
            case 0:
                p->stateVar0++;
                Player_UpdateMotionTime(p, 0);
                switch (variant) {
                case 1:
                    Player_SetMotionPair(p, 0x589, 4, 0);
                    break;
                case 2:
                    Player_SetMotionPair(p, 0x58D, 4, 0);
                    break;
                case 3:
                case 4:
                    p->rot[1] = (u16) (p->rot[1] + 0x2000);
                    p->targetYaw = p->rot[1];
                    if (variant == 3) {
                        Player_SetMotionPair(p, 0x58F, 0, 0);
                    } else {
                        Player_SetMotionPair(p, 0x593, 0, 0);
                    }
                    break;
                }
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    switch (variant) {
                    case 2:
                        Player_SetMotionPair(p, 0x57E, 2, 0x2C);
                        break;
                    case 4:
                        Player_SetMotionPair(p, 0x57E, 0, 0x2C);
                        break;
                    default:
                        Player_SetMotionPair(p, 0x57E, 2, 0x1A);
                        break;
                    }
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 2);
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        } else if (p->hitPending != 0 && p->stateVar1 == 0) {
            p->stateVar1 = 1;
            CamShake_ReqA(p, 0);
            switch (p->hitKind) {
            case 0:
                StageObj_K05_Create(p, 0xF, 0);
                break;
            case 1:
                p->hitStop = 0;
                p->hitStopAdd = 0;
                p->unk417 = 0;
                Player_SetState(p, 1, 3, 0);
                break;
            case 2:
            case 3:
                p->step++;
                Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
                p->weaponSeMute = 2;
                CamShake_ReqA(p, 2);
                break;
            }
        } else if (Motion_ReachedFrame(p, 0, 72.0f) != 0 && Motion_ReachedFrame(p, 0, 98.0f) == 0) {
            GreatSword_CheckInput(p, 1);
        }
        break;
    case 2:
        if (p->hitSlowTimer <= 0) {
            p->step++;
            StageObj_K05_Create(p, 0x1C, 0);
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        } else if (Motion_ReachedFrame(p, 0, 78.0f) != 0 && Motion_ReachedFrame(p, 0, 104.0f) == 0) {
            GreatSword_CheckInput(p, 1);
        }
        break;
    }
}

void GreatSword_State01(Player *p, s32 variant)
{
    f32 v[4];
    f32 out[4];

    switch (p->step) {
    case 0:
        p->unk417 = 1;
        p->stateVar1 = 0;
        p->noFlinchTimer = 0x3C;
        switch (variant) {
        case 0:
            p->step++;
            Player_SetMotionPair(p, 0x57F, 4, 0);
            p->motionTime = 0;
            Eff00_Create(p, 1);
            Player_SetMoveMode(p, 0, 1, 0);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            switch (p->stateVar0) {
            case 0:
                p->stateVar0++;
                Player_UpdateMotionTime(p, 0);
                switch (variant) {
                case 1:
                    Player_SetMotionPair(p, 0x588, 4, 0);
                    break;
                case 2:
                    Player_SetMotionPair(p, 0x58B, 4, 0);
                    break;
                case 3:
                case 4:
                    p->rot[1] = (u16) (p->rot[1] - 0x2000);
                    p->targetYaw = p->rot[1];
                    if (variant == 3) {
                        Player_SetMotionPair(p, 0x58E, 0, 0);
                    } else {
                        Player_SetMotionPair(p, 0x591, 0, 0);
                    }
                    break;
                }
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    if (variant == 1) {
                        Player_SetMotionPair(p, 0x57F, 2, 0x40);
                    } else {
                        Player_SetMotionPair(p, 0x57F, 0, 0x42);
                    }
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 1);
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        } else if (p->hitPending != 0 && p->stateVar1 == 0) {
            p->stateVar1 = 1;
            CamShake_ReqA(p, 0);
            switch (p->hitKind) {
            case 0:
                StageObj_K05_Create(p, 0xF, 0);
                break;
            case 1:
                p->hitStop = 0;
                p->hitStopAdd = 0;
                p->unk417 = 0;
                Player_SetState(p, 1, 3, 0);
                break;
            case 2:
            case 3:
                p->step++;
                Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
                p->weaponSeMute = 2;
                CamShake_ReqA(p, 2);
                break;
            }
        } else {
            if (p->stateVar1 == 0 && Motion_PassedFrame(p, 0, 106.0f) != 0) {
                v[0] = 30.0f;
                v[1] = 0.0f;
                v[2] = -150.0f;
                Vec_Rotate(out, v, p->rotMtx);
                v[0] = p->pos[0] + out[0];
                v[1] = 15.0f + (p->pos[1] + out[1]);
                v[2] = p->pos[2] + out[2];
                StageObj_K00_Create(p, 0, v, 1);
                Vib_StartLocal(p, 2);
                CamShake_ReqA(p, 0);
            }
            if (Motion_ReachedFrame(p, 0, 116.0f) != 0 && Motion_ReachedFrame(p, 0, 192.0f) == 0) {
                GreatSword_CheckInput(p, 2);
            }
        }
        break;
    case 2:
        if (p->hitSlowTimer <= 0) {
            p->step++;
            StageObj_K05_Create(p, 0x1C, 0);
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        } else if (Motion_ReachedFrame(p, 0, 116.0f) != 0 && Motion_ReachedFrame(p, 0, 192.0f) == 0) {
            GreatSword_CheckInput(p, 2);
        }
        break;
    }
}

void GreatSword_State02(Player *p, u32 variant)
{
    f32 v[4];
    f32 out[4];

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0x3C;
        p->weaponOut = 1;
        p->unk417 = 1;
        switch (variant) {
        case 0:
            p->step++;
            Player_SetMotionPair(p, 0x579, 4, 0);
            Player_UpdateMotionTime(p, 0);
            break;
        case 1:
            p->step++;
            Player_SetMotionPair(p, 0x582, 2, 0);
            p->unk417 = 0;
            break;
        case 2:
            Player_SetMotionPair(p, 0x58C, 4, 0);
            break;
        case 3:
            Player_SetMotionPair(p, 0x58A, 4, 0);
            break;
        case 4:
            p->step++;
            Player_SetMotionPair(p, 0x57D, 4, 0);
            break;
        case 5:
            p->rot[1] += 0x2000;
            p->targetYaw = p->rot[1];
            Player_SetMotionPair(p, 0x590, 0, 0);
            break;
        case 6:
            p->rot[1] -= 0x2000;
            p->targetYaw = p->rot[1];
            Player_SetMotionPair(p, 0x592, 0, 0);
            break;
        }
        break;
    case 1:
        switch (variant) {
        case 0:
        case 1:
        case 4:
            p->step++;
            break;
        case 2:
            if (p->motion[0].playing == 0) {
                p->step++;
                Player_SetMotionPair(p, 0x579, 2, 0x32);
            }
            break;
        case 3:
            if (Motion_ReachedFrame(p, 0, 24.0f) != 0) {
                p->step++;
                Player_SetMotionPair(p, 0x595, 2, 0);
            }
            break;
        case 5:
            if (Motion_ReachedFrame(p, 0, 26.0f) != 0) {
                p->step++;
                Player_SetMotionPair(p, 0x596, 2, 0);
            }
            break;
        case 6:
            if (p->motion[0].playing == 0) {
                p->step++;
                Player_SetMotionPair(p, 0x579, 0, 0x34);
            }
            break;
        }
        break;
    case 2:
        switch (variant) {
        case 0:
        case 2:
        case 6:
            if (Motion_ReachedFrame(p, 0, 58.0f) != 0) {
                p->step++;
                Player_SetMotionPair(p, 0x594, GreatSword_State02_Motions[variant].blend, GreatSword_State02_Motions[variant].start);
                Player_UpdateMotionTime(p, 0);
                Eff00_Create(p, 3);
            }
            break;
        case 1:
        case 3:
        case 4:
        case 5:
            if (p->motion[0].playing == 0) {
                p->step++;
                Player_SetMotionPair(p, 0x594, GreatSword_State02_Motions[variant].blend, GreatSword_State02_Motions[variant].start);
                Player_UpdateMotionTime(p, 0);
                Eff00_Create(p, 3);
            }
            break;
        }
        break;
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
        } else if (p->hitPending != 0 && p->stateVar1 == 0) {
            p->stateVar1 = 1;
            CamShake_ReqA(p, 0);
            switch (p->hitKind) {
            case 0:
                StageObj_K05_Create(p, 0xF, 0);
                break;
            case 1:
                p->hitStop = 0;
                p->hitStopAdd = 0;
                Player_SetState(p, 1, 3, 0);
                break;
            case 2:
            case 3:
                p->step++;
                Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
                p->weaponSeMute = 2;
                CamShake_ReqA(p, 1);
                break;
            }
        } else {
            if (p->stateVar1 == 0 && Motion_PassedFrame(p, 0, 42.0f) != 0) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 150.0f;
                Vec_Rotate(out, v, p->rotMtx);
                v[0] = p->pos[0] + out[0];
                v[1] = 15.0f + (p->pos[1] + out[1]);
                v[2] = p->pos[2] + out[2];
                StageObj_K00_Create(p, 0, v, 1);
                Vib_StartLocal(p, 2);
                CamShake_ReqA(p, 0);
            }
            if (Motion_ReachedFrame(p, 0, 48.0f) != 0 && Motion_ReachedFrame(p, 0, 102.0f) == 0) {
                GreatSword_CheckInput(p, 3);
            }
        }
        break;
    case 4:
        if (p->hitSlowTimer <= 0) {
            p->step++;
            StageObj_K05_Create(p, 0x1C, 0);
        }
        break;
    case 5:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
        } else if (p->hitPending != 0 && p->stateVar1 == 0) {
            p->stateVar1 = 1;
            CamShake_ReqA(p, 0);
            switch (p->hitKind) {
            case 0:
                StageObj_K05_Create(p, 0xF, 0);
                break;
            case 1:
                p->hitStop = 0;
                p->hitStopAdd = 0;
                Player_SetState(p, 1, 3, 0);
                break;
            case 2:
            case 3:
                p->step++;
                Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
                p->weaponSeMute = 2;
                CamShake_ReqA(p, 1);
                break;
            }
        } else {
            if (p->stateVar1 == 0 && Motion_PassedFrame(p, 0, 42.0f) != 0) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 150.0f;
                Vec_Rotate(out, v, p->rotMtx);
                v[0] = p->pos[0] + out[0];
                v[1] = 15.0f + (p->pos[1] + out[1]);
                v[2] = p->pos[2] + out[2];
                StageObj_K00_Create(p, 0, v, 1);
                Vib_StartLocal(p, 2);
                CamShake_ReqA(p, 0);
            }
            if (Motion_ReachedFrame(p, 0, 48.0f) != 0 && Motion_ReachedFrame(p, 0, 102.0f) == 0) {
                GreatSword_CheckInput(p, 3);
            }
        }
        break;
    case 6:
        if (p->hitSlowTimer <= 0) {
            p->step++;
            StageObj_K05_Create(p, 0x1C, 0);
        }
        break;
    case 7:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        } else if (Motion_ReachedFrame(p, 0, 48.0f) != 0 && Motion_ReachedFrame(p, 0, 102.0f) == 0) {
            GreatSword_CheckInput(p, 3);
        }
        break;
    }
}

void GreatSword_State03(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x57C, 0, 0);
        break;
    case 1:
        if (p->stateVar0 != 0) {
            if (Motion_ReachedFrame(p, 0, 18.0f) != 0) {
                Player_SetState(p, 1U, 0x10U, 0xC);
            }
        } else if ((Motion_InFrameRange(p, 0, 2.0f, 30.0f) != 0) && (p->stateVar0 == 0) && (p->itemSelect == 0) && (p->input.stickDirPressed & 0x3C)) {
            p->stateVar0 = 1;
            p->stateVar1 = 0;
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 0xA, 0);
        }
        break;
    }
}

void GreatSword_State04(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x583, 0, 0);
        Eff00_Create(p, 4U);
        Player_SetMoveMode(p, 0, 1, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

u8 GreatSword_ChargeLevel(Player *p)
{
    s16 v = p->weaponTimer;
    u8 r;

    if (v < 40) {
        return 0;
    }
    r = (s16) (v - 40) / 20 + 1;
    if (r > 3) {
        r = 3;
    }
    return r;
}

void GreatSword_State19(Player *p, u32 variant)
{
    f32 from;
    f32 to;

    p->noFlinchTimer = 2;
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        p->weaponOut = 1;
        p->unk417 = 1;
        p->weaponTimer = 0;
        p->timer = 90;
        switch (variant) {
        case 0:
            p->step++;
            Player_SetMotionPair(p, 0x579, 4, 0);
            Player_UpdateMotionTime(p, 0);
            break;
        case 1:
            Player_SetMotionPair(p, 0x597, 2, 0);
            break;
        case 2:
            Player_SetMotionPair(p, 0x58C, 4, 0);
            break;
        case 3:
            p->step++;
            Player_SetMotionPair(p, 0x58A, 4, 0);
            break;
        case 5:
            p->step++;
            p->rot[1] += 0x2000;
            p->targetYaw = p->rot[1];
            Player_SetMotionPair(p, 0x590, 0, 0);
            break;
        case 6:
            p->rot[1] -= 0x2000;
            p->targetYaw = p->rot[1];
            Player_SetMotionPair(p, 0x592, 0, 0);
            break;
        }
        break;
    case 1:
        switch (variant) {
        default:
            /* the game reloads the step here (the cases reuse the switch value): volatile gives the reload */
            (*(volatile u8 *) &p->step)++;
            break;
        case 1:
            if (p->motion[0].playing == 0) {
                p->step++;
                Player_SetMotionPair(p, 0x579, 0, 0x38);
            }
            break;
        case 2:
            if (p->motion[0].playing == 0) {
                p->step++;
                Player_SetMotionPair(p, 0x579, 2, 0x32);
            }
            break;
        case 6:
            if (p->motion[0].playing == 0) {
                p->step++;
                Player_SetMotionPair(p, 0x579, 0, 0x34);
            }
            break;
        }
        break;
    case 2:
        if (Player_IsLocal(p) != 0) {
            if (variant == 3) {
                to = 24.0f;
                from = 0.0f;
            } else if (variant == 5) {
                to = 26.0f;
                from = 2.0f;
            } else {
                to = 58.0f;
                from = 34.0f;
            }
            if (Motion_ReachedFrame(p, 0, from) != 0) {
                if (p->stateVar1 == 0) {
                    p->stateVar1++;
                    StageObj_K29_CreateOnPlayerDefault(p, 8, 1.0f);
                }
                if (!(p->input.stickDir & 0x10)) {
                    p->stateVar0 = 1;
                }
                if (p->stateVar0 == 0) {
                    p->motionSpeed = 0.5f;
                    p->weaponTimer++;
                    if (--p->timer <= 0) {
                        p->stateVar0 = 1;
                    }
                } else {
                    p->motionSpeed = 1.0f;
                }
            }
            if (Motion_ReachedFrame(p, 0, to) != 0 && p->stateVar0 == 1) {
                p->chargeLevel = GreatSword_ChargeLevel(p);
                p->motionSpeed = 1.0f;
                switch (variant) {
                case 3:
                    Player_RequestState(p, 1, 0x1B, 4);
                    break;
                case 5:
                    Player_RequestState(p, 1, 0x1C, 4);
                    break;
                default:
                    Player_RequestState(p, 1, 0x1A, 4);
                    break;
                }
            }
        }
        break;
    }
}

void GreatSword_SetChargeAttack(Player *p)
{

    switch (p->chargeLevel) {
    case 0:
    default:
        p->attackPercent = 100;
        Eff00_Create(p, 3);
        break;
    case 1:
        p->attackPercent = 110;
        Eff00_Create(p, 0x5B);
        break;
    case 2:
        p->attackPercent = 120;
        Eff00_Create(p, 0x5C);
        break;
    case 3:
        p->attackPercent = 130;
        Eff00_Create(p, 0x5D);
        break;
    }
}

void GreatSword_State26(Player *p, s32 variant)
{
    f32 v[4];
    f32 out[4];

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0x3C;
        p->unk417 = 1;
        switch (variant) {
        case 0:
            p->step = 2;
            Player_SetMotionPair(p, 0x594, 2, 0);
            Player_UpdateMotionTime(p, 0);
            GreatSword_SetChargeAttack(p);
            break;
        case 1:
            Player_SetMotionPair(p, 0x595, 0, 0);
            Player_UpdateMotionTime(p, 0);
            break;
        case 2:
            Player_SetMotionPair(p, 0x596, 0, 0);
            Player_UpdateMotionTime(p, 0);
            break;
        }
        if (p->chargeLevel >= 2) {
            StageObj_K29_CreateOnPlayerDefault(p, 4, 1.0f);
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_UpdateMotionTime(p, 0);
            if (variant == 1) {
                Player_SetMotionPair(p, 0x594, 2, 8);
            } else {
                Player_SetMotionPair(p, 0x594, 0, 0xA);
            }
            GreatSword_SetChargeAttack(p);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
        } else if (p->hitPending != 0 && p->stateVar1 == 0) {
            p->stateVar1 = 1;
            CamShake_ReqA(p, 0);
            switch (p->hitKind) {
            case 0:
                StageObj_K05_Create(p, 0xF, 0);
                break;
            case 1:
                p->hitStop = 0;
                p->hitStopAdd = 0;
                Player_SetState(p, 1, 3, 0);
                break;
            case 2:
            case 3:
                p->step++;
                Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
                p->weaponSeMute = 2;
                CamShake_ReqA(p, 1);
                break;
            }
        } else {
            if (p->stateVar1 == 0 && Motion_PassedFrame(p, 0, 42.0f) != 0) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 150.0f;
                Vec_Rotate(out, v, p->rotMtx);
                v[0] = p->pos[0] + out[0];
                v[1] = 15.0f + (p->pos[1] + out[1]);
                v[2] = p->pos[2] + out[2];
                if (p->chargeLevel == 3) {
                    StageObj_K29_CreateOnPlayerDefault(p, 0xA, 1.0f);
                } else {
                    StageObj_K00_Create(p, 0, v, 1);
                }
                Vib_StartLocal(p, 2);
                CamShake_ReqA(p, 0);
            }
            if (Motion_ReachedFrame(p, 0, 48.0f) != 0 && Motion_ReachedFrame(p, 0, 102.0f) == 0) {
                GreatSword_CheckInput(p, 4);
            }
        }
        break;
    case 3:
        if (p->hitSlowTimer <= 0) {
            p->step++;
            StageObj_K05_Create(p, 0x1C, 0);
        }
        break;
    case 4:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 8, 0);
        } else if (p->hitPending != 0 && p->stateVar1 == 0) {
            p->stateVar1 = 1;
            CamShake_ReqA(p, 0);
            switch (p->hitKind) {
            case 0:
                StageObj_K05_Create(p, 0xF, 0);
                break;
            case 1:
                p->hitStop = 0;
                p->hitStopAdd = 0;
                Player_SetState(p, 1, 3, 0);
                break;
            case 2:
            case 3:
                p->step++;
                Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
                p->weaponSeMute = 2;
                CamShake_ReqA(p, 1);
                break;
            }
        } else {
            if (p->stateVar1 == 0 && Motion_PassedFrame(p, 0, 42.0f) != 0) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 150.0f;
                Vec_Rotate(out, v, p->rotMtx);
                v[0] = p->pos[0] + out[0];
                v[1] = 15.0f + (p->pos[1] + out[1]);
                v[2] = p->pos[2] + out[2];
                StageObj_K00_Create(p, 0, v, 1);
                Vib_StartLocal(p, 2);
                CamShake_ReqA(p, 0);
            }
            if (Motion_ReachedFrame(p, 0, 48.0f) != 0 && Motion_ReachedFrame(p, 0, 102.0f) == 0) {
                GreatSword_CheckInput(p, 4);
            }
        }
        break;
    case 5:
        if (p->hitSlowTimer <= 0) {
            p->step++;
            StageObj_K05_Create(p, 0x1C, 0);
        }
        break;
    case 6:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        } else if (Motion_ReachedFrame(p, 0, 48.0f) != 0 && Motion_ReachedFrame(p, 0, 102.0f) == 0) {
            GreatSword_CheckInput(p, 4);
        }
        break;
    }
}

void GreatSword_RunState(Player *p)
{
    switch (p->state) {
    case 0:
        GreatSword_State00(p, 0);
        break;
    case 1:
        GreatSword_State01(p, 0);
        break;
    case 2:
        GreatSword_State02(p, 0U);
        break;
    case 3:
        GreatSword_State03(p, 0);
        break;
    case 4:
        GreatSword_State04(p, 0);
        break;
    case 5:
        GreatSword_State00(p, 1);
        break;
    case 6:
        GreatSword_State00(p, 2);
        break;
    case 7:
        GreatSword_State00(p, 3);
        break;
    case 8:
        GreatSword_State00(p, 4);
        break;
    case 9:
        GreatSword_State01(p, 1);
        break;
    case 10:
        GreatSword_State01(p, 2);
        break;
    case 11:
        GreatSword_State01(p, 3);
        break;
    case 12:
        GreatSword_State01(p, 4);
        break;
    case 13:
        GreatSword_State02(p, 1U);
        break;
    case 14:
        GreatSword_State02(p, 2U);
        break;
    case 15:
        GreatSword_State02(p, 3U);
        break;
    case 16:
        GreatSword_State02(p, 4U);
        break;
    case 17:
        GreatSword_State02(p, 5U);
        break;
    case 18:
        GreatSword_State02(p, 6U);
        break;
    case 19:
        GreatSword_State19(p, 0U);
        break;
    case 20:
        GreatSword_State19(p, 1U);
        break;
    case 21:
        GreatSword_State19(p, 2U);
        break;
    case 22:
        GreatSword_State19(p, 3U);
        break;
    case 23:
        GreatSword_State19(p, 4U);
        break;
    case 24:
        GreatSword_State19(p, 5U);
        break;
    case 25:
        GreatSword_State19(p, 6U);
        break;
    case 26:
        GreatSword_State26(p, 0);
        break;
    case 27:
        GreatSword_State26(p, 1);
        break;
    case 28:
        GreatSword_State26(p, 2);
        
    default:
        break;
    }
}

static s32 GreatSword_StickAttackDir(Player *p)
{
    s32 dir;
    s32 ret;

    if (p->input.stickDirPressed & 0x3C) {
        dir = (p->input.prevStickAngle + 0x2AAB) & 0xFFFF;
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

//---
s32 GreatSword_AttackInput(Player *p)
{

    switch (GreatSword_StickAttackDir(p)) {
    case 0:
        break;
    case 1:
        Player_SetState(p, 1, 0, 4);
        return 1;
    case 2:
        Player_SetState(p, 1, 1, 4);
        return 1;
    case 3:
        Player_SetState(p, 1, 2, 4);
        return 1;
    case 4:
        Player_SetState(p, 1, 0x13, 4);
        return 1;
    }
    if (p->input.pressed & 1) {
        Player_SetState(p, 1, 4, 4);
        return 1;
    }
    if ((p->input.pressed & 0x40) && p->stamina >= 0x4B) {
        Player_SetState(p, 0, 0x1C, 4);
        return 1;
    }
    return 0;
}

void GreatSword_CheckInput(Player *p, s32 caller)
{
    s32 k;
    s32 dir;

    if ((Player_IsLocal(p) != 0 || Game_IsOnline() != 1) && Quest_PhaseIsOver(1) != 1 && p->itemSelect == 0) {
        if ((p->input.pressed & 0x40) && p->stamina >= 0x4B) {
            if (p->input.stickDir & 0x3C00) {
                dir = (u16) (p->input.stickAngle + 0x2AAB);
                if (dir < 0x5C72) {
                    Player_SetState(p, 0, 0x31, 4);
                } else if (dir < 0xA38E) {
                    Player_SetState(p, 0, 0x1C, 4);
                } else {
                    Player_SetState(p, 0, 0x32, 4);
                }
            } else {
                Player_SetState(p, 0, 0x1C, 4);
            }
        }
        k = GreatSword_StickAttackDir(p);
        if (k == 0 || k == caller) {
            return;
        }
        switch (k) {
        case 1:
            switch (caller) {
            case 3:
                if (p->input.stickDir & 0x800) {
                    Player_SetState(p, 1, 7, 4);
                } else {
                    Player_SetState(p, 1, 5, 4);
                }
                break;
            case 2:
                if (p->input.stickDir & 0x800) {
                    Player_SetState(p, 1, 8, 4);
                } else {
                    Player_SetState(p, 1, 6, 4);
                }
                break;
            case 4:
                if (p->input.stickDir & 0x800) {
                    Player_SetState(p, 1, 7, 4);
                } else {
                    Player_SetState(p, 1, 5, 4);
                }
                break;
            }
            break;
        case 2:
            switch (caller) {
            case 3:
                if (p->input.stickDir & 0x400) {
                    Player_SetState(p, 1, 0xB, 4);
                } else {
                    Player_SetState(p, 1, 9, 4);
                }
                break;
            case 1:
                if (p->input.stickDir & 0x400) {
                    Player_SetState(p, 1, 0xC, 4);
                } else {
                    Player_SetState(p, 1, 0xA, 4);
                }
                break;
            case 4:
                if (p->input.stickDir & 0x400) {
                    Player_SetState(p, 1, 0xB, 4);
                } else {
                    Player_SetState(p, 1, 9, 4);
                }
                break;
            }
            break;
        case 3:
            switch (caller) {
            case 2:
                if (p->input.stickDir & 0x400) {
                    Player_SetState(p, 1, 0x12, 4);
                } else {
                    Player_SetState(p, 1, 0xE, 4);
                }
                break;
            case 1:
                if (p->input.stickDir & 0x800) {
                    Player_SetState(p, 1, 0x11, 4);
                } else {
                    Player_SetState(p, 1, 0xF, 4);
                }
                break;
            case 4:
                break;
            }
            break;
        case 4:
            switch (caller) {
            case 1:
                if (p->input.stickDir & 0x800) {
                    Player_SetState(p, 1, 0x18, 4);
                } else {
                    Player_SetState(p, 1, 0x16, 4);
                }
                break;
            case 2:
                if (p->input.stickDir & 0x400) {
                    Player_SetState(p, 1, 0x19, 4);
                } else {
                    Player_SetState(p, 1, 0x15, 4);
                }
                break;
            case 3:
                break;
            }
            break;
        }
    }
}

void GreatSword_DrawAttack(Player *p)
{
    if (p->input.stickDir & 0x10) {
        Player_SetState(p, 1U, 0x14U, 4);
        return;
    }
    Player_SetState(p, 1U, 0xDU, 4);
}

static u32 Bowgun_RapidFireMask(Player *p)
{
    GunAmmoMask *e = D_45EE20;

    if (p->weaponType != 5) {
        return 0;
    }
    for (; e->id != 999; e++) {
        if (e->id == p->equip[6].id) {
            return e->mask;
        }
    }
    return 0;
}

s32 Bowgun_CanRapidFire(Player *p)
{
    u32 m = Bowgun_RapidFireMask(p);

    if (m == 0) {
        return 0;
    }
    if (m & (1 << p->ammoType)) {
        return 1;
    }
    return 0;
}

static void Bowgun_SetAimLayers(Player *p, u16 motA, u16 motB)
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

void Bowgun_State00(Player *p, s32 aim, s32 variant)
{
    s32 k;

    if (p->aimLockTimer != 0) {
        p->aimLockTimer = 5;
    }
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        if (variant == 0) {
            p->stateVar1 = 0;
        } else {
            p->stateVar1 = D_45E0E2[p->ammoType][0];
        }
        Player_SetMoveMode(p, 0, 1, 0);
        Bowgun_SetAimLayers(p, Bowgun_State00_AimLayers[aim].aimA, Bowgun_State00_AimLayers[aim].aimB);
        Player_SetMotionPair(p, Bowgun_State00_AimLayers[aim].mot, 0, 0);
        break;
    case 1:
    case 2:
        if (Motion_PassedFrame(p, 1, 8.0f) != 0 && p->step == 1) {
            p->step++;
            Vib_StartLocal(p, 0);
            CamShake_ReqB(p, 1);
            if (variant == 0) {
                Bowgun_FireShot(p, 0, 0xE, 1.0f);
            } else {
                Bowgun_FireShot(p, 0, 0xE, 0.5f);
            }
            if (p->stateVar0 <= 0) {
                p->stateVar0++;
                if (Player_HasSkill(p, 0x33) == 0 || variant == 1) {
                    p->roundsLoaded--;
                }
                if (Quest_PhaseIsOver(1) == 0) {
                    Pouch_Add(p, p->pouch[p->ammoSlot].item, -1);
                }
                if (p->ammoCount == 0) {
                    p->roundsLoaded = 0;
                }
                p->savedAmmoCount = p->ammoCount;
                p->savedClipSize = p->clipSize;
                p->savedRoundsLoaded = p->roundsLoaded;
            }
        }
        if (p->aimMode == 0 && Motion_PassedFrame(p, 0, 28.0f) != 0) {
            StageObj_Gun_CreateReload(p, 2);
        }
        if (p->motion[0].playing == 0) {
            Bowgun_SetAimLayers(p, 0x3ED, 0x3EE);
            Player_BeginStateKeepParts(p, 0, 0, 0);
        } else if (Motion_ReachedFrame(p, 0, 34.0f) != 0 && p->stateVar1 > 0) {
            p->step = 1;
            p->stateVar1--;
            k = p->stateVar1 > 0;
            if (p->stateRand & 0x8000) {
                p->stateRand = (p->stateRand << 1) | 1;
            } else {
                p->stateRand = p->stateRand << 1;
            }
            Bowgun_SetAimLayers(p, Bowgun_State00_AimLayers[k].aimA, Bowgun_State00_AimLayers[k].aimB);
            Player_SetMotionPair(p, Bowgun_State00_AimLayers[k].mot, 0, 0);
        }
        break;
    }
}

void Bowgun_State01(Player *p, s32 unused)
{

    if (p->aimLockTimer != 0) {
        p->aimLockTimer = 5;
    }
    switch (p->step) {
    case 0:
        p->step++;
        Bowgun_SetAimLayers(p, 0x586, 0x587);
        Player_SetMotionPair(p, 0x57C, 0, 0);
        p->roundsLoaded = p->clipSize;
        if (p->ammoCount < p->roundsLoaded) {
            p->roundsLoaded = p->ammoCount;
        }
        Player_SetMoveMode(p, 0, 0, 0);
        switch (Bowgun_ReloadLevel(p, p->ammoType)) {
        case 0:
            p->motionSpeed = 1.25f;
            break;
        case 1:
        default:
            p->motionSpeed = 1.0f;
            break;
        case 2:
            p->motionSpeed = 0.75f;
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginStateKeepParts(p, 0, 0, 0);
        }
        break;
    }
}

//---
void Bowgun_State06(Player *p, s32 variant)
{

    if (p->aimLockTimer != 0) {
        p->aimLockTimer = 5;
    }
    switch (p->step) {
    case 0:
        p->step++;
        p->weaponOut = 1;
        if (variant == 0) {
            Player_SetMotionPair(p, 0x58E, 0, 0);
        } else {
            Player_SetMotionPair(p, 0x58F, 0, 0);
        }
        p->roundsLoaded = p->clipSize;
        if (p->ammoCount < p->roundsLoaded) {
            p->roundsLoaded = p->ammoCount;
        }
        p->stateVar0 = Bowgun_ReloadLevel(p, p->ammoType);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Bowgun_SetAimLayers(p, 0x586, 0x587);
            Player_SetMotionPair(p, 0x57C, 0, 0x1E);
            switch (p->stateVar0) {
            case 0:
                p->motionSpeed = 1.25f;
                break;
            case 1:
            default:
                p->motionSpeed = 1.0f;
                break;
            case 2:
                p->motionSpeed = 0.75f;
                break;
            }
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginStateKeepParts(p, 0, 0, 0);
        }
        break;
    }
}

//---
void Bowgun_State02(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Bowgun_SetAimLayers(p, 0x588, 0x589);
        Player_SetMotionPair(p, 0x584, 0, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginStateKeepParts(p, 0, 0, 0);
        }
        break;
    }
}

void Bowgun_State03(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x585, 4, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        Eff00_Create(p, 0x1DU);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Bowgun_RunState(Player *p)
{
    switch (p->state) {
    case 0:
        Bowgun_State00(p, 1, 0);
        break;
    case 1:
        Bowgun_State01(p, 0);
        break;
    case 2:
        Bowgun_State02(p, 0);
        break;
    case 3:
        Bowgun_State03(p, 0);
        break;
    case 4:
        Bowgun_State00(p, 0, 0);
        break;
    case 5:
        Bowgun_State00(p, 2, 0);
        break;
    case 6:
        Bowgun_State06(p, 0);
        break;
    case 7:
        Bowgun_State06(p, 1);
        break;
    case 8:
        Bowgun_State00(p, 1, 1);
        
    default:
        break;
    }
}

s32 Bowgun_AttackInput(Player *p)
{

    if ((p->input.pressed & 0x40) && p->stamina >= 0x4B) {
        Player_SetState(p, 0, 0x1C, 4);
        return 1;
    }
    if (p->roundsLoaded == 0) {
        if (p->input.pressed & 1) {
            Player_SetState(p, 1, 2, 5);
            return 1;
        }
    } else {
        p->bowgunReady = 1;
        if (p->input.pressed & 1) {
            if (Bowgun_CanRapidFire(p) == 1) {
                Player_SetState(p, 1, 8, 5);
                return 1;
            }
            switch (Bowgun_RecoilLevel(p, p->ammoType)) {
            case 0:
                Player_SetState(p, 1, 0, 5);
                break;
            case 1:
            default:
                Player_SetState(p, 1, 4, 5);
                break;
            case 2:
                Player_SetState(p, 1, 5, 5);
                break;
            }
            return 1;
        }
    }
    if (p->roundsLoaded == 0 || (p->roundsLoaded < p->clipSize && p->roundsLoaded < p->ammoCount)) {
        if ((p->input.stickDirPressed & 0x10) && p->ammoCount > 0) {
            Player_RequestState(p, 1, 1, 5);
            return 1;
        }
    }
    if ((p->input.stickDirPressed & 0x20) && p->aimLockTimer == 0 && p->aimMode == 0) {
        Player_SetState(p, 1, 3, 4);
        return 1;
    }
    return 0;
}

void Bowgun_DrawAttack(Player *p, s32 arg1)
{
    u8 rounds;

    if (p->input.stickDir & 0x10) {
        rounds = p->roundsLoaded;
        if (rounds != 0) {
            if (((s32) rounds < (s32) p->clipSize) && ((s32) rounds < p->ammoCount)) {
                goto block_5;
            }
            goto block_13;
        }
block_5:
        if (p->ammoCount > 0) {
            if (p->weaponType == 1) {
                Player_SetState(p, 1U, 6U, 4);
                return;
            }
            if (!(arg1 & 0xFF)) {
                Player_SetState(p, 1U, 6U, 4);
                return;
            }
            Player_SetState(p, 1U, 7U, 4);
            return;
        }
        goto block_13;
    }
block_13:
    if ((p->weaponType == 1) || !(arg1 & 0xFF)) {
        Player_SetState(p, 0U, 4U, 0);
        return;
    }
    Player_SetState(p, 0U, 0x17U, 0);
}

s32 Hammer_CheckHit(Player *p)
{

    switch (p->stateVar1) {
    case 0:
        if (p->hitPending != 0) {
            CamShake_ReqA(p, 0);
            switch (p->hitKind) {
            case 0:
                p->stateVar1++;
                break;
            case 1:
                p->hitStop = 0;
                p->hitStopAdd = 0;
                Player_RequestState(p, 1, 0xA, 0);
                return 1;
            case 2:
            case 3:
                p->stateVar1++;
                Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
                p->weaponSeMute = 2;
                CamShake_ReqA(p, 1);
                break;
            }
        }
        break;
    case 1:
        if (p->hitSlowTimer <= 0) {
            p->stateVar1++;
        }
        break;
    case 2:
        break;
    }
    return 0;
}

void Hammer_State00(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        Player_SetMotionPair(p, 0x579, 4, 0);
        Eff00_Create(p, 6U);
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (Hammer_CheckHit(p) != 1) {
            if (p->motion[0].playing == 0) {
                Player_BeginState(p, 0, 6, 0);
                break;
            }
            if (Player_IsLocal(p) == 1) {
                if (Motion_InFrameRange(p, 0, 50.0f, 80.0f) != 0) {
                    Hammer_CheckInput(p, 1);
                }
                if ((Motion_ReachedFrame(p, 0, 72.0f) != 0) && (p->input.held & 0x80)) {
                    Player_SetState(p, 1U, 0xDU, 0);
                }
            }
        } else {
        }
        break;
    }
}

void Hammer_State01(Player *p, s32 variant)
{
    u8 stateVar0;

    switch (p->step) {
    case 0:
        Player_SetMoveMode(p, 0, 1, 0);
        p->timer = 0;
        p->stateVar1 = 0;
        switch (variant) {
        case 0:
            p->step++;
            Player_SetMotionPair(p, 0x57A, 4, 0);
            Player_UpdateMotionTime(p, 0);
            Player_SetMoveMode(p, 0, 1, 0);
            Eff00_Create(p, 7U);
            p->noFlinchTimer = 0x1B;
            break;
        case 1:
            stateVar0 = p->stateVar0;
            switch (stateVar0) {
            case 0:
                p->stateVar0 = stateVar0 + 1;
                Player_UpdateMotionTime(p, 0);
                Player_SetMotionPair(p, 0x581, 4, 0);
                Player_SetMoveMode(p, 0, 1, 0);
                p->weaponOut = 1;
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    Player_SetMotionPair(p, 0x57A, 0, 0x10);
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 7U);
                }
                break;
            }
        }
        break;
    case 1:
        if (Hammer_CheckHit(p) != 1) {
            if (Motion_PassedFrame(p, 0, 58.0f) != 0) {
                Vib_StartLocal(p, 2);
                CamShake_ReqA(p, 2);
            }
            if ((Player_IsLocal(p) == 1) && (Motion_InFrameRange(p, 0, 30.0f, 88.0f) != 0) && (p->input.stickDirPressed & 0x3C) && (((p->input.prevStickAngle + 0x1555) & 0xFFFF) < 0xAAAC)) {
                p->timer = 1;
            }
            if (p->motion[0].playing == 0) {
                Player_BeginState(p, 0, 6, 0);
                break;
            }
            if (p->hitSlowTimer == 0) {
                if (Motion_InFrameRange(p, 0, 80.0f, 120.0f) != 0) {
                    Hammer_CheckInput(p, 2);
                    break;
                }
                if ((Motion_ReachedFrame(p, 0, 70.0f) != 0) && (p->timer != 0)) {
                    Player_SetState(p, 1U, 2U, 4);
                }
            }
        }
        break;
    }
}

void Hammer_State02(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        Player_SetMotionPair(p, 0x584, 2, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0;
        Eff00_Create(p, 0x21U);
        p->timer = 0;
        break;
    case 1:
        if (Hammer_CheckHit(p) != 1) {
            if (p->motion[0].playing == 0) {
                if ((Player_IsLocal(p) == 1) && (p->timer != 0) && (variant == 0)) {
                    Player_SetState(p, 1U, 0x13U, 4);
                    break;
                }
                p->step++;
                Player_SetMotionPair(p, 0x57A, 4, 0x4A);
                break;
            }
            if ((Player_IsLocal(p) == 1) && (Motion_InFrameRange(p, 0, 30.0f, 60.0f) != 0) && (p->input.stickDirPressed & 0x3C) && (((p->input.prevStickAngle + 0x1555) & 0xFFFF) < 0xAAAC)) {
                p->timer = 1;
            }
            if (Motion_PassedFrame(p, 0, 52.0f) != 0) {
                Vib_StartLocal(p, 2);
                CamShake_ReqA(p, 2);
                break;
            }
        } else {
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (Motion_InFrameRange(p, 0, 80.0f, 120.0f) != 0) {
            Hammer_CheckInput(p, 2);
        }
        break;
    }
}

//---
void Hammer_State03(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->weaponOut = 1;
        Player_SetMoveMode(p, 0, 1, 0);
        p->timer = 0;
        switch (variant) {
        case 0:
        case 3:
            p->step = 2;
            if (variant == 3) {
                Player_SetMotionPair(p, 0x3EE, 6, 0xC);
            } else {
                Player_SetMotionPair(p, 0x3EE, 4, 0);
            }
            p->weaponTimer = 0;
            Effect06_SpawnAtBone(p, 6, 0, 2, 4.0f);
            p->noFlinchTimer = 0x3C;
            break;
        case 1:
            p->step = 3;
            Player_SetMotionPair(p, 0x3EF, 8, 0);
            goto charge;
        case 2:
            p->step = 1;
            Player_SetMotionPair(p, 0x582, 4, 0);
            p->weaponTimer = 0;
            p->noFlinchTimer = 0x3C;
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x3EE, 0, 0xE);
            p->weaponTimer = 0;
            Effect06_SpawnAtBone(p, 6, 0, 2, 4.0f);
            p->noFlinchTimer = 0x3C;
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x3EF, 4, 0);
            p->noFlinchTimer = 1;
        }
        break;
    case 3:
    charge:
        Player_TickWeaponTimer(p);
        if (Player_IsLocal(p) == 1) {
            p->timer++;
            if (p->timer >= 2) {
                Player_UseStamina(p, -1);
            }
        }
        Hammer_CheckChargeInput(p);
        break;
    }
}

void Hammer_State04(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x3ED, 8, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        Player_SetFlag(p, 0x02001000U);
        p->timer = 0;
        
    case 1:
        Player_TickWeaponTimer(p);
        if (Player_IsLocal(p) == 1) {
            p->timer++;
            if (p->timer >= 2) {
                Player_UseStamina(p, -1);
            }
        }
        Hammer_CheckChargeInput(p);
        break;
    }
}

void Hammer_State05(Player *p, s32 unused)
{

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x57B, 6, 0);
        p->stateVar0 = p->chargeLevel + 2;
        if (p->stateVar0 >= 5) {
            p->stateVar0 = 5;
        }
        p->stateVar1 = 0;
        p->timer = 0;
        p->targetYaw = p->rot[1];
        Eff00_Create(p, 0xC);
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0x3C;
        break;
    case 1:
        p->noFlinchTimer = 2;
        if (Motion_PassedFrame(p, 0, 44.0f) != 0) {
            p->timer++;
            if (--p->stateVar0 <= 0) {
                p->step++;
                Player_SetMotionPair(p, 0x57C, 0, 0);
                p->stateRand += 3;
                Eff00_Create(p, 0xF);
                break;
            }
            switch (p->timer) {
            case 0:
                break;
            case 1:
            case 2:
                if (p->stateVar1 != 0) {
                    if (p->input.stickDir & 0x800) {
                        p->rot[1] += 0x2000;
                        p->targetYaw = p->rot[1];
                    }
                    if (p->input.stickDir & 0x400) {
                        p->rot[1] -= 0x2000;
                        p->targetYaw = p->rot[1];
                    }
                    Player_RequestState(p, 1, 0x11, 4);
                }
                break;
            case 3:
            case 4:
            case 5:
                if (p->stateVar1 != 0) {
                    if (p->input.stickDir & 0x800) {
                        p->rot[1] += 0x2000;
                        p->targetYaw = p->rot[1];
                    }
                    if (p->input.stickDir & 0x400) {
                        p->rot[1] -= 0x2000;
                        p->targetYaw = p->rot[1];
                    }
                    Player_RequestState(p, 1, 9, 4);
                }
                break;
            }
        } else {
            if (Player_IsLocal(p) == 1) {
                if (p->input.stickDir & 0x800) {
                    p->targetYaw += 0xC0;
                } else if (p->input.stickDir & 0x400) {
                    p->targetYaw -= 0xC0;
                }
                if (p->input.stickDirPressed & 0x20) {
                    p->stateVar1++;
                }
            }
            if (Motion_PassedFrame(p, 0, 26.0f) != 0) {
                Eff00_Create(p, 0xD);
            }
            if (Motion_PassedFrame(p, 0, 52.0f) != 0) {
                Eff00_Create(p, 0xE);
            }
        }
        break;
    case 2:
        p->noFlinchTimer = 2;
        if (Motion_PassedFrame(p, 0, 38.0f) != 0) {
            Vib_StartLocal(p, 2);
            CamShake_ReqA(p, 1);
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Hammer_State06(Player *p, s32 variant)
{
    f32 in[4];
    f32 out[4];

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        if (variant == 4) {
            Player_SetMotionPair(p, 0x57D, 2, 0);
        } else {
            Player_SetMotionPair(p, 0x57D, 4, 0);
        }
        Player_SetMoveMode(p, 0, 1, 0);
        Eff00_Create(p, 8);
        p->noFlinchTimer = 0x3C;
        break;
    case 1:
        if ((Motion_ReachedFrame(p, 0, 30.0f) == 0) && (variant == 2 || variant == 3)) {
            in[0] = 0.0f;
            in[1] = 0.0f;
            in[2] = 18.0f * Math_Cos((6.2831855f * (90.0f * (p->motion[0].frame / 24.0f))) / 360.0f);
            Vec_Rotate(out, in, p->rotMtx);
            p->pos[0] += out[0];
            p->pos[2] += out[2];
        }
        if (variant == 0 || variant == 3) {
            if (Motion_PassedFrame(p, 0, 60.0f) != 0) {
                Eff00_Create(p, 9);
            }
            if (p->motion[0].playing == 0) {
                Player_BeginState(p, 0, 4, 0);
            } else if (Player_IsLocal(p) == 1) {
                if ((Motion_ReachedFrame(p, 0, 80.0f) != 0) && (p->input.held & 0x80)) {
                    Player_SetState(p, 1, 0xE, 0);
                } else if (Motion_ReachedFrame(p, 0, 88.0f) != 0) {
                    Hammer_CheckInput(p, 0);
                }
            }
        } else if ((Hammer_CheckHit(p) != 1) && (Motion_ReachedFrame(p, 0, 30.0f) != 0)) {
            p->step++;
            Player_SetMotionPair(p, 0x57F, 0, 0);
        }
        break;
    case 2:
        if (Hammer_CheckHit(p) != 1) {
            if (p->motion[0].playing == 0) {
                Player_BeginState(p, 0, 4, 0);
            } else if (Player_IsLocal(p) == 1) {
                if ((Motion_ReachedFrame(p, 0, 46.0f) != 0) && (p->input.held & 0x80)) {
                    Player_SetState(p, 1, 0xD, 0);
                } else if (Motion_InFrameRange(p, 0, 32.0f, 46.0f) != 0) {
                    Hammer_CheckInput(p, 0);
                }
            }
        }
        break;
    }
}

void Hammer_State07(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x57E, 2, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        Eff00_Create(p, 0xAU);
        p->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (Motion_PassedFrame(p, 0, 74.0f) != 0) {
            Eff00_Create(p, 0xBU);
        }
        if (Motion_PassedFrame(p, 0, 80.0f) != 0) {
            Vib_StartLocal(p, 2);
            CamShake_ReqA(p, 2);
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (Motion_InFrameRange(p, 0, 84.0f, 170.0f) != 0) {
            Hammer_CheckInput(p, 0);
        }
        break;
    }
}

void Hammer_State08(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 1, 0);
        Player_UpdateMotionTime(p, 0);
        Player_SetMotionPair(p, 0x3F2, 2, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        p->weaponOut = 1;
        p->unk417 = 1;
        p->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x57D, 2, 0x2E);
            Player_UpdateMotionTime(p, 0);
            p->unk417 = 1;
        }
        break;
    case 2:
        if (Motion_PassedFrame(p, 0, 56.0f) != 0) {
            Eff00_Create(p, 0x22U);
            p->step++;
        }
        
    case 3:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (Player_IsLocal(p) == 1) {
            if ((Motion_ReachedFrame(p, 0, 80.0f) != 0) && (p->input.held & 0x80)) {
                Player_SetState(p, 1U, 0xEU, 0);
                break;
            }
            if (Motion_ReachedFrame(p, 0, 88.0f) != 0) {
                Hammer_CheckInput(p, 0);
            }
        }
        break;
    }
}

void Hammer_State09(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0x1C;
        switch (variant) {
        case 0:
            p->step = 2;
            Player_SetMotionPair(p, 0x583, 0, 0);
            Eff00_Create(p, 0x20U);
            break;
        case 1:
            Player_SetMotionPair(p, 0x585, 0, 0);
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x583, 0, 0x20);
            Eff00_Create(p, 0x20U);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (Motion_PassedFrame(p, 0, 40.0f) != 0) {
            Vib_StartLocal(p, 2);
            CamShake_ReqA(p, 1);
        }
        if (Motion_InFrameRange(p, 0, 102.0f, 132.0f) != 0) {
            Hammer_CheckInput(p, 0);
        }
        break;
    }
}

void Hammer_State10(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x580, 0, 0);
        break;
    case 1:
        if (p->stateVar0 != 0) {
            if (Motion_ReachedFrame(p, 0, 32.0f) != 0) {
                Player_SetState(p, 1U, 0x12U, 0xC);
            }
        } else if ((Motion_InFrameRange(p, 0, 2.0f, 30.0f) != 0) && (p->stateVar0 == 0) && (p->itemSelect == 0) && (p->input.stickDirPressed & 0x3C)) {
            p->stateVar0 = 1;
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 0xA, 0);
        }
        break;
    }
}

void Hammer_RunState(Player *p)
{
    switch (p->state) {
    case 0:
        Hammer_State00(p, 0);
        break;
    case 1:
        Hammer_State01(p, 0);
        break;
    case 2:
        Hammer_State02(p, 0);
        break;
    case 3:
        Hammer_State03(p, 0);
        break;
    case 4:
        Hammer_State04(p, 0);
        break;
    case 5:
        Hammer_State05(p, 0);
        break;
    case 6:
        Hammer_State06(p, 0);
        break;
    case 7:
        Hammer_State07(p, 0);
        break;
    case 8:
        Hammer_State08(p, 0);
        break;
    case 9:
        Hammer_State09(p, 0);
        break;
    case 10:
        Hammer_State10(p, 0);
        break;
    case 11:
        Hammer_State01(p, 1);
        break;
    case 12:
        Hammer_State03(p, 1);
        break;
    case 13:
        Hammer_State03(p, 2);
        break;
    case 14:
        Hammer_State03(p, 3);
        break;
    case 15:
        Hammer_State06(p, 1);
        break;
    case 16:
        Hammer_State06(p, 2);
        break;
    case 17:
        Hammer_State06(p, 3);
        break;
    case 18:
        Hammer_State06(p, 4);
        break;
    case 19:
        Hammer_State09(p, 1);
        
    default:
        break;
    }
}

s32 Hammer_AttackInput(Player *p)
{
    if (p->input.stickDirPressed & 0x3C) {
        if (((p->input.prevStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
            Player_SetState(p, 1U, 1U, 4);
            return 1;
        }
        Player_SetState(p, 1U, 0U, 4);
        return 1;
    }
    if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B)) {
        Player_SetState(p, 0U, 0x1CU, 4);
        return 1;
    }
    return 0;
}

void Hammer_CheckInput(Player *p, s32 caller)
{
    s32 dir;

    if (((Player_IsLocal(p) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1) && (p->itemSelect == 0)) {
        if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B)) {
            if (p->input.stickDir & 0x3C00) {
                dir = (p->input.stickAngle + 0x4000) & 0xFFFF;
                if (dir < 0x5C72) {
                    Player_SetState(p, 0U, 0x31U, 4);
                } else if (dir < 0xA38E) {
                    Player_SetState(p, 0U, 0x1CU, 4);
                } else {
                    Player_SetState(p, 0U, 0x32U, 4);
                }
            } else {
                Player_SetState(p, 0U, 0x1CU, 4);
            }
        }
        if ((p->input.stickDirPressed & 0x3C) && (caller == 1) && (((p->input.prevStickAngle + 0x1555) & 0xFFFF) < 0xAAAC)) {
            Player_SetState(p, 1U, 0xBU, 4);
        }
    }
}

u8 Hammer_ChargeLevel(Player *p)
{
    return p->weaponTimer / 30;
}

void Hammer_ReleaseCharge(Player *p)
{
    s32 level;

    level = Hammer_ChargeLevel(p) & 0xFF;
    switch (level) {
    case 0:
        if (Player_TestFlag(p, 0x1000U) != 0) {
            Player_RequestState(p, 1U, 0x10U, 4);
            break;
        }
        Player_RequestState(p, 1U, 0xFU, 4);
        break;
    case 1:
        if (Player_TestFlag(p, 0x1000U) != 0) {
            Player_RequestState(p, 1U, 0x11U, 4);
            break;
        }
        Player_RequestState(p, 1U, 6U, 4);
        break;
    default:
    case 2:
        if (Player_TestFlag(p, 0x1000U) != 0) {
            p->chargeLevel = Hammer_ChargeLevel(p);
            Player_RequestState(p, 1U, 5U, 4);
            break;
        }
        Player_RequestState(p, 1U, 7U, 4);
        break;
    }
}

void Hammer_CheckChargeInput(Player *p)
{

    if (Player_IsLocal(p) != 0) {
        if ((p->input.pressed & 0x40) && p->stamina >= 0x4B) {
            Player_RequestState(p, 0, 0x1C, 4);
            return;
        }
        if ((p->input.held & 0x80) && p->stamina > 0) {
            if (Player_StickMoveLevel(p, 0) != 0) {
                if (Player_IsState(p, 1, 4) == 0) {
                    Player_FaceStick(p);
                    Player_SetState(p, 1, 4, 0);
                } else {
                    Player_FaceStick(p);
                }
            } else if (p->stateGroup == 1) {
                switch (p->state) {
                case 3:
                case 0xC:
                case 0xD:
                case 0xE:
                    break;
                default:
                    Player_SetState(p, 1, 0xC, 0);
                    break;
                }
            } else {
                Player_SetState(p, 1, 0xC, 0);
            }
            if ((p->input.stickDirPressed & 0x20) && p->itemSelect == 0) {
                Hammer_ReleaseCharge(p);
            }
        } else {
            Hammer_ReleaseCharge(p);
        }
    }
}

void Hammer_DrawAttack(Player *p)
{
    Player_SetState(p, 0x1, 0x8, 0x4);
}

void Hammer_EvadeCharge(Player *p)
{
    Player_SetState(p, 0x1, 0x3, 0x4);
}

void Hammer_Draw(Player *p)
{
    Player_SetState(p, 0x1, 0x3, 0x4);
}

s32 Lance_CheckHit(Player *p)
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

void Lance_State00(Player *p, s32 variant)
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
            Eff00_Create(p, 0x10);
            break;
        case 1:
            p->step++;
            Player_SetMotionPair(p, 0x3F2, 2, 0);
            p->weaponOut = 1;
            Eff00_Create(p, 0x14);
            break;
        case 2:
        case 3:
            p->step = 2;
            Player_SetMotionPair(p, 0x57D, 6, 0xA);
            if (variant == 3) {
                Eff00_Create(p, 0x24);
                break;
            }
            Eff00_Create(p, 0x10);
            break;
        case 4:
            p->step = 2;
            Player_SetMotionPair(p, 0x57D, 4, 0xA);
            Eff00_Create(p, 0x10);
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
        if (Lance_CheckHit(p) != 1 && Player_IsLocal(p) == 1 && Motion_InFrameRange(p, 0, 36.0f, 62.0f) != 0) {
            Lance_CheckInput(p, 0);
            if ((p->input.stickDirPressed & 0x3C) && variant != 3) {
                if (((p->input.prevStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
                    if (variant == 2) {
                        Player_SetState(p, 1, 0xA, 4);
                        break;
                    }
                    Player_SetState(p, 1, 9, 4);
                    break;
                }
                if (variant == 2) {
                    Player_SetState(p, 1, 0xD, 4);
                    break;
                }
                Player_SetState(p, 1, 0xC, 4);
            }
        }
        break;
    }
}

void Lance_State01(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        switch (variant) {
        default:
            Player_SetMotionPair(p, 0x57E, 4, 0);
            Eff00_Create(p, 0x1C);
            break;
        case 1:
        case 2:
            Player_SetMotionPair(p, 0x57E, -6, 0xA);
            if (variant == 2) {
                Eff00_Create(p, 0x23);
                break;
            }
            Eff00_Create(p, 0x1C);
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (Lance_CheckHit(p) != 1 && Player_IsLocal(p) == 1 && Motion_InFrameRange(p, 0, 36.0f, 62.0f) != 0) {
            Lance_CheckInput(p, 0);
            if ((p->input.stickDirPressed & 0x3C) && variant != 2) {
                if (((p->input.prevStickAngle + 0x1555) & 0xFFFF) < 0xAAAC) {
                    if (variant == 1) {
                        Player_SetState(p, 1, 0xA, 4);
                        break;
                    }
                    Player_SetState(p, 1, 9, 4);
                    break;
                }
                if (variant == 1) {
                    Player_SetState(p, 1, 0xD, 4);
                    break;
                }
                Player_SetState(p, 1, 0xC, 4);
            }
        }
        break;
    }
}

void Lance_State02(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x588, 2, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        Eff00_Create(p, 0x12U);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 2U, 0xCU, 0);
            break;
        }
        if ((Lance_CheckHit(p) != 1) && (Motion_ReachedFrame(p, 0, 18.0f) != 0) && (Motion_ReachedFrame(p, 0, 28.0f) == 0)) {
            Lance_CheckInput(p, 0);
        }
        break;
    }
}

void Lance_State03(Player *p, s32 variant)
{
    u8 level;

    p->noFlinchTimer = 2;
    switch (p->step) {
    case 0:
        if (variant == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x579, 4, 0);
        } else {
            p->step = 2;
            Player_SetMotionPair(p, 0x57A, 0, 6);
            Eff00_Create(p, 0x11);
        }
        p->hitPending = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x57A, 0, 4);
            Eff00_Create(p, 0x11);
        }
        break;
    case 2:
        if (Player_IsLocal(p) == 1) {
            if (p->input.stickDir & 0x800) {
                p->targetYaw += 0x80;
            }
            if (p->input.stickDir & 0x400) {
                p->targetYaw -= 0x80;
            }
        }
        Player_UseStamina(p, -2);
        if (p->stamina <= 0) {
            Player_SetState(p, 1, 4, 4);
            break;
        }
        if (Player_IsLocal(p) == 1) {
            if ((p->input.stickDirPressed & 0x10) || (p->input.pressed & 1)) {
                Player_SetState(p, 1, 4, 4);
            } else if (p->input.stickDirPressed & 0x20) {
                Player_SetState(p, 1, 6, 4);
            }
        }
        if (p->hitPending != 0) {
            if (Player_HasSkill(p, 0x24) == 0 && Player_HasSkill(p, 0x25) == 0) {
                if (p->hitPending & 0x80) {
                    level = 0;
                } else {
                    level = p->sharpnessLevel;
                }
                switch (level) {
                case 0:
                    p->hitStop = 0;
                    p->hitStopAdd = 0;
                    Player_SetState(p, 2, 0, 0);
                    break;
                case 2:
                    p->hitStop = 0;
                    p->hitStopAdd = 0;
                    Player_RequestState(p, 1, 5, 0);
                    break;
                case 1:
                    p->hitStop = 0;
                    p->hitStopAdd = 0;
                    Player_RequestState(p, 1, 0xF, 0);
                    break;
                }
            }
        } else if (p->wallHitDirs & 0xE0000007) {
            Player_RequestState(p, 1, 0x10, 0);
        }
        break;
    }
}

void Lance_State04(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        p->noFlinchTimer = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(p, 0x57B, 2, 0);
            break;
        }
        Player_SetMotionPair(p, 0x583, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void Lance_State05(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->hitStop = 0;
        p->hitStopAdd = 0;
        Player_SetMotionPair(p, 0x57C, 0, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0;
        break;
    case 1:
        Player_UseStamina(p, -2);
        if (p->motion[0].playing == 0) {
            if ((variant == 0) && (p->stateVar0 == 0)) {
                Player_RequestState(p, 1U, 0xEU, 0);
                break;
            }
            Player_RequestState(p, 1U, 4U, 0);
            break;
        }
        if ((p->input.stickDirPressed & 0x10) || (p->input.pressed & 1)) {
            p->stateVar0 = 1;
        }
        break;
    }
}

void Lance_State06(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x584, 2, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0x3C;
        Eff00_Create(p, 0x1FU);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->noFlinchTimer = 0;
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (Motion_InFrameRange(p, 0, 72.0f, 102.0f) != 0) {
            Lance_CheckInput(p, 0);
        }
        break;
    }
}

void Lance_State07(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x589, 0, 0);
        CamShake_ReqA(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->stateVar0 != 0) {
            if (Motion_ReachedFrame(p, 0, 42.0f) != 0) {
                Player_SetState(p, 1U, 0xBU, 0xC);
            }
        } else if ((Motion_InFrameRange(p, 0, 10.0f, 40.0f) != 0) && (p->stateVar0 == 0) && (p->itemSelect == 0) && (p->input.stickDirPressed & 0x3C)) {
            p->stateVar0 = 1;
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void Lance_RunState(Player *p)
{
    switch (p->state) {
    case 0:
        Lance_State00(p, 0);
        break;
    case 1:
        Lance_State01(p, 0);
        break;
    case 2:
        Lance_State02(p, 0);
        break;
    case 3:
        Lance_State03(p, 0);
        break;
    case 4:
        Lance_State04(p, 0);
        break;
    case 5:
        Lance_State05(p, 0);
        break;
    case 6:
        Lance_State06(p, 0);
        break;
    case 7:
        Lance_State07(p, 0);
        break;
    case 8:
        Lance_State00(p, 1);
        break;
    case 9:
        Lance_State00(p, 2);
        break;
    case 10:
        Lance_State00(p, 3);
        break;
    case 11:
        Lance_State00(p, 4);
        break;
    case 12:
        Lance_State01(p, 1);
        break;
    case 13:
        Lance_State01(p, 2);
        break;
    case 14:
        Lance_State03(p, 1);
        break;
    case 15:
        Lance_State04(p, 1);
        break;
    case 16:
        Lance_State05(p, 1);
        
    default:
        break;
    }
}

s32 Lance_AttackInput(Player *p)
{

    if ((p->input.pressed & 0x40) && p->stamina >= 0x4B) {
        Player_SetState(p, 0, 0x6D, 4);
    }
    if (p->input.pressed & 1) {
        Player_SetState(p, 1, 3, 0xC);
    }
    if (p->input.stickDirPressed & 0x3C) {
        if ((u16)(p->input.prevStickAngle + 0x1555) < 0xAAAC) {
            Player_SetState(p, 1, 0, 0xC);
        } else {
            Player_SetState(p, 1, 1, 0xC);
        }
    }
    return 0;
}

void Lance_CheckInput(Player *p, s32 variant)
{
    s32 dir;

    if ((Player_IsLocal(p) != 0 || Game_IsOnline() != 1) && Quest_PhaseIsOver(1) != 1 && p->itemSelect == 0) {
        if ((p->input.pressed & 0x40) && p->stamina >= 0x4B) {
            if (p->input.stickDir & 0x3C00) {
                dir = (u16) (p->input.stickAngle + 0x2000);
                if (dir < 0x6000) {
                    Player_SetState(p, 0, 0x6C, 4);
                } else if (dir >= 0x6000 && dir < 0xC001) {
                    Player_SetState(p, 0, 0x6B, 4);
                } else {
                    Player_SetState(p, 0, 0x6E, 4);
                }
            } else {
                Player_SetState(p, 0, 0x6D, 4);
            }
        }
        if (p->input.pressed & 1) {
            Player_SetState(p, 1, 3, 0xC);
        }
    }
}

void Lance_GatherAttack(Player *p, s32 arg1)
{
    if (arg1 == 0) {
        Player_SetState(p, 1U, 3U, 0xC);
        return;
    }
    Player_SetState(p, 1U, 2U, 0xC);
}

void Lance_DrawAttack(Player *p)
{
    Player_SetState(p, 0x1, 0x8, 0x4);
}

//---
s32 SwordShield_CheckHit(Player *p)
{

    if (p->hitPending != 0 && p->stateVar1 == 0) {
        p->stateVar1 = 1;
        switch (p->hitKind) {
        case 0:
            break;
        case 1:
            p->hitStopAdd = p->hitStop = 0;
            Player_SetState(p, 1, 5, 0);
            return 1;
        case 2:
        case 3:
            Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
            p->weaponSeMute = 2;
            break;
        }
    }
    return 0;
}

void SwordShield_State00(Player *p, s32 variant)
{
    f32 from;
    f32 to;
    s32 dir;

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        switch (variant) {
        case 0:
            Player_SetMotionPair(p, 0x579, 4, 0);
            Eff00_Create(p, 0x15);
            break;
        case 1:
            Player_SetMotionPair(p, 0x57A, 2, 0);
            Eff00_Create(p, 0x19);
            break;
        case 2:
            Player_SetMotionPair(p, 0x57B, 2, 0);
            Eff00_Create(p, 0x1A);
            break;
        case 3:
            Player_SetMotionPair(p, 0x579, 2, 0xA);
            Eff00_Create(p, 0x15);
            StageObj_K05_Create(p, 8, 1);
            break;
        }
        break;
    case 1:
        if (p->stateVar0 == 0) {
            from = 10.0f;
            to = 50.0f;
            if (variant == 2) {
                from = 30.0f;
                to = 70.0f;
            }
            if (Motion_ReachedFrame(p, 0, from) != 0 && Motion_ReachedFrame(p, 0, to) == 0 && (p->input.stickDirPressed & 0x3C)) {
                dir = (u16) (p->input.prevStickAngle + 0x2AAB);
                if (dir < 0x4001) {
                    p->stateVar0 = 2;
                } else if (dir < 0x9556) {
                    p->stateVar0 = 1;
                } else if (dir < 0xD556) {
                    p->stateVar0 = 2;
                }
            }
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (SwordShield_CheckHit(p) == 1) {
            break;
        }
        switch (variant) {
        case 0:
        case 3:
        case 4:
            if (Motion_ReachedFrame(p, 0, 24.0f) != 0) {
                if (p->stateVar0 != 0) {
                    if (p->stateVar0 == 1) {
                        Player_SetState(p, 1, 6, 4);
                    } else {
                        Player_SetState(p, 1, 2, 4);
                    }
                } else if (Motion_ReachedFrame(p, 0, 50.0f) == 0) {
                    SwordShield_CheckInput(p, 0);
                }
            }
            break;
        case 1:
            if (Motion_ReachedFrame(p, 0, 20.0f) != 0) {
                if (p->stateVar0 != 0) {
                    if (p->stateVar0 == 1) {
                        Player_SetState(p, 1, 7, 4);
                    } else {
                        Player_SetState(p, 1, 2, 4);
                    }
                } else if (Motion_ReachedFrame(p, 0, 50.0f) == 0) {
                    SwordShield_CheckInput(p, 0);
                }
            }
            break;
        case 2:
            if (Motion_ReachedFrame(p, 0, 64.0f) != 0) {
                if (p->stateVar0 == 2) {
                    Player_SetState(p, 1, 2, 4);
                } else if (Motion_ReachedFrame(p, 0, 92.0f) == 0) {
                    SwordShield_CheckInput(p, 0);
                }
            }
            break;
        }
        if (Motion_PassedFrame(p, 0, 52.0f) != 0 && variant == 2) {
            Eff00_Create(p, 0x1B);
        }
        break;
    }
}

void SwordShield_State01(Player *p, s32 variant)
{
    s32 dir;

    switch (p->step) {
    case 0:
        Player_SetMoveMode(p, 0, 1, 0);
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        switch (variant) {
        case 0:
            p->step = 2;
            Player_SetMotionPair(p, 0x57C, 2, 0);
            Eff00_Create(p, 0x16U);
            p->noFlinchTimer = 0x64;
            break;
        case 1:
            p->step++;
            p->weaponOut = 1;
            Player_SetMotionPair(p, 0x3F2, 2, 0);
            p->noFlinchTimer = 0x64;
            break;
        case 2:
            p->step = 2;
            Player_SetMotionPair(p, 0x57C, 4, 0);
            Eff00_Create(p, 0x16U);
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x57C, 0, 8);
            Eff00_Create(p, 0x16U);
        }
        break;
    case 2:
        if ((p->stateVar0 == 0) && (Motion_ReachedFrame(p, 0, 20.0f) != 0) && (Motion_ReachedFrame(p, 0, 42.0f) == 0) && (p->input.stickDirPressed & 0x3C)) {
            dir = (p->input.prevStickAngle + 0x2AAB) & 0xFFFF;
            if ((dir > 16384) && (dir < 0x9556)) {
                p->stateVar0 = 1;
            }
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if ((SwordShield_CheckHit(p) != 1) && (Motion_ReachedFrame(p, 0, 34.0f) != 0)) {
            if (p->stateVar0 != 0) {
                Player_SetState(p, 1U, 4U, 4);
                break;
            }
            if (Motion_ReachedFrame(p, 0, 42.0f) == 0) {
                SwordShield_CheckInput(p, 0);
            }
        }
        break;
    }
}

void SwordShield_State02(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        Player_SetMotionPair(p, 0x57D, 4, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        Eff00_Create(p, 0x17U);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if ((SwordShield_CheckHit(p) != 1) && (Motion_ReachedFrame(p, 0, 20.0f) != 0) && (Motion_ReachedFrame(p, 0, 40.0f) != 0)) {
            SwordShield_CheckInput(p, 0);
        }
        break;
    }
}

void SwordShield_State03(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        Player_SetMotionPair(p, 0x588, 4, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        Eff00_Create(p, 0x18U);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_RequestState(p, 2U, 0xCU, 0);
            break;
        }
        SwordShield_CheckHit(p);
        break;
    }
}

void SwordShield_State04(Player *p, s32 variant)
{
    s32 dir;
    u8 stateVar0;
    u8 temp_v1;

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 1, 0);
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        switch (variant) {
        case 0:
            p->step = 2;
            Player_SetMotionPair(p, 0x57E, 2, 0);
            Eff00_Create(p, 0x1EU);
            break;
        case 1:
            p->step = 2;
            Player_SetMotionPair(p, 0x57E, 2, 8);
            Eff00_Create(p, 0x1EU);
            break;
        case 2:
            Player_SetMotionPair(p, 0x589, 2, 0);
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            p->stateVar0 = 0;
            Player_SetMotionPair(p, 0x57E, 0, 0x10);
            break;
        }
        if (Motion_ReachedFrame(p, 0, 18.0f) != 0) {
            temp_v1 = p->stateVar0;
            if (temp_v1 == 0) {
                p->stateVar0 = temp_v1 + 1;
                Eff00_Create(p, 0x64U);
                break;
            }
        }
        break;
    case 2:
        if ((p->stateVar0 == 0) && (Motion_ReachedFrame(p, 0, 26.0f) != 0) && (Motion_ReachedFrame(p, 0, 38.0f) == 0) && (p->input.stickDirPressed & 0x3C)) {
            dir = (u16) (p->input.prevStickAngle + 0x2AAB);
            if (dir <= 0x4000) {
                p->stateVar0 = 2;
            } else if (dir < 0x9556) {
                p->stateVar0 = 1;
            } else if (dir < 0xD556) {
                p->stateVar0 = 2;
            }
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if ((SwordShield_CheckHit(p) != 1) && (Motion_ReachedFrame(p, 0, 26.0f) != 0)) {
            stateVar0 = p->stateVar0;
            if (stateVar0 == 1) {
                Player_SetState(p, 1U, 8U, 4);
                break;
            }
            if (stateVar0 == 2) {
                Player_SetState(p, 1U, 2U, 4);
                break;
            }
            if (Motion_ReachedFrame(p, 0, 38.0f) == 0) {
                SwordShield_CheckInput(p, 0);
            }
        }
        break;
    }
}

void SwordShield_State05(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x580, 4, 0);
        CamShake_ReqA(p, 0);
        Player_SetMoveMode(p, 0, 0, 0);
        break;
    case 1:
        if (p->stateVar0 != 0) {
            if (Motion_ReachedFrame(p, 0, 32.0f) != 0) {
                Player_RequestState(p, 1U, 0xAU, 0xC);
            }
        } else if ((Motion_InFrameRange(p, 0, 14.0f, 40.0f) != 0) && (p->stateVar0 == 0) && (p->itemSelect == 0) && (p->input.stickDirPressed & 0x3C)) {
            p->stateVar0 = 1;
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void SwordShield_RunState(Player *p)
{
    switch (p->state) {
    case 0:
        SwordShield_State00(p, 0);
        break;
    case 1:
        SwordShield_State01(p, 0);
        break;
    case 2:
        SwordShield_State02(p, 0);
        break;
    case 3:
        SwordShield_State03(p, 0);
        break;
    case 4:
        SwordShield_State04(p, 0);
        break;
    case 5:
        SwordShield_State05(p, 0);
        break;
    case 6:
        SwordShield_State00(p, 1);
        break;
    case 7:
        SwordShield_State00(p, 2);
        break;
    case 8:
        SwordShield_State00(p, 3);
        break;
    case 9:
        SwordShield_State01(p, 1);
        break;
    case 10:
        SwordShield_State01(p, 2);
        break;
    case 11:
        SwordShield_State04(p, 1);
        break;
    case 12:
        SwordShield_State04(p, 2);
        
    default:
        break;
    }
}

s32 SwordShield_AttackInput(Player *p)
{
    s32 dir;
    u16 btn;

    btn = p->input.pressed;
    if (btn & 1) {
        Player_SetState(p, 1U, 0xCU, 4);
        return 1;
    }
    if (p->input.stickDirPressed & 0x3C) {
        dir = (p->input.prevStickAngle + 0x2AAB) & 0xFFFF;
        if (dir < 0x4001) {
            Player_SetState(p, 1U, 2U, 4);
            return 1;
        }
        if (dir < 0x9556) {
            Player_SetState(p, 1U, 0U, 4);
            return 1;
        }
        if (dir < 0xD556) {
            Player_SetState(p, 1U, 2U, 4);
            return 1;
        }
        Player_SetState(p, 1U, 1U, 4);
        return 1;
    }
    if ((btn & 0x40) && (p->stamina >= 0x4B)) {
        Player_SetState(p, 0U, 0x1CU, 4);
        return 1;
    }
    return 0;
}

void SwordShield_CheckInput(Player *p, s32 unused)
{
    s32 dir;

    if (((Player_IsLocal(p) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1) && (p->itemSelect == 0) && (p->input.pressed & 0x40) && (p->stamina >= 0x4B)) {
        if (p->input.stickDir & 0x3C00) {
            dir = (p->input.stickAngle + 0x4000) & 0xFFFF;
            if (dir < 0x5C72) {
                Player_SetState(p, 0U, 0x31U, 4);
                return;
            }
            if (dir < 0xA38E) {
                Player_SetState(p, 0U, 0x1CU, 4);
                return;
            }
            Player_SetState(p, 0U, 0x32U, 4);
            return;
        }
        Player_SetState(p, 0U, 0x1CU, 4);
    }
}

void SwordShield_DrawAttack(Player *p)
{
    Player_SetState(p, 0x1, 0x9, 0x4);
}

void SwordShield_GatherAttack(Player *p)
{
    Player_SetState(p, 0x1, 0x3, 0x4);
}

void SwordShield_EvadeAttack(Player *p)
{
    Player_SetState(p, 0x1, 0xB, 0x4);
}

s32 DualSwords_CheckHit(Player *p)
{

    if (p->hitPending != 0 && p->stateVar1 == 0) {
        p->stateVar1 = 1;
        switch (p->hitKind) {
        case 0:
            break;
        case 1:
            p->hitStopAdd = p->hitStop = 0;
            Player_SetState(p, 1, 8, 0);
            return 1;
        case 2:
        case 3:
            Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
            p->weaponSeMute = 2;
            break;
        }
    }
    return 0;
}

void DualSwords_State00(Player *p, s32 variant)
{
    f32 f0;
    f32 f1;
    u16 a;

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        switch (variant) {
        case 0:
            Player_SetMotionPair(p, 0x579, 2, 0);
            Eff00_Create(p, 0x25);
            break;
        case 1:
            Player_SetMotionPair(p, 0x57A, 2, 0);
            Eff00_Create(p, 0x26);
            break;
        case 2:
            Player_SetMotionPair(p, 0x57B, 2, 0);
            Eff00_Create(p, 0x28);
            break;
        case 3:
            Player_SetMotionPair(p, 0x579, 2, 10);
            Eff00_Create(p, 0x25);
            StageObj_K05_Create(p, 8, 1);
            break;
        }
        break;
    case 1:
        if (p->stateVar0 == 0) {
            f0 = 10.0f;
            f1 = 50.0f;
            if (variant == 2) {
                f0 = 60.0f;
                f1 = 120.0f;
            }
            if (Motion_ReachedFrame(p, 0, f0) != 0 && Motion_ReachedFrame(p, 0, f1) == 0
                && (p->input.stickDirPressed & 0x3C)) {
                a = p->input.prevStickAngle + 0x2AAB;
                if (a <= 0x4000) {
                    p->stateVar0 = 2;
                } else if (a <= 0x9555) {
                    p->stateVar0 = 1;
                } else if (a <= 0xD555) {
                    p->stateVar0 = 3;
                } else {
                    p->stateVar0 = 4;
                }
            }
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (DualSwords_CheckHit(p) == 1) {
            break;
        }
        switch (variant) {
        case 0:
        case 3:
            if (Motion_ReachedFrame(p, 0, 22.0f) != 0) {
                switch (p->stateVar0) {
                case 1:
                    Player_SetState(p, 1, 9, 4);
                    break;
                case 2:
                    DualSwords_ComboInput(p, 0, 1);
                    break;
                case 3:
                    DualSwords_ComboInput(p, 1, 1);
                    break;
                case 4:
                    DualSwords_ComboInput(p, 3, 1);
                    break;
                default:
                    if (Motion_ReachedFrame(p, 0, 50.0f) == 0) {
                        DualSwords_CheckInput(p, 0);
                    }
                    break;
                }
            }
            break;
        case 1:
            if (Motion_ReachedFrame(p, 0, 28.0f) != 0) {
                switch (p->stateVar0) {
                case 1:
                    Player_SetState(p, 1, 10, 4);
                    break;
                case 2:
                    DualSwords_ComboInput(p, 0, 1);
                    break;
                case 3:
                    DualSwords_ComboInput(p, 1, 1);
                    break;
                case 4:
                    DualSwords_ComboInput(p, 3, 1);
                    break;
                default:
                    if (Motion_ReachedFrame(p, 0, 50.0f) == 0) {
                        DualSwords_CheckInput(p, 0);
                    }
                    break;
                }
            }
            if (Motion_PassedFrame(p, 0, 18.0f) != 0) {
                Eff00_Create(p, 0x27);
            }
            break;
        case 2:
            if (Motion_ReachedFrame(p, 0, 88.0f) != 0) {
                switch (p->stateVar0) {
                case 2:
                    DualSwords_ComboInput(p, 0, 1);
                    break;
                case 3:
                    DualSwords_ComboInput(p, 1, 1);
                    break;
                case 4:
                    DualSwords_ComboInput(p, 3, 1);
                    break;
                default:
                    if (Motion_ReachedFrame(p, 0, 120.0f) == 0) {
                        DualSwords_CheckInput(p, 0);
                    }
                    break;
                }
            }
            if (Motion_PassedFrame(p, 0, 58.0f) != 0) {
                Eff00_Create(p, 0x29);
            }
            if (Motion_PassedFrame(p, 0, 70.0f) != 0) {
                Eff00_Create(p, 0x2A);
            }
            break;
        }
        break;
    }
}

void DualSwords_State01(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0;
        Player_SetMotionPair(p, DualSwords_State01_Motions[variant * 2], 4, DualSwords_State01_Motions[variant * 2 + 1]);
        Player_SetMoveMode(p, 0, 1, 0);
        Eff00_CreateDual(p, 0x2D, variant & 0xFF);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (DualSwords_CheckHit(p) != 1) {
            if (Motion_ReachedFrame(p, 0, 50.0f) != 0) {
                DualSwords_CheckInput(p, 0);
            }
            if (Motion_PassedFrame(p, 0, 24.0f) != 0) {
                Eff00_CreateDual(p, 0x2E, variant & 0xFF);
            }
            if (Motion_PassedFrame(p, 0, 34.0f) != 0) {
                Eff00_CreateDual(p, 0x2F, variant & 0xFF);
            }
        }
        break;
    }
}

void DualSwords_State02(Player *p, s32 variant)
{
    s32 dir;
    s32 timer;

    switch (p->step) {
    case 0:
        Player_SetMoveMode(p, 0, 1, 0);
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        p->timer = 0;
        switch (variant) {
        case 0:
            p->step = 2;
            Player_SetMotionPair(p, 0x57C, 2, 0);
            Eff00_Create(p, 0x2BU);
            break;
        case 1:
            p->step++;
            p->weaponOut = 1;
            Player_SetMotionPair(p, 0x585, 4, 0);
            break;
        case 2:
            p->step = 2;
            Player_SetMotionPair(p, 0x57C, 4, 0);
            Eff00_Create(p, 0x2BU);
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x57C, 2, 0x1C);
            Eff00_Create(p, 0x37U);
        }
        break;
    case 2:
        if ((p->stateVar0 == 0) && (Motion_ReachedFrame(p, 0, 20.0f) != 0) && (Motion_ReachedFrame(p, 0, 42.0f) == 0) && (p->input.stickDirPressed & 0x3C)) {
            dir = (p->input.prevStickAngle + 0x2AAB) & 0xFFFF;
            if ((dir > 16384) && (dir < 0x9556)) {
                p->stateVar0 = 1;
            }
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (DualSwords_CheckHit(p) != 1) {
            if (Motion_PassedFrame(p, 0, 36.0f) != 0) {
                timer = p->timer;
                if (timer == 0) {
                    p->timer = timer + 1;
                    Eff00_Create(p, 0x38U);
                }
            }
            if (Motion_ReachedFrame(p, 0, 42.0f) != 0) {
                if (p->stateVar0 != 0) {
                    Player_SetState(p, 1U, 3U, 4);
                    break;
                }
                if (Motion_ReachedFrame(p, 0, 60.0f) == 0) {
                    DualSwords_CheckInput(p, 0);
                }
            }
        }
        break;
    }
}

//---
void DualSwords_State03(Player *p, s32 variant)
{
    u16 a;

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 1, 0);
        p->stateVar0 = 0;
        Player_SetMotionPair(p, 0x57E, 2, variant ? 4 : 0);
        Eff00_Create(p, 0x2C);
        break;
    case 1:
        if (p->stateVar0 == 0 && Motion_ReachedFrame(p, 0, 26.0f) != 0 && Motion_ReachedFrame(p, 0, 38.0f) == 0
            && (p->input.stickDirPressed & 0x3C)) {
            a = p->input.prevStickAngle + 0x2AAB;
            if (a <= 0x4000) {
                p->stateVar0 = 2;
            } else if (a < 0x9556) {
                p->stateVar0 = 1;
            } else if (a < 0xD556) {
                p->stateVar0 = 3;
            }
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
            break;
        }
        if (DualSwords_CheckHit(p) == 1) {
            break;
        }
        if (Motion_ReachedFrame(p, 0, 26.0f) == 0) {
            break;
        }
        switch (p->stateVar0) {
        case 1:
            Player_SetState(p, 1, 0xB, 4);
            break;
        case 2:
            DualSwords_ComboInput(p, 0, 0);
            break;
        case 3:
            DualSwords_ComboInput(p, 1, 0);
            break;
        default:
            if (Motion_ReachedFrame(p, 0, 38.0f) == 0) {
                DualSwords_CheckInput(p, 0);
            }
            break;
        }
        break;
    }
}

/* The dual swords state 4. Step 0: variant 1 (motion 0x3FB) or the others (0x3FA; variant 0 without the
 * blend); the shared case keeps the compare of 2 (). */
void DualSwords_State04(Player *p, s32 variant)
{

    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        p->weaponOut = 1;
        switch (variant) {
        case 0:
        default:
        case 2:
            p->demonMode = 1;
            p->noFlinchTimer = 2;
            if (variant == 0) {
                Player_SetMotionPair(p, 0x3FA, 4, 0);
                break;
            }
            Player_SetMotionPair(p, 0x3FA, 4, 0xA);
            break;
        case 1:
            p->demonMode = 0;
            Player_SetMotionPair(p, 0x3FB, 4, 0);
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->noFlinchTimer = 0;
            Player_BeginState(p, 0, 2, 0);
            break;
        }
        if (variant != 1) {
            p->noFlinchTimer = 2;
            if (Player_IsLocal(p) == 1 && Motion_ReachedFrame(p, 0, 60.0f) != 0 && (p->input.stickDirPressed & 0x3C)) {
                Player_SetState(p, 1, 2, 4);
            }
        }
        break;
    }
}

/* The dual swords state 5: a two-part attack (motions 0x583/0x582, or 0x57D/0x581 for a nonzero
 * variant); from frame 22 of the second part, the stick and buttons read before it pick the next
 * state (unk006: 1 forward with a 0x2000 turn from timer, 2-5 by button and direction, 6 back). */
void DualSwords_State05(Player *p, s32 variant)
{
    u16 a;

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0xFF;
        p->stateVar1 = 0;
        p->timer = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        if (variant == 0) {
            Player_SetMotionPair(p, 0x583, 4, 0);
        } else {
            Player_SetMotionPair(p, 0x57D, 4, 0);
        }
        Eff00_CreateDual(p, 0x2D, variant & 0xFF);
        break;
    case 1:
        if (Motion_ReachedFrame(p, 0, 40.0f) != 0) {
            p->step++;
            if (variant == 0) {
                Player_SetMotionPair(p, 0x582, 0, 0);
            } else {
                Player_SetMotionPair(p, 0x581, 0, 0);
            }
            Eff00_CreateDual(p, 0x34, variant & 0xFF);
            p->stateVar1 = 0;
            break;
        }
        if (DualSwords_CheckHit(p) == 1) {
            break;
        }
        if (Motion_PassedFrame(p, 0, 24.0f) != 0) {
            Eff00_CreateDual(p, 0x2E, variant & 0xFF);
        }
        if (Motion_PassedFrame(p, 0, 34.0f) != 0) {
            Eff00_CreateDual(p, 0x2F, variant & 0xFF);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (DualSwords_CheckHit(p) == 1) {
            break;
        }
        if (Motion_PassedFrame(p, 0, 28.0f) != 0) {
            Eff00_CreateDual(p, 0x35, variant & 0xFF);
        }
        if (Motion_PassedFrame(p, 0, 38.0f) != 0) {
            Eff00_CreateDual(p, 0x36, variant & 0xFF);
        }
        if (Player_IsLocal(p) != 1) {
            break;
        }
        if (p->stateVar0 == 0xFF) {
            if (Motion_ReachedFrame(p, 0, 22.0f) == 0) {
                if (p->input.stickDirPressed & 0x3C) {
                    a = p->input.prevStickAngle + 0x2AAB;
                    if (a > 0x4000 && a < 0x9556) {
                        p->stateVar0 = 1;
                        if (p->input.stickDir & 0x800) {
                            p->timer = 1;
                        } else if (p->input.stickDir & 0x400) {
                            p->timer = 2;
                        }
                    } else if (a > 0xD555) {
                        p->stateVar0 = 6;
                    }
                }
                if ((p->input.pressed & 0x40) && p->stamina >= 0x4B) {
                    if (p->input.stickDir & 0x3C00) {
                        a = p->input.stickAngle + 0x4000;
                        if (a < 0x5C72) {
                            p->stateVar0 = 2;
                        } else if (a < 0xA38E) {
                            p->stateVar0 = 3;
                        } else {
                            p->stateVar0 = 4;
                        }
                    } else {
                        p->stateVar0 = 5;
                    }
                }
            }
        } else if (Motion_PassedFrame(p, 0, 22.0f) != 0) {
            switch (p->stateVar0) {
            case 1:
                switch (p->timer) {
                case 1:
                    p->rot[1] += 0x2000;
                    p->targetYaw = p->rot[1];
                    break;
                case 2:
                    p->rot[1] -= 0x2000;
                    p->targetYaw = p->rot[1];
                    break;
                }
                Player_SetState(p, 1, 2, 4);
                break;
            case 2:
                Player_SetState(p, 0, 0x31, 4);
                break;
            case 3:
                Player_SetState(p, 0, 0x1C, 4);
                break;
            case 4:
                Player_SetState(p, 0, 0x32, 4);
                break;
            case 5:
                Player_SetState(p, 0, 0x1C, 4);
                break;
            case 6:
                DualSwords_ComboInput(p, 3, 1);
                break;
            }
        }
        break;
    }
}

void DualSwords_State06(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x57F, 2, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        p->noFlinchTimer = 0x1C;
        Eff00_Create(p, 0x30U);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if ((Motion_PassedFrame(p, 0, 58.0f) != 0) && ((s32) p->sharpnessLevel < 2)) {
            Player_SetState(p, 1U, 7U, 4);
            break;
        }
        if (Motion_PassedFrame(p, 0, 20.0f) != 0) {
            Eff00_Create(p, 0x31U);
        }
        if (Motion_PassedFrame(p, 0, 26.0f) != 0) {
            Eff00_Create(p, 0x32U);
        }
        if (Motion_PassedFrame(p, 0, 50.0f) != 0) {
            Eff00_Create(p, 0x32U);
        }
        if (Motion_PassedFrame(p, 0, 54.0f) != 0) {
            Eff00_Create(p, 0x31U);
        }
        if (Motion_PassedFrame(p, 0, 66.0f) != 0) {
            Eff00_Create(p, 0x31U);
        }
        if (Motion_PassedFrame(p, 0, 70.0f) != 0) {
            Eff00_Create(p, 0x32U);
        }
        if (Motion_PassedFrame(p, 0, 82.0f) != 0) {
            Eff00_Create(p, 0x32U);
        }
        if (Motion_PassedFrame(p, 0, 88.0f) != 0) {
            Eff00_Create(p, 0x31U);
        }
        if (Motion_PassedFrame(p, 0, 118.0f) != 0) {
            Eff00_Create(p, 0x33U);
        }
        if ((Player_IsLocal(p) == 1) && (Motion_ReachedFrame(p, 0, 132.0f) != 0)) {
            DualSwords_CheckInput(p, 1);
        }
        break;
    }
}

void DualSwords_State07(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x584, 0, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        }
        break;
    }
}

void DualSwords_State08(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        Player_SetMotionPair(p, 0x580, 4, 0);
        CamShake_ReqA(p, 0);
        Player_SetMoveMode(p, 0, 1, 0);
        break;
    case 1:
        if (p->stateVar0 != 0) {
            if (Motion_ReachedFrame(p, 0, 32.0f) != 0) {
                Player_RequestState(p, 1U, 0x10U, 0xC);
            }
        } else if ((Motion_InFrameRange(p, 0, 14.0f, 40.0f) != 0) && (p->stateVar0 == 0) && (p->itemSelect == 0) && (p->input.stickDirPressed & 0x3C)) {
            p->stateVar0 = 1;
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 6, 0);
        }
        break;
    }
}

void DualSwords_RunState(Player *p)
{
    switch (p->state) {
    case 0:
        DualSwords_State00(p, 0);
        break;
    case 1:
        DualSwords_State01(p, 0);
        break;
    case 2:
        DualSwords_State02(p, 0);
        break;
    case 3:
        DualSwords_State03(p, 0);
        break;
    case 4:
        DualSwords_State04(p, 0);
        break;
    case 5:
        DualSwords_State05(p, 0);
        break;
    case 6:
        DualSwords_State06(p, 0);
        break;
    case 7:
        DualSwords_State07(p, 0);
        break;
    case 8:
        DualSwords_State08(p, 0);
        break;
    case 9:
        DualSwords_State00(p, 1);
        break;
    case 10:
        DualSwords_State00(p, 2);
        break;
    case 11:
        DualSwords_State00(p, 3);
        break;
    case 12:
        DualSwords_State01(p, 1);
        break;
    case 13:
        DualSwords_State01(p, 2);
        break;
    case 14:
        DualSwords_State01(p, 3);
        break;
    case 15:
        DualSwords_State02(p, 1);
        break;
    case 16:
        DualSwords_State02(p, 2);
        break;
    case 17:
        DualSwords_State03(p, 1);
        break;
    case 18:
        DualSwords_State04(p, 1);
        break;
    case 19:
        DualSwords_State04(p, 2);
        break;
    case 20:
        DualSwords_State05(p, 1);
        
    default:
        break;
    }
}

void DualSwords_ComboInput(Player *p, s32 arg1, s32 arg2)
{
    switch (arg1) {
    case 0:
        if ((s8) p->demonMode != 0) {
            Player_SetState(p, 1U, 5U, 4);
            break;
        }
        if (arg2 == 0) {
            Player_SetState(p, 1U, 1U, 4);
            break;
        }
        Player_SetState(p, 1U, 0xDU, 4);
        break;
    case 1:
        if ((s8) p->demonMode != 0) {
            Player_SetState(p, 1U, 0x14U, 4);
            break;
        }
        if (arg2 == 0) {
            Player_SetState(p, 1U, 0xCU, 4);
            break;
        }
        Player_SetState(p, 1U, 0xEU, 4);
        break;
    case 2:
    case 3:
        if ((s8) p->demonMode != 0) {
            Player_SetState(p, 1U, 6U, 4);
            break;
        }
        if (arg1 == 2) {
            Player_SetState(p, 1U, 2U, 4);
        }
        break;
    }
}

s32 DualSwords_AttackInput(Player *p)
{
    s32 dir;

    if (p->input.stickDirPressed & 0x3C) {
        dir = (p->input.prevStickAngle + 0x2AAB) & 0xFFFF;
        if (dir < 0x4001) {
            DualSwords_ComboInput(p, 0, 0);
            return 1;
        }
        if (dir < 0x9556) {
            Player_SetState(p, 1U, 0U, 4);
            return 1;
        }
        if (dir < 0xD556) {
            DualSwords_ComboInput(p, 1, 0);
            return 1;
        }
        DualSwords_ComboInput(p, 2, 0);
        return 1;
    }
    if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B)) {
        Player_SetState(p, 0U, 0x1CU, 4);
        return 1;
    }
    return 0;
}

void DualSwords_CheckInput(Player *p, s32 variant)
{
    u16 pressed;
    s32 dir;

    if ((Player_IsLocal(p) != 0 || Game_IsOnline() != 1) && Quest_PhaseIsOver(1) != 1 && p->itemSelect == 0) {
        pressed = p->input.pressed;
        if ((pressed & 0x40) && p->stamina >= 75) {
            if (p->input.stickDir & 0x3C00) {
                dir = (p->input.stickAngle + 0x4000) & 0xFFFF;
                if (dir < 0x5C72) {
                    Player_SetState(p, 0, 0x31, 4);
                    return;
                }
                if (dir < 0xA38E) {
                    Player_SetState(p, 0, 0x1C, 4);
                    return;
                }
                Player_SetState(p, 0, 0x32, 4);
                return;
            }
            Player_SetState(p, 0, 0x1C, 4);
            return;
        }
        if (variant == 0 && (pressed & 0x80)) {
            if (p->demonMode == 0) {
                Player_RequestState(p, 1, 0x13, 0);
                return;
            }
            Player_RequestState(p, 1, 0x12, 0);
            return;
        }
    }
}

void DualSwords_DrawAttack(Player *p)
{
    Player_SetState(p, 0x1, 0xF, 0x4);
}

void DualSwords_EvadeAttack(Player *p)
{
    Player_SetState(p, 0x1, 0x11, 0x4);
}

void DualSwords_Draw(Player *p, s32 arg1)
{
    if (arg1 == 0) {
        Player_SetState((void *) p, 1U, 4U, 4);
        return;
    }
    Player_SetState((void *) p, 1U, 0x12U, 4);
}

s32 LongSword_CheckHit(Player *p)
{

    if (p->hitPending != 0 && *(s32 *) &p->timer == 0) {
        *(s32 *) &p->timer = 1;
        switch (p->hitKind) {
        case 0:
            break;
        case 1:
            p->hitStopAdd = p->hitStop = 0;
            Player_SetState(p, 1, 3, 0);
            return 1;
        case 2:
        case 3:
            Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
            p->weaponSeMute = 2;
            break;
        }
    }
    return 0;
}

void LongSword_State00(Player *p, s32 variant, s32 arg2)
{
    f32 in[4];
    f32 out[4];

    switch (p->step) {
    case 0:
        switch (variant) {
        case 0:
            p->unk417 = 1;
            p->weaponOut = 1;
            p->fxMade = 0;
            p->step++;
            p->stateVar1 = 0xFF;
            p->timer = 0;
            Player_SetMoveMode(p, 0, 1, 0);
            Player_SetMotionPair(p, 0x579, 4, 0xA);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x39);
            p->noFlinchTimer = 0x28;
            break;
        case 1:
            switch (p->stateVar0) {
            case 0:
                p->stateVar0++;
                p->unk417 = 1;
                p->weaponOut = 1;
                p->fxMade = 0;
                Player_SetMoveMode(p, 0, 1, 0);
                Player_SetMotionPair(p, 0x584, 2, 0);
                Player_UpdateMotionTime(p, 0);
                p->noFlinchTimer = 0x28;
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    Player_SetMotionPair(p, 0x579, 2, 0x40);
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 0x39);
                    p->stateVar1 = 0xFF;
                    p->timer = 0;
                }
                break;
            }
            break;
        case 2:
            switch (p->stateVar0) {
            case 0:
                p->stateVar0++;
                p->unk417 = 1;
                p->fxMade = 0;
                p->weaponOut = 1;
                Player_SetMoveMode(p, 0, 1, 0);
                Player_SetMotionPair(p, 0x586, 2, 0);
                Player_UpdateMotionTime(p, 0);
                p->noFlinchTimer = 0x28;
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    Player_SetMotionPair(p, 0x579, 2, 0x26);
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 0x39);
                    p->stateVar1 = 0xFF;
                    p->timer = 0;
                }
                break;
            }
            break;
        case 3:
            switch (p->stateVar0) {
            case 0:
                p->stateVar0++;
                p->unk417 = 1;
                p->fxMade = 0;
                p->weaponOut = 1;
                Player_SetMoveMode(p, 0, 1, 0);
                Player_SetMotionPair(p, 0x587, 2, 0);
                Player_UpdateMotionTime(p, 0);
                p->noFlinchTimer = 0x28;
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    Player_SetMotionPair(p, 0x579, 0, 0x30);
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 0x39);
                    p->stateVar1 = 0xFF;
                    p->timer = 0;
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        } else if (LongSword_CheckHit(p) != 1) {
            if ((Motion_ReachedFrame(p, 0, 48.0f) != 0) && (p->fxMade == 0)) {
                p->fxMade = 1;
                StageObj_K05_Create(p, 9, 5);
            }
            if ((p->stateVar1 == 0xFF) && (Motion_InFrameRange(p, 0, 44.0f, 94.0f) != 0)) {
                p->stateVar1 = LongSword_ComboInput(p, 1, LongSword_StickAction(p) & 0xFF, 1);
            }
            if ((Motion_InFrameRange(p, 0, 40.0f, 56.0f) != 0) && (arg2 != 0)) {
                in[0] = 0.0f;
                in[1] = 0.0f;
                in[2] = 7.5f;
                Vec_Rotate(out, in, p->rotMtx);
                p->pos[0] += out[0];
                p->pos[2] += out[2];
            }
            if ((Motion_ReachedFrame(p, 0, 80.0f) != 0) && (p->stateVar1 != 0xFF)) {
                LongSword_ComboInput(p, 1, p->stateVar1, 0);
            } else if (Motion_InFrameRange(p, 0, 80.0f, 164.0f) != 0) {
                LongSword_CheckInput(p);
            }
        }
        break;
    }
}

void LongSword_State01(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        switch (variant) {
        case 0:
            p->step++;
            p->stateVar1 = 0xFF;
            p->timer = 0;
            p->unk417 = 1;
            Player_SetMotionPair(p, 0x57A, 4, 0xA);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x3A);
            p->noFlinchTimer = 0x46;
            Player_SetMoveMode(p, 0, 1, 0);
            break;
        case 1:
            switch (p->stateVar0) {
            case 0:
                p->stateVar0++;
                p->unk417 = 1;
                p->weaponOut = 1;
                Player_SetMoveMode(p, 0, 1, 0);
                Player_SetMotionPair(p, 0x58C, 2, 0);
                Player_UpdateMotionTime(p, 0);
                p->noFlinchTimer = 0x28;
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    Player_SetMotionPair(p, 0x57A, 0, 0x18);
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 0x3A);
                    p->stateVar1 = 0xFF;
                    p->timer = 0;
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        } else if (LongSword_CheckHit(p) != 1) {
            if (Motion_PassedFrame(p, 0, 48.0f) != 0) {
                StageObj_K05_Create(p, 8, 5);
            }
            if ((p->stateVar1 == 0xFF) && (Motion_InFrameRange(p, 0, 44.0f, 104.0f) != 0)) {
                p->stateVar1 = LongSword_ComboInput(p, 2, LongSword_StickAction(p) & 0xFF, 1);
            }
            if ((Motion_ReachedFrame(p, 0, 80.0f) != 0) && (p->stateVar1 != 0xFF)) {
                LongSword_ComboInput(p, 2, p->stateVar1, 0);
            } else if (Motion_InFrameRange(p, 0, 80.0f, 164.0f) != 0) {
                LongSword_CheckInput(p);
            }
        }
        break;
    }
}

void LongSword_State02(Player *p, u32 variant)
{
    switch (p->step) {
    case 0:
        switch (variant) {
        case 0:
            p->unk417 = 1;
            p->step++;
            p->stateVar1 = 0xFF;
            p->timer = 0;
            Player_SetMoveMode(p, 0, 1, 0);
            Player_SetMotionPair(p, 0x57B, 4, 0);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x3B);
            p->noFlinchTimer = 0x28;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            switch (p->stateVar0) {
            case 0:
                p->unk417 = 1;
                p->stateVar0++;
                switch (variant) {
                case 1:
                case 2:
                    Player_SetMotionPair(p, 0x582, 2, 0);
                    break;
                case 3:
                    Player_SetMotionPair(p, 0x589, 2, 0);
                    break;
                case 4:
                    Player_SetMotionPair(p, 0x58A, 2, 0);
                    break;
                case 5:
                    Player_SetMotionPair(p, 0x58D, 4, 0);
                    break;
                case 6:
                    Player_SetMotionPair(p, 0x58E, 2, 0);
                    break;
                }
                Player_UpdateMotionTime(p, 0);
                Player_SetMoveMode(p, 0, 1, 0);
                p->noFlinchTimer = 0x28;
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    p->stateVar1 = 0xFF;
                    p->timer = 0;
                    switch (variant) {
                    case 1:
                    case 2:
                        Player_SetMotionPair(p, 0x57B, 2, 0x12);
                        break;
                    case 3:
                        Player_SetMotionPair(p, 0x57B, 0, 0x14);
                        break;
                    case 4:
                        Player_SetMotionPair(p, 0x57B, 0, 0x14);
                        break;
                    case 5:
                        Player_SetMotionPair(p, 0x57B, 0, 0x10);
                        break;
                    case 6:
                        Player_SetMotionPair(p, 0x57B, 0, 0xC);
                        break;
                    }
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 0x3B);
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        } else if (LongSword_CheckHit(p) != 1) {
            if (Motion_PassedFrame(p, 0, 22.0f) != 0) {
                StageObj_K05_Create(p, 0x11, 5);
            }
            if ((p->stateVar1 == 0xFF) && (Motion_InFrameRange(p, 0, 28.0f, 68.0f) != 0)) {
                p->stateVar1 = LongSword_ComboInput(p, 3, LongSword_StickAction(p) & 0xFF, 1);
            }
            if ((Motion_ReachedFrame(p, 0, 68.0f) != 0) && (p->stateVar1 != 0xFF)) {
                LongSword_ComboInput(p, 3, p->stateVar1, 0);
            } else if (Motion_InFrameRange(p, 0, 80.0f, 128.0f) != 0) {
                LongSword_CheckInput(p);
            }
        }
        break;
    }
}

void LongSword_State03(Player *p, s32 unused)
{
    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar0 = 0;
        p->stateVar1 = 0;
        Player_SetMoveMode(p, 0, 0, 0);
        Player_SetMotionPair(p, 0x585, 0, 0);
        break;
    case 1:
        if (p->stateVar0 != 0) {
            if (Motion_ReachedFrame(p, 0, 22.0f) != 0) {
                Player_SetState(p, 1U, 0x11U, 4);
            }
        } else if ((Motion_InFrameRange(p, 0, 2.0f, 30.0f) != 0) && (p->stateVar0 == 0) && (p->itemSelect == 0) && (p->input.stickDirPressed & 0x3C)) {
            p->stateVar0 = 1;
            p->stateVar1 = 0;
        }
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 0xA, 0);
        }
        break;
    }
}

void LongSword_State04(Player *p, s32 unused)
{
    u8 stick;

    switch (p->step) {
    case 0:
        p->step++;
        p->stateVar1 = 0xFF;
        p->timer = 0;
        p->unk417 = 1;
        Player_SetMotionPair(p, 0x57D, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Eff00_Create(p, 0x48U);
        p->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (LongSword_CheckHit(p) != 1) {
            if (Motion_PassedFrame(p, 0, 18.0f) != 0) {
                StageObj_K05_Create(p, 9, 5);
            }
            if (p->motion[0].playing == 0) {
                Player_BeginState(p, 0, 4, 0);
                break;
            }
            if ((p->stateVar1 == 0xFF) && (Motion_InFrameRange(p, 0, 28.0f, 100.0f) != 0)) {
                p->stateVar1 = LongSword_ComboInput(p, 0xBU, LongSword_StickAction(p) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(p, 0, 38.0f) != 0) {
                stick = p->stateVar1;
                if (stick != 0xFF) {
                    LongSword_ComboInput(p, 0xBU, stick, 0);
                    break;
                }
            }
            if (Motion_InFrameRange(p, 0, 38.0f, 160.0f) != 0) {
                LongSword_CheckInput(p);
            }
        } else {
        }
        break;
    }
}

void LongSword_State07(Player *p, u32 variant)
{
    u8 a;

    switch (p->step) {
    case 0:
        switch (variant) {
        case 0:
        case 1:
            p->unk417 = 1;
            p->weaponOut = 1;
            p->fxMade = 0;
            p->step++;
            p->stateVar1 = 0xFF;
            p->timer = 0;
            Player_SetMoveMode(p, 0, 1, 0);
            Player_SetMotionPair(p, 0x57C, 4, 8);
            Player_UpdateMotionTime(p, 0);
            if (variant == 0) {
                Eff00_Create(p, 0x45);
                LongSword_AddGauge(p, -0xF);
                StageObj_K29_CreateOnPlayerDefault(p, 0, 1.0f);
            } else {
                Eff00_Create(p, 0x4F);
            }
            p->noFlinchTimer = 0x23;
            break;
        case 2:
        case 3:
        case 4:
        case 5:
            switch (p->stateVar0) {
            case 0:
                p->stateVar0++;
                p->unk417 = 1;
                p->weaponOut = 1;
                p->fxMade = 0;
                Player_SetMoveMode(p, 0, 1, 0);
                if (variant == 2 || variant == 3) {
                    Player_SetMotionPair(p, 0x591, 2, 0);
                } else {
                    Player_SetMotionPair(p, 0x592, 4, 0);
                }
                Player_UpdateMotionTime(p, 0);
                if (variant == 2 || variant == 4) {
                    LongSword_AddGauge(p, -0x14);
                    StageObj_K29_CreateOnPlayerDefault(p, 0, 1.0f);
                }
                p->noFlinchTimer = 0x23;
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    if (variant == 2 || variant == 3) {
                        Player_SetMotionPair(p, 0x57C, 0, 0x1E);
                    } else {
                        Player_SetMotionPair(p, 0x57C, 0, 0x2C);
                    }
                    Player_UpdateMotionTime(p, 0);
                    if (variant == 2 || variant == 4) {
                        Eff00_Create(p, 0x45);
                    } else {
                        Eff00_Create(p, 0x4F);
                    }
                    p->stateVar1 = 0xFF;
                    p->timer = 0;
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (variant == 1 || variant == 3 || variant == 5) {
            if (LongSword_CheckHit(p) == 1) {
                break;
            }
        }
        if (Motion_ReachedFrame(p, 0, 40.0f) != 0 && p->fxMade == 0) {
            p->fxMade = 1;
            if (variant == 0 || variant == 2 || variant == 4) {
                StageObj_K05_Create(p, 6, 6);
            } else {
                StageObj_K05_Create(p, 6, 5);
            }
        }
        if (p->stateVar1 == 0xFF && Motion_InFrameRange(p, 0, 46.0f, 100.0f) != 0) {
            a = LongSword_StickAction(p);
            if (variant == 0 || variant == 2 || variant == 4) {
                p->stateVar1 = LongSword_ComboInput(p, 4, a, 1);
            } else {
                p->stateVar1 = LongSword_ComboInput(p, 0xD, a, 1);
            }
        }
        if (Motion_ReachedFrame(p, 0, 56.0f) != 0) {
            a = p->stateVar1;
            if (a != 0xFF) {
                if (variant == 0 || variant == 2 || variant == 4) {
                    LongSword_ComboInput(p, 4, a, 0);
                    break;
                }
                LongSword_ComboInput(p, 0xD, a, 0);
                break;
            }
        }
        if (Motion_InFrameRange(p, 0, 56.0f, 160.0f) != 0) {
            LongSword_CheckInput(p);
        }
        break;
    }
}

void LongSword_State08(Player *p, s32 unused)
{
    u8 stick;

    switch (p->step) {
    case 0:
        p->unk417 = 1;
        p->step++;
        p->stateVar1 = 0xFF;
        p->timer = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        Player_SetMotionPair(p, 0x58F, 2, 0);
        Player_UpdateMotionTime(p, 0);
        p->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x57D, 0, 0x10);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x47U);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (LongSword_CheckHit(p) != 1) {
            if (Motion_PassedFrame(p, 0, 18.0f) != 0) {
                StageObj_K05_Create(p, 9, 5);
            }
            if ((p->stateVar1 == 0xFF) && (Motion_InFrameRange(p, 0, 28.0f, 100.0f) != 0)) {
                p->stateVar1 = LongSword_ComboInput(p, 5U, LongSword_StickAction(p) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(p, 0, 38.0f) != 0) {
                stick = p->stateVar1;
                if (stick != 0xFF) {
                    LongSword_ComboInput(p, 5U, stick, 0);
                    break;
                }
            }
            if (Motion_InFrameRange(p, 0, 38.0f, 160.0f) != 0) {
                LongSword_CheckInput(p);
            }
        }
        break;
    }
}

void LongSword_State09(Player *p, s32 variant)
{
    u8 stick;
    u8 stateVar0;

    switch (p->step) {
    case 0:
        switch (variant) {
        case 0:
            p->unk417 = 1;
            p->step++;
            p->stateVar1 = 0xFF;
            p->timer = 0;
            Player_SetMoveMode(p, 0, 1, 0);
            Player_SetMotionPair(p, 0x57E, 4, 0);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x49U);
            LongSword_AddGauge(p, -0x14);
            StageObj_K29_CreateOnPlayerDefault(p, 0U, 1.0f);
            p->noFlinchTimer = 0x3C;
            break;
        case 1:
            stateVar0 = p->stateVar0;
            switch (stateVar0) {
            case 0:
                p->stateVar0 = stateVar0 + 1;
                p->unk417 = 1;
                p->weaponOut = 1;
                Player_SetMoveMode(p, 0, 1, 0);
                Player_SetMotionPair(p, 0x590, 2, 0);
                Player_UpdateMotionTime(p, 0);
                LongSword_AddGauge(p, -0x14);
                StageObj_K29_CreateOnPlayerDefault(p, 0U, 1.0f);
                p->noFlinchTimer = 0x3C;
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    Player_SetMotionPair(p, 0x57E, 0, 0x18);
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 0x49U);
                    p->stateVar1 = 0xFF;
                    p->timer = 0;
                }
                break;
            }
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (Motion_PassedFrame(p, 0, 28.0f) != 0) {
            StageObj_K05_Create(p, 8, 6);
        }
        if ((p->stateVar1 == 0xFF) && (Motion_InFrameRange(p, 0, 34.0f, 100.0f) != 0)) {
            p->stateVar1 = LongSword_ComboInput(p, 6U, LongSword_StickAction(p) & 0xFF, 1);
        }
        if (Motion_ReachedFrame(p, 0, 44.0f) != 0) {
            stick = p->stateVar1;
            if (stick != 0xFF) {
                LongSword_ComboInput(p, 6U, stick, 0);
                break;
            }
        }
        if (Motion_InFrameRange(p, 0, 62.0f, 160.0f) != 0) {
            LongSword_CheckInput(p);
        }
        break;
    }
}

void LongSword_State10(Player *p, s32 unused)
{
    u8 stick;

    switch (p->step) {
    case 0:
        p->unk417 = 1;
        p->step++;
        p->stateVar1 = 0xFF;
        p->timer = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        Player_SetMotionPair(p, 0x57F, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Eff00_Create(p, 0x4AU);
        p->noFlinchTimer = 0x3C;
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (LongSword_CheckHit(p) != 1) {
            if (Motion_PassedFrame(p, 0, 22.0f) != 0) {
                StageObj_K05_Create(p, 5, 5);
            }
            if ((p->stateVar1 == 0xFF) && (Motion_InFrameRange(p, 0, 32.0f, 100.0f) != 0)) {
                p->stateVar1 = LongSword_ComboInput(p, 7U, LongSword_StickAction(p) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(p, 0, 42.0f) != 0) {
                stick = p->stateVar1;
                if (stick != 0xFF) {
                    LongSword_ComboInput(p, 7U, stick, 0);
                    break;
                }
            }
            if (Motion_InFrameRange(p, 0, 42.0f, 160.0f) != 0) {
                LongSword_CheckInput(p);
            }
        } else {
        }
        break;
    }
}

void LongSword_State14(Player *p, s32 variant)
{
    switch (p->step) {
    case 0:
        switch (variant) {
        case 0:
            p->step++;
            p->stateVar1 = 0xFF;
            p->timer = 0;
            p->unk417 = 1;
            Player_SetMotionPair(p, 0x57F, 4, 0);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x4B);
            break;
        case 1:
            switch (p->stateVar0) {
            case 0:
                p->stateVar0++;
                p->unk417 = 1;
                p->weaponOut = 1;
                Player_SetMoveMode(p, 0, 1, 0);
                Player_SetMotionPair(p, 0x588, 2, 0);
                Player_UpdateMotionTime(p, 0);
                p->noFlinchTimer = 0x28;
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    Player_SetMotionPair(p, 0x57F, 0, 0x14);
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 0x4B);
                    p->stateVar1 = 0xFF;
                    p->timer = 0;
                }
                break;
            }
            break;
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
        } else if (LongSword_CheckHit(p) != 1) {
            if (Motion_PassedFrame(p, 0, 22.0f) != 0) {
                StageObj_K05_Create(p, 5, 5);
            }
            if ((p->stateVar1 == 0xFF) && (Motion_InFrameRange(p, 0, 32.0f, 100.0f) != 0)) {
                p->stateVar1 = LongSword_ComboInput(p, 0xC, LongSword_StickAction(p) & 0xFF, 1);
            }
            if ((Motion_ReachedFrame(p, 0, 42.0f) != 0) && (p->stateVar1 != 0xFF)) {
                LongSword_ComboInput(p, 0xC, p->stateVar1, 0);
            } else if (Motion_InFrameRange(p, 0, 42.0f, 160.0f) != 0) {
                LongSword_CheckInput(p);
            }
        }
        break;
    }
}

void LongSword_State15(Player *p, s32 variant)
{
    u8 stick;
    u8 stateVar0;

    switch (p->step) {
    case 0:
        switch (variant) {
        case 0:
            p->unk417 = 1;
            p->step++;
            p->stateVar0 = 0;
            p->stateVar1 = 0xFF;
            p->timer = 0;
            Player_SetMoveMode(p, 0, 1, 0);
            Player_SetMotionPair(p, 0x580, 4, 0);
            Player_UpdateMotionTime(p, 0);
            Eff00_Create(p, 0x4CU);
            LongSword_AddGauge(p, -0x19);
            StageObj_K29_CreateOnPlayerDefault(p, 0U, 1.0f);
            p->noFlinchTimer = 0x3C;
            break;
        case 1:
            stateVar0 = p->stateVar0;
            switch (stateVar0) {
            case 0:
                p->stateVar0 = stateVar0 + 1;
                p->unk417 = 1;
                p->weaponOut = 1;
                Player_SetMoveMode(p, 0, 1, 0);
                Player_SetMotionPair(p, 0x58B, 2, 0);
                Player_UpdateMotionTime(p, 0);
                p->noFlinchTimer = 0x3C;
                LongSword_AddGauge(p, -0x19);
                StageObj_K29_CreateOnPlayerDefault(p, 0U, 1.0f);
                break;
            case 1:
                if (p->motion[0].playing == 0) {
                    p->step++;
                    p->stateVar0 = 0;
                    Player_SetMotionPair(p, 0x580, 0, 0xA);
                    Player_UpdateMotionTime(p, 0);
                    Eff00_Create(p, 0x4CU);
                    p->stateVar1 = 0xFF;
                    p->timer = 0;
                }
                break;
            }
        }
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (Motion_PassedFrame(p, 0, 14.0f) != 0) {
            StageObj_K05_Create(p, 6, 6);
        }
        if (Motion_PassedFrame(p, 0, 34.0f) != 0) {
            StageObj_K05_Create(p, 6, 6);
        }
        if (Motion_PassedFrame(p, 0, 84.0f) != 0) {
            StageObj_K05_Create(p, 8, 6);
        }
        if ((Motion_PassedFrame(p, 0, 34.0f) != 0) && (p->stateVar0 != 1)) {
            p->stateVar0 = 1;
            Eff00_Create(p, 0x4DU);
        }
        if ((Motion_PassedFrame(p, 0, 88.0f) != 0) && (p->stateVar0 != 2)) {
            p->stateVar0 = 2;
            Eff00_Create(p, 0x50U);
        }
        if ((p->stateVar1 == 0xFF) && (Motion_InFrameRange(p, 0, 100.0f, 160.0f) != 0)) {
            p->stateVar1 = LongSword_ComboInput(p, 8U, LongSword_StickAction(p) & 0xFF, 1);
        }
        if (Motion_ReachedFrame(p, 0, 110.0f) != 0) {
            stick = p->stateVar1;
            if (stick != 0xFF) {
                LongSword_ComboInput(p, 8U, stick, 0);
                break;
            }
        }
        if (Motion_InFrameRange(p, 0, 110.0f, 160.0f) != 0) {
            LongSword_CheckInput(p);
        }
        break;
    }
}

void LongSword_State16(Player *p, s32 unused)
{
    u8 stick;

    switch (p->step) {
    case 0:
        p->unk417 = 1;
        p->step++;
        p->stateVar1 = 0xFF;
        p->timer = 0;
        Player_SetMoveMode(p, 0, 1, 0);
        Player_SetMotionPair(p, 0x581, 4, 0);
        Player_UpdateMotionTime(p, 0);
        Eff00_Create(p, 0x4EU);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            Player_BeginState(p, 0, 4, 0);
            break;
        }
        if (LongSword_CheckHit(p) != 1) {
            if ((p->stateVar1 == 0xFF) && (Motion_InFrameRange(p, 0, 40.0f, 100.0f) != 0)) {
                p->stateVar1 = LongSword_ComboInput(p, 9U, LongSword_StickAction(p) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(p, 0, 60.0f) != 0) {
                stick = p->stateVar1;
                if (stick != 0xFF) {
                    LongSword_ComboInput(p, 9U, stick, 0);
                    break;
                }
            }
            if (Motion_InFrameRange(p, 0, 90.0f, 160.0f) != 0) {
                LongSword_CheckInput(p);
            }
        } else {
        }
        break;
    }
}

void LongSword_RunState(Player *p)
{
    switch (p->state) {
    case 0:
        LongSword_State00(p, 0, 0);
        break;
    case 1:
        LongSword_State01(p, 0);
        break;
    case 2:
        LongSword_State02(p, 0U);
        break;
    case 3:
        LongSword_State03(p, 0);
        break;
    case 4:
        LongSword_State04(p, 0);
        break;
    case 5:
        LongSword_State02(p, 1U);
        break;
    case 6:
        LongSword_State02(p, 2U);
        break;
    case 7:
        LongSword_State07(p, 0U);
        break;
    case 8:
        LongSword_State08(p, 0);
        break;
    case 9:
        LongSword_State09(p, 0);
        break;
    case 10:
        LongSword_State10(p, 0);
        break;
    case 11:
        LongSword_State00(p, 1, 0);
        break;
    case 12:
        LongSword_State00(p, 0, 1);
        break;
    case 13:
        LongSword_State00(p, 1, 1);
        break;
    case 14:
        LongSword_State14(p, 0);
        break;
    case 15:
        LongSword_State15(p, 0);
        break;
    case 16:
        LongSword_State16(p, 0);
        break;
    case 17:
        LongSword_State00(p, 2, 0);
        break;
    case 18:
        LongSword_State07(p, 1U);
        break;
    case 19:
        LongSword_State00(p, 3, 0);
        break;
    case 20:
        LongSword_State14(p, 1);
        break;
    case 21:
        LongSword_State02(p, 3U);
        break;
    case 22:
        LongSword_State02(p, 4U);
        break;
    case 23:
        LongSword_State15(p, 1);
        break;
    case 24:
        LongSword_State01(p, 1);
        break;
    case 25:
        LongSword_State02(p, 5U);
        break;
    case 26:
        LongSword_State02(p, 6U);
        break;
    case 27:
        LongSword_State09(p, 1);
        break;
    case 28:
        LongSword_State07(p, 2U);
        break;
    case 29:
        LongSword_State07(p, 3U);
        break;
    case 30:
        LongSword_State07(p, 4U);
        break;
    case 31:
        LongSword_State07(p, 5U);
        
    default:
        break;
    }
}

s32 LongSword_AttackInput(Player *p)
{
    u16 pressed;

    switch (LongSword_StickAction(p) & 0xFF) {
    case 0:
        break;
    case 1:
        if (p->input.stickDir & 0x3C00) {
            Player_SetState(p, 1, 0xC, 4);
        } else {
            Player_SetState(p, 1, 0, 4);
        }
        return 1;
    case 3:
        Player_SetState(p, 1, 2, 4);
        return 1;
    case 4:
        if (LongSword_GaugeAtLeast(p, 0xF) == 1) {
            Player_SetState(p, 1, 7, 4);
        } else {
            Player_SetState(p, 1, 0x12, 4);
        }
        return 1;
    }
    pressed = p->input.pressed;
    if (pressed & 1) {
        Player_SetState(p, 1, 4, 4);
        return 1;
    }
    if ((pressed & 0x40) && p->stamina >= 75) {
        Player_SetState(p, 0, 0x1C, 4);
        return 1;
    }
    return 0;
}

s32 LongSword_StickAction(Player *p)
{
    u16 btn;

    if (p->input.stickDirPressed & 0x3C) {
        if (p->input.prevStickAngle < 0x8001) {
            return 1;
        }
        return 3;
    }
    btn = p->input.pressed;
    if (btn & 0x80) {
        return 4;
    }
    return !(btn & 1) ? 0 : 0xA;
}

void LongSword_CheckInput(Player *p)
{
    s32 dir;

    if ((p->input.pressed & 0x40) && (p->stamina >= 0x4B)) {
        if (p->input.stickDir & 0x3C00) {
            dir = (p->input.stickAngle + 0x4000) & 0xFFFF;
            if (dir < 0x5C72) {
                Player_SetState(p, 0U, 0x31U, 4);
                return;
            }
            if (dir < 0xA38E) {
                Player_SetState(p, 0U, 0x1CU, 4);
                return;
            }
            Player_SetState(p, 0U, 0x32U, 4);
            return;
        }
        Player_SetState(p, 0U, 0x1CU, 4);
    }
}

u8 LongSword_ComboInput(Player *p, u32 kind, u8 stick, u8 check)
{
    switch (stick) {
    case 1:
        switch (kind) {
        case 1:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 1, 4, 0);
            } else {
                return 1;
            }
            break;
        case 4:
        case 13:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 8, 4, 1);
            } else {
                return 1;
            }
            break;
        case 6:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 0xA, 4, 1);
            } else {
                return 1;
            }
            break;
        case 12:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 0x18, 4, 1);
            } else {
                return 1;
            }
            break;
        case 11:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 0x14, 4, 1);
            } else {
                return 1;
            }
            break;
        default:
            return 0xFF;
        }
        break;
    case 3:
        switch (kind) {
        case 1:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 5, 4, 0);
            } else {
                return 3;
            }
            break;
        case 2:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 6, 4, 0);
            } else {
                return 3;
            }
            break;
        case 4:
        case 13:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 0x19, 4, 0);
            } else {
                return 3;
            }
            break;
        case 6:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 0x1A, 4, 0);
            } else {
                return 3;
            }
            break;
        case 8:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 5, 4, 0);
            } else {
                return 3;
            }
            break;
        case 9:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 2, 4, 0);
            } else {
                return 3;
            }
            break;
        case 5:
        case 11:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 0x15, 4, 0);
            } else {
                return 3;
            }
            break;
        case 7:
        case 12:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 0x16, 4, 0);
            } else {
                return 3;
            }
            break;
        default:
            return 0xFF;
        }
        break;
    case 4:
        switch (kind) {
        case 1:
        case 2:
        case 11:
            if (check == 0) {
                if (LongSword_GaugeAtLeast(p, 0xF) == 1) {
                    Player_TurnAndSetState(p, 1, 7, 4, 1);
                } else {
                    Player_TurnAndSetState(p, 1, 0x12, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 12:
            if (check == 0) {
                if (LongSword_GaugeAtLeast(p, 0xF) == 1) {
                    Player_TurnAndSetState(p, 1, 0x1C, 4, 1);
                } else {
                    Player_TurnAndSetState(p, 1, 0x1D, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 4:
            if (check == 0) {
                if (LongSword_GaugeAtLeast(p, 0x14) == 1) {
                    Player_TurnAndSetState(p, 1, 9, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 5:
            if (check == 0) {
                if (LongSword_GaugeAtLeast(p, 0x14) == 1) {
                    Player_TurnAndSetState(p, 1, 0x1B, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 6:
            if (check == 0) {
                if (LongSword_GaugeAtLeast(p, 0x19) == 1) {
                    Player_TurnAndSetState(p, 1, 0xF, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 7:
            if (check == 0) {
                if (LongSword_GaugeAtLeast(p, 0x19) == 1) {
                    Player_TurnAndSetState(p, 1, 0x17, 4, 1);
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
        switch (kind) {
        case 1:
        case 2:
            if (check == 0) {
                Player_TurnAndSetState(p, 1, 4, 4, 1);
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

void LongSword_DrawAttack(Player *p)
{
    Player_SetState(p, 0x1, 0x13, 0x4);
}

void LongSword_Draw(Player *p)
{
    if (LongSword_GaugeAtLeast(p, 0xF) == 1) {
        Player_SetState(p, 1U, 0x1EU, 4);
        return;
    }
    Player_SetState(p, 1U, 0x1FU, 4);
}

void Player_TurnAndSetState(Player *p, u8 group, u8 state, s32 mode, u8 turn)
{
    if (turn == 1) {
        if (p->input.stickDir & 0x800) {
            p->targetYaw += 0x1000;
        } else if (p->input.stickDir & 0x400) {
            p->targetYaw -= 0x1000;
        }
    }
    Player_SetState(p, group, state, mode);
}

s32 HuntingHorn_CheckHit(Player *p)
{

    switch (p->stateVar1) {
    case 0:
        if (p->hitPending != 0) {
            CamShake_ReqA(p, 0);
            switch (p->hitKind) {
            case 0:
                p->stateVar1++;
                break;
            case 1:
                p->stateVar1++;
                p->hitStop = 0;
                p->hitStopAdd = 0;
                Player_RequestState(p, 1, 0x18, 0);
                return 1;
            case 2:
            case 3:
                p->stateVar1++;
                Player_SetMotionPair(p, p->motionId[0], 0, (s16) (p->motion[0].frame - 2.0f));
                p->weaponSeMute = 2;
                CamShake_ReqA(p, 1);
                break;
            }
        }
        break;
    case 1:
        if (p->hitSlowTimer <= 0) {
            p->stateVar1++;
        }
        break;
    case 2:
        break;
    }
    return 0;
}

u16 Camera_GetYaw(void);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
/* While the stick is pushed: turns rotY toward the stick direction (+0x388, relative to the camera
 * yaw) by at most n, then moves dist along the new heading. */
void Player_StickTurnMove(Player *p, s16 n, f32 dist)
{
    f32 m[16];
    f32 v[3];
    f32 out[3];
    s32 ang[3];
    u16 target;
    s32 d;

    if (Player_StickMoveLevel(p, 0) > 0) {
        target = *(u16 *) ((u8 *) p + 0x388) + Camera_GetYaw();
        d = (target - p->rot[1]) & 0xFFFF;
        if ((u32) (d + n) < (u32) (n * 2)) {
            p->rot[1] = target;
        } else if (d < 0x8000U) {
            p->rot[1] += n;
        } else {
            p->rot[1] -= n;
        }
        p->targetYaw = p->rot[1];
        ang[0] = 0;
        ang[2] = 0;
        ang[1] = p->rot[1];
        Mtx_FromAnglesXYZ(ang, m);
        v[0] = 0.0f;
        v[2] = dist;
        v[1] = 0.0f;
        Vec_Rotate(out, v, m);
        p->pos[0] += out[0];
        p->pos[2] += out[2];
    }
}

void HuntingHorn_State00(Player *p, s32 motion, s32 variant)
{
    s16 i;
    FrameRange *r;
    FrameRange *r2;
    s16 j;
    FrameRange *r3;
    s16 k;

    if (p->timer > 0) {
        p->timer--;
    }
    if (p->stateVar1 > 0) {
        p->stateVar1--;
    }
    switch (p->step) {
    case 0:
        p->step++;
        p->weaponOut = 1;
        p->timer = 10;
        p->stateVar0 = 0;
        p->stateVar1 = 40;
        switch (variant) {
        case 0:
            p->step = 3;
            Player_SetMotionPair(p, HuntingHorn_State00_Motions[motion], 4, 0);
            break;
        case 1:
            p->step = 3;
            Player_SetMotionPair(p, HuntingHorn_State00_Motions[motion], 0, 0);
            break;
        case 2:
        case 3:
            p->step = 2;
            Player_SetMotionPair(p, HuntingHorn_State00_VarMotions[variant].mot, HuntingHorn_State00_VarMotions[variant].blend, 0);
            break;
        case 4:
            Player_SetMotionPair(p, HuntingHorn_State00_VarMotions[variant].mot, HuntingHorn_State00_VarMotions[variant].blend, 0);
            break;
        }
        Player_SetMoveMode(p, 0, 1, 0);
        break;
    case 1:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, 0x594, 0, 8);
        }
        break;
    case 2:
        if (p->motion[0].playing == 0) {
            p->step++;
            Player_SetMotionPair(p, HuntingHorn_State00_VarMotions[variant].mot2, HuntingHorn_State00_VarMotions[variant].blend2, HuntingHorn_State00_VarMotions[variant].start2);
        }
        break;
    case 3:
        if (Player_IsLocal(p) == 1) {
            if (p->stateVar1 == 0) {
                p->stateVar0++;
                p->stateVar1 = 60;
                StageObj_K29_CreateOnPlayerDefault(p, 0xB, 1.0f);
                switch (p->motionId[0]) {
                case 0x57D:
                default:
                    HuntingHorn_AddNote(p, 0);
                    break;
                case 0x57E:
                    HuntingHorn_AddNote(p, 1);
                    break;
                case 0x57F:
                    HuntingHorn_AddNote(p, 2);
                    break;
                }
            }
            if (p->timer == 0) {
                if (p->input.pressed & 0x80) {
                    switch (p->motionId[0]) {
                    case 0x57D:
                    default:
                        Player_RequestState(p, 1, 0x13, 4);
                        break;
                    case 0x57E:
                        Player_RequestState(p, 1, 0x1A, 4);
                        break;
                    case 0x57F:
                        Player_RequestState(p, 1, 0x1B, 4);
                        break;
                    }
                } else {
                    switch (p->motionId[0]) {
                    case 0x57D:
                        for (i = 0, r = HuntingHorn_State00_Fx57D; i < 3; i++, r++) {
                            if (Motion_InFrameRange(p, 0, r->from, r->to) != 0) {
                                Player_StickTurnMove(p, 0x180, 3.0f);
                            }
                        }
                        break;
                    case 0x57E:
                        for (j = 0, r2 = HuntingHorn_State00_Fx57E; j < 3; j++, r2++) {
                            if (Motion_InFrameRange(p, 0, r2->from, r2->to) != 0) {
                                Player_StickTurnMove(p, 0x180, 1.5f);
                            }
                        }
                        break;
                    case 0x57F:
                        for (k = 0, r3 = HuntingHorn_State00_Fx57F; k < 2; k++, r3++) {
                            if (Motion_InFrameRange(p, 0, r3->from, r3->to) != 0) {
                                Player_StickTurnMove(p, 0x180, 1.5f);
                            }
                        }
                        break;
                    }
                    HuntingHorn_IdleInput(p, motion);
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
MotionBlend GreatSword_State02_Motions[8] = {
#include "game/00896DE0.inc"
};
GunMotion Bowgun_State00_AimLayers[5] = {
#include "game/00896E00.inc"
};
s16 DualSwords_State01_Motions[8] = {
#include "game/00896E20.inc"
};
u16 HuntingHorn_State00_Motions[8] = {
#include "game/00896E30.inc"
};
HornMotion HuntingHorn_State00_VarMotions[6] = {
#include "game/00896E40.inc"
};
FrameRange HuntingHorn_State00_Fx57D[4] = {
#include "game/00896E80.inc"
};
FrameRange HuntingHorn_State00_Fx57E[4] = {
#include "game/00896EA0.inc"
};
FrameRange HuntingHorn_State00_Fx57F[2] = {
#include "game/00896EC0.inc"
};
