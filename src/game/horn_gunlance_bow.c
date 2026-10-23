/*
 * The drawn weapons' states of the hunting horn (its swings, its play
 * stances, the notes and the songs), the gunlance and the bow: each weapon's
 * RunState maps the player's state in PL_GROUP_WEAPON to a handler, which
 * steps through pl->step, plays its motions and makes its attack objects.
 * The weapon hits these states make are PlayerAttack objects, and the bow's
 * arrows are BowArrow.
 */
#include "common.h"
#include "em.h"
#include "pad.h"
#include "player.h"
#include "qsync.h"
#include "area_draw.h"
#include "camera.h"
#include "quest.h"
#include "save.h"
#include "sound.h"
#include "bone_effects.h"
#include "cam_helpers.h"
#include "dust_flash_effects.h"
#include "engine_math.h"
#include "game_util.h"
#include "hit_effects.h"
#include "rand.h"
#include "vibration.h"
#include "libc.h"
#include "area.h"
#include "attack_objects.h"
#include "cam_shake.h"

/* prototypes */
s32 Player_HasSongEffect(Player *pl, s16 kind);
extern s16 Gunlance_ShellMotions[][2];
extern s16 D_00897022[][2];
u8 Pouch_Add(Player *p, u16 item, s16 n);
static void Bow_SetAimLayers(Player *em, u16 motA, u16 motB);
s16 Player_IsState(void *obj, u8 state, u8 sub);
extern u8 HuntingHorn_SongNotes[];
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
extern u8 gBladeSpecialMarks;
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 50;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        switch (variant) {
        case 0:
        case 1:
        case 2:
            pl->step = 2;
            if (variant == 1) {
                Player_SetMotionPair(pl, 1401, 10, 40);
            } else {
                Player_SetMotionPair(pl, 1401, 4, 0);
            }
            if (variant == 2) {
                pl->motionSpeed = 0.77f;
            }
            PlayerAttack_Create(pl, 81);
            break;
        case 3:
            pl->step++;
            Player_SetMotionPair(pl, 1006, 2, 0);
            break;
        case 4:
            pl->step++;
            Player_SetMotionPair(pl, 1430, 4, 0);
            break;
        }
        Player_UpdateMotionTime(pl, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            if (variant == 3) {
                Player_SetMotionPair(pl, 1401, 0, 26);
                Player_UpdateMotionTime(pl, 0);
                PlayerAttack_Create(pl, 99);
                break;
            }
            Player_SetMotionPair(pl, 1401, 0, 10);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 81);
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
        Player_SetMotionPair(pl, 1402, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 82U);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 50;
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
        Player_SetMotionPair(pl, 1404, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 83U);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 30;
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
        Player_SetMotionPair(pl, 1416, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->noFlinchTimer = 30;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1404, 0, 12);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 84);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, 1429, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1401, 0, 68);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        if (variant == 0) {
            pl->step = 2;
            Player_SetMotionPair(pl, 1412, 2, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 94U);
            break;
        }
        Player_SetMotionPair(pl, 1431, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1412, 0, 38);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 94U);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        Player_PlaySe(pl, 30, 0, &pl->pos[0], 1, 0);
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
                    case 1408:
                        if (Motion_InFrameRange(pl, 0, 112.0f, 132.0f) != 0) {
                            HuntingHorn_CheckInput(pl, 4);
                        }
                        break;
                    case 1409:
                        if (Motion_InFrameRange(pl, 0, 128.0f, 148.0f) != 0) {
                            HuntingHorn_CheckInput(pl, 4);
                        }
                        break;
                    case 1410:
                        if (Motion_InFrameRange(pl, 0, 108.0f, 128.0f) != 0) {
                            HuntingHorn_CheckInput(pl, 4);
                        }
                        break;
                    case 1411:
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 1413, 2, 0);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 95U);
            break;
        case 1:
            pl->step++;
            Player_SetMotionPair(pl, 1414, 2, 0);
            break;
        case 2:
            pl->step++;
            Player_SetMotionPair(pl, 1415, 2, 0);
            break;
        }
        pl->weaponOut = 1;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1413, 0, 26);
            Player_UpdateMotionTime(pl, 0);
            PlayerAttack_Create(pl, 95U);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        Player_SetMotionPair(pl, 1403, 2, 0);
        Player_UpdateMotionTime(pl, 0);
        PlayerAttack_Create(pl, 85U);
        pl->noFlinchTimer = 60;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 84.0f) != 0) {
            PlayerAttack_Create(pl, 86U);
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
        HuntingHorn_StatePlayAttack(pl, 10);
        break;
    case HORN_NOTE2_ATTACK_DOWN:
        HuntingHorn_StatePlayAttack(pl, 11);
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
        if (dir < 18205) {
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
    if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75)) {
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
        if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75 && caller != 5) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (u16) (pl->input.stickAngle + ANGLE_TURN / 4);
                if (dir < 23666) {
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
            if (dir < 18205) {
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
        if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75)) {
            if (pl->input.stickDir & PAD_DPAD) {
                dir = (pl->input.stickAngle + ANGLE_TURN / 4) & 0xFFFF;
                if (dir < 23666) {
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
            dir2 = (pl->input.rightStickAngle + ANGLE_TURN / 8) & 0xFFFF;
            if (dir2 < 16385) {
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
    case 1412:
        if (Motion_ReachedFrame(pl, 0, 72.0f) == 0 || pl->motion[0].blend != 0) {
            ret = 0;
        }
        break;
    case 1427:
    case 1428:
    case 1432:
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
    char buf[256];

    if (Player_IsLocal(pl)) {
        sprintf(buf, "%s%s", gHornSongTexts[0], gHornSongTexts[song]);
        Notice_PushText(buf);
    }
}

/* two empty strings after "%s%s" whose users were dropped */
const u8 D_008DD028[8] = {0};

/*
 * Applies a song's effect through the song effect slots (buffs, max HP, roar
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
        if (Player_HasSongEffect(p, PL_SONG_SPEED) == 1) {
            p->songEffect[PL_SONG_SPEED] = 2;
            p->songEffectTimer[PL_SONG_SPEED] += 1800;
            if (p->songEffectTimer[PL_SONG_SPEED] >= 3600) {
                p->songEffectTimer[PL_SONG_SPEED] = 3600;
            }
            HuntingHorn_ShowSongMessage(p, 2);
        } else {
            p->songEffect[PL_SONG_SPEED] = 1;
            p->songEffectTimer[PL_SONG_SPEED] = 3600;
            HuntingHorn_ShowSongMessage(p, 1);
        }
        break;
    case 2:
    case 3:
        if (Player_HasSongEffect(p, PL_SONG_ATTACK) == 1) {
            if (song == 2) {
                if (p->songEffect[PL_SONG_ATTACK] <= 2) {
                    p->songEffect[PL_SONG_ATTACK] = 2;
                    p->songEffectTimer[PL_SONG_ATTACK] += 1800;
                    if (p->songEffectTimer[PL_SONG_ATTACK] >= 2700) {
                        p->songEffectTimer[PL_SONG_ATTACK] = 2700;
                    }
                    HuntingHorn_ShowSongMessage(p, 4);
                }
            } else if (p->songEffect[PL_SONG_ATTACK] >= 3) {
                p->songEffect[PL_SONG_ATTACK] = 4;
                p->songEffectTimer[PL_SONG_ATTACK] += 900;
                if (p->songEffectTimer[PL_SONG_ATTACK] >= 1800) {
                    p->songEffectTimer[PL_SONG_ATTACK] = 1800;
                }
                HuntingHorn_ShowSongMessage(p, 6);
            } else {
                p->songEffect[PL_SONG_ATTACK] = 3;
                p->songEffectTimer[PL_SONG_ATTACK] = 1800;
                HuntingHorn_ShowSongMessage(p, 5);
            }
        } else if (song == 2) {
            p->songEffect[PL_SONG_ATTACK] = 1;
            p->songEffectTimer[PL_SONG_ATTACK] = 2700;
            HuntingHorn_ShowSongMessage(p, 3);
        } else {
            p->songEffect[PL_SONG_ATTACK] = 3;
            p->songEffectTimer[PL_SONG_ATTACK] = 1800;
            HuntingHorn_ShowSongMessage(p, 5);
        }
        break;
    case 4:
    case 5:
        if (Player_HasSongEffect(p, PL_SONG_DEFENSE) == 1) {
            if (song == 4) {
                if (p->songEffect[PL_SONG_DEFENSE] <= 2) {
                    p->songEffect[PL_SONG_DEFENSE] = 2;
                    p->songEffectTimer[PL_SONG_DEFENSE] += 1800;
                    if (p->songEffectTimer[PL_SONG_DEFENSE] >= 2700) {
                        p->songEffectTimer[PL_SONG_DEFENSE] = 2700;
                    }
                    HuntingHorn_ShowSongMessage(p, 8);
                }
            } else if (p->songEffect[PL_SONG_DEFENSE] >= 3) {
                p->songEffect[PL_SONG_DEFENSE] = 4;
                p->songEffectTimer[PL_SONG_DEFENSE] += 900;
                if (p->songEffectTimer[PL_SONG_DEFENSE] >= 1800) {
                    p->songEffectTimer[PL_SONG_DEFENSE] = 1800;
                }
                HuntingHorn_ShowSongMessage(p, 10);
            } else {
                p->songEffect[PL_SONG_DEFENSE] = 3;
                p->songEffectTimer[PL_SONG_DEFENSE] = 1800;
                HuntingHorn_ShowSongMessage(p, 9);
            }
        } else if (song == 4) {
            p->songEffect[PL_SONG_DEFENSE] = 1;
            p->songEffectTimer[PL_SONG_DEFENSE] = 2700;
            HuntingHorn_ShowSongMessage(p, 7);
        } else {
            p->songEffect[PL_SONG_DEFENSE] = 3;
            p->songEffectTimer[PL_SONG_DEFENSE] = 1800;
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
            HuntingHorn_ShowSongMessage(p, 11);
            break;
        case 7:
            a = 20;
            HuntingHorn_ShowSongMessage(p, 13);
            break;
        case 8:
            a = 30;
            HuntingHorn_ShowSongMessage(p, 15);
            break;
        }
        if (Player_HasSongEffect(p, PL_SONG_HEALTH) == 1) {
            k = p->songEffect[PL_SONG_HEALTH];
            if (k == a) {
                p->songEffectTimer[PL_SONG_HEALTH] += 1800;
                if (p->songEffectTimer[PL_SONG_HEALTH] >= 3600) {
                    p->songEffectTimer[PL_SONG_HEALTH] = 3600;
                }
            } else if (a > k) {
                Player_AddMaxHp(p, a - k, 1);
                Player_AddHp(p, a - p->songEffect[PL_SONG_HEALTH]);
                p->songEffect[PL_SONG_HEALTH] = a;
                p->songEffectTimer[PL_SONG_HEALTH] = 3600;
            }
        } else {
            n = a;
            Player_AddMaxHp(p, n, 1);
            Player_AddHp(p, n);
            p->songEffect[PL_SONG_HEALTH] = a;
            p->songEffectTimer[PL_SONG_HEALTH] = 3600;
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
        if (Player_HasSongEffect(p, PL_SONG_ROAR_GUARD) == 1) {
            k = p->songEffect[PL_SONG_ROAR_GUARD];
            if (k == a || k == b) {
                if (k <= 2) {
                    p->songEffect[PL_SONG_ROAR_GUARD] = b;
                    p->songEffectTimer[PL_SONG_ROAR_GUARD] = 1800;
                } else {
                    p->songEffect[PL_SONG_ROAR_GUARD] = b;
                    p->songEffectTimer[PL_SONG_ROAR_GUARD] += 1800;
                    if (p->songEffectTimer[PL_SONG_ROAR_GUARD] >= 3600) {
                        p->songEffectTimer[PL_SONG_ROAR_GUARD] = 3600;
                    }
                }
            } else if (a > k) {
                p->songEffect[PL_SONG_ROAR_GUARD] = a;
                p->songEffectTimer[PL_SONG_ROAR_GUARD] = 3600;
            } else {
                break;
            }
        } else {
            p->songEffect[PL_SONG_ROAR_GUARD] = a;
            p->songEffectTimer[PL_SONG_ROAR_GUARD] = 3600;
        }
        switch (p->songEffect[PL_SONG_ROAR_GUARD]) {
        case 1:
        default:
            HuntingHorn_ShowSongMessage(p, 17);
            break;
        case 2:
            HuntingHorn_ShowSongMessage(p, 19);
            break;
        case 3:
            HuntingHorn_ShowSongMessage(p, 21);
            break;
        }
        break;
    case 13:
    case 14:
        p->songEffect[PL_SONG_STAMINA] = 1;
        if (Player_HasSongEffect(p, PL_SONG_STAMINA) == 1) {
            p->songEffectTimer[PL_SONG_STAMINA] += 900;
            if (song == 13) {
                if (p->songEffectTimer[PL_SONG_STAMINA] >= 1800) {
                    p->songEffectTimer[PL_SONG_STAMINA] = 1800;
                }
            } else if (p->songEffectTimer[PL_SONG_STAMINA] >= 3600) {
                p->songEffectTimer[PL_SONG_STAMINA] = 3600;
            }
            HuntingHorn_ShowSongMessage(p, 52);
        } else if (song == 13) {
            p->songEffectTimer[PL_SONG_STAMINA] = 1800;
            HuntingHorn_ShowSongMessage(p, 23);
        } else {
            p->songEffectTimer[PL_SONG_STAMINA] = 3600;
            HuntingHorn_ShowSongMessage(p, 25);
        }
        break;
    case 15:
    case 16:
        if (Rand_Next(1) % 3 != 0) {
            p->poison = 0;
            HuntingHorn_ShowSongMessage(p, 51);
        } else {
            Player_AddHp(p, 10);
            HuntingHorn_ShowSongMessage(p, 27);
        }
        break;
    case 17:
        if (Rand_Next(1) % 3 != 0) {
            p->poison = 0;
            HuntingHorn_ShowSongMessage(p, 51);
        } else {
            Player_AddHp(p, 20);
            HuntingHorn_ShowSongMessage(p, 29);
        }
        break;
    case 18:
    case 19:
        if (Player_HasSongEffect(p, PL_SONG_RES_FIRE) == 1) {
            switch (p->songEffect[PL_SONG_RES_FIRE]) {
            case 1:
            default:
            case 2:
                if (song == 18) {
                    p->songEffect[PL_SONG_RES_FIRE] = 2;
                    HuntingHorn_ShowSongMessage(p, 32);
                } else {
                    p->songEffect[PL_SONG_RES_FIRE] = 3;
                    HuntingHorn_ShowSongMessage(p, 33);
                }
                break;
            case 3:
            case 4:
                if (song == 19) {
                    p->songEffect[PL_SONG_RES_FIRE] = 4;
                    HuntingHorn_ShowSongMessage(p, 34);
                } else {
                    p->songEffect[PL_SONG_RES_FIRE] = 1;
                    HuntingHorn_ShowSongMessage(p, 31);
                }
                break;
            }
        } else if (song == 18) {
            p->songEffect[PL_SONG_RES_FIRE] = 1;
            HuntingHorn_ShowSongMessage(p, 31);
        } else {
            p->songEffect[PL_SONG_RES_FIRE] = 3;
            HuntingHorn_ShowSongMessage(p, 33);
        }
        p->songEffectTimer[PL_SONG_RES_FIRE] = 1800;
        break;
    case 20:
    case 21:
        if (Player_HasSongEffect(p, PL_SONG_RES_WATER) == 1) {
            switch (p->songEffect[PL_SONG_RES_WATER]) {
            case 1:
            default:
            case 2:
                if (song == 20) {
                    p->songEffect[PL_SONG_RES_WATER] = 2;
                    HuntingHorn_ShowSongMessage(p, 36);
                } else {
                    p->songEffect[PL_SONG_RES_WATER] = 3;
                    HuntingHorn_ShowSongMessage(p, 37);
                }
                break;
            case 3:
            case 4:
                if (song == 21) {
                    p->songEffect[PL_SONG_RES_WATER] = 4;
                    HuntingHorn_ShowSongMessage(p, 38);
                } else {
                    p->songEffect[PL_SONG_RES_WATER] = 1;
                    HuntingHorn_ShowSongMessage(p, 35);
                }
                break;
            }
        } else if (song == 20) {
            p->songEffect[PL_SONG_RES_WATER] = 1;
            HuntingHorn_ShowSongMessage(p, 35);
        } else {
            p->songEffect[PL_SONG_RES_WATER] = 3;
            HuntingHorn_ShowSongMessage(p, 37);
        }
        p->songEffectTimer[PL_SONG_RES_WATER] = 1800;
        break;
    case 22:
    case 23:
        if (Player_HasSongEffect(p, PL_SONG_RES_THUNDER) == 1) {
            switch (p->songEffect[PL_SONG_RES_THUNDER]) {
            case 1:
            default:
            case 2:
                if (song == 22) {
                    p->songEffect[PL_SONG_RES_THUNDER] = 2;
                    HuntingHorn_ShowSongMessage(p, 40);
                } else {
                    p->songEffect[PL_SONG_RES_THUNDER] = 3;
                    HuntingHorn_ShowSongMessage(p, 41);
                }
                break;
            case 3:
            case 4:
                if (song == 23) {
                    p->songEffect[PL_SONG_RES_THUNDER] = 4;
                    HuntingHorn_ShowSongMessage(p, 42);
                } else {
                    p->songEffect[PL_SONG_RES_THUNDER] = 1;
                    HuntingHorn_ShowSongMessage(p, 39);
                }
                break;
            }
        } else if (song == 22) {
            p->songEffect[PL_SONG_RES_THUNDER] = 1;
            HuntingHorn_ShowSongMessage(p, 39);
        } else {
            p->songEffect[PL_SONG_RES_THUNDER] = 3;
            HuntingHorn_ShowSongMessage(p, 41);
        }
        p->songEffectTimer[PL_SONG_RES_THUNDER] = 1800;
        break;
    case 24:
    case 25:
        if (Player_HasSongEffect(p, PL_SONG_RES_DRAGON) == 1) {
            switch (p->songEffect[PL_SONG_RES_DRAGON]) {
            case 1:
            default:
            case 2:
                if (song == 24) {
                    p->songEffect[PL_SONG_RES_DRAGON] = 2;
                    HuntingHorn_ShowSongMessage(p, 44);
                } else {
                    p->songEffect[PL_SONG_RES_DRAGON] = 3;
                    HuntingHorn_ShowSongMessage(p, 45);
                }
                break;
            case 3:
            case 4:
                if (song == 25) {
                    p->songEffect[PL_SONG_RES_DRAGON] = 4;
                    HuntingHorn_ShowSongMessage(p, 46);
                } else {
                    p->songEffect[PL_SONG_RES_DRAGON] = 1;
                    HuntingHorn_ShowSongMessage(p, 43);
                }
                break;
            }
        } else if (song == 24) {
            p->songEffect[PL_SONG_RES_DRAGON] = 1;
            HuntingHorn_ShowSongMessage(p, 43);
        } else {
            p->songEffect[PL_SONG_RES_DRAGON] = 3;
            HuntingHorn_ShowSongMessage(p, 45);
        }
        p->songEffectTimer[PL_SONG_RES_DRAGON] = 1800;
        break;
    case 26:
    case 27:
    case 28:
    case 29:
        if (Player_HasSongEffect(p, PL_SONG_RES_ICE) == 1) {
            switch (p->songEffect[PL_SONG_RES_ICE]) {
            case 1:
            default:
            case 2:
                if (song == 26 || song == 27) {
                    p->songEffect[PL_SONG_RES_ICE] = 2;
                    HuntingHorn_ShowSongMessage(p, 48);
                } else {
                    p->songEffect[PL_SONG_RES_ICE] = 3;
                    HuntingHorn_ShowSongMessage(p, 49);
                }
                break;
            case 3:
            case 4:
                if (song == 28 || song == 29) {
                    p->songEffect[PL_SONG_RES_ICE] = 4;
                    HuntingHorn_ShowSongMessage(p, 50);
                } else {
                    p->songEffect[PL_SONG_RES_ICE] = 1;
                    HuntingHorn_ShowSongMessage(p, 47);
                }
                break;
            }
        } else if (song == 26 || song == 27) {
            p->songEffect[PL_SONG_RES_ICE] = 1;
            HuntingHorn_ShowSongMessage(p, 47);
        } else {
            p->songEffect[PL_SONG_RES_ICE] = 3;
            HuntingHorn_ShowSongMessage(p, 49);
        }
        p->songEffectTimer[PL_SONG_RES_ICE] = 1800;
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
                        && (Game_IsOnline() != 1 || gScene.inQuest[i] == 1) && pl->areaMoveState == PL_AREA_MOVE_NONE
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
                && (Game_IsOnline() != 1 || sc->inQuest[0] == 1) && pl->areaMoveState == PL_AREA_MOVE_NONE
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
    Sound_PlaySeFlat(0, 34, 0);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 1405, 4, 0);
            PlayerAttack_Create(pl, 63);
            break;
        case 1:
            pl->step++;
            Player_SetMotionPair(pl, 1010, 2, 0);
            pl->weaponOut = 1;
            PlayerAttack_Create(pl, 65);
            break;
        case 2:
        case 3:
            pl->step = 2;
            Player_SetMotionPair(pl, 1405, 6, 10);
            if (variant == 3) {
                PlayerAttack_Create(pl, 66);
            } else {
                PlayerAttack_Create(pl, 63);
            }
            break;
        case 4:
            pl->step = 2;
            Player_SetMotionPair(pl, 1405, 4, 10);
            PlayerAttack_Create(pl, 63);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
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
                Player_SetMotionPair(pl, 1401, 0, 0);
                break;
            case 4:
                Player_SetMotionPair(pl, 1401, 0, 0);
                break;
            case 5:
                Player_SetMotionPair(pl, 1402, 0, 0);
                break;
            case 6:
                Player_SetMotionPair(pl, 1403, 0, 0);
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
            ShotFlash_CreateGunlanceShell(pl, variant);
            switch (pl->bladeSpecial % 3) {
            case 0:
                GunFx_CreateShell(pl, 0);
                break;
            case 1:
                GunFx_CreateShell(pl, 2);
                break;
            case 2:
                GunFx_CreateShell(pl, 1);
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
            st = 24;
            break;
        case 1:
        case 5:
            lo = 28.0f;
            hi = 58.0f;
            st = 25;
            break;
        case 2:
        case 6:
            lo = 32.0f;
            st = 26;
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        switch (variant) {
        default:
            pl->step = 2;
            Player_SetMotionPair(pl, 1416, 2, 0);
            PlayerAttack_Create(pl, 67);
            break;
        case 3:
        case 4:
            pl->step++;
            Player_SetMotionPair(pl, 1421, 4, 0);
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1416, 0, 8);
            PlayerAttack_Create(pl, 67);
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
 * sideways, then the blast: sharpness -10 and a 1350-frame cooldown
 * (weaponTimer).
 */
void Gunlance_StateWyvernFire(Player *pl, s32 variant)
{
    u16 h;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        Player_SetMotionPair(pl, 1407, 4, 0);
        pl->timer = 0;
        pl->noFlinchTimer = 2;
        break;
    case 1:
        pl->noFlinchTimer = 2;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Gunlance_SetAimLayers(pl, 1419, 1420);
            Player_SetMotionPair(pl, 1418, 0, 0);
            break;
        }
        if (Motion_ReachedFrame(pl, 0, 42.0f) != 0 && pl->stateVar0 == 0) {
            pl->stateVar0++;
            GunFx_CreateType4(pl);
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
            Player_SetMotionPair(pl, 1408, 0, 0);
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
            Gunlance_SetAimLayers(pl, 1419, 1420);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 2.0f) != 0) {
            ShotFlash_CreateGunlanceShell(pl, 3);
            GunFx_CreateShell(pl, 3);
            GunlanceBlast_Create(pl, 4);
            Player_AddSharpness(pl, -10);
            Vib_StartLocal(pl, 2);
            CamShake_ReqA(pl, 2);
            pl->weaponTimer = 1350;
        }
        if (Motion_PassedFrame(pl, 0, 44.0f) != 0) {
            GunFx_CreateType6(pl);
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        Player_SetMotionPair(pl, 1406, 4, 10);
        PlayerAttack_Create(pl, 64);
        pl->noFlinchTimer = 25;
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
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, 1404, 2, 0);
        } else {
            Player_SetMotionPair(pl, 1404, 2, 6);
        }
        pl->roundsLoaded = (u8) pl->clipSize;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (Motion_PassedFrame(pl, 0, 44.0f) != 0) {
            GunFx_CreateReload(pl, 3);
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
        pl->noFlinchTimer = 40;
        pl->weaponOut = 1;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        switch (variant) {
        case 0:
            pl->step = 2;
            Player_SetMotionPair(pl, 1409, 4, 10);
            PlayerAttack_Create(pl, 70);
            break;
        case 1:
            pl->step++;
            Player_SetMotionPair(pl, 1010, 2, 0);
            break;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1409, 0, 42);
            PlayerAttack_Create(pl, 70);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (Gunlance_CheckHit(pl) != 1) {
            if (pl->motionId[0] == 1409 && Motion_ReachedFrame(pl, 0, 44.0f) == 0) {
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
        Player_SetMotionPair(pl, 1417, 0, 0);
        CamShake_ReqA(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
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
        Player_SetMotionPair(pl, 1410, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
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

    if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75) {
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
            if (pl->input.stickTilt >= 120) {
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
    if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75)) {
        if (pl->input.stickDir & PAD_DPAD) {
            dir = (pl->input.stickAngle + ANGLE_TURN / 8) & 0xFFFF;
            if (dir < 24576) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LANCE_HOP_SIDE_B, PL_SET_SNAP_POS);
                return 1;
            }
            if ((dir >= 24576) && (dir < 0xC001)) {
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
        if (pl->motionId[0] == PL_MOT_BOW_CHARGE) {
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

    if (slot == 255) {
        player->ammoType = 0;
    } else {
        player->ammoType = gItemData[player->pouch[slot].item].ammoKind;
    }
}

/*
 * Returns the charge level: weaponTimer / 30 (faster with SKILL_FREE_SHOTS), 2 at
 * most (3 with SKILL_LOAD_UP).
 */
u8 Bow_ChargeLevel(Player *pl)
{
    s16 t = pl->weaponTimer;
    u8 lvl;
    u8 max;

    if (Player_HasSkill(pl, SKILL_FREE_SHOTS) == 1) {
        t = 1.1764705f * t;
    }
    lvl = t / 30;
    if (Player_HasSkill(pl, SKILL_LOAD_UP) == 1) {
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
 * PAD_UP/PAD_DOWN tilt aimWeight (gScene.aimReverse reverses it), PAD_LEFT/PAD_RIGHT
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
        Bow_SetAimLayers(pl, 1402, 1403);
        break;
    case 1:
        Bow_SetAimLayers(pl, 1406, 1407);
        break;
    case 2:
        Bow_SetAimLayers(pl, 1419, 1420);
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
        pl->stateVar1 = 255;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        if (variant == 0) {
            Bow_SetAimLayers(pl, 1409, 1410);
            Player_SetMotionPair(pl, 1408, 0, 0);
        } else {
            Bow_SetAimLayers(pl, 1416, 1417);
            Player_SetMotionPair(pl, 1415, 0, 0);
        }
        Vib_StartLocal(pl, 0);
        CamShake_ReqB(pl, 1);
        pl->chargeLevel = Bow_ChargeLevel(pl);
        Bow_FireArrows(pl, 0);
        pl->weaponTimer = 0;
        if (Player_HasSkill(pl, SKILL_FREE_SHOTS) == 0) {
            pl->roundsLoaded--;
        }
        if (pl->coatingSlot != 255) {
            if (Player_IsLocal(pl) == 1) {
                slot = pl->coatingSlot;
                if (Quest_PhaseIsOver(1) == 0) {
                    Pouch_Add(pl, pl->pouch[pl->coatingSlot].item, -1);
                }
                if (slot == pl->ammoSlot && pl->pouch[pl->ammoSlot].count <= 0) {
                    pl->ammoSlot = Bowgun_NextAmmoSlot(pl, 20, 2);
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
            if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75) {
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
            Bow_SetAimLayers(pl, 1406, 1407);
            if (variant == 0 || variant == 2) {
                Player_SetMotionPair(pl, PL_MOT_BOW_CHARGE, 0, 0);
                pl->weaponTimer = 0;
                pl->timer = 0;
                PlayerFx_CreateOnPlayerDefault(pl, 2, 1.0f);
            } else {
                pl->timer = 30;
                Player_SetMotionPair(pl, PL_MOT_BOW_CHARGE, 0, 120);
                Bow_TickCharge(pl);
            }
            break;
        case 3:
            pl->step++;
            Bow_SetAimLayers(pl, 1419, 1420);
            Player_SetMotionPair(pl, PL_MOT_BOW_CHARGE_FROM_SHOT, 0, 0);
            pl->weaponTimer = 0;
            pl->timer = 0;
            PlayerFx_CreateOnPlayerDefault(pl, 2, 1.0f);
            break;
        case 4:
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_BOW_CHARGE_FROM_STAB, 4, 0);
            pl->weaponTimer = 0;
            pl->timer = 0;
            PlayerFx_CreateOnPlayerDefault(pl, 2, 1.0f);
            break;
        case 5:
            pl->step++;
            pl->timer = 0;
            Player_SetMotionPair(pl, PL_MOT_BOW_CHARGE_FROM_DRAW, 0, 0);
            pl->weaponTimer = 0;
            PlayerFx_CreateOnPlayerDefault(pl, 2, 1.0f);
            break;
        }
        pl->weaponOut = 1;
        pl->roundsLoaded = pl->clipSize;
        if (pl->roundsLoaded > pl->ammoCount) {
            pl->roundsLoaded = pl->ammoCount;
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        Bow_TickCharge(pl);
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Bow_SetAimLayers(pl, 1406, 1407);
            switch (variant) {
            case 5:
            default:
                Player_SetMotionPair(pl, PL_MOT_BOW_CHARGE, 0, 30);
                break;
            case 3:
                Player_SetMotionPair(pl, PL_MOT_BOW_CHARGE, 0, 40);
                break;
            case 4:
                Player_SetMotionPair(pl, PL_MOT_BOW_CHARGE, 0, 14);
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
        Player_SetMotionPair(pl, 1412, 0, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
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
        pl->stateVar0 = 255;
        if (variant == 0) {
            pl->step = 2;
            Player_SetMotionPair(pl, 1413, 4, 0);
            PlayerAttack_CreateBowStab(pl, 60);
        } else {
            pl->step++;
            Player_SetMotionPair(pl, 1014, 2, 0);
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        pl->weaponOut = 1;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_UpdateMotionTime(pl, 0);
            Player_SetMotionPair(pl, 1413, 4, 24);
            PlayerAttack_CreateBowStab(pl, 60);
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
        if (pl->stateVar0 == 255) {
            if (Motion_InFrameRange(pl, 0, 34.0f, 64.0f) != 0) {
                if ((pl->input.stickDirPressed & PAD_RSTICK_UP) && pl->aimLockTimer == 0 && pl->aimMode == 0) {
                    pl->stateVar0 = 4;
                } else if (pl->input.pressed & PAD_CROSS) {
                    pl->stateVar0 = 18;
                } else if (pl->input.stickDir & PAD_RSTICK_DOWN) {
                    pl->stateVar0 = 19;
                }
            }
        } else if (Motion_ReachedFrame(pl, 0, 44.0f) != 0) {
            x = pl->stateVar0;
            if (x == 18) {
                if (pl->stamina >= 75) {
                    Player_RequestState(pl, PL_GROUP_WEAPON, BOW_STAB_EVADE, PL_SET_SNAP_POS);
                }
            } else if (x == 4) {
                Bow_UpdateAmmoType(pl);
                Player_RequestState(pl, PL_GROUP_WEAPON, BOW_STAB_2, PL_SET_SNAP_POS);
            } else if (x == 19) {
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
        Player_SetMotionPair(pl, 1414, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        PlayerAttack_CreateBowStab(pl, 61);
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
            Player_SetMotionPair(pl, 1401, 4, 0);
        } else {
            Player_SetMotionPair(pl, 1401, 0, 0);
        }
        Bow_SetAimLayers(pl, 1402, 1403);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
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
                Player_ReturnToIdle(pl, 0, 10, 0);
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
        Player_SetMotionPair(pl, PL_MOT_BOW_CHARGE_WALK, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
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
            Player_SetMotionPair(pl, 1006, 2, 0);
            break;
        case 1:
            pl->step = 2;
            Player_SetMotionPair(pl, 1006, 0, 6);
            break;
        case 2:
            Player_SetMotionPair(pl, 1011, 2, 0);
            break;
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_AddStamina(pl, -75);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 1006, 0, 14);
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
        Player_SetMotionPair(pl, 1010, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
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
        Player_SetMotionPair(pl, 1411, 0, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
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
 * motion's speed by the reload skills (SKILL_RELOAD_*).
 */
void Bow_StateLoadCoating(Player *pl, s32 unused)
{
    u16 k;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 1404, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->motionSpeed = 1.0f;
        if (Player_HasSkill(pl, SKILL_RELOAD_PLUS1) == 1) {
            pl->motionSpeed = 1.17647052f;
        } else if (Player_HasSkill(pl, SKILL_RELOAD_PLUS2) == 1) {
            pl->motionSpeed = 1.33333337f;
        } else if (Player_HasSkill(pl, SKILL_RELOAD_PLUS3) == 1) {
            pl->motionSpeed = 1.53846157f;
        } else if (Player_HasSkill(pl, SKILL_RELOAD_MINUS1) == 1) {
            pl->motionSpeed = 0.909090877f;
        } else if (Player_HasSkill(pl, SKILL_RELOAD_MINUS3) == 1) {
            pl->motionSpeed = 0.714285731f;
        }
        k = pl->pouch[pl->ammoSlot].item;
        if (k == 135 || k == 0) {
            Bow_ClearCoatingSlot(pl);
        } else if (pl->coatingSlot != 255) {
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

    if (pl->stamina >= 75 && (pl->input.pressed & PAD_CROSS)) {
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
    if ((pl->input.pressed & PAD_R3) && pl->pouch[pl->ammoSlot].item != 135 && pl->coatingSlot == 255) {
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
    if (pl->stamina >= 75 && (pl->input.pressed & PAD_CROSS)) {
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
    if ((pl->input.pressed & PAD_R3) && pl->pouch[pl->ammoSlot].item != 135 && pl->coatingSlot == 255) {
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

/* The file's data. */
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
