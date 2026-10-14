/*
 * The memory card menu, NetMenu (0x3D54B0-0x3DDBA0, one translation unit): its helpers,
 * the patch area (0xAA6780..0xAC67C0), save slot checks, the state setter
 * NetMenu_SetState and the states themselves.
 *
 * NetMenu_SetState is static, as in the original: MWCC then knows it leaves
 * a0/a1 alone, and its callers use them after the call (CardSys_Start stores
 * the state number from a1; NetMenu_StartCardRead(m) needs no reload of m). Its
 * callers still in assembly link against it because tools/mwcc_cc.py makes
 * the symbol global in the object.
 */
#include "common.h"
#include "save.h"
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
void SaveFile_Decrypt(NetMenu *m, u16 *p);

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
void CardNew_ChooseCharSlot(NetMenu *m);
void CardGame_AskQuit(NetMenu *m);
void CardSave_PrepareSave(NetMenu *m);
void CardTask_StartReadClear(s32 slot, u8 *buf, s32 size);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void CardMenu_InitBuffers(NetMenu *m);
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
    u8 unk0[0x10];              /* 0x00 */
    u8 pad10[0x20 - 0x10];
    s32 unk20;
    u8 unk24;
    u8 unk25;
    u8 unk26;
    u8 unk27;
    u8 unk28;
    u8 unk29;
    u8 pad2A[0x30 - 0x2A];
    s32 unk30[2][2];            /* 0x30 */
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

void func_003D54B0(s32 unused) /* unused: the caller passes 0 */
{
}

s32 func_003D54C0(void)
{
    return 0;
}

void func_003D54D0(void)
{
}

void LobbyDay_SetNoon(void)
{
    gLobbyDayFrac = 0.5f;
}

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

s32 Patch_LoadStep(void)
{
    u8 *hdr;
    u8 *dst;
    u8 *src;
    s32 r;
    s32 st;

    hdr = gPatchHdr;
    src = gPatchSrc;
    dst = gPatchDst;
    st = gPatchResult;
    if (st != 0) {
        return (s8)st;
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
        r = NetXfer_Step2(&gPatchXferA, (s16 *)&gPatchXferB, dst, *(s32 *)(hdr + 0x14), 0x10000);
        if (r > 0) {
            if (strncmp((char *)dst, D_004B0240, 8) != 0) {
                gPatchResult = 1;
            } else {
                gPatchStep = 0;
                gPatchXferA = 0;
                gPatchResult = 2;
                gPatchXferB = 0;
            }
        } else if (r < 0) {
            gPatchStep = 0;
            gPatchResult = -1;
            gPatchXferA = 0;
            gPatchXferB = 0;
        }
        break;
    }
    return gPatchResult;
}

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
void CardMenu_DrawSlots(NetMenu *m, s32 mode, u8 n, s32 unused, s32 flag)
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
        if (i == m->sub) {
            if (flag == 0) {
                color = 5;
            } else {
                pos = &gCardSlotPos[i * 4];
                size = 0x18;
                col = 0x807F7F7F;
            }
            if (mode == 0 || mode == 2 || n >= 2) {
                if (flag == 0) {
                    Menu_DrawGlowBar(win.x + 0x73, win.y - 3, 0x118, 0x46, 0xFF20C0C0);
                }
            }
        } else if (flag == 0) {
            color = 0;
        } else {
            size = 0x12;
            col = 0x80101010;
            if (i == 1 && m->sub == 2) {
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


s32 SaveMenu_SlotUsed(s32 i)
{
    SaveData *p = gCardSlots;

    p += i;
    return p->used != 0;
}

void CardMenu_DrawSlotFrames(ChrEdit *e, s32 mode, u8 n, s8 sel)
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
        if (i == sel && (mode == 0 || mode == 2 || n >= 2)) {
            Menu_DrawGlowBar(win.x + 0x73, win.y - 3, 0x118, 0x46, 0xFF20C0C0);
        }
        Window_DrawTextAlpha((Window *)&win, 0, 0x80);
        win.y += 0x58;
    }
}

void CardMenu_DrawCharacters(ChrEdit *e, s32 b, s32 c, s32 d, s8 sel)
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

/*
 * Encrypts a save file in place: 0x40 random bytes, then the header words (the version, the
 * key taken from the random bytes, the sum of the plain data) and the data xor-ed with a key
 * stream. Round 29: the data walker is the parameter reused after the random bytes (the game
 * copies p into a0), the sum is kept through a pointer (t2), and the version and the key are
 * two locals (the key's first web, numbered before the sum pointer).
 */
void SaveFile_Encrypt(u8 *buf)
{
    s32 i;
    u16 ver;
    s32 size;
    u8 *p;
    u16 *h;
    u16 key;
    u16 *sum;

    h = (u16 *)buf;
    p = buf;
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
        *p = Rand_Next(0) & 0xFFFF;
        size--;
        p++;
    }
    h[0] = ver;
    buf = p;
    h[2] = 0;
    sum = &h[2];
    key = h[1];
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
void SaveFile_Decrypt(NetMenu *m, u16 *p)
{
    s32 size;
    u16 ver;
    s32 i;
    u16 sum;
    u8 *buf;
    u16 key;

    switch (m->fileKind) {
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
        m->badVersion = 1;
        m->badSum = 1;
        return;
    }
    buf = (u8 *)p;
    m->badVersion = 0;
    size -= 0x40;
    sum = 0;
    m->badVersion = ((u16 *)buf)[0] != ver;
    key = ((u16 *)buf)[1];
    p = (u16 *)(buf + 0x40);
    for (i = 0; i < size / 2; i++) {
        *p ^= key;
        sum += *p;
        p++;
        if (key == 0) {
            key = 1;
        }
        key = (key * 0xB0) % 0xFF53;
    }
    m->badSum = ((u16 *)buf)[2] != sum;
}

s32 SaveMenu_SystemIdMatches(NetMenu *m)
{
    s32 *p;
    u8 *buf = m->readBuf;

    switch (m->fileKind) {
    case 1:
        p = (s32 *)(buf + 0x40);
        break;
    case 2:
    case 4:
    case 8:
        p = (s32 *)(buf + 0x40);
        break;
    default:
        return 0;
    }
    if (m->pairs[0][0] != p[0] || m->pairs[0][1] != p[1]) {
        return 0;
    }
    return 1;
}

static s32 SaveMenu_CharIdMatches(NetMenu *m, s32 k)
{
    if (m->pairs[k][0] != m->charId0 || m->pairs[k][1] != m->charId1) {
        return 0;
    }
    return 1;
}

s32 SaveMenu_ReadBad(NetMenu *m)
{
    FlushCache(0);
    SaveFile_Decrypt(m, (u16 *)m->readBuf);
    if (m->badVersion != 0 || m->badSum != 0) {
        return 1;
    }
    return 0;
}

s32 SysSave_Exchange(s32 dir, u8 *buf)
{
    SysBody *b = (SysBody *)(buf + 0x50);
    SysSave *sys = &gSysSave;
    s32 i;

    if (dir == 0) {
        memcpy(sys, b, 0x10);
        sys->unk458 |= b->unk20;
        sys->lastSlot = b->unk24;
        sys->unk45D = b->unk25;
        sys->unk45E |= b->unk26;
        sys->unk45F |= b->unk27;
        sys->unk460 |= b->unk28;
        sys->unk461 |= b->unk29;
        i = 0;
        do {
            sys->unk3C64[i][0] |= b->unk30[i][0];
            sys->unk3C64[i][1] |= b->unk30[i][1];
            i++;
        } while (i < 2);
        SysSettings_Reset();
    } else {
        memcpy(b, sys, 0x10);
        b->unk20 = sys->unk458;
        b->unk24 = sys->lastSlot;
        b->unk25 = sys->unk45D;
        b->unk26 = sys->unk45E;
        b->unk27 = sys->unk45F;
        b->unk28 = sys->unk460;
        b->unk29 = sys->unk461;
        i = 0;
        do {
            b->unk30[i][0] = sys->unk3C64[i][0];
            b->unk30[i][1] = sys->unk3C64[i][1];
            i++;
        } while (i < 2);
    }
    return 1;
}

void SysSave_MergeFlags(u8 *buf)
{
    SysBody *b = (SysBody *)(buf + 0x50);
    s32 i;

    gSysSave.unk458 |= b->unk20;
    gSysSave.unk45E |= b->unk26;
    gSysSave.unk45F |= b->unk27;
    gSysSave.unk460 |= b->unk28;
    gSysSave.unk461 |= b->unk29;
    i = 0;
    do {
        gSysSave.unk3C64[i][0] |= b->unk30[i][0];
        gSysSave.unk3C64[i][1] |= b->unk30[i][1];
        i++;
    } while (i < 2);
}

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

s32 Patch_FileHasPatch(u8 *buf)
{
    return *(s32 *)(buf + 0x54) != 0;
}

void Patch_BeginFile(u8 *buf)
{
    u8 *hdr = buf + 0x40;

    Patch_BeginLoad(hdr, hdr + 0x40, (u8 *)((s32)buf + 0x10080)); /* integer sum, as in Patch_InstallFile */
}

void Save_ClearCopy(void)
{
    memset(&gSysSave.save, 0, sizeof(SaveData));
}

void SaveMenu_CopyAll(NetMenu *m)
{
    s32 i;
    s32 saved = m->fileKind;

    memset(gCardSlots, 0, sizeof(SaveData) * 3);
    m->fileKind = 1;
    SaveMenu_CopySystem((struct SaveMenu *)m, (u8 *)SaveBuf_Offset(m->imageBuf, 0, 0), 1);
    for (i = 0; i < 3; i++) {
        m->fileKind = 2 << i;
        SaveMenu_CopyCharacter((struct SaveMenu *)m, SaveBuf_Offset(m->imageBuf, 1, i), 1, i);
    }
    m->fileKind = 0x10;
    Patch_CopyFile(1, SaveBuf_Offset(m->imageBuf, 2, 0));
    m->fileKind = saved;
}

void SaveMenu_ClearCharacter(NetMenu *m)
{
    SaveData *s = gCardSlots;

    s += m->charIndex;
    memset(s, 0, sizeof(SaveData));
}

void SaveMenu_CopyCharacterSlot(NetMenu *m, s32 i)
{
    SaveMenu_CopyCharacter((struct SaveMenu *)m, SaveBuf_Offset(m->imageBuf, 1, i), 1, i);
}

/* The callers in this file pass the menu m they already hold in a0 (the game's code sets no
 * argument register). */
void CardMenu_InitBuffers(NetMenu *m)
{
    s32 buf;

    CardTask_Begin(0);
    buf = gLoadBuf;
    memset((void *)buf, 0, 0x6AFE8);
    m->iconBuf = (u8 *)buf;
    m->imageBuf = buf + 0x20000;
    m->readBuf = (u8 *)(buf + 0x40400);
    m->slots = (SaveData *)(buf + 0x60800);
    LoadCompressedFile(m->iconBuf, 0x1714);
    m->fileBlocks = CardOp_SetFiles((s32 *)m->iconBuf);
    m->charRead[0] = 0;
    m->charRead[1] = 0;
    m->charRead[2] = 0;
    gCardSlots = m->slots;
    D_006ADAAC = 0;
}

