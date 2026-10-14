#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_globals.h"
#include "scene.h"
#include "player.h"

s32 LobbyNet_GetErrText(s8 *);

/* prototypes */
s32 LobbyLevel_SelId(s32);
s32 NetList1_GetUsers(u16 i, u16 *a, u16 *b);
s32 LobbyNet_Get6319(u32 **arg0);
void LobbyNet_Get631A(NetInfo631A *dst);
void LobbyDialog_OpenWait(void *);
s32 LobbyMenu_TickTimer(void);
void Lobby_SetPlace3928(s32);
void LobbyScreen_ClearRequests(void);
s32 LobbyReq_6319(NetSearch6319 q, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_631A(u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_640A(u16, void (*)(NetMsg, NetMsg *)); /* called with an int: no prototype here */
s32 LobbyReq_6403(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6412(u16 id, void (*onReply)(NetMsg, NetMsg *));
void LobbyLevel1_On6319Reply(NetMsg m, NetMsg *pm);
void LobbyLevel1_On631AReply(NetMsg m, NetMsg *pm);
void LobbyLevel2_On640AReply(NetMsg m, NetMsg *pm);
void LobbyLevel2_On6403Reply(NetMsg m, NetMsg *pm);
void LobbyLevel2_On6412Reply(NetMsg m, NetMsg *pm);
extern u8 gLobbyLevel1Entries[];
extern u8 D_5C41D2[];
extern void *gGroupSearchResults;
extern void *gChosenRoom;
extern u8 gLobbyLevel2Sel;
extern u8 D_008EA0A2[];
void Lobby_Disconnect(s32);
s32 NetList2_GetUser(u16 i, char *id, char *name, u8 *text);
s32 NetList2_GetUsers(u16 id, u16 *a, u16 *b);
s32 NetList2_Get6412(s32 id, u16 *a, u16 *b);
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
void LobbyMenu_StartTimer(void);
s32 LobbyReq_6509(u32 v, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_650B(void *data, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6406(u16 id, char *pass, u8 v, void (*onReply)(NetMsg, NetMsg *));
s32 NetTask4_Start(s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *));
void LobbyLevel2_On6509Reply(NetMsg m, NetMsg *pm);
void LobbyLevel2_On650BReply(NetMsg m, NetMsg *pm);
void LobbyLevel2_On6406Reply(NetMsg m, NetMsg *pm);
void LobbyLevel2_OnList(NetMsg m, NetMsg *pm);
void LobbyLevel2_Refresh(void);
extern u8 gLobbyLevel2Index;
extern char gMyUserId[];
extern char gMyName[];
s32 LobbyNet_GetListReplies(u16 *out);
s32 NetList2_GetCount(u16 *out);
s32 NetList2_GetByte(u16 id, u8 *out);
s32 NetList2_GetText(u16 id, char *dst);
s32 NetList2_Get640B(u16 id, u16 *a, u16 *b, u16 *c, u16 *d, u16 *e);
s32 Net_Get650A(u16 id, s32 *out);
s32 Net_Get650C(u16 id, u8 *dst);
s32 NetList2_Get6405(u16 id, u8 *out);
s32 LobbyReq_6407(u16 id, u8 v, void (*onReply)(NetMsg, NetMsg *));
void LobbyText_Open(s32 msg);
void LobbyLevel2_On6407Reply(NetMsg m, NetMsg *pm);
extern u16 gLobbyLevel2Count;
extern u8 D_008EA204[];

extern LobbyRecs gLobbyRecs;
extern LobbyMenu *gLobbyMenu;
extern char gRoomText[];
extern u16 gLobbyQuestId;             /* the quest */
s32 NetTask6_Start(u16 id, void (*onDone)(NetMsg, NetMsg *));
s32 NetTask7_Start(NetEntrySettings2 s, void (*onDone)(NetMsg, NetMsg *));
s32 NetTask6_GetRecs(u16 id, NetRecs2 *dst); /* unit's own declaration: with the definition's s32 (s32 a, NetRecs2 *dst) (lobby/lobby_net.c), LobbyLevel2_FetchRecords 1 of 163 words differ */
u8 Quest_GetListFlags(u16 id);
u8 Quest_GetStars(u16 id);
void LobbyLevel2_On36Reply(NetMsg m, NetMsg *pm);
void LobbyLevel2_On37Reply(NetMsg m, NetMsg *pm);
char *strcpy(char *dst, const char *src);
s32 LobbyScreen_FadeOut(s32);
s32 LobbyScreen_EnterSeq(s32);
u8 SeqB_GetState(void);
void SeqB_Start(s32);
s32 LobbyTown_WantsMovie(void);
void LobbyTown_StartMovie(void);
s32 LobbyTown_MovieDone(void);
void LobbyScene_Run(void);
extern u8 gLobbyScreenMode;
u16 Pad_GetPressedRepeat(s32 pad);
void EquipPreview_Show(u8 on);
void StageModels_Free(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
s32 LobbyText_Update(void);
s32 LobbyStep_6408(void);
struct Player;
void TownPc_Remove(struct Player *);
void Mem_Fill(void *dst, u8 c, s32 n);
u8 SeqA_GetState(void);
void LobbyVillage_Begin3(s32);
void Text_SetSize(s32 size);
s32 sprintf(char *, const char *, ...);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Lobby_DrawPressButton(void);
void LobbySe_Play(s32 se);
extern u8 gHouseAccess;
extern u8 D_008BEB75;
extern u8 gLobbyTextDirty;
extern NetUser gNetUsers[];
extern u16 gPlayerIds[];               /* sub_main; [0x10] is 0x5CA910 */
/* end of prototypes */

/* the entry to the town's screens (step its step): the menus
 * (LobbyScreen_FadeOut, LobbyScreen_EnterSeq with 4, or 5 when unk0 is set), then
 * LobbyTown_WantsMovie/LobbyTown_StartMovie and LobbyTown_MovieDone, which resets the town's
 * members' slots (unk152E, 0x3989) and copies 0x6ADCC4 to 0x5CA910 */
void LobbyTown_Enter(void)
{
    u8 *step = &gLobby->step;

    switch (*step) {
    case 0:
        (*step)++;
        gLobby->subStep = 0;
        Lobby_SetPlace3928(1);
    case 1:
        switch (LobbyScreen_FadeOut(gLobby->groupJoin ? 5 : 4)) {
        case 0:
            gLobby->step++;
            break;
        }
        break;
    case 2:
        switch (LobbyScreen_EnterSeq(gLobby->groupJoin ? 5 : 4)) {
        case 0:
            gLobby->step++;
            gLobby->subStep = 0;
            gLobbyScreenMode = 2;
            if (gLobby->groupJoin != 0) {
                memset(gLobby, 0, 0x1C);
            }
            if (SeqB_GetState() != 0) {
                SeqB_Start(0xC);
            }
            break;
        }
        break;
    case 3:
        if (LobbyTown_WantsMovie() == 1) {
            gLobby->step = 4;
            LobbyTown_StartMovie();
        } else {
            LobbyScene_Run();
        }
        break;
    case 4:
        if (LobbyTown_MovieDone() == 1) {
            gLobby->movieState = 0;
            gPlayerIds[0x10] = gScene.area;
            memset(gLobby->pcLoaded, 0, 10);
            memset(&gLobby->roomName[0xF], 0, 10);
            gLobby->step++;
        }
        break;
    case 5:
        switch (LobbyScreen_EnterSeq(5)) {
        case 0:
            gLobby->step = 3;
            gLobby->subStep = 0;
            gLobbyScreenMode = 2;
            break;
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
}

/* leaving the town (subStep its step): the stream and the screens stopped
 * (EquipPreview_Show, StageModels_Free), the server's text shown with a 300-frame
 * countdown (countdown, printed as seconds), LobbyText_Update and
 * LobbyStep_6408; then the other players' characters, gNetUsers and the
 * members cleared (all but the player's slot, 0x6ADCEC), LobbyScreen_FadeOut(3)
 * and LobbyVillage_Begin3(1). gLobbyMenu[0x33]: 0 counting down, 1 waiting for
 * the button 0x20 (message 0x11) */
void LobbyTown_Leave(void)
{
    char buf[0x10];
    u16 pad;
    s32 i;

    pad = Pad_GetPressedRepeat(0);
    gLobbyMenu->redraw = 1;
    switch (gLobby->subStep) {
    case 0:
        gLobby->subStep++;
        gLobby->seqStep = 0;
        EquipPreview_Show(1);
        StageModels_Free();
        LobbyMenu_SetDraw(0, 0, 0, 0);
        gLobby->countdown = 300;
        LobbyDialog_OpenWait(gLobby->errText);
        gLobbyMenu->promptStep = 0;
        gHouseAccess = 0;
        D_008BEB75 = 0;
        gPlayers[gScene.localPlayer].questStatus = 0;
        LobbyText_Update();
        gLobbyTextDirty = 0;
        break;
    case 1:
        switch (LobbyStep_6408()) {
        case 0:
            gLobby->subStep++;
            break;
        case 1:
            Lobby_Disconnect(5);
            break;
        }
        break;
    case 2:
        if (gLobbyMenu->promptStep == 2) {
            Lobby_SetPlace3928(0);
            for (i = 0; i < 10; i++) {
                if (gScene.localPlayer != i) {
                    TownPc_Remove(&gPlayers[i]);
                    Mem_Fill(&gPlayers[i], 0, sizeof(Player));
                    Mem_Fill(&gNetUsers[i], 0, 0x11C);
                    Mem_Fill(&gLobby->members[i], 0, 0x11C);
                }
            }
            gLobby->subStep++;
        }
        break;
    case 3:
        switch (LobbyScreen_FadeOut(3)) {
        case 0:
            gLobby->subStep++;
            break;
        }
        break;
    case 4:
        if (SeqA_GetState() != 1) {
            LobbyMenu_SetDraw(0, 0, 0, 0);
            LobbyVillage_Begin3(1);
        }
        break;
    case 99:
        gLobby->subStep = 0;
        Lobby_Disconnect(5);
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
    switch (gLobbyMenu->promptStep) {
    case 0:
        if (--gLobby->countdown == 0) {
            gLobbyMenu->promptStep++;
        } else {
            Text_SetSize(5);
            sprintf(buf, "%3d", gLobby->countdown / 30);
            Text_DrawShadow(0x1D6, 0x54, 1, 5, buf);
        }
        break;
    case 1:
        Lobby_DrawPressButton();
        if (pad & 0x20) {
            gLobbyMenu->promptStep++;
            LobbyText_Open(0x11);
            LobbySe_Play(0);
        }
        break;
    }
}

/* the reply to 6307 (entry i of the first level, LobbyLevel_SelId(1)): its
 * two values into the list (gLobbyLevel1Entries, 0x164-byte records); a failure
 * clears the entry's count (D_5C41D2) */
void LobbyLevel1_On6307Reply(NetMsg m, NetMsg *pm)
{
    s32 i;

    if (gLobby->phase != 5) {
        i = LobbyLevel_SelId(1);
        if (m.kind == 0) {
            NetList1_GetUsers(i, (u16 *)(gLobbyLevel1Entries + (i - 1) * 0x164 + 6), (u16 *)(gLobbyLevel1Entries + (i - 1) * 0x164 + 0x12));
        } else {
            *(s16 *)(D_5C41D2 + i * 0x164) = 0;
        }
    }
}

/* the reply to 6319 (awaited as 24): the search's results (gGroupSearchResults,
 * LobbyNet_Get6319) */
void LobbyLevel1_On6319Reply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 24) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
                LobbyNet_Get6319((u32 **) &gGroupSearchResults);
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/* searches second-level entries (6319); 2 while running, 0 done, 1 failed */
s32 LobbyLevel1_Search(NetSearch6319 *q)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        gLobby->awaitReply = 24;
        LobbyReq_6319(*q, LobbyLevel1_On6319Reply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

/* the reply to 631A (awaited as 25): the entry's details (gChosenRoom,
 * LobbyNet_Get631A) */
void LobbyLevel1_On631AReply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 25) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
                LobbyNet_Get631A((NetInfo631A *) &gChosenRoom);
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
            }
        }
    }
}

/* fetches second-level entry i's details (631A); 2 while running, 0 done,
 * 1 failed */
s32 LobbyLevel1_FetchInfo(u8 i)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        gLobby->awaitReply = 25;
        LobbyReq_631A(i, LobbyLevel1_On631AReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

/* asks for the third-level entry the player is in (gLobbyLevel2Sel): its users
 * (640A), its values (6403) and 6412 */
void LobbyLevel2_Refresh(void)
{
    s32 id;

    gLobby->subStep = 0;
    LobbyScreen_ClearRequests();
    gLobby->roomMembers = 0;
    id = gLobbyLevel2Sel;
    LobbyReq_640A(id, LobbyLevel2_On640AReply);
    LobbyReq_6403(id, LobbyLevel2_On6403Reply);
    LobbyReq_6412(id, LobbyLevel2_On6412Reply);
    Lobby_SetPlace3928(2);
}

/* fetches the third level's list (task 4) after resetting list2 (five
 * entries, ids 1-5); 2 while running, 0 done, 1 failed */
s32 LobbyLevel2_FetchList(void)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        memset(gLobbyLevel2Entries, 0, sizeof(gLobbyLevel2Entries));
        gLobbyLevel2Entries[0].id = 1;
        gLobbyLevel2Entries[1].id = 2;
        gLobbyLevel2Entries[2].id = 3;
        gLobbyLevel2Entries[3].id = 4;
        gLobbyLevel2Entries[4].id = 5;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 34;
        NetTask4_Start(0, 0x1BB, LobbyLevel2_OnList);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    default:
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

/* task 4's onDone (the third level's list, awaited as 34): kind 2 flag
 * 11 is progress (LobbyNet_GetListReplies); done: the list's count (gLobbyLevel2Count,
 * NetList2_GetCount), then each entry's replies copied into list2's records
 * (gLobbyLevel2Entries, 0x164 bytes: id, 6405, the values, 640B, 6418, 650A, the
 * name, 650C into gPostedQuests) */
void LobbyLevel2_OnList(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;
    s32 v;
    s32 i;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 34) {
            if (m.kind == 2) {
                if (m.flag == 11) {
                    LobbyNet_GetListReplies((u16 *)&v);
                    return;
                }
                return;
            }
            if (m.kind == 0) {
                *s = 0;
                gLobby->seqStep++;
                NetList2_GetCount(&gLobbyLevel2Count);
                for (i = 0; i < gLobbyLevel2Count; i++) {
                    gLobbyLevel2Entries[i].id = i + 1;
                    NetList2_GetByte(i + 1, &gLobbyLevel2Entries[i].state);
                    NetList2_GetUsers(i + 1, &gLobbyLevel2Entries[i].users, &gLobbyLevel2Entries[i].questUsers);
                    NetList2_Get640B(i + 1, &gLobbyLevel2Entries[i].maxUsers, &gLobbyLevel2Entries[i].unkA[0], &gLobbyLevel2Entries[i].unkA[1], &gLobbyLevel2Entries[i].unkA[2], &gLobbyLevel2Entries[i].unkA[3]);
                    NetList2_Get6405(i + 1, &gLobbyLevel2Entries[i].unk17);
                    Net_Get650A(i + 1, &gLobbyLevel2Entries[i].color);
                    NetList2_GetText(i + 1, gLobbyLevel2Entries[i].text);
                    Net_Get650C(i + 1, (u8 *)&gPostedQuests[i]);
                }
            } else {
                p->seqStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
            }
        }
    }
}

/* enters the first free third-level entry (6407 with v): the first of
 * list2's five with unk16 == 1 (gLobbyLevel2Sel its id, gLobbyLevel2Index its index);
 * none: message 0x69; 2 while running, 0 entered (a quest taken:
 * questPaid, questTaken), 1 failed */
s32 LobbyLevel2_EnterFree(s32 v)
{
    s32 i;
    u8 *e;

    switch (gLobby->seqStep) {
    case 0:
        for (i = 0, e = (u8 *)gLobbyLevel2Entries; i < 5; i++, e += 0x164) {
            if (e[0x16] == 1) {
                gLobbyLevel2Sel = *(u16 *)(D_008EA204 + i * 0x164);
                gLobbyLevel2Index = i;
                break;
            }
        }
        if (i == 5) {
            LobbyText_Open(0x69);
            gLobby->seqStep = 0;
            return 1;
        }
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 35;
        LobbyReq_6407(LobbyLevel_SelId(2), v, LobbyLevel2_On6407Reply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        gLobby->questPaid = 1;
        gLobby->questTaken = 1;
        return 0;
    default:
    case 99:
        gLobby->seqStep = 0;
        LobbyDialog_OpenWait(gLobby->errText);
        gLobby->questTaken = 0;
        return 1;
    }
    return 2;
}

void LobbyLevel2_On6407Reply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 35) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText((s8 *)((u8 *)gLobby + 0x3608));
                gLobby->seqStep = 99;
            }
        }
    }
}

