#ifndef LOBBY_NET_H
#define LOBBY_NET_H

#include "common.h"
#include "lobby_state.h"
#include "net_tcp.h"

/*
 * The lobby protocol, src/lobby/lobby_net.c.
 * NETWORK code: it must stay byte-for-byte for online play.
 */

/*
 * An outgoing message: a 12-byte header, sent from +4, then the payload.
 * The header is 82 (or 81) flag cmd(BE16) size(BE16) echo[2] 00 FF FF FF.
 */
typedef struct LobbyPkt {
    /* 0x00 */ u16 len;         /* payload bytes written */
    /* 0x02 */ u16 pos;         /* write position in data */
    /* 0x04 */ u8 magic;        /* 0x82 (LobbyPkt_Begin), 0x81 (LobbyPkt_BeginCmd) */
    /* 0x05 */ u8 flag;         /* 2: a reply, which echoes the request's echo bytes */
    /* 0x06 */ u8 cmd[2];       /* big-endian */
    /* 0x08 */ u8 size[2];      /* big-endian payload size (LobbyPkt_End) */
    /* 0x0A */ u8 echo[2];      /* a request's sequence number, or the echo of a reply */
    /* 0x0C */ u8 unkC;
    /* 0x0D */ u8 ff[3];
    /* 0x10 */ u8 data[0x300];
} LobbyPkt;

/* A received message's header (gLobbyRecvHdr, 0x10 bytes; Lobby_RecvPacket reads it): the
 * outgoing header's layout (LobbyPkt from +4), its byte 8 the reply's error. */
typedef struct LobbyHdr {
    /* 0x0 */ u8 magic;         /* the kind << 4 (0x82, 0x81) */
    /* 0x1 */ u8 flag;
    /* 0x2 */ u8 cmd[2];        /* big-endian */
    /* 0x4 */ u8 size[2];       /* big-endian payload size */
    /* 0x6 */ u8 echo[2];       /* a request's sequence number, or the echo of a reply */
    /* 0x8 */ s8 err;           /* nonzero: the reply reports an error (gLobbyNet.replyErr) */
    /* 0x9 */ u8 ff[3];
    /* 0xC */ u8 unkC[4];
} LobbyHdr;
typedef char LobbyHdr_size_check[(sizeof(LobbyHdr) == 0x10) ? 1 : -1];

/* 6 bytes, passed by value (in one 64-bit register); a u8 local can take
 * the last byte of its 8-byte stack home */
typedef struct NetMsg {
    s8 kind;
    s8 flag;                    /* signed: LobbyLogin_OnProgress and LobbyLevel2_OnList read it with lb */
    u16 n;
    u16 unk4;
} NetMsg;

/* a blob to send: passed by value */
typedef struct NetBlob {
    u8 kind;
    u8 pad;
    u16 n;
    void *data;
} NetBlob;

/* A request waiting for its reply (the message whose echo is seq). */
typedef struct NetReq {
    /* 0x00 */ void (*onTick)(s32 i);                   /* every frame while state is 2 */
    /* 0x04 */ void (*onReply)(NetMsg m, NetMsg *pm);
    /* 0x08 */ u8 unk8[0xE];
    /* 0x16 */ u16 seq;
    /* 0x18 */ u8 state;                                /* 0: free */
    /* 0x19 */ u8 unk19;
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 pad1B;
} NetReq;

/*
 * A network task (gLobbyNet.task[]): a request sequence run every frame while
 * active. Its starter clears it, sets func (the task's dispatcher) and onDone;
 * the dispatcher runs the current step while state is 0, and calls onDone with
 * a NetMsg of kind 0 once the steps are done (state 1) or -1 if one failed
 * (state 2). A step sends a request; the request's reply callback sets reply
 * (1 or 2, or bits of it and 0x80 for an error), which the next step checks.
 */
typedef struct NetTask {
    /* 0x00 */ void (*func)(s32 i);                     /* every frame while active is 1 */
    /* 0x04 */ void (*onDone)(NetMsg m, NetMsg *pm);    /* kind 0: done, -1: failed */
    /* 0x08 */ u32 idx; /* the record a step works on (NetTask6: gLobbyNet.recs2.rec[idx], counted up); the starters set 0 or 1 */
    /* 0x0C */ u32 pos; /* the entry or offset a step asks for next (counted up; +0x200 a block for the data reads) */
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 arg;                                 /* the starter's argument: the id the requests name (NetTask6's LobbyReq_66xx) */
    /* 0x18 */ s32 reqFlags; /* which requests to send for each entry (the starter's flags & mask; the NetListEntry.flags bits: 1 6303, 2 6304, 4 6302...) */
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u8 active;
    /* 0x21 */ u8 state;                                /* 0 running, 1 done, 2 failed */
    /* 0x22 */ u8 step;
    /* 0x23 */ u8 reply;                                /* set by its reply callback */
    /* 0x24 */ u8 reqCount; /* requests per entry (the bits of reqFlags counted by the starter): nReplies = entries * reqCount; a step waits for that many free requests */
    /* 0x25 */ u8 pad25[3];
} NetTask;

/* the values of an entry: the replies to 6312, 6313 and 6314 (unk_7055A0.c) */
typedef struct NetEntryVals {
    u8 v[4];
} NetEntryVals;

/* a user as 630A's reply lists them (up to 10); the notifications
 * 6702/670C send one with one more byte. plsel's chat log takes its
 * sender in this form (Plsel_AddChatLog). */
