#ifndef HUD_H
#define HUD_H

#include "common.h"
#include "grid.h"
#include "combine.h"
#include "sprite.h"

/*
 * The HUD and in-game menu state (gHudState, pointing to gHudStateBuf; Hud_Init fills it
 * for a quest): the local player, the health bar, the pouch, supply box, combine and paged
 * list menus (hud_init.c, hud_query.c, hud.c). Several menus use the same bytes.
 */

struct Player;

/* A recipe the combine list found for an item (Combine_ListRecipesFor): the
 * recipe and its place in the indexes (Combine_FindRecipeIndex). */
typedef struct RecipeHit {
    Recipe *recipe;     /* 0x00 */
    s16 info[4];        /* 0x04 */
} RecipeHit;

/* Map_GetHalf: on stages 13 to 15 the map texture holds two maps side by
 * side, and an area's map is one half of it (the drawing in hud.c
 * takes u 0..0x78 for the left half, 0x78..0xF0 for the right). */
#define MAP_HALF_WHOLE  0   /* a stage with one map */
#define MAP_HALF_LEFT   1   /* AREA_TOWER_0 to AREA_TOWER_2 */
#define MAP_HALF_RIGHT  2   /* the stage's other areas */

/* gMapIconKinds[monster kind]: how the map draws a monster
 * (Map_DrawMonsterIcon). Below 0x80, the icon's index into
 * gMapMonsterIconU (8 more while the monster's stance is not 2). */
#define MAP_ICON_NONE       0xFF    /* not drawn */
#define MAP_ICON_DOT        0x80    /* a dot, not an icon */
#define MAP_ICON_DOT_LARGE  0x40    /* with MAP_ICON_DOT: a larger dot coloured by Map_MonsterColor */
#define MAP_ICON_DOT_COLOR  0x0F    /* a small dot's colour, index into gMapMonsterIconColors */

/* Player_DetectLevel's bits: which monsters the map shows (Map_MonsterVisibility). */
#define DETECT_TIMER        1   /* a detect effect runs (Player.detectTimer or actDetectTimer) */
#define DETECT_SKILL        2   /* the detect skill (skill 0x1B), its lower level */
#define DETECT_SKILL_HIGH   4   /* the detect skill at its higher level (0xA2) */

/* The field menu's entries (FieldMenu_Open): offline one more when the
 * quest has advice (Quest_HasAdvice). By fieldMenuCursor (the tables
 * gFieldMenuDecide, gFieldMenuInput, gFieldMenuDraw and their Online
 * twins): 0 the pouch, 1 combining, 2 the status page, 3 the quest, 4 the
 * info page, 5 the phrase menu (online the chat menu), 6 the options, and
 * offline 7 the pause menu and 8 the quest's advice (MsgEvent_OpenList). */
#define FIELD_MENU_ENTRIES          8
#define FIELD_MENU_ENTRIES_ONLINE   7


/* The status page's text colours (0x80BBGGRR, the font's half-scale colour). */
#define STATUS_COLOR_LABEL  0x807F7F28  /* the labels */
#define STATUS_COLOR_VALUE  0x807F7F7F  /* the values, and a stat the same as the other's */
#define STATUS_COLOR_BETTER 0x802B7F7F  /* a stat higher than the compared one's */
#define STATUS_COLOR_WORSE  0x807F212E  /* a stat lower than the compared one's */
/* The status page's entries (StatusMenu_Update, by listCursor): offline
 * the first two, online all five (three in the lobby while not online). */
#define STATUS_PERSONAL     0   /* the hunter's status (PersonalStatus_*) */
#define STATUS_EQUIPMENT    1   /* the equipment (EquipStatus_*) */
#define STATUS_COMMENT      2   /* the comment line (CommentEdit_*) */
#define STATUS_ONLINE_MARK  3   /* the online mark window (OnlineMark_*) */
#define STATUS_FRIEND_LIMIT 4   /* the lobby's setting (FriendLimit_*) */

/* The info page's entries (InfoMenu_Update, infoEntry): a set lists
 * gInfoMenuCounts[infoSet] of them, from the INFO_SET_ENTRIES bytes of
 * gInfoMenuEntries at infoSet (InfoMenu_Reset). */
