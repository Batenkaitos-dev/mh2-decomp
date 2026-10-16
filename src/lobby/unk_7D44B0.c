/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "syssettings.h"
#include "net_mode.h"
#include "em.h"
#include "m2c_macros.h"
#include "sprite.h"
#include "lobby_vobj.h"
#include "stageobj.h"
#include "scene.h"
#include "ram_areas.h"
#include "obj.h"

/* a timed event (0x18 bytes; LobbyWorld.events, D_008FAD30):
 * LobbyWorld_RunEvents calls func once the world's time reaches the event's */
typedef struct LobbyWorldEvent {
    /* 0x00 */ s32 active;
    /* 0x04 */ s32 year;
    /* 0x08 */ s32 season;
    /* 0x0C */ s32 day;
    /* 0x10 */ u32 tick;            /* added after the days are scaled */
    /* 0x14 */ void (*func)(void);
} LobbyWorldEvent;

/*
 * The lobby world's state (gLobbyWorld, 0x200 bytes; the events at +0x130
 * have the label D_008FAD30 too): its clock and calendar (a day of `dayFrames` frames, three seasons
 * of seasonDays[] days; LobbyWorld_Tick advances it by the frames counted at
 * 0x6AE398, LobbyWorld_AddDays by a fraction of a day, LobbyWorld_NextDay starts the
 * next day and, past a season's last day, the next season), the background
 * music (Stream_* channel 0: the track, its position table and a file per
 * entry), three colour fades keyed by time (LobbyWorld_NextFadeKey) and four timed
 * events. The assembly reaches many fields by labels of their own
 * (D_008FACxx, D_008FADxx).
 */
typedef struct LobbyWorld {
    /* 0x000 */ u8 unk0[4];
    /* 0x004 */ u32 lastVsync;      /* 0x6AE398 when last counted (LobbyWorld_Tick) */
    /* 0x008 */ u32 frames;         /* frames counted */
    /* 0x00C */ f32 dayTicks;       /* LobbyWorld_GetTime's units per day */
    /* 0x010 */ f32 dayFrames;      /* frames per day */
    /* 0x014 */ f32 unk14;          /* the length of a second cycle (unk34, unk4C) */
    /* 0x018 */ f32 unk18;
    /* 0x01C */ s32 seasonDays[3];  /* the three seasons' lengths in days */
    /* 0x028 */ u32 unk28;
    /* 0x02C */ s32 unk2C;
    /* 0x030 */ s32 unk30;
    /* 0x034 */ f32 unk34;          /* the second cycle's total */
    /* 0x038 */ f32 seconds;        /* seconds since the start */
    /* 0x03C */ u8 pad3C[4];
    /* 0x040 */ s32 days;           /* days since the start */
    /* 0x044 */ u8 pad44[4];
    /* 0x048 */ s32 unk48;          /* counted with year */
    /* 0x04C */ f32 unk4C;          /* the second cycle's position */
    /* 0x050 */ f32 daySeconds;     /* the time of day in seconds (0-86400) */
    /* 0x054 */ f32 dayFrac;        /* daySeconds / 86400 */
    /* 0x058 */ s32 hour;
    /* 0x05C */ s32 day;            /* in the season */
    /* 0x060 */ s32 season;         /* 0-2 */
    /* 0x064 */ s32 year;
    /* 0x068 */ u8 pad68[8];
    /* 0x070 */ s32 pm;             /* 0 from hour 0, 1 from hour 12 */
    /* 0x074 */ s32 hold;           /* 1: the next count is skipped */
    /* 0x078 */ u8 active;          /* the music plays */
    /* 0x079 */ u8 unk79;
    /* 0x07A */ u8 cur;             /* the entry of the table selected (LobbyBgm_SelectEntry) */
    /* 0x07B */ u8 track;           /* LobbyBgm_Play's id */
    /* 0x07C */ u32 unk7C;
    /* 0x080 */ u32 pos;            /* the selected entry's position */
    /* 0x084 */ u32 unk84;          /* LobbyBgm_Play's position */
    /* 0x088 */ s32 unk88;
    /* 0x08C */ s32 unk8C;
    /* 0x090 */ s32 n;              /* the table's entries */
    /* 0x094 */ u32 unk94;
    /* 0x098 */ s32 unk98;
    /* 0x09C */ u16 *files;         /* per entry, the stream file (LobbyBgm_Restart) */
    /* 0x0A0 */ u32 *table;         /* the entries' positions (LobbyBgm_FindEntry) */
    /* 0x0A4 */ f32 fade[3][4];     /* three colours (LobbyWorld_NextFadeKey) */
    /* 0x0D4 */ f32 fadeStep[3][4];
    /* 0x104 */ s16 fadeTime[3];
    /* 0x10A */ s16 unk10A;         /* the time the fades' keys are compared with */
    /* 0x10C */ struct LobbyFadeKey ***fadeTbl; /* the track's key tables (gLobbyFadeTables) */
    /* 0x110 */ u16 unk110;
    /* 0x112 */ s8 fadeOn[3];
    /* 0x115 */ s8 fadeSet;
    /* 0x116 */ s8 unk116;
    /* 0x117 */ s8 unk117;
    /* 0x118 */ s8 unk118;
    /* 0x119 */ u8 unk119;
    /* 0x11A */ s8 unk11A;
    /* 0x11B */ u8 unk11B;
    /* 0x11C */ u8 unk11C;          /* 1 started, 3 playing, 4 stopped */
    /* 0x11D */ u8 unk11D;          /* the entry playing (LobbyBgm_GetEntry) */
    /* 0x11E */ s8 unk11E;
    /* 0x11F */ u8 unk11F;
    /* 0x120 */ s8 unk120;
    /* 0x121 */ u8 unk121;
    /* 0x122 */ u8 pad122[2];
    /* 0x124 */ Em *actor;           /* the lobby NPC the music's motion scripts move (Npc_Init: LobbyBgm_SetActor; LobbyActor_StartScript, LobbyActor_Update, LobbyActor_Draw) */
    /* 0x128 */ s8 movieStep;       /* LobbyMovie_Run: 0 start, 1 playing, 2 stop, 3 done */
    /* 0x129 */ s8 movie;           /* Movie_Play's id less 2 (LobbyMovie_Prepare) */
    /* 0x12A */ s16 movieLen;       /* in frames (gMovieLengths[movie]) */
    /* 0x12C */ s32 movieFrames;
    /* 0x130 */ LobbyWorldEvent events[4];
    /* 0x190 */ u8 unk190[0x70];
} LobbyWorld;

extern LobbyWorld gLobbyWorld;


/* a key of a fade (12 bytes): from its frame on, the colour goes from
 * `from` to `to` in `time` frames; a frame of -1 ends the list */
typedef struct LobbyFadeKey {
    /* 0x0 */ s16 frame;
    /* 0x2 */ s16 time;
    /* 0x4 */ u8 from[4];
    /* 0x8 */ u8 to[4];
} LobbyFadeKey;

/* the music tracks: per track its entries' positions (gLobbyBgmMarks), their
 * count and the track's length in frames */
typedef struct LobbyBgmInfo {
    /* 0x0 */ s32 n;
    /* 0x4 */ u32 len;
} LobbyBgmInfo;

extern u32 *gLobbyBgmMarks[];
extern LobbyBgmInfo gLobbyBgmInfo[];

/* prototypes */
void EventFlag_Set(s32 n);
extern s16 gMovieLengths[];
extern LobbyFadeKey ***gLobbyFadeTables[];
extern s8 D_008FAD29;
extern s16 D_008FAD2A;
extern s32 D_008FAD2C;
s32 LobbyKddi_PortalStep(void);
s32 NetScreen_ConnectStep(void);
s32 NetScreen_Run(void);
void LobbyDialog_ShowConnMsg(s32 arg0);
void NetScene_ToState6(void);
void NetMode_Init(void);
s32 LoginPad_Decide(s32 sound);
void NetMode_CallDrawScreen(s32 a0);
s32 NetMode_SeqAStart(s32 arg0, s32 arg1, s64 arg2);
void NetMode_OpenFrame(void);
void NetMode_OpenIcon(void);
void NetModeMsg_QueueKind3(u8 id);
void Loading_Stop(void);
extern u8 gLoginState;
extern s8 gLoginConnectStep;
extern s16 gNetStepTimeout;
extern s8 gNetEntryMode;
void LobbyCard_ResetNetSave(void);
extern s32 D_008FAC90;
extern void *D_008FACA0;
void NetMode_ResetToFrame(void);
extern s8 gNetErrorCode;
extern s32 gTownDayTicks;
extern u32 gRandGaussSum[];         /* sub_main 0x6AE390 */
/* the vsync counter at 0x6AE398 (display.c increments it) has no symbol:
 * the original addresses it as a named object, so it is reached through
 * its neighbour */
#define gSubVSyncCount (gRandGaussSum[2])
extern u32 gTownDay;
extern u32 gTownSeason;
extern u32 gLobbyCycle2Pos;
extern u32 gLobbyTime;
s32 Game_IsOnline(void);
extern s32 gTownSeasonDays[3];              /* the seasons lengths in days (the server mode) */
extern s32 D_008BF0A0;                 /* the year (the server mode) */
void LobbyWorld_ClearEvents(void);
void LobbyWorld_OnNextDay(void);
void VillageClock_Wobble(void);
void LobbyWorld_OnAddDays(void);
void LobbyWorld_NextDay(void);
void LobbyWorld_AddDays(f32 days);
void LobbyWorld_Skip(u32 n);
void LobbyWorld_RunEvents(void);
void CamReq_Cancel(void);
void CamReq_Mode25(void);
extern u32 D_0081B7A4[][2];          /* gLobbyBgmInfo[].len through its own label */
extern u16 *gLobbyBgmFiles[];              /* per track, the stream file of each entry */
extern f32 D_008FAC10;
extern s32 D_008FAC04;
extern s32 D_008FAC08;
void NetOverlay_Load(s32 ovl);
/* fade i's colour as 0xRRGGBBAA (0 when the stream is idle) */
s32 LobbyNet_DisconnectStep(void);
s32 LobbyLogin_ShowNetError(void);
s32 LobbyLogin_LeaveToSettings(void);
s32 LobbyLogin_LeaveWithMsg13(void);
extern s8 gNetDropped;
s32 LobbyCard_SaveNetFile(void);
void NetMode_OpenFrame4E(void);
void NetMode_SeqAReset(void);
void func_007CF630(s32 unused);
void NetMode_SetTitle(s32 arg0);
void NetModeMsg_Page1(s32 msg);
void NetMode_OpenYesNo(s8 cursor);
s32 LoginPad_Triggered(void);
s32 NetMode_ChooseYesNo(void);
void NetMode_BeginCardTask(s32 i);
void NetMode_CloseYesNo(void);
extern u8 gLobbySaveBuf[];                /* the NET file id and password, saved (0x200) */
extern u8 gMmbbLoginId[];                  /* the id (0xB) */
extern u8 D_5BE6BB[];                  /* the password (9) */
u16 Rand_Next(s32 stream);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void LobbyActor_DrawPart(DrawSlot *d);
void Mtx_Identity(f32 *m);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_TurnY(f32 *m, f32 a);
void Mtx_TurnZ(f32 *m, f32 a);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);

s32 Model_Draw(u32 h, s32 flags);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
s32 Gs_SetState(u8 reg, u32 v);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateY(f32 *m, f32 a);
extern s16 gAreaAEModels[];                /* area 0xAE's 3 models: part, angle, position */
extern u16 gAreaAEPropRot[];
extern f32 gAreaAEPropPos[];
extern s16 gAreaAFModels[];                /* area 0xAF's 5 */
extern u16 gAreaAFPropRot[];
extern f32 gAreaAFPropPos[];
/* the props of lobby areas 0xAE and 0xAF (a sub_main object, Obj_Alloc(0);
 * LobbyProp_Run by state; a view of the Obj): they stand 120 frames, tip over (tilt to
 * -0x4000, faster each frame), raise effects at their places, then sink
 * (LobbyProp_Draw draws them) */
typedef struct LobbyProp {
    /* 0x00 */ u8 active;           /* Obj.active */
    /* 0x01 */ u8 visible;          /* Obj.visible: 1 while it stands or sinks */
    /* 0x02 */ u8 kind;             /* Obj.kind: 0xF (LobbyProp_Spawn) */
    /* 0x03 */ u8 type;             /* Obj.type */
    /* 0x04 */ u8 state;            /* Obj.state: LobbyProp_Run's switch */
    /* 0x05 */ u8 phase;            /* Obj.var0: 0 standing, 1 tipping, 2 down */
    /* 0x06 */ u8 pad6[2];
    /* 0x08 */ s16 timer;           /* Obj.timer */
    /* 0x0A */ s16 tilt;            /* Obj.var3: 0x10000 a turn */
    /* 0x0C */ u8 padC[8];
    /* 0x14 */ void *draw;          /* Obj.draw: NULL (it draws through its slot) */
    /* 0x18 */ u8 pad18[8];
    /* 0x20 */ f32 pos[3];          /* Obj.pos */
    /* 0x2C */ u8 pad2C[0xC];
    /* 0x38 */ DrawSlot *ds;        /* Obj.ds */
    /* 0x3C */ s16 slot;            /* Obj.slot */
    /* 0x3E */ u8 pad3E[2];
} LobbyProp;
typedef char LobbyProp_size_check[(sizeof(LobbyProp) == 0x40) ? 1 : -1]; /* an Obj */