typedef struct NetUserRec {
    /* 0x000 */ NetUser u;
    /* 0x11C */ u8 color[4];    /* the chat colours, as ChatLine.color: plsel's log copies them (6702 sends them) */
} NetUserRec;

/* 6892's reply (LobbyReply_6892): five u32, as many u32 as the fifth says
 * (at most 12), one more u32 */
typedef struct Net6892 {
    /* 0x00 */ u32 time;             /* the lobby clock in ticks (gLobbyTime = LobbyWorld_TicksToSeconds) */
    /* 0x04 */ u32 dayTicks;         /* gTownDayTicks */
    /* 0x08 */ u32 day;              /* gTownDay */
    /* 0x0C */ u32 season;           /* gTownSeason */
    /* 0x10 */ s32 n;
    /* 0x14 */ u32 list[12];
    /* 0x44 */ u32 unk44;
} Net6892;

/*
 * An entry of one of the three levels' lists (gLobbyNet.list0/list1/list2),
 * indexed by the server's id - 1 (the C also reads list1 as D_00852F40[id],
 * the same address). flags says which of the entry's requests were answered;
 * for the second level (63xx; the other levels have their own codes):
 * 1 6303 (users-unk10), 2 6304 (unk1E), 4 6302 (name), 8 6308 (text),
 * 0x10 630B, 0x80 630D (unk4), 0x200 631B (unk188), 0x400 630E (kind).
 */
typedef struct NetListEntry {
    /* 0x000 */ u32 flags;
    /* 0x004 */ s32 color;          /* level 1 (630D): the name colour (Text_SetColor of its low half); level 2: 650A */
    /* 0x008 */ u16 id;
    /* 0x00A */ u16 users;           /* LevelList_DrawEntries draws users/maxUsers */
    /* 0x00C */ u16 questUsers;     /* users out of town: the level-1 list shows users - this as in town */
    /* 0x00E */ u16 maxUsers;        /* level 2: also the first value of 640B's reply */
    /* 0x010 */ u16 unk10;
    /* 0x012 */ u16 unk12;
    /* 0x014 */ u16 unk14;
    /* 0x016 */ u16 unk16;
    /* 0x018 */ u16 unk18;
    /* 0x01A */ u16 roomMembers;    /* 6412 (level 2): NetList2_Get6412 gives gLobby->roomMembers */
    /* 0x01C */ u16 unk1C;
    /* 0x01E */ u8 state;           /* 6206, 6304, 6404: 3 and 4 open (LevelEntry.state) */
    /* 0x01F */ u8 unk1F;
    /* 0x020 */ char name[0x44];
    /* 0x064 */ char text[0x101];
    /* 0x165 */ u8 kind;              /* 630E, 6418, 620B (NetList0_GetKind: level 0's kind) */
    /* 0x166 */ u8 unk166[2];
    /* 0x168 */ u8 postedQuest[0x20]; /* 650C (level 2): the posted quest, copied to gPostedQuests */
    /* 0x188 */ u8 groupInfo[4];    /* 631B (level 1): [0] the group limit kind, [1] the quest target */
} NetListEntry;

/* a file the server offers for download (6883's reply: u8 index, u32 a, u32 b) */
typedef struct NetDlFile {
    /* 0x0 */ u32 a;
    /* 0x4 */ u32 b;
} NetDlFile;

/* 6417's reply (LobbyReply_6417Read): a third-level entry with up to 4 members */
typedef struct NetMember4 {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x11];
    /* 0x19 */ u8 unk19;
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 pad1B[5];
} NetMember4;

typedef struct NetInfo6417 {
    /* 0x000 */ u16 unk0;
    /* 0x002 */ u16 unk2;
    /* 0x004 */ u8 unk4[4];
    /* 0x008 */ u16 n;              /* members (at most 4) */
    /* 0x00A */ u16 unkA;
    /* 0x00C */ char text[0x101];
    /* 0x10D */ u8 unk10D;
    /* 0x10E */ u8 pad10E[2];
    /* 0x110 */ NetMember4 members[4];
} NetInfo6417;

/* 631A's reply (LobbyReply_631ARead): a second-level entry with up to 10 users */
typedef struct NetMember10 {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x14];
    /* 0x1C */ u16 unk1C;           /* bytes 0-1 of a 0x10-byte blob, little-endian */
    /* 0x1E */ u8 unk1E;            /* its byte 3 */
    /* 0x1F */ u8 pad1F;
} NetMember10;

typedef struct NetInfo631A {
    /* 0x000 */ char name[0x10];
    /* 0x010 */ u16 unk10;
    /* 0x012 */ u16 unk12;
    /* 0x014 */ char text14[0x44];
    /* 0x058 */ char text58[0x44];
    /* 0x09C */ u8 unk9C[4];
    /* 0x0A0 */ u16 unkA0[4];
    /* 0x0A8 */ u16 n;              /* users (at most 10) */
    /* 0x0AA */ u16 unkAA;
    /* 0x0AC */ u16 unkAC;
    /* 0x0AE */ u8 padAE[2];
    /* 0x0B0 */ char textB0[0x104];
    /* 0x1B4 */ NetMember10 users[10];
} NetInfo631A;

/* the settings task 8 sends for the second-level entry the player is in
 * (gLobbyNet.settings): 6315 name, 6316 password, 6317 text, 6318 four bytes */
typedef struct NetEntrySettings {
    /* 0x000 */ char name[0x41];
    /* 0x041 */ char pass[9];
    /* 0x04A */ char text[0x101];
    /* 0x14B */ u8 b[4];
} NetEntrySettings;