#define INFO_SET_ENTRIES    4
#define INFO_LOCATION       0   /* where the hunter is (online only) */
#define INFO_COMBINE_LIST   1   /* the combine list */
#define INFO_MONSTER_NOTES  2   /* the monster notes */
#define INFO_QUEST          3   /* the quest's pages */
#define INFO_LOBBY_MEMBERS  4   /* the lobby's member list */

/* The help window's pages (gHelpPage): HelpWindow_DrawCurrent shows line
 * gHelpLine of the page while gHelpShow is set. */
#define HELP_PAGE_FIELD_MENU    0       /* the field menu's entries */
#define HELP_PAGE_ITEMS         1       /* the item menus: the drop dialog's lines, then the items' */
#define HELP_PAGE_PHRASES       3       /* the phrase menu */
#define HELP_PAGE_LOBBY         4       /* the lobby's menus */
#define HELP_PAGE_RESULTS_MENU  6       /* the quest results' item box and arena record menus (plsel) */
#define HELP_PAGE_STATUS        9       /* the status page */
#define HELP_PAGE_INFO          0xA     /* the info page */
#define HELP_PAGE_CHAT          0xB     /* the chat menu */
#define HELP_PAGE_OPTIONS       0xC     /* the options */
/* An item's description on HELP_PAGE_ITEMS. */
#define HELP_ITEM_LINE(item)    ((item) + 0x18)
#define HELP_LINE_NONE          0xFFFF  /* gHelpLine: no line */

/* HudState.notePage: the monster notes show the records (MonsterNote_Open
 * starts there); clear, the monster's materials and the help line. */
#define MONSTER_NOTE_RECORD     0x80

/* Note_DrawCrown's flags (the size records' crowns). */
#define CROWN_GOLD      1   /* the gold crown (else the silver) */
#define CROWN_SMALL     2   /* the small crown's mark inside it */

/* A texture corner as one word, u in the low half and v in the high, as
 * the game stores a sprite's corner (SpriteWords, Sprite's u0/v0 pair). */
#define TEX_UV(u, v)    (((v) << 16) | (u))

/* A Sprite (sprite.h) with each texture corner as one word (TEX_UV): the
 * HUD's sprites are filled this way, a corner with one store, and
 * EntryPicture_GetUV writes the words. Sprite_Draw takes it as a Sprite. */
typedef struct SpriteWords {
    s16 x, y, w, h;
    u32 color;                  /* 0x08 */
    s32 uv0;                    /* 0x0C: TEX_UV(u0, v0) */
    s32 uv1;                    /* 0x10: TEX_UV(u1, v1) */
} SpriteWords;

/* The chat menu's entries (online; ChatMenu_Update). */
#define CHAT_MENU_LOG       0   /* the chat log */
#define CHAT_MENU_PHRASES   1   /* the phrase menu */
#define CHAT_MENU_ENTRIES   2

/* The options (HudState.options, gOptionMenuRows' entries): 2, 4 and 6
 * take three values, the others two. */
#define OPTION_COCKPIT_OFF      0   /* hide the cockpit (the HUD) */
#define OPTION_MAP_OFF          1   /* hide the map (not while the cockpit is hidden) */
#define OPTION_AIM_REVERSE      2   /* gSysSave.options.aimReverse */
#define OPTION_CAM_FOLLOW       3   /* gSysSave.options.camFollow */
#define OPTION_CAM_CONTROL      4   /* gSysSave.options.camControl */
#define OPTION_VIBRATION        5   /* gSysSave.options.vibration */
#define OPTION_CHAT_SIZE        6   /* gSysSave.options.chatSize */
#define OPTION_CHAT_HIDE_NAMES  7   /* gChatHideNames */
#define OPTION_CHAT_BEEP        8   /* gSysSave.options.chatBeep */
#define OPTION_COUNT            9

/* Frames an item get holds the HUD's item-get lock (ItemGet_Reserve). */
#define ITEM_GET_WAIT   4

/* The quest pages' kinds (gQuestMenuKinds, QuestMenu_DrawPage). */
#define QUEST_PAGE_TARGETS  0   /* the goals, the faints and the time left */
#define QUEST_PAGE_SUMMARY  1   /* the client, stars and terms */
#define QUEST_PAGE_TERMS    2   /* the time limit, fee and rewards (a drill: its par time) */
#define QUEST_PAGE_PLACE    3   /* the place (a drill: its best record) */

/* Shift-JIS characters the quest pages build their numbers from, as the
 * u16 a little-endian load of the two bytes gives. */