void Vec_Copy(f32 *dst, f32 *src);
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 size);
void LobbyProp_Draw(DrawSlot *d);
/* end of prototypes */


void func_007CF620(void);
void NetMode_LoadTextures(s32);
void *memcpy(void *dst, const void *src, u32 n);
void NetMode_OpenMenu2(s8 a, s8 b);
void NetModeMsg_Page3(s32 a0);
void CardTask_Begin(s32);
s32 LobbyCard_LoadFiles(s32 mode);
void CardTask_StartReadClear(s32 a, u8 *buf, s32 c);
void CardTask_Update(void);
s32 CardTask_Result(void);
s32 Card_IsChanged(s32 slot);
void CardTask_StartRead(s32 a, u8 *buf);
s32 LobbyCard_CheckNetFile(s32 arg0);
void Card_ClearChanged(void);
s32 Dnas_StateMachine(s8 *step, s16 *timer, s16 *arg2);
s32 NetXfer_Step(s8 *st, s16 *timer, u8 *buf, s32 a, s32 max);
s32 NetXfer_Step2(s8 *st, s16 *timer, u8 *buf, s32 a, s32 max);
s32 NetXfer_GetState_Unk(void);
char *strncpy(char *dst, const char *src, u32 n);  /* strncpy */
void Patch_ExecAll(void);
void LobbyLogin_FreeTextures(void);
void Gfx_ClearModelBuffers(void);
void func_007CFBF0(u8 msg);
extern s32 D_008FABF8;
extern s32 D_008EDEFC;
extern char D_008EDF00[];
extern char gNetVerDigits[];



