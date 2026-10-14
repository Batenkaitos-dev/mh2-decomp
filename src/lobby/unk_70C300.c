/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"

void Stream_Pause(s32, s32);
void Bgm_SetPaused(u8);
void Bgm_FadeInFromZero(s32 fade);
extern s8 gTownMsg0DPending;

void TownNetMenu_Resume(void) {
    gLobby->connected = 1;
    gTownMsg0DPending = 1;
    gFieldControl = 1;
    Stream_Pause(0, 0);
    Bgm_FadeInFromZero(0x1E);
    Bgm_SetPaused(0);
}
