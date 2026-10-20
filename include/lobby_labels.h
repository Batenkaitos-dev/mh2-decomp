#ifndef LOBBY_LABELS_H
#define LOBBY_LABELS_H

/*
 * The lobby's label tables: the words, column heads, prompts and help
 * lines its screens print, defined at the end of the item shop's file
 * (each entry a pointer to one string). Each table holds one screen's
 * labels, or one list a screen indexes; the enums name the entries the code
 * reads by number.
 *
 * Every table of 16 bytes or more starts on a 16-byte boundary and every
 * smaller one on an 8-byte boundary, so a label that sits at an address
 * 4 past such a boundary can only be the second entry of the table before
 * it: some tables therefore hold labels of two screens. Where the game's
 * code loads an entry as an object of its own, the table is cut there, or
 * the entry is read under a name of its own (a linker alias).
 */

/* gLobbyWords: the lobby's common words and marks, read by many screens */
extern char *gLobbyWords[20];
enum LobbyWord {
    LOBBY_WORD_QUESTION = 0,
    LOBBY_WORD_PIECES = 1,              /* the counter for items */
    LOBBY_WORD_ZENNY = 2,               /* the money mark after a price */
    LOBBY_WORD_INFINITY = 3,
    LOBBY_WORD_TIMES = 4,               /* the times sign before a count */
    LOBBY_WORD_MID_DOT = 5,
    LOBBY_WORD_WIDE_SPACE = 6,
    LOBBY_WORD_ELLIPSIS = 7,
    LOBBY_WORD_POINTS = 8,              /* the points mark */
    LOBBY_WORD_NAME_ID_FMT = 9,         /* the name and ID line */
    LOBBY_WORD_LAND = 10,
    LOBBY_WORD_TILDE = 11,
    LOBBY_WORD_LODGING_FEE = 12,
    LOBBY_WORD_COLON = 13,              /* the colon between a label and its value */
    LOBBY_WORD_NUM_BRACKET_FMT = 14,
    LOBBY_WORD_UNREGISTERED = 15,       /* an empty name or ID */
    LOBBY_WORD_RARE = 16,               /* the rarity prefix */
    LOBBY_WORD_PERSONS = 17,            /* the counter for people */
    LOBBY_WORD_SELECT = 18,
    LOBBY_WORD_MINUTES = 19,            /* the minutes unit */
};

/* gNoneSetTexts: "none" and "set", indexed by whether a password is set */
extern char *gNoneSetTexts[2];

/* gCountLabels: the column heads of a count */
extern char *gCountLabels[2];
enum CountLabel {
    COUNT_LABEL_PEOPLE = 0,             /* the number of people */
    COUNT_LABEL_HELD = 1,               /* the number held */
};

/* gGameMenuLabels: the lobby game menu's entries and guide lines, then the price board's unknown name and title */
extern char *gGameMenuLabels[32];
enum MenuLabel {
    MENU_LABEL_DETAILS = 0,
    MENU_LABEL_DETAILS_GUIDE = 1,       /* the details button's guide */
    MENU_LABEL_MATERIALS = 2,
    MENU_LABEL_DEPART_READY = 3,
    MENU_LABEL_DEPART_CANCEL = 4,
    MENU_LABEL_MENU_GUIDE = 5,          /* the menu button's guide */
    MENU_LABEL_MOVE = 6,
    MENU_LABEL_ENTER_TOWN = 7,
    MENU_LABEL_AREA_MOVE = 8,
    MENU_LABEL_LAND_MOVE = 9,
    MENU_LABEL_SEARCH = 10,
    MENU_LABEL_FRIEND_LIST = 11,
    MENU_LABEL_PLAYER_SEARCH = 12,
    MENU_LABEL_QUEST_SEARCH = 13,
    MENU_LABEL_INFO = 14,
    MENU_LABEL_SHORT_MAIL = 15,
    MENU_LABEL_EVENT_INFO = 16,
    MENU_LABEL_OWN_INFO = 17,
    MENU_LABEL_STATUS = 18,
    MENU_LABEL_COMMENT = 19,
    MENU_LABEL_STATE_SETTING = 20,
    MENU_LABEL_MAIL_CHAT = 21,
    MENU_LABEL_PHRASE_EDIT = 22,
    MENU_LABEL_CHAT_LOG = 23,
    MENU_LABEL_DISCONNECT = 24,
    MENU_LABEL_HUNTERS_SITE = 25,
    MENU_LABEL_LOGOUT = 26,
    MENU_LABEL_SECURITY = 27,
    MENU_LABEL_CHAT_LIMIT = 28,
    MENU_LABEL_MAIL_LIMIT = 29,
    MENU_LABEL_UNKNOWN_NAME = 30,       /* the name of something not yet seen */
    MENU_LABEL_PRICE_INFO = 31,         /* the price board's title */
};

/* gPriceInfoTexts: the monster price board's lines */
extern char *gPriceInfoTexts[12];
enum PriceInfo {
    PRICE_INFO_TITLE = 0,
    PRICE_INFO_WANTED_A = 1,            /* the WANTED lines by class, A to C */
    PRICE_INFO_HALL_TAVERN = 4,         /* the halls a request is posted in, by list */
    PRICE_INFO_HALL_ELDER = 5,
    PRICE_INFO_HALL_WANTED = 6,
    PRICE_INFO_NONE = 7,
    PRICE_INFO_HUNT_FMT = 8,            /* the goals, by kind from 1: hunt, capture, deliver */
    PRICE_INFO_CAPTURE_FMT = 9,
    PRICE_INFO_DELIVER_FMT = 10,
    PRICE_INFO_BRACKET_FMT = 11,        /* a name in brackets, also used by other screens */
};

