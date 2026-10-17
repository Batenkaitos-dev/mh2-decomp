/*
 * The memory card menus (NetMenu, net_menu.h; gCardMenu): one state machine per mode
 * (enum NetMenuMode, card.h), run by NetMenu_Update: the boot load, the settings' save, a new
 * character's save, the load, the town's and the lobby's saves, the in-game save, and the
 * NET file's login id. Each state (card.h's CARD_*_ enums) draws a message (enum NetMenuMsg)
 * and moves on by the card task's result (unk_3DDBA0.c: CardTask_*, SaveMenu_*). Also here:
 * the patch loader (PATCH_DATA, PATCH_HEADER, PATCH_RECS), the save files' encryption, and the
 * character slots' windows. The select overlay and the lobby open the menu (NetMenu_Open).
 *
 * NetMenu_SetState is static, as in the original: MWCC then knows it leaves a0/a1 alone, and
 * its callers use them after the call (CardSys_Start stores the state number from a1;
 * NetMenu_StartCardRead(menu) needs no reload of menu). Its callers still in assembly link
 * against it because tools/mwcc_cc.py makes the symbol global in the object.
 */
#include "common.h"
#include "save.h"
#include "card.h"
#include "chredit.h"
#include "net_menu.h"
#include "window.h"
#include "pad.h"
#include "sprite.h"
#include "ram_areas.h"
#include "scene.h"
#include "syssave.h"
#include "syssettings.h"
#include "qsync.h"
#include "sub_main/unk_3D54B0.h"   /* the texts this file uses (tools/content_gen.py) */
#include "render_state.h"
#include "sound.h"

extern f32 gLobbyDayFrac;
extern SaveData *gCardSlots;

s16 NetMenu_DrawMessage(s32 x, s16 y, s32 id);
void Patch_BeginLoad(u8 *a, u8 *b, u8 *c);
void *memset(void *dst, s32 c, u32 n);
s32 Card_IsChanged(s32 slot);
s32 Card_PortState(s32 slot);
void CardTask_StartRead(s32 slot, u8 *buf);
void CardTask_SelectFile(s32 a, s32 b);
void CardState_SetFlags(s32 a);
s32 SaveBuf_Offset(s32 a, s32 b, s32 c);
struct SaveMenu;
void SaveMenu_CopyCharacter(struct SaveMenu *m, s32 data, s32 save, s32 slot);
void FlushCache(s32 op);
void SaveFile_Decrypt(NetMenu *menu, u16 *p);

/* The patch loader state (Patch_BeginLoad, Patch_LoadStep). */
extern s8 gPatchStep;
extern s8 gPatchXferA;
extern s8 gPatchXferB;
extern s8 gPatchResult;
extern u8 *gPatchHdr;
extern u8 *gPatchSrc;
extern u8 *gPatchDst;
extern u8 gGmSubLoaded;
extern s32 gLoadBuf;
extern s8 D_006ADAAC;
extern SaveData *gSaveBuf;
extern u8 gLobbySaveBuf[];

void PatchExecCS(u8 mode, u8 sub);
s32 Overlay_IsLoaded(s32 a);
void *memcpy(void *dst, const void *src, u32 n);
void Patch_ExecAll(void);
void SaveFile_Encrypt(u8 *buf);
void Patch_CopyFile(s32 mode, s32 buf);
void SaveMenu_CopySystem(struct SaveMenu *m, u8 *data, s32 save);
void CardTask_Begin(s32 a);
s32 LoadCompressedFile(u8 *dst, s32 id);
s32 CardOp_SetFiles(s32 *data);
s32 CardTask_Result(void);
void CardTask_StartCheck(void);
void CardNew_ChooseCharSlot(NetMenu *menu);
void CardGame_AskQuit(NetMenu *menu);
void CardSave_PrepareSave(NetMenu *menu);
void CardTask_StartReadClear(s32 slot, u8 *buf, s32 size);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void CardMenu_InitBuffers(NetMenu *menu);
extern u8 gOldGameKind;
s32 SaveMenu_LoadSystem(struct SaveMenu *m, s32 mode);
s32 SaveMenu_CheckCharacter(struct SaveMenu *m);
extern s32 gCardFileKind;
u16 Rand_Next(s32 a);
s32 strncmp(const char *a, const char *b, u32 n);
void strncpy(char *dst, const char *src, u32 n);
void printf(const char *s);
extern char D_004B0240[];
extern char gVersionString[];

/* A Window (window.h) as CardMenu_DrawSlots and CardMenu_DrawSlotFrames hold it on the stack:
 * their frames hold 0x20 bytes (as a 0x10 Window, sp moves by 16). */
typedef struct NetWin {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    u8 charW;                   /* 0x04 */
    u8 charH;                   /* 0x05 */
    u8 lineChars;               /* 0x06 */
    u8 lines;                   /* 0x07 */
    s16 color;                  /* 0x08: text colour */
    s16 flags;                  /* 0x0A: frame style */
    u32 frameColor;             /* 0x0C */
    u8 pad10[0x10];
} NetWin;

struct Window;
void Window_DrawTextAlpha(struct Window *w, u8 *text, u8 alpha);
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);
extern NetMenu gCardMenu;
extern char **D_00468394[];
extern SaveData *D_006AF028;
extern char *gGenderLabels[];
extern char D_004B0298[];
extern char D_004B02A0[];
extern char D_004B02B8[];


/* Its copy in the card buffer, at +0x50. */
typedef struct SysBody {
    u8 options[0x10];           /* 0x00: SysSave.options */
    u8 pad10[0x20 - 0x10];
    s32 bonusUnlocked;          /* 0x20: SysSave.bonusUnlocked (ORed in on a load) */
    u8 lastSlot;                /* 0x24: SysSave.lastSlot */
    u8 charSaved;               /* 0x25: SysSave.charSaved */
    u8 unk45E;                  /* 0x26-0x29: SysSave.unk45E-unk461 (ORed in on a load) */
    u8 unk45F;
    u8 unk460;
    u8 unk461;
    u8 pad2A[0x30 - 0x2A];
    s32 eventFlags[2][2];       /* 0x30: SysSave.eventFlags (ORed in on a load) */
} SysBody;

void SysSettings_Reset(void);
void NetMenu_Draw(void);
void *Pack_GetPart(u8 *buf, s32 a);
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
extern s32 D_006AF044;
extern s32 gCardMenuTex;
extern s32 gSystemTexFiles;
extern u8 gNetMenuDrawHooks[2][0x20];
extern void (*D_006A3A24)(void);
void LoadOverlay(s32 id);
s32 NetXfer_Step2(s8 *st, s16 *b, u8 *dst, s32 a, s32 max);
char *strcpy(char *dst, const char *src);
char *strchr(const char *s, s32 c);
char *strcat(char *dst, const char *src);
s32 sprintf(char *buf, const char *fmt, ...);
s32 strlen(const char *s);
void Sjis_Widen(char *src, char *dst);
s32 Text_DisplayLength(char *s);
extern u8 **gNetMenuMsgLines[];
extern char **gNetMenuMsgLines40[];
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
extern char *gNetMenuYesNoLabels[];
extern char **D_004682AC[];
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Text_SetColor(s16 color);
void Text_Printf(const char *fmt, ...);

/* unk_3D54B0's .rodata (0x4AEB48-0x4B0238) */
#pragma rodata on
char D_004AEB48[0x8] = {
#include "sub_main/004AEB48.inc"
};
char D_004AEB50[0x10] = {
#include "sub_main/004AEB50.inc"
};
char D_004AEB60[0x20] = {
#include "sub_main/004AEB60.inc"
};
char D_004AEB80[0x18] = {
#include "sub_main/004AEB80.inc"
};
#pragma explicit_zero_data on
char D_004AEB98[8] = {
#include "sub_main/004AEB98.inc"
};
#pragma explicit_zero_data reset
char D_004AEBA0[0x30] = {
#include "sub_main/004AEBA0.inc"
};
char D_004AEBD0[0x20] = {
#include "sub_main/004AEBD0.inc"
};
char D_004AEBF0[0x20] = {
#include "sub_main/004AEBF0.inc"
};
char D_004AEC10[0x20] = {
#include "sub_main/004AEC10.inc"
};
char D_004AEC30[0x30] = {
#include "sub_main/004AEC30.inc"
};
char D_004AEC60[0x30] = {
#include "sub_main/004AEC60.inc"
};
char D_004AEC90[0x30] = {
#include "sub_main/004AEC90.inc"
};
char D_004AECC0[0x20] = {
#include "sub_main/004AECC0.inc"
};
char D_004AECE0[0x30] = {
#include "sub_main/004AECE0.inc"
};
char D_004AED10[0x30] = {
#include "sub_main/004AED10.inc"
};
char D_004AED40[0x20] = {
#include "sub_main/004AED40.inc"
};
char D_004AED60[0x30] = {
#include "sub_main/004AED60.inc"
};
char D_004AED90[0x20] = {
#include "sub_main/004AED90.inc"
};
char D_004AEDB0[0x20] = {
#include "sub_main/004AEDB0.inc"
};
char D_004AEDD0[0x20] = {
#include "sub_main/004AEDD0.inc"
};
char D_004AEDF0[0x20] = {
#include "sub_main/004AEDF0.inc"
};
char D_004AEE10[0x20] = {
#include "sub_main/004AEE10.inc"
};
char D_004AEE30[0x20] = {
#include "sub_main/004AEE30.inc"
};
char D_004AEE50[0x20] = {
#include "sub_main/004AEE50.inc"
};
char D_004AEE70[0x30] = {
#include "sub_main/004AEE70.inc"
};
char D_004AEEA0[0x20] = {
#include "sub_main/004AEEA0.inc"
};
char D_004AEEC0[0x20] = {
#include "sub_main/004AEEC0.inc"
};
char D_004AEEE0[0x30] = {
#include "sub_main/004AEEE0.inc"
};
char D_004AEF10[0x30] = {
#include "sub_main/004AEF10.inc"
};
char D_004AEF40[0x30] = {
#include "sub_main/004AEF40.inc"
};
char D_004AEF70[0x20] = {
#include "sub_main/004AEF70.inc"
};
char D_004AEF90[0x20] = {
#include "sub_main/004AEF90.inc"
};
char D_004AEFB0[0x30] = {
#include "sub_main/004AEFB0.inc"
};
char D_004AEFE0[0x30] = {
#include "sub_main/004AEFE0.inc"
};
char D_004AF010[0x30] = {
#include "sub_main/004AF010.inc"
};
char D_004AF040[0x20] = {
#include "sub_main/004AF040.inc"
};
char D_004AF060[0x30] = {
#include "sub_main/004AF060.inc"
};
char D_004AF090[0x20] = {
#include "sub_main/004AF090.inc"
};
char D_004AF0B0[0x20] = {
#include "sub_main/004AF0B0.inc"
};
char D_004AF0D0[0x30] = {
#include "sub_main/004AF0D0.inc"
};
char D_004AF100[0x30] = {
#include "sub_main/004AF100.inc"
};
char D_004AF130[0x30] = {
#include "sub_main/004AF130.inc"
};
char D_004AF160[0x20] = {
#include "sub_main/004AF160.inc"
};
char D_004AF180[0x20] = {
#include "sub_main/004AF180.inc"
};
char D_004AF1A0[0x20] = {
#include "sub_main/004AF1A0.inc"
};
char D_004AF1C0[0x20] = {
#include "sub_main/004AF1C0.inc"
};
char D_004AF1E0[0x20] = {
#include "sub_main/004AF1E0.inc"
};
char D_004AF200[0x30] = {
#include "sub_main/004AF200.inc"
};
char D_004AF230[0x30] = {
#include "sub_main/004AF230.inc"
};
char D_004AF260[0x20] = {
#include "sub_main/004AF260.inc"
};
char D_004AF280[0x30] = {
#include "sub_main/004AF280.inc"
};
char D_004AF2B0[0x30] = {
#include "sub_main/004AF2B0.inc"
};
char D_004AF2E0[0x30] = {
#include "sub_main/004AF2E0.inc"
};
char D_004AF310[0x30] = {
#include "sub_main/004AF310.inc"
};
char D_004AF340[0x20] = {
#include "sub_main/004AF340.inc"
};
char D_004AF360[0x30] = {
#include "sub_main/004AF360.inc"
};
char D_004AF390[0x30] = {
#include "sub_main/004AF390.inc"
};
char D_004AF3C0[0x30] = {
#include "sub_main/004AF3C0.inc"
};
char D_004AF3F0[0x30] = {
#include "sub_main/004AF3F0.inc"
};
char D_004AF420[0x30] = {
#include "sub_main/004AF420.inc"
};
char D_004AF450[0x20] = {
#include "sub_main/004AF450.inc"
};
char D_004AF470[0x20] = {
#include "sub_main/004AF470.inc"
};
char D_004AF490[0x30] = {
#include "sub_main/004AF490.inc"
};
char D_004AF4C0[0x30] = {
#include "sub_main/004AF4C0.inc"
};
char D_004AF4F0[0x20] = {
#include "sub_main/004AF4F0.inc"
};
char D_004AF510[0x20] = {
#include "sub_main/004AF510.inc"
};
char D_004AF530[0x30] = {
#include "sub_main/004AF530.inc"
};
char D_004AF560[0x20] = {
#include "sub_main/004AF560.inc"
};
char D_004AF580[0x30] = {
#include "sub_main/004AF580.inc"
};
char D_004AF5B0[0x40] = {
#include "sub_main/004AF5B0.inc"
};
char D_004AF5F0[0x30] = {
#include "sub_main/004AF5F0.inc"
};
char D_004AF620[0x20] = {
#include "sub_main/004AF620.inc"
};
char D_004AF640[0x30] = {
#include "sub_main/004AF640.inc"
};
char D_004AF670[0x20] = {
#include "sub_main/004AF670.inc"
};
char D_004AF690[0x20] = {
#include "sub_main/004AF690.inc"
};
char D_004AF6B0[0x40] = {
#include "sub_main/004AF6B0.inc"
};
char D_004AF6F0[0x30] = {
#include "sub_main/004AF6F0.inc"
};
char D_004AF720[0x20] = {
#include "sub_main/004AF720.inc"
};
char D_004AF740[0x30] = {
#include "sub_main/004AF740.inc"
};
char D_004AF770[0x40] = {
#include "sub_main/004AF770.inc"
};
char D_004AF7B0[0x30] = {
#include "sub_main/004AF7B0.inc"
};
char D_004AF7E0[0x30] = {
#include "sub_main/004AF7E0.inc"
};
char D_004AF810[0x40] = {
#include "sub_main/004AF810.inc"
};
char D_004AF850[0x40] = {
#include "sub_main/004AF850.inc"
};
char D_004AF890[0x40] = {
#include "sub_main/004AF890.inc"
};
char D_004AF8D0[0x40] = {
#include "sub_main/004AF8D0.inc"
};
char D_004AF910[0x40] = {
#include "sub_main/004AF910.inc"
};
char D_004AF950[0x20] = {
#include "sub_main/004AF950.inc"
};
char D_004AF970[0x40] = {
#include "sub_main/004AF970.inc"
};
char D_004AF9B0[0x30] = {
#include "sub_main/004AF9B0.inc"
};
char D_004AF9E0[0x30] = {
#include "sub_main/004AF9E0.inc"
};
char D_004AFA10[0x30] = {
#include "sub_main/004AFA10.inc"
};
char D_004AFA40[0x30] = {
#include "sub_main/004AFA40.inc"
};
char D_004AFA70[0x30] = {
#include "sub_main/004AFA70.inc"
};
char D_004AFAA0[0x40] = {
#include "sub_main/004AFAA0.inc"
};
char D_004AFAE0[0x20] = {
#include "sub_main/004AFAE0.inc"
};
char D_004AFB00[0x40] = {
#include "sub_main/004AFB00.inc"
};
char D_004AFB40[0x30] = {
#include "sub_main/004AFB40.inc"
};
char D_004AFB70[0x20] = {
#include "sub_main/004AFB70.inc"
};
char D_004AFB90[0x30] = {
#include "sub_main/004AFB90.inc"
};
char D_004AFBC0[0x30] = {
#include "sub_main/004AFBC0.inc"
};
char D_004AFBF0[0x20] = {
#include "sub_main/004AFBF0.inc"
};
char D_004AFC10[0x30] = {
#include "sub_main/004AFC10.inc"
};
char D_004AFC40[0x30] = {
#include "sub_main/004AFC40.inc"
};
char D_004AFC70[0x20] = {
#include "sub_main/004AFC70.inc"
};
char D_004AFC90[0x30] = {
#include "sub_main/004AFC90.inc"
};
char D_004AFCC0[0x30] = {
#include "sub_main/004AFCC0.inc"
};
char D_004AFCF0[0x30] = {
#include "sub_main/004AFCF0.inc"
};
char D_004AFD20[0x40] = {
#include "sub_main/004AFD20.inc"
};
char D_004AFD60[0x20] = {
#include "sub_main/004AFD60.inc"
};
char D_004AFD80[0x30] = {
#include "sub_main/004AFD80.inc"
};
char D_004AFDB0[0x20] = {
#include "sub_main/004AFDB0.inc"
};
char D_004AFDD0[0x20] = {
#include "sub_main/004AFDD0.inc"
};
char D_004AFDF0[0x20] = {
#include "sub_main/004AFDF0.inc"
};
char D_004AFE10[0x20] = {
#include "sub_main/004AFE10.inc"
};
char D_004AFE30[0x30] = {
#include "sub_main/004AFE30.inc"
};
char D_004AFE60[0x30] = {
#include "sub_main/004AFE60.inc"
};
char D_004AFE90[0x20] = {
#include "sub_main/004AFE90.inc"
};
char D_004AFEB0[0x18] = {
#include "sub_main/004AFEB0.inc"
};
char D_004AFEC8[0x8] = {
#include "sub_main/004AFEC8.inc"
};
char D_004AFED0[0x10] = {
#include "sub_main/004AFED0.inc"
};
char D_004AFEE0[0x30] = {
#include "sub_main/004AFEE0.inc"
};
char D_004AFF10[0x30] = {
#include "sub_main/004AFF10.inc"
};
char D_004AFF40[0x20] = {
#include "sub_main/004AFF40.inc"
};
char D_004AFF60[0x20] = {
#include "sub_main/004AFF60.inc"
};
char D_004AFF80[0x20] = {
#include "sub_main/004AFF80.inc"
};
char D_004AFFA0[0x30] = {
#include "sub_main/004AFFA0.inc"
};
char D_004AFFD0[0x40] = {
#include "sub_main/004AFFD0.inc"
};
char D_004B0010[0x20] = {
#include "sub_main/004B0010.inc"
};
char D_004B0030[32] = {
#include "sub_main/004B0030.inc"
};
char D_004B0050[0x20] = {
#include "sub_main/004B0050.inc"
};
char D_004B0070[0x30] = {
#include "sub_main/004B0070.inc"
};
char D_004B00A0[0x20] = {
#include "sub_main/004B00A0.inc"
};
char D_004B00C0[0x20] = {
#include "sub_main/004B00C0.inc"
};
char D_004B00E0[32] = {
#include "sub_main/004B00E0.inc"
};
#pragma explicit_zero_data on
char D_004B0100[0x10] = {
#include "sub_main/004B0100.inc"
};
#pragma explicit_zero_data reset
char D_004B0110[0x20] = {
#include "sub_main/004B0110.inc"
};
char D_004B0130[0x20] = {
#include "sub_main/004B0130.inc"
};
char D_004B0150[0x20] = {
#include "sub_main/004B0150.inc"
};
char D_004B0170[0x30] = {
#include "sub_main/004B0170.inc"
};
char D_004B01A0[0x30] = {
#include "sub_main/004B01A0.inc"
};
char D_004B01D0[0x30] = {
#include "sub_main/004B01D0.inc"
};
char D_004B0200[0x10] = {
#include "sub_main/004B0200.inc"
};
char D_004B0210[0x28] = {
#include "sub_main/004B0210.inc"
};
#pragma rodata off

/* Does nothing. Loading_SetMessage calls it for its mode 0. */
void func_003D54B0(s32 unused) /* unused: the caller passes 0 */
{
}

/* Returns 0. The lobby's Town_ReloadLocalPlayer calls it (with an argument it doesn't read); 1
 * would make that call func_003D54D0(1) instead of Loading_Reset. */
s32 func_003D54C0(void)
{
    return 0;
}

/* Does nothing. The lobby's Town_ReloadLocalPlayer calls it with 1 and with 0. */
void func_003D54D0(void)
{
}

/* Sets the lobby's time of day, gLobbyDayFrac, to noon (0.5). select_boot.c calls it. */
void LobbyDay_SetNoon(void)
{
    gLobbyDayFrac = 0.5f;
}

/* Clears PATCH_DATA and PATCH_RECS (0x10000 bytes each) and PATCH_HEADER (0x40 bytes), then writes
 * gVersionString (10 bytes) and a 4-digit number at +0xA into the header. */
