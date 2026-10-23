/*
 * The network screen's boxes: closing them and opening each one.
 *
 * gNetMode.boxes holds a bit for each box of the network screen that is
 * shown (NET_BOX_*); NetMode_DrawScreen draws the boxes whose bits are set.
 * The openers set a box's bit and reset the counters its drawing animates
 * by; NetMode_CloseBoxes clears bits, and closing a menu's cursor starts
 * the chosen row's opening animation.
 */
#include "common.h"
#include "net_mode.h"
#include "lobby_globals.h"

/* Closes the boxes in `bits` (NET_BOX_*). Closing a menu's cursor alone
 * (NET_BOX_MENU0_CURSOR or NET_BOX_MENU1_CURSOR) opens the menu's chosen
 * row's animation (NET_BOX_MENU0_PICKED or NET_BOX_MENU1_PICKED) from its
 * first step, at the cursor's place. */
void NetMode_CloseBoxes(s32 bits)
{
    gNetMode.boxes &= ~bits;
    if (bits == NET_BOX_MENU0_CURSOR) {
        gNetMode.openW = 0;
        gNetMode.openH = 1;
        gNetMode.boxes |= NET_BOX_MENU0_PICKED;
        NetMode_SetCursorY(0);
    } else if (bits == NET_BOX_MENU1_CURSOR) {
        gNetMode.openW = 0;
        gNetMode.openH = 1;
        gNetMode.boxes |= NET_BOX_MENU1_PICKED;
        NetMode_SetCursorY(1);
    }
}

void NetMode_OpenFrame(void)
{
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_FRAME);
}

/* Opens the icon box with icon 0 (gNetModeIcons). */
void NetMode_OpenIcon(void)
{
    gNetMode.icon = 0;
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_ICON);
}

/* Opens the icon box with icon `icon` (gNetModeIcons). */
void NetMode_OpenIconN(s32 icon)
{
    gNetMode.icon = (s8) icon;
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_ICON);
}

/* Opens the second icon box with icon `icon` (gNetModeIcons). */
void NetMode_OpenIcon2(s8 icon)
{
    gNetMode.icon2 = icon;
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_ICON2);
}

/* Opens menu 1 (its rows, its chosen row and its cursor) in `layout`
 * (gNetMode.layout1: the cursor's places, NetMode_SetCursorY). `arg` is
 * kept in gNetMode.unk26, which nothing reads. */
void NetMode_OpenMenu(s8 layout, s8 arg)
{
    gNetMode.unk26 = arg;
    gNetMode.layout1 = layout;
    gNetMode.boxes |= NET_BOX_MENU1_ROWS;
    gNetMode.boxes |= NET_BOX_MENU1_LIT;
    gNetMode.boxes |= NET_BOX_MENU1_CURSOR;
}

/* Opens the progress bar, with its animations from their first frames. */
void NetMode_OpenProgress(s32 unused)
{
    gNetMode.anim0 = 0;
    gNetMode.anim0Frame = 0;
    gNetMode.blink = 0;
    gNetMode.blinkOn = 0;
    gNetMode.anim1 = 0;
    gNetMode.anim1Frame = 0;
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_PROGRESS);
}

/* Opens the second progress bar, with its icon from its first frame. */
void NetMode_OpenProgress2(void)
{
    gNetMode.anim1 = 0;
    gNetMode.anim1Frame = 0;
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_PROGRESS2);
}

void NetMode_OpenPercent(void)
{
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_PERCENT);
}

/* Opens the transfer's status: its messages, its two status sprites and
 * the empty status box, with their animations from their first frames. */
void NetMode_OpenStatus(void)
{
    gNetMode.anim0 = 0;
    gNetMode.anim0Frame = 0;
    gNetMode.blink = 0;
    gNetMode.blinkOn = 0;
    gNetMode.anim1 = 0;
    gNetMode.anim1Frame = 0;
    gNetMode.boxes |= NET_BOX_STATUS_MSG;
    gNetMode.boxes |= NET_BOX_STATUS;
    gNetMode.boxes |= NET_BOX_STATUS_NONE;
}

void NetMode_OpenError(void)
{
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_ERROR);
}

void NetMode_OpenYesNoBar(void)
{
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_YES_NO);
}

/* Closes both yes/no bars. */
void NetMode_CloseYesNo(void)
{
    NetMode_CloseBoxes(NET_BOX_YES_NO_LARGE);
    NetMode_CloseBoxes(NET_BOX_YES_NO);
}

/* Opens the memory card slot's choice. */
void NetMode_OpenSlotChoice(void)
{
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_SLOT_CHOICE);
}
