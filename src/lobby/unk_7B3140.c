#include "common.h"
#include "lobby_net.h"
#include "quest.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"

s32 LobbyNet_GetErrText(s8 *);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
s32 HunterCard_Sanitize(void *);
s32 LobbyReq_6139(void *src, s32 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_614C(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_614D(char *dst, s32 arg1, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6190(void *src, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_670F(u8 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyNet_Get614C(struct Net614C *dst);
s32 LobbyNet_Get614D(s32 *dst);
void LobbyStep_6190Done(NetMsg m, NetMsg *pm);
void LobbyStep_614CDone(NetMsg m, NetMsg *pm);
void LobbyStep_614DDone(NetMsg m, NetMsg *pm);
void LobbyStep_670FDone(NetMsg m, NetMsg *pm);
void LobbyStep_6139Done(NetMsg m, NetMsg *pm);
extern u8 gQuestRecord[];
s32 LobbyReq_6884(u8 i, u8 *dst, s32 max, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6892(void (*onReply)(NetMsg, NetMsg *));
void Lobby_Apply6884(void);
void LobbyStep_6884Done(NetMsg m, NetMsg *pm);
void LobbyStep_6892Done(NetMsg m, NetMsg *pm);
void LobbyStep_Task3Done(NetMsg m, NetMsg *pm);
void LobbyStep_Task5Done(NetMsg m, NetMsg *pm);
s32 LobbyNet_Get6884(u8 *);
s32 LobbyNet_Get6892(Net6892 *dst);
void Season_Set(s32 a);
u32 LobbyWorld_TicksToSeconds(u32 t);
void LobbyTime_SetClock(u32 time);
extern u32 D_008BF0A0;
extern u32 gTownDay;
extern u32 gTownSeason;
extern u32 gTownSeasonDays[3];
extern u32 gTownDayTicks;
extern s32 gLobbyTime;
s32 LobbyNet_GetListReplies(u16 *);
s32 LobbyLevel_SelId(s32 level);
s32 LobbyReq_6305(u16 id, char *pass, void (*onReply)(NetMsg, NetMsg *)); /* unit's own declaration: with the definition's s32 (s32 id, char *pass, void (*onReply)(NetMsg, NetMsg *)) (lobby/unk_703F60.c), LobbyStep_6305 1 of 54 words differ */
s32 LobbyReq_630A(u16 id, void (*onReply)(NetMsg, NetMsg *));
void Lobby_SetPlace3928(s32 a);
void LobbyDialog_OpenWait(void *text);
extern u8 gLobbyLevel1Sel;
void LobbyStep_6305Done(NetMsg m, NetMsg *pm);
void LobbyStep_630ADone(NetMsg m, NetMsg *pm);
void LobbyStep_630AUsersDone(NetMsg m, NetMsg *pm);
s32 LobbyNet_GetUser(u16, char *, char *, u8 *);
s32 memcmp(void *a, void *b, s32 n);
extern NetUser gNetUsers[10];
extern u8 gMyUserId[];                   /* the player's id (sub_main) */
void HunterCard_Apply(s8 idx, void *src);
void TownPc_Spawn(u8 pl, struct NetUser *id, char *name);
void Lobby_Disconnect(s32);
extern u8 gLobbyScreenStep;
s32 LobbyReq_6310(s32 id, void (*onReply)(NetMsg, NetMsg *)); /* unit's own declaration: with the definition's s32 (u16 id, void (*onReply)(NetMsg, NetMsg *)) (lobby/unk_704F30.c), LobbyStep_6310 1 of 56 words differ */
void LobbyStep_6310Done(NetMsg m, NetMsg *pm);
void LobbyStep_Task8Done(NetMsg m, NetMsg *pm);
s32 NetTask8_Start(NetEntrySettings *p, void (*onDone)(NetMsg, NetMsg *));
extern char *D_0080D6E4;                /* the default text */
char *strcpy(char *dst, const char *src);
s32 Lobby_HrAtLeast31(void);
void Lobby_ClearHost(void);
s32 LobbyStep_6305(void);
s32 LobbyStep_630A(void);
s32 LobbyReq_630C(s8 v, void (*onReply)(NetMsg, NetMsg *)); /* unit's own declaration: with the definition's s32 (s32 v, void (*onReply)(NetMsg, NetMsg *)) (lobby/unk_706440.c), LobbyStep_630C 7 of 52 words differ */
void LobbyStep_630CDone(NetMsg m, NetMsg *pm);
QuestInfo *Quest_CurrentInfo(void);
s32 NetTask14_Start(u8 file, u8 *buf, s32 size, void (*onDone)(NetMsg, NetMsg *));
void LobbyStep_DownloadDone(NetMsg m, NetMsg *pm);
s32 LobbyReq_6408(void (*onReply)(NetMsg, NetMsg *));
void LobbyStep_6408Done(NetMsg m, NetMsg *pm);
extern s32 gTownMenuOpen;
extern s32 gTownMode;
extern u8 gTownModeStep;
s32 Game_IsOnline(void);
void HunterCard_Build(void *out);
s32 LobbyReq_6192(void *src, u16 n, u16 x, void (*onReply)(NetMsg, NetMsg *));
void func_007B61E0(NetMsg m, NetMsg *pm);
extern u8 gLobbyTextDirty;
extern u8 gChatTextColor;

/* sends 6190 (the player's text); returns 2 while waiting, 0 when done, 1 on a failure */
s32 LobbyStep_6190(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 4;
        HunterCard_Sanitize(gPlayerText.raw);
        LobbyReq_6190(gPlayerText.raw, 0x100, LobbyStep_6190Done);
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

void LobbyStep_6190Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 4) {
            *s = 0;
            if (m.kind == 0) {
                memcpy(gLobby->myText, gPlayerText.raw, 0x100);
                gLobby->seqStep++;
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/* sends 614C */
s32 LobbyStep_614C(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 5;
        LobbyReq_614C(LobbyStep_614CDone);
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

void LobbyStep_614CDone(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 5) {
            *s = 0;
            if (m.kind == 0) {
                gLobby->seqStep++;
                LobbyNet_Get614C((struct Net614C *)gLobby->msg614C);
            } else {
                gLobby->seqStep = 99;
            }
        }
    }
}

/* sends 614D; its reply's text goes to errText */
s32 LobbyStep_614D(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 1;
        memset(gLobby->errText, 0, 0x300);
        LobbyReq_614D(gLobby->errText, 0x300, LobbyStep_614DDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    case 99:
        gLobby->seqStep = 0;
        memset(gLobby->errText, 0, 0x300);
        return 1;
    }
    return 2;
}

void LobbyStep_614DDone(NetMsg m, NetMsg *pm)
{
    s32 v;
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 1) {
            *s = 0;
            if (m.kind == 0) {
                gLobby->seqStep++;
                LobbyNet_Get614D(&v);
            } else {
                gLobby->seqStep = 99;
            }
        }
    }
}

/* sends 670F (0, or 3 when gPlayerText.card.chatColor is 8) */
s32 LobbyStep_670F(void)
{
    HunterCard *t = &gPlayerText.card;
    u8 v;
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        gLobby->awaitReply = 6;
        LobbyMenu_StartTimer();
        switch (t->chatColor) {
        case 8:
            v = 3;
            break;
        default:
            v = 0;
            break;
        }
        LobbyReq_670F(v, LobbyStep_670FDone);
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

void LobbyStep_670FDone(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 6) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/* sends 6139 (0xDC bytes of gQuestRecord) */
s32 LobbyStep_6139(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        gLobby->awaitReply = 9;
        LobbyMenu_StartTimer();
        LobbyReq_6139(gQuestRecord, 0xDC, LobbyStep_6139Done);
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

void LobbyStep_6139Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 9) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = 99;
            }
        }
    }
}

/* a world event to set (LobbyWorldEvent, unk_7D44B0.c: active, the time,
 * the function called then) */
typedef struct NetEvent {
    /* 0x00 */ s32 active;
    /* 0x04 */ s32 year;
    /* 0x08 */ s32 season;
    /* 0x0C */ s32 day;
    /* 0x10 */ u32 tick;
    /* 0x14 */ void (*func)(void);
} NetEvent;
/* the 6884 slot 3 (gElderRaidEvent): two event times, then values ElderRaid_SetState
 * and ElderRaid_CheckChanged test */
typedef struct Net6884Times {
    struct {
        s32 year;
        s32 season;
        s32 day;
        u32 tick;
    } t[2];
} Net6884Times;
extern u16 gLobbyRateValues[];                /* the 6884 slot 1 */
extern Net6884Times gElderRaidEvent;
extern s32 gRateSnapshot[0x4A];              /* sub_main */
void ElderRaid_SetState(void);
void ElderRaid_CheckChanged(void);
struct LobbyWorldEvent;
void LobbyWorld_ScheduleEvents(struct LobbyWorldEvent *ev, s32 *flags);

/* converts the 6884 slots: slot 0's first byte into LobbyState.unk3924,
 * slot 1's 0x4A u16 into sub_main's gRateSnapshot, slot 3's two times into
 * world events (LobbyWorld_ScheduleEvents) that call ElderRaid_SetState and ElderRaid_CheckChanged */
void Lobby_Apply6884(void)
{
    NetEvent ev[2];
    s32 done[2];
    Net6884Times *t;
    u16 *src;
    s16 i;

    gLobby->townEvent = ((u8 *)&gTownQuestLists)[0];
    src = gLobbyRateValues;
    for (i = 0; i < 0x4A; i++) {
        gRateSnapshot[i] = *src++;
    }
    t = (Net6884Times *)&gElderRaidEvent;
    ev[0].year = t->t[0].year;
    ev[0].season = t->t[0].season;
    ev[0].day = t->t[0].day;
    ev[0].tick = t->t[0].tick;
    ev[0].func = ElderRaid_SetState;
    ev[1].year = t->t[1].year;
    ev[1].season = t->t[1].season;
    ev[1].day = t->t[1].day;
    ev[1].tick = t->t[1].tick;
    ev[1].func = ElderRaid_CheckChanged;
    LobbyWorld_ScheduleEvents((struct LobbyWorldEvent *)ev, done);
}

/* fetches the 6884 slots first..last (0x100 bytes each, into gTownQuestLists),
 * then converts them (Lobby_Apply6884) */
s32 LobbyStep_6884(u8 first, u8 last)
{
    s16 *seq;
    LobbyState *p;
    s32 n;
    s16 step;

    p = gLobby;
    seq = &p->seqStep;
    step = *seq;
    switch (step) {
    case 0:
        p->questList = first;
        gLobby->seqStep++;
    case 1:
        gLobby->seqStep++;
        gLobby->awaitReply = 7;
        LobbyMenu_StartTimer();
        memset((u8 *)&gTownQuestLists + (gLobby->questList << 8), 0, 0x100);
        n = gLobby->questList;
        LobbyReq_6884(n, (u8 *)&gTownQuestLists + (n << 8), 0x100, LobbyStep_6884Done);
        break;
    case 2:
        LobbyMenu_TickTimer();
        break;
    case 3:
        n = p->questList + 1;
        p->questList = n;
        if (n <= last) {
            gLobby->seqStep = 1;
            break;
        }
        gLobby->seqStep = 0;
        gLobby->questList = 0;
        Lobby_Apply6884();
        return 0;
    case 99:
        *seq = 0;
        gLobby->questList = 0;
        return 1;
    }
    return 2;
}

/* 6884 reply: the slot byte from LobbyNet_Get6884 and the next seqStep, or
 * 99 with the error text (the slot byte sits in the last byte of m's 8-byte
 * home, as NetMsg is 6 bytes) */
void LobbyStep_6884Done(NetMsg m, NetMsg *pm)
{
    u8 slot = 0;
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 7) {
            *s = 0;
            if (m.kind == 0) {
                LobbyNet_Get6884(&slot);
                gLobby->seqStep++;
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/* sends 6892 */
s32 LobbyStep_6892(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        gLobby->awaitReply = 8;
        LobbyMenu_StartTimer();
        LobbyReq_6892(LobbyStep_6892Done);
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

/* 6892's reply: its values go to D_008BF0A0.. */
void LobbyStep_6892Done(NetMsg m, NetMsg *pm)
{
    Net6892 x;
    u32 *l;
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 8) {
            *s = 0;
            if (m.kind == 0) {
                LobbyNet_Get6892(&x);
                l = x.list;
                gTownSeasonDays[0] = l[0];
                gTownSeasonDays[1] = l[1];
                gTownSeasonDays[2] = l[2];
                D_008BF0A0 = x.unk44;
                gTownSeason = x.season;
                gTownDay = x.day;
                gTownDayTicks = x.dayTicks;
                Season_Set(*(u8 *)&gTownSeason);
                gLobbyTime = LobbyWorld_TicksToSeconds(x.time);
                LobbyTime_SetClock(gLobbyTime);
                gLobby->seqStep++;
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/* a level-1 entry as sub_main keeps them (gLobbyLevel1Entries, 0x164 bytes each) */
extern LevelEntry gLobbyLevel1Entries[];          /* level 1's entries (sub_main copies) */
extern u16 gLobbyLevel1Count;                  /* the number of level-1 entries */
s32 NetTask3_Start(u16 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *)); /* unit's own declaration: with the definition's s32 (s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *)) (lobby/unk_700AF0.c), LobbyStep_Level1List 1 of 202 words differ */
s32 NetTask5_Start(u16 n, void (*onDone)(NetMsg, NetMsg *));
s32 NetList1_GetCount(u16 *n);
s32 NetList1_GetByte(u16 id, u8 *out);
s32 NetList1_Get630D(u16 id, s32 *out);
s32 NetList1_GetUsers(u16 id, u16 *a, u16 *b);
s32 NetList1_GetName(u16 id, char *dst);
s32 NetList1_GetMaxUsers(u16 id, u16 *out);
s32 NetList1_GetText(u16 id, char *dst);
s32 NetList1_Get631B(u16 id, u8 *out);
s32 LobbyNet_GetEntryVals(u16 id, NetEntryVals *out);

/* fetches level 1's entries (task 3), their values (task 5), and copies
 * them into sub_main's gLobbyLevel1Entries: all of them (id 0) or entry id alone;
 * 0 done, 1 failed, 2 busy */
s32 LobbyStep_Level1List(s32 id)
{
    LevelEntry *e;
    u32 i;
    NetEntryVals v;
    s8 *seq = &gLobby->taskStep;

    switch (*seq) {
    case 0:
        (*seq)++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x1D;
        NetTask3_Start(id, 0x28F, LobbyStep_Task3Done);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        if (id == 0) {
            NetList1_GetCount(&gLobbyLevel1Count);
            for (i = 0; i < gLobbyLevel1Count; i++) {
                gLobbyLevel1Entries[i].id = i + 1;
                NetList1_GetByte(i + 1, &gLobbyLevel1Entries[i].state);
                NetList1_Get630D(i + 1, &gLobbyLevel1Entries[i].color);
                NetList1_GetUsers(i + 1, &gLobbyLevel1Entries[i].users, &gLobbyLevel1Entries[i].questUsers);
                NetList1_GetName(i + 1, gLobbyLevel1Entries[i].name);
                NetList1_GetMaxUsers(i + 1, &gLobbyLevel1Entries[i].maxUsers);
                NetList1_GetText(i + 1, gLobbyLevel1Entries[i].text);
                NetList1_Get631B(i + 1, gLobbyLevel1Entries[i].groupInfo);
            }
        } else {
            i = id - 1;
            gLobbyLevel1Entries[i].id = (u16)id;
            e = &gLobbyLevel1Entries[i];
            NetList1_GetByte(id, &e->state);
            NetList1_Get630D(id, &e->color);
            NetList1_GetUsers(id, &e->users, &e->questUsers);
            NetList1_GetName(id, e->name);
            NetList1_GetMaxUsers(id, &e->maxUsers);
            NetList1_GetText(id, e->text);
            NetList1_Get631B(id, e->groupInfo);
        }
        gLobby->taskStep++;
        gLobby->awaitReply = 0x1E;
        NetTask5_Start(id, LobbyStep_Task5Done);
        break;
    case 3:
        LobbyMenu_TickTimer();
        break;
    case 4:
        if (id == 0) {
            for (i = 0; i < gLobbyLevel1Count; i++) {
                LobbyNet_GetEntryVals(i + 1, &v);
                gLobbyLevel1Entries[i].unk19 = v.v[2];
            }
        } else {
            LobbyNet_GetEntryVals(id, &v);
            gLobbyLevel1Entries[id - 1].unk19 = v.v[2];
        }
    gLobby->taskStep = 0;
        return 0;
    case 99:
        *seq = 0;
        return 1;
    default:
        Lobby_Disconnect(5);
        break;
    }
    return 2;
}

/* task 5's progress and end */
void LobbyStep_Task5Done(NetMsg m, NetMsg *pm)
{
    LobbyState *p = gLobby;
    s8 flag = m.flag;
    u8 *s;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 0x1E) {
            if (m.kind == 2) {
                if (flag == 11) {
                    return;
                }
                return;
            }
            if (m.kind == 0) {
                *s = 0;
                gLobby->taskStep++;
                return;
            }
            p->taskStep = 99;
        }
    }
}

/* task 3's progress and end */
void LobbyStep_Task3Done(NetMsg m, NetMsg *pm)
{
    u32 n;
    LobbyState *p = gLobby;
    s8 flag = m.flag;
    u8 *s;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 0x1D) {
            if (m.kind == 2) {
                if (flag == 11) {
                    LobbyNet_GetListReplies((u16 *)&n);
                    return;
                }
                return;
            }
            if (m.kind == 0) {
                *s = 0;
                gLobby->taskStep++;
                return;
            }
            p->taskStep = 99;
        }
    }
}

/* enters level 1's entry (6305, no password) */
s32 LobbyStep_6305(void)
{
    char *pass = "";
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0xF;
        LobbyReq_6305(LobbyLevel_SelId(1), pass, LobbyStep_6305Done);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        return 0;
    case 99:
    default:
        gLobby->subStep = 0;
        return 1;
    }
    return 2;
}

void LobbyStep_6305Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 0xF) {
            *s = 0;
            if (m.kind == 0) {
                gLobby->subStep = 2;
                Lobby_SetPlace3928(1);
            } else {
                gLobby->subStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobbyLevel1Sel = 0;
            }
        }
    }
}