/* gListTitles: the titles and guides of the town's selection windows */
extern char *gListTitles[16];
enum ListTitle {
    LIST_TITLE_CLEAR = 0,               /* the quest list's clear mark */
    LIST_TITLE_DECIDE_GUIDE = 1,
    LIST_TITLE_CANCEL_GUIDE = 2,
    LIST_TITLE_SEE_STATE = 3,           /* the name tags' look-at prompt */
    LIST_TITLE_TARGET = 4,
    LIST_TITLE_LAND_LIST = 5,
    LIST_TITLE_LAND_NAME = 6,
    LIST_TITLE_AREA_LIST = 7,
    LIST_TITLE_AREA_NAME = 8,
    LIST_TITLE_PRIZE_CHECK = 9,
    LIST_TITLE_INGREDIENT = 10,
    LIST_TITLE_BAIT = 11,
    LIST_TITLE_FISHING_FMT = 12,
    LIST_TITLE_BACK_TO_BAIT = 13,
    LIST_TITLE_PICKAXE = 14,
    LIST_TITLE_HUNTER_RECRUIT = 15,
};

/* gQuestBoardRows: the quest board's rows: the desks, the quest kinds, the taker */
extern char *gQuestBoardRows[12];
enum BoardRow {
    BOARD_ROW_TAVERN = 0,               /* the desks, by desk */
    BOARD_ROW_ARENA = 1,
    BOARD_ROW_ELDER_HALL = 2,
    BOARD_ROW_FREE = 3,                 /* the quest kinds, by desk */
    BOARD_ROW_OFFICIAL = 4,
    BOARD_ROW_DOJO = 5,
    BOARD_ROW_ARENA_CUP = 6,
    BOARD_ROW_TUTORIAL = 7,
    BOARD_ROW_TAKER = 8,
    BOARD_ROW_MEMBERS_FMT = 9,
    BOARD_ROW_WANTED = 10,
    BOARD_ROW_NO_TAKER = 11,
};

/* gTownLabels: labels and help lines of the town's small windows */
extern char *gTownLabels[24];
enum TownLabel {
    TOWN_LABEL_ZENNY_GOT_FMT = 0,       /* a talk's money reward */
    TOWN_LABEL_OPEN = 1,                /* the house access states */
    TOWN_LABEL_CLOSE = 2,
    TOWN_LABEL_PASSWORD = 3,
    TOWN_LABEL_GONE_OUT = 4,
    TOWN_LABEL_HOUSE_ACCESS = 5,
    TOWN_LABEL_MY_HOUSE = 6,
    TOWN_LABEL_CLOSE_ASK = 7,
    TOWN_LABEL_NEEDED_SLOTS = 8,        /* the item info's slots line */
    TOWN_LABEL_BAIT_HELP = 9,
    TOWN_LABEL_PICKAXE_HELP = 10,
    TOWN_LABEL_NO_PICKAXE = 11,
    TOWN_LABEL_INGREDIENT_LIST = 12,
    TOWN_LABEL_NO_INGREDIENTS = 13,
    TOWN_LABEL_ZOOM = 14,               /* the equipment preview's guide */
    TOWN_LABEL_MOVE = 15,
    TOWN_LABEL_BACK_GUIDE = 16,
    TOWN_LABEL_BLOCK_LIST = 17,         /* the mail block list's title */
    TOWN_LABEL_HOST_CHOOSING = 18,      /* the kitchen guest's wait line */
    TOWN_LABEL_TIME_LEFT = 19,
    TOWN_LABEL_HUNTER_DOJO = 20,
    TOWN_LABEL_NO_LIMIT = 21,           /* the friend limit's two choices */
    TOWN_LABEL_LIMIT = 22,
    TOWN_LABEL_SHOW_ON = 23,            /* the member names' title */
};

/* gShipMenuTexts: the ship menu's entries, drawn as a list */
extern char *gShipMenuTexts[2];

/* gTownGuideTopics: the town guide's groups, by group */
extern char *gTownGuideTopics[3];

/* gTownGuideHelp: the town guide's help lines by group, then its scroll arrows */
extern char *gTownGuideHelp[8];
enum TownGuide {
    TOWN_GUIDE_ARROWS = 3,
    TOWN_GUIDE_ARROWS_DIM = 4,
};

/* gVillageBoardTitles: the village board's title lines and prompts */
extern char *gVillageBoardTitles[4];
enum VillageBoard {
    VILLAGE_BOARD_TITLE = 0,
    VILLAGE_BOARD_TOP_HELP = 1,
    VILLAGE_BOARD_TOP_PROMPT = 2,
    VILLAGE_BOARD_ARTICLE_PROMPT = 3,
};

/* gVillageBoardTexts: the village board's article kinds by kind from 0, then each kind's description from 5 */
extern char *gVillageBoardTexts[12];

/* gElderForecastTexts: the elder dragon forecast's menu, from 0, then its panel's labels */
extern char *gElderForecastTexts[8];
enum ElderForecast {
    ELDER_FORECAST_HABITAT = 0,
    ELDER_FORECAST_ATTACK = 1,
    ELDER_FORECAST_EVENTS = 2,
    ELDER_FORECAST_PANEL_TITLE = 3,
    ELDER_FORECAST_NO_TIME = 4,
    ELDER_FORECAST_DAY = 5,
    ELDER_FORECAST_NIGHT = 6,
    ELDER_FORECAST_DAY_NIGHT = 7,
};

/* gHairMenuTexts: the hair style menu's labels */
extern char *gHairMenuTexts[8];
enum HairMenu {
    HAIR_MENU_TITLE = 0,
    HAIR_MENU_PRICE_HEAD = 1,
    HAIR_MENU_COLOR_ASK = 2,
    HAIR_MENU_COLOR_HELP = 3,
    HAIR_MENU_SHOW_HINT = 4,
};

/* gRecordsMenuTexts: the records menu's pages, by RecordKind from 0, then its prompts */
extern char *gRecordsMenuTexts[24];
enum RecordsMenu {
    RECORDS_MENU_HOUSE_ACCESS = 0,
    RECORDS_MENU_HUNTER_GUIDE = 1,
    RECORDS_MENU_HUNTS = 2,
    RECORDS_MENU_ITEMS = 3,
    RECORDS_MENU_WEAPONS = 4,
    RECORDS_MENU_ARMOR = 5,
    RECORDS_MENU_TITLES = 6,
    RECORDS_MENU_HORN_SCORES = 7,
    RECORDS_MENU_ANCIENT_SCORES = 8,
    RECORDS_MENU_MAIN_PROMPT = 12,
    RECORDS_MENU_ITEM_PROMPT = 13,
    RECORDS_MENU_TITLE_GENRE_PROMPT = 14,
    RECORDS_MENU_TITLE_PROMPT = 15,
    RECORDS_MENU_TITLE_CONFIRM = 16,
    RECORDS_MENU_WEAPON_GENRE_PROMPT = 17,
    RECORDS_MENU_ARMOR_GENRE_PROMPT = 18,
    RECORDS_MENU_NO_WEAPON = 19,
    RECORDS_MENU_NO_ARMOR = 20,
};

