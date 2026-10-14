#include "common.h"
#include "net_mode.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_globals.h"

void NetMode_SetCursorX(s8 which);



/* closes the boxes in `bits` (0x5D46BC); closing box 0x10 or 0x2000 opens
 * the next one (0x4000, 0x8000) through NetMode_SetCursorX */
void NetMode_CloseBoxes(s32 bits)
{
    gNetMode.boxes &= ~bits;
    if (bits == 0x10) {
        gNetMode.openW = 0;
        gNetMode.openH = 1;
        gNetMode.boxes |= 0x4000;
        NetMode_SetCursorX(0);
    } else if (bits == 0x2000) {
        gNetMode.openW = 0;
        gNetMode.openH = 1;
        gNetMode.boxes |= 0x8000;
        NetMode_SetCursorX(1);
    }
}

void NetMode_OpenFrame(void)
{
    gNetMode.boxes = (s32) (gNetMode.boxes | 1);
}

void NetMode_OpenIcon(void)
{
    gNetMode.icon = 0;
    gNetMode.boxes = (s32) (gNetMode.boxes | 0x20);
}

void NetMode_OpenIconN(s32 arg0)
{
    gNetMode.icon = (s8) arg0;
    gNetMode.boxes = (s32) (gNetMode.boxes | 0x20);
}

void NetMode_OpenIcon2(s8 arg0)
{
    gNetMode.icon2 = arg0;
    gNetMode.boxes = (s32) (gNetMode.boxes | 0x40000);
}

/* opens boxes 0x800, 0x1000 and 0x2000 with two values */
/* Raw addresses kept here: the named form changes the code. */
void NetMode_OpenMenu2(s8 a, s8 b)
{
    gNetMode.unk26 = b;
    gNetMode.layout1 = a;
    gNetMode.boxes |= 0x800;
    gNetMode.boxes |= 0x1000;
    gNetMode.boxes |= 0x2000;
}

void NetMode_OpenProgress(s32 unused)
{
    gNetMode.anim0 = 0;
    gNetMode.anim0Frame = 0;
    gNetMode.blink = 0;
    gNetMode.blinkOn = 0;
    gNetMode.anim1 = 0;
    gNetMode.anim1Frame = 0;
    gNetMode.boxes = (s32) (gNetMode.boxes | 0x40);
}

void NetMode_OpenProgress2(void)
{
    gNetMode.anim1 = 0;
    gNetMode.anim1Frame = 0;
    gNetMode.boxes = (s32) (gNetMode.boxes | 0x800000);
}

void NetMode_OpenPercent(void)
{
    gNetMode.boxes = (s32) (gNetMode.boxes | 0x80);
}

/* opens boxes 0x100, 0x200 and 0x10000 */
/* Raw addresses kept here: the named form changes the code. */
void NetMode_ResetAnim2(void)
{
    gNetMode.anim0 = 0;
    gNetMode.anim0Frame = 0;
    gNetMode.blink = 0;
    gNetMode.blinkOn = 0;
    gNetMode.anim1 = 0;
    gNetMode.anim1Frame = 0;
    gNetMode.boxes |= 0x100;
    gNetMode.boxes |= 0x200;
    gNetMode.boxes |= 0x10000;
}

void NetMode_OpenError(void)
{
    gNetMode.boxes = (s32) (gNetMode.boxes | 0x20000);
}

void NetMode_OpenYesNoBar(void)
{
    gNetMode.boxes = (s32) (gNetMode.boxes | 0x400000);
}
