#include "common.h"
#include "lobby_menu.h"
#include "net_mode.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_net.h"
#include "scene.h"
#include "player.h"
#include "lobby_globals.h"
#include "save.h"
#include "syssettings.h"
#include "chat_log.h"
s32 LobbyStep_DownloadQuests(void);

/* prototypes */
s32 LobbyMenu_TickTimer(void);
void Lobby_Disconnect(s32);
void Lobby_MarkDisconnected(void);
void LobbyWeb_Run(void);
char *strcpy(void *dst, const void *src);
void SeqA_Start(s32);
extern u8 gLobbyWebUrl;
extern s8 D_008BFAFD;
s32 LobbyNet_GetPatchInfo(NetPatchInfo *info);
s32 LobbyNet_Send6125(void);
void LobbyNetXfer_Reset(void);
s32 LobbyNetXfer_Run(void);
void Lobby_ReleaseNetTextures(void);
void LoadOverlay(s32 id);
void Game_ResetSubsystems(void);
void NetMode_CallDrawScreen(s32 a0);
void Lobby_LoadTextures(void);
void LobbyMenu_StartTimer(void);
extern LobbyMenu *gLobbyMenu;
u16 LobbyPad_Get(s32);
s32 Lobby_NetMenuAllowed(u8 item);
s32 LobbyNet_Send6105(u8 arg0);
void LobbyDialog_OpenCountdown(void *text, s32 arg1);
void LobbyText_Open(s32 msg);
/* end of prototypes */

/* the lobby web page's settings (LobbyNet_Get68B0 fills it): the page's URL,
 * and a text shown for secs seconds before it opens */
typedef struct LobbyWebInfo {
    /* 0x000 */ u8 url[0x100];
    /* 0x100 */ s32 unk100;
    /* 0x104 */ u8 text[0x300];
    /* 0x404 */ u16 secs;
    /* 0x406 */ u16 pad406;
} LobbyWebInfo;

s32 LobbyNet_Get68B0(struct Net68B0 *);
s32 LobbyLogin_SendSummary(void);
void Loading_Request(void);
void HunterCard_Build(void *out);
s32 LobbyStep_6190(void);
void ServerList_ClearTried(void);
extern u8 gNetEntryMode;                   /* the network mode's entry (LobbyLogin_OnProgress) */
extern s16 gFriendCheckTimers[12];

/* a received mail (LobbyNet_Get6705 copies it out) */
typedef struct MailMsg {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x14];
    /* 0x1C */ char text[0x80];
} MailMsg;

extern MailEntry gMailBox[8];
extern u8 gMailBlockList[];                   /* the blocked ids (8 records of 0x18) */
void LobbyNet_Get6705(char *arg0, u8 *arg1, u8 *arg2);
void Chat_FilterWords(char *text);


void LobbyNet_Get6702(struct Net6702 *dst);
void Chat_Receive(u8 *arg0);
s32 strcmp(char *s, char *word);
void Chat_SendText(s32 arg0, void *text, u8 color);
extern u8 gChatClassColors[];
extern char *D_0080D46C;
extern u8 gLobbyLevel1Sel;
s32 LobbyNet_GetUserInOut(NetUser *dst);
s32 Lobby_GetPlace3928(void);
void ActSpot_ClearAll(void);
s32 TownPc_IsSeatedState(u8, u8);
s32 Area_Is86to88(s32 arg0);
s32 Area_IsOnlineHouse(s32 arg0);
void TownMsg_Send0F(void);
void TownMsg_Send18(void);
s32 TownPc_IsInContest(Player *p);
void TownMsg_Send1E(void);
extern s8 gTownSeats;
extern s8 D_008BEB2D;
extern s8 D_008BEB2E;
extern char gSongRequesterId[8];
extern u16 gLobbyQuestId;                 /* the quest */
extern s8 D_008BEB68;
extern s8 D_008BEB69;
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);

/* the login request (0x40 bytes, NetTask0_Start: task 0, the login) */
typedef struct LoginReq {
    /* 0x00 */ u8 unk0;             /* 1 */
    /* 0x01 */ s8 entry;            /* the network mode's entry (sub_main 0x5D46B1) */
    /* 0x02 */ char id[0xB];        /* the account id (sub_main 0x5CAEE1), 10 characters */
    /* 0x0D */ char password[0x13]; /* the password (sub_main 0x5CAED0), 16 characters */
    /* 0x20 */ s32 unk20;           /* sub_main 0x6AE018 */
    /* 0x24 */ u8 pad24[0x1C];
} LoginReq;

void LobbyMenu_SetDraw(void *, void *, void *, void *);
void LobbyMenu_Init(void);
void LobbyEntry_Draw(MenuTask *task);
void LobbyNet_Reset(void);
void LobbyRoute_Register(void);
void Net_GetDeviceState(s16 *a, s16 *b, s16 *c, s16 *d);
char *strncpy(char *dst, const char *src, s32 n);  /* strncpy */
s32 LobbyNet_Set6103(Net6103 *src);
void LobbyTitle_Set(s32 a);
s32 NetTask0_Start(struct NetLoginParams *, void (*onDone)(NetMsg, NetMsg *));
void LobbyLogin_OnProgress(NetMsg m, NetMsg *pm);
extern s32 gLobbyActive;
extern s32 gQuestQNetReadyArg;
extern s8 D_005D46B1;
u8 SeqA_GetState(void);
void LobbyDialog_OpenWait(void *text);
s32 LobbyStep_614C(void);
s32 Lobby_NetMenu2Allowed(u8 item);
u8 LobbyNet_GetCharaCount(void);
s32 LobbyNet_GetCharaId(u8 i, char *dst);
s32 LobbyNet_GetCharaName(u8 i, char *dst);
s32 LobbyNet_GetCharaText(u8 i, u8 *dst);
s32 LobbyStep_614D(void);
u8 SeqB_GetState(void);
void SeqB_Start(s32);
s32 IdMenu_Run(void);                  /* an s8, used as an int */
s32 LobbyNet_SetMe(char *id, char *name, u8 *text);
s32 LobbyNet_GetMyCharaId(char *dst);
s32 LobbyNet_GetMyCharaName(char *dst);
void Player_LoadFromSave(Player *p);
void FileQueue_Flush(void);
void NetMenu_Open(u8);
void Stream_Reset(void);
s32 LobbyNet_Send6141(void);
u8 NetMenu_Update(void);
s32 LobbyStep_6139(void);
s32 LobbyStep_68C0(void);
s32 LobbyStep_670F(void);
s32 LobbyStep_6892(void);
void LobbyScreen_InitLists(void);
s32 LobbyNet_SetSlotBuf(u8 i, char *buf, s32 n);
s32 LobbyStep_6884(u8 first, u8 last);
s32 LobbyWorld_GetSeason(void);
s32 LobbyTime_IsPm(void);
void QuestList_Rebuild(void);
void LobbyEnter_Begin(void);
extern char gVersionString[];
extern char gAccountId[];
extern char gAccountData[];

