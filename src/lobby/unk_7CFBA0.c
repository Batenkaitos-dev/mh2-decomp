/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "net_mode.h"
#include "em.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
void *memset(void *, s32, u32);



void NetMode_CloseYesNo(void);
void func_007CFBF0(u8 unused);
void NetOverlay_Load(s32 id);
s32 NetOverlay_Busy(void);
char *NetKbd_GetText(void);
void NetKbd_Draw(void);
void func_007CFF30(void);
void Kbd_ReadUsb(void);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
extern u8 gNetKbdOn;
extern M2C_UNK gNetKbdText;
void NetMode_CloseBoxes(s32);
void LoadOverlay(s32 id);
void Chat_DrawKbdBox(s32 color);

void NetMode_CloseYesNo(void) {
    NetMode_CloseBoxes(0x100000);
    NetMode_CloseBoxes(0x400000);
}

void NetKbd_SetActive(s32 arg0);
char *strcpy(void *dst, const void *src);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
extern u8 gNetKbdParams[];
extern u8 D_00819844[];

void NetMode_OpenBit21(void)
{
    gNetMode.boxes = (s32) (gNetMode.boxes | 0x200000);
}

void func_007CFBF0(u8 unused) {

}

/* queues message id (1-14) in the second queue with kind 3 */
void NetModeMsg_QueueKind3(u8 id)
{
    NetModeState *st;

    st = &gNetMode;
    switch (id) {
    case 0:
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        if (st->nB < 8) {
            st->idB[st->nB] = id;
            st->kindB[st->nB] = 3;
            st->nB++;
        }
        break;
    }
}

/* queues message id (0-9) with kind 2: 3 and 4 in the first queue, the
 * others in the second */
void NetModeMsg_QueueField(u8 id)
{
    NetModeState *st;

    st = &gNetMode;
    switch (id) {
    case 0:
    case 1:
    case 2:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        if (st->nB < 8) {
            st->idB[st->nB] = id;
            st->kindB[st->nB] = 2;
            st->nB++;
        }
        break;
    case 3:
    case 4:
        if (st->nA < 8) {
            st->idA[st->nA] = id;
            st->kindA[st->nA] = 2;
            st->nA++;
        }
        break;
    }
}

/* queues id in the second queue with kind 6 */
void NetModeMsg_QueueCountdown(u8 id)
{
    NetModeState *st;

    st = &gNetMode;
    if (st->nB < 8) {
        st->idB[st->nB] = id;
        st->kindB[st->nB] = 6;
        st->nB++;
    }
}

void NetOverlay_Load(s32 id) {
    LoadOverlay(id);
}

s32 NetOverlay_Busy(void) {
    return 0;
}

s32 NetKbd_Run(void)
{
    s32 ret;

    ret = 0;
    Kbd_ReadUsb();
    if (gNetKbdOn == 1) {
        ret = (s8)(Kbd_Run((char *) &gNetKbdText, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed));
    }
    return ret;
}

void NetKbd_Open(s32 f, s32 n, void *d, s32 arg3, s32 arg4)
{
    s32 temp_a0;

    Kbd_SetPos((s16) n, (f32) f);
    temp_a0 = arg4 * 8;
    Chat_OpenKbd(*(u32 *)(gNetKbdParams + temp_a0), (s32) *(D_00819844 + temp_a0), arg3 & 0xFFFF, d);
    memset(&gNetKbdText, 0, 0x100);
    strcpy(&gNetKbdText, (void *) d);
    NetKbd_SetActive(1);
}

char *NetKbd_GetText(void) {
    return (char *) &gNetKbdText;
}

void NetKbd_SetActive(s32 arg0)
{
    s32 temp_a0;

    temp_a0 = arg0 & 0xFF;
    switch (temp_a0) {
    case 0:
        gNetKbdOn = 0;
        break;
    case 1:
        gNetKbdOn = 1;
        break;
    }
}

void NetKbd_Draw(void) {
    if (gNetKbdOn == 1) {
        Chat_DrawKbdBox(0xBD6C6CC5);
    }
}

void func_007CFF30(void) {

}

/* unk_7CFBA0's .data (0x819840-0x819870) */
u8 gNetKbdParams[48] = {
#include "lobby/00819840.inc"
};