#define SJIS_WIDE_DIGIT(c)  ((((c) + 0x1F) << 8) | 0x82)   /* an ASCII digit's full-width form */
#define SJIS_WIDE_COLON     0x4681
#define SJIS_WIDE_SLASH     0x5E81
#define SJIS_WIDE_SPACE     0x4081
#define SJIS_STAR           0x9A81
#define SJIS_MONEY_UNIT     0x9A82  /* the full-width letter of the money unit */

/* The menus' cursor layouts (menu_util.c). */
#define POUCH_MENU_ROWS     10  /* the pouch's 20 slots: two columns of 10 */
#define PHRASE_MENU_ROWS    6   /* the phrase menu: four columns of 6 */
#define PHRASE_MENU_COLUMNS 4

/* Shake_Start's flags: the axis in the low four bits, and the slide bit. */
#define SHAKE_AXIS_MASK 0x0F
#define SHAKE_AXIS_X    0
#define SHAKE_AXIS_Y    1
#define SHAKE_SLIDE     0x80    /* slide from 0 out to the amplitude and stay, not ease back */
#define SHAKE_FRAMES    6

/* Shake_Start's id: the menu window that slides in (flags SHAKE_AXIS_X)
 * or out (SHAKE_SLIDE) as it opens and closes; its draw takes the offset
 * from Shake_GetOffset(id), and Shake_Update(id) is 1 while it moves. The
 * game overlay uses 7, the lobby 0x10 (hud.c's menus). */
#define SHAKE_FIELD_MENU    0       /* the field menu's list */
#define SHAKE_POUCH_MENU    1       /* the pouch, combine and drop windows */
#define SHAKE_QUEST_MENU    5       /* the quest info pages */
#define SHAKE_RESULTS_MENU  6       /* the quest results' item box and arena record menus (plsel) */
#define SHAKE_COMBINE_LIST  8       /* the combine list */
#define SHAKE_OPTION_MENU   9       /* the options */
#define SHAKE_MONSTER_LIST  0xD     /* the monster notes */
#define SHAKE_PHRASE_MENU   0xE     /* the chat phrase menu */
#define SHAKE_CHAT_HUNTERS  0xF     /* the chat's hunter page (the lobby) */
#define SHAKE_CHAT_STATE    0x10    /* the chat state page (the lobby) */

/* The menus' D-pad key repeat (Menu_ReadDpad), in frames. */
#define DPAD_REPEAT_DELAY   8   /* from a press to the first repeat */
#define DPAD_REPEAT_RATE    3   /* between repeats */

/* ItemList_Draw's and ItemList_DrawPouch's flags (the item lists of the
 * pouch, combine and drop windows). */
#define ITEM_LIST_TITLE2    0x01    /* the second title, gItemListTitles[1] */
#define ITEM_LIST_SAVED     0x02    /* ItemList_DrawPouch: the saved pouch (gSave.pouch), not the player's */
#define ITEM_LIST_COMBINE   0x04    /* colour the items by the combine menu's picks and recipes */
#define ITEM_LIST_FRAME     0x08    /* draw the list's frame (Menu_DrawFrame) */
#define ITEM_LIST_SORTED    0x10    /* ItemList_DrawPouch: pack and sort the stacks first */
#define ITEM_LIST_SORT_HINT 0x20    /* the text box and the sort button's hint (dropped while the window slides) */
#define ITEM_LIST_MOVING    0x40    /* dim the cursor's slot (HudState.slot), the stack being moved */
#define ITEM_LIST_SLIDE     0x80    /* follow the pouch window's slide (SHAKE_POUCH_MENU) */

/* HudState.pouchBarState (Hud_DrawPouchItem): the pouch bar opens while
 * the player holds the item select (Player.itemSelect), with
 * pouchBarTimer sliding from 6 down to 0 by 3 a frame; it slides back
 * from 3 up to 9 as it closes. Closed, the HUD shows the selector box. */
#define POUCH_BAR_CLOSED    0
#define POUCH_BAR_OPENING   1
#define POUCH_BAR_OPEN      2
#define POUCH_BAR_CLOSING   3

/* HudState.dropState: the pouch-full dialog (PouchDrop_Update), when an
 * item is got with no room for it (HudState.dropItem). */
