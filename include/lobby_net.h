#ifndef LOBBY_NET_H
#define LOBBY_NET_H

#include "common.h"
#include "lobby_state.h"
#include "net_tcp.h"
#include "lobby_cmd.h"
#include "player.h"

/*
 * The lobby protocol: its messages, requests, tasks and routes, the lists
 * of the three levels and the records the replies fill (gLobbyNet).
 * Network code: it stays byte for byte, as the whole build does.
 */

/*
 * An outgoing message: a 12-byte header, sent from +4, then the payload.
 * The header is 82 (or 81) flag cmd(BE16) size(BE16) echo[2] 00 FF FF FF.
 */
#define LOBBY_HEADER_SIZE 12        /* the header sent before the payload */
#define LOBBY_MAGIC_CMD 0x81        /* LobbyPkt.magic of a message started from the command tables */
#define LOBBY_MAGIC_RAW 0x82        /* of one started with a raw code */
#define LOBBY_RECV_MAX 0x1000       /* the most payload bytes one receive takes (gLobbyRecvData) */
#define LOBBY_RECV_PER_FRAME 4      /* the most messages Lobby_ReceiveAll takes in one frame */

/* LobbyPkt.flag and LobbyHdr.flag (gLobbyNet.flag for the message received),
 * and gLobbyCmdFlag[] */
#define LOBBY_FLAG_REQUEST 1        /* a request: the sender waits for the reply */
#define LOBBY_FLAG_REPLY 2          /* a reply: echo is the request's sequence number */
#define LOBBY_FLAG_NOTIFY 0x10      /* a notification the server sends unasked */

/* gLobbyCmdKind[]: an entry of the command tables is a message the client
 * sends (LobbyPkt_BeginCmd takes its index) or one it receives
 * (Lobby_DispatchCommand calls its handler) */
#define LOBBY_KIND_RECV 1
#define LOBBY_KIND_SEND 8
#define LOBBY_CMD_COUNT 0x155       /* the command tables' entries */

/* The routes: a notification or a request of the server's is passed by
 * Lobby_RouteMessage(route) to the handler registered for its route
 * (Net_SetHandler: LobbyRoute_Register registers them all). Each
 * route carries one command (its code on the wire in the comment), or a few
 * of one kind. NET_ROUTE_6706, NET_ROUTE_6202, NET_ROUTE_LEVEL0_TEXT,
 * NET_ROUTE_630B, NET_ROUTE_640F, NET_ROUTE_6414, NET_ROUTE_USER_IN_KIND4 to
 * NET_ROUTE_6890, NET_ROUTE_SERVER_MESSAGE, NET_ROUTE_TOWN_CLOCK and
 * NET_ROUTE_LEVEL0_KIND to NET_ROUTE_LEVEL2_KIND have no handler
 * registered: their messages are dropped. */
#define NET_ROUTE_COUNT 0x38        /* gNetHandlers' entries */
enum NetRoute {
    NET_ROUTE_NONE = 0x00,          /* a reply only: nothing is routed */
    NET_ROUTE_PARTY_READY = 0x01,   /* 6910: the quest party is complete */
    NET_ROUTE_SERVER_CLOSE = 0x02,  /* 6003, and an error in 61A0's reply: the server closes the session */
    NET_ROUTE_SHORT_MAIL = 0x03,    /* 6705: a short mail arrived */
    NET_ROUTE_6706 = 0x04,          /* 6706, meaning unknown */
    NET_ROUTE_DEPART_CANCEL = 0x05, /* 6004: the departure for a quest is cancelled */
    NET_ROUTE_DEPART_ERROR = 0x06,  /* 6005: the same, with the server's text */
    NET_ROUTE_SERVER_LEAVE = 0x07,  /* 6007: the server sends the player out of the lobby, with a text */
    NET_ROUTE_LEVEL0_KICK = 0x08,   /* 6307: the server sends the player out of the level-0 entry */
    NET_ROUTE_LEVEL1_KICK = 0x09,   /* 640C: out of the level-1 entry (the town) */
    NET_ROUTE_LEVEL2_KICK = 0x0A,   /* 6505: out of the level-2 entry (the quest room) */
    NET_ROUTE_CHAT_MESSAGE = 0x0B,  /* 6702: a chat message */
    NET_ROUTE_CHAT_TELL = 0x0C,     /* 670C: a message to the player alone */
    NET_ROUTE_TOWN_MSG = 0x0D,      /* 6708 and 670E: a town message */
    NET_ROUTE_6202 = 0x0F,          /* 6202, meaning unknown */
    NET_ROUTE_LEVEL0_NAME = 0x10,   /* 6204 */
    NET_ROUTE_LEVEL0_VALUES = 0x11, /* 6205 */
    NET_ROUTE_LEVEL0_STATE = 0x12,  /* 6206 */
    NET_ROUTE_LEVEL0_TEXT = 0x13,   /* 620A */
    NET_ROUTE_LEVEL1_NAME = 0x14,   /* 6302 */
    NET_ROUTE_LEVEL1_VALUES = 0x15, /* 6303 */
    NET_ROUTE_LEVEL1_STATE = 0x16,  /* 6304 */
    NET_ROUTE_LEVEL1_TEXT = 0x17,   /* 6308: the recruiting text */
    NET_ROUTE_630B = 0x18,          /* 630B, meaning unknown */
    NET_ROUTE_LEVEL1_COLOR = 0x19,  /* 630D */
    NET_ROUTE_LEVEL1_USER_IN = 0x1A,  /* 6411: a user entered the town */
    NET_ROUTE_LEVEL1_USER_OUT = 0x1B, /* 6410: a user left it */
    NET_ROUTE_640F = 0x1C,          /* 640F, meaning unknown */
    NET_ROUTE_LEVEL2_NAME = 0x1D,   /* 6402 */
    NET_ROUTE_LEVEL2_VALUES = 0x1E, /* 6403 */
    NET_ROUTE_LEVEL2_STATE = 0x1F,  /* 6404 */
    NET_ROUTE_LEVEL2_TEXT = 0x20,   /* 640D */
    NET_ROUTE_LEVEL2_LIMITS = 0x21, /* 640B */
    NET_ROUTE_LEVEL2_LOCKED = 0x22, /* 6405 */
    NET_ROUTE_LEVEL2_COLOR = 0x23,  /* 650A */
    NET_ROUTE_LEVEL2_USER_IN = 0x24,  /* 6503: a user entered the quest room */
    NET_ROUTE_LEVEL2_USER_OUT = 0x25, /* 6502: a user left it */
    NET_ROUTE_LEVEL2_MEMBERS = 0x26,  /* 6412 and 6506: the room's members */
    NET_ROUTE_6414 = 0x27,          /* 6414, meaning unknown */
    NET_ROUTE_USER_IN_KIND4 = 0x29,  /* LobbyNotify_UserInOut's kind 4, which no caller passes */
    NET_ROUTE_USER_OUT_KIND5 = 0x2A, /* its kind 5, likewise */
    NET_ROUTE_6890 = 0x2B,          /* 6890, meaning unknown */
    NET_ROUTE_USER_CARD = 0x2C,     /* 6191 */
    NET_ROUTE_SERVER_MESSAGE = 0x2D, /* 6010: a text from the server */
    NET_ROUTE_USER_CARD_PART = 0x2E, /* 6193 */
    NET_ROUTE_TOWN_CLOCK = 0x2F,    /* 6892 */
    NET_ROUTE_TOWN_DATA = 0x30,     /* 6884, called directly (Net_CallHandler) */
    NET_ROUTE_LEVEL2_POSTED_QUEST = 0x31, /* 650C */
    NET_ROUTE_LEVEL1_GROUP_INFO = 0x33,   /* 631B */
    NET_ROUTE_LEVEL0_KIND = 0x34,   /* 620B */
    NET_ROUTE_LEVEL1_KIND = 0x35,   /* 630E */
    NET_ROUTE_LEVEL2_KIND = 0x36    /* 6418 */
};

/*
 * A town message's command: the first byte of a message that travels
 * on NET_ROUTE_TOWN_MSG between the members of a town (TownMsg_Send to
 * every member, TownMsg_SendTo to one, TownMsg_SendSelf to the player
 * itself), then its data. TownMsg_Recv passes each to its handler, by
 * these names.
 * - The town's table seats and the contest's seats are kept by the town
 *   host (LobbyState.hostId), and a house's seats by the house's owner: a
 *   member asks, the keeper answers yes or no to everyone (the seat and
 *   the asker's id), and the member then says it took the seat or left
 *   it.
 * - TOWN_MSG_09, TOWN_MSG_11 and TOWN_MSG_13 do nothing, and TOWN_MSG_19
 *   is read and dropped; nothing in the game sends them, nor
 *   TOWN_MSG_BGM_POS.
 */
enum TownMsgCmd {
    TOWN_MSG_WALK = 0x00,           /* a member's position and area: it walks there (TownMsg_OnMove) */
    TOWN_MSG_ACTOR = 0x01,          /* a member's state, position and look, to every member (TownMsg_OnActor) */
    TOWN_MSG_PLACE = 0x02,          /* a member's position: it is put there at once (TownMsg_OnMove) */
    TOWN_MSG_AREA = 0x03,           /* the area a member moved to (TownMsg_OnArea) */
    TOWN_MSG_SEAT_ASK = 0x04,       /* a table seat (an ActSpot) is asked of the host */
    TOWN_MSG_SEAT_ANSWER = 0x05,    /* the host's answer: yes or no, the seat, the asker's id */
    TOWN_MSG_SOBER = 0x06,          /* the member is no longer drunk */
    TOWN_MSG_SEAT_FREE = 0x07,      /* a member left its table seat (ActSpot_Release) */
    TOWN_MSG_ACTOR_TO = 0x08,       /* TOWN_MSG_ACTOR, to one member (the answer to TOWN_MSG_SYNC_ASK) */
    TOWN_MSG_09 = 0x09,             /* nothing: the only command accepted from outside the town */
    TOWN_MSG_OFFER = 0x0A,          /* a member holds out items to another (OfferMsg) */
    TOWN_MSG_OFFER_TAKE = 0x0B,     /* the taker asks for them: the giver checks and answers with TOWN_MSG_OFFER_RESULT */
    TOWN_MSG_OFFER_RESULT = 0x0C,   /* the giver's answer (OfferResultMsg) */
    TOWN_MSG_SYNC_ASK = 0x0D,       /* a newcomer asks a member for its area, state, offer, contest seat and tune */
    TOWN_MSG_NAME_NOTICE = 0x0E,    /* a member's name, shown in the chat as it arrives */
    TOWN_MSG_SEAT_TAKEN = 0x0F,     /* a member sat at the table seat the host gave it */
    TOWN_MSG_FACING = 0x10,         /* a member's facing (Player.rot[1]) */
    TOWN_MSG_11 = 0x11,             /* nothing */
    TOWN_MSG_OFFER_END = 0x12,      /* the giver's offer is over: members waiting on it go back to idle */
    TOWN_MSG_13 = 0x13,             /* nothing */
    TOWN_MSG_HOUSE_VISIT = 0x14,    /* a member went into the house of the member whose id it gives (LobbyState.memberSlot) */
    TOWN_MSG_HOUSE_SEAT_ASK = 0x15, /* a seat in a house is asked of its owner */
    TOWN_MSG_HOUSE_SEAT_ANSWER = 0x16, /* the owner's answer: yes or no, the seat, the asker's id */
    TOWN_MSG_HOUSE_SEAT_FREE = 0x17,   /* a member left its house seat (LobbyState.houseSeatBits) */
    TOWN_MSG_HOUSE_SEAT_TAKEN = 0x18,  /* a member sat in the house seat its owner gave it */
    TOWN_MSG_19 = 0x19,             /* read only by a house's guest, seated, and dropped */
    TOWN_MSG_QUEST_INFO = 0x1A,     /* the quest the party sets out on: its data, the season and the rare roll */
    TOWN_MSG_CONTEST_SEAT_ASK = 0x1B,    /* a contest seat (gTownSeats) is asked of the host */
    TOWN_MSG_CONTEST_SEAT_ANSWER = 0x1C, /* the host's answer: yes or no, the seat, the asker's id */
    TOWN_MSG_CONTEST_SEAT_FREE = 0x1D,   /* a member left its contest seat */
    TOWN_MSG_CONTEST_SEAT_TAKEN = 0x1E,  /* a member sits in a contest seat (also the answer to TOWN_MSG_SYNC_ASK) */
    TOWN_MSG_CONTEST_OPEN = 0x1F,   /* the host, both contest seats taken: each contestant gets the other's id */
    TOWN_MSG_CONTEST_POWER = 0x20,  /* a contestant's power (Contest_RecvPower) */
    TOWN_MSG_CONTEST_RESULT = 0x21, /* a contest's result (Contest_RecvResult) */
    TOWN_MSG_CONTEST_START = 0x22,  /* the contest starts (Contest_RecvStart) */
    TOWN_MSG_BGM = 0x23,            /* the town's tune and its position, played when none is (LobbyBgm_Play) */
    TOWN_MSG_SONG_ASK = 0x24,       /* a member asks the host to play a song */
    TOWN_MSG_SONG_ANSWER = 0x25,    /* the host's answer: busy or not, the song, the asker's id */
    TOWN_MSG_BGM_POS = 0x26,        /* the first tune from a position */
    TOWN_MSG_ARENA_OPEN = 0x27,     /* the arena quest is paid and the room full: the arena roles 1 and 3 choose (ArenaMenu_Open), the others wait */
    TOWN_MSG_ARENA_TEAM = 0x28,     /* the second side's monsters and item (gNextQuest.teams[1], .teamItems[1]) */
    TOWN_MSG_ARENA_TEAMS = 0x29,    /* both sides' monsters and items (gNextQuest.teams, .teamItems) */
    TOWN_MSG_ARENA_SIDES = 0x2A,    /* who is on which side (gNextQuest.sideIds) */
    TOWN_MSG_RECORDS_ASK = 0x2B,    /* asks a member for one kind of its records (TownMsg_SendRecords) */
    TOWN_MSG_RECORDS_ITEMS = 0x2C,  /* the answers: the items seen (gSave.itemBits) */
    TOWN_MSG_RECORDS_TITLES = 0x2D, /* the titles (gSave.titles) */
    TOWN_MSG_RECORDS_SIZE_MAX = 0x2E, /* the monsters' largest sizes (gSave.sizeMax) */
    TOWN_MSG_RECORDS_SIZE_MIN = 0x2F, /* their smallest (gSave.sizeMin) */
    TOWN_MSG_RECORDS_HUNTED = 0x30, /* how many of each were hunted or captured */
    TOWN_MSG_KITCHEN = 0x31,        /* the house kitchen's messages (enum KitchenMsg; TownMsg_OnKitchen) */
    TOWN_MSG_RECORDS_LEGS = 0x32,   /* the equipment made, by EquipType: EQUIP_LEGS */
    TOWN_MSG_RECORDS_HEAD = 0x33,   /* EQUIP_HEAD */
    TOWN_MSG_RECORDS_BODY = 0x34,   /* EQUIP_BODY */
    TOWN_MSG_RECORDS_ARMS = 0x35,   /* EQUIP_ARMS */
    TOWN_MSG_RECORDS_WAIST = 0x36,  /* EQUIP_WAIST */
    TOWN_MSG_RECORDS_BLADES = 0x37, /* EQUIP_BLADE */
    TOWN_MSG_RECORDS_GUNS = 0x38,   /* EQUIP_GUN */
    TOWN_MSG_POOGIE_POS = 0x39,     /* where the house's Poogie (NPC_POOGIE) stands */
    TOWN_MSG_RECORDS_WEAPON_TALLY = 0x3A, /* the weapons' tallies (Records_TallyWeapons) */
    TOWN_MSG_RECORDS_ARMOR_TALLY = 0x3B   /* the armour's (Records_TallyArmor) */
};
#define TOWN_MSG_AREAS 16   /* the town's areas a position message can name (gMsgAreaIds, Area_GetSetIndex) */