void LobbyFlock_Init(LobbyFlock *o);
void LobbyFlock_Update(LobbyFlock *o);
void LobbyFlock_End(LobbyFlock *);
void LobbyFlock_Free(void *o);
void LobbyProp_Init(LobbyProp *o);
void LobbyProp_Update(LobbyProp *o);
void LobbyProp_End(LobbyProp *o);
void LobbyProp_Free(void *o);
void NetSetup_ToSettings(void);
void NetMode_CloseAll(void);
void NetMode_CloseBoxes(s32);
void func_007D44B0(s32 unused, s32 unused2);
void func_007D4730(s32 unused, s32 unused2);
s32 NetOverlay_Ready(s32 arg0);
s32 LoginPad_HoldTimer(s16 *arg0);
void LobbyLogin_BlinkWait(void);
s32 LobbyLogin_LeaveRelease(void);
s32 LobbyWorld_GetSeason(void);
s32 LobbyWorld_GetSeasonDay(void);
void LobbyWorld_HoldClock(void);
s32 LobbyBgm_PollStat(void);
u8 LobbyBgm_FindEntry(u32 t);
s32 LobbyBgm_GetEntry(void);
u32 LobbyBgm_GetEnd(void);
void LobbyActor_StartScript(Em *o, s16 set, s16 idx);
void LobbyActor_Update(Em *o);
void Effect06_Start(s16 k);
void CamReq_FromTable(u16 a, u16 b);
void LobbyWorld_StartFades(s32 t, s32 k);
void LobbyWorld_StepFades(void);
s32 Debug_IsOn(s32);
void LobbyBgm_SelectEntry(s32 i);
void Stream_FadeOut(s32 ch, s32 frames);
s32 Stream_IsReady(s32 ch);
void Stream_StartPrepared(s32 ch);
void Bgm_ForgetCurrent(void);
s32 func_007D8890(void);
void LobbyBgm_EndToArea(void);
void LobbyBgm_End(void);
void LobbyBgm_StopAll(void);
s32 LobbyBgm_IsActive(void);
s32 LobbyBgm_GetState(void);
u8 func_007D9130(void);
void func_007D9140(void);
s32 func_007D98B0(void);
void LobbyBgm_SetActor(Em *actor);
Em *LobbyBgm_GetActor(void);
void LobbyBgm_Shutdown(void);
void LobbyMovie_Stop(void);
void LobbyProp_Spawn(void);
void LobbyFlock_Spawn(void);
void *memset(void *dst, s32 c, u32 n);
extern u8 gLbsConnectStep;
extern u8 gNetScreenStep;
void LobbyStream_Stop(void);
void LobbyMenu_QueueLayers(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Lobby_SetBootMode(s32);
void NetMode_ReleaseTextures(M2C_UNK unused0);
u8 FadeA_GetState(void);
void FadeA_Start(s32);
void Game_ResetSubsystems(void);
void NetMenu_Open(u8);
u8 NetMenu_Update(void);
extern u8 gNetConnStep;
extern M2C_UNK Gfx_DrawBlackLineFixed;
u32 LobbyBgm_GetPos(void);
s32 LobbyBgm_GetTrack(void);
void LobbyBgm_Play(s32 id, s32 pos, s32 arg2);
extern u8 D_008FAC78;
extern f32 D_008FAC54;
extern f32 D_008FAC14;
void Sound_PlaySeMusicVol(u32 slot, u32 id, s32 prog);
s32 NetOverlay_Busy(void);
void Lobby_ReleaseNetTextures(void);
extern s32 D_008FAC60;
extern s32 D_008FAC5C;
extern s32 D_008FAC74;
void Bgm_PlayAreaMusic(u16);
s32 Bgm_StopInArea87(void);
s32 Stream_GetStat(s32);
extern s32 D_008FAC8C;
extern u8 D_008FAC79;
extern s8 D_008FAD16;
void LobbyBgm_ClearState(void);
void LobbyWorld_ClearFades(void);
#include "stream.h"
#include "lobby_globals.h"
#include "save.h"
#include "pad.h"
#include "obj.h"
void LobbyBgm_Stop(void);
extern s8 D_008FAD18;
extern u8 D_008FAD1F;
void LobbyWorld_Reset(void);
void Movie_RequestStop(void);
s32 Movie_Play(s32 id);
s32 Movie_Update(void);
s32 Movie_GetStatus(void);
void Movie_Stop(void);
extern s8 D_008FAD28;
struct Obj *Obj_Alloc(s32 n);
void LobbyProp_Run(LobbyProp *);
void DrawSlot_Free(s32);
void Obj_Free(Obj *p);
void LobbyFlock_Run(LobbyFlock *);
void func_007D4740(void);
void Text_Reset(void);
extern s16 gNetConnTries;
void LobbyWorld_PlayAreaMusic(void);

void NetMode_PlayBgm(void)
{
    Sound_PlaySeMusicVol(0x6, 0, 0);
}

s32 NetOverlay_Request(s32 kind)
{
    s32 r;

    r = 0;
    if (NetOverlay_Busy() == 0) {
        switch (kind) {
        case 0:
            NetOverlay_Load(5);
            break;
        case 1:
            NetOverlay_Load(7);
            break;
        case 2:
            NetOverlay_Load(8);
            break;
        case 3:
            break;
        }
        r = 1;
    }
    return r;
}

s32 NetOverlay_Ready(s32 unused) {
    s32 ret;

    ret = 0;
    if (NetOverlay_Busy() == 0) {
        ret = 1;
    }
    return ret;
}

s32 NetSetup_DeleteAccount(s32 mode)
{
    NetModeState *st;
    u8 *id;
    u8 *pw;
    s32 r;

    st = &gNetMode;
    r = 0;
    NetMode_SetTitle(5);
    if (mode == 0) {
        id = (u8 *)gLobbySaveBuf;
        pw = id + 0x100;
    } else {
        id = *(u8 * *)&gSaveBuf;
        pw = id + 0x100;
    }
    LoginPad_Triggered();
    switch (st->subStep) {
    case 0:
        st->innerStep = 0;
        if (mode == 1) {
            st->subStep++;
        } else {
            memcpy(id, gMmbbLoginId, 0xB);
            memcpy(pw, D_5BE6BB, 9);
            st->subStep = 3;
            st->slot = gSysSettings.unk4A;
            NetMode_OpenYesNo(1);
        }
        break;
    case 1:
    case 2:
        break;
    case 3:
        NetModeMsg_Page1(0x26);
        switch (NetMode_ChooseYesNo()) {
        case 0:
            break;
        case 1:
            NetMode_CloseYesNo();
            memset(gMmbbLoginId, 0, 0xB);
            memset(D_5BE6BB, 0, 9);
            st->subStep = 4;
            st->innerStep = 0;
            NetMode_BeginCardTask(1);
            gSysBusy = 1;
            break;
        case -1:
            NetMode_CloseYesNo();
            r = 2;
            break;
        }
        break;
    case 4:
        switch (LobbyCard_SaveNetFile()) {
        case 0:
            break;
        case 1:
            r = 1;
            gSysBusy = 0;
            break;
        case -1:
            memcpy(gMmbbLoginId, id, 0xB);
            memcpy(D_5BE6BB, pw, 9);
            r = -1;
            gSysBusy = 0;
            break;
        }
        break;
    }
    return r;
}

s32 LoginPad_Held(s8);
s32 LoginPad_HoldTimer(s16 *sel2) {
    s32 ret;

    ret = 0;
    if (LoginPad_Held(0) & 0x8000) {
        *sel2 += 1;
        if (*sel2 >= 0x1E) {
            ret = 1;
        }
    } else {
        *sel2 = 0;
    }
    return ret;
}

void NetMode_OpenPercent(void);
void NetMode_ResetAnim2(void);
void NetMode_OpenProgress2(void);
void NetMode_OpenError(void);
void NetMode_OpenIcon2(s8 arg0);
void NetModeMsg_Page0(s32 arg0);
s32 NetMode_SeqABusy(void);
s32 Dnas_AuthStep(s8 *arg0, s16 *arg1, s16 *arg2);
s32 Dnas_GetError(void);

s32 NetSetup_Dnas(void)
{
    s32 r;
    NetModeState *st;
    s32 k;

    r = 0;
    st = &gNetMode;
    switch (st->step) {
    case 0:
        st->step++;
        st->percent = 0;
        st->unk7E = 1;
        NetMode_OpenPercent();
        NetMode_OpenIcon2(3);
        NetMode_OpenProgress2();
        break;
    case 1:
        if (NetOverlay_Request(1) != 0) {
            st->step++;
        }
        break;
    case 2:
        if (NetOverlay_Ready(1) != 0) {
            st->step++;
            st->innerStep = 0;
            st->percent = 0;
            st->netResult = 0;
            NetMode_OpenProgress2();
        }
        break;
    case 3:
        st->step++;
        break;
    case 4:
        st->step = 10;
        st->innerStep = 0;
        NetMode_ResetAnim2();
        break;
    case 10:
        st->step = 5;
        st->timer = 0;
        st->netResult = 1;
        st->sel2 = 0;
        break;
    case 5:
        if (LoginPad_HoldTimer(&st->sel2) != 0) {
            st->timer = 0x3D;
        }
        k = Dnas_AuthStep((s8 *)&st->innerStep, &st->timer, &st->percent);
        if (k < 0) {
            st->step = 15;
            st->subStep = 0;
            st->innerStep = 0;
            st->timer = 0x38;
            st->netResult = 2;
            st->err = Dnas_GetError();
            NetMode_CloseBoxes(0x800000);
        } else if (k > 0) {
            st->step = 11;
            st->subStep = 0;
            st->innerStep = 0;
            st->timer = 0x18;
            st->netResult = 3;
        }
        break;
    case 9:
        if (--st->timer <= 0) {
            st->percent = 0;
            st->step = 8;
            st->timer = 10;
            NetMode_CloseBoxes(0x100);
            NetMode_CloseBoxes(0x200);
            NetMode_CloseBoxes(0x10000);
            NetMode_CloseBoxes(0x80);
            NetMode_CloseBoxes(0x800000);
        }
        break;
    case 11:
        if (--st->timer <= 0) {
            st->percent = 0;
            st->step = 0x14;
            st->timer = 0;
            NetMode_CloseBoxes(0x80);
            NetMode_CloseBoxes(0x800000);
        }
        break;
    case 8:
        st->step = 0x64;
        st->percent = 0;
        NetMode_OpenIcon2(4);
        NetMode_OpenError();
        break;
    case 15:
        if (--st->timer <= 0) {
            st->percent = 0;
            st->step = 9;
            st->timer = 0x2D;
            st->netResult = 3;
        }
        break;
    case 0x64:
        if (st->percent <= 0) {
            if (LoginPad_Decide(2) != 0) {
                st->step++;
            } else {
                NetModeMsg_Page0(0x14);
            }
        } else {
            st->percent--;
        }
        break;
    case 0x65:
        st->step++;
    case 0x66:
        st->step++;
        NetMode_CloseAll();
    case 0x67:
        st->unk7E = 0;
        r = -1;
        break;
    case 0x14:
        if (NetMode_SeqAStart(0, 10, 1) != 0) {
            st->step++;
        }
        break;
    case 0x15:
        if (NetMode_SeqABusy() != 0) {
            break;
        }
        st->step++;
        NetMode_CloseAll();
    case 0x16:
        st->unk7E = 0;
        r = 1;
        break;
    }
    return r;
}

s32 NetSetup_SaveNetFile(void)
{
    NetModeState *st;
    s32 r;

    st = &gNetMode;
    r = 0;
    switch (st->step) {
    case 0:
        if (gNetSetupResult == 1 && st->ynResult == 0) {
            r = 1;
            break;
        }
        gNetSetupResult = 1;
        st->step++;
        st->innerStep = 0;
    case 1:
        st->sel3 = 0;
        st->step++;
        func_007CF630(0);
        NetMode_SeqAReset();
        break;
    case 2:
        st->step++;
        st->percent = 10;
        NetMode_OpenFrame4E();
        break;
    case 3:
        switch (LobbyCard_SaveNetFile()) {
        case 0:
            break;
        case 1:
            st->ynResult = 0;
        case -1:
            st->step++;
            break;
        }
        break;
    case 4:
        r = 1;
        break;
    }
    return r;
}

void LobbyNetXfer_Reset(void)
{
    gNetMode.xferStep = 0;
    gNetMode.xferSub = 0;
    gNetMode.pad15 = 1;
    LobbyCard_ResetNetSave();
}

/* the online data exchange, one step a frame (step 0x5D472A, from
 * LobbyNetXfer_Reset): reads the memory card's NET file, passes DNAS
 * (Dnas_StateMachine), sends the 64K block at *(0x6AE018) (NetXfer_Step),
 * asks with NetMenu, receives the reply into the second block
 * (NetXfer_Step2) and copies both to 0xAA6780. Returns 1 when done, -1
 * after an error's message (steps 25-45), else 0 */
s32 LobbyNetXfer_Run(void)
{
    NetModeState *st;
    s32 r;
    u8 *buf0;
    u8 *buf1;
    u8 *buf2;

    st = &gNetMode;
    buf0 = gSysSettings.extraBuf;
    buf1 = buf0 + 0x10000;
    buf2 = buf1 + 0x10000;
    r = 0;
    switch (*(u8 *)&gNetMode.xferStep) {
    case 0:
        switch (st->xferSub) {
        case 0:
            st->xferSub++;
            st->netResult = 0;
            st->cardMsg = 0;
            st->err = 0;
            D_008FABF8 = D_6AEFA8;
            st->slotUsed = D_008FABF8;
            NetMode_ReleaseTextures(0);
            func_007CF620();
            func_007CF630(0);
            NetMode_SeqAReset();
            NetMode_LoadTextures(0);
            memset(buf1, 0, 0x10000);
            memset(buf2, 0, 0x40);
            memcpy(buf1, buf0, 0x10000);
            break;
        case 1:
            st->xferSub++;
            break;
        case 2:
            st->xferSub++;
            break;
        case 3:
            if (NetOverlay_Request(1) != 0) {
                st->xferSub++;
            }
            break;
        case 4:
            if (NetOverlay_Ready(1) != 0) {
                st->xferStep++;
                st->xferSub = 0;
                st->percent = 2;
                st->sel2 = 0;
                NetMode_OpenFrame();
                NetMode_OpenMenu2(0xB, 2);
                NetMode_ResetToFrame();
                NetMode_SeqAStart(0, 0xA, 0);
                NetMode_OpenYesNo(1);
            }
            break;
        }
        break;
    case 1:
        NetModeMsg_Page3(1);
        if (--st->percent <= 0) {
            st->xferStep++;
            st->percent = 0xE10;
        }
        break;
    case 2:
        NetModeMsg_Page3(1);
        switch (NetMode_ChooseYesNo()) {
        case 0:
            break;
        case 1:
            st->xferStep = 9;
            st->percent = 2;
            NetMode_ResetToFrame();
            break;
        case -1:
            st->xferStep = 0x28;
            st->percent = 2;
            NetMode_ResetToFrame();
            break;
        }
        break;
    case 3:
        NetModeMsg_Page3(1);
        if (--st->percent <= 0) {
            st->xferStep++;
            st->percent = 2;
            gSysBusy = 1;
        }
        break;
    case 4:
        NetModeMsg_Page3(0xA);
        if (--st->percent <= 0) {
            st->xferStep++;
            st->xferState = 0;
            NetMode_BeginCardTask(0);
            CardTask_Begin(0);
            gSysBusy = 1;
        }
        break;
    case 5:
        NetModeMsg_Page3(0xA);
        if (LobbyCard_LoadFiles(1) != 0) {
            st->xferStep++;
            st->slotUsed = D_008FABF8;
            CardTask_StartReadClear(st->slotUsed, (u8 *) (*(s32 *)&gLoadBuf), 0);
        }
        break;
    case 6:
        NetModeMsg_Page3(0xA);
        CardTask_Update();
        switch (CardTask_Result()) {
        case 0:
            if (Card_IsChanged(st->slotUsed) != 0) {
                st->xferStep++;
                CardTask_StartRead(st->slotUsed, (u8 *) (*(s32 *)&gLoadBuf));
            } else {
                st->xferStep = 8;
                st->percent = 2;
                NetMode_ResetToFrame();
                gSysBusy = 0;
            }
            break;
        case -251:
        case -254:
        case -256:
        case -253:
        case -252:
            st->xferStep = 0x1B;
            st->percent = 2;
            st->cardMsg = 7;
            gSysBusy = 0;
            break;
        case -255:
            st->xferStep = 0x19;
            st->percent = 2;
            st->cardMsg = 6;
            gSysBusy = 0;
            break;
        }
        break;
    case 7:
        NetModeMsg_Page3(0xA);
        CardTask_Update();
        switch (CardTask_Result()) {
        case 0:
            switch (LobbyCard_CheckNetFile(0)) {
            case -1:
                st->xferStep = 0x1D;
                st->percent = 2;
                st->cardMsg = 0x62;
                gSysBusy = 0;
                break;
            case -2:
                st->xferStep = 0x1E;
                st->percent = 2;
                st->cardMsg = 0x78;
                gSysBusy = 0;
                break;
            default:
                st->xferStep++;
                Card_ClearChanged();
                st->percent = 2;
                NetMode_ResetToFrame();
                gSysBusy = 0;
                break;
            }
            break;
        case -251:
        case -254:
        case -256:
        case -253:
        case -252:
            st->xferStep = 0x1B;
            st->percent = 2;
            st->cardMsg = 0x45;
            gSysBusy = 0;
            break;
        case -255:
            st->xferStep = 0x19;
            st->percent = 2;
            st->cardMsg = 0x44;
            gSysBusy = 0;
            break;
        }
        break;
    case 8:
        if (--st->percent <= 0) {
            st->xferStep++;
            st->percent = 2;
            gSysBusy = 0;
        }
        break;
    case 9:
        if (--st->percent <= 0) {
            st->xferStep++;
            st->xferState = 0;
            st->percent = 0;
            st->timer = 0;
            st->netResult = 1;
            st->sel2 = 0;
            st->err = 0;
            NetMode_ResetAnim2();
            NetMode_OpenPercent();
            NetMode_OpenIcon2(3);
            NetMode_OpenProgress2();
        }
        break;
    case 10: {
        s32 ret;

        if (LoginPad_HoldTimer(&st->sel2) != 0) {
            st->timer = 0x1E;
        }
        ret = Dnas_StateMachine(&st->xferState, &st->timer, &st->percent);
        if (ret < 0) {
            st->xferStep = 0x23;
            st->xferSub = 0;
            st->xferState = 0;
            st->percent = 0;
            st->timer = 0x38;
            st->netResult = 2;
            st->err = Dnas_GetError();
            NetMode_CloseBoxes(0x800000);
        } else if (ret > 0) {
            st->timer = 0x18;
            st->xferStep = 0x32;
            st->xferSub = 0;
            st->xferState = 0;
            st->size = D_008EDEFC;
            memcpy(buf0, buf1, 0x10000);
        }
        break;
    }
    case 11:
        switch (st->xferSub) {
        case 0:
            if (NetOverlay_Request(2) != 0) {
                st->xferSub++;
            }
            break;
        case 1:
            if (NetOverlay_Ready(2) != 0) {
                st->xferStep++;
                st->xferSub = 0;
            }
            break;
        }
        break;
    case 12: {
        s32 ret;

        ret = NetXfer_Step(&st->xferState, &st->timer, buf0, st->size, 0x10000);
        if (0 < ret) {
            st->percent = ret;
            st->xferStep = 0xD;
            st->xferSub = 0;
            st->xferState = 0;
            st->timer = 0x72;
            st->netResult = 3;
            *(s32 *)(buf2 + 0x14) = ret;
        } else if (ret < 0) {
            st->percent = 0;
            st->xferStep = 0x23;
            st->xferSub = 0;
            st->xferState = 0;
            st->timer = 0x20;
            st->netResult = 2;
            st->err = NetXfer_GetState_Unk();
        }
        break;
    }
    case 13:
        if (--st->timer <= 0) {
            st->xferStep++;
            st->percent = 2;
            NetMode_CloseBoxes(0x100);
            NetMode_CloseBoxes(0x200);
            NetMode_CloseBoxes(0x10000);
            NetMode_CloseBoxes(0x80);
            NetMode_CloseBoxes(0x800000);
        } else if (st->timer < 0x3D && LoginPad_Decide(2) != 0) {
            st->xferStep++;
            st->percent = 2;
            NetMode_CloseBoxes(0x100);
            NetMode_CloseBoxes(0x200);
            NetMode_CloseBoxes(0x10000);
            NetMode_CloseBoxes(0x80);
            NetMode_CloseBoxes(0x800000);
        }
        break;
    case 14:
        if (--st->percent <= 0) {
            st->xferStep++;
            st->subStep = 0;
            st->innerStep = 0;
            strncpy((char *)buf2, D_008EDF00, 0xA);
            strncpy((char *)buf2 + 0xA, gNetVerDigits, 4);
            NetMode_ResetToFrame();
            NetMenu_Open(0x11);
        }
        break;
    case 15:
        switch ((u8)NetMenu_Update()) {
        case 1:
            st->xferStep = 0x10;
            st->xferState = 0;
            st->size = *(s32 *)(buf2 + 0x14);
            st->timer = 0;
            memcpy(buf1, buf0, 0x10000);
            st->err = 0;
            st->netResult = 1;
            gSysBusy = 0;
            break;
        case 2:
            st->xferStep = 0x14;
            st->percent = 0xA;
            gSysBusy = 0;
            break;
        }
        break;
    case 16: {
        s32 ret;

        ret = NetXfer_Step2(&st->xferState, &st->timer, buf1, st->size, 0x10000);
        if (0 < ret) {
            memcpy(PATCH_DATA, buf0, 0x10000);
            strncpy((char *)PATCH_HEADER, (char *)buf2, 0xA);
            strncpy((char *)(PATCH_HEADER + 0xA), (char *)buf2 + 0xA, 4);
            *(s32 *)(PATCH_HEADER + 0x14) = *(s32 *)(buf2 + 0x14);
            memcpy(PATCH_RECS, buf1, 0x10000);
            Patch_ExecAll();
            st->percent = 0;
            st->xferStep++;
            st->xferSub = 0;
            st->xferState = 0;
            st->timer = 0;
            st->percent = 0x96;
            st->netResult = 3;
        } else if (ret < 0) {
            st->percent = 0;
            st->xferStep = 0x23;
            st->xferSub = 0;
            st->xferState = 0;
            st->timer = 0x20;
            st->netResult = 2;
            st->err = NetXfer_GetState_Unk();
        }
        break;
    }
    case 17:
        st->xferStep++;
        NetMode_SeqAStart(0, 0x14, 1);
        break;
    case 18:
        if (NetMode_SeqABusy() != 0) {
            break;
        }
        st->xferStep++;
        st->slotUsed = st->slot;
        NetMode_CloseAll();
        LobbyLogin_FreeTextures();
    case 19:
        Gfx_ClearModelBuffers();
        r = 1;
        break;
    case 20:
        if (--st->percent <= 0) {
            st->xferStep++;
            st->percent = 2;
            st->sel2 = 0;
            NetMode_OpenMenu2(0xB, 2);
            NetMode_ResetToFrame();
        }
        break;
    case 21:
        switch (st->xferSub) {
        case 0:
            if (NetOverlay_Request(1) != 0) {
                st->xferSub++;
            }
            break;
        case 1:
            if (NetOverlay_Ready(1) != 0) {
                st->xferStep = 1;
                st->xferSub = 0;
                NetMode_OpenMenu2(0xB, 2);
                NetMode_ResetToFrame();
                NetMode_OpenYesNo(1);
            }
            break;
        }
        break;
    case 25:
        if (--st->percent <= 0) {
            st->xferStep++;
            st->percent = 0x96;
            st->cardMsg = 6;
        }
        break;
    case 26:
        NetModeMsg_Page3(st->cardMsg);
        if (--st->percent <= 0) {
            st->xferStep = 0x28;
            st->percent = 2;
        } else if (st->percent < 0x97) {
            func_007CFBF0(st->cardMsg);
            if (LoginPad_Decide(2) != 0) {
                st->xferStep = 0x28;
                st->percent = 2;
            }
        }
        break;
    case 27:
        if (--st->percent <= 0) {
            st->xferStep = 0x1A;
            st->percent = 0x96;
            st->cardMsg = 7;
        }
        break;
    case 28:
        if (--st->percent <= 0) {
            st->xferStep = 0x1A;
            st->percent = 0x96;
            st->cardMsg = 9;
        }
        break;
    case 29:
        if (--st->percent <= 0) {
            st->xferStep = 0x1A;
            st->percent = 0x96;
            st->cardMsg = 0xF;
        }
        break;
    case 30:
        if (--st->percent <= 0) {
            st->xferStep = 0x1A;
            st->percent = 0x96;
            st->cardMsg = 8;
        }
        break;
    case 35:
        if (--st->percent <= 0) {
            st->xferStep++;
            st->netResult = 3;
            st->percent = 0x2D;
            NetMode_CloseBoxes(0x80);
        }
        break;
    case 36:
        if (--st->percent <= 0) {
            st->xferStep++;
            st->percent = 2;
            NetMode_CloseBoxes(0x100);
            NetMode_CloseBoxes(0x200);
            NetMode_CloseBoxes(0x10000);
            NetMode_CloseBoxes(0x20);
        }
        break;
    case 37:
        if (--st->percent <= 0) {
            st->xferStep++;
            st->percent = 2;
            NetMode_OpenIcon2(4);
            NetMode_OpenError();
        }
        break;
    case 38:
        if (--st->percent <= 0) {
            st->xferStep++;
            st->percent = 0x13B;
            st->timer = 0xD2;
        }
        break;
    case 39:
        if (--st->percent <= 0) {
            st->xferStep = 0x28;
            st->percent = 2;
            st->sel2 = 0;
            NetMode_CloseBoxes(0x20000);
        } else if (st->percent <= st->timer && LoginPad_Decide(2) != 0) {
            st->xferStep = 0x28;
            st->percent = 2;
            NetMode_CloseBoxes(0x20000);
        }
        break;
    case 40:
        if (--st->percent <= 0) {
            st->xferStep++;
            st->percent = 0xA;
            st->sel2 = 0;
            NetMode_OpenMenu2(0xB, 2);
            NetMode_ResetToFrame();
            NetMode_OpenYesNo(1);
        }
        break;
    case 41:
        NetModeMsg_Page3(3);
        st->xferStep++;
        st->percent = 0xE10;
        break;
    case 42:
        NetModeMsg_Page3(3);
        switch (NetMode_ChooseYesNo()) {
        case 0:
            break;
        case 1:
            st->xferStep++;
            st->percent = 0x3C;
            NetMode_ResetToFrame();
            break;
        case -1:
            st->xferStep = 0x2D;
            st->percent = 2;
            NetMode_ResetToFrame();
            break;
        }
        break;
    case 43:
        if (--st->percent <= 0) {
            st->xferStep++;
            NetMode_CloseAll();
            st->slotUsed = st->slot;
            LobbyLogin_FreeTextures();
        }
        break;
    case 44:
        Gfx_ClearModelBuffers();
        r = -1;
        break;
    case 45:
        if (--st->percent <= 0) {
            st->xferStep = 0x15;
            st->xferSub = 0;
            st->subStep = 0;
            st->percent = 2;
        }
        break;
    case 50:
        if (--st->timer <= 0) {
            NetMode_CloseBoxes(0x80);
            NetMode_CloseBoxes(0x800000);
            st->xferStep = 0xB;
            st->xferSub = 0;
            st->xferState = 0;
            st->timer = 0;
        }
        break;
    }
    return r;
}

void LobbyLogin_BlinkWait(void) {
    Text_Reset();
    gNetConnTries += 1;
    if (!(gNetConnTries & 0x10)) {
        func_007D4740();
    }
}

s32 LobbyLogin_RunConnect(void)
{
    s16 temp_v0_3;
    s32 temp_v0;
    s32 temp_v0_2;
    s32 temp_v0_4;
    s32 ret;

    ret = 0;
    switch (gLoginState) {
    case 0:
        gLoginState += 1;
        break;
    case 1:
        temp_v0 = NetScreen_Run();
        switch (temp_v0) {
        case 1:
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            gLoginState += 1;
            break;
        case -1:
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginState = 1;
            gNetConnStep = 1;
            gLoginConnectStep = 0;
            break;
        case -2:
            gNetConnStep = 0;
            ret = -2;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginState = 1;
            gLoginConnectStep = 0;
            break;
        case -3:
            gPortalPage = 2;
            NetScene_ToState6();
            break;
        case -4:
            gLoginState = 0;
            ret = -1;
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            break;
        case -5:
            gLoginState = 0;
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            break;
        default:
            ret = 2;
        }
        break;
    case 2:
        LobbyDialog_ShowConnMsg(2);
        temp_v0_2 = NetScreen_ConnectStep();
        switch (temp_v0_2) {
        case 1:
            Loading_Stop();
            ret = 1;
            gLoginState = 0;
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            break;
        case -1:
            Loading_Stop();
            gNetConnStep = 0;
            gLoginState = 0xA;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
        }
        break;
    case 10:
        switch (gNetConnStep) {
        case 0:
            NetMode_Init();
            gNetConnStep += 1;
            
        case 1:
            gLbsConnectStep = 0;
            gNetConnStep += 1;
            gNetStepTimeout = 1;
            NetMode_OpenFrame();
            break;
        case 2:
            temp_v0_3 = gNetStepTimeout - 1;
            gNetStepTimeout = temp_v0_3;
            if (temp_v0_3 <= 0) {
                gNetConnStep += 1;
                gNetStepTimeout = 0x12C;
                NetMode_OpenIcon();
                NetMode_SeqAStart(0, 0xA, 0);
            }
            break;
        case 3:
            gNetStepTimeout -= 1;
            if (gNetStepTimeout > 0) {
                if ((gNetStepTimeout < 0xF0) && (LoginPad_Decide(2) != 0)) {
                    goto block_40;
                }
            } else {
block_40:
                NetMode_SeqAStart(0, 0xA, 1);
                gNetStepTimeout = 0x14;
                gNetConnStep += 1;
            }
block_41:
            NetModeMsg_QueueKind3(0xDU);
            NetMode_CallDrawScreen(0);
            break;
        case 4:
            gNetStepTimeout -= 1;
            if (gNetStepTimeout <= 0) {
                NetMode_CloseAll();
                gNetEntryMode = 0;
                gPortalPage = 3U;
                NetScene_ToState6();
                gNetStepTimeout = 0;
                gNetConnTries = 0;
            } else {
                goto block_41;
            }
        }
        break;
    case 20:
        temp_v0_4 = LobbyKddi_PortalStep();
        switch (temp_v0_4) {
        case 1:
            gLoginState = 1;
            break;
        case -1:
            gNetConnStep = 0;
            ret = -2;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginState = 1;
            gLoginConnectStep = 0;
            break;
        default:
            if (gPortalPage == 3) {
                gNetStepTimeout += 1;
                if (gNetStepTimeout >= 0x5A) {
                    LobbyLogin_BlinkWait();
                }
            }
        }
        break;
    }
    return ret;
}

s32 LobbyLogin_Exit(void)
{
    s32 temp_v1;
    s32 ret;

    ret = 0;
    switch (gNetConnStep) {                           /* irregular */
    case 0:
        Game_ResetSubsystems();
        NetMode_ReleaseTextures(0);
        LobbyMenu_SetDraw(0, 0, &Gfx_DrawBlackLineFixed, 0);
        FadeA_Start(2);
        LobbyStream_Stop();
        NetMenu_Open(8);
        gNetConnStep += 1;
        break;
    case 1:
        temp_v1 = NetMenu_Update() & 0xFF;
        if (temp_v1 != 0) {
            if (temp_v1 == 2) {
                Lobby_SetBootMode(1);
            }
            gNetConnStep += 1;
            FadeA_Start(1);
        }
        break;
    case 2:
        if ((FadeA_GetState() & 0xFF) != 1) {
            gNetConnStep += 1;
        }
        break;
    case 3:
        ret = 1;
        break;
    }
    LobbyMenu_QueueLayers();
    return ret;
}

s32 LobbyLogin_Leave(void)
{
    s32 r;

    r = 0;
    switch (gLoginState) {
    case 0:
        switch (LobbyKddi_PortalStep()) {
        case 1:
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            gNetDropped = 0;
            gLoginState += 1;
            break;
        case -1:
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            gNetDropped = 0;
            gLoginState += 1;
        }
        break;
    case 1:
        switch (LobbyNet_DisconnectStep()) {
        case 1:
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            gNetDropped = 0;
            gLoginState += 1;
            break;
        }
        break;
    case 2:
        switch (gNetErrorCode) {
        case -2:
        case -3:
        case -5:
        case -81:
        case -82:
        case -83:
        case -84:
        case -85:
        case -80:
        case -86:
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gLoginState += 1;
            break;
        case -9:
            gLoginState += 1;
            gNetConnStep = 1;
            gLbsConnectStep = 0;
            break;
        case -11:
            gNetConnStep = 2;
            gLbsConnectStep = 0;
            gLoginState += 1;
            break;
        case -50:
        case -17:
            gLbsConnectStep = 0;
            gLoginState += 1;
            gNetConnStep = 3;
            break;
        default:
            gLoginState = 4;
        }
        break;
    case 3:
        switch (gNetConnStep) {
        case 0:
            if (LobbyLogin_ShowNetError() != 0) {
                r = 3;
                gLoginState = 0;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
            }
            break;
        case 1:
            if (LobbyLogin_LeaveToSettings() != 0) {
                r = 3;
                gLoginState = 0;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
            }
            break;
        case 2:
            if (LobbyLogin_LeaveRelease() != 0) {
                r = 1;
                gLoginState = 0;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
            }
            break;
        case 3:
            if (LobbyLogin_LeaveWithMsg13() != 0) {
                r = 2;
                gLoginState = 0;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
                gPortalPage = 2;
            }
        }
        break;
    case 4:
        gLoginState = 0;
        r = 1;
        gNetConnStep = 0;
        gLbsConnectStep = 0;
        gNetScreenStep = 0;
        gLoginConnectStep = 0;
        break;
    }
    NetMode_CallDrawScreen(0);
    return r;
}

s32 LobbyLogin_ShowNetError(void)
{
    s16 temp_v0;
    s16 temp_v0_2;
    s32 ret;

    ret = 0;
    switch (gLbsConnectStep) {
    case 0:
        if (gOvlTex != 0) {
            if (gNetMode.boxes != 0) {
                gLbsConnectStep = 2;
                gNetStepTimeout = 0x14;
                NetMode_ResetToFrame();
            }
        } else {
            gNetScreenStep = 0;
            gLbsConnectStep = 1;
            Game_ResetSubsystems();
            NetMode_ReleaseTextures(0);
        }
        break;
    case 1:
        switch (gNetScreenStep) {
        case 0:
            gNetScreenStep += 1;
            break;
        case 1:
            gLbsConnectStep += 1;
            gNetStepTimeout = 0x14;
            gNetScreenStep = 0;
            NetMode_OpenFrame();
        }
        break;
    case 2:
        temp_v0 = gNetStepTimeout - 1;
        gNetStepTimeout = temp_v0;
        if (temp_v0 <= 0) {
            gLbsConnectStep += 1;
            gNetStepTimeout = 0x258;
            NetMode_OpenIcon();
        }
        break;
    case 3:
        switch (gNetErrorCode) {
        case -2:
            NetModeMsg_QueueKind3(3U);
            break;
        case -3:
            NetModeMsg_QueueKind3(4U);
            break;
        case -5:
            NetModeMsg_QueueKind3(5U);
            break;
        case -81:
            NetModeMsg_QueueKind3(1U);
            break;
        case -82:
            NetModeMsg_QueueKind3(2U);
            break;
        case -83:
            NetModeMsg_QueueKind3(6U);
            break;
        case -84:
            NetModeMsg_QueueKind3(7U);
            break;
        case -85:
            NetModeMsg_QueueKind3(8U);
            break;
        case -86:
            NetModeMsg_QueueKind3(0xCU);
            break;
        }
        if (--gNetStepTimeout <= 0) {
            gLbsConnectStep += 1;
        } else if ((gNetStepTimeout < 0x1E1) && (LoginPad_Decide(2) != 0)) {
            gLbsConnectStep += 1;
        }
        break;
    case 4:
        gLbsConnectStep += 1;
        NetSetup_ToSettings();
        NetMode_CloseAll();
        
    case 5:
        ret = 1;
        break;
    }
    return ret;
}

s32 LobbyLogin_LeaveToSettings(void)
{
    s32 ret;

    ret = 0;
    switch (gLbsConnectStep) {
    case 0:
        gLbsConnectStep += 1;
        gNetScreenStep = 0;
        Game_ResetSubsystems();
        NetMode_ReleaseTextures(0);
        break;
    case 1:
        switch (gNetScreenStep) {
        case 0:
            gNetScreenStep += 1;
            NetMode_CloseBoxes(0x20);
            break;
        case 1:
            gLbsConnectStep += 1;
            gNetScreenStep = 0;
            NetMode_CloseBoxes(0x20);
        }
        break;
    case 2:
        gLbsConnectStep += 1;
        NetSetup_ToSettings();
        NetMode_CloseAll();
        
    case 3:
        ret = 1;
        break;
    }
    return ret;
}

s32 LobbyLogin_LeaveRelease(void) {
    Lobby_ReleaseNetTextures();
    return 1;
}

s32 LobbyLogin_LeaveWithMsg13(void)
{
    s32 r;

    r = 0;
    switch (gLbsConnectStep) {
    case 0:
        gLbsConnectStep += 1;
        gNetScreenStep = 0;
        Game_ResetSubsystems();
        NetMode_ReleaseTextures(0);
        break;
    case 1:
        switch (gNetScreenStep) {
        case 0:
            gNetScreenStep += 1;
            break;
        case 1:
            gLbsConnectStep += 1;
            gNetStepTimeout = 0x14;
            gNetScreenStep = 0;
            NetMode_OpenFrame();
        }
        break;
    case 2:
        if (--gNetStepTimeout <= 0) {
            gLbsConnectStep += 1;
            gNetStepTimeout = 0x258;
            NetMode_OpenIcon();
        }
        break;
    case 3:
        NetModeMsg_QueueKind3(0xD);
        if (--gNetStepTimeout <= 0) {
            gLbsConnectStep += 1;
            NetMode_CloseBoxes(0x20);
        } else if (gNetStepTimeout < 0x1E1 && LoginPad_Decide(2) != 0) {
            gLbsConnectStep += 1;
            NetMode_CloseBoxes(0x20);
        }
        break;
    case 4:
        gLbsConnectStep += 1;
        NetMode_CloseAll();
    case 5:
        gNetErrorCode = 0;
        r = 1;
        break;
    }
    return r;
}

extern u32 gSubVSyncCount;
void LobbyWorld_SetClock(u32 time)
{
    LobbyWorld *s = &gLobbyWorld;
    s32 i;
    s32 *days;

    if (gTownDayTicks == 0) {
        gTownDayTicks = 86400;
    }
    time %= 86400;
    s->dayTicks = (u32)gTownDayTicks;
    s->unk14 = 30.0f * s->dayTicks;
    s->dayFrames = s->dayTicks * gFps;
    s->unk18 = time;
    s->dayFrac = (f32)time / 86400.0f;
    if (Game_IsOnline() == 1) {
        s->unk28 = gTownSeason;
        s->unk2C = gTownDay;
    } else {
        s->unk2C = gSave.day;
        s->unk28 = gSave.season;
    }
    for (i = 0, days = gTownSeasonDays; i < 3; i++, days++) {
        if (*days == 0) {
            *days = 5;
        }
        s->seasonDays[i] = *days;
    }
    s->unk30 = D_008BF0A0;
    *(u32 *)s->unk0 = 0;
    s->lastVsync = 0;
    s->frames = 0;
    s->unk34 = 0.0f;
    s->seconds = 0.0f;
    s->days = 0;
    s->unk48 = 0;
    if (Game_IsOnline() != 1) {
        s->days = gSave.townDays;
    }
    s->day = s->unk2C;
    s->season = s->unk28;
    s->year = s->unk30;
    s->daySeconds = s->unk18;
    s->unk4C = s->dayFrac * s->unk14;
    s->hour = s->daySeconds / 3600.0f;
    if ((u32)s->hour < 12) {
        s->pm = 0;
    } else {
        s->pm = 1;
    }
    gSubVSyncCount = *(u32 *)s->unk0;
    gLobbyTime = s->daySeconds;
    gLobbyCycle2Pos = s->unk4C;
    LobbyWorld_ClearEvents();
    LobbyBgm_End();
}

/* the next day: past the season's last day, the next season (past the
 * third, the next year); the save's (or the server mode's) copies follow */
void LobbyWorld_NextDay(void)
{
    LobbyWorld *s;
    u32 n;

    s = &gLobbyWorld;
    s->days++;
    s->day++;
    n = s->seasonDays[s->season];
    if (s->day >= n) {
        s->day -= n;
        s->season++;
        if ((u32)s->season >= 3) {
            s->season -= 3;
            s->unk48++;
            s->year++;
        }
        if (Game_IsOnline() != 1) {
            gSave.season = s->season;
        } else {
            gTownSeason = s->season;
        }
        LobbyWorld_OnNextDay();
    }
    if (Game_IsOnline() != 1) {
        gSave.day = s->day;
        gSave.townDays = s->days;
    } else {
        gTownDay = s->day;
    }
}

/* advances the world's clock by d days: the seconds, the time of day (the
 * next day at 86400), the hour (VillageClock_Wobble each hour, LobbyWorld_OnAddDays at 0
 * and 12), the second cycle, the day's fraction */
/* advances the world's clock by d days: the seconds, the time of day (the
 * next day at 86400), the hour (VillageClock_Wobble each hour, LobbyWorld_OnAddDays at 0
 * and 12), the second cycle (unk4C, of unk14 per day), the day's fraction */
void LobbyWorld_AddDays(f32 d)
{
    LobbyWorld *s = &gLobbyWorld;
    f32 sec;
    f32 len;
    f32 t;
    s32 h;

    len = 86400.0f;
    sec = d * len;
    s->seconds += sec;
    t = s->daySeconds + sec;
    s->daySeconds = t;
    while (t >= len) {
        s->daySeconds -= len;
        LobbyWorld_NextDay();
        t = s->daySeconds;
    }
    gLobbyTime = t;
    h = s->daySeconds / 3600.0f;
    if (s->hour != h) {
        s->hour = h;
        VillageClock_Wobble();
        if (s->hour == 0) {
            s->pm = 0;
            LobbyWorld_OnAddDays();
        }
        if (s->hour == 12) {
            s->pm = 1;
            LobbyWorld_OnAddDays();
        }
    }
    len = s->unk14;
    sec = d * len;
    s->unk34 += sec;
    s->unk4C += sec;
    if (s->unk4C >= len) {
        s->unk4C -= len;
    }
    gLobbyCycle2Pos = s->unk4C;
    if (Game_IsOnline() != 1) {
        gSave.time = gLobbyTime;
    }
    gLobbyDayFrac = s->dayFrac = s->daySeconds / 86400.0f;
}

/* once a frame: the frames counted since the last call move the clock on
 * (not while gTownDayTicks is 0; skipped once after `hold`), then the events */
void LobbyWorld_Tick(void)
{
    LobbyWorld *s = &gLobbyWorld;
    s32 hold;
    u32 now;
    f32 d;

    hold = s->hold;
    if (hold != 0) {
        s->hold = 0;
    }
    if (gTownDayTicks != 0 && s->lastVsync < (now = gSubVSyncCount)) {
        d = now - s->lastVsync;
        s->lastVsync = now;
        if (hold == 0) {
            s->frames += (u32)d;
            LobbyWorld_AddDays(d / s->dayFrames);
            LobbyWorld_RunEvents();
        }
    }
}

/* t (of gTownDayTicks) as seconds of a day */
u32 LobbyWorld_TicksToSeconds(u32 t)
{
    f32 f;

    if (gTownDayTicks == 0) {
        return 0;
    }
    f = (f32)t / (f32)(u32)gTownDayTicks;
    f *= 86400.0f;
    return f;
}

/* skips n units of time (times sub_main's f32 at 0x6AE050, in frames):
 * the frame counters move on and the clock with them. The vsync counter is
 * a named object here: a `lui %hi(sym)` isn't moved into the `b`'s delay
 * slot where `lui 0x6B` of an absolute address is */
void LobbyWorld_Skip(u32 n)
{
    LobbyWorld *s = &gLobbyWorld;
    u32 fr;
    u32 v;
    u32 f;

    f = n * gFps;
    v = gSubVSyncCount + f;
    fr = s->frames + f;
    gSubVSyncCount = v;
    s->frames = fr;
    s->lastVsync = v;
    LobbyWorld_AddDays(v / s->dayFrames);
}

/* catches the clock up with the time sub_main counted (0x5D4928, in 30ths),
 * then, once after 0x6AE070 was set (not in the server mode): into the
 * next morning when the save's season or day changed meanwhile, else past
 * the next midnight */
void LobbyWorld_CatchUp(void)
{
    u8 day;
    u8 season;
    LobbyWorld *s;

    s = &gLobbyWorld;
    season = gSave.season;
    day = gSave.day;
    LobbyWorld_Skip(D_5D4928 / 30.0f);
    if (Game_IsOnline() != 1 && gSysSettings.unkE0 != 0) {
        gSysSettings.unkE0 = 0;
        if (season != gSave.season || day != gSave.day) {
            if (s->dayFrac > 0.25f) {
                LobbyWorld_AddDays(0.8f);
            }
            return;
        }
        LobbyWorld_AddDays(0.1f + (1.0f - s->dayFrac));
    }
}

s32 LobbyWorld_GetSeason(void) {
    return D_008FAC60;
}

s32 LobbyWorld_GetSeasonDay(void) {
    return D_008FAC5C;
}

void LobbyWorld_HoldClock(void) {
    D_008FAC74 = 1;
}

f32 LobbyWorld_GetDayLen30(void)
{
    return D_008FAC14;
}

f32 LobbyWorld_GetDayFrac(void)
{
    return D_008FAC54;
}

void LobbyWorld_PlayAreaMusic(void) {
    Bgm_StopInArea87();
    Bgm_PlayAreaMusic(gScene.area);
}

/* selects the table entry nearest to position t (-1 past the last) */
static s32 LobbyBgm_NearestEntry(u32 t)
{
    u32 *p;
    s32 r;
    u32 b;
    s32 i;
    LobbyWorld *s;
    s32 n;
    u32 a;
    u32 mid;

    s = &gLobbyWorld;
    p = gLobbyWorld.table;
    r = -1;
    if (t >= p[gLobbyWorld.n - 1] + (p[gLobbyWorld.n] - p[gLobbyWorld.n - 1]) / 2) {
        return r;
    }
    n = s->n;
    for (i = 0; i < n; i++, p++) {
        a = p[0];
        b = p[1];
        mid = a + (b - a) / 2;
        if (t >= a && t < mid) {
            r = i;
            break;
        }
        if (t >= mid && t < b) {
            r = i + 1;
            break;
        }
    }
    if (r >= n || r == -1) {
        return -1;
    }
    s->cur = r;
    return r;
}

s32 LobbyBgm_Cue(s32 track, u32 pos)
{
    LobbyWorld *s = &gLobbyWorld;
    s32 e;

    if (s->active != 0) {
        if (s->unk79 != 3 && s->unk79 != 5) {
            return 0;
        }
        pos = s->pos;
    } else {
        if (track >= 4) {
            track -= 4;
        }
        s->pos = pos;
    }
    s->track = track;
    s->unk84 = pos;
    s->unk7C = gSubVSyncCount;
    s->unk11A = 0;
    s->files = gLobbyBgmFiles[track];
    s->table = gLobbyBgmMarks[track];
    s->n = gLobbyBgmInfo[track].n;
    s->unk94 = D_0081B7A4[track][0];
    if (s->active != 0) {
        if (s->table[s->n - 1] <= s->unk84) {
            LobbyBgm_End();
            s->unk79 = 6;
            return 2;
        }
    } else if (s->table[s->n] <= s->unk84) {
        LobbyBgm_End();
        s->unk79 = 6;
        return 2;
    }
    e = LobbyBgm_NearestEntry(pos);
    if (e < 0 || e >= s->n) {
        LobbyBgm_End();
        s->unk79 = 6;
        return 2;
    }
    s->unk98 = e;
    s->active = 1;
    s->unk11F = 0;
    if (s->active == 0) {
        return 0;
    }
    s->unk79 = 3;
    return 1;
}

/* restarts the stream at the entry nearest to its position (its file,
 * unk9C[entry]); stops it past the last one; 1 if started */
s32 LobbyBgm_Restart(void)
{
    LobbyWorld *s = &gLobbyWorld;
    s32 r;

    if (s->active == 0) {
        return 0;
    }
    r = LobbyBgm_NearestEntry(s->pos);
    if (r >= s->n) {
        LobbyBgm_EndToArea();
        return 0;
    }
    Stream_Prepare(0, s->files[r], 0, 0);
    return 1;
}

s32 LobbyBgm_PollStat(void) {
    s32 stat;

    stat = Stream_GetStat(0);
    D_008FAC8C = stat;
    if (stat == 3) {
        return 1;
    }
    if ((stat < 3) && (stat != 0)) {
        return -1;
    }
    return 0;
}

s32 func_007D8890(void) {
    D_008FAD16 = 0;
    D_008FAC79 = 4;
    return 1;
}

/* stops the stream */
void LobbyBgm_Stop(void)
{
    LobbyWorld *s = &gLobbyWorld;

    if (s->active != 0) {
        s->unk116 = 0;
        Stream_Stop(0);
        s->unk79 = 3;
        s->unk11C = 4;
        if (s->unk11F == 1) {
            CamReq_Cancel();
        }
        s->unk118 = 0;
    }
}

void LobbyBgm_EndToArea(void) {
    D_008FAC78 = 0;
    D_008FAC79 = 0;
    Stream_Stop(0);
    LobbyWorld_ClearFades();
    LobbyWorld_PlayAreaMusic();
    LobbyBgm_ClearState();
}

void LobbyBgm_End(void) {
    D_008FAC78 = 0;
    D_008FAC79 = 0;
    LobbyWorld_ClearFades();
    LobbyBgm_ClearState();
}

void LobbyBgm_StopAll(void) {
    LobbyBgm_Stop();
    Stream_Stop(0);
    D_008FAD18 = 0;
}

void LobbyBgm_Update(void)
{
    u32 now;
    LobbyWorld *s;
    s32 r;
    s32 e;

    now = gSubVSyncCount;
    s = &gLobbyWorld;
    r = LobbyBgm_PollStat();
    switch (s->unk79) {
    case 0:
    case 1:
    case 6:
        break;
    case 2:
        if (r == 0) {
            LobbyBgm_EndToArea();
        }
        break;
    case 3:
        if (gScene.area == 0x87 && r == 1) {
            LobbyBgm_Play(s->track, s->unk84, 0);
        }
        break;
    case 4:
        switch (s->unk116) {
        case 0:
            s->unk116++;
            s->unk119 = 0;
            if (s->unk8C == 3) {
                s->unk119 = 1;
            }
            Stream_FadeOut(0, 0x1E);
            Stream_Stop(1);
            s->unk88 = 0x20;
        case 1:
            if (--s->unk88 == 0 && LobbyBgm_Restart() != 0) {
                if (s->unk119 == 1) {
                    Bgm_ForgetCurrent();
                }
                s->unk116++;
            }
            break;
        case 2:
            if (Stream_IsReady(0) != 0) {
                s->unk116 = 0;
                s->unk7C = now;
                s->unk11A = 1;
                s->unk79 = 2;
                LobbyBgm_SelectEntry(s->cur);
                Stream_StartPrepared(0);
            }
            break;
        }
        break;
    case 5:
        Stream_Stop(0);
        LobbyWorld_PlayAreaMusic();
        s->unk79 = 3;
        break;
    }
    if (s->active != 0) {
        if (s->unk7C < now) {
            s->pos += now - s->unk7C;
            s->unk7C = now;
            e = (u8)LobbyBgm_FindEntry(s->pos);
            if (s->cur < e) {
                s->cur = e;
            }
        }
        if (s->pos >= s->unk94) {
            if (s->unk79 == 4 || s->unk79 == 2) {
                LobbyBgm_EndToArea();
            } else {
                LobbyBgm_End();
            }
        }
    }
}

s32 LobbyBgm_StepPlay(void)
{
    LobbyWorld *s;
    s32 r;
    s32 k;
    u32 pos;

    s = &gLobbyWorld;
    r = 0;
    switch (s->unk11C) {
    case 1:
        s->unk11C++;
        if (s->unk11B == 0) {
            if (LobbyBgm_Cue(s->track, s->unk84) == 2) {
                break;
            }
            func_007D8890();
        } else {
            if (LobbyBgm_Cue(s->track, s->unk84) == 2) {
                break;
            }
            s->unk116 = 0;
            s->unk79 = 3;
            s->unk11C = 4;
            break;
        }
        if (s->active == 0) {
            s->unk11C = 0;
        } else {
            s->unk11D = LobbyBgm_NearestEntry(s->pos);
        }
        break;
    case 2:
        k = LobbyBgm_GetState();
        if (k != 4 && k != 2) {
            LobbyBgm_EndToArea();
            r = 1;
            s->unk11C = 5;
        } else if (k == 2) {
            s->unk11C++;
            s->unk11D = LobbyBgm_GetEntry();
            if (s->actor != NULL) {
                LobbyActor_StartScript(s->actor, s->track, s->unk11D);
            }
            if (s->unk11D == 0) {
                Effect06_Start(0);
                s->unk121 = 1;
            }
            LobbyWorld_StartFades(s->track, s->unk11D);
            goto playing;
        }
        break;
    case 3:
        if (LobbyBgm_IsActive() == 0) {
            s->unk118 = 0;
            if (s->unk121 == 1) {
                Effect06_Start(0);
                s->unk121 = 0;
            }
            LobbyBgm_End();
            r = 1;
            s->unk11C = 5;
            break;
        }
    playing:
        pos = LobbyBgm_GetPos();
        if (s->unk121 < 2 && pos >= 100) {
            s->unk118 = 1;
            s->unk121 = 1;
        }
        if (s->unk121 == 1 && pos >= LobbyBgm_GetEnd() - 100) {
            s->unk121 = 2;
            Effect06_Start(0);
        }
        if (s->unk121 == 2 && pos >= LobbyBgm_GetEnd() - 50 && s->unk11F == 1) {
            CamReq_Cancel();
        }
        pos = LobbyBgm_GetEntry();
        if (s->unk11D != pos) {
            s->unk11D = pos;
            if (s->actor != NULL) {
                LobbyActor_StartScript(s->actor, s->track, s->unk11D);
            }
            if ((s32)pos >= 2 && s->unk11F != 0) {
                CamReq_FromTable(s->track, s->unk11D - 2);
            }
            LobbyWorld_StartFades(s->track, s->unk11D);
        }
        if (s->actor != NULL) {
            LobbyActor_Update(s->actor);
        }
        LobbyWorld_StepFades();
        break;
    case 4:
        if (LobbyBgm_IsActive() == 0) {
            LobbyBgm_End();
            r = 1;
            s->unk11C = 5;
        }
        break;
    case 5:
        if (LobbyBgm_IsActive() != 1) {
            if (s->unk11F == 1) {
                CamReq_Cancel();
            }
            r = 1;
        }
        break;
    case 6:
        if (LobbyBgm_IsActive() != 1) {
            if (s->unk11F == 1) {
                CamReq_Cancel();
            }
            r = 1;
        }
        break;
    }
    if (Debug_IsOn(0) != 0 && Debug_IsOn(0x125) != 0) {
        func_007D9140();
    }
    return r;
}

/* the zero words after LobbyBgm_StepPlay's jump table, up to the end of the
 * overlay's .rodata (0x849F80) */
const u32 D_00849F60[8] = {0};

/* the entry of the stream's table at position t (the last whose next entry
 * is past it), 0 if none */
u8 LobbyBgm_FindEntry(u32 t)
{
    u32 *p;
    s32 i;

    p = gLobbyWorld.table;
    for (i = 0; i < gLobbyWorld.n; i++, p++) {
        if (t < p[1]) {
            return i;
        }
    }
    return 0;
}

s32 LobbyBgm_IsActive(void) {
    return D_008FAC78;
}

s32 LobbyBgm_GetState(void) {
    return D_008FAC79;
}

u32 LobbyBgm_GetPos(void)
{
    LobbyWorld *s = &gLobbyWorld;

    if (s->active == 0) {
        return 0;
    }
    return s->pos;
}

/* the entry selected (LobbyBgm_SelectEntry), or -1 when no stream plays */
s32 LobbyBgm_GetEntry(void)
{
    LobbyWorld *s = &gLobbyWorld;

    if (s->active == 0) {
        return -1;
    }
    return s->cur;
}

s32 LobbyBgm_GetTrack(void)
{
    LobbyWorld *s = &gLobbyWorld;

    if (s->active == 0) {
        return -1;
    }
    return s->track;
}

u8 func_007D9130(void) {
    return D_008FAD18;
}

void func_007D9140(void) {
}

/* selects entry i of the stream's table */
void LobbyBgm_SelectEntry(s32 i)
{
    LobbyWorld *s = &gLobbyWorld;

    if (s->active != 0) {
        u32 *t = s->table;
        s->cur = i;
        t += i;
        s->pos = *t;
    }
}

/* starts fade i with key table k of tbl: the first key from the current
 * frame on sets the fade's colour and its step per frame to the next;
 * 0 (and the fade cleared) when there is none
 * FAKEMATCH: the time slot is addressed as a byte offset cast to u32,
 * *(s16 *)((u8 *)s->fadeTime + (u32)(i * 2)) (the permuter, round 26, with no
 * unset read): s->fadeTime[i], i * sizeof(s16), (u32)i * 2, any copy of i and an
 * s16 *ft local give 16-25 words (MWCC then loads p->time first and every
 * register moves up one); the unsigned offset keeps the game's schedule. Before:
 * tbl reused as the fadeTime pointer (2 words); an unset index local (0, not
 * accepted). Matching it also matches LobbyWorld_StartFades and LobbyWorld_StepFades.
 * Also contrived: the key search tests p->frame >= n twice, as the game compares
 * twice (slt, slt); a while (p->frame < n) loop or one test give 26-82 words.
 */
static s32 LobbyWorld_NextFadeKey(s32 i, s32 k, LobbyFadeKey ***tbl)
{
    LobbyWorld *s = &gLobbyWorld;
    LobbyFadeKey *p;
    s32 j;
    s32 n;
    f32 a[4];
    f32 b[4];

    if (tbl[i] == NULL) {
clear:
        s->fade[i][0] = 0.0f;
        s->fadeStep[i][0] = 0.0f;
        s->fade[i][1] = 0.0f;
        s->fadeStep[i][1] = 0.0f;
        s->fade[i][2] = 0.0f;
        s->fadeStep[i][2] = 0.0f;
        s->fade[i][3] = 0.0f;
        s->fadeStep[i][3] = 0.0f;
        return 0;
    }
    p = tbl[i][k];
    if (p == NULL) {
        goto clear;
    }
    for (;;) {
        n = s->unk10A;
        if (p->frame >= n && p->frame >= n) {
            break;
        }
        p++;
        n = p->frame;
        if (n == -1) {
            return 0;
        }
    }
    *(s16 *)((u8 *)s->fadeTime + (u32)(i * 2)) = p->time;
    for (j = 0; j < 4; j++) {
        a[j] = p->from[j];
        b[j] = p->to[j];
        s->fade[i][j] = a[j];
        if (*(s16 *)((u8 *)s->fadeTime + (u32)(i * 2)) == 0) {
            s->fadeStep[i][j] = 0.0f;
        } else {
            s->fadeStep[i][j] = (b[j] - a[j]) / *(s16 *)((u8 *)s->fadeTime + (u32)(i * 2));
        }
    }
    return 1;
}

/* starts track t's three fades with key set k (all cleared when the track
 * has none); it keeps t3-t6 across the call to the static leaf
 * LobbyWorld_NextFadeKey, so it matches only with that function in C */
void LobbyWorld_StartFades(s32 t, s32 k)
{
    LobbyWorld *s;
    LobbyFadeKey ***tbl;
    s32 i;

    gLobbyWorld.unk10A = 0;
    gLobbyWorld.fadeSet = k;
    s = &gLobbyWorld;
    tbl = gLobbyFadeTables[t];
    gLobbyWorld.fadeTbl = tbl;
    if (tbl == NULL) {
        for (i = 0; i < 3; i++) {
            s->fadeOn[i] = 0;
            s->fade[i][0] = 0.0f;
            s->fadeStep[i][0] = 0.0f;
            s->fade[i][1] = 0.0f;
            s->fadeStep[i][1] = 0.0f;
            s->fade[i][2] = 0.0f;
            s->fadeStep[i][2] = 0.0f;
            s->fade[i][3] = 0.0f;
            s->fadeStep[i][3] = 0.0f;
        }
    } else {
        for (i = 0; i < 3; i++) {
            s->fadeOn[i] = LobbyWorld_NextFadeKey(i, k, tbl);
        }
    }
}

/* steps the fades: a running fade adds its steps until its time runs out,
 * then starts its next key (matches only with LobbyWorld_NextFadeKey in C, as
 * LobbyWorld_StartFades) */
void LobbyWorld_StepFades(void)
{
    LobbyWorld *s = &gLobbyWorld;
    s32 i;
    s32 j;

    gLobbyWorld.unk10A++;
    i = 0;
    for (;;) {
        if (i >= 3) {
            break;
        }
        switch (s->fadeOn[i]) {
        case 0:
            break;
        case 1:
            if (--s->fadeTime[i] <= 0) {
                s->fadeOn[i] = LobbyWorld_NextFadeKey(i, s->fadeSet, s->fadeTbl);
            } else {
                for (j = 0; j < 4; j++) {
                    if (s->fadeStep[i][j] != 0.0f) {
                        s->fade[i][j] += s->fadeStep[i][j];
                    }
                }
            }
            break;
        }
        i++;
    }
    if (s->cur == 1) {
        s->unk110 += 8;
        if (s->unk110 >= 0x100) {
            s->unk110 = 0xFF;
        }
    } else {
        s->unk110 = 0xFF;
    }
}

/* fade i's colour as 0xRRGGBBAA (0 when the stream is idle) */
u32 LobbyWorld_GetFadeColor(s32 i)
{
    LobbyWorld *s = &gLobbyWorld;
    u8 c[4];
    s32 j;

    if (s->active == 0) {
        return 0;
    }
    for (j = 0; j < 4; j++) {
        c[j] = s->fade[i][j];
    }
    return (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3];
}

u8 func_007D96D0(void)
{
    LobbyWorld *s = &gLobbyWorld;

    if (s->active == 0) {
        return 0;
    }
    return s->unk110;
}

/* plays stream id from position pos (below its end) */
void LobbyBgm_Play(s32 id, s32 pos, s32 arg2)
{
    LobbyWorld *s = &gLobbyWorld;

    if (s->active != 0 && s->unk11C == 3) {
        Stream_Stop(0);
        s->active = 0;
        s->unk79 = 0;
    }
    LobbyWorld_ClearFades();
    s->unk121 = 0;
    if (pos < gLobbyBgmMarks[id][gLobbyBgmInfo[id].n]) {
        s->unk11C = 1;
        s->track = id;
        s->unk84 = pos;
        s->unk11B = arg2;
        s->unk120 = s->track % 3;
        if (id == 0 && pos >= 100) {
            s->unk118 = 1;
        }
    }
}

void LobbyBgm_Replay(void)
{
    s32 id;

    if (D_008FAC78 != 0) {
        id = LobbyBgm_GetTrack();
        LobbyBgm_Play(id, LobbyBgm_GetPos(), 0);
    }
}

void LobbyBgm_SetUnk11F(s32 on)
{
    LobbyWorld *s = &gLobbyWorld;

    if (s->active != 0) {
        if (on == 1) {
            s->unk11F = 1;
            CamReq_Mode25();
        } else {
            s->unk11F = 0;
            CamReq_Cancel();
        }
    }
}

s32 func_007D98B0(void) {
    return D_008FAD1F != 0;
}

void LobbyBgm_SetActor(Em *actor) {
    gLobbyWorld.actor = actor;
}

Em *LobbyBgm_GetActor(void) {
    return gLobbyWorld.actor;
}

/* the stream's end: the last entry of its table */
u32 LobbyBgm_GetEnd(void)
{
    return gLobbyBgmMarks[gLobbyWorld.track][gLobbyBgmInfo[gLobbyWorld.track].n];
}

/* 1 when track i, started now, would end before the day does */
s32 LobbyBgm_EndsToday(s32 i)
{
    f32 f;

    f = gLobbyBgmInfo[i].len / gLobbyWorld.dayFrames;
    f += gLobbyWorld.dayFrac;
    return !(f >= 1.0f);
}

/* the stream player's reset */
void LobbyWorld_Reset(void)
{
    LobbyWorld *s = &gLobbyWorld;
    s32 i;

    s->active = 0;
    s->cur = 0;
    s->track = 0;
    s->unk7C = 0;
    s->pos = 0;
    s->unk84 = 0;
    s->unk88 = 0;
    s->unk8C = 0;
    for (i = 0; i < 3; i++) {
        s->fade[i][0] = 0.0f;
        s->fadeStep[i][0] = 0.0f;
        s->fade[i][1] = 0.0f;
        s->fadeStep[i][1] = 0.0f;
        s->fade[i][2] = 0.0f;
        s->fadeStep[i][2] = 0.0f;
        s->fade[i][3] = 0.0f;
        s->fadeStep[i][3] = 0.0f;
        s->fadeTime[i] = 0;
        s->fadeOn[i] = 0;
    }
    s->unk10A = 0;
    s->fadeTbl = NULL;
    s->unk110 = 0;
    s->fadeSet = 0;
    s->unk116 = 0;
    s->unk117 = 0;
    s->unk119 = 0;
    s->unk11A = 0;
    s->unk11B = 0;
    s->unk11C = 0;
    s->unk11D = 0;
    s->unk11E = 0;
    s->unk11F = 0;
    s->unk120 = 0;
    s->movieStep = 0;
}

/* clears the three fades */
void LobbyWorld_ClearFades(void)
{
    LobbyWorld *s = &gLobbyWorld;
    s32 i;

    for (i = 0; i < 3; i++) {
        s->fade[i][0] = 0.0f;
        s->fadeStep[i][0] = 0.0f;
        s->fade[i][1] = 0.0f;
        s->fadeStep[i][1] = 0.0f;
        s->fade[i][2] = 0.0f;
        s->fadeStep[i][2] = 0.0f;
        s->fade[i][3] = 0.0f;
        s->fadeStep[i][3] = 0.0f;
        s->fadeTime[i] = 0;
        s->fadeOn[i] = 0;
    }
    s->unk10A = 0;
    s->fadeTbl = NULL;
    s->unk110 = 0;
    s->fadeSet = 0;
    s->unk117 = 0;
    s->unk11F = 0;
    s->unk120 = 0;
}

void LobbyBgm_Shutdown(void) {
    Stream_Stop(0);
    D_008FAC79 = 0;
    LobbyWorld_ClearFades();
    LobbyBgm_ClearState();
    LobbyWorld_Reset();
}

/* the world's time: the days since the start (the seasons' days times
 * the years, the days of this year's past seasons, the day) in dayTicks,
 * with the time of day */
static u32 LobbyWorld_GetTime(void)
{
    LobbyWorld *s = &gLobbyWorld;
    s32 i;
    u32 t;
    s32 k;

    k = 0;
    for (i = 0; i < 3; i++) {
        k += s->seasonDays[i];
    }
    t = 0;
    t += k * s->year;
    for (i = 0; i < s->season; i++) {
        t += s->seasonDays[i];
    }
    t += s->day;
    t *= s->dayTicks;
    t += (u32)(s->dayTicks * s->dayFrac);
    return t;
}

/* event e's time, counted as LobbyWorld_GetTime counts */
static u32 LobbyWorld_EventTime(LobbyWorldEvent *e)
{
    LobbyWorld *s = &gLobbyWorld;
    s32 i;
    u32 t;
    s32 k;

    k = 0;
    for (i = 0; i < 3; i++) {
        k += s->seasonDays[i];
    }
    t = 0;
    t += k * e->year;
    for (i = 0; i < e->season; i++) {
        t += s->seasonDays[i];
    }
    t += e->day;
    t *= s->dayTicks;
    t += e->tick;
    return t;
}

/* clears the four events */
void LobbyWorld_ClearEvents(void)
{
    LobbyWorldEvent *e;
    s32 i;

    e = gLobbyWorld.events;
    i = 0;
    do {
        memset(e, 0, sizeof(LobbyWorldEvent));
        i++;
        e++;
    } while (i < 4);
}

/* calls the events whose time has come, once */
void LobbyWorld_RunEvents(void)
{
    LobbyWorldEvent *e;
    s32 i;
    u32 t;

    t = LobbyWorld_GetTime();
    e = gLobbyWorld.events;
    for (i = 0; i < 4; i++, e++) {
        if (e->active != 0 && t >= LobbyWorld_EventTime(e)) {
            if (e->func != NULL) {
                e->func();
            }
            e->active = 0;
            e->func = NULL;
        }
    }
}

void LobbyWorld_ScheduleEvents(LobbyWorldEvent *ev, s32 *flags)
{
    LobbyWorld *s = &gLobbyWorld;
    LobbyWorldEvent tmp;
    s32 done[2];
    u32 end;
    LobbyWorldEvent *e;
    u32 t;
    s32 j;
    s32 i;
    u32 mid;
    u32 now;

    t = LobbyWorld_EventTime(ev);
    now = LobbyWorld_EventTime(ev + 1);
    mid = (now - t) * 95 / 100;
    mid += t;
    end = now + (u32)(3.0f * s->dayTicks);
    done[1] = 0;
    done[0] = 0;
    now = LobbyWorld_GetTime();
    if (now >= mid) {
        done[0] = 1;
        flags[0] = 0;
    }
    if (now >= end) {
        done[1] = 1;
        flags[1] = 0;
    }
    for (i = 0; i < 2; i++, ev++, flags++) {
        if (done[i] == 0) {
            tmp = *ev;
            for (e = s->events, j = 0; j < 4; j++, e++) {
                if (e->active == 0) {
                    *e = tmp;
                    e->active = 1;
                    *flags = 1;
                    break;
                }
            }
        }
    }
}

void LobbyMovie_Prepare(s32 arg0)
{
    LobbyBgm_Shutdown();
    Bgm_StopInArea87();
    Stream_Stop(0);
    D_008FAD28 = 0;
    D_008FAD29 = (s8) arg0;
    D_008FAD2A = gMovieLengths[arg0];
    D_008FAD2C = 0;
    EventFlag_Set(arg0 + 0x2E);
}

void LobbyMovie_Stop(void) {
    Movie_RequestStop();
    D_008FAD28 = 2;
}

/* plays movie `movie` + 2 (LobbyMovie_Prepare picks it): 0 start, 1 playing
 * until it ends, the pad's 0x8000 or `movieLen` frames, 2 stop, 3 done;
 * returns the step */
s32 LobbyMovie_Run(void)
{
    LobbyWorld *s = &gLobbyWorld;

    switch (s->movieStep) {
    case 0:
        Movie_Play(s->movie + 2);
        s->movieStep++;
        break;
    case 1:
        Movie_Update();
        if (Movie_GetStatus() == 3 || (gPadState[0].pressed & 0x8000)) {
            Movie_RequestStop();
            s->movieStep++;
        }
        break;
    case 2:
        Movie_Stop();
        s->movieStep++;
        break;
    case 3:
        break;
    }
    if (s->movieStep == 1) {
        s->movieFrames++;
        if (s->movieFrames >= s->movieLen) {
            LobbyMovie_Stop();
        }
    }
    return s->movieStep;
}

void LobbyProp_Spawn(void) {
    Obj *o;

    o = Obj_Alloc(0);
    if (o != NULL) {
        o->kind = 0xF;
        o->update = (void *)LobbyProp_Run;
    }
}

void LobbyProp_Run(LobbyProp *o)
{
    switch (o->state) {
    case 0:
        LobbyProp_Init(o);
        break;
    case 1:
        LobbyProp_Update(o);
        break;
    case 2:
        LobbyProp_End(o);
        break;
    case 3:
        LobbyProp_Free(o);
        break;
    }
}

void LobbyProp_Init(LobbyProp *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->phase = 0;
    o->tilt = 0;
    o->timer = 0;
    switch (gScene.area) {
    case 0xAE:
        Vec_Copy(o->pos, gAreaAEPropPos);
        break;
    case 0xAF:
        Vec_Copy(o->pos, gAreaAFPropPos);
        break;
    default:
        Obj_Free((Obj *)o);
        return;
    }
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->ds = DrawSlot_Get(o->slot);
        o->ds->owner = o;
        o->ds->draw = LobbyProp_Draw;
        LobbyProp_Update(o);
    } else {
        Obj_Free((Obj *)o);
    }
}

