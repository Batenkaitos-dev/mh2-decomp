/*
 * The login screens' input and sounds: the pad reads (LoginPad_*: the
 * buttons pressed, held or repeating, Circle, Cross and Start), the
 * sounds of the menus and of a problem's message (LoginSe_*,
 * LoginMenu_Play*), the reads that play their sound (LoginPad_Decide,
 * _DecideEx, _Cancel), and NetMode_ChooseYesNo, the network screens'
 * yes/no choice.
 */

#include "common.h"
#include "net_save.h"
#include "net_mode.h"
#include "scene.h"
#include "pad.h"
#include "sound.h"

/* The menus' cursor sound. */
void LoginMenu_PlayCursorSe(void)
{
    LoginSe_Cursor();
}

/* The menus' confirm sound. */
void LoginMenu_PlayDecideSe(void)
{
    LoginSe_Decide();
}

/* The buttons pressed or repeating on the local player's pad. */
s32 LoginPad_Triggered(void)
{
    s32 pad;
    u8 port;

    port = gScene.padIndex[0];
    pad = 0;
    pad |= gPadState[port].pressed | gPadState[port].repeat;
    return pad;
}

/* The buttons pressed this frame on the first pad. */
s32 LoginPad_Pressed(void)
{
    s32 pad;

    pad = 0;
    pad |= gPadState[0].pressed;
    return pad;
}

/* The buttons held on pad `port`: only port 0 is read, any other gives 0. */
s32 LoginPad_Held(s8 port)
{
    s32 pad;

    pad = 0;
    if ((1 << port) & 1) {
        pad |= gPadState[0].held;
    }
    return pad;
}

/* Circle pressed on the first pad (the port is ignored). */
s32 LoginPad_DecidePressed(s32 port)
{
    return (gPadState[0].pressed & PAD_CIRCLE) != 0;
}

/* Cross pressed on the first pad (the port is ignored). */
s32 LoginPad_CancelPressed(s32 port)
{
    return (gPadState[0].pressed & PAD_CROSS) != 0;
}

/* Start held on the first pad (the port is ignored). */
s32 LoginPad_StartHeld(s32 port)
{
    return (gPadState[0].held & PAD_START) != 0;
}

/* Start held, as asked of ports 0 and 1: 1 or 0. Holding it cancels a
 * connection (gNetCancelHold). */
s32 LoginPad_AnyStartHeld(void)
{
    if ((LoginPad_StartHeld(0) == 0) && (LoginPad_StartHeld(1) == 0)) {
        return 0;
    }
    return 1;
}

/* The system bank's confirm sound. */
void LoginSe_Decide(void)
{
    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
}

/* The system bank's cancel sound. */
void LoginSe_Cancel(void)
{
    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
}

/* The sound of a problem's message (a card or connection problem). */
void LoginSe_Alert(void)
{
    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_DESCRIPTION, 0);
}

/* The system bank's sound for a cursor or a yes/no choice. */
void LoginSe_Cursor(void)
{
    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
}

/* Circle pressed: plays LOGIN_SE_DECIDE's or LOGIN_SE_ALERT's sound (or
 * none for LOGIN_SE_NONE) and returns 1; else 0. */
s32 LoginPad_Decide(s32 sound)
{
    s32 ret;

    ret = 0;
    if (LoginPad_DecidePressed(0) != 0) {
        switch (sound) {
        case LOGIN_SE_NONE:
            break;
        case LOGIN_SE_DECIDE:
            LoginSe_Decide();
            break;
        case LOGIN_SE_ALERT:
            LoginSe_Alert();
            break;
        }
        ret = 1;
    }
    return ret;
}

/* As LoginPad_Decide, for the online menu: with `idRow` set (the login
 * ID's row), the confirm sound plays only when a login ID is saved, the
 * alert's otherwise. */
s32 LoginPad_DecideEx(s32 sound, s32 idRow)
{
    s32 ret;

    ret = 0;
    if (LoginPad_DecidePressed(0) != 0) {
        switch (sound) {
        case LOGIN_SE_NONE:
            break;
        case LOGIN_SE_DECIDE:
            if (idRow == 1) {
                if (gNetSaveData.mmbbLoginId[0] == LOGIN_ID_EMPTY) {
                    LoginSe_Alert();
                } else {
                    LoginSe_Decide();
                }
            } else {
                LoginSe_Decide();
            }
            break;
        case LOGIN_SE_ALERT:
            LoginSe_Alert();
            break;
        }
        ret = 1;
    }
    return ret;
}

/* Cross pressed: plays the cancel sound and returns 1; else 0. */
s32 LoginPad_Cancel(void)
{
    s32 result;

    result = 0;
    if (LoginPad_CancelPressed(0) != 0) {
        LoginSe_Cancel();
        result = 1;
    }
    return result;
}

/* A yes/no choice on the network screens: left and right move the cursor
 * (gNetMode.yesNo: 0 yes, 1 no) with the cursor's sound. Returns 1 when yes
 * is decided, -1 when no is decided or the choice is cancelled, 0 while
 * choosing. */
s32 NetMode_ChooseYesNo(void)
{
    s32 pad;

    pad = LoginPad_Pressed();
    if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if ((pad & PAD_LEFT) && gNetMode.yesNo != 0) {
            gNetMode.yesNo = 0;
            LoginSe_Cursor();
            return 0;
        }
        if ((pad & PAD_RIGHT) && gNetMode.yesNo == 0) {
            gNetMode.yesNo = 1;
            LoginSe_Cursor();
            return 0;
        }
    }
    if (LoginPad_Decide(LOGIN_SE_DECIDE) != 0) {
        return (gNetMode.yesNo == 0) ? 1 : -1;
    }
    return -(LoginPad_Cancel() != 0);
}
