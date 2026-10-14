#ifndef LOBBY_STATE_H
#define LOBBY_STATE_H

#include "common.h"
#include "menu_task.h"
#include "item.h"

/* A hunter card (0xD8 bytes): what a player shows the others. HunterCard_Build
 * makes the local player's from the save and the player, the lobby relays it
 * as the first 0xD8 bytes of a NetUser's 0x100-byte text (gPlayerText is the
 * local one), HunterCard_Sanitize clamps a received one and HunterCard_Apply
 * applies it to gPlayers[idx]. */
typedef struct HunterCard {
    /* 0x00 */ u16 hr;
    /* 0x02 */ u8 chatColor;        /* gChatTextColor: the chat text colour, 0-8 (the colour window) */
    /* 0x03 */ u8 weaponClass;
    /* 0x04 */ s32 hairColor;       /* SaveData.hairColor, Player.hairColor */
    /* 0x08 */ u8 questStatus;      /* Player.questStatus */
    /* 0x09 */ u8 hairStyle;        /* SaveData.hairStyle, Player.hairStyle */
    /* 0x0A */ u8 gender;           /* 0 or 1: Player.gender */
    /* 0x0B */ u8 voice;            /* SaveData.voice, Player.voice (0-14) */
    /* 0x0C */ u8 symbolColor;      /* SaveData.symbolColor once a style is earned (styleBits), else 0xFF; Player_SetColorWord */
    /* 0x0D */ u8 charType;         /* SaveData.charType, Player.charType (0-3) */
    /* 0x0E */ u8 title;            /* SaveData.title (below 0x94) */
    /* 0x0F */ u8 restriction;      /* SaveData.restriction, 0 or 1: gRestrictTexts[it] (MemberCard_DrawRestrict) */
    /* 0x10 */ u8 comment[0x62];    /* SaveData.comment (Chat_FilterWords on a received one) */
    /* 0x72 */ s16 versusWins;      /* SaveData.versusWins */
    /* 0x74 */ EquipItem weapon;
    /* 0x80 */ EquipItem armour[6];
    /* 0xC8 */ u8 padC8;
    /* 0xC9 */ u8 houseLevel;       /* Village_FacilityLevel(4), 0-2: the house's area is 0xAD + it */
    /* 0xCA */ u8 poogieOutfit;     /* SaveData.poogieOutfit when the Poogie has a name (poogieName), else 0xFF (at most 4):
                                       the town loads model 0xD + it (unk_7A6470.c) */
    /* 0xCB */ u8 houseAccess;      /* gHouseAccess: 0 closed, 1 open, 2 by password */
    /* 0xCC */ u8 housePass[9];     /* gHouseAccessName: the house's password */
    /* 0xD5 */ u8 boxLevel;         /* Village_FacilityLevel(5): the box's pages - 1 (at most 4) */
    /* 0xD6 */ u8 arenaRole;        /* in an arena quest (at most 4): 1 posted it (unk_71F8A0.c), 2-4 QuestCounterMenu.kind;
                                       Quest_DrawArenaTeams groups the members by it, 1 and 3 open ArenaMenu */
    /* 0xD7 */ u8 unkD7;            /* D_008BEB75 (0 or 1) */
} HunterCard;
typedef char HunterCard_size_check[(sizeof(HunterCard) == 0xD8) ? 1 : -1];

/* a user's text as the lobby relays it (0x100 bytes): a HunterCard and 0x28
 * unused bytes. NetUser.text holds one as bytes (NetUser is byte-aligned in
 * LobbyState); gPlayerText is the local player's. */
typedef union UserText {
    HunterCard card;
    u8 raw[0x100];
} UserText;
typedef char UserText_size_check[(sizeof(UserText) == 0x100) ? 1 : -1];

/* a user: id (6 characters used), name and a 0x100-byte text (the player's
 * own at gLobbyNet.me, set by LobbyNet_SetMe): a HunterCard in its first
 * 0xD8 bytes */
typedef struct NetUser {
    /* 0x000 */ char id[8];
    /* 0x008 */ char name[0x14];
    /* 0x01C */ u8 text[0x100];
} NetUser;