void Patch_Init(void)
{
    memset((void *)PATCH_DATA, 0, 0x10000);
    memset((void *)PATCH_RECS, 0, 0x10000);
    memset((void *)PATCH_HEADER, 0, 0x40);
    strncpy((char *)PATCH_HEADER, gVersionString, 10);
    strncpy((char *)(PATCH_HEADER + 0xA), "1000", 4);
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3D54B0", D_004B0240);

/* Applies the patch records after the header at 0xAB6780 (magic, count) whose id is (mode << 4) | sub, then flushes the cache. */
void PatchExecCS(u8 mode, u8 sub)
{
    u8 *rec;
    u8 *hdr;
    s32 count;
    u32 id = (mode << 4) | sub;
    u32 rid;
    u8 *dst;
    s32 i;
    s32 len;
    u8 *src;
    s32 j;

    if (*(s32 *)(PATCH_HEADER + 0x14) != 0) {
        count = *(s32 *)(PATCH_RECS + 0x8);
        rec = PATCH_RECS;
        hdr = rec;
        rec += 12;
        if (strncmp((char *)hdr, D_004B0240, 8) == 0) {
            if (count != 0) {
                if (strncmp(gVersionString, (char *)PATCH_HEADER, 10) < 0) {
                    strncpy(gVersionString, (char *)PATCH_HEADER, 10);
                } else {
                    printf("PatchExecCS: Magic-No is different.\n");
                }
            }
            for (i = 0; i < count; i++) {
                rid = *(u32 *)rec;
                dst = *(u8 **)(rec + 4);
                len = *(s32 *)(rec += 8);
                rec += 4;
                if (id == rid) {
                    if (len & 0x80000000) {
                        src = *(u8 **)rec;
                        len &= 0x7FFFFFFF;
                        rec += 4;
                        for (j = 0; j < len; j++) {
                            *dst++ = *src++;
                        }
                    } else {
                        for (j = 0; j < len; j++) {
                            *dst++ = *rec++;
                        }
                    }
                } else if (len & 0x80000000) {
                    rec += 4;
                } else {
                    rec += len;
                }
                rec = (u8 *)(((s32)rec + 3) / 4 * 4);
            }
            FlushCache(0);
        }
    }
}

/* Sets up Patch_LoadStep: resets gPatchStep, the transfer state (gPatchXferA, gPatchXferB) and
 * gPatchResult, keeps the patch header, its data and where the records go, and clears the records'
 * buffer (0x10000 bytes). */
void Patch_BeginLoad(u8 *hdr, u8 *src, u8 *dst)
{
    gPatchStep = 0;
    gPatchXferA = 0;
    gPatchXferB = 0;
    gPatchResult = 0;
    gPatchHdr = hdr;
    gPatchSrc = src;
    gPatchDst = dst;
    memset(dst, 0, 0x10000);
}

/* Applies the patch records of each loaded part (PatchExecCS by mode and sub): (0, 0) always, (1,
 * 1) sub_main, (2, 1)-(2, 3) the overlays 2-4 when loaded, (3, 2) gm_sub and (6, 1) plsel. */
void Patch_ExecAll(void)
{
    PatchExecCS(0, 0);
    if (gSysSettings.subMainLoaded == 1) {
        PatchExecCS(1, 1);
    }
    if (Overlay_IsLoaded(2) == 1) {
        PatchExecCS(2, 1);
    }
    if (Overlay_IsLoaded(3) == 1) {
        PatchExecCS(2, 2);
    }
    if (Overlay_IsLoaded(4) == 1) {
        PatchExecCS(2, 3);
    }
    if (gGmSubLoaded == 1) {
        PatchExecCS(3, 2);
    }
    if (gSysSettings.plselLoaded == 1) {
        PatchExecCS(6, 1);
    }
}

/* Runs one step of the load Patch_BeginLoad set up: copies the data, loads overlay 8, then runs
 * NetXfer_Step2 on the records. Returns gPatchResult: 0 running, 2 loaded, -1 failed, 1 nothing to
 * load (no records size, a version not newer than gVersionString, or records without the D_004B0240
 * magic). */
s32 Patch_LoadStep(void)
{
    u8 *hdr;
    u8 *dst;
    u8 *src;
    s32 xferResult;
    s32 loadResult;

    hdr = gPatchHdr;
    src = gPatchSrc;
    dst = gPatchDst;
    loadResult = gPatchResult;
    if (loadResult != 0) {
        return (s8)loadResult;
    }
    switch (gPatchStep) {
    case 0:
        if (*(s32 *)(hdr + 0x14) == 0) {
            gPatchResult = 1;
            break;
        }
        if (strncmp(gVersionString, (char *)hdr, 10) >= 0) {
            gPatchResult = 1;
            break;
        }
        memcpy(dst, src, 0x10000);
        gPatchXferA = 0;
        gPatchXferB = 0;
        gPatchStep++;
    case 1:
        gPatchStep++;
        LoadOverlay(8);
        break;
    case 2:
        gPatchStep++;
    case 3:
        xferResult = NetXfer_Step2(&gPatchXferA, (s16 *)&gPatchXferB, dst, *(s32 *)(hdr + 0x14), 0x10000);
        if (xferResult > 0) {
            if (strncmp((char *)dst, D_004B0240, 8) != 0) {
                gPatchResult = 1;
            } else {
                gPatchStep = 0;
                gPatchXferA = 0;
                gPatchResult = 2;
                gPatchXferB = 0;
            }
        } else if (xferResult < 0) {
            gPatchStep = 0;
            gPatchResult = -1;
            gPatchXferA = 0;
            gPatchXferB = 0;
        }
        break;
    }
    return gPatchResult;
}

/* Does nothing. NetMenu_Open calls it for the modes of CardMenu_RunGameSaveNet. */
void func_003D5B70(void)
{
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3D54B0", D_004B0278);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3D54B0", D_004B0280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3D54B0", D_004B0288);

extern s16 gCardSlotPos[];
extern s16 D_00468848[];
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
s32 Card_SlotReady(s32 slot);

/*
 * Draws the three save slots of the card menu (sub-state m->sub is the selected one): with
 * flag 0 in framed windows (a glow bar on the selected one in modes 0 and 2, or with 2 or more
 * slots n), else at the positions of gCardSlotPos in the selection's colours; an empty slot shows
 * the empty-slot text (or a card message, D_00468394[0]), a used one the name, play time, sex and money.
 */
void CardMenu_DrawSlots(NetMenu *menu, s32 mode, u8 selectMode, s32 unused, s32 flag)
{
    NetWin win;
    s16 i;
    s16 x;
    s16 y;
    s16 color;
    SaveData *save;
    s16 *pos;
    u8 gender;
    u8 used;
    s32 money;
    s16 size;
    u32 col;
    u32 time;
    char **tbl;
    SysSave *sys;

    sys = &gSysSave;

    Font_SetSpacing(0x14, 0x14);
    win.x = 0x32;
    win.y = 0x58;
    win.charW = 0x14;
    win.charH = 0x14;
    win.lineChars = 0xC;
    win.lines = 3;
    win.color = 5;
    win.frameColor = 0xC0402010;
    x = 0x3C;
    y = 0x5A;
    if (mode == 2) {
        win.flags = 2;
    } else {
        win.flags = 4;
    }
    save = *(SaveData **)((u8 *)sys + 0x3C60);
    for (i = 0; i < 3; i++) {
        if (i == menu->sub) {
            if (flag == 0) {
                color = 5;
            } else {
                pos = &gCardSlotPos[i * 4];
                size = 0x18;
                col = 0x807F7F7F;
            }
            if (mode == 0 || mode == 2 || selectMode >= 2) {
                if (flag == 0) {
                    Menu_DrawGlowBar(win.x + 0x73, win.y - 3, 0x118, 0x46, 0xFF20C0C0);
                }
            }
        } else if (flag == 0) {
            color = 0;
        } else {
            size = 0x12;
            col = 0x80101010;
            if (i == 1 && menu->sub == 2) {
                pos = D_00468848;
            } else {
                pos = &gCardSlotPos[i * 4 + 2];
            }
        }
        used = save->used;
        gender = save->gender;
        time = save->playTime;
        money = save->money;
        if (flag == 0) {
            Window_DrawTextAlpha((Window *)&win, 0, 0x80);
            if (used == 0) {
                Font_SetSpacing(0x18, 0x18);
                Text_PrintfAt(x + 0x3E, y + 0x12, color, TXT(0x004B0290) /* the text of an empty save slot */, i + 1);
            } else {
                Font_SetSpacing(0x18, 0x18);
                Text_PrintfAt(x, y, color, D_004B0298, i + 1, save->name);
                Font_SetSpacing(0x12, 0x12);
                Text_PrintfAt(x, y + 0x18, color, D_004B02A0, time / 3600, time % 3600 / 60, gGenderLabels[gender + 2]);
                Text_PrintfAt(x, y + 0x2A, color, D_004B02B8, money);
            }
        } else {
            tbl = D_00468394[0];
            x = pos[0];
            y = pos[1];
            pos += 2;
            Font_SetColor(col);
            if (used == 0) {
                Font_SetSpacing((u8)size, (u8)size);
                if (Card_SlotReady(i) == 0) {
                    if (size == 0x18) {
                        Font_SetPos(x + 0x18, y + 0x12);
                    } else {
                        Font_SetPos(x + 0x18, y + 0xC);
                    }
                    Font_SetColor(0x80301010);
                    Text_PrintfColored(tbl[1]);
                    Font_SetColor(col);
                } else {
                    Font_SetPos(x + 0x18, y + 0xA);
                    Text_PrintfColored(tbl[0]);
                }
            } else {
                Font_SetSpacing((u8)size, (u8)size);
                Font_SetPos(x, y);
                Text_PrintfColored(D_004B0298, i + 1, save->name);
                Font_SetSpacing((u8)(size - 6), (u8)(size - 6));
                Font_SetPos(x, y + size);
                Text_PrintfColored(D_004B02A0, time / 3600, time % 3600 / 60, gGenderLabels[gender + 2]);
                Font_SetPos(x, y + size + (size - 6));
                Text_PrintfColored(D_004B02B8, money);
            }
        }
        save = (SaveData *)((u8 *)save + 0x37F8);
        y += 0x58;
        win.y += 0x58;
    }
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3D54B0", D_004B0298);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3D54B0", D_004B02A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3D54B0", D_004B02B8);


/* 1 when character slot i of gCardSlots is used, else 0. select_title.c calls it. */
s32 SaveMenu_SlotUsed(s32 i)
{
    SaveData *save = gCardSlots;

    save += i;
    return save->used != 0;
}

/* Draws the three character slots' windows (window flags 2 in slotMode 2, else 4); slot sel gets
 * the glow bar when slotMode is 0 or 2 or selectMode is 2 or more. edit is unused (NetMenu_Draw
 * passes gChrEdit). */
void CardMenu_DrawSlotFrames(ChrEdit *edit, s32 mode, u8 selectMode, s8 sel)
{
    NetWin win;
    s16 i;

    Font_SetSpacing(0x14, 0x14);
    win.x = 0x32;
    win.y = 0x58;
    win.charW = 0x14;
    win.charH = 0x14;
    win.lineChars = 0xC;
    win.lines = 3;
    win.color = 5;
    win.frameColor = 0xC0402010;
    if (mode == 2) {
        win.flags = 2;
    } else {
        win.flags = 4;
    }
    for (i = 0; i < 3; i++) {
        if (i == sel && (mode == 0 || mode == 2 || selectMode >= 2)) {
            Menu_DrawGlowBar(win.x + 0x73, win.y - 3, 0x118, 0x46, 0xFF20C0C0);
        }
        Window_DrawTextAlpha((Window *)&win, 0, 0x80);
        win.y += 0x58;
    }
}

/* Draws the characters of gCardMenu.slots (D_006AF028) in the slot windows, sel in colour 5: the
 * bad slot's label when charRead is 0x80, the empty slot's label when the slot is unused, else the
 * number and name, the play time and sex, and the money. edit, slotMode, selectMode and unused are
 * not read. */
void CardMenu_DrawCharacters(ChrEdit *edit, s32 slotMode, s32 selectMode, s32 unused, s8 sel)
{
    s16 i;
    s16 y;
    char **tbl;
    NetMenu *menu;
    s16 color;
    SaveData *save;
    u32 time;
    s32 money;
    u8 gender;

    i = 0;
    y = 0x5A;
    tbl = D_00468394[0];
    menu = &gCardMenu;
    save = D_006AF028;
    do {
        color = (i == sel) ? 5 : 0;
        if (menu->charRead[i] == 0x80) {
            Font_SetSpacing(0x16, 0x16);
            Text_PrintfAt(0x7A, y + 0x12, 2, tbl[1], i);
        } else {
            gender = save->gender;
            time = save->playTime;
            money = save->money;
            if (save->used == 0) {
                Font_SetSpacing(0x18, 0x18);
                Text_PrintfAt(0x7A, y + 0x12, color, tbl[0], i + 1);
            } else {
                Font_SetSpacing(0x18, 0x18);
                Text_PrintfAt(0x3C, y, color, D_004B0298, i + 1, save->name);
                Font_SetSpacing(0x12, 0x12);
                Text_PrintfAt(0x3C, y + 0x18, color, D_004B02A0, time / 3600, time % 3600 / 60, gGenderLabels[gender + 2]);
                Text_PrintfAt(0x3C, y + 0x2A, color, D_004B02B8, money);
            }
        }
        y += 0x58;
        save++;
        i++;
    } while (i < 3);
}

/* Encrypts a save file in place (buf, of fileKind's size): 0x40 random bytes, then the header
 * words (the version, the key taken from the random bytes, the sum of the plain data) and the
 * data XORed with a key stream (the key steps as key * 0xB0 % 0xFF53). Kept for the match: the
 * data walker is the parameter reused after the random bytes, the sum is kept through a
 * pointer, and the version and the key are two locals. */
void SaveFile_Encrypt(u8 *buf)
{
    s32 i;
    u16 ver;
    s32 size;
    u8 *pos;
    u16 *header;
    u16 key;
    u16 *sum;

    header = (u16 *)buf;
    pos = buf;
    switch (gCardFileKind) {
    case 1:
        size = 0x550;
        ver = 0x100;
        break;
    case 2:
    case 4:
    case 8:
        size = 0x8050;
        ver = 1;
        break;
    case 0x10:
        size = 0x10080;
        ver = 1;
        break;
    default:
        return;
    }
    for (i = 0; i < 0x40; i++) {
        *pos = Rand_Next(0) & 0xFFFF;
        size--;
        pos++;
    }
    header[0] = ver;
    buf = pos;
    header[2] = 0;
    sum = &header[2];
    key = header[1];
    for (i = 0; i < size / 2; i++) {
        *sum += *(u16 *)buf;
        *(u16 *)buf ^= key;
        buf += 2;
        if (key == 0) {
            key = 1;
        }
        key = (key * 0xB0) % 0xFF53;
    }
}

/* Decrypts a save file read from the card and checks it: badVersion if its version word is wrong, badSum if
 * the sum of its decrypted words is. The walker is the parameter itself (a1), with the header read through a
 * byte-pointer copy made after the switch (as a u16 * copy, the walker start was folded onto the parameter). */
void SaveFile_Decrypt(NetMenu *menu, u16 *words)
{
    s32 size;
    u16 ver;
    s32 i;
    u16 sum;
    u8 *buf;
    u16 key;

    switch (menu->fileKind) {
    case 1:
        size = 0x550;
        ver = 0x100;
        break;
    case 2:
    case 4:
    case 8:
        size = 0x8050;
        ver = 1;
        break;
    case 0x10:
        size = 0x10080;
        ver = 1;
        break;
    default:
        menu->badVersion = 1;
        menu->badSum = 1;
        return;
    }
    buf = (u8 *)words;
    menu->badVersion = 0;
    size -= 0x40;
    sum = 0;
    menu->badVersion = ((u16 *)buf)[0] != ver;
    key = ((u16 *)buf)[1];
    words = (u16 *)(buf + 0x40);
    for (i = 0; i < size / 2; i++) {
        *words ^= key;
        sum += *words;
        words++;
        if (key == 0) {
            key = 1;
        }
        key = (key * 0xB0) % 0xFF53;
    }
    menu->badSum = ((u16 *)buf)[2] != sum;
}

/* 1 when the date stamp at readBuf + 0x40 (a Card_GetDate pair) equals pairs[0], the system file's;
 * 0 when not, or when fileKind is not a system or character file. The lobby's unk_7D2C80.c calls
 * it. */
s32 SaveMenu_SystemIdMatches(NetMenu *menu)
{
    s32 *stamp;
    u8 *buf = menu->readBuf;

    switch (menu->fileKind) {
    case 1:
        stamp = (s32 *)(buf + 0x40);
        break;
    case 2:
    case 4:
    case 8:
        stamp = (s32 *)(buf + 0x40);
        break;
    default:
        return 0;
    }
    if (menu->pairs[0][0] != stamp[0] || menu->pairs[0][1] != stamp[1]) {
        return 0;
    }
    return 1;
}

/* 1 when pairs[k] (k: the character slot + 1) equals the date stamp of the character file just read
 * (charId0, charId1), else 0. */
static s32 SaveMenu_CharIdMatches(NetMenu *menu, s32 k)
{
    if (menu->pairs[k][0] != menu->charId0 || menu->pairs[k][1] != menu->charId1) {
        return 0;
    }
    return 1;
}

/* Flushes the cache and decrypts readBuf as fileKind (SaveFile_Decrypt). 1 when its version or sum
 * is wrong, else 0. */
s32 SaveMenu_ReadBad(NetMenu *menu)
{
    FlushCache(0);
    SaveFile_Decrypt(menu, (u16 *)menu->readBuf);
    if (menu->badVersion != 0 || menu->badSum != 0) {
        return 1;
    }
    return 0;
}

/* Copies the system data between gSysSave and a system file's body (buf + 0x50). dir 0 loads: the
 * options, lastSlot and charSaved are copied, bonusUnlocked, unk45E-unk461 and eventFlags are ORed
 * in, then SysSettings_Reset. Any other dir saves them into the body. Returns 1. */
s32 SysSave_Exchange(s32 dir, u8 *buf)
{
    SysBody *body = (SysBody *)(buf + 0x50);
    SysSave *sys = &gSysSave;
    s32 i;

    if (dir == 0) {
        memcpy(sys, body, 0x10);
        sys->bonusUnlocked |= body->bonusUnlocked;
        sys->lastSlot = body->lastSlot;
        sys->charSaved = body->charSaved;
        sys->unk45E |= body->unk45E;
        sys->unk45F |= body->unk45F;
        sys->unk460 |= body->unk460;
        sys->unk461 |= body->unk461;
        i = 0;
        do {
            sys->eventFlags[i][0] |= body->eventFlags[i][0];
            sys->eventFlags[i][1] |= body->eventFlags[i][1];
            i++;
        } while (i < 2);
        SysSettings_Reset();
    } else {
        memcpy(body, sys, 0x10);
        body->bonusUnlocked = sys->bonusUnlocked;
        body->lastSlot = sys->lastSlot;
        body->charSaved = sys->charSaved;
        body->unk45E = sys->unk45E;
        body->unk45F = sys->unk45F;
        body->unk460 = sys->unk460;
        body->unk461 = sys->unk461;
        i = 0;
        do {
            body->eventFlags[i][0] = sys->eventFlags[i][0];
            body->eventFlags[i][1] = sys->eventFlags[i][1];
            i++;
        } while (i < 2);
    }
    return 1;
}

/* ORs a system file's bonusUnlocked, unk45E-unk461 and eventFlags (its body at buf + 0x50) into
 * gSysSave, leaving the rest. */
void SysSave_MergeFlags(u8 *buf)
{
    SysBody *body = (SysBody *)(buf + 0x50);
    s32 i;

    gSysSave.bonusUnlocked |= body->bonusUnlocked;
    gSysSave.unk45E |= body->unk45E;
    gSysSave.unk45F |= body->unk45F;
    gSysSave.unk460 |= body->unk460;
    gSysSave.unk461 |= body->unk461;
    i = 0;
    do {
        gSysSave.eventFlags[i][0] |= body->eventFlags[i][0];
        gSysSave.eventFlags[i][1] |= body->eventFlags[i][1];
        i++;
    } while (i < 2);
}

/* When the patch file in buf has records (the size at buf + 0x54), copies its data (+0x80), header
 * (+0x40) and records (+0x10080) into PATCH_DATA, PATCH_HEADER and PATCH_RECS, and applies them
 * (Patch_ExecAll). */
void Patch_InstallFile(u8 *buf)
{
    u8 *hdr = buf + 0x40;

    if (*(s32 *)(buf + 0x54) != 0) {
        memcpy((void *)PATCH_DATA, hdr + 0x40, 0x10000);
        memcpy((void *)PATCH_HEADER, hdr, 0x40);
        memcpy((void *)PATCH_RECS, (void *)((s32)buf + 0x10080), 0x10000); /* integer sum: as a pointer sum the code changes */
        Patch_ExecAll();
    }
}

/* mode 0: copies the file at addr into PATCH_DATA and PATCH_HEADER, as Patch_CopyFile does.
 * Otherwise fills the file from gSysSettings.extraBuf (the data, then the header from extraBuf +
 * 0x20000) and encrypts it (SaveMenu_SaveExtra passes 1). */
void SaveExtra_CopyFile(s32 mode, s32 addr)
{
    u8 *hdr;
    u8 *buf = (u8 *)addr;
    u8 *src;

    hdr = buf + 0x40;
    if (mode == 0) {
        memcpy(PATCH_DATA, hdr + 0x40, 0x10000);
        memcpy(PATCH_HEADER, hdr, 0x40);
        return;
    }
    src = gSysSettings.extraBuf;
    memcpy(hdr + 0x40, src, 0x10000);
    memcpy(hdr, src + 0x20000, 0x40);
    SaveFile_Encrypt(buf);
}

/* mode 0: copies the patch file at addr (header +0x40, data +0x80) into PATCH_HEADER and
 * PATCH_DATA. Otherwise fills the file from them and encrypts it (SaveMenu_CopyAll passes 1). */
void Patch_CopyFile(s32 mode, s32 addr)
{
    u8 *hdr;
    u8 *buf = (u8 *)addr;

    hdr = buf + 0x40;
    if (mode == 0) {
        memcpy(PATCH_DATA, hdr + 0x40, 0x10000);
        memcpy(PATCH_HEADER, hdr, 0x40);
        return;
    }
    memcpy(hdr + 0x40, PATCH_DATA, 0x10000);
    memcpy(hdr, PATCH_HEADER, 0x40);
    SaveFile_Encrypt(buf);
}

/* 1 when the patch file in buf has a records size (header + 0x14, buf + 0x54), else 0. */
s32 Patch_FileHasPatch(u8 *buf)
{
    return *(s32 *)(buf + 0x54) != 0;
}

/* Starts Patch_LoadStep on the patch file in buf: the header at +0x40, the data at +0x80, the
 * records to +0x10080. */
void Patch_BeginFile(u8 *buf)
{
    u8 *hdr = buf + 0x40;

    Patch_BeginLoad(hdr, hdr + 0x40, (u8 *)((s32)buf + 0x10080)); /* integer sum, as in Patch_InstallFile */
}

/* Clears gSysSave.save, the copy of the loaded character. */
void Save_ClearCopy(void)
{
    memset(&gSysSave.save, 0, sizeof(SaveData));
}

/* Builds a whole card image at imageBuf: the system file, three character files (from gCardSlots,
 * cleared first, so the characters are empty) and the patch file, setting fileKind for each and
 * restoring it after. */
void SaveMenu_CopyAll(NetMenu *menu)
{
    s32 i;
    s32 saved = menu->fileKind;

    memset(gCardSlots, 0, sizeof(SaveData) * 3);
    menu->fileKind = 1;
    SaveMenu_CopySystem((struct SaveMenu *)menu, (u8 *)SaveBuf_Offset(menu->imageBuf, 0, 0), 1);
    for (i = 0; i < 3; i++) {
        menu->fileKind = 2 << i;
        SaveMenu_CopyCharacter((struct SaveMenu *)menu, SaveBuf_Offset(menu->imageBuf, 1, i), 1, i);
    }
    menu->fileKind = 0x10;
    Patch_CopyFile(1, SaveBuf_Offset(menu->imageBuf, 2, 0));
    menu->fileKind = saved;
}

/* Clears gCardSlots[charIndex]. The load functions call it on bad data. */
void SaveMenu_ClearCharacter(NetMenu *menu)
{
    SaveData *save = gCardSlots;

    save += menu->charIndex;
    memset(save, 0, sizeof(SaveData));
}

/* Stores gCardSlots[i] as character file i of imageBuf, stamped and encrypted. */
void SaveMenu_CopyCharacterSlot(NetMenu *menu, s32 i)
{
    SaveMenu_CopyCharacter((struct SaveMenu *)menu, SaveBuf_Offset(menu->imageBuf, 1, i), 1, i);
}

/* Sets up the card buffers for the save data: card operation 0, the save icons (file 0x1714)
 * and gLoadBuf split into the menu's buffers (readBuf at +0x40400); clears charRead and
 * D_006ADAAC; gCardSlots is gLobbySaveBuf when overlay 4 is loaded, else gSaveBuf. The callers
 * in this file pass the menu they already hold in a0 (the game's code sets no argument
 * register). */
void CardMenu_InitBuffers(NetMenu *menu)
{
    s32 buf;

    CardTask_Begin(0);
    buf = gLoadBuf;
    memset((void *)buf, 0, 0x6AFE8);
    menu->iconBuf = (u8 *)buf;
    menu->imageBuf = buf + 0x20000;
    menu->readBuf = (u8 *)(buf + 0x40400);
    menu->slots = (SaveData *)(buf + 0x60800);
    LoadCompressedFile(menu->iconBuf, 0x1714);
    menu->fileBlocks = CardOp_SetFiles((s32 *)menu->iconBuf);
    menu->charRead[0] = 0;
    menu->charRead[1] = 0;
    menu->charRead[2] = 0;
    gCardSlots = menu->slots;
    D_006ADAAC = 0;
}

/* CardMenu_InitBuffers without the icon file: card operation 0, gLoadBuf split into the menu's
 * buffers, charRead and D_006ADAAC cleared. gCardSlots is gLobbySaveBuf when overlay 4 is loaded,
 * else gSaveBuf. */
void CardMenu_InitLoadBuffers(NetMenu *menu)
{
    s32 buf;

    CardTask_Begin(0);
    buf = gLoadBuf;
    memset((void *)buf, 0, 0x6AFE8);
    menu->iconBuf = (u8 *)buf;
    menu->imageBuf = buf + 0x20000;
    menu->readBuf = (u8 *)(buf + 0x40400);
    menu->slots = (SaveData *)(buf + 0x60800);
    menu->charRead[0] = 0;
    menu->charRead[1] = 0;
    menu->charRead[2] = 0;
    if (Overlay_IsLoaded(4) == 1) {
        gCardSlots = (SaveData *)gLobbySaveBuf;
    } else {
        gCardSlots = gSaveBuf;
    }
    D_006ADAAC = 0;
}

/* Draws message id from y (NetMenu_DrawMessage) and returns the y of its last line. */
s16 NetMenu_DrawMessageY(s16 y, s32 id)
{
    return NetMenu_DrawMessage(0, y, id);
}

/* Draws the lines of message id (enum NetMenuMsg), 0x12 apart from y and centred on x 0x140 (x is
 * unused); ids from 0x40 on read on into gNetMenuMsgLines40. In NETMENU_MSG_OLD_DATA_FOUND and
 * NETMENU_MSG_ASK_DELETE_LOGIN_ID it fills in %t, %p, %i and %q (the old game's title, oldSlot + 1,
 * the login id at readBuf + 0x960 and the masked password at readBuf + 0x96B). Returns the y of the
 * last line. */
s16 NetMenu_DrawMessage(s32 x, s16 y, s32 id)
{
    char num[4];
    char wideNum[4];
    char line[100];
    char name[16];
    char wide[64];
    NetMenu *menu = &gCardMenu;
    u8 **lines;
    s32 hasArgs;
    char *pct;
    s32 len;
    s32 i;
    u8 *text;

    Font_SetSpacing(0x12, 0x12);
    Text_SetColor(0);
    lines = gNetMenuMsgLines[id];
    hasArgs = 0;
    if (id == 0x3F) {
        hasArgs = 1;
    }
    if (id == 0x48) {
        hasArgs = 1;
    }
    while ((text = *lines) != NULL) {
        if (*text != 0) {
            if (hasArgs == 1) {
                strcpy(line, (char *)text);
                pct = strchr(line, '%');
                if (pct != NULL) {
                    switch (pct[1]) {
                    case 't':
                        *pct = 0;
                        strcat(line, gNetMenuMsgLines40[0][gOldGameKind - 1U]);
                        strcat(line, strchr((char *)text, '%') + 2);
                        break;
                    case 'p':
                        sprintf(num, "%d", menu->oldSlot + 1);
                        Sjis_Widen(num, wideNum);
                        pct[0] = wideNum[0];
                        pct[1] = wideNum[1];
                        break;
                    case 'i':
                        *pct = 0;
                        sprintf(name, "%s", menu->readBuf + 0x960);
                        if ((len = strlen(name)) < 10) {
                            do {
                                strcat(name, " ");
                                len++;
                            } while (len < 10);
                        }
                        Sjis_Widen(name, wide);
                        strcat(line, wide);
                        strcat(line, strchr((char *)text, '%') + 2);
                        break;
                    case 'q':
                        *pct = 0;
                        len = strlen((char *)menu->readBuf + 0x96B);
                        if (len != 0) {
                            for (i = 0; i < len; i++) {
                                strcat(line, TXT(0x004B02E0) /* a full-width asterisk */);
                            }
                            for (; i < 10; i++) {
                                strcat(line, TXT(0x004B02E8) /* a full-width space */);
                            }
                        } else {
                            for (i = 0; i < 10; i++) {
                                strcat(line, TXT(0x004B02E8) /* a full-width space */);
                            }
                        }
                        strcat(line, strchr((char *)text, '%') + 2);
                        break;
                    }
                }
                Font_SetPos(0x140 - Text_DisplayLength(line) * 18 / 4, y);
                Text_Printf(line);
            } else {
                Font_SetPos(0x140 - Text_DisplayLength((char *)text) * 18 / 4, y);
                Text_Printf((char *)text);
            }
        }
        y += 0x12;
        lines++;
    }
    return y - 0x12;
}

/* Sets the state, resets sub to 0 and restarts the button's blink (blinkTimer 15, blinkPhase 1). */
static void NetMenu_SetState(NetMenu *menu, u8 state)
{
    menu->state = state;
    menu->sub = 0;
    menu->blinkTimer = 0xF;
    menu->blinkPhase = 1;
}

/* Draws the yes and no labels at y (x is unused); left and right move *cursor (0 yes, 1 no) and the
 * cursor box. Returns 0 for circle on yes, 1 for circle on no or for cross, else -1. */
s32 NetMenu_YesNo(NetMenu *menu, s32 x, s16 y, u8 *cursor)
{
    s32 i;
    s32 labelX;
    char **labels;

    if (*cursor == 0) {
        if (menu->buttons & PAD_RIGHT) {
            *cursor ^= 1;
            Sound_PlaySeFlat(0, SE_SYS_CURSOR, 0);
        }
    } else if (menu->buttons & PAD_LEFT) {
        *cursor ^= 1;
        Sound_PlaySeFlat(0, SE_SYS_CURSOR, 0);
    }
    Font_SetSpacing(0x12, 0x12);
    for (i = 0, labelX = 0xF8, labels = gNetMenuYesNoLabels; i < 2; i++, labelX += 0x5A, labels++) {
        if (*cursor == i) {
            Text_SetColor(5);
        } else {
            Text_SetColor(0);
        }
        Font_SetPos(labelX, y);
        Text_Printf("%s", *labels);
    }
    menu->showCursor = 1;
    if (*cursor == 0) {
        menu->boxX = 0x10A;
    } else {
        menu->boxX = 0x16D;
    }
    menu->boxY = y - 2;
    menu->boxW = 0x6C;
    menu->boxH = 0x16;
    if (menu->buttons & PAD_CIRCLE) {
        if (*cursor == 0) {
            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
            return 0;
        }
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        return 1;
    }
    if (menu->buttons & PAD_CROSS) {
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        return 1;
    }
    return -1;
}

/* Draws the two slots' labels (NETMENU_MSG_SLOT_LABELS' lines 0 and 2) below y. displayOnly 0: up
 * and down toggle *slot and the cursor box follows; returns *slot on circle, 2 on cross, else -1.
 * Otherwise it only draws and returns -1. */
s32 NetMenu_ChooseSlot(NetMenu *menu, s16 x, s16 y, u8 *slot, s32 displayOnly)
{
    s32 i;
    char **labels = D_004682AC[0];
    s32 firstY;
    s32 top;
    s32 lineY;

    if (displayOnly == 0 && (menu->buttons & (PAD_UP | PAD_DOWN))) {
        *slot ^= 1;
        Sound_PlaySeFlat(0, SE_SYS_CURSOR, 0);
    }
    Font_SetSpacing(0x12, 0x12);
    firstY = y + 0x12;
    top = (s16)firstY;
    lineY = top;
    for (i = 0; i < 2; i++, lineY += 0x24, labels += 2) {
        if (*slot == i) {
            Text_SetColor(5);
        } else {
            Text_SetColor(0);
        }
        Font_SetPos(x, lineY);
        Text_Printf(*labels);
    }
    if (displayOnly == 0) {
        menu->showCursor = 1;
        menu->boxX = x + 0x63;
        if (*slot == 0) {
            menu->boxY = firstY;
        } else {
            menu->boxY = top + 0x24;
        }
        menu->boxY -= 2;
        menu->boxW = 0x132;
        menu->boxH = 0x16;
        if (menu->buttons & PAD_CIRCLE) {
            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
            return *slot;
        }
        if (menu->buttons & PAD_CROSS) {
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            return 2;
        }
    }
    return -1;
}

/* Places the blinking button icon at x, y (hidden in mode 2). Returns 1 once circle is pressed,
 * else 0. mode picks the sound: SE_SYS_CONFIRM for 0 and 2, sound 4 of bank 1 for 3, SE_SYS_CANCEL
 * for 1. */
s32 NetMenu_WaitButton(NetMenu *menu, s16 x, s16 y, s32 mode)
{
    menu->buttonX = x;
    menu->buttonY = y;
    menu->blinkTimer--;
    if (menu->blinkTimer <= 0) {
        menu->blinkTimer = 0xF;
        menu->blinkPhase ^= 1;
    }
    if (mode != 2) {
        menu->showButton = menu->blinkPhase;
    } else {
        menu->showButton = 0;
    }
    if (menu->buttons & PAD_CIRCLE) {
        if (mode == 0 || mode == 2) {
            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        } else if (mode == 3) {
            Sound_PlaySeFlat(1, 4, 0);
        } else {
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        }
        return 1;
    }
    return 0;
}

/* 1 when the card in port slot was changed (Card_IsChanged) or is gone (Card_PortState 0), else 0.
 */
s32 NetMenu_CardChanged(s32 slot)
{
    if (Card_IsChanged(slot)) {
        return 1;
    }
    if (Card_PortState(slot) == 0) {
        return 1;
    }
    return 0;
}

/* Opens the card menu in a mode (enum NetMenuMode); NetMenu_Update then runs the mode's
 * CardMenu_Run*. Loads gCardMenuTex for the boot, logout, save-asking and login-id modes;
 * NETMENU_MODE_SAVE_B runs as NETMENU_MODE_SAVE with openMode set. Resets the state, done and
 * result[] (CARD_NOT_TRIED), clears gNetMenuDrawHooks and makes NetMenu_Draw hook 0's draw. */
void NetMenu_Open(u8 mode)
{
    NetMenu *menu = &gCardMenu;
    s32 buf;
    s32 i;

    D_006AF044 = 0;
    switch (mode) {
    case NETMENU_MODE_BOOT:
    case NETMENU_MODE_SAVE_ASK:
    case NETMENU_MODE_GAME_SAVE_LOGOUT:
    case NETMENU_MODE_DELETE_LOGIN_ID:
        menu->menuTexOn = 1;
        if (gCardMenuTex == 0) {
            buf = gLoadBuf;
            LoadCompressedFile((u8 *)buf, gSystemTexFiles);
            gCardMenuTex = Tex_CreateFromTim2((u8 *) (Pack_GetPart((u8 *)buf, 1)), 4);
        }
        break;
    default:
        menu->menuTexOn = 0;
        break;
    case NETMENU_MODE_SAVE_B:
        menu->openMode = mode;
        menu->menuTexOn = 0;
        mode = NETMENU_MODE_SAVE;
        break;
    }
    menu->state = 0;
    menu->sub = 0;
    menu->mode = mode;
    menu->done = 0;
    menu->result[0] = CARD_NOT_TRIED;
    menu->result[1] = CARD_NOT_TRIED;
    for (i = 0; i < 2; i++) {
        memset(gNetMenuDrawHooks[i], 0, 0x20);
    }
    gSysSettings.cardMenuDone = 0;
    D_006A3A24 = NetMenu_Draw;
    switch (mode) {
    case NETMENU_MODE_GAME_SAVE_TOWN:
    case NETMENU_MODE_GAME_SAVE_ENTRY:
    case NETMENU_MODE_GAME_SAVE_LOGOUT:
        func_003D5B70();
        break;
    }
}

s32 Gs_SetState(u8 reg, u32 v);
void Gfx_SetBlend(s32 a, s32 b);
s32 Gs_GetState(u8 reg);
s32 Tex_Use(s32 n, u32 *b);
s32 Sprite_Draw(Sprite *s);
struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, s8 *title, s32 sel, u8 alpha);
void Hud_DrawButtonIconWhite(s32 a, s32 b, s32 c, s32 d, f32 scale);
extern u8 D_006AF040;
void Text_SetSize(s32 size);
extern ListWin *gNetMenuPanels[];

/* Draws the card menu: the gCardMenuTex backdrop when menuTexOn (D_006AF040) is 1, the four layers
 * (a gNetMenuPanels panel, opaque in the settings modes; 5, 6 and 8 the slot frames:
 * CardMenu_DrawSlotFrames), the cursor box and the blinking button icon. Saves and restores the
 * z-write, blend and texture filter states. */
void NetMenu_Draw(void)
{
    NetMenu *menu = &gCardMenu;
    s32 savedZwrite;
    s32 savedBlend;
    s32 savedTexFilter;
    s32 i;
    u8 k;
    Sprite sprite;

    Gs_SetState(RS_ALPHA_REF, 0);
    Gfx_SetBlend(4, 5);
    if (D_006AF040 == 1 && gCardMenuTex != 0) {
        savedZwrite = Gs_GetState(0x6C);
        savedBlend = Gs_GetState(0x5E);
        savedTexFilter = Gs_GetState(0x63);
        Gs_GetState(4);
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x5E, 0x32);
        Gs_SetState(RS_TEX_FILTER, 0);
        Tex_Use(1, (u32 *)&gCardMenuTex);
        Gs_SetState(RS_TEXTURE, gCardMenuTex);
        sprite.w = 0x280;
        sprite.u1 = 0xFF;
        sprite.h = 0x1C0;
        sprite.v1 = 0xFF;
        sprite.color = -1;
        sprite.x = 0;
        sprite.y = 0;
        sprite.u0 = 0;
        sprite.v0 = 0;
        Sprite_Draw(&sprite);
        Gs_SetState(RS_TEX_FILTER, savedTexFilter);
        Gs_SetState(0x5E, savedBlend);
        Gs_SetState(0x6C, savedZwrite);
    }
    Text_SetSize(4);
    for (i = 0; i < 4; i++) {
        k = menu->layers[i];
        switch (k) {
        default:
            if (menu->mode == NETMENU_MODE_SAVE_SYSTEM || menu->mode == NETMENU_MODE_SAVE_GALLERY) {
                ListPanel_DrawAlpha(gNetMenuPanels[k], 0, -1, 0xFF);
            } else {
                ListPanel_DrawAlpha(gNetMenuPanels[k], 0, -1, 0x80);
            }
            break;
        case 5:
            CardMenu_DrawSlotFrames(&gChrEdit, 0, 0, (s8)gChrEdit.slot);
            break;
        case 6:
            CardMenu_DrawSlotFrames(&gChrEdit, 2, 0, (s8)gChrEdit.slot);
            break;
        case 8:
            CardMenu_DrawSlotFrames(&gChrEdit, 0, 0, menu->slotSel);
            break;
        case 0:
            break;
        }
    }
    if (menu->showCursor != 0) {
        Menu_DrawGlowBar(menu->boxX, menu->boxY, menu->boxW, menu->boxH, 0xFF20C0C0);
    }
    if (menu->showButton != 0) {
        switch (menu->mode) {
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            Hud_DrawButtonIconWhite(0x14, menu->buttonX, menu->buttonY, 0x146, 1.0f);
            break;
        default:
            Hud_DrawButtonIconWhite(0x14, menu->buttonX, menu->buttonY, 8, 1.0f);
            break;
        }
    }
    Text_SetSize(0);
}

/* Starts reading the patch file (file 8) of port menu->slot into readBuf, as fileKind 0x10. */
void NetMenu_StartCardRead(NetMenu *menu)
{
    CardTask_StartRead(menu->slot, (u8 *)menu->readBuf);
    CardTask_SelectFile(8, 0);
    CardState_SetFlags(0x10);
}

/* Selects the first old game's card operation (CardTask_Begin(2)), clears gOldGameKind and oldSlot
 * and starts a save check on port 0; then CardBoot_FindMH. */
void CardBoot_Start(NetMenu *menu)
{
    CardTask_Begin(2);
    NetMenu_SetState(menu, CARD_BOOT_FIND_MH);
    menu->slot = 0;
    menu->timer = 0;
    CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 0);
    CardTask_SelectFile(4, 0);
    gOldGameKind = 0;
    menu->oldSlot = -1;
}