/* gHouseAccessTexts: the house access page's rows, by HouseAccessRow */
extern char *gHouseAccessTexts[4];

/* gRecordsListTexts: the records' title genres, item kinds and hunt lines */
extern char *gRecordsListTexts[24];
enum RecordsList {
    RECORDS_LIST_HUNTER_RANK = 0,       /* the title genres, from 0 */
    RECORDS_LIST_UNKNOWN_TITLE = 6,
    RECORDS_LIST_NO_TITLE = 7,
    RECORDS_LIST_ITEM_KINDS = 9,        /* the item kinds, from 9 */
    RECORDS_LIST_UNKNOWN_ITEM = 18,
    RECORDS_LIST_NO_ITEM = 19,
    RECORDS_LIST_HUNT_COUNT_FMT = 20,
    RECORDS_LIST_MAX_SIZE_FMT = 21,
    RECORDS_LIST_MIN_SIZE_FMT = 22,
    RECORDS_LIST_NO_INFO = 23,
};

/* gSongMenuTexts: the song menu's labels */
extern char *gSongMenuTexts[4];
enum SongText {
    SONG_TEXT_TITLE = 0,
    SONG_TEXT_SOUL_SONG = 1,
    SONG_TEXT_DASHES = 2,
};

/* gOfferMenuTexts: the village offer menu's labels */
extern char *gOfferMenuTexts[10];
enum OfferText {
    OFFER_TEXT_DELIVERY = 0,
    OFFER_TEXT_QUEST_ORDER = 1,
    OFFER_TEXT_LIMIT = 2,
    OFFER_TEXT_CLIENT = 3,
    OFFER_TEXT_DELIVER = 4,
    OFFER_TEXT_CANCEL = 5,
    OFFER_TEXT_DELIVERY_PROMPT = 6,     /* the delivery's prompts, by prompt from 6 */
    OFFER_TEXT_ITEM_LINE = 9,
};

/* gQuestPeriodTexts: how long an offer stays, by period */
extern char *gQuestPeriodTexts[2];

/* gOfferHelpTexts: the offered quest's help lines */
extern char *gOfferHelpTexts[8];
enum OfferHelp {
    OFFER_HELP_ORDER_ASK = 0,
    OFFER_HELP_NO_MONEY = 1,            /* the fee is more than the money held */
    OFFER_HELP_QUEST_TAKEN = 2,         /* another quest is taken */
    OFFER_HELP_ORDER_CONFIRM = 3,
    OFFER_HELP_ORDERED = 4,
};

/* gDojoMenuTexts: the hunter dojo's menus and titles */
extern char *gDojoMenuTexts[16];
enum DojoText {
    DOJO_TEXT_FIRST_CHOICE = 0,         /* the first menu's entries, from 0 */
    DOJO_TEXT_REQUEST = 2,
    DOJO_TEXT_DRILL_MENU = 3,           /* the drills menu's entries, from 3 */
    DOJO_TEXT_POINT_EXCHANGE = 4,
    DOJO_TEXT_RULES = 5,
    DOJO_TEXT_END_TRAINING = 6,
    DOJO_TEXT_DRILL_KINDS = 7,          /* the drill kinds, from 7 */
    DOJO_TEXT_EXCHANGE_HELP = 9,
    DOJO_TEXT_EXCHANGE_COUNT_ASK = 10,
    DOJO_TEXT_EXCHANGE_COUNT_HELP = 11,
    DOJO_TEXT_TRAINING = 12,            /* the training categories, from 12 */
};

/* gNetLabels: the network screens' labels: the group and area boards, the
 * searches, the friend and block lists, the hunter info and the short mail */
extern char *gNetLabels[52];
enum NetLabel {
    NET_LABEL_GROUP_BOARD = 0,
    NET_LABEL_NUMBER = 1,
    NET_LABEL_RECRUIT_MSG = 2,
    NET_LABEL_HEADCOUNT = 3,
    NET_LABEL_IN_TOWN = 4,
    NET_LABEL_NAME = 5,
    NET_LABEL_HR = 6,
    NET_LABEL_ID = 7,
    NET_LABEL_CONN_STATE = 8,
    NET_LABEL_WEAPON = 9,
    NET_LABEL_LIMITED = 10,
    NET_LABEL_TARGET = 11,
    NET_LABEL_DASHES = 12,
    NET_LABEL_RECRUIT_MEMBERS = 13,
    NET_LABEL_RECRUIT_MSG_COLON = 14,
    NET_LABEL_DECIDE = 15,
    NET_LABEL_CANCEL = 16,
    NET_LABEL_MEMBER_DETAILS = 17,
    NET_LABEL_SELECT = 18,
    NET_LABEL_JOIN_GROUP = 19,
    NET_LABEL_GROUP_BOARD_TITLE = 20,
    NET_LABEL_MAIL_UNIT = 21,           /* the counter for mails */
    NET_LABEL_LIMIT_COND = 22,
    NET_LABEL_TARGET_COLON = 23,
    NET_LABEL_RECRUIT_MSG_LABEL = 24,
    NET_LABEL_GROUP_SEARCH = 25,
    NET_LABEL_SEARCH_START = 26,
    NET_LABEL_SEARCH_RESULTS = 27,
    NET_LABEL_LOCATION = 28,
    NET_LABEL_NOT_REGISTERED = 29,
    NET_LABEL_CHECKING = 30,            /* the connection states, from 30 */
    NET_LABEL_ONLINE = 31,
    NET_LABEL_ON_QUEST = 32,
    NET_LABEL_OFFLINE = 33,
    NET_LABEL_STATUS = 34,                  /* the friend list's actions, from 34 */
    NET_LABEL_DELETE = 35,
    NET_LABEL_ONLINE_MARK = 36,
    NET_LABEL_GROUP_MEMBERS = 37,
    NET_LABEL_TITLE = 38,
    NET_LABEL_WEAPON_COLON = 39,
    NET_LABEL_SYMBOL_COLOR = 40,
    NET_LABEL_ARENA_CUP = 41,
    NET_LABEL_WINS = 42,
    NET_LABEL_COMMENT = 43,
    NET_LABEL_MAIL_BODY = 44,
    NET_LABEL_SEND = 45,
    NET_LABEL_MARK = 46,
    NET_LABEL_FRIEND_LIST = 47,             /* the short mail's address choices, from 47 */
    NET_LABEL_KEYBOARD = 48,
    NET_LABEL_AREA_BOARD_HELP = 49,
    NET_LABEL_JOIN_PROMPT = 50,
    NET_LABEL_AREA_JOIN_PROMPT = 51,
};

