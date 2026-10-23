/*
 * The drawn weapons' states, first part: the great sword, the bowguns (heavy
 * and light share them), the hammer, the lance, the sword and shield, the
 * dual swords, the long sword, and the hunting horn's play stance (the horn's
 * other states, the gunlance and the bow are written separately).
 *
 * Player_RunWeaponState runs the drawn weapon's
 * <Weapon>_RunState, which maps the player's state in group PL_GROUP_WEAPON
 * (the per-weapon state enums) to a handler and a variant.
 * A handler steps through pl->step: it plays its motions
 * (Player_SetMotionPair; motion ids are plain numbers), makes the attack's
 * hit object (PlayerAttack_Create with an attack id), checks the hit
 * (<Weapon>_CheckHit: a bounce, the hit slow) and in its input window reads
 * the next input (<Weapon>_CheckInput); then it chains another state or
 * returns to idle (Player_ReturnToIdle). <Weapon>_AttackInput reads the drawn
 * idle's input, <Weapon>_DrawAttack starts the attack that draws on the move.
 * The attack input is a flick of the right stick (input.stickDirPressed
 * PL_RSTICK_ANY), its direction in input.rightStickAngle; the roll button is
 * input.pressed PAD_CROSS, and input.held PAD_R1 the weapon's hold special
 * (Player_HoldSpecialInput: the guard, the charge, the demon mode...).
 */
#include "common.h"
#include "item.h"
#include "stageobj.h"
#include "em.h"
#include "pad.h"
#include "player.h"
#include "camera.h"
#include "quest.h"
#include "bone_effects.h"
#include "engine_math.h"
#include "game_util.h"
#include "vibration.h"
#include "vec_math.h"
#include "attack_objects.h"
#include "cam_shake.h"
#include "save.h"
#include "plsel.h"

/* prototypes */
extern u16 HuntingHorn_StanceMotions[];
extern HornMotion HuntingHorn_StanceEntryMotions[];
extern FrameRange HuntingHorn_Note0WalkFrames[];
extern FrameRange HuntingHorn_Note1WalkFrames[];
extern FrameRange HuntingHorn_Note2WalkFrames[];
extern MotionBlend GreatSword_OverheadBlends[];
extern GunMotion Bowgun_FireMotions[];
/* Byte 2 of each 4-byte row of sub_main's gAmmoLevels (one row per ammo type):
 * the rapid-fire shots after the first. A linker alias (game_aliases.ld), which
 * the match needs: the code indexes from it, not from gAmmoLevels. */
extern u8 gAmmoRapidShots[][4];
u8 Pouch_Add(Player *p, u16 item, s16 n);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Player_SetMotionPair(Player *, s32, s32, s32); /* unit's own declaration: the last parameter declared s32 (the definition's is s16); with the s16 the calls compile differently (the great sword's sweeps and overhead slash) */
void Player_SetPosture(Player *, u16 mode, s16 flag1, s16 flag2);
void Player_SetState(Player *, u8, u8, s32); /* unit's own declaration: declared (u8, u8, s32) as Player_TurnAndSetState passes them; with the definition's (u16, u8, u16) its call compiles differently */
u32 Player_TestFlag(void *obj, u32 flag);
extern s16 DualSwords_SideSlashMotions[][2];
/* The light bowguns' rapid-fire ammo (Bowgun_RapidFireMask): a gun's id and a
 * mask with a bit per ammo type; id 999 ends it. A linker alias into sub_main's
 * data (game_aliases.ld). */
extern GunAmmoMask gRapidFireAmmo[];
/* end of prototypes */