void LobbyProp_Update(LobbyProp *o)
{
    s16 i;
    s16 n;
    f32 *pos;

    o->timer++;
    switch (o->phase) {
    case 0:
        if (o->timer > 0x78) {
            o->phase++;
            o->timer = 0;
        }
        break;
    case 1:
        o->tilt -= (s16)(o->timer * 32);
        if ((u16)o->tilt < 0xC000) {
            o->phase++;
            o->tilt = -0x4000;
            o->timer = 0;
            switch (gScene.area) {
            case 0xAE:
                n = 3;
                pos = gAreaAEPropPos;
                break;
            case 0xAF:
                n = 5;
                pos = gAreaAFPropPos;
                break;
            default:
                n = 0;
                break;
            }
            for (i = 0; i < n; i++) {
                Effect0D_SpawnAt(pos, 7, 8.0f);
                pos += 3;
            }
        }
        break;
    case 2:
        if (o->timer > 0x50) {
            o->state++;
            return;
        }
        break;
    }
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 1);
}

void LobbyProp_End(LobbyProp *o) {
    if (o->ds != NULL) {
        DrawSlot_Free(o->slot);
    }
    o->state++;
    o->visible = 0;
}

void LobbyProp_Free(void *o) {
    Obj_Free((Obj *)o);
}

