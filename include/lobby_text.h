#ifndef LOBBY_TEXT_H
#define LOBBY_TEXT_H

#include "common.h"

/* The lobby's text helpers, its message table and its message dialog. */

/* The x that centres s on x: x - w * (its display length) / 4. */
s16 Text_CenterX(char *s, s16 x, s16 w);

/* Messages of the lobby's message table gLobbyMsgTexts (152 of them) that
 * LobbyText_Open shows, named by the role their callers give them. */
enum LobbyTextMsg {
    LOBBY_TEXT_TO_TOWN = 0x00,               /* on the way to the town (LobbyDialog_ShowConnMsg) */
    LOBBY_TEXT_NET_PREPARING = 0x01,         /* the network is being prepared */
    LOBBY_TEXT_QUEST_PREPARING = 0x02,       /* the quest is being prepared */
    LOBBY_TEXT_RETURN_TAVERN = 0x03,         /* going back to AREA_TAVERN after a quest (LobbyEnter_Begin) */
    LOBBY_TEXT_RETURN_TOWN_ARENA = 0x04,     /* going back to AREA_TOWN_ARENA */
    LOBBY_TEXT_RETURN_ELDER_HALL = 0x05,     /* going back to AREA_ELDER_HALL */
    LOBBY_TEXT_RETURN_TOWN = 0x06,           /* going back to the town's other areas */
    LOBBY_TEXT_CHARA_SLOT_USED = 0x07,       /* a character slot holding a character is chosen (IdMenu_Choose) */
    LOBBY_TEXT_CHARA_SLOT_EMPTY = 0x08,      /* an empty character slot is chosen */
    LOBBY_TEXT_DISCONNECTING = 0x09,         /* the lobby server is being left (LobbyExit_RunPlanned, LobbyExit_RunCut) */
    LOBBY_TEXT_IDLE_QUIT = 0x0A,             /* the result of LOBBY_QUIT_IDLE (LobbyExit_ShowReason) */
    LOBBY_TEXT_IDLE_CUT = 0x0B,              /* the line was cut after a time without input (NetScreen_Step5) */
    LOBBY_TEXT_QUIT_GMSUB = 0x0C,            /* LobbyExit_ShowReason's text for LOBBY_QUIT_TO_GMSUB (no caller passes that reason) */
    LOBBY_TEXT_LINK_LOST = 0x0D,             /* the result of LOBBY_QUIT_FAILED and LOBBY_QUIT_LINK_LOST */
    LOBBY_TEXT_LINE_UNSTABLE = 0x0E,         /* the server cut the line, which was unstable */
    LOBBY_TEXT_AREA_RANK = 0x0F,             /* the hunter rank doesn't match the chosen area (Lobby_CanEnterByRank) */
    LOBBY_TEXT_AREA_ENTERING = 0x10,         /* the chosen area is being entered */
    LOBBY_TEXT_TOWN_ENTERING = 0x11,         /* the town's next area is being entered (the move menu's chosen area; LobbyVillage_BeginReturn) */
    LOBBY_TEXT_AREA_FULL = 0x14,             /* the chosen area is full (its LevelEntry.state isn't LEVEL_ENTRY_OPEN) */
    LOBBY_TEXT_AREA_CURRENT = 0x18,          /* the chosen area is the one the player is in (the move menu, Lobby_CanEnterByRank) */
    LOBBY_TEXT_AREA_LIST_FETCH = 0x1B,       /* the areas' list is being fetched (AreaList_Begin) */
    LOBBY_TEXT_MOVING = 0x1D,                /* a move is under way: shown when entering the chosen group failed, while its area's list is fetched again (LobbyLogin_EnterEntry) */
    LOBBY_TEXT_FRIEND_REMOVE_ASK = 0x1E,     /* whether to take the friend off the friend list (a yes or no; FriendList_Update) */
    LOBBY_TEXT_FRIEND_REMOVED = 0x1F,        /* the friend was taken off the list */
    LOBBY_TEXT_HUNTER_SEARCHING = 0x20,      /* a hunter is being looked up (the id search, as HunterInfo_Open starts; the name search) */
    LOBBY_TEXT_ID_TOO_SHORT = 0x23,          /* the id typed is shorter than ID_SEARCH_ID_CHARS */
    LOBBY_TEXT_NAME_EMPTY = 0x24,            /* the name search: no name was typed */
    LOBBY_TEXT_FRIEND_ADD_ASK = 0x25,        /* whether to add the user to the friend list (a yes or no; FriendList_AskAdd) */
    LOBBY_TEXT_FRIEND_IS_SELF = 0x26,        /* the user to add is the player */
    LOBBY_TEXT_FRIEND_ADD_NO = 0x28,         /* the friend's adding is answered no */
    LOBBY_TEXT_FRIEND_ADDED = 0x29,          /* the user is added to a free entry */
    LOBBY_TEXT_FRIEND_UPDATED = 0x2A,        /* the user's entry, already listed, is written again */
    LOBBY_TEXT_FRIEND_LISTED_ASK = 0x2B,     /* the user is already listed: write the entry again? (a yes or no) */
    LOBBY_TEXT_FRIEND_NEW_ASK = 0x2C,        /* the user isn't listed yet: add? (a yes or no) */
    LOBBY_TEXT_FRIEND_LIST_FULL = 0x2D,      /* the friend list has no free entry */
    LOBBY_TEXT_BLOCK_ADD_ASK = 0x30,         /* whether to add the user to the mail block list (BlockList_AskAdd) */
    LOBBY_TEXT_BLOCK_ADD_NO = 0x31,          /* the user isn't added: the player, or answered no */
    LOBBY_TEXT_BLOCK_ADDED = 0x32,           /* the user is added to a free entry */
    LOBBY_TEXT_BLOCK_UPDATED = 0x33,         /* the user's entry, already listed, is written again */
    LOBBY_TEXT_BLOCK_LISTED_ASK = 0x34,      /* the user is already listed: write the entry again? */
    LOBBY_TEXT_BLOCK_NEW_ASK = 0x35,         /* the user isn't listed yet: add? */
    LOBBY_TEXT_BLOCK_LIST_FULL = 0x36,       /* the block list has no free entry */
    LOBBY_TEXT_BLOCK_REMOVE_ASK = 0x37,      /* whether to take the user off the block list (BlockList_AskRemove) */
    LOBBY_TEXT_BLOCK_KEPT = 0x38,            /* the user stays on the block list (answered no) */
    LOBBY_TEXT_BLOCK_REMOVED = 0x39,         /* the user is taken off the block list */
    LOBBY_TEXT_LEAVE_ASK = 0x3F,             /* whether to leave the lobby server (a yes or no; the move menu) */
    LOBBY_TEXT_GROUP_DISBANDED = 0x49,       /* the chosen entry was closed meanwhile (the area board's join) */
    LOBBY_TEXT_GROUP_OPENING = 0x4B,         /* a free entry of the group board is being opened as the player's group */
    LOBBY_TEXT_ID_IS_SELF = 0x50,            /* the id typed is the player's own */
    LOBBY_TEXT_QUIT_ASK = 0x51,              /* whether to quit the menu (a yes or no): the character select, the server choice */
    LOBBY_TEXT_GROUP_SEARCHING = 0x54,       /* the group search is running */
    LOBBY_TEXT_GROUP_NONE_FOUND = 0x55,      /* the group search found no group (circle goes back to the form) */
    LOBBY_TEXT_GROUP_FETCHING = 0x56,        /* a group's details are being fetched (also before joining one of the area's groups) */
    LOBBY_TEXT_SAVE_ASK = 0x5D,              /* asks whether to save (the bed's menu, offline) */
    LOBBY_TEXT_ROOM_CLOSED = 0x6C,           /* the quest room has closed (LobbyRoom_BeginClose's kind 0) */
    LOBBY_TEXT_SET_OUT_SHORT_ASK = 0x6D,     /* the quest's payer at the gate with members missing: set out? (TOWN_MODE_SET_OUT_ASK) */
    LOBBY_TEXT_SET_OUT_CANCEL_ASK = 0x6E,    /* a member at the gate after setting out: cancel? (TOWN_MODE_SET_OUT_CANCEL_ASK) */
    LOBBY_TEXT_SET_OUT_FULL_ASK = 0x6F,      /* the payer at the gate with the room full: set out? (TOWN_MODE_SET_OUT_ASK_FULL) */
    LOBBY_TEXT_ORDER_TERMS = 0x64,           /* the quest's order terms are not met (Quest_CheckOrderTerms) */
    LOBBY_TEXT_QUEST_DESK_BUSY = 0x69,       /* no quest room is free: the quest can't be ordered now (LobbyLevel2_EnterFree) */
    LOBBY_TEXT_SERVER_INTRO = 0x76,          /* shown before the server choice's list when the screen asks the player to pick a server (SrvMenu_Intro) */
    LOBBY_TEXT_ROOM_LEAVING = 0x77,          /* a quest room or a group is being left (while LobbyStep_Level2Leave runs; a new group given up on the group board) */
    LOBBY_TEXT_SENDING = 0x78,               /* a setting or a posting is being sent (the house access setting, the quest desk's posting, a new group's settings; also while a group found is entered) */
    LOBBY_TEXT_RECORDS_NO_REPLY = 0x79,      /* the house's owner didn't send the records asked for in time */
    LOBBY_TEXT_EXIT_ASK = 0x7B,              /* the town's exit spot: leave the town? (TOWN_MODE_EXIT_ASK) */
    LOBBY_TEXT_AREA_MOVING = 0x7D,           /* online, moving to another area (LobbyScreen_LeaveArea) */
    LOBBY_TEXT_GATE_CLOSED = 0x82,           /* the quest gate: no quest is taken, or it isn't this season's or this time of day's (QUEST_ORDER_WRONG_TIME) */
    LOBBY_TEXT_GATE_ORDER_TERMS = 0x83,      /* the quest gate: the quest's order terms aren't met (QUEST_ORDER_TERMS) */
    LOBBY_TEXT_MOVE_ASK = 0x84,              /* the move menu's question: move to another area, or leave the lobby (MoveMenu_Update) */
    LOBBY_TEXT_GROUP_JOIN_ASK = 0x86,        /* whether to join the group found, on the player's own land (a yes or no) */
    LOBBY_TEXT_GROUP_JOIN_MOVE_ASK = 0x87,   /* whether to join the group found, which is on another land (a yes or no) */
    LOBBY_TEXT_GROUP_RANK_REFUSED = 0x88,    /* the group's limit (GROUP_LIMIT_*) doesn't admit the player's hunter rank (the group board, LobbyLogin_EnterEntry, LobbyLevel1_Enter) */
    LOBBY_TEXT_ENTRY_JOIN_ASK = 0x8B,        /* whether to join the chosen entry (a yes or no; the area board's join) */
    LOBBY_TEXT_ARENA_RELEASE_ASK = 0x8C,     /* the arena cup's desk asks whether to release the chosen monster */
    LOBBY_TEXT_RECORDS_WAIT = 0x8E,          /* waiting for the house owner's records */
    LOBBY_TEXT_GROUP_BOARD_ASK = 0x8F,       /* the group board's first question: the area's groups or the group search (a two-way choice; GroupBoard_Open) */
    LOBBY_TEXT_PASSWORD_EMPTY = 0x90,        /* the house password typed is empty */
    LOBBY_TEXT_ARENA_TERMS_UNMET = 0x92,     /* the arena cup's quest: the order terms are not met (Quest_CheckOrderTerms) */
    LOBBY_TEXT_FRIEND_RESTRICTED = 0x93,     /* shown instead of LOBBY_TEXT_FRIEND_ADD_ASK when the member's friend limit is set (HunterCard.friendLimit; LobbyMember_ListInput) */
    LOBBY_TEXT_RECRUIT_TEXT_NONE = 0x94      /* no recruiting text is picked yet (the area board's recruit form) */
};