extern u8 gChatBlockList[];                   /* the blocked ids of the chat (0x14 records) */
s32 IdList_Find(s8 *arg0, s32 arg1, void *key);

s32 LobbyNet_GetErrText(char *);
extern s8 D_1BD421[];                   /* main's character classes (4: a digit) */
extern u8 gChatDigitColors[8];
void LobbyNet_Get6708(struct Net6708 *dst);
void TownMsg_Recv(void *from, u8 *pkt);
void Notice_ResetQueue(void);
extern void (*gLobbyEntrySteps[])(void);
s32 LobbyNet_Get6191(NetUser *dst);
s32 LobbyNet_Get6193(Net6193 *dst);
s32 HunterCard_Sanitize(void *);
s32 memcmp(void *a, void *b, s32 n);
extern NetUser gNetUsers[10];
extern u8 gLobbyLevel2Sel;                   /* the level-2 entry the player is in */
extern s8 gQuestMinMembers;
extern s32 gTownMode;
s32 NetList2_Get6412(u16 id, u16 *a, u16 *b); /* unit's own declaration: with the definition's s32 (s32 id, u16 *a, u16 *b) (lobby/unk_716EC0.c), LobbyRoute_6412 50 of 66 words differ */
void Chat_SystemMsg(s32 msg, char *arg);
void LobbySe_Play(s32 a);

s32 LobbyNet_GetLastId(u16 *);
s32 Net_Get650C(u16, u8 *);
s32 NetList2_GetText(u16 id, char *dst);
s32 NetList2_Get6405(u16, u8 *);
s32 Net_Get650A(u16 id, s32 *out);
s32 NetList2_GetName(u16 id, char *dst);
s32 NetList1_Get630D(u16 id, s32 *out);
extern M2C_UNK gLobbyLevel1Entries;
s32 NetList1_GetByte(u16, u8 *);
s32 NetList1_GetName(u16 id, char *dst);
s32 NetList1_Get631B(u16 id, u8 *out);
s32 NetList1_GetText(u16 id, char *dst);
s32 NetList0_GetByte(u16, u8 *);
extern M2C_UNK gLobbyLevel0Entries;
/* The game's own mismatch: LobbyRoute_6204 passes gLobbyLevel0Entries[id - 1] + 0x1F (the entry's name)
 * where the definition takes the reply handler, which NetReq_OnReply calls: a native build must not call it. */
s32 LobbyReq_6204(u16 id, void *name); /* unit's own declaration: a name pointer, see above */
s32 NetList0_GetUsers(u16 id, u16 *a, u16 *b);
s32 NetList0_GetValue10(u16 id, u16 *a);
s32 NetList1_GetUsers(u16 id, u16 *a, u16 *b);
s32 NetList2_GetByte(u16 id, u8 *dst);
s32 NetList2_GetUsers(u16 id, u16 *a, u16 *b);
s32 NetList2_Get640B(u16 id, u16 *a, u16 *b, u16 *c, u16 *d, u16 *e);

/* route handler: the server's error text while the screen is in mode 4 */
void LobbyRoute_6005(NetMsg m, NetMsg *pm)
{
    u8 mode;
    LobbyState *p = gLobby;

    mode = p->phase;
    if (mode != 5 && mode == 4) {
        LobbyNet_GetErrText(p->errText);
        gLobby->unk3963 = 2;
    }
}

void LobbyRoute_6007(NetMsg m, NetMsg *pm)
{
    LobbyState *p = gLobby;

    if (p->phase != 5) {
        p->serverClose = 2;
        LobbyNet_GetErrText(gLobby->errText);
    }
}

/* the digit c ('0'-'7') as an index into gChatDigitColors, 0xFF for anything else */
static u8 ChatMsg_ColorDigit(u8 c)
{
    if (c == 0 || !(D_1BD421[c] & 4)) {
        c = 0xFF;
    } else {
        c = gChatDigitColors[(c - '0') & 7];
    }
    return c;
}

void LobbyRoute_6702(NetMsg msg, NetMsg *pm)
{
    ChatMsg m;
    u8 *b;
    u8 *c;
    u8 *k;
    u8 *a;

    memset(&m, 0, sizeof(m));
    LobbyNet_Get6702((struct Net6702 *)&m);
    k = &m.kind;
    if (*k == 1 && IdList_Find((s8 *) gChatBlockList, 0x14, &m) != -1) {
        return;
    }
    a = &m.color[0];
    *a = ChatMsg_ColorDigit(*a);
    b = &m.color[1];
    *b = ChatMsg_ColorDigit(*b);
    c = &m.color[2];
    *c = ChatMsg_ColorDigit(*c);
    if (*a == 0xFF) {
        *a = 0;
        if (*k != 1 && gLobbyLevel1Sel != 0) {
            if ((gLobby->step == 4 || gLobby->step == 5) && gLobby->phase == 3 &&
                strcmp(m.text, D_0080D46C) != 0) {
                Chat_SendText(gScene.localPlayer, D_0080D46C, 0);
            }
        }
    } else if (*k == 1) {
        *a = 0xFF;
    }
    if (*b == 0xFF) {
        *b = 0;
    }
    if (*c == 0xFF) {
        *c = 0;
    }
    Chat_Receive((u8 *) &m);
}