void CardMenu_InitLoadBuffers(NetMenu *m)
{
    s32 buf;

    CardTask_Begin(0);
    buf = gLoadBuf;
    memset((void *)buf, 0, 0x6AFE8);
    m->iconBuf = (u8 *)buf;
    m->imageBuf = buf + 0x20000;
    m->readBuf = (u8 *)(buf + 0x40400);
    m->slots = (SaveData *)(buf + 0x60800);
    m->charRead[0] = 0;
    m->charRead[1] = 0;
    m->charRead[2] = 0;
    if (Overlay_IsLoaded(4) == 1) {
        gCardSlots = (SaveData *)gLobbySaveBuf;
    } else {
        gCardSlots = gSaveBuf;
    }
    D_006ADAAC = 0;
}

s16 NetMenu_DrawMessageY(s16 y, s32 id)
{
    return NetMenu_DrawMessage(0, y, id);
}

s16 NetMenu_DrawMessage(s32 x, s16 y, s32 id)
{
    char num[4];
    char sj[4];
    char line[100];
    char name[16];
    char wide[64];
    NetMenu *menu = &gCardMenu;
    u8 **lines;
    s32 special;
    char *p;
    s32 n;
    s32 i;
    u8 *s;

    Font_SetSpacing(0x12, 0x12);
    Text_SetColor(0);
    lines = gNetMenuMsgLines[id];
    special = 0;
    if (id == 0x3F) {
        special = 1;
    }
    if (id == 0x48) {
        special = 1;
    }
    while ((s = *lines) != NULL) {
        if (*s != 0) {
            if (special == 1) {
                strcpy(line, (char *)s);
                p = strchr(line, '%');
                if (p != NULL) {
                    switch (p[1]) {
                    case 't':
                        *p = 0;
                        strcat(line, gNetMenuMsgLines40[0][gOldGameKind - 1U]);
                        strcat(line, strchr((char *)s, '%') + 2);
                        break;
                    case 'p':
                        sprintf(num, "%d", menu->oldSlot + 1);
                        Sjis_Widen(num, sj);
                        p[0] = sj[0];
                        p[1] = sj[1];
                        break;
                    case 'i':
                        *p = 0;
                        sprintf(name, "%s", menu->readBuf + 0x960);
                        if ((n = strlen(name)) < 10) {
                            do {
                                strcat(name, " ");
                                n++;
                            } while (n < 10);
                        }
                        Sjis_Widen(name, wide);
                        strcat(line, wide);
                        strcat(line, strchr((char *)s, '%') + 2);
                        break;
                    case 'q':
                        *p = 0;
                        n = strlen((char *)menu->readBuf + 0x96B);
                        if (n != 0) {
                            for (i = 0; i < n; i++) {
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
                        strcat(line, strchr((char *)s, '%') + 2);
                        break;
                    }
                }
                Font_SetPos(0x140 - Text_DisplayLength(line) * 18 / 4, y);
                Text_Printf(line);
            } else {
                Font_SetPos(0x140 - Text_DisplayLength((char *)s) * 18 / 4, y);
                Text_Printf((char *)s);
            }
        }
        y += 0x12;
        lines++;
    }
    return y - 0x12;
}

static void NetMenu_SetState(NetMenu *m, u8 state)
{
    m->state = state;
    m->sub = 0;
    m->blinkTimer = 0xF;
    m->blinkPhase = 1;
}

s32 NetMenu_YesNo(NetMenu *m, s32 x, s16 y, u8 *cursor)
{
    s32 i;
    s32 px;
    char **lbl;

    if (*cursor == 0) {
        if (m->buttons & 0x400) {
            *cursor ^= 1;
            Sound_PlaySeFlat(0, 0x12, 0);
        }
    } else if (m->buttons & 0x800) {
        *cursor ^= 1;
        Sound_PlaySeFlat(0, 0x12, 0);
    }
    Font_SetSpacing(0x12, 0x12);
    for (i = 0, px = 0xF8, lbl = gNetMenuYesNoLabels; i < 2; i++, px += 0x5A, lbl++) {
        if (*cursor == i) {
            Text_SetColor(5);
        } else {
            Text_SetColor(0);
        }
        Font_SetPos(px, y);
        Text_Printf("%s", *lbl);
    }
    m->showCursor = 1;
    if (*cursor == 0) {
        m->boxX = 0x10A;
    } else {
        m->boxX = 0x16D;
    }
    m->boxY = y - 2;
    m->boxW = 0x6C;
    m->boxH = 0x16;
    if (m->buttons & 0x20) {
        if (*cursor == 0) {
            Sound_PlaySeFlat(0, 0x13, 0);
            return 0;
        }
        Sound_PlaySeFlat(0, 0x14, 0);
        return 1;
    }
    if (m->buttons & 0x40) {
        Sound_PlaySeFlat(0, 0x14, 0);
        return 1;
    }
    return -1;
}

s32 NetMenu_ChooseSlot(NetMenu *m, s16 x, s16 y, u8 *slot, s32 flag)
{
    s32 i;
    char **lbl = D_004682AC[0];
    s32 y2;
    s32 top;
    s32 py;

    if (flag == 0 && (m->buttons & 0x3000)) {
        *slot ^= 1;
        Sound_PlaySeFlat(0, 0x12, 0);
    }
    Font_SetSpacing(0x12, 0x12);
    y2 = y + 0x12;
    top = (s16)y2;
    py = top;
    for (i = 0; i < 2; i++, py += 0x24, lbl += 2) {
        if (*slot == i) {
            Text_SetColor(5);
        } else {
            Text_SetColor(0);
        }
        Font_SetPos(x, py);
        Text_Printf(*lbl);
    }
    if (flag == 0) {
        m->showCursor = 1;
        m->boxX = x + 0x63;
        if (*slot == 0) {
            m->boxY = y2;
        } else {
            m->boxY = top + 0x24;
        }
        m->boxY -= 2;
        m->boxW = 0x132;
        m->boxH = 0x16;
        if (m->buttons & 0x20) {
            Sound_PlaySeFlat(0, 0x13, 0);
            return *slot;
        }
        if (m->buttons & 0x40) {
            Sound_PlaySeFlat(0, 0x14, 0);
            return 2;
        }
    }
    return -1;
}

s32 NetMenu_WaitButton(NetMenu *m, s16 x, s16 y, s32 mode)
{
    m->buttonX = x;
    m->buttonY = y;
    m->blinkTimer--;
    if (m->blinkTimer <= 0) {
        m->blinkTimer = 0xF;
        m->blinkPhase ^= 1;
    }
    if (mode != 2) {
        m->showButton = m->blinkPhase;
    } else {
        m->showButton = 0;
    }
    if (m->buttons & 0x20) {
        if (mode == 0 || mode == 2) {
            Sound_PlaySeFlat(0, 0x13, 0);
        } else if (mode == 3) {
            Sound_PlaySeFlat(1, 4, 0);
        } else {
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        return 1;
    }
    return 0;
}

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

void NetMenu_Open(u8 mode)
{
    NetMenu *menu = &gCardMenu;
    s32 buf;
    s32 i;

    D_006AF044 = 0;
    switch (mode) {
    case 0:
    case 9:
    case 8:
    case 0x12:
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
    case 0x10:
        menu->openMode = mode;
        menu->menuTexOn = 0;
        mode = 5;
        break;
    }
    menu->state = 0;
    menu->sub = 0;
    menu->mode = mode;
    menu->done = 0;
    menu->result[0] = -100;
    menu->result[1] = -100;
    for (i = 0; i < 2; i++) {
        memset(gNetMenuDrawHooks[i], 0, 0x20);
    }
    gSysSettings.unk12 = 0;
    D_006A3A24 = NetMenu_Draw;
    switch (mode) {
    case 6:
    case 7:
    case 8:
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

void NetMenu_Draw(void)
{
    NetMenu *m = &gCardMenu;
    s32 a;
    s32 b;
    s32 c;
    s32 i;
    u8 k;
    Sprite s;

    Gs_SetState(0x60, 0);
    Gfx_SetBlend(4, 5);
    if (D_006AF040 == 1 && gCardMenuTex != 0) {
        a = Gs_GetState(0x6C);
        b = Gs_GetState(0x5E);
        c = Gs_GetState(0x63);
        Gs_GetState(4);
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x5E, 0x32);
        Gs_SetState(0x63, 0);
        Tex_Use(1, (u32 *)&gCardMenuTex);
        Gs_SetState(4, gCardMenuTex);
        s.w = 0x280;
        s.u1 = 0xFF;
        s.h = 0x1C0;
        s.v1 = 0xFF;
        s.color = -1;
        s.x = 0;
        s.y = 0;
        s.u0 = 0;
        s.v0 = 0;
        Sprite_Draw(&s);
        Gs_SetState(0x63, c);
        Gs_SetState(0x5E, b);
        Gs_SetState(0x6C, a);
    }
    Text_SetSize(4);
    for (i = 0; i < 4; i++) {
        k = m->layers[i];
        switch (k) {
        default:
            if (m->mode == 1 || m->mode == 14) {
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
            CardMenu_DrawSlotFrames(&gChrEdit, 0, 0, m->slotSel);
            break;
        case 0:
            break;
        }
    }
    if (m->showCursor != 0) {
        Menu_DrawGlowBar(m->boxX, m->boxY, m->boxW, m->boxH, 0xFF20C0C0);
    }
    if (m->showButton != 0) {
        switch (m->mode) {
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            Hud_DrawButtonIconWhite(0x14, m->buttonX, m->buttonY, 0x146, 1.0f);
            break;
        default:
            Hud_DrawButtonIconWhite(0x14, m->buttonX, m->buttonY, 8, 1.0f);
            break;
        }
    }
    Text_SetSize(0);
}

void NetMenu_StartCardRead(NetMenu *m)
{
    CardTask_StartRead(m->slot, (u8 *)m->readBuf);
    CardTask_SelectFile(8, 0);
    CardState_SetFlags(0x10);
}

void CardBoot_Start(NetMenu *m)
{
    CardTask_Begin(2);
    NetMenu_SetState(m, 1);
    m->slot = 0;
    m->timer = 0;
    CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 0);
    CardTask_SelectFile(4, 0);
    gOldGameKind = 0;
    m->oldSlot = -1;
}

void CardBoot_FindMH(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x64, 0x88, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        gOldGameKind = 1;
        m->oldSlot = m->slot;
        NetMenu_SetState(m, 4);
        m->slot = 0;
        m->timer = 0;
        CardMenu_InitBuffers(m);
        m->unkA3 = 0;
        m->sub = 0;
        CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 0);
        break;
    case -1:
        break;
    default:
        if (m->slot == 0) {
            m->slot = 1;
            CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 0);
            CardTask_SelectFile(4, 0);
            break;
        }
        CardTask_Begin(3);
        NetMenu_SetState(m, 2);
        m->result[1] = 0;
        m->result[0] = 0;
        m->slot = 0;
        m->timer = 0;
        CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 0);
        CardTask_SelectFile(4, 0);
        break;
    }
}