/* the same for the third-level entry the player is in (gLobbyNet.settings2, sent by
 * task 7: 64xx/65xx): the four fields, the bytes sent one by one (as many as
 * gLobbyNet.recs2.n) */
typedef struct NetEntrySettings2 {
    /* 0x000 */ char name[0x41];
    /* 0x041 */ char pass[9];
    /* 0x04A */ char text[0x101];
    /* 0x14B */ u8 b[0x20];         /* one per record (660B) */
    /* 0x16B */ u8 c[4];            /* 660F */
} NetEntrySettings2;

/* a third-level entry record of task 6 (0x14A5 bytes): a name, a few bytes,
 * and up to 32 sub-entries, each with a name and up to 32 three-byte values */
typedef struct NetRec2 {
    /* 0x0000 */ u8 flags;          /* 1 6604, 2 6606, 8 6605, 4 6607 arrived */
    /* 0x0001 */ char name[0x41];   /* 6604 */
    /* 0x0042 */ u8 unk42;          /* 6605; 1: task 7 sends settings2.b[i] */
    /* 0x0043 */ u8 m;              /* 6607: the sub-entries */
    /* 0x0044 */ u8 unk44;          /* 6606 */
    /* 0x0045 */ u8 subFlags[0x20]; /* 1 6608, 2 660E arrived */
    /* 0x0065 */ char subName[0x20][0x41];  /* 6608 */
    /* 0x0885 */ u8 subN[0x20];     /* 660E: the values */
    /* 0x08A5 */ u8 subData[0x20][0x20][3]; /* 660E */
} NetRec2;

/* the third level's entry records (gLobbyNet.recs2, 0x294A4 bytes; NetTask6_GetRecs
 * copies them out): what the server allows to be sent, and the records */
typedef struct NetRecs2 {
    /* 0x00000 */ u8 sendName;      /* 1: the name may be sent (a u8 reply) */
    /* 0x00001 */ u8 sendPass;
    /* 0x00002 */ u8 sendText;
    /* 0x00003 */ u8 n;             /* records */
    /* 0x00004 */ NetRec2 rec[32];
} NetRecs2;

/* a user's id and data (the notification 6193, 6712's reply) */
typedef struct Net6193 {
    /* 0x000 */ char id[8];
    /* 0x008 */ char name[0x14];
    /* 0x01C */ u8 data[0x100];
    /* 0x11C */ u16 n;
    /* 0x11E */ u16 x;
    /* 0x120 */ u8 pad120[2];
} Net6193;

/* the server's 6105 at login: a text, sent in parts of 0x1000 bytes */
typedef struct Net6105 {
    /* 0x0000 */ u8 unk0;
    /* 0x0001 */ u8 pad1;
    /* 0x0002 */ u16 unk2;
    /* 0x0004 */ char text[0x1000];
} Net6105;

/* 614C's reply: a text, in parts too */
typedef struct Net614C {
    /* 0x0000 */ u8 unk0;
    /* 0x0001 */ u8 pad1[3];
    /* 0x0004 */ char text[0x1000];
} Net614C;

/* the paged search results (a request's reply comes in pages: u8 total, u8
 * first, u8 count, u8 last, then count records; while last is 0 the client
 * asks for the next page with the matching notification, u8 first) */
typedef struct Net6416Rec {
    /* 0x00 */ u8 b[4];
    /* 0x04 */ u16 h[4];
    /* 0x0C */ u8 c;
    /* 0x0D */ u8 pad[3];
} Net6416Rec;

typedef struct Net6711Rec {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x11];
    /* 0x19 */ char text[0xF];
} Net6711Rec;

typedef struct Net6319Rec {
    /* 0x000 */ char text[0x104];
    /* 0x104 */ char s104[0x10];
    /* 0x114 */ u8 b[4];
    /* 0x118 */ u16 h118;
    /* 0x11A */ u16 h11A;
    /* 0x11C */ u16 h11C;
    /* 0x11E */ u16 h11E;
    /* 0x120 */ u16 h120;
    /* 0x122 */ u8 index;           /* its place in the whole list */
    /* 0x123 */ u8 pad123;
} Net6319Rec;

/* the results of a search as the pages of its reply fill them (a count,
 * then up to 100 records); a new request clears them (memset of the
 * whole struct: 0x6EF4 for 6709, 0xFA4 for 6711) */
typedef struct NetFound6709 {
    /* 0x0000 */ u8 total;              /* the users found */
    /* 0x0001 */ u8 pad1[3];
    /* 0x0004 */ NetUser users[100];
} NetFound6709;

typedef struct NetFound6416 {
    /* 0x000 */ u8 total;
    /* 0x001 */ u8 pad1[3];
    /* 0x004 */ Net6416Rec list[100];
} NetFound6416;

typedef struct NetFound6711 {
    /* 0x000 */ u8 total;
    /* 0x001 */ u8 pad1[3];
    /* 0x004 */ Net6711Rec list[100];
} NetFound6711;

typedef struct NetFound6319 {
    /* 0x0000 */ u8 total;
    /* 0x0001 */ u8 pad1[3];
    /* 0x0004 */ Net6319Rec list[100];
} NetFound6319;

/* a search's conditions (6709: up to 8, 6319: up to 4) */
typedef struct NetCond6709 {
    /* 0x00 */ u8 type;             /* 0-6: what data holds (a 6-byte id, a string, bytes) */
    /* 0x01 */ u8 n;
    /* 0x02 */ u8 pad2[2];
    /* 0x04 */ u8 data[0x40];
} NetCond6709;