/* 630A: the users of level 1's entry */
s32 LobbyStep_630A(void)
{
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x11;
        LobbyReq_630A(LobbyLevel_SelId(1), LobbyStep_630ADone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        memset(gLobby->memberSlot, 0, 0xA);
        return 0;
    case 99:
    default:
        gLobby->subStep = 0;
        return 1;
    }
    return 2;
}

/* 630A's reply: the entry's users become the members (LobbyState and
 * gNetUsers), each in the slot it had or the first free one */
/* Level 4 (the file stays at 3: LobbyText_UpdateByte2 doesn't match at 4);
 * at level 3, k * 0x11C is computed again after the host block. */
#pragma optimization_level 4
void LobbyStep_630ADone(NetMsg m, NetMsg *pm)
{
    NetUser u;
    s32 i;
    s32 k;
    u8 *tx;
    char *nm;
    s32 j;
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 0x11) {
            *s = 0;
            if (m.kind == 0) {
                i = 0;
                gScene.playerCount = 0;
                tx = u.text;
                nm = u.name;
                do {
                    memset(&u, 0, 0x11C);
                    LobbyNet_GetUser(i, u.id, nm, tx);
                    HunterCard_Sanitize(tx);
                    if (u.id[0] != 0) {
                        for (k = 0; k < 10; k++) {
                            if (gNetUsers[k].id[0] == 0) {
                                break;
                            }
                        }
                        for (j = 0; j < 10; j++) {
                            if (memcmp(u.id, gNetUsers[j].id, 8) == 0) {
                                k = j;
                            }
                        }
                        memset(&gLobby->members[k], 0, 0x11C);
                        memset(&gNetUsers[k], 0, 0x11C);
                        memcpy(gLobby->members[k].id, u.id, 8);
                        memcpy(gLobby->members[k].name, nm, 0x10);
                        memcpy(gLobby->members[k].text, tx, 0x100);
                        memcpy(gNetUsers[k].id, gLobby->members[k].id, 8);
                        memcpy(gNetUsers[k].name, gLobby->members[k].name, 0x10);
                        memcpy(gNetUsers[k].text, gLobby->members[k].text, 0x100);
                        if (memcmp(u.id, gMyUserId, 8) == 0) {
                            gScene.localPlayer = k;
                        }
                        if (gScene.playerCount == 0 || 0 > memcmp(gLobby->hostId, gLobby->members[k].id, 8)) {
                            memset(gLobby->hostId, 0, 8);
                            memcpy(gLobby->hostId, gLobby->members[k].id, 8);
                        }
                        HunterCard_Apply(k, gLobby->members[k].text);
                        (gScene.playerCount)++;
                        TownPc_Spawn(k, (struct NetUser *)gLobby->members[k].id, gLobby->members[k].name);
                    }
                    i++;
                } while (i < 10);
                gLobby->subStep++;
                gLobbyScreenStep = 0;
            } else {
                Lobby_Disconnect(5);
            }
        }
    }
}
#pragma optimization_level reset