/*
 * The dialog's kinds (gLobbyDialogKind): a message's flags without LOBBY_MSG_DIM
 * (LobbyText_Open), or set by LobbyDialog_OpenWait and LobbyDialog_OpenCountdown.
 */
enum LobbyDialogKind {
    DIALOG_TEXT = 0,        /* the text alone */
    DIALOG_WAIT = 1,        /* a text of its own over a dimmed screen; "press a button" once no leave is pending */
    DIALOG_PAGES = 2,       /* no box: a page of a text with the page number, or a "none now" text, and "back" */
    DIALOG_COUNTDOWN = 3,   /* a text of its own and a countdown in seconds, then "press a button" */
    DIALOG_YES_NO = 4,      /* the text, then yes and no (gLobbyDialogChoice, 0 yes) */
    DIALOG_OK = 5,          /* the text, then yes alone */
    DIALOG_SAVE_QUIT = 6,   /* the text, then save, or quit without saving */
    DIALOG_CHOICE2 = 7,     /* a box of its own; the bar on one of two lines of the text */
    DIALOG_CHOICES = 8,     /* the same box; the bar on line gLobbyDialogChoice of a list */
    DIALOG_BLINK = 9,       /* the text, shown 20 frames of every 40 */
    DIALOG_DIM = 10,        /* only the dimmed screen */
    DIALOG_NONE = 11        /* nothing */
};