void LobbyProp_Draw(DrawSlot *d)
{
    f32 m[16];
    Model *scene;
    LobbyProp *w;
    ModelMatSet *part;
    s16 i;
    s16 n;
    s16 *idx;
    u16 *rot;
    f32 *pos;

    w = d->owner;
    scene = gAreaSubModels[0];
    if (w->visible != 0 && scene != NULL && scene->active != 0) {
        switch (gScene.area) {
        case 0xAE:
            n = 3;
            idx = gAreaAEModels;
            rot = gAreaAEPropRot;
            pos = gAreaAEPropPos;
            break;
        case 0xAF:
            n = 5;
            idx = gAreaAFModels;
            rot = gAreaAFPropRot;
            pos = gAreaAFPropPos;
            break;
        default:
            return;
        }
        Gs_SetState(0x60, 0x80);
        Gs_SetState(0x67, -1);
        for (i = 0; i < n; i++) {
            part = &scene->matSets[*idx];
            Mtx_Translation(m, pos[0], pos[1], pos[2]);
            if (w->phase == 2) {
                m[13] -= 500.0f * (w->timer / 80.0f);
            }
            Mtx_RotateX(m, 6.2831855f * w->tilt / 65536.0f);
            Mtx_RotateY(m, 6.2831855f * *rot / 65536.0f);
            Gs_SetState(0x1A, (s32)m);
            if (part != NULL && part->id != -1) {
                Draw_ApplyStates(part->flags);
                Model_Draw(part->id, 0);
            }
            idx++;
            pos += 3;
            rot++;
        }
        Draw_SetDefaultState();
    }
}