void LobbyStep_6310Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 20) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->subStep)++;
            } else {
                gLobby->subStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/* enters level 1's entry id with 6310 */
s32 LobbyStep_6310(s32 id)
{
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x14;
        LobbyReq_6310(id, LobbyStep_6310Done);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        gLobbyLevel1Sel = id;
        return 0;
    case 99:
        gLobby->subStep = 0;
        gLobbyLevel1Sel = 0;
        return 1;
    }
    return 2;
}

void LobbyStep_Task8Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 22) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->subStep)++;
            } else {
                gLobby->subStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/* task 8: sends the settings of the entry the player is in (its text, 4 bytes) */
s32 LobbyStep_Task8(char *text, u8 *vals)
{
    NetEntrySettings st;
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        memset(&st, 0, sizeof(st));
        if (text == NULL) {
            strcpy(st.text, D_0080D6E4);
        } else {
            strcpy(st.text, text);
        }
        if (vals != NULL) {
            memcpy(st.b, vals, 4);
        }
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x16;
        NetTask8_Start(&st, LobbyStep_Task8Done);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        return 0;
    case 99:
        gLobby->subStep = 0;
        return 1;
    }
    return 2;
}

/* may level 1's entry id be entered (its byte 0x1B: 0 always, 1 or 2 by Lobby_HrAtLeast31) */
/* may level 1's entry id be entered (its byte 0x1B: 0 always, 1 or 2 by Lobby_HrAtLeast31) */
s32 Level1_CanEnter(s32 id)
{
    switch (((u8 *)gLobbyLevel1Entries + (id - 1U) * 0x164)[0x1B]) {
    case 0:
        return 1;
    case 1:
        if (Lobby_HrAtLeast31() == 1) {
            return 0;
        }
        break;
    case 2:
        if (Lobby_HrAtLeast31() == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

/* enters level 1's entry (LobbyStep_6305), then fetches its users (LobbyStep_630A) */
s32 LobbyStep_EnterLevel1(void)
{
    switch (gLobby->subStep2) {
    case 0:
        LobbyLevel_SelId(1);
        gLobby->subStep2++;
        Lobby_ClearHost();
    case 1:
        switch (LobbyStep_6305()) {
        case 0:
            gLobby->subStep2++;
            break;
        case 1:
            gLobby->subStep2 = 0;
            return 1;
        }
        break;
    case 2:
        switch (LobbyStep_630A()) {
        case 0:
            gLobby->subStep2 = 0;
            return 0;
        case 1:
            gLobby->subStep2 = 0;
            return 1;
        }
        break;
    }
    return 2;
}

void LobbyStep_630CDone(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 23) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->subStep)++;
            } else {
                gLobby->subStep = 99;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/* sends 630C */
s32 LobbyStep_630C(s32 v)
{
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x17;
        LobbyReq_630C(v, LobbyStep_630CDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        return 0;
    case 99:
        gLobby->subStep = 0;
        return 1;
    }
    return 2;
}

/* fetches the users of level 1's entry (LobbyStep_630A) */
s32 LobbyStep_UsersLevel1(void)
{
    u8 subStep2;

    subStep2 = gLobby->subStep2;
    switch (subStep2) {
    case 0:
        gLobby->subStep2 = (u8)(subStep2 + 1);
        Lobby_ClearHost();
        break;
    case 1:
        switch (LobbyStep_630A()) {
        case 0:
            gLobby->subStep2 = 0;
            return 0;
        case 1:
            gLobby->subStep2 = 0;
            return 1;
        }
        break;
    }
    return 2;
}

/* 630A: the users of level 1's entry id into users[10] */
s32 LobbyStep_630AUsers(NetUser *users, s32 id)
{
    s16 *seq;
    LobbyState *p;
    s32 i;

    p = gLobby;
    seq = &p->seqStep;
    switch (*seq) {
    case 0:
        p->awaitReply = 0x11;
        gLobby->seqStep++;
        memset(users, 0, 0xB18);
        LobbyReq_630A(id, LobbyStep_630AUsersDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        for (i = 0; i < 10; i++) {
            LobbyNet_GetUser(i, users[i].id, users[i].name, users[i].text);
            HunterCard_Sanitize(gLobby->members[i].text);
        }
        gLobby->seqStep = 0;
        return 0;
    case 99:
        *seq = 0;
        Lobby_Disconnect(5);
        return 1;
    }
    return 2;
}

void LobbyStep_630AUsersDone(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 17) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = 99;
            }
        }
    }
}