/* gGroupSearchHelpTexts: the group search form's help line */
extern char *gGroupSearchHelpTexts[2];

/* gMemberActTexts: the hunter info's actions, by action */
extern char *gMemberActTexts[3];

/* gMailActTexts: the mailbox's actions, by action */
extern char *gMailActTexts[3];

/* gGroupLimitTexts: a group's limit, by GroupLimit, then the same in short from 3 */
extern char *gGroupLimitTexts[8];

/* gGroupLimitNewTexts: the search form's limit choices for a new hunter, by rank */
extern char *gGroupLimitNewTexts[4];

/* gGroupLimitExpTexts: the search form's limit choices for an expert, by rank */
extern char *gGroupLimitExpTexts[4];

/* gArenaCupTexts: the arena cup's entry form, records, monster list and monster pages */
extern char *gArenaCupTexts[40];
enum ArenaText {
    ARENA_TEXT_FORM_TITLE = 0,
    ARENA_TEXT_PASSWORD = 1,
    ARENA_TEXT_RULE = 2,
    ARENA_TEXT_TIME_LIMIT = 3,
    ARENA_TEXT_COMMENT = 4,
    ARENA_TEXT_ENTRY = 5,
    ARENA_TEXT_QUIT = 6,
    ARENA_TEXT_APPLY = 7,
    ARENA_TEXT_MONSTER_CHECK = 8,
    ARENA_TEXT_TOTAL_RECORD = 9,
    ARENA_TEXT_RECORD_FMT = 10,
    ARENA_TEXT_BATTLES = 11,
    ARENA_TEXT_WINS = 12,
    ARENA_TEXT_LOSSES = 13,
    ARENA_TEXT_BATTLE_TIME = 14,
    ARENA_TEXT_MONSTER_LIST = 15,
    ARENA_TEXT_CONDITION = 16,
    ARENA_TEXT_NAME = 17,
    ARENA_TEXT_KIND = 18,
    ARENA_TEXT_UNSET = 19,
    ARENA_TEXT_LIST_HELP = 21,
    ARENA_TEXT_CHECK_HELP = 22,
    ARENA_TEXT_MONSTER_MENU = 23,       /* the monster menu's entries, from 23 */
    ARENA_TEXT_MENU_HELP = 26,
    ARENA_TEXT_MONSTER_INFO = 27,
    ARENA_TEXT_INFO_LABELS = 28,        /* the monster info page's labels, from 28 */
    ARENA_TEXT_PREPARE = 37,
    ARENA_TEXT_TIME_LEFT = 38,          /* also the group and area boards' recruiting time */
    ARENA_TEXT_SECONDS = 39,
};

/* gArenaFeedTexts: the arena feed list's lines, then the digits */
extern char *gArenaFeedTexts[14];
enum ArenaFeed {
    ARENA_FEED_TITLE = 0,
    ARENA_FEED_NO_FEED = 1,
    ARENA_FEED_DIGITS = 4,              /* the digits 0 to 9, from 4 */
};

/* gLobbyMarks: two marks: the quest sheet's star and the page counter's slash */
extern char *gLobbyMarks[2];
enum LobbyMark {
    LOBBY_MARK_STAR = 0,                /* before a quest's star rank */
    LOBBY_MARK_SLASH = 1,               /* between a page number and the page count */
};

/* gMemberCountTexts: the member counts by count - 1 (one to four), then two labels */
extern char *gMemberCountTexts[6];
enum MemberCount {
    MEMBER_COUNT_QUEST_MEMBERS = 4,     /* the label of a quest's member count */
    MEMBER_COUNT_SYSTEM_NAME = 5,       /* the speaker name the chat gives the system's messages */
};

/* gMiscTexts: two unrelated lines that share one 8-byte object */
extern char *gMiscTexts[2];
enum MiscText {
    MISC_TEXT_NO_COMBINE = 0,           /* the combine shop's empty list */
    MISC_TEXT_PRESS_BUTTON = 1,         /* the press-a-button prompt */
};

/* gIdMenuTexts: the ID menu's labels */
extern char *gIdMenuTexts[2];
enum IdMenuText {
    ID_MENU_TEXT_NAME = 0,              /* the character name's label */
};

/* gWeaponArmorTexts: the equipment shop's two lists: weapons, armour */
extern char *gWeaponArmorTexts[2];

/* gEquipGroupTexts: the equipment groups: the weapon classes, then the armour parts */
extern char *gEquipGroupTexts[12];
enum EquipGroupText {
    EQUIP_GROUP_TEXT_WEAPONS = 0,       /* the five weapon classes, from 0 */
    EQUIP_GROUP_TEXT_ARMOR = 5,         /* the five armour parts, from 5, in gEquipTabTypes' order */
};

/* gEquipLimitTexts: who can wear a piece, by the piece's wearer limit */
extern char *gEquipLimitTexts[8];

/* gEquipNowTexts: the equip-now and send-home choice, by the yes/no cursor */
extern char *gEquipNowTexts[4];

