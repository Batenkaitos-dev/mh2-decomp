/*
 * The network screen's message queue helpers, and the software keyboard
 * and overlay loads for yn's network settings.
 *
 * NetModeMsg_QueuePage4, NetModeMsg_QueueField and NetModeMsg_QueueCountdown
 * add a message of their kind to one of gNetMode's two queues (A, drawn
 * under the boxes, and B, over them: NetModeMsg_Flush). NetKbd_* run the
 * software keyboard that yn's settings screens type into: NetKbd_Open opens
 * it on a copy of the text (gNetKbdText) with one of the six layouts of
 * gNetKbdParams, the file's .data. NetOverlay_Load and NetOverlay_Busy load
 * the network overlays for the set-up.
 */
#include "common.h"
#include "net_mode.h"
#include "pad.h"
#include "lobby_globals.h"
#include "bgm.h"
#include "chat_box.h"
#include "kbd.h"
#include "window.h"
#include "main.h"
void *memset(void *, s32, u32);

s8 Kbd_Run(char *dst, s16 held, s16 pressed);
extern u8 gNetKbdOn;
extern char gNetKbdText[256];

char *strcpy(void *dst, const void *src);

/* A software keyboard set-up of yn's settings screens (NetKbd_Open). */
typedef struct NetKbdParam {
    u32 flags;                  /* Chat_OpenKbd's flags */
    u8 layout;                  /* Chat_OpenKbd's layout */
    u8 pad5[3];
} NetKbdParam;

extern NetKbdParam gNetKbdParams[];

/* Does nothing: the patch's steps call it each frame with the message
 * shown. */
void NetMode_EmptyMsgHook(u8 unused)
{

}

/* Queues part `part` (1-14) of page 4 in queue B; 0 queues nothing. */
void NetModeMsg_QueuePage4(u8 part)
{
    NetModeState *st;

    st = &gNetMode;
    switch (part) {
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
        if (st->nB < NETMODE_QUEUE_LEN) {
            st->idB[st->nB] = part;
            st->kindB[st->nB] = NETMODE_KIND_PAGE4;
            st->nB++;
        }
        break;
    }
}

/* Queues the printing of field `field` (0-9, NET_FIELD_*): fields 3 and 4
 * in queue A, the others in queue B. */
void NetModeMsg_QueueField(u8 field)
{
    NetModeState *st;

    st = &gNetMode;
    switch (field) {
    case 0:
    case NET_FIELD_LOGIN_ID:
    case NET_FIELD_PASSWORD:
    case NET_FIELD_PERCENT:
    case NET_FIELD_ERROR_CODE:
    case NET_FIELD_VERSION:
    case 8:
    case 9:
        if (st->nB < NETMODE_QUEUE_LEN) {
            st->idB[st->nB] = field;
            st->kindB[st->nB] = NETMODE_KIND_FIELD;
            st->nB++;
        }
        break;
    case 3:
    case 4:
        if (st->nA < NETMODE_QUEUE_LEN) {
            st->idA[st->nA] = field;
            st->kindA[st->nA] = NETMODE_KIND_FIELD;
            st->nA++;
        }
        break;
    }
}

/* Queues the countdown in layout `layout` (LobbyMsg_DrawCountdown's 0-2)
 * in queue B. */
void NetModeMsg_QueueCountdown(u8 layout)
{
    NetModeState *st;

    st = &gNetMode;
    if (st->nB < NETMODE_QUEUE_LEN) {
        st->idB[st->nB] = layout;
        st->kindB[st->nB] = NETMODE_KIND_COUNTDOWN;
        st->nB++;
    }
}

/* Loads overlay `id` (LoadOverlay). */
void NetOverlay_Load(s32 id)
{
    LoadOverlay(id);
}

/* 0: an overlay load never waits here. */
s32 NetOverlay_Busy(void)
{
    return 0;
}

/* Runs the software keyboard one frame while it is open (gNetKbdOn), on
 * gNetKbdText with the first pad's buttons. Returns Kbd_Run's result
 * (nonzero once the text is entered), or 0 while it is closed. */
s32 NetKbd_Run(void)
{
    s32 ret;

    ret = 0;
    Kbd_ReadUsb();
    if (gNetKbdOn == 1) {
        /* the buttons are read as s16, as Kbd_Run takes them */
        ret = (s8)(Kbd_Run(gNetKbdText, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed));
    }
    return ret;
}

/* Opens the software keyboard at (x, y) with set-up `kind` of
 * gNetKbdParams, at most maxChars letters, on a copy of text (gNetKbdText),
 * and marks it open. */
void NetKbd_Open(s32 x, s32 y, void *text, s32 maxChars, s32 kind)
{
    s32 off;

    Kbd_SetPos((s16) y, (f32) x);
    /* the set-up is read through byte offsets: its flags, then its
     * layout 4 bytes on */
    off = kind * sizeof(NetKbdParam);
    Chat_OpenKbd(*(u32 *)((u8 *)gNetKbdParams + off), (&gNetKbdParams[0].layout)[off], maxChars & 0xFFFF, text);
    memset(gNetKbdText, 0, sizeof(gNetKbdText));
    strcpy(gNetKbdText, (void *) text);
    NetKbd_SetActive(1);
}

/* The text being typed. */
char *NetKbd_GetText(void)
{
    return gNetKbdText;
}

/* Marks the keyboard open (1) or closed (0); other values change nothing. */
void NetKbd_SetActive(s32 on)
{
    s32 value;

    value = on & 0xFF;
    switch (value) {
    case 0:
        gNetKbdOn = 0;
        break;
    case 1:
        gNetKbdOn = 1;
        break;
    }
}

/* Draws the keyboard's box while it is open. */
void NetKbd_Draw(void)
{
    if (gNetKbdOn == 1) {
        Chat_DrawKbdBox(0xBD6C6CC5);
    }
}

/* Does nothing: NetMode_DrawScreen calls it after the text pages. */
void NetMode_EmptyDrawEnd(void)
{

}

/* The file's .data */
NetKbdParam gNetKbdParams[6] = {
#include "lobby/00819840.inc"
};