/*
 * The lobby's state (gLobbyState, 0x39A0 bytes): the screens' modes and steps,
 * the players, the error text of the last failed request. gLobby always
 * points to it (Town_Reset, Lobby_ResetState and NetSetup_ResetLobby set it), and
 * the code reaches it through that pointer; the village's code reads it too.
 * Most fields are still named by offset.
 */
typedef struct LobbyState {
    /* 0x0000 */ s8 groupJoin;      /* set on joining a group (Group_Join, the group board): the level lists go on by themselves */
    /* 0x0001 */ char groupId[0x10]; /* the group joined: its id (memcpy 0x10 from the group, LobbySrv_Index) */
    /* 0x0011 */ u8 unk11[3];
    /* 0x0014 */ s32 area;          /* the group's area (Group_Join; LobbyLogin_EnterEntry) */
    /* 0x0018 */ s32 land;          /* the group's land (Group_Join; LobbyLogin_EnterEntry) */
    /* 0x001C */ u8 idCursor;       /* the id menu's cursor kept (LobbyEntry_IdSelect) */
    /* 0x001D */ u8 unk1D;
    /* 0x001E */ s8 hostId[8];          /* the user id of the town host: it answers the TownMsg requests when it equals myId */
    /* 0x0026 */ char charaIds[4][8];   /* the four character ids (LobbyNet_GetCharaId; IdMenu_Redraw, LobbyEntry_IdSelect) */
    /* 0x0046 */ u8 charaNames[0x44]; /* four names of 0x11 bytes (LobbyNet_GetCharaName; IdMenu_DrawText) */
    /* 0x008A */ u8 charaTexts[0x400]; /* four texts of 0x100 bytes (LobbyNet_GetCharaText; HunterCard_Sanitize) */
    /* 0x048A */ u8 myId[8];            /* this player's user id (login; gMyUserId keeps a copy) */
    /* 0x0492 */ u8 myName[0x10];   /* this player's character name (LobbyNet_GetMyCharaName; gMyName keeps a copy) */
    /* 0x04A2 */ u8 unk4A2[4];
    /* 0x04A6 */ u8 myText[0x100];  /* this player's text, 0x100 bytes (gPlayerText; Lobby_UpdatePlayerText) */
    /* 0x05A6 */ NetUser players[4];
    /* 0x0A16 */ NetUser members[10];   /* the town's members (the village: LobbyMember_GetCard, LobbyMember_InMyParty) */
    /* 0x152E */ u8 pcLoaded[0xA];        /* per player: 1 once its model is loaded (TownPc_LoadModel); 0: the town updates skip it; cleared when it leaves */
    /* 0x1538 */ char memberSlot[0xA];     /* per member: '0' + the player slot it is in; read as u8 */
    /* 0x1542 */ u8 unk1542[0x20];
    /* 0x1562 */ u16 readyBits;         /* ready bits, one per slot (Lobby_SetReadyBit) */
    /* 0x1564 */ u8 slotIds[0x80];      /* 16 user ids of 8 bytes, by slot: memcpy(slotIds + slot * 8, id, 8); copied to the id menus (m->ids), cleared by slot */
    /* 0x15E4 */ u8 msg6105;        /* the 6105 text (LobbyNet_Get6105 fills 0x1004 bytes from here: Net6105.unk0) */
    /* 0x15E5 */ u8 pad15E5;
    /* 0x15E6 */ s16 msg6105Secs;   /* Net6105.unk2: the countdown shown with the text, in seconds (read as u16 and s16) */
    /* 0x15E8 */ u8 msg6105Text[0x1000]; /* Net6105.text (LobbyDialog_OpenCountdown; copied to errText) */
    /* 0x25E8 */ u8 msg614C[4];     /* 614C's reply (LobbyNet_Get614C fills 0x1004 bytes from here: Net614C.unk0) */
    /* 0x25EC */ u8 msg614CText[0x1000]; /* Net614C.text (LobbyDialog_OpenWait) */
    /* 0x35EC */ s32 unk35EC;
    /* 0x35F0 */ u8 unk35F0[0x10];
    /* 0x3600 */ s8 unk3600[8];
    /* 0x3608 */ char errText[0x300];   /* the server's error text (LobbyNet_GetErrText) */
    /* 0x3908 */ s32 timer;             /* frames: 0xE10 is a minute */
    /* 0x390C */ s32 recvTimer;         /* frames left without a message before Lobby_Disconnect(7) (0xE10) */
    /* 0x3910 */ s32 idleTimer;         /* frames left without input before Lobby_Disconnect(4) */
    /* 0x3914 */ s32 countdown;          /* frames, shown in seconds (/30) */
    /* 0x3918 */ s32 waitTimer;     /* frames left on an entry screen (8, 0x708: LobbyEntry_Info) */
    /* 0x391C */ s32 questList;     /* the town quest list (0x100 bytes of gTownQuestLists) filled (LobbyStep_6884) */
    /* 0x3920 */ s32 unk3920;
    /* 0x3924 */ s32 townEvent;     /* the town event from 6884 (slot 0 first byte, Lobby_Apply6884): 1 to 3 change the shop stock (Shop_BuildStock), 4 halves CombShop prices; tested by the Talk_Check functions */
    /* 0x3928 */ s32 place;         /* Lobby_SetPlace3928, Lobby_GetPlace3928 */
    /* 0x392C */ s32 enterSeq;      /* the village entry, 1-3 (LobbyVillage_Begin1-3; LobbyScreen_EnterSeq) */
    /* 0x3930 */ u16 roomMembers;       /* the other members of the room: +1 is compared with the room size, below gQuestMinMembers the quest is disbanded */
    /* 0x3932 */ u16 buttons;           /* the menu buttons of this frame (LobbyPad_Read, LobbyPad_Get) */
    /* 0x3934 */ s16 seqStep;            /* a sequence's step (99: failed) */
    /* 0x3936 */ u8 phase;             /* the lobby phase (5: leaving, Lobby_Disconnect) */
    /* 0x3937 */ u8 phaseStep;           /* phase > phaseStep > mode > step > subStep: the levels */
    /* 0x3938 */ u8 mode;                /* of the lobby's state machines (Lobby_Disconnect clears them) */
    /* 0x3939 */ u8 step;
    /* 0x393A */ u8 subStep;             /* 99: failed */
    /* 0x393B */ u8 subStep2;           /* the level below subStep (also a reply loop part counter) */
    /* 0x393C */ u8 unk393C;
    /* 0x393D */ u8 unk393D;
    /* 0x393E */ s8 taskStep;       /* the step of the LobbyStep_* sequences (99: done) */
    /* 0x393F */ s8 unk393F;
    /* 0x3940 */ u8 unk3940;
    /* 0x3941 */ u8 unk3941;
    /* 0x3942 */ u8 unk3942;
    /* 0x3943 */ u8 webStep;              /* the step of LobbyWeb_Run (switch on it, counted up) */
    /* 0x3944 */ u8 linkState;      /* 0 at reset; 1 disconnected (Lobby_MarkDisconnected, with connected 0); 2 back from the web pages (LobbyWeb_Run): LobbyEntry_Terms tests 2, Lobby_Tick 1 */
    /* 0x3945 */ u8 awaitReply;            /* the request whose reply is awaited */
    /* 0x3946 */ u8 quitReason;        /* the reason given to Lobby_Disconnect */
    /* 0x3947 */ u8 partyCount;     /* the room's party list count (LobbyRoom_FetchParty) */
    /* 0x3948 */ u8 questPaid;            /* 1: a quest was accepted with its fee paid */
    /* 0x3949 */ s8 areaLoaded;     /* 1 once the area's models are loaded (Town_EnterArea, TownPc_LoadAllModels; Town_FreeArea clears it) */
    /* 0x394A */ u8 modelLoaded[0xA]; /* per player: 1 once TownPc_LoadModel or Player_ApplyLook set the models up */
    /* 0x3954 */ u8 modelLoading[0xA]; /* per player: 1 while TownPc_LoadModel loads the models (TownPc_IsLoaded waits) */
    /* 0x395E */ s8 connected;           /* 1: on the lobby server (Lobby_Tick checks its timeouts) */
    /* 0x395F */ s8 leave6307;      /* a leave request: 1 the server's 6307 (LobbyRoute_6307), 2 begun here (LobbyVillage_BeginLeave); Lobby_HandleLeaveRequest */
    /* 0x3960 */ s8 leave640C;      /* the same for 640C (LobbyRoute_640C; 2: LobbyTown_BeginLeave) */
    /* 0x3961 */ s8 leave6505;      /* the same for 6505 (LobbyRoute_6505; 2: LobbyTown_BeginLeave) */
    /* 0x3962 */ s8 playersListed;  /* 1 once the players list came (LobbyRoute_Players); Lobby_SendHeld returns it */
    /* 0x3963 */ s8 unk3963;
    /* 0x3964 */ s8 serverClose;    /* the server closed the session: 1 by 6003, 2 by 6007 (out of phase 5): Lobby_Disconnect(6 or 8) */
    /* 0x3965 */ s8 dlQuestsReady;      /* 1: the downloaded quests can be used (Quest_IsDownloadId checks skip them while 0) */
    /* 0x3966 */ u8 dlQuestCount;       /* the downloaded quests (count) */
    /* 0x3967 */ u8 unk3967;
    /* 0x3968 */ u8 unk3968;
    /* 0x3969 */ u8 unk3969;
    /* 0x396A */ u8 questTaken;            /* 1 while a quest is taken */
    /* 0x396B */ u8 entered6504;    /* 1 after 6504 (LobbyStep_6504Done), 0 after its leave (LobbyStep_6504LeaveDone) */
    /* 0x396C */ u8 mealEaten;      /* 1 after a meal (Kitchen_EatDish, Meal_ApplyEffects; PickMenu_Update clears it) */
    /* 0x396D */ u8 unk396D;
    /* 0x396E */ u8 unk396E;
    /* 0x396F */ u8 elderRaidState; /* ElderRaid_SetState (1, 2) */
    /* 0x3970 */ u8 movieState;     /* the town movie: 1 wanted (LobbyTown_WantsMovie, LobbyTown_StartMovie) */
    /* 0x3971 */ u8 pad3971;
    /* 0x3972 */ s16 poogieFeed;    /* the poogie's mood counters, indexed by mood from here: 0 fed (PoogieMenu_Feed) */
    /* 0x3974 */ s16 poogieCare;    /* 1 cared for (PoogieMenu_UpdateCare) */
    /* 0x3976 */ s16 poogieFeedMax; /* the counters' limits (VillageGather_DailyReset rolls them) */
    /* 0x3978 */ s16 poogieCareMax;
    /* 0x397A */ u8 roomName[0xF];      /* the room's name (6915; LobbyRoom_FetchParty), 15 bytes: the
                                         * game copies 0x10 in (unk_7BDB00.c), so the name's terminator
                                         * lands on player 0's byte below, and 0xF out */
    /* 0x3989 */ u8 lookPending[0x17];  /* per player: nonzero while its look is to be applied: 2 changed (TownPc_RefreshChanged), 0 after Player_ApplyLook; a byte per player (10 used), read as s8 (unk_793C40.c,
                                         * unk_7952C0.c); unk_7C7410.c clears the 10 as
                                         * &roomName[0xF] */
} LobbyState;
typedef char LobbyState_size_check[(sizeof(LobbyState) == 0x39A0) ? 1 : -1];