void CardBoot_FindMHG(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x64, 0x88, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        gOldGameKind = 2;
        m->oldSlot = m->slot;
    next:
        NetMenu_SetState(m, 4);
        gSysSettings.cardHasChara = 0;
        m->slot = 0;
        m->timer = 0;
        CardMenu_InitBuffers(m);
        m->unkA3 = 0;
        m->sub = 0;
        break;
    case -1:
        break;
    default:
        if (m->slot != 0) {
            goto next;
        }
        m->slot = 1;
        CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 0);
        CardTask_SelectFile(4, 0);
        break;
    }
}

void CardBoot_AutoLoad(NetMenu *m)
{
    s32 r;
    s32 r0;
    s32 r1;

    switch (m->sub) {
    case 0:
        NetMenu_DrawMessage(0x64, 0x88, 1);
        switch (SaveMenu_LoadSystem((struct SaveMenu *)m, 1)) {
        case 0:
            break;
        case 1:
            m->charIndex = 0;
            m->sub++;
            m->unkA3 = 0;
            break;
        case -1:
            switch (m->result[m->slot]) {
            case -0xFD:
            case -0xFE:
                if (m->slot == 1) {
                    NetMenu_SetState(m, 7);
                    break;
                }
            case -0xFC:
            case -0xFF:
            default:
                if (m->slot == 0) {
                    m->slot = 1;
                    m->sub = 0;
                    m->unkA3 = 0;
                    break;
                }
                r0 = m->result[0];
                if (r0 == -0x100 && m->result[1] != 0) {
                    m->slot = 0;
                    NetMenu_SetState(m, 8);
                    m->msg = 0x42;
                    m->cursor = 1;
                    break;
                }
                if (r0 == -0xFF && m->result[1] == -0x100) {
                    m->slot = 1;
                    NetMenu_SetState(m, 8);
                    m->msg = 0x42;
                    m->cursor = 1;
                    break;
                }
                if (r0 == -0xFB && m->result[1] != 0) {
                    m->slot = 0;
                    NetMenu_SetState(m, 8);
                    m->msg = 3;
                    m->cursor = 1;
                    break;
                }
                r1 = m->result[1];
                if (r1 == -0xFB && r0 != 0) {
                    m->slot = 1;
                    NetMenu_SetState(m, 8);
                    m->msg = 3;
                    m->cursor = 1;
                    break;
                }
                if (r0 == -0xFD || r0 == -0xFE) {
                    NetMenu_SetState(m, 7);
                    break;
                }
                if (r1 == -0xFD || r1 == -0xFE) {
                    NetMenu_SetState(m, 7);
                    break;
                }
                m->cursor = 1;
                if (m->result[0] == -0xFC && m->result[1] == -0xFC) {
                    m->slot = 0;
                    NetMenu_SetState(m, 8);
                    m->msg = 0x2F;
                    break;
                }
                if (m->result[0] == -0xFC) {
                    m->slot = 0;
                    NetMenu_SetState(m, 8);
                    m->msg = 6;
                    break;
                }
                if (m->result[1] == -0xFC) {
                    m->slot = 1;
                    NetMenu_SetState(m, 8);
                    m->msg = 6;
                    break;
                }
                NetMenu_SetState(m, 8);
                m->msg = 5;
                break;
            }
            break;
        }
        break;
    case 1:
        NetMenu_DrawMessage(0x64, 0x88, 2);
        SaveMenu_CheckCharacter((struct SaveMenu *)m);
        r = CardTask_Result();
        m->result[m->slot] = r;
        switch (r) {
        case 0:
            gSysSettings.cardHasChara = 1;
        done:
            NetMenu_SetState(m, 5);
            m->timer = 0x78;
            break;
        case -1:
            break;
        default:
            m->charIndex++;
            if (m->charIndex >= 3) {
                m->charIndex = 0;
                goto done;
            }
            m->unkA3 = 0;
            break;
        }
        break;
    }
}

/* 0x3D8510: states of the network menu */

s32 NetMenu_YesNo(NetMenu *m, s32 x, s16 y, u8 *cursor);
s32 NetMenu_CardChanged(s32 slot);
void Patch_InstallFile(u8 *buf);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void CardMenu_InitBuffers(NetMenu *m);
void SaveMenu_CopyAll(NetMenu *m);
s32 SaveMenu_SaveSystem(struct SaveMenu *m);
void CardTask_StartDelete(s32 slot);
s32 CardTask_Result(void);
s32 SaveMenu_ReadBad(NetMenu *m);
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
void SaveMenu_CopyCharacterSlot(NetMenu *m, s32 i);
extern s8 D_006ADAAE;
extern u8 gOldGameKind;
extern SaveData *gCardSlots;

void CardBoot_Loaded(NetMenu *m)
{
    NetMenu_DrawMessage(0x64, 0x88, 2);
    NetMenu_SetState(m, 9);
}

void CardBoot_ShowLoaded(NetMenu *m)
{
    if (NetMenu_WaitButton(m, 0x1DE, NetMenu_DrawMessage(0x64, 0x88, 4) + 0x12, 2) != 0 || m->timer <= 0) {
        NetMenu_SetState(m, 7);
        return;
    }
}

void CardBoot_ShowOldData(NetMenu *m)
{
    switch (m->sub) {
    case 0:
        if (D_006ADAAE != 0 || gOldGameKind == 0) {
            NetMenu_SetState(m, 0xC);
            break;
        }
        m->timer = 0x78;
        m->sub++;
    case 1:
        if (NetMenu_WaitButton(m, 0x1DE, NetMenu_DrawMessage(0x64, 0x88, 0x3F) + 0x12, 2) != 0 || m->timer <= 0) {
            NetMenu_SetState(m, 0xC);
        }
        break;
    }
}

void CardBoot_AskStart(NetMenu *m)
{
    s32 msg = m->msg;

    switch (NetMenu_YesNo(m, 0x52, NetMenu_DrawMessage(0x64, 0x88, msg) + 0x12, &m->cursor)) {
    case 1:
        m->state = 0;
        break;
    case 0:
        NetMenu_SetState(m, 7);
        break;
    }
}

void CardBoot_ReadPatch(NetMenu *m)
{
    NetMenu_DrawMessage(0x64, 0x88, 2);
    NetMenu_SetState(m, 0xA);
    NetMenu_StartCardRead(m);
}

void CardBoot_CheckPatch(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x64, 0x88, 2);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        if (SaveMenu_ReadBad(m) != 0) {
        fail:
            m->timer = 0x78;
            NetMenu_SetState(m, 6);
            Sound_PlaySeFlat(0, 0x19, 0);
            break;
        }
        if (Patch_FileHasPatch(m->readBuf) == 0) {
            goto fail;
        }
        m->timer = 0x78;
        NetMenu_SetState(m, 0xB);
        Patch_BeginFile(m->readBuf);
        break;
    case -1:
        break;
    case -253:
    case -252:
    case -255:
    default:
        m->timer = 0x78;
        NetMenu_SetState(m, 6);
        Sound_PlaySeFlat(0, 0x19, 0);
        break;
    }
}

void CardBoot_LoadPatch(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x64, 0x88, 4);
    r = Patch_LoadStep();
    if (r != 0) {
        if (r == 2) {
            Patch_InstallFile(m->readBuf);
        }
        NetMenu_SetState(m, 6);
        Sound_PlaySeFlat(0, 0x19, 0);
    }
}

void CardMenu_RunBoot(NetMenu *m)
{
    switch (m->state) {
    case 0:
        CardBoot_Start(m);
        break;
    case 1:
        CardBoot_FindMH(m);
        break;
    case 2:
        CardBoot_FindMHG(m);
        break;
    case 3:
        CardBoot_FindMHG(m);
        break;
    case 4:
        CardBoot_AutoLoad(m);
        break;
    case 5:
        CardBoot_Loaded(m);
        break;
    case 6:
        CardBoot_ShowLoaded(m);
        break;
    case 7:
        CardBoot_ShowOldData(m);
        break;
    case 8:
        CardBoot_AskStart(m);
        break;
    case 9:
        CardBoot_ReadPatch(m);
        break;
    case 10:
        CardBoot_CheckPatch(m);
        break;
    case 11:
        CardBoot_LoadPatch(m);
        break;
    case 12:
        m->done = 1;
        break;
    }
}

void CardSys_Start(NetMenu *m)
{
    NetMenu_SetState(m, 1);
    m->cursor = 1;
    m->slot = 0;
}

void CardSys_AskSave(NetMenu *m)
{
    s16 h;

    if (m->mode == 1) {
        h = NetMenu_DrawMessage(0xDD, 0x88, 9);
    } else {
        h = NetMenu_DrawMessage(0xDD, 0x88, 0x3D);
    }
    switch (NetMenu_YesNo(m, 0xCB, h + 0x24, &m->cursor)) {
    case 1:
        NetMenu_SetState(m, 0xF);
        break;
    case 0:
        NetMenu_SetState(m, 2);
        m->sub = 0;
        m->slot = 0;
        break;
    }
}

void CardSys_ChooseSlot(NetMenu *m)
{
    NetMenu_DrawMessage(0x28, 0x160, 8);
    switch (m->sub) {
    case 0:
        switch (NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 0)) {
        case 0:
        case 1:
            m->sub++;
            m->timer = 4;
            break;
        case 2:
            NetMenu_SetState(m, 1);
            break;
        }
        break;
    case 1:
        NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
        if (--m->timer <= 0) {
            NetMenu_SetState(m, 3);
            m->timer = 0x1E;
            CardMenu_InitBuffers(m);
            m->charIndex = 0;
            m->unkA3 = 0;
        }
        break;
    }
}

void CardSys_ReadCharacters(NetMenu *m)
{
    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    switch (SaveMenu_LoadCharacter((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        m->pairs[m->charIndex + 1][0] = m->charId0;
        m->pairs[m->charIndex + 1][1] = m->charId1;
    case -1:
        m->charIndex++;
        if (m->charIndex == 3) {
            NetMenu_SetState(m, 4);
            CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 1);
            CardTask_SelectFile(4, 0);
            CardState_SetFlags(1);
            break;
        }
        m->unkA3 = 0;
        break;
    }
}

void CardSys_CheckSystem(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        Card_ClearChanged();
        if (SaveMenu_ReadBad(m) != 0) {
            goto retry;
        }
        SysSave_MergeFlags(m->readBuf);
        NetMenu_SetState(m, 0xA);
        CardTask_StartCheck();
        m->cursor = 1;
        break;
    case -1:
        break;
    case -253:
        CardState_SetFlag(0x1F);
    retry:
        NetMenu_SetState(m, 0xB);
        break;
    case -252:
        NetMenu_SetState(m, 0xE);
        m->msg = 0xC;
        break;
    case -255:
        NetMenu_SetState(m, 0xE);
        m->msg = 0xB;
        break;
    case -254:
        NetMenu_SetState(m, 5);
        m->cursor = 1;
        Card_ClearChanged();
        CardTask_StartCheck();
        break;
    default:
        NetMenu_SetState(m, 0xE);
        m->msg = 0x14;
        break;
    }
}

