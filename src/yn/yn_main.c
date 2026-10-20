/*
 * The yn overlay's network settings, which the online set-up loads: the
 * settings list read from the memory card's settings file through Sony's
 * netcnfif library, the connect with the saved settings, and the proxy
 * screen.
 *
 * gYnWork (YnWork) holds the overlay's state. Yn_Update runs the list or
 * the connect (YnWork.state, enum YnState), Yn_UpdateProxy the proxy
 * screen (enum YnProxyState); each queues its draw callback. The chosen
 * entry goes into sub_main's network globals (Yn_ExportSettings) and into
 * gNetSaveData.config, the settings kept in the NET file; gNetMode carries the
 * result back to the online set-up. The YnCard_* functions, in the
 * overlay's other file, check the memory cards. The file owns its .data
 * (the screens' text tables), .rodata and .bss.
 */
#include "common.h"
extern s8 gNetLanMode;
extern s32 gNetNetmask;
extern s32 gNetIpAddr;
extern s32 gNetGateway;
extern s32 gNetDns2;
extern s32 gNetDns1;
extern u16 gFrameTick; /* sub_main: the frame counter */
#include "pad.h"
#include "m2c_macros.h"
#include "yn.h"
#include "net_save.h"
#include "syssave.h"
#include "dma.h"
#include "render_state.h"

/* The game compiled this file at optimization level 4 (the build's -O3,p
 * plus MWCC's level-4 passes): Yn_Connect, Yn_UpdateEntryList, Yn_DrawPort
 * and Yn_DrawChoiceCursor match only there, and every other function
 * compiles the same at both levels. */
#pragma optimization_level 4

/* main: libc and the IOP module calls */
s32 printf(char *fmt, ...);               /* printf */
s32 strlen(char *s);                      /* strlen */
char *strchr(char *s, s32 c);               /* strchr */
char *strncpy(char *dst, char *src, u32 n);    /* strncpy */
s32 flReleaseTextureHandle(u32 handle);
s32 flReleasePaletteHandle(u32 handle);
void Mem_Compact(void);
void *memset(void *dst, s32 c, u32 n);
s32 sprintf(char *dst, char *fmt, ...);    /* sprintf */
s32 strcmp(char *a, char *b);             /* strcmp */
char *strcpy(char *dst, char *src);       /* strcpy */
char *strcat(char *dst, char *src);       /* strcat */
void FlushCache(s32 mode);
s32 sceSifStopModule(s32 id, s32 argc, char *argv, s32 *result);   /* stop a module */
s32 sceSifUnloadModule(s32 id);                       /* unload a module */
s32 sceSifSearchModuleByName(char *name);                   /* find a loaded module by name */
s32 sceSifLoadModule(char *path, s32 argc, char *argv);            /* load a module */
s32 sceSifLoadStartModule(char *path, s32 argc, char *argv, void *buf);
/* sub_main */
s32 Sjis_CharBytes(u8 *s);                     /* size of the character at s */
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_Printf(char *fmt, ...);
void Font_SetSize(f32 scale);
void Font_SetColorIndex(s16 c);
void Font_SetHalfWidth(s8 mode); /* unit's own declaration: an s8 mode as Yn_SetTextMode passes it; with the definition's s32, the call compiles differently */
s32 Str_Length(char *text);
void Text_Printf(const char *fmt, ...);
void Font_ResetState(void);
void NetDev_ClearList(void);
s32 NetDev_IsKnownName(char *vendor, char *product);
s32 net_inet_addr2(char *addr);                  /* the address in a dotted string */
s32 Net_StopDevice(void);
s32 Net_LoadDriverType1(void);
s32 Net_LoadDrivers(void);
s32 NetDev_Refresh(void);
void NetMode_OpenIcon7(void);
void NetMode_CloseBoxes(s32 arg);
void LoginSe_Decide(void);                        /* lobby: sound effect 0x13 */
void LoginSe_Play9(void);                        /* lobby: sound effect 9 */
void LoginSe_Cancel(void);                        /* lobby: sound effect 0x14 */
void LoginSe_Cursor(void);                        /* lobby: sound effect 0x17 */
void Callbacks_Add(void (*fn)(void));           /* sub_main */
/* lobby: the software keyboard and more */
s32 NetKbd_Run(void);
void NetKbd_Open(s32 arg0, s32 arg1, void *arg2, s32 arg3, s32 arg4);
char *NetKbd_GetText(void);
void NetKbd_SetActive(s32 on);
void NetKbd_Draw(void);
/* more of main and sub_main */
s32 Gs_SetState(u8 reg, u32 v);
void Font_DrawPage(s32 layer);
void Gfx_SetTexFilter(s32 a);
void Text_Reset(void);
void Text_DrawIcons(s32 a);
/* sub_main: the network connection settings */
extern char gNetAuthKey[0x100];
extern char gNetAuthName[0x100];
extern char gNetDomain[0x100];
extern char gNetPppAuthText[0x10];
extern char gNetPppConfBuf[0xF0];
extern char D_59DD00[0x100];
extern char gHttpsProxy[0x110];         /* the HTTPS proxy URL */
extern char gHttpProxy[0x110];         /* the HTTP proxy URL */
void Window_DrawTextAlpha(); /* unit's own declaration: the definition (sub_main) takes (Window *, u8 *text, u8 alpha); Yn_DrawBackground also passes &rect in a3, which it ignores (a native build drops it), and a fixed 4-parameter type reschedules that function */
s32 Prim_FillRect(PrimLine *rect);
void Hud_DrawButtonIcon(s16 id, s16 x, s16 y, s16 depth, u32 color, f32 scale); /* unit's own declaration:
 * the definition (sub_main) takes s32 id, x, y, depth; Yn_DrawHelpBar narrows its arguments to
 * 16 bits before the call, as the game's code does, only with these s16 parameters */
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);               /* draws a box */
s16 Font_GetAdvance(void);
s16 Font_GetCharH(void);
f32 Math_Sin(f32 x);                        /* sinf */
s32 Dma_WaitIdle(void);
void Pad_End(void);
void DisableDmac(s32 a);
void DisableIntc(s32 a);
void sceCdInit(s32 a);
void sceSifExitCmd(void);
void func_11DD10(char *path, s32 argc, char **argv);
void sceGsSyncPath(s32 mode, u16 timeout);
void DIntr(void);
void EIntr(void);
s32 RemoveDmacHandler(s32 channel, s32 id);
s32 RemoveIntcHandler(s32 cause, s32 id);
void *sceGsSyncVCallback(void *func);
/* Sony's netcnfif (netcnfif.s) */
s32 yn_entry(void);
s32 func_00AC8128(void);
s32 sceNetcnfifCheck(void);
s32 sceNetcnfifGetResult(s32 *result);
void sceNetcnfifSetFNoDecode(s32 flag);
void sceNetcnfifGetCount(char *path, s32 type);
void sceNetcnfifGetList(char *path, s32 type, NetcnfEntry *list, s32 count);
s32 sceNetcnfifLoadEntry(char *path, s32 type, char *usrName, NetcnfData *data);
s32 sceNetcnfifAllocWorkarea(s32 size);
s32 sceNetcnfifFreeWorkarea(void);
void func_00AC8DC0(void);
void sceNetcnfifDataInit(NetcnfData *data);
s32 sceNetcnfifConvAuthname(char *src, char *dst, s32 size);
/* this file */
void Yn_ConfigToIfc(YnConfig *cfg, NetcnfData *data, char *name);
void Yn_ConfigToDev(YnConfig *cfg, NetcnfData *data);
void Yn_DrawMessageText(void);
s32 Yn_ScanCardsForConnect(void);
s32 Yn_Connect(void);
void Yn_ResetStreams(void);
void Yn_Draw(void);
void Yn_Quit(void);
s32 Yn_ScanCards(void);
s32 Yn_UpdateEntryList(void);
void Yn_UpdatePadRepeat(PadState *pad);
void Yn_PreUpdate(void);
void Yn_LoadProxy(void);
s32 YnNetcnf_LoadModules(YnNetcnf *n);
s32 YnNetcnf_Open(YnNetcnf *n);
void Yn_ResetNetDevices(void);
s32 Yn_IsCurrentCardGone(void);
void Yn_StrCopy(char *dst, char *src);
void Yn_DrawProxy(void);
void Yn_AlignAddress(u8 *dst, u8 *src);
s32 Yn_IsCardGone(void);
void Yn_ExportSettings(NetcnfData *d);
void Yn_ApplyEntry(NetcnfData *data);
s32 Yn_FindCombination(YnConfig *cfg, YnCombo *combo, s32 n);
s32 Yn_CompareProxy(YnEntryKey *a, YnEntryKey *b);
s32 YnNetcnf_IsEmpty(YnNetcnf *n);
s32 Yn_SelectDevice(s32 i);
void Yn_DrawSlotMessage(s32 sel);
void Yn_DrawEntryDetails(void);
s16 Yn_CenterX(char *text, s32 w);
void Yn_DrawIcon(f32 scale, s32 id, s32 x, s32 y, s32 blink);
void YnNetcnf_SetPath(YnNetcnf *n, s32 slot);
s32 YnNetcnf_CheckFile(YnNetcnf *n);
void YnNetcnf_LoadCombos(YnNetcnf *n);
void Yn_BuildCombos(YnNetcnf *n, YnCombo *combo);
void Yn_PreDraw(void);
void Yn_DrawScreen(void);
void Yn_DrawItems(void);
void Yn_DrawChoiceItems(s32 sel, s32 color);
void Yn_DrawEntryList(void);
void Yn_DrawPort(u16 *value, s16 x, s16 y);
void Yn_DrawTextField(u8 *text, char *label, s16 x, s16 y, s32 w);
void Yn_DrawMessage(void);
void Yn_DrawHelp(void);
void Yn_Print(s16 x, s16 y, char *text);
void Yn_SetColor(s8 color);
void Yn_SetFontSize(s32 w, s32 h);
void Yn_SetGlyphSize(f32 scale);
void Yn_SetTextMode(s8 mode);
s32 Yn_GetTextMode(void);
s16 Yn_CenterXSjis(char *text, s32 w);
void Yn_CopyForPrint(char *dst, u8 *src, s32 max);
void Yn_DrawChoiceCursor(void);
void Yn_IfcToConfig(YnConfig *cfg, NetcnfData *data, char *name);
void Yn_DevToConfig(YnConfig *cfg, NetcnfData *data);
void Yn_DrawTextLayer(void);
void Yn_DrawBackground(void);
void Yn_DrawHelpBar(s32 arg);
s32 YnNetcnf_Close(YnNetcnf *n);
char *YnNetcnf_FindUsrName(YnNetcnf *n, char *name);
s32 Yn_Init(s32 arg);
/* yn_card.c */
void YnCard_Reset(YnCardCheck *c);
void YnCard_SetSlot(YnCardCheck *c, u8 slot);
s32 YnCard_ListNetcnfDir(YnCardCheck *c);
s32 YnCard_CheckSlots(YnCardCheck *c);
s32 YnCard_CheckCurrentSlot(YnCardCheck *c);
s32 YnCard_GetSlot(YnCardCheck *c);
void YnNetcnf_SetType(YnNetcnf *n, s32 type);
s32 YnNetcnf_GetCount(YnNetcnf *n);
s32 YnNetcnf_GetList(YnNetcnf *n);
s32 YnNetcnf_LoadFirst(YnNetcnf *n);
void Yn_ParseAddress(u8 *str, u8 *ip);
void Yn_FormatAddress(char *str, u8 *ip);
s32 Yn_IsAddressSet(u8 *ip);
s32 Yn_FindDevice(char *vendor, char *product);
s32 Yn_GetDeviceKind(char *vendor, char *product);
void Yn_Utf8ToSjis(char *dst, char *src);
void Yn_SjisToUtf8(char *dst, char *src);
void Yn_UnloadModule(s32 id);
void Yn_LoadModule(char *path, s32 argc, char *argv, void *buf, s32 tries);

/* The busy wait in the IRX loaders' retry loops: the original has eight
 * nops as the loop body. */
#define YN_WAIT() __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop")
#define YN_WAIT7() __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop")

extern NetDevice *gNetDevCur;
#define gNetDevice (gNetDevCur)   /* sub_main: the device in use */

/* .bss, declared from the highest address down (MWCC emits them last-declared
 * first). gYnWorkBuf is the work area gYnWork points to (0x1AF00 bytes
 * cleared by Yn_Init) and what follows it. */
u8 gYnWorkBuf[0x40500];
u8 gYnSetupGuideArg[0x130];

/* .data: the screens' text tables. */
YnWindow gYnWindows[4] = {
#include "yn/00AD11A0.inc"
};
YnWindow gYnProxyWindows[3] = {
#include "yn/00AD11F0.inc"
};
u32 gYnStorageNames[4] = {
#include "yn/00AD1230.inc"
};
char *gYnHelpBarLabels[12] = {
#include "yn/00AD1240.inc"
};
u32 gYnEtherModeTexts[8] = {
#include "yn/00AD1270.inc"
};
/* The space and the empty text come here in the game's .rodata, before the
 * next table's strings, although the tables that use the space come later
 * and no code uses the empty text: they were probably first used by a
 * function the linker dropped. */