/* Waits for the old game's save check. Found: gOldGameKind 1 and oldSlot, then CardBoot_AutoLoad.
 * Missing on port 0: tries port 1. Missing on both: checks port 0 for the G version's save
 * (CardTask_Begin(3)) and goes to CardBoot_FindMHG. */
void CardBoot_FindMH(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_CHECKING_BOTH);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        gOldGameKind = 1;
        menu->oldSlot = menu->slot;
        NetMenu_SetState(menu, CARD_BOOT_AUTO_LOAD);
        menu->slot = 0;
        menu->timer = 0;
        CardMenu_InitBuffers(menu);
        menu->saveStep = 0;
        menu->sub = 0;
        CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 0);
        break;
    case CARD_BUSY:
        break;
    default:
        if (menu->slot == 0) {
            menu->slot = 1;
            CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 0);
            CardTask_SelectFile(4, 0);
            break;
        }
        CardTask_Begin(3);
        NetMenu_SetState(menu, CARD_BOOT_FIND_MHG);
        menu->result[1] = 0;
        menu->result[0] = 0;
        menu->slot = 0;
        menu->timer = 0;
        CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 0);
        CardTask_SelectFile(4, 0);
        break;
    }
}

/* Waits for the G version's save check. Found: gOldGameKind 2 and oldSlot. After that, or when port
 * 1 has none either: clears cardHasChara, CardMenu_InitBuffers, then CardBoot_AutoLoad. */
void CardBoot_FindMHG(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_CHECKING_BOTH);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        gOldGameKind = 2;
        menu->oldSlot = menu->slot;
    next:
        NetMenu_SetState(menu, CARD_BOOT_AUTO_LOAD);
        gSysSettings.cardHasChara = 0;
        menu->slot = 0;
        menu->timer = 0;
        CardMenu_InitBuffers(menu);
        menu->saveStep = 0;
        menu->sub = 0;
        break;
    case CARD_BUSY:
        break;
    default:
        if (menu->slot != 0) {
            goto next;
        }
        menu->slot = 1;
        CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 0);
        CardTask_SelectFile(4, 0);
        break;
    }
}

/* sub 0 loads the system file from port 0, then port 1 (SaveMenu_LoadSystem); by the two results it
 * goes to CardBoot_ShowOldData (no save or unformatted) or asks whether to start anyway
 * (CardBoot_AskStart, with the message for bad data, no card or no space). sub 1 checks the
 * character files until one reads (cardHasChara 1), then CardBoot_Loaded. */
void CardBoot_AutoLoad(NetMenu *menu)
{
    s32 cardResult;
    s32 port0Result;
    s32 port1Result;

    switch (menu->sub) {
    case 0:
        NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_CHECKING_BOTH);
        switch (SaveMenu_LoadSystem((struct SaveMenu *)menu, 1)) {
        case 0:
            break;
        case 1:
            menu->charIndex = 0;
            menu->sub++;
            menu->saveStep = 0;
            break;
        case -1:
            switch (menu->result[menu->slot]) {
            case -0xFD:
            case -0xFE:
                if (menu->slot == 1) {
                    NetMenu_SetState(menu, CARD_BOOT_SHOW_OLD_DATA);
                    break;
                }
            case -0xFC:
            case -0xFF:
            default:
                if (menu->slot == 0) {
                    menu->slot = 1;
                    menu->sub = 0;
                    menu->saveStep = 0;
                    break;
                }
                port0Result = menu->result[0];
                if (port0Result == -0x100 && menu->result[1] != 0) {
                    menu->slot = 0;
                    NetMenu_SetState(menu, CARD_BOOT_ASK_START);
                    menu->msg = NETMENU_MSG_ASK_START_LOAD_FAILED;
                    menu->cursor = 1;
                    break;
                }
                if (port0Result == -0xFF && menu->result[1] == -0x100) {
                    menu->slot = 1;
                    NetMenu_SetState(menu, CARD_BOOT_ASK_START);
                    menu->msg = NETMENU_MSG_ASK_START_LOAD_FAILED;
                    menu->cursor = 1;
                    break;
                }
                if (port0Result == -0xFB && menu->result[1] != 0) {
                    menu->slot = 0;
                    NetMenu_SetState(menu, CARD_BOOT_ASK_START);
                    menu->msg = NETMENU_MSG_ASK_START_BAD_DATA;
                    menu->cursor = 1;
                    break;
                }
                port1Result = menu->result[1];
                if (port1Result == -0xFB && port0Result != 0) {
                    menu->slot = 1;
                    NetMenu_SetState(menu, CARD_BOOT_ASK_START);
                    menu->msg = NETMENU_MSG_ASK_START_BAD_DATA;
                    menu->cursor = 1;
                    break;
                }
                if (port0Result == -0xFD || port0Result == -0xFE) {
                    NetMenu_SetState(menu, CARD_BOOT_SHOW_OLD_DATA);
                    break;
                }
                if (port1Result == -0xFD || port1Result == -0xFE) {
                    NetMenu_SetState(menu, CARD_BOOT_SHOW_OLD_DATA);
                    break;
                }
                menu->cursor = 1;
                if (menu->result[0] == -0xFC && menu->result[1] == -0xFC) {
                    menu->slot = 0;
                    NetMenu_SetState(menu, CARD_BOOT_ASK_START);
                    menu->msg = NETMENU_MSG_ASK_START_NO_SPACE_BOTH;
                    break;
                }
                if (menu->result[0] == -0xFC) {
                    menu->slot = 0;
                    NetMenu_SetState(menu, CARD_BOOT_ASK_START);
                    menu->msg = NETMENU_MSG_ASK_START_NO_SPACE;
                    break;
                }
                if (menu->result[1] == -0xFC) {
                    menu->slot = 1;
                    NetMenu_SetState(menu, CARD_BOOT_ASK_START);
                    menu->msg = NETMENU_MSG_ASK_START_NO_SPACE;
                    break;
                }
                NetMenu_SetState(menu, CARD_BOOT_ASK_START);
                menu->msg = NETMENU_MSG_ASK_START_NO_CARD;
                break;
            }
            break;
        }
        break;
    case 1:
        NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_AUTOLOADING);
        SaveMenu_CheckCharacter((struct SaveMenu *)menu);
        cardResult = CardTask_Result();
        menu->result[menu->slot] = cardResult;
        switch (cardResult) {
        case CARD_OK:
            gSysSettings.cardHasChara = 1;
        done:
            NetMenu_SetState(menu, CARD_BOOT_LOADED);
            menu->timer = 0x78;
            break;
        case CARD_BUSY:
            break;
        default:
            menu->charIndex++;
            if (menu->charIndex >= 3) {
                menu->charIndex = 0;
                goto done;
            }
            menu->saveStep = 0;
            break;
        }
        break;
    }
}