/* Notices of gLobbyNoticeTexts that Menu_PushNoticeOnce shows, named by
 * the role their callers give them. */
enum LobbyNotice {
    LOBBY_NOTICE_GUNNER_ONLY = 0x00, /* the shop: the item chosen is ammo and the weapon isn't a bowgun (Shop_BuyListInput) */
    LOBBY_NOTICE_QUEST_TAKEN = 0x01, /* a menu is refused while a quest is taken (gLobby->questTaken): the game menu's exit, the ship's boarding and three other menus */
    LOBBY_NOTICE_PASSWORD_EMPTY = 0x02, /* a room's password is turned on but empty (the quest desk's and the other posting menus' room settings) */
    LOBBY_NOTICE_BOX_UPGRADE_1 = 0x05, /* ITEM_ID_BOX_UPGRADE_1 is bought (Shop_Buy) */
    LOBBY_NOTICE_BOX_UPGRADE_2 = 0x06, /* ITEM_ID_BOX_UPGRADE_2 is bought */
    LOBBY_NOTICE_HOUSE_UPGRADED = 0x07, /* a house upgrade is bought and the house isn't at its top level yet */
    LOBBY_NOTICE_HOUSE_TOP = 0x08,  /* a house upgrade is bought and the house is at its top level (VILLAGE_FACILITY_HOUSE 2) */
    LOBBY_NOTICE_BOX_FULL = 0x0B,   /* the equipment box has no free slot (a forge's recipe list opens) */
    LOBBY_NOTICE_BLADE_ONLY = 0x0D, /* the shop: the item chosen is for blademasters and the weapon is a bowgun (ITEM_ID_SHARPEN_150) */
    LOBBY_NOTICE_BOW_ONLY = 0x0E,   /* the shop: the item chosen is a coating and the weapon isn't a bow */
    LOBBY_NOTICE_SENT_TO_BOX = 0x0F, /* what was got went to the box: a workshop counter's new or changed piece that isn't worn, or items past the pouch's room (Items_Give returned 1) */
    LOBBY_NOTICE_NO_HUNTER_RANK = 0x10, /* the quest counter's spot at hunter rank 0 */
    LOBBY_NOTICE_GUN_NO_AMMO = 0x13, /* the hunter's gun has no ammo (Equip_GunHasNoAmmo), shown when a quest menu opens */
    LOBBY_NOTICE_POUCH_FULL = 0x15, /* the pouch has no room for the item given (a resident's gift, the mine's find) */
    LOBBY_NOTICE_NO_MONEY = 0x16,   /* the price is more than the money held (the bed's menu, the shooting gallery's fee) */
    LOBBY_NOTICE_EQUIPPED = 0x17,   /* a workshop counter's new or changed piece is now worn */
    LOBBY_NOTICE_BAIT_CHOSEN = 0x18, /* the bait menu: a bait is chosen and the fishing starts */
    LOBBY_NOTICE_BAIT_GONE = 0x19,  /* the bait menu: the bait in use ran out while other bait is held */
    LOBBY_NOTICE_ARENA_CUP_MEMBERS = 0x1A, /* the arena cup's quest at the gate, its room not at the members it needs */
    LOBBY_NOTICE_ARENA_CUP_HOURS = 0x1B, /* the arena cup's quest at the gate in the morning (it sets out in the afternoon) */
    LOBBY_NOTICE_NO_FREE_SLOT = 0x1C, /* the decorations counter: the box piece chosen has no free slot */
    LOBBY_NOTICE_NO_DECO = 0x1D,    /* the decorations counter: the box piece chosen holds no decoration */
    LOBBY_NOTICE_DECO_ATTACHED = 0x1E, /* the decorations counter: the decoration is attached */
    LOBBY_NOTICE_DECO_REMOVED = 0x1F, /* the decorations counter: the decoration is removed */
    LOBBY_NOTICE_SONG_TRACK3_FIRST = 0x21, /* shown with the first request of the song menu's track 3 (SongMenu_OnAccepted) */
    LOBBY_NOTICE_ARENA_RELEASE_REFUSED = 0x22, /* the arena cup's desk: a monster can't be released while the cup's quest is taken */
    LOBBY_NOTICE_EQUIP_GOT = 0x23,  /* a townsperson's gift of equipment went into the box (Talk_Update) */
    LOBBY_NOTICE_CANNOT_WEAR = 0x25, /* the armour forge: the chosen upgrade can't be worn by this hunter (Equip_CanWear) */
    LOBBY_NOTICE_HOUSE_OPEN = 0x26, /* the house's Poogie while the house is open to visitors (NPC_STATE_HOUSE_OPEN): no menu */
    LOBBY_NOTICE_MEAL_MONEY = 0x27, /* the kitchen: money given with a dish eaten online with others (Kitchen_Update) */
    LOBBY_NOTICE_SONG_UNAVAILABLE = 0x28   /* the requested track can't be played now, or the host refused it (the song menu) */
};