const char gYnSpaceText[2] = {
#include "yn/00AD28D8.inc"
};
const char gYnEmptyText[1] = {
#include "yn/00AD28E0.inc"
};
u32 gYnFieldHeaders[6] = {
#include "yn/00AD1290.inc"
};
u32 gYnHttpsTexts[2] = {
#include "yn/00AD12A8.inc"
};
u32 gYnDeviceTexts[2] = {
#include "yn/00AD12B0.inc"
};
u32 gYnUserIdTexts[2] = {
#include "yn/00AD12B8.inc"
};
u32 gYnHostIpTexts[2] = {
#include "yn/00AD12C0.inc"
};
u32 gYnNetmaskTexts[2] = {
#include "yn/00AD12C8.inc"
};
u32 gYnDnsTexts[2] = {
#include "yn/00AD12D0.inc"
};
u32 gYnYesNoTexts[2] = {
#include "yn/00AD12D8.inc"
};
u32 gYnNoTexts[4] = {
#include "yn/00AD12E0.inc"
};
u32 gYnOtherPs2Lines[6] = {
#include "yn/00AD12F0.inc"
};
u32 gYnNoCardLines[3] = {
#include "yn/00AD1308.inc"
};
u32 gYnNoSettingsLines[3] = {
#include "yn/00AD1318.inc"
};
YnItem gYnScreenNoneItems[1] = {
    {0, 0, (char *)gYnSpaceText},
};
YnItem gYnScreenListItems[4] = {
#include "yn/00AD1330.inc"
};
YnItem gYnScreenItems_02[4] = {
#include "yn/00AD1350.inc"
};
YnItem gYnScreenItems_03[2] = {
#include "yn/00AD1370.inc"
};
YnItem gYnScreenItems_04[2] = {
#include "yn/00AD1380.inc"
};
YnItem gYnScreenItems_05[2] = {
#include "yn/00AD1390.inc"
};
YnItem gYnScreenItems_06[2] = {
#include "yn/00AD13A0.inc"
};
YnItem gYnScreenItems_07[2] = {
#include "yn/00AD13B0.inc"
};
YnItem gYnScreenItems_08[6] = {
#include "yn/00AD13C0.inc"
};
YnItem gYnScreenItems_09[4] = {
#include "yn/00AD13F0.inc"
};
YnItem gYnScreenItems_0A[6] = {
#include "yn/00AD1410.inc"
};
YnItem gYnScreenItems_0B[4] = {
#include "yn/00AD1440.inc"
};
YnItem gYnScreenItems_0C[4] = {
#include "yn/00AD1460.inc"
};
YnItem gYnScreenItems_0D[4] = {
#include "yn/00AD1480.inc"
};
YnItem gYnScreenItems_0E[2] = {
#include "yn/00AD14A0.inc"
};
YnItem gYnScreenItems_0F[4] = {
#include "yn/00AD14B0.inc"
};
YnItem gYnScreenItems_10[2] = {
#include "yn/00AD14D0.inc"
};
YnItem gYnScreenItems_11[2] = {
#include "yn/00AD14E0.inc"
};
YnItem gYnScreenItems_12[2] = {
#include "yn/00AD14F0.inc"
};
YnItem gYnScreenItems_15[2] = {
#include "yn/00AD1500.inc"
};
YnItem gYnScreenProxyUseItems[4] = {
#include "yn/00AD1510.inc"
};
YnItem gYnScreenProxyFieldsItems[6] = {
#include "yn/00AD1530.inc"
};
YnItem gYnScreenItems_18[2] = {
#include "yn/00AD1560.inc"
};
YnItem *gYnScreenItems[28] = {
    gYnScreenNoneItems, gYnScreenListItems, gYnScreenItems_02, gYnScreenItems_03,
    gYnScreenItems_04, gYnScreenItems_05, gYnScreenItems_06, gYnScreenItems_07,
    gYnScreenItems_08, gYnScreenItems_09, gYnScreenItems_0A, gYnScreenItems_0B,
    gYnScreenItems_0C, gYnScreenItems_0D, gYnScreenItems_0E, gYnScreenItems_0F,
    gYnScreenItems_10, gYnScreenItems_11, gYnScreenItems_12, gYnScreenNoneItems,
    gYnScreenNoneItems, gYnScreenItems_15, gYnScreenProxyUseItems, gYnScreenProxyFieldsItems,
    gYnScreenItems_18, NULL, NULL, NULL,
};
s8 gYnScreenItemCounts[32] = {
#include "yn/00AD15E0.inc"
};
YnItem gYnMsgItems_00[2] = {
    {0, 0, (char *)gYnSpaceText},
    {0, 0, NULL},
};
YnItem gYnMsgItems_01[4] = {
#include "yn/00AD1610.inc"
};
YnItem gYnMsgItems_02[6] = {
#include "yn/00AD1630.inc"
};
YnItem gYnMsgItems_03[6] = {
#include "yn/00AD1660.inc"
};
YnItem gYnMsgDeviceUnknownItems[4] = {
#include "yn/00AD1690.inc"
};
YnItem gYnMsgDeviceAbsentItems[4] = {
#include "yn/00AD16B0.inc"
};
YnItem gYnMsgItems_06[2] = {
#include "yn/00AD16D0.inc"
};
YnItem gYnMsgItems_07[2] = {
#include "yn/00AD16E0.inc"
};
YnItem gYnMsgItems_08[6] = {
#include "yn/00AD16F0.inc"
};
YnItem gYnMsgItems_09[2] = {
#include "yn/00AD1720.inc"
};
YnItem gYnMsgItems_0A[6] = {
#include "yn/00AD1730.inc"
};
YnItem gYnMsgItems_0B[4] = {
#include "yn/00AD1760.inc"
};
YnItem gYnMsgItems_0C[2] = {
#include "yn/00AD1780.inc"
};
YnItem gYnMsgItems_0D[2] = {
#include "yn/00AD1790.inc"
};
YnItem gYnMsgItems_0E[4] = {
#include "yn/00AD17A0.inc"
};
YnItem gYnMsgItems_0F[2] = {
#include "yn/00AD17C0.inc"
};
YnItem gYnMsgItems_10[4] = {
#include "yn/00AD17D0.inc"
};
YnItem gYnMsgItems_11[4] = {
#include "yn/00AD17F0.inc"
};
YnItem gYnMsgItems_12[4] = {
#include "yn/00AD1810.inc"
};
YnItem gYnMsgItems_13[1] = {
#include "yn/00AD1830.inc"
};
YnItem gYnMsgItems_14[1] = {
#include "yn/00AD1838.inc"
};
YnItem gYnMsgItems_15[1] = {
#include "yn/00AD1840.inc"
};
YnItem gYnMsgItems_16[1] = {
#include "yn/00AD1848.inc"
};
YnItem gYnMsgItems_17[4] = {
#include "yn/00AD1850.inc"
};
YnItem gYnMsgProxyConfirmItems[3] = {
#include "yn/00AD1870.inc"
};
YnItem gYnMsgEntryDetailsItems[1] = {
#include "yn/00AD1888.inc"
};
YnItem gYnMsgQuitAskItems[4] = {
#include "yn/00AD1890.inc"
};
YnItem gYnMsgDialUpItems[2] = {
#include "yn/00AD18B0.inc"
};
YnItem gYnMsgItems_1C[4] = {
#include "yn/00AD18C0.inc"
};
YnItem gYnMsgFileErrorItems[4] = {
#include "yn/00AD18E0.inc"
};
YnItem gYnMsgItems_1E[2] = {
#include "yn/00AD1900.inc"
};
YnItem gYnMsgItems_1F[6] = {
#include "yn/00AD1910.inc"
};
YnItem gYnMsgItems_20[4] = {
#include "yn/00AD1940.inc"
};
YnItem gYnMsgItems_21[4] = {
#include "yn/00AD1960.inc"
};
YnItem gYnMsgItems_22[4] = {
#include "yn/00AD1980.inc"
};
YnItem gYnMsgItems_23[4] = {
#include "yn/00AD19A0.inc"
};
YnItem gYnMsgItems_24[4] = {
#include "yn/00AD19C0.inc"
};
YnItem gYnMsgItems_25[8] = {
#include "yn/00AD19E0.inc"
};
YnItem gYnMsgItems_26[6] = {
#include "yn/00AD1A20.inc"
};
YnItem gYnMsgItems_27[6] = {
#include "yn/00AD1A50.inc"
};
YnItem gYnMsgItems_28[6] = {
#include "yn/00AD1A80.inc"
};
YnItem gYnMsgItems_29[6] = {
#include "yn/00AD1AB0.inc"
};
YnItem gYnMsgItems_2A[4] = {
#include "yn/00AD1AE0.inc"
};
YnItem gYnMsgItems_2B[4] = {
#include "yn/00AD1B00.inc"
};
YnItem gYnMsgItems_2C[2] = {
#include "yn/00AD1B20.inc"
};
YnItem gYnMsgItems_2D[4] = {
#include "yn/00AD1B30.inc"
};
YnItem gYnMsgItems_2E[6] = {
#include "yn/00AD1B50.inc"
};
YnItem gYnMsgItems_2F[4] = {
#include "yn/00AD1B80.inc"
};
YnItem gYnMsgItems_30[6] = {
#include "yn/00AD1BA0.inc"
};
YnItem gYnMsgItems_31[6] = {
#include "yn/00AD1BD0.inc"
};
YnItem gYnMsgProxySavedItems[2] = {
#include "yn/00AD1C00.inc"
};
YnItem gYnMsgNoNameItems[4] = {
#include "yn/00AD1C10.inc"
};
YnItem gYnMsgConnectedItems[4] = {
#include "yn/00AD1C30.inc"
};
YnItem gYnMsgItems_35[6] = {
#include "yn/00AD1C50.inc"
};
YnItem gYnMsgItems_36[4] = {
#include "yn/00AD1C80.inc"
};
YnItem gYnMsgItems_37[6] = {
#include "yn/00AD1CA0.inc"
};
YnItem gYnMsgItems_38[4] = {
#include "yn/00AD1CD0.inc"
};
YnItem gYnMsgItems_39[2] = {
#include "yn/00AD1CF0.inc"
};
YnItem *gYnMsgItems[60] = {
    gYnMsgItems_00, gYnMsgItems_01, gYnMsgItems_02, gYnMsgItems_03,
    gYnMsgDeviceUnknownItems, gYnMsgDeviceAbsentItems, gYnMsgItems_06, gYnMsgItems_07,
    gYnMsgItems_08, gYnMsgItems_09, gYnMsgItems_0A, gYnMsgItems_0B,
    gYnMsgItems_0C, gYnMsgItems_0D, gYnMsgItems_0E, gYnMsgItems_0F,
    gYnMsgItems_10, gYnMsgItems_11, gYnMsgItems_12, gYnMsgItems_13,
    gYnMsgItems_14, gYnMsgItems_15, gYnMsgItems_16, gYnMsgItems_17,
    gYnMsgProxyConfirmItems, gYnMsgEntryDetailsItems, gYnMsgQuitAskItems, gYnMsgDialUpItems,
    gYnMsgItems_1C, gYnMsgFileErrorItems, gYnMsgItems_1E, gYnMsgItems_1F,
    gYnMsgItems_20, gYnMsgItems_21, gYnMsgItems_22, gYnMsgItems_23,
    gYnMsgItems_24, gYnMsgItems_25, gYnMsgItems_26, gYnMsgItems_27,
    gYnMsgItems_28, gYnMsgItems_29, gYnMsgItems_2A, gYnMsgItems_2B,
    gYnMsgItems_2C, gYnMsgItems_2D, gYnMsgItems_2E, gYnMsgItems_2F,
    gYnMsgItems_30, gYnMsgItems_31, gYnMsgProxySavedItems, gYnMsgNoNameItems,
    gYnMsgConnectedItems, gYnMsgItems_35, gYnMsgItems_36, gYnMsgItems_37,
    gYnMsgItems_38, gYnMsgItems_39, NULL, NULL,
};
s8 gYnMsgItemCounts[64] = {
#include "yn/00AD1DF0.inc"
};
YnItem gYnHelpItems_00[2] = {
    {0, 0, (char *)gYnSpaceText},
    {0, 0, NULL},
};
YnItem gYnHelpListItems[2] = {
#include "yn/00AD1E40.inc"
};
YnItem gYnHelpItems_02[4] = {
#include "yn/00AD1E50.inc"
};
YnItem gYnHelpItems_03[2] = {
#include "yn/00AD1E70.inc"
};
YnItem gYnHelpItems_04[4] = {
#include "yn/00AD1E80.inc"
};
YnItem gYnHelpItems_05[2] = {
#include "yn/00AD1EA0.inc"
};
YnItem gYnHelpItems_06[2] = {
#include "yn/00AD1EB0.inc"
};
YnItem gYnHelpItems_07[2] = {
#include "yn/00AD1EC0.inc"
};
YnItem gYnHelpItems_08[2] = {
#include "yn/00AD1ED0.inc"
};
YnItem gYnHelpItems_0D[4] = {
#include "yn/00AD1EE0.inc"
};
YnItem gYnHelpItems_0E[2] = {
#include "yn/00AD1F00.inc"
};
YnItem gYnHelpItems_0F[2] = {
#include "yn/00AD1F10.inc"
};
YnItem gYnHelpItems_10[2] = {
#include "yn/00AD1F20.inc"
};
YnItem gYnHelpItems_11[2] = {
#include "yn/00AD1F30.inc"
};
YnItem gYnHelpItems_12[1] = {
#include "yn/00AD1F40.inc"
};
YnItem gYnHelpItems_13[1] = {
#include "yn/00AD1F48.inc"
};
YnItem gYnHelpItems_14[2] = {
#include "yn/00AD1F50.inc"
};
YnItem gYnHelpItems_15[2] = {
#include "yn/00AD1F60.inc"
};
YnItem gYnHelpItems_16[2] = {
#include "yn/00AD1F70.inc"
};
YnItem gYnHelpItems_17[2] = {
#include "yn/00AD1F80.inc"
};
YnItem gYnHelpItems_18[4] = {
#include "yn/00AD1F90.inc"
};
YnItem gYnHelpItems_19[4] = {
#include "yn/00AD1FB0.inc"
};
YnItem gYnHelpItems_1A[2] = {
#include "yn/00AD1FD0.inc"
};
YnItem gYnHelpItems_1C[2] = {
    {0, 0, (char *)gYnSpaceText},
    {0, 0, NULL},
};
YnItem gYnHelpItems_1D[2] = {
#include "yn/00AD1FF0.inc"
};
YnItem gYnHelpProxyUseItems[2] = {
#include "yn/00AD2000.inc"
};
YnItem gYnHelpProxyHostItems[1] = {
#include "yn/00AD2010.inc"
};
YnItem gYnHelpProxyPortItems[1] = {
#include "yn/00AD2018.inc"
};
YnItem gYnHelpProxyHttpsItems[4] = {
#include "yn/00AD2020.inc"
};
YnItem gYnHelpItems_24[4] = {
#include "yn/00AD2040.inc"
};
YnItem *gYnHelpItems[40] = {
    gYnHelpItems_00, gYnHelpListItems, gYnHelpItems_02, gYnHelpItems_03,
    gYnHelpItems_04, gYnHelpItems_05, gYnHelpItems_06, gYnHelpItems_07,
    gYnHelpItems_08, gYnHelpItems_08, gYnHelpItems_08, gYnHelpItems_08,
    gYnHelpItems_08, gYnHelpItems_0D, gYnHelpItems_0E, gYnHelpItems_0F,
    gYnHelpItems_10, gYnHelpItems_11, gYnHelpItems_12, gYnHelpItems_13,
    gYnHelpItems_14, gYnHelpItems_15, gYnHelpItems_16, gYnHelpItems_17,
    gYnHelpItems_18, gYnHelpItems_19, gYnHelpItems_1A, gYnHelpItems_08,
    gYnHelpItems_1C, gYnHelpItems_1D, gYnHelpItems_08, gYnHelpProxyUseItems,
    gYnHelpProxyHostItems, gYnHelpProxyPortItems, gYnHelpProxyHttpsItems, gYnHelpProxyPortItems,
    gYnHelpItems_24, NULL, NULL, NULL,
};
s8 gYnHelpItemCounts[40] = {
#include "yn/00AD2100.inc"
};

/*
 * Points gYnWork at gYnWorkBuf, clears the work area and sets state 0.
 * arg is kept in initArg: 0 runs the settings list (and calls
 * NetMode_OpenIcon7), nonzero connects with the card's settings.
 * Returns 0.
 */
s32 Yn_Init(s32 arg)
{
    gYnWork = (YnWork *)gYnWorkBuf;
    memset(gYnWork, 0, sizeof(YnWork));
    gYnWork->state = YN_STATE_LOAD_MODULES;
    gYnWork->initArg = arg;
    Font_ResetState();
    if (arg == 0) {
        NetMode_OpenIcon7();
    }
    return 0;
}

/*
 * One frame of the overlay's state machine (state): module loading, the
 * card scan, then Yn_Connect (per-slot errors into gNetMode.error) or the
 * settings list and its quit question. Queues Yn_Draw unless ynNoDraw.
 * Returns 0 while running, -1 on quitting, changed + 1 once chosen.
 * The scan's and the connect's failures jump into the success case's
 * tail (`list`, `next`): the game's code shares those tails, and the
 * gotos keep its layout.
 */
s32 Yn_Update(void)
{
    NetModeState *netMode;
    s32 ret;

    netMode = &gNetMode;
    if (gYnWork == NULL) {
        return -1;
    }
    ret = 0;
    Yn_PreUpdate();
    switch (gYnWork->state) {
    case YN_STATE_LOAD_MODULES:
        Yn_ResetStreams();
        Yn_ResetNetDevices();
        YnNetcnf_LoadModules(&gYnWork->netcnf);
        gYnWork->timer = 10;
        gYnWork->state = YN_STATE_OPEN;
        break;
    case YN_STATE_OPEN:
        if (--gYnWork->timer <= 0) {
            YnNetcnf_Open(&gYnWork->netcnf);
            Yn_LoadProxy();
            if (gYnWork->initArg == 0) {
                gYnWork->slot = 0;
                YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
                gYnWork->state = YN_STATE_SCAN;
            } else {
                gYnWork->state = YN_STATE_CONNECT;
                netMode->trySlot = 0;
                gYnWork->slot = netMode->trySlot;
                YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
                gYnWork->step = YN_CONNECT_STEP_START;
            }
        }
        break;
    case YN_STATE_SCAN:
        gYnWork->timer++;
        gYnWork->result = Yn_ScanCards();
        switch (gYnWork->result) {
        case YN_SCAN_BUSY:
            break;
        case YN_SCAN_DONE:
            gYnWork->scanResult = 0;
        list:
            gYnWork->state = YN_STATE_LIST;
            gYnWork->scanStep = YN_SCANSTEP_START;
            gYnWork->listDetail = 0;
            gYnWork->msg = YN_MSG_NONE;
            break;
        case YN_SCAN_NO_SETTINGS:
            gYnWork->scanResult = YN_SCAN_NO_SETTINGS;
            goto list;
        case YN_SCAN_CARD_CHANGED:
            gYnWork->scanResult = YN_SCAN_CARD_CHANGED;
            goto list;
        case YN_SCAN_OTHER_PS2:
            gYnWork->scanResult = YN_SCAN_OTHER_PS2;
            goto list;
        }
        break;
    case YN_STATE_WAIT_CARD:
        gYnWork->timer++;
        if (Yn_IsCardGone() != 0) {
            gYnWork->state = YN_STATE_SCAN;
            gYnWork->step = 0;
        }
        break;
    case YN_STATE_CONNECT:
        gYnWork->timer++;
        switch (Yn_Connect()) {
        case YN_CONNECT_DONE:
            netMode->error[netMode->trySlot] = NET_YN_ERR_NONE;
            gYnWork->state = YN_STATE_FINISHED;
            netMode->ynResult = gYnWork->changed + 1;
            break;
        case YN_CONNECT_ALL_TRIED:
            netMode->error[netMode->trySlot] = NET_YN_ERR_NO_SETTINGS;
            goto next;
        case YN_CONNECT_NO_CARD:
            netMode->error[netMode->trySlot] = NET_YN_ERR_NO_CARD;
            goto next;
        case YN_CONNECT_DEVICE:
            netMode->error[netMode->trySlot] = NET_YN_ERR_DEVICE;
            goto next;
        case YN_CONNECT_PROXY_CHANGED:
            netMode->error[netMode->trySlot] = NET_YN_ERR_NO_SETTINGS;
            goto next;
        case YN_CONNECT_OTHER_PS2:
            netMode->error[netMode->trySlot] = NET_YN_ERR_OTHER_PS2;
            goto next;
        case YN_CONNECT_CARD_REMOVED:
            netMode->error[netMode->trySlot] = NET_YN_ERR_CARD_REMOVED;
            goto next;
        case YN_CONNECT_EMPTY_FILE:
            netMode->error[netMode->trySlot] = NET_YN_ERR_EMPTY_FILE;
            goto next;
        case YN_CONNECT_NO_MATCH:
            netMode->error[netMode->trySlot] = NET_YN_ERR_NO_SETTINGS;
        next:
            if (netMode->trySlot == 0) {
                netMode->trySlot = 1;
                gYnWork->slot = netMode->trySlot;
                YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
                gYnWork->scanStep = YN_SCANSTEP_START;
                gYnWork->step = YN_CONNECT_STEP_SCAN;
            } else {
                gYnWork->state = YN_STATE_FINISHED;
                netMode->ynResult = gYnWork->changed + 1;
            }
            break;
        }
        break;
    case YN_STATE_LIST:
        gYnWork->timer++;
        gYnWork->numEditing = 0;
        Yn_UpdatePadRepeat(gPadState);
        switch (Yn_UpdateEntryList()) {
        case 0:
            break;
        case 1:
            gYnWork->state = YN_STATE_CHOSEN;
            gYnWork->step = 0;
            break;
        case -1:
            gYnWork->state = YN_STATE_QUIT_ASK;
            gYnWork->step = 0;
            gYnWork->msg = YN_MSG_QUIT_ASK;
            gYnWork->quitAsked = 1;
            gYnWork->listDetail = 0;
            gYnWork->quitChoice = 1;
            LoginSe_Play9();
            break;
        case -2:
            gYnWork->state = YN_STATE_SCAN;
            gYnWork->step = 0;
            break;
        }
        break;
    case YN_STATE_CHOSEN:
        Yn_Quit();
        netMode->ynResult = gYnWork->changed + 1;
        ret = netMode->ynResult;
        break;
    case YN_STATE_QUIT_ASK:
        gYnWork->helpBar = YN_HELPBAR_MESSAGE;
        gYnWork->msgCloseMode = YN_MSG_CLOSE_EITHER;
        gYnWork->timer++;
        if (gPadState[0].pressed & PAD_CROSS) {
            gYnWork->msg = YN_MSG_NONE;
            gYnWork->quitAsked = 0;
            LoginSe_Cancel();
            gYnWork->state = YN_STATE_LIST;
        } else if (gPadState[0].pressed & PAD_CIRCLE) {
            gYnWork->quitAsked = 0;
            if (gYnWork->quitChoice == 0) {
                LoginSe_Decide();
                Yn_Quit();
                ret = -1;
            } else {
                gYnWork->msg = YN_MSG_NONE;
                LoginSe_Cancel();
                gYnWork->state = YN_STATE_LIST;
            }
        } else if (gPadState[0].pressed & (PAD_LEFT | PAD_RIGHT)) {
            gYnWork->quitChoice ^= 1;
            LoginSe_Cursor();
        }
        break;
    case YN_STATE_ERROR_QUIT:
        gYnWork->msgCloseMode = YN_MSG_CLOSE_EITHER;
        gYnWork->timer++;
        if ((gPadState[0].pressed & PAD_CIRCLE) || (gPadState[0].pressed & PAD_CROSS)) {
            LoginSe_Decide();
            Yn_Quit();
            ret = -1;
        }
        break;
    case YN_STATE_MESSAGE:
        if ((gPadState[0].pressed & PAD_CIRCLE) || (gPadState[0].pressed & PAD_CROSS)) {
            gYnWork->state = YN_STATE_LIST;
            gYnWork->msg = YN_MSG_NONE;
            LoginSe_Decide();
        }
        break;
    }
    if (gNetMode.ynNoDraw != 0) {
        return ret;
    }
    Callbacks_Add(Yn_Draw);
    return ret;
}