/* gEquipShopTexts: the equipment shop's notes and questions */
extern char *gEquipShopTexts[12];
enum EquipShopText {
    EQUIP_SHOP_TEXT_PICK_WEAPON = 0,    /* pick a weapon to buy */
    EQUIP_SHOP_TEXT_PICK_ARMOR = 1,     /* pick armour to buy */
    EQUIP_SHOP_TEXT_PRICE_ASK = 2,      /* the price, then a yes/no question */
    EQUIP_SHOP_TEXT_EQUIP_ASK = 3,
    EQUIP_SHOP_TEXT_WEAPON_CHANGE_WARN = 5,/* changing the weapon takes armour off */
    EQUIP_SHOP_TEXT_CANNOT_WEAR = 6,    /* the hunter type can't wear it */
    EQUIP_SHOP_TEXT_PICK_SELL = 7,      /* pick a piece to sell */
    EQUIP_SHOP_TEXT_PICK_SELL_IN_SET = 9,/* the same, for a piece registered in an equipment set */
};

/* gSmithyMenuTexts: the workshop's first menu */
extern char *gSmithyMenuTexts[4];

/* gWorkshopTexts: the workshop's make menu, then the weapon forge's empty-list line */
extern char *gWorkshopTexts[4];
enum WorkshopText {
    WORKSHOP_TEXT_MAKE_MENU = 0,        /* the make menu's three entries, from 0 */
    WORKSHOP_TEXT_NO_WEAPON_TO_MAKE = 3,
};

/* gNoArmorToMakeTexts: the armour forge's empty-list line */
extern char *gNoArmorToMakeTexts[4];

/* gWeaponForgeTexts: the weapon forge's titles, by its choice: make, upgrade */
extern char *gWeaponForgeTexts[2];

/* gArmorForgeTexts: the armour forge's titles */
extern char *gArmorForgeTexts[2];
enum ArmorForgeTitle {
    ARMOR_FORGE_TITLE_MAKE = 0,
    ARMOR_FORGE_TITLE_UPGRADE = 1,
};

/* gDecoMenuTexts: the decorations counter's titles, then the make menu's parts */
extern char *gDecoMenuTexts[8];
enum DecoTitle {
    DECO_TITLE_MAKE = 0,
    DECO_TITLE_ATTACH = 1,
    DECO_TITLE_REMOVE = 2,
};

/* gWeaponClassTexts: the weapon classes, by the forge's row */
extern char *gWeaponClassTexts[8];

/* gForgeTexts: the forges' and the decorations counter's questions, headers and notes */
extern char *gForgeTexts[68];
enum ForgeText {
    FORGE_TEXT_NO_UPGRADE = 3,          /* the piece has no upgrade */
    FORGE_TEXT_CANNOT_UPGRADE = 4,
    FORGE_TEXT_PICK_WEAPON_UPGRADE = 5, /* pick a weapon to upgrade */
    FORGE_TEXT_PICK_UPGRADE_KIND = 6,
    FORGE_TEXT_PICK_ARMOR_UPGRADE = 7,  /* pick armour to upgrade */
    FORGE_TEXT_MATERIALS_ASK = 8,       /* the materials and the fee, then a yes/no question */
    FORGE_TEXT_EQUIP_ASK = 9,
    FORGE_TEXT_WEAPON_CHANGE_WARN = 10, /* changing the weapon takes armour off */
    FORGE_TEXT_MATERIALS_FEE_ASK = 11,  /* an upgrade's materials and fee, then a question */
    FORGE_TEXT_UPGRADES_HEAD = 12,      /* the upgrade list's header */
    FORGE_TEXT_MODS_HEAD = 13,          /* the mod list's header */
    FORGE_TEXT_EMPTY_LINE = 14,         /* an upgrade list's empty line */
    FORGE_TEXT_MATERIALS_HEAD = 15,     /* the needed materials' header */
    FORGE_TEXT_USED_MATERIALS_HEAD = 16,
    FORGE_TEXT_ARMOR_UPGRADE_HEAD = 17,
    FORGE_TEXT_NO_MATERIALS = 18,       /* no materials are needed */
    FORGE_TEXT_GUN_MODS = 19,           /* the gun mods' names, indexed by enum GunMod (GUN_MOD_LEVEL_UP is 19) */
    FORGE_TEXT_GUN_MOD_HELP = 30,       /* each gun mod's help, GUN_MOD_HELP_OFFSET past its name */
    FORGE_TEXT_MOD_FEE_ASK = 41,        /* a mod's fee, then a question */
    FORGE_TEXT_MOD_FEE_DROPS_SILENCER = 42,/* the same, saying the fitted silencer comes off */
    FORGE_TEXT_MOD_FEE_DROPS_BARREL = 43,
    FORGE_TEXT_MOD_FEE_DROPS_SHIELD = 44,
    FORGE_TEXT_MOD_FEE_DROPS_HEAVY_BARREL = 45,
    FORGE_TEXT_PICK_WEAPON_TO_MAKE = 46,
    FORGE_TEXT_PICK_ARMOR_TO_MAKE = 47,
    FORGE_TEXT_ATTACH_PICK_PIECE = 48,  /* pick the piece to attach a decoration to */
    FORGE_TEXT_ATTACH_PICK_PIECE_SLOTS = 49,/* the same, with the piece's free slots */
    FORGE_TEXT_ATTACH_NO_FREE_SLOT = 50,
    FORGE_TEXT_ATTACH_NOT_FOR_PIECE = 51,/* the decoration doesn't fit the chosen piece */
    FORGE_TEXT_ATTACH_TOO_FEW_SLOTS = 52,
    FORGE_TEXT_ATTACH_NO_MONEY = 53,
    FORGE_TEXT_ATTACH_PICK_DECO = 54,
    FORGE_TEXT_ATTACH_FEE_ASK = 55,
    FORGE_TEXT_NO_DECO_HELD = 56,
    FORGE_TEXT_REMOVE_PICK_PIECE = 57,  /* pick the piece to take a decoration off */
    FORGE_TEXT_REMOVE_NONE_ATTACHED = 58,
    FORGE_TEXT_REMOVE_PICK_DECO = 59,   /* the removed decoration breaks */
    FORGE_TEXT_REMOVE_FEE_ASK = 60,
    FORGE_TEXT_SELECT_SHOWS_EQUIP = 61, /* the SELECT button shows the equipment */
    FORGE_TEXT_MAKE_PICK_DECO = 62,
    FORGE_TEXT_MAKE_COUNT_ASK = 63,
    FORGE_TEXT_NO_DECO_TO_MAKE = 64,
    FORGE_TEXT_NO_SAMPLE = 65,          /* no sample to show */
};