/* the downloaded quest files (sub_main): the offered files' records as last
 * downloaded (NetDlFile a, b) in the save at *(0x6AE020) + 0x6EF4, file 0
 * (the list) at + 0x20 after them, files 1-3 at gQuestFiles (0x300 bytes each) */
#define QDL_SAVED ((NetDlFile *)(*(s32 *)&gLobbyWorkBuf + 0x6EF4))
extern u8 gQuestFiles[];
s32 NetTask13_Start(void (*onDone)(NetMsg, NetMsg *));
s32 LobbyNet_GetDlFiles(u8 *count, NetDlFile **files);
void Lobby_ApplyServerTimers(void);
void LobbyStep_Task13Done(NetMsg m, NetMsg *pm);

/* downloads the quests the server offers: task 13 fetches the list of files
 * (6883: a record a, b per file), then each file whose record differs from
 * the saved one is fetched (task 14) and its record saved; files the server
 * no longer offers are cleared. Counts the downloaded quests (unk3966) and
 * sets the event flags of the list's word 0x7A9C. 0 done, 1 failed, 2 busy.
 * The compare chains' delay slots are empty in the game: a switch with a
 * non-empty default written first does that for the whole function; the game's
 * is the last one, switch (k) (an outer default-first switch pads its default's
 * bare b to 8 bytes, which the game does not). */