/*
 * Draw callback Yn_Update queues: the background, the current screen, the
 * help bar, the message and help lines and the text layer, with the text
 * mode saved and restored around them.
 */
void Yn_Draw(void)
{
    Yn_DrawBackground();
    gYnWork->textMode = Yn_GetTextMode();
    Yn_SetTextMode(1);
    Yn_SetColor(0);
    Yn_SetFontSize(0x16, 0x16);
    Yn_SetGlyphSize(621.4285888671875f);
    Yn_PreDraw();
    Yn_DrawScreen();
    Yn_DrawHelpBar(gYnWork->helpBar);
    Yn_DrawMessage();
    Yn_DrawHelp();
    Yn_DrawTextLayer();
    Yn_SetTextMode(gYnWork->textMode);
}

/*
 * Leaves the overlay: releases the texture and palette handles in texPal,
 * clears the work area (Yn_Init), closes netcnf and unloads its modules,
 * compacts memory and closes the network boxes (NetMode_CloseBoxes).
 */
void Yn_Quit(void)
{
    u32 palHandle;

    printf("!!!\n");
    if ((u16)gYnWork->texPal != 0) {
        flReleaseTextureHandle((u16)gYnWork->texPal);
    }
    palHandle = (gYnWork->texPal & 0xFFFF0000) >> 16;
    if (palHandle != 0) {
        flReleasePaletteHandle(palHandle);
    }
    Yn_Init(0);
    YnNetcnf_Close(&gYnWork->netcnf);
    Mem_Compact();
    NetMode_CloseBoxes(0x20);
}

/*
 * Looks for the memory card slot to read the settings from (scanStep's
 * YN_SCANSTEP_*), then lists the file's combinations. Returns YN_SCAN_BUSY
 * while scanning, YN_SCAN_DONE with the combinations built, else a failure
 * (YN_SCAN_NO_SETTINGS, YN_SCAN_CARD_CHANGED, YN_SCAN_OTHER_PS2).
 * The slot test is written `!(...) != 0`: the game's code computes the
 * negation into a register before branching, which a plain `!(...)`
 * doesn't give. The slot search is a goto back to `loop`: a while loop
 * compiles to a different branch layout.
 */
s32 Yn_ScanCards(void)
{
    s32 ret;
    s32 err;

    switch (gYnWork->scanStep) {
    case YN_SCANSTEP_START:
        gYnWork->scanStep++;
        gYnWork->prevSlot = YnCard_GetSlot(&gYnWork->card);
        gYnWork->cardSlots = 0;
        gYnWork->entryCount = 0;
        gYnWork->unk1F = 0;
        gYnWork->scanSlot = gYnWork->slot;
        YnCard_Reset(&gYnWork->card);
        memset(&gYnWork->netcnf, 0, sizeof(YnNetcnf));
        memset(gYnWork->combos, 0, sizeof(gYnWork->combos));
    case YN_SCANSTEP_CHECK_SLOTS:
        if (YnCard_CheckSlots(&gYnWork->card) < 0) {
            break;
        }
        gYnWork->cardSlots |= ((gYnWork->card.okSlots | gYnWork->card.slots3) >> 1) & 3;
        if (gYnWork->cardSlots & (1 << gYnWork->prevSlot)) {
            gYnWork->scanStep = YN_SCANSTEP_PICK_SLOT;
            break;
        }
        gYnWork->scanStep = YN_SCANSTEP_START;
        gYnWork->slot = gYnWork->prevSlot;
        YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
        return YN_SCAN_CARD_CHANGED;
    case YN_SCANSTEP_PICK_SLOT:
    case YN_SCANSTEP_NEXT_SLOT:
    loop:
        if (!(gYnWork->cardSlots & (1 << gYnWork->scanSlot)) != 0) {
            gYnWork->scanSlot = (gYnWork->scanSlot + 1) % 2;
            if (gYnWork->scanSlot != gYnWork->slot) {
                goto loop;
            }
        }
        if (gYnWork->scanStep == YN_SCANSTEP_NEXT_SLOT && gYnWork->scanSlot == gYnWork->slot) {
            gYnWork->scanStep = YN_SCANSTEP_START;
            gYnWork->slot = gYnWork->scanSlot;
            YnCard_SetSlot(&gYnWork->card, gYnWork->scanSlot);
            return YN_SCAN_NO_SETTINGS;
        }
        gYnWork->scanStep = YN_SCANSTEP_LIST_DIR;
        gYnWork->slot = gYnWork->scanSlot;
        YnCard_SetSlot(&gYnWork->card, gYnWork->scanSlot);
        break;
    case YN_SCANSTEP_LIST_DIR:
        ret = YnCard_ListNetcnfDir(&gYnWork->card);
        if (ret != -2) {
            if (ret != -1) {
                gYnWork->scanStep = YN_SCANSTEP_NEXT_SLOT;
            } else {
                gYnWork->scanStep = YN_SCANSTEP_READ_FILE;
            }
        }
        break;
    case YN_SCANSTEP_READ_FILE:
        YnNetcnf_SetPath(&gYnWork->netcnf, YnCard_GetSlot(&gYnWork->card));
        gYnWork->entryCount = 0;
        if (YnNetcnf_CheckFile(&gYnWork->netcnf) != 0) {
            return YN_SCAN_OTHER_PS2;
        }
        err = 0;
        YnNetcnf_SetType(&gYnWork->netcnf, 0);
        if ((gYnWork->entryCount = YnNetcnf_GetCount(&gYnWork->netcnf)) == 0) {
            err = 1;
        }
        YnNetcnf_GetList(&gYnWork->netcnf);
        YnNetcnf_SetType(&gYnWork->netcnf, 2);
        if (YnNetcnf_GetCount(&gYnWork->netcnf) == 0) {
            err = 1;
        }
        YnNetcnf_GetList(&gYnWork->netcnf);
        YnNetcnf_SetType(&gYnWork->netcnf, 1);
        if (YnNetcnf_GetCount(&gYnWork->netcnf) == 0) {
            err = 1;
        }
        YnNetcnf_GetList(&gYnWork->netcnf);
        if (err == 0) {
            YnNetcnf_LoadCombos(&gYnWork->netcnf);
            Yn_BuildCombos(&gYnWork->netcnf, gYnWork->combos);
            gYnWork->scanStep = YN_SCANSTEP_START;
            return YN_SCAN_DONE;
        }
        gYnWork->scanStep = YN_SCANSTEP_START;
        return YN_SCAN_NO_SETTINGS;
    }
    return YN_SCAN_BUSY;
}

/*
 * Yn_ScanCards for Yn_Connect: the same scan, but a failed file check is
 * also stored in cardError[slot]. Returns the same YN_SCAN_* results.
 * The slot search keeps the same goto as Yn_ScanCards.
 */
s32 Yn_ScanCardsForConnect(void)
{
    s32 listResult;
    s32 missing;

    switch (gYnWork->scanStep) {
    case YN_SCANSTEP_START:
        gYnWork->scanStep++;
        gYnWork->prevSlot = YnCard_GetSlot(&gYnWork->card);
        gYnWork->cardSlots = 0;
        gYnWork->entryCount = 0;
        gYnWork->unk1F = 0;
        gYnWork->scanSlot = gYnWork->slot;
        YnCard_Reset(&gYnWork->card);
        memset(&gYnWork->netcnf, 0, sizeof(YnNetcnf));
        memset(gYnWork->combos, 0, sizeof(gYnWork->combos));
    case YN_SCANSTEP_CHECK_SLOTS:
        if (YnCard_CheckSlots(&gYnWork->card) < 0) {
            break;
        }
        gYnWork->cardSlots |= ((gYnWork->card.okSlots | gYnWork->card.slots3) >> 1) & 3;
        if (gYnWork->cardSlots & (1 << gYnWork->prevSlot)) {
            gYnWork->scanStep = YN_SCANSTEP_PICK_SLOT;
            break;
        }
        gYnWork->scanStep = YN_SCANSTEP_START;
        gYnWork->slot = gYnWork->prevSlot;
        YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
        return YN_SCAN_CARD_CHANGED;
    case YN_SCANSTEP_PICK_SLOT:
    case YN_SCANSTEP_NEXT_SLOT:
    loop:
        if (!(gYnWork->cardSlots & (1 << gYnWork->scanSlot)) != 0) {
            gYnWork->scanSlot = (gYnWork->scanSlot + 1) % 2;
            if (gYnWork->scanSlot != gYnWork->slot) {
                goto loop;
            }
        }
        if (gYnWork->scanStep == YN_SCANSTEP_NEXT_SLOT && gYnWork->scanSlot == gYnWork->slot) {
            gYnWork->scanStep = YN_SCANSTEP_START;
            gYnWork->slot = gYnWork->scanSlot;
            YnCard_SetSlot(&gYnWork->card, gYnWork->scanSlot);
            return YN_SCAN_NO_SETTINGS;
        }
        gYnWork->scanStep = YN_SCANSTEP_LIST_DIR;
        gYnWork->slot = gYnWork->scanSlot;
        YnCard_SetSlot(&gYnWork->card, gYnWork->scanSlot);
        break;
    case YN_SCANSTEP_LIST_DIR:
        listResult = YnCard_ListNetcnfDir(&gYnWork->card);
        if (listResult != -2) {
            if (listResult != -1) {
                gYnWork->scanStep = YN_SCANSTEP_NEXT_SLOT;
            } else {
                gYnWork->scanStep = YN_SCANSTEP_READ_FILE;
            }
        }
        break;
    case YN_SCANSTEP_READ_FILE:
        YnNetcnf_SetPath(&gYnWork->netcnf, YnCard_GetSlot(&gYnWork->card));
        gYnWork->entryCount = 0;
        if (YnNetcnf_CheckFile(&gYnWork->netcnf) != 0) {
            gYnWork->cardError[gYnWork->slot] = YN_SCAN_OTHER_PS2;
            return YN_SCAN_OTHER_PS2;
        }
        missing = 0;
        YnNetcnf_SetType(&gYnWork->netcnf, 0);
        if ((gYnWork->entryCount = YnNetcnf_GetCount(&gYnWork->netcnf)) == 0) {
            missing = 1;
        }
        YnNetcnf_GetList(&gYnWork->netcnf);
        YnNetcnf_SetType(&gYnWork->netcnf, 2);
        if (YnNetcnf_GetCount(&gYnWork->netcnf) == 0) {
            missing = 1;
        }
        YnNetcnf_GetList(&gYnWork->netcnf);
        YnNetcnf_SetType(&gYnWork->netcnf, 1);
        if (YnNetcnf_GetCount(&gYnWork->netcnf) == 0) {
            missing = 1;
        }
        YnNetcnf_GetList(&gYnWork->netcnf);
        if (missing == 0) {
            YnNetcnf_LoadCombos(&gYnWork->netcnf);
            Yn_BuildCombos(&gYnWork->netcnf, gYnWork->combos);
            gYnWork->scanStep = YN_SCANSTEP_START;
            return YN_SCAN_DONE;
        }
        gYnWork->scanStep = YN_SCANSTEP_START;
        return YN_SCAN_NO_SETTINGS;
    }
    return YN_SCAN_BUSY;
}

/*
 * Connects with the settings saved in gNetSaveData.config, looking for them on
 * the card in each slot (step: YN_CONNECT_STEP_*). Returns YN_CONNECT_BUSY
 * while working, YN_CONNECT_DONE once connected, else the failure
 * (YN_CONNECT_*).
 * The chosen combination is reached through a byte offset k added to the
 * work pointer as an integer: that way the compiler keeps the offset in
 * one register across the calls, as the game's code does. The connected
 * message's timer jumps into the PAD_CIRCLE case (`cancel`), which the
 * game's code shares. This function is one of those that need the file's
 * optimization level 4.
 */
s32 Yn_Connect(void)
{
    s32 scanStatus;
    s32 n;
    s32 dev;
    s32 k;

    switch (gYnWork->step) {
    case YN_CONNECT_STEP_START:
        gYnWork->step++;
        gYnWork->slot = 0;
        gYnWork->triedSlots = 0;
        gYnWork->cardError[0] = 0;
        gYnWork->cardError[1] = 0;
    case YN_CONNECT_STEP_CHECK_PROXY:
        if (Yn_CompareProxy((YnEntryKey *)&gYnWork->proxy, (YnEntryKey *)&gNetSaveData.configProxy) < 0) {
            return YN_CONNECT_PROXY_CHANGED;
        }
        gYnWork->step++;
        gYnWork->scanStep = YN_SCANSTEP_START;
        break;
    case YN_CONNECT_STEP_SCAN:
        scanStatus = Yn_ScanCardsForConnect();
        if (scanStatus == YN_SCAN_DONE) {
            n = Yn_FindCombination(&gNetSaveData.config, gYnWork->combos, gYnWork->entryCount);
            if (n >= 0) {
                k = n * sizeof(YnCombo);
                dev = ((YnWork *)(k + (s32)gYnWork))->combos[0].device;
                if (dev < 0) {
                    return YN_CONNECT_DEVICE;
                }
                if (Yn_SelectDevice(dev) < 0) {
                    return YN_CONNECT_DEVICE;
                }
                Yn_ApplyEntry(((YnWork *)(k + (s32)gYnWork))->combos[0].data);
                Yn_ExportSettings(((YnWork *)(k + (s32)gYnWork))->combos[0].data);
                if (gNetMode.ynNoDraw != 0) {
                    gYnWork->step = YN_CONNECT_STEP_START;
                    gYnWork->msg = YN_MSG_NONE;
                    LoginSe_Decide();
                    return YN_CONNECT_DONE;
                }
                gYnWork->step = YN_CONNECT_STEP_CONNECTED;
                gYnWork->msg = YN_MSG_CONNECTED;
                LoginSe_Play9();
                gYnWork->msgTimer = 60;
                break;
            }
        } else if (scanStatus == YN_SCAN_CARD_CHANGED) {
            return YN_CONNECT_NO_CARD;
        } else if (scanStatus == YN_SCAN_BUSY) {
            break;
        }
        if (gYnWork->triedSlots & YN_TRIED_FIRST) {
            if (gYnWork->triedSlots & (1 << gYnWork->slot)) {
                return YN_CONNECT_ALL_TRIED;
            }
        } else {
            gYnWork->triedSlots |= YN_TRIED_FIRST;
        }
        gYnWork->triedSlots |= (u8)(1 << gYnWork->slot);
        if (YnNetcnf_IsEmpty(&gYnWork->netcnf) != 0) {
            return YN_CONNECT_EMPTY_FILE;
        }
        return (YnNetcnf_CheckFile(&gYnWork->netcnf) != 0) ? YN_CONNECT_OTHER_PS2 : YN_CONNECT_NO_MATCH;
    case YN_CONNECT_STEP_NEXT_SLOT:
        gYnWork->step = YN_CONNECT_STEP_SCAN;
        gYnWork->scanStep = YN_SCANSTEP_START;
        gYnWork->slot++;
        gYnWork->slot &= 1;
        break;
    case YN_CONNECT_STEP_WAIT_NEXT:
        if (Yn_IsCardGone() != 0) {
            gYnWork->step = YN_CONNECT_STEP_START;
            gYnWork->msg = YN_MSG_NONE;
            return YN_CONNECT_CARD_REMOVED;
        }
        if ((gPadState[0].pressed & PAD_CIRCLE) || gYnWork->initArg != 0) {
            gYnWork->step = YN_CONNECT_STEP_NEXT_SLOT;
            gYnWork->msg = YN_MSG_NONE;
            LoginSe_Decide();
        }
        break;
    case YN_CONNECT_STEP_CONNECTED:
        if (gNetSaveData.setupResult == 1 && gSysSave.netOptions[1] != 0) {
            if (gYnWork->msgTimer <= 0) {
                goto cancel;
            }
            gYnWork->msgTimer--;
        }
        if (gPadState[0].pressed & PAD_CIRCLE) {
        cancel:
            gYnWork->step = YN_CONNECT_STEP_START;
            gYnWork->msg = YN_MSG_NONE;
            LoginSe_Decide();
            return YN_CONNECT_DONE;
        }
        break;
    }
    return YN_CONNECT_BUSY;
}

