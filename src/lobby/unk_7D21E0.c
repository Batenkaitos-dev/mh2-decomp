/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "sprite.h"
#include "syssettings.h"
#include "net_mode.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"

s32 CardOp_SetFiles(s32 *data);



typedef struct Spr {
    s16 x, y, w, h;
    u32 color;
    UV uv0;
    UV uv1;
} Spr;

extern u8 *gLoadBuf;
extern u8 gNetSaveData[];               /* the lobby's data saved in the NET file (0x1BEC) */
void Lobby_SetSysBusy(void);
void Lobby_ClearSysBusy(void);
void NetModeMsg_Page1(s32 msg);
void NetModeMsg_QueueCountdown(u8 id);
s32 LoginPad_Decide(s32 sound);
void LobbyCard_FixNetValues(u8 *buf);
void CardTask_Begin(s32 mode);
void CardTask_StartReadClear(s32 slot, u8 *buf, s32 a);
void CardTask_Update(void);
s32 CardTask_Result(void);
void CardTask_StartSave(s32 slot, u8 *buf);
void CardTask_StartCheck(void);
void Card_ClearChanged(void);
s32 Card_IsChanged(s32 slot);
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
s32 Gs_GetState(u8 reg);
s32 Gs_SetState(u8, u32);
s32 Tex_Use(s32 n, u32 *handles);
struct Sprite;
s32 Sprite_Draw(struct Sprite *);
extern f32 gWaveTitleSine[12];
extern UV D_008198E0[20];
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetSize(f32 scale);
void Font_SetColorIndex(s16 c);
s32 Str_Length(char *s);
void Font_Printf(char *fmt, ...);
void Font_DrawPage(s32 layer);
s32 LoadCompressedFile(u8 *dst, s32 id);
extern char *gFullWidthDigits;           /* the full-width digits */
extern char **D_0081B4F4;

s32 func_007D21E0(void) {
    return gNetMode.tex[9];
}

/* draws the 10 letters of the title (font texture tex[11], D_008198E0:
 * each letter's UV then its x, y) waving: letter i is lowered by 4 *
 * gWaveTitleSine[phase] (a sine table of 12), the phase moving one step every
 * other frame (0x5D4779 counts the frames); a black shadow at +2, +2 */
/* Raw addresses kept here: the named form changes the code. */
void LobbyLogin_DrawWaveTitle(s32 unused0)
{
    NetModeState *st;
    s32 r6C;
    s32 r63;
    s32 r5E;
    Spr s;
    s32 i;
    s32 k;
    UV *p;
    s16 x;
    s16 y;

    st = &gNetMode;
    ((*(s8 *)&gNetMode.padD9[0]))++;
    if ((*(s8 *)&gNetMode.padD9[0]) & 1) {
        st->wave++;
    }
    if (st->wave > 11) {
        st->wave -= 12;
    }
    r6C = Gs_GetState(0x6C);
    r63 = Gs_GetState(0x63);
    r5E = Gs_GetState(0x5E);
    Gs_SetState(0x6C, 0);
    Tex_Use(1, (u32 *) &st->tex[11]);
    Gs_SetState(4, st->tex[11]);
    Gs_SetState(0x63, 0);
    Gs_SetState(0x5E, 0x32);
    s.w = 0x1C;
    s.h = 0x20;
    p = D_008198E0;
    k = st->wave;
    for (i = 0; i < 10; i++) {
        s.uv0.u = p->u;
        s.uv0.v = p->v;
        s.uv1.u = s.uv0.u + 0x1F;
        s.uv1.v = s.uv0.v + 0x1F;
        x = (++p)->u;
        y = p->v + 4.0f * gWaveTitleSine[k];
        p++;
        s.color = 0xFF000000;
        s.x = x + 2;
        s.y = y + 2;
        s.w = 0x1C;
        s.h = 0x20;
        Sprite_Draw((struct Sprite *)&s);
        s.color = -1;
        s.x = x;
        s.y = y;
        s.w = 0x1C;
        s.h = 0x20;
        Sprite_Draw((struct Sprite *)&s);
        if (i == 2) {
            k += 2;
        } else {
            k++;
        }
        if (k > 11) {
            k -= 12;
        }
    }
    Gs_SetState(0x6C, r6C);
    Gs_SetState(0x63, r63);
    Gs_SetState(0x5E, r5E);
}

s32 LobbyCard_LoadFiles(s32 mode)
{
    s32 ret;
    u8 step;

    step = gNetMode.innerStep;
    ret = 0;
    switch (step) {
    case 0:
        gNetMode.innerStep = (u8) (step + 1);
        if (mode == 0) {
            LoadCompressedFile((u8 *)((s32)gLoadBuf + 0x20000), 0x1715);
            CardOp_SetFiles((s32 *) (*(s32 *)&gLoadBuf + 0x20000));
        } else {
            LoadCompressedFile((u8 *)((s32)gLoadBuf + 0x20000), 0x1714);
        }
        break;
    case 1:
        gNetMode.innerStep = (u8) (step + 1);
        
    case 2:
        ret = 1;
        break;
    }
    return ret;
}

/* saves the NET file to the memory card (NetModeState.unk3 its step): the
 * card's current data read (slot 0x6ADFDA), the lobby's data (gNetSaveData,
 * 0x1BEC bytes) written; a failure shows message 0x17, 0x18, 0x25 or 0xE
 * and checks the card again. 1 when saved, -1 when given up, else 0 */