void LobbyRoute_670C(NetMsg msg, NetMsg *pm)
{
    ChatMsg m;
    u8 *a;
    u8 *b;
    u8 *c;

    memset(&m, 0, sizeof(m));
    LobbyNet_Get6702((struct Net6702 *)&m);
    a = &m.color[0];
    *a = ChatMsg_ColorDigit(*a);
    b = &m.color[1];
    *b = ChatMsg_ColorDigit(*b);
    c = &m.color[2];
    *c = ChatMsg_ColorDigit(*c);
    if (*a == 0xFF) {
        *a = 0;
    }
    if (*b == 0xFF) {
        *b = 0;
    }
    if (*c == 0xFF) {
        *c = gChatClassColors[m.cls];
    }
    if (m.kind != 1 && gLobbyLevel1Sel != 0) {
        if ((gLobby->step == 4 || gLobby->step == 5) && gLobby->phase == 3 &&
            strcmp(m.text, D_0080D46C) != 0) {
            Chat_SendText(gScene.localPlayer, D_0080D46C, 0);
        }
    }
    Chat_Receive((u8 *) &m);
}

/* route handler: a mail received. Ignored when its sender's id is in the
 * blocked list (gMailBlockList); else the mail box (gMailBox, 8 mails, newest
 * first) moves down one and the mail goes first, its text filtered
 * (Chat_FilterWords); LobbySe_Play(7) (the new-mail notice) when the box
 * was empty and the player's screen allows it. */
void LobbyRoute_6705(NetMsg msg, NetMsg *pm)
{
    s32 j;
    s32 n;
    MailMsg m;
    s32 i;
    u8 *b;
    MailEntry *e;
    LobbyState *p;
    s32 mode;

    memset(&m, 0, sizeof(m));
    LobbyNet_Get6705(m.id, (u8 *) m.name, (u8 *) m.text);
    for (i = 0, b = gMailBlockList; i < 8; i++, b += 0x18) {
        if (memcmp(b, m.id, 8) == 0) {
            return;
        }
    }
    n = 0;
    for (i = 0, e = gMailBox; i < 8; i++, e++) {
        if (e->used != 0) {
            n++;
        }
    }
    p = gLobby;
    if (p->modelLoaded[gScene.localPlayer] != 0) {
        mode = p->phase;
        if ((mode != 3 || p->step != 4) && (mode != 2 || p->mode != 6) && n == 0) {
            LobbySe_Play(7);
        }
    }
    for (n = 6, j = 7; j > 0; n--, j--) {
        memcpy(&gMailBox[j], &gMailBox[n], sizeof(MailEntry));
    }
    memset(&gMailBox[0], 0, sizeof(MailEntry));
    memcpy(gMailBox[0].id, m.id, 8);
    memcpy(gMailBox[0].name, m.name, 0x10);
    memcpy(gMailBox[0].text, m.text, 0x78);
    Chat_FilterWords(gMailBox[0].text);
    gMailBox[0].used = 1;
}

void LobbyRoute_6204(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    LobbyReq_6204(id, (u8 *)&gLobbyLevel0Entries + ((id - 1) * 0x164) + 0x1F);
}

void LobbyRoute_6205(NetMsg unused0, NetMsg *unused1)
{
    u16 id;
    u8 *e;

    LobbyNet_GetLastId(&id);
    e = (u8 *)&gLobbyLevel0Entries + ((id - 1) * 0x164);
    NetList0_GetUsers(id, (u16 *)(e + 6), (u16 *)(e + 0x12));
    NetList0_GetValue10(id, (u16 *)((u8 *)&gLobbyLevel0Entries + ((id - 1) * 0x164) + 0x14));
}

void LobbyRoute_6206(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList0_GetByte(id, (u8 *)&gLobbyLevel0Entries + ((id - 1) * 0x164) + 0x16);
}

void LobbyRoute_6303(NetMsg unused0, NetMsg *unused1)
{
    u16 id;
    u8 *e;

    LobbyNet_GetLastId(&id);
    e = (u8 *)&gLobbyLevel1Entries + ((id - 1) * 0x164);
    NetList1_GetUsers(id, (u16 *)(e + 6), (u16 *)(e + 0x12));
}

void LobbyRoute_6308(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList1_GetText(id, (char *) ((u8 *)&gLobbyLevel1Entries + ((id - 1) * 0x164) + 0x60));
}

void LobbyRoute_631B(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList1_Get631B(id, (u8 *)&gLobbyLevel1Entries + ((id - 1) * 0x164) + 0x1B);
}

void LobbyRoute_6302(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList1_GetName(id, (char *) ((u8 *)&gLobbyLevel1Entries + ((id - 1) * 0x164) + 0x1F));
}

void LobbyRoute_6304(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList1_GetByte(id, (u8 *)&gLobbyLevel1Entries + ((id - 1) * 0x164) + 0x16);
}

void LobbyRoute_630D(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList1_Get630D(id, (s32 *) ((u8 *)&gLobbyLevel1Entries + ((id - 1) * 0x164)));
}

void LobbyRoute_UserIn(NetMsg m, NetMsg *pm)
{
    NetUser u;
    s32 i;
    NetUser *e;

    memset(&u, 0, sizeof(u));
    LobbyNet_GetUserInOut(&u);
    if (gLobby->hostId[0] == 0 || 0 > memcmp(gLobby->hostId, u.id, 8)) {
        memset(gLobby->hostId, 0, 8);
        memcpy(gLobby->hostId, u.id, 8);
    }
    for (i = 0, e = gNetUsers; i < 10; i++, e++) {
        if (memcmp(e, u.id, 8) == 0) {
            return;
        }
    }
    for (i = 0, e = gNetUsers; i < 10; i++, e++) {
        if (e->id[0] == 0) {
            memcpy(&gNetUsers[i], &u, sizeof(NetUser));
            break;
        }
    }
    if (Lobby_GetPlace3928() != 0 && u.name[0] != 0) {
        if (gChatHideNames != 0) {
            Chat_SystemMsg(2, u.id);
        } else {
            Chat_SystemMsg(2, u.name);
        }
    }
}