/* The data of TOWN_MSG_WALK (5 bytes sent; TownMsg_SendPosition) and of
 * TOWN_MSG_PLACE (its first 4: TownMsg_SendPlace): a member's position on
 * the ground and, for TOWN_MSG_WALK, its area as an index into
 * gMsgAreaIds (Area_GetSetIndex). TownMsg_OnMove reads both. */
typedef struct TownPosMsg {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 z;
    /* 0x4 */ u8 area;              /* below TOWN_MSG_AREAS */
} TownPosMsg;

/* The data of TOWN_MSG_OFFER and TOWN_MSG_OFFER_TAKE (14 bytes): the
 * items held out, and the other member's user id. */
typedef struct OfferMsg {
    /* 0x0 */ u16 item;
    /* 0x2 */ u16 count;
    /* 0x4 */ u8 id[8];             /* LOBBY_USER_ID_LEN bytes */
    /* 0xC */ u8 padC[2];
} OfferMsg;

/* The data of TOWN_MSG_OFFER_RESULT (14 bytes): the giver's answer to
 * TOWN_MSG_OFFER_TAKE. */
typedef struct OfferResultMsg {
    /* 0x0 */ u16 item;
    /* 0x2 */ s16 count;
    /* 0x4 */ u8 id[8];             /* the taker's user id */
    /* 0xC */ u8 refused;           /* 0: the items were given; 1: refused */
    /* 0xD */ u8 padD;
} OfferResultMsg;

typedef struct LobbyPkt {
    /* 0x00 */ u16 len;         /* payload bytes written */
    /* 0x02 */ u16 pos;         /* write position in data */
    /* 0x04 */ u8 magic;        /* LOBBY_MAGIC_CMD (LobbyPkt_BeginCmd) or LOBBY_MAGIC_RAW (LobbyPkt_Begin) */
    /* 0x05 */ u8 flag;         /* LOBBY_FLAG_*: a reply echoes the request's echo bytes */
    /* 0x06 */ u8 cmd[2];       /* big-endian */
    /* 0x08 */ u8 size[2];      /* big-endian payload size (LobbyPkt_End) */
    /* 0x0A */ u8 echo[2];      /* a request's sequence number, or the echo of a reply */
    /* 0x0C */ u8 err;          /* a reply's error byte (LobbyHdr.err at the receiver): 0, or 255 for a request refused (LobbyPkt_SetError) */
    /* 0x0D */ u8 ff[3];
    /* 0x10 */ u8 data[0x300];
} LobbyPkt;


/* A received message's header (gLobbyRecvHdr, 16 bytes; Lobby_RecvPacket reads it): the
 * outgoing header's layout (LobbyPkt from +4), its byte 8 the reply's error. */
typedef struct LobbyHdr {
    /* 0x0 */ u8 magic;         /* the kind << 4 (LOBBY_MAGIC_RAW, LOBBY_MAGIC_CMD) */
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
    s8 flag;                    /* signed: LobbyLogin_OnProgress and LobbyLevel2_OnListDone read it with lb */
    u16 n;
    u16 unk4;
} NetMsg;

/* NetMsg.kind, as the tasks and replies set it. */
enum NetMsgKind {
    NET_MSG_FAILED = -1,            /* a request or a task failed */
    NET_MSG_DONE = 0,               /* a reply, or a task done */
    NET_MSG_ROUTE = 1,              /* a route's message: flag is the route */
    NET_MSG_PROGRESS = 2            /* a task's progress: flag is a NetProgress */
};

/*
 * NetMsg.flag in the login task's and the list tasks' onDone calls says
 * what happened (seen from the senders).
 */
enum NetProgress {
    NET_PROGRESS_LOGIN_DONE = 0,    /* 6104: the login is over */
    NET_PROGRESS_CHARAS = 1,        /* 6131: the account's characters are in gLobbyNet.charas */
    NET_PROGRESS_CHARA_CHOSEN = 2,  /* 6132's reply: the character's id */
    NET_PROGRESS_PATCH_OK = 3,      /* 6125: the patch arrived whole */
    NET_PROGRESS_NOTICE = 4,        /* 6105: gLobbyNet.loginNotice is whole */
    NET_PROGRESS_CHARA_FAILED = 7,  /* 6132's reply was an error (kind -1) */
    NET_PROGRESS_PATCH_BAD = 9,     /* 6125: the patch's checksum is wrong (kind -1) */
    NET_PROGRESS_LIST_COUNT = 10,   /* a list task has its count: gLobbyNet.nReplies replies to come (kind 2) */
    NET_PROGRESS_LIST_REPLY = 11,   /* a list task got one of them (kind 2) */
    NET_PROGRESS_LOGIN_PARAMS = 12, /* 61A0's reply: gLobbyNet.loginParams */
    NET_PROGRESS_WEB_PAGE = 13      /* 68B0: gNetLoginWebPage */
};

/* a blob to send: passed by value */
typedef struct NetBlob {
    u8 kind;
    u8 pad;
    u16 n;
    void *data;
} NetBlob;

/* A request waiting for its reply (the message whose echo is seq). */
#define NET_REQ_COUNT 0x80          /* gLobbyNet.req */
#define NET_REQ2_COUNT 0x10         /* gLobbyNet.req2: the fixed slots (NetReq_TakeFixed) */
#define NET_TASK_COUNT 0x10         /* gLobbyNet.task */
/* gLobbyNet.task's slots: each runs one sequence of requests (slots 1, 10-12 and 15 are unused) */
#define NET_TASK_LOGIN 0            /* the login (NetLoginTask) */
#define NET_TASK_LEVEL0_LIST 2      /* level 0's list (NetLevel0ListTask) */
#define NET_TASK_LEVEL1_LIST 3      /* level 1's list */
#define NET_TASK_LEVEL2_LIST 4      /* level 2's list */
#define NET_TASK_LEVEL1_ALLOWED 5   /* which level-1 settings may be set */
#define NET_TASK_LEVEL2_RECS 6      /* the level-2 entry's records */
#define NET_TASK_LEVEL2_SETTINGS 7  /* the level-2 entry's settings sent */
#define NET_TASK_LEVEL1_SETTINGS 8  /* the level-1 entry's settings sent */
#define NET_TASK_PARTY 9            /* the quest party */
#define NET_TASK_DL_LIST 13         /* the files offered for download */
#define NET_TASK_DL_FILE 14         /* one file downloaded */
enum NetReqState {
    NET_REQ_FREE,                   /* 0 */
    NET_REQ_WAITING,                /* 1: sent, waiting for its reply (every starter) */
    NET_REQ_TICKING                 /* 2: waiting, and onTick runs every frame */
};
typedef struct NetReq {
    /* 0x00 */ void (*onTick)(s32 i);                   /* every frame while state is 2 */
    /* 0x04 */ void (*onReply)(NetMsg m, NetMsg *pm);
    /* 0x08 */ u8 unk8[0xE];                          /* no use seen */
    /* 0x16 */ u16 seq;
    /* 0x18 */ u8 state;                                /* NetReqState */
    /* 0x19 */ u8 unk19;                              /* cleared with the slot; no read seen */
    /* 0x1A */ u8 unk1A;                              /* cleared with the slot; no read seen */
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
enum NetTaskState {
    NET_TASK_RUNNING,               /* 0: the steps run */
    NET_TASK_DONE,                  /* 1: onDone gets kind 0 */
    NET_TASK_FAILED,                /* 2: onDone gets kind -1 */
    /* The level-2 settings and records tasks (7 and 6), whose states 0-2 are
     * stages, end on these instead. */
    NET_TASK_END_DONE = 10,
    NET_TASK_END_FAILED = 11
};
/* NetLevel2RecsTask's stages: its NetTask.state until NET_TASK_END_*. */
enum NetRecsStage {
    RECS_STAGE_COUNTS = 0,          /* 6603 the records' count, 6607 each record's sub-entry count */
    RECS_STAGE_SENDABLE = 1,        /* 6601, 6602, 660F: what may be sent */
    RECS_STAGE_FIELDS = 2           /* each record's and each sub-entry's fields */
};
enum NetTaskReply {
    NET_REPLY_NONE,                 /* 0: waiting */
    NET_REPLY_OK,                   /* 1: the reply came (kind 0) */
    NET_REPLY_ERROR                 /* 2: it reported an error */
};
typedef struct NetTask {
    /* 0x00 */ void (*func)(s32 i);                     /* every frame while active is 1 */
    /* 0x04 */ void (*onDone)(NetMsg m, NetMsg *pm);    /* kind 0: done, -1: failed */
    /* 0x08 */ u32 idx; /* the record a step works on (NetLevel2RecsTask: gLobbyNet.recs2.rec[idx], counted up); the starters set 0 or 1 */
    /* 0x0C */ u32 pos; /* the entry or offset a step asks for next (counted up; +0x200 a block for the data reads) */
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 arg;                                 /* the starter's argument: the id the requests name (NetLevel2RecsTask's LobbyReq_66xx) */
    /* 0x18 */ s32 reqFlags; /* which requests to send for each entry: NET_ENTRY_* bits, the starter's flags within the level's NET_LEVELn_REQS */
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u8 active;
    /* 0x21 */ u8 state;                                /* NetTaskState */
    /* 0x22 */ u8 step;
    /* 0x23 */ u8 reply;                                /* NetTaskReply, set by its reply callback */
    /* 0x24 */ u8 reqCount; /* requests per entry (the bits of reqFlags counted by the starter): nReplies = entries * reqCount; a step waits for that many free requests */
    /* 0x25 */ u8 pad25[3];
} NetTask;

/* the values of an entry: the replies to 6312, 6313 and 6314 */
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

/* 6892's reply (LobbyReply_TownClock): five u32, as many u32 as the fifth says
 * (at most 12), one more u32 */
typedef struct NetTownClock {
    /* 0x00 */ u32 time;             /* the lobby clock in ticks (gLobbyTime = LobbyWorld_TicksToSeconds) */
    /* 0x04 */ u32 dayTicks;         /* gTownDayTicks */
    /* 0x08 */ u32 day;              /* gTownDay */
    /* 0x0C */ u32 season;           /* gTownSeason */
    /* 0x10 */ s32 n;
    /* 0x14 */ u32 list[12];
    /* 0x44 */ u32 year;             /* gTownYear */
} NetTownClock;

/*
 * An entry of one of the three levels' lists (gLobbyNet.list0/list1/list2),
 * indexed by the server's id - 1. flags says which of the entry's requests were answered
 * (NET_ENTRY_*); a list task (NetLevel0ListTask-4) sends the requests its caller's
 * flags ask for, within its level's mask, and is done when all are answered.
 */
#define NET_ENTRY_VALUES 0x001      /* users .. unk10 (6205, 6303, 6403) */
#define NET_ENTRY_STATE 0x002       /* state (6206, 6304, 6404) */
#define NET_ENTRY_NAME 0x004        /* name (6204, 6302, 6402) */
#define NET_ENTRY_TEXT 0x008        /* text (620A, 6308, 640D) */
#define NET_ENTRY_LOCKED 0x010      /* locked (level 1: 630B; level 2: 6405) */
#define NET_ENTRY_LIMITS 0x020      /* level 2: maxUsers and unk12-unk18 (640B) */
#define NET_ENTRY_MEMBERS 0x040     /* level 2: roomMembers (6412) */
#define NET_ENTRY_COLOR 0x080       /* color (630D, 650A) */
#define NET_ENTRY_POSTED_QUEST 0x100 /* level 2: postedQuest (650C) */
#define NET_ENTRY_GROUP_INFO 0x200  /* level 1: groupInfo (631B) */
#define NET_ENTRY_KIND 0x400        /* kind (620B, 630E, 6418) */
#define NET_LEVEL0_REQS (NET_ENTRY_VALUES | NET_ENTRY_STATE | NET_ENTRY_NAME | NET_ENTRY_TEXT | NET_ENTRY_KIND)
/* level 1's mask leaves out NET_ENTRY_LOCKED: its 630B is never asked for */
#define NET_LEVEL1_REQS (NET_LEVEL0_REQS | NET_ENTRY_COLOR | NET_ENTRY_GROUP_INFO)
#define NET_LEVEL2_REQS (NET_LEVEL0_REQS | NET_ENTRY_LOCKED | NET_ENTRY_LIMITS | NET_ENTRY_MEMBERS \
                         | NET_ENTRY_COLOR | NET_ENTRY_POSTED_QUEST)