typedef struct NetSearch6709 {
    /* 0x000 */ u8 unk0;
    /* 0x001 */ u8 n;
    /* 0x002 */ u8 pad2[2];
    /* 0x004 */ NetCond6709 cond[8];
} NetSearch6709;

typedef struct NetCond6319 {
    /* 0x00 */ u8 type;
    /* 0x01 */ u8 n;
    /* 0x02 */ u8 v[0x42];
} NetCond6319;

typedef struct NetSearch6319 {
    /* 0x000 */ u8 id[0x10];        /* 0xC bytes sent */
    /* 0x010 */ u8 unk10;
    /* 0x011 */ u8 n;
    /* 0x012 */ u8 pad12[2];
    /* 0x014 */ NetCond6319 cond[4];
} NetSearch6319;

/* 6713's reply: a user's details */
typedef struct Net6713 {
    /* 0x000 */ u8 s0[0x14];
    /* 0x014 */ u8 s14[0x10];
    /* 0x024 */ u8 s24[0x104];
    /* 0x128 */ u8 s128[0x104];
    /* 0x22C */ u8 s22C[0x44];
    /* 0x270 */ u8 s270[0x44];
    /* 0x2B4 */ u8 s2B4[0x104];
    /* 0x3B8 */ u16 h3B8;
    /* 0x3BA */ u16 h3BA;
    /* 0x3BC */ u16 h3BC;
    /* 0x3BE */ u16 h3BE;
    /* 0x3C0 */ u8 b3C0[4];
    /* 0x3C4 */ u32 w3C4;
    /* 0x3C8 */ u8 b3C8;
    /* 0x3C9 */ u8 b3C9;
    /* 0x3CA */ u8 b3CA;
    /* 0x3CB */ u8 pad3CB;
} Net6713;

/* The quest party task 9 fetches (69xx): 6911 the number of players, 6912 a
 * byte (less one), 6913 and 6917 for each player, 6915 the room's name, 6914 a
 * text, then 6916 the address of the quest server. LobbyRoom_FetchParty copies the
 * players into LobbyState.players and sub_main's tables; the connection then
 * goes to the 6916 address (unk_7B0D80.c: the 1031 challenge, QNetIf_Start). */
typedef struct NetPartyMember {
    /* 0x000 */ char id[8];         /* 6913 */
    /* 0x008 */ char name[0x14];    /* 6913 */
    /* 0x01C */ char text[0x100];   /* 6913 */
    /* 0x11C */ char s11C[0x20];    /* 6913 */
    /* 0x13C */ u16 h13C;           /* 6917 */
    /* 0x13E */ u8 pad13E[2];
    /* 0x140 */ u32 w[5];           /* 6917 */
    /* 0x154 */ u8 index;           /* 1-based */
    /* 0x155 */ u8 b155;            /* 6913 */
    /* 0x156 */ u8 b156;            /* 6913 */
    /* 0x157 */ u8 pad157;
} NetPartyMember;

typedef struct NetParty {
    /* 0x000 */ u8 count;           /* 6911 */
    /* 0x001 */ u8 unk1;            /* 6912 (less one) */
    /* 0x002 */ char name[0x10];    /* 6915: the room's name */
    /* 0x012 */ u16 unk12;          /* 6915 */
    /* 0x014 */ u8 text[0x100];     /* 6914 (LobbyRoom_FetchParty reads bytes 0 and 2) */
    /* 0x114 */ NetPartyMember members[10];
} NetParty;

/* what the connection was started with (NetTask0_Start, 0x40 bytes; the login
 * code at 0x7C2E9C fills it from sub_main's account data) */
typedef struct NetLoginParams {
    /* 0x00 */ u8 type;             /* 1; nonzero: 6103 is answered with pings (600E) */
    /* 0x01 */ s8 unk1;             /* sub_main 0x5D46B1 */
    /* 0x02 */ char id[0xB];        /* the account id, 10 characters (sub_main 0x5CAEE1) */
    /* 0x0D */ char pass[0x13];     /* 16 characters (sub_main 0x5CAED0), sent obfuscated */
    /* 0x20 */ u8 *patchBuf;        /* where a patch sent at login goes (sub_main 0x6AE018) */
    /* 0x24 */ s32 unk24[7];
} NetLoginParams;

/* An entry of one of the three levels as the lobby screens keep it (0x164
 * bytes; level 0 in sub_main's gLobbyLevel0Entries, level 1 in gLobbyLevel1Entries, level 2 in
 * D_008EA200): copied out of gLobbyNet.list0-2 by the NetListN_Get* getters. */
typedef struct LevelEntry {
    /* 0x000 */ s32 color;          /* level 1 (630D): the name colour (Text_SetColor of its low half); level 2: 650A */
    /* 0x004 */ u16 id;
    /* 0x006 */ u16 users;
    /* 0x008 */ u16 maxUsers;       /* level 2: 640B's first value */
    /* 0x00A */ u16 unkA[4];        /* level 2: 640B's other values */
    /* 0x012 */ u16 questUsers;     /* users out of town (users - this: in town) */
    /* 0x014 */ u16 unk14;          /* NetListEntry.unk10 */
    /* 0x016 */ u8 state;           /* 6206, 6304, 6404: 3 and 4 open (enterable) */
    /* 0x017 */ u8 unk17;           /* level 2: func_00716B00 */
    /* 0x018 */ u8 pad18;
    /* 0x019 */ u8 unk19;
    /* 0x01A */ u8 kind;             /* level 0: 1-4 (func_0070C170) */
    /* 0x01B */ u8 groupInfo[4];    /* level 1 (631B): [0] the group limit kind (gGroupLimitTexts[k + 3]), [1] the quest target */
    /* 0x01F */ char name[0x41];
    /* 0x060 */ char text[0x104];
} LevelEntry;