s32 LobbyStep_DownloadQuests(void)
{
    NetDlFile *list;
    u8 n = 0;
    u8 *step = &gLobby->subStep;
    LobbyState *p;
    u8 k;
    u16 f;
    NetDlFile *sv;
    u8 j;

    switch (*step) {
    case 0:
        *step = *step + 1;
        gLobby->dlQuestCount = 0;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x1F;
        NetTask13_Start(LobbyStep_Task13Done);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        LobbyNet_GetDlFiles(&n, &list);
        switch (n) {
        case 0:
            break;
        case 1:
            gLobby->dlQuestsReady = 0;
        default:
            for (k = n; k < 4; k++) {
                memset(QDL_SAVED + k, 0, 8);
                memset(&gQuestFiles[(k - 1) * 0x300], 0, 0x300);
            }
            break;
        }
        gLobby->subStep2 = n;
        gLobby->subStep++;
    case 3:
        if (gLobby->subStep2 != 0) {
            LobbyNet_GetDlFiles(&n, &list);
            p = gLobby;
            j = n - p->subStep2;
            list += j;
            sv = QDL_SAVED + j;
            if (list->a != sv->a || list->b != sv->b) {
                p->subStep++;
                LobbyMenu_StartTimer();
                gLobby->awaitReply = 0x20;
                switch (j) {
                case 0:
                    NetTask14_Start(j, (u8 *)(QDL_SAVED + 4), list->a, LobbyStep_DownloadDone);
                    memcpy(QDL_SAVED + j, list, 8);
                    break;
                default:
                    NetTask14_Start(j, &gQuestFiles[(j - 1) * 0x300], 0x300, LobbyStep_DownloadDone);
                    memcpy(QDL_SAVED + j, list, 8);
                    break;
                }
            } else {
                p->subStep = 5;
            }
            break;
        }
        gLobby->subStep = 0;
        gLobby->subStep2 = 0;
        f = *(u16 *)(gLobbyWorkBuf + 0x7A9C);
        if (f & 1) {
            gSave.recordCounts[40] = 1;
        } else if (f & 2) {
            gSave.recordCounts[41] = 1;
        } else if (f & 4) {
            gSave.recordCounts[42] = 1;
        } else if (f & 8) {
            gSave.recordCounts[43] = 1;
        }
        return 0;
    case 4:
        LobbyMenu_TickTimer();
        break;
    case 5:
        LobbyNet_GetDlFiles(&n, &list);
        p = gLobby;
        k = n - p->subStep2;
        list += k;
        if (k != 0) {
            if (list->a != 0) {
                p->dlQuestsReady = 1;
                gLobby->dlQuestCount++;
            }
        } else {
            Lobby_ApplyServerTimers();
        }
        gLobby->subStep = 3;
        gLobby->subStep2--;
        break;
    case 99:
        *step = 0;
        gLobby->subStep2 = 0;
        LobbyNet_GetDlFiles(&n, &list);
        k = n - gLobby->subStep2;
        list += k;
        switch (k) {
        default:
            /* a debug print? MWCC then leaves every conditional branch slot of the function empty */
            if (k > 4 || n > 4) {} /* compiled-out check */
            break;
        case 0:
            if (QDL_SAVED->a == 0) {
                Lobby_Disconnect(5);
            }
            break;
        }
        return 1;
    }
    return 2;
}