typedef struct NetListEntry {
    /* 0x000 */ u32 flags;
    /* 0x004 */ s32 color;          /* level 1 (630D): the name colour (Text_SetColor of its low half); level 2: 650A */
    /* 0x008 */ u16 id;
    /* 0x00A */ u16 users;           /* AreaList_DrawEntries draws users/maxUsers */
    /* 0x00C */ u16 questUsers;     /* users out of town: the level-1 list shows users - this as in town */
    /* 0x00E */ u16 maxUsers;        /* level 2: also the first value of 640B's reply */
    /* 0x010 */ u16 unk10;
    /* 0x012 */ u16 unk12;
    /* 0x014 */ u16 unk14;
    /* 0x016 */ u16 unk16;
    /* 0x018 */ u16 unk18;
    /* 0x01A */ u16 roomMembers;    /* 6412 (level 2): NetList2_GetMembers gives gLobby->roomMembers */
    /* 0x01C */ u16 unk1C;
    /* 0x01E */ u8 state;           /* 6206, 6304, 6404: 3 and 4 open (LevelEntry.state) */
    /* 0x01F */ u8 locked;          /* level 2: a password is set (6405); level 1: 630B's byte */
    /* 0x020 */ char name[0x44];
    /* 0x064 */ char text[0x101];
    /* 0x165 */ u8 kind;              /* 630E, 6418, 620B (NetList0_GetKind: level 0's kind) */
    /* 0x166 */ u8 unk166[2];
    /* 0x168 */ u8 postedQuest[0x20]; /* 650C (level 2): the posted quest, copied to gPostedQuests */
    /* 0x188 */ u8 groupInfo[4];    /* 631B (level 1): [0] the group limit kind, [1] the quest target */
} NetListEntry;

/* a file the server offers for download (6883's reply: u8 index, u32 a, u32 b) */
typedef struct NetDlFile {
    /* 0x0 */ u32 size;
    /* 0x4 */ u32 stamp;            /* a version or date the server gives with it */
} NetDlFile;

/* The downloaded quest files in the lobby work buffer (gLobbyWorkBuf): the
 * QDL_FILE_COUNT files' records as last downloaded (NetDlFile; file 0 is
 * the quest list, files 1-3 the quests) at +QDL_OFFSET, the list file right
 * after them (RoomData; QDL_LIST_SIZE bytes are cleared for it), and the
 * quests in gQuestFiles (sub_main), QUEST_FILE_SIZE bytes each. The list's
 * word at +0xB88 holds its event bits. */
#define QDL_OFFSET 0x6EF4
#define QDL_FILE_COUNT 4
#define QDL_LIST_SIZE 0x11C4
#define QUEST_FILE_SIZE 0x300
#define QDL_SAVED ((NetDlFile *)(*(s32 *)&gLobbyWorkBuf + QDL_OFFSET))
#define QDL_LIST_FILE ((u8 *)(QDL_SAVED + QDL_FILE_COUNT))
#define QDL_LIST_EVENT_BITS (*(u16 *)(QDL_LIST_FILE + 0xB88))
/* The list file's RoomData.areaKindHelp[kind] (at 0x27C), its offsets
 * added one by one as the game's code adds them: the member's address
 * through QDL_LIST_FILE compiles differently. */
#define QDL_AREA_KIND_HELP(kind)     ((u8 *)((s32)(gLobbyWorkBuf + QDL_OFFSET) + QDL_FILE_COUNT * sizeof(NetDlFile)) + ((kind) << 8) + 0x27C)

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

/* 631A's reply (LobbyReply_Level1SearchInfoRead): a second-level entry with up to 10 users */
typedef struct NetMember10 {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x14];
    /* 0x1C */ u16 hr;              /* the hunter rank (bytes 0-1 of a 16-byte blob, little-endian) */
    /* 0x1E */ u8 icon;             /* its byte 3: an icon kind */
    /* 0x1F */ u8 pad1F;
} NetMember10;

typedef struct NetLevel1SearchInfo {
    /* 0x000 */ char serverId[0x10]; /* the land (server) the group is on: compared with gCurServerId, named by LobbySrv_Name */
    /* 0x010 */ u16 level0Id;        /* the level-0 entry it is in */
    /* 0x012 */ u16 level1Id;        /* its own id */
    /* 0x014 */ char level0Name[0x44];
    /* 0x058 */ char level1Text[0x44]; /* the recruiting text */
    /* 0x09C */ u8 groupInfo[4];     /* as NetListEntry.groupInfo */
    /* 0x0A0 */ u16 unkA0[4];
    /* 0x0A8 */ u16 n;              /* users (at most 10) */
    /* 0x0AA */ u16 unkAA;
    /* 0x0AC */ u16 unkAC;
    /* 0x0AE */ u8 padAE[2];
    /* 0x0B0 */ char textB0[0x104];
    /* 0x1B4 */ NetMember10 users[10];
} NetLevel1SearchInfo;

/* the settings task 8 sends for the second-level entry the player is in
 * (gLobbyNet.settings): 6315 name, 6316 password, 6317 text, 6318 four bytes */
typedef struct NetEntrySettings {
    /* 0x000 */ char name[0x41];
    /* 0x041 */ char pass[9];
    /* 0x04A */ char text[0x101];
    /* 0x14B */ u8 groupInfo[4];     /* 6318 (LevelEntry.groupInfo) */
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

/* a third-level entry record of task 6 (5285 bytes): a name, a few bytes,
 * and up to 32 sub-entries, each with a name and up to 32 three-byte values */
typedef struct NetRec2 {
    /* 0x0000 */ u8 flags;          /* 1 6604, 2 6606, 8 6605, 4 6607 arrived */
    /* 0x0001 */ char name[0x41];   /* 6604 */
    /* 0x0042 */ u8 editable;       /* 6605; 1: the player sets it (task 7 sends settings2.b[i]) */
    /* 0x0043 */ u8 subCount;       /* 6607: how many sub-entries are used */
    /* 0x0044 */ u8 defaultChoice;  /* 6606: the value chosen at first */
    /* 0x0045 */ u8 subFlags[0x20]; /* 1 6608, 2 660E arrived */
    /* 0x0065 */ char subName[0x20][0x41];  /* 6608 */
    /* 0x0885 */ u8 subN[0x20];     /* 660E: the values */
    /* 0x08A5 */ u8 subData[0x20][0x20][3]; /* 660E */
} NetRec2;

/* the third level's entry records (gLobbyNet.recs2, 169124 bytes; NetLevel2RecsTask_GetRecs
 * copies them out): what the server allows to be sent, and the records */
typedef struct NetRecs2 {
    /* 0x00000 */ u8 sendName;      /* 1: the name may be sent (a u8 reply) */
    /* 0x00001 */ u8 sendPass;
    /* 0x00002 */ u8 sendText;
    /* 0x00003 */ u8 n;             /* records */
    /* 0x00004 */ NetRec2 rec[32];
} NetRecs2;

/* a range of a user's hunter card (the notification 6193, 6712's reply) */
typedef struct NetUserCardPart {
    /* 0x000 */ char id[8];
    /* 0x008 */ char name[0x14];
    /* 0x01C */ u8 data[0x100];
    /* 0x11C */ u16 n;              /* bytes in data */
    /* 0x11E */ u16 offset;         /* where they go in the card */
    /* 0x120 */ u8 pad120[2];
} NetUserCardPart;

/* the server's 6105 at login: a text, sent in parts of 4096 bytes */
typedef struct NetLoginNotice {
    /* 0x0000 */ u8 unk0;
    /* 0x0001 */ u8 pad1;
    /* 0x0002 */ u16 unk2;
    /* 0x0004 */ char text[0x1000];
} NetLoginNotice;

/* 614C's reply: a text, in parts too */
typedef struct NetLoginInfo {
    /* 0x0000 */ u8 unk0;
    /* 0x0001 */ u8 pad1[3];
    /* 0x0004 */ char text[0x1000];
} NetLoginInfo;

/* the paged search results (a request's reply comes in pages: u8 total, u8
 * first, u8 count, u8 last, then count records; while last is 0 the client
 * asks for the next page with the matching notification, u8 first) */
typedef struct Net6416Rec {
    /* 0x00 */ u8 b[4];
    /* 0x04 */ u16 h[4];
    /* 0x0C */ u8 c;
    /* 0x0D */ u8 pad[3];
} Net6416Rec;

typedef struct NetUserFindByNameRec {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x11];
    /* 0x19 */ char text[0xF];
} NetUserFindByNameRec;

#define LEVEL1_SEARCH_MAX_FOUND 100 /* the records a search reply holds at most */
typedef struct NetLevel1SearchRec {
    /* 0x000 */ char text[0x104];
    /* 0x104 */ char serverId[0x10]; /* the group's land (server): LobbySrv_Name */
    /* 0x114 */ u8 groupInfo[4];    /* as NetListEntry.groupInfo: [0] the limit (GROUP_LIMIT_*), [1] the target quest */
    /* 0x118 */ u16 h118;
    /* 0x11A */ u16 h11A;
    /* 0x11C */ u16 users;
    /* 0x11E */ u16 maxUsers;
    /* 0x120 */ u16 usersInTown;
    /* 0x122 */ u8 index;           /* its place in the whole list */
    /* 0x123 */ u8 pad123;
} NetLevel1SearchRec;

/* the results of a search as the pages of its reply fill them (a count,
 * then up to 100 records); a new request clears them (memset of the
 * whole struct: 28404 for 6709, 4004 for 6711) */
typedef struct NetUserSearchFound {
    /* 0x0000 */ u8 total;              /* the users found */
    /* 0x0001 */ u8 pad1[3];
    /* 0x0004 */ NetUser users[100];
} NetUserSearchFound;

typedef struct NetFound6416 {
    /* 0x000 */ u8 total;
    /* 0x001 */ u8 pad1[3];
    /* 0x004 */ Net6416Rec list[100];
} NetFound6416;

typedef struct NetUserFindByNameFound {
    /* 0x000 */ u8 total;
    /* 0x001 */ u8 pad1[3];
    /* 0x004 */ NetUserFindByNameRec list[100];
} NetUserFindByNameFound;

typedef struct NetLevel1SearchFound {
    /* 0x0000 */ u8 total;
    /* 0x0001 */ u8 pad1[3];
    /* 0x0004 */ NetLevel1SearchRec list[LEVEL1_SEARCH_MAX_FOUND];
} NetLevel1SearchFound;

/* gLobbyNet.level1Search and the records after it, as LobbyNet_GetLevel1Search
 * hands them out (gGroupSearchResults) */
typedef struct NetLevel1SearchReply {
    /* 0x0000 */ u32 head;          /* the reply's first value */
    /* 0x0004 */ NetLevel1SearchFound found;
} NetLevel1SearchReply;

/* a search's conditions (6709: up to 8, 6319: up to 4) */
typedef struct NetUserSearchCond {
    /* 0x00 */ u8 type;             /* 0-6: what data holds (a 6-byte id, a string, bytes) */
    /* 0x01 */ u8 n;
    /* 0x02 */ u8 pad2[2];
    /* 0x04 */ u8 data[0x40];
} NetUserSearchCond;

typedef struct NetUserSearchQuery {
    /* 0x000 */ u8 kind;            /* sent as 100 (LobbyUser_Search) */
    /* 0x001 */ u8 n;
    /* 0x002 */ u8 pad2[2];
    /* 0x004 */ NetUserSearchCond cond[8];
} NetUserSearchQuery;

typedef struct NetLevel1SearchCond {
    /* 0x00 */ u8 type;
    /* 0x01 */ u8 n;
    /* 0x02 */ u8 values[0x42];   /* the values matched (LEVEL1_SEARCH_ANY: any) */
} NetLevel1SearchCond;
#define LEVEL1_SEARCH_COND_LIMIT 0  /* NetLevel1SearchCond.type: the group's limit, GROUP_LIMIT_* */
#define LEVEL1_SEARCH_COND_TARGET 1 /* the group's target quest */
#define LEVEL1_SEARCH_ANY 0xFF      /* a condition's value that matches any */

typedef struct NetLevel1SearchQuery {
    /* 0x000 */ u8 id[0x10];        /* 12 bytes sent */
    /* 0x010 */ u8 maxFound;      /* the most results wanted (LEVEL1_SEARCH_MAX_FOUND) */
    /* 0x011 */ u8 n;               /* the conditions */
    /* 0x012 */ u8 pad12[2];
    /* 0x014 */ NetLevel1SearchCond cond[4];
} NetLevel1SearchQuery;

/* 6713's reply: a user's details, where the user is and the group there
 * (the hunter info shows them; the lines are those the online window shows
 * for the player's own place). */