/* the patch announced at login (6121), as LobbyNet_GetPatchInfo copies it out */
typedef struct NetPatchInfo {
    /* 0x00 */ u32 size;
    /* 0x04 */ char version[0x10];  /* 10 characters */
    /* 0x14 */ char tag[8];         /* 4 characters */
} NetPatchInfo;

/* the player's answer to the server's request 6103 (LobbyPkt_6103: u8 x3,
 * blob (10 bytes), u16 x8), set with LobbyNet_Set6103 */
typedef struct Net6103 {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 pad3;
    /* 0x04 */ char version[0x10];  /* gVersionString (10 characters sent) */
    /* 0x14 */ u16 state[8];        /* [0..3] Net_GetDeviceState, [4] the average round trip (600E), [5..7] 0 */
} Net6103;

/* a user entering: the notifications 6702 and 670C (blob id 8, blob name
 * 0x10, blob text 0x100, u8 x4, u8) */
typedef struct Net6702 {
    /* 0x000 */ NetUserRec r;
    /* 0x120 */ u8 unk120;
    /* 0x121 */ u8 pad121[3];
} Net6702;

/* 6705's reply: three strings (a user's id, name and a text) */
typedef struct Net6705 {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x14];
    /* 0x1C */ char text[0x80];
} Net6705;

/* 6707's reply: u16 x3, u8 (LobbyReply_6713Read clears all 0x20A bytes) */
typedef struct Net6707 {
    /* 0x000 */ u16 a;
    /* 0x002 */ u16 b;
    /* 0x004 */ u16 c;
    /* 0x006 */ u8 pad6;
    /* 0x007 */ u8 d;
    /* 0x008 */ u8 unk8[0x202];
} Net6707;

/* a chat message: the notifications 6708 and 670E (blob id 8, blob text 0x300) */
typedef struct Net6708 {
    /* 0x000 */ char id[8];
    /* 0x008 */ u8 text[0x300];
} Net6708;

/* the notification 68B0 at login: blob text (0x100), u16 n, then n bytes
 * (blob, at most 0x300) if n (the global right after gLobbyNet) */
typedef struct Net68B0 {
    /* 0x000 */ char text[0x104];
    /* 0x104 */ u8 data[0x300];
    /* 0x404 */ u16 n;
    /* 0x406 */ u8 pad406[2];
} Net68B0;

/* the notification 6891: three u16 (LobbyNet_Get6891) */
typedef struct Net6891 {
    u16 v[3];
} Net6891;

/* The connection state (gLobbyNet, 0x84A440): one large object; the code
 * reaches its fields at offsets from gLobbyNet (the D_0084xxxx-D_0088xxxx
 * symbols in the C are fields of it too). */