/*
 * The lobby screens' menu state (gLobbyMenu): the screen and its step, a
 * cursor, a page of pages, a countdown. A file that uses it declares
 * `extern LobbyMenu *gLobbyMenu;` itself (some still declare it u8 *).
 */
typedef struct LobbyMenu {
    /* 0x00 */ MenuTask *layer[8];            /* sub_main draw tasks gLobbyMenuLayer0..gLobbyMenuLayer7 (0x20 apart): +0x14 the
                                         * draw callback, +0x18 the text size (LobbyMenu_Init, LobbyMenu_SetDraw) */
    /* 0x20 */ u8 unk20[4];
    /* 0x24 */ s16 timer;               /* frames left (screen changes, fades) */
    /* 0x26 */ u8 unk26[2];
    /* 0x28 */ s16 page;
    /* 0x2A */ s16 pages;
    /* 0x2C */ u8 unk2C[2];
    /* 0x2E */ u8 screen;               /* the menu's page (gIdMenuScreens[screen]) */
    /* 0x2F */ u8 step;
    /* 0x30 */ s8 unk30;
    /* 0x31 */ u8 subStep;             /* a step within the screen step (switch, then ++) */
    /* 0x32 */ u8 unk32;
    /* 0x33 */ u8 promptStep;       /* a third step byte: the closing prompt runs a countdown (0), press a button (1), then the screen goes on (2) (unk_7C6A40.c, unk_7C7410.c); SrvMenu_Run starts at 1 */
    /* 0x34 */ u8 unk34;
    /* 0x35 */ u8 cursor;
    /* 0x36 */ u8 unk36[2];
    /* 0x38 */ u8 quitYesNo;        /* the quit question's answer: 0 yes (SrvMenu_Quit, Menu_SetYesNo) */
    /* 0x39 */ u8 redraw;               /* 1: redraw the texts (IdMenu_Redraw clears it) */
    /* 0x3A */ u8 unk3A[2];
    /* 0x3C */ u8 yesNo;               /* the yes/no cursor: 0 yes, 1 no (Menu_SetYesNo) */
    /* 0x3D */ u8 hideLayers;       /* nonzero: the frame queues no menu layers (the screens skip LobbyMenu_QueueLayers) */
    /* 0x3E */ s16 slideTimer;      /* frames left of a slide or wait (SrvMenu_SlideIn: 0x1E, KitchenGuest_Update) */
    /* 0x40 */ u8 unk40[4];             /* LobbyMenu_Init clears 0x44 bytes */
} LobbyMenu;
typedef char LobbyMenu_size_check[(sizeof(LobbyMenu) == 0x44) ? 1 : -1];