typedef struct NetUserProfile {
    /* 0x000 */ u8 name[0x14];      /* the user's name */
    /* 0x014 */ u8 serverId[0x10];  /* the server the user is on (12 bytes sent; LobbySrv_Exists, LobbySrv_IsCur) */
    /* 0x024 */ u8 location[0x104]; /* the server's location (as LobbySrv_CurName) */
    /* 0x128 */ u8 serverName[0x104]; /* the server's name (as LobbySrv_MyName), drawn with one mark in serverColor */
    /* 0x22C */ u8 areaName[0x44];  /* the level-0 entry's name, when area isn't 0 */
    /* 0x270 */ u8 landName[0x44];  /* the level-1 entry's name, when land isn't 0 */
    /* 0x2B4 */ u8 message[0x104];  /* the group's recruiting message */
    /* 0x3B8 */ u16 area;           /* the level-0 entry (as gLobbyLevel0Sel); 0: none */
    /* 0x3BA */ u16 land;           /* the level-1 entry (as gLobbyLevel1Sel); 0: none */
    /* 0x3BC */ u16 members;        /* the group's members */
    /* 0x3BE */ u16 maxMembers;     /* the most it takes */
    /* 0x3C0 */ u8 groupInfo[4];    /* as LevelEntry.groupInfo: [0] the limit (GROUP_LIMIT_*), [1] the target quest */
    /* 0x3C4 */ u32 serverColor;    /* the server's mark colour */
    /* 0x3C8 */ u8 status;          /* PROFILE_STATUS_JOINABLE: the user can be joined */
    /* 0x3C9 */ u8 areaKind;        /* the level-0 entry's kind, 1-4 (LevelEntry.kind) */
    /* 0x3CA */ u8 unk3CA;          /* read from the reply; no use seen */
    /* 0x3CB */ u8 pad3CB;
} NetUserProfile;
/* NetUserProfile.status: the only one Group_CanJoin accepts (a level entry's
 * open state, LEVEL_ENTRY_OPEN, has the same number) */
#define PROFILE_STATUS_JOINABLE 3

/* The quest party task 9 fetches (69xx): 6911 the number of players, 6912 a
 * byte (less one), 6913 and 6917 for each player, 6915 the room's name, 6914 a
 * text, then 6916 the address of the quest server. LobbyRoom_FetchParty copies the
 * players into LobbyState.players and sub_main's tables; the connection then
 * goes to the 6916 address (the quest network's 1031 challenge, QNetIf_Start). */
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

/* what the connection was started with (NetLoginTask_Start, 64 bytes; LobbyEntry_Start
 * fills it from sub_main's account data) */
typedef struct NetLoginParams {
    /* 0x00 */ u8 type;             /* 1; nonzero: 6103 is answered with pings (600E) */
    /* 0x01 */ s8 entry;            /* the network mode's entry (gNetMode.entry) */
    /* 0x02 */ char id[0xB];        /* the account id, 10 characters (sub_main 0x5CAEE1) */
    /* 0x0D */ char pass[0x13];     /* 16 characters (sub_main 0x5CAED0), sent obfuscated */
    /* 0x20 */ u8 *patchBuf;        /* where a patch sent at login goes (sub_main 0x6AE018) */
    /* 0x24 */ s32 unk24[7];
} NetLoginParams;

/* LevelEntry.groupInfo[0] of a level-1 entry: who may enter it
 * (LobbyLevel1_CanEnter) */
#define GROUP_LIMIT_NONE 0          /* everyone */
#define GROUP_LIMIT_HR_BELOW_31 1   /* hunter ranks below 31 only */
#define GROUP_LIMIT_HR_31_UP 2      /* hunter rank 31 and above only */
#define GROUP_LIMIT_SHORT_TEXT 3    /* gGroupLimitTexts[limit + this]: the limit's short form */

/* An entry of one of the three levels as the lobby screens keep it (356
 * bytes; level 0 in sub_main's gLobbyLevel0Entries, level 1 in gLobbyLevel1Entries, level 2 in
 * gLobbyLevel2Entries): copied out of gLobbyNet.list0-2 by the NetListN_Get* getters. */
typedef struct LevelEntry {
    /* 0x000 */ s32 color;          /* level 1 (630D): the name colour (Text_SetColor of its low half); level 2: 650A */
    /* 0x004 */ u16 id;
    /* 0x006 */ u16 users;
    /* 0x008 */ u16 maxUsers;       /* level 2: 640B's first value */
    /* 0x00A */ u16 unkA[4];        /* level 2: 640B's other values */
    /* 0x012 */ u16 questUsers;     /* users out of town (users - this: in town) */
    /* 0x014 */ u16 unk14;          /* NetListEntry.unk10 */
    /* 0x016 */ u8 state;           /* 6206, 6304, 6404: 3 and 4 open (enterable) */
    /* 0x017 */ u8 locked;          /* level 2: a password is set (NetList2_GetLocked) */
    /* 0x018 */ u8 pad18;
    /* 0x019 */ u8 textAllowed;     /* level 1 (6314): whether the entry's text may be set */
    /* 0x01A */ u8 kind;             /* level 0: 1-4 (NetList0_GetKind) */
    /* 0x01B */ u8 groupInfo[4];    /* level 1 (631B): [0] the group limit, GROUP_LIMIT_* (gGroupLimitTexts[k + 3]), [1] the quest target */
    /* 0x01F */ char name[0x41];
    /* 0x060 */ char text[0x104];
} LevelEntry;
#define LEVEL_ENTRY_FREE 1          /* LevelEntry.state of a level-1 entry no group holds: it can be opened as a new group */
/* The level-1 entry of id `id` (from 1), its address computed as bytes
 * (LobbyLevel1_CanEnter): indexed as an array, the compiler adds the
 * field's offset to the table's address instead. */
#define LEVEL1_ENTRY(id) ((LevelEntry *)((u8 *)gLobbyLevel1Entries + ((id) - 1U) * sizeof(LevelEntry)))
#define LEVEL_ENTRY_OPEN 3          /* LevelEntry.state of an area that can be entered (AreaList_Input) */
#define LEVEL_ENTRY_OPEN_ALT 4      /* also open (enterable); what tells it from LEVEL_ENTRY_OPEN isn't known */
#define LEVEL0_ENTRY_MAX 25         /* gLobbyLevel0Entries: the areas a server lists at most */
#define LEVEL1_ENTRY_MAX 45         /* gLobbyLevel1Entries: its entries, cleared together when the village starts */
#define LEVEL2_ENTRY_MAX 5          /* gLobbyLevel2Entries: a town's quest rooms (ids 1-5) */

/* A quest of a room's list in the room's data (24 bytes). */
typedef struct RoomQuest {
    /* 0x00 */ char name[0x14];
    /* 0x14 */ u8 used;             /* 0: no quest in this entry */
    /* 0x15 */ u8 unlockKind;       /* enum RoomQuestUnlock: what opens it to the player (GroupBoard_QuestAllowed) */
    /* 0x16 */ u8 unlockArg1;       /* ROOM_QUEST_UNLOCK_HR: the hunter rank; with a monster hunted: the monster's gSave.hunted slot */
    /* 0x17 */ u8 unlockArg2;       /* ROOM_QUEST_UNLOCK_HUNTED_HR: the hunter rank */
} RoomQuest;
/* RoomQuest.unlockKind */
enum RoomQuestUnlock {
    ROOM_QUEST_UNLOCK_NONE = 0,             /* always open */
    ROOM_QUEST_UNLOCK_HR = 1,               /* a hunter rank of unlockArg1 */
    ROOM_QUEST_UNLOCK_HUNTED_HR = 2,        /* monster unlockArg1 hunted and a hunter rank of unlockArg2 */
    ROOM_QUEST_UNLOCK_ARENA_CUP = 3,        /* EVENT_FLAG_ARENA_CUP_OPEN */
    ROOM_QUEST_UNLOCK_ELDER_RAID = 4,       /* the elder raid running (ElderRaid_IsOn) */
    ROOM_QUEST_UNLOCK_SONG_TRACK_3 = 5,     /* EVENT_FLAG_SONG_TRACK_3 */
    ROOM_QUEST_UNLOCK_SONG_TRACK_3_HUNTED = 6 /* EVENT_FLAG_SONG_TRACK_3 and monster unlockArg1 hunted */
};
/* A recruiting slot of the room's data (the group board). */
typedef struct RoomSlot {
    /* 0x00 */ u8 unk0[8];
    /* 0x08 */ char text[0x18];     /* the recruiting text; empty when the slot is free */
} RoomSlot;
#define ROOM_SLOTS 16
#define GROUP_BOARD_SLOTS 11    /* the slots GroupBoard_Open counts */
/* The room's data, 32 bytes into the block at gLobbyWorkBuf + 0x6EF4: its
 * quest list (LevelEntry.groupInfo[1] indexes it). */
#define ROOM_QUESTS 64              /* RoomData.quests: the quests a room offers */
typedef struct RoomData {
    /* 0x0000 */ RoomSlot slots[ROOM_SLOTS];
    /* 0x0200 */ u8 unk200[8];
    /* 0x0208 */ s8 slotColor[ROOM_SLOTS];  /* each slot's text colour (LobbyStep_Level1SetColor sends it; Text_SetColor draws it) */
    /* 0x0218 */ s8 slotLocked[ROOM_SLOTS]; /* nonzero: the slot's text can't be picked */
    /* 0x0228 */ u8 unk228[4];
    /* 0x022C */ s8 slotKinds[4][ROOM_SLOTS]; /* by area kind (LevelEntry.kind - 1): nonzero where the slot's text is offered */
    /* 0x026C */ s16 areaKindHr[4][2]; /* each area kind's (LevelEntry.kind 1-4) hunter rank range: the lowest, the highest (Lobby_CanEnterByRank) */
    /* 0x27C */ char areaKindHelp[4][0x100]; /* the area list's help line for each kind of area (AreaGroup.kind) */
    /* 0x67C */ u8 unk67C[0x540];
    /* 0x0BBC */ RoomQuest quests[ROOM_QUESTS];
    /* 0x11BC */ u16 unk11BC;
    /* 0x11BE */ u16 arenaFormRows; /* bits 1-3: the arena cup's entry form shows its rows ARENA_FORM_SETTING_1 to _3 (ArenaCup_IsFormRowOpen) */
    /* 0x11C0 */ u8 unk11C0[4];
} RoomData;
typedef char RoomData_size_check[(sizeof(RoomData) == QDL_LIST_SIZE) ? 1 : -1];
/* The room's data in the lobby work buffer (QDL_LIST_FILE), its address
 * formed as the group board's code forms it. */
#define QDL_ROOM ((RoomData *)((s32)(gLobbyWorkBuf + QDL_OFFSET) + QDL_FILE_COUNT * sizeof(NetDlFile)))

/* the player's answer to the server's request 6103 (LobbyPkt_ClientInfo: u8 x3,
 * blob (10 bytes), u16 x8), set with LobbyNet_SetClientInfo */
typedef struct NetClientInfo {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 pad3;
    /* 0x04 */ char version[0x10];  /* gVersionString (10 characters sent) */
    /* 0x14 */ u16 state[8];        /* [0..3] Net_GetDeviceState, [4] the average round trip (600E), [5..7] 0 */
} NetClientInfo;

/* the client's answer to the server's 6138 (LobbyPkt_6138), kept in
 * gLobby6138Data, which only the answer reads and the login clears */
typedef struct Net6138Answer {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 pad11[3];
    /* 0x14 */ u16 unk14[8];
} Net6138Answer;

/* a chat message: the notifications 6702 and 670C (blob id 8, blob name
 * 16, blob text 256, u8 x4 the colour, u8 kind) */
typedef struct NetChatMessage {
    /* 0x000 */ NetUserRec r;
    /* 0x120 */ u8 kind;            /* ChatMsg.kind: 1 the log, 2 a balloon (LobbyPkt_ChatSay's kind) */
    /* 0x121 */ u8 pad121[3];
} NetChatMessage;

/* 6705's reply: three strings (a user's id, name and a text) */
typedef struct NetShortMail {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x14];
    /* 0x1C */ char text[0x80];
} NetShortMail;

/* 6707's reply: u16 x3, u8 (LobbyReply_UserProfileRead clears all 522 bytes) */
typedef struct NetUserStatus {
    /* 0x000 */ u16 a;
    /* 0x002 */ u16 b;
    /* 0x004 */ u16 c;
    /* 0x006 */ u8 pad6;
    /* 0x007 */ u8 d;
    /* 0x008 */ u8 unk8[0x202];
} NetUserStatus;

/* a chat message: the notifications 6708 and 670E (blob id 8, blob text 768) */
typedef struct NetTownMsg {
    /* 0x000 */ char id[8];
    /* 0x008 */ u8 text[0x300];
} NetTownMsg;

/* the notification 68B0 at login: blob text (256), u16 n, then n bytes
 * (blob, at most 768) if n (the global right after gLobbyNet) */
typedef struct NetLoginWebPage {
    /* 0x000 */ char url[0x100];    /* the page the lobby opens (gLobbyWebUrl) */
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ u8 text[0x300];     /* a text shown before the page opens */
    /* 0x404 */ u16 secs;           /* how long the text is shown, in seconds (0: no text) */
    /* 0x406 */ u8 pad406[2];
} NetLoginWebPage;

/* the notification 6891: three u16 (LobbyNet_GetCurrentPlace) */
typedef struct NetCurrentPlace {
    u16 v[3];
} NetCurrentPlace;

/* The connection state (gLobbyNet, 0x84A440): one large object; the code
 * reaches its fields at offsets from gLobbyNet (the D_0084xxxx-D_0088xxxx
 * symbols in the C are fields of it too). */