/* fetches the third-level entry's records (task 6) and copies them from
 * recs2 into gLobbyRecs; 2 while running, 0 done, 1 failed */
s32 LobbyLevel2_FetchRecords(void)
{
    NetRecs2 r;
    s32 id;
    s32 i;
    s32 j;
    s32 k;

    id = LobbyLevel_SelId(2);
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 36;
        NetTask6_Start(id, LobbyLevel2_On36Reply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        gLobby->unk3940 = 0;
        NetTask6_GetRecs(id, &r);
        memset(&gLobbyRecs, 0, sizeof(LobbyRecs));
        gLobbyRecs.sendName = r.sendName;
        gLobbyRecs.sendPass = r.sendPass;
        gLobbyRecs.n = r.n;
        for (i = 0; i < gLobbyRecs.n; i++) {
            strcpy(gLobbyRecs.rec[i].name, r.rec[i].name);
            gLobbyRecs.rec[i].unk43 = r.rec[i].unk42;
            gLobbyRecs.rec[i].m = r.rec[i].m;
            gLobbyRecs.rec[i].unk44 = gLobbyRecs.rec[i].cur = r.rec[i].unk44;
            for (j = 0; j < gLobbyRecs.rec[i].m; j++) {
                strcpy(gLobbyRecs.rec[i].subName[j], r.rec[i].subName[j]);
            }
            for (j = 0; j < 32; j++) {
                gLobbyRecs.rec[i].subN[j] = r.rec[i].subN[j];
                for (k = 0; k < 32; k++) {
                    gLobbyRecs.rec[i].subData[j][k][0] = r.rec[i].subData[j][k][0];
                    gLobbyRecs.rec[i].subData[j][k][1] = r.rec[i].subData[j][k][1];
                    gLobbyRecs.rec[i].subData[j][k][2] = r.rec[i].subData[j][k][2];
                }
            }
        }
        gLobbyMenu->promptStep = 3;
        return 0;
    default:
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

/* the reply awaited as 36 (kind 2: not yet) */
void LobbyLevel2_On36Reply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 36) {
            if (m.kind == 2) {
                return;
            }
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobby->seqStep = 99;
            }
        }
    }
}