void LobbyRoute_UserOut(NetMsg m, NetMsg *pm)
{
    NetUser u;
    s32 i;
    s32 found;
    Player *me;
    NetUser *e;
    Player *pl;
    NetUser *f;
    NetUser *g;
    s32 j;

    LobbyNet_GetUserInOut(&u);
    found = -1;
    me = &gPlayers[gScene.localPlayer];
    for (i = 0, e = gNetUsers; i < 10; i++, e++) {
        if (memcmp(e, u.id, 8) == 0) {
            if (Lobby_GetPlace3928() != 0) {
                f = &gNetUsers[i];
                if (f->id[0] != 0) {
                    if (gChatHideNames != 0) {
                        Chat_SystemMsg(8, f->id);
                    } else {
                        Chat_SystemMsg(8, f->name);
                    }
                }
            }
            found = i;
            pl = &gPlayers[i];
            pl->pos[0] = pl->pos[1] = pl->pos[2] = 0.0f;
            pl->moveTo[0] = pl->moveTo[1] = pl->moveTo[2] = 0.0f;
            pl->rot[1] = pl->targetYaw = 0;
            ActSpot_ClearAll();
            gLobby->readyBits = 0;
            gLobby->memberSlot[i] = 0;
            memset(gLobby->slotIds, 0, 0x80);
            if (TownPc_IsSeatedState(pl->stateGroup, pl->state) == 1) {
                if (Area_Is86to88(gScene.area) == 1) {
                    TownMsg_Send0F();
                } else if (Area_IsOnlineHouse(gScene.area) == 1) {
                    TownMsg_Send18();
                }
            }
            gTownSeats = D_008BEB2D = D_008BEB2E = 0;
            if (TownPc_IsInContest((Player *)(&gPlayers[gScene.localPlayer].active)) == 1) {
                TownMsg_Send1E();
            }
            if (me->target != NULL && me->target == pl) {
                me->target = NULL;
            }
            break;
        }
    }
    if (memcmp(gLobby->hostId, u.id, 8) == 0) {
        gLobby->hostId[0] = 0;
        for (j = 0, g = gNetUsers; j < 10; j++, g++) {
            if (g->id[0] != 0) {
                if (gLobby->hostId[0] == 0 || 0 > memcmp(gLobby->hostId, g->id, 8)) {
                    memset(gLobby->hostId, 0, 8);
                    memcpy(gLobby->hostId, g->id, 8);
                }
            }
        }
    }
    if (found >= 0) {
        memset(&gNetUsers[found], 0, sizeof(NetUser));
    }
    if (memcmp(gSongRequesterId, u.id, 8) == 0) {
        memset(gSongRequesterId, 0, 8);
    }
}

void LobbyRoute_6402(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList2_GetName(id, gLobbyLevel2Entries[id - 1].name);
}

void LobbyRoute_6404(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList2_GetByte(id, &gLobbyLevel2Entries[id - 1].state);
    if (gLobbyLevel2Entries[id - 1].state == 1) {
        memset(&gLobbyLevel2Entries[id - 1], 0, sizeof(LevelEntry));
        memset(&gPostedQuests[id - 1], 0, sizeof(PostedQuest));
    }
}

void LobbyRoute_650A(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    Net_Get650A(id, &gLobbyLevel2Entries[id - 1].color);
}

void LobbyRoute_6403(NetMsg unused0, NetMsg *unused1)
{
    u16 id;
    LevelEntry *e;

    LobbyNet_GetLastId(&id);
    e = &gLobbyLevel2Entries[id - 1];
    NetList2_GetUsers(id, &e->users, &e->questUsers);
}

void LobbyRoute_640B(NetMsg unused0, NetMsg *unused1)
{
    u16 id;
    LevelEntry *e;

    LobbyNet_GetLastId(&id);
    e = &gLobbyLevel2Entries[id - 1];
    NetList2_Get640B(id, &e->maxUsers, &e->unkA[0], &e->unkA[1], &e->unkA[2], &e->unkA[3]);
}

void LobbyRoute_6405(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList2_Get6405(id, &gLobbyLevel2Entries[id - 1].unk17);
}

void LobbyRoute_640D(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList2_GetText(id, gLobbyLevel2Entries[id - 1].text);
}

void LobbyRoute_650C(NetMsg unused0, NetMsg *unused1)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    Net_Get650C(id, (u8 *)&gPostedQuests[id - 1]);
}

/* route handler (6412/6506): the entry's two values into unk3930 */
/* route handler (6412/6506): the entry's two values into unk3930 */
void LobbyRoute_6412(NetMsg m, NetMsg *pm)
{
    s32 id = gLobbyLevel2Sel;
    u16 v;
    s32 last;

    last = 0;
    if (gLobby->roomMembers + 1 == gQuestMinMembers + 1) {
        last = 1;
    }
    NetList2_Get6412(id, &gLobby->roomMembers, &v);
    if (gTownMode != 6 && gQuestMinMembers + 1 > 1 && gLobby->roomMembers == gQuestMinMembers &&
        gLobby->questPaid != 0) {
        Chat_SystemMsg(0, NULL);
        LobbySe_Play(0x14);
    }
    if (last == 1 && gQuestMinMembers + 1 > 1 && gQuestMinMembers + 1 > gLobby->roomMembers + 1 &&
        gLobby->questPaid != 0) {
        Chat_SystemMsg(1, NULL);
    }
}