/* The screen titles of gLobbyTitleTexts (LobbyTitle_Set), named by the
 * screen that sets each. */
enum LobbyTitle {
    LOBBY_TITLE_LOGIN = 0,          /* the login's steps */
    LOBBY_TITLE_INFO = 1,           /* the server's information pages at login */
    LOBBY_TITLE_SERVER_MENU = 2,    /* the server choice menu (SrvMenu_Start) */
    LOBBY_TITLE_AREA_LIST = 3,      /* the area list (AreaList_Init) */
    LOBBY_TITLE_ID_MENU = 4         /* the character select menu (IdMenu_*) */
};

#define LOBBY_MSG_DIM 0x80          /* LobbyMsgDef.flags: dim the screen behind the box */

/* The chat's system messages of gLobbyEventTexts (Chat_SystemMsg), named
 * by what they announce; most print a member's name or id into the text. */
enum LobbyEventText {
    LOBBY_EVENT_ALL_READY = 0,      /* the room's members reached the quest's least number */
    LOBBY_EVENT_ENTRY_CANCELLED = 1, /* they fell below it again */
    LOBBY_EVENT_USER_COMING = 2,    /* a user is on the way to the town (route 6411) */
    LOBBY_EVENT_USER_ARRIVED = 3,   /* a user arrived in the town */
    LOBBY_EVENT_MEMBER_JOINED = 4,  /* a user joined the quest room */
    LOBBY_EVENT_MEMBER_LEFT = 5,    /* a user left it */
    LOBBY_EVENT_CHAT_SEND_ERROR = 6, /* the chat could not be sent (its settings) */
    LOBBY_EVENT_USER_DEPARTED = 7,  /* a user set out on a quest */
    LOBBY_EVENT_USER_LEFT_TOWN = 8, /* a user left the town (route 6410) */
    LOBBY_EVENT_QUEST_READY = 9,    /* the quest is ready to set out */
    LOBBY_EVENT_TEXT = 10,          /* the argument alone */
    LOBBY_EVENT_TOWN_CHECKING = 11  /* the town's state is being checked (sent as chat during a raid's movie) */
};

#endif /* LOBBY_TEXT_H */