extern u8 *gStageFile;
/* downloads quest file (s8)slot + 1 (task 14, its size from the saved
 * records, into sub_main's *0x6AE024) unless quest id is the current one */
s32 LobbyStep_Download(s32 slot, s32 id)
{
    u8 *step;
    LobbyState *p;
    QuestInfo *q;
    s32 k;

    p = gLobby;
    step = &p->subStep;
    switch (*step) {
    case 0:
        q = Quest_CurrentInfo();
        if (q != NULL && q->id == (u16)id) {
            return 0;
        }
        gLobby->subStep++;
        LobbyMenu_StartTimer();
        k = (s8)slot;
        gLobby->awaitReply = 0x20;
        NetTask14_Start(k + 1, gStageFile, (QDL_SAVED + 1)[k].a, LobbyStep_DownloadDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        *step = 0;
        return 0;
    case 99:
        LobbyNet_GetErrText((s8 *)p->errText);
        LobbyDialog_OpenWait(gLobby->errText);
        gLobby->subStep = 0;
        return 1;
    }
    return 2;
}

void LobbyStep_Task13Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 31) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->subStep)++;
            } else {
                gLobby->subStep = 99;
            }
        }
    }
}

void LobbyStep_DownloadDone(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 32) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->subStep)++;
            } else {
                gLobby->subStep = 99;
            }
        }
    }
}