/* The file's constants. */
#pragma rodata on
#pragma explicit_zero_data on
char D_008DA570[16] = {
#include "game/008DA570.inc"
};
#pragma explicit_zero_data reset
char gStagePageTexts_1_5[32] = {
#include "game/008DA580.inc"
};
char gStagePageTexts_1_6[32] = {
#include "game/008DA5A0.inc"
};
char gStagePageTexts_1_7[48] = {
#include "game/008DA5C0.inc"
};
char gStagePageTexts_1_8[48] = {
#include "game/008DA5F0.inc"
};
char gStagePageTexts_1_10[48] = {
#include "game/008DA620.inc"
};
char gStagePageTexts_1_11[32] = {
#include "game/008DA650.inc"
};
char gStagePageTexts_1_12[32] = {
#include "game/008DA670.inc"
};
char D_008DA690[48] = {
#include "game/008DA690.inc"
};
char D_008DA6C0[32] = {
#include "game/008DA6C0.inc"
};
char D_008DA6E0[32] = {
#include "game/008DA6E0.inc"
};
char gStagePageTexts_2_0[32] = {
#include "game/008DA700.inc"
};
char D_008DA720[48] = {
#include "game/008DA720.inc"
};
char gStagePageTexts_2_3[32] = {
#include "game/008DA750.inc"
};
char gStagePageTexts_2_4[32] = {
#include "game/008DA770.inc"
};
char gStagePageTexts_2_5[32] = {
#include "game/008DA790.inc"
};
char gStagePageTexts_2_6[32] = {
#include "game/008DA7B0.inc"
};
char gStagePageTexts_2_7[16] = {
#include "game/008DA7D0.inc"
};
char gStagePageTexts_2_8[48] = {
#include "game/008DA7E0.inc"
};
char gStagePageTexts_2_9[32] = {
#include "game/008DA810.inc"
};
char D_008DA830[48] = {
#include "game/008DA830.inc"
};
char D_008DA860[48] = {
#include "game/008DA860.inc"
};
char D_008DA890[32] = {
#include "game/008DA890.inc"
};
char D_008DA8B0[48] = {
#include "game/008DA8B0.inc"
};
char D_008DA8E0[32] = {
#include "game/008DA8E0.inc"
};
char gStagePageTexts_3_0[48] = {
#include "game/008DA900.inc"
};
char gStagePageTexts_3_1[32] = {
#include "game/008DA930.inc"
};
char gStagePageTexts_3_3[48] = {
#include "game/008DA950.inc"
};
char gStagePageTexts_3_4[48] = {
#include "game/008DA980.inc"
};
char gStagePageTexts_3_5[48] = {
#include "game/008DA9B0.inc"
};
char gStagePageTexts_3_6[32] = {
#include "game/008DA9E0.inc"
};
char gStagePageTexts_4_0[48] = {
#include "game/008DAA00.inc"
};
char gStagePageTexts_4_1[32] = {
#include "game/008DAA30.inc"
};
char gStagePageTexts_4_2[32] = {
#include "game/008DAA50.inc"
};
char gStagePageTexts_4_3[32] = {
#include "game/008DAA70.inc"
};
char gStagePageTexts_4_5[48] = {
#include "game/008DAA90.inc"
};
char gStagePageTexts_4_6[32] = {
#include "game/008DAAC0.inc"
};
char gStagePageTexts_4_8[48] = {
#include "game/008DAAE0.inc"
};
char gStagePageTexts_4_9[32] = {
#include "game/008DAB10.inc"
};
char gStagePageTexts_4_10[32] = {
#include "game/008DAB30.inc"
};
char gStagePageTexts_4_11[32] = {
#include "game/008DAB50.inc"
};
char D_008DAB70[48] = {
#include "game/008DAB70.inc"
};
char gStagePageTexts_5_0[48] = {
#include "game/008DABA0.inc"
};
char gStagePageTexts_5_1[32] = {
#include "game/008DABD0.inc"
};
char gStagePageTexts_5_2[48] = {
#include "game/008DABF0.inc"
};
char gStagePageTexts_5_3[32] = {
#include "game/008DAC20.inc"
};
char gStagePageTexts_5_5[48] = {
#include "game/008DAC40.inc"
};
char gStagePageTexts_5_6[48] = {
#include "game/008DAC70.inc"
};
char gStagePageTexts_5_7[32] = {
#include "game/008DACA0.inc"
};
char gStagePageTexts_5_8[16] = {
#include "game/008DACC0.inc"
};
char gStagePageTexts_5_10[48] = {
#include "game/008DACD0.inc"
};
char gStagePageTexts_5_11[32] = {
#include "game/008DAD00.inc"
};
char D_008DAD20[48] = {
#include "game/008DAD20.inc"
};
char D_008DAD50[32] = {
#include "game/008DAD50.inc"
};
char D_008DAD70[32] = {
#include "game/008DAD70.inc"
};
char D_008DAD90[32] = {
#include "game/008DAD90.inc"
};
char D_008DADB0[48] = {
#include "game/008DADB0.inc"
};
char D_008DADE0[32] = {
#include "game/008DADE0.inc"
};
char gStagePageTexts_6_0[32] = {
#include "game/008DAE00.inc"
};
char gStagePageTexts_6_2[48] = {
#include "game/008DAE20.inc"
};
char gStagePageTexts_6_3[32] = {
#include "game/008DAE50.inc"
};
char gStagePageTexts_6_4[48] = {
#include "game/008DAE70.inc"
};
char gStagePageTexts_6_5[32] = {
#include "game/008DAEA0.inc"
};
char D_008DAEC0[48] = {
#include "game/008DAEC0.inc"
};
char gStagePageTexts_7_0[32] = {
#include "game/008DAEF0.inc"
};
char gStagePageTexts_7_1[32] = {
#include "game/008DAF10.inc"
};
char gStagePageTexts_7_2[16] = {
#include "game/008DAF30.inc"
};
char gStagePageTexts_7_4[64] = {
#include "game/008DAF40.inc"
};
char gStagePageTexts_7_5[48] = {
#include "game/008DAF80.inc"
};
char gStagePageTexts_7_6[64] = {
#include "game/008DAFB0.inc"
};
char gStagePageTexts_7_8[32] = {
#include "game/008DAFF0.inc"
};
char gStagePageTexts_7_9[32] = {
#include "game/008DB010.inc"
};
char gStagePageTexts_7_10[40] = {
#include "game/008DB030.inc"
};
char gStagePageTexts_7_11[12] = {
#include "game/008DB058.inc"
};
char D_008DB070[32] = {
#include "game/008DB070.inc"
};
char D_008DB090[48] = {
#include "game/008DB090.inc"
};
char D_008DB0C0[48] = {
#include "game/008DB0C0.inc"
};
char gStagePageTexts_8_0[32] = {
#include "game/008DB0F0.inc"
};
char gStagePageTexts_8_1[32] = {
#include "game/008DB110.inc"
};
char gStagePageTexts_8_3[32] = {
#include "game/008DB130.inc"
};
char gStagePageTexts_8_4[32] = {
#include "game/008DB150.inc"
};
char gStagePageTexts_8_6[40] = {
#include "game/008DB170.inc"
};
char gStagePageTexts_8_7[12] = {
#include "game/008DB198.inc"
};
char D_008DB1B0[32] = {
#include "game/008DB1B0.inc"
};
char D_008DB1D0[48] = {
#include "game/008DB1D0.inc"
};
char gStagePageTexts_9_0[32] = {
#include "game/008DB200.inc"
};
char gStagePageTexts_9_2[32] = {
#include "game/008DB220.inc"
};
char gStagePageTexts_9_3[32] = {
#include "game/008DB240.inc"
};
char gStagePageTexts_9_5[32] = {
#include "game/008DB260.inc"
};
char gStagePageTexts_9_6[32] = {
#include "game/008DB280.inc"
};
char gStagePageTexts_9_8[32] = {
#include "game/008DB2A0.inc"
};
char gStagePageTexts_9_9[32] = {
#include "game/008DB2C0.inc"
};
char gStagePageTexts_9_10[48] = {
#include "game/008DB2E0.inc"
};
char D_008DB310[32] = {
#include "game/008DB310.inc"
};
char gStagePageTexts_10_1[32] = {
#include "game/008DB330.inc"
};
char D_008DB350[16] = {
#include "game/008DB350.inc"
};
char gStagePageTexts_10_4[48] = {
#include "game/008DB360.inc"
};
char gStagePageTexts_10_6[32] = {
#include "game/008DB390.inc"
};
char gStagePageTexts_10_7[32] = {
#include "game/008DB3B0.inc"
};
char gStagePageTexts_10_9[32] = {
#include "game/008DB3D0.inc"
};
char gStagePageTexts_10_10[48] = {
#include "game/008DB3F0.inc"
};
char gStagePageTexts_10_11[32] = {
#include "game/008DB420.inc"
};
char gStagePageTexts_11_1[32] = {
#include "game/008DB440.inc"
};
char gStagePageTexts_11_4[32] = {
#include "game/008DB460.inc"
};
char gStagePageTexts_11_5[48] = {
#include "game/008DB480.inc"
};
char gStagePageTexts_11_7[32] = {
#include "game/008DB4B0.inc"
};
char gStagePageTexts_11_8[32] = {
#include "game/008DB4D0.inc"
};
char gStagePageTexts_11_9[32] = {
#include "game/008DB4F0.inc"
};
char gStagePageTexts_11_11[32] = {
#include "game/008DB510.inc"
};
char gStagePageTexts_11_12[32] = {
#include "game/008DB530.inc"
};
char gStagePageTexts_12_1[32] = {
#include "game/008DB550.inc"
};
char gStagePageTexts_12_4[48] = {
#include "game/008DB570.inc"
};
char D_008DB5A0[32] = {
#include "game/008DB5A0.inc"
};
char gStagePageTexts_12_7[32] = {
#include "game/008DB5C0.inc"
};
char gStagePageTexts_12_9[48] = {
#include "game/008DB5E0.inc"
};
char gStagePageTexts_12_11[32] = {
#include "game/008DB610.inc"
};
char D_008DB630[48] = {
#include "game/008DB630.inc"
};
char D_008DB660[48] = {
#include "game/008DB660.inc"
};
char D_008DB690[48] = {
#include "game/008DB690.inc"
};
char D_008DB6C0[32] = {
#include "game/008DB6C0.inc"
};
char D_008DB6E0[32] = {
#include "game/008DB6E0.inc"
};
char gStagePageTexts_13_0[32] = {
#include "game/008DB700.inc"
};
char gStagePageTexts_13_1[32] = {
#include "game/008DB720.inc"
};
char gStagePageTexts_13_2[16] = {
#include "game/008DB740.inc"
};
char gStagePageTexts_13_3[48] = {
#include "game/008DB750.inc"
};
char gStagePageTexts_13_4[32] = {
#include "game/008DB780.inc"
};
char gStagePageTexts_13_5[32] = {
#include "game/008DB7A0.inc"
};
char D_008DB7C0[32] = {
#include "game/008DB7C0.inc"
};
char D_008DB7E0[48] = {
#include "game/008DB7E0.inc"
};
char gStagePageTexts_14_0[32] = {
#include "game/008DB810.inc"
};
char gStagePageTexts_14_1[48] = {
#include "game/008DB830.inc"
};
char gStagePageTexts_14_2[48] = {
#include "game/008DB860.inc"
};
char gStagePageTexts_14_3[40] = {
#include "game/008DB890.inc"
};
char gStagePageTexts_14_4[12] = {
#include "game/008DB8B8.inc"
};
char gStagePageTexts_14_6[32] = {
#include "game/008DB8D0.inc"
};
char gStagePageTexts_14_7[48] = {
#include "game/008DB8F0.inc"
};
char gStagePageTexts_14_8[32] = {
#include "game/008DB920.inc"
};
char gStagePageTexts_14_9[16] = {
#include "game/008DB940.inc"
};
char gStagePageTexts_14_10[48] = {
#include "game/008DB950.inc"
};
char gStagePageTexts_15_0[48] = {
#include "game/008DB980.inc"
};
char gStagePageTexts_15_1[32] = {
#include "game/008DB9B0.inc"
};
char gStagePageTexts_15_3[48] = {
#include "game/008DB9D0.inc"
};
char gStagePageTexts_15_4[48] = {
#include "game/008DBA00.inc"
};
char gStagePageTexts_15_5[32] = {
#include "game/008DBA30.inc"
};
char gStagePageTexts_15_6[32] = {
#include "game/008DBA50.inc"
};
char gStagePageTexts_15_8[48] = {
#include "game/008DBA70.inc"
};
char gStagePageTexts_15_9[48] = {
#include "game/008DBAA0.inc"
};
char gStagePageTexts_15_10[48] = {
#include "game/008DBAD0.inc"
};
char gStagePageTexts_15_11[32] = {
#include "game/008DBB00.inc"
};
char D_008DBB20[48] = {
#include "game/008DBB20.inc"
};
char D_008DBB50[48] = {
#include "game/008DBB50.inc"
};
char gStagePageTexts_16_0[32] = {
#include "game/008DBB80.inc"
};
char gStagePageTexts_16_1[48] = {
#include "game/008DBBA0.inc"
};
char gStagePageTexts_16_2[48] = {
#include "game/008DBBD0.inc"
};
char gStagePageTexts_16_3[32] = {
#include "game/008DBC00.inc"
};
char gStagePageTexts_16_4[48] = {
#include "game/008DBC20.inc"
};
char gStagePageTexts_16_5[48] = {
#include "game/008DBC50.inc"
};
char gStagePageTexts_16_6[48] = {
#include "game/008DBC80.inc"
};
char gStagePageTexts_16_7[48] = {
#include "game/008DBCB0.inc"
};
char gStagePageTexts_16_8[48] = {
#include "game/008DBCE0.inc"
};
char gStagePageTexts_16_9[32] = {
#include "game/008DBD10.inc"
};
char D_008DBD30[48] = {
#include "game/008DBD30.inc"
};
char D_008DBD60[32] = {
#include "game/008DBD60.inc"
};
char gStagePageTexts_17_0[32] = {
#include "game/008DBD80.inc"
};
char gStagePageTexts_17_1[64] = {
#include "game/008DBDA0.inc"
};
char gStagePageTexts_17_4[48] = {
#include "game/008DBDE0.inc"
};
char gStagePageTexts_17_5[32] = {
#include "game/008DBE10.inc"
};
char gStagePageTexts_17_6[32] = {
#include "game/008DBE30.inc"
};
char gStagePageTexts_17_7[32] = {
#include "game/008DBE50.inc"
};
char D_008DBE70[48] = {
#include "game/008DBE70.inc"
};
char gStagePageTexts_18_0[32] = {
#include "game/008DBEA0.inc"
};
char gStagePageTexts_18_1[48] = {
#include "game/008DBEC0.inc"
};
char gStagePageTexts_18_2[40] = {
#include "game/008DBEF0.inc"
};
char gStagePageTexts_18_4[12] = {
#include "game/008DBF18.inc"
};
char gStagePageTexts_18_5[48] = {
#include "game/008DBF30.inc"
};
char gStagePageTexts_18_6[32] = {
#include "game/008DBF60.inc"
};
char gStagePageTexts_18_7[48] = {
#include "game/008DBF80.inc"
};
char gStagePageTexts_18_8[48] = {
#include "game/008DBFB0.inc"
};
char D_008DBFE0[48] = {
#include "game/008DBFE0.inc"
};
char gStagePageTexts_19_0[40] = {
#include "game/008DC010.inc"
};
char gStagePageTexts_19_1[12] = {
#include "game/008DC038.inc"
};
char gStagePageTexts_19_2[48] = {
#include "game/008DC050.inc"
};
char gStagePageTexts_19_3[48] = {
#include "game/008DC080.inc"
};
char gStagePageTexts_19_4[32] = {
#include "game/008DC0B0.inc"
};
char gStagePageTexts_19_5[48] = {
#include "game/008DC0D0.inc"
};
char gStagePageTexts_19_7[48] = {
#include "game/008DC100.inc"
};
char gStagePageTexts_19_8[48] = {
#include "game/008DC130.inc"
};
char D_008DC160[48] = {
#include "game/008DC160.inc"
};
char D_008DC190[32] = {
#include "game/008DC190.inc"
};
char D_008DC1B0[32] = {
#include "game/008DC1B0.inc"
};
char D_008DC1D0[48] = {
#include "game/008DC1D0.inc"
};
char gStagePageTexts_20_0[32] = {
#include "game/008DC200.inc"
};
char gStagePageTexts_20_1[32] = {
#include "game/008DC220.inc"
};
char gStagePageTexts_20_3[48] = {
#include "game/008DC240.inc"
};
char gStagePageTexts_20_4[32] = {
#include "game/008DC270.inc"
};
char gStagePageTexts_20_5[32] = {
#include "game/008DC290.inc"
};
char D_008DC2B0[64] = {
#include "game/008DC2B0.inc"
};
char gStagePageTexts_21_0[32] = {
#include "game/008DC2F0.inc"
};
char gStagePageTexts_21_1[48] = {
#include "game/008DC310.inc"
};
char gStagePageTexts_21_2[48] = {
#include "game/008DC340.inc"
};
char gStagePageTexts_21_3[32] = {
#include "game/008DC370.inc"
};
char gStagePageTexts_21_4[48] = {
#include "game/008DC390.inc"
};
char gStagePageTexts_21_5[32] = {
#include "game/008DC3C0.inc"
};
char gStagePageTexts_21_6[32] = {
#include "game/008DC3E0.inc"
};
char gStagePageTexts_21_7[64] = {
#include "game/008DC400.inc"
};
char gStagePageTexts_21_8[48] = {
#include "game/008DC440.inc"
};
char D_008DC470[32] = {
#include "game/008DC470.inc"
};
char D_008DC490[32] = {
#include "game/008DC490.inc"
};
char D_008DC4B0[32] = {
#include "game/008DC4B0.inc"
};
char D_008DC4D0[32] = {
#include "game/008DC4D0.inc"
};
char gStagePageTexts_22_0[48] = {
#include "game/008DC4F0.inc"
};
char gStagePageTexts_22_2[48] = {
#include "game/008DC520.inc"
};
char gStagePageTexts_22_3[32] = {
#include "game/008DC550.inc"
};
char gStagePageTexts_22_4[32] = {
#include "game/008DC570.inc"
};
char gStagePageTexts_22_6[48] = {
#include "game/008DC590.inc"
};
char gStagePageTexts_22_7[48] = {
#include "game/008DC5C0.inc"
};
char gStagePageTexts_22_8[32] = {
#include "game/008DC5F0.inc"
};
char gStagePageTexts_23_0[48] = {
#include "game/008DC610.inc"
};
char gStagePageTexts_23_1[48] = {
#include "game/008DC640.inc"
};
char gStagePageTexts_23_4[48] = {
#include "game/008DC670.inc"
};
char gStagePageTexts_23_6[32] = {
#include "game/008DC6A0.inc"
};
char gStagePageTexts_23_7[48] = {
#include "game/008DC6C0.inc"
};
char gStagePageTexts_23_8[32] = {
#include "game/008DC6F0.inc"
};
char D_008DC710[32] = {
#include "game/008DC710.inc"
};
char D_008DC730[32] = {
#include "game/008DC730.inc"
};
char gStagePageTexts_23_12[48] = {
#include "game/008DC750.inc"
};
char gStagePageTexts_24_0[48] = {
#include "game/008DC780.inc"
};
char gStagePageTexts_24_1[32] = {
#include "game/008DC7B0.inc"
};
char gStagePageTexts_24_4[32] = {
#include "game/008DC7D0.inc"
};
char gStagePageTexts_24_6[32] = {
#include "game/008DC7F0.inc"
};
char gStagePageTexts_24_7[48] = {
#include "game/008DC810.inc"
};
char gStagePageTexts_24_8[32] = {
#include "game/008DC840.inc"
};
char gStagePageTexts_24_12[32] = {
#include "game/008DC860.inc"
};
char D_008DC880[48] = {
#include "game/008DC880.inc"
};
char gStagePageTexts_25_3[48] = {
#include "game/008DC8B0.inc"
};
char gStagePageTexts_25_5[32] = {
#include "game/008DC8E0.inc"
};
char gStagePageTexts_25_7[32] = {
#include "game/008DC900.inc"
};
char gStagePageTexts_25_8[48] = {
#include "game/008DC920.inc"
};
char gStagePageTexts_25_9[32] = {
#include "game/008DC950.inc"
};
char gStagePageTexts_25_11[48] = {
#include "game/008DC970.inc"
};
char gStagePageTexts_25_12[32] = {
#include "game/008DC9A0.inc"
};
char gStagePageTexts_26_3[48] = {
#include "game/008DC9C0.inc"
};
char gStagePageTexts_26_6[32] = {
#include "game/008DC9F0.inc"
};
char gStagePageTexts_26_10[32] = {
#include "game/008DCA10.inc"
};
char D_008DCA30[32] = {
#include "game/008DCA30.inc"
};
char D_008DCA50[32] = {
#include "game/008DCA50.inc"
};
char D_008DCA70[32] = {
#include "game/008DCA70.inc"
};
char D_008DCA90[32] = {
#include "game/008DCA90.inc"
};
char D_008DCAB0[32] = {
#include "game/008DCAB0.inc"
};
char D_008DCAD0[16] = {
#include "game/008DCAD0.inc"
};
char D_008DCAE0[32] = {
#include "game/008DCAE0.inc"
};
char gStagePageTexts_27_0[32] = {
#include "game/008DCB00.inc"
};
char gStagePageTexts_27_1[32] = {
#include "game/008DCB20.inc"
};
char gStagePageTexts_27_2[16] = {
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
        pl->noFlinchTimer = 60;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, 1406, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 2);
            Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
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
                    Player_SetMotionPair(pl, 1417, 4, 0);
                    break;
                case 2:
                    Player_SetMotionPair(pl, 1421, 4, 0);
                    break;
                case 3:
                case 4:
                    pl->rot[1] = (u16) (pl->rot[1] + ANGLE_TURN / 8);
                    pl->targetYaw = pl->rot[1];
                    if (variant == 3) {
                        Player_SetMotionPair(pl, 1423, 0, 0);
                    } else {
                        Player_SetMotionPair(pl, 1427, 0, 0);
                    }
                    break;
                }
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    switch (variant) {
                    case 2:
                        Player_SetMotionPair(pl, 1406, 2, 44);
                        break;
                    case 4:
                        Player_SetMotionPair(pl, 1406, 0, 44);
                        break;
                    default:
                        Player_SetMotionPair(pl, 1406, 2, 26);
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
                WeaponTrail_Create(pl, 15, WTRAIL_GREAT_SWORD);
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
            WeaponTrail_Create(pl, 28, WTRAIL_GREAT_SWORD);
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
        pl->noFlinchTimer = 60;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, 1407, 4, 0);
            pl->motionTime = 0;
            PlayerAttack_Create(pl, 1);
            Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
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
                    Player_SetMotionPair(pl, 1416, 4, 0);
                    break;
                case 2:
                    Player_SetMotionPair(pl, 1419, 4, 0);
                    break;
                case 3:
                case 4:
                    pl->rot[1] = (u16) (pl->rot[1] - ANGLE_TURN / 8);
                    pl->targetYaw = pl->rot[1];
                    if (variant == 3) {
                        Player_SetMotionPair(pl, 1422, 0, 0);
                    } else {
                        Player_SetMotionPair(pl, 1425, 0, 0);
                    }
                    break;
                }
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    if (variant == 1) {
                        Player_SetMotionPair(pl, 1407, 2, 64);
                    } else {
                        Player_SetMotionPair(pl, 1407, 0, 66);
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
                WeaponTrail_Create(pl, 15, WTRAIL_GREAT_SWORD);
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
                ImpactRocks_Create(pl, IMPACT_ROCKS_LOW, point, IMPACT_ROCKS_MESH_DEBRIS);
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
            WeaponTrail_Create(pl, 28, WTRAIL_GREAT_SWORD);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 60;
        pl->weaponOut = 1;
        pl->unk417 = 1;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, 1401, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            break;
        case 1:
            pl->step++;
            Player_SetMotionPair(pl, 1410, 2, 0);
            pl->unk417 = 0;
            break;
        case 2:
            Player_SetMotionPair(pl, 1420, 4, 0);
            break;
        case 3:
            Player_SetMotionPair(pl, 1418, 4, 0);
            break;
        case 4:
            pl->step++;
            Player_SetMotionPair(pl, 1405, 4, 0);
            break;
        case 5:
            pl->rot[1] += ANGLE_TURN / 8;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, 1424, 0, 0);
            break;
        case 6:
            pl->rot[1] -= ANGLE_TURN / 8;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, 1426, 0, 0);
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
                Player_SetMotionPair(pl, 1401, 2, 50);
            }
            break;
        case 3:
            if (Motion_ReachedFrame(pl, 0, 24.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, 1429, 2, 0);
            }
            break;
        case 5:
            if (Motion_ReachedFrame(pl, 0, 26.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, 1430, 2, 0);
            }
            break;
        case 6:
            if (pl->motion[0].playing == 0) {
                pl->step++;
                Player_SetMotionPair(pl, 1401, 0, 52);
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
                Player_SetMotionPair(pl, 1428, GreatSword_OverheadBlends[variant].blend, GreatSword_OverheadBlends[variant].start);
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
                Player_SetMotionPair(pl, 1428, GreatSword_OverheadBlends[variant].blend, GreatSword_OverheadBlends[variant].start);
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
                WeaponTrail_Create(pl, 15, WTRAIL_GREAT_SWORD);
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
                ImpactRocks_Create(pl, IMPACT_ROCKS_LOW, point, IMPACT_ROCKS_MESH_DEBRIS);
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
            WeaponTrail_Create(pl, 28, WTRAIL_GREAT_SWORD);
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
                WeaponTrail_Create(pl, 15, WTRAIL_GREAT_SWORD);
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
                ImpactRocks_Create(pl, IMPACT_ROCKS_LOW, point, IMPACT_ROCKS_MESH_DEBRIS);
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
            WeaponTrail_Create(pl, 28, WTRAIL_GREAT_SWORD);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, 1404, 0, 0);
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
            Player_ReturnToIdle(pl, 0, 10, 0);
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
        Player_SetMotionPair(pl, 1411, 0, 0);
        PlayerAttack_Create(pl, 4U);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->weaponOut = 1;
        pl->unk417 = 1;
        pl->weaponTimer = 0;
        pl->timer = 90;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, 1401, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, 1431, 2, 0);
            break;
        case 2:
            Player_SetMotionPair(pl, 1420, 4, 0);
            break;
        case 3:
            pl->step++;
            Player_SetMotionPair(pl, 1418, 4, 0);
            break;
        case 5:
            pl->step++;
            pl->rot[1] += ANGLE_TURN / 8;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, 1424, 0, 0);
            break;
        case 6:
            pl->rot[1] -= ANGLE_TURN / 8;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, 1426, 0, 0);
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
                Player_SetMotionPair(pl, 1401, 0, 56);
            }
            break;
        case 2:
            if (pl->motion[0].playing == 0) {
                pl->step++;
                Player_SetMotionPair(pl, 1401, 2, 50);
            }
            break;
        case 6:
            if (pl->motion[0].playing == 0) {
                pl->step++;
                Player_SetMotionPair(pl, 1401, 0, 52);
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
                    PlayerFx_CreateOnPlayerDefault(pl, 8, 1.0f);
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
 * 120 or 130, and hit object 3, 91, 92 or 93.
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
        PlayerAttack_Create(pl, 91);
        break;
    case 2:
        pl->attackPercent = 120;
        PlayerAttack_Create(pl, 92);
        break;
    case 3:
        pl->attackPercent = 130;
        PlayerAttack_Create(pl, 93);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 60;
        pl->unk417 = 1;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 1428, 2, 0);
            Player_UpdateMotionTime(pl, 0);
            GreatSword_SetChargeAttack(pl);
            break;
        case 1:
            Player_SetMotionPair(pl, 1429, 0, 0);
            Player_UpdateMotionTime(pl, 0);
            break;
        case 2:
            Player_SetMotionPair(pl, 1430, 0, 0);
            Player_UpdateMotionTime(pl, 0);
            break;
        }
        if (pl->chargeLevel >= 2) {
            PlayerFx_CreateOnPlayerDefault(pl, 4, 1.0f);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_UpdateMotionTime(pl, 0);
            if (variant == 1) {
                Player_SetMotionPair(pl, 1428, 2, 8);
            } else {
                Player_SetMotionPair(pl, 1428, 0, 10);
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
                WeaponTrail_Create(pl, 15, WTRAIL_GREAT_SWORD);
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
                    PlayerFx_CreateOnPlayerDefault(pl, 10, 1.0f);
                } else {
                    ImpactRocks_Create(pl, IMPACT_ROCKS_LOW, point, IMPACT_ROCKS_MESH_DEBRIS);
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
            WeaponTrail_Create(pl, 28, WTRAIL_GREAT_SWORD);
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
                WeaponTrail_Create(pl, 15, WTRAIL_GREAT_SWORD);
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
                ImpactRocks_Create(pl, IMPACT_ROCKS_LOW, point, IMPACT_ROCKS_MESH_DEBRIS);
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
            WeaponTrail_Create(pl, 28, WTRAIL_GREAT_SWORD);
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
        if (dir < 18204) {
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
    if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75) {
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
        if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (u16) (pl->input.stickAngle + 0x2AAB);
                if (dir < 23666) {
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
        if (motA == 1409) {
            pl->partAnimMotion[0] = 1408;
            pl->partAnimOn[0] = 0;
        } else {
            pl->partAnimMotion[0] = motA;
        }
        pl->partAnimMotion[1] = motA + 100;
        pl->partAnimWeight[0] = pl->aimWeight;
        pl->partAnimWeight[1] = pl->aimWeight;
    } else {
        if (motB == 1410) {
            pl->partAnimMotion[0] = 1408;
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
 * rapid fire. Fires at frame 8 and spends the round (kept with SKILL_FREE_SHOTS,
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
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
                Bowgun_FireShot(pl, 0, 14, 1.0f);
            } else {
                Bowgun_FireShot(pl, 0, 14, 0.5f);
            }
            if (pl->stateVar0 <= 0) {
                pl->stateVar0++;
                if (Player_HasSkill(pl, SKILL_FREE_SHOTS) == 0 || variant == 1) {
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
            GunFx_CreateReload(pl, 2);
        }
        if (pl->motion[0].playing == 0) {
            Bowgun_SetAimLayers(pl, 1005, 1006);
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
        Bowgun_SetAimLayers(pl, 1414, 1415);
        Player_SetMotionPair(pl, 1404, 0, 0);
        pl->roundsLoaded = pl->clipSize;
        if (pl->ammoCount < pl->roundsLoaded) {
            pl->roundsLoaded = pl->ammoCount;
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
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
            Player_SetMotionPair(pl, 1422, 0, 0);
        } else {
            Player_SetMotionPair(pl, 1423, 0, 0);
        }
        pl->roundsLoaded = pl->clipSize;
        if (pl->ammoCount < pl->roundsLoaded) {
            pl->roundsLoaded = pl->ammoCount;
        }
        pl->stateVar0 = Bowgun_ReloadLevel(pl, pl->ammoType);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Bowgun_SetAimLayers(pl, 1414, 1415);
            Player_SetMotionPair(pl, 1404, 0, 30);
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
        Bowgun_SetAimLayers(pl, 1416, 1417);
        Player_SetMotionPair(pl, 1412, 0, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdleKeepParts(pl, 0, 0, 0);
        }
        break;
    }
}

/* BOWGUN_MELEE: one motion with hit object 29, then idle. */
void Bowgun_StateMelee(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 1413, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        PlayerAttack_Create(pl, 29U);
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

    if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75) {
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
        Player_SetMotionPair(pl, 1401, 4, 0);
        PlayerAttack_Create(pl, 6U);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 60;
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->timer = 0;
        pl->stateVar1 = 0;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, 1402, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
            PlayerAttack_Create(pl, 7U);
            pl->noFlinchTimer = 27;
            break;
        case 1:
            stateVar0 = pl->stateVar0;
            switch (stateVar0) {
            case 0:
                pl->stateVar0 = stateVar0 + 1;
                Player_UpdateMotionTime(pl, 0);
                Player_SetMotionPair(pl, 1409, 4, 0);
                Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
                pl->weaponOut = 1;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 1402, 0, 16);
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
 * HAMMER_COMBO_2 (hit object 33). A flick in frames 30-60 queues
 * HAMMER_COMBO_3 at the motion's end; else a recovery with the evade window.
 * variant is always 0.
 */
void Hammer_StateCombo2(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMotionPair(pl, 1412, 2, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 0;
        PlayerAttack_Create(pl, 33U);
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
                Player_SetMotionPair(pl, 1402, 4, 74);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->timer = 0;
        switch (variant) {
        case 0:
        case 3:
            pl->step = 2;
            if (variant == 3) {
                Player_SetMotionPair(pl, PL_MOT_HAMMER_CHARGE_START, 6, 12);
            } else {
                Player_SetMotionPair(pl, PL_MOT_HAMMER_CHARGE_START, 4, 0);
            }
            pl->weaponTimer = 0;
            Effect06_SpawnAtBone(pl, 6, 0, 2, 4.0f);
            pl->noFlinchTimer = 60;
            break;
        case 1:
            pl->step = 3;
            Player_SetMotionPair(pl, PL_MOT_HAMMER_CHARGE_HOLD, 8, 0);
            goto charge;
        case 2:
            pl->step = 1;
            Player_SetMotionPair(pl, PL_MOT_HAMMER_CHARGE_ENTRY, 4, 0);
            pl->weaponTimer = 0;
            pl->noFlinchTimer = 60;
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_HAMMER_CHARGE_START, 0, 14);
            pl->weaponTimer = 0;
            Effect06_SpawnAtBone(pl, 6, 0, 2, 4.0f);
            pl->noFlinchTimer = 60;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_HAMMER_CHARGE_HOLD, 4, 0);
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
        Player_SetMotionPair(pl, PL_MOT_HAMMER_CHARGE_WALK, 8, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
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
        Player_SetMotionPair(pl, 1403, 6, 0);
        pl->stateVar0 = pl->chargeLevel + 2;
        if (pl->stateVar0 >= 5) {
            pl->stateVar0 = 5;
        }
        pl->stateVar1 = 0;
        pl->timer = 0;
        pl->targetYaw = pl->rot[1];
        PlayerAttack_Create(pl, 12);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 60;
        break;
    case 1:
        pl->noFlinchTimer = 2;
        if (Motion_PassedFrame(pl, 0, 44.0f) != 0) {
            pl->timer++;
            if (--pl->stateVar0 <= 0) {
                pl->step++;
                Player_SetMotionPair(pl, 1404, 0, 0);
                pl->stateRand += 3;
                PlayerAttack_Create(pl, 15);
                break;
            }
            switch (pl->timer) {
            case 0:
                break;
            case 1:
            case 2:
                if (pl->stateVar1 != 0) {
                    if (pl->input.stickDir & PAD_LEFT) {
                        pl->rot[1] += ANGLE_TURN / 8;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->input.stickDir & PAD_RIGHT) {
                        pl->rot[1] -= ANGLE_TURN / 8;
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
                        pl->rot[1] += ANGLE_TURN / 8;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->input.stickDir & PAD_RIGHT) {
                        pl->rot[1] -= ANGLE_TURN / 8;
                        pl->targetYaw = pl->rot[1];
                    }
                    Player_RequestState(pl, PL_GROUP_WEAPON, HAMMER_SPIN_FINISH, PL_SET_SNAP_POS);
                }
                break;
            }
        } else {
            if (Player_IsLocal(pl) == 1) {
                if (pl->input.stickDir & PAD_LEFT) {
                    pl->targetYaw += 192;
                } else if (pl->input.stickDir & PAD_RIGHT) {
                    pl->targetYaw -= 192;
                }
                if (pl->input.stickDirPressed & PAD_RSTICK_UP) {
                    pl->stateVar1++;
                }
            }
            if (Motion_PassedFrame(pl, 0, 26.0f) != 0) {
                PlayerAttack_Create(pl, 13);
            }
            if (Motion_PassedFrame(pl, 0, 52.0f) != 0) {
                PlayerAttack_Create(pl, 14);
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
            Player_SetMotionPair(pl, 1405, 2, 0);
        } else {
            Player_SetMotionPair(pl, 1405, 4, 0);
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        PlayerAttack_Create(pl, 8);
        pl->noFlinchTimer = 60;
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
            Player_SetMotionPair(pl, 1407, 0, 0);
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
 * HAMMER_CHARGE_RELEASE_2: two hits (10, then 11 at frame 74), an impact at
 * frame 80; evade window frames 84-170.
 */
void Hammer_StateChargeRelease2(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 1406, 2, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        PlayerAttack_Create(pl, 10U);
        pl->noFlinchTimer = 60;
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 74.0f) != 0) {
            PlayerAttack_Create(pl, 11U);
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
 * HAMMER_DRAW_ATTACK: draws into a swing (hit object 34 at frame 56). The
 * charge button held at frame 80 charges again; evade from frame 88.
 */
void Hammer_StateDrawAttack(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, 1010, 2, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->weaponOut = 1;
        pl->unk417 = 1;
        pl->noFlinchTimer = 60;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1405, 2, 46);
            Player_UpdateMotionTime(pl, 0);
            pl->unk417 = 1;
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 56.0f) != 0) {
            PlayerAttack_Create(pl, 34U);
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
 * The finishing swing (hit object 32). variant: 0 HAMMER_SPIN_FINISH, 1
 * HAMMER_COMBO_3 (a lead-in motion first). Evade window frames 102-132.
 */
void Hammer_StateFinisher(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 28;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 1411, 0, 0);
            PlayerAttack_Create(pl, 32U);
            break;
        case 1:
            Player_SetMotionPair(pl, 1413, 0, 0);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1411, 0, 32);
            PlayerAttack_Create(pl, 32U);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, 1408, 0, 0);
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
            Player_ReturnToIdle(pl, 0, 10, 0);
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
    if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75)) {
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
        if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75)) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (pl->input.stickAngle + ANGLE_TURN / 4) & 0xFFFF;
                if (dir < 23666) {
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
        if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75) {
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 1405, 4, 0);
            PlayerAttack_Create(pl, 16);
            break;
        case 1:
            pl->step++;
            Player_SetMotionPair(pl, 1010, 2, 0);
            pl->weaponOut = 1;
            PlayerAttack_Create(pl, 20);
            break;
        case 2:
        case 3:
            pl->step = 2;
            Player_SetMotionPair(pl, 1405, 6, 10);
            if (variant == 3) {
                PlayerAttack_Create(pl, 36);
                break;
            }
            PlayerAttack_Create(pl, 16);
            break;
        case 4:
            pl->step = 2;
            Player_SetMotionPair(pl, 1405, 4, 10);
            PlayerAttack_Create(pl, 16);
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1405, 0, 34);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        switch (variant) {
        default:
            Player_SetMotionPair(pl, 1406, 4, 0);
            PlayerAttack_Create(pl, 28);
            break;
        case 1:
        case 2:
            Player_SetMotionPair(pl, 1406, -6, 10);
            if (variant == 2) {
                PlayerAttack_Create(pl, 35);
                break;
            }
            PlayerAttack_Create(pl, 28);
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
        Player_SetMotionPair(pl, 1416, 2, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        PlayerAttack_Create(pl, 18U);
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
 * LANCE_CHARGE_HIT; SKILL_AFFINITY_PLUS10_NO_BOUNCE and SKILL_AFFINITY_PLUS15_NO_BOUNCE skip this); a wall gives
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
            Player_SetMotionPair(pl, 1401, 4, 0);
        } else {
            pl->step = 2;
            Player_SetMotionPair(pl, 1402, 0, 6);
            PlayerAttack_Create(pl, 17);
        }
        pl->hitPending = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1402, 0, 4);
            PlayerAttack_Create(pl, 17);
        }
        break;
    case 2:
        if (Player_IsLocal(pl) == 1) {
            if (pl->input.stickDir & PAD_LEFT) {
                pl->targetYaw += 128;
            }
            if (pl->input.stickDir & PAD_RIGHT) {
                pl->targetYaw -= 128;
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
            if (Player_HasSkill(pl, SKILL_AFFINITY_PLUS10_NO_BOUNCE) == 0 && Player_HasSkill(pl, SKILL_AFFINITY_PLUS15_NO_BOUNCE) == 0) {
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
        } else if (pl->wallHitDirs & WALL_DIRS_AHEAD) {
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, 1403, 2, 0);
            break;
        }
        Player_SetMotionPair(pl, 1411, 0, 0);
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
        Player_SetMotionPair(pl, 1404, 0, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
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
        Player_SetMotionPair(pl, 1412, 2, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 60;
        PlayerAttack_Create(pl, 31U);
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
        Player_SetMotionPair(pl, 1417, 0, 0);
        CamShake_ReqA(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
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

    if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75) {
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
        if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (u16) (pl->input.stickAngle + ANGLE_TURN / 8);
                if (dir < 24576) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_SIDE_B, PL_SET_SNAP_POS);
                } else if (dir >= 24576 && dir < 0xC001) {
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, 1401, 4, 0);
            PlayerAttack_Create(pl, 21);
            break;
        case 1:
            Player_SetMotionPair(pl, 1402, 2, 0);
            PlayerAttack_Create(pl, 25);
            break;
        case 2:
            Player_SetMotionPair(pl, 1403, 2, 0);
            PlayerAttack_Create(pl, 26);
            break;
        case 3:
            Player_SetMotionPair(pl, 1401, 2, 10);
            PlayerAttack_Create(pl, 21);
            WeaponTrail_Create(pl, 8, WTRAIL_BLADE);
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
                if (dir < 16385) {
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
            PlayerAttack_Create(pl, 27);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 1404, 2, 0);
            PlayerAttack_Create(pl, 22U);
            pl->noFlinchTimer = 100;
            break;
        case 1:
            pl->step++;
            pl->weaponOut = 1;
            Player_SetMotionPair(pl, 1010, 2, 0);
            pl->noFlinchTimer = 100;
            break;
        case 2:
            pl->step = 2;
            Player_SetMotionPair(pl, 1404, 4, 0);
            PlayerAttack_Create(pl, 22U);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1404, 0, 8);
            PlayerAttack_Create(pl, 22U);
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
        Player_SetMotionPair(pl, 1405, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        PlayerAttack_Create(pl, 23U);
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
        Player_SetMotionPair(pl, 1416, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        PlayerAttack_Create(pl, 24U);
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
 * SNS_EVADE_RISING_SLASH, 2 SNS_SHIELD_BASH (hit object 100 at frame 18,
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 1406, 2, 0);
            PlayerAttack_Create(pl, 30U);
            break;
        case 1:
            pl->step = 2;
            Player_SetMotionPair(pl, 1406, 2, 8);
            PlayerAttack_Create(pl, 30U);
            break;
        case 2:
            Player_SetMotionPair(pl, 1417, 2, 0);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->stateVar0 = 0;
            Player_SetMotionPair(pl, 1406, 0, 16);
            break;
        }
        if (Motion_ReachedFrame(pl, 0, 18.0f) != 0) {
            attackMade = pl->stateVar0;
            if (attackMade == 0) {
                pl->stateVar0 = attackMade + 1;
                PlayerAttack_Create(pl, 100U);
                break;
            }
        }
        break;
    case 2:
        if ((pl->stateVar0 == 0) && (Motion_ReachedFrame(pl, 0, 26.0f) != 0) && (Motion_ReachedFrame(pl, 0, 38.0f) == 0) && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            dir = (u16) (pl->input.rightStickAngle + 0x2AAB);
            if (dir <= 16384) {
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
        Player_SetMotionPair(pl, 1408, 4, 0);
        CamShake_ReqA(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
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
        if (dir < 16385) {
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
    if ((btn & 0x40) && (pl->stamina >= 75)) {
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

    if (((Player_IsLocal(pl) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1) && (pl->itemSelect == 0) && (pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75)) {
        if (pl->input.stickDir & PAD_DPAD) {
            dir = (pl->input.stickAngle + ANGLE_TURN / 4) & 0xFFFF;
            if (dir < 23666) {
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, 1401, 2, 0);
            PlayerAttack_Create(pl, 37);
            break;
        case 1:
            Player_SetMotionPair(pl, 1402, 2, 0);
            PlayerAttack_Create(pl, 38);
            break;
        case 2:
            Player_SetMotionPair(pl, 1403, 2, 0);
            PlayerAttack_Create(pl, 40);
            break;
        case 3:
            Player_SetMotionPair(pl, 1401, 2, 10);
            PlayerAttack_Create(pl, 37);
            WeaponTrail_Create(pl, 8, WTRAIL_BLADE);
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
                if (angle <= ANGLE_TURN / 4) {
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
                PlayerAttack_Create(pl, 39);
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
                PlayerAttack_Create(pl, 41);
            }
            if (Motion_PassedFrame(pl, 0, 70.0f) != 0) {
                PlayerAttack_Create(pl, 42);
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
        Player_SetMotionPair(pl, DualSwords_SideSlashMotions[variant][0], 4, DualSwords_SideSlashMotions[variant][1]);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        PlayerAttack_CreateDual(pl, 45, variant & 0xFF);
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
                PlayerAttack_CreateDual(pl, 46, variant & 0xFF);
            }
            if (Motion_PassedFrame(pl, 0, 34.0f) != 0) {
                PlayerAttack_CreateDual(pl, 47, variant & 0xFF);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        pl->timer = 0;
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 1404, 2, 0);
            PlayerAttack_Create(pl, 43U);
            break;
        case 1:
            pl->step++;
            pl->weaponOut = 1;
            Player_SetMotionPair(pl, 1413, 4, 0);
            break;
        case 2:
            pl->step = 2;
            Player_SetMotionPair(pl, 1404, 4, 0);
            PlayerAttack_Create(pl, 43U);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1404, 2, 28);
            PlayerAttack_Create(pl, 55U);
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
                    PlayerAttack_Create(pl, 56U);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->stateVar0 = 0;
        Player_SetMotionPair(pl, 1406, 2, variant ? 4 : 0);
        PlayerAttack_Create(pl, 44);
        break;
    case 1:
        if (pl->stateVar0 == 0 && Motion_ReachedFrame(pl, 0, 26.0f) != 0 && Motion_ReachedFrame(pl, 0, 38.0f) == 0
            && (pl->input.stickDirPressed & PL_RSTICK_ANY)) {
            angle = pl->input.rightStickAngle + 0x2AAB;
            if (angle <= ANGLE_TURN / 4) {
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 1;
        switch (variant) {
        case 0:
        default:
        case 2:
            pl->demonMode = 1;
            pl->noFlinchTimer = 2;
            if (variant == 0) {
                Player_SetMotionPair(pl, 1018, 4, 0);
                break;
            }
            Player_SetMotionPair(pl, 1018, 4, 10);
            break;
        case 1:
            pl->demonMode = 0;
            Player_SetMotionPair(pl, 1019, 4, 0);
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
        pl->stateVar0 = 255;
        pl->stateVar1 = 0;
        pl->timer = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, 1411, 4, 0);
        } else {
            Player_SetMotionPair(pl, 1405, 4, 0);
        }
        PlayerAttack_CreateDual(pl, 45, variant & 0xFF);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 40.0f) != 0) {
            pl->step++;
            if (variant == 0) {
                Player_SetMotionPair(pl, 1410, 0, 0);
            } else {
                Player_SetMotionPair(pl, 1409, 0, 0);
            }
            PlayerAttack_CreateDual(pl, 52, variant & 0xFF);
            pl->stateVar1 = 0;
            break;
        }
        if (DualSwords_CheckHit(pl) == 1) {
            break;
        }
        if (Motion_PassedFrame(pl, 0, 24.0f) != 0) {
            PlayerAttack_CreateDual(pl, 46, variant & 0xFF);
        }
        if (Motion_PassedFrame(pl, 0, 34.0f) != 0) {
            PlayerAttack_CreateDual(pl, 47, variant & 0xFF);
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
            PlayerAttack_CreateDual(pl, 53, variant & 0xFF);
        }
        if (Motion_PassedFrame(pl, 0, 38.0f) != 0) {
            PlayerAttack_CreateDual(pl, 54, variant & 0xFF);
        }
        if (Player_IsLocal(pl) != 1) {
            break;
        }
        if (pl->stateVar0 == 255) {
            if (Motion_ReachedFrame(pl, 0, 22.0f) == 0) {
                if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
                    angle = pl->input.rightStickAngle + 0x2AAB;
                    if (angle > (ANGLE_TURN / 4) && angle < 0x9556) {
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
                if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75) {
                    if (pl->input.stickDir & PAD_DPAD) {
                        angle = pl->input.stickAngle + ANGLE_TURN / 4;
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
                    pl->rot[1] += ANGLE_TURN / 8;
                    pl->targetYaw = pl->rot[1];
                    break;
                case 2:
                    pl->rot[1] -= ANGLE_TURN / 8;
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
        Player_SetMotionPair(pl, 1407, 2, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 28;
        PlayerAttack_Create(pl, 48U);
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
            PlayerAttack_Create(pl, 49U);
        }
        if (Motion_PassedFrame(pl, 0, 26.0f) != 0) {
            PlayerAttack_Create(pl, 50U);
        }
        if (Motion_PassedFrame(pl, 0, 50.0f) != 0) {
            PlayerAttack_Create(pl, 50U);
        }
        if (Motion_PassedFrame(pl, 0, 54.0f) != 0) {
            PlayerAttack_Create(pl, 49U);
        }
        if (Motion_PassedFrame(pl, 0, 66.0f) != 0) {
            PlayerAttack_Create(pl, 49U);
        }
        if (Motion_PassedFrame(pl, 0, 70.0f) != 0) {
            PlayerAttack_Create(pl, 50U);
        }
        if (Motion_PassedFrame(pl, 0, 82.0f) != 0) {
            PlayerAttack_Create(pl, 50U);
        }
        if (Motion_PassedFrame(pl, 0, 88.0f) != 0) {
            PlayerAttack_Create(pl, 49U);
        }
        if (Motion_PassedFrame(pl, 0, 118.0f) != 0) {
            PlayerAttack_Create(pl, 51U);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, 1412, 0, 0);
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
        Player_SetMotionPair(pl, 1408, 4, 0);
        CamShake_ReqA(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
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
        if (dir < 16385) {
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
    if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75)) {
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
                dir = (pl->input.stickAngle + ANGLE_TURN / 4) & 0xFFFF;
                if (dir < 23666) {
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
 * The slash. variant: the opening (0 none, 1 a wind-up, 2 from the bounce, 3
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
            pl->stateVar1 = 255;
            pl->timer = 0;
            Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
            Player_SetMotionPair(pl, 1401, 4, 10);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 57);
            pl->noFlinchTimer = 40;
            break;
        case 1:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                pl->fxMade = 0;
                Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
                Player_SetMotionPair(pl, 1412, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 40;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 1401, 2, 64);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 57);
                    pl->stateVar1 = 255;
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
                Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
                Player_SetMotionPair(pl, 1414, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 40;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 1401, 2, 38);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 57);
                    pl->stateVar1 = 255;
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
                Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
                Player_SetMotionPair(pl, 1415, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 40;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 1401, 0, 48);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 57);
                    pl->stateVar1 = 255;
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
                WeaponTrail_Create(pl, 9, WTRAIL_LONG_SWORD);
            }
            if ((pl->stateVar1 == 255) && (Motion_InFrameRange(pl, 0, 44.0f, 94.0f) != 0)) {
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
            if ((Motion_ReachedFrame(pl, 0, 80.0f) != 0) && (pl->stateVar1 != 255)) {
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
            pl->stateVar1 = 255;
            pl->timer = 0;
            pl->unk417 = 1;
            Player_SetMotionPair(pl, 1402, 4, 10);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 58);
            pl->noFlinchTimer = 70;
            Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
            break;
        case 1:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
                Player_SetMotionPair(pl, 1420, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 40;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 1402, 0, 24);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 58);
                    pl->stateVar1 = 255;
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
                WeaponTrail_Create(pl, 8, WTRAIL_LONG_SWORD);
            }
            if ((pl->stateVar1 == 255) && (Motion_InFrameRange(pl, 0, 44.0f, 104.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 2, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if ((Motion_ReachedFrame(pl, 0, 80.0f) != 0) && (pl->stateVar1 != 255)) {
                LongSword_ComboInput(pl, 2, pl->stateVar1, 0);
            } else if (Motion_InFrameRange(pl, 0, 80.0f, 164.0f) != 0) {
                LongSword_EvadeInput(pl);
            }
        }
        break;
    }
}

/*
 * The fade slash (likely a backward slash). variant 0-6: the opening motion
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
            pl->stateVar1 = 255;
            pl->timer = 0;
            Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
            Player_SetMotionPair(pl, 1403, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 59);
            pl->noFlinchTimer = 40;
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
                    Player_SetMotionPair(pl, 1410, 2, 0);
                    break;
                case 3:
                    Player_SetMotionPair(pl, 1417, 2, 0);
                    break;
                case 4:
                    Player_SetMotionPair(pl, 1418, 2, 0);
                    break;
                case 5:
                    Player_SetMotionPair(pl, 1421, 4, 0);
                    break;
                case 6:
                    Player_SetMotionPair(pl, 1422, 2, 0);
                    break;
                }
                Player_UpdateMotionTime(pl, 0);
                Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
                pl->noFlinchTimer = 40;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    pl->stateVar1 = 255;
                    pl->timer = 0;
                    switch (variant) {
                    case 1:
                    case 2:
                        Player_SetMotionPair(pl, 1403, 2, 18);
                        break;
                    case 3:
                        Player_SetMotionPair(pl, 1403, 0, 20);
                        break;
                    case 4:
                        Player_SetMotionPair(pl, 1403, 0, 20);
                        break;
                    case 5:
                        Player_SetMotionPair(pl, 1403, 0, 16);
                        break;
                    case 6:
                        Player_SetMotionPair(pl, 1403, 0, 12);
                        break;
                    }
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 59);
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
                WeaponTrail_Create(pl, 17, WTRAIL_LONG_SWORD);
            }
            if ((pl->stateVar1 == 255) && (Motion_InFrameRange(pl, 0, 28.0f, 68.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 3, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if ((Motion_ReachedFrame(pl, 0, 68.0f) != 0) && (pl->stateVar1 != 255)) {
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, 1413, 0, 0);
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
            Player_ReturnToIdle(pl, 0, 10, 0);
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
        pl->stateVar1 = 255;
        pl->timer = 0;
        pl->unk417 = 1;
        Player_SetMotionPair(pl, 1405, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 72U);
        pl->noFlinchTimer = 60;
        break;
    case 1:
        if (LongSword_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 18.0f) != 0) {
                WeaponTrail_Create(pl, 9, WTRAIL_LONG_SWORD);
            }
            if (pl->motion[0].playing == 0) {
                Player_ReturnToIdle(pl, 0, 4, 0);
                break;
            }
            if ((pl->stateVar1 == 255) && (Motion_InFrameRange(pl, 0, 28.0f, 100.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 11U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(pl, 0, 38.0f) != 0) {
                stick = pl->stateVar1;
                if (stick != 255) {
                    LongSword_ComboInput(pl, 11U, stick, 0);
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
            pl->stateVar1 = 255;
            pl->timer = 0;
            Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
            Player_SetMotionPair(pl, 1404, 4, 8);
            Player_UpdateMotionTime(pl, 0);
            if (variant == 0) {
                PlayerAttack_Create(pl, 69);
                LongSword_AddGauge(pl, -15);
                PlayerFx_CreateOnPlayerDefault(pl, 0, 1.0f);
            } else {
                PlayerAttack_Create(pl, 79);
            }
            pl->noFlinchTimer = 35;
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
                Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
                if (variant == 2 || variant == 3) {
                    Player_SetMotionPair(pl, 1425, 2, 0);
                } else {
                    Player_SetMotionPair(pl, 1426, 4, 0);
                }
                Player_UpdateMotionTime(pl, 0);
                if (variant == 2 || variant == 4) {
                    LongSword_AddGauge(pl, -20);
                    PlayerFx_CreateOnPlayerDefault(pl, 0, 1.0f);
                }
                pl->noFlinchTimer = 35;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    if (variant == 2 || variant == 3) {
                        Player_SetMotionPair(pl, 1404, 0, 30);
                    } else {
                        Player_SetMotionPair(pl, 1404, 0, 44);
                    }
                    Player_UpdateMotionTime(pl, 0);
                    if (variant == 2 || variant == 4) {
                        PlayerAttack_Create(pl, 69);
                    } else {
                        PlayerAttack_Create(pl, 79);
                    }
                    pl->stateVar1 = 255;
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
                WeaponTrail_Create(pl, 6, WTRAIL_LONG_SWORD_SPIRIT);
            } else {
                WeaponTrail_Create(pl, 6, WTRAIL_LONG_SWORD);
            }
        }
        if (pl->stateVar1 == 255 && Motion_InFrameRange(pl, 0, 46.0f, 100.0f) != 0) {
            attackKey = LongSword_ReadAttackKey(pl);
            if (variant == 0 || variant == 2 || variant == 4) {
                pl->stateVar1 = LongSword_ComboInput(pl, 4, attackKey, 1);
            } else {
                pl->stateVar1 = LongSword_ComboInput(pl, 13, attackKey, 1);
            }
        }
        if (Motion_ReachedFrame(pl, 0, 56.0f) != 0) {
            attackKey = pl->stateVar1;
            if (attackKey != 255) {
                if (variant == 0 || variant == 2 || variant == 4) {
                    LongSword_ComboInput(pl, 4, attackKey, 0);
                    break;
                }
                LongSword_ComboInput(pl, 13, attackKey, 0);
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
        pl->stateVar1 = 255;
        pl->timer = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        Player_SetMotionPair(pl, 1423, 2, 0);
        Player_UpdateMotionTime(pl, 0);
        pl->noFlinchTimer = 60;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1405, 0, 16);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 71U);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (LongSword_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 18.0f) != 0) {
                WeaponTrail_Create(pl, 9, WTRAIL_LONG_SWORD);
            }
            if ((pl->stateVar1 == 255) && (Motion_InFrameRange(pl, 0, 28.0f, 100.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 5U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(pl, 0, 38.0f) != 0) {
                stick = pl->stateVar1;
                if (stick != 255) {
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
            pl->stateVar1 = 255;
            pl->timer = 0;
            Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
            Player_SetMotionPair(pl, 1406, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 73U);
            LongSword_AddGauge(pl, -20);
            PlayerFx_CreateOnPlayerDefault(pl, 0U, 1.0f);
            pl->noFlinchTimer = 60;
            break;
        case 1:
            stateVar0 = pl->stateVar0;
            switch (stateVar0) {
            case 0:
                pl->stateVar0 = stateVar0 + 1;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
                Player_SetMotionPair(pl, 1424, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                LongSword_AddGauge(pl, -20);
                PlayerFx_CreateOnPlayerDefault(pl, 0U, 1.0f);
                pl->noFlinchTimer = 60;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 1406, 0, 24);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 73U);
                    pl->stateVar1 = 255;
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
            WeaponTrail_Create(pl, 8, WTRAIL_LONG_SWORD_SPIRIT);
        }
        if ((pl->stateVar1 == 255) && (Motion_InFrameRange(pl, 0, 34.0f, 100.0f) != 0)) {
            pl->stateVar1 = LongSword_ComboInput(pl, 6U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
        }
        if (Motion_ReachedFrame(pl, 0, 44.0f) != 0) {
            stick = pl->stateVar1;
            if (stick != 255) {
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
        pl->stateVar1 = 255;
        pl->timer = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        Player_SetMotionPair(pl, 1407, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 74U);
        pl->noFlinchTimer = 60;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (LongSword_CheckHit(pl) != 1) {
            if (Motion_PassedFrame(pl, 0, 22.0f) != 0) {
                WeaponTrail_Create(pl, 5, WTRAIL_LONG_SWORD);
            }
            if ((pl->stateVar1 == 255) && (Motion_InFrameRange(pl, 0, 32.0f, 100.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 7U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(pl, 0, 42.0f) != 0) {
                stick = pl->stateVar1;
                if (stick != 255) {
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
            pl->stateVar1 = 255;
            pl->timer = 0;
            pl->unk417 = 1;
            Player_SetMotionPair(pl, 1407, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 75);
            break;
        case 1:
            switch (pl->stateVar0) {
            case 0:
                pl->stateVar0++;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
                Player_SetMotionPair(pl, 1416, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 40;
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    Player_SetMotionPair(pl, 1407, 0, 20);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 75);
                    pl->stateVar1 = 255;
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
                WeaponTrail_Create(pl, 5, WTRAIL_LONG_SWORD);
            }
            if ((pl->stateVar1 == 255) && (Motion_InFrameRange(pl, 0, 32.0f, 100.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 12, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if ((Motion_ReachedFrame(pl, 0, 42.0f) != 0) && (pl->stateVar1 != 255)) {
                LongSword_ComboInput(pl, 12, pl->stateVar1, 0);
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
            pl->stateVar1 = 255;
            pl->timer = 0;
            Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
            Player_SetMotionPair(pl, 1408, 4, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 76U);
            LongSword_AddGauge(pl, -25);
            PlayerFx_CreateOnPlayerDefault(pl, 0U, 1.0f);
            pl->noFlinchTimer = 60;
            break;
        case 1:
            stateVar0 = pl->stateVar0;
            switch (stateVar0) {
            case 0:
                pl->stateVar0 = stateVar0 + 1;
                pl->unk417 = 1;
                pl->weaponOut = 1;
                Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
                Player_SetMotionPair(pl, 1419, 2, 0);
                Player_UpdateMotionTime(pl, 0);
                pl->noFlinchTimer = 60;
                LongSword_AddGauge(pl, -25);
                PlayerFx_CreateOnPlayerDefault(pl, 0U, 1.0f);
                break;
            case 1:
                if (pl->motion[0].playing == 0) {
                    pl->step++;
                    pl->stateVar0 = 0;
                    Player_SetMotionPair(pl, 1408, 0, 10);
                    Player_UpdateMotionTime(pl, 0);
                    PlayerAttack_Create(pl, 76U);
                    pl->stateVar1 = 255;
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
            WeaponTrail_Create(pl, 6, WTRAIL_LONG_SWORD_SPIRIT);
        }
        if (Motion_PassedFrame(pl, 0, 34.0f) != 0) {
            WeaponTrail_Create(pl, 6, WTRAIL_LONG_SWORD_SPIRIT);
        }
        if (Motion_PassedFrame(pl, 0, 84.0f) != 0) {
            WeaponTrail_Create(pl, 8, WTRAIL_LONG_SWORD_SPIRIT);
        }
        if ((Motion_PassedFrame(pl, 0, 34.0f) != 0) && (pl->stateVar0 != 1)) {
            pl->stateVar0 = 1;
            PlayerAttack_Create(pl, 77U);
        }
        if ((Motion_PassedFrame(pl, 0, 88.0f) != 0) && (pl->stateVar0 != 2)) {
            pl->stateVar0 = 2;
            PlayerAttack_Create(pl, 80U);
        }
        if ((pl->stateVar1 == 255) && (Motion_InFrameRange(pl, 0, 100.0f, 160.0f) != 0)) {
            pl->stateVar1 = LongSword_ComboInput(pl, 8U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
        }
        if (Motion_ReachedFrame(pl, 0, 110.0f) != 0) {
            stick = pl->stateVar1;
            if (stick != 255) {
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
        pl->stateVar1 = 255;
        pl->timer = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        Player_SetMotionPair(pl, 1409, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 78U);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (LongSword_CheckHit(pl) != 1) {
            if ((pl->stateVar1 == 255) && (Motion_InFrameRange(pl, 0, 40.0f, 100.0f) != 0)) {
                pl->stateVar1 = LongSword_ComboInput(pl, 9U, LongSword_ReadAttackKey(pl) & 0xFF, 1);
            }
            if (Motion_ReachedFrame(pl, 0, 60.0f) != 0) {
                stick = pl->stateVar1;
                if (stick != 255) {
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
        if (LongSword_GaugeAtLeast(pl, 15) == 1) {
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
    return !(btn & 1) ? 0 : 10;
}

/* The roll button (75 stamina): a roll, or a side evade by the left stick. */
void LongSword_EvadeInput(Player *pl)
{
    s32 dir;

    if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75)) {
        if (pl->input.stickDir & PAD_DPAD) {
            dir = (pl->input.stickAngle + ANGLE_TURN / 4) & 0xFFFF;
            if (dir < 23666) {
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
 * key when the row has a follow-up for it, else 255; with peek 0,
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
            return 255;
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
            return 255;
        }
        break;
    case 4:
        switch (row) {
        case 1:
        case 2:
        case 11:
            if (peek == 0) {
                if (LongSword_GaugeAtLeast(pl, 15) == 1) {
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
                if (LongSword_GaugeAtLeast(pl, 15) == 1) {
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
                if (LongSword_GaugeAtLeast(pl, 20) == 1) {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_2, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 5:
            if (peek == 0) {
                if (LongSword_GaugeAtLeast(pl, 20) == 1) {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_2_AFTER_THRUST, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 6:
            if (peek == 0) {
                if (LongSword_GaugeAtLeast(pl, 25) == 1) {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_3, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        case 7:
            if (peek == 0) {
                if (LongSword_GaugeAtLeast(pl, 25) == 1) {
                    Player_TurnAndSetState(pl, PL_GROUP_WEAPON, LSWORD_SPIRIT_3_AFTER_RISING, 4, 1);
                }
            } else {
                return 4;
            }
            break;
        default:
            return 255;
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
            return 255;
        }
        break;
    }
    return 255;
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
    if (LongSword_GaugeAtLeast(pl, 15) == 1) {
        Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_DRAW_SPIRIT_1, PL_SET_SNAP_POS);
        return;
    }
    Player_SetState(pl, PL_GROUP_WEAPON, LSWORD_DRAW_SPIRIT_1_EMPTY, PL_SET_SNAP_POS);
}

/*
 * Sets the state; with turn 1, first turns targetYaw ANGLE_TURN / 16 toward a sideways
 * left stick.
 */
void Player_TurnAndSetState(Player *pl, u8 group, u8 state, s32 mode, u8 turn)
{
    if (turn == 1) {
        if (pl->input.stickDir & PAD_LEFT) {
            pl->targetYaw += ANGLE_TURN / 16;
        } else if (pl->input.stickDir & PAD_RIGHT) {
            pl->targetYaw -= ANGLE_TURN / 16;
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1428, 0, 8);
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
                PlayerFx_CreateOnPlayerDefault(pl, 11, 1.0f);
                switch (pl->motionId[0]) {
                case 1405:
                default:
                    HuntingHorn_AddNote(pl, 0);
                    break;
                case 1406:
                    HuntingHorn_AddNote(pl, 1);
                    break;
                case 1407:
                    HuntingHorn_AddNote(pl, 2);
                    break;
                }
            }
            if (pl->timer == 0) {
                if (pl->input.pressed & PAD_R1) {
                    switch (pl->motionId[0]) {
                    case 1405:
                    default:
                        Player_RequestState(pl, PL_GROUP_WEAPON, HORN_PLAY_END, PL_SET_SNAP_POS);
                        break;
                    case 1406:
                        Player_RequestState(pl, PL_GROUP_WEAPON, HORN_PLAY_END_NOTE1, PL_SET_SNAP_POS);
                        break;
                    case 1407:
                        Player_RequestState(pl, PL_GROUP_WEAPON, HORN_PLAY_END_NOTE2, PL_SET_SNAP_POS);
                        break;
                    }
                } else {
                    switch (pl->motionId[0]) {
                    case 1405:
                        for (i = 0, note0Range = HuntingHorn_Note0WalkFrames; i < 3; i++, note0Range++) {
                            if (Motion_InFrameRange(pl, 0, note0Range->from, note0Range->to) != 0) {
                                Player_StickTurnMove(pl, 384, 3.0f);
                            }
                        }
                        break;
                    case 1406:
                        for (j = 0, note1Range = HuntingHorn_Note1WalkFrames; j < 3; j++, note1Range++) {
                            if (Motion_InFrameRange(pl, 0, note1Range->from, note1Range->to) != 0) {
                                Player_StickTurnMove(pl, 384, 1.5f);
                            }
                        }
                        break;
                    case 1407:
                        for (k = 0, note2Range = HuntingHorn_Note2WalkFrames; k < 2; k++, note2Range++) {
                            if (Motion_InFrameRange(pl, 0, note2Range->from, note2Range->to) != 0) {
                                Player_StickTurnMove(pl, 384, 1.5f);
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

/* The file's data. */
extern char D_008DA300[];
extern char gStagePageTexts_0_1[];
extern char gStagePageTexts_0_2[];
extern char gStagePageTexts_0_3[];
extern char gStagePageTexts_0_4[];
extern char gStagePageTexts_0_5[];
extern char gStagePageTexts_0_6[];
extern char gStagePageTexts_0_7[];
extern char gStagePageTexts_0_8[];
extern char gStagePageTexts_0_9[];
extern char gStagePageTexts_0_10[];
extern char gStagePageTexts_0_11[];
extern char gStagePageTexts_0_12[];
extern char gStagePageTexts_1_1[];
extern char gStagePageTexts_1_2[];
extern char gStagePageTexts_1_3[];
extern char D_008DAFD8[];
extern char D_008DB5F8[];
extern char D_008DBDC8[];
StartItem gStartItemLists_0[4] = {
#include "game/00895300.inc"
};
StartItem gStartItemLists_1[8] = {
#include "game/00895310.inc"
};
StartItem gStartItemLists_2[4] = {
#include "game/00895330.inc"
};
StartItem gStartItemLists_3[12] = {
#include "game/00895340.inc"
};
StartItem gStartItemLists_4[12] = {
#include "game/00895370.inc"
};
StartItem gStartItemLists_5[4] = {
#include "game/008953A0.inc"
};
StartItem gStartItemLists_6[12] = {
#include "game/008953B0.inc"
};
StartItem gStartItemLists_7[12] = {
#include "game/008953E0.inc"
};
StartItem gStartItemLists_8[12] = {
#include "game/00895410.inc"
};
StartItem gStartItemLists_9[8] = {
#include "game/00895440.inc"
};
StartItem gStartItemLists_10[12] = {
#include "game/00895460.inc"
};
StartItem gStartItemLists_11[8] = {
#include "game/00895490.inc"
};
StartItem gStartItemLists_12[12] = {
#include "game/008954B0.inc"
};
StartItem gStartItemLists_13[12] = {
#include "game/008954E0.inc"
};
StartItem gStartItemLists_14[8] = {
#include "game/00895510.inc"
};
StartItem gStartItemLists_15[12] = {
#include "game/00895530.inc"
};
StartItem gStartItemLists_16[16] = {
#include "game/00895560.inc"
};
StartItem gStartItemLists_17[4] = {
#include "game/008955A0.inc"
};
StartItem gStartItemLists_18[8] = {
#include "game/008955B0.inc"
};
StartItem gStartItemLists_19[16] = {
#include "game/008955D0.inc"
};
StartItem gStartItemLists_20[12] = {
#include "game/00895610.inc"
};
StartItem gStartItemLists_21[12] = {
#include "game/00895640.inc"
};
StartItem gStartItemLists_22[8] = {
#include "game/00895670.inc"
};
StartItem gStartItemLists_23[12] = {
#include "game/00895690.inc"
};
StartItem gStartItemLists_24[8] = {
#include "game/008956C0.inc"
};
StartItem gStartItemLists_25[20] = {
#include "game/008956E0.inc"
};
StartItem gStartItemLists_26[16] = {
#include "game/00895730.inc"
};
StartItem gStartItemLists_27[8] = {
#include "game/00895770.inc"
};
StartItem gStartItemLists_28[8] = {
#include "game/00895790.inc"
};
StartItem gStartItemLists_29[16] = {
#include "game/008957B0.inc"
};
StartItem gStartItemLists_30[8] = {
#include "game/008957F0.inc"
};
StartItem gStartItemLists_31[16] = {
#include "game/00895810.inc"
};
StartItem gStartItemLists_32[16] = {
#include "game/00895850.inc"
};
StartItem gStartItemLists_33[16] = {
#include "game/00895890.inc"
};
StartItem gStartItemLists_34[16] = {
#include "game/008958D0.inc"
};
StartItem gStartItemLists_35[12] = {
#include "game/00895910.inc"
};
StartItem gStartItemLists_36[16] = {
#include "game/00895940.inc"
};
StartItem gStartItemLists_37[8] = {
#include "game/00895980.inc"
};
StartItem gStartItemLists_38[16] = {
#include "game/008959A0.inc"
};
StartItem gStartItemLists_39[8] = {
#include "game/008959E0.inc"
};
StartItem gStartItemLists_40[8] = {
#include "game/00895A00.inc"
};
StartItem gStartItemLists_41[12] = {
#include "game/00895A20.inc"
};
StartItem gStartItemLists_42[16] = {
#include "game/00895A50.inc"
};
StartItem gStartItemLists_43[8] = {
#include "game/00895A90.inc"
};
StartItem gStartItemLists_44[12] = {
#include "game/00895AB0.inc"
};
StartItem gStartItemLists_45[4] = {
#include "game/00895AE0.inc"
};
StartItem gStartItemLists_46[12] = {
#include "game/00895AF0.inc"
};
StartItem gStartItemLists_47[6] = {
#include "game/00895B20.inc"
};
StartItem gStartItemLists_48[2] = {
#include "game/00895B38.inc"
};
StartItem gStartItemLists_49[4] = {
#include "game/00895B40.inc"
};
StartItem gStartItemLists_50[6] = {
#include "game/00895B50.inc"
};
StartItem gStartItemLists_51[2] = {
#include "game/00895B68.inc"
};
StartItem gStartItemLists_52[8] = {
#include "game/00895B70.inc"
};
StartItem gStartItemLists_53[8] = {
#include "game/00895B90.inc"
};
StartItem gStartItemLists_54[8] = {
#include "game/00895BB0.inc"
};
StartItem gStartItemLists_55[12] = {
#include "game/00895BD0.inc"
};
StartItem gStartItemLists_56[8] = {
#include "game/00895C00.inc"
};
StartItem gStartItemLists_57[4] = {
#include "game/00895C20.inc"
};
StartItem gStartItemLists_58[8] = {
#include "game/00895C30.inc"
};
StartItem gStartItemLists_59[8] = {
#include "game/00895C50.inc"
};
StartItem gStartItemLists_60[8] = {
#include "game/00895C70.inc"
};
StartItem gStartItemLists_61[8] = {
#include "game/00895C90.inc"
};
StartItem gStartItemLists_62[8] = {
#include "game/00895CB0.inc"
};
StartItem gStartItemLists_63[12] = {
#include "game/00895CD0.inc"
};
StartItem gStartItemLists_64[16] = {
#include "game/00895D00.inc"
};
StartItem gStartItemLists_65[8] = {
#include "game/00895D40.inc"
};
StartItem gStartItemLists_66[8] = {
#include "game/00895D60.inc"
};
StartItem gStartItemLists_67[8] = {
#include "game/00895D80.inc"
};
StartItem *gStartItemLists[68] = {
#include "game/00895DA0.inc"
};
PlselPreset gPlselPresets_0[4] = {
#include "game/00895EB0.inc"
};
PlselPreset gPlselPresets_1[4] = {
#include "game/00895EF0.inc"
};
PlselPreset gPlselPresets_2[4] = {
#include "game/00895F30.inc"
};
PlselPreset gPlselPresets_3[4] = {
#include "game/00895F70.inc"
};
PlselPreset gPlselPresets_4[4] = {
#include "game/00895FB0.inc"
};
PlselPreset gPlselPresets_5[4] = {
#include "game/00895FF0.inc"
};
PlselPreset gPlselPresets_6[4] = {
#include "game/00896030.inc"
};
PlselPreset gPlselPresets_7[4] = {
#include "game/00896070.inc"
};
PlselPreset gPlselPresets_8[4] = {
#include "game/008960B0.inc"
};
PlselPreset gPlselPresets_9[4] = {
#include "game/008960F0.inc"
};
PlselPreset gPlselPresets_10[4] = {
#include "game/00896130.inc"
};
PlselPreset gPlselPresets_11[4] = {
#include "game/00896170.inc"
};
PlselPreset gPlselPresets_12[4] = {
#include "game/008961B0.inc"
};
PlselPreset gPlselPresets_13[4] = {
#include "game/008961F0.inc"
};
PlselPreset gPlselPresets_14[4] = {
#include "game/00896230.inc"
};
PlselPreset gPlselPresets_15[4] = {
#include "game/00896270.inc"
};
PlselPreset gPlselPresets_16[4] = {
#include "game/008962B0.inc"
};
PlselPreset *gPlselPresets[20] = {
#include "game/008962F0.inc"
};
u16 gPlselPresetStages[17][2] = {
#include "game/00896340.inc"
};
char *gStagePageTexts_0[16] = {
    D_008DA300, gStagePageTexts_0_1, gStagePageTexts_0_2, gStagePageTexts_0_3,
    gStagePageTexts_0_4, gStagePageTexts_0_5, gStagePageTexts_0_6, gStagePageTexts_0_7,
    gStagePageTexts_0_8, gStagePageTexts_0_9, gStagePageTexts_0_10, gStagePageTexts_0_11,
    gStagePageTexts_0_12, NULL, NULL, NULL,
};
char *gStagePageTexts_1[16] = {
    D_008DA300, gStagePageTexts_1_1, gStagePageTexts_1_2, gStagePageTexts_1_3,
    D_008DA570, gStagePageTexts_1_5, gStagePageTexts_1_6, gStagePageTexts_1_7,
    gStagePageTexts_1_8, D_008DA570, gStagePageTexts_1_10, gStagePageTexts_1_11,
    gStagePageTexts_1_12, NULL, NULL, NULL,
};
char *gStagePageTexts_2[16] = {
    D_008DA690, D_008DA6C0, D_008DA6E0, gStagePageTexts_2_0,
    D_008DA720, D_008DA570, gStagePageTexts_2_3, gStagePageTexts_2_4,
    gStagePageTexts_2_5, gStagePageTexts_2_6, gStagePageTexts_2_7, gStagePageTexts_2_8,
    gStagePageTexts_2_9, NULL, NULL, NULL,
};
char *gStagePageTexts_3[16] = {
    D_008DA830, D_008DA860, D_008DA890, D_008DA8B0,
    D_008DA8E0, gStagePageTexts_3_0, gStagePageTexts_3_1, D_008DA570,
    gStagePageTexts_3_3, gStagePageTexts_3_4, gStagePageTexts_3_5, gStagePageTexts_3_6,
    D_008DA720, NULL, NULL, NULL,
};
char *gStagePageTexts_4[16] = {
    gStagePageTexts_4_0, gStagePageTexts_4_1, gStagePageTexts_4_2, gStagePageTexts_4_3,
    D_008DA570, gStagePageTexts_4_5, gStagePageTexts_4_6, D_008DA570,
    gStagePageTexts_4_8, gStagePageTexts_4_9, gStagePageTexts_4_10, gStagePageTexts_4_11,
    NULL, NULL, NULL, NULL,
};
char *gStagePageTexts_5[16] = {
    D_008DAB70, gStagePageTexts_5_0, gStagePageTexts_5_1, gStagePageTexts_5_2,
    gStagePageTexts_5_3, D_008DA570, gStagePageTexts_5_5, gStagePageTexts_5_6,
    gStagePageTexts_5_7, gStagePageTexts_5_8, D_008DA570, gStagePageTexts_5_10,
    gStagePageTexts_5_11, NULL, NULL, NULL,
};
char *gStagePageTexts_6[16] = {
    D_008DAD20, D_008DAD50, D_008DAD70, D_008DAD90,
    D_008DA570, D_008DADB0, D_008DADE0, gStagePageTexts_6_0,
    D_008DA570, gStagePageTexts_6_2, gStagePageTexts_6_3, gStagePageTexts_6_4,
    gStagePageTexts_6_5, NULL, NULL, NULL,
};
char *gStagePageTexts_7[16] = {
    D_008DAEC0, gStagePageTexts_7_0, gStagePageTexts_7_1, gStagePageTexts_7_2,
    D_008DA570, gStagePageTexts_7_4, gStagePageTexts_7_5, gStagePageTexts_7_6,
    D_008DAFD8, gStagePageTexts_7_8, gStagePageTexts_7_9, gStagePageTexts_7_10,
    gStagePageTexts_7_11, NULL, NULL, NULL,
};
char *gStagePageTexts_8[16] = {
    D_008DB070, D_008DB090, D_008DA570, D_008DB0C0,
    gStagePageTexts_8_0, gStagePageTexts_8_1, D_008DA570, gStagePageTexts_8_3,
    gStagePageTexts_8_4, D_008DA570, gStagePageTexts_8_6, gStagePageTexts_8_7,
    D_008DA570, NULL, NULL, NULL,
};
char *gStagePageTexts_9[16] = {
    D_008DB1B0, D_008DB1D0, gStagePageTexts_9_0, D_008DA570,
    gStagePageTexts_9_2, gStagePageTexts_9_3, D_008DA570, gStagePageTexts_9_5,
    gStagePageTexts_9_6, D_008DA570, gStagePageTexts_9_8, gStagePageTexts_9_9,
    gStagePageTexts_9_10, NULL, NULL, NULL,
};
char *gStagePageTexts_10[16] = {
    D_008DB310, gStagePageTexts_10_1, D_008DA570, D_008DB350,
    gStagePageTexts_10_4, D_008DA570, gStagePageTexts_10_6, gStagePageTexts_10_7,
    D_008DA570, gStagePageTexts_10_9, gStagePageTexts_10_10, gStagePageTexts_10_11,
    NULL, NULL, NULL, NULL,
};
char *gStagePageTexts_11[16] = {
    D_008DB310, gStagePageTexts_11_1, D_008DA570, D_008DB350,
    gStagePageTexts_11_4, gStagePageTexts_11_5, D_008DA570, gStagePageTexts_11_7,
    gStagePageTexts_11_8, gStagePageTexts_11_9, D_008DA570, gStagePageTexts_11_11,
    gStagePageTexts_11_12, NULL, NULL, NULL,
};
char *gStagePageTexts_12[16] = {
    D_008DB310, gStagePageTexts_12_1, D_008DA570, D_008DB350,
    gStagePageTexts_12_4, D_008DA570, D_008DB5A0, gStagePageTexts_12_7,
    D_008DA570, gStagePageTexts_12_9, D_008DB5F8, gStagePageTexts_12_11,
    D_008DA570, NULL, NULL, NULL,
};
char *gStagePageTexts_13[16] = {
    D_008DB630, D_008DB660, D_008DA570, D_008DB690,
    D_008DB6C0, D_008DB6E0, D_008DA570, gStagePageTexts_13_0,
    gStagePageTexts_13_1, gStagePageTexts_13_2, gStagePageTexts_13_3, gStagePageTexts_13_4,
    gStagePageTexts_13_5, NULL, NULL, NULL,
};
char *gStagePageTexts_14[16] = {
    D_008DB7C0, D_008DB7E0, gStagePageTexts_14_0, gStagePageTexts_14_1,
    gStagePageTexts_14_2, gStagePageTexts_14_3, gStagePageTexts_14_4, D_008DA570,
    gStagePageTexts_14_6, gStagePageTexts_14_7, gStagePageTexts_14_8, gStagePageTexts_14_9,
    gStagePageTexts_14_10, NULL, NULL, NULL,
};
char *gStagePageTexts_15[16] = {
    D_008DB7C0, gStagePageTexts_15_0, gStagePageTexts_15_1, D_008DA570,
    gStagePageTexts_15_3, gStagePageTexts_15_4, gStagePageTexts_15_5, gStagePageTexts_15_6,
    D_008DA570, gStagePageTexts_15_8, gStagePageTexts_15_9, gStagePageTexts_15_10,
    gStagePageTexts_15_11, NULL, NULL, NULL,
};
char *gStagePageTexts_16[16] = {
    D_008DBB20, D_008DBB50, gStagePageTexts_15_1, gStagePageTexts_16_0,
    gStagePageTexts_16_1, gStagePageTexts_16_2, gStagePageTexts_16_3, gStagePageTexts_16_4,
    gStagePageTexts_16_5, gStagePageTexts_16_6, gStagePageTexts_16_7, gStagePageTexts_16_8,
    gStagePageTexts_16_9, NULL, NULL, NULL,
};
char *gStagePageTexts_17[16] = {
    D_008DB7C0, D_008DBD30, gStagePageTexts_15_1, D_008DA570,
    D_008DBD60, gStagePageTexts_17_0, gStagePageTexts_17_1, D_008DBDC8,
    D_008DA570, gStagePageTexts_17_4, gStagePageTexts_17_5, gStagePageTexts_17_6,
    gStagePageTexts_17_7, NULL, NULL, NULL,
};
char *gStagePageTexts_18[16] = {
    D_008DB7C0, D_008DBE70, gStagePageTexts_15_1, D_008DA570,
    gStagePageTexts_18_0, gStagePageTexts_18_1, gStagePageTexts_18_2, D_008DA570,
    gStagePageTexts_18_4, gStagePageTexts_18_5, gStagePageTexts_18_6, gStagePageTexts_18_7,
    gStagePageTexts_18_8, NULL, NULL, NULL,
};
char *gStagePageTexts_19[16] = {
    D_008DB7C0, D_008DBFE0, gStagePageTexts_15_1, D_008DA570,
    gStagePageTexts_19_0, gStagePageTexts_19_1, gStagePageTexts_19_2, gStagePageTexts_19_3,
    gStagePageTexts_19_4, gStagePageTexts_19_5, D_008DA570, gStagePageTexts_19_7,
    gStagePageTexts_19_8, NULL, NULL, NULL,
};
char *gStagePageTexts_20[16] = {
    D_008DB7C0, D_008DC160, gStagePageTexts_15_1, D_008DA570,
    D_008DC190, D_008DC1B0, D_008DC1D0, gStagePageTexts_20_0,
    gStagePageTexts_20_1, D_008DA570, gStagePageTexts_20_3, gStagePageTexts_20_4,
    gStagePageTexts_20_5, NULL, NULL, NULL,
};
char *gStagePageTexts_21[16] = {
#include "game/008968D0.inc"
};
char *gStagePageTexts_22[16] = {
    D_008DC470, D_008DC490, D_008DC4B0, D_008DC4D0,
    gStagePageTexts_22_0, D_008DA570, gStagePageTexts_22_2, gStagePageTexts_22_3,
    gStagePageTexts_22_4, D_008DA570, gStagePageTexts_22_6, gStagePageTexts_22_7,
    gStagePageTexts_22_8, NULL, NULL, NULL,
};
char *gStagePageTexts_23[16] = {
    gStagePageTexts_23_0, gStagePageTexts_23_1, D_008DA570, D_008DB350,
    gStagePageTexts_23_4, D_008DA570, gStagePageTexts_23_6, gStagePageTexts_23_7,
    gStagePageTexts_23_8, D_008DA570, D_008DC710, D_008DC730,
    gStagePageTexts_23_12, NULL, NULL, NULL,
};
char *gStagePageTexts_24[16] = {
    gStagePageTexts_24_0, gStagePageTexts_24_1, D_008DA570, D_008DB350,
    gStagePageTexts_24_4, D_008DA570, gStagePageTexts_24_6, gStagePageTexts_24_7,
    gStagePageTexts_24_8, D_008DA570, D_008DC710, D_008DC730,
    gStagePageTexts_24_12, NULL, NULL, NULL,
};
char *gStagePageTexts_25[16] = {
    D_008DC880, D_008DA570, D_008DB350, gStagePageTexts_25_3,
    D_008DA570, gStagePageTexts_25_5, D_008DA570, gStagePageTexts_25_7,
    gStagePageTexts_25_8, gStagePageTexts_25_9, D_008DA570, gStagePageTexts_25_11,
    gStagePageTexts_25_12, NULL, NULL, NULL,
};
char *gStagePageTexts_26[12] = {
    D_008DC880, D_008DA570, D_008DB350, gStagePageTexts_26_3,
    D_008DA570, D_008DB5A0, gStagePageTexts_26_6, D_008DA570,
    D_008DC710, D_008DC730, gStagePageTexts_26_10, NULL,
};
char *gStagePageTexts_27[16] = {
#include "game/00896A40.inc"
};
char **gStagePageTexts[28] = {
#include "game/00896A80.inc"
};
u16 gStagePageStageIds[29] = {
#include "game/00896AF0.inc"
};
StartItem gTrainingStartItems_1[2] = {
#include "game/00896B30.inc"
};
StartItem gTrainingStartItems_2[2] = {
#include "game/00896B38.inc"
};
StartItem gTrainingStartItems_3[2] = {
#include "game/00896B40.inc"
};
StartItem gTrainingStartItems_4[2] = {
#include "game/00896B48.inc"
};
StartItem gTrainingStartItems_5[2] = {
#include "game/00896B50.inc"
};
StartItem gTrainingStartItems_6[2] = {
#include "game/00896B58.inc"
};
StartItem gTrainingStartItems_7[2] = {
#include "game/00896B60.inc"
};
StartItem gTrainingStartItems_8[2] = {
#include "game/00896B68.inc"
};
StartItem gTrainingStartItems_9[2] = {
#include "game/00896B70.inc"
};
StartItem gTrainingStartItems_10[2] = {
#include "game/00896B78.inc"
};
StartItem gTrainingStartItems_11[2] = {
#include "game/00896B80.inc"
};
StartItem gTrainingStartItems_12[2] = {
#include "game/00896B88.inc"
};
StartItem gTrainingStartItems_13[4] = {
#include "game/00896B90.inc"
};
StartItem gTrainingStartItems_14[12] = {
#include "game/00896BA0.inc"
};
StartItem gTrainingStartItems_15[8] = {
#include "game/00896BD0.inc"
};
StartItem gTrainingStartItems_16[6] = {
#include "game/00896BF0.inc"
};
StartItem gTrainingStartItems_17[2] = {
#include "game/00896C08.inc"
};
StartItem gTrainingStartItems_18[4] = {
#include "game/00896C10.inc"
};
StartItem *gTrainingStartItems[20] = {
#include "game/00896C20.inc"
};
PlselPreset gTrainingPresets_1 = {
#include "game/00896C70.inc"
};
PlselPreset gTrainingPresets_2 = {
#include "game/00896C80.inc"
};
PlselPreset gTrainingPresets_3 = {
#include "game/00896C90.inc"
};
PlselPreset gTrainingPresets_4 = {
#include "game/00896CA0.inc"
};
PlselPreset gTrainingPresets_5 = {
#include "game/00896CB0.inc"
};
PlselPreset gTrainingPresets_6 = {
#include "game/00896CC0.inc"
};
PlselPreset gTrainingPresets_7 = {
#include "game/00896CD0.inc"
};
PlselPreset gTrainingPresets_8 = {
#include "game/00896CE0.inc"
};
PlselPreset gTrainingPresets_9 = {
#include "game/00896CF0.inc"
};
PlselPreset gTrainingPresets_10 = {
#include "game/00896D00.inc"
};
PlselPreset gTrainingPresets_11 = {
#include "game/00896D10.inc"
};
PlselPreset gTrainingPresets_12 = {
#include "game/00896D20.inc"
};
PlselPreset gTrainingPresets_13 = {
#include "game/00896D30.inc"
};
PlselPreset gTrainingPresets_14 = {
#include "game/00896D40.inc"
};
PlselPreset gTrainingPresets_15 = {
#include "game/00896D50.inc"
};
PlselPreset gTrainingPresets_16 = {
#include "game/00896D60.inc"
};
PlselPreset gTrainingPresets_17 = {
#include "game/00896D70.inc"
};
PlselPreset gTrainingPresets_18 = {
#include "game/00896D80.inc"
};
PlselPreset *gTrainingPresets[20] = {
#include "game/00896D90.inc"
};
MotionBlend GreatSword_OverheadBlends[8] = {
#include "game/00896DE0.inc"
};
GunMotion Bowgun_FireMotions[5] = {
#include "game/00896E00.inc"
};
s16 DualSwords_SideSlashMotions[4][2] = {
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