void LobbyFlock_Spawn(void) {
    Obj *o;

    o = Obj_Alloc(1);
    if (o != NULL) {
        o->kind = 0x18;
        o->type = 0;
        o->update = (void *)LobbyFlock_Run;
    }
}

void LobbyFlock_Run(LobbyFlock *o)
{
    switch (o->state) {
    case 0:
        LobbyFlock_Init(o);
        break;
    case 1:
        LobbyFlock_Update(o);
        break;
    case 2:
        LobbyFlock_End(o);
        break;
    case 3:
        LobbyFlock_Free(o);
        break;
    }
}

void LobbyFlock_ResetPart(LobbyPart *p)
{
    p->vel[0] = 0.0f;
    p->vel[1] = 0.0f;
    p->vel[2] = 0.0f;
    p->rot[0] = Rand_Next(1);
    p->rot[1] = Rand_Next(1);
    p->rot[2] = Rand_Next(1);
    p->spin = 0;
    p->unk16 = 0;
    if (p->draw != NULL) {
        p->draw->pos[0] = 5500.0f + ((u16)Rand_Next(1) & 0x7FF);
        p->draw->pos[1] = 2500.0f;
        p->draw->pos[2] = 10300.0f + ((u16)Rand_Next(1) & 0x3FF);
    }
}