void CardSys_AskFormat(NetMenu *m)
{
    s16 h;

    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    h = NetMenu_DrawMessage(0x28, 0x160, 0xD);
    if (NetMenu_CardChanged(m->slot) != 0) {
        NetMenu_SetState(m, 0xE);
        m->msg = 0x26;
        return;
    }
    switch (NetMenu_YesNo(m, 0x16, h + 0x12, &m->cursor)) {
    case 1:
        NetMenu_SetState(m, 2);
        break;
    case 0:
        NetMenu_SetState(m, 6);
        m->timer = 0x1E;
        CardTask_StartDelete(m->slot);
        break;
    }
}

void CardSys_Format(NetMenu *m)
{
    s32 r;

    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, 0xE);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        CardState_SetFlag(0x1F);
        NetMenu_SetState(m, 0xB);
        break;
    case -1:
        break;
    case -256:
    default:
        NetMenu_SetState(m, 0xE);
        m->msg = 0xF;
        break;
    }
}

void CardSys_CheckSystemReadPatch(NetMenu *m)
{
    s32 r;

    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        if (SaveMenu_ReadBad(m) != 0) {
            NetMenu_SetState(m, 0xE);
            m->msg = 0x14;
            break;
        }
        SysSave_MergeFlags(m->readBuf);
        NetMenu_SetState(m, 8);
        NetMenu_StartCardRead(m);
        CardTask_StartCheck();
        break;
    case -1:
        break;
    case -253:
    case -252:
    case -255:
    default:
        NetMenu_SetState(m, 0xE);
        m->msg = 0x14;
        break;
    }
}

void CardSys_CheckPatch(NetMenu *m)
{
    s32 r;

    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        if (SaveMenu_ReadBad(m) != 0) {
        fail:
            m->timer = 0x78;
            NetMenu_SetState(m, 0xA);
            break;
        }
        if (Patch_FileHasPatch(m->readBuf) == 0) {
            goto fail;
        }
        m->timer = 0x78;
        NetMenu_SetState(m, 9);
        Patch_BeginFile(m->readBuf);
        break;
    case -1:
        break;
    case -253:
    case -252:
    case -255:
    default:
        m->timer = 0x78;
        NetMenu_SetState(m, 5);
        break;
    }
}

void CardSys_LoadPatch(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    r = Patch_LoadStep();
    if (r != 0) {
        if (r == 2) {
            Patch_InstallFile(m->readBuf);
        }
        NetMenu_SetState(m, 0xA);
        m->cursor = 1;
    }
}

void CardSys_AskOverwrite(NetMenu *m)
{
    s16 h;

    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    h = NetMenu_DrawMessage(0x28, 0x160, 0x10);
    if (NetMenu_CardChanged(m->slot) != 0) {
        NetMenu_SetState(m, 0xE);
        m->msg = 0x26;
        return;
    }
    switch (NetMenu_YesNo(m, 0x16, h + 0x12, &m->cursor)) {
    case 1:
        NetMenu_SetState(m, 2);
        break;
    case 0:
        CardState_SetFlag(1);
        NetMenu_SetState(m, 0xB);
        break;
    }
}

void CardSys_PrepareSave(NetMenu *m)
{
    CardMenu_InitBuffers(m);
    SaveMenu_CopyAll(m);
    NetMenu_SetState(m, 0xC);
    m->timer = 0x1E;
    m->unkA3 = 0;
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, 0x11);
}

void CardSys_Save(NetMenu *m)
{
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, 0x11);
    switch (SaveMenu_SaveSystem((struct SaveMenu *)m)) {
    case 1:
        NetMenu_SetState(m, 0xF);
        Sound_PlaySeFlat(0, 0x19, 0);
        break;
    case 0:
        break;
    case -1:
        NetMenu_SetState(m, 0xE);
        m->msg = 0x13;
        break;
    }
}

void CardSys_ShowSaved(NetMenu *m)
{
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, 0x12);
    if (NetMenu_WaitButton(m, 0x232, 0x184, 0) != 0) {
        NetMenu_SetState(m, 0xF);
    }
}

void CardSys_ShowError(NetMenu *m)
{
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, m->msg);
    if (NetMenu_WaitButton(m, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(m, 2);
    }
}

void CardMenu_RunSaveSystem(NetMenu *m)
{
    switch (m->state) {
    case 0:
        CardSys_Start(m);
        break;
    case 1:
        CardSys_AskSave(m);
        break;
    case 2:
        CardSys_ChooseSlot(m);
        break;
    case 3:
        CardSys_ReadCharacters(m);
        break;
    case 4:
        CardSys_CheckSystem(m);
        break;
    case 5:
        CardSys_AskFormat(m);
        break;
    case 6:
        CardSys_Format(m);
        break;
    case 7:
        CardSys_CheckSystemReadPatch(m);
        break;
    case 8:
        CardSys_CheckPatch(m);
        break;
    case 9:
        CardSys_LoadPatch(m);
        break;
    case 10:
        CardSys_AskOverwrite(m);
        break;
    case 11:
        CardSys_PrepareSave(m);
        break;
    case 12:
        CardSys_Save(m);
        break;
    case 13:
        CardSys_ShowSaved(m);
        break;
    case 14:
        CardSys_ShowError(m);
        break;
    case 15:
        m->done = 1;
        break;
    }
}

void CardNew_Start(NetMenu *m)
{
    NetMenu_SetState(m, 2);
    m->cursor = 0;
    m->slot = 0;
    m->hasSave = 0;
}

void CardNew_AskSave(NetMenu *m)
{
    switch (NetMenu_YesNo(m, 0x52, NetMenu_DrawMessage(0x28, 0x160, 0x2C) + 0x12, &m->cursor)) {
    case 1:
        NetMenu_SetState(m, 0xF);
        break;
    case 0:
        NetMenu_SetState(m, 1);
        break;
    }
}

void CardNew_ChooseSlot(NetMenu *m)
{
    s32 r;

    switch (m->sub) {
    case 0:
        NetMenu_DrawMessage(0x28, 0x160, 8);
        switch (NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 0)) {
        case 0:
        case 1:
            m->sub++;
            m->timer = 4;
            m->charNo = 0;
            CardMenu_InitBuffers(m);
            SaveMenu_CopyAll(m);
            CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 1);
            CardTask_SelectFile(4, 0);
            CardState_SetFlags(1);
            m->timer = 0x1E;
            break;
        case 2:
            NetMenu_SetState(m, 2);
            m->cursor = 0;
            break;
        }
        break;
    case 1:
        NetMenu_DrawMessage(0x28, 0x160, 0xA);
        NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
        r = CardTask_Result();
        m->result[m->slot] = r;
        switch (r) {
        case 0:
            if (SaveMenu_ReadBad(m) == 0) {
                SysSave_MergeFlags(m->readBuf);
            }
            m->timer = 0x1E;
            m->sub++;
            m->charLoaded = 0;
            Card_ClearChanged();
            D_006ADAAC = 0;
            m->charIndex = m->charNo;
            m->unkA3 = 0;
            break;
        case -1:
            break;
        case -253:
        case -254:
            NetMenu_SetState(m, 5);
            m->charLoaded = 0;
            gChrEdit.slot = 0;
            Card_ClearChanged();
            CardTask_StartCheck();
            break;
        case -252:
            NetMenu_SetState(m, 0xD);
            m->msg = 0xC;
            break;
        case -255:
            NetMenu_SetState(m, 0xD);
            m->msg = 0xB;
            break;
        default:
            NetMenu_SetState(m, 0xD);
            m->msg = 0x14;
            break;
        }
        break;
    case 2:
        NetMenu_DrawMessage(0x28, 0x160, 0xA);
        NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
        switch (SaveMenu_LoadCharacter((struct SaveMenu *)m)) {
        case 0:
            break;
        case 1:
            m->pairs[m->charIndex + 1][0] = m->charId0;
            m->pairs[m->charIndex + 1][1] = m->charId1;
            m->charLoaded |= 1;
            D_006ADAAC++;
        next:
            m->charNo++;
            if (m->charNo == 3) {
                CardTask_StartCheck();
                m->timer = 0x78;
                NetMenu_SetState(m, 5);
                m->charNo = 0;
            } else {
                m->charIndex = m->charNo;
                m->unkA3 = 0;
            }
            break;
        case -1:
            switch (m->result[m->slot]) {
            case -255:
                NetMenu_SetState(m, 0xD);
                m->msg = 0xB;
                m->charNo = 0;
                break;
            case -253:
            case -252:
            case -251:
                goto next;
            case -254:
                NetMenu_SetState(m, 5);
                m->charLoaded = 0;
                gChrEdit.slot = 0;
                m->charIndex = 0;
                CardTask_StartCheck();
                break;
            default:
                NetMenu_SetState(m, 0xD);
                m->msg = 0x14;
                m->charNo = 0;
                break;
            }
            break;
        }
        break;
    }
}

void CardNew_ChooseCharSlot(NetMenu *m)
{
    SaveData *saves;

    if (m->buttons & 0x2000) {
        Sound_PlaySeFlat(0, 0x12, 0);
        gChrEdit.slot--;
        if ((s8)gChrEdit.slot < 0) {
            gChrEdit.slot = 2;
        }
    }
    if (m->buttons & 0x1000) {
        Sound_PlaySeFlat(0, 0x12, 0);
        gChrEdit.slot++;
        if (gChrEdit.slot > 2) {
            gChrEdit.slot = 0;
        }
    }
    NetMenu_DrawMessage(0x28, 0x160, 0x2A);
    if (NetMenu_CardChanged(m->slot) != 0) {
        NetMenu_SetState(m, 0xD);
        m->msg = 0x26;
        return;
    }
    if (m->buttons & 0x20) {
        Sound_PlaySeFlat(0, 0x13, 0);
        NetMenu_SetState(m, 6);
        m->cursor = 1;
        saves = m->slots;
        if (m->charLoaded == 1) {
            if (saves[gChrEdit.slot].used == 0) {
                m->msg = 0x17;
            } else {
                m->msg = 0x16;
            }
        } else {
            m->msg = 0x2B;
        }
    } else if (m->buttons & 0x40) {
        Sound_PlaySeFlat(0, 0x14, 0);
        NetMenu_SetState(m, 1);
    }
    m->layers[1] = 5;
    CardMenu_DrawCharacters(&gChrEdit, 0, 0, 0, (s8)gChrEdit.slot);
}

void CardNew_ConfirmCharSlot(NetMenu *m)
{
    s16 h = NetMenu_DrawMessage(0xDD, 0x88, m->msg);

    NetMenu_DrawMessage(0x28, 0x160, 0x2A);
    if (NetMenu_CardChanged(m->slot) != 0) {
        NetMenu_SetState(m, 0xD);
        m->msg = 0x26;
        return;
    }
    switch (NetMenu_YesNo(m, 0xCB, h + 0x24, &m->cursor)) {
    case 1:
        NetMenu_SetState(m, 5);
        break;
    case 0:
        Sound_PlaySeFlat(1, 6, 0);
        switch (m->result[m->slot]) {
        case 0:
        case -253:
        case -251:
            NetMenu_SetState(m, 9);
            break;
        case -254:
            NetMenu_SetState(m, 7);
            m->cursor = 1;
            break;
        default:
            NetMenu_SetState(m, 0xD);
            m->msg = 0x13;
            break;
        }
        break;
    }
}