/* sends the player's settings for the third-level entry (task 7): the
 * password, the text (gRoomText), the records' chosen values and the
 * quest (gLobbyQuestId: its id, list flags and stars); 2 while running, 0
 * done, 1 failed */
s32 LobbyLevel2_SendSettings(void)
{
    NetEntrySettings2 st;
    s32 i;

    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep = 1;
        memset(&st, 0, sizeof(st));
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 37;
        memcpy(st.pass, gLobbyRecs.pass, 9);
        memcpy(st.text, gRoomText, 0x3D);
        for (i = 0; i < gLobbyRecs.n; i++) {
            st.b[i] = gLobbyRecs.rec[i].cur;
        }
        st.c[0] = gLobbyQuestId;
        st.c[1] = (gLobbyQuestId & 0xFF00) >> 8;
        st.c[2] = Quest_GetListFlags(gLobbyQuestId);
        st.c[3] = Quest_GetStars(gLobbyQuestId);
        NetTask7_Start(st, LobbyLevel2_On37Reply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        LobbyLevel2_Refresh();
        gLobby->seqStep = 0;
        return 0;
    default:
    case 99:
        gLobby->seqStep = 0;
        Lobby_Disconnect(5);
        return 1;
    }
    return 2;
}

/* the reply awaited as 37: a quest was taken (questTaken) */
void LobbyLevel2_On37Reply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 37) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
                gLobby->questTaken = 1;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobby->seqStep = 99;
            }
        }
    }
}