void LobbyRoute_In36(NetMsg m, NetMsg *pm)
{
    NetUser u;
    s32 i;
    u8 *q;
    s32 k;

    memset(&u, 0, sizeof(u));
    LobbyNet_GetUserInOut(&u);
    for (i = 0, q = (u8 *)gLobby; i < 4; i++, q += sizeof(NetUser)) {
        if (((LobbyState *)q)->players[0].id[0] == 0) {
            k = i * 0x11C;
            memset((u8 *)gLobby + k + 0x5A6, 0, sizeof(NetUser));
            memcpy((u8 *)gLobby + k + 0x5A6, u.id, 8);
            memcpy((u8 *)gLobby + k + 0x5AE, u.name, 0x10);
            memcpy((u8 *)gLobby + k + 0x5C2, u.text, 0x100);
            break;
        }
    }
    if (Lobby_GetPlace3928() != 0 && u.name[0] != 0) {
        if (gChatHideNames != 0) {
            Chat_SystemMsg(4, u.id);
        } else {
            Chat_SystemMsg(4, u.name);
        }
    }
    if (gLobby->questPaid != 0 && gLobbyQuestId == 0x61A9) {
        D_008BEB68 = 1;
        D_008BEB69 = 0x2C;
    }
    Sound_PlaySeFlat(1, 0x94, 0);
}

void LobbyRoute_Out37(NetMsg m, NetMsg *pm)
{
    NetUser u;
    s32 found;
    s32 i;

    memset(&u, 0, sizeof(u));
    LobbyNet_GetUserInOut(&u);
    for (i = 0; i < 4; i++) {
        if (gLobby->playersListed == 0 && memcmp(gLobby->players[i].id, u.id, 8) == 0) {
            if (Lobby_GetPlace3928() != 0 && gLobby->players[i].name[0] != 0) {
                if (gChatHideNames != 0) {
                    Chat_SystemMsg(5, gLobby->players[i].id);
                } else {
                    Chat_SystemMsg(5, gLobby->players[i].name);
                }
            }
            memset(&gLobby->players[i], 0, sizeof(NetUser));
            found = i;
        }
    }
    for (i = found, found++; i < 4; i++, found++) {
        if (found >= 4) {
            memset(&gLobby->players[i], 0, sizeof(NetUser));
        } else {
            memcpy(&gLobby->players[i], &gLobby->players[found], sizeof(NetUser));
        }
    }
}

/* route handler: a chat message (6708) to the message window */
void LobbyRoute_Chat(NetMsg m, NetMsg *pm)
{
    struct {
        char id[8];
        char text[0x300];
    } msg;
    u8 mode;
    LobbyState *p = gLobby;

    mode = p->phase;
    if (mode != 5) {
        if (mode == 4 || p->playersListed != 0) {
            return;
        }
        LobbyNet_Get6708((struct Net6708 *)&msg);
        TownMsg_Recv(msg.id, (u8 *) msg.text);
    }
}

void LobbyRoute_6307(NetMsg m, NetMsg *pm)
{
    s8 *s;
    u8 mode;
    LobbyState *p = gLobby;

    mode = p->phase;
    if (mode != 5 && mode == 2) {
        s = &p->leave6307;
        if (*s == 0) {
            *s = 1;
            LobbyNet_GetErrText(gLobby->errText);
        }
    }
}

void LobbyRoute_640C(NetMsg m, NetMsg *pm)
{
    s8 *s;
    u8 mode;
    LobbyState *p = gLobby;

    mode = p->phase;
    if (mode != 5 && mode == 3) {
        s = &p->leave640C;
        if (*s == 0) {
            *s = 1;
            LobbyNet_GetErrText(gLobby->errText);
        }
    }
}

void LobbyRoute_6505(NetMsg m, NetMsg *pm)
{
    s8 *s;
    u8 mode;
    LobbyState *p = gLobby;

    mode = p->phase;
    if (mode != 5 && mode == 3 && p->playersListed == 0) {
        s = &p->leave6505;
        if (*s == 0) {
            *s = 1;
            Notice_ResetQueue();
            LobbyNet_GetErrText(gLobby->errText);
        }
    }
}

/* route handler (6191): a member's new text */
void LobbyRoute_6191(NetMsg m, NetMsg *pm)
{
    NetUser u;
    s8 i;
    NetUser *e;
    u8 *t;

    LobbyNet_Get6191(&u);
    t = u.text;
    HunterCard_Sanitize(t);
    for (i = 0, e = gNetUsers; i < 10; i++, e++) {
        if (memcmp(e, u.id, 8) == 0) {
            memcpy(gNetUsers[i].text, t, 0x100);
            return;
        }
    }
}

/* route handler (6193): n bytes of a member's text at an offset */
void LobbyRoute_6193(NetMsg m, NetMsg *pm)
{
    Net6193 x;
    u8 *t;
    s8 i;
    NetUser *e;

    LobbyNet_Get6193(&x);
    for (i = 0, e = gNetUsers; i < 10; i++, e++) {
        if (memcmp(e, x.id, 8) == 0) {
            t = gNetUsers[i].text;
            memcpy(t + x.x, x.data, x.n);
            HunterCard_Sanitize(t);
            return;
        }
    }
}

/* the screen's step (gLobbyEntrySteps[mode], 10 steps) */
void LobbyEntry_Run(void)
{
    if (gLobby->mode >= 10) {
        Lobby_Disconnect(5);
    }
    gLobbyEntrySteps[gLobby->mode]();
}