typedef struct LobbyNet {
    /* 0x0000 */ s32 connected; /* 1 while connected (set by the connecting step at 0x7069B0); Lobby_ReceiveAll reads nothing while 0 */
    /* 0x0004 */ u8 unk4[4];
    /* 0x0008 */ s32 unk8;
    /* 0x000C */ u8 unkC[4];
    /* 0x0010 */ s16 sentKeepalive; /* 1 once 6001 was sent (LobbySrvReq_Keepalive) */
    /* 0x0012 */ u8 unk12[2];
    /* 0x0014 */ u8 unk14;
    /* 0x0015 */ u8 unk15[3];
    /* 0x0018 */ NetReq req[NET_REQ_COUNT];
    /* 0x0E18 */ NetTask task[NET_TASK_COUNT];
    /* 0x1098 */ NetReq req2[NET_REQ2_COUNT];
    /* 0x1258 */ u8 unk1258[0x14];
    /* 0x126C */ s32 sock;
    /* 0x1270 */ u32 recvResult;    /* Lobby_RecvPacket; 1 the last part, >= 2 more follow; nethttp reads the bytes left */
    /* 0x1274 */ u16 cmd;           /* of the message received */
    /* 0x1276 */ u16 flag;
    /* 0x1278 */ u16 echo;
    /* 0x127A */ u16 size;
    /* 0x127C */ s8 replyErr;       /* nonzero: the reply received reports an error */
    /* 0x127D */ s8 reqState;       /* the HTTP request state (nethttp http_front.c) */
    /* 0x127E */ u16 key;           /* Net_LockrockObfuscate's key */
    /* 0x1280 */ s32 rxLen;         /* bytes gathered in rxBuf: a long reply's parts are appended (memcpy(rxBuf + rxLen, gLobbyRecvData, len)); 0 at the start */
    /* 0x1284 */ s32 rxSize;        /* rxBuf's size (4096); a part is cut to rxSize - rxLen - 1 */
    /* 0x1288 */ u8 *rxBuf;         /* where a long reply is gathered (rxStore) */
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
    /* 0x1420 */ NetClientInfo clientInfo;     /* the answer to the server's 6103 (LobbyNet_SetClientInfo) */
    /* 0x1444 */ u8 unk1444[0x1D6];
    /* 0x161A */ NetEntrySettings settings; /* task 8's */
    /* 0x1769 */ NetEntrySettings2 settings2; /* task 7's */
    /* 0x18D8 */ u8 nCharas;        /* the account's characters (6131, at most 3) */
    /* 0x18D9 */ u8 pad18D9[2];
    /* 0x18DB */ NetUser charas[4]; /* 6131's; charas[3]: the one playing (LobbyNet_SetMe sets its id and name) */
    /* 0x1D4B */ NetUser self;      /* the character's id (6132's reply) and name */
    /* 0x1E67 */ NetUser userCard;
    /* 0x1F83 */ u8 pad1F83;
    /* 0x1F84 */ NetUserCardPart userCardPart;
    /* 0x20A6 */ NetUserCardPart userCardFetch;
    /* 0x21C8 */ NetLoginInfo loginInfo;
    /* 0x31CC */ NetLoginNotice loginNotice;
    /* 0x41D0 */ NetUser userInOut; /* the user entering or leaving (LobbyNotify_UserInOut) */
    /* 0x42EC */ NetUserRec users[10]; /* 630A's reply */
    /* 0x4E2C */ NetUserRec users2[10]; /* 640A's reply */
    /* 0x596C */ u8 unk596C[0xB40];
    /* 0x64AC */ NetUserRec user6414; /* cleared by LobbyNotify_6414 (which then fills users2[0]) */
    /* 0x65CC */ u16 count0;        /* list0's entries (6203's reply; at most 25) */
    /* 0x65CE */ u16 count1;        /* list1's entries (6301's reply; at most 45) */
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
    /* 0x37E6C */ NetShortMail shortMail;
    /* 0x37F08 */ NetUserStatus userStatus;
    /* 0x38112 */ u8 pad38112[2];
    /* 0x38114 */ NetUserProfile userProfile;
    /* 0x384E0 */ NetChatMessage chatMessage;    /* the last chat message (6702, 670C) */
    /* 0x38604 */ NetTownMsg townMsg;    /* a chat message (6708, 670E) */
    /* 0x3890C */ u8 errText[0x300];  /* the error text a reply carries (LobbyMsg_ReadErrText) */
    /* 0x38C0C */ u8 rxStore[0x2000]; /* rxBuf points here */
    /* 0x3AC0C */ u8 dlCount;       /* files offered (6881's reply) */
    /* 0x3AC0D */ u8 dlFile;        /* the file task 14 downloads */
    /* 0x3AC0E */ u8 pad3AC0E[2];
    /* 0x3AC10 */ u8 *dlBuf;        /* where it goes */
    /* 0x3AC14 */ u32 dlSize;
    /* 0x3AC18 */ u32 dlGot;        /* bytes received (6882's replies) */
    /* 0x3AC1C */ NetDlFile dlFiles[32];
    /* 0x3AD1C */ NetUserSearchFound userSearchFound; /* 6709's search: the users found */
    /* 0x41C10 */ u16 searchReq;    /* the request the pages of a search answer (req[]) */
    /* 0x41C12 */ NetFound6416 found6416;
    /* 0x42256 */ NetInfo6417 x6417;
    /* 0x423E6 */ NetUserFindByNameFound userFindByNameFound;
    /* 0x4338A */ u8 pad4338A[2];
    /* 0x4338C */ u32 level1Search;
    /* 0x43390 */ NetLevel1SearchFound level1SearchFound;
    /* 0x4A5A4 */ NetLevel1SearchInfo level1SearchInfo;
    /* 0x4A898 */ u32 x6890;
    /* 0x4A89C */ NetCurrentPlace currentPlace;
    /* 0x4A8A2 */ u8 pad4A8A2[2];
    /* 0x4A8A4 */ u32 loginParams[2];     /* 61A0's reply at login */
    /* 0x4A8AC */ u8 x68A0n;
    /* 0x4A8AD */ u8 pad4A8AD[3];
    /* 0x4A8B0 */ char *x68A1Str;
    /* 0x4A8B4 */ u8 x68A0[4];
    /* 0x4A8B8 */ u8 unk4A8B8[8];
    /* 0x4A8C0 */ u8 *x6884Buf;     /* 6884's reply to an ordinary request goes here */
    /* 0x4A8C4 */ s32 townDataMax;
    /* 0x4A8C8 */ u8 townDataSlot;     /* the slot byte of that reply */
    /* 0x4A8C9 */ u8 pad4A8C9[3];
    /* 0x4A8CC */ void *slotBuf[16];    /* the buffers 6885's notice has 6884 fill */
    /* 0x4A90C */ s32 slotMax;
    /* 0x4A910 */ u8 unk4A910[2];
    /* 0x4A912 */ s16 unk4A912;
    /* 0x4A914 */ u16 slotMask;     /* the slots filled since */
    /* 0x4A916 */ u8 unk4A916[2];
    /* 0x4A918 */ char *x614DDst;   /* 614D's reply: the string goes here */
    /* 0x4A91C */ s32 charaSelectTextMax;
    /* 0x4A920 */ s32 charaSelectText;        /* the u16 of that reply */
    /* 0x4A924 */ NetTownClock townClock;
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
typedef char NetTownClock_size_check[(sizeof(NetTownClock) == 0x48) ? 1 : -1];
typedef char NetListEntry_size_check[(sizeof(NetListEntry) == 0x18C) ? 1 : -1];
typedef char NetDlFile_size_check[(sizeof(NetDlFile) == 0x8) ? 1 : -1];
typedef char NetMember4_size_check[(sizeof(NetMember4) == 0x20) ? 1 : -1];
typedef char NetInfo6417_size_check[(sizeof(NetInfo6417) == 0x190) ? 1 : -1];
typedef char NetMember10_size_check[(sizeof(NetMember10) == 0x20) ? 1 : -1];
typedef char NetLevel1SearchInfo_size_check[(sizeof(NetLevel1SearchInfo) == 0x2F4) ? 1 : -1];
typedef char NetEntrySettings_size_check[(sizeof(NetEntrySettings) == 0x14F) ? 1 : -1];
typedef char NetEntrySettings2_size_check[(sizeof(NetEntrySettings2) == 0x16F) ? 1 : -1];
typedef char NetRec2_size_check[(sizeof(NetRec2) == 0x14A5) ? 1 : -1];
typedef char NetRecs2_size_check[(sizeof(NetRecs2) == 0x294A4) ? 1 : -1];
typedef char NetUserCardPart_size_check[(sizeof(NetUserCardPart) == 0x122) ? 1 : -1];
typedef char NetLoginNotice_size_check[(sizeof(NetLoginNotice) == 0x1004) ? 1 : -1];
typedef char NetLoginInfo_size_check[(sizeof(NetLoginInfo) == 0x1004) ? 1 : -1];
typedef char NetClientInfo_size_check[(sizeof(NetClientInfo) == 0x24) ? 1 : -1];
typedef char NetChatMessage_size_check[(sizeof(NetChatMessage) == 0x124) ? 1 : -1];
typedef char NetShortMail_size_check[(sizeof(NetShortMail) == 0x9C) ? 1 : -1];
typedef char NetUserStatus_size_check[(sizeof(NetUserStatus) == 0x20A) ? 1 : -1];
typedef char NetTownMsg_size_check[(sizeof(NetTownMsg) == 0x308) ? 1 : -1];
typedef char NetLoginWebPage_size_check[(sizeof(NetLoginWebPage) == 0x408) ? 1 : -1];
typedef char Net6416Rec_size_check[(sizeof(Net6416Rec) == 0x10) ? 1 : -1];
typedef char NetUserFindByNameRec_size_check[(sizeof(NetUserFindByNameRec) == 0x28) ? 1 : -1];
typedef char NetLevel1SearchRec_size_check[(sizeof(NetLevel1SearchRec) == 0x124) ? 1 : -1];
typedef char NetUserSearchCond_size_check[(sizeof(NetUserSearchCond) == 0x44) ? 1 : -1];
typedef char NetUserSearchQuery_size_check[(sizeof(NetUserSearchQuery) == 0x224) ? 1 : -1];
typedef char NetLevel1SearchCond_size_check[(sizeof(NetLevel1SearchCond) == 0x44) ? 1 : -1];
typedef char NetLevel1SearchQuery_size_check[(sizeof(NetLevel1SearchQuery) == 0x124) ? 1 : -1];
typedef char NetUserProfile_size_check[(sizeof(NetUserProfile) == 0x3CC) ? 1 : -1];
typedef char NetPartyMember_size_check[(sizeof(NetPartyMember) == 0x158) ? 1 : -1];
typedef char NetParty_size_check[(sizeof(NetParty) == 0xE84) ? 1 : -1];
typedef char LevelEntry_size_check[(sizeof(LevelEntry) == 0x164) ? 1 : -1];
extern LevelEntry gLobbyLevel2Entries[5];  /* level 2's entries (lobby) */
typedef char NetPatchInfo_size_check[(sizeof(NetPatchInfo) == 0x1C) ? 1 : -1];
typedef char NetLoginParams_size_check[(sizeof(NetLoginParams) == 0x40) ? 1 : -1];
typedef char NetUserSearchFound_size_check[(sizeof(NetUserSearchFound) == 0x6EF4) ? 1 : -1];
typedef char NetFound6416_size_check[(sizeof(NetFound6416) == 0x644) ? 1 : -1];
typedef char NetUserFindByNameFound_size_check[(sizeof(NetUserFindByNameFound) == 0xFA4) ? 1 : -1];
typedef char NetLevel1SearchFound_size_check[(sizeof(NetLevel1SearchFound) == 0x7214) ? 1 : -1];
typedef char LobbyNet_size_check[(sizeof(LobbyNet) == 0x4A96C) ? 1 : -1];

extern LobbyNet gLobbyNet;
extern NetLoginWebPage gNetLoginWebPage;       /* 0x894DAC: right after gLobbyNet */


extern LobbyPkt gLobbyPkt;     /* the message being built */
extern u8 gLobbyRecvData[];         /* the payload of the message received */

void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);
s32 Net_LockrockObfuscate(u8 *dst, u8 *src, u16 seed, s32 n, s32 key); /* unit's own declaration: declared with the u16 seed the callers pass; with the definition's u8 seed, LobbyPkt_PutBlob, LobbyPkt_PutBlob2 and HttpLbs_Send compile differently */
u32 strlen(const void *s);