typedef struct LobbyNet {
    /* 0x0000 */ s32 connected; /* 1 while connected (set by the connecting step at 0x7069B0); Lobby_ReceiveAll reads nothing while 0 */
    /* 0x0004 */ u8 unk4[4];
    /* 0x0008 */ s32 unk8;
    /* 0x000C */ u8 unkC[4];
    /* 0x0010 */ s16 sent6001; /* 1 once 6001 was sent (LobbySrvReq_6001) */
    /* 0x0012 */ u8 unk12[2];
    /* 0x0014 */ u8 unk14;
    /* 0x0015 */ u8 unk15[3];
    /* 0x0018 */ NetReq req[0x80];
    /* 0x0E18 */ NetTask task[0x10];
    /* 0x1098 */ NetReq req2[0x10];
    /* 0x1258 */ u8 unk1258[0x14];
    /* 0x126C */ s32 sock;
    /* 0x1270 */ u32 recvResult;    /* Lobby_RecvPacket; 1 the last part, >= 2 more follow; nethttp reads the bytes left */
    /* 0x1274 */ u16 cmd;           /* of the message received */
    /* 0x1276 */ u16 flag;
    /* 0x1278 */ u16 echo;
    /* 0x127A */ u16 size;
    /* 0x127C */ s8 replyErr;       /* nonzero: the reply received reports an error */
    /* 0x127D */ s8 reqState;       /* the HTTP request state (nethttp unk_8FB130.c) */
    /* 0x127E */ u16 key;           /* Net_LockrockObfuscate's key */
    /* 0x1280 */ s32 rxLen;         /* bytes gathered in rxBuf: a long reply's parts are appended (memcpy(rxBuf + rxLen, gLobbyRecvData, len)); 0 at the start */
    /* 0x1284 */ s32 rxSize;        /* rxBuf's size (0x1000); a part is cut to rxSize - rxLen - 1 */
    /* 0x1288 */ u8 *rxBuf;         /* where a long reply is gathered (buf38C0C) */
    /* 0x128C */ u8 *patchDst;      /* 6122 appends here */
    /* 0x1290 */ s32 unk1290;
    /* 0x1294 */ s16 unk1294;
    /* 0x1296 */ u16 patchUnk;      /* 6121 */
    /* 0x1298 */ u32 patchSize;     /* 6121 */
    /* 0x129C */ u32 patchSum;      /* 6121: the byte sum 6125 checks */
    /* 0x12A0 */ u8 pad12A0[4];
    /* 0x12A4 */ char patchVersion[0x10]; /* 6121: 10 characters */
    /* 0x12B4 */ char patchTag[8];  /* 6121: 4 characters */
    /* 0x12BC */ u16 frames;        /* Lobby_ReceiveAll calls */
    /* 0x12BE */ u16 rttSum;        /* 600E: the ping replies' round trips (frames * 16) summed; >> 2 goes in 6103 */
    /* 0x12C0 */ u8 rttCount;       /* 600E: the ping replies counted (4 are sent) */
    /* 0x12C1 */ u8 unk12C1;
    /* 0x12C2 */ u16 nReplies;      /* the replies a list task expects (count x requests per entry) */
    /* 0x12C4 */ NetLoginParams login;
    /* 0x1304 */ NetUser me;        /* LobbyNet_SetMe */
    /* 0x1420 */ Net6103 x6103;     /* the answer to the server's 6103 (LobbyNet_Set6103) */
    /* 0x1444 */ u8 unk1444[0x1D6];
    /* 0x161A */ NetEntrySettings settings; /* task 8's */
    /* 0x1769 */ NetEntrySettings2 settings2; /* task 7's */
    /* 0x18D8 */ u8 nCharas;        /* the account's characters (6131, at most 3) */
    /* 0x18D9 */ u8 pad18D9[2];
    /* 0x18DB */ NetUser charas[4]; /* 6131's; charas[3]: the one playing (LobbyNet_SetMe sets its id and name) */
    /* 0x1D4B */ NetUser self;      /* the character's id (6132's reply) and name */
    /* 0x1E67 */ NetUser x6191;
    /* 0x1F83 */ u8 pad1F83;
    /* 0x1F84 */ Net6193 x6193;
    /* 0x20A6 */ Net6193 x6712;
    /* 0x21C8 */ Net614C x614C;
    /* 0x31CC */ Net6105 x6105;
    /* 0x41D0 */ NetUser userInOut; /* the user entering or leaving (func_00704250) */
    /* 0x42EC */ NetUserRec users[10]; /* 630A's reply */
    /* 0x4E2C */ NetUserRec users2[10]; /* 640A's reply */
    /* 0x596C */ u8 unk596C[0xB40];
    /* 0x64AC */ NetUserRec user6414; /* cleared by LobbyNotify_6414 (which then fills users2[0]) */
    /* 0x65CC */ u16 count0;        /* list0's entries (6203's reply; at most 0x19) */
    /* 0x65CE */ u16 count1;        /* list1's entries (6301's reply; at most 0x2D) */
    /* 0x65D0 */ u16 count2;        /* list2's entries (6401's reply; at most 5) */
    /* 0x65D2 */ u8 sel[3];         /* the entry being entered, levels 0-2 (NetList_Select) */
    /* 0x65D5 */ u8 unk65D5;
    /* 0x65D6 */ u8 unk65D6[6];
    /* 0x65DC */ u8 cur[3];         /* the entry entered (sel once the server agreed) */
    /* 0x65DF */ u8 unk65DF;
    /* 0x65E0 */ NetListEntry list0[25];   /* the first level (62xx) */
    /* 0x8C8C */ NetListEntry list1[45];   /* the second level (63xx) */
    /* 0xD228 */ NetListEntry list2[5];    /* the third level (64xx) */
    /* 0xD9E4 */ NetRecs2 recs2;    /* task 6's */
    /* 0x36E88 */ u8 flags[3][0x2D];     /* entry i + 1's reply to 6312, 6313, 6314 arrived */
    /* 0x36F0F */ NetEntryVals rec[0x2D]; /* entry i + 1: the values of those replies */
    /* 0x36FC3 */ u8 nUsers;        /* users[] */
    /* 0x36FC4 */ u8 nUsers2;       /* users2[] */
    /* 0x36FC5 */ u8 pad36FC5;
    /* 0x36FC6 */ u16 x640F[3];     /* 640F's reply and notification */
    /* 0x36FCC */ u8 unk36FCC[8];
    /* 0x36FD4 */ u16 x6409;        /* 6409's reply: its second u16 */
    /* 0x36FD6 */ u16 lastId;       /* the entry the last 62xx-64xx reply was about */
    /* 0x36FD8 */ u8 partyAddr[8];  /* 6916: the quest server's IPv4 address (4 bytes, sent as a string) */
    /* 0x36FE0 */ u8 partyPort[8];  /* 6916: its port (2 bytes, big-endian) */
    /* 0x36FE8 */ NetParty party;   /* task 9's */
    /* 0x37E6C */ Net6705 x6705;
    /* 0x37F08 */ Net6707 x6707;
    /* 0x38112 */ u8 pad38112[2];
    /* 0x38114 */ Net6713 x6713;
    /* 0x384E0 */ Net6702 x6702;    /* a user entering (6702, 670C) */
    /* 0x38604 */ Net6708 x6708;    /* a chat message (6708, 670E) */
    /* 0x3890C */ u8 errText[0x300];  /* the error text a reply carries (LobbyMsg_ReadErrText) */
    /* 0x38C0C */ u8 buf38C0C[0x2000]; /* unk1288 points here */
    /* 0x3AC0C */ u8 dlCount;       /* files offered (6881's reply) */
    /* 0x3AC0D */ u8 dlFile;        /* the file task 14 downloads */
    /* 0x3AC0E */ u8 pad3AC0E[2];
    /* 0x3AC10 */ u8 *dlBuf;        /* where it goes */
    /* 0x3AC14 */ u32 dlSize;
    /* 0x3AC18 */ u32 dlGot;        /* bytes received (6882's replies) */
    /* 0x3AC1C */ NetDlFile dlFiles[32];
    /* 0x3AD1C */ NetFound6709 found6709; /* 6709's search: the users found */
    /* 0x41C10 */ u16 searchReq;    /* the request the pages of a search answer (req[]) */
    /* 0x41C12 */ NetFound6416 found6416;
    /* 0x42256 */ NetInfo6417 x6417;
    /* 0x423E6 */ NetFound6711 found6711;
    /* 0x4338A */ u8 pad4338A[2];
    /* 0x4338C */ u32 x6319;
    /* 0x43390 */ NetFound6319 found6319;
    /* 0x4A5A4 */ NetInfo631A x631A;
    /* 0x4A898 */ u32 x6890;
    /* 0x4A89C */ Net6891 x6891;
    /* 0x4A8A2 */ u8 pad4A8A2[2];
    /* 0x4A8A4 */ u32 x61A0[2];     /* 61A0's reply at login */
    /* 0x4A8AC */ u8 x68A0n;
    /* 0x4A8AD */ u8 pad4A8AD[3];
    /* 0x4A8B0 */ char *x68A1Str;
    /* 0x4A8B4 */ u8 x68A0[4];
    /* 0x4A8B8 */ u8 unk4A8B8[8];
    /* 0x4A8C0 */ u8 *x6884Buf;     /* 6884's reply to an ordinary request goes here */
    /* 0x4A8C4 */ s32 x6884Max;
    /* 0x4A8C8 */ u8 x6884Slot;     /* the slot byte of that reply */
    /* 0x4A8C9 */ u8 pad4A8C9[3];
    /* 0x4A8CC */ void *slotBuf[16];    /* the buffers 6885's notice has 6884 fill */
    /* 0x4A90C */ s32 slotMax;
    /* 0x4A910 */ u8 unk4A910[2];
    /* 0x4A912 */ s16 unk4A912;
    /* 0x4A914 */ u16 slotMask;     /* the slots filled since */
    /* 0x4A916 */ u8 unk4A916[2];
    /* 0x4A918 */ char *x614DDst;   /* 614D's reply: the string goes here */
    /* 0x4A91C */ s32 x614DMax;
    /* 0x4A920 */ s32 x614D;        /* the u16 of that reply */
    /* 0x4A924 */ Net6892 x6892;
} LobbyNet;