void LobbyEntry_Init(void)
{
    LoginReq req;
    Net6103 dev;
    s16 d0;
    s16 d1;
    s16 d2;
    s16 d3;
    u8 *seq;

    seq = &gLobby->step;
    switch (*seq) {
    case 0:
        Game_ResetSubsystems();
        LobbyMenu_Init();
        LobbyMenu_SetDraw(LobbyEntry_Draw, 0, NULL, 0);
        Lobby_LoadTextures();
        gLobbyActive = 1;
        gLobby->step++;
        gLobby->connected = 1;
        memset(&gLobby->msg6105, 0, 0x1004);
        memset(gLobby->msg614C, 0, 0x1004);
        LobbyNet_Reset();
        LobbyRoute_Register();
        memset(&dev, 0, sizeof(dev));
        Net_GetDeviceState(&d0, &d1, &d2, &d3);
        dev.unk1 = 1;
        dev.unk2 = 8;
        dev.unk0 = 0;
        strncpy(dev.version, gVersionString, 0xA);
        gSrvQNetMaxStall = 0;
        dev.state[4] = 0;
        gSrvQNetPeerTimeout = 0;
        dev.state[0] = d0;
        dev.state[1] = d1;
        dev.state[2] = d2;
        dev.state[3] = d3;
        dev.state[5] = 0;
        dev.state[6] = 0;
        dev.state[7] = 0;
        gQuestQNetReadyArg = 0;
        LobbyNet_Set6103(&dev);
        LobbyTitle_Set(0);
        LobbyText_Open(0);
        break;
    case 1:
        (*seq)++;
    case 2:
        gLobby->step++;
        memset(&req, 0, sizeof(req));
        req.entry = gNetMode.entry;
        req.unk0 = 1;
        req.unk20 = (s32)gSysSettings.extraBuf;
        strncpy(req.id, gAccountId, 0xA);
        strncpy(req.password, gAccountData, 0x10);
        LobbyMenu_StartTimer();
        NetTask0_Start((struct NetLoginParams *)&req, LobbyLogin_OnProgress);
        break;
    case 3:
        LobbyMenu_TickTimer();
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
    gLobbyMenu->redraw = 1;
}

void LobbyEntry_Terms(void)
{
    u8 step;

    step = gLobby->step;
    switch (step) {
    case 0:
        gLobby->step = (u8) (step + 1);
        SeqA_Start(2);
        strcpy(&gLobbyWebUrl, "file://SRC/BROWSER/DATA/N_KYK_TP.HTM");
        Lobby_MarkDisconnected();
        break;
    case 1:
        if (gLobby->linkState == 2) {
            gLobby->step = (u8) (step + 1);
            gLobby->linkState = 0U;
            SeqA_Start(1);
            break;
        }
        LobbyWeb_Run();
        break;
    case 3:
        LobbyMenu_TickTimer();
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
}

void LobbyEntry_Step2(void)
{
    u16 btn;
    LobbyState *p;
    u8 *seq;
    s32 *t;
    s16 *c;

    btn = LobbyPad_Get(0);
    p = gLobby;
    seq = &p->step;
    switch (*seq) {
    case 0:
        if (Lobby_NetMenuAllowed(p->msg6105) == 0) {
            gLobby->step = 5;
            LobbyMenu_StartTimer();
            LobbyNet_Send6105(0);
        } else {
            gLobby->step++;
        }
        break;
    case 1:
        (*seq)++;
        gLobby->countdown = 0x1E;
        break;
    case 2:
        (*seq)++;
        if (gLobby->msg6105 != 0) {
            gLobby->waitTimer = 8;
            LobbyDialog_OpenCountdown(gLobby->msg6105Text, *(u16 *)&gLobby->msg6105Secs * 30);
        } else {
            memcpy(gLobby->errText, gLobby->msg6105Text, 0x300);
            Lobby_Disconnect(6);
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        if (*(u16 *)&gLobby->msg6105Secs != 0) {
            gLobby->countdown--;
            t = &gLobby->countdown;
            if (*(s8 *)t <= 0) {
                *t = 0x1E;
                (*(u16 *)&gLobby->msg6105Secs)--;
                c = &gLobby->msg6105Secs;
                if (*c < 0) {
                    *c = 0;
                }
            }
        } else if (btn & 0x20) {
            gLobby->step++;
            LobbySe_Play(0);
            LobbyText_Open(0);
        }
        break;
    case 4:
        (*seq)++;
        LobbyMenu_StartTimer();
        LobbyNet_Send6105(1);
        break;
    case 5:
        LobbyMenu_TickTimer();
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
}

void LobbyEntry_Step3(void)
{
    LobbyState *p;
    u8 *seq;
    s32 r;

    p = gLobby;
    seq = &p->step;
    switch (*seq) {
    case 0:
        (*seq)++;
        gLobby->connected = 0;
        LobbyNet_GetPatchInfo((NetPatchInfo *)&gLobby->unk35EC);
        LobbyNetXfer_Reset();
        Game_ResetSubsystems();
        D_008BFAFD = 1;
        break;
    case 1:
        (*seq)++;
        D_008BFAFD = 1;
        break;
    case 2:
        D_008BFAFD = 1;
        r = LobbyNetXfer_Run();
        if (r == 1) {
            gLobby->step++;
            Lobby_ReleaseNetTextures();
            LoadOverlay(0xA);
        } else if (r == -1) {
            gLobby->step = 6;
            Lobby_ReleaseNetTextures();
            LoadOverlay(0xA);
        } else {
            NetMode_CallDrawScreen(1);
        }
        break;
    case 3:
        (*seq)++;
        Lobby_LoadTextures();
        D_008BFAFD = 0;
        break;
    case 4:
        (*seq)++;
        gLobby->connected = 1;
        LobbyMenu_StartTimer();
        LobbyNet_Send6125();
        break;
    case 5:
        LobbyMenu_TickTimer();
        break;
    case 6:
        (*seq)++;
        Lobby_LoadTextures();
        D_008BFAFD = 0;
        break;
    case 7:
        p->connected = 1;
        Lobby_Disconnect(1);
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
}

void LobbyEntry_IdSelect(void)
{
    s32 i;
    s32 n;
    s32 o17;
    s32 k;
    s32 o8;

    switch (gLobby->step) {
    case 0:
        gLobbyMenu->redraw = 1;
        gLobby->unk1D = 0;
        gLobby->idCursor = 0;
        memset(gLobby->charaIds, 0, sizeof(gLobby->charaIds));
        memset(gLobby->charaNames, 0, sizeof(gLobby->charaNames));
        memset(gLobby->charaTexts, 0, sizeof(gLobby->charaTexts));
        n = (u8)LobbyNet_GetCharaCount();
        for (i = 0; i < n; i++) {
            LobbyNet_GetCharaId(i, gLobby->charaIds[i]);
            LobbyNet_GetCharaName(i, (char *) (&gLobby->charaNames[i * 0x11]));
            LobbyNet_GetCharaText(i, &gLobby->charaTexts[i * 0x100]);
        }
        gLobby->step++;
        gLobby->subStep = 0;
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_614D()) {
        case 0:
        case 1:
            gLobby->step++;
            LobbyMenu_Init();
            gLobbyMenu->promptStep = gLobby->unk1D;
            gLobbyMenu->cursor = gLobby->idCursor;
            break;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if ((u8)SeqA_GetState() != 1) {
            gLobby->step++;
            if (gNetEntryMode == 0) {
                memset(gLobby, 0, 0x1C);
                gElderForecastHeard = 0;
                SeqA_Start(2);
                if ((u8)SeqB_GetState() != 0) {
                    SeqB_Start(2);
                }
            }
        }
        break;
    case 3:
        switch (IdMenu_Run()) {
        case 0:
            gMemberNamesOffSaved = 0;
            gChatTextColor = 0;
            gLobby->step++;
            break;
        case -2:
            Lobby_Disconnect(1);
            break;
        case -1:
            break;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        if ((u8)SeqA_GetState() != 1) {
            gLobby->step++;
        }
    case 5:
        gLobby->step++;
        gLobbyMenu->redraw = 1;
        k = gLobbyMenu->cursor;
        gLobby->idCursor = k;
        LobbyMenu_StartTimer();
        for (i = 0; i < 10; i++) {
            memset(&gPlayers[i], 0, sizeof(Player));
        }
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        o17 = k * 0x11;
        memcpy(&gLobby->charaNames[o17], gSave.name, 0x10);
        o8 = k * 8;
        if (gLobby->charaIds[0][o8] == 0) {
            HunterCard_Sanitize(&gLobby->charaTexts[k * 0x100]);
            LobbyNet_SetMe((char *) NULL, (char *) &gLobby->charaNames[o17], &gLobby->charaTexts[k * 0x100]);
        } else {
            HunterCard_Sanitize(&gLobby->charaTexts[k * 0x100]);
            LobbyNet_SetMe((char *) &gLobby->charaIds[0][o8], (char *) &gLobby->charaNames[o17], &gLobby->charaTexts[k * 0x100]);
        }
        LobbyNet_GetMyCharaId((char *) gLobby->myId);
        LobbyNet_GetMyCharaName((char *) gLobby->myName);
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        if (LobbyMenu_TickTimer() == 0) {
            Loading_Request();
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
}

void LobbyEntry_Step5(void)
{
    u16 btn;

    btn = LobbyPad_Get(0);
    switch (gLobby->step) {
    case 0:
        LobbyMenu_Init();
        LobbyMenu_SetDraw(LobbyEntry_Draw, 0, NULL, 0);
        gLobby->step++;
        SeqA_Start(2);
        gLobby->countdown = 0x258;
        LobbyDialog_OpenWait(gLobby->errText);
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        gLobby->countdown--;
        if (gLobby->countdown > 0) {
            if (gLobby->countdown < 0x1E0 && (btn & 0x20)) {
                gLobby->step++;
                LobbySe_Play(0);
                SeqA_Start(1);
            }
        } else {
            gLobby->step++;
            SeqA_Start(1);
        }
        break;
    case 2:
        if ((u8)SeqA_GetState() != 1) {
            gLobbyMenu->hideLayers = 1;
            gLobby->mode = 4;
            gLobby->step = 1;
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
}

void LobbyEntry_Login(void)
{
    s32 k;
    s32 i;
    s16 v;
    HunterCard *pt;

    pt = &gPlayerText.card;
    switch (gLobby->step) {
    case 0:
        switch (LobbyLogin_SendSummary()) {
        case 0:
            gLobby->step++;
            break;
        case 1:
            Lobby_Disconnect(5);
            break;
        default:
            Loading_Request();
            break;
        }
        break;
    case 1:
        if (gNetEntryMode == 0) {
            HunterCard_Build(pt);
            gPlayerText.card.arenaRole = 0;
            HunterCard_Sanitize(pt);
        }
        v = (gNetEntryMode != 3) ? 0x83 : 0x86;
        k = gScene.localPlayer;
        gScene.area = gPlayers[k].nextArea = gPlayers[k].area = v;
        memset((u8 *)(*(s32 *)&gLobbyWorkBuf + 0x6EF4), 0, 0x20);
        pt->chatColor = gChatTextColor;
        pt->questStatus = 0;
        for (i = 0; i < 12; i++) {
            gFriendCheckTimers[i] = 0;
        }

        gLobby->step++;
        gLobby->seqStep = 0;
        break;
    case 2:
        switch (LobbyStep_6190()) {
        case 0:
            gLobby->step++;
            break;
        case 1:
            Lobby_Disconnect(5);
            break;
        }
        break;
    case 3:
        ServerList_ClearTried();
        if (gNetEntryMode == 0) {
            gNetEntryMode = 1;
            Lobby_Disconnect(0);
        } else {
            gLobby->mode = 7;
            gLobby->step = 0;
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
    gLobbyMenu->redraw = 1;
}

void LobbyEntry_Info(void)
{
    LobbyState *p;
    u8 *seq;
    s32 *t;
    u16 btn;

    btn = LobbyPad_Get(0);
    p = gLobby;
    seq = &p->step;
    switch (*seq) {
    case 0:
        (*seq)++;
        gLobby->seqStep = 0;
        LobbyMenu_SetDraw(LobbyEntry_Draw, 0, NULL, 0);
        LobbyTitle_Set(1);
        break;
    case 1:
        switch (LobbyStep_614C()) {
        case 0:
            gLobby->step++;
            break;
        case 1:
            gLobby->step = 7;
            break;
        }
        break;
    case 2:
        (*seq)++;
    case 3:
        if (Lobby_NetMenu2Allowed(gLobby->msg614C[0])) {
            gLobby->step++;
        } else {
            gLobby->step = 7;
        }
        break;
    case 4:
        (*seq)++;
        if ((u8)SeqA_GetState() != 0) {
            SeqA_Start(2);
        }
        LobbyDialog_OpenWait(gLobby->msg614CText);
        gLobby->countdown = 0x1E;
        gLobby->waitTimer = 0x708;
        break;
    case 5:
        t = &p->countdown;
        if (*t != 0) {
            (*t)--;
        } else if ((u8)SeqA_GetState() != 1) {
            if ((btn & 0x20) || --gLobby->waitTimer <= 0) {
                gLobby->step++;
                LobbySe_Play(0);
                LobbyText_Open(0x1B);
            }
        }
        break;
    case 6:
        if ((u8)SeqA_GetState() == 1) {
            break;
        }
    case 7:
        gLobby->mode = 8;
        gLobby->step = 0;
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
    gLobbyMenu->redraw = 1;
}

void LobbyEntry_AreaInfo(void)
{
    LobbyState *p;
    u8 *seq;
    s32 *t;
    s32 i;
    char *buf;

    p = gLobby;
    seq = &p->step;
    switch (*seq) {
    case 0:
        if (gNetEntryMode == 3) {
            FileQueue_Flush();
            gLobby->step++;
            NetMenu_Open(7);
            gLobby->countdown = 3;
            Stream_Reset();
            gLobby->connected = 0;
            gLobby->dlQuestsReady = 0;
        } else {
            p->dlQuestsReady = 0;
            LobbyNet_Send6141();
            gLobby->step = 3;
            gLobby->subStep = 0;
            gLobby->connected = 1;
        }
        LobbyText_Open(0x1B);
        if ((u8)SeqA_GetState() != 0) {
            SeqA_Start(2);
        }
        break;
    case 1:
        t = &p->countdown;
        if (*t == 0) {
            if ((u8)NetMenu_Update() != 0) {
                gLobby->step++;
                LobbyNet_Send6141();
                LobbyText_Open(0x1B);
                gLobbyMenu->redraw = 0;
                gLobby->connected = 1;
                return;
            }
        } else {
            (*t)--;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_6139()) {
        case 1:
        case 0:
            gLobby->step++;
            return;
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_DownloadQuests()) {
        case 1:
        case 0:
            gLobby->step++;
            return;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_68C0()) {
        case 0:
            gLobby->step++;
            return;
        }
        break;
    case 5:
        (*seq)++;
        gLobby->seqStep = 0;
    case 6:
        switch (LobbyStep_6190()) {
        case 0:
        case 1:
            gLobby->step++;
            gLobby->seqStep = 0;
            break;
        }
        break;
    case 7:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_670F()) {
        case 1:
        case 0:
            gLobby->step++;
            break;
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_6892()) {
        case 1:
        case 0:
            gLobby->step++;
            LobbyScreen_InitLists();
            for (i = 0, buf = (char *)&gTownQuestLists; i < 4; i++, buf += 0x100) {
                LobbyNet_SetSlotBuf(i, buf, 0x100);
            }
            break;
        }
        break;
    case 9:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_6884(0, 3)) {
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            gLobby->step = 0xB;
            break;
        case 0:
            gLobby->step++;
            if (D_5D4924 != 7 || gLobbyLastSeason != LobbyWorld_GetSeason() || gLobbyLastPm != LobbyTime_IsPm()) {
                QuestList_Rebuild();
            }
            break;
        }
        break;
    case 10:
        gLobbyMenu->redraw = 1;
        if ((u8)SeqA_GetState() != 1) {
            if (gNetEntryMode == 3 && gLobby->groupJoin == 0) {
                gLobby->unk3969 = 1;
            } else {
                gLobby->unk3969 = 0;
            }
            LobbyEnter_Begin();
            gNetEntryMode = 2;
        }
        break;
    case 11:
        gLobbyMenu->redraw = 1;
        if (LobbyPad_Get(0) & 0x20) {
            Lobby_Disconnect(5);
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
    if (gLobby->step >= 2) {
        gLobbyMenu->redraw = 1;
    }
}

void LobbyEntry_Step9(void)
{
    LobbyWebInfo info;
    u8 *seq;
    u16 *secs;

    seq = &gLobby->step;
    switch (*seq) {
    case 0:
        LobbyNet_Get68B0((struct Net68B0 *)&info);
        memcpy(&gLobbyWebUrl, info.url, 0x100);
        secs = &info.secs;
        if (*secs != 0) {
            gLobby->step++;
            gLobby->countdown = *secs * 30;
            memcpy(gLobby->errText, info.text, 0x300);
            LobbyDialog_OpenCountdown(gLobby->errText, gLobby->countdown);
            gLobbyMenu->redraw = 1;
        } else {
            gLobby->step = 2;
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        if (--gLobby->countdown <= 0) {
            gLobby->step++;
        }
        break;
    case 2:
        (*seq)++;
        Lobby_MarkDisconnected();
        break;
    case 3:
        LobbyWeb_Run();
        break;
    }
}

/* unk_7C1540's .data (0x819700-0x819738) */
void LobbyEntry_AreaInfo(void);
void LobbyEntry_IdSelect(void);
void LobbyEntry_Info(void);
void LobbyEntry_Init(void);
void LobbyEntry_Login(void);
void LobbyEntry_Step2(void);
void LobbyEntry_Step3(void);
void LobbyEntry_Step5(void);
void LobbyEntry_Step9(void);
void LobbyEntry_Terms(void);
u8 gChatDigitColors[8] = {
#include "lobby/00819700.inc"
};
void (*gLobbyEntrySteps[10])(void) = {
    LobbyEntry_Init, LobbyEntry_Terms, LobbyEntry_Step2, LobbyEntry_Step3,
    LobbyEntry_IdSelect, LobbyEntry_Step5, LobbyEntry_Login, LobbyEntry_Info,
    LobbyEntry_AreaInfo, LobbyEntry_Step9,
};