/* gKitchenTexts: the kitchen's labels, questions and stat-change lines */
extern char *gKitchenTexts[20];
enum KitchenText {
    KITCHEN_TEXT_INGREDIENT_A = 0,      /* the first pick's label */
    KITCHEN_TEXT_INGREDIENT_B = 1,      /* the second pick's label */
    KITCHEN_TEXT_HEALTH_UP = 2,
    KITCHEN_TEXT_HEALTH_DOWN = 3,
    KITCHEN_TEXT_STAMINA_UP = 4,
    KITCHEN_TEXT_STAMINA_DOWN = 5,
    KITCHEN_TEXT_ATTACK_UP = 6,
    KITCHEN_TEXT_ATTACK_DOWN = 7,
    KITCHEN_TEXT_DEFENSE_UP = 8,
    KITCHEN_TEXT_DEFENSE_DOWN = 9,
    KITCHEN_TEXT_AND = 10,              /* the joiner between two stat lines */
    KITCHEN_TEXT_DISH_LABEL = 11,       /* the finished dish's label */
    KITCHEN_TEXT_PICK_TWO = 12,         /* pick two ingredients */
    KITCHEN_TEXT_CONFIRM_DISH = 13,     /* the yes/no question on the dish */
    KITCHEN_TEXT_PICK_TWO_PARTY = 14,   /* pick two ingredients, enough for every diner */
    KITCHEN_TEXT_TODAYS_DISH = 15,      /* the guest menu's title */
    KITCHEN_TEXT_COURSE_SPECIAL = 16,   /* the set courses' names, from 16 */
    KITCHEN_TEXT_COURSE_SERVICE = 17,
};

/* gBoxActionTexts: the item box's first menu, by action */
extern char *gBoxActionTexts[6];

/* gBoxControlTexts: the item box's control guide */
extern char *gBoxControlTexts[2];
enum BoxControlText {
    BOX_CONTROL_TEXT_CAMERA = 0,        /* turn the camera */
    BOX_CONTROL_TEXT_HIDE_MENU = 1,
};

/* gMenuShowTexts: the control guide's show-the-menu line */
extern char *gMenuShowTexts[2];

/* gBoxKindTexts: the box's two lists: items, equipment */
extern char *gBoxKindTexts[2];

/* gEquipMenuTexts: the equipment menu's entries */
extern char *gEquipMenuTexts[8];

/* gEquipSlotTexts: the equipment slots' names, by part */
extern char *gEquipSlotTexts[8];

/* gBoxMenuTexts: the item box's labels and formats */
extern char *gBoxMenuTexts[24];
enum BoxMenuText {
    BOX_MENU_TEXT_PRICE_FMT = 0,        /* a sale's price */
    BOX_MENU_TEXT_TAKEN_FREE = 1,       /* the shop takes the piece for nothing */
    BOX_MENU_TEXT_SKILL_HINT = 2,       /* the bottom line's skill hint */
    BOX_MENU_TEXT_BACK = 3,
    BOX_MENU_TEXT_DETAILS = 5,
    BOX_MENU_TEXT_BACK_GUIDE = 6,       /* the part list's back guide */
    BOX_MENU_TEXT_PICK_PART_FMT = 7,    /* pick the piece to register, by the part's name */
    BOX_MENU_TEXT_MARK = 8,             /* the mark before a set's row */
    BOX_MENU_TEXT_COLOR_LIST = 9,       /* the colour list's title */
    BOX_MENU_TEXT_SET_TITLE_FMT = 10,   /* an equipment set's title and number */
    BOX_MENU_TEXT_NAME_FMT = 11,        /* a piece's colour and name */
    BOX_MENU_TEXT_REGISTER = 12,
    BOX_MENU_TEXT_SKILLS_HEAD = 13,     /* the equipment info's heads */
    BOX_MENU_TEXT_STATUS_HEAD = 14,
    BOX_MENU_TEXT_SKILLS_HEAD_ALT = 15,
    BOX_MENU_TEXT_STATUS_HEAD_ALT = 16,
    BOX_MENU_TEXT_DETAIL_SWITCH = 17,
    BOX_MENU_TEXT_PICK_FMT = 18,        /* the piece list's title, by row colour and part */
    BOX_MENU_TEXT_NO_EQUIP = 19,        /* the piece list is empty */
    BOX_MENU_TEXT_PAGE_FMT = 21,        /* the page and the page count */
};

/* gBoxHelpTexts: the item box's help lines, by message (the field menus' help lists hold it too) */
extern char *gBoxHelpTexts[72];

/* gQuestCounterTexts: the quest counter's board rows, room pages and arena teams */
extern char *gQuestCounterTexts[56];
enum QuestCounterText {
    QUEST_COUNTER_TEXT_ARENA_CUP = 0,   /* the arena cup's title */
    QUEST_COUNTER_TEXT_NOT_TAKEN = 1,   /* a board row with no quest */
    QUEST_COUNTER_TEXT_UNKNOWN_QUEST = 2,/* a quest whose text isn't held */
    QUEST_COUNTER_TEXT_PREPARING = 3,
    QUEST_COUNTER_TEXT_FULL = 4,
    QUEST_COUNTER_TEXT_DEPARTED = 5,
    QUEST_COUNTER_TEXT_DISBANDING = 6,
    QUEST_COUNTER_TEXT_ROOM_LABELS = 7, /* the room and quest pages' labels, from 7 */
    QUEST_COUNTER_TEXT_LEVELS = 28,     /* each quest level and the hunter ranks it needs, in pairs, from 28 */
    QUEST_COUNTER_TEXT_URGENT = 45,
    QUEST_COUNTER_TEXT_EVENT = 46,
    QUEST_COUNTER_TEXT_TEAM_A = 47,     /* team A's heading */
    QUEST_COUNTER_TEXT_TEAM_B = 48,     /* team B's heading */
    QUEST_COUNTER_TEXT_OPEN = 49,       /* an open place in a team */
    QUEST_COUNTER_TEXT_PICK_TEAM = 50,  /* the team choice's prompt */
    QUEST_COUNTER_TEXT_ROLES = 51,      /* the three roles to join as, from 51: A's assist, B's leader, B's assist */
};