/* 0x3D8510: states of the network menu */

s32 NetMenu_YesNo(NetMenu *menu, s32 x, s16 y, u8 *cursor);
s32 NetMenu_CardChanged(s32 slot);
void Patch_InstallFile(u8 *buf);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void CardMenu_InitBuffers(NetMenu *menu);
void SaveMenu_CopyAll(NetMenu *menu);
s32 SaveMenu_SaveSystem(struct SaveMenu *m);
void CardTask_StartDelete(s32 slot);
s32 CardTask_Result(void);
s32 SaveMenu_ReadBad(NetMenu *menu);
void Patch_BeginFile(u8 *buf);
void SysSave_MergeFlags(u8 *buf);
void CardTask_StartCheck(void);
void CardState_SetFlag(s32 a);
s32 SaveMenu_SaveCharacter(struct SaveMenu *m);
s32 SaveMenu_LoadCharacter(struct SaveMenu *m);
void CardTask_StartReadClear(s32 slot, u8 *buf, s32 size);
void CardTask_SelectFile(s32 a, s32 b);
void CardState_SetFlags(s32 a);
void Card_ClearChanged(void);
void SaveMenu_CopySystem(struct SaveMenu *m, u8 *data, s32 save);
void func_6AF3D0(void *a, s32 b);
void Save_CopyToSlot(s32 i);
void SaveMenu_CopyCharacterSlot(NetMenu *menu, s32 i);
extern s8 D_006ADAAE;
extern u8 gOldGameKind;
extern SaveData *gCardSlots;

/* Shows the loading notice for one frame, then CardBoot_ReadPatch. */
void CardBoot_Loaded(NetMenu *menu)
{
    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_AUTOLOADING);
    NetMenu_SetState(menu, CARD_BOOT_READ_PATCH);
}

/* Shows that the load finished until circle or the timer's end, then CardBoot_ShowOldData. */
void CardBoot_ShowLoaded(NetMenu *menu)
{
    if (NetMenu_WaitButton(menu, 0x1DE, NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_AUTOLOADED) + 0x12, 2) != 0 || menu->timer <= 0) {
        NetMenu_SetState(menu, CARD_BOOT_SHOW_OLD_DATA);
        return;
    }
}

/* When an old game's save was found (gOldGameKind) and the system data records none yet
 * (gSysSave.oldGameKind, read as D_006ADAAE), shows NETMENU_MSG_OLD_DATA_FOUND for 120 frames or
 * until circle. Then CARD_BOOT_DONE. */
void CardBoot_ShowOldData(NetMenu *menu)
{
    switch (menu->sub) {
    case 0:
        if (D_006ADAAE != 0 || gOldGameKind == 0) {
            NetMenu_SetState(menu, CARD_BOOT_DONE);
            break;
        }
        menu->timer = 0x78;
        menu->sub++;
    case 1:
        if (NetMenu_WaitButton(menu, 0x1DE, NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_OLD_DATA_FOUND) + 0x12, 2) != 0 || menu->timer <= 0) {
            NetMenu_SetState(menu, CARD_BOOT_DONE);
        }
        break;
    }
}

/* Asks menu->msg's start-anyway prompt. Yes: CardBoot_ShowOldData. No: back to CARD_BOOT_START,
 * which checks the cards again. */
void CardBoot_AskStart(NetMenu *menu)
{
    s32 msg = menu->msg;

    switch (NetMenu_YesNo(menu, 0x52, NetMenu_DrawMessage(0x64, 0x88, msg) + 0x12, &menu->cursor)) {
    case NETMENU_NO:
        menu->state = 0;
        break;
    case NETMENU_YES:
        NetMenu_SetState(menu, CARD_BOOT_SHOW_OLD_DATA);
        break;
    }
}

/* Shows the loading notice and starts reading the patch file (NetMenu_StartCardRead); then
 * CardBoot_CheckPatch. */
void CardBoot_ReadPatch(NetMenu *menu)
{
    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_AUTOLOADING);
    NetMenu_SetState(menu, CARD_BOOT_CHECK_PATCH);
    NetMenu_StartCardRead(menu);
}

/* Waits for the patch file. A good one with records: Patch_BeginFile, then CardBoot_LoadPatch. Bad,
 * without records, or a card error: the success sound, then CardBoot_ShowLoaded (timer 120 frames).
 */
void CardBoot_CheckPatch(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_AUTOLOADING);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        if (SaveMenu_ReadBad(menu) != 0) {
        fail:
            menu->timer = 0x78;
            NetMenu_SetState(menu, CARD_BOOT_SHOW_LOADED);
            Sound_PlaySeFlat(0, 0x19, 0);
            break;
        }
        if (Patch_FileHasPatch(menu->readBuf) == 0) {
            goto fail;
        }
        menu->timer = 0x78;
        NetMenu_SetState(menu, CARD_BOOT_LOAD_PATCH);
        Patch_BeginFile(menu->readBuf);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
    case CARD_ERR_NO_SPACE:
    case CARD_ERR_NO_CARD:
    default:
        menu->timer = 0x78;
        NetMenu_SetState(menu, CARD_BOOT_SHOW_LOADED);
        Sound_PlaySeFlat(0, 0x19, 0);
        break;
    }
}

/* Runs Patch_LoadStep until it ends, installs the patch when it returned 2 (Patch_InstallFile),
 * plays the success sound and goes to CardBoot_ShowLoaded. */
void CardBoot_LoadPatch(NetMenu *menu)
{
    s32 patchResult;

    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_AUTOLOADED);
    patchResult = Patch_LoadStep();
    if (patchResult != 0) {
        if (patchResult == 2) {
            Patch_InstallFile(menu->readBuf);
        }
        NetMenu_SetState(menu, CARD_BOOT_SHOW_LOADED);
        Sound_PlaySeFlat(0, 0x19, 0);
    }
}

/* NETMENU_MODE_BOOT: runs the CardBoot_* function of menu->state (enum CardBootState): finds an old
 * game's save, loads the system and character files and a newer patch. CARD_BOOT_DONE sets done 1.
 */
void CardMenu_RunBoot(NetMenu *menu)
{
    switch (menu->state) {
    case CARD_BOOT_START:
        CardBoot_Start(menu);
        break;
    case CARD_BOOT_FIND_MH:
        CardBoot_FindMH(menu);
        break;
    case CARD_BOOT_FIND_MHG:
        CardBoot_FindMHG(menu);
        break;
    case CARD_BOOT_FIND_MHG2:
        CardBoot_FindMHG(menu);
        break;
    case CARD_BOOT_AUTO_LOAD:
        CardBoot_AutoLoad(menu);
        break;
    case CARD_BOOT_LOADED:
        CardBoot_Loaded(menu);
        break;
    case CARD_BOOT_SHOW_LOADED:
        CardBoot_ShowLoaded(menu);
        break;
    case CARD_BOOT_SHOW_OLD_DATA:
        CardBoot_ShowOldData(menu);
        break;
    case CARD_BOOT_ASK_START:
        CardBoot_AskStart(menu);
        break;
    case CARD_BOOT_READ_PATCH:
        CardBoot_ReadPatch(menu);
        break;
    case CARD_BOOT_CHECK_PATCH:
        CardBoot_CheckPatch(menu);
        break;
    case CARD_BOOT_LOAD_PATCH:
        CardBoot_LoadPatch(menu);
        break;
    case CARD_BOOT_DONE:
        menu->done = 1;
        break;
    }
}

/* Puts the cursor on no and the port at 0; then CardSys_AskSave. */
void CardSys_Start(NetMenu *menu)
{
    NetMenu_SetState(menu, CARD_SYS_ASK_SAVE);
    menu->cursor = 1;
    menu->slot = 0;
}

/* Asks whether to save the settings (the gallery's prompt in NETMENU_MODE_SAVE_GALLERY). Yes:
 * CardSys_ChooseSlot (port 0). No: CARD_SYS_DONE. */
void CardSys_AskSave(NetMenu *menu)
{
    s16 lastY;

    if (menu->mode == NETMENU_MODE_SAVE_SYSTEM) {
        lastY = NetMenu_DrawMessage(0xDD, 0x88, NETMENU_MSG_ASK_SAVE_SETTINGS);
    } else {
        lastY = NetMenu_DrawMessage(0xDD, 0x88, NETMENU_MSG_ASK_SAVE_GALLERY);
    }
    switch (NetMenu_YesNo(menu, 0xCB, lastY + 0x24, &menu->cursor)) {
    case NETMENU_NO:
        NetMenu_SetState(menu, CARD_SYS_DONE);
        break;
    case NETMENU_YES:
        NetMenu_SetState(menu, CARD_SYS_CHOOSE_SLOT);
        menu->sub = 0;
        menu->slot = 0;
        break;
    }
}

/* Asks for the port. After a choice it waits 4 frames, calls CardMenu_InitBuffers and goes to
 * CardSys_ReadCharacters; cross goes back to CardSys_AskSave. */
void CardSys_ChooseSlot(NetMenu *menu)
{
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHOOSE_SAVE_SLOT);
    switch (menu->sub) {
    case 0:
        switch (NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 0)) {
        case 0:
        case 1:
            menu->sub++;
            menu->timer = 4;
            break;
        case NETMENU_SLOT_CANCEL:
            NetMenu_SetState(menu, CARD_SYS_ASK_SAVE);
            break;
        }
        break;
    case 1:
        NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
        if (--menu->timer <= 0) {
            NetMenu_SetState(menu, CARD_SYS_READ_CHARACTERS);
            menu->timer = 0x1E;
            CardMenu_InitBuffers(menu);
            menu->charIndex = 0;
            menu->saveStep = 0;
        }
        break;
    }
}

/* Reads the three character files (SaveMenu_LoadCharacter), keeping each one's date stamp in
 * pairs[charIndex + 1], then starts reading the system file (fileKind 1) and goes to
 * CardSys_CheckSystem. */
void CardSys_ReadCharacters(NetMenu *menu)
{
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    switch (SaveMenu_LoadCharacter((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        menu->pairs[menu->charIndex + 1][0] = menu->charId0;
        menu->pairs[menu->charIndex + 1][1] = menu->charId1;
    case -1:
        menu->charIndex++;
        if (menu->charIndex == 3) {
            NetMenu_SetState(menu, CARD_SYS_CHECK_SYSTEM);
            CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 1);
            CardTask_SelectFile(4, 0);
            CardState_SetFlags(1);
            break;
        }
        menu->saveStep = 0;
        break;
    }
}

/* Waits for the system file. Read: SysSave_MergeFlags, then CardSys_AskOverwrite (the cursor on
 * no). Bad data: CardSys_PrepareSave. No file: every file marked for writing
 * (CardState_SetFlag(0x1F)), then CardSys_PrepareSave. Unformatted: CardSys_AskFormat. Other
 * errors: CardSys_ShowError. */
void CardSys_CheckSystem(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        Card_ClearChanged();
        if (SaveMenu_ReadBad(menu) != 0) {
            goto retry;
        }
        SysSave_MergeFlags(menu->readBuf);
        NetMenu_SetState(menu, CARD_SYS_ASK_OVERWRITE);
        CardTask_StartCheck();
        menu->cursor = 1;
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
        CardState_SetFlag(0x1F);
    retry:
        NetMenu_SetState(menu, CARD_SYS_PREPARE_SAVE);
        break;
    case CARD_ERR_NO_SPACE:
        NetMenu_SetState(menu, CARD_SYS_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_SPACE;
        break;
    case CARD_ERR_NO_CARD:
        NetMenu_SetState(menu, CARD_SYS_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_CARD;
        break;
    case CARD_ERR_UNFORMATTED:
        NetMenu_SetState(menu, CARD_SYS_ASK_FORMAT);
        menu->cursor = 1;
        Card_ClearChanged();
        CardTask_StartCheck();
        break;
    default:
        NetMenu_SetState(menu, CARD_SYS_SHOW_ERROR);
        menu->msg = NETMENU_MSG_BAD_DATA;
        break;
    }
}

/* Asks to format the card. Yes: starts the format (CardTask_StartDelete) and goes to
 * CardSys_Format. No: CardSys_ChooseSlot. A changed or removed card: CardSys_ShowError. */
void CardSys_AskFormat(NetMenu *menu)
{
    s16 lastY;

    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    lastY = NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_ASK_FORMAT);
    if (NetMenu_CardChanged(menu->slot) != 0) {
        NetMenu_SetState(menu, CARD_SYS_SHOW_ERROR);
        menu->msg = NETMENU_MSG_CARD_CHANGED;
        return;
    }
    switch (NetMenu_YesNo(menu, 0x16, lastY + 0x12, &menu->cursor)) {
    case NETMENU_NO:
        NetMenu_SetState(menu, CARD_SYS_CHOOSE_SLOT);
        break;
    case NETMENU_YES:
        NetMenu_SetState(menu, CARD_SYS_FORMAT);
        menu->timer = 0x1E;
        CardTask_StartDelete(menu->slot);
        break;
    }
}

/* Waits for the format. Done: every file marked for writing (CardState_SetFlag(0x1F)), then
 * CardSys_PrepareSave. Failed: CardSys_ShowError. */
void CardSys_Format(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_FORMATTING);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        CardState_SetFlag(0x1F);
        NetMenu_SetState(menu, CARD_SYS_PREPARE_SAVE);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_FAILED:
    default:
        NetMenu_SetState(menu, CARD_SYS_SHOW_ERROR);
        menu->msg = NETMENU_MSG_FORMAT_FAILED;
        break;
    }
}

/* Unused (no state sets CARD_SYS_CHECK_SYSTEM_READ_PATCH). Waits for the system file: a good one
 * merges its flags, starts reading the patch file and goes to CardSys_CheckPatch; bad data or an
 * error goes to CardSys_ShowError. */
void CardSys_CheckSystemReadPatch(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        if (SaveMenu_ReadBad(menu) != 0) {
            NetMenu_SetState(menu, CARD_SYS_SHOW_ERROR);
            menu->msg = NETMENU_MSG_BAD_DATA;
            break;
        }
        SysSave_MergeFlags(menu->readBuf);
        NetMenu_SetState(menu, CARD_SYS_CHECK_PATCH);
        NetMenu_StartCardRead(menu);
        CardTask_StartCheck();
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
    case CARD_ERR_NO_SPACE:
    case CARD_ERR_NO_CARD:
    default:
        NetMenu_SetState(menu, CARD_SYS_SHOW_ERROR);
        menu->msg = NETMENU_MSG_BAD_DATA;
        break;
    }
}

/* Reached only from CardSys_CheckSystemReadPatch. Waits for the patch file: with records,
 * Patch_BeginFile and CardSys_LoadPatch; bad or none, CardSys_AskOverwrite; a card error,
 * CardSys_AskFormat. */
void CardSys_CheckPatch(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        if (SaveMenu_ReadBad(menu) != 0) {
        fail:
            menu->timer = 0x78;
            NetMenu_SetState(menu, CARD_SYS_ASK_OVERWRITE);
            break;
        }
        if (Patch_FileHasPatch(menu->readBuf) == 0) {
            goto fail;
        }
        menu->timer = 0x78;
        NetMenu_SetState(menu, CARD_SYS_LOAD_PATCH);
        Patch_BeginFile(menu->readBuf);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
    case CARD_ERR_NO_SPACE:
    case CARD_ERR_NO_CARD:
    default:
        menu->timer = 0x78;
        NetMenu_SetState(menu, CARD_SYS_ASK_FORMAT);
        break;
    }
}

/* Runs Patch_LoadStep until it ends, installs the patch when it returned 2, then goes to
 * CardSys_AskOverwrite with the cursor on no. */
void CardSys_LoadPatch(NetMenu *menu)
{
    s32 patchResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    patchResult = Patch_LoadStep();
    if (patchResult != 0) {
        if (patchResult == 2) {
            Patch_InstallFile(menu->readBuf);
        }
        NetMenu_SetState(menu, CARD_SYS_ASK_OVERWRITE);
        menu->cursor = 1;
    }
}

/* Asks to overwrite the save. Yes: marks only the system file for writing (CardState_SetFlag(1)),
 * then CardSys_PrepareSave. No: CardSys_ChooseSlot. A changed card: CardSys_ShowError. */
void CardSys_AskOverwrite(NetMenu *menu)
{
    s16 lastY;

    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    lastY = NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_ASK_OVERWRITE);
    if (NetMenu_CardChanged(menu->slot) != 0) {
        NetMenu_SetState(menu, CARD_SYS_SHOW_ERROR);
        menu->msg = NETMENU_MSG_CARD_CHANGED;
        return;
    }
    switch (NetMenu_YesNo(menu, 0x16, lastY + 0x12, &menu->cursor)) {
    case NETMENU_NO:
        NetMenu_SetState(menu, CARD_SYS_CHOOSE_SLOT);
        break;
    case NETMENU_YES:
        CardState_SetFlag(1);
        NetMenu_SetState(menu, CARD_SYS_PREPARE_SAVE);
        break;
    }
}

/* Rebuilds the buffers and the card image (CardMenu_InitBuffers, SaveMenu_CopyAll), shows the
 * saving notice and goes to CardSys_Save. */
void CardSys_PrepareSave(NetMenu *menu)
{
    CardMenu_InitBuffers(menu);
    SaveMenu_CopyAll(menu);
    NetMenu_SetState(menu, CARD_SYS_SAVE);
    menu->timer = 0x1E;
    menu->saveStep = 0;
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVING);
}

/* Saves the system file (SaveMenu_SaveSystem). Done: the success sound, then CARD_SYS_DONE. Failed:
 * CardSys_ShowError. */
void CardSys_Save(NetMenu *menu)
{
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVING);
    switch (SaveMenu_SaveSystem((struct SaveMenu *)menu)) {
    case 1:
        NetMenu_SetState(menu, CARD_SYS_DONE);
        Sound_PlaySeFlat(0, 0x19, 0);
        break;
    case 0:
        break;
    case -1:
        NetMenu_SetState(menu, CARD_SYS_SHOW_ERROR);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        break;
    }
}

/* Unused (CardSys_Save goes to CARD_SYS_DONE). Shows the saved notice until circle, then
 * CARD_SYS_DONE. */
void CardSys_ShowSaved(NetMenu *menu)
{
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVED);
    if (NetMenu_WaitButton(menu, 0x232, 0x184, 0) != 0) {
        NetMenu_SetState(menu, CARD_SYS_DONE);
    }
}

/* Shows menu->msg until circle, then goes back to CardSys_ChooseSlot. */
void CardSys_ShowError(NetMenu *menu)
{
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, menu->msg);
    if (NetMenu_WaitButton(menu, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(menu, CARD_SYS_CHOOSE_SLOT);
    }
}

/* NETMENU_MODE_SAVE_SYSTEM and NETMENU_MODE_SAVE_GALLERY: saves the system data. Asks first, picks
 * a port, reads the three character files and the system file, offers a format or an overwrite,
 * then writes the system file. Ends with done 1 (CARD_SYS_DONE); CARD_SYS_CHECK_SYSTEM_READ_PATCH,
 * CARD_SYS_CHECK_PATCH, CARD_SYS_LOAD_PATCH and CARD_SYS_SHOW_SAVED are not reached. */
void CardMenu_RunSaveSystem(NetMenu *menu)
{
    switch (menu->state) {
    case CARD_SYS_START:
        CardSys_Start(menu);
        break;
    case CARD_SYS_ASK_SAVE:
        CardSys_AskSave(menu);
        break;
    case CARD_SYS_CHOOSE_SLOT:
        CardSys_ChooseSlot(menu);
        break;
    case CARD_SYS_READ_CHARACTERS:
        CardSys_ReadCharacters(menu);
        break;
    case CARD_SYS_CHECK_SYSTEM:
        CardSys_CheckSystem(menu);
        break;
    case CARD_SYS_ASK_FORMAT:
        CardSys_AskFormat(menu);
        break;
    case CARD_SYS_FORMAT:
        CardSys_Format(menu);
        break;
    case CARD_SYS_CHECK_SYSTEM_READ_PATCH:
        CardSys_CheckSystemReadPatch(menu);
        break;
    case CARD_SYS_CHECK_PATCH:
        CardSys_CheckPatch(menu);
        break;
    case CARD_SYS_LOAD_PATCH:
        CardSys_LoadPatch(menu);
        break;
    case CARD_SYS_ASK_OVERWRITE:
        CardSys_AskOverwrite(menu);
        break;
    case CARD_SYS_PREPARE_SAVE:
        CardSys_PrepareSave(menu);
        break;
    case CARD_SYS_SAVE:
        CardSys_Save(menu);
        break;
    case CARD_SYS_SHOW_SAVED:
        CardSys_ShowSaved(menu);
        break;
    case CARD_SYS_SHOW_ERROR:
        CardSys_ShowError(menu);
        break;
    case CARD_SYS_DONE:
        menu->done = 1;
        break;
    }
}