/* leaves level 1 (6408) */
s32 LobbyStep_6408(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x2A;
        LobbyReq_6408(LobbyStep_6408Done);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    default:
        Lobby_Disconnect(5);
    case 99:
        gLobby->seqStep = 0;
        return 1;
    }
    return 2;
}

void LobbyStep_6408Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 42) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                gLobby->seqStep = 99;
            }
        }
    }
}

/* the third-level entry the player looks at (gLobbyRecs): what may be sent,
 * and task 6's records as the level-2 screens show them (0x14A8 bytes each) */
typedef struct Level2Rec {
    /* 0x000 */ char name[0x41];
    /* 0x041 */ u8 m;                   /* the sub-entries */
    /* 0x042 */ u8 unk42;
    /* 0x043 */ u8 unk43;
    /* 0x044 */ u8 unk44;
    /* 0x045 */ u8 pad45[0x22];
    /* 0x067 */ char subName[0x20][0x41];
    /* 0x887 */ u8 pad887[0xC21];
} Level2Rec;
typedef struct Level2Info {
    /* 0x00 */ u8 sendName;
    /* 0x01 */ u8 sendPass;
    /* 0x02 */ u8 pad2[0x50];
    /* 0x52 */ u8 unk52;                /* the entry's byte +0x17 (gLobbyLevel2Entries) */
    /* 0x53 */ u8 pad53;
    /* 0x54 */ u8 n;                    /* records */
    /* 0x55 */ u8 pad55;
    /* 0x56 */ Level2Rec rec[32];
} Level2Info;
extern Level2Info gLobbyRecs;
s32 NetTask6_Start(u16 id, void (*onDone)(NetMsg, NetMsg *));
s32 NetTask6_GetRecs(u16 id, NetRecs2 *dst); /* unit's own declaration: with the definition's s32 (s32 a, NetRecs2 *dst) (lobby/lobby_net.c), LobbyStep_Level2Entry 1 of 182 words differ */
s32 LobbyReq_640A(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList2_GetUser(u16 i, char *id, char *name, u8 *text);
void LobbyStep_Task6Done(NetMsg m, NetMsg *pm);
void LobbyStep_640ADone(NetMsg m, NetMsg *pm);

/* the third-level entry id + 1: its records (task 6), copied into
 * gLobbyRecs, then its users (640A) into users[4]; 0 done, 2 busy */
s32 LobbyStep_Level2Entry(u16 id, NetUser *users)
{
    NetRecs2 buf;
    s32 i;
    s32 j;
    s32 k;
    s16 *step = &gLobby->seqStep;

    switch (*step) {
    case 0:
        *step = *step + 1;
        memset(&gLobbyRecs, 0, 0x29555);
        gLobbyRecs.unk52 = gLobbyLevel2Entries[id].unk17;
        break;
    case 1:
        *step = *step + 1;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x27;
        NetTask6_Start(id + 1, LobbyStep_Task6Done);
        break;
    case 2:
        LobbyMenu_TickTimer();
        break;
    case 3:
        *step = *step + 1;
        k = id + 1;
        NetTask6_GetRecs(k, &buf);
        gLobbyRecs.sendName = buf.sendName;
        gLobbyRecs.sendPass = buf.sendPass;
        gLobbyRecs.n = buf.n;
        for (i = 0; i < gLobbyRecs.n; i++) {
            strcpy(gLobbyRecs.rec[i].name, buf.rec[i].name);
            gLobbyRecs.rec[i].unk42 = buf.rec[i].unk42;
            gLobbyRecs.rec[i].m = buf.rec[i].m;
            gLobbyRecs.rec[i].unk43 = gLobbyRecs.rec[i].unk44 = buf.rec[i].unk44;
            for (j = 0; j < gLobbyRecs.rec[i].m; j++) {
                strcpy(gLobbyRecs.rec[i].subName[j], buf.rec[i].subName[j]);
            }
        }
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x28;
        LobbyReq_640A(k, LobbyStep_640ADone);
        break;
    case 4:
        LobbyMenu_TickTimer();
        break;
    case 5:
        *step = 0;
        memset(users, 0, 0x470);
        for (i = 0; i < 4; i++) {
            NetList2_GetUser(i, users[i].id, users[i].name, users[i].text);
        }
        return 0;
    case 99:
        *step = 0;
        break;
    }
    return 2;
}

void LobbyStep_Task6Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 0x27 && m.kind != 2) {
            *s = 0;
            if (m.kind == 0) {
                gLobby->seqStep++;
            } else if (m.kind == -1) {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gFieldControl = 0;
                gTownMenuOpen = 1;
                gTownMode = 0x18;
                gLobby->seqStep = 0;
            }
        }
    }
}