/* sends 6509 (v); 2 while running, 0 done, 1 failed */
s32 LobbyLevel2_Send6509(u32 v)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 45;
        LobbyReq_6509(v, LobbyLevel2_On6509Reply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    default:
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

void LobbyLevel2_On6509Reply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 45) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText((s8 *)((u8 *)gLobby + 0x3608));
                gLobby->seqStep = 99;
            }
        }
    }
}

/* sends 650B: the posting d with the character's id and name; 2 while running,
 * 0 done, 1 failed */
s32 LobbyLevel2_Send650B(Msg650B *d)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        memset(d->userId, 0, 7);
        memset(d->name, 0, 9);
        memcpy(d->userId, gMyUserId, 6);
        memcpy(d->name, gMyName, 8);
        LobbyReq_650B(d, 0x18, LobbyLevel2_On650BReply);
        gLobby->awaitReply = 38;
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    default:
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

/* the reply awaited as 38 */
void LobbyLevel2_On650BReply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 38) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobby->seqStep = 99;
            }
        }
    }
}

/* enters third-level entry gLobbyLevel2Index + 1 with the password D_008C0CA2
 * (6406); 2 while running, 0 when entered (LobbyLevel2_Refresh), 1 failed */
s32 LobbyLevel2_EnterWithPass(s32 v)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 41;
        gLobbyLevel2Sel = gLobbyLevel2Index + 1;
        LobbyReq_6406(LobbyLevel_SelId(2), gLobbyRecs.pass, v, LobbyLevel2_On6406Reply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        LobbyLevel2_Refresh();
        return 0;
    default:
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

/* the reply awaited as 41: a quest was taken (questTaken) */
void LobbyLevel2_On6406Reply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 41) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
                gLobby->questTaken = 1;
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
            }
        }
    }
}