void CardNew_AskFormat(NetMenu *m)
{
    s16 h = NetMenu_DrawMessage(0x28, 0x160, 0xD);

    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    if (NetMenu_CardChanged(m->slot) != 0) {
        NetMenu_SetState(m, 0xD);
        m->msg = 0x26;
        return;
    }
    switch (NetMenu_YesNo(m, 0x16, h + 0x12, &m->cursor)) {
    case 1:
        NetMenu_SetState(m, 1);
        break;
    case 0:
        NetMenu_SetState(m, 8);
        m->timer = 0x1E;
        CardTask_StartDelete(m->slot);
        break;
    }
}

void CardNew_Format(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xE);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        NetMenu_SetState(m, 9);
        break;
    case -1:
        break;
    case -256:
    default:
        NetMenu_SetState(m, 0xD);
        m->msg = 0xF;
        break;
    }
}

void CardNew_PrepareSave(NetMenu *m)
{
    s32 i = gChrEdit.slot;

    m->charIndex = i;
    NetMenu_SetState(m, 0xA);
    m->timer = 0x1E;
    func_6AF3D0(&gChrEdit, 0);
    Save_CopyToSlot(i);
    gSysSave.lastSlot = gChrEdit.slot;
    gSysSave.unk45D = 3;
    SaveMenu_CopyCharacterSlot(m, i);
    m->fileKind = 2 << i;
    Card_ClearChanged();
    m->charIndex = gChrEdit.slot;
    m->unkA3 = 0;
    NetMenu_DrawMessage(0x28, 0x160, 0x11);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
}