#define DROP_NONE           0
#define DROP_NOTICE         1   /* post the item's notice and wait for it */
#define DROP_WAIT_BANNER    2   /* the pouch-full text, until the banner is gone; then the window slides in */
#define DROP_CHOOSE         3   /* swap it for a pouch item, or leave it (dropChoice 0 or 1) */
#define DROP_PICK_SLOT      4   /* pick the pouch slot to give up */
#define DROP_CONFIRM        5   /* yes or no; yes swaps the item in */
#define DROP_CLOSING        6   /* wait for the window to slide out */

/* Hud_DrawBlinkIcon's icons (gBlinkIconDefs), the gunners' prompts. */
#define BLINK_ICON_RELOAD       0   /* the clip is empty */
#define BLINK_ICON_RELOADING    1   /* a reload runs (the gunlance's reload states, the bowgun's reload motion) */
#define BLINK_ICON_NEXT_AMMO    2   /* the loaded ammo is used up and the pouch has another */

/* HudState.aimFade while the player is not aiming (Player_IsAiming): it
 * counts down to 0 as the aim view comes in (Hud_DrawAimView). */
#define AIM_FADE_IDLE 4

/* A point on a stage's map (gMapPoints, read by Map_GetPoint): the map
 * coordinates of an area's mark, z its dot's radius (0: none, the area's
 * record gives the place). */
typedef struct MapPoint {
    s16 x, y, z;
} MapPoint;

/* A health or stamina gauge of the cockpit (Hud_DrawGaugeFrame,
 * Hud_DrawGaugeFill): its place, its full length and its value. */
typedef struct Gauge {
    f32 x;              /* 0x00 */
    f32 len;            /* 0x04: the full length */
    s16 y;              /* 0x08 */
    s16 cur;            /* 0x0A */
    s16 max;            /* 0x0C */
    u8 unkE[2];
    u32 color;          /* 0x10 */
} Gauge;

/* The menus' fades (Hud_FadeStart, Fade_UpdateAll): each of the six
 * gHudFades plays a FadeDef, a texture rectangle moved and faded by two
 * key lists (a key time of 0 ends a list). */
typedef struct AlphaKey {
    s32 time;           /* frames from the fade's start */
    f32 value;          /* 0 to 1 */
} AlphaKey;

typedef struct PosKey {
    s32 time;           /* frames from the fade's start */
    f32 x;              /* scales of the rectangle's corners */
    f32 y;
} PosKey;

typedef struct FadeDef {
    s16 angle;          /* 0x00 */
    u16 tex;            /* 0x02 */
    u32 color;          /* 0x04 */
    s16 u;              /* 0x08 */
    s16 v;              /* 0x0A */
    s16 x1;             /* 0x0C */
    s16 y1;             /* 0x0E */
    s16 x0;             /* 0x10 */
    s16 y0;             /* 0x12 */
    PosKey *pos;        /* 0x14 */
    AlphaKey *alpha;    /* 0x18 */
    u8 unk1C[4];
} FadeDef;

typedef struct Fade {
    u8 active;          /* 0x00 */
    u8 visible;         /* 0x01: it has started */
    u8 alpha;           /* 0x02 */
    u8 start;           /* 0x03: the frame it starts at (HudState.fadeFrame) */
    f32 x0;             /* 0x04 */
    f32 x1;             /* 0x08 */
    f32 y0;             /* 0x0C */
    f32 y1;             /* 0x10 */
    f32 bx;             /* 0x14: the base position */
    s16 by;             /* 0x18 */
    u8 unk1A[2];
    FadeDef *def;       /* 0x1C */
} Fade;

/* A gunner's prompt (gBlinkIconDefs, Hud_DrawBlinkIcon): its size, its x
 * in the weapon box and its texture corners (TEX_UV words). */
typedef struct IconDef {
    s16 unk0;
    s16 w;              /* 0x2 */
    s16 h;              /* 0x4 */
    s16 x;              /* 0x6 */
    s32 uv0;            /* 0x8 */
    s32 uv1;            /* 0xC */
} IconDef;