/*
 * Restarts the scroll of the cursor entry's name (offset 0, 60 frames to
 * the first step). nameScroll is set when the name, in Shift-JIS, is over
 * 44 bytes; never for kind 2 entries.
 */
void Yn_ResetNameScroll(void)
{
    char buf[0x100];
    char *name;
    s32 i;

    gYnWork->nameScroll = 0;
    gYnWork->nameOfs = 0;
    gYnWork->nameAtEnd = 0;
    gYnWork->nameTimer = 60;
    i = gYnWork->cursor + gYnWork->listTop;
    if (gYnWork->combos[i].kind != 2) {
        name = YnNetcnf_FindUsrName(&gYnWork->netcnf, gYnWork->netcnf.data[i].attachIfc);
        if (name != NULL) {
            Yn_Utf8ToSjis(buf, name);
            if (strlen(buf) > 44) {
                gYnWork->nameScroll = 1;
            }
        }
    }
}

/*
 * Steps the cursor entry's name scroll when nameScroll is set: each time
 * nameTimer runs out nameOfs moves on one character (6 frames apart);
 * once the rest fits it waits 40 frames, then 90 at the start again.
 */
void Yn_UpdateNameScroll(void)
{
    char buf[0x100];
    char *visible;
    s32 i;
    s32 n;

    if (gYnWork->nameScroll != 0) {
        gYnWork->nameTimer--;
        if (gYnWork->nameTimer <= 0) {
            if (gYnWork->nameAtEnd != 0) {
                gYnWork->nameOfs = 0;
                gYnWork->nameAtEnd = 0;
                gYnWork->nameTimer = 90;
            } else {
                i = gYnWork->cursor + gYnWork->listTop;
                Yn_Utf8ToSjis(buf, YnNetcnf_FindUsrName(&gYnWork->netcnf, gYnWork->netcnf.data[i].attachIfc));
                visible = buf;
                visible += gYnWork->nameOfs;
                n = Sjis_CharBytes((u8 *)visible);
                visible += n;
                gYnWork->nameOfs += n;
                if (strlen(visible) < 45) {
                    gYnWork->nameAtEnd = 1;
                    gYnWork->nameTimer = 40;
                } else {
                    gYnWork->nameTimer = 6;
                }
            }
        }
    }
}

/*
 * One frame of the settings list (step): cursor, the entry's details,
 * switching card slot (PAD_L1); on PAD_CIRCLE checks the entry and its
 * device, sets changed, applies and exports it. Returns 1 once applied,
 * -1 on PAD_CROSS (the quit question), -2 to rescan the cards, else 0.
 */
s32 Yn_UpdateEntryList(void)
{
    YnWork *w;
    char *nameCopy;
    s32 changed;
    s32 n;
    s32 unused;
    u16 pad;
    s32 ok;
    s32 i;
    char *name;
    s32 dev;
    s32 idx;

    changed = 0;
    if (gYnWork->step != YN_LIST_INIT && gYnWork->step != YN_LIST_CARD_MESSAGE && gYnWork->step != YN_LIST_WAIT_CARD && gYnWork->step != YN_LIST_CHOOSE) {
        changed = Yn_IsCurrentCardGone();
    }
    switch (gYnWork->step) {
    case YN_LIST_INIT:
        gYnWork->step++;
        for (i = 1; i < 16; i++) {
            (&gYnWork->listDetail)[i] = 0;
        }
        YnCard_Reset(&gYnWork->card);
        Yn_LoadProxy();
        Yn_ResetNameScroll();
        gYnWork->listDetail = 2;
        gYnWork->listBusy = 0;
        break;
    case YN_LIST_INPUT:
        gYnWork->listBusy = 0;
        gYnWork->cardGone = changed;
        gYnWork->helpBar = YN_HELPBAR_LIST;
        gYnWork->unk46 = 1;
        gYnWork->msgCloseMode = (gYnWork->listDetail >= 2) ? 4 : YN_MSG_CLOSE_EITHER;
        gYnWork->unk48 = 1;
        gYnWork->screen = YN_SCREEN_LIST;
        gYnWork->help = YN_HELP_LIST;
        gYnWork->helpVariant = (gYnWork->listDetail >= 2) ? 2 : gYnWork->listDetail;
        if (changed != 0) {
            gYnWork->scanResult = YN_SCAN_CARD_CHANGED;
        } else {
            pad = gPadState[0].pressed;
            if (pad & PAD_CROSS) {
                LoginSe_Cancel();
                return -1;
            }
            w = gYnWork;
            if (w->scanResult < 0) {
                if (w->scanResult == YN_SCAN_CARD_CHANGED && (w->okSlots & (1 << YnCard_GetSlot(&w->card)))) {
                    w->scanResult = 0;
                    return -2;
                }
            } else if (w->listBusy == 0) {
                if (pad & PAD_CIRCLE) {
                    LoginSe_Decide();
                    gYnWork->step = YN_LIST_CHOOSE;
                    break;
                }
                if (pad & PAD_TRIANGLE) {
                    LoginSe_Decide();
                    gYnWork->msgCloseMode = YN_MSG_CLOSE_EITHER;
                    gYnWork->backStep = gYnWork->step;
                    gYnWork->step = YN_LIST_MESSAGE;
                    gYnWork->msg = YN_MSG_ENTRY_DETAILS;
                    break;
                }
                if (pad & (PAD_UP | PAD_DOWN)) {
                    if (w->cursor > 0 && (pad & PAD_UP)) {
                        LoginSe_Cursor();
                        gYnWork->cursor--;
                        if (gYnWork->cursor < gYnWork->listTop) {
                            gYnWork->listTop = gYnWork->listTop - 1;
                        }
                        Yn_ResetNameScroll();
                        break;
                    }
                    if (w->cursor < w->entryCount - 1 && (pad & PAD_DOWN)) {
                        LoginSe_Cursor();
                        gYnWork->cursor++;
                        if (gYnWork->cursor >= gYnWork->listTop + YN_LIST_ROWS) {
                            gYnWork->listTop++;
                        }
                        Yn_ResetNameScroll();
                        break;
                    }
                }
            }
        }
        if (gPadState[0].pressed & PAD_L1) {
            gYnWork->scanResult = YN_SCAN_SLOT_SWITCHED;
            LoginSe_Play9();
            gYnWork->slot++;
            gYnWork->slot &= 1;
            YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
            return -2;
        }
        Yn_UpdateNameScroll();
        break;
    case YN_LIST_MESSAGE:
        if (changed != 0) {
            gYnWork->step = gYnWork->backStep;
            gYnWork->msg = YN_MSG_NONE;
            LoginSe_Cancel();
        } else {
            gYnWork->helpBar = YN_HELPBAR_MESSAGE;
            ok = 0;
            if (gYnWork->msgCloseMode == YN_MSG_CLOSE_CIRCLE) {
                if (gPadState[0].pressed & PAD_CIRCLE) {
                    ok = 1;
                }
            } else if ((gPadState[0].pressed & PAD_CIRCLE) || (gPadState[0].pressed & PAD_CROSS)) {
                ok = 1;
            }
            if (ok != 0) {
                gYnWork->step = gYnWork->backStep;
                gYnWork->msg = YN_MSG_NONE;
                LoginSe_Decide();
            }
        }
        break;
    case YN_LIST_ERROR:
        gYnWork->helpBar = YN_HELPBAR_MESSAGE;
        gYnWork->msgCloseMode = YN_MSG_CLOSE_EITHER;
        if ((gPadState[0].pressed & PAD_CIRCLE) || (gPadState[0].pressed & PAD_CROSS)) {
            gYnWork->step = YN_LIST_INIT;
            gYnWork->msg = YN_MSG_NONE;
            LoginSe_Decide();
            return -2;
        }
        break;
    case YN_LIST_CARD_MESSAGE:
        gYnWork->helpBar = YN_HELPBAR_MESSAGE;
        gYnWork->msgCloseMode = YN_MSG_CLOSE_EITHER;
        if ((gPadState[0].pressed & PAD_CIRCLE) || (gPadState[0].pressed & PAD_CROSS)) {
            gYnWork->step = YN_LIST_WAIT_CARD;
            gYnWork->msg = YN_MSG_NONE;
            LoginSe_Decide();
        }
        break;
    case YN_LIST_WAIT_CARD:
        gYnWork->helpBar = YN_HELPBAR_NONE;
        if (YnCard_CheckSlots(&gYnWork->card) >= 0) {
            LoginSe_Play9();
            return -2;
        }
        break;
    case YN_LIST_CHOOSE:
        gYnWork->helpBar = YN_HELPBAR_NONE;
        name = YnNetcnf_FindUsrName(&gYnWork->netcnf, gYnWork->netcnf.data[gYnWork->cursor].attachIfc);
        Yn_StrCopy(gYnWork->netcnf.name, name);
        nameCopy = gYnWork->netcnf.name;
        if (nameCopy == NULL || nameCopy[0] == 0) {
            gYnWork->step = YN_LIST_ERROR;
            gYnWork->msg = YN_MSG_NO_NAME;
            LoginSe_Play9();
            break;
        }
        if (YnNetcnf_CheckFile(&gYnWork->netcnf) != 0) {
            gYnWork->step = YN_LIST_ERROR;
            gYnWork->msg = YN_MSG_FILE_ERROR;
            LoginSe_Play9();
            break;
        }
        idx = gYnWork->cursor;
        if (gYnWork->combos[idx].kind == YN_COMBO_ETHERNET
            && gYnWork->netcnf.data[idx].phoneNumbers[0][0] != 0
            && gYnWork->netcnf.data[idx].ifcType == 2
            && gYnWork->netcnf.data[idx].pppoe == 0xFF) {
            gYnWork->step = YN_LIST_ERROR;
            gYnWork->msg = YN_MSG_DIAL_UP;
            LoginSe_Play9();
            break;
        }
        dev = gYnWork->combos[idx].device;
        if (dev < 0) {
            gYnWork->step = YN_LIST_ERROR;
            if (dev == -1) {
                gYnWork->msg = YN_MSG_DEVICE_ABSENT;
            } else {
                gYnWork->msg = YN_MSG_DEVICE_UNKNOWN;
            }
            LoginSe_Play9();
            break;
        }
        if (Yn_SelectDevice(dev) < 0) {
            gYnWork->step = YN_LIST_ERROR;
            gYnWork->msg = YN_MSG_DEVICE_ABSENT;
            LoginSe_Play9();
            break;
        }
        if (Yn_CompareProxy((YnEntryKey *)&gYnWork->proxy, (YnEntryKey *)&gNetSaveData.configProxy) < 0) {
            gYnWork->changed = 1;
        }
        if (Yn_FindCombination(&gNetSaveData.config, &gYnWork->combos[gYnWork->cursor], 1) < 0) {
            gYnWork->changed = 1;
        }
        Yn_ApplyEntry(gYnWork->combos[gYnWork->cursor].data);
        Yn_ExportSettings(gYnWork->combos[gYnWork->cursor].data);
        return 1;
    }
    return 0;
}

/*
 * Polls both card slots (YnCard_CheckSlots) into okSlots. Returns 1 once
 * the check is done and the selected slot has no usable card, else 0.
 */
s32 Yn_IsCardGone(void)
{
    gYnWork->okSlots = 0;
    if (YnCard_CheckSlots(&gYnWork->card) >= 0) {
        gYnWork->okSlots |= ((gYnWork->card.okSlots | gYnWork->card.slots3) >> 1) & 3;
        if (!(gYnWork->okSlots & (1 << YnCard_GetSlot(&gYnWork->card)))) {
            return 1;
        }
    }
    return 0;
}

/*
 * Yn_IsCardGone for the selected slot only (YnCard_CheckCurrentSlot):
 * returns 1 once the check is done and its card is gone, else 0.
 */
s32 Yn_IsCurrentCardGone(void)
{
    gYnWork->okSlots = 0;
    if (YnCard_CheckCurrentSlot(&gYnWork->card) >= 0) {
        gYnWork->okSlots |= ((gYnWork->card.okSlots | gYnWork->card.slots3) >> 1) & 3;
        if (!(gYnWork->okSlots & (1 << YnCard_GetSlot(&gYnWork->card)))) {
            return 1;
        }
    }
    return 0;
}

/* Empty in the game; Yn_Draw calls it before the screen. */
void Yn_PreDraw(void)
{

}

/*
 * Draws the screen chosen by screen: nothing for 0, the settings list for
 * 1, the proxy on/off choice for 22, the proxy hosts and ports for 23,
 * otherwise the screen's text items (Yn_DrawItems).
 */
void Yn_DrawScreen(void)
{
    switch (gYnWork->screen) {
    case YN_SCREEN_NONE:
        break;
    case YN_SCREEN_LIST:
        Yn_DrawEntryList();
        break;
    case YN_SCREEN_PROXY_FIELDS:
        Yn_DrawTextField((u8 *)gYnWork->proxy.http, (char *)gYnFieldHeaders[5], 0x16B, 0xB4, 0x12);
        Yn_DrawTextField((u8 *)gYnWork->proxy.https, (char *)gYnHttpsTexts[0], 0x16B, 0xE6, 0x12);
        Yn_DrawPort(&gYnWork->proxy.httpPort, 0x16B, 0xCD);
        Yn_DrawPort(&gYnWork->proxy.httpsPort, 0x16B, 0xFF);
        Yn_DrawItems();
        break;
    case YN_SCREEN_PROXY_USE:
        Yn_DrawChoiceItems(gYnWork->proxy.use ^ 1, 5);
        break;
    default:
        Yn_DrawItems();
        break;
    }
}

/*
 * Prints the current screen's text items (gYnScreenItems), centring those
 * whose x is -1.
 */
void Yn_DrawItems(void)
{
    s8 count;
    YnItem *item;
    s8 i;
    s16 x;

    item = gYnScreenItems[gYnWork->screen];
    count = gYnScreenItemCounts[gYnWork->screen];
    for (i = 0; i < count; i++, item++) {
        x = item->x;
        if (x == -1) {
            x = Yn_CenterXSjis(item->text, 0x16);
        }
        Yn_Print(x, item->y, item->text);
    }
}

/*
 * Yn_DrawItems for a two-way choice: of the first two items, item sel is
 * printed in color and the other one in color 0.
 */
void Yn_DrawChoiceItems(s32 sel, s32 color)
{
    s8 count;
    YnItem *item;
    s8 i;
    s16 x;

    item = gYnScreenItems[gYnWork->screen];
    count = gYnScreenItemCounts[gYnWork->screen];
    for (i = 0; i < count; i++, item++) {
        x = item->x;
        if (x == -1) {
            x = Yn_CenterXSjis(item->text, 0x16);
        }
        if (i < 2) {
            if (i == sel) {
                Yn_SetColor(color);
            } else {
                Yn_SetColor(0);
            }
            Yn_Print(x, item->y, item->text);
            Yn_SetColor(0);
        } else {
            Yn_Print(x, item->y, item->text);
        }
    }
}


/* The format that prints one string: first used here, and shared with
 * Yn_DrawEntryDetails. */
const char gYnFmtString[3] = {
#include "yn/00AD3828.inc"
};

/*
 * Draws the settings list: the card slot's name, then either the scan's
 * error lines (with the slot's name filled in) or four entries from
 * listTop, numbered, unusable ones in another color and the cursor's name
 * scrolled; once unk8 >= 2 also the cursor entry's device.
 */