/* Clears hasSave, the port and the cursor, then CardNew_AskSave. */
void CardNew_Start(NetMenu *menu)
{
    NetMenu_SetState(menu, CARD_NEW_ASK_SAVE);
    menu->cursor = 0;
    menu->slot = 0;
    menu->hasSave = 0;
}

/* Asks whether to save the new character. Yes: CardNew_ChooseSlot. No: CARD_NEW_QUIT (done 2). */
void CardNew_AskSave(NetMenu *menu)
{
    switch (NetMenu_YesNo(menu, 0x52, NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_ASK_SAVE_NEW_CHARA) + 0x12, &menu->cursor)) {
    case NETMENU_NO:
        NetMenu_SetState(menu, CARD_NEW_QUIT);
        break;
    case NETMENU_YES:
        NetMenu_SetState(menu, CARD_NEW_CHOOSE_SLOT);
        break;
    }
}

/* sub 0 picks the port (cross goes back to CardNew_AskSave); sub 1 reads the system file and merges
 * its flags; sub 2 loads the three character files, then CardNew_ChooseCharSlot. No save file or an
 * unformatted card goes there with charLoaded 0; other errors go to CardNew_ShowError. */
void CardNew_ChooseSlot(NetMenu *menu)
{
    s32 cardResult;

    switch (menu->sub) {
    case 0:
        NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHOOSE_SAVE_SLOT);
        switch (NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 0)) {
        case 0:
        case 1:
            menu->sub++;
            menu->timer = 4;
            menu->charNo = 0;
            CardMenu_InitBuffers(menu);
            SaveMenu_CopyAll(menu);
            CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 1);
            CardTask_SelectFile(4, 0);
            CardState_SetFlags(1);
            menu->timer = 0x1E;
            break;
        case NETMENU_SLOT_CANCEL:
            NetMenu_SetState(menu, CARD_NEW_ASK_SAVE);
            menu->cursor = 0;
            break;
        }
        break;
    case 1:
        NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
        NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
        cardResult = CardTask_Result();
        menu->result[menu->slot] = cardResult;
        switch (cardResult) {
        case CARD_OK:
            if (SaveMenu_ReadBad(menu) == 0) {
                SysSave_MergeFlags(menu->readBuf);
            }
            menu->timer = 0x1E;
            menu->sub++;
            menu->charLoaded = 0;
            Card_ClearChanged();
            D_006ADAAC = 0;
            menu->charIndex = menu->charNo;
            menu->saveStep = 0;
            break;
        case CARD_BUSY:
            break;
        case CARD_ERR_NO_FILE:
        case CARD_ERR_UNFORMATTED:
            NetMenu_SetState(menu, CARD_NEW_CHOOSE_CHAR_SLOT);
            menu->charLoaded = 0;
            gChrEdit.slot = 0;
            Card_ClearChanged();
            CardTask_StartCheck();
            break;
        case CARD_ERR_NO_SPACE:
            NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
            menu->msg = NETMENU_MSG_NO_SPACE;
            break;
        case CARD_ERR_NO_CARD:
            NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
            menu->msg = NETMENU_MSG_NO_CARD;
            break;
        default:
            NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
            menu->msg = NETMENU_MSG_BAD_DATA;
            break;
        }
        break;
    case 2:
        NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
        NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
        switch (SaveMenu_LoadCharacter((struct SaveMenu *)menu)) {
        case 0:
            break;
        case 1:
            menu->pairs[menu->charIndex + 1][0] = menu->charId0;
            menu->pairs[menu->charIndex + 1][1] = menu->charId1;
            menu->charLoaded |= 1;
            D_006ADAAC++;
        next:
            menu->charNo++;
            if (menu->charNo == 3) {
                CardTask_StartCheck();
                menu->timer = 0x78;
                NetMenu_SetState(menu, CARD_NEW_CHOOSE_CHAR_SLOT);
                menu->charNo = 0;
            } else {
                menu->charIndex = menu->charNo;
                menu->saveStep = 0;
            }
            break;
        case -1:
            switch (menu->result[menu->slot]) {
            case CARD_ERR_NO_CARD:
                NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
                menu->msg = NETMENU_MSG_NO_CARD;
                menu->charNo = 0;
                break;
            case CARD_ERR_NO_FILE:
            case CARD_ERR_NO_SPACE:
            case CARD_ERR_BAD_DATA:
                goto next;
            case CARD_ERR_UNFORMATTED:
                NetMenu_SetState(menu, CARD_NEW_CHOOSE_CHAR_SLOT);
                menu->charLoaded = 0;
                gChrEdit.slot = 0;
                menu->charIndex = 0;
                CardTask_StartCheck();
                break;
            default:
                NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
                menu->msg = NETMENU_MSG_BAD_DATA;
                menu->charNo = 0;
                break;
            }
            break;
        }
        break;
    }
}

/* Picks one of the three character slots (gChrEdit.slot, up and down). Confirm goes to
 * CardNew_ConfirmCharSlot with the cursor on no and the prompt for a used slot, an empty slot, or a
 * card without a save (charLoaded not 1). Cross goes back to CardNew_ChooseSlot; a changed card to
 * CardNew_ShowError. */
void CardNew_ChooseCharSlot(NetMenu *menu)
{
    SaveData *saves;

    if (menu->buttons & PAD_UP) {
        Sound_PlaySeFlat(0, SE_SYS_CURSOR, 0);
        gChrEdit.slot--;
        if ((s8)gChrEdit.slot < 0) {
            gChrEdit.slot = 2;
        }
    }
    if (menu->buttons & PAD_DOWN) {
        Sound_PlaySeFlat(0, SE_SYS_CURSOR, 0);
        gChrEdit.slot++;
        if (gChrEdit.slot > 2) {
            gChrEdit.slot = 0;
        }
    }
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHOOSE_CHARA_SLOT);
    if (NetMenu_CardChanged(menu->slot) != 0) {
        NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
        menu->msg = NETMENU_MSG_CARD_CHANGED;
        return;
    }
    if (menu->buttons & PAD_CIRCLE) {
        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        NetMenu_SetState(menu, CARD_NEW_CONFIRM_CHAR_SLOT);
        menu->cursor = 1;
        saves = menu->slots;
        if (menu->charLoaded == 1) {
            if (saves[gChrEdit.slot].used == 0) {
                menu->msg = NETMENU_MSG_ASK_SAVE_HERE;
            } else {
                menu->msg = NETMENU_MSG_ASK_OVERWRITE_CHARA;
            }
        } else {
            menu->msg = NETMENU_MSG_ASK_CREATE_DATA;
        }
    } else if (menu->buttons & PAD_CROSS) {
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        NetMenu_SetState(menu, CARD_NEW_CHOOSE_SLOT);
    }
    menu->layers[1] = 5;
    CardMenu_DrawCharacters(&gChrEdit, 0, 0, 0, (s8)gChrEdit.slot);
}

/* Shows menu->msg over a yes/no box. Yes, by the port's result: CardNew_PrepareSave (done, no file,
 * bad data), CardNew_AskFormat (unformatted), else CardNew_ShowError. No goes back to
 * CardNew_ChooseCharSlot; a changed card to CardNew_ShowError. */
void CardNew_ConfirmCharSlot(NetMenu *menu)
{
    s16 lastY = NetMenu_DrawMessage(0xDD, 0x88, menu->msg);

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHOOSE_CHARA_SLOT);
    if (NetMenu_CardChanged(menu->slot) != 0) {
        NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
        menu->msg = NETMENU_MSG_CARD_CHANGED;
        return;
    }
    switch (NetMenu_YesNo(menu, 0xCB, lastY + 0x24, &menu->cursor)) {
    case NETMENU_NO:
        NetMenu_SetState(menu, CARD_NEW_CHOOSE_CHAR_SLOT);
        break;
    case NETMENU_YES:
        Sound_PlaySeFlat(1, 6, 0);
        switch (menu->result[menu->slot]) {
        case 0:
        case CARD_ERR_NO_FILE:
        case CARD_ERR_BAD_DATA:
            NetMenu_SetState(menu, CARD_NEW_PREPARE_SAVE);
            break;
        case CARD_ERR_UNFORMATTED:
            NetMenu_SetState(menu, CARD_NEW_ASK_FORMAT);
            menu->cursor = 1;
            break;
        default:
            NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
            menu->msg = NETMENU_MSG_SAVE_FAILED;
            break;
        }
        break;
    }
}

/* Asks to format the card. Yes: starts the format (CardTask_StartDelete), then CardNew_Format. No:
 * CardNew_ChooseSlot. A changed card: CardNew_ShowError. */
void CardNew_AskFormat(NetMenu *menu)
{
    s16 lastY = NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_ASK_FORMAT);

    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    if (NetMenu_CardChanged(menu->slot) != 0) {
        NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
        menu->msg = NETMENU_MSG_CARD_CHANGED;
        return;
    }
    switch (NetMenu_YesNo(menu, 0x16, lastY + 0x12, &menu->cursor)) {
    case NETMENU_NO:
        NetMenu_SetState(menu, CARD_NEW_CHOOSE_SLOT);
        break;
    case NETMENU_YES:
        NetMenu_SetState(menu, CARD_NEW_FORMAT);
        menu->timer = 0x1E;
        CardTask_StartDelete(menu->slot);
        break;
    }
}

/* Waits for the format. Done: CardNew_PrepareSave. Failed: CardNew_ShowError. */
void CardNew_Format(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_FORMATTING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        NetMenu_SetState(menu, CARD_NEW_PREPARE_SAVE);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_FAILED:
    default:
        NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
        menu->msg = NETMENU_MSG_FORMAT_FAILED;
        break;
    }
}

/* Builds the new character from gChrEdit into gSysSave.save (Chara_MakeSave, through func_6AF3D0),
 * copies it to gCardSlots[gChrEdit.slot] and into the card image, sets gSysSave.lastSlot, charSaved
 * and fileKind, then CardNew_SaveCharacter. */
void CardNew_PrepareSave(NetMenu *menu)
{
    s32 i = gChrEdit.slot;

    menu->charIndex = i;
    NetMenu_SetState(menu, CARD_NEW_SAVE_CHARACTER);
    menu->timer = 0x1E;
    func_6AF3D0(&gChrEdit, 0);
    Save_CopyToSlot(i);
    gSysSave.lastSlot = gChrEdit.slot;
    gSysSave.charSaved = 3;
    SaveMenu_CopyCharacterSlot(menu, i);
    menu->fileKind = 2 << i;
    Card_ClearChanged();
    menu->charIndex = gChrEdit.slot;
    menu->saveStep = 0;
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
}

/* Writes the character file. Done: sets hasSave and gSysSettings.cardHasChara, then
 * CardNew_SaveSystem. Failed: CardNew_ShowError. */