void CardNew_SaveCharacter(NetMenu *m)
{
    NetMenu_DrawMessage(0x28, 0x160, 0x11);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    switch (SaveMenu_SaveCharacter((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        NetMenu_SetState(m, 0xB);
        m->hasSave = 1;
        gSysSettings.cardHasChara = 1;
        m->unkA3 = 0;
        break;
    case -1:
        NetMenu_SetState(m, 0xD);
        m->msg = 0x13;
        break;
    }
}

void CardNew_SaveSystem(NetMenu *m)
{
    NetMenu_DrawMessage(0x28, 0x160, 0x11);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    switch (SaveMenu_SaveSystem((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        NetMenu_SetState(m, 0xC);
        Sound_PlaySeFlat(0, 0x19, 0);
        gSysSettings.systemSaved = 1;
        break;
    case -1:
        NetMenu_SetState(m, 0xD);
        m->msg = 0x13;
        break;
    }
}

void CardNew_ShowSaved(NetMenu *m)
{
    NetMenu_DrawMessage(0x28, 0x160, 0x18);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    if (NetMenu_WaitButton(m, 0x232, 0x184, 3) != 0) {
        NetMenu_SetState(m, 0xE);
    }
}

void CardNew_ShowError(NetMenu *m)
{
    s32 msg = m->msg;

    NetMenu_DrawMessage(0x28, 0x160, msg);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    if (NetMenu_WaitButton(m, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(m, 1);
    }
}

void CardNew_CheckPatch(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        if (SaveMenu_ReadBad(m) != 0) {
        fail:
            m->timer = 0x78;
            NetMenu_SetState(m, 5);
            break;
        }
        if (Patch_FileHasPatch(m->readBuf) == 0) {
            goto fail;
        }
        m->timer = 0x78;
        NetMenu_SetState(m, 4);
        Patch_BeginFile(m->readBuf);
        break;
    case -1:
        break;
    case -253:
    case -252:
    case -255:
    default:
        m->timer = 0x78;
        NetMenu_SetState(m, 5);
        break;
    }
}

void CardNew_LoadPatch(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = Patch_LoadStep();
    if (r != 0) {
        if (r == 2) {
            Patch_InstallFile(m->readBuf);
        }
        m->timer = 0x78;
        NetMenu_SetState(m, 5);
    }
}

void CardMenu_RunSaveNew(NetMenu *m)
{
    switch (m->state) {
    case 0:
        CardNew_Start(m);
        break;
    case 2:
        CardNew_AskSave(m);
        break;
    case 1:
        CardNew_ChooseSlot(m);
        m->layers[1] = 2;
        break;
    case 3:
        CardNew_CheckPatch(m);
        m->layers[1] = 2;
        break;
    case 4:
        CardNew_LoadPatch(m);
        m->layers[1] = 2;
        break;
    case 5:
        CardNew_ChooseCharSlot(m);
        break;
    case 6:
        CardNew_ConfirmCharSlot(m);
        m->layers[1] = 2;
        break;
    case 7:
        CardNew_AskFormat(m);
        m->layers[1] = 2;
        break;
    case 8:
        CardNew_Format(m);
        m->layers[1] = 2;
        break;
    case 9:
        CardNew_PrepareSave(m);
        m->layers[1] = 2;
        break;
    case 10:
        CardNew_SaveCharacter(m);
        m->layers[1] = 2;
        break;
    case 11:
        CardNew_SaveSystem(m);
        m->layers[1] = 2;
        break;
    case 12:
        CardNew_ShowSaved(m);
        m->layers[1] = 2;
        break;
    case 13:
        CardNew_ShowError(m);
        m->layers[1] = 2;
        break;
    case 14:
        m->done = 1;
        break;
    case 15:
    default:
        m->done = 2;
        break;
    }
}

void CardLoad_Start(NetMenu *m)
{
    NetMenu_SetState(m, 1);
    m->slot = 0;
    m->hasSave = 0;
}

void CardLoad_ChooseSlot(NetMenu *m)
{
    NetMenu_DrawMessage(0x28, 0x160, 0x15);
    switch (m->sub) {
    case 0:
        switch (NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 0)) {
        case 0:
        case 1:
            m->sub++;
            m->timer = 4;
            break;
        case 2:
            NetMenu_SetState(m, 0xA);
            break;
        }
        break;
    case 1:
        NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
        if (m->timer <= 0) {
            NetMenu_SetState(m, 2);
            m->timer = 0x1E;
            CardMenu_InitLoadBuffers(m);
            m->fileKind = 1;
            CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 1);
            CardTask_SelectFile(4, 0);
        }
        break;
    }
}

void CardLoad_CheckSystem(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        if (SaveMenu_ReadBad(m) != 0) {
            m->result[m->slot] = -0xFB;
            NetMenu_SetState(m, 7);
            m->msg = 0x14;
            break;
        }
        SaveMenu_CopySystem((struct SaveMenu *)m, (u8 *)m->readBuf, 0);
        Card_ClearChanged();
        NetMenu_SetState(m, 4);
        NetMenu_StartCardRead(m);
        break;
    case -1:
        break;
    case -253:
        NetMenu_SetState(m, 7);
        m->msg = 0x19;
        break;
    case -254:
        NetMenu_SetState(m, 7);
        m->msg = 0x19;
        break;
    case -252:
        NetMenu_SetState(m, 7);
        m->msg = 0x19;
        break;
    case -255:
        NetMenu_SetState(m, 7);
        m->msg = 0x23;
        break;
    default:
        NetMenu_SetState(m, 7);
        m->msg = 0x14;
        break;
    }
}

static s32 SaveMenu_FreeCharSlots(NetMenu *m)
{
    s32 bits = 0;
    s32 i;
    SaveData *s = gCardSlots;

    for (i = 0; i < 3; i++, s++) {
        switch (m->charRead[i]) {
        case 0:
            bits |= 1 << i;
            break;
        case 1:
            if (s->used == 0) {
                bits |= 1 << i;
            }
            break;
        case 0x80:
            break;
        }
    }
    return bits;
}

void CardLoad_LoadCharacters(NetMenu *m)
{
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, 0x1C);
    switch (SaveMenu_LoadCharacter((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        m->pairs[m->charIndex + 1][0] = m->charId0;
        m->pairs[m->charIndex + 1][1] = m->charId1;
        D_006ADAAC++;
    next:
        m->charNo++;
        if (m->charNo == 3) {
            CardTask_StartCheck();
            if (SaveMenu_FreeCharSlots(m) == 7) {
                NetMenu_SetState(m, 7);
                m->msg = 0x1A;
                gSysSettings.cardHasChara = 0;
                break;
            }
            m->timer = 0x1E;
            NetMenu_SetState(m, 8);
            m->hasSave = 1;
            gSysSettings.cardHasChara = 1;
            Sound_PlaySeFlat(0, 0x19, 0);
            break;
        }
        m->charIndex = m->charNo;
        m->unkA3 = 0;
        break;
    case -1:
        switch (m->result[m->slot]) {
        case -0xFD:
        case -0xFC:
        case -0xFB:
            goto next;
        case -0xFF:
        default:
            NetMenu_SetState(m, 7);
            m->msg = 0x14;
            break;
        }
        break;
    }
}

void CardLoad_ShowError(NetMenu *m)
{
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, m->msg);
    if (NetMenu_WaitButton(m, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(m, 1);
        m->sub = 0;
    }
}

void CardLoad_Wait(NetMenu *m)
{
    if (--m->timer <= 0) {
        NetMenu_SetState(m, 9);
    }
}

void CardLoad_CheckPatch(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        if (SaveMenu_ReadBad(m) != 0) {
        fail:
            m->timer = 0x78;
            m->charNo = 0;
            m->charIndex = m->charNo;
            m->unkA3 = 0;
            NetMenu_SetState(m, 6);
            break;
        }
        if (Patch_FileHasPatch(m->readBuf) == 0) {
            goto fail;
        }
        m->timer = 0x78;
        NetMenu_SetState(m, 5);
        Patch_BeginFile(m->readBuf);
        break;
    case -1:
        break;
    case -253:
    case -252:
    case -255:
    default:
        m->timer = 0x78;
        m->charNo = 0;
        m->charIndex = m->charNo;
        m->unkA3 = 0;
        NetMenu_SetState(m, 6);
        break;
    }
}

void CardLoad_LoadPatch(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = Patch_LoadStep();
    if (r != 0) {
        if (r == 2) {
            Patch_InstallFile(m->readBuf);
        }
        NetMenu_SetState(m, 6);
        m->charNo = 0;
        m->charIndex = m->charNo;
        m->unkA3 = 0;
    }
}

void CardMenu_RunLoad(NetMenu *m)
{
    switch (m->state) {
    case 0:
        CardLoad_Start(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 1:
        CardLoad_ChooseSlot(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 2:
        CardLoad_CheckSystem(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 4:
        CardLoad_CheckPatch(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 5:
        CardLoad_LoadPatch(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 6:
        CardLoad_LoadCharacters(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 7:
        CardLoad_ShowError(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 8:
        CardLoad_Wait(m);
        break;
    case 3:
    case 9:
    default:
        m->done = 1;
        break;
    case 10:
        m->done = 2;
        break;
    }
}

/* 0x3DB040: more states of the network menu */

s32 NetMenu_YesNo(NetMenu *m, s32 x, s16 y, u8 *cursor);
s32 NetMenu_CardChanged(s32 slot);
void Patch_InstallFile(u8 *buf);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void CardMenu_InitBuffers(NetMenu *m);
void SaveMenu_CopyAll(NetMenu *m);
s32 SaveMenu_SaveSystem(struct SaveMenu *m);
void CardTask_StartDelete(s32 slot);
s32 CardTask_Result(void);
s32 SaveMenu_ReadBad(NetMenu *m);
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
void SaveMenu_CopyCharacterSlot(NetMenu *m, s32 i);
extern s8 D_006ADAAE;
extern u8 gOldGameKind;
extern SaveData *gCardSlots;
extern u8 gBootMode;
void *memcpy(void *dst, const void *src, u32 n);
s32 Card_IsChanged(s32 slot);
s32 SaveMenu_SaveExtra(struct SaveMenu *m);
void CardMenu_InitNetBuffers(NetMenu *m);
void CardSetup_Start(NetMenu *m);
void CardTask_StartRead(s32 slot, u8 *buf);
void CardTask_StartSave(s32 a, u8 *buf);
void CardTask_Begin(s32 a);
void *memset(void *dst, s32 c, u32 n);
s32 LoadCompressedFile(u8 *dst, s32 id);
s32 CardOp_SetFiles(s32 *data);
s32 Card_PortState(s32 slot);
extern s32 gLoadBuf;

void CardGame_Start(NetMenu *m)
{
    NetMenu_SetState(m, 1);
    m->timer = 0x1E;
    CardMenu_InitBuffers(m);
    SaveMenu_CopyAll(m);
    CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 1);
    CardTask_SelectFile(4, 0);
    CardState_SetFlags(1);
}

void CardGame_CheckSystem(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x64, 0x88, 0x24);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        if (SaveMenu_ReadBad(m) == 0) {
            SysSave_MergeFlags(m->readBuf);
        }
        if (Card_IsChanged(m->slot) != 0) {
            NetMenu_SetState(m, 2);
            m->charIndex = gQuestSync.charIndex;
            m->unkA3 = 0;
            break;
        }
        NetMenu_SetState(m, 3);
        break;
    case -1:
        break;
    case -251:
        NetMenu_SetState(m, 7);
        m->msg = 0x1B;
        m->timer = 0x3C;
        break;
    case -253:
    case -254:
    case -252:
    case -255:
    default:
        NetMenu_SetState(m, 7);
        m->msg = 0x25;
        m->timer = 0x3C;
        break;
    }
}

/* Loads the character being checked: on to state 3 when it is the one expected,
 * else (or on a card error) state 7 with message 0x25; 0x1B for error -251. */
void CardGame_CheckCharacter(NetMenu *m)
{
    NetMenu_DrawMessage(0x64, 0x88, 0x24);
    switch (SaveMenu_LoadCharacter((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        if (SaveMenu_CharIdMatches(m, m->charIndex + 1) != 0) {
            NetMenu_SetState(m, 3);
            break;
        }
        goto fail;
    case -1:
        switch (m->result[m->slot]) {
        case -0xFB:
            NetMenu_SetState(m, 7);
            m->msg = 0x1B;
            m->timer = 0x3C;
            break;
        case -0xFD:
        case -0xFE:
        case -0xFC:
        case -0xFF:
        default:
        fail:
            NetMenu_SetState(m, 7);
            m->msg = 0x25;
            m->timer = 0x3C;
            break;
        }
        break;
    }
}

/* Copies the save into slot gQuestSync.charIndex of the menu's save list (the address as int arithmetic
 * gives the original registers) and shows message 0x64. */
void CardGame_PrepareSave(NetMenu *m)
{
    s32 i;

    NetMenu_SetState(m, 4);
    i = gQuestSync.charIndex;
    memcpy((void *)((s32)m->slots + i * (s32)sizeof(SaveData)), &gSave, sizeof(SaveData)); /* an integer sum: &m->slots[i] changes the registers */
    gSysSave.unk45D = 3;
    SaveMenu_CopyCharacterSlot(m, i);
    m->charIndex = i;
    m->unkA3 = 0;
    NetMenu_DrawMessage(0x64, 0x88, 0x1F);
}

void CardGame_SaveCharacter(NetMenu *m)
{
    NetMenu_DrawMessage(0x64, 0x88, 0x1F);
    switch (SaveMenu_SaveCharacter((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        m->hasSave = 1;
        m->unkA3 = 0;
        gSysSettings.cardHasChara = 1;
        NetMenu_SetState(m, 5);
        break;
    case -1:
        NetMenu_SetState(m, 7);
        m->msg = 0x13;
        m->timer = 0x3C;
        break;
    }
}

void CardGame_SaveCharacterExtra(NetMenu *m)
{
    NetMenu_DrawMessage(0x64, 0x88, 0x1F);
    switch (SaveMenu_SaveCharacter((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        m->hasSave = 1;
        m->unkA3 = 0;
        gSysSettings.cardHasChara = 1;
        NetMenu_SetState(m, 0xC);
        break;
    case -1:
        NetMenu_SetState(m, 7);
        m->msg = 0x13;
        m->timer = 0x3C;
        break;
    }
}

void CardGame_SaveSystem(NetMenu *m)
{
    NetMenu_DrawMessage(0x64, 0x88, 0x1F);
    switch (SaveMenu_SaveSystem((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        Card_ClearChanged();
        NetMenu_SetState(m, 0xA);
        Sound_PlaySeFlat(0, 0x19, 0);
        gSysSettings.systemSaved = 1;
        break;
    case -1:
        NetMenu_SetState(m, 7);
        m->msg = 0x13;
        m->timer = 0x3C;
        break;
    }
}

void CardGame_ShowResult(NetMenu *m)
{
    s32 msg = m->msg;
    NetMenu_DrawMessage(0x64, 0x88, msg);
    if (NetMenu_WaitButton(m, 0x1F4, 0xF4, 0) != 0) {
        NetMenu_SetState(m, 0xA);
    }
}

void CardGame_ShowError(NetMenu *m)
{
    NetMenu_DrawMessage(0x64, 0x88, m->msg);
    if (NetMenu_WaitButton(m, 0x1F4, 0xF4, 1) != 0) {
        if (m->mode == 0xA) {
            NetMenu_SetState(m, 8);
            m->cursor = 0;
            return;
        }
        NetMenu_SetState(m, 8);
        m->cursor = 1;
    }
}

/* After an error. Mode 10 asks whether to save the game so far (0x2E): yes starts
 * again from state 1; no goes on to state 0xB (or 9 when +0xB4 is 0x10 and the mode
 * is not 0x11). The other modes ask whether to go on without saving (0x2D, 0x27). */
void CardGame_AskRetry(NetMenu *m)
{
    s32 mode = m->mode;
    s16 h;

    if (mode == 10) {
        switch (NetMenu_YesNo(m, 0xCB, NetMenu_DrawMessage(0x64, 0x88, 0x2E) + 0x24, &m->cursor)) {
        case 1:
            if (m->openMode != 0x10) {
                NetMenu_SetState(m, 0xB);
                break;
            }
            if (m->mode == 0x11) {
                NetMenu_SetState(m, 0xB);
                break;
            }
            NetMenu_SetState(m, 9);
            break;
        case 0:
        load:
            NetMenu_SetState(m, 1);
            CardMenu_InitBuffers(m);
            SaveMenu_CopyAll(m);
            CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 1);
            CardTask_SelectFile(4, 0);
            CardState_SetFlags(1);
            break;
        }
    } else {
        if (mode == 6 || mode == 7 || mode == 0x11) {
            h = NetMenu_DrawMessage(0x64, 0x88, 0x2D);
        } else {
            h = NetMenu_DrawMessage(0x64, 0x88, 0x27);
        }
        switch (NetMenu_YesNo(m, 0xCB, h + 0x24, &m->cursor)) {
        case 1:
            goto load;
        case 0:
            NetMenu_SetState(m, 0xB);
            break;
        }
    }
}

extern u8 *gSubMenuWork;

/* The menu work is read by its name (round 25): through its raw address
 * (0x8B43F0) the constant 1 for gBootMode went to v0, not a1 as in the game. */
void CardGame_AskQuit(NetMenu *m)
{
    u8 *p;

    switch (NetMenu_YesNo(m, 0xCB, NetMenu_DrawMessage(0x64, 0x88, 0x43) + 0x24, &m->cursor)) {
    case 1:
        NetMenu_SetState(m, 8);
        break;
    case 0:
        p = gSubMenuWork;
        gBootMode = 1;
        p[0xA] = 0;
        NetMenu_SetState(m, 0xB);
        break;
    }
}

void CardGame_SaveExtra(NetMenu *m)
{
    NetMenu_DrawMessage(0x64, 0x88, 0x1F);
    switch (SaveMenu_SaveExtra((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        Card_ClearChanged();
        m->unkA3 = 0;
        NetMenu_SetState(m, 5);
        break;
    case -1:
        NetMenu_SetState(m, 7);
        m->msg = 0x13;
        m->timer = 0x3C;
        break;
    }
}

extern u8 gSysBusy;

void CardMenu_RunGameSave(NetMenu *m)
{
    switch (m->state) {
    case 0:
        CardGame_Start(m);
        gSysBusy = 1;
        break;
    case 1:
        CardGame_CheckSystem(m);
        gSysBusy = 1;
        break;
    case 2:
        CardGame_CheckCharacter(m);
        gSysBusy = 1;
        break;
    case 3:
        CardGame_PrepareSave(m);
        gSysBusy = 1;
        break;
    case 4:
        CardGame_SaveCharacter(m);
        gSysBusy = 1;
        break;
    case 5:
        CardGame_SaveSystem(m);
        gSysBusy = 1;
        break;
    case 6:
        CardGame_ShowResult(m);
        gSysBusy = 0;
        break;
    case 7:
        CardGame_ShowError(m);
        gSysBusy = 0;
        break;
    case 8:
        CardGame_AskRetry(m);
        gSysBusy = 0;
        break;
    case 9:
        CardGame_AskQuit(m);
        gSysBusy = 0;
        break;
    case 10:
    default:
        gSysBusy = 0;
        m->done = 1;
        break;
    case 11:
        gSysBusy = 0;
        m->done = 2;
        break;
    }
}

void NetMode_Begin(DrawSlot *ds);
s32 LobbyCard_SaveNetFile(void);

void CardMenu_RunGameSaveNet(NetMenu *m)
{
    switch (m->state) {
    case 0:
        CardGame_Start(m);
        gSysBusy = 1;
        break;
    case 1:
        CardGame_CheckSystem(m);
        gSysBusy = 1;
        break;
    case 2:
        CardGame_CheckCharacter(m);
        gSysBusy = 1;
        break;
    case 3:
        CardGame_PrepareSave(m);
        gSysBusy = 1;
        break;
    case 4:
        CardGame_SaveCharacter(m);
        gSysBusy = 1;
        break;
    case 5:
        CardGame_SaveSystem(m);
        gSysBusy = 1;
        break;
    case 6:
        CardGame_ShowResult(m);
        gSysBusy = 0;
        break;
    case 7:
        CardGame_ShowError(m);
        gSysBusy = 0;
        break;
    case 8:
        CardGame_AskRetry(m);
        gSysBusy = 0;
        break;
    case 9:
        CardGame_AskQuit(m);
        gSysBusy = 0;
        break;
    case 10:
    default:
        m->quitResult = 1;
        goto quit;
    case 11:
        m->quitResult = 2;
    quit:
        NetMenu_SetState(m, 0xC);
        NetMode_Begin((DrawSlot *)gNetMenuDrawHooks);
        break;
    case 12:
        switch (LobbyCard_SaveNetFile()) {
        case 0:
            break;
        case 1:
        case -1:
            gSysBusy = 0;
            m->done = m->quitResult;
            break;
        }
        break;
    }
}

void CardMenu_RunGameSaveExtra(NetMenu *m)
{
    switch (m->state) {
    case 0:
        CardGame_Start(m);
        gSysBusy = 1;
        break;
    case 1:
        CardGame_CheckSystem(m);
        gSysBusy = 1;
        break;
    case 2:
        CardGame_CheckCharacter(m);
        gSysBusy = 1;
        break;
    case 3:
        CardGame_PrepareSave(m);
        gSysBusy = 1;
        break;
    case 4:
        CardGame_SaveCharacterExtra(m);
        gSysBusy = 1;
        break;
    case 5:
        CardGame_SaveSystem(m);
        gSysBusy = 1;
        break;
    case 6:
        CardGame_ShowResult(m);
        gSysBusy = 0;
        break;
    case 7:
        CardGame_ShowError(m);
        gSysBusy = 0;
        break;
    case 8:
        CardGame_AskRetry(m);
        gSysBusy = 0;
        break;
    case 9:
        CardGame_AskQuit(m);
        gSysBusy = 0;
        break;
    case 10:
    default:
        gSysBusy = 0;
        m->done = 1;
        break;
    case 11:
        gSysBusy = 0;
        m->done = 2;
        break;
    case 12:
        CardGame_SaveExtra(m);
        gSysBusy = 1;
        break;
    }
}

void CardSave_Start(NetMenu *m)
{
    if (m->mode == 9) {
        NetMenu_SetState(m, 2);
        m->cursor = 0;
        if (m->hasSave == 0) {
            m->slot = 0;
        }
    } else if (m->hasSave == 0) {
        NetMenu_SetState(m, 1);
        m->cursor = 0;
        m->slot = 0;
    } else {
        m->mode = 10;
        m->state = 0;
    }
}

void CardSave_AskSave(NetMenu *m)
{
    switch (NetMenu_YesNo(m, 0x52, NetMenu_DrawMessage(0x64, 0x88, 0x1D) + 0x24, &m->cursor)) {
    case 1:
        NetMenu_SetState(m, 0xF);
        break;
    case 0:
        if (m->hasSave == 0) {
            NetMenu_SetState(m, 1);
            m->cursor = 0;
            break;
        }
        m->mode = 0xA;
        m->state = 0;
        break;
    }
}

void CardSave_ChooseSlot(NetMenu *m)
{
    NetMenu_DrawMessage(0x28, 0x160, 8);
    switch (m->sub) {
    case 0:
        switch (NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 0)) {
        case 0:
        case 1:
            m->sub++;
            m->timer = 4;
            break;
        case 2:
            NetMenu_SetState(m, 2);
            m->cursor = 0;
            break;
        }
        break;
    case 1:
        NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
        if (--m->timer <= 0) {
            NetMenu_SetState(m, 3);
            m->timer = 0x1E;
            CardMenu_InitBuffers(m);
            CardMenu_InitLoadBuffers(m);
            SaveMenu_CopyAll(m);
            CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 1);
            CardTask_SelectFile(4, 0);
            CardState_SetFlags(1);
        }
        break;
    }
}

void CardSave_CheckSystem(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        if (SaveMenu_ReadBad(m) == 0) {
            SysSave_MergeFlags(m->readBuf);
        }
        m->timer = 0x1E;
    start:
        Card_ClearChanged();
        NetMenu_SetState(m, 4);
        m->charNo = 0;
        m->fileKind = 2 << m->charNo;
        CardTask_StartRead(m->slot, (u8 *)m->readBuf);
        CardTask_SelectFile(m->charNo + 5, 0);
        break;
    case -1:
        break;
    case -253:
        m->charLoaded = 0;
        goto start;
    case -254:
        NetMenu_SetState(m, 5);
        m->charLoaded = 0;
        gChrEdit.slot = 0;
        Card_ClearChanged();
        CardTask_StartCheck();
        break;
    case -252:
        NetMenu_SetState(m, 0xD);
        m->msg = 0xC;
        break;
    case -255:
        NetMenu_SetState(m, 0xD);
        m->msg = 0xB;
        break;
    default:
        NetMenu_SetState(m, 0xD);
        m->msg = 0x14;
        break;
    }
}

/* Reads the three characters' save files one after the other (m->charNo): a good one
 * is copied into its SaveData (a bad one cleared), then the next is read; after the
 * third, state 5. */
void CardSave_ReadCharacters(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        {
            u8 *buf = m->readBuf;
            u8 *save = (u8 *)&m->slots[m->charNo];

            if (SaveMenu_ReadBad(m) != 0) {
                m->charRead[m->charNo] = 0x80;
                memset(save, 0, 0x37F8);
            } else {
                m->charRead[m->charNo] = 1;
                memcpy(save, buf + 0x50, 0x37F8);
            }
            m->charLoaded = 2;
        }
        goto next;
    case -1:
        break;
    case -0xFF:
        goto err; /* the compare chain wants this label before -0xFD */
    case -0xFD:
    case -0xFC:
    next:
        m->charNo++;
        if (m->charNo == 3) {
            m->charNo = 0;
            CardTask_StartCheck();
            NetMenu_SetState(m, 5);
            m->cursor = 1;
            gChrEdit.slot = 0;
            break;
        }
        m->fileKind = 2 << m->charNo;
        CardTask_StartRead(m->slot, (u8 *)m->readBuf);
        CardTask_SelectFile(m->charNo + 5, 0);
        break;
    err:
        NetMenu_SetState(m, 0xD);
        m->msg = 0xB;
        break;
    default:
        NetMenu_SetState(m, 0xD);
        m->msg = 0x14;
        break;
    }
}

/* Chooses one of the three characters (gChrEdit.slot, left and right): confirm goes to
 * state 6 with the message for an empty or used slot (0x2B when the files are not
 * read), cancel back to state 1; a card change to state 0xD. */
void CardSave_ChooseCharSlot(NetMenu *m)
{
    u8 *saves;

    if (m->buttons & 0x2000) {
        Sound_PlaySeFlat(0, 0x12, 0);
        gChrEdit.slot--;
        if ((s8)gChrEdit.slot < 0) {
            gChrEdit.slot = 2;
        }
    } else if (m->buttons & 0x1000) {
        Sound_PlaySeFlat(0, 0x12, 0);
        gChrEdit.slot++;
        if (gChrEdit.slot > 2) {
            gChrEdit.slot = 0;
        }
    }
    NetMenu_DrawMessage(0x28, 0x160, 0x2A);
    if (NetMenu_CardChanged(m->slot) != 0) {
        NetMenu_SetState(m, 0xD);
        m->msg = 0x26;
        return;
    }
    if (m->buttons & 0x20) {
        Sound_PlaySeFlat(0, 0x13, 0);
        NetMenu_SetState(m, 6);
        m->charIndex = gChrEdit.slot;
        m->cursor = 1;
        saves = (u8 *)m->slots;
        if (m->charLoaded != 0) {
            if (saves[gChrEdit.slot * 0x37F8] == 0) {
                m->msg = 0x17;
            } else {
                m->msg = 0x16;
            }
        } else {
            m->msg = 0x2B;
        }
    } else if (m->buttons & 0x40) {
        Sound_PlaySeFlat(0, 0x14, 0);
        NetMenu_SetState(m, 1);
    }
    m->layers[1] = 6;
    CardMenu_DrawCharacters(&gChrEdit, 2, 0, 0, (s8)gChrEdit.slot);
}

/* The message with a yes/no choice under it: yes goes to state 5; no to state 9 (or 7
 * with the cursor on 1 for a full card, 0xD with message 0x13 on other errors); a card
 * change to state 0xD with message 0x26. */
void CardSave_ConfirmCharSlot(NetMenu *m)
{
    s16 h;

    h = NetMenu_DrawMessage(0xDD, 0x88, m->msg);
    NetMenu_DrawMessage(0x28, 0x160, 0x2A);
    if (NetMenu_CardChanged(m->slot) != 0) {
        NetMenu_SetState(m, 0xD);
        m->msg = 0x26;
        return;
    }
    switch (NetMenu_YesNo(m, 0xCB, h + 0x24, &m->cursor)) {
    case 1:
        NetMenu_SetState(m, 5);
        break;
    case 0:
        switch (m->result[m->slot]) {
        case 0:
        case -0xFD:
            NetMenu_SetState(m, 9);
            break;
        case -0xFE:
            NetMenu_SetState(m, 7);
            m->cursor = 1;
            break;
        default:
            NetMenu_SetState(m, 0xD);
            m->msg = 0x13;
            break;
        }
        break;
    }
}

void CardSave_AskFormat(NetMenu *m)
{
    s16 h = NetMenu_DrawMessage(0x28, 0x160, 0xD);

    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    if (NetMenu_CardChanged(m->slot) != 0) {
        NetMenu_SetState(m, 0xD);
        m->msg = 0x26;
        return;
    }
    switch (NetMenu_YesNo(m, 0x16, h + 0x12, &m->cursor)) {
    case 1:
        NetMenu_SetState(m, 1);
        break;
    case 0:
        NetMenu_SetState(m, 8);
        m->timer = 0x1E;
        CardTask_StartDelete(m->slot);
        break;
    }
}

void CardSave_Format(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xE);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        NetMenu_SetState(m, 9);
        break;
    case -1:
        break;
    case -256:
    default:
        NetMenu_SetState(m, 0xD);
        m->msg = 0xF;
        break;
    }
}

void CardSave_PrepareSave(NetMenu *m)
{
    s32 i;

    NetMenu_SetState(m, 0xA);
    m->timer = 0x1E;
    LoadCompressedFile(m->iconBuf, 0x1714);
    gCardSlots = m->slots;
    i = gChrEdit.slot;
    m->charIndex = i;
    memcpy((void *)((s32)m->slots + i * (s32)sizeof(SaveData)), &gSave, sizeof(SaveData)); /* an integer sum: &m->slots[i] changes the registers */
    gSysSave.unk45D = 3;
    gSysSave.lastSlot = gChrEdit.slot;
    gQuestSync.charIndex = gChrEdit.slot;
    SaveMenu_CopyCharacterSlot(m, i);
    m->fileKind = 2 << i;
    m->unkA3 = 0;
    NetMenu_DrawMessage(0x28, 0x160, 0x11);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
}

void CardSave_SaveCharacter(NetMenu *m)
{
    NetMenu_DrawMessage(0x28, 0x160, 0x11);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    switch (SaveMenu_SaveCharacter((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        NetMenu_SetState(m, 0xB);
        m->hasSave = 1;
        m->unkA3 = 0;
        gSysSettings.cardHasChara = 1;
        break;
    case -1:
        NetMenu_SetState(m, 0xD);
        m->msg = 0x13;
        break;
    }
}

void CardSave_SaveSystem(NetMenu *m)
{
    NetMenu_DrawMessage(0x28, 0x160, 0x11);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    switch (SaveMenu_SaveSystem((struct SaveMenu *)m)) {
    case 0:
        break;
    case 1:
        Card_ClearChanged();
        NetMenu_SetState(m, 0xE);
        Sound_PlaySeFlat(0, 0x19, 0);
        gSysSettings.systemSaved = 1;
        break;
    case -1:
        NetMenu_SetState(m, 0xD);
        m->msg = 0x13;
        break;
    }
}

void CardSave_ShowSaved(NetMenu *m)
{
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, 0x18);
    if (NetMenu_WaitButton(m, 0x232, 0x184, 0) != 0) {
        NetMenu_SetState(m, 0xE);
    }
}

void CardSave_ShowError(NetMenu *m)
{
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, m->msg);
    if (NetMenu_WaitButton(m, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(m, 1);
    }
}

void CardMenu_RunSave(NetMenu *m)
{
    switch (m->state) {
    case 0:
        CardSave_Start(m);
        break;
    case 2:
        CardSave_AskSave(m);
        m->layers[1] = 3;
        break;
    case 1:
        CardSave_ChooseSlot(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 3:
        CardSave_CheckSystem(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 4:
        CardSave_ReadCharacters(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 5:
        CardSave_ChooseCharSlot(m);
        m->layers[0] = 4;
        break;
    case 6:
        CardSave_ConfirmCharSlot(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 7:
        CardSave_AskFormat(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 8:
        CardSave_Format(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 9:
        CardSave_PrepareSave(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 10:
        CardSave_SaveCharacter(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 11:
        CardSave_SaveSystem(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 12:
        CardSave_ShowSaved(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 13:
        CardSave_ShowError(m);
        m->layers[0] = 4;
        m->layers[1] = 3;
        break;
    case 14:
        m->done = 1;
        break;
    case 15:
    default:
        m->done = 2;
        break;
    }
}

void CardSetup_Start(NetMenu *m)
{
    NetMenu_SetState(m, 1);
    m->slot = 0;
    CardMenu_InitBuffers(m);
    CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 0);
}

void CardSetup_Read(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        NetMenu_SetState(m, 2);
        SysSave_Exchange(1, m->readBuf);
        SaveFile_Encrypt((u8 *)gLoadBuf);
        CardTask_StartSave(m->slot, (u8 *) gLoadBuf);
        break;
    case -1:
        break;
    case -253:
    case -254:
    case -252:
    case -255:
    default:
        NetMenu_SetState(m, 3);
        m->msg = 0x13;
        m->timer = 0x78;
        break;
    }
}

void CardSetup_Save(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0x11);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        NetMenu_SetState(m, 3);
        m->msg = 0x12;
        m->timer = 0x78;
        break;
    case -1:
        break;
    default:
        NetMenu_SetState(m, 3);
        m->msg = 0x13;
        m->timer = 0x78;
        break;
    }
}

void CardSetup_ShowResult(NetMenu *m)
{
    s32 msg = m->msg;
    NetMenu_DrawMessage(0x28, 0x160, msg);
    if (NetMenu_WaitButton(m, 0x232, 0x184, 0) != 0 || m->timer <= 0) {
        NetMenu_SetState(m, 4);
    }
}

/* Runs the current step of the menu setup. */
void CardMenu_RunSetup(NetMenu *m)
{
    switch (m->state) {
    case 0:
        CardSetup_Start(m);
        m->layers[1] = 2;
        break;
    case 1:
        CardSetup_Read(m);
        break;
    case 2:
        CardSetup_Save(m);
        break;
    case 3:
        CardSetup_ShowResult(m);
        break;
    case 4:
    default:
        m->done = 1;
        break;
    }
}

/* CardLogin_ChooseSlot passes m, already in a0. */
void CardMenu_InitNetBuffers(NetMenu *m)
{
    s32 buf;

    CardTask_Begin(1);
    buf = gLoadBuf;
    memset((void *)buf, 0, 0x6AFE8);
    m->iconBuf = (u8 *)buf;
    m->imageBuf = buf + 0x20000;
    m->readBuf = (u8 *)(buf + 0x40400);
    LoadCompressedFile(m->iconBuf, 0x1715);
    m->fileBlocks = CardOp_SetFiles((s32 *)m->iconBuf);
}

void CardLogin_Start(NetMenu *m)
{
    NetMenu_SetState(m, 1);
    m->slot = 0;
}

void CardLogin_ChooseSlot(NetMenu *m)
{
    NetMenu_DrawMessage(0x28, 0x160, 0x44);
    switch (m->sub) {
    case 0:
        switch (NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 0)) {
        case 0:
        case 1:
            m->sub++;
            m->timer = 4;
            break;
        case 2:
            NetMenu_SetState(m, 8);
            break;
        }
        break;
    case 1:
        NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
        if (m->timer <= 0) {
            NetMenu_SetState(m, 2);
            m->timer = 0x1E;
            CardMenu_InitNetBuffers(m);
            CardTask_StartReadClear(m->slot, (u8 *)m->readBuf, 1);
        }
        break;
    }
}

void CardLogin_ReadNetFile(NetMenu *m)
{
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, 0xA);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        Card_ClearChanged();
        CardTask_StartCheck();
        if (((s8 *)m->readBuf)[0x960] == 0) {
            NetMenu_SetState(m, 5);
            m->msg = 0x4B;
        } else {
            NetMenu_SetState(m, 3);
            m->msg = 0x48;
            m->cursor = 1;
        }
        m->charNo = 0;
        break;
    case -1:
        break;
    case -253:
        NetMenu_SetState(m, 5);
        m->msg = 0x45;
        break;
    case -254:
        NetMenu_SetState(m, 5);
        m->msg = 0x45;
        break;
    case -252:
        NetMenu_SetState(m, 5);
        m->msg = 0x45;
        break;
    case -255:
        NetMenu_SetState(m, 5);
        m->msg = 0x47;
        break;
    default:
        NetMenu_SetState(m, 5);
        m->msg = 0x46;
        break;
    }
}

void CardLogin_AskDelete(NetMenu *m)
{
    u8 *p;

    if (Card_PortState(m->slot) == 0) {
        NetMenu_SetState(m, 5);
        m->msg = 0x4A;
        return;
    }
    switch (NetMenu_YesNo(m, 0x52, NetMenu_DrawMessage(0x64, 0x88, m->msg) + 0x24, &m->cursor)) {
    case 1:
        m->state = 0;
        break;
    case 0:
        NetMenu_SetState(m, 4);
        m->msg = 0x11;
        p = m->readBuf;
        memset(p + 0x960, 0, 0xB);
        memset(p + 0x96B, 0, 9);
        CardTask_StartSave(m->slot, m->readBuf);
        break;
    }
}

void CardLogin_Save(NetMenu *m)
{
    s32 msg = m->msg;
    s32 r;

    NetMenu_DrawMessage(0x28, 0x160, msg);
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    r = CardTask_Result();
    m->result[m->slot] = r;
    switch (r) {
    case 0:
        NetMenu_SetState(m, 7);
        break;
    case -1:
        break;
    case -253:
    case -252:
    case -255:
    default:
        NetMenu_SetState(m, 6);
        m->msg = 0x49;
        break;
    }
}

void CardLogin_ShowError(NetMenu *m)
{
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, m->msg);
    if (NetMenu_WaitButton(m, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(m, 1);
    }
}

void CardLogin_ShowResult(NetMenu *m)
{
    NetMenu_ChooseSlot(m, 0xDD, 0x88, (u8 *)&m->slot, 1);
    NetMenu_DrawMessage(0x28, 0x160, m->msg);
    if (NetMenu_WaitButton(m, 0x232, 0x184, 1) != 0) {
        NetMenu_SetState(m, 8);
    }
}

void CardMenu_RunDeleteLoginId(NetMenu *m)
{
    switch (m->state) {
    case 0:
        CardLogin_Start(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 1:
        CardLogin_ChooseSlot(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 2:
        CardLogin_ReadNetFile(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 3:
        CardLogin_AskDelete(m);
        m->layers[0] = 0;
        m->layers[1] = 0;
        break;
    case 4:
        CardLogin_Save(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 5:
        CardLogin_ShowError(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 6:
        CardLogin_ShowResult(m);
        m->layers[0] = 1;
        m->layers[1] = 2;
        break;
    case 7:
    default:
        m->done = 1;
        break;
    case 8:
        m->done = 2;
        break;
    }
}

void Text_SetSize(s32 size);
void CardTask_Update(void);
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
s32 Game_IsOnline(void);
void LobbyWorld_HoldClock(void);
void CardMenu_RunSetup(NetMenu *m);

u8 NetMenu_Update(void)
{
    NetMenu *m = &gCardMenu;

    Text_SetSize(4);
    *(s32 *)(m->layers) = 0;
    m->showCursor = 0;
    m->showButton = 0;
    m->buttons = gPadState[0].pressed;
    m->held = gPadState[0].held;
    if (m->timer > 0) {
        m->timer--;
    }
    switch (m->mode) {
    case 3:
        CardMenu_RunSaveNew(m);
        m->layers[0] = 1;
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case 0:
        CardMenu_RunBoot(m);
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case 1:
    case 14:
        CardMenu_RunSaveSystem(m);
        if (m->state > 1 && m->state != 15) {
            m->layers[0] = 1;
        }
        m->layers[1] = 2;
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case 10:
        CardMenu_RunGameSave(m);
        m->layers[0] = 3;
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case 6:
    case 7:
    case 8:
        CardMenu_RunGameSaveNet(m);
        m->layers[0] = 3;
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case 17:
        CardMenu_RunGameSaveExtra(m);
        m->layers[0] = 3;
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case 2:
        CardMenu_RunSetup(m);
        break;
    case 5:
    case 9:
        CardMenu_RunSave(m);
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case 4:
        CardMenu_RunLoad(m);
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case 18:
        CardMenu_RunDeleteLoginId(m);
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
        break;
    case 11:
    case 12:
    case 13:
    case 15:
    case 16:
    default:
        m->done = 1;
        break;
    }
    Text_SetSize(0);
    CardTask_Update();
    if (m->done != 0) {
        gSysSettings.unk12 = 1;
    }
    if (Game_IsOnline() == 0 && Overlay_IsLoaded(4) == 1 && gScene.online == 1) {
        LobbyWorld_HoldClock();
    }
    return m->done;
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
