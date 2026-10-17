/*
 * yn: the network settings screens (0xAC9B90-0xAD1180), the Capcom part of
 * the overlay after the memory card checks (yn_card.c). The file owns its
 * .data (0xAD11A0-0xAD2280: the screens' text tables), .rodata
 * (0xAD2780-0xAD3A80) and .bss (0xAD9B50-0xB1A180).
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
#include "syssave.h"

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
void Font_SetHalfWidth(s8 mode); /* unit's own declaration: with the definition's void (s32 a) (sub_main/unk_2E3940.c), Yn_SetTextMode 3 of 3 words differ */
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
extern YnConfig D_5BE9E0;      /* sub_main: the settings in use */
extern YnProxy D_5BE6C4;       /* sub_main: the proxy settings */
extern YnProxy D_5BF400;
/* sub_main: the AVE network settings */
extern char gNetAuthKey[0x100];
extern char gNetAuthName[0x100];
extern char gNetDomain[0x100];
extern char D_59DBE0[0x10];
extern char D_59DBF0[0xF0];
extern char D_59DD00[0x100];
extern char gHttpsProxy[0x110];         /* the HTTPS proxy URL */
extern char gHttpProxy[0x110];         /* the HTTP proxy URL */
void Window_DrawTextAlpha(); /* unit's own declaration: the definition (sub_main) takes (Window *, u8 *text, u8 alpha); Yn_DrawBackground also passes &rect in a3, which it ignores (a native build drops it), and a fixed 4-parameter type reschedules that function */
s32 Prim_FillRect(PrimLine *rect);
void Hud_DrawButtonIcon(s16 id, s16 x, s16 y, s16 depth, u32 color, f32 scale); /* unit's own declaration:
 * the definition (sub_main) takes s32 id, x, y, depth: Yn_DrawHelpBar is 31 words off with them */
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
void func_00ACDF60(void);
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
void func_00ACBF40(void);
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
/* " " and "" come here in the game's .rodata, before the next table's
 * strings, although the first table that points to "" is D_00AD1350: they
 * were probably first used by a function the linker dead-stripped. */