void LobbyStep_640ADone(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 0x28) {
            *s = 0;
            if (m.kind == 0) {
                gLobby->seqStep++;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gFieldControl = 0;
                gTownMenuOpen = 1;
                gTownMode = 0x18;
                gLobby->seqStep = 0;
                gTownModeStep = 0;
            }
        }
    }
}

/* sends the player's text (6190) when it changed, and 670F first when its
 * byte 2 changed */
/* sends the player's text (6190) when it changed, and 670F first when its
 * byte 2 changed */
s32 LobbyText_Update(void)
{
    u8 text[0xE0];
    u8 v;
    u8 c;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    HunterCard_Build(text);
    if (memcmp(text, gPlayerText.raw, 0x100) == 0) {
        return 0;
    }
    c = text[2];
    if (c != gPlayerText.card.chatColor) {
        switch (c) {
        case 8:
            v = 3;
            break;
        default:
            v = 0;
            break;
        }
        LobbyReq_670F(v, func_007B61E0);
    }
    memcpy(gLobby->members[gScene.localPlayer].text, text, 0x100);
    memcpy(gLobby->myText, text, 0x100);
    memcpy(gPlayerText.raw, text, 0x100);
    memcpy(gNetUsers[gScene.localPlayer].text, text, 0x100);
    LobbyReq_6190(text, 0x100, func_007B61E0);
    gLobbyTextDirty = 0;
    return 0;
}

/* byte 2 of the player's text from 0x5BFB98 (6192 sends it; 670F first when it
 * changes between 8 and not). The byte goes through a, the first switch's
 * variable (the game reused it). */
/* gChatTextColor is read through a cast of its address (`*(u8 *)&gChatTextColor`, which
 * aliases like a raw address): read by name, the code changes. pt is made by a cast
 * (docs/mwcc-matching.md, "Address arithmetic and base registers"). */
void LobbyText_UpdateByte2(void)
{
    HunterCard *t;
    HunterCard *pt;
    u8 a;
    u8 b;

    t = (HunterCard *)gNetUsers[gScene.localPlayer].text;
    switch (t->chatColor) {
    case 8:
        a = 3;
        break;
    default:
        a = 0;
        break;
    }
    switch (*(u8 *)&gChatTextColor) {
    case 8:
        b = 3;
        break;
    default:
        b = 0;
        break;
    }
    if (a != b) {
        LobbyReq_670F(b, func_007B61E0);
    }
    a = *(u8 *)&gChatTextColor;
    pt = (HunterCard *)gPlayerText.raw;
    t->chatColor = a;
    t = (HunterCard *)gLobby->members[gScene.localPlayer].text;
    t->chatColor = *(u8 *)&gChatTextColor;
    pt->chatColor = *(u8 *)&gChatTextColor;
    LobbyReq_6192(&pt->chatColor, 1, 2, func_007B61E0);
}

/* sends n bytes of the player's text from off (6192) when they changed */
void LobbyText_UpdatePart(u16 off, u16 n)
{
    char text[0xE0];
    u8 *p;
    u32 o;

    if (Game_IsOnline() != 0) {
        HunterCard_Build((u8 *)text);
        HunterCard_Sanitize((u8 *)text);
        o = off;
        p = (u8 *)text;
        if (memcmp(&gNetUsers[gScene.localPlayer].text[o], p + o, n) != 0) {
            memcpy(gLobby->members[gScene.localPlayer].text, p, 0x100);
            memcpy(gLobby->myText, p, 0x100);
            memcpy(gPlayerText.raw, p, 0x100);
            memcpy(gNetUsers[gScene.localPlayer].text, p, 0x100);
            p += o;
            LobbyReq_6192(p, n, off, func_007B61E0);
        }
    }
}