void Yn_DrawEntryList(void)
{
    char buf[0x100];
    char scratch[0x100];
    char *name;
    s8 idx;
    s32 kind;
    s32 i;
    s32 color;
    s32 y;
    char **lines;
    u8 *percent;
    s8 scanStatus;

    scratch[0] = 0;
    buf[0] = 0;
    Yn_SetFontSize(0x14, 0x14);
    Yn_SetColor(0);
    Yn_Print(0x2C, 0x54, (char *)(&gYnStorageNames[1])[YnCard_GetSlot(&gYnWork->card)]);
    scanStatus = gYnWork->scanResult;
    if (scanStatus == -10) {
        return;
    }
    if (scanStatus != 0) {
        y = 0x90;
        Yn_SetColor(5);
        if (gYnWork->state == 4 && gYnWork->step == 1) {
            switch (gYnWork->scanResult) {
            case 0:
                lines = NULL;
                break;
            case -2:
                lines = (char **)gYnNoCardLines;
                break;
            case -3:
                lines = (char **)gYnOtherPs2Lines;
                break;
            case -1:
            default:
                lines = (char **)gYnNoSettingsLines;
                break;
            }
        } else {
            lines = NULL;
        }
        if (lines != NULL && *lines != NULL) {
            do {
                buf[0] = 0;
                strcpy(scratch, *lines);
                percent = (u8 *)strchr(scratch, '%');
                if (percent != NULL && percent[1] == 's') {
                    strcpy(buf, (char *)gYnStorageNames[YnCard_GetSlot(&gYnWork->card) + 1]);
                    strcat(buf, (char *)percent + 2);
                } else {
                    strcpy(buf, scratch);
                }
                Yn_SetFontSize(0x14, 0x14);
                Yn_Print(0x2C, y, buf);
                lines++;
                y += 0x1A;
            } while (*lines != NULL);
        }
        Yn_SetColor(0);
        Yn_SetFontSize(0x16, 0x16);
        return;
    }
    Yn_SetFontSize(0x16, 0x16);
    y = 0x89;
    for (i = 0; i < 4; i++) {
        color = 0;
        Yn_SetColor(0);
        if (i >= gYnWork->entryCount) {
            break;
        }
        if (gYnWork->combos[gYnWork->listTop + i].device == -2) {
            color = 2;
        } else if (gYnWork->combos[gYnWork->listTop + i].device == -1) {
            color = 9;
        } else if (gYnWork->combos[gYnWork->listTop + i].kind == 1
                   && gYnWork->netcnf.data[gYnWork->listTop + i].phoneNumbers[0][0] != 0
                   && gYnWork->netcnf.data[gYnWork->listTop + i].ifcType == 2
                   && gYnWork->netcnf.data[gYnWork->listTop + i].pppoe == 0xFF) {
            color = 2;
        } else if (gYnWork->combos[gYnWork->listTop + i].kind == 2) {
            color = 2;
        }
        name = YnNetcnf_FindUsrName(&gYnWork->netcnf, gYnWork->netcnf.data[gYnWork->listTop + i].attachIfc);
        if (name == NULL || gYnWork->combos[gYnWork->listTop + i].kind == 2) {
            color = 2;
            sprintf(buf, (char *)gYnFmtString, gYnHttpsTexts[1]);
        } else {
            Yn_Utf8ToSjis(buf, name);
            if (gYnWork->nameScroll != 0 && gYnWork->cursor == i) {
                Yn_CopyForPrint(scratch, (u8 *)buf + gYnWork->nameOfs, 0x2C);
                if (gYnWork->nameOfs == 0) {
                    strcpy(buf, scratch);
                } else {
                    strcpy(buf, "..");
                    strcat(buf, scratch);
                }
            } else {
                Yn_CopyForPrint(scratch, (u8 *)buf, 0x2C);
                strcpy(buf, scratch);
            }
        }
        Font_SetPos(0x2C, y);
        Font_Printf("%d: ", i + 1 + gYnWork->listTop);
        Yn_SetColor(color);
        Font_SetPos(0x4D, y);
        Font_Printf(buf);
        y += 0x20;
    }
    if (gYnWork->listDetail >= 2) {
        Yn_SetColor(0);
        idx = gYnWork->cursor;
        kind = gYnWork->combos[idx].kind;
        if (kind == 0
            || (kind == 1 && gYnWork->netcnf.data[idx].phoneNumbers[0][0] != 0
                && gYnWork->netcnf.data[idx].ifcType == 2 && gYnWork->netcnf.data[idx].pppoe == 0xFF)) {
            sprintf(buf, "%s:%s", gYnDeviceTexts[0], gYnHttpsTexts[1]);
        } else {
            sprintf(buf, "%s:%s/%s", gYnDeviceTexts[0], gYnWork->combos[idx].vendor, gYnWork->combos[idx].product);
        }
        Yn_SetFontSize(0x14, 0x14);
        Yn_Print(0x2C, 0x107, buf);
        Yn_SetColor(0);
        Yn_SetFontSize(0x16, 0x16);
    }
}

/*
 * Prints the port *value at (x, y) right-aligned in 18 characters; while
 * it is the number being edited, prints all five digits one by one with
 * the one at numDigitPos highlighted.
 */
void Yn_DrawPort(u16 *value, s16 x, s16 y)
{
    s8 digits[8];
    char buf[0x48];
    char *bufPos;
    s32 j;
    s32 i;
    s32 div;
    s32 started;
    s32 n;

    started = 0;
    n = 0;
    div = 10000;
    for (i = 0; i < 5; i++) {
        digits[i] = (*value / div) % 10;
        div /= 10;
        if (digits[i] != 0) {
            started = 1;
        }
        if (started != 0) {
            n++;
        }
    }
    if (n == 0) {
        n++;
    }
    if (gYnWork->numEditing == 0 || gYnWork->numTarget != value) {
        for (i = 0; i < 18 - n; i++) {
            buf[i] = ' ';
        }
        sprintf(&buf[18 - n], "%d", *value);
        Yn_Print(x, y, buf);
    } else {
        for (i = 0; i < 13; i++) {
            buf[i] = ' ';
        }
        for (j = 0; j < 5; j++) {
            buf[i++] = digits[j] + '0';
        }
        buf[i] = 0;
        bufPos = buf;
        for (i = 0; i < 18; i++) {
            digits[0] = *bufPos++;
            digits[1] = 0;
            if (17 - i == gYnWork->numDigitPos) {
                Yn_SetColor(0xB);
            } else {
                Yn_SetColor(0);
            }
            Yn_Print(x, y, (char *)digits);
            x += 10;
        }
        Yn_SetColor(0);
    }
}

/*
 * Prints text at (x, y) cut to w bytes (Yn_CopyForPrint), or label
 * instead when text is empty.
 */
void Yn_DrawTextField(u8 *text, char *label, s16 x, s16 y, s32 w)
{
    char buf[0x40];

    if (*text == 0) {
        Yn_Print(x, y, label);
        return;
    }
    Yn_CopyForPrint(buf, text, w);
    Yn_Print(x, y, buf);
}

/*
 * Draws the message window frame (gYnWindows[2]) when the current msg
 * is one that has a frame; nothing for msg 0.
 */
void Yn_DrawMessageWindow(void)
{
    switch (gYnWork->msg) {
    case YN_MSG_DEVICE_ABSENT:
    case YN_MSG_DEVICE_UNKNOWN:
    case 6:
    case 7:
    case 0x17:
    case YN_MSG_ENTRY_DETAILS:
    case YN_MSG_DIAL_UP:
    case YN_MSG_FILE_ERROR:
    case 0x1E:
    case 0x2B:
    case YN_MSG_PROXY_SAVED:
    case YN_MSG_NO_NAME:
    case YN_MSG_CONNECTED:
    case 0x39:
    case YN_MSG_QUIT_ASK:
        Window_DrawTextAlpha(&gYnWindows[2], 0, 0xF0);
        break;
    case 1:
    case YN_MSG_PROXY_CONFIRM:
    case 0x37:
    case 0x38:
        Window_DrawTextAlpha(&gYnWindows[2], 0, 0xF0);
        break;
    case YN_MSG_NONE:
        break;
    }
}

/*
 * Draws the current message (msg) at a smaller glyph size: its text, the
 * card slot's number for the messages that show it, or the details
 * message with Yn_DrawEntryDetails under it. Restores the glyph size.
 */
void Yn_DrawMessage(void)
{
    s16 w;
    s16 height;

    Yn_SetGlyphSize(478.5714416503906f);
    switch (gYnWork->msg) {
    case 0x01:
    case YN_MSG_DEVICE_ABSENT:
    case YN_MSG_DEVICE_UNKNOWN:
    case 0x06:
    case 0x07:
    case 0x17:
    case YN_MSG_PROXY_CONFIRM:
    case 0x38:
    case YN_MSG_QUIT_ASK:
    case YN_MSG_DIAL_UP:
    case 0x1E:
    case 0x2B:
    case YN_MSG_PROXY_SAVED:
    case 0x37:
    case 0x39:
        Yn_DrawMessageText();
        break;
    case YN_MSG_NO_NAME:
        Yn_DrawSlotMessage(0);
        break;
    case YN_MSG_FILE_ERROR:
        Yn_DrawSlotMessage(0);
        break;
    case YN_MSG_CONNECTED:
        Yn_DrawSlotMessage(0);
        break;
    case YN_MSG_ENTRY_DETAILS:
        Yn_DrawMessageText();
        w = Font_GetAdvance();
        height = Font_GetCharH();
        Yn_SetFontSize(0x12, 0x12);
        Yn_DrawEntryDetails();
        Yn_SetFontSize(w, height);
        break;
    case YN_MSG_NONE:
        break;
    }
    Yn_SetGlyphSize(621.4285888671875f);
}

/*
 * Prints the current message's lines (gYnMsgItems[msg]), some messages
 * (seen: the device errors) in color 11. The line holding the choice
 * marker becomes the yes/no choice, quitChoice's side highlighted;
 * x == -1 centres a line (Yn_CenterX).
 */
void Yn_DrawMessageText(void)
{
    s16 advance;
    s8 count;
    YnItem *item;
    s8 i;
    s16 x;
    char *percent;

    switch (gYnWork->msg) {
    case 1:
    case YN_MSG_DEVICE_ABSENT:
    case YN_MSG_DEVICE_UNKNOWN:
    case 6:
    case 7:
    case YN_MSG_DIAL_UP:
    case 0x39:
        Yn_SetColor(0xB);
        break;
    default:
        Yn_SetColor(0);
        break;
    }
    advance = Font_GetAdvance();
    item = gYnMsgItems[gYnWork->msg];
    count = gYnMsgItemCounts[gYnWork->msg];
    for (i = 0; i < count; i++, item++) {
        percent = strchr(item->text, '%');
        if (percent != NULL && percent[1] == 'a') {
            x = Yn_CenterX((char *)gYnYesNoTexts[0], advance);
            if (gYnWork->quitChoice == 0) {
                Yn_SetColor(5);
            } else {
                Yn_SetColor(0);
            }
            Yn_Print(x, item->y, (char *)gYnYesNoTexts[1]);
            x += advance * 5;
            if (gYnWork->quitChoice == 0) {
                Yn_SetColor(0);
            } else {
                Yn_SetColor(5);
            }
            Yn_Print(x, item->y, (char *)gYnNoTexts[0]);
            Yn_SetColor(0);
        } else {
            x = item->x;
            if (x == -1) {
                x = Yn_CenterX(item->text, advance);
            }
            Yn_Print(x, item->y, item->text);
        }
    }
    Yn_SetColor(0);
}

/*
 * Prints the current message's lines; line sel is a format that gets the
 * memory card slot's number (1 or 2).
 */
void Yn_DrawSlotMessage(s32 sel)
{
    char buf[0x100];
    s8 count;
    YnItem *item;
    s8 i;
    s16 x;

    item = gYnMsgItems[gYnWork->msg];
    count = gYnMsgItemCounts[gYnWork->msg];
    for (i = 0; i < count; i++, item++) {
        x = item->x;
        if (x == -1) {
            x = Yn_CenterXSjis(item->text, 0x16);
        }
        if (i == sel) {
            sprintf(buf, item->text, YnCard_GetSlot(&gYnWork->card) + 1);
            Yn_Print(x, item->y, buf);
        } else {
            Yn_Print(x, item->y, item->text);
        }
    }
}

/*
 * Draws the cursor entry's details under its message: the device, then
 * for a usable entry the ethernet mode, the PPPoE user, the DHCP host or
 * the fixed address, netmask and gateway, and both DNS servers.
 */
void Yn_DrawEntryDetails(void)
{
    char scratch[0x200];
    char buf[0x200];
    YnCombo *combo;
    NetcnfData *data;
    s32 y;
    char *name;
    s32 x;
    s32 etherMode;

    y = 0x82;
    combo = &gYnWork->combos[gYnWork->cursor];
    data = combo->data;
    name = YnNetcnf_FindUsrName(&gYnWork->netcnf, gYnWork->netcnf.data[gYnWork->cursor].attachIfc);
    if (name != NULL && combo->kind == 1
        && !(gYnWork->netcnf.data[gYnWork->cursor].phoneNumbers[0][0] != 0
             && gYnWork->netcnf.data[gYnWork->cursor].ifcType == 2
             && gYnWork->netcnf.data[gYnWork->cursor].pppoe == 0xFF)) {
        sprintf(scratch, "        %s  %s/%s", gYnDeviceTexts[0], combo->vendor, combo->product);
        Yn_CopyForPrint(buf, (u8 *)scratch, 0x3C);
        x = 0x58;
    } else {
        Yn_SetColor(2);
        sprintf(buf, "        %s", gYnHttpsTexts[1]);
        x = 0x44;
    }
    Yn_SetColor(0);
    Yn_Print(x, y, buf);
    y += 0x19;
    if (data != NULL && name != NULL && combo->kind == 1
        && !(combo->kind == 1 && gYnWork->netcnf.data[gYnWork->cursor].phoneNumbers[0][0] != 0
             && gYnWork->netcnf.data[gYnWork->cursor].ifcType == 2
             && gYnWork->netcnf.data[gYnWork->cursor].pppoe == 0xFF)) {
        switch (data->phyConfig) {
        case -1:
        default:
            etherMode = -1;
            break;
        case 1:
            etherMode = 0;
            break;
        case 2:
            etherMode = 1;
            break;
        case 3:
            etherMode = 2;
            break;
        case 5:
            etherMode = 3;
            break;
        case 6:
            etherMode = 4;
            break;
        }
        if (0 <= etherMode) {
            sprintf(scratch, "  %s  %s", gYnDeviceTexts[1], gYnEtherModeTexts[etherMode]);
            Yn_CopyForPrint(buf, (u8 *)scratch, 0x2C);
            Yn_Print(0x58, y, buf);
            y += 0x19;
        }
        if (data->pppoe == 1) {
            Yn_CopyForPrint(scratch, (u8 *)data->authName, 0x1C);
            sceNetcnfifConvAuthname(scratch, buf, 0x100);
            sprintf(scratch, "    %s  %s", gYnUserIdTexts[0], buf);
            Yn_Print(0x58, y, scratch);
            y += 0x19;
        } else if (data->dhcp == 1) {
            if (data->dhcpHostname[0] == 0) {
                sprintf(scratch, (char *)gYnFmtString, gYnUserIdTexts[1]);
            } else {
                Yn_CopyForPrint(scratch, (u8 *)data->dhcpHostname, 0x1C);
            }
            sprintf(buf, "%s  %s", gYnHostIpTexts[0], scratch);
            Yn_Print(0x46, y, buf);
            sprintf(scratch, (char *)gYnFmtString, gYnUserIdTexts[1]);
            sprintf(buf, "    %s  %s", gYnHostIpTexts[1], scratch);
            Yn_Print(0x58, y + 0x19, buf);
            y += 0x32;
        } else if (data->address[0] != 0) {
            Yn_AlignAddress((u8 *)buf, (u8 *)data->address);
            sprintf(scratch, "    %s  %s", gYnHostIpTexts[1], buf);
            Yn_Print(0x58, y, scratch);
            Yn_AlignAddress((u8 *)buf, (u8 *)data->netmask);
            sprintf(scratch, "    %s  %s", gYnNetmaskTexts[0], buf);
            Yn_Print(0x58, y + 0x19, scratch);
            Yn_AlignAddress((u8 *)buf, (u8 *)data->gateway);
            sprintf(scratch, "%s  %s", gYnNetmaskTexts[1], buf);
            Yn_Print(0x58, y + 0x32, scratch);
            y += 0x4B;
        }
        if (data->dns1Address[0] == 0) {
            if (data->pppoe != 1 && data->dhcp != 1) {
                sprintf(scratch, (char *)gYnFmtString, gYnHttpsTexts[1]);
            } else {
                sprintf(scratch, (char *)gYnFmtString, gYnUserIdTexts[1]);
            }
        } else {
            Yn_AlignAddress((u8 *)scratch, (u8 *)data->dns1Address);
        }
        sprintf(buf, "%s  %s", gYnDnsTexts[0], scratch);
        Yn_Print(0x58, y, buf);
        if (data->dns2Address[0] == 0) {
            if (data->pppoe != 1 && data->dhcp != 1) {
                sprintf(scratch, (char *)gYnFmtString, gYnHttpsTexts[1]);
            } else {
                sprintf(scratch, (char *)gYnFmtString, gYnUserIdTexts[1]);
            }
        } else {
            Yn_AlignAddress((u8 *)scratch, (u8 *)data->dns2Address);
        }
        sprintf(buf, "%s  %s", gYnDnsTexts[1], scratch);
        Yn_Print(0x58, y + 0x19, buf);
    }
}

/*
 * Copies the dotted address src to dst with each of its four numbers
 * right-aligned in three characters (padded with spaces).
 */