const char D_00AD28D8[2] = {
#include "yn/00AD28D8.inc"
};
const char D_00AD28E0[1] = {
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
YnItem D_00AD1328[1] = {
    {0, 0, (char *)D_00AD28D8},
};
YnItem D_00AD1330[4] = {
#include "yn/00AD1330.inc"
};
YnItem D_00AD1350[4] = {
#include "yn/00AD1350.inc"
};
YnItem D_00AD1370[2] = {
#include "yn/00AD1370.inc"
};
YnItem D_00AD1380[2] = {
#include "yn/00AD1380.inc"
};
YnItem D_00AD1390[2] = {
#include "yn/00AD1390.inc"
};
YnItem D_00AD13A0[2] = {
#include "yn/00AD13A0.inc"
};
YnItem D_00AD13B0[2] = {
#include "yn/00AD13B0.inc"
};
YnItem D_00AD13C0[6] = {
#include "yn/00AD13C0.inc"
};
YnItem D_00AD13F0[4] = {
#include "yn/00AD13F0.inc"
};
YnItem D_00AD1410[6] = {
#include "yn/00AD1410.inc"
};
YnItem D_00AD1440[4] = {
#include "yn/00AD1440.inc"
};
YnItem D_00AD1460[4] = {
#include "yn/00AD1460.inc"
};
YnItem D_00AD1480[4] = {
#include "yn/00AD1480.inc"
};
YnItem D_00AD14A0[2] = {
#include "yn/00AD14A0.inc"
};
YnItem D_00AD14B0[4] = {
#include "yn/00AD14B0.inc"
};
YnItem D_00AD14D0[2] = {
#include "yn/00AD14D0.inc"
};
YnItem D_00AD14E0[2] = {
#include "yn/00AD14E0.inc"
};
YnItem D_00AD14F0[2] = {
#include "yn/00AD14F0.inc"
};
YnItem D_00AD1500[2] = {
#include "yn/00AD1500.inc"
};
YnItem D_00AD1510[4] = {
#include "yn/00AD1510.inc"
};
YnItem D_00AD1530[6] = {
#include "yn/00AD1530.inc"
};
YnItem D_00AD1560[2] = {
#include "yn/00AD1560.inc"
};
YnItem *gYnScreenItems[28] = {
    D_00AD1328, D_00AD1330, D_00AD1350, D_00AD1370,
    D_00AD1380, D_00AD1390, D_00AD13A0, D_00AD13B0,
    D_00AD13C0, D_00AD13F0, D_00AD1410, D_00AD1440,
    D_00AD1460, D_00AD1480, D_00AD14A0, D_00AD14B0,
    D_00AD14D0, D_00AD14E0, D_00AD14F0, D_00AD1328,
    D_00AD1328, D_00AD1500, D_00AD1510, D_00AD1530,
    D_00AD1560, NULL, NULL, NULL,
};
s8 gYnScreenItemCounts[32] = {
#include "yn/00AD15E0.inc"
};
YnItem D_00AD1600[2] = {
    {0, 0, (char *)D_00AD28D8},
    {0, 0, NULL},
};
YnItem D_00AD1610[4] = {
#include "yn/00AD1610.inc"
};
YnItem D_00AD1630[6] = {
#include "yn/00AD1630.inc"
};
YnItem D_00AD1660[6] = {
#include "yn/00AD1660.inc"
};
YnItem D_00AD1690[4] = {
#include "yn/00AD1690.inc"
};
YnItem D_00AD16B0[4] = {
#include "yn/00AD16B0.inc"
};
YnItem D_00AD16D0[2] = {
#include "yn/00AD16D0.inc"
};
YnItem D_00AD16E0[2] = {
#include "yn/00AD16E0.inc"
};
YnItem D_00AD16F0[6] = {
#include "yn/00AD16F0.inc"
};
YnItem D_00AD1720[2] = {
#include "yn/00AD1720.inc"
};
YnItem D_00AD1730[6] = {
#include "yn/00AD1730.inc"
};
YnItem D_00AD1760[4] = {
#include "yn/00AD1760.inc"
};
YnItem D_00AD1780[2] = {
#include "yn/00AD1780.inc"
};
YnItem D_00AD1790[2] = {
#include "yn/00AD1790.inc"
};
YnItem D_00AD17A0[4] = {
#include "yn/00AD17A0.inc"
};
YnItem D_00AD17C0[2] = {
#include "yn/00AD17C0.inc"
};
YnItem D_00AD17D0[4] = {
#include "yn/00AD17D0.inc"
};
YnItem D_00AD17F0[4] = {
#include "yn/00AD17F0.inc"
};
YnItem D_00AD1810[4] = {
#include "yn/00AD1810.inc"
};
YnItem D_00AD1830[1] = {
#include "yn/00AD1830.inc"
};
YnItem D_00AD1838[1] = {
#include "yn/00AD1838.inc"
};
YnItem D_00AD1840[1] = {
#include "yn/00AD1840.inc"
};
YnItem D_00AD1848[1] = {
#include "yn/00AD1848.inc"
};
YnItem D_00AD1850[4] = {
#include "yn/00AD1850.inc"
};
YnItem D_00AD1870[3] = {
#include "yn/00AD1870.inc"
};
YnItem D_00AD1888[1] = {
#include "yn/00AD1888.inc"
};
YnItem D_00AD1890[4] = {
#include "yn/00AD1890.inc"
};
YnItem D_00AD18B0[2] = {
#include "yn/00AD18B0.inc"
};
YnItem D_00AD18C0[4] = {
#include "yn/00AD18C0.inc"
};
YnItem D_00AD18E0[4] = {
#include "yn/00AD18E0.inc"
};
YnItem D_00AD1900[2] = {
#include "yn/00AD1900.inc"
};
YnItem D_00AD1910[6] = {
#include "yn/00AD1910.inc"
};
YnItem D_00AD1940[4] = {
#include "yn/00AD1940.inc"
};
YnItem D_00AD1960[4] = {
#include "yn/00AD1960.inc"
};
YnItem D_00AD1980[4] = {
#include "yn/00AD1980.inc"
};
YnItem D_00AD19A0[4] = {
#include "yn/00AD19A0.inc"
};
YnItem D_00AD19C0[4] = {
#include "yn/00AD19C0.inc"
};
YnItem D_00AD19E0[8] = {
#include "yn/00AD19E0.inc"
};
YnItem D_00AD1A20[6] = {
#include "yn/00AD1A20.inc"
};
YnItem D_00AD1A50[6] = {
#include "yn/00AD1A50.inc"
};
YnItem D_00AD1A80[6] = {
#include "yn/00AD1A80.inc"
};
YnItem D_00AD1AB0[6] = {
#include "yn/00AD1AB0.inc"
};
YnItem D_00AD1AE0[4] = {
#include "yn/00AD1AE0.inc"
};
YnItem D_00AD1B00[4] = {
#include "yn/00AD1B00.inc"
};
YnItem D_00AD1B20[2] = {
#include "yn/00AD1B20.inc"
};
YnItem D_00AD1B30[4] = {
#include "yn/00AD1B30.inc"
};
YnItem D_00AD1B50[6] = {
#include "yn/00AD1B50.inc"
};
YnItem D_00AD1B80[4] = {
#include "yn/00AD1B80.inc"
};
YnItem D_00AD1BA0[6] = {
#include "yn/00AD1BA0.inc"
};
YnItem D_00AD1BD0[6] = {
#include "yn/00AD1BD0.inc"
};
YnItem D_00AD1C00[2] = {
#include "yn/00AD1C00.inc"
};
YnItem D_00AD1C10[4] = {
#include "yn/00AD1C10.inc"
};
YnItem D_00AD1C30[4] = {
#include "yn/00AD1C30.inc"
};
YnItem D_00AD1C50[6] = {
#include "yn/00AD1C50.inc"
};
YnItem D_00AD1C80[4] = {
#include "yn/00AD1C80.inc"
};
YnItem D_00AD1CA0[6] = {
#include "yn/00AD1CA0.inc"
};
YnItem D_00AD1CD0[4] = {
#include "yn/00AD1CD0.inc"
};
YnItem D_00AD1CF0[2] = {
#include "yn/00AD1CF0.inc"
};
YnItem *gYnMsgItems[60] = {
    D_00AD1600, D_00AD1610, D_00AD1630, D_00AD1660,
    D_00AD1690, D_00AD16B0, D_00AD16D0, D_00AD16E0,
    D_00AD16F0, D_00AD1720, D_00AD1730, D_00AD1760,
    D_00AD1780, D_00AD1790, D_00AD17A0, D_00AD17C0,
    D_00AD17D0, D_00AD17F0, D_00AD1810, D_00AD1830,
    D_00AD1838, D_00AD1840, D_00AD1848, D_00AD1850,
    D_00AD1870, D_00AD1888, D_00AD1890, D_00AD18B0,
    D_00AD18C0, D_00AD18E0, D_00AD1900, D_00AD1910,
    D_00AD1940, D_00AD1960, D_00AD1980, D_00AD19A0,
    D_00AD19C0, D_00AD19E0, D_00AD1A20, D_00AD1A50,
    D_00AD1A80, D_00AD1AB0, D_00AD1AE0, D_00AD1B00,
    D_00AD1B20, D_00AD1B30, D_00AD1B50, D_00AD1B80,
    D_00AD1BA0, D_00AD1BD0, D_00AD1C00, D_00AD1C10,
    D_00AD1C30, D_00AD1C50, D_00AD1C80, D_00AD1CA0,
    D_00AD1CD0, D_00AD1CF0, NULL, NULL,
};
s8 gYnMsgItemCounts[64] = {
#include "yn/00AD1DF0.inc"
};
YnItem D_00AD1E30[2] = {
    {0, 0, (char *)D_00AD28D8},
    {0, 0, NULL},
};
YnItem D_00AD1E40[2] = {
#include "yn/00AD1E40.inc"
};
YnItem D_00AD1E50[4] = {
#include "yn/00AD1E50.inc"
};
YnItem D_00AD1E70[2] = {
#include "yn/00AD1E70.inc"
};
YnItem D_00AD1E80[4] = {
#include "yn/00AD1E80.inc"
};
YnItem D_00AD1EA0[2] = {
#include "yn/00AD1EA0.inc"
};
YnItem D_00AD1EB0[2] = {
#include "yn/00AD1EB0.inc"
};
YnItem D_00AD1EC0[2] = {
#include "yn/00AD1EC0.inc"
};
YnItem D_00AD1ED0[2] = {
#include "yn/00AD1ED0.inc"
};
YnItem D_00AD1EE0[4] = {
#include "yn/00AD1EE0.inc"
};
YnItem D_00AD1F00[2] = {
#include "yn/00AD1F00.inc"
};
YnItem D_00AD1F10[2] = {
#include "yn/00AD1F10.inc"
};
YnItem D_00AD1F20[2] = {
#include "yn/00AD1F20.inc"
};
YnItem D_00AD1F30[2] = {
#include "yn/00AD1F30.inc"
};
YnItem D_00AD1F40[1] = {
#include "yn/00AD1F40.inc"
};
YnItem D_00AD1F48[1] = {
#include "yn/00AD1F48.inc"
};
YnItem D_00AD1F50[2] = {
#include "yn/00AD1F50.inc"
};
YnItem D_00AD1F60[2] = {
#include "yn/00AD1F60.inc"
};
YnItem D_00AD1F70[2] = {
#include "yn/00AD1F70.inc"
};
YnItem D_00AD1F80[2] = {
#include "yn/00AD1F80.inc"
};
YnItem D_00AD1F90[4] = {
#include "yn/00AD1F90.inc"
};
YnItem D_00AD1FB0[4] = {
#include "yn/00AD1FB0.inc"
};
YnItem D_00AD1FD0[2] = {
#include "yn/00AD1FD0.inc"
};
YnItem D_00AD1FE0[2] = {
    {0, 0, (char *)D_00AD28D8},
    {0, 0, NULL},
};
YnItem D_00AD1FF0[2] = {
#include "yn/00AD1FF0.inc"
};
YnItem D_00AD2000[2] = {
#include "yn/00AD2000.inc"
};
YnItem D_00AD2010[1] = {
#include "yn/00AD2010.inc"
};
YnItem D_00AD2018[1] = {
#include "yn/00AD2018.inc"
};
YnItem D_00AD2020[4] = {
#include "yn/00AD2020.inc"
};
YnItem D_00AD2040[4] = {
#include "yn/00AD2040.inc"
};
YnItem *gYnHelpItems[40] = {
    D_00AD1E30, D_00AD1E40, D_00AD1E50, D_00AD1E70,
    D_00AD1E80, D_00AD1EA0, D_00AD1EB0, D_00AD1EC0,
    D_00AD1ED0, D_00AD1ED0, D_00AD1ED0, D_00AD1ED0,
    D_00AD1ED0, D_00AD1EE0, D_00AD1F00, D_00AD1F10,
    D_00AD1F20, D_00AD1F30, D_00AD1F40, D_00AD1F48,
    D_00AD1F50, D_00AD1F60, D_00AD1F70, D_00AD1F80,
    D_00AD1F90, D_00AD1FB0, D_00AD1FD0, D_00AD1ED0,
    D_00AD1FE0, D_00AD1FF0, D_00AD1ED0, D_00AD2000,
    D_00AD2010, D_00AD2018, D_00AD2020, D_00AD2018,
    D_00AD2040, NULL, NULL, NULL,
};
s8 gYnHelpItemCounts[40] = {
#include "yn/00AD2100.inc"
};

s32 Yn_Init(s32 arg) {
    gYnWork = (YnWork *)gYnWorkBuf;
    memset(gYnWork, 0, 0x1AF00);
    gYnWork->state = 0;
    gYnWork->initArg = arg;
    Font_ResetState();
    if (arg == 0) {
        NetMode_OpenIcon7();
    }
    return 0;
}

s32 Yn_Update(void) {
    NetModeState *ns;
    s32 ret;

    ns = &gNetMode;
    if (gYnWork == NULL) {
        return -1;
    }
    ret = 0;
    func_00ACDF60();
    switch (gYnWork->state) {
    case 0:
        Yn_ResetStreams();
        Yn_ResetNetDevices();
        YnNetcnf_LoadModules(&gYnWork->netcnf);
        gYnWork->timer = 10;
        gYnWork->state = 1;
        break;
    case 1:
        if (--gYnWork->timer <= 0) {
            YnNetcnf_Open(&gYnWork->netcnf);
            Yn_LoadProxy();
            if (gYnWork->initArg == 0) {
                gYnWork->slot = 0;
                YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
                gYnWork->state = 2;
            } else {
                gYnWork->state = 3;
                ns->trySlot = 0;
                gYnWork->slot = ns->trySlot;
                YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
                gYnWork->step = 0;
            }
        }
        break;
    case 2:
        gYnWork->timer++;
        gYnWork->result = Yn_ScanCards();
        switch (gYnWork->result) {
        case 0:
            break;
        case 1:
            gYnWork->scanResult = 0;
        list:
            gYnWork->state = 4;
            gYnWork->scanStep = 0;
            gYnWork->unk8 = 0;
            gYnWork->msg = 0;
            break;
        case -1:
            gYnWork->scanResult = -1;
            goto list;
        case -2:
            gYnWork->scanResult = -2;
            goto list;
        case -3:
            gYnWork->scanResult = -3;
            goto list;
        }
        break;
    case 10:
        gYnWork->timer++;
        if (Yn_IsCardGone() != 0) {
            gYnWork->state = 2;
            gYnWork->step = 0;
        }
        break;
    case 3:
        gYnWork->timer++;
        switch (Yn_Connect()) {
        case 1:
            ns->error[ns->trySlot] = 0;
            gYnWork->state = 9;
            ns->ynResult = gYnWork->changed + 1;
            break;
        case -1:
            ns->error[ns->trySlot] = 0x1F;
            goto next;
        case -2:
            ns->error[ns->trySlot] = 0x20;
            goto next;
        case -3:
            ns->error[ns->trySlot] = 0x22;
            goto next;
        case -4:
            ns->error[ns->trySlot] = 0x1F;
            goto next;
        case -5:
            ns->error[ns->trySlot] = 0x1E;
            goto next;
        case -6:
            ns->error[ns->trySlot] = 0x21;
            goto next;
        case -7:
            ns->error[ns->trySlot] = 0x2D;
            goto next;
        case -8:
            ns->error[ns->trySlot] = 0x1F;
        next:
            if (ns->trySlot == 0) {
                ns->trySlot = 1;
                gYnWork->slot = ns->trySlot;
                YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
                gYnWork->scanStep = 0;
                gYnWork->step = 2;
            } else {
                gYnWork->state = 9;
                ns->ynResult = gYnWork->changed + 1;
            }
            break;
        }
        break;
    case 4:
        gYnWork->timer++;
        gYnWork->numEditing = 0;
        Yn_UpdatePadRepeat(gPadState);
        switch (Yn_UpdateEntryList()) {
        case 0:
            break;
        case 1:
            gYnWork->state = 5;
            gYnWork->step = 0;
            break;
        case -1:
            gYnWork->state = 6;
            gYnWork->step = 0;
            gYnWork->msg = 0x1A;
            gYnWork->quitAsked = 1;
            gYnWork->unk8 = 0;
            gYnWork->quitChoice = 1;
            LoginSe_Play9();
            break;
        case -2:
            gYnWork->state = 2;
            gYnWork->step = 0;
            break;
        }
        break;
    case 5:
        Yn_Quit();
        ns->ynResult = gYnWork->changed + 1;
        ret = ns->ynResult;
        break;
    case 6:
        gYnWork->helpBar = 1;
        gYnWork->unk47 = 2;
        gYnWork->timer++;
        if (gPadState[0].pressed & 0x40) {
            gYnWork->msg = 0;
            gYnWork->quitAsked = 0;
            LoginSe_Cancel();
            gYnWork->state = 4;
        } else if (gPadState[0].pressed & 0x20) {
            gYnWork->quitAsked = 0;
            if (gYnWork->quitChoice == 0) {
                LoginSe_Decide();
                Yn_Quit();
                ret = -1;
            } else {
                gYnWork->msg = 0;
                LoginSe_Cancel();
                gYnWork->state = 4;
            }
        } else if (gPadState[0].pressed & 0xC00) {
            gYnWork->quitChoice ^= 1;
            LoginSe_Cursor();
        }
        break;
    case 7:
        gYnWork->unk47 = 2;
        gYnWork->timer++;
        if ((gPadState[0].pressed & 0x20) || (gPadState[0].pressed & 0x40)) {
            LoginSe_Decide();
            Yn_Quit();
            ret = -1;
        }
        break;
    case 8:
        if ((gPadState[0].pressed & 0x20) || (gPadState[0].pressed & 0x40)) {
            gYnWork->state = 4;
            gYnWork->msg = 0;
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

void Yn_Draw(void) {
    Yn_DrawBackground();
    gYnWork->textMode = Yn_GetTextMode();
    Yn_SetTextMode(1);
    Yn_SetColor(0);
    Yn_SetFontSize(0x16, 0x16);
    Yn_SetGlyphSize(621.4285888671875f);
    func_00ACBF40();
    Yn_DrawScreen();
    Yn_DrawHelpBar(gYnWork->helpBar);
    Yn_DrawMessage();
    Yn_DrawHelp();
    Yn_DrawTextLayer();
    Yn_SetTextMode(gYnWork->textMode);
}

void Yn_Quit(void) {
    u32 hi;

    printf("!!!\n");
    if ((u16)gYnWork->texPal != 0) {
        flReleaseTextureHandle((u16)gYnWork->texPal);
    }
    hi = (gYnWork->texPal & 0xFFFF0000) >> 16;
    if (hi != 0) {
        flReleasePaletteHandle(hi);
    }
    Yn_Init(0);
    YnNetcnf_Close(&gYnWork->netcnf);
    Mem_Compact();
    NetMode_CloseBoxes(0x20);
}

/* Looks for the memory card slot to read the settings from (states 0-5 in
 * scanStep), then lists the file's combinations. The slot test is written
 * `!(...) != 0`: the game materializes the negation (sltu/xori) before
 * branching, which a plain `!(...)` doesn't give. */
s32 Yn_ScanCards(void) {
    s32 ret;
    s32 err;

    switch (gYnWork->scanStep) {
    case 0:
        gYnWork->scanStep++;
        gYnWork->prevSlot = YnCard_GetSlot(&gYnWork->card);
        gYnWork->cardSlots = 0;
        gYnWork->entryCount = 0;
        gYnWork->unk1F = 0;
        gYnWork->scanSlot = gYnWork->slot;
        YnCard_Reset(&gYnWork->card);
        memset(&gYnWork->netcnf, 0, sizeof(YnNetcnf));
        memset(gYnWork->combos, 0, sizeof(gYnWork->combos));
    case 1:
        if (YnCard_CheckSlots(&gYnWork->card) < 0) {
            break;
        }
        gYnWork->cardSlots |= ((gYnWork->card.okSlots | gYnWork->card.slots3) >> 1) & 3;
        if (gYnWork->cardSlots & (1 << gYnWork->prevSlot)) {
            gYnWork->scanStep = 2;
            break;
        }
        gYnWork->scanStep = 0;
        gYnWork->slot = gYnWork->prevSlot;
        YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
        return -2;
    case 2:
    case 3:
    loop:
        if (!(gYnWork->cardSlots & (1 << gYnWork->scanSlot)) != 0) {
            gYnWork->scanSlot = (gYnWork->scanSlot + 1) % 2;
            if (gYnWork->scanSlot != gYnWork->slot) {
                goto loop;
            }
        }
        if (gYnWork->scanStep == 3 && gYnWork->scanSlot == gYnWork->slot) {
            gYnWork->scanStep = 0;
            gYnWork->slot = gYnWork->scanSlot;
            YnCard_SetSlot(&gYnWork->card, gYnWork->scanSlot);
            return -1;
        }
        gYnWork->scanStep = 4;
        gYnWork->slot = gYnWork->scanSlot;
        YnCard_SetSlot(&gYnWork->card, gYnWork->scanSlot);
        break;
    case 4:
        ret = YnCard_ListNetcnfDir(&gYnWork->card);
        if (ret != -2) {
            if (ret != -1) {
                gYnWork->scanStep = 3;
            } else {
                gYnWork->scanStep = 5;
            }
        }
        break;
    case 5:
        YnNetcnf_SetPath(&gYnWork->netcnf, YnCard_GetSlot(&gYnWork->card));
        gYnWork->entryCount = 0;
        if (YnNetcnf_CheckFile(&gYnWork->netcnf) != 0) {
            return -3;
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
            gYnWork->scanStep = 0;
            return 1;
        }
        gYnWork->scanStep = 0;
        return -1;
    }
    return 0;
}

s32 Yn_ScanCardsForConnect(void) {
    s32 r;
    s32 missing;

    switch (gYnWork->scanStep) {
    case 0:
        gYnWork->scanStep++;
        gYnWork->prevSlot = YnCard_GetSlot(&gYnWork->card);
        gYnWork->cardSlots = 0;
        gYnWork->entryCount = 0;
        gYnWork->unk1F = 0;
        gYnWork->scanSlot = gYnWork->slot;
        YnCard_Reset(&gYnWork->card);
        memset(&gYnWork->netcnf, 0, sizeof(YnNetcnf));
        memset(gYnWork->combos, 0, sizeof(gYnWork->combos));
    case 1:
        if (YnCard_CheckSlots(&gYnWork->card) < 0) {
            break;
        }
        gYnWork->cardSlots |= ((gYnWork->card.okSlots | gYnWork->card.slots3) >> 1) & 3;
        if (gYnWork->cardSlots & (1 << gYnWork->prevSlot)) {
            gYnWork->scanStep = 2;
            break;
        }
        gYnWork->scanStep = 0;
        gYnWork->slot = gYnWork->prevSlot;
        YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
        return -2;
    case 2:
    case 3:
    loop:
        if (!(gYnWork->cardSlots & (1 << gYnWork->scanSlot)) != 0) {
            gYnWork->scanSlot = (gYnWork->scanSlot + 1) % 2;
            if (gYnWork->scanSlot != gYnWork->slot) {
                goto loop;
            }
        }
        if (gYnWork->scanStep == 3 && gYnWork->scanSlot == gYnWork->slot) {
            gYnWork->scanStep = 0;
            gYnWork->slot = gYnWork->scanSlot;
            YnCard_SetSlot(&gYnWork->card, gYnWork->scanSlot);
            return -1;
        }
        gYnWork->scanStep = 4;
        gYnWork->slot = gYnWork->scanSlot;
        YnCard_SetSlot(&gYnWork->card, gYnWork->scanSlot);
        break;
    case 4:
        r = YnCard_ListNetcnfDir(&gYnWork->card);
        if (r != -2) {
            if (r != -1) {
                gYnWork->scanStep = 3;
            } else {
                gYnWork->scanStep = 5;
            }
        }
        break;
    case 5:
        YnNetcnf_SetPath(&gYnWork->netcnf, YnCard_GetSlot(&gYnWork->card));
        gYnWork->entryCount = 0;
        if (YnNetcnf_CheckFile(&gYnWork->netcnf) != 0) {
            gYnWork->cardError[gYnWork->slot] = -3;
            return -3;
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
            gYnWork->scanStep = 0;
            return 1;
        }
        gYnWork->scanStep = 0;
        return -1;
    }
    return 0;
}

extern s8 gNetSetupResult;
/* Connects with the settings on the card (sub_main's request). The chosen
 * combination is addressed through the byte offset k added to the work
 * pointer as an integer, which keeps n * sizeof(YnCombo) in s0 across the
 * calls as the original does. (At level 3 the work pointer and the field
 * addresses of step/triedSlots/msgTimer trade registers; level 4 matches.) */
s32 Yn_Connect(void) {
    s32 r;
    s32 n;
    s32 dev;
    s32 k;

    switch (gYnWork->step) {
    case 0:
        gYnWork->step++;
        gYnWork->slot = 0;
        gYnWork->triedSlots = 0;
        gYnWork->cardError[0] = 0;
        gYnWork->cardError[1] = 0;
    case 1:
        if (Yn_CompareProxy((YnEntryKey *)&gYnWork->proxy, (YnEntryKey *)&D_5BF400) < 0) {
            return -4;
        }
        gYnWork->step++;
        gYnWork->scanStep = 0;
        break;
    case 2:
        r = Yn_ScanCardsForConnect();
        if (r == 1) {
            n = Yn_FindCombination(&D_5BE9E0, gYnWork->combos, gYnWork->entryCount);
            if (n >= 0) {
                k = n * sizeof(YnCombo);
                dev = ((YnWork *)(k + (s32)gYnWork))->combos[0].device;
                if (dev < 0) {
                    return -3;
                }
                if (Yn_SelectDevice(dev) < 0) {
                    return -3;
                }
                Yn_ApplyEntry(((YnWork *)(k + (s32)gYnWork))->combos[0].data);
                Yn_ExportSettings(((YnWork *)(k + (s32)gYnWork))->combos[0].data);
                if (gNetMode.ynNoDraw != 0) {
                    gYnWork->step = 0;
                    gYnWork->msg = 0;
                    LoginSe_Decide();
                    return 1;
                }
                gYnWork->step = 5;
                gYnWork->msg = 0x34;
                LoginSe_Play9();
                gYnWork->msgTimer = 60;
                break;
            }
        } else if (r == -2) {
            return -2;
        } else if (r == 0) {
            break;
        }
        if (gYnWork->triedSlots & 0x80) {
            if (gYnWork->triedSlots & (1 << gYnWork->slot)) {
                return -1;
            }
        } else {
            gYnWork->triedSlots |= 0x80;
        }
        gYnWork->triedSlots |= (u8)(1 << gYnWork->slot);
        if (YnNetcnf_IsEmpty(&gYnWork->netcnf) != 0) {
            return -7;
        }
        return (YnNetcnf_CheckFile(&gYnWork->netcnf) != 0) ? -5 : -8;
    case 3:
        gYnWork->step = 2;
        gYnWork->scanStep = 0;
        gYnWork->slot++;
        gYnWork->slot &= 1;
        break;
    case 4:
        if (Yn_IsCardGone() != 0) {
            gYnWork->step = 0;
            gYnWork->msg = 0;
            return -6;
        }
        if ((gPadState[0].pressed & 0x20) || gYnWork->initArg != 0) {
            gYnWork->step = 3;
            gYnWork->msg = 0;
            LoginSe_Decide();
        }
        break;
    case 5:
        if (gNetSetupResult == 1 && gSysSave.netOptions[1] != 0) {
            if (gYnWork->msgTimer <= 0) {
                goto cancel;
            }
            gYnWork->msgTimer--;
        }
        if (gPadState[0].pressed & 0x20) {
        cancel:
            gYnWork->step = 0;
            gYnWork->msg = 0;
            LoginSe_Decide();
            return 1;
        }
        break;
    }
    return 0;
}

void Yn_ResetNameScroll(void) {
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

void Yn_UpdateNameScroll(void) {
    char buf[0x100];
    char *p;
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
                p = buf;
                p += gYnWork->nameOfs;
                n = Sjis_CharBytes((u8 *)p);
                p += n;
                gYnWork->nameOfs += n;
                if (strlen(p) < 45) {
                    gYnWork->nameAtEnd = 1;
                    gYnWork->nameTimer = 40;
                } else {
                    gYnWork->nameTimer = 6;
                }
            }
        }
    }
}

s32 Yn_UpdateEntryList(void) {
    YnWork *w;
    char *p;
    s32 changed;
    s32 n;
    s32 m;
    u16 pad;
    s32 ok;
    s32 i;
    char *name;
    s32 dev;
    s32 idx;

    changed = 0;
    if (gYnWork->step != 0 && gYnWork->step != 7 && gYnWork->step != 8 && gYnWork->step != 9) {
        changed = Yn_IsCurrentCardGone();
    }
    switch (gYnWork->step) {
    case 0:
        gYnWork->step++;
        for (i = 1; i < 16; i++) {
            (&gYnWork->unk8)[i] = 0;
        }
        YnCard_Reset(&gYnWork->card);
        Yn_LoadProxy();
        Yn_ResetNameScroll();
        gYnWork->unk8 = 2;
        gYnWork->listBusy = 0;
        break;
    case 1:
        gYnWork->listBusy = 0;
        gYnWork->cardGone = changed;
        gYnWork->helpBar = 7;
        gYnWork->unk46 = 1;
        gYnWork->unk47 = (gYnWork->unk8 >= 2) ? 4 : 2;
        gYnWork->unk48 = 1;
        gYnWork->screen = 1;
        gYnWork->help = 1;
        gYnWork->helpVariant = (gYnWork->unk8 >= 2) ? 2 : gYnWork->unk8;
        if (changed != 0) {
            gYnWork->scanResult = -2;
        } else {
            pad = gPadState[0].pressed;
            if (pad & 0x40) {
                LoginSe_Cancel();
                return -1;
            }
            w = gYnWork;
            if (w->scanResult < 0) {
                if (w->scanResult == -2 && (w->okSlots & (1 << YnCard_GetSlot(&w->card)))) {
                    w->scanResult = 0;
                    return -2;
                }
            } else if (w->listBusy == 0) {
                if (pad & 0x20) {
                    LoginSe_Decide();
                    gYnWork->step = 9;
                    break;
                }
                if (pad & 0x100) {
                    LoginSe_Decide();
                    gYnWork->unk47 = 2;
                    gYnWork->backStep = gYnWork->step;
                    gYnWork->step = 5;
                    gYnWork->msg = 0x19;
                    break;
                }
                if (pad & 0x3000) {
                    if (w->cursor > 0 && (pad & 0x2000)) {
                        LoginSe_Cursor();
                        gYnWork->cursor--;
                        if (gYnWork->cursor < gYnWork->listTop) {
                            gYnWork->listTop = gYnWork->listTop - 1;
                        }
                        Yn_ResetNameScroll();
                        break;
                    }
                    if (w->cursor < w->entryCount - 1 && (pad & 0x1000)) {
                        LoginSe_Cursor();
                        gYnWork->cursor++;
                        if (gYnWork->cursor >= gYnWork->listTop + 4) {
                            gYnWork->listTop++;
                        }
                        Yn_ResetNameScroll();
                        break;
                    }
                }
            }
        }
        if (gPadState[0].pressed & 8) {
            gYnWork->scanResult = -10;
            LoginSe_Play9();
            gYnWork->slot++;
            gYnWork->slot &= 1;
            YnCard_SetSlot(&gYnWork->card, gYnWork->slot);
            return -2;
        }
        Yn_UpdateNameScroll();
        break;
    case 5:
        if (changed != 0) {
            gYnWork->step = gYnWork->backStep;
            gYnWork->msg = 0;
            LoginSe_Cancel();
        } else {
            gYnWork->helpBar = 1;
            ok = 0;
            if (gYnWork->unk47 == 1) {
                if (gPadState[0].pressed & 0x20) {
                    ok = 1;
                }
            } else if ((gPadState[0].pressed & 0x20) || (gPadState[0].pressed & 0x40)) {
                ok = 1;
            }
            if (ok != 0) {
                gYnWork->step = gYnWork->backStep;
                gYnWork->msg = 0;
                LoginSe_Decide();
            }
        }
        break;
    case 6:
        gYnWork->helpBar = 1;
        gYnWork->unk47 = 2;
        if ((gPadState[0].pressed & 0x20) || (gPadState[0].pressed & 0x40)) {
            gYnWork->step = 0;
            gYnWork->msg = 0;
            LoginSe_Decide();
            return -2;
        }
        break;
    case 7:
        gYnWork->helpBar = 1;
        gYnWork->unk47 = 2;
        if ((gPadState[0].pressed & 0x20) || (gPadState[0].pressed & 0x40)) {
            gYnWork->step = 8;
            gYnWork->msg = 0;
            LoginSe_Decide();
        }
        break;
    case 8:
        gYnWork->helpBar = 0;
        if (YnCard_CheckSlots(&gYnWork->card) >= 0) {
            LoginSe_Play9();
            return -2;
        }
        break;
    case 9:
        gYnWork->helpBar = 0;
        name = YnNetcnf_FindUsrName(&gYnWork->netcnf, gYnWork->netcnf.data[gYnWork->cursor].attachIfc);
        Yn_StrCopy(gYnWork->netcnf.name, name);
        p = gYnWork->netcnf.name;
        if (p == NULL || p[0] == 0) {
            gYnWork->step = 6;
            gYnWork->msg = 0x33;
            LoginSe_Play9();
            break;
        }
        if (YnNetcnf_CheckFile(&gYnWork->netcnf) != 0) {
            gYnWork->step = 6;
            gYnWork->msg = 0x1D;
            LoginSe_Play9();
            break;
        }
        idx = gYnWork->cursor;
        if (gYnWork->combos[idx].kind == 1
            && gYnWork->netcnf.data[idx].phoneNumbers[0][0] != 0
            && gYnWork->netcnf.data[idx].ifcType == 2
            && gYnWork->netcnf.data[idx].pppoe == 0xFF) {
            gYnWork->step = 6;
            gYnWork->msg = 0x1B;
            LoginSe_Play9();
            break;
        }
        dev = gYnWork->combos[idx].device;
        if (dev < 0) {
            gYnWork->step = 6;
            if (dev == -1) {
                gYnWork->msg = 5;
            } else {
                gYnWork->msg = 4;
            }
            LoginSe_Play9();
            break;
        }
        if (Yn_SelectDevice(dev) < 0) {
            gYnWork->step = 6;
            gYnWork->msg = 5;
            LoginSe_Play9();
            break;
        }
        if (Yn_CompareProxy((YnEntryKey *)&gYnWork->proxy, (YnEntryKey *)&D_5BF400) < 0) {
            gYnWork->changed = 1;
        }
        if (Yn_FindCombination(&D_5BE9E0, &gYnWork->combos[gYnWork->cursor], 1) < 0) {
            gYnWork->changed = 1;
        }
        Yn_ApplyEntry(gYnWork->combos[gYnWork->cursor].data);
        Yn_ExportSettings(gYnWork->combos[gYnWork->cursor].data);
        return 1;
    }
    return 0;
}

s32 Yn_IsCardGone(void) {
    gYnWork->okSlots = 0;
    if (YnCard_CheckSlots(&gYnWork->card) >= 0) {
        gYnWork->okSlots |= ((gYnWork->card.okSlots | gYnWork->card.slots3) >> 1) & 3;
        if (!(gYnWork->okSlots & (1 << YnCard_GetSlot(&gYnWork->card)))) {
            return 1;
        }
    }
    return 0;
}

s32 Yn_IsCurrentCardGone(void) {
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
void func_00ACBF40(void) {

}

void Yn_DrawScreen(void) {
    switch (gYnWork->screen) {
    case 0:
        break;
    case 1:
        Yn_DrawEntryList();
        break;
    case 23:
        Yn_DrawTextField((u8 *)gYnWork->proxy.http, (char *)gYnFieldHeaders[5], 0x16B, 0xB4, 0x12);
        Yn_DrawTextField((u8 *)gYnWork->proxy.https, (char *)gYnHttpsTexts[0], 0x16B, 0xE6, 0x12);
        Yn_DrawPort(&gYnWork->proxy.httpPort, 0x16B, 0xCD);
        Yn_DrawPort(&gYnWork->proxy.httpsPort, 0x16B, 0xFF);
        Yn_DrawItems();
        break;
    case 22:
        Yn_DrawChoiceItems(gYnWork->proxy.use ^ 1, 5);
        break;
    default:
        Yn_DrawItems();
        break;
    }
}

void Yn_DrawItems(void) {
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

void Yn_DrawChoiceItems(s32 sel, s32 color) {
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


/* "%s": first used here, and shared with Yn_DrawEntryDetails (a named object
 * since round 4, when Yn_DrawEntryList was still assembly). */
const char D_00AD3828[3] = {
#include "yn/00AD3828.inc"
};

void Yn_DrawEntryList(void) {
    char buf[0x100];
    char buf2[0x100];
    char *name;
    s8 idx;
    s32 kind;
    s32 i;
    s32 color;
    s32 y;
    char **lines;
    u8 *p;
    s8 st;

    buf2[0] = 0;
    buf[0] = 0;
    Yn_SetFontSize(0x14, 0x14);
    Yn_SetColor(0);
    Yn_Print(0x2C, 0x54, (char *)(&gYnStorageNames[1])[YnCard_GetSlot(&gYnWork->card)]);
    st = gYnWork->scanResult;
    if (st == -10) {
        return;
    }
    if (st != 0) {
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
                strcpy(buf2, *lines);
                p = (u8 *)strchr(buf2, '%');
                if (p != NULL && p[1] == 's') {
                    strcpy(buf, (char *)gYnStorageNames[YnCard_GetSlot(&gYnWork->card) + 1]);
                    strcat(buf, (char *)p + 2);
                } else {
                    strcpy(buf, buf2);
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
            sprintf(buf, (char *)D_00AD3828, gYnHttpsTexts[1]);
        } else {
            Yn_Utf8ToSjis(buf, name);
            if (gYnWork->nameScroll != 0 && gYnWork->cursor == i) {
                Yn_CopyForPrint(buf2, (u8 *)buf + gYnWork->nameOfs, 0x2C);
                if (gYnWork->nameOfs == 0) {
                    strcpy(buf, buf2);
                } else {
                    strcpy(buf, "..");
                    strcat(buf, buf2);
                }
            } else {
                Yn_CopyForPrint(buf2, (u8 *)buf, 0x2C);
                strcpy(buf, buf2);
            }
        }
        Font_SetPos(0x2C, y);
        Font_Printf("%d: ", i + 1 + gYnWork->listTop);
        Yn_SetColor(color);
        Font_SetPos(0x4D, y);
        Font_Printf(buf);
        y += 0x20;
    }
    if (gYnWork->unk8 >= 2) {
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

void Yn_DrawPort(u16 *value, s16 x, s16 y) {
    s8 digits[8];
    char buf[0x48];
    char *p;
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
        p = buf;
        for (i = 0; i < 18; i++) {
            digits[0] = *p++;
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

void Yn_DrawTextField(u8 *text, char *label, s16 x, s16 y, s32 w) {
    char buf[0x40];

    if (*text == 0) {
        Yn_Print(x, y, label);
        return;
    }
    Yn_CopyForPrint(buf, text, w);
    Yn_Print(x, y, buf);
}

void Yn_DrawMessageWindow(void) {
    switch (gYnWork->msg) {
    case 5:
    case 4:
    case 6:
    case 7:
    case 0x17:
    case 0x19:
    case 0x1B:
    case 0x1D:
    case 0x1E:
    case 0x2B:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x39:
    case 0x1A:
        Window_DrawTextAlpha(&gYnWindows[2], 0, 0xF0);
        break;
    case 1:
    case 0x18:
    case 0x37:
    case 0x38:
        Window_DrawTextAlpha(&gYnWindows[2], 0, 0xF0);
        break;
    case 0:
        break;
    }
}

void Yn_DrawMessage(void) {
    s16 w;
    s16 h;

    Yn_SetGlyphSize(478.5714416503906f);
    switch (gYnWork->msg) {
    case 0x01:
    case 0x05:
    case 0x04:
    case 0x06:
    case 0x07:
    case 0x17:
    case 0x18:
    case 0x38:
    case 0x1A:
    case 0x1B:
    case 0x1E:
    case 0x2B:
    case 0x32:
    case 0x37:
    case 0x39:
        Yn_DrawMessageText();
        break;
    case 0x33:
        Yn_DrawSlotMessage(0);
        break;
    case 0x1D:
        Yn_DrawSlotMessage(0);
        break;
    case 0x34:
        Yn_DrawSlotMessage(0);
        break;
    case 0x19:
        Yn_DrawMessageText();
        w = Font_GetAdvance();
        h = Font_GetCharH();
        Yn_SetFontSize(0x12, 0x12);
        Yn_DrawEntryDetails();
        Yn_SetFontSize(w, h);
        break;
    case 0:
        break;
    }
    Yn_SetGlyphSize(621.4285888671875f);
}

void Yn_DrawMessageText(void) {
    s16 h;
    s8 count;
    YnItem *item;
    s8 i;
    s16 x;
    char *p;

    switch (gYnWork->msg) {
    case 1:
    case 5:
    case 4:
    case 6:
    case 7:
    case 0x1B:
    case 0x39:
        Yn_SetColor(0xB);
        break;
    default:
        Yn_SetColor(0);
        break;
    }
    h = Font_GetAdvance();
    item = gYnMsgItems[gYnWork->msg];
    count = gYnMsgItemCounts[gYnWork->msg];
    for (i = 0; i < count; i++, item++) {
        p = strchr(item->text, '%');
        if (p != NULL && p[1] == 'a') {
            x = Yn_CenterX((char *)gYnYesNoTexts[0], h);
            if (gYnWork->quitChoice == 0) {
                Yn_SetColor(5);
            } else {
                Yn_SetColor(0);
            }
            Yn_Print(x, item->y, (char *)gYnYesNoTexts[1]);
            x += h * 5;
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
                x = Yn_CenterX(item->text, h);
            }
            Yn_Print(x, item->y, item->text);
        }
    }
    Yn_SetColor(0);
}

void Yn_DrawSlotMessage(s32 sel) {
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

void Yn_DrawEntryDetails(void) {
    char buf2[0x200];
    char buf[0x200];
    YnCombo *combo;
    NetcnfData *data;
    s32 y;
    char *name;
    s32 x;
    s32 m;

    y = 0x82;
    combo = &gYnWork->combos[gYnWork->cursor];
    data = combo->data;
    name = YnNetcnf_FindUsrName(&gYnWork->netcnf, gYnWork->netcnf.data[gYnWork->cursor].attachIfc);
    if (name != NULL && combo->kind == 1
        && !(gYnWork->netcnf.data[gYnWork->cursor].phoneNumbers[0][0] != 0
             && gYnWork->netcnf.data[gYnWork->cursor].ifcType == 2
             && gYnWork->netcnf.data[gYnWork->cursor].pppoe == 0xFF)) {
        sprintf(buf2, "        %s  %s/%s", gYnDeviceTexts[0], combo->vendor, combo->product);
        Yn_CopyForPrint(buf, (u8 *)buf2, 0x3C);
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
            m = -1;
            break;
        case 1:
            m = 0;
            break;
        case 2:
            m = 1;
            break;
        case 3:
            m = 2;
            break;
        case 5:
            m = 3;
            break;
        case 6:
            m = 4;
            break;
        }
        if (0 <= m) {
            sprintf(buf2, "  %s  %s", gYnDeviceTexts[1], gYnEtherModeTexts[m]);
            Yn_CopyForPrint(buf, (u8 *)buf2, 0x2C);
            Yn_Print(0x58, y, buf);
            y += 0x19;
        }
        if (data->pppoe == 1) {
            Yn_CopyForPrint(buf2, (u8 *)data->authName, 0x1C);
            sceNetcnfifConvAuthname(buf2, buf, 0x100);
            sprintf(buf2, "    %s  %s", gYnUserIdTexts[0], buf);
            Yn_Print(0x58, y, buf2);
            y += 0x19;
        } else if (data->dhcp == 1) {
            if (data->dhcpHostname[0] == 0) {
                sprintf(buf2, (char *)D_00AD3828, gYnUserIdTexts[1]);
            } else {
                Yn_CopyForPrint(buf2, (u8 *)data->dhcpHostname, 0x1C);
            }
            sprintf(buf, "%s  %s", gYnHostIpTexts[0], buf2);
            Yn_Print(0x46, y, buf);
            sprintf(buf2, (char *)D_00AD3828, gYnUserIdTexts[1]);
            sprintf(buf, "    %s  %s", gYnHostIpTexts[1], buf2);
            Yn_Print(0x58, y + 0x19, buf);
            y += 0x32;
        } else if (data->address[0] != 0) {
            Yn_AlignAddress((u8 *)buf, (u8 *)data->address);
            sprintf(buf2, "    %s  %s", gYnHostIpTexts[1], buf);
            Yn_Print(0x58, y, buf2);
            Yn_AlignAddress((u8 *)buf, (u8 *)data->netmask);
            sprintf(buf2, "    %s  %s", gYnNetmaskTexts[0], buf);
            Yn_Print(0x58, y + 0x19, buf2);
            Yn_AlignAddress((u8 *)buf, (u8 *)data->gateway);
            sprintf(buf2, "%s  %s", gYnNetmaskTexts[1], buf);
            Yn_Print(0x58, y + 0x32, buf2);
            y += 0x4B;
        }
        if (data->dns1Address[0] == 0) {
            if (data->pppoe != 1 && data->dhcp != 1) {
                sprintf(buf2, (char *)D_00AD3828, gYnHttpsTexts[1]);
            } else {
                sprintf(buf2, (char *)D_00AD3828, gYnUserIdTexts[1]);
            }
        } else {
            Yn_AlignAddress((u8 *)buf2, (u8 *)data->dns1Address);
        }
        sprintf(buf, "%s  %s", gYnDnsTexts[0], buf2);
        Yn_Print(0x58, y, buf);
        if (data->dns2Address[0] == 0) {
            if (data->pppoe != 1 && data->dhcp != 1) {
                sprintf(buf2, (char *)D_00AD3828, gYnHttpsTexts[1]);
            } else {
                sprintf(buf2, (char *)D_00AD3828, gYnUserIdTexts[1]);
            }
        } else {
            Yn_AlignAddress((u8 *)buf2, (u8 *)data->dns2Address);
        }
        sprintf(buf, "%s  %s", gYnDnsTexts[1], buf2);
        Yn_Print(0x58, y + 0x19, buf);
    }
}

void Yn_AlignAddress(u8 *dst, u8 *src) {
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

void Yn_DrawHelp(void) {
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

void Yn_Print(s16 x, s16 y, char *text) {
    Font_SetPos(x, y);
    Text_Printf(text);
}

void Yn_SetColor(s8 color) {
    Font_SetColorIndex(color);
}

void Yn_SetFontSize(s32 w, s32 h) {
    Font_SetSpacing(w, h);
}

/* Sets the text object's glyph size (sub_main Font_SetSize: 0..1000 picks one of 7 glyph tables; 621.4 is table 4). */
void Yn_SetGlyphSize(f32 scale) {
    Font_SetSize(scale);
}

void Yn_SetTextMode(s8 mode) {
    Font_SetHalfWidth(mode);
}

/* The game's getter of the text mode always returns 1; Yn_Draw saves it, sets 1 and restores it. */
s32 Yn_GetTextMode(void) {
    return 1;
}

s16 Yn_CenterX(char *text, s32 w) {
    s32 len;

    len = Str_Length(text);
    return (s32)((640 - len * (w / 2)) / 2.0f);
}

s16 Yn_CenterXSjis(char *str, s32 w) {
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

void Yn_CopyForPrint(char *dst, u8 *src, s32 max) {
    s32 cut;
    s32 n;
    u8 c;

    if (src != NULL) {
        cut = 0;
        if ((u32)strlen((char *)src) > max) {
            cut = 1;
        }
        n = 0;
        c = *src;
        while (c != 0) {
            if (c >= 0x80) {
                dst[0] = c;
                n += 2;
                dst[1] = src[1];
                src += 2;
                dst += 2;
            } else {
                if (c == '%') {
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
            c = *src;
        }
    }
    *dst = 0;
}

void Yn_StrCopy(char *dst, char *src) {
    if (src == NULL) {
        *dst = 0;
        return;
    }
    strcpy(dst, src);
}

void Yn_BeginNumberEdit(u16 *value) {
    gYnWork->numTarget = value;
    gYnWork->numBackup = *value;
    gYnWork->numStep = 1;
    gYnWork->numDigitPos = 0;
}

void Yn_UpdatePadRepeat(PadState *pad) {
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

void Yn_OpenKeyboard(s32 mode, char *str) {
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

s32 Yn_PollKeyboard(void) {
    if (NetKbd_Run() != 0) {
        NetKbd_SetActive(0);
        strcpy(gYnWork->editBuf, NetKbd_GetText());
        strcpy(gYnWork->editTarget, gYnWork->editBuf);
        return 1;
    }
    return 0;
}

/* Empty in the game; Yn_Update and Yn_UpdateProxy call it first. */
void func_00ACDF60(void) {

}

void Yn_DrawTextLayer(void) {
    Gs_SetState(0x61, 0x01000000);
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(0x5F, 5);
    Gs_SetState(0x60, 0);
    Gs_SetState(0x6C, 0);
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
    Gs_SetState(0x6C, 1);
}

extern s16 D_0059DCF0;
extern s32 D_0059DCF8;
extern s8 gProxyMode;
extern u8 D_0059D8C0;
extern u8 gNetPhyMode;
extern u8 D_0059DCE0;
extern u8 D_0059DE00;
void Yn_ExportSettings(NetcnfData *d) {
    YnProxy *proxy;

    D_0059DE00 = 0;
    proxy = &gYnWork->proxy;
    memset(D_59DD00, 0, 0x100);
    D_0059DCF8 = 0;
    D_0059DCF0 = 0;
    memset(gNetDomain, 0, 0x100);
    memset(gNetAuthName, 0, 0x100);
    memset(gNetAuthKey, 0, 0x100);
    gNetIpAddr = 0;
    gNetNetmask = 0;
    gNetGateway = 0;
    gNetDns1 = 0;
    gNetDns2 = 0;
    memset(D_59DBF0, 0, 0xF0);
    memset(D_59DBE0, 0, 0x10);
    D_0059DCE0 = 0;
    D_0059D8C0 = 3;
    if (d->dhcp == 1) {
        gNetLanMode = 1;
        strncpy(gNetDomain, d->dhcpHostname, 0x100);
    } else if (d->pppoe == 1) {
        gNetLanMode = 2;
        strncpy(gNetAuthName, d->authName, 0x100);
        strncpy(gNetAuthKey, d->authKey, 0x100);
    } else {
        gNetLanMode = 0;
        gNetIpAddr = net_inet_addr2(d->address);
        gNetNetmask = net_inet_addr2(d->netmask);
        gNetGateway = net_inet_addr2(d->gateway);
    }
    if (d->dns1Address[0] != 0) {
        gNetDns1 = net_inet_addr2(d->dns1Address);
    }
    if (d->dns2Address[0] != 0) {
        gNetDns2 = net_inet_addr2(d->dns2Address);
    }
    switch (d->phyConfig) {
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
    memset(gHttpProxy, 0, 0x110);
    memset(gHttpsProxy, 0, 0x110);
    if (proxy->use == 1) {
        gProxyMode = 1;
        sprintf(gHttpProxy, "http://%s:%d", proxy->http, proxy->httpPort);
        if ((u8)proxy->https[0] != 0) {
            gProxyMode |= 2;
            sprintf(gHttpsProxy, "https://%s:%d", proxy->https, proxy->httpsPort);
        }
    }
}

void Yn_ApplyEntry(NetcnfData *data) {
    char name[4];

    name[0] = 0;
    Yn_IfcToConfig(&D_5BE9E0, data, name);
    Yn_DevToConfig(&D_5BE9E0, data);
    D_5BF400 = gYnWork->proxy;
}

void Yn_LoadProxy(void) {
    gYnWork->proxy = D_5BE6C4;
}

void Yn_StoreProxy(void) {
    D_5BE6C4 = gYnWork->proxy;
}

s16 gYnListBarYs[4] = {
#include "yn/00AD2128.inc"
};
PrimLine gYnBackgroundRect = {
#include "yn/00AD2130.inc"
};
void Yn_DrawBackground(void) {
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
extern s32 D_002189B8;
extern s32 gDmaQueue;
void Yn_LaunchSetupGuide(void) {
    strcpy((char *)gYnSetupGuideArg, "cdrom0:\\SLPM_662.80;1");
    gSysExiting = 1;
    Dma_WaitIdle();
    sceGsSyncPath(0, 0);
    Pad_End();
    DIntr();
    DisableDmac(gDmaQueue);
    RemoveDmacHandler(gDmaQueue, D_002189B8);
    DisableIntc(0);
    RemoveIntcHandler(0, gDmaGsIntcId);
    EIntr();
    sceGsSyncVCallback(0);
    sceCdInit(5);
    sceSifExitCmd();
    func_11DD10("cdrom0:\\NTGUIDVD\\NTGDVDJ.BIN;1", 3, (char **)gYnGuideArgv);
}

void Yn_DrawChoiceCursor(void) {
    s16 h;
    s16 w;
    s16 x;

    h = Font_GetAdvance();
    w = Yn_CenterX((char *)gYnYesNoTexts[0], h);
    if (gYnWork->quitChoice == 0) {
        x = w + h;
    } else {
        x = w + h * 5 + (s16)(1.5f * h);
    }
    Menu_DrawGlowBar(x, 0x12A, h * 6, h + 4, 0xFF20C0C0);
}

void Yn_DrawIcon(f32 scale, s32 id, s32 x, s32 y, s32 blink) {
    f32 a;
    u32 color;
    f32 s;

    s = Math_Sin(9.58738e-05f * (u16)((gFrameTick & 0x3F) << 10));
    a = 160.0f + 80.0f * s;
    if (blink == 0) {
        color = ((u32)a << 24) | 0xF08200;
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
void Yn_DrawProxy(void) {
    YnWindow win;
    s16 *py;
    s8 mode;
    s32 x;
    s32 state;

    win = *(YnWindow *)((u8 *)gYnProxyWindows + 0);
    Window_DrawTextAlpha(&win, 0, 0x80);
    win = gYnProxyWindows[1];
    Window_DrawTextAlpha(&win, 0, 0x80);
    switch (state = gYnWork->state) {
    case 1:
        Menu_DrawGlowBar(0x140, gYnProxyChoiceYs[gYnWork->proxyCursor], 0xF0, 0x18, 0xFF20C0C0);
        Yn_DrawIcon(2.0f, 0x10, 0x230, 0xD0, 0);
        break;
    case 2:
    case 3:
    case 4:
        mode = gYnWork->menuCursor;
        if ((mode == 1 || mode == 3) && state == 4) {
            win = gYnProxyWindows[2];
            py = &gYnProxyBarYs[mode];
            win.win.y = *py + 3;
            Window_DrawTextAlpha(&win, 0, 0x80);
            Menu_DrawGlowBar(0x204, *py, 0x46, 0x18, 0xFF20C0C0);
            x = 0x214 - gYnWork->numDigitPos * 10;
            state = win.win.y - 11;
            Yn_DrawIcon(0.5f, 0x18, x, state, 1);
            Yn_DrawIcon(0.5f, 0x17, x, state + 0x1D, 1);
        } else {
            Menu_DrawGlowBar(0x140, gYnProxyBarYs[mode], 0x212, 0x18, 0xFF20C0C0);
        }
        switch (mode) {
        case 0:
            gYnWork->help = 0x20;
            break;
        case 1:
        case 3:
            gYnWork->help = 0x21;
            break;
        case 2:
            gYnWork->help = 0x22;
            break;
        }
        if (gYnWork->state == 2) {
            Yn_DrawIcon(2.0f, 0x16, 0x1E, 0xD0, 0);
            if ((u8)gYnWork->proxy.http[0] != 0 && gYnWork->proxy.httpPort != 0) {
                Yn_DrawIcon(2.0f, 0x10, 0x230, 0xD0, 0);
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

s32 Yn_UpdateProxy(void) {
    u32 v;
    u32 digit;
    s32 up;
    s32 down;
    u16 pad;
    u16 div;
    s32 pad2;
    u32 rest;

    if (gYnWork == NULL) {
        return -1;
    }
    func_00ACDF60();
    switch (gYnWork->state) {
    case 0:
        Yn_LoadProxy();
        gYnWork->state = 1;
        gYnWork->unk46 = 0x10;
        gYnWork->unk47 = 2;
        gYnWork->unk48 = 0x13;
        gYnWork->screen = 0x16;
        gYnWork->help = 0x1F;
        gYnWork->proxyCursor = 1;
        gYnWork->menuCursor = 0;
        gYnWork->helpBar = 0;
        break;
    case 1:
        if (gYnWork->proxy.use == 0) {
            gYnWork->helpBar = 5;
        } else {
            gYnWork->helpBar = 2;
        }
        pad = gPadState[0].pressed;
        if (pad & 0x40) {
            LoginSe_Cancel();
            return -1;
        }
        if (pad & 0x20) {
            gYnWork->proxy.use = 1 - gYnWork->proxyCursor;
            LoginSe_Decide();
        } else if (pad & 0x3000) {
            gYnWork->proxyCursor ^= 1;
            LoginSe_Cursor();
        } else if (pad & 0x400) {
            LoginSe_Cursor();
            if (gYnWork->proxy.use != 0) {
                gYnWork->state = 2;
                gYnWork->screen = 0x17;
                gYnWork->help = 0x20;
            } else {
                gYnWork->state = 5;
                gYnWork->msg = 0x18;
                gYnWork->quitChoice = 1;
            }
        }
        break;
    case 2:
        if ((u8)gYnWork->proxy.http[0] != 0 && gYnWork->proxy.httpPort != 0) {
            gYnWork->helpBar = 6;
        } else {
            gYnWork->helpBar = 4;
        }
        pad = gPadState[0].pressed;
        if (pad & 0x40) {
            LoginSe_Cancel();
            return -1;
        }
        if (pad & 0x800) {
            gYnWork->state = 1;
            gYnWork->screen = 0x16;
            gYnWork->help = 0x1F;
            LoginSe_Cursor();
        } else if (pad & 0x2000) {
            gYnWork->menuCursor--;
            gYnWork->menuCursor = (gYnWork->menuCursor + 4) % 4;
            LoginSe_Cursor();
        } else if (pad & 0x1000) {
            gYnWork->menuCursor++;
            gYnWork->menuCursor = (gYnWork->menuCursor + 4) % 4;
            LoginSe_Cursor();
        } else if (pad & 0x20) {
            LoginSe_Decide();
            switch (gYnWork->menuCursor) {
            case 0:
                gYnWork->state = 3;
                Yn_OpenKeyboard(1, gYnWork->proxy.http);
                break;
            case 1:
                gYnWork->state = 4;
                Yn_BeginNumberEdit(&gYnWork->proxy.httpPort);
                break;
            case 2:
                gYnWork->state = 3;
                Yn_OpenKeyboard(1, gYnWork->proxy.https);
                break;
            case 3:
                gYnWork->state = 4;
                Yn_BeginNumberEdit(&gYnWork->proxy.httpsPort);
                break;
            }
        } else if ((pad & 0x400) && (u8)gYnWork->proxy.http[0] != 0 && gYnWork->proxy.httpPort != 0) {
            LoginSe_Cursor();
            gYnWork->state = 5;
            gYnWork->msg = 0x18;
            gYnWork->quitChoice = 1;
        }
        break;
    case 3:
        gYnWork->helpBar = 0;
        if (Yn_PollKeyboard() != 0) {
            gYnWork->state = 2;
        }
        break;
    case 4:
        gYnWork->helpBar = 0;
        gYnWork->numEditing = 2;
        pad = gPadState[0].pressed;
        pad2 = pad | gPadState[0].repeat;
        if (pad & 0x40) {
            gYnWork->state = 2;
            *gYnWork->numTarget = gYnWork->numBackup;
            gYnWork->numEditing = 0;
            gYnWork->numDigitPos = 0;
            LoginSe_Cancel();
            break;
        }
        if (pad & 0x20) {
            gYnWork->state = 2;
            gYnWork->numEditing = 0;
            gYnWork->numDigitPos = 0;
            LoginSe_Decide();
            break;
        }
        if (pad2 & 0xC00) {
            if ((pad2 & 0x800) && gYnWork->numStep != 10000) {
                gYnWork->numStep *= 10;
                gYnWork->numDigitPos++;
                LoginSe_Cursor();
                break;
            }
            if ((pad2 & 0x400) && gYnWork->numStep != 1) {
                gYnWork->numStep /= 10;
                gYnWork->numDigitPos--;
                LoginSe_Cursor();
                break;
            }
        }
        if (pad2 & 0x3000) {
            up = pad2 & 0x1000;
            down = pad2 & 0x2000;
            div = gYnWork->numStep;
            digit = (*gYnWork->numTarget / div) % 10;
            rest = *gYnWork->numTarget - div * digit;
            do {
                if (up) {
                    digit = (digit + 9) % 10;
                }
                if (down) {
                    digit = (digit + 11) % 10;
                }
                v = rest + digit * div;
            } while (v >= 0x10000);
            *gYnWork->numTarget = v;
            LoginSe_Cursor();
        }
        break;
    case 5:
        gYnWork->helpBar = 1;
        gYnWork->quitAsked = 1;
        pad = gPadState[0].pressed;
        if (pad & 0x40) {
            gYnWork->quitAsked = 0;
            LoginSe_Cancel();
        back:
            gYnWork->msg = 0;
            if (gYnWork->proxy.use != 0) {
                gYnWork->state = 2;
            } else {
                gYnWork->state = 1;
            }
        } else if (pad & 0x20) {
            gYnWork->quitAsked = 0;
            LoginSe_Decide();
            if (gYnWork->quitChoice != 0) {
                goto back;
            }
            gYnWork->state = 6;
            gYnWork->msg = 0x32;
            Yn_StoreProxy();
        } else if (pad & 0xC00) {
            gYnWork->quitChoice ^= 1;
            LoginSe_Cursor();
        }
        break;
    case 6:
        gYnWork->helpBar = 1;
        if (gPadState[0].pressed & 0x20) {
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
 * The name table in a local (s7 across the loop), the height as an s32
 * holding an s16 and passed back as `(s16)h` (spilled with sw, reloaded
 * with lh) and the declaration order give the game's registers. */
void Yn_DrawHelpBar(s32 type) {
    char **names;
    s16 w;
    s32 h;
    YnHelpBar bar;
    s16 count;
    char *text;
    s32 x;
    s32 i;
    s8 *hint;
    s32 len;

    w = Font_GetAdvance();
    h = (s16)Font_GetCharH();
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
        if (type == 7) {
            x += len * 9 + 0x1A;
        } else {
            x += len * 10 + 0x36;
        }
    }
    Yn_SetFontSize(w, (s16)h);
}

u32 gYnNetcnfArgs[32] = {
#include "yn/00AD21E0.inc"
};
u32 gYnNetcnfifArgs[8] = {
#include "yn/00AD2260.inc"
};
s32 YnNetcnf_LoadModules(YnNetcnf *n) {
    Yn_LoadModule("cdrom0:\\IOP\\MODULES\\NETICON\\NETCNF.IRX;1", 0x7F, (char *)gYnNetcnfArgs, 0, 0);
    Yn_LoadModule("cdrom0:\\IOP\\MODULES\\NETICON\\NETCNFIF.IRX;1", 0x16, (char *)gYnNetcnfifArgs, 0, 0);
    func_00AC8128();
    while (yn_entry() < 0) {
    }
    return 0;
}

s32 YnNetcnf_Open(YnNetcnf *n) {
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

s32 YnNetcnf_Close(YnNetcnf *n) {
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

char *YnNetcnf_FindUsrName(YnNetcnf *n, char *name) {
    s32 i;

    for (i = 0; i < n->count[1]; i++) {
        if (strcmp(n->list1[i].sysName, name) == 0) {
            return n->list1[i].usrName;
        }
    }
    return NULL;
}

void Yn_ConfigToIfc(YnConfig *cfg, NetcnfData *data, char *name) {
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

void Yn_IfcToConfig(YnConfig *cfg, NetcnfData *data, char *name) {
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

void Yn_ConfigToDev(YnConfig *cfg, NetcnfData *data) {
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

void Yn_DevToConfig(YnConfig *cfg, NetcnfData *data) {
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

void Yn_BuildCombos(YnNetcnf *n, YnCombo *combo) {
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

s32 Yn_FindCombination(YnConfig *cfg, YnCombo *combo, s32 n) {
    u32 ip;
    char name[0x200];
    struct {
        NetcnfData d;
        u8 unk1340[0x30];
    } ifcx;
    NetcnfData dev;
    NetcnfData *d;
    s32 i;

    name[0] = 0;
    Yn_ConfigToIfc(cfg, &ifcx.d, name);
    Yn_ConfigToDev(cfg, &dev);
    for (i = 0; i < n; i++, combo++) {
        d = combo->data;
        if (d != NULL && strcmp(d->vendor, dev.vendor) == 0 && strcmp(d->product, dev.product) == 0
            && d->dhcp == ifcx.d.dhcp && strcmp(d->dhcpHostname, ifcx.d.dhcpHostname) == 0
            && d->pppoe == ifcx.d.pppoe && strcmp(d->authName, ifcx.d.authName) == 0
            && strcmp(d->authKey, ifcx.d.authKey) == 0) {
            ip = 0;
            Yn_ParseAddress((u8 *)d->address, (u8 *)&ip);
            if (ip == *(u32 *)cfg->address) {
                ip = 0;
                Yn_ParseAddress((u8 *)d->netmask, (u8 *)&ip);
                if (ip == *(u32 *)cfg->netmask) {
                    ip = 0;
                    Yn_ParseAddress((u8 *)d->gateway, (u8 *)&ip);
                    if (ip == *(u32 *)cfg->gateway) {
                        ip = 0;
                        Yn_ParseAddress((u8 *)d->dns1Address, (u8 *)&ip);
                        if (ip == *(u32 *)cfg->dns1) {
                            ip = 0;
                            Yn_ParseAddress((u8 *)d->dns2Address, (u8 *)&ip);
                            if (ip == *(u32 *)cfg->dns2) {
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

s32 Yn_CompareProxy(YnEntryKey *a, YnEntryKey *b) {
    if (a->use != b->use) {
        return -1;
    }
    if (strcmp(a->http, b->http) != 0) {
        return -1;
    }
    if (a->httpPort != b->httpPort) {
        return -1;
    }
    if (strcmp(a->https, b->https) != 0) {
        return -1;
    }
    if (a->httpsPort != b->httpsPort) {
        return -1;
    }
    return 0;
}

s32 Yn_IsAddressSet(u8 *ip) {
    s32 i;

    for (i = 0; i < 4; i++, ip++) {
        if (!*ip == 0) {
            return 1;
        }
    }
    return 0;
}

void YnNetcnf_SetType(YnNetcnf *n, s32 type) {
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

s32 YnNetcnf_GetCount(YnNetcnf *n) {
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

s32 YnNetcnf_GetList(YnNetcnf *n) {
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

void YnNetcnf_LoadCombos(YnNetcnf *n) {
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

s32 YnNetcnf_IsEmpty(YnNetcnf *n) {
    n->savedType = n->type;
    YnNetcnf_SetType(n, 0);
    return YnNetcnf_GetCount(n) <= 0;
}

s32 YnNetcnf_CheckFile(YnNetcnf *n) {
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

s32 YnNetcnf_LoadFirst(YnNetcnf *n) {
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

void YnNetcnf_SetPath(YnNetcnf *n, s32 slot) {
    sprintf(n->path, "mc%d:%s", slot, "/BWNETCNF/BWNETDCNF");
}

void Yn_ParseAddress(u8 *str, u8 *ip) {
    u8 v;

    v = 0;
    for (; *str != 0; str++) {
        if (*str == '.') {
            *ip++ = v;
            v = 0;
        } else {
            v *= 10;
            v += *str - '0';
        }
    }
    *ip = v;
}

void Yn_FormatAddress(char *str, u8 *ip) {
    sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void Yn_ResetNetDevices(void) {
    if (gNetDevice != 0) {
        Net_StopDevice();
    }
    NetDev_ClearList();
    NetDev_Refresh();
}

s32 Yn_FindDevice(char *vendor, char *product) {
    s32 i;
    NetDevice *d;

    for (i = 0, d = gNetDevs; i < 35; i++, d++) {
        if (d->type != -1 && strcmp(d->vendor, vendor) == 0 && strcmp(d->product, product) == 0) {
            return i;
        }
    }
    return (NetDev_IsKnownName(vendor, product) != 0) ? -1 : -2;
}

s32 Yn_GetDeviceKind(char *vendor, char *product) {
    s32 i;
    NetDevice *d;

    for (i = 0, d = gNetDevs; i < 35; i++, d++) {
        if (d->type != -1 && strcmp(d->vendor, vendor) == 0 && strcmp(d->product, product) == 0) {
            if (gNetDevs[i].type == 1) {
                return 1;
            }
            return 2;
        }
    }
    return 0;
}

s32 Yn_SelectDevice(s32 i) {
    NetDevice *d;

    d = &gNetDevs[i];
    if (gNetDevice != NULL) {
        return -(gNetDevice != d);
    }
    if (d->type != 1) {
        return -2;
    }
    gNetDevice = d;
    Net_LoadDrivers();
    Net_LoadDriverType1();
    return 1;
}

void Yn_Utf8ToSjis(char *dstp, char *srcp) {
    u8 *dst = (u8 *)dstp;
    u8 *src = (u8 *)srcp;
    u16 c;

    c = *src;
    while (c != 0) {
        if (c >= 0x80) {
            c = (src[1] << 8) + *(src += 2);
            src++;
            if (c >= 0x8181 && c < 0x81C0) {
                c += 0x11E;
            } else if (c >= 0x8280 && c < 0x8294) {
                c += 0x5E;
            } else if (c >= 0x82A1 && c < 0x82C0) {
                c += 0x9F;
            } else if (c >= 0x8380 && c < 0x83A0) {
                c -= 0x21;
            } else if (c >= 0x83A0 && c < 0x83B7) {
                c -= 0x20;
            } else {
                c = 0x815B;
            }
            dst[0] = (c >> 8) & 0xFF;
            dst[1] = c & 0xFF;
            dst += 2;
        } else {
            *dst = c;
            src++;
            dst++;
        }
        c = *src;
    }
    *dst = 0;
}

void Yn_SjisToUtf8(char *dstp, char *srcp) {
    u8 *dst = (u8 *)dstp;
    u8 *src = (u8 *)srcp;
    s32 n;
    u16 c;

    n = 0;
    c = *src;
    while (c != 0) {
        if (c >= 0x80) {
            if (n >= 0xFD) {
                break;
            }
            c = ((c & 0xFF) << 8) + src[1];
            src += 2;
            if (c >= 0x829F && c < 0x82DE) {
                c -= 0x11E;
            } else if (c >= 0x82DE && c < 0x82F2) {
                c -= 0x5E;
            } else if (c >= 0x8340 && c < 0x835F) {
                c -= 0x9F;
            } else if (c >= 0x835F && c < 0x837F) {
                c += 0x21;
            } else if (c >= 0x8380 && c < 0x8397) {
                c += 0x20;
            } else {
                c = 0x83BC;
            }
            dst[0] = 0xE3;
            dst[1] = (c >> 8) & 0xFF;
            n += 3;
            *(dst += 2) = c & 0xFF;
            dst++;
        } else {
            if (n >= 0xFF) {
                break;
            }
            *dst = c;
            src++;
            dst++;
            n++;
        }
        c = *src;
    }
    *dst = 0;
}

void Yn_UnloadModule(s32 id) {
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

void Yn_LoadModule(char *path, s32 argc, char *argv, void *buf, s32 tries) {
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