/* requests and replies */
s32 LobbyReq_Level2Kind(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
void Net_SetHandler(s32 route, void (*handler)(NetMsg, NetMsg *));
void Net_CallHandler(s32 route, s32 unused);
void Lobby_RouteMessage(s32 a);
void NetReq_ClearAll(void);
void NetTask_ClearAll(void);
s32 NetReq_Alloc(u8 state, void (*onTick)(s32), void (*onReply)(NetMsg, NetMsg *));
s32 NetReq_TakeFixed(s32 i, void (*onTick)(s32), void (*onReply)(NetMsg, NetMsg *));
s32 NetReq_OnReply(NetMsg m, s32 a, s32 b);
void Net_TickRequests(void);
s32 NetReq_CountFree(void);
s32 NetReq_FindReply(void);
s32 Lobby_DispatchCommand(u16 cmd, s32 kind, u8 flag);
s32 Lobby_ReceiveAll(s32 sock);
void Lobby_HandleMessage(void);
s32 LobbyReq_LobbyLeave(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_LobbyLeave(void);
void LobbyReply_LobbyLeave(void);
s32 LobbyReq_Logout(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Logout(void);
void LobbyReply_Logout(void);
void LobbySrvReq_Keepalive(void);
void LobbyPkt_Keepalive(void);
void LobbyNotify_ServerClose(void);
void LobbyNotify_DepartCancel(void);
void LobbyNotify_DepartError(void);
void LobbyNotify_ServerLeave(void);

/* receiving: each reads one field of src into dst and returns the next field */
u8 *LobbyMsg_GetU8(void *dst, u8 *src);
u8 *LobbyMsg_GetU16(void *dst, u8 *src);
u8 *LobbyMsg_GetU32(void *dst, u8 *src);
u8 *LobbyMsg_GetString(void *dst, u8 *src);
u8 *LobbyMsg_GetBytes(void *dst, u8 *src, u16 n);
u8 *LobbyMsg_GetBlob(void *dst, u16 max, u8 *src);
void LobbyMsg_ReadErrText(void);
u8 *LobbyMsg_ReadU8(void *dst);
u8 *LobbyMsg_RecvU16(void *dst);
u8 *LobbyMsg_ReadU32(void *dst);
u8 *LobbyMsg_ReadU8Str(void *first, void *second);
u8 *LobbyMsg_ReadU8U8(void *first, void *second);
u8 *LobbyMsg_ReadU16U8(void *first, void *second);
u8 *LobbyMsg_ReadU16U16(void *first, void *second);
u8 *LobbyMsg_ReadU16U32(void *first, void *second);
u8 *LobbyMsg_ReadU8U8Str(void *first, void *second, void *third);

/* sending */
u16 LobbyPkt_BeginCmd(LobbyPkt *p, u16 cmd);
void LobbyPkt_Begin(LobbyPkt *p, u16 cmd);
void LobbyPkt_SetFlag(LobbyPkt *p, u8 flag);
void LobbyPkt_SetError(LobbyPkt *p, u8 err);
void LobbyPkt_End(LobbyPkt *p);
void LobbyPkt_PutU8(LobbyPkt *p, u8 v);
void LobbyPkt_PutU16(LobbyPkt *p, u16 v);
void LobbyPkt_PutU32(LobbyPkt *p, u32 v);
void LobbyPkt_PutBytes(LobbyPkt *p, void *src, u16 n);
void LobbyPkt_PutBlob(LobbyPkt *p, void *src, u16 n);
void LobbyPkt_PutBlob2(LobbyPkt *p, void *src, u16 n);
void LobbyPkt_PutBytes2(LobbyPkt *p, void *src, u16 n);
s32 LobbyPkt_Send(LobbyPkt *p);

/* LobbyKddi_PortalStep's steps (gLbsConnectStep). */
enum PortalStep {
    PORTAL_STEP_START = 0,      /* the sequence player is reset */
    PORTAL_STEP_OPEN = 1,       /* the browser opens gPortalPage's page */
    PORTAL_STEP_BROWSE = 2,     /* the browser runs until it ends */
    PORTAL_STEP_CLOSE = 3,      /* on to the lobby: the browser is closed */
    PORTAL_STEP_WAIT = 4,       /* two frames' wait */
    PORTAL_STEP_WAIT2 = 5,
    PORTAL_STEP_DONE = 6,       /* returns 1 */
    PORTAL_STEP_TOP_AGAIN = 0x63 /* the portal's top page is opened again */
};

/* The steps of the quest server's connection: gNetConnStep's values, gLobbyConnSteps' order. */
enum LobbyConnStep {
    CONN_CLOSE = 0,         /* LobbyConn_Close */
    CONN_OPEN = 1,          /* LobbyConn_Open */
    CONN_WAIT_OPEN = 2,     /* LobbyConn_WaitOpen */
    CONN_SET_OPTION = 3,    /* LobbyConn_SetOption */
    CONN_CHALLENGE = 4,     /* LobbyConn_AnswerChallenge */
    CONN_NEXT = 5,          /* LobbyConn_Next */
    CONN_START_QUEST = 6,   /* LobbyConn_StartQuest: connected */
    CONN_FAILED = 7         /* LobbyConn_Fail */
};

void Lobby_ResetTimers(void);
void Lobby_ApplyServerTimers(void);
void Town_Reset(void);
void Chat_SendText(s32 unused, void *text, u8 color);
void Chat_OnSendError(NetMsg reply, NetMsg *replyPtr);
void TownMsg_SendPosition(Player *unused0);
void TownMsg_SendPoogiePos(Em *poogie, void *member);
void TownMsg_SendKitchen(u8 *msg, void *member);
void TownMsg_SendArenaTeams(void);
void TownMsg_SendBgmTo(s32 unused, void *member);
void TownMsg_SendBgm(s32 track);
void TownMsg_SendSongAsk(s32 song);
void TownMsg_SendSongAnswer(void *askerId, s32 song, u8 busy);
void LobbyBgm_ClearState(void);
void TownMsg_SendHouseVisit(void *ownerId);
void TownMsg_SendPlace(Player *pl);
void TownMsg_SendPlaceOrigin(void);
void TownMsg_SendSober(struct Player *pl);
void TownMsg_SendStatus(void);
s32 Town_IsAlone(s16 *unused0);
void TownMsg_SendStateNow(Player *pl);
void TownMsg_SendState(void *unused, u8 flag);
void TownMsg_SendActor(Player *pl, void *member);
void TownMsg_SendFacing(Player *pl);
void TownMsg_SendArea(u16 id);
void TownMsg_SendAreaTo(void *member, u16 id);
void TownMsg_SendNameNotice(void);
void TownMsg_SendRecordsAsk(u8 kind);
void TownMsg_SendRecords(void *member, u8 kind);
void TownMsg_SendArenaOpen(void);
void TownMsg_SendHouseSeatAsk(Player *pl);
void TownMsg_SendHouseSeatFree(void *pl);
void TownMsg_SendHouseSeatAnswer(char *askerId, u8 answer, u8 seat);
void TownMsg_SendHouseSeatTaken(void);
void TownMsg_SendContestSeatAsk(Player *pl);
void TownMsg_SendContestSeatFree(Player *unused);
void TownMsg_SendContestSeatAnswer(char *askerId, u8 granted, u8 seat);
void TownMsg_SendContestSeatTaken(void);
void TownMsg_SendContestOpen(void *firstId, void *secondId);
void TownMsg_SendContestStart(s32 *toId);
void TownMsg_SendContestPower(void *toId, s32 power);
void TownMsg_SendContestResult(s32 *toId, s8 result);
void TownMsg_SendSeatAsk(Player *pl);
void TownMsg_SendSeatFree(Player *unused);
void TownMsg_SendSeatAnswer(char *askerId, u8 granted, u8 seat);
void TownMsg_SendSeatTaken(void);
void TownMsg_SendOfferAll(Player *pl);
void TownMsg_SendOffer(Player *pl, void *toId);
void TownMsg_SendOfferTake(void *toId, Player *pl);
void TownMsg_SendOfferResult(void *toId, u8 refused);
void TownMsg_SendOfferEnd(void);
void TownMsg_SendSyncAsk(void);
void TownMsg_SendSyncAskTo(u8 slot);
void TownMsg_Send(u8 len, u8 cmd, void *data);
void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *toId);
void TownMsg_OnSent(void);
void TownMsg_SendSelf(u8 len, u8 cmd, void *data);
extern u16 gMsgAreaIds[16];

void LobbyNotify_PatchHeader(void);
void NetPatch_ReadHeader(void);
void LobbyNotify_PatchData(void);
void NetPatch_ReadData(void);
void LobbySrvReq_PatchAck(void);
s32 LobbyPkt_PatchAck(u16 value);
void LobbyNotify_6123(void);
void LobbySrvReq_PatchEnd(void);
s32 LobbyNet_SendPatchEnd(void);
s32 LobbyPkt_PatchEnd(void);
s32 LobbyNet_GetPatchInfo(NetPatchInfo *info);
s32 NetPatch_CheckSum(u8 *buf, u32 n, u32 sum);
s32 NetLevel0ListTask_Start(s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *));
void NetLevel0ListTask_Run(s32 task);
void NetLevel0ListTask_Steps(NetTask *task);
void NetLevel0ListTask_Done(NetTask *task);
void NetLevel0ListTask_Failed(NetTask *task);
void NetLevel0ListTask_Step0(NetTask *task);
void NetLevel0ListTask_Step1(NetTask *task);
void NetLevel0ListTask_Step2(NetTask *task);
void NetLevel0ListTask_Step3(NetTask *task);
void NetLevel0ListTask_Step4(NetTask *task);
void NetLevel0ListTask_Step5(NetTask *task);
void NetLevel0ListTask_Step1Reply(NetMsg msg, NetMsg *msgRef);
void NetLevel0ListTask_Step4Reply(NetMsg msg, NetMsg *msgRef);
s32 LobbyReq_Level0Count(void (*onReply)(NetMsg, NetMsg *));
s32 NetList0_GetCount(u16 *out);
void LobbyReply_Level0Count(void);
s32 NetList0_GetName(u16 id, char *dst);
void LobbyReply_Level0Name(void);
s32 LobbyReq_Level0Values(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList0_GetUsers(u16 id, u16 *users, u16 *questUsers);
s32 NetList0_GetMaxUsers(u16 id, u16 *out);
s32 NetList0_GetValue10(u16 id, u16 *out);
void LobbyReply_Level0Values(void);
s32 LobbyReq_Level0State(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList0_GetByte(u16 id, u8 *out);
void LobbyReply_Level0State(void);
s32 LobbyReq_Level0Text(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList0_GetText(u16 id, char *dst);
void LobbyReply_Level0Text(void);
void LobbyReply_Level0Enter(void);
s32 LobbyReq_Level0Leave(void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_Level0Leave(void);
void LobbyNotify_Level0Kick(void);
void LobbyMsg_ReadU16(void *dst);
void LobbyMsg_ReadU16Str(void *num, void *str);
void LobbyMsg_ReadU16U8b(void *num, void *byte);
void LobbyMsg_ReadU16Str2(void *num, void *str);
void LobbyMsg_ReadU16U8b2(void *num, void *byte);
s32 LobbyReq_Level0Kind(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList0_GetKind(u16 id, u8 *out);
void LobbyReply_Level0Kind(void);

void TownMsg_Recv(void *from, u8 *pkt);
void TownMsg_OnKitchen(s8 idx, u8 *data);
void TownMsg_OnArenaTeam(u8 *data);
void Lobby_ShowMemberName(s8 idx, void *data);
void TownMsg_OnSongAsk(void *src, void *data);
void TownMsg_OnSongAnswer(s8 *data);
void TownMsg_OnOffer(s8 idx, void *src);
void TownMsg_OnOfferTake(void *from, void *src);
void TownMsg_OnOfferResult(void *src);
s32 LobbyMember_SetRecord(s8 idx, NetUser *user, u8 *rec);
void Chat_Receive(u8 *msg);
u8 Lobby_FindMember(u8 *id);
void TownMsg_OnActor(u8 pl, void *data);
void TownMsg_OnMove(u8 pl, void *data, u8 cmd);
void TownMsg_OnPoogiePos(u8 unused, void *data);
void TownMsg_OnArea(u8 pl, u8 *msg);
void TownMsg_OnContestSeatAsk(void *id, u8 *msg);
void TownMsg_OnContestSeatAnswer(s8 idx, u8 *msg);
void TownMsg_OnContestSeatTaken(u8 *id, u8 *data);
void TownMsg_OnSeatAsk(void *name, u8 *data);
void TownMsg_OnHouseSeatAsk(char *id, u8 *msg);
void TownMsg_OnSeatAnswer(s8 unused, u8 *data);
void TownMsg_OnHouseSeatAnswer(s8 unused, u8 *data);
void TownMsg_OnHouseSeatFree(s8 *msg);
void TownMsg_OnSeatTaken(u8 *a0);
void TownMsg_OnHouseSeatTaken(u8 *msg);
s32 Area_HasTownPcs(u16 area);
void TownMsg_OnSyncAsk(u8 *from);
void TownMsg_OnHouseVisit(s8 idx, u8 *msg);
void TownMsg_OnSober(s8 idx);

void LobbyNotify_ShortMail(void);
s32 LobbyReq_ShortMailSend(void *id, char *text, void (*onReply)(NetMsg, NetMsg *));
void LobbyNet_GetShortMail(char *id, u8 *name, u8 *text);
void LobbyReply_ShortMailSend(void);
s32 LobbyPkt_ShortMailSend(void *id, char *text);
void LobbyNotify_ShortMailRead(void);
s32 LobbyReq_UserProfile(void *id, void (*onReply)(NetMsg, NetMsg *));
void LobbyNet_GetUserProfile(NetUserProfile *dst);
void LobbyReply_UserProfile(void);
s32 LobbyPkt_UserProfile(void *id);
void LobbyReply_UserProfileRead(void);
void LobbySrvReq_6706(void);
s32 LobbyReq_UserSearch(NetUserSearchQuery query, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyNet_GetUserSearch(NetUserSearchFound **out);
s32 LobbyPkt_UserSearch(NetUserSearchQuery query);
s32 LobbyPkt_UserSearchNext(u8 value);
void LobbyReply_UserSearch(void);
s32 LobbyPkt_6416Next(u8 value);
void LobbyReply_6416(void);
s32 LobbyReq_UserStatus(void *id, void (*onReply)(NetMsg, NetMsg *));
void LobbyNet_GetUserStatus(u16 *a, u16 *b, u16 *c, u8 *d);
void LobbyReply_UserStatus(void);
s32 LobbyPkt_UserStatus(void *id);
void LobbyReply_UserStatusRead(void);
s32 LobbyReq_ChatColorMode(u8 value, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_ChatColorMode(void);
s32 LobbyPkt_ChatColorMode(u8 value);
void LobbyReply_6417(void);
s32 LobbyReq_UserFindByName(NetBlob name, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyNet_GetUserFindByName(NetUserFindByNameFound **out); /* the 6711 list (gLobbyNet.userFindByNameFound) */
s32 LobbyPkt_UserFindByName(NetBlob name);
void LobbyReply_UserFindByName(void);
s32 LobbyPkt_UserFindByNameNext(u8 value);
s32 LobbyReq_Level1Search(NetLevel1SearchQuery query, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyNet_GetLevel1Search(u32 **out);
s32 LobbyPkt_Level1Search(NetLevel1SearchQuery query);
s32 LobbyPkt_Level1SearchNext(u8 value);
void LobbyReply_Level1Search(void);
s32 LobbyReq_Level1SearchInfo(u8 value, void (*onReply)(NetMsg, NetMsg *));
void LobbyNet_GetLevel1SearchInfo(NetLevel1SearchInfo *dst);
s32 LobbyPkt_Level1SearchInfo(u8 value);
void LobbyReply_Level1SearchInfo(void);
void LobbyNotify_PartyReady(void);
s32 LobbyPkt_QuestPaid(void);

void LobbyReply_640E(void);
s32 LobbyReq_SetOut(u8 value, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_SetOut(u8 value);
void LobbyReply_SetOut(void);
s32 NetPartyTask_Start(void (*onDone)(NetMsg, NetMsg *));
void LobbyReply_PartyCount(void);
void LobbyReply_PartySelf(void);
void LobbyReply_PartyMember(void);
void LobbyReply_PartyMemberValues(void);
void LobbyReply_PartyRoom(void);
void LobbyReply_PartyText(void);
void LobbyReply_PartyServer(void);
void NetPartyTask_Finish(NetMsg msg);
s32 LobbyPkt_PartyCount(void);
void LobbyPkt_PartySelf(u8 value);
void LobbyPkt_PartyMember(u8 value);
void LobbyPkt_PartyMemberValues(u8 value);
void LobbyPkt_PartyText(void);
void LobbyPkt_PartyRoom(void);
void LobbyPkt_PartyServer(void);
s32 LobbyPkt_PartyFail(void);
s32 LobbyNet_GetParty(NetParty *dst);
void LobbyNet_GetPartyAddr(s32 *addr, s16 *port);

void LobbyReply_Level1Users(void);
void LobbyReply_Level1UsersRead(void);
void LobbyMsg_ReadUsers(u8 *count, NetUserRec *out);
void LobbyNotify_Level1UserOut(void);
void LobbyNotify_Level1UserIn(void);
void LobbyReply_6890(void);
s32 LobbyReq_CurrentPlace(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyNet_GetCurrentPlace(NetCurrentPlace *dst);
void LobbyReply_CurrentPlace(void);
s32 LobbyPkt_CurrentPlace(void);
s32 LobbyPkt_Level1Open(u16 id);
void LobbyReply_Level1Open(void);

s32 LobbyPkt_Level1SettingsEnd(void);
void LobbyReply_Level1SettingsEnd(void);
s32 LobbyReq_TownClock(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyNet_GetTownClock(NetTownClock *dst);
void LobbyReply_TownClock(void);
s32 LobbyPkt_TownClock(void);
s32 LobbyReq_Level1Kind(u16 value, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_Level1Kind(void);
void LobbyNet_Reset(void);
s32 NetLoginTask_Start(NetLoginParams *src, void (*onDone)(NetMsg, NetMsg *));
s32 LobbyNet_SetClientInfo(NetClientInfo *src);
s32 LobbyNet_SetMe(char *id, char *name, u8 *text);
u8 LobbyNet_GetCharaCount(void);
s32 LobbyNet_GetCharaId(u8 i, char *dst);
s32 LobbyNet_GetCharaName(u8 i, char *dst);
s32 LobbyNet_GetCharaText(u8 i, u8 *dst);
s32 LobbyNet_GetMyCharaId(char *dst);
s32 LobbyNet_GetMyCharaName(char *dst);
void LobbySrvReq_LoginAuth(void);
void LobbySrvReq_ClientInfo(void);

void LobbyReply_Level2Enter(void);
s32 LobbyReq_Level2Enter(u16 id, char *pass, u8 value, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2Enter(u16 id, char *pass, u8 value);
s32 LobbyReq_Level2Leave(void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_Level2Leave(void);
s32 LobbyReq_Level2Open(u16 id, u8 value, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2Open(u16 id, u8 value);
void LobbyReply_Level2Open(void);
s32 NetLevel2SettingsTask_Start(NetEntrySettings2 settings, void (*onDone)(NetMsg, NetMsg *));
void NetLevel2SettingsTask_Run(s32 task);
void NetLevel2SettingsTask_Steps(NetTask *task);
void NetLevel2SettingsTask_Done(NetTask *task);
void NetLevel2SettingsTask_Failed(NetTask *task);
void NetLevel2SettingsTask_Step0(NetTask *task);
void NetLevel2SettingsTask_Step1(NetTask *task);
void NetLevel2SettingsTask_Step2(NetTask *task);
void NetLevel2SettingsTask_Step3(NetTask *task);
void NetLevel2SettingsTask_Step4(NetTask *task);
void NetLevel2SettingsTask_Step5(NetTask *task);
void NetLevel2SettingsTask_Step6(NetTask *task);
void NetLevel2SettingsTask_Step7(NetTask *task);
void NetLevel2SettingsTask_Step8(NetTask *task);
void NetLevel2SettingsTask_Step9(NetTask *task);
void NetLevel2SettingsTask_Step10(NetTask *task);
void NetLevel2SettingsTask_Step11(NetTask *task);
void NetLevel2SettingsTask_Step12(NetTask *task);
void NetLevel2SettingsTask_OnReply(NetMsg msg, NetMsg *msgRef);
s32 LobbyPkt_Level2SetRecChoice(u8 i, u8 value);
void LobbyReply_Level2SetRecChoice(void);
s32 LobbyReq_Level2SetName(char *src, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2SetName(char *src);
void LobbyReply_Level2SetName(void);
s32 LobbyReq_Level2SetPass(char *src, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2SetPass(char *src);
void LobbyReply_Level2SetPass(void);
s32 LobbyReq_Level2SetText(char *src, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2SetText(char *src, u16 n);
void LobbyReply_Level2SetText(void);
s32 LobbyReq_Level2SettingsEnd(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2SettingsEnd(void);

void LobbyNet_SendChatSay(s32 text, u16 len, u8 option, u8 kind);
void LobbyNet_GetChatMessage(NetChatMessage *dst);
void LobbyPkt_ChatSay(s32 text, u16 len, u8 option, u8 kind);
void LobbyNotify_ChatMessage(void);
void LobbyNet_SendTownMsg(s32 src, u16 n);
void LobbyNet_GetTownMsg(NetTownMsg *dst);
void LobbyPkt_TownMsg(s32 src, u16 n);
void LobbyNotify_TownMsg(void);
s32 LobbyReq_ChatTell(u8 value, void *src, s32 text, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_ChatTell(u8 value, void *src, s32 text, u16 n);
void LobbyReply_ChatTell(void);
void LobbyNotify_ChatTellMessage(void);
s32 LobbyReq_TownMsgTo(void *src, void *data, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_TownMsgTo(void *src, s32 data, u16 n);
void LobbyReply_TownMsgTo(void);
void LobbyNotify_TownMsgToRecv(void);
void LobbyReply_6181(void);
void LobbyReply_6188(void);
s32 NetDlListTask_Start(void (*onDone)(NetMsg, NetMsg *));
void NetDlListTask_Run(s32 task);
void NetDlListTask_Steps(NetTask *task);
void NetDlListTask_Done(NetTask *task);
void NetDlListTask_Failed(NetTask *task);
void NetDlListTask_Step0(NetTask *task);
void NetDlListTask_Step1(NetTask *task);
void NetDlListTask_Step2(NetTask *task);
void NetDlListTask_Step3(NetTask *task);
void NetDlListTask_OnReply(NetMsg msg, NetMsg *msgRef);
s32 LobbyNet_GetDlFiles(u8 *count, NetDlFile **files);
s32 NetDlFileTask_Start(u8 file, u8 *buf, s32 size, void (*onDone)(NetMsg, NetMsg *));
void NetDlFileTask_Run(s32 task);
void NetDlFileTask_Steps(NetTask *task);
void NetDlFileTask_Done(NetTask *task);
void NetDlFileTask_Failed(NetTask *task);
void NetDlFileTask_Step0(NetTask *task);
void NetDlFileTask_Step1(NetTask *task);
void NetDlFileTask_Step2(NetTask *task);
void NetDlFileTask_OnReply(NetMsg msg, NetMsg *msgRef);
s32 LobbyReq_DlData(u8 value, s32 off, s32 n, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_DlData(void);
s32 LobbyPkt_DlData(u8 value, s32 off, s32 n);
s32 LobbyReq_DlList(void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_DlList(void);
s32 LobbyPkt_DlList(void);
s32 LobbyReq_DlFileInfo(u8 value, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_DlFileInfo(u8 value);
void LobbyReply_DlFileInfo(void);
void LobbyNotify_ServerMessage(void);
void LobbyReply_68A0(void);
void LobbyReply_68A1(void);
s32 LobbyReq_SaveSummaryBegin(u8 value, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_SaveSummaryBegin(u8 value);
void LobbyReply_SaveSummaryBegin(void);
s32 LobbyReq_SaveSummaryPart(u8 value, u8 subStep2, s32 src, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_SaveSummaryPart(u8 value, u8 subStep2, s32 src, u16 n);
void LobbyReply_SaveSummaryPart(void);
s32 LobbyNet_GetSaveSummaryPart(u8 *out);
s32 LobbyReq_TownData(u8 i, u8 *dst, s32 max, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_TownData(u8 value);
void Bits16_Set(u16 *slotMask, s32 i);
void LobbyReply_TownData(void);
s32 LobbyNet_GetTownData(u8 *slot);
s32 LobbyNet_SetSlotBuf(u8 i, char *buf, s32 n);
s32 LobbyReq2_TownData(u8 i, void (*onReply)(NetMsg, NetMsg *));
void LobbyNotify_TownDataChangedOnReply(NetMsg msg, NetMsg *msgRef);
void LobbyNotify_TownDataChanged(void);
s32 LobbyReq_ServerName(void *src, void *buf, s32 size, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_ServerName(void *src);
void LobbyReply_ServerName(void);
void LobbyReply_6144(void);
void LobbyReply_6145(void);
void LobbyReply_6146(void);
void LobbyPkt_6138(void);
s32 LobbyPkt_QuestRecord(void *src, s32 n);
void LobbyReply_6417Read(void);
void LobbyReply_Level1SearchInfoRead(void);
void LobbyReply_Ping(void);
void LobbyNotify_CharaList(void);
void LobbyReply_CharaSelect(void);
s32 LobbyReq_HunterCard(void *src, u16 n, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_HunterCard(void);
s32 LobbyNet_GetUserCard(NetUser *dst);
void LobbyNotify_UserCard(void);
s32 LobbyReq_HunterCardPart(void *src, u16 n, u16 offset, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_HunterCardPart(void);
s32 LobbyNet_GetUserCardPart(NetUserCardPart *dst);
void LobbyNotify_UserCardPart(void);
s32 LobbyPkt_HunterCardPart(void *src, u16 n, u16 offset);
void LobbyNotify_LoginDone(void);
void LobbySrvReq_LoginNotice(void);
void LobbyNet_GetLoginNotice(NetLoginNotice *dst);
s32 LobbyNet_SendLoginNotice(u8 value);
void LobbyPkt_LoginNotice(u8 value);
s32 LobbyNet_SendLoginReady(void);
void LobbyReply_6143(void);
s32 LobbyReq_LoginInfo(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyNet_GetLoginInfo(NetLoginInfo *dst);
s32 LobbyPkt_LoginInfo(void);
void LobbyReply_LoginInfo(void);
void NetLogin_Done(void);
void LobbySrvReq_6189(void);
void LobbySrvReq_6138(void);
s32 LobbyReq_QuestRecord(void *src, s32 n, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_QuestRecord(void);
void LobbyPkt_LoginAuth(void);
void LobbyPkt_ClientInfo(void);
void LobbyPkt_CharaSelect(void);
void LobbyPkt_LoginReady(void);
s32 LobbyPkt_HunterCard(void *src, u16 n);
s32 LobbyPkt_Ping(void);
s32 NetLogin_ReadCharas(void);
s32 LobbyReply_CharaSelectRead(void);
void LobbyPkt_LoginParams(void);
void LobbyReply_LoginParams(void);
void LobbyPkt_QnetTimeouts(void);
void LobbyReply_QnetTimeouts(void);
s32 LobbyReq_CharaSelectText(char *dst, s32 max, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyNet_GetCharaSelectText(s32 *dst);
void LobbyReply_CharaSelectText(void);
s32 LobbyPkt_CharaSelectText(void);
s32 LobbyReq_UserCardFetch(void *id, u16 size, u16 offset, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_UserCardFetch(void);
s32 LobbyNet_GetUserCardFetch(NetUserCardPart *dst); /* copies 6712's reply (gLobbyNet.userCardFetch) */
s32 LobbyPkt_UserCardFetch(void *id, u16 size, u16 offset);
void LobbyNotify_LoginWebPage(void);
s32 LobbyNet_GetLoginWebPage(NetLoginWebPage *dst);
void LobbyNotify_ServerIdRange(void);
void LobbySrvReq_MemoryCheck(void);

void NetList_Select(s32 levelArg, u16 id);
void NetList_Leave(u8 level);
void NetList_Enter(u8 level);
s32 LobbyNet_GetListReplies(u16 *nReplies);
void NetLevel1ListTask_Run(s32 task);
void NetLevel1ListTask_Steps(NetTask *task);
void NetLevel1ListTask_Done(NetTask *task);
void NetLevel1ListTask_Failed(NetTask *task);
void NetLevel1ListTask_Step0(NetTask *task);
void NetLevel1ListTask_Step1(NetTask *task);
void NetLevel1ListTask_Step2(NetTask *task);
void NetLevel1ListTask_Step3(NetTask *task);
void NetLevel1ListTask_Step4(NetTask *task);
void NetLevel1ListTask_Step5(NetTask *task);
void NetLevel1ListTask_Step1Reply(NetMsg msg, NetMsg *msgRef);
void NetLevel1ListTask_Step4Reply(NetMsg msg, NetMsg *msgRef);
s32 LobbyReq_Level1Count(void (*onReply)(NetMsg, NetMsg *));
s32 NetList1_GetCount(u16 *count1);
void LobbyReply_Level1Count(void);
s32 LobbyReq_Level1Name(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList1_GetName(u16 id, char *dst);
void LobbyReply_Level1Name(void);
s32 LobbyReq_Level1Values(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList1_GetUsers(u16 id, u16 *users, u16 *questUsers);
s32 NetList1_GetMaxUsers(u16 id, u16 *out);
void LobbyReply_Level1Values(void);
s32 LobbyReq_Level1State(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList1_GetByte(u16 id, u8 *out);
void LobbyReply_Level1State(void);
s32 LobbyReq_Level1Text(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList1_GetText(u16 id, char *dst);
void LobbyReply_Level1Text(void);
s32 LobbyReq_Level1Color(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList1_GetColor(u16 id, s32 *out);
s32 LobbyPkt_Level1Color(u16 id);
void LobbyReply_Level1Color(void);
s32 LobbyReq_630B(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_630B(u16 id);
void LobbyReply_630B(void);
s32 LobbyReq_Level1GroupInfo(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList1_GetGroupInfo(u16 id, u8 *out);
s32 LobbyPkt_Level1GroupInfo(u16 id);
void LobbyReply_Level1GroupInfo(void);
s32 LobbyReq_Level1Enter(u16 id, char *pass, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_Level1Enter(void);
s32 LobbyReq_Level1Leave(void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_Level1Leave(void);
void LobbyNotify_UserInOut(u8 kind);
void NetList_ReadValues(NetListEntry *list);
s32 LobbyReq_EnterLevels(u16 level0Id, u16 level1Id, u16 level2Id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_EnterLevels(u16 level0Id, u16 level1Id, u16 level2Id);
void LobbyReply_EnterLevels(void);
void LobbyReply_LeaveAll(void);
s32 LobbyNet_GetLastId(u16 *id);
void LobbyReply_6202(void);
void LobbyNotify_Level1Kick(void);
s32 LobbyPkt_ListCount(u16 level);
s32 LobbyPkt_EntryName(u16 level, u16 id);
s32 LobbyPkt_EntryValues(u16 level, u16 id);
s32 LobbyPkt_EntryState(u16 level, u16 id);
s32 LobbyPkt_EntryText(u16 level, u16 id);
s32 LobbyPkt_Level1Enter(u16 id, char *password);
s32 LobbyPkt_Leave(u16 level);
s32 LobbyPkt_EntryKind(u16 level, u16 id);
void LobbyMsg_ReadU16Array(u16 *id, u16 n, u16 *out);
void LobbyReply_640F(void);
s32 LobbyReq_Level1Users(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyNet_GetUser(u16 i, char *id, char *name, u8 *text);
s32 LobbyPkt_Level1Users(u16 id);
void NetLevel1AllowedTask_Run(s32 task);
void NetLevel1AllowedTask_Steps(NetTask *task);
void NetLevel1AllowedTask_Done(NetTask *task);
void NetLevel1AllowedTask_Failed(NetTask *task);
void NetLevel1AllowedTask_Step0(NetTask *task);
void NetLevel1AllowedTask_Step1(NetTask *task);
void NetLevel1AllowedTask_Step2(NetTask *task);
void NetLevel1AllowedTask_Step1Reply1(NetMsg msg, NetMsg *msgRef);
void NetLevel1AllowedTask_Step1Reply2(NetMsg msg, NetMsg *msgRef);
void NetLevel1AllowedTask_Step1Reply4(NetMsg msg, NetMsg *msgRef);
s32 LobbyNet_GetEntryVals(u16 entry, NetEntryVals *out);
s32 LobbyReq_Level1NameAllowed(u16 entry, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_Level1NameAllowed(void);
s32 LobbyPkt_Level1NameAllowed(u16 entry);
s32 LobbyReq_Level1PassAllowed(u16 entry, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_Level1PassAllowed(void);
s32 LobbyPkt_Level1PassAllowed(u16 entry);
s32 LobbyReq_Level1TextAllowed(u16 entry, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_Level1TextAllowed(void);
s32 LobbyPkt_Level1TextAllowed(u16 entry);
s32 LobbyReq_Level1SetName(char *name, u16 len, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level1SetName(char *src, u16 n);
void LobbyReply_Level1SetName(void);
s32 LobbyReq_Level1SetPass(s8 *src, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level1SetPass(s8 *src, u16 n);
void LobbyReply_Level1SetPass(void);
s32 LobbyReq_Level1SetText(s8 *src, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level1SetText(s8 *src, u16 n);
void LobbyReply_Level1SetText(void);
s32 LobbyPkt_Level1SetColor(s32 value);
void LobbyReply_Level1SetColor(void);
s32 LobbyReq_Level1SetGroupInfo(u16 value, u8 *bytes, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level1SetGroupInfo(u16 value, u8 *bytes);
void LobbyReply_Level1SetGroupInfo(void);
s32 LobbyReq_Level1SettingsEnd(void (*onReply)(NetMsg, NetMsg *));
s32 NetLevel2ListTask_Start(s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *));
void NetLevel2ListTask_Run(s32 task);
void NetLevel2ListTask_Steps(NetTask *task);
void NetLevel2ListTask_Done(NetTask *task);
void NetLevel2ListTask_Failed(NetTask *task);
void NetLevel2ListTask_Step0(NetTask *task);
void NetLevel2ListTask_Step1(NetTask *task);
void NetLevel2ListTask_Step2(NetTask *task);
void NetLevel2ListTask_Step3(NetTask *task);
void NetLevel2ListTask_Step4(NetTask *task);
void NetLevel2ListTask_Step5(NetTask *task);
void NetLevel2ListTask_Step1Reply(NetMsg msg, NetMsg *msgRef);
void NetLevel2ListTask_Step4Reply(NetMsg msg, NetMsg *msgRef);
s32 LobbyReq_Level2Count(void (*onReply)(NetMsg, NetMsg *));
s32 NetList2_GetCount(u16 *out);
void LobbyReply_Level2Count(void);
s32 LobbyReq_Level2Values(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList2_GetUsers(u16 id, u16 *userCount, u16 *questUserCount);
void LobbyReply_Level2Values(void);
s32 LobbyReq_Level2State(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList2_GetByte(u16 id, u8 *out);
void LobbyReply_Level2State(void);
s32 LobbyReq_Level2Name(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList2_GetName(u16 id, char *dst);
void LobbyReply_Level2Name(void);
s32 LobbyReq_Level2Text(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList2_GetText(u16 id, char *dst);
void LobbyReply_Level2Text(void);
s32 LobbyReq_Level2Limits(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList2_GetLimits(u16 id, u16 *maxUserCount, u16 *value1, u16 *value2, u16 *value3, u16 *value4);
s32 LobbyPkt_Level2Limits(u16 id);
void LobbyReply_Level2Limits(void);
s32 LobbyReq_Level2Color(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 Net_GetLevel2Color(u16 id, s32 *out);
s32 LobbyPkt_Level2Color(u16 id);
void LobbyReply_Level2Color(void);
s32 LobbyReq_Level2PostedQuest(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 Net_GetLevel2PostedQuest(u16 id, u8 *dst);
s32 LobbyPkt_Level2PostedQuest(u16 id);
void LobbyReply_Level2PostedQuest(void);
s32 LobbyReq_QuestAccept(u32 value, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_QuestAccept(u32 value);
void LobbyReply_QuestAccept(void);
s32 LobbyReq_QuestPost(void *data, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_QuestPost(void *data, u16 n);
void LobbyReply_QuestPost(void);
void LobbyReply_Level2SettingsEnd(void);
s32 LobbyNet_GetUserInOut(NetUser *dst);
void LobbyNotify_Level2UserIn(void);
void LobbyNotify_Level2UserOut(void);
s32 NetLevel2RecsTask_Start(u16 id, void (*onDone)(NetMsg, NetMsg *));
void NetLevel2RecsTask_Run(s32 task);
void NetLevel2RecsTask_State0(NetTask *task);
void NetLevel2RecsTask_State1(NetTask *task);
void NetLevel2RecsTask_State2(NetTask *task);
void NetLevel2RecsTask_Done(NetTask *task);
void NetLevel2RecsTask_Failed(NetTask *task);
void NetLevel2RecsTask_S0Step0(NetTask *task);
void NetLevel2RecsTask_S0Step1(NetTask *task);
void NetLevel2RecsTask_S0Step2(NetTask *task);
void NetLevel2RecsTask_S0Step3(NetTask *task);
void NetLevel2RecsTask_S0Step4(NetTask *task);
void NetLevel2RecsTask_S0Step5(NetTask *task);
void NetLevel2RecsTask_S1Step0(NetTask *task);
void NetLevel2RecsTask_S1Step1(NetTask *task);
void NetLevel2RecsTask_S1Step2(NetTask *task);
void NetLevel2RecsTask_S1Step3(NetTask *task);
void NetLevel2RecsTask_S1Step4(NetTask *task);
void NetLevel2RecsTask_S1Step5(NetTask *task);
void NetLevel2RecsTask_S1Step6(NetTask *task);
void NetLevel2RecsTask_S2Step0(NetTask *task);
void NetLevel2RecsTask_S2Step1(NetTask *task);
void NetLevel2RecsTask_S2Step2(NetTask *task);
void NetLevel2RecsTask_S2Step3(NetTask *task);
void NetLevel2RecsTask_S2Step4(NetTask *task);
void NetLevel2RecsTask_S2Step5(NetTask *task);
void NetLevel2RecsTask_OnReply(NetMsg msg, NetMsg *msgRef);
void NetLevel2RecsTask_OnRecReply(NetMsg msg, NetMsg *msgRef);
s32 LobbyReq_Level2RecCount(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2RecCount(u16 id);
void LobbyReply_Level2RecCount(void);
s32 LobbyReq_Level2NameEditable(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2NameEditable(u16 id);
void LobbyReply_Level2NameEditable(void);
s32 LobbyReq_Level2PassEditable(s32 id, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_Level2PassEditable(void);
s32 LobbyPkt_Level2PassEditable(u16 id);
s32 LobbyReq_Level2Locked(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 NetList2_GetLocked(u16 id, u8 *out);
s32 LobbyPkt_Level2Locked(u16 id);
void LobbyReply_Level2Locked(void);
s32 LobbyReq_Level2Users(u16 id, void (*onReply)(NetMsg, NetMsg *)); /* called with an int: no prototype here */
s32 NetList2_GetUser(u16 i, char *id, char *name, u8 *text);
s32 LobbyPkt_Level2Users(u16 id);
void LobbyReply_Level2Users(void);
void LobbyReply_Level2Users_Users(void);
s32 LobbyReq_Level2RecName(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2RecName(u16 id, u8 i);
void LobbyReply_Level2RecName(void);
s32 LobbyReq_Level2RecChoiceCount(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2RecChoiceCount(u16 id, u8 i);
void LobbyReply_Level2RecChoiceCount(void);
s32 LobbyReq_Level2RecDefault(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2RecDefault(u16 id, u8 i);
void LobbyReply_Level2RecDefault(void);
s32 LobbyReq_Level2RecEditable(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2RecEditable(u16 id, u8 i);
void LobbyReply_Level2RecEditable(void);
s32 LobbyReq_Level2RecChoiceName(u16 id, u8 i, u8 j, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2RecChoiceName(u16 id, u8 i, u8 j);
void LobbyReply_Level2RecChoiceName(void);
s32 LobbyReq_Level2RecChoiceValues(u16 id, u8 i, u8 j, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2RecChoiceValues(u16 id, u8 i, u8 j);
void LobbyReply_Level2RecChoiceValues(void);
void LobbyMsg_ReadU8U8Rest(u8 *i, u8 *j, u8 **rest);
void LobbyReply_6409(void);
s32 LobbyReq_Level2Members(u16 id, void (*onReply)(NetMsg, NetMsg *));
void LobbyReply_Level2Members(void);
void LobbyNotify_Level2MembersNotify(void);
s32 LobbyPkt_Level2Members(u16 id);
void LobbyReply_6413(void);
void LobbyNotify_6414(void);
s32 LobbyReq_Level2TextEditable(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2TextEditable(u16 id);
void LobbyReply_Level2TextEditable(void);
s32 LobbyReq_Level2Quest(u16 id, u8 *values, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2Quest(u16 id, u8 *values);
void LobbyReply_Level2Quest(void);
void LobbyNotify_Level2Kick(void);
void LobbyReply_6507(void);
void LobbyReply_Level2Kind(void);

s32 NetLevel1SettingsTask_Start(NetEntrySettings *src, void (*onDone)(NetMsg, NetMsg *));
void NetLevel1SettingsTask_Run(s32 task);
void NetLevel1SettingsTask_Steps(NetTask *task);
void NetLevel1SettingsTask_Done(NetTask *task);
void NetLevel1SettingsTask_Failed(NetTask *task);
void NetLevel1SettingsTask_Step0(NetTask *task);
void NetLevel1SettingsTask_Step1(NetTask *task);
void NetLevel1SettingsTask_Step2(NetTask *task);
void NetLevel1SettingsTask_Step3(NetTask *task);
void NetLevel1SettingsTask_Step4(NetTask *task);
void NetLevel1SettingsTask_Step1Reply1(NetMsg msg, NetMsg *msgRef);
void NetLevel1SettingsTask_Step1Reply2(NetMsg msg, NetMsg *msgRef);
void NetLevel1SettingsTask_Step1Reply4(NetMsg msg, NetMsg *msgRef);
void NetLevel1SettingsTask_Step3Reply(NetMsg msg, NetMsg *msgRef);
void NetLevel1SettingsTask_Step1Reply8(NetMsg msg, NetMsg *msgRef);

#endif /* LOBBY_NET_H */