void Yn_AlignAddress(u8 *dst, u8 *src)
{
    u8 num[4];
    s32 i;
    s32 n;
    s32 j;

    for (i = 0; i < 4; i++) {
        n = 0;
        while (*src != '.' && *src != 0) {
            num[n++] = *src++;
        }
        for (j = n; j < 3; j++) {
            *dst++ = ' ';
        }
        for (j = 0; j < n; j++) {
            *dst++ = num[j];
        }
        *dst++ = *src++;
    }
    *dst = 0;
}

/*
 * Prints the help lines gYnHelpItems[help + helpVariant] in the small
 * font, centring those whose x is -1, then restores the normal size.
 */
void Yn_DrawHelp(void)
{
    s8 count;
    YnItem *item;
    s8 i;
    s16 x;

    Yn_SetFontSize(0x12, 0x12);
    item = gYnHelpItems[gYnWork->help + gYnWork->helpVariant];
    count = gYnHelpItemCounts[gYnWork->help + gYnWork->helpVariant];
    for (i = 0; i < count; i++, item++) {
        x = item->x;
        if (x == -1) {
            x = Yn_CenterXSjis(item->text, 0x16);
        }
        Yn_Print(x, item->y, item->text);
    }
    Yn_SetFontSize(0x16, 0x16);
}

void Yn_Print(s16 x, s16 y, char *text)
{
    Font_SetPos(x, y);
    Text_Printf(text);
}

void Yn_SetColor(s8 color)
{
    Font_SetColorIndex(color);
}

void Yn_SetFontSize(s32 w, s32 height)
{
    Font_SetSpacing(w, height);
}

/* Sets the text object's glyph size (sub_main Font_SetSize: 0..1000 picks one of 7 glyph tables; 621.4 is table 4). */
void Yn_SetGlyphSize(f32 scale)
{
    Font_SetSize(scale);
}

void Yn_SetTextMode(s8 mode)
{
    Font_SetHalfWidth(mode);
}

/* The game's getter of the text mode always returns 1; Yn_Draw saves it, sets 1 and restores it. */
s32 Yn_GetTextMode(void)
{
    return 1;
}

/*
 * Returns the x that centres text on the 640-pixel screen, counting w / 2
 * pixels for each unit of Str_Length.
 */
s16 Yn_CenterX(char *text, s32 w)
{
    s32 len;

    len = Str_Length(text);
    return (s32)((640 - len * (w / 2)) / 2.0f);
}

/*
 * Returns the x that centres the Shift-JIS string str on the 640-pixel
 * screen: w pixels per two-byte character, w / 2 per one-byte character.
 */
s16 Yn_CenterXSjis(char *str, s32 w)
{
    u8 *text;
    s32 i;
    s32 full;
    s32 half;

    text = (u8 *)str;
    half = 0;
    full = 0;
    for (i = 0; i < 0x100; i++) {
        if (*text == 0) {
            break;
        }
        if (*text >= 0x80) {
            text++;
            full++;
        } else {
            half++;
        }
        text++;
    }
    return (640 - w * full - w * half / 2) / 2;
}

/*
 * Copies src to dst for printing: doubles each % sign, keeps two-byte
 * characters whole and, when src is over max bytes, cuts it near max - 2
 * and ends it with two dots. A NULL src gives an empty dst.
 */
void Yn_CopyForPrint(char *dst, u8 *src, s32 max)
{
    s32 cut;
    s32 n;
    u8 ch;

    if (src != NULL) {
        cut = 0;
        if ((u32)strlen((char *)src) > max) {
            cut = 1;
        }
        n = 0;
        ch = *src;
        while (ch != 0) {
            if (ch >= 0x80) {
                dst[0] = ch;
                n += 2;
                dst[1] = src[1];
                src += 2;
                dst += 2;
            } else {
                if (ch == '%') {
                    *dst++ = '%';
                }
                n++;
                *dst++ = *src++;
            }
            if (cut != 0 && n > max - 3) {
                dst[0] = '.';
                dst[1] = '.';
                dst += 2;
                break;
            }
            ch = *src;
        }
    }
    *dst = 0;
}

/* strcpy that gives an empty dst when src is NULL. */
void Yn_StrCopy(char *dst, char *src)
{
    if (src == NULL) {
        *dst = 0;
        return;
    }
    strcpy(dst, src);
}

/*
 * Starts the number editor on *value: keeps the old value in numBackup
 * for a cancel and puts the cursor on the ones digit.
 */
void Yn_BeginNumberEdit(u16 *value)
{
    gYnWork->numTarget = value;
    gYnWork->numBackup = *value;
    gYnWork->numStep = 1;
    gYnWork->numDigitPos = 0;
}

/*
 * Key repeat: counts the frames pad's held buttons stay the same (repeat)
 * and from the 23rd frame on puts them in repeatPad every third frame;
 * repeatPad is 0 on the other frames.
 */
void Yn_UpdatePadRepeat(PadState *pad)
{
    gYnWork->repeatPad = 0;
    if (pad->prevHeld == pad->held) {
        gYnWork->repeat++;
        if (gYnWork->repeat > 20 && gYnWork->repeat > 22) {
            gYnWork->repeat = 20;
            gYnWork->repeatPad = pad->held;
        }
    } else {
        gYnWork->repeat = 0;
    }
}

/*
 * Opens the software keyboard on a copy of str in editBuf; mode 0, 1 or
 * 2 becomes NetKbd_Open's last argument 3, 4 or 5 (seen: 1 for the proxy
 * hosts). str is kept in editTarget for Yn_PollKeyboard to write back.
 */
void Yn_OpenKeyboard(s32 mode, char *str)
{
    char *buf;

    Font_ResetState();
    buf = gYnWork->editBuf;
    memset(buf, 0, 0x200);
    gYnWork->editTarget = str;
    strcpy(buf, str);
    NetKbd_SetActive(1);
    switch (mode) {
    case 0:
        NetKbd_Open(0x50, 0x12A, buf, 0xFF, 3);
        break;
    case 1:
        NetKbd_Open(0x50, 0x12A, buf, 0xFF, 4);
        break;
    case 2:
        NetKbd_Open(0x50, 0x12A, buf, 0xFF, 5);
        break;
    }
}

/*
 * Runs the software keyboard; once it closes, copies its text to editBuf
 * and to the string being edited (editTarget) and returns 1, else 0.
 */
s32 Yn_PollKeyboard(void)
{
    if (NetKbd_Run() != 0) {
        NetKbd_SetActive(0);
        strcpy(gYnWork->editBuf, NetKbd_GetText());
        strcpy(gYnWork->editTarget, gYnWork->editBuf);
        return 1;
    }
    return 0;
}

/* Empty in the game; Yn_Update and Yn_UpdateProxy call it first. */
void Yn_PreUpdate(void)
{

}

/*
 * Flushes the text: sets the GS state for it, draws font page 4, the
 * message window and the quit cursor (quitAsked), page 3 with its icons,
 * the software keyboard and page 1, then resets the text and GS state.
 */
void Yn_DrawTextLayer(void)
{
    Gs_SetState(0x61, 0x01000000);
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    Gs_SetState(RS_ALPHA_TEST, RS_CMP_NOTEQUAL);
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_ZWRITE, 0);
    Gfx_SetTexFilter(0);
    Font_DrawPage(4);
    Yn_DrawMessageWindow();
    if (gYnWork->quitAsked != 0) {
        Yn_DrawChoiceCursor();
    }
    Font_DrawPage(3);
    Text_DrawIcons(3);
    Yn_SetGlyphSize(192.85714721679688f);
    NetKbd_Draw();
    Font_DrawPage(1);
    Text_Reset();
    Gs_SetState(RS_ZWRITE, 1);
}

extern s16 D_0059DCF0;
extern s32 D_0059DCF8;
extern s8 gProxyMode;
extern u8 gNetPppLineModeFirst;
extern u8 gNetPhyMode;
extern u8 gNetPppFlagSaved;
extern u8 D_0059DE00;
/*
 * Writes the chosen entry into the network globals after clearing them:
 * gNetLanMode 1 DHCP, 2 PPPoE or 0 a fixed address, the DNS servers,
 * gNetPhyMode, and the proxy URLs from proxy when proxy.use is 1
 * (gProxyMode 1, or 3 with an HTTPS proxy).
 */
void Yn_ExportSettings(NetcnfData *data)
{
    YnProxy *proxy;

    D_0059DE00 = 0;
    proxy = &gYnWork->proxy;
    memset(D_59DD00, 0, sizeof(D_59DD00));
    D_0059DCF8 = 0;
    D_0059DCF0 = 0;
    memset(gNetDomain, 0, sizeof(gNetDomain));
    memset(gNetAuthName, 0, sizeof(gNetAuthName));
    memset(gNetAuthKey, 0, sizeof(gNetAuthKey));
    gNetIpAddr = 0;
    gNetNetmask = 0;
    gNetGateway = 0;
    gNetDns1 = 0;
    gNetDns2 = 0;
    memset(gNetPppConfBuf, 0, sizeof(gNetPppConfBuf));
    memset(gNetPppAuthText, 0, sizeof(gNetPppAuthText));
    gNetPppFlagSaved = 0;
    gNetPppLineModeFirst = 3;
    if (data->dhcp == 1) {
        gNetLanMode = 1;
        strncpy(gNetDomain, data->dhcpHostname, sizeof(gNetDomain));
    } else if (data->pppoe == 1) {
        gNetLanMode = 2;
        strncpy(gNetAuthName, data->authName, sizeof(gNetAuthName));
        strncpy(gNetAuthKey, data->authKey, sizeof(gNetAuthKey));
    } else {
        gNetLanMode = 0;
        gNetIpAddr = net_inet_addr2(data->address);
        gNetNetmask = net_inet_addr2(data->netmask);
        gNetGateway = net_inet_addr2(data->gateway);
    }
    if (data->dns1Address[0] != 0) {
        gNetDns1 = net_inet_addr2(data->dns1Address);
    }
    if (data->dns2Address[0] != 0) {
        gNetDns2 = net_inet_addr2(data->dns2Address);
    }
    switch (data->phyConfig) {
    case 1:
    default:
        gNetPhyMode = 0;
        break;
    case 2:
        gNetPhyMode = 1;
        break;
    case 3:
        gNetPhyMode = 0;
        break;
    case 5:
        gNetPhyMode = 3;
        break;
    case 6:
        gNetPhyMode = 4;
        break;
    }
    gProxyMode = 0;
    memset(gHttpProxy, 0, sizeof(gHttpProxy));
    memset(gHttpsProxy, 0, sizeof(gHttpsProxy));
    if (proxy->use == 1) {
        gProxyMode = 1;
        sprintf(gHttpProxy, "http://%s:%d", proxy->http, proxy->httpPort);
        if ((u8)proxy->https[0] != 0) {
            gProxyMode |= 2;
            sprintf(gHttpsProxy, "https://%s:%d", proxy->https, proxy->httpsPort);
        }
    }
}

/*
 * Makes entry data the settings in use: Yn_IfcToConfig and Yn_DevToConfig
 * fill gNetSaveData.config from it (with an empty name), and the work area's
 * proxy goes to gNetSaveData.configProxy.
 */
void Yn_ApplyEntry(NetcnfData *data)
{
    char name[4];

    name[0] = 0;
    Yn_IfcToConfig(&gNetSaveData.config, data, name);
    Yn_DevToConfig(&gNetSaveData.config, data);
    gNetSaveData.configProxy = gYnWork->proxy;
}

void Yn_LoadProxy(void)
{
    gYnWork->proxy = gNetSaveData.proxy;
}

void Yn_StoreProxy(void)
{
    gNetSaveData.proxy = gYnWork->proxy;
}

s16 gYnListBarYs[4] = {
#include "yn/00AD2128.inc"
};
PrimLine gYnBackgroundRect = {
#include "yn/00AD2130.inc"
};
/*
 * Draws the frames: the background window and rectangle, the two panels
 * and, on the settings list, a button icon and the cursor's glow bar.
 */
void Yn_DrawBackground(void)
{
    YnWindow win;
    PrimLine rect;
    s32 i;
    YnWindow *w;

    rect = gYnBackgroundRect;
    win = gYnWindows[3];
    Window_DrawTextAlpha(&win, 0, 0x80, &rect);
    for (i = 0, w = gYnWindows; i < 2; i++, w++) {
        win = *w;
        Window_DrawTextAlpha(&win, 0, 0x80);
    }
    Prim_FillRect(&rect);
    if (gYnWork->state == 4 && gYnWork->step == 1) {
        Hud_DrawButtonIcon(4, 0x136, 0x50, 0x155, -1, 1.2f);
    }
    if (gYnWork->state == 4 && gYnWork->step == 1 && gYnWork->scanResult == 0 && gYnWork->entryCount != 0) {
        Menu_DrawGlowBar(0x140, gYnListBarYs[(gYnWork->cursor - gYnWork->listTop) & 3], 0x240, 0x1E, 0xFF20C0C0);
    }
}

u32 gYnGuideArgv[4] = {
#include "yn/00AD2140.inc"
};
extern s32 gSysExiting;
extern s32 gDmaGsIntcId;
extern DmaQueue gDmaQueue;
/*
 * Shuts down the DMA, GS and interrupt handlers, the pads, CD and SIF and
 * boots the disc's network setup guide program; not expected to return
 * (seen: called from the options menu). gYnSetupGuideArg first gets the
 * game's own boot path.
 */
void Yn_LaunchSetupGuide(void)
{
    strcpy((char *)gYnSetupGuideArg, "cdrom0:\\SLPM_662.80;1");
    gSysExiting = 1;
    Dma_WaitIdle();
    sceGsSyncPath(0, 0);
    Pad_End();
    DIntr();
    DisableDmac(gDmaQueue.chan);
    RemoveDmacHandler(gDmaQueue.chan, gDmaQueue.handler);
    DisableIntc(0);
    RemoveIntcHandler(0, gDmaGsIntcId);
    EIntr();
    sceGsSyncVCallback(0);
    sceCdInit(5);
    sceSifExitCmd();
    func_11DD10("cdrom0:\\NTGUIDVD\\NTGDVDJ.BIN;1", 3, (char **)gYnGuideArgv);
}

/*
 * Draws the glow bar behind the quit question's yes or no (quitChoice),
 * placed as Yn_DrawMessageText places the two choices.
 */
void Yn_DrawChoiceCursor(void)
{
    s16 advance;
    s16 w;
    s16 x;

    advance = Font_GetAdvance();
    w = Yn_CenterX((char *)gYnYesNoTexts[0], advance);
    if (gYnWork->quitChoice == 0) {
        x = w + advance;
    } else {
        x = w + advance * 5 + (s16)(1.5f * advance);
    }
    Menu_DrawGlowBar(x, 0x12A, advance * 6, advance + 4, 0xFF20C0C0);
}

/*
 * Draws button icon id at (x, y) and scale, in orange; blink 0 makes it
 * pulse (alpha from a sine over 64 frames), nonzero draws it opaque.
 */
void Yn_DrawIcon(f32 scale, s32 id, s32 x, s32 y, s32 blink)
{
    f32 alpha;
    u32 color;
    f32 wave;

    wave = Math_Sin(9.58738e-05f * (u16)((gFrameTick & 0x3F) << 10));
    alpha = 160.0f + 80.0f * wave;
    if (blink == 0) {
        color = ((u32)alpha << 24) | 0xF08200;
    } else {
        color = 0xFFF08200;
    }
    Hud_DrawButtonIcon(id, x, y, 0x155, color, scale);
}

s16 gYnProxyBarYs[4] = {
#include "yn/00AD2150.inc"
};
s16 gYnProxyChoiceYs[4] = {
#include "yn/00AD2158.inc"
};
/*
 * Draw callback Yn_UpdateProxy queues: the frames, the cursor bar of the
 * on/off choice or of the four fields (the edit box and digit arrows
 * while a port is edited), the field's help line (sets help), then the
 * screen's text as Yn_Draw does.
 * The first frame is copied through a byte pointer to the table: with
 * the plain `gYnProxyWindows[0]` the whole function compiles differently.
 */
void Yn_DrawProxy(void)
{
    YnWindow win;
    s16 *barY;
    s8 mode;
    s32 x;
    s32 state;

    win = *(YnWindow *)((u8 *)gYnProxyWindows + 0);
    Window_DrawTextAlpha(&win, 0, 0x80);
    win = gYnProxyWindows[1];
    Window_DrawTextAlpha(&win, 0, 0x80);
    switch (state = gYnWork->state) {
    case YN_PROXY_USE:
        Menu_DrawGlowBar(0x140, gYnProxyChoiceYs[gYnWork->proxyCursor], 0xF0, 0x18, 0xFF20C0C0);
        Yn_DrawIcon(2.0f, YN_ICON_ARROW_RIGHT, 0x230, 0xD0, 0);
        break;
    case YN_PROXY_FIELDS:
    case YN_PROXY_KEYBOARD:
    case YN_PROXY_NUMBER:
        mode = gYnWork->menuCursor;
        if ((mode == YN_FIELD_HTTP_PORT || mode == YN_FIELD_HTTPS_PORT) && state == YN_PROXY_NUMBER) {
            win = gYnProxyWindows[2];
            barY = &gYnProxyBarYs[mode];
            win.win.y = *barY + 3;
            Window_DrawTextAlpha(&win, 0, 0x80);
            Menu_DrawGlowBar(0x204, *barY, 0x46, 0x18, 0xFF20C0C0);
            x = 0x214 - gYnWork->numDigitPos * 10;
            state = win.win.y - 11;
            Yn_DrawIcon(0.5f, YN_ICON_ARROW_UP, x, state, 1);
            Yn_DrawIcon(0.5f, YN_ICON_ARROW_DOWN, x, state + 0x1D, 1);
        } else {
            Menu_DrawGlowBar(0x140, gYnProxyBarYs[mode], 0x212, 0x18, 0xFF20C0C0);
        }
        switch (mode) {
        case YN_FIELD_HTTP:
            gYnWork->help = YN_HELP_PROXY_HOST;
            break;
        case YN_FIELD_HTTP_PORT:
        case YN_FIELD_HTTPS_PORT:
            gYnWork->help = YN_HELP_PROXY_PORT;
            break;
        case YN_FIELD_HTTPS:
            gYnWork->help = YN_HELP_PROXY_HTTPS;
            break;
        }
        if (gYnWork->state == YN_PROXY_FIELDS) {
            Yn_DrawIcon(2.0f, YN_ICON_ARROW_LEFT, 0x1E, 0xD0, 0);
            if ((u8)gYnWork->proxy.http[0] != 0 && gYnWork->proxy.httpPort != 0) {
                Yn_DrawIcon(2.0f, YN_ICON_ARROW_RIGHT, 0x230, 0xD0, 0);
            }
        }
        break;
    }
    gYnWork->textMode = Yn_GetTextMode();
    Yn_SetTextMode(1);
    Yn_SetColor(0);
    Yn_SetFontSize(0x14, 0x14);
    Yn_SetGlyphSize(621.4285888671875f);
    Yn_DrawScreen();
    Yn_DrawHelpBar(gYnWork->helpBar);
    Yn_DrawMessage();
    Yn_DrawHelp();
    Yn_DrawTextLayer();
}