void CardNew_SaveCharacter(NetMenu *menu)
{
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    switch (SaveMenu_SaveCharacter((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        NetMenu_SetState(menu, CARD_NEW_SAVE_SYSTEM);
        menu->hasSave = 1;
        gSysSettings.cardHasChara = 1;
        menu->saveStep = 0;
        break;
    case -1:
        NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        break;
    }
}

/* Writes the system file. Done: the success sound, gSysSettings.systemSaved, then
 * CardNew_ShowSaved. Failed: CardNew_ShowError. */
void CardNew_SaveSystem(NetMenu *menu)
{
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    switch (SaveMenu_SaveSystem((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        NetMenu_SetState(menu, CARD_NEW_SHOW_SAVED);
        Sound_PlaySeFlat(0, 0x19, 0);
        gSysSettings.systemSaved = 1;
        break;
    case -1:
        NetMenu_SetState(menu, CARD_NEW_SHOW_ERROR);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        break;
    }
}

/* Shows the saved notice until a button press, then CARD_NEW_DONE (done 1). */
void CardNew_ShowSaved(NetMenu *menu)
{
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVED_KEEP_CARD);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    if (NetMenu_WaitButton(menu, 0x232, 0x184, 3) != 0) {
        NetMenu_SetState(menu, CARD_NEW_DONE);
    }
}

/* Shows menu->msg until a button press, then goes back to CardNew_ChooseSlot. */
void CardNew_ShowError(NetMenu *menu)
{
    s32 msg = menu->msg;

    NetMenu_DrawMessage(0x28, 0x160, msg);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    if (NetMenu_WaitButton(menu, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(menu, CARD_NEW_CHOOSE_SLOT);
    }
}

/* Unused (no state sets CARD_NEW_CHECK_PATCH). A good file with a patch starts it (Patch_BeginFile)
 * and goes to CardNew_LoadPatch; anything else goes to CardNew_ChooseCharSlot. */
void CardNew_CheckPatch(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        if (SaveMenu_ReadBad(menu) != 0) {
        fail:
            menu->timer = 0x78;
            NetMenu_SetState(menu, CARD_NEW_CHOOSE_CHAR_SLOT);
            break;
        }
        if (Patch_FileHasPatch(menu->readBuf) == 0) {
            goto fail;
        }
        menu->timer = 0x78;
        NetMenu_SetState(menu, CARD_NEW_LOAD_PATCH);
        Patch_BeginFile(menu->readBuf);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
    case CARD_ERR_NO_SPACE:
    case CARD_ERR_NO_CARD:
    default:
        menu->timer = 0x78;
        NetMenu_SetState(menu, CARD_NEW_CHOOSE_CHAR_SLOT);
        break;
    }
}

/* Runs Patch_LoadStep until it ends, installs the patch when it returned 2, then
 * CardNew_ChooseCharSlot. */
void CardNew_LoadPatch(NetMenu *menu)
{
    s32 patchResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    patchResult = Patch_LoadStep();
    if (patchResult != 0) {
        if (patchResult == 2) {
            Patch_InstallFile(menu->readBuf);
        }
        menu->timer = 0x78;
        NetMenu_SetState(menu, CARD_NEW_CHOOSE_CHAR_SLOT);
    }
}

/* NETMENU_MODE_SAVE_NEW (Chara_CreateTask): saves a newly made character. Asks, picks a port, reads
 * its system and character files, picks a character slot, formats when needed, then writes the
 * character file and the system file. done 1 once saved, 2 when the player declines. */
void CardMenu_RunSaveNew(NetMenu *menu)
{
    switch (menu->state) {
    case CARD_NEW_START:
        CardNew_Start(menu);
        break;
    case CARD_NEW_ASK_SAVE:
        CardNew_AskSave(menu);
        break;
    case CARD_NEW_CHOOSE_SLOT:
        CardNew_ChooseSlot(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_CHECK_PATCH:
        CardNew_CheckPatch(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_LOAD_PATCH:
        CardNew_LoadPatch(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_CHOOSE_CHAR_SLOT:
        CardNew_ChooseCharSlot(menu);
        break;
    case CARD_NEW_CONFIRM_CHAR_SLOT:
        CardNew_ConfirmCharSlot(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_ASK_FORMAT:
        CardNew_AskFormat(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_FORMAT:
        CardNew_Format(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_PREPARE_SAVE:
        CardNew_PrepareSave(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_SAVE_CHARACTER:
        CardNew_SaveCharacter(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_SAVE_SYSTEM:
        CardNew_SaveSystem(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_SHOW_SAVED:
        CardNew_ShowSaved(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_SHOW_ERROR:
        CardNew_ShowError(menu);
        menu->layers[1] = 2;
        break;
    case CARD_NEW_DONE:
        menu->done = 1;
        break;
    case CARD_NEW_QUIT:
    default:
        menu->done = 2;
        break;
    }
}

/* Clears hasSave and the port, then CardLoad_ChooseSlot. */
void CardLoad_Start(NetMenu *menu)
{
    NetMenu_SetState(menu, CARD_LOAD_CHOOSE_SLOT);
    menu->slot = 0;
    menu->hasSave = 0;
}

/* Picks the port to load from (cross: CARD_LOAD_QUIT, done 2). After the pick it sets up the load
 * buffers, starts reading the system file and goes to CardLoad_CheckSystem. */
void CardLoad_ChooseSlot(NetMenu *menu)
{
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHOOSE_LOAD_SLOT);
    switch (menu->sub) {
    case 0:
        switch (NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 0)) {
        case 0:
        case 1:
            menu->sub++;
            menu->timer = 4;
            break;
        case NETMENU_SLOT_CANCEL:
            NetMenu_SetState(menu, CARD_LOAD_QUIT);
            break;
        }
        break;
    case 1:
        NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
        if (menu->timer <= 0) {
            NetMenu_SetState(menu, CARD_LOAD_CHECK_SYSTEM);
            menu->timer = 0x1E;
            CardMenu_InitLoadBuffers(menu);
            menu->fileKind = 1;
            CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 1);
            CardTask_SelectFile(4, 0);
        }
        break;
    }
}

/* Checks the system file read. A good one is copied in (SaveMenu_CopySystem), the patch file's read
 * starts, then CardLoad_CheckPatch. Otherwise CardLoad_ShowError, with the message for bad data
 * (result set to CARD_ERR_BAD_DATA), no save, or no card. */
void CardLoad_CheckSystem(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        if (SaveMenu_ReadBad(menu) != 0) {
            menu->result[menu->slot] = -0xFB;
            NetMenu_SetState(menu, CARD_LOAD_SHOW_ERROR);
            menu->msg = NETMENU_MSG_BAD_DATA;
            break;
        }
        SaveMenu_CopySystem((struct SaveMenu *)menu, (u8 *)menu->readBuf, 0);
        Card_ClearChanged();
        NetMenu_SetState(menu, CARD_LOAD_CHECK_PATCH);
        NetMenu_StartCardRead(menu);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
        NetMenu_SetState(menu, CARD_LOAD_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_GAME_DATA;
        break;
    case CARD_ERR_UNFORMATTED:
        NetMenu_SetState(menu, CARD_LOAD_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_GAME_DATA;
        break;
    case CARD_ERR_NO_SPACE:
        NetMenu_SetState(menu, CARD_LOAD_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_GAME_DATA;
        break;
    case CARD_ERR_NO_CARD:
        NetMenu_SetState(menu, CARD_LOAD_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_CARD_LOAD;
        break;
    default:
        NetMenu_SetState(menu, CARD_LOAD_SHOW_ERROR);
        menu->msg = NETMENU_MSG_BAD_DATA;
        break;
    }
}

/* A mask with bit i set when character slot i is free: not read (charRead 0), or read with an
 * unused gCardSlots entry; bad slots (0x80) are not free. 7: all three free. */
static s32 SaveMenu_FreeCharSlots(NetMenu *menu)
{
    s32 freeMask = 0;
    s32 i;
    SaveData *save = gCardSlots;

    for (i = 0; i < 3; i++, save++) {
        switch (menu->charRead[i]) {
        case 0:
            freeMask |= 1 << i;
            break;
        case 1:
            if (save->used == 0) {
                freeMask |= 1 << i;
            }
            break;
        case 0x80:
            break;
        }
    }
    return freeMask;
}

/* Loads the three character files one by one, skipping the missing ones. After the third: when
 * every slot is free, CardLoad_ShowError and cardHasChara 0; else hasSave and cardHasChara set, the
 * success sound, then CardLoad_Wait. Other errors: CardLoad_ShowError. */
void CardLoad_LoadCharacters(NetMenu *menu)
{
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_LOADING);
    switch (SaveMenu_LoadCharacter((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        menu->pairs[menu->charIndex + 1][0] = menu->charId0;
        menu->pairs[menu->charIndex + 1][1] = menu->charId1;
        D_006ADAAC++;
    next:
        menu->charNo++;
        if (menu->charNo == 3) {
            CardTask_StartCheck();
            if (SaveMenu_FreeCharSlots(menu) == 7) {
                NetMenu_SetState(menu, CARD_LOAD_SHOW_ERROR);
                menu->msg = NETMENU_MSG_NO_CHARA;
                gSysSettings.cardHasChara = 0;
                break;
            }
            menu->timer = 0x1E;
            NetMenu_SetState(menu, CARD_LOAD_WAIT);
            menu->hasSave = 1;
            gSysSettings.cardHasChara = 1;
            Sound_PlaySeFlat(0, 0x19, 0);
            break;
        }
        menu->charIndex = menu->charNo;
        menu->saveStep = 0;
        break;
    case -1:
        switch (menu->result[menu->slot]) {
        case -0xFD:
        case -0xFC:
        case -0xFB:
            goto next;
        case -0xFF:
        default:
            NetMenu_SetState(menu, CARD_LOAD_SHOW_ERROR);
            menu->msg = NETMENU_MSG_BAD_DATA;
            break;
        }
        break;
    }
}

/* Shows menu->msg until a button press, then goes back to CardLoad_ChooseSlot. */
void CardLoad_ShowError(NetMenu *menu)
{
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, menu->msg);
    if (NetMenu_WaitButton(menu, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(menu, CARD_LOAD_CHOOSE_SLOT);
        menu->sub = 0;
    }
}

/* Counts menu->timer down (NetMenu_Update counts it too), then CARD_LOAD_DONE (done 1). */
void CardLoad_Wait(NetMenu *menu)
{
    if (--menu->timer <= 0) {
        NetMenu_SetState(menu, CARD_LOAD_DONE);
    }
}

/* Checks the patch file read. A good one with a patch starts it (Patch_BeginFile) and goes to
 * CardLoad_LoadPatch; anything else goes to CardLoad_LoadCharacters from character 0. */
void CardLoad_CheckPatch(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        if (SaveMenu_ReadBad(menu) != 0) {
        fail:
            menu->timer = 0x78;
            menu->charNo = 0;
            menu->charIndex = menu->charNo;
            menu->saveStep = 0;
            NetMenu_SetState(menu, CARD_LOAD_LOAD_CHARACTERS);
            break;
        }
        if (Patch_FileHasPatch(menu->readBuf) == 0) {
            goto fail;
        }
        menu->timer = 0x78;
        NetMenu_SetState(menu, CARD_LOAD_LOAD_PATCH);
        Patch_BeginFile(menu->readBuf);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
    case CARD_ERR_NO_SPACE:
    case CARD_ERR_NO_CARD:
    default:
        menu->timer = 0x78;
        menu->charNo = 0;
        menu->charIndex = menu->charNo;
        menu->saveStep = 0;
        NetMenu_SetState(menu, CARD_LOAD_LOAD_CHARACTERS);
        break;
    }
}

/* Runs Patch_LoadStep until it ends, installs the patch when it returned 2, then
 * CardLoad_LoadCharacters from character 0. */
void CardLoad_LoadPatch(NetMenu *menu)
{
    s32 patchResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    patchResult = Patch_LoadStep();
    if (patchResult != 0) {
        if (patchResult == 2) {
            Patch_InstallFile(menu->readBuf);
        }
        NetMenu_SetState(menu, CARD_LOAD_LOAD_CHARACTERS);
        menu->charNo = 0;
        menu->charIndex = menu->charNo;
        menu->saveStep = 0;
    }
}

/* NETMENU_MODE_LOAD (Chara_SelectTask): loads the save from a chosen port: the system file, then
 * the patch file (installing a newer patch), then the three character files. done 1 once loaded, 2
 * when the port choice is cancelled. */
void CardMenu_RunLoad(NetMenu *menu)
{
    switch (menu->state) {
    case CARD_LOAD_START:
        CardLoad_Start(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOAD_CHOOSE_SLOT:
        CardLoad_ChooseSlot(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOAD_CHECK_SYSTEM:
        CardLoad_CheckSystem(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOAD_CHECK_PATCH:
        CardLoad_CheckPatch(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOAD_LOAD_PATCH:
        CardLoad_LoadPatch(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOAD_LOAD_CHARACTERS:
        CardLoad_LoadCharacters(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOAD_SHOW_ERROR:
        CardLoad_ShowError(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOAD_WAIT:
        CardLoad_Wait(menu);
        break;
    case CARD_LOAD_DONE_B:
    case CARD_LOAD_DONE:
    default:
        menu->done = 1;
        break;
    case CARD_LOAD_QUIT:
        menu->done = 2;
        break;
    }
}

/* 0x3DB040: more states of the network menu */

s32 NetMenu_YesNo(NetMenu *menu, s32 x, s16 y, u8 *cursor);
s32 NetMenu_CardChanged(s32 slot);
void Patch_InstallFile(u8 *buf);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void CardMenu_InitBuffers(NetMenu *menu);
void SaveMenu_CopyAll(NetMenu *menu);
s32 SaveMenu_SaveSystem(struct SaveMenu *m);
void CardTask_StartDelete(s32 slot);
s32 CardTask_Result(void);
s32 SaveMenu_ReadBad(NetMenu *menu);
void Patch_BeginFile(u8 *buf);
void SysSave_MergeFlags(u8 *buf);
void CardTask_StartCheck(void);
void CardState_SetFlag(s32 a);
s32 SaveMenu_SaveCharacter(struct SaveMenu *m);
s32 SaveMenu_LoadCharacter(struct SaveMenu *m);
void CardTask_StartReadClear(s32 slot, u8 *buf, s32 size);
void CardTask_SelectFile(s32 a, s32 b);
void CardState_SetFlags(s32 a);
void Card_ClearChanged(void);
void SaveMenu_CopySystem(struct SaveMenu *m, u8 *data, s32 save);
void func_6AF3D0(void *a, s32 b);
void Save_CopyToSlot(s32 i);
void SaveMenu_CopyCharacterSlot(NetMenu *menu, s32 i);
extern s8 D_006ADAAE;
extern u8 gOldGameKind;
extern SaveData *gCardSlots;
extern u8 gBootMode;
void *memcpy(void *dst, const void *src, u32 n);
s32 Card_IsChanged(s32 slot);
s32 SaveMenu_SaveExtra(struct SaveMenu *m);
void CardMenu_InitNetBuffers(NetMenu *menu);
void CardSetup_Start(NetMenu *menu);
void CardTask_StartRead(s32 slot, u8 *buf);
void CardTask_StartSave(s32 a, u8 *buf);
void CardTask_Begin(s32 a);
void *memset(void *dst, s32 c, u32 n);
s32 LoadCompressedFile(u8 *dst, s32 id);
s32 CardOp_SetFiles(s32 *data);
s32 Card_PortState(s32 slot);
extern s32 gLoadBuf;

/* Prepares the card image (CardMenu_InitBuffers, SaveMenu_CopyAll), starts reading the system file
 * of the card in use (menu->slot), then CardGame_CheckSystem. */
void CardGame_Start(NetMenu *menu)
{
    NetMenu_SetState(menu, CARD_GAME_CHECK_SYSTEM);
    menu->timer = 0x1E;
    CardMenu_InitBuffers(menu);
    SaveMenu_CopyAll(menu);
    CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 1);
    CardTask_SelectFile(4, 0);
    CardState_SetFlags(1);
}

/* Checks the card in use and merges its system file's flags. When the card was changed since the
 * last access, CardGame_CheckCharacter (slot gQuestSync.charIndex), else CardGame_PrepareSave.
 * Errors: CardGame_ShowError. */
void CardGame_CheckSystem(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_GAME_SAVE_CHECKING);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        if (SaveMenu_ReadBad(menu) == 0) {
            SysSave_MergeFlags(menu->readBuf);
        }
        if (Card_IsChanged(menu->slot) != 0) {
            NetMenu_SetState(menu, CARD_GAME_CHECK_CHARACTER);
            menu->charIndex = gQuestSync.charIndex;
            menu->saveStep = 0;
            break;
        }
        NetMenu_SetState(menu, CARD_GAME_PREPARE_SAVE);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_BAD_DATA:
        NetMenu_SetState(menu, CARD_GAME_SHOW_ERROR);
        menu->msg = NETMENU_MSG_SAVE_BAD_DATA;
        menu->timer = 0x3C;
        break;
    case CARD_ERR_NO_FILE:
    case CARD_ERR_UNFORMATTED:
    case CARD_ERR_NO_SPACE:
    case CARD_ERR_NO_CARD:
    default:
        NetMenu_SetState(menu, CARD_GAME_SHOW_ERROR);
        menu->msg = NETMENU_MSG_GAME_CARD_MISSING;
        menu->timer = 0x3C;
        break;
    }
}

/* Loads the character being checked: on to CardGame_PrepareSave when it is the one expected (the
 * date stamps match), else (or on a card error) CardGame_ShowError with the missing-card error, or
 * the bad-data one for CARD_ERR_BAD_DATA. */
void CardGame_CheckCharacter(NetMenu *menu)
{
    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_GAME_SAVE_CHECKING);
    switch (SaveMenu_LoadCharacter((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        if (SaveMenu_CharIdMatches(menu, menu->charIndex + 1) != 0) {
            NetMenu_SetState(menu, CARD_GAME_PREPARE_SAVE);
            break;
        }
        goto fail;
    case -1:
        switch (menu->result[menu->slot]) {
        case -0xFB:
            NetMenu_SetState(menu, CARD_GAME_SHOW_ERROR);
            menu->msg = NETMENU_MSG_SAVE_BAD_DATA;
            menu->timer = 0x3C;
            break;
        case -0xFD:
        case -0xFE:
        case -0xFC:
        case -0xFF:
        default:
        fail:
            NetMenu_SetState(menu, CARD_GAME_SHOW_ERROR);
            menu->msg = NETMENU_MSG_GAME_CARD_MISSING;
            menu->timer = 0x3C;
            break;
        }
        break;
    }
}

/* Copies the save into character slot gQuestSync.charIndex of the menu's slots and the card image,
 * shows the in-game saving notice and goes to CardGame_SaveCharacter. The slot's address is
 * computed as an integer: the match needs that form. */
void CardGame_PrepareSave(NetMenu *menu)
{
    s32 i;

    NetMenu_SetState(menu, CARD_GAME_SAVE_CHARACTER);
    i = gQuestSync.charIndex;
    memcpy((void *)((s32)menu->slots + i * (s32)sizeof(SaveData)), &gSave, sizeof(SaveData)); /* an integer sum: &m->slots[i] changes the registers */
    gSysSave.charSaved = 3;
    SaveMenu_CopyCharacterSlot(menu, i);
    menu->charIndex = i;
    menu->saveStep = 0;
    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_SAVING_TO_SLOT);
}

/* Writes the character file. Done: hasSave and cardHasChara set, then CardGame_SaveSystem. Failed:
 * CardGame_ShowError. */
void CardGame_SaveCharacter(NetMenu *menu)
{
    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_SAVING_TO_SLOT);
    switch (SaveMenu_SaveCharacter((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        menu->hasSave = 1;
        menu->saveStep = 0;
        gSysSettings.cardHasChara = 1;
        NetMenu_SetState(menu, CARD_GAME_SAVE_SYSTEM);
        break;
    case -1:
        NetMenu_SetState(menu, CARD_GAME_SHOW_ERROR);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        menu->timer = 0x3C;
        break;
    }
}

/* CardGame_SaveCharacter for NETMENU_MODE_GAME_SAVE_EXTRA: done goes to CardGame_SaveExtra. */
void CardGame_SaveCharacterExtra(NetMenu *menu)
{
    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_SAVING_TO_SLOT);
    switch (SaveMenu_SaveCharacter((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        menu->hasSave = 1;
        menu->saveStep = 0;
        gSysSettings.cardHasChara = 1;
        NetMenu_SetState(menu, CARD_GAME_SAVE_EXTRA);
        break;
    case -1:
        NetMenu_SetState(menu, CARD_GAME_SHOW_ERROR);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        menu->timer = 0x3C;
        break;
    }
}

/* Writes the system file. Done: clears the changed-card bits, the success sound,
 * gSysSettings.systemSaved, then CARD_GAME_DONE. Failed: CardGame_ShowError. */
void CardGame_SaveSystem(NetMenu *menu)
{
    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_SAVING_TO_SLOT);
    switch (SaveMenu_SaveSystem((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        Card_ClearChanged();
        NetMenu_SetState(menu, CARD_GAME_DONE);
        Sound_PlaySeFlat(0, 0x19, 0);
        gSysSettings.systemSaved = 1;
        break;
    case -1:
        NetMenu_SetState(menu, CARD_GAME_SHOW_ERROR);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        menu->timer = 0x3C;
        break;
    }
}

/* Unused (no state sets CARD_GAME_SHOW_RESULT). Shows menu->msg until a button press, then
 * CARD_GAME_DONE. */
void CardGame_ShowResult(NetMenu *menu)
{
    s32 msg = menu->msg;
    NetMenu_DrawMessage(0x64, 0x88, msg);
    if (NetMenu_WaitButton(menu, 0x1F4, 0xF4, 0) != 0) {
        NetMenu_SetState(menu, CARD_GAME_DONE);
    }
}

/* Shows menu->msg until a button press, then CardGame_AskRetry, with the cursor on yes in
 * NETMENU_MODE_GAME_SAVE and on no in the other modes. */
void CardGame_ShowError(NetMenu *menu)
{
    NetMenu_DrawMessage(0x64, 0x88, menu->msg);
    if (NetMenu_WaitButton(menu, 0x1F4, 0xF4, 1) != 0) {
        if (menu->mode == NETMENU_MODE_GAME_SAVE) {
            NetMenu_SetState(menu, CARD_GAME_ASK_RETRY);
            menu->cursor = 0;
            return;
        }
        NetMenu_SetState(menu, CARD_GAME_ASK_RETRY);
        menu->cursor = 1;
    }
}

/* After an error. NETMENU_MODE_GAME_SAVE asks whether to save the game so far: yes starts again
 * from CardGame_CheckSystem; no goes to CARD_GAME_QUIT, or to CardGame_AskQuit when openMode is
 * NETMENU_MODE_SAVE_B (its test for NETMENU_MODE_GAME_SAVE_EXTRA there can't hold). The other modes
 * ask whether to go on without saving. */
void CardGame_AskRetry(NetMenu *menu)
{
    s32 mode = menu->mode;
    s16 lastY;

    if (mode == 10) {
        switch (NetMenu_YesNo(menu, 0xCB, NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_ASK_SAVE_PROGRESS) + 0x24, &menu->cursor)) {
        case NETMENU_NO:
            if (menu->openMode != 0x10) {
                NetMenu_SetState(menu, CARD_GAME_QUIT);
                break;
            }
            if (menu->mode == NETMENU_MODE_GAME_SAVE_EXTRA) {
                NetMenu_SetState(menu, CARD_GAME_QUIT);
                break;
            }
            NetMenu_SetState(menu, CARD_GAME_ASK_QUIT);
            break;
        case NETMENU_YES:
        load:
            NetMenu_SetState(menu, CARD_GAME_CHECK_SYSTEM);
            CardMenu_InitBuffers(menu);
            SaveMenu_CopyAll(menu);
            CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 1);
            CardTask_SelectFile(4, 0);
            CardState_SetFlags(1);
            break;
        }
    } else {
        if (mode == 6 || mode == 7 || mode == 0x11) {
            lastY = NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_ASK_CONTINUE_UNSAVED);
        } else {
            lastY = NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_ASK_QUIT_UNSAVED);
        }
        switch (NetMenu_YesNo(menu, 0xCB, lastY + 0x24, &menu->cursor)) {
        case NETMENU_NO:
            goto load;
        case NETMENU_YES:
            NetMenu_SetState(menu, CARD_GAME_QUIT);
            break;
        }
    }
}

extern u8 *gSubMenuWork;

/* Asks whether to quit to the menu without saving (NETMENU_MODE_SAVE_B's path): yes sets gBootMode
 * 1, clears byte 0xA of gSubMenuWork and ends with CARD_GAME_QUIT; no goes back to
 * CardGame_AskRetry. The menu work is read through its name: through its raw address the constant 1
 * for gBootMode went to another register than the game's. */
void CardGame_AskQuit(NetMenu *menu)
{
    u8 *work;

    switch (NetMenu_YesNo(menu, 0xCB, NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_ASK_QUIT_TO_MENU) + 0x24, &menu->cursor)) {
    case NETMENU_NO:
        NetMenu_SetState(menu, CARD_GAME_ASK_RETRY);
        break;
    case NETMENU_YES:
        work = gSubMenuWork;
        gBootMode = 1;
        work[0xA] = 0;
        NetMenu_SetState(menu, CARD_GAME_QUIT);
        break;
    }
}

/* Writes the extra file (SaveMenu_SaveExtra, file 8). Done: CardGame_SaveSystem. Failed:
 * CardGame_ShowError. */
void CardGame_SaveExtra(NetMenu *menu)
{
    NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_SAVING_TO_SLOT);
    switch (SaveMenu_SaveExtra((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        Card_ClearChanged();
        menu->saveStep = 0;
        NetMenu_SetState(menu, CARD_GAME_SAVE_SYSTEM);
        break;
    case -1:
        NetMenu_SetState(menu, CARD_GAME_SHOW_ERROR);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        menu->timer = 0x3C;
        break;
    }
}

extern u8 gSysBusy;

/* NETMENU_MODE_GAME_SAVE, which CardSave_Start and CardSave_AskSave switch to when a card is in use
 * (hasSave): checks the card the game uses again, then saves the character and the system data to
 * it. gSysBusy is 1 in the card states and 0 at the prompts. done 1 once saved, 2 when the player
 * gives up. */
void CardMenu_RunGameSave(NetMenu *menu)
{
    switch (menu->state) {
    case CARD_GAME_START:
        CardGame_Start(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_CHECK_SYSTEM:
        CardGame_CheckSystem(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_CHECK_CHARACTER:
        CardGame_CheckCharacter(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_PREPARE_SAVE:
        CardGame_PrepareSave(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_SAVE_CHARACTER:
        CardGame_SaveCharacter(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_SAVE_SYSTEM:
        CardGame_SaveSystem(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_SHOW_RESULT:
        CardGame_ShowResult(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_SHOW_ERROR:
        CardGame_ShowError(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_ASK_RETRY:
        CardGame_AskRetry(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_ASK_QUIT:
        CardGame_AskQuit(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_DONE:
    default:
        gSysBusy = 0;
        menu->done = 1;
        break;
    case CARD_GAME_QUIT:
        gSysBusy = 0;
        menu->done = 2;
        break;
    }
}

void NetMode_Begin(DrawSlot *ds);
s32 LobbyCard_SaveNetFile(void);

/* The online in-game save (NETMENU_MODE_GAME_SAVE_TOWN, _ENTRY and _LOGOUT): CardMenu_RunGameSave's
 * states, but the end states keep 1 or 2 in quitResult, call NetMode_Begin and go to
 * CARD_GAME_SAVE_EXTRA, which runs LobbyCard_SaveNetFile until it returns 1 or -1 and then sets
 * done. */
void CardMenu_RunGameSaveNet(NetMenu *menu)
{
    switch (menu->state) {
    case CARD_GAME_START:
        CardGame_Start(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_CHECK_SYSTEM:
        CardGame_CheckSystem(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_CHECK_CHARACTER:
        CardGame_CheckCharacter(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_PREPARE_SAVE:
        CardGame_PrepareSave(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_SAVE_CHARACTER:
        CardGame_SaveCharacter(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_SAVE_SYSTEM:
        CardGame_SaveSystem(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_SHOW_RESULT:
        CardGame_ShowResult(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_SHOW_ERROR:
        CardGame_ShowError(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_ASK_RETRY:
        CardGame_AskRetry(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_ASK_QUIT:
        CardGame_AskQuit(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_DONE:
    default:
        menu->quitResult = 1;
        goto quit;
    case CARD_GAME_QUIT:
        menu->quitResult = 2;
    quit:
        NetMenu_SetState(menu, CARD_GAME_SAVE_EXTRA);
        NetMode_Begin((DrawSlot *)gNetMenuDrawHooks);
        break;
    case CARD_GAME_SAVE_EXTRA:
        switch (LobbyCard_SaveNetFile()) {
        case 0:
            break;
        case 1:
        case -1:
            gSysBusy = 0;
            menu->done = menu->quitResult;
            break;
        }
        break;
    }
}

/* NETMENU_MODE_GAME_SAVE_EXTRA (LobbyNetXfer_Run): CardMenu_RunGameSave, also writing the extra
 * file: CardGame_SaveCharacterExtra, CardGame_SaveExtra, then CardGame_SaveSystem. */
void CardMenu_RunGameSaveExtra(NetMenu *menu)
{
    switch (menu->state) {
    case CARD_GAME_START:
        CardGame_Start(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_CHECK_SYSTEM:
        CardGame_CheckSystem(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_CHECK_CHARACTER:
        CardGame_CheckCharacter(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_PREPARE_SAVE:
        CardGame_PrepareSave(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_SAVE_CHARACTER:
        CardGame_SaveCharacterExtra(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_SAVE_SYSTEM:
        CardGame_SaveSystem(menu);
        gSysBusy = 1;
        break;
    case CARD_GAME_SHOW_RESULT:
        CardGame_ShowResult(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_SHOW_ERROR:
        CardGame_ShowError(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_ASK_RETRY:
        CardGame_AskRetry(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_ASK_QUIT:
        CardGame_AskQuit(menu);
        gSysBusy = 0;
        break;
    case CARD_GAME_DONE:
    default:
        gSysBusy = 0;
        menu->done = 1;
        break;
    case CARD_GAME_QUIT:
        gSysBusy = 0;
        menu->done = 2;
        break;
    case CARD_GAME_SAVE_EXTRA:
        CardGame_SaveExtra(menu);
        gSysBusy = 1;
        break;
    }
}

/* NETMENU_MODE_SAVE_ASK goes to CardSave_AskSave first. Otherwise, with no card in use (hasSave 0),
 * CardSave_ChooseSlot; with one, the menu switches to NETMENU_MODE_GAME_SAVE. */
void CardSave_Start(NetMenu *menu)
{
    if (menu->mode == NETMENU_MODE_SAVE_ASK) {
        NetMenu_SetState(menu, CARD_SAVE_ASK_SAVE);
        menu->cursor = 0;
        if (menu->hasSave == 0) {
            menu->slot = 0;
        }
    } else if (menu->hasSave == 0) {
        NetMenu_SetState(menu, CARD_SAVE_CHOOSE_SLOT);
        menu->cursor = 0;
        menu->slot = 0;
    } else {
        menu->mode = 10;
        menu->state = 0;
    }
}

/* Asks whether to save. Yes: CardSave_ChooseSlot, or NETMENU_MODE_GAME_SAVE when a card is in use
 * (hasSave). No: CARD_SAVE_QUIT (done 2). */
void CardSave_AskSave(NetMenu *menu)
{
    switch (NetMenu_YesNo(menu, 0x52, NetMenu_DrawMessage(0x64, 0x88, NETMENU_MSG_ASK_SAVE) + 0x24, &menu->cursor)) {
    case NETMENU_NO:
        NetMenu_SetState(menu, CARD_SAVE_QUIT);
        break;
    case NETMENU_YES:
        if (menu->hasSave == 0) {
            NetMenu_SetState(menu, CARD_SAVE_CHOOSE_SLOT);
            menu->cursor = 0;
            break;
        }
        menu->mode = 0xA;
        menu->state = 0;
        break;
    }
}

/* Picks the port (cross: CardSave_AskSave). After the pick it prepares the buffers and the card
 * image, starts reading the system file and goes to CardSave_CheckSystem. */
void CardSave_ChooseSlot(NetMenu *menu)
{
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHOOSE_SAVE_SLOT);
    switch (menu->sub) {
    case 0:
        switch (NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 0)) {
        case 0:
        case 1:
            menu->sub++;
            menu->timer = 4;
            break;
        case NETMENU_SLOT_CANCEL:
            NetMenu_SetState(menu, CARD_SAVE_ASK_SAVE);
            menu->cursor = 0;
            break;
        }
        break;
    case 1:
        NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
        if (--menu->timer <= 0) {
            NetMenu_SetState(menu, CARD_SAVE_CHECK_SYSTEM);
            menu->timer = 0x1E;
            CardMenu_InitBuffers(menu);
            CardMenu_InitLoadBuffers(menu);
            SaveMenu_CopyAll(menu);
            CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 1);
            CardTask_SelectFile(4, 0);
            CardState_SetFlags(1);
        }
        break;
    }
}

/* Checks the system file read. A good one (its flags merged) or none (CARD_ERR_NO_FILE) starts
 * reading character file 0 and goes to CardSave_ReadCharacters. Unformatted:
 * CardSave_ChooseCharSlot with charLoaded 0. Other errors: CardSave_ShowError. */
void CardSave_CheckSystem(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        if (SaveMenu_ReadBad(menu) == 0) {
            SysSave_MergeFlags(menu->readBuf);
        }
        menu->timer = 0x1E;
    start:
        Card_ClearChanged();
        NetMenu_SetState(menu, CARD_SAVE_READ_CHARACTERS);
        menu->charNo = 0;
        menu->fileKind = 2 << menu->charNo;
        CardTask_StartRead(menu->slot, (u8 *)menu->readBuf);
        CardTask_SelectFile(menu->charNo + 5, 0);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
        menu->charLoaded = 0;
        goto start;
    case CARD_ERR_UNFORMATTED:
        NetMenu_SetState(menu, CARD_SAVE_CHOOSE_CHAR_SLOT);
        menu->charLoaded = 0;
        gChrEdit.slot = 0;
        Card_ClearChanged();
        CardTask_StartCheck();
        break;
    case CARD_ERR_NO_SPACE:
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_SPACE;
        break;
    case CARD_ERR_NO_CARD:
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_CARD;
        break;
    default:
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_BAD_DATA;
        break;
    }
}

/* Reads the three characters' save files one after another (menu->charNo): a good one is copied
 * into its SaveData (a bad one cleared), then the next is read; after the third,
 * CardSave_ChooseCharSlot. */
void CardSave_ReadCharacters(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        {
            u8 *buf = menu->readBuf;
            u8 *save = (u8 *)&menu->slots[menu->charNo];

            if (SaveMenu_ReadBad(menu) != 0) {
                menu->charRead[menu->charNo] = 0x80;
                memset(save, 0, 0x37F8);
            } else {
                menu->charRead[menu->charNo] = 1;
                memcpy(save, buf + 0x50, 0x37F8);
            }
            menu->charLoaded = 2;
        }
        goto next;
    case CARD_BUSY:
        break;
    case -0xFF:
        goto err; /* the compare chain wants this label before -0xFD */
    case -0xFD:
    case -0xFC:
    next:
        menu->charNo++;
        if (menu->charNo == 3) {
            menu->charNo = 0;
            CardTask_StartCheck();
            NetMenu_SetState(menu, CARD_SAVE_CHOOSE_CHAR_SLOT);
            menu->cursor = 1;
            gChrEdit.slot = 0;
            break;
        }
        menu->fileKind = 2 << menu->charNo;
        CardTask_StartRead(menu->slot, (u8 *)menu->readBuf);
        CardTask_SelectFile(menu->charNo + 5, 0);
        break;
    err:
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_CARD;
        break;
    default:
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_BAD_DATA;
        break;
    }
}

/* Picks one of the three character slots (gChrEdit.slot, up and down). Confirm goes to
 * CardSave_ConfirmCharSlot with the prompt for an empty or used slot (or for a card without a save,
 * when the files were not read); cross goes back to CardSave_ChooseSlot; a changed card to
 * CardSave_ShowError. */
void CardSave_ChooseCharSlot(NetMenu *menu)
{
    u8 *saves;

    if (menu->buttons & PAD_UP) {
        Sound_PlaySeFlat(0, SE_SYS_CURSOR, 0);
        gChrEdit.slot--;
        if ((s8)gChrEdit.slot < 0) {
            gChrEdit.slot = 2;
        }
    } else if (menu->buttons & PAD_DOWN) {
        Sound_PlaySeFlat(0, SE_SYS_CURSOR, 0);
        gChrEdit.slot++;
        if (gChrEdit.slot > 2) {
            gChrEdit.slot = 0;
        }
    }
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHOOSE_CHARA_SLOT);
    if (NetMenu_CardChanged(menu->slot) != 0) {
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_CARD_CHANGED;
        return;
    }
    if (menu->buttons & PAD_CIRCLE) {
        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
        NetMenu_SetState(menu, CARD_SAVE_CONFIRM_CHAR_SLOT);
        menu->charIndex = gChrEdit.slot;
        menu->cursor = 1;
        saves = (u8 *)menu->slots;
        if (menu->charLoaded != 0) {
            if (saves[gChrEdit.slot * 0x37F8] == 0) {
                menu->msg = NETMENU_MSG_ASK_SAVE_HERE;
            } else {
                menu->msg = NETMENU_MSG_ASK_OVERWRITE_CHARA;
            }
        } else {
            menu->msg = NETMENU_MSG_ASK_CREATE_DATA;
        }
    } else if (menu->buttons & PAD_CROSS) {
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        NetMenu_SetState(menu, CARD_SAVE_CHOOSE_SLOT);
    }
    menu->layers[1] = 6;
    CardMenu_DrawCharacters(&gChrEdit, 2, 0, 0, (s8)gChrEdit.slot);
}

/* Shows menu->msg over a yes/no box. Yes: CardSave_PrepareSave, or CardSave_AskFormat (the cursor
 * on no) for an unformatted card, or CardSave_ShowError on other errors. No: back to
 * CardSave_ChooseCharSlot. A changed card: CardSave_ShowError. */
void CardSave_ConfirmCharSlot(NetMenu *menu)
{
    s16 lastY;

    lastY = NetMenu_DrawMessage(0xDD, 0x88, menu->msg);
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHOOSE_CHARA_SLOT);
    if (NetMenu_CardChanged(menu->slot) != 0) {
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_CARD_CHANGED;
        return;
    }
    switch (NetMenu_YesNo(menu, 0xCB, lastY + 0x24, &menu->cursor)) {
    case NETMENU_NO:
        NetMenu_SetState(menu, CARD_SAVE_CHOOSE_CHAR_SLOT);
        break;
    case NETMENU_YES:
        switch (menu->result[menu->slot]) {
        case 0:
        case -0xFD:
            NetMenu_SetState(menu, CARD_SAVE_PREPARE_SAVE);
            break;
        case -0xFE:
            NetMenu_SetState(menu, CARD_SAVE_ASK_FORMAT);
            menu->cursor = 1;
            break;
        default:
            NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
            menu->msg = NETMENU_MSG_SAVE_FAILED;
            break;
        }
        break;
    }
}

/* Asks to format the card. Yes: starts the format, then CardSave_Format. No: CardSave_ChooseSlot. A
 * changed card: CardSave_ShowError. */
void CardSave_AskFormat(NetMenu *menu)
{
    s16 lastY = NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_ASK_FORMAT);

    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    if (NetMenu_CardChanged(menu->slot) != 0) {
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_CARD_CHANGED;
        return;
    }
    switch (NetMenu_YesNo(menu, 0x16, lastY + 0x12, &menu->cursor)) {
    case NETMENU_NO:
        NetMenu_SetState(menu, CARD_SAVE_CHOOSE_SLOT);
        break;
    case NETMENU_YES:
        NetMenu_SetState(menu, CARD_SAVE_FORMAT);
        menu->timer = 0x1E;
        CardTask_StartDelete(menu->slot);
        break;
    }
}

/* Waits for the format. Done: CardSave_PrepareSave. Failed: CardSave_ShowError. */
void CardSave_Format(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_FORMATTING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        NetMenu_SetState(menu, CARD_SAVE_PREPARE_SAVE);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_FAILED:
    default:
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_FORMAT_FAILED;
        break;
    }
}

/* Loads the save icons (file 0x1714) and copies gSave into slots[gChrEdit.slot] and the card image;
 * records the slot in gSysSave.lastSlot and gQuestSync.charIndex, sets charSaved and fileKind, then
 * CardSave_SaveCharacter. */
void CardSave_PrepareSave(NetMenu *menu)
{
    s32 i;

    NetMenu_SetState(menu, CARD_SAVE_SAVE_CHARACTER);
    menu->timer = 0x1E;
    LoadCompressedFile(menu->iconBuf, 0x1714);
    gCardSlots = menu->slots;
    i = gChrEdit.slot;
    menu->charIndex = i;
    memcpy((void *)((s32)menu->slots + i * (s32)sizeof(SaveData)), &gSave, sizeof(SaveData)); /* an integer sum: &m->slots[i] changes the registers */
    gSysSave.charSaved = 3;
    gSysSave.lastSlot = gChrEdit.slot;
    gQuestSync.charIndex = gChrEdit.slot;
    SaveMenu_CopyCharacterSlot(menu, i);
    menu->fileKind = 2 << i;
    menu->saveStep = 0;
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
}

/* Writes the character file. Done: hasSave and cardHasChara set, then CardSave_SaveSystem. Failed:
 * CardSave_ShowError. */
void CardSave_SaveCharacter(NetMenu *menu)
{
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    switch (SaveMenu_SaveCharacter((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        NetMenu_SetState(menu, CARD_SAVE_SAVE_SYSTEM);
        menu->hasSave = 1;
        menu->saveStep = 0;
        gSysSettings.cardHasChara = 1;
        break;
    case -1:
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        break;
    }
}

/* Writes the system file. Done: clears the changed-card bits, the success sound,
 * gSysSettings.systemSaved, then CARD_SAVE_DONE (done 1). Failed: CardSave_ShowError. */
void CardSave_SaveSystem(NetMenu *menu)
{
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    switch (SaveMenu_SaveSystem((struct SaveMenu *)menu)) {
    case 0:
        break;
    case 1:
        Card_ClearChanged();
        NetMenu_SetState(menu, CARD_SAVE_DONE);
        Sound_PlaySeFlat(0, 0x19, 0);
        gSysSettings.systemSaved = 1;
        break;
    case -1:
        NetMenu_SetState(menu, CARD_SAVE_SHOW_ERROR);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        break;
    }
}

/* Unused (no state sets CARD_SAVE_SHOW_SAVED). Shows the saved notice until a button press, then
 * CARD_SAVE_DONE. */
void CardSave_ShowSaved(NetMenu *menu)
{
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVED_KEEP_CARD);
    if (NetMenu_WaitButton(menu, 0x232, 0x184, 0) != 0) {
        NetMenu_SetState(menu, CARD_SAVE_DONE);
    }
}

/* Shows menu->msg until a button press, then goes back to CardSave_ChooseSlot. */
void CardSave_ShowError(NetMenu *menu)
{
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, menu->msg);
    if (NetMenu_WaitButton(menu, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(menu, CARD_SAVE_CHOOSE_SLOT);
    }
}

/* NETMENU_MODE_SAVE and NETMENU_MODE_SAVE_ASK (the town offline, LobbyScreen_LoadBanks): saves the
 * character to a chosen port and character slot after reading the card's system and character
 * files, formatting when needed; with a card already in use it switches to NETMENU_MODE_GAME_SAVE.
 * done 1 once saved, 2 when declined. */
void CardMenu_RunSave(NetMenu *menu)
{
    switch (menu->state) {
    case CARD_SAVE_START:
        CardSave_Start(menu);
        break;
    case CARD_SAVE_ASK_SAVE:
        CardSave_AskSave(menu);
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_CHOOSE_SLOT:
        CardSave_ChooseSlot(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_CHECK_SYSTEM:
        CardSave_CheckSystem(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_READ_CHARACTERS:
        CardSave_ReadCharacters(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_CHOOSE_CHAR_SLOT:
        CardSave_ChooseCharSlot(menu);
        menu->layers[0] = 4;
        break;
    case CARD_SAVE_CONFIRM_CHAR_SLOT:
        CardSave_ConfirmCharSlot(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_ASK_FORMAT:
        CardSave_AskFormat(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_FORMAT:
        CardSave_Format(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_PREPARE_SAVE:
        CardSave_PrepareSave(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_SAVE_CHARACTER:
        CardSave_SaveCharacter(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_SAVE_SYSTEM:
        CardSave_SaveSystem(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_SHOW_SAVED:
        CardSave_ShowSaved(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_SHOW_ERROR:
        CardSave_ShowError(menu);
        menu->layers[0] = 4;
        menu->layers[1] = 3;
        break;
    case CARD_SAVE_DONE:
        menu->done = 1;
        break;
    case CARD_SAVE_QUIT:
    default:
        menu->done = 2;
        break;
    }
}

/* Sets up the card buffers, starts reading the system file of port 0, then CardSetup_Read. */
void CardSetup_Start(NetMenu *menu)
{
    NetMenu_SetState(menu, CARD_SETUP_READ);
    menu->slot = 0;
    CardMenu_InitBuffers(menu);
    CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 0);
}

/* Waits for the read. Done: writes the current system data into the file read (SysSave_Exchange),
 * encrypts gLoadBuf, starts saving it and goes to CardSetup_Save. An error: CardSetup_ShowResult
 * with the failure. */
void CardSetup_Read(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        NetMenu_SetState(menu, CARD_SETUP_SAVE);
        SysSave_Exchange(1, menu->readBuf);
        SaveFile_Encrypt((u8 *)gLoadBuf);
        CardTask_StartSave(menu->slot, (u8 *) gLoadBuf);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
    case CARD_ERR_UNFORMATTED:
    case CARD_ERR_NO_SPACE:
    case CARD_ERR_NO_CARD:
    default:
        NetMenu_SetState(menu, CARD_SETUP_SHOW_RESULT);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        menu->timer = 0x78;
        break;
    }
}

/* Waits for the save, then CardSetup_ShowResult with the saved notice or the failure. */
void CardSetup_Save(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_SAVING);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        NetMenu_SetState(menu, CARD_SETUP_SHOW_RESULT);
        menu->msg = NETMENU_MSG_SAVED;
        menu->timer = 0x78;
        break;
    case CARD_BUSY:
        break;
    default:
        NetMenu_SetState(menu, CARD_SETUP_SHOW_RESULT);
        menu->msg = NETMENU_MSG_SAVE_FAILED;
        menu->timer = 0x78;
        break;
    }
}

/* Shows menu->msg until a button press or the timer's end, then CARD_SETUP_DONE. */
void CardSetup_ShowResult(NetMenu *menu)
{
    s32 msg = menu->msg;
    NetMenu_DrawMessage(0x28, 0x160, msg);
    if (NetMenu_WaitButton(menu, 0x232, 0x184, 0) != 0 || menu->timer <= 0) {
        NetMenu_SetState(menu, CARD_SETUP_DONE);
    }
}

/* NETMENU_MODE_SETUP: rewrites the system file of port 0 with the current system data
 * (CardSetup_*); no NetMenu_Open caller is seen. Its draw hooks are not queued. */
void CardMenu_RunSetup(NetMenu *menu)
{
    switch (menu->state) {
    case CARD_SETUP_START:
        CardSetup_Start(menu);
        menu->layers[1] = 2;
        break;
    case CARD_SETUP_READ:
        CardSetup_Read(menu);
        break;
    case CARD_SETUP_SAVE:
        CardSetup_Save(menu);
        break;
    case CARD_SETUP_SHOW_RESULT:
        CardSetup_ShowResult(menu);
        break;
    case CARD_SETUP_DONE:
    default:
        menu->done = 1;
        break;
    }
}

/* CardMenu_InitBuffers for the NET file: card operation 1, the NET file's icons (0x1715) and the
 * menu's buffers in gLoadBuf. CardLogin_ChooseSlot passes menu, already in a0. */
void CardMenu_InitNetBuffers(NetMenu *menu)
{
    s32 buf;

    CardTask_Begin(1);
    buf = gLoadBuf;
    memset((void *)buf, 0, 0x6AFE8);
    menu->iconBuf = (u8 *)buf;
    menu->imageBuf = buf + 0x20000;
    menu->readBuf = (u8 *)(buf + 0x40400);
    LoadCompressedFile(menu->iconBuf, 0x1715);
    menu->fileBlocks = CardOp_SetFiles((s32 *)menu->iconBuf);
}

/* Sets port 0, then CardLogin_ChooseSlot. */
void CardLogin_Start(NetMenu *menu)
{
    NetMenu_SetState(menu, CARD_LOGIN_CHOOSE_SLOT);
    menu->slot = 0;
}

/* Picks the port holding the NET file (cross: CARD_LOGIN_QUIT, done 2). After the pick it sets up
 * the NET file's buffers, starts the read and goes to CardLogin_ReadNetFile. */
void CardLogin_ChooseSlot(NetMenu *menu)
{
    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHOOSE_NET_SLOT);
    switch (menu->sub) {
    case 0:
        switch (NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 0)) {
        case 0:
        case 1:
            menu->sub++;
            menu->timer = 4;
            break;
        case NETMENU_SLOT_CANCEL:
            NetMenu_SetState(menu, CARD_LOGIN_QUIT);
            break;
        }
        break;
    case 1:
        NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
        if (menu->timer <= 0) {
            NetMenu_SetState(menu, CARD_LOGIN_READ_NET_FILE);
            menu->timer = 0x1E;
            CardMenu_InitNetBuffers(menu);
            CardTask_StartReadClear(menu->slot, (u8 *)menu->readBuf, 1);
        }
        break;
    }
}

/* Checks the NET file read. With a login id saved (readBuf + 0x960 nonzero): CardLogin_AskDelete
 * with the cursor on no; without one, CardLogin_ShowError. Errors (no file, no card, others) go to
 * CardLogin_ShowError too. */
void CardLogin_ReadNetFile(NetMenu *menu)
{
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, NETMENU_MSG_CHECKING);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        Card_ClearChanged();
        CardTask_StartCheck();
        if (((s8 *)menu->readBuf)[0x960] == 0) {
            NetMenu_SetState(menu, CARD_LOGIN_SHOW_ERROR);
            menu->msg = NETMENU_MSG_NO_LOGIN_ID;
        } else {
            NetMenu_SetState(menu, CARD_LOGIN_ASK_DELETE);
            menu->msg = NETMENU_MSG_ASK_DELETE_LOGIN_ID;
            menu->cursor = 1;
        }
        menu->charNo = 0;
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
        NetMenu_SetState(menu, CARD_LOGIN_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_NET_FILE;
        break;
    case CARD_ERR_UNFORMATTED:
        NetMenu_SetState(menu, CARD_LOGIN_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_NET_FILE;
        break;
    case CARD_ERR_NO_SPACE:
        NetMenu_SetState(menu, CARD_LOGIN_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NO_NET_FILE;
        break;
    case CARD_ERR_NO_CARD:
        NetMenu_SetState(menu, CARD_LOGIN_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NET_NO_CARD;
        break;
    default:
        NetMenu_SetState(menu, CARD_LOGIN_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NET_LOAD_FAILED;
        break;
    }
}

/* Asks to delete the login id. Yes: clears the id (readBuf + 0x960, 11 bytes) and the password
 * (+0x96B, 9 bytes), starts saving the file and goes to CardLogin_Save. No: back to
 * CardLogin_Start. A removed card: CardLogin_ShowError. */
void CardLogin_AskDelete(NetMenu *menu)
{
    u8 *netFile;

    if (Card_PortState(menu->slot) == 0) {
        NetMenu_SetState(menu, CARD_LOGIN_SHOW_ERROR);
        menu->msg = NETMENU_MSG_NET_CARD_REMOVED;
        return;
    }
    switch (NetMenu_YesNo(menu, 0x52, NetMenu_DrawMessage(0x64, 0x88, menu->msg) + 0x24, &menu->cursor)) {
    case NETMENU_NO:
        menu->state = 0;
        break;
    case NETMENU_YES:
        NetMenu_SetState(menu, CARD_LOGIN_SAVE);
        menu->msg = NETMENU_MSG_SAVING;
        netFile = menu->readBuf;
        memset(netFile + 0x960, 0, 0xB);
        memset(netFile + 0x96B, 0, 9);
        CardTask_StartSave(menu->slot, menu->readBuf);
        break;
    }
}

/* Waits for the NET file's save. Done: CARD_LOGIN_DONE (done 1). Failed: CardLogin_ShowResult. */
void CardLogin_Save(NetMenu *menu)
{
    s32 msg = menu->msg;
    s32 cardResult;

    NetMenu_DrawMessage(0x28, 0x160, msg);
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    cardResult = CardTask_Result();
    menu->result[menu->slot] = cardResult;
    switch (cardResult) {
    case CARD_OK:
        NetMenu_SetState(menu, CARD_LOGIN_DONE);
        break;
    case CARD_BUSY:
        break;
    case CARD_ERR_NO_FILE:
    case CARD_ERR_NO_SPACE:
    case CARD_ERR_NO_CARD:
    default:
        NetMenu_SetState(menu, CARD_LOGIN_SHOW_RESULT);
        menu->msg = NETMENU_MSG_NET_SAVE_FAILED;
        break;
    }
}

/* Shows menu->msg until a button press, then goes back to CardLogin_ChooseSlot. */
void CardLogin_ShowError(NetMenu *menu)
{
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, menu->msg);
    if (NetMenu_WaitButton(menu, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(menu, CARD_LOGIN_CHOOSE_SLOT);
    }
}

/* Shows menu->msg (the failed save) until a button press, then CARD_LOGIN_QUIT. */
void CardLogin_ShowResult(NetMenu *menu)
{
    NetMenu_ChooseSlot(menu, 0xDD, 0x88, (u8 *)&menu->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, menu->msg);
    if (NetMenu_WaitButton(menu, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(menu, CARD_LOGIN_QUIT);
    }
}

/* NETMENU_MODE_DELETE_LOGIN_ID (NetOption_Update): deletes the saved login id and password from the
 * NET file on a chosen card. done 1 once deleted, 2 when cancelled at the port choice or after a
 * failed save. */
void CardMenu_RunDeleteLoginId(NetMenu *menu)
{
    switch (menu->state) {
    case CARD_LOGIN_START:
        CardLogin_Start(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOGIN_CHOOSE_SLOT:
        CardLogin_ChooseSlot(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOGIN_READ_NET_FILE:
        CardLogin_ReadNetFile(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOGIN_ASK_DELETE:
        CardLogin_AskDelete(menu);
        menu->layers[0] = 0;
        menu->layers[1] = 0;
        break;
    case CARD_LOGIN_SAVE:
        CardLogin_Save(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOGIN_SHOW_ERROR:
        CardLogin_ShowError(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOGIN_SHOW_RESULT:
        CardLogin_ShowResult(menu);
        menu->layers[0] = 1;
        menu->layers[1] = 2;
        break;
    case CARD_LOGIN_DONE:
    default:
        menu->done = 1;
        break;
    case CARD_LOGIN_QUIT:
        menu->done = 2;
        break;
    }
}

void Text_SetSize(s32 size);
void CardTask_Update(void);
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
s32 Game_IsOnline(void);
void LobbyWorld_HoldClock(void);
void CardMenu_RunSetup(NetMenu *menu);

/* Runs one frame of the card menu: reads the pad, counts menu->timer down, runs the state machine
 * of menu->mode (CardMenu_Run*; the modes without one end at once), queues the draw hooks (not in
 * NETMENU_MODE_SETUP), then CardTask_Update. Sets gSysSettings.cardMenuDone when done. Returns
 * menu->done: 0 running, 1 done, 2 left without saving. */
u8 NetMenu_Update(void)
{
    NetMenu *menu = &gCardMenu;

    Text_SetSize(4);
    *(s32 *)(menu->layers) = 0;
    menu->showCursor = 0;
    menu->showButton = 0;
    menu->buttons = gPadState[0].pressed;
    menu->held = gPadState[0].held;
    if (menu->timer > 0) {
        menu->timer--;
    }
    switch (menu->mode) {
    case NETMENU_MODE_SAVE_NEW:
        CardMenu_RunSaveNew(menu);
        menu->layers[0] = 1;
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case NETMENU_MODE_BOOT:
        CardMenu_RunBoot(menu);
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case NETMENU_MODE_SAVE_SYSTEM:
    case NETMENU_MODE_SAVE_GALLERY:
        CardMenu_RunSaveSystem(menu);
        if (menu->state > 1 && menu->state != 15) {
            menu->layers[0] = 1;
        }
        menu->layers[1] = 2;
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case NETMENU_MODE_GAME_SAVE:
        CardMenu_RunGameSave(menu);
        menu->layers[0] = 3;
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case NETMENU_MODE_GAME_SAVE_TOWN:
    case NETMENU_MODE_GAME_SAVE_ENTRY:
    case NETMENU_MODE_GAME_SAVE_LOGOUT:
        CardMenu_RunGameSaveNet(menu);
        menu->layers[0] = 3;
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case NETMENU_MODE_GAME_SAVE_EXTRA:
        CardMenu_RunGameSaveExtra(menu);
        menu->layers[0] = 3;
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case NETMENU_MODE_SETUP:
        CardMenu_RunSetup(menu);
        break;
    case NETMENU_MODE_SAVE:
    case NETMENU_MODE_SAVE_ASK:
        CardMenu_RunSave(menu);
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case NETMENU_MODE_LOAD:
        CardMenu_RunLoad(menu);
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case NETMENU_MODE_DELETE_LOGIN_ID:
        CardMenu_RunDeleteLoginId(menu);
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case 11:
    case 12:
    case 13:
    case 15:
    case NETMENU_MODE_SAVE_B:
    default:
        menu->done = 1;
        break;
    }
    Text_SetSize(0);
    CardTask_Update();
    if (menu->done != 0) {
        gSysSettings.cardMenuDone = 1;
    }
    if (Game_IsOnline() == 0 && Overlay_IsLoaded(4) == 1 && gScene.online == 1) {
        LobbyWorld_HoldClock();
    }
    return menu->done;
}

/* unk_3D54B0's .data (0x467C78-0x468850) */
extern char D_004AEB48[];
extern char D_004AEB50[];
extern char D_004AEB60[];
extern char D_004AEB80[];
extern char D_004AEB98[];
extern char D_004AEBA0[];
extern char D_004AEBD0[];
extern char D_004AEBF0[];
extern char D_004AEC10[];
extern char D_004AEC30[];
extern char D_004AEC60[];
extern char D_004AEC90[];
extern char D_004AECC0[];
extern char D_004AECE0[];
extern char D_004AED10[];
extern char D_004AED40[];
extern char D_004AED60[];
extern char D_004AED90[];
extern char D_004AEDB0[];
extern char D_004AEDD0[];
extern char D_004AEDF0[];
extern char D_004AEE10[];
extern char D_004AEE30[];
extern char D_004AEE50[];
extern char D_004AEE70[];
extern char D_004AEEA0[];
extern char D_004AEEC0[];
extern char D_004AEEE0[];
extern char D_004AEF10[];
extern char D_004AEF40[];
extern char D_004AEF70[];
extern char D_004AEF90[];
extern char D_004AEFB0[];
extern char D_004AF010[];
extern char D_004AF040[];
extern char D_004AF060[];
extern char D_004AF090[];
extern char D_004AF0B0[];
extern char D_004AF0D0[];
extern char D_004AF100[];
extern char D_004AF130[];
extern char D_004AF160[];
extern char D_004AF180[];
extern char D_004AF1A0[];
extern char D_004AF1C0[];
extern char D_004AF200[];
extern char D_004AF230[];
extern char D_004AF260[];
extern char D_004AF280[];
extern char D_004AF2B0[];
extern char D_004AF310[];
extern char D_004AF340[];
extern char D_004AF360[];
extern char D_004AF3C0[];
extern char D_004AF3F0[];
extern char D_004AF420[];
extern char D_004AF450[];
extern char D_004AF470[];
extern char D_004AF490[];
extern char D_004AF4C0[];
extern char D_004AF4F0[];
extern char D_004AF510[];
extern char D_004AF530[];
extern char D_004AF560[];
extern char D_004AF580[];
extern char D_004AF5B0[];
extern char D_004AF5F0[];
extern char D_004AF620[];
extern char D_004AF640[];
extern char D_004AF670[];
extern char D_004AF6B0[];
extern char D_004AF6F0[];
extern char D_004AF720[];
extern char D_004AF740[];
extern char D_004AF770[];
extern char D_004AF7B0[];
extern char D_004AF7E0[];
extern char D_004AF810[];
extern char D_004AF850[];
extern char D_004AF8D0[];
extern char D_004AF910[];
extern char D_004AF950[];
extern char D_004AF970[];
extern char D_004AF9B0[];
extern char D_004AF9E0[];
extern char D_004AFA10[];
extern char D_004AFA40[];
extern char D_004AFA70[];
extern char D_004AFAA0[];
extern char D_004AFB00[];
extern char D_004AFB40[];
extern char D_004AFB70[];
extern char D_004AFB90[];
extern char D_004AFBC0[];
extern char D_004AFBF0[];
extern char D_004AFC10[];
extern char D_004AFC40[];
extern char D_004AFC70[];
extern char D_004AFCC0[];
extern char D_004AFCF0[];
extern char D_004AFD20[];
extern char D_004AFD60[];
extern char D_004AFD80[];
extern char D_004AFDB0[];
extern char D_004AFDD0[];
extern char D_004AFDF0[];
extern char D_004AFE10[];
extern char D_004AFE30[];
extern char D_004AFE60[];
extern char D_004AFE90[];
extern char D_004AFEB0[];
extern char D_004AFEC8[];
extern char D_004AFED0[];
extern char D_004AFEE0[];
extern char D_004AFF10[];
extern char D_004AFF40[];
extern char D_004AFF60[];
extern char D_004AFF80[];
extern char D_004AFFA0[];
extern char D_004AFFD0[];
extern char D_004B0010[];
extern char D_004B0030[];
extern char D_004B0070[];
extern char D_004B00A0[];
extern char D_004B00C0[];
extern char D_004B00E0[];
extern char D_004B0110[];
extern char D_004B0130[];
extern char D_004B0150[];
extern char D_004B0170[];
extern char D_004B01A0[];
extern char D_004B01D0[];
extern char D_004B0200[];
extern char D_004B0210[];
extern u32 D_004B0278[];
extern u32 D_004B0280[];
extern u32 D_004B0288[];
char *gNetMenuYesNoLabels[2] = {
    D_004AEB48, D_004AEB50,
};
u8 *D_00467C80[8] = {
    (u8 *)D_004AEB60, (u8 *)D_004AEB80, (u8 *)D_004AEB98, (u8 *)D_004AEBA0,
    (u8 *)D_004AEBD0, NULL, NULL, NULL,
};
u8 *D_00467CA0[8] = {
    (u8 *)D_004AEBF0, (u8 *)D_004AEC10, (u8 *)D_004AEC30, (u8 *)D_004AEB98,
    (u8 *)D_004AEBA0, (u8 *)D_004AEBD0, NULL, NULL,
};
u8 *D_00467CC0[8] = {
    (u8 *)D_004AEC60, (u8 *)D_004AEC90, (u8 *)D_004AECC0, (u8 *)D_004AEB98,
    (u8 *)D_004AECE0, NULL, NULL, NULL,
};
u8 *D_00467CE0[4] = {
    (u8 *)D_004AEBF0, (u8 *)D_004AEC10, (u8 *)D_004AED10, NULL,
};
u8 *D_00467CF0[12] = {
    (u8 *)D_004AED40, (u8 *)D_004AED60, (u8 *)D_004AEB98, (u8 *)D_004AED90,
    (u8 *)D_004AEDB0, (u8 *)D_004AEDD0, (u8 *)D_004AEB98, (u8 *)D_004AECE0,
    NULL, NULL, NULL, NULL,
};
u8 *D_00467D20[12] = {
    (u8 *)D_004AEBF0, (u8 *)D_004AEDF0, (u8 *)D_004AEE10, (u8 *)D_004AEB98,
    (u8 *)D_004AED90, (u8 *)D_004AEDB0, (u8 *)D_004AEDD0, (u8 *)D_004AEB98,
    (u8 *)D_004AECE0, NULL, NULL, NULL,
};
u8 *D_00467D50[4] = {
    (u8 *)D_004AEE30, (u8 *)D_004AEB98, (u8 *)D_004AEE50, NULL,
};
u8 *D_00467D60[2] = {
    (u8 *)D_004AEE70, NULL,
};
u8 *D_00467D68[3] = {
    (u8 *)D_004AEEA0, (u8 *)D_004AEEC0, NULL,
};
u8 *D_00467D78[3] = {
    (u8 *)D_004AEEE0, (u8 *)D_004AEF10, NULL,
};
u8 *D_00467D90[4] = {
    (u8 *)D_004AED60, (u8 *)D_004AEF40, (u8 *)D_004AEDD0, NULL,
};
u8 *D_00467DA0[4] = {
    (u8 *)D_004AEE10, (u8 *)D_004AEF40, (u8 *)D_004AEDD0, NULL,
};
u8 *D_00467DB0[4] = {
    (u8 *)D_004AEF70, (u8 *)D_004AEF90, NULL, NULL,
};
u8 *D_00467DC0[4] = {
    (u8 *)D_004AEFB0, (u8 *)D_004AEF10, NULL, NULL,
};
u8 *D_00467DD0[2] = {
    (u8 *)D_004AEFE0, NULL,
};
u8 *D_00467DD8[3] = {
    (u8 *)D_004AF010, (u8 *)D_004AF040, NULL,
};
u8 *D_00467DE8[3] = {
    (u8 *)D_004AF060, (u8 *)D_004AEF10, NULL,
};
u8 *D_00467DF8[2] = {
    (u8 *)D_004AF090, NULL,
};
u8 *D_00467E00[2] = {
    (u8 *)D_004AF0B0, NULL,
};
u8 *D_00467E08[3] = {
    (u8 *)D_004AF0D0, (u8 *)D_004AECC0, NULL,
};
u8 *D_00467E18[2] = {
    (u8 *)D_004AF100, NULL,
};
u8 *D_00467E20[8] = {
    (u8 *)D_004AF130, (u8 *)D_004AF160, (u8 *)D_004AF180, (u8 *)D_004AF1A0,
    (u8 *)D_004AF1C0, NULL, NULL, NULL,
};
u8 *D_00467E40[4] = {
    (u8 *)D_004AF1E0, (u8 *)D_004AF1A0, (u8 *)D_004AF1C0, NULL,
};
u8 *D_00467E50[4] = {
    (u8 *)D_004AF090, (u8 *)D_004AF200, (u8 *)D_004AF230, NULL,
};
u8 *D_00467E60[4] = {
    (u8 *)D_004AF260, (u8 *)D_004AF280, (u8 *)D_004AF2B0, NULL,
};
u8 *D_00467E70[4] = {
    (u8 *)D_004AF2E0, (u8 *)D_004AF280, (u8 *)D_004AF2B0, NULL,
};
u8 *D_00467E80[4] = {
    (u8 *)D_004AEC60, (u8 *)D_004AF0D0, (u8 *)D_004AF0B0, NULL,
};
u8 *D_00467E90[4] = {
    (u8 *)D_004AF310, (u8 *)D_004AEF10, NULL, NULL,
};
u8 *D_00467EA0[4] = {
    (u8 *)D_004AEEC0, NULL, NULL, NULL,
};
u8 *D_00467EB0[4] = {
    (u8 *)D_004AF340, (u8 *)D_004AEEE0, (u8 *)D_004AEF10, NULL,
};
u8 *D_00467EC0[4] = {
    (u8 *)D_004AF360, (u8 *)D_004AF060, (u8 *)D_004AEF10, NULL,
};
u8 *D_00467ED0[4] = {
    (u8 *)D_004AF390, (u8 *)D_004AF3C0, (u8 *)D_004AF3F0, NULL,
};
u8 *D_00467EE0[4] = {
    (u8 *)D_004AF360, (u8 *)D_004AF0B0, NULL, NULL,
};
u8 *D_00467EF0[4] = {
    (u8 *)D_004AF360, (u8 *)D_004AF090, NULL, NULL,
};
u8 *D_00467F00[4] = {
    (u8 *)D_004AED60, NULL, NULL, NULL,
};
u8 *D_00467F10[8] = {
    (u8 *)D_004AF420, (u8 *)D_004AF340, (u8 *)D_004AF450, (u8 *)D_004AEBA0,
    (u8 *)D_004AEBD0, NULL, NULL, NULL,
};
u8 *D_00467F30[4] = {
    (u8 *)D_004AF390, (u8 *)D_004AF3C0, (u8 *)D_004AF3F0, NULL,
};
u8 *D_00467F40[4] = {
    (u8 *)D_004AED60, (u8 *)D_004AEF40, (u8 *)D_004AEDD0, NULL,
};
u8 *D_00467F50[8] = {
    (u8 *)D_004AF470, (u8 *)D_004AF490, (u8 *)D_004AF4C0, (u8 *)D_004AEB98,
    (u8 *)D_004AF4F0, NULL, NULL, NULL,
};
u8 *D_00467F70[4] = {
    (u8 *)D_004AF510, (u8 *)D_004AF200, (u8 *)D_004AF230, NULL,
};
u8 *D_00467F80[4] = {
    (u8 *)D_004AF360, (u8 *)D_004AF090, NULL, NULL,
};
u8 *D_00467F90[4] = {
    (u8 *)D_004AEE70, (u8 *)D_004AEBF0, (u8 *)D_004AF530, NULL,
};
u8 *D_00467FA0[4] = {
    (u8 *)D_004AF1E0, (u8 *)D_004AF560, (u8 *)D_004AF1C0, NULL,
};
u8 *D_00467FB0[4] = {
    (u8 *)D_004AF580, (u8 *)D_004AF5B0, NULL, NULL,
};
u8 *D_00467FC0[2] = {
    (u8 *)D_004AF5F0, NULL,
};
u8 *D_00467FC8[3] = {
    (u8 *)D_004AF620, (u8 *)D_004AF640, NULL,
};
u8 *D_00467FE0[10] = {
    (u8 *)D_004AF670, (u8 *)D_004AEDF0, (u8 *)D_004AEE10, (u8 *)D_004AEB98,
    (u8 *)D_004AED90, (u8 *)D_004AEDB0, (u8 *)D_004AEDD0, (u8 *)D_004AEB98,
    (u8 *)D_004AECE0, NULL,
};
u8 *D_00468008[2] = {
    (u8 *)D_004AF690, NULL,
};
u8 *D_00468010[12] = {
    (u8 *)D_004AF6B0, (u8 *)D_004AF6F0, (u8 *)D_004AF720, (u8 *)D_004AEB98,
    (u8 *)D_004AF740, (u8 *)D_004AF770, (u8 *)D_004AEB98, (u8 *)D_004AF7B0,
    (u8 *)D_004AF7E0, (u8 *)D_004AF810, (u8 *)D_004AF850, NULL,
};
u8 *D_00468040[12] = {
    (u8 *)D_004AF360, (u8 *)D_004AF6F0, (u8 *)D_004AF720, (u8 *)D_004AEB98,
    (u8 *)D_004AF740, (u8 *)D_004AF770, (u8 *)D_004AEB98, (u8 *)D_004AF7B0,
    (u8 *)D_004AF7E0, (u8 *)D_004AF810, (u8 *)D_004AF850, NULL,
};
u8 *D_00468070[12] = {
    (u8 *)D_004AF890, (u8 *)D_004AF8D0, (u8 *)D_004AF910, (u8 *)D_004AF950,
    (u8 *)D_004AF970, (u8 *)D_004AF9B0, (u8 *)D_004AF9E0, (u8 *)D_004AFA10,
    (u8 *)D_004AFA40, (u8 *)D_004AEB98, (u8 *)D_004AFA70, NULL,
};
u8 *D_004680A0[4] = {
    (u8 *)D_004AFAA0, (u8 *)D_004AF280, (u8 *)D_004AF2B0, NULL,
};
u8 *D_004680B0[4] = {
    (u8 *)D_004AFAE0, (u8 *)D_004AF0D0, (u8 *)D_004AECC0, NULL,
};
u8 *D_004680C0[4] = {
    (u8 *)D_004AFB00, (u8 *)D_004AF280, (u8 *)D_004AF2B0, NULL,
};
u8 *D_004680D0[4] = {
    (u8 *)D_004AFB40, (u8 *)D_004AFB70, (u8 *)D_004AF100, NULL,
};
u8 *D_004680E0[12] = {
    (u8 *)D_004AEC60, (u8 *)D_004AFB90, (u8 *)D_004AFBC0, (u8 *)D_004AEB98,
    (u8 *)D_004AFBF0, (u8 *)D_004AFC10, (u8 *)D_004AFC40, (u8 *)D_004AFC70,
    (u8 *)D_004AEB98, (u8 *)D_004AF1C0, NULL, NULL,
};
u8 *D_00468110[4] = {
    (u8 *)D_004AFC90, (u8 *)D_004AF3C0, (u8 *)D_004AF3F0, NULL,
};
u8 *D_00468120[4] = {
    (u8 *)D_004AF090, (u8 *)D_004AFCC0, (u8 *)D_004AFCF0, NULL,
};
u8 *D_00468130[4] = {
    (u8 *)D_004AF510, (u8 *)D_004AFD20, (u8 *)D_004AF230, NULL,
};
u8 *D_00468140[4] = {
    (u8 *)D_004AEBF0, (u8 *)D_004AEC10, (u8 *)D_004AFD60, NULL,
};
u8 *D_00468150[4] = {
    (u8 *)D_004AFD80, (u8 *)D_004AEEC0, NULL, NULL,
};
u8 *D_00468160[4] = {
    (u8 *)D_004AFAE0, (u8 *)D_004AF0D0, (u8 *)D_004AF0B0, NULL,
};
u8 *D_00468170[10] = {
    (u8 *)D_004AFDB0, (u8 *)D_004AFDD0, (u8 *)D_004AFDF0, (u8 *)D_004AF720,
    (u8 *)D_004AEB98, (u8 *)D_004AFE10, (u8 *)D_004AFE30, (u8 *)D_004AFE60,
    NULL, NULL,
};
char *D_00468198[3] = {
    D_004AFE90, D_004AFEB0, NULL,
};
char *D_004681A8[3] = {
    D_004AFEC8, D_004AFED0, NULL,
};
char *D_004681C0[8] = {
    D_004AEC60, D_004AFEE0, D_004AEB98, D_004AECE0,
    NULL, NULL, NULL, NULL,
};
char *D_004681E0[6] = {
    D_004AFF10, D_004AFF40, D_004AFF60, D_004AF4C0,
    D_004AF1C0, NULL,
};
char *D_004681F8[3] = {
    D_004AFF80, D_004AFFA0, NULL,
};
char *D_00468208[3] = {
    D_004AFFD0, D_004B0010, NULL,
};
char *D_00468218[3] = {
    D_004B0030, D_004B0070, NULL,
};
char *D_00468228[3] = {
    D_004B00A0, D_004AED60, NULL,
};
char *D_00468240[8] = {
    D_004B00C0, D_004AEB98, D_004B00E0, D_004B0110,
    D_004AEB98, D_004B0130, D_004B0150, NULL,
};
char *D_00468260[4] = {
    D_004AF360, D_004B0170, D_004B01A0, NULL,
};
char *D_00468270[4] = {
    D_004B01D0, D_004B0200, D_004B01A0, NULL,
};
char *D_00468280[4] = {
    D_004B0210, NULL, NULL, NULL,
};
u8* *gNetMenuMsgLines[64] = {
    NULL, D_00467C80, D_00467CA0, D_00467CC0,
    D_00467CE0, D_00467CF0, D_00467D20, D_00467D50,
    D_00467D60, D_00467D68, D_00467D78, D_00467D90,
    D_00467DA0, D_00467DB0, D_00467DC0, D_00467DD0,
    D_00467DD8, D_00467DE8, D_00467DF8, D_00467E00,
    D_00467E08, D_00467E18, D_00467E20, D_00467E40,
    D_00467E50, D_00467E60, D_00467E70, D_00467E80,
    D_00467E90, D_00467EA0, D_00467EB0, D_00467EC0,
    D_00467ED0, D_00467EE0, D_00467EF0, D_00467F00,
    D_00467F10, D_00467F30, D_00467F40, D_00467F50,
    D_00467F70, D_00467F80, D_00467F90, D_00467FA0,
    D_00467FB0, D_00467FC0, D_00467FC8, D_00467FE0,
    D_00468008, D_00468010, D_00468040, D_00468070,
    D_004680A0, D_004680B0, D_004680C0, D_004680D0,
    D_004680E0, D_00468110, D_00468120, D_00468130,
    D_00468140, D_00468150, D_00468160, D_00468170,
};
char* *gNetMenuMsgLines40[12] = {
    (char* *)D_00468198, (char* *)D_004681A8, (char* *)D_004681C0, (char* *)D_004681E0,
    (char* *)D_004681F8, (char* *)D_00468208, (char* *)D_00468218, (char* *)D_00468228,
    (char* *)D_00468240, (char* *)D_00468260, (char* *)D_00468270, (char* *)D_00468280,
};
u32 gMovieDescs[52] = {
#include "sub_main/004683C0.inc"
};
u32 gLoadingImageSizes[12] = {
#include "sub_main/00468490.inc"
};
u32 gLoadingSprites[28] = {
#include "sub_main/004684C0.inc"
};
u32 D_00468530[120] = {
#include "sub_main/00468530.inc"
};
u32 gLoadingBobOffsets[12] = {
#include "sub_main/00468710.inc"
};
u32 gLoadingStartPos[6] = {
#include "sub_main/00468740.inc"
};
f32 gLobbyDayFrac = {
#include "sub_main/00468758.inc"
};
ListWin gNetMenuPanels_1 = {
#include "sub_main/00468760.inc"
};
ListWin gNetMenuPanels_2 = {
#include "sub_main/00468780.inc"
};
ListWin gNetMenuPanels_3 = {
#include "sub_main/004687A0.inc"
};
ListWin gNetMenuPanels_4 = {
#include "sub_main/004687C0.inc"
};
ListWin gNetMenuPanels_7 = {
#include "sub_main/004687E0.inc"
};
ListWin *gNetMenuPanels[8] = {
    NULL, &gNetMenuPanels_1, &gNetMenuPanels_2, &gNetMenuPanels_3,
    &gNetMenuPanels_4, NULL, NULL, &gNetMenuPanels_7,
};
char *gGenderLabels[4] = {
    (char *)D_004B0278, (char *)D_004B0280, (char *)D_004B0288, (char *)D_004B0280,
};
s16 gCardSlotPos[12] = {
#include "sub_main/00468830.inc"
};
s16 D_00468848[4] = {
#include "sub_main/00468848.inc"
};