/* gQuestSheetTexts: the quest sheets' and the room forms' labels */
extern char *gQuestSheetTexts[36];
enum QuestSheetText {
    QUEST_SHEET_TEXT_FREE_QUEST = 0,
    QUEST_SHEET_TEXT_OFFICIAL_QUEST = 1,
    QUEST_SHEET_TEXT_HUNT_INFO_HEAD = 2,
    QUEST_SHEET_TEXT_INTRO_HEAD = 3,    /* the quest introduction's head */
    QUEST_SHEET_TEXT_MAIN_TARGET = 4,
    QUEST_SHEET_TEXT_MAIN_REWARD = 5,
    QUEST_SHEET_TEXT_MAIN_POINTS = 6,
    QUEST_SHEET_TEXT_SUB_A_REWARD = 7,
    QUEST_SHEET_TEXT_SUB_A_POINTS = 8,
    QUEST_SHEET_TEXT_SUB_B_REWARD = 9,
    QUEST_SHEET_TEXT_SUB_B_POINTS = 10,
    QUEST_SHEET_TEXT_SUB_TARGET_A = 11,
    QUEST_SHEET_TEXT_SUB_TARGET_B = 12,
    QUEST_SHEET_TEXT_SPECIAL_COND = 13,
    QUEST_SHEET_TEXT_CONTRACT_FEE = 14,
    QUEST_SHEET_TEXT_TIME_LIMIT = 15,
    QUEST_SHEET_TEXT_DESTINATION = 16,
    QUEST_SHEET_TEXT_DIFFICULTY = 17,
    QUEST_SHEET_TEXT_CLEAR_COND = 18,
    QUEST_SHEET_TEXT_FAIL_COND = 19,
    QUEST_SHEET_TEXT_CLIENT = 20,
    QUEST_SHEET_TEXT_CLIENT_DETAILS = 21,
    QUEST_SHEET_TEXT_TIME_ATTACK = 22,
    QUEST_SHEET_TEXT_TIME_ATTACK_FMT = 23,/* the time attack's minutes */
    QUEST_SHEET_TEXT_HIGH_SCORE = 24,
    QUEST_SHEET_TEXT_CLEAR_TIME = 25,
    QUEST_SHEET_TEXT_NO_VALUE = 26,     /* the dashes of an empty value */
    QUEST_SHEET_TEXT_MUST_ORDER = 27,
    QUEST_SHEET_TEXT_REWARD = 28,
    QUEST_SHEET_TEXT_RECRUIT = 29,      /* the member recruiting form's title */
    QUEST_SHEET_TEXT_MEMBERS = 30,
    QUEST_SHEET_TEXT_PASSWORD = 31,
    QUEST_SHEET_TEXT_DECIDE = 32,       /* the forms' decide button */
    QUEST_SHEET_TEXT_FIELD_INFO = 33,
    QUEST_SHEET_TEXT_ECOLOGY = 34,      /* the monster ecology's label */
    QUEST_SHEET_TEXT_SPECIALTY_ITEMS = 35,
};

/* gGameMenuHubTexts: the game menu's entries in a hall (GAME_MENU_HUB) */
extern char *gGameMenuHubTexts[12];

/* gGameMenuOnlineTexts: the game menu's entries online (GAME_MENU_ONLINE) */
extern char *gGameMenuOnlineTexts[12];

/* gGameMenuOnlineBusyTexts: the same in the busy colours */
extern char *gGameMenuOnlineBusyTexts[12];

/* gGameMenuOfflineTexts: the game menu's entries offline (GAME_MENU_OFFLINE) */
extern char *gGameMenuOfflineTexts[12];

/* gGameMenuOfflineBusyTexts: the same in the busy colours */
extern char *gGameMenuOfflineBusyTexts[12];

/* gGameMenuHelpTexts: the game menu's help lines, by entry (the field menus' help lists hold it too) */
extern char *gGameMenuHelpTexts[40];

/* gSearchMenuTexts: the search menu's entries, which also title their pages */
extern char *gSearchMenuTexts[4];
enum SearchMenuText {
    SEARCH_MENU_TEXT_FRIEND_LIST = 0,
    SEARCH_MENU_TEXT_AREA = 1,          /* search the area */
    SEARCH_MENU_TEXT_NAME = 2,          /* search by name */
    SEARCH_MENU_TEXT_ID = 3,            /* search by user id */
};

/* gChatPageTexts: the chat pages' titles and labels: the speakers, the muted players, the chat state */
extern char *gChatPageTexts[12];
enum ChatPageText {
    CHAT_PAGE_TEXT_SPEAKERS_TITLE = 0,
    CHAT_PAGE_TEXT_SPEAKERS_TITLE_FMT = 1,/* the same with the page and the page count */
    CHAT_PAGE_TEXT_SPEAKERS_HEAD = 2,   /* the name and id column heads */
    CHAT_PAGE_TEXT_STATE_HEAD = 3,      /* the chat state page's head */
    CHAT_PAGE_TEXT_NO_SPEAKERS = 4,
    CHAT_PAGE_TEXT_MUTED_TITLE = 5,
    CHAT_PAGE_TEXT_MUTED_TITLE_FMT = 6, /* the same with the page and the page count */
    CHAT_PAGE_TEXT_NO_MUTED = 7,
    CHAT_PAGE_TEXT_MUTED_PAGE_TITLE = 8,
    CHAT_PAGE_TEXT_MUTED_LABEL = 9,
};

/* gFriendLimitTexts: the friend limit's two settings: off, on */
extern char *gFriendLimitTexts[2];

/* gPlaceInfoTexts: the place info's labels */
extern char *gPlaceInfoTexts[12];
enum PlaceInfoText {
    PLACE_INFO_TEXT_SEASON = 0,
    PLACE_INFO_TEXT_PLACE_DETAIL = 1,
    PLACE_INFO_TEXT_PLACE = 2,
    PLACE_INFO_TEXT_RECRUIT_MSG = 3,    /* a group's recruiting message */
    PLACE_INFO_TEXT_LIMIT = 4,          /* a group's limit */
    PLACE_INFO_TEXT_TARGET = 5,
    PLACE_INFO_TEXT_AREA_POP = 6,       /* the area's head count */
    PLACE_INFO_TEXT_VILLAGE_HEAD = 7,   /* the village's progress head */
    PLACE_INFO_TEXT_TAVERN = 8,         /* the four facilities' labels, from 8 */
    PLACE_INFO_TEXT_PORT = 9,
    PLACE_INFO_TEXT_SMITHY = 10,
    PLACE_INFO_TEXT_MINE = 11,
};