/* sizes: these records are read from and written to the network field by
 * field, and gLobbyNet's fields are reached by address elsewhere */
typedef char LobbyPkt_size_check[(sizeof(LobbyPkt) == 0x310) ? 1 : -1];
typedef char NetMsg_size_check[(sizeof(NetMsg) == 0x6) ? 1 : -1];
typedef char NetBlob_size_check[(sizeof(NetBlob) == 0x8) ? 1 : -1];
typedef char NetReq_size_check[(sizeof(NetReq) == 0x1C) ? 1 : -1];
typedef char NetTask_size_check[(sizeof(NetTask) == 0x28) ? 1 : -1];
typedef char NetEntryVals_size_check[(sizeof(NetEntryVals) == 0x4) ? 1 : -1];
typedef char NetUser_size_check[(sizeof(NetUser) == 0x11C) ? 1 : -1];
typedef char NetUserRec_size_check[(sizeof(NetUserRec) == 0x120) ? 1 : -1];
typedef char Net6892_size_check[(sizeof(Net6892) == 0x48) ? 1 : -1];
typedef char NetListEntry_size_check[(sizeof(NetListEntry) == 0x18C) ? 1 : -1];
typedef char NetDlFile_size_check[(sizeof(NetDlFile) == 0x8) ? 1 : -1];
typedef char NetMember4_size_check[(sizeof(NetMember4) == 0x20) ? 1 : -1];
typedef char NetInfo6417_size_check[(sizeof(NetInfo6417) == 0x190) ? 1 : -1];
typedef char NetMember10_size_check[(sizeof(NetMember10) == 0x20) ? 1 : -1];
typedef char NetInfo631A_size_check[(sizeof(NetInfo631A) == 0x2F4) ? 1 : -1];
typedef char NetEntrySettings_size_check[(sizeof(NetEntrySettings) == 0x14F) ? 1 : -1];
typedef char NetEntrySettings2_size_check[(sizeof(NetEntrySettings2) == 0x16F) ? 1 : -1];
typedef char NetRec2_size_check[(sizeof(NetRec2) == 0x14A5) ? 1 : -1];
typedef char NetRecs2_size_check[(sizeof(NetRecs2) == 0x294A4) ? 1 : -1];
typedef char Net6193_size_check[(sizeof(Net6193) == 0x122) ? 1 : -1];
typedef char Net6105_size_check[(sizeof(Net6105) == 0x1004) ? 1 : -1];
typedef char Net614C_size_check[(sizeof(Net614C) == 0x1004) ? 1 : -1];
typedef char Net6103_size_check[(sizeof(Net6103) == 0x24) ? 1 : -1];
typedef char Net6702_size_check[(sizeof(Net6702) == 0x124) ? 1 : -1];
typedef char Net6705_size_check[(sizeof(Net6705) == 0x9C) ? 1 : -1];
typedef char Net6707_size_check[(sizeof(Net6707) == 0x20A) ? 1 : -1];
typedef char Net6708_size_check[(sizeof(Net6708) == 0x308) ? 1 : -1];
typedef char Net68B0_size_check[(sizeof(Net68B0) == 0x408) ? 1 : -1];
typedef char Net6416Rec_size_check[(sizeof(Net6416Rec) == 0x10) ? 1 : -1];
typedef char Net6711Rec_size_check[(sizeof(Net6711Rec) == 0x28) ? 1 : -1];
typedef char Net6319Rec_size_check[(sizeof(Net6319Rec) == 0x124) ? 1 : -1];
typedef char NetCond6709_size_check[(sizeof(NetCond6709) == 0x44) ? 1 : -1];
typedef char NetSearch6709_size_check[(sizeof(NetSearch6709) == 0x224) ? 1 : -1];
typedef char NetCond6319_size_check[(sizeof(NetCond6319) == 0x44) ? 1 : -1];
typedef char NetSearch6319_size_check[(sizeof(NetSearch6319) == 0x124) ? 1 : -1];
typedef char Net6713_size_check[(sizeof(Net6713) == 0x3CC) ? 1 : -1];
typedef char NetPartyMember_size_check[(sizeof(NetPartyMember) == 0x158) ? 1 : -1];
typedef char NetParty_size_check[(sizeof(NetParty) == 0xE84) ? 1 : -1];
typedef char LevelEntry_size_check[(sizeof(LevelEntry) == 0x164) ? 1 : -1];
extern LevelEntry gLobbyLevel2Entries[5];  /* level 2's entries (lobby) */
typedef char NetPatchInfo_size_check[(sizeof(NetPatchInfo) == 0x1C) ? 1 : -1];
typedef char NetLoginParams_size_check[(sizeof(NetLoginParams) == 0x40) ? 1 : -1];
typedef char NetFound6709_size_check[(sizeof(NetFound6709) == 0x6EF4) ? 1 : -1];
typedef char NetFound6416_size_check[(sizeof(NetFound6416) == 0x644) ? 1 : -1];
typedef char NetFound6711_size_check[(sizeof(NetFound6711) == 0xFA4) ? 1 : -1];
typedef char NetFound6319_size_check[(sizeof(NetFound6319) == 0x7214) ? 1 : -1];
typedef char LobbyNet_size_check[(sizeof(LobbyNet) == 0x4A96C) ? 1 : -1];