void LobbyFlock_Init(LobbyFlock *o)
{
    LobbyPart *p;
    s16 i;

    p = o->parts;
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->unk5 = (u16)Rand_Next(1) & 3;
    o->pos[0] = 5500.0f + ((u16)Rand_Next(1) & 0x7FF);
    o->pos[1] = 3000.0f;
    o->pos[2] = 10300.0f + ((u16)Rand_Next(1) & 0x3FF);
    o->timer = 0;
    for (i = 0; i < 16; i++, p++) {
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->draw = DrawSlot_Get(p->slot);
            p->draw->owner = o;
            p->draw->part = i;
            p->draw->draw = LobbyActor_DrawPart;
        } else {
            p->draw = NULL;
        }
        LobbyFlock_ResetPart(p);
    }
}

void LobbyFlock_Update(LobbyFlock *o)
{
    f32 v[3];
    f32 m[16];
    LobbyPart *p;
    s16 i;

    i = 0;
    p = o->parts;
    do {
        p->spin = 0.965f * p->spin;
        p->unk16 = 0.975f * p->unk16;
        p->spin += (s16)((Rand_Next(1) & 0xFF) - 0x80);
        p->unk16 += (s16)(((u16)Rand_Next(1) & 0x7F) - 0x40);
        p->rot[0] += p->spin;
        p->rot[1] += (u16)(((u16)Rand_Next(1) & 0x1F) - 0xF);
        p->rot[2] += p->unk16;
        Mtx_Identity(m);
        Mtx_RotateX(m, 6.2831855f * p->rot[0] / 65536.0f);
        Mtx_TurnY(m, 6.2831855f * p->rot[1] / 65536.0f);
        Mtx_TurnZ(m, 6.2831855f * p->rot[2] / 65536.0f);
        p->vel[0] += -4.0f * m[1];
        p->vel[1] += -4.0f * m[5];
        p->vel[2] += -4.0f * m[9];
        p->vel[0] *= 0.64f;
        p->vel[1] *= 0.25f;
        p->vel[2] *= 0.74f;
        Vec_Rotate(v, p->vel, m);
        if (p->draw != NULL) {
            p->draw->pos[0] += v[0];
            p->draw->pos[1] += v[1];
            p->draw->pos[2] += v[2];
            if (p->draw->pos[1] < -30.0f) {
                LobbyFlock_ResetPart(p);
            } else {
                OT_AddByDepth(gDrawOt32, p->draw, 0x20, 0);
            }
        }
        i++;
        p++;
    } while (i < 16);
}

