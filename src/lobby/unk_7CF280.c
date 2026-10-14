#include "common.h"
#include "net_mode.h"
#include "lobby_globals.h"

s32 LoginPad_Pressed(void);
void LoginSe_Cursor(void);
s32 LoginPad_Decide(s32 sound);
s32 LoginPad_Cancel(void);

/* a yes/no choice: left and right move the cursor (0x5D46B8: 0 yes, 1 no)
 * with a sound; returns 1 for yes, -1 for no or cancel, 0 while choosing */
s32 NetMode_ChooseYesNo(void)
{
    s32 pad;

    pad = LoginPad_Pressed();
    if (pad & 0xC00) {
        if ((pad & 0x800) && gNetMode.yesNo != 0) {
            gNetMode.yesNo = 0;
            LoginSe_Cursor();
            return 0;
        }
        if ((pad & 0x400) && gNetMode.yesNo == 0) {
            gNetMode.yesNo = 1;
            LoginSe_Cursor();
            return 0;
        }
    }
    if (LoginPad_Decide(1) != 0) {
        return (gNetMode.yesNo == 0) ? 1 : -1;
    }
    return -(LoginPad_Cancel() != 0);
}