extern LobbyNet gLobbyNet;
extern Net68B0 gNet68B0;       /* 0x894DAC: right after gLobbyNet */


extern LobbyPkt gLobbyPkt;     /* the message being built */
extern u8 gLobbyRecvData[];         /* the payload of the message received */


s32 NetTcp_Status(s32 sock, NetTcpStatus *st);
s32 NetTcp_Recv(s32 sock, void *buf, s16 len);

void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);
s32 NetTcp_Send(s32 sock, void *data, s16 len);
s32 Net_LockrockObfuscate(u8 *dst, u8 *src, u16 seed, s32 n, s32 key); /* unit's own declaration: with the definition's s32 (u8 *out, u8 *in, u8 seed, s32 len, s32 salt) (sub_main/unk_308BC0.c), the u8 seed changes LobbyPkt_PutBlob and LobbyPkt_PutBlob2 (lobby_net.c) by 1 of 37 words and HttpLbs_Send (nethttp) by 5 of 174: the callers pass a u16 */
u32 strlen(const void *s);

/* requests and replies */
s32 LobbyReq_6418(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
void Lobby_RouteMessage(s32 a);
s32 NetReq_Alloc(u8 state, void (*onTick)(s32), void (*onReply)(NetMsg, NetMsg *));
s32 NetReq_OnReply(NetMsg m, s32 a, s32 b);
void Net_TickRequests(void);
s32 Lobby_DispatchCommand(u16 cmd, s32 kind, u8 flag);
void Lobby_HandleMessage(void);
void LobbyPkt_6001(void);

/* receiving: each reads one field of src into dst and returns the next field */
u8 *LobbyMsg_GetU8(void *dst, u8 *src);
u8 *LobbyMsg_GetU16(void *dst, u8 *src);
u8 *LobbyMsg_GetU32(void *dst, u8 *src);
u8 *LobbyMsg_GetString(void *dst, u8 *src);
u8 *LobbyMsg_GetBytes(void *dst, u8 *src, u16 n);
u8 *LobbyMsg_GetBlob(void *dst, u16 max, u8 *src);

/* sending */
u16 LobbyPkt_BeginCmd(LobbyPkt *p, u16 cmd);
void LobbyPkt_Begin(LobbyPkt *p, u16 cmd);
void LobbyPkt_SetFlag(LobbyPkt *p, u8 flag);
void func_00719440(LobbyPkt *p, u8 v);
void LobbyPkt_End(LobbyPkt *p);
void LobbyPkt_PutU8(LobbyPkt *p, u8 v);
void LobbyPkt_PutU16(LobbyPkt *p, u16 v);
void LobbyPkt_PutU32(LobbyPkt *p, u32 v);
void LobbyPkt_PutBytes(LobbyPkt *p, void *src, u16 n);
void LobbyPkt_PutBlob(LobbyPkt *p, void *src, u16 n);
void LobbyPkt_PutBlob2(LobbyPkt *p, void *src, u16 n);
void LobbyPkt_PutBytes2(LobbyPkt *p, void *src, u16 n);
s32 LobbyPkt_Send(LobbyPkt *p);

#endif /* LOBBY_NET_H */