/* an object's end state: its 16 parts' draw slots freed */
void LobbyFlock_End(LobbyFlock *o)
{
    LobbyPart *p;
    s16 i;

    i = 0;
    p = o->parts;
    o->state++;
    o->visible = 0;
    for (; i < 16; i++, p++) {
        if (p->draw != NULL) {
            DrawSlot_Free(p->slot);
        }
    }
}

void LobbyFlock_Free(void *o) {
    Obj_Free((Obj *)o);
}

/* unk_7D44B0's .data (0x81B650-0x81BCA0) */
extern u32 gLobbyFadeTables_0_0_6_0[];
u16 gLobbyBgmFiles_0[8] = {
#include "lobby/0081B650.inc"
};
u16 gLobbyBgmFiles_1[16] = {
#include "lobby/0081B660.inc"
};
u16 gLobbyBgmFiles_2[8] = {
#include "lobby/0081B680.inc"
};
u16 gLobbyBgmFiles_3[16] = {
#include "lobby/0081B690.inc"
};
u16 *gLobbyBgmFiles[4] = {
    gLobbyBgmFiles_0, gLobbyBgmFiles_1, gLobbyBgmFiles_2, gLobbyBgmFiles_3,
};
u32 gLobbyBgmMarks_0[12] = {
#include "lobby/0081B6C0.inc"
};
u32 gLobbyBgmMarks_1[12] = {
#include "lobby/0081B6F0.inc"
};
u32 gLobbyBgmMarks_2[12] = {
#include "lobby/0081B720.inc"
};
u32 gLobbyBgmMarks_3[16] = {
#include "lobby/0081B750.inc"
};
u32 *gLobbyBgmMarks[4] = {
    gLobbyBgmMarks_0, gLobbyBgmMarks_1, gLobbyBgmMarks_2, gLobbyBgmMarks_3,
};
LobbyBgmInfo gLobbyBgmInfo[4] = {
#include "lobby/0081B7A0.inc"
};
u32 gLobbyFadeTables_0_0_2[8] = {
#include "lobby/0081B7C0.inc"
};
u32 gLobbyFadeTables_0_0_3[8] = {
#include "lobby/0081B7E0.inc"
};
u32 gLobbyFadeTables_0_0_5[8] = {
#include "lobby/0081B800.inc"
};
u32 gLobbyFadeTables_0_0_6[12] = {
#include "lobby/0081B820.inc"
};
u32 gLobbyFadeTables_0_0_7[8] = {
#include "lobby/0081B850.inc"
};
u32 gLobbyFadeTables_0_0[8] = {
    0, 0, (u32)gLobbyFadeTables_0_0_2, (u32)gLobbyFadeTables_0_0_3,
    (u32)gLobbyFadeTables_0_0_3, (u32)gLobbyFadeTables_0_0_5, (u32)gLobbyFadeTables_0_0_6, (u32)gLobbyFadeTables_0_0_7,
};
LobbyFadeKey* *gLobbyFadeTables_0[4] = {
    (LobbyFadeKey* *)gLobbyFadeTables_0_0, (LobbyFadeKey* *)gLobbyFadeTables_0_0, (LobbyFadeKey* *)gLobbyFadeTables_0_0, NULL,
};
u32 gLobbyFadeTables_1_0_2[8] = {
#include "lobby/0081B8A0.inc"
};
u32 gLobbyFadeTables_1_0_3[8] = {
#include "lobby/0081B8C0.inc"
};
u32 gLobbyFadeTables_2_0_3[8] = {
#include "lobby/0081B8E0.inc"
};
u32 gLobbyFadeTables_1_0_7[8] = {
#include "lobby/0081B900.inc"
};
u32 gLobbyFadeTables_1_0_8[8] = {
#include "lobby/0081B920.inc"
};
u32 gLobbyFadeTables_1_0_5[16] = {
#include "lobby/0081B940.inc"
};
u32 gLobbyFadeTables_1_0[10] = {
    0, 0, (u32)gLobbyFadeTables_1_0_2, (u32)gLobbyFadeTables_1_0_3,
    (u32)gLobbyFadeTables_1_0_3, (u32)gLobbyFadeTables_1_0_5, (u32)gLobbyFadeTables_1_0_3, (u32)gLobbyFadeTables_1_0_7,
    (u32)gLobbyFadeTables_1_0_8, 0,
};
LobbyFadeKey* *gLobbyFadeTables_1[3] = {
    (LobbyFadeKey* *)gLobbyFadeTables_1_0, (LobbyFadeKey* *)gLobbyFadeTables_1_0, (LobbyFadeKey* *)gLobbyFadeTables_1_0,
};
u32 gLobbyFadeTables_2_0_7[12] = {
#include "lobby/0081B9C0.inc"
};
u32 gLobbyFadeTables_2_0_2[8] = {
#include "lobby/0081B9F0.inc"
};
u32 gLobbyFadeTables_2_0_5[8] = {
#include "lobby/0081BA10.inc"
};
u32 gLobbyFadeTables_2_0_6[8] = {
#include "lobby/0081BA30.inc"
};
u32 gLobbyFadeTables_2_0[8] = {
    0, 0, (u32)gLobbyFadeTables_2_0_2, (u32)gLobbyFadeTables_2_0_3,
    (u32)gLobbyFadeTables_2_0_3, (u32)gLobbyFadeTables_2_0_5, (u32)gLobbyFadeTables_2_0_6, (u32)gLobbyFadeTables_2_0_7,
};
LobbyFadeKey* *gLobbyFadeTables_2[4] = {
    (LobbyFadeKey* *)gLobbyFadeTables_2_0, (LobbyFadeKey* *)gLobbyFadeTables_2_0, (LobbyFadeKey* *)gLobbyFadeTables_2_0, NULL,
};
u32 gLobbyFadeTables_3_0_5[8] = {
#include "lobby/0081BA80.inc"
};
u32 gLobbyFadeTables_3_0_7[8] = {
#include "lobby/0081BAA0.inc"
};
u32 gLobbyFadeTables_3_0_9[8] = {
#include "lobby/0081BAC0.inc"
};
u32 gLobbyFadeTables_3_0_11[12] = {
#include "lobby/0081BAE0.inc"
};
u32 gLobbyFadeTables_3_0_12[8] = {
#include "lobby/0081BB10.inc"
};
u32 gLobbyFadeTables_3_0_6[8] = {
#include "lobby/0081BB30.inc"
};
u32 gLobbyFadeTables_3_0_10[8] = {
#include "lobby/0081BB50.inc"
};
u32 gLobbyFadeTables_3_0_8[8] = {
#include "lobby/0081BB70.inc"
};
u32 gLobbyFadeTables_3_0[14] = {
    0, 0, (u32)gLobbyFadeTables_1_0_2, (u32)gLobbyFadeTables_1_0_3,
    (u32)gLobbyFadeTables_1_0_3, (u32)gLobbyFadeTables_3_0_5, (u32)gLobbyFadeTables_3_0_6, (u32)gLobbyFadeTables_3_0_7,
    (u32)gLobbyFadeTables_3_0_8, (u32)gLobbyFadeTables_3_0_9, (u32)gLobbyFadeTables_3_0_10, (u32)gLobbyFadeTables_3_0_11,
    (u32)gLobbyFadeTables_3_0_12, 0,
};
LobbyFadeKey* *gLobbyFadeTables_3[3] = {
    (LobbyFadeKey* *)gLobbyFadeTables_3_0, (LobbyFadeKey* *)gLobbyFadeTables_3_0, (LobbyFadeKey* *)gLobbyFadeTables_3_0,
};
LobbyFadeKey** *gLobbyFadeTables[4] = {
    gLobbyFadeTables_0, gLobbyFadeTables_1, gLobbyFadeTables_2, gLobbyFadeTables_3,
};
s16 gMovieLengths[8] = {
#include "lobby/0081BBF0.inc"
};
f32 gAreaAEPropPos[10] = {
#include "lobby/0081BC00.inc"
};
u16 gAreaAEPropRot[4] = {
#include "lobby/0081BC28.inc"
};
s16 gAreaAEModels[8] = {
#include "lobby/0081BC30.inc"
};
f32 gAreaAFPropPos[16] = {
#include "lobby/0081BC40.inc"
};
u16 gAreaAFPropRot[8] = {
#include "lobby/0081BC80.inc"
};
s16 gAreaAFModels[8] = {
#include "lobby/0081BC90.inc"
};