/*
 * One frame of the proxy screen (state): the on/off choice, the four
 * fields (hosts by keyboard, ports by the digit editor), the save
 * question (yes stores it, Yn_StoreProxy). Queues Yn_DrawProxy. Returns
 * -1 when the screen is left (PAD_CROSS or after saving), else 0.
 * The save question's PAD_CIRCLE on "no" jumps to the PAD_CROSS case's
 * code (`back`), which the game's code shares.
 */
s32 Yn_UpdateProxy(void)
{
    u32 newValue;
    u32 digit;
    s32 padDown;
    s32 down;
    u16 pad;
    u16 div;
    s32 padRepeat;
    u32 rest;

    if (gYnWork == NULL) {
        return -1;
    }
    Yn_PreUpdate();
    switch (gYnWork->state) {
    case YN_PROXY_INIT:
        Yn_LoadProxy();
        gYnWork->state = YN_PROXY_USE;
        gYnWork->unk46 = 0x10;
        gYnWork->msgCloseMode = YN_MSG_CLOSE_EITHER;
        gYnWork->unk48 = 0x13;
        gYnWork->screen = YN_SCREEN_PROXY_USE;
        gYnWork->help = YN_HELP_PROXY_USE;
        gYnWork->proxyCursor = 1;
        gYnWork->menuCursor = 0;
        gYnWork->helpBar = YN_HELPBAR_NONE;
        break;
    case YN_PROXY_USE:
        if (gYnWork->proxy.use == 0) {
            gYnWork->helpBar = YN_HELPBAR_PROXY_OFF;
        } else {
            gYnWork->helpBar = YN_HELPBAR_PROXY_ON;
        }
        pad = gPadState[0].pressed;
        if (pad & PAD_CROSS) {
            LoginSe_Cancel();
            return -1;
        }
        if (pad & PAD_CIRCLE) {
            gYnWork->proxy.use = 1 - gYnWork->proxyCursor;
            LoginSe_Decide();
        } else if (pad & (PAD_UP | PAD_DOWN)) {
            gYnWork->proxyCursor ^= 1;
            LoginSe_Cursor();
        } else if (pad & PAD_RIGHT) {
            LoginSe_Cursor();
            if (gYnWork->proxy.use != 0) {
                gYnWork->state = YN_PROXY_FIELDS;
                gYnWork->screen = YN_SCREEN_PROXY_FIELDS;
                gYnWork->help = YN_HELP_PROXY_HOST;
            } else {
                gYnWork->state = YN_PROXY_CONFIRM;
                gYnWork->msg = YN_MSG_PROXY_CONFIRM;
                gYnWork->quitChoice = 1;
            }
        }
        break;
    case YN_PROXY_FIELDS:
        if ((u8)gYnWork->proxy.http[0] != 0 && gYnWork->proxy.httpPort != 0) {
            gYnWork->helpBar = YN_HELPBAR_FIELDS_DONE;
        } else {
            gYnWork->helpBar = YN_HELPBAR_FIELDS;
        }
        pad = gPadState[0].pressed;
        if (pad & PAD_CROSS) {
            LoginSe_Cancel();
            return -1;
        }
        if (pad & PAD_LEFT) {
            gYnWork->state = YN_PROXY_USE;
            gYnWork->screen = YN_SCREEN_PROXY_USE;
            gYnWork->help = YN_HELP_PROXY_USE;
            LoginSe_Cursor();
        } else if (pad & PAD_UP) {
            gYnWork->menuCursor--;
            gYnWork->menuCursor = (gYnWork->menuCursor + YN_FIELD_COUNT) % YN_FIELD_COUNT;
            LoginSe_Cursor();
        } else if (pad & PAD_DOWN) {
            gYnWork->menuCursor++;
            gYnWork->menuCursor = (gYnWork->menuCursor + YN_FIELD_COUNT) % YN_FIELD_COUNT;
            LoginSe_Cursor();
        } else if (pad & PAD_CIRCLE) {
            LoginSe_Decide();
            switch (gYnWork->menuCursor) {
            case YN_FIELD_HTTP:
                gYnWork->state = YN_PROXY_KEYBOARD;
                Yn_OpenKeyboard(1, gYnWork->proxy.http);
                break;
            case YN_FIELD_HTTP_PORT:
                gYnWork->state = YN_PROXY_NUMBER;
                Yn_BeginNumberEdit(&gYnWork->proxy.httpPort);
                break;
            case YN_FIELD_HTTPS:
                gYnWork->state = YN_PROXY_KEYBOARD;
                Yn_OpenKeyboard(1, gYnWork->proxy.https);
                break;
            case YN_FIELD_HTTPS_PORT:
                gYnWork->state = YN_PROXY_NUMBER;
                Yn_BeginNumberEdit(&gYnWork->proxy.httpsPort);
                break;
            }
        } else if ((pad & PAD_RIGHT) && (u8)gYnWork->proxy.http[0] != 0 && gYnWork->proxy.httpPort != 0) {
            LoginSe_Cursor();
            gYnWork->state = YN_PROXY_CONFIRM;
            gYnWork->msg = YN_MSG_PROXY_CONFIRM;
            gYnWork->quitChoice = 1;
        }
        break;
    case YN_PROXY_KEYBOARD:
        gYnWork->helpBar = YN_HELPBAR_NONE;
        if (Yn_PollKeyboard() != 0) {
            gYnWork->state = YN_PROXY_FIELDS;
        }
        break;
    case YN_PROXY_NUMBER:
        gYnWork->helpBar = YN_HELPBAR_NONE;
        gYnWork->numEditing = 2;
        pad = gPadState[0].pressed;
        padRepeat = pad | gPadState[0].repeat;
        if (pad & PAD_CROSS) {
            gYnWork->state = YN_PROXY_FIELDS;
            *gYnWork->numTarget = gYnWork->numBackup;
            gYnWork->numEditing = 0;
            gYnWork->numDigitPos = 0;
            LoginSe_Cancel();
            break;
        }
        if (pad & PAD_CIRCLE) {
            gYnWork->state = YN_PROXY_FIELDS;
            gYnWork->numEditing = 0;
            gYnWork->numDigitPos = 0;
            LoginSe_Decide();
            break;
        }
        if (padRepeat & 0xC00) {
            if ((padRepeat & 0x800) && gYnWork->numStep != 10000) {
                gYnWork->numStep *= 10;
                gYnWork->numDigitPos++;
                LoginSe_Cursor();
                break;
            }
            if ((padRepeat & 0x400) && gYnWork->numStep != 1) {
                gYnWork->numStep /= 10;
                gYnWork->numDigitPos--;
                LoginSe_Cursor();
                break;
            }
        }
        if (padRepeat & 0x3000) {
            padDown = padRepeat & 0x1000;
            down = padRepeat & 0x2000;
            div = gYnWork->numStep;
            digit = (*gYnWork->numTarget / div) % 10;
            rest = *gYnWork->numTarget - div * digit;
            do {
                if (padDown) {
                    digit = (digit + 9) % 10;
                }
                if (down) {
                    digit = (digit + 11) % 10;
                }
                newValue = rest + digit * div;
            } while (newValue >= 0x10000);
            *gYnWork->numTarget = newValue;
            LoginSe_Cursor();
        }
        break;
    case YN_PROXY_CONFIRM:
        gYnWork->helpBar = YN_HELPBAR_MESSAGE;
        gYnWork->quitAsked = 1;
        pad = gPadState[0].pressed;
        if (pad & PAD_CROSS) {
            gYnWork->quitAsked = 0;
            LoginSe_Cancel();
        back:
            gYnWork->msg = YN_MSG_NONE;
            if (gYnWork->proxy.use != 0) {
                gYnWork->state = YN_PROXY_FIELDS;
            } else {
                gYnWork->state = YN_PROXY_USE;
            }
        } else if (pad & PAD_CIRCLE) {
            gYnWork->quitAsked = 0;
            LoginSe_Decide();
            if (gYnWork->quitChoice != 0) {
                goto back;
            }
            gYnWork->state = YN_PROXY_SAVED;
            gYnWork->msg = YN_MSG_PROXY_SAVED;
            Yn_StoreProxy();
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            gYnWork->quitChoice ^= 1;
            LoginSe_Cursor();
        }
        break;
    case YN_PROXY_SAVED:
        gYnWork->helpBar = YN_HELPBAR_MESSAGE;
        if (gPadState[0].pressed & PAD_CIRCLE) {
            LoginSe_Decide();
            return -1;
        }
        break;
    }
    Callbacks_Add(Yn_DrawProxy);
    return 0;
}

s8 gYnHelpHints[7][2] = {
#include "yn/00AD2160.inc"
};
YnHelpBar gYnHelpBars[8] = {
#include "yn/00AD2170.inc"
};

/* Draws the help bar's button hints (gYnHelpBars[type]: count, width, hint
 * ids), centred on the screen, in the font size it restores afterwards.
 * The name table kept in a local, the height kept in an s32 that holds an
 * s16 and is passed on as `(s16)height`, and the order of the declarations
 * are kept as they are: with them the compiler places the values in the
 * registers and stack slots the game's code uses. */
void Yn_DrawHelpBar(s32 type)
{
    char **names;
    s16 w;
    s32 height;
    YnHelpBar bar;
    s16 count;
    char *text;
    s32 x;
    s32 i;
    s8 *hint;
    s32 len;

    w = Font_GetAdvance();
    height = (s16)Font_GetCharH();
    Yn_SetFontSize(0x12, 0x12);
    bar = gYnHelpBars[type];
    count = bar.count;
    names = gYnHelpBarLabels;
    x = (640 - bar.width) / 2.0f;
    for (i = 0; i < count; i++) {
        hint = gYnHelpHints[bar.items[i]];
        Hud_DrawButtonIcon(*hint++, x, 0x188, 0x155, -1, 0.8f);
        x += 0x18;
        text = names[*hint];
        len = Str_Length(text);
        Yn_Print(x, 0x188, text);
        if (type == YN_HELPBAR_LIST) {
            x += len * 9 + 0x1A;
        } else {
            x += len * 10 + 0x36;
        }
    }
    Yn_SetFontSize(w, (s16)height);
}

u32 gYnNetcnfArgs[32] = {
#include "yn/00AD21E0.inc"
};
u32 gYnNetcnfifArgs[8] = {
#include "yn/00AD2260.inc"
};
/*
 * Loads the netcnf and netcnfif IOP modules from the disc with
 * gYnNetcnfArgs and gYnNetcnfifArgs, then retries yn_entry until it
 * succeeds. Returns 0; n is unused.
 */
s32 YnNetcnf_LoadModules(YnNetcnf *n)
{
    Yn_LoadModule("cdrom0:\\IOP\\MODULES\\NETICON\\NETCNF.IRX;1", 0x7F, (char *)gYnNetcnfArgs, 0, 0);
    Yn_LoadModule("cdrom0:\\IOP\\MODULES\\NETICON\\NETCNFIF.IRX;1", 0x16, (char *)gYnNetcnfifArgs, 0, 0);
    func_00AC8128();
    while (yn_entry() < 0) {
    }
    return 0;
}

/*
 * Allocates netcnfif's work area (each call retried or waited on) and
 * keeps the result in n->result. Returns -1 on failure, else clears the
 * no-decode flag and returns 0.
 */
s32 YnNetcnf_Open(YnNetcnf *n)
{
    while (sceNetcnfifAllocWorkarea(0x1000) < 0) {
    }
    while (sceNetcnfifCheck() != 0) {
    }
    while (sceNetcnfifGetResult(&n->result) < 0) {
    }
    if (n->result < 0) {
        return -1;
    }
    sceNetcnfifSetFNoDecode(0);
    return 0;
}

/*
 * Frees netcnfif's work area, then stops and unloads the netcnfif and
 * netcnf modules when they are loaded. Returns 0.
 */
s32 YnNetcnf_Close(YnNetcnf *n)
{
    s32 id;

    while (sceNetcnfifFreeWorkarea() < 0) {
    }
    while (sceNetcnfifCheck() != 0) {
    }
    func_00AC8DC0();
    id = sceSifSearchModuleByName("Netcnf_Interface");
    if (0 <= id) {
        Yn_UnloadModule(id);
    }
    id = sceSifSearchModuleByName("NET_configuration");
    if (0 <= id) {
        Yn_UnloadModule(id);
    }
    return 0;
}

/*
 * Returns the usrName of the type 1 entry whose sysName is name (seen: an
 * entry's attachIfc), or NULL.
 */
char *YnNetcnf_FindUsrName(YnNetcnf *n, char *name)
{
    s32 i;

    for (i = 0; i < n->count[1]; i++) {
        if (strcmp(n->list1[i].sysName, name) == 0) {
            return n->list1[i].usrName;
        }
    }
    return NULL;
}

/*
 * Fills netcnf interface data from cfg: PPPoE (user, password, MTU 1454),
 * DHCP or a fixed address, given or negotiated DNS. name gets cfg->name
 * through Yn_SjisToUtf8.
 */
void Yn_ConfigToIfc(YnConfig *cfg, NetcnfData *data, char *name)
{
    sceNetcnfifDataInit(data);
    data->auth = 0;
    if (cfg->ifcType3 == 1) {
        data->ifcType = 3;
    } else {
        data->ifcType = 1;
    }
    if (cfg->noPppoe == 0) {
        data->pppoe = 1;
        data->dhcp = 0;
        strcpy(data->authName, cfg->authName);
        strcpy(data->authKey, cfg->authKey);
        strcpy(data->peerName, "*");
        data->fAuth = 1;
        data->auth = 4;
        data->prcNego = 0;
        data->accNego = 0;
        data->accmNego = 0;
        data->mtu = 0x5AE;
        data->ifcType = 2;
        data->ifcIdleTimeout = 0;
    } else {
        data->pppoe = 0xFF;
        if (cfg->fixedIp == 0) {
            data->dhcp = 1;
            strcpy(data->dhcpHostname, cfg->dhcpHostname);
        } else {
            data->dhcp = 0;
            Yn_FormatAddress(data->address, cfg->address);
            Yn_FormatAddress(data->netmask, cfg->netmask);
            Yn_FormatAddress(data->gateway, cfg->gateway);
        }
    }
    if (cfg->dnsSet != 0) {
        if (Yn_IsAddressSet(cfg->dns1) != 0) {
            Yn_FormatAddress(data->dns1Address, cfg->dns1);
        }
        if (Yn_IsAddressSet(cfg->dns2) != 0) {
            Yn_FormatAddress(data->dns2Address, cfg->dns2);
        }
        if (data->dns1Address[0] == 0) {
            memset(data->dns1Address, 0, 0x100);
            memset(data->dns2Address, 0, 0x100);
            data->dns1Nego = 1;
            data->dns2Nego = 1;
        }
        if (data->dns2Address[0] == 0) {
            memset(data->dns2Address, 0, 0x100);
        }
    } else if (data->dhcp != 1) {
        data->dns1Nego = 1;
        data->dns2Nego = 1;
    }
    Yn_SjisToUtf8(name, cfg->name);
}

/*
 * Fills cfg (cleared first) from interface data: PPPoE user and password,
 * or the DHCP host name or fixed addresses, and the DNS servers when set;
 * cfg->name gets name through Yn_Utf8ToSjis.
 */