typedef struct HudState {
    s8 started;                 /* 0x000: set to 1 by the first update (ItemHud_Update, Hud_Update); nothing reads it */
    u8 unk001[0x004 - 0x001];
    union {
        struct {
            u8 phase;           /* 0x004 */
            u8 mode;            /* 0x005: 1: the pouch-drop state (FieldMenu_Update) */
            u8 supply;          /* 0x006: the supply box is open (Player.supplyBoxOpen) */
            u8 dropState;       /* 0x007: the pouch-full drop dialog state (PouchDrop_Update), 0: none */
        };
        s32 states;             /* 0x004: the four above, cleared together (Hud_Init) */
    };
    struct Player *player;      /* 0x008: the local player (Hud_Init) */
    s16 dpadRepeatDir;          /* 0x00C: the D-pad direction being repeated */
    s8 dpadRepeatTimer;         /* 0x00E: frames to the next repeat (DPAD_REPEAT_DELAY, then DPAD_REPEAT_RATE) */
    u8 yesNo;                   /* 0x00F: the yes/no choice (0 yes) */
    s16 statusTimer[4];         /* 0x010: per player: frames the status icon stays (20), -1 when no status shows (Hud_DrawStatusIcon) */
    u8 unk018[4];               /* 0x018 */
    u8 statusIcon[4];           /* 0x01C: per player: the status icon shown (0..14, cycles through the set bits of the mask) */
    u8 unk020[0x022 - 0x020];
    u8 deliveryMenu;            /* 0x022: Player.deliveryMenu, copied by ItemHud_Update */
    u8 deliveryOpen;            /* 0x023: Player.deliveryOpen, copied each frame (ItemHud_Update) */
    s32 timeLeft;               /* 0x024: Quest_GetTime(0) (Hud_DrawClock) */
    s32 timeTotal;              /* 0x028: Quest_GetTime(1) (Hud_DrawClock) */
    s16 hp;                     /* 0x02C: the health shown, moves one a frame toward Player.hp */
    s16 hpRecoverable;          /* 0x02E */
    s8 aimFade;                 /* 0x030: AIM_FADE_IDLE while not aiming; counts down to 0 while aiming */
    u8 aimBlink;                /* 0x031: blink phase at aimFade 0 (& 0x1F) */
    s8 aimKind;                 /* 0x032: the aim camera's kind (CamAim_GetState), -1: not aiming */
    s8 aimStart;                /* 0x033: 1 on the frame aiming starts (aimFade still AIM_FADE_IDLE) */
    f32 mapScaleX;              /* 0x034: world x to map units: 1 / gMapScales[stage][0] (Hud_Init) */
    f32 mapScaleZ;              /* 0x038: world z to map units: 1 / gMapScales[stage][1] */
    s16 pingTimer[4];           /* 0x03C: per player: frames since the map ping, -1: none */
    s16 pingHold;               /* 0x044: frames the ping button must be held (10) */
    u8 mapZoomed;               /* 0x046: 1: the zoomed map (Map_DrawZoomed) */
    u8 glowTimer;               /* 0x047: frames of the glow effect (0x1A) */
    u8 menuPhase;               /* 0x048: the field menu: 0 the list, 1 a page (FieldMenu_Update) */
    u8 fieldMenuCount;          /* 0x049: the field menu entries (7 or 8) */
    u8 fieldMenuCursor;         /* 0x04A: the field menu entry (FieldMenu_Update tables) */
    u8 listCursor;              /* 0x04B: the cursor of the status, info and chat menus */
    u8 infoSet;                 /* 0x04C: the info menu set (0-3: table gInfoMenuCounts/gInfoMenuEntries) */
    u8 menuStep;                /* 0x04D: step of the status, info and chat menus */
    u8 pageCursor;              /* 0x04E: the page (left/right) of the quest, status and equipment pages */
    u8 infoEntry;               /* 0x04F: the chosen info entry (from gInfoMenuEntries) */
    u8 state;                   /* 0x050: pouch menu: 0 pick a slot, 1 pick an action, 2 do it, 3 offered, 4 closing */
    u8 slot;                    /* 0x051: the selected pouch slot */
    u8 action;                  /* 0x052: 0 drop, 1 give, 2 move */
    u8 step;                    /* 0x053: giving: 0 the count, 1 the player */
    s8 target;                  /* 0x054: the player given to */
    u8 count;                   /* 0x055: the count given (Menu_AdjustCount) */
    u8 atMax;                   /* 0x056 */
    u8 dropChoice;              /* 0x057: the drop dialog yes/no (0 yes) */
    s16 dropHelpLine;           /* 0x058: the help line of the drop dialog: 0x14/0x15/0x16 or item + 0x18 */
    u16 dropItem;               /* 0x05A: the item that did not fit, 0: none */
    u8 dropNotice;              /* 0x05C: the notice id of the item-get notice, 0xFF: not posted */
    u8 dropBlink;               /* 0x05D: blink counter of the drop dialog */
    s8 dropCount;               /* 0x05E: how many of dropItem */
    s8 pouchBarShown;           /* 0x05F: 1 when Hud_DrawPouchBar drew this frame */
    s8 blink60;                 /* 0x060 */
    s8 dir61;                   /* 0x061 */
    u16 pouchCursor;            /* 0x062 */
    s8 blink64;                 /* 0x064 */
    s8 dir65;                   /* 0x065 */
    u16 ammoSlot;               /* 0x066 */
    s16 supplyMsgTimer;         /* 0x068: frames of the supply box flash (0x1F) */
    u8 supplyFlashSlot;         /* 0x06A: the supply box cursor flashed (Player +0x81B) */
    u8 supplyMoveFrames;        /* 0x06B: an item taken: its frames */
    u8 supplyMoveTimer;         /* 0x06C: its frames left */
    u8 supplyMoveSlot;          /* 0x06D: its supply box slot */
    u16 supplyMoveItem;         /* 0x06E: the item */
    Recipe *combineRecipe;      /* 0x070: the combine menu recipe (CombineWork.recipe) */
    u16 combineItems[3];        /* 0x074: the combine menu's ingredients */
    u8 combineSlots[3];         /* 0x07A: the pouch slots of combineItems, 0xFF: none (CombineWork.slots) */
    u8 combineThree;            /* 0x07D: 1 for a three-ingredient combine (CombineWork.three) */
    s16 combineTimer;           /* 0x07E: frames of the combine menu's result wait */
    s16 combineResult;          /* 0x080: the recipe result item, -1: none */
    u8 combineMade;             /* 0x082: how many the combine made */
    u8 combineKnown;            /* 0x083: the recipe was known before (no reveal) */
    s16 combineTicket;          /* 0x084: the combine menu's item-get ticket (ItemGet_Reserve; CombineMenu_Open sets -1) */
    s16 combineMadeItem;        /* 0x086: the item the combine made (COMBINE_FAILED_ITEM on a failure) */
    union {
        struct {
            u8 combineState;    /* 0x088: the combine menu's state (CombineMenu_Input) */
            u8 combinePick;     /* 0x089: the ingredient being picked */
        };
        s16 combineSteps;       /* 0x088: the two above, cleared together (CombineMenu_Reset) */
    };
    u8 combineConfirm;          /* 0x08A: the combine confirm cursor (CombineWork.unk8A) */
    s8 combineError;            /* 0x08B: COMBINE_ERR_* of the last combine, 0: none */
    u8 fadeActive;              /* 0x08C: 1 while a menu fade runs */
    u8 fadeMode;                /* 0x08D: the Hud_FadeStart mode */
    s16 fadeFrame;              /* 0x08E: frames since Hud_FadeStart (from 1) */
    u8 equipCursor;             /* 0x090: the equipment status page */
    u8 equipCursorMax;          /* 0x091: its page count (Equip_LevelMark) */
    u8 hasQuestInfo;            /* 0x092: QuestMenu_CheckInfo: 0 online without a quest, else 1 */
    u8 questMenuInField;        /* 0x093: 1 outside the town (gScene.inLobby 0): picks the quest menu tables (gQuestMenuCursorMax/gQuestMenuKinds) */
    s8 unk94;                   /* 0x094: cleared by InfoMenu_Open; nothing reads it */
    s8 noteMonster;             /* 0x095: the monster shown in the monster notes, -1: none */
    s8 notePage;                /* 0x096: the monster notes' page: MONSTER_NOTE_RECORD set, the records; clear, the materials (0x200 switches) */
    u8 phraseStep;              /* 0x097: the phrase menu step */
    u8 hidden;                  /* 0x098 */
    u8 optionKind;              /* 0x099: the option menu set (0-4, by Game_IsOnline) */
    u8 optionCount;             /* 0x09A: the option menu entries (gOptionMenuCounts[optionKind]) */
    union {
        struct {
        u8 cockpitOff;              /* 0x09B: the cockpit (HUD) display option: 1 hidden */
        u8 mapOff;                  /* 0x09C: the map display option: 1 hidden (Hud_Init sets it in the online town) */
        s8 optAimReverse;           /* 0x09D: the option menu copy of gScene.aimReverse */
        s8 optCamFollow;            /* 0x09E: the option menu copy of gSysSave.options.camFollow */
        s8 optCamControl;           /* 0x09F: the option menu copy of gSysSave.options.camControl */
        s8 optVibration;            /* 0x0A0: the option menu copy of gSysSave.options.vibration */
        s8 optChatSize;             /* 0x0A1: the option menu copy of gSysSave.options.chatSize */
        u8 optChatHideNames;        /* 0x0A2: the option menu copy of gChatHideNames */
        s8 optChatBeep;             /* 0x0A3: the option menu copy of gSysSave.options.chatBeep */
        };
        u8 options[OPTION_COUNT];   /* 0x09B: the options above, by OPTION_* (OptionMenu_Update) */
    };
    u8 dropShowSlot;            /* 0x0A4: 1: the drop dialog shows the pouch slot item */
    u8 combineTaken;            /* 0x0A5: ingredients taken out of the pouch (CombineWork.unkA5) */
    u8 sorting;                 /* 0x0A6: the pouch menu's sort prompt is open */
    u8 dest;                    /* 0x0A7: moving: the slot moved to */
    u8 cancel;                  /* 0x0A8 */
    u8 camReqStarted;           /* 0x0A9: CamReq_Started() of the last update */
    u8 detectLevel;             /* 0x0AA: Player_DetectLevel of the local player, set as the map draws (DETECT_*) */
    u8 unk0AB[0x0AC - 0x0AB];
    u16 clockHand;              /* 0x0AC: the clock hand shown (16-bit turn), catches up 0x16C a frame */
    u16 dpadRepeat;             /* 0x0AE: the D-pad presses of this frame with key repeat (Menu_ReadDpad) */
    u16 dpadPrev;               /* 0x0B0: the D-pad held last frame (Menu_ReadDpad) */
    u8 mapHalf;                 /* 0x0B2: the map half shown, Player_GetMapHalf of the local player (MAP_HALF_*) */
    u8 pouchBarState;           /* 0x0B3: the pouch bar's slide state, 0 to 3 (Hud_DrawPouchSelector); cleared offline by Hud_Init and Hud_StartQuest */
    s8 pouchBarTimer;           /* 0x0B4: its slide position, 0 to 9 */
    u8 cursorMax;               /* 0x0B5: the entry or page count for listCursor / pageCursor */
    u8 questMenuStep;           /* 0x0B6: the quest menu step */
    u8 popupOpen;               /* 0x0B7: nonzero while a message popup takes the input (MsgEvent_PopupInput, ItemHud_Update): Hud_InputBlocked offline, Hud_DrawLayer3 skips */
    s32 gsSave6C;               /* 0x0B8: Gs state 0x6C saved by Hud_SaveGsState */
    s32 gsSave6D;               /* 0x0BC: Gs state 0x6D saved by Hud_SaveGsState */
    s16 hpBlink;                /* 0x0C0: phase of the health bar colour pulse (16-bit turn) */
    s16 staminaBlink;           /* 0x0C2: phase of the stamina bar colour pulse */
    Grid grid;                  /* 0x0C4: the combine list's cursor (CombineList_Setup; grid.unk0 is
                                   Combine_ListRecipesFor's flags, grid.row the item listItems[row]) */
    RecipeHit sets[3];          /* 0x0D0: the recipes found for the item (Combine_ListRecipesFor) */
    u16 listItems[5];           /* 0x0F4 */
    u8 set;                     /* 0x0FE */
    u8 setCount;                /* 0x0FF */
    u8 kbdOpen;                 /* 0x100: 1 while the chat input line keyboard is open (Chat_InputLine; the comment saved in gLoadBuf) */
    u8 hornLast;                /* 0x101: the hunting horn's note slot shown last (Hud_DrawHornNotes) */
    u8 hornAnim;                /* 0x102: 6 down to 1 while a new note slides in */
    u8 hornHist[5];             /* 0x103: the notes shown, newest first */
} HudState;

typedef char HudState_size_check[(sizeof(HudState) == 0x108) ? 1 : -1];

extern HudState gHudStateBuf;  /* the state (0x108 bytes, cleared by Hud_Reset) */
extern HudState *gHudState; /* points to gHudStateBuf */

#endif /* HUD_H */