s32 LobbyCard_SaveNetFile(void)
{
    NetModeState *st = &gNetMode;
    s32 r;

    r = 0;
    switch (st->innerStep) {
    case 0:
        Lobby_SetSysBusy();
        st->innerStep++;
        st->slot = gSysSettings.unk4A;
        st->slotUsed = st->slot;
        CardTask_Begin(1);
        LoadCompressedFile((u8 *)((s32)gLoadBuf + 0x20000), 0x1715);
        st->files = CardOp_SetFiles((s32 *)((s32)gLoadBuf + 0x20000));
        CardTask_StartReadClear(st->slotUsed, gLoadBuf, 0);
        break;
    case 1:
        NetModeMsg_Page1(0x2F);
        CardTask_Update();
        switch (CardTask_Result()) {
        case -1:
            break;
        case 0:
        case -0xFB:
            st->innerStep++;
            st->slot = st->slotUsed;
            memset(gLoadBuf, 0, 0x1BEC);
            memcpy(gLoadBuf, gNetSaveData, 0x1BEC);
            LobbyCard_FixNetValues(gLoadBuf);
            CardTask_StartSave(st->slot, gLoadBuf);
            break;
        case -0xFF:
            st->msg = 0x17;
            goto check;
        case -0x100:
            st->msg = 0x25;
            goto check;
        default:
            st->msg = 0x18;
        check:
            Card_ClearChanged();
            CardTask_StartCheck();
            st->innerStep = 3;
            st->timer = 0x744;
            break;
        }
        break;
    case 2:
        NetModeMsg_Page1(0xC);
        CardTask_Update();
        switch (CardTask_Result()) {
        case -1:
            break;
        case 0:
            Lobby_ClearSysBusy();
            r = 1;
            break;
        case -0xFF:
            st->msg = 0x17;
            goto check;
        case -0x100:
        case -0xFB:
            st->msg = 0xE;
            goto check;
        }
        break;
    case 3:
        NetModeMsg_Page1(st->msg);
        NetModeMsg_QueueCountdown(0);
        CardTask_Update();
        if (st->timer > 0) {
            if (Card_IsChanged(st->slotUsed)) {
                CardTask_StartReadClear(st->slotUsed, gLoadBuf, 0);
                st->innerStep = 1;
            } else if (LoginPad_Decide(2)) {
                Lobby_ClearSysBusy();
                r = -1;
            }
        } else {
            Lobby_ClearSysBusy();
            r = -1;
        }
        break;
    }
    return r;
}

void LobbyCard_OpenMsgBox(s8 step, s8 msg) {
    gNetMode.saveNext = step;
    gNetMode.cardMsg = msg;
    gNetMode.saveStep = 0x64;
    gNetMode.percent = 8;
    gNetMode.timer = 0x744;
}

void LobbyCard_CloseMsgBox(s8 step) {
    gNetMode.saveNext = step;
    gNetMode.saveStep = 0x6E;
}

/* counts NetModeState.timer down and prints the seconds left (timer / 60,
 * in full-width digits from gFullWidthDigits) centred, with the message
 * D_0081B4F4[0] under it; mode 0, 1, 2 the layouts (y 0x160/0x132,
 * 0x14C/0x11E) */
void LobbyMsg_DrawCountdown(u8 mode)
{
    char buf[8];
    char *p;
    char **q;
    char *d;
    s32 sec;
    s32 tens;
    s32 ones;
    s16 x;

    if (gNetMode.timer > 0) {
        (gNetMode.timer)--;
    }
    sec = gNetMode.timer / 60;
    tens = sec / 10;
    ones = sec % 10;
    p = buf;
    if (tens > 0) {
        d = &gFullWidthDigits[tens * 2];
        *p++ = d[0];
        *p++ = d[1];
    }
    d = &gFullWidthDigits[ones * 2];
    *p++ = d[0];
    *p++ = d[1];
    *p = 0;
    Font_SetSpacing(0x16, 0x16);
    Font_SetColorIndex(5);
    x = (0x280 - Str_Length(buf) * 0xB) / 2.0f;
    switch (mode) {
    case 0:
        Font_SetPos(x, 0x160);
        Font_SetSize(478.57144f);
        Font_Printf(buf);
        Font_SetSpacing(0x12, 0x12);
        Font_SetColorIndex(0);
        q = D_0081B4F4;
        x = (0x280 - Str_Length(*q) * 9) / 2.0f;
        Font_SetPos(x, 0x132);
        Font_Printf(*q);
        Font_DrawPage(3);
        Font_SetSize(621.4286f);
        break;
    case 1:
        Font_SetPos(x, 0x14C);
        Font_SetSize(764.28577f);
        Font_Printf(buf);
        Font_SetSpacing(0x12, 0x12);
        Font_SetColorIndex(0);
        q = D_0081B4F4;
        x = (0x280 - Str_Length(*q) * 9) / 2.0f;
        Font_SetPos(x, 0x11E);
        Font_Printf(*q);
        Font_DrawPage(5);
        Font_SetSize(621.4286f);
        break;
    case 2:
        Font_SetSize(478.57144f);
        Font_SetPos(x, 0x14C);
        Font_Printf(buf);
        Font_SetSpacing(0x12, 0x12);
        Font_SetColorIndex(0);
        q = D_0081B4F4;
        x = (0x280 - Str_Length(*q) * 9) / 2.0f;
        Font_SetPos(x, 0x11E);
        Font_Printf(*q);
        Font_DrawPage(3);
        Font_SetSize(621.4286f);
        break;
    }
}

void LobbyCard_ResetNetSave(void) {
    gNetMode.saveStep = 0;
    gNetMode.saveNext = 0;
}

/* unk_7D21E0's .rodata (0x847260-0x847280) */
#pragma rodata on
u32 gFullWidthDigits_0[8] = {
#include "lobby/00847260.inc"
};
#pragma rodata off