void Yn_IfcToConfig(YnConfig *cfg, NetcnfData *data, char *name)
{
    memset(cfg, 0, 0xA20);
    cfg->ifcType = data->ifcType;
    if (data->pppoe == 1) {
        strcpy(cfg->authName, data->authName);
        strcpy(cfg->authKey, data->authKey);
    } else {
        cfg->noPppoe = 1;
        if (data->dhcp != 0xFF) {
            if (data->dhcp == 1) {
                strcpy(cfg->dhcpHostname, data->dhcpHostname);
            } else {
                cfg->fixedIp = 1;
                Yn_ParseAddress((u8 *)data->address, cfg->address);
                Yn_ParseAddress((u8 *)data->netmask, cfg->netmask);
                Yn_ParseAddress((u8 *)data->gateway, cfg->gateway);
            }
        }
    }
    if (data->dns1Address[0] != 0) {
        cfg->dnsSet = 1;
        Yn_ParseAddress((u8 *)data->dns1Address, cfg->dns1);
        Yn_ParseAddress((u8 *)data->dns2Address, cfg->dns2);
    }
    if (Yn_IsAddressSet(cfg->dns1) == 0) {
        cfg->dnsSet = 0;
        cfg->dns1[0] = 0;
        cfg->dns2[0] = 0;
        cfg->dns1[1] = 0;
        cfg->dns2[1] = 0;
        cfg->dns1[2] = 0;
        cfg->dns2[2] = 0;
        cfg->dns1[3] = 0;
        cfg->dns2[3] = 0;
    }
    Yn_Utf8ToSjis(cfg->name, name);
}

/*
 * Fills netcnf device data from cfg: vendor, product, devType (3 when
 * ifcType3 is 1, else 1) and phyConfig from cfg's ethernet mode.
 */
void Yn_ConfigToDev(YnConfig *cfg, NetcnfData *data)
{
    sceNetcnfifDataInit(data);
    strcpy(data->vendor, cfg->vendor);
    strcpy(data->product, cfg->product);
    if (cfg->ifcType3 == 1) {
        data->devType = 3;
    } else {
        data->devType = 1;
    }
    switch (cfg->phyConfig) {
    case 0:
    default:
        data->phyConfig = 1;
        break;
    case 1:
        data->phyConfig = 2;
        break;
    case 2:
        data->phyConfig = 3;
        break;
    case 3:
        data->phyConfig = 5;
        break;
    case 4:
        data->phyConfig = 6;
        break;
    }
}

/*
 * Copies the device data's vendor and product into cfg and maps its
 * phyConfig back to cfg's ethernet mode (0 for auto or unknown).
 */
void Yn_DevToConfig(YnConfig *cfg, NetcnfData *data)
{
    strcpy(cfg->vendor, data->vendor);
    strcpy(cfg->product, data->product);
    switch (data->phyConfig) {
    case 1:
    default:
        cfg->phyConfig = 0;
        break;
    case 2:
        cfg->phyConfig = 1;
        break;
    case 3:
        cfg->phyConfig = 2;
        break;
    case 5:
        cfg->phyConfig = 3;
        break;
    case 6:
        cfg->phyConfig = 4;
        break;
    }
}

/*
 * Fills one YnCombo per type 0 entry: its data, vendor and product, the
 * device kind (Yn_GetDeviceKind) and the attached device's index
 * (Yn_FindDevice). Leaves type 1 selected.
 */
void Yn_BuildCombos(YnNetcnf *n, YnCombo *combo)
{
    s32 i;

    YnNetcnf_SetType(n, 1);
    for (i = 0; i < n->count[0]; i++) {
        combo->vendor = n->data[i].vendor;
        combo->product = n->data[i].product;
        combo->data = &n->data[i];
        if (combo->vendor == NULL || combo->product == NULL) {
            combo->kind = 0;
            combo->device = -2;
        } else {
            combo->kind = Yn_GetDeviceKind(combo->vendor, combo->product);
            combo->device = Yn_FindDevice(combo->vendor, combo->product);
        }
        combo++;
    }
}

/*
 * Returns the index of the first of the n combos whose entry matches cfg
 * (device, DHCP host, PPPoE user and password, address, netmask, gateway
 * and both DNS servers), or -1.
 */
s32 Yn_FindCombination(YnConfig *cfg, YnCombo *combo, s32 n)
{
    u32 addr;
    char name[0x200];
    struct {
        NetcnfData data;
        u8 spare[0x30];
    } ifcx;
    NetcnfData dev;
    NetcnfData *entry;
    s32 i;

    name[0] = 0;
    Yn_ConfigToIfc(cfg, &ifcx.data, name);
    Yn_ConfigToDev(cfg, &dev);
    for (i = 0; i < n; i++, combo++) {
        entry = combo->data;
        if (entry != NULL && strcmp(entry->vendor, dev.vendor) == 0 && strcmp(entry->product, dev.product) == 0
            && entry->dhcp == ifcx.data.dhcp && strcmp(entry->dhcpHostname, ifcx.data.dhcpHostname) == 0
            && entry->pppoe == ifcx.data.pppoe && strcmp(entry->authName, ifcx.data.authName) == 0
            && strcmp(entry->authKey, ifcx.data.authKey) == 0) {
            addr = 0;
            Yn_ParseAddress((u8 *)entry->address, (u8 *)&addr);
            if (addr == *(u32 *)cfg->address) {
                addr = 0;
                Yn_ParseAddress((u8 *)entry->netmask, (u8 *)&addr);
                if (addr == *(u32 *)cfg->netmask) {
                    addr = 0;
                    Yn_ParseAddress((u8 *)entry->gateway, (u8 *)&addr);
                    if (addr == *(u32 *)cfg->gateway) {
                        addr = 0;
                        Yn_ParseAddress((u8 *)entry->dns1Address, (u8 *)&addr);
                        if (addr == *(u32 *)cfg->dns1) {
                            addr = 0;
                            Yn_ParseAddress((u8 *)entry->dns2Address, (u8 *)&addr);
                            if (addr == *(u32 *)cfg->dns2) {
                                return i;
                            }
                        }
                    }
                }
            }
        }
    }
    return -1;
}

/*
 * Returns 0 when the two proxy settings match (use, hosts and ports),
 * else -1.
 */
s32 Yn_CompareProxy(YnEntryKey *edited, YnEntryKey *inUse)
{
    if (edited->use != inUse->use) {
        return -1;
    }
    if (strcmp(edited->http, inUse->http) != 0) {
        return -1;
    }
    if (edited->httpPort != inUse->httpPort) {
        return -1;
    }
    if (strcmp(edited->https, inUse->https) != 0) {
        return -1;
    }
    if (edited->httpsPort != inUse->httpsPort) {
        return -1;
    }
    return 0;
}

/* Returns 1 when any of the address's four bytes is nonzero, else 0. */
s32 Yn_IsAddressSet(u8 *addr)
{
    s32 i;

    for (i = 0; i < 4; i++, addr++) {
        if (!*addr == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * Selects entry type 0, 1 or 2: points list, curData and countp at that
 * type's list, data and count.
 */
void YnNetcnf_SetType(YnNetcnf *n, s32 type)
{
    n->type = type;
    switch (type) {
    case 0:
        n->list = n->list0;
        n->curData = &n->cur[0];
        n->countp = &n->count[0];
        break;
    case 1:
        n->list = n->list1;
        n->curData = &n->cur[1];
        n->countp = &n->count[1];
        break;
    case 2:
        n->list = n->list2;
        n->curData = &n->cur[2];
        n->countp = &n->count[2];
        break;
    }
}

/*
 * Asks netcnfif how many entries of the selected type the file at path
 * has (waiting for the answer) and stores it in *countp. Returns it; 0 or
 * a negative error when there are none.
 */
s32 YnNetcnf_GetCount(YnNetcnf *n)
{
    sceNetcnfifGetCount(n->path, n->type);
    while (sceNetcnfifCheck() != 0) {
    }
    sceNetcnfifGetResult(&n->result);
    *n->countp = n->result;
    if (n->result <= 0) {
        return n->result;
    }
    return *n->countp;
}

/*
 * Reads the selected type's *countp entries into list (waiting for the
 * answer). Returns the count, or netcnfif's result when it is 0 or less.
 */
s32 YnNetcnf_GetList(YnNetcnf *n)
{
    if (*n->countp == 0) {
        return 0;
    }
    FlushCache(0);
    sceNetcnfifGetList(n->path, n->type, n->list, *n->countp);
    while (sceNetcnfifCheck() != 0) {
    }
    sceNetcnfifGetResult(&n->result);
    if (n->result <= 0) {
        return n->result;
    }
    return *n->countp;
}

/*
 * Loads every type 0 entry's full data into n->data: the count, the list,
 * then each entry by its usrName. Leaves type 0 selected.
 */
void YnNetcnf_LoadCombos(YnNetcnf *n)
{
    s32 i;

    memset(n->data, 0, sizeof(n->data));
    YnNetcnf_SetType(n, 0);
    YnNetcnf_GetCount(n);
    YnNetcnf_GetList(n);
    for (i = 0; i < n->count[0]; i++) {
        sceNetcnfifDataInit(&n->data[i]);
        FlushCache(0);
        sceNetcnfifLoadEntry(n->path, n->type, n->list[i].usrName, &n->data[i]);
        while (sceNetcnfifCheck() != 0) {
        }
    }
}

/*
 * Returns 1 when the file has no type 0 entries (or the count fails).
 * Keeps the selected type in savedType but leaves type 0 selected.
 */
s32 YnNetcnf_IsEmpty(YnNetcnf *n)
{
    n->savedType = n->type;
    YnNetcnf_SetType(n, 0);
    return YnNetcnf_GetCount(n) <= 0;
}

/*
 * Loads the first entry of the first non-empty type (0, 1, then 2) as a
 * test. Returns -1 when netcnfif answers -15 (seen: the list then shows
 * gYnOtherPs2Lines), else 0, also when the file has no entries.
 */
s32 YnNetcnf_CheckFile(YnNetcnf *n)
{
    n->savedType = n->type;
    YnNetcnf_SetType(n, 0);
    if (YnNetcnf_GetCount(n) > 0) {
        YnNetcnf_GetList(n);
        return YnNetcnf_LoadFirst(n);
    }
    YnNetcnf_SetType(n, 1);
    if (YnNetcnf_GetCount(n) > 0) {
        YnNetcnf_GetList(n);
        return YnNetcnf_LoadFirst(n);
    }
    YnNetcnf_SetType(n, 2);
    if (YnNetcnf_GetCount(n) > 0) {
        YnNetcnf_GetList(n);
        return YnNetcnf_LoadFirst(n);
    }
    return 0;
}

/*
 * Loads the first entry of the selected list into a local (retrying until
 * netcnfif takes the call), then reselects savedType. Returns -1 when the
 * result is -15, else 0.
 */
s32 YnNetcnf_LoadFirst(YnNetcnf *n)
{
    NetcnfData data;

    sceNetcnfifDataInit(&data);
    FlushCache(0);
    while (sceNetcnfifLoadEntry(n->path, n->type, n->list->usrName, &data) < 0) {
    }
    while (sceNetcnfifCheck() != 0) {
    }
    sceNetcnfifGetResult(&n->result);
    YnNetcnf_SetType(n, n->savedType);
    return -(n->result == -15);
}

void YnNetcnf_SetPath(YnNetcnf *n, s32 slot)
{
    sprintf(n->path, "mc%d:%s", slot, "/BWNETCNF/BWNETDCNF");
}

/* Parses a dotted address string into its four bytes (no checks). */
void Yn_ParseAddress(u8 *str, u8 *addr)
{
    u8 octet;

    octet = 0;
    for (; *str != 0; str++) {
        if (*str == '.') {
            *addr++ = octet;
            octet = 0;
        } else {
            octet *= 10;
            octet += *str - '0';
        }
    }
    *addr = octet;
}

void Yn_FormatAddress(char *str, u8 *addr)
{
    sprintf(str, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
}

/*
 * Stops the device in use, if any, then clears and refills the list of
 * attached network devices (NetDev_ClearList, NetDev_Refresh).
 */
void Yn_ResetNetDevices(void)
{
    if (gNetDevice != 0) {
        Net_StopDevice();
    }
    NetDev_ClearList();
    NetDev_Refresh();
}

/*
 * Returns the gNetDevs index of the attached device named vendor and
 * product; else -1 when it is a known model (NetDev_IsKnownName), -2 when
 * it is not.
 */
s32 Yn_FindDevice(char *vendor, char *product)
{
    s32 i;
    NetDevice *dev;

    for (i = 0, dev = gNetDevs; i < 35; i++, dev++) {
        if (dev->type != -1 && strcmp(dev->vendor, vendor) == 0 && strcmp(dev->product, product) == 0) {
            return i;
        }
    }
    return (NetDev_IsKnownName(vendor, product) != 0) ? -1 : -2;
}

/*
 * Returns 1 when the device named vendor and product is attached with
 * type 1, 2 when it is attached with another type, 0 when not attached.
 */
s32 Yn_GetDeviceKind(char *vendor, char *product)
{
    s32 i;
    NetDevice *dev;

    for (i = 0, dev = gNetDevs; i < 35; i++, dev++) {
        if (dev->type != -1 && strcmp(dev->vendor, vendor) == 0 && strcmp(dev->product, product) == 0) {
            if (gNetDevs[i].type == 1) {
                return 1;
            }
            return 2;
        }
    }
    return 0;
}

/*
 * Makes gNetDevs[i] the device in use and loads the drivers; returns 1.
 * With a device already in use returns 0 if it is this one, else -1;
 * -2 for a device not of type 1.
 */
s32 Yn_SelectDevice(s32 i)
{
    NetDevice *dev;

    dev = &gNetDevs[i];
    if (gNetDevice != NULL) {
        return -(gNetDevice != dev);
    }
    if (dev->type != 1) {
        return -2;
    }
    gNetDevice = dev;
    Net_LoadDrivers();
    Net_LoadDriverType1();
    return 1;
}

/*
 * Converts UTF-8 src to Shift-JIS in dst: ASCII is copied, hiragana and
 * katakana are mapped, any other three-byte character becomes one fixed
 * placeholder character.
 */
void Yn_Utf8ToSjis(char *dstp, char *srcp)
{
    u8 *dst = (u8 *)dstp;
    u8 *src = (u8 *)srcp;
    u16 code;

    code = *src;
    while (code != 0) {
        if (code >= 0x80) {
            code = (src[1] << 8) + *(src += 2);
            src++;
            if (code >= 0x8181 && code < 0x81C0) {
                code += 0x11E;
            } else if (code >= 0x8280 && code < 0x8294) {
                code += 0x5E;
            } else if (code >= 0x82A1 && code < 0x82C0) {
                code += 0x9F;
            } else if (code >= 0x8380 && code < 0x83A0) {
                code -= 0x21;
            } else if (code >= 0x83A0 && code < 0x83B7) {
                code -= 0x20;
            } else {
                code = 0x815B;
            }
            dst[0] = (code >> 8) & 0xFF;
            dst[1] = code & 0xFF;
            dst += 2;
        } else {
            *dst = code;
            src++;
            dst++;
        }
        code = *src;
    }
    *dst = 0;
}

/*
 * Converts Shift-JIS src to UTF-8 in dst: ASCII is copied, kana become
 * three-byte sequences, other two-byte characters one fixed placeholder;
 * stops before the output passes 255 bytes.
 */
void Yn_SjisToUtf8(char *dstp, char *srcp)
{
    u8 *dst = (u8 *)dstp;
    u8 *src = (u8 *)srcp;
    s32 n;
    u16 code;

    n = 0;
    code = *src;
    while (code != 0) {
        if (code >= 0x80) {
            if (n >= 0xFD) {
                break;
            }
            code = ((code & 0xFF) << 8) + src[1];
            src += 2;
            if (code >= 0x829F && code < 0x82DE) {
                code -= 0x11E;
            } else if (code >= 0x82DE && code < 0x82F2) {
                code -= 0x5E;
            } else if (code >= 0x8340 && code < 0x835F) {
                code -= 0x9F;
            } else if (code >= 0x835F && code < 0x837F) {
                code += 0x21;
            } else if (code >= 0x8380 && code < 0x8397) {
                code += 0x20;
            } else {
                code = 0x83BC;
            }
            dst[0] = 0xE3;
            dst[1] = (code >> 8) & 0xFF;
            n += 3;
            *(dst += 2) = code & 0xFF;
            dst++;
        } else {
            if (n >= 0xFF) {
                break;
            }
            *dst = code;
            src++;
            dst++;
            n++;
        }
        code = *src;
    }
    *dst = 0;
}

/*
 * Stops IOP module id, then unloads it, retrying each call until the IOP
 * accepts it.
 */
void Yn_UnloadModule(s32 id)
{
    s32 result;
    s32 ret;

    while ((ret = sceSifStopModule(id, 0, 0, &result)) < 0) {
        YN_WAIT();
    }
    if (0 <= ret) {
        while (sceSifUnloadModule(id) < 0) {
            YN_WAIT();
        }
    }
}

/*
 * Loads the IOP module at path with argc and argv: sceSifLoadModule, or
 * sceSifLoadStartModule (given buf) when buf is set. tries 0 retries
 * until it works, else gives up after tries attempts.
 */
void Yn_LoadModule(char *path, s32 argc, char *argv, void *buf, s32 tries)
{
    s32 i;

    if (buf == NULL) {
        if (tries == 0) {
            while (sceSifLoadModule(path, argc, argv) < 0) {
                YN_WAIT();
            }
        } else {
            for (i = 0; i < tries; i++) {
                if (0 <= sceSifLoadModule(path, argc, argv)) {
                    i = 0;
                    break;
                }
                YN_WAIT7();
            }
        }
    } else {
        if (tries == 0) {
            while (sceSifLoadStartModule(path, argc, argv, buf) < 0) {
                YN_WAIT();
            }
        } else {
            for (i = 0; i < tries; i++) {
                if (0 <= sceSifLoadStartModule(path, argc, argv, buf)) {
                    i = 0;
                    break;
                }
                YN_WAIT7();
            }
        }
    }
}