/* gOnlineMarkNames: the online marks' names, by mark (ONLINE_MARK_*) */
extern char *gOnlineMarkNames[12];

/* gShopMenuTexts: the shops' menu entries; the item shop's list starts at its second */
extern char *gShopMenuTexts[4];
enum ShopMenuText {
    SHOP_MENU_TEXT_COMBINE = 0,         /* the combine request */
    SHOP_MENU_TEXT_BUY = 1,             /* the item shop's list from 1: buy, sell, the gallery */
    SHOP_MENU_TEXT_SELL = 2,
    SHOP_MENU_TEXT_GALLERY = 3,
};

/* gShopBuySellTexts: the shop list of buy, sell and request */
extern char *gShopBuySellTexts[4];

/* gItemShopMenuTexts: the combine shop's menu */
extern char *gItemShopMenuTexts[4];

/* gCombineShopTexts: the combine shop's labels */
extern char *gCombineShopTexts[12];
enum CombineShopText {
    COMBINE_SHOP_TEXT_RECIPE_HEADS = 0, /* the three recipes' heads, from 0 */
    COMBINE_SHOP_TEXT_RECIPE_NAMES = 3, /* the three recipes' names, from 3 */
    COMBINE_SHOP_TEXT_UNKNOWN = 6,      /* an unknown result */
    COMBINE_SHOP_TEXT_RESULT = 7,
    COMBINE_SHOP_TEXT_FEE = 8,
};

/* gShopPromptTexts: the shops' and the gallery's questions and notes */
extern char *gShopPromptTexts[16];
enum ShopPrompt {
    SHOP_PROMPT_COMBINE_COUNT = 0,      /* how many times to combine */
    SHOP_PROMPT_BUY_COUNT = 1,
    SHOP_PROMPT_SELL_COUNT = 2,
    SHOP_PROMPT_PRICE_ASK = 4,          /* the price, then a yes/no question */
    SHOP_PROMPT_SELL_PRICE_ASK = 5,
    SHOP_PROMPT_PICK_COMBINE = 6,
    SHOP_PROMPT_PICK_RECIPE = 8,
    SHOP_PROMPT_PICK_BUY = 9,
    SHOP_PROMPT_PICK_BUY_POUCH_FULL = 10,/* the same, with the pouch full */
    SHOP_PROMPT_PICK_SELL = 11,
    SHOP_PROMPT_SELL_FREE_ASK = 12,     /* the shop takes it for nothing */
    SHOP_PROMPT_PICK_PRIZE = 13,        /* the gallery's prize exchange */
    SHOP_PROMPT_PICK_COURSE = 14,
    SHOP_PROMPT_PRIZES_NOW = 15,        /* the prizes on offer now */
};

/* gBedMenuTexts: the bed's menu: its entries, their help lines, the living cost question */
extern char *gBedMenuTexts[8];
enum BedMenuText {
    BED_MENU_TEXT_ENTRIES = 0,          /* save, nap, deep sleep, from 0 */
    BED_MENU_TEXT_HELP = 3,             /* each entry's help line, from 3 */
    BED_MENU_TEXT_LIVING_COST = 6,      /* the living cost, then a yes/no question */
};

/* gGalleryCourseTexts: the shooting gallery's courses, by course, then its score and shot labels */
extern char *gGalleryCourseTexts[12];

/* gEmptyListTexts: the notes of the lists and searches that come up empty */
extern char *gEmptyListTexts[8];
enum EmptyList {
    EMPTY_LIST_TOWN = 0,                /* nobody in the town */
    EMPTY_LIST_MAIL = 1,                /* no mail received */
    EMPTY_LIST_NO_RESULTS = 2,          /* a search found nobody */
    EMPTY_LIST_NONE_REGISTERED = 3,     /* nobody on the list */
    EMPTY_LIST_NO_QUEST = 4,            /* no quest taken */
    EMPTY_LIST_SEARCHING = 5,
    EMPTY_LIST_NO_SPEAKERS = 6,
    EMPTY_LIST_NONE_NAMED = 7,
};

/* gNameTagActionTexts: the actions the members' name tags offer */
extern char *gNameTagActionTexts[4];
enum NameTagAction {
    NAME_TAG_ACTION_STAND_UP = 0,
    NAME_TAG_ACTION_CANCEL_READY = 1,   /* cancel the departure's readiness */
    NAME_TAG_ACTION_SAVE = 2,           /* save the game */
};

/* gLobbyHelpTexts: the lobby dialogs' help lines, by help id, then the lists' column heads */
extern char *gLobbyHelpTexts[68];
enum LobbyHelp {
    LOBBY_HELP_COLUMN_HEADS = 48,       /* the lists' column heads, from 48 */
    LOBBY_HELP_NAME_LABEL = 67,
};

/* gLobbyLabelTexts: the lobby dialogs' labels, titles and choices */
extern char *gLobbyLabelTexts[58];
enum LobbyLabel {
    LOBBY_LABEL_ID = 0,                 /* the user id's label */
    LOBBY_LABEL_NOTHING_OFFERED = 46,   /* nothing is on offer now */
    LOBBY_LABEL_TOWN_UNNAMED = 49,      /* a town's default name */
    LOBBY_LABEL_YES = 57,
};

/* gLobbyNoSaveTexts: the lobby dialogs' no and save choices */
extern char *gLobbyNoSaveTexts[2];
enum LobbyNoSave {
    LOBBY_NO_SAVE_NO = 0,
    LOBBY_NO_SAVE_SAVE = 1,
};

/* gLobbyButtonTexts: the lobby dialogs' quit choice, states and button guide labels */
extern char *gLobbyButtonTexts[40];
enum LobbyButton {
    LOBBY_BUTTON_QUIT_NO_SAVE = 0,
    LOBBY_BUTTON_GUIDE = 12,            /* the button guide's labels, from 12 */
    LOBBY_BUTTON_BACK = 17,
};

#endif