/* the reply to 640A: the entry's four players (players[], 640A's users
 * through NetList2_GetUser) */
void LobbyLevel2_On640AReply(NetMsg m, NetMsg *pm)
{
    s32 i;

    if (gLobby->phase != 5) {
        memset(gLobby->players, 0, sizeof(gLobby->players));
        if (m.kind == 0) {
            for (i = 0; i < 4; i++) {
                NetList2_GetUser(i, gLobby->players[i].id, gLobby->players[i].name, gLobby->players[i].text);
            }
        } else {
            Lobby_Disconnect(5);
        }
    }
}

/* the reply to 6403 (the third-level entry gLobbyLevel2Sel): its values into
 * list2 (gLobbyLevel2Entries are its 0x164-byte records); a failure clears the
 * entry's count (D_008EA0A2) */
void LobbyLevel2_On6403Reply(NetMsg m, NetMsg *pm)
{
    s32 id;

    if (gLobby->phase != 5) {
        id = gLobbyLevel2Sel;
        if (m.kind == 0) {
            NetList2_GetUsers(id, &gLobbyLevel2Entries[id - 1].users, &gLobbyLevel2Entries[id - 1].questUsers);
        } else {
            *(s16 *)(D_008EA0A2 + id * 0x164) = 0;
        }
    }
}

/* the reply to 6412: the entry's two values (unk3930 the first) */
void LobbyLevel2_On6412Reply(NetMsg m, NetMsg *pm)
{
    if (m.kind == 0) {
        NetList2_Get6412(gLobbyLevel2Sel, &gLobby->roomMembers, (u16 *)&m + 3);
    } else {
        gLobby->roomMembers = 0;
    }
}