/* The third level's entry records as the screens keep them (gLobbyRecs, 0x29555 bytes at
 * 0x8C0CA0; LobbyLevel2_FetchRecords copies them from gLobbyNet.recs2, NetRecs2): the room's
 * settings (password, the member count, the arena cup's kinds) are the records' values. */
typedef struct LobbyRec {
    /* 0x0000 */ u8 flags;
    /* 0x0001 */ char name[0x41];
    /* 0x0042 */ u8 m;              /* the sub-entries */
    /* 0x0043 */ u8 unk43;
    /* 0x0044 */ u8 unk44;
    /* 0x0045 */ u8 cur;            /* the value chosen (task 7 sends it) */
    /* 0x0046 */ u8 pad46[0x22];
    /* 0x0068 */ char subName[0x20][0x41];
    /* 0x0888 */ u8 subN[0x20];
    /* 0x08A8 */ u8 subData[0x20][0x20][3];
} LobbyRec;
typedef char LobbyRec_size_check[(sizeof(LobbyRec) == 0x14A8) ? 1 : -1];

typedef struct LobbyRecs {
    /* 0x00000 */ u8 sendName;
    /* 0x00001 */ u8 sendPass;
    /* 0x00002 */ char pass[9];     /* the room's password (LobbyMenu_KbdEntry) */
    /* 0x0000B */ u8 padB[0x47];
    /* 0x00052 */ u8 usePass;       /* 1: a password is set (gNoneSetTexts) */
    /* 0x00053 */ u8 pad53;
    /* 0x00054 */ u8 n;             /* records */
    /* 0x00055 */ LobbyRec rec[32]; /* rec[0]: the member count (gMemberCountTexts); rec[1..3]: the arena cup's */
} LobbyRecs;
typedef char LobbyRecs_size_check[(sizeof(LobbyRecs) == 0x29555) ? 1 : -1];

extern LobbyState gLobbyState;
extern UserText gPlayerText;  /* the local player's (sub_main; Lobby_UpdatePlayerText) */
extern LobbyState *gLobby;      /* = &gLobbyState */

#endif /* LOBBY_STATE_H */
