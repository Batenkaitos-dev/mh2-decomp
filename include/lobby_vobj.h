#ifndef LOBBY_VOBJ_H
#define LOBBY_VOBJ_H

#include "common.h"
#include "sprite.h"
#include "item.h"
#include "menu_task.h"
#include "stageobj.h"
#include "lobby_globals.h"
#include "lobby_menu.h"

/* A 4x4 matrix, as one object (the draws hand its address to the GS state). */
typedef struct Mtx44 {
    f32 m[4][4];
} Mtx44;

/*
 * The shooting gallery's target board: a lobby stage object (StageObj, kind
 * LOBBY_OBJ_GALLERY_BOARD, type GALLERY_BOARD_TARGETS) whose buf is this
 * record. It stands 15 targets on 15 spots, 3 rows of 5 from
 * gGalleryRowBases, flips them up, holds them standing for the course's
 * frames and flips them down; the gallery's round refills it once it is
 * cleared (Gallery_PlaceTargets). A shot counts hits off a target
 * (GalleryBoard_HitTarget) until it falls.
 *
 * Types 1 to 9 of the same kind are the bursts a hit leaves
 * (GalleryBurst_SpawnHit, GalleryBurst_SpawnDowned): they use only
 * facing[15], turnStep and life of this record.
 */
#define GALLERY_TARGETS         15  /* the board's targets and spots */
#define GALLERY_ROW_TARGETS     5   /* the targets of a row */
#define GALLERY_FLIP_FRAMES     12  /* the frames of a flip up or down (turnStep a frame) */
#define GALLERY_TILT_DOWN       0xA000  /* a target's tilt while flipped down, before it flips up */
#define GALLERY_FLIP_STEP       0x800   /* a target's tilt a frame of a flip (ANGLE_TURN to a turn) */
#define GALLERY_FALL_STEP       0xCCD   /* a downed target's tilt a frame as it falls */
#define GALLERY_FALL_FRAMES     6   /* a downed target falls this long, then leaves its spot */
#define GALLERY_CLEAR_WAIT      10  /* frames between the flip down and the board's refill */
#define GALLERY_HIT_BURSTS      5   /* the bursts of a hit that leaves its target standing (gGalleryHitBursts) */
#define GALLERY_SWING_FRAMES    25  /* a target's swing after a hit that leaves it standing */

/* StageObj.type of the gallery's shots (GalleryShot). */
enum GalleryShotType {
    GALLERY_SHOT_BULLET = 0,    /* the bullet */
    GALLERY_SHOT_FLASH = 1,     /* the flash where it hits a wall */
    GALLERY_SHOT_PUFF1 = 2,     /* the three puffs there, one after another */
    GALLERY_SHOT_PUFF2 = 3,
    GALLERY_SHOT_PUFF3 = 4,
    GALLERY_SHOT_TYPES = 5
};
#define GALLERY_SHOT_FRAMES     60  /* a bullet that hits nothing ends after this */

/* The gallery gun's muzzle flash (kind LOBBY_OBJ_GALLERY_MUZZLE): one object
 * of each type, each with its own model, keys and life. */
enum GalleryMuzzleType {
    GALLERY_MUZZLE_BLAST = 0,   /* common model set 18, 8 frames, not rolled */
    GALLERY_MUZZLE_FLASH = 1,   /* the shot's flash model (set 1), 4 frames */
    GALLERY_MUZZLE_PUFF = 2,    /* the shot's puff model (set 6), 22 frames */
    GALLERY_MUZZLE_TYPES = 3
};
typedef struct GalleryMuzzle {
    /* 0x0 */ s32 roll;             /* its roll (ANGLE_TURN to a turn) */
    /* 0x4 */ u16 life;             /* the frames it shows */
} GalleryMuzzle;

/* StageObj.type of the board's kind: the board, then the bursts, named by
 * how they move (GalleryBoard_Run). A downed target leaves bursts 1 to 6
 * (GalleryBurst_SpawnDowned), any other hit those of gGalleryHitBursts. */
enum GalleryBoardType {
    GALLERY_BOARD_TARGETS = 0,
    GALLERY_BURST_DRIFT1 = 1,   /* drift along the camera's direction for their life */
    GALLERY_BURST_DRIFT2 = 2,
    GALLERY_BURST_DRIFT3 = 3,
    GALLERY_BURST_STILL1 = 4,   /* show in place for 6 frames, one after another */
    GALLERY_BURST_STILL2 = 5,
    GALLERY_BURST_STILL3 = 6,
    GALLERY_BURST_SPIN1 = 7,    /* roll in place for 8 frames, one after another */
    GALLERY_BURST_SPIN2 = 8,
    GALLERY_BURST_SPIN3 = 9
};

/* The board's phase (StageObj.var0; GalleryBoard_Run). */
enum GalleryBoardPhase {
    GALLERY_BOARD_FLIP = 0,     /* the targets flip up (or down, with flipDown) */
    GALLERY_BOARD_STAND = 1,    /* they stand for hold frames */
    GALLERY_BOARD_CLEARING = 2  /* flipped down: waits clearWait frames, then cleared */
};

typedef struct GalleryBoard {
    /* 0x000 */ Vec3f spots[GALLERY_TARGETS];  /* the spots (GalleryBoard_Init: five a row from gGalleryRowBases) */
    /* 0x0B4 */ s8 hitsLeft[16];    /* per target: the hits it still takes (its kind's hits; GalleryBoard_HitTarget) */
    /* 0x0C4 */ u16 tilt[GALLERY_TARGETS];  /* per target: its tilt about x (ANGLE_TURN to a turn) */
    /* 0x0E2 */ u16 facing[16];     /* per target: its yaw, away from the board's direction; [15] a burst's roll */
    /* 0x102 */ u8 downed[GALLERY_TARGETS];   /* per target: 1 once its last hit is counted (it falls) */
    /* 0x111 */ s8 swing[GALLERY_TARGETS];    /* per target: frames left of the swing a hit gives (GalleryBoard_SwingTarget) */
    /* 0x120 */ s16 fallFrames[16]; /* per target: frames since it was downed */
    /* 0x140 */ Vec3f dir;          /* the board's direction (GalleryBoard_Spawn); the targets face away from it */
    /* 0x14C */ s32 turnStep;       /* the board: the targets' tilt a frame of a flip; bursts 7 to 9: their roll a frame */
    /* 0x150 */ u32 life;           /* bursts 1 to 3: the frames they show */
    /* 0x154 */ s8 flipTimer;       /* frames left of a flip (GALLERY_FLIP_FRAMES) */
    /* 0x155 */ u8 flipDown;        /* 0: the targets flip up, 1: they flip down */
    /* 0x156 */ s8 holdLeft;        /* frames left of the targets standing (from hold) */
    /* 0x157 */ s8 hold;            /* the frames the targets stand (the course's) */
    /* 0x158 */ u8 cleared;         /* 1 once flipped down and waited: the round refills it */
    /* 0x159 */ u8 pad159[3];
    /* 0x15C */ u8 *kinds;          /* per target: its kind (gGalleryTargetKinds; 0 none): the round's PrizeMenu.slots */
    /* 0x160 */ u8 *spotOrder;      /* per target: the spot it stands on (the round's PrizeMenu.order) */
    /* 0x164 */ s16 slot[16];       /* per target: its draw slot */
    /* 0x184 */ DrawSlot *parts[GALLERY_TARGETS];  /* per target: DrawSlot_Get(slot) (NULL: none) */
    /* 0x1C0 */ s8 clearWait;       /* frames left before cleared (GALLERY_CLEAR_WAIT) */
    /* 0x1C1 */ u8 pad1C1[3];
} GalleryBoard;
typedef char GalleryBoard_size_check[(sizeof(GalleryBoard) == 0x1C4) ? 1 : -1];

/* The lobby's stage object kinds (StageObj.kind) and the states their
 * updates step through (StageObj.state; ModelObj_Update, GalleryBoard_Update). */
#define LOBBY_OBJ_MODEL     0x19    /* a model object (ModelObj_Update) */
#define LOBBY_OBJ_GALLERY_BOARD 0x20    /* the shooting gallery's target board and its bursts (GalleryBoard_Update) */
#define LOBBY_OBJ_GALLERY_SHOT  0x21    /* the gallery's bullet and its wall hits (GalleryShot_Update) */
#define LOBBY_OBJ_GALLERY_SCORE 0x22    /* the gallery's results score (GalleryScore_Update) */
#define LOBBY_OBJ_ACTOR_PARTICLES 0x23  /* particles thrown from an actor (ActorParticles_Update) */
#define LOBBY_OBJ_GALLERY_POINTS 0x24   /* the points rising from a downed target (GalleryPoints_Update) */
#define LOBBY_OBJ_GALLERY_MUZZLE 0x26   /* the gallery gun's muzzle flash (GalleryMuzzle_Update) */
#define LOBBY_OBJ_DRINK_MUG 0x28        /* a drinking townsperson's mug (DrinkMug_Update) */
#define LOBBY_OBJ_BIRD_FLOCK 0x2A       /* a flock of birds in a hub (BirdFlock_Update) */
enum LobbyObjState {
    LOBBY_OBJ_INIT = 0,     /* set up, then RUN */
    LOBBY_OBJ_RUN = 1,      /* every frame until it ends */
    LOBBY_OBJ_END = 2,      /* frees its draw slots */
    LOBBY_OBJ_FREE = 3      /* frees the object */
};

/* A model object's type (StageObj.type; the gModelObj* tables' index), by
 * the spawns seen. Types 3 to 5 have table entries but no spawner. */
enum ModelObjType {
    MODEL_OBJ_NPC_BURST = 0,    /* the pet's PET_STATE_JOY (Pet_StateJoy), once: 7 parts */
    MODEL_OBJ_NPC_PUFF = 1,     /* the pet's PET_STATE_ANGRY (Pet_StateAngry), every 3 to 7 frames; the parts drift */
    MODEL_OBJ_SPARKLE = 2,      /* a stationed townsperson's STATIONED_MOT_BUSY_IDLE_FIDGET, every 10 frames from frame 38 to 198, and fixed spots in AREA_HUB_0x85 */
    MODEL_OBJ_PLAYER_FX_A = 6,  /* the player's motion 250 at its frame 250, with PLAYER_FX_B */
    MODEL_OBJ_PLAYER_FX_B = 7,  /* eight parts appearing one after another */
    MODEL_OBJ_PET_JOY = 8       /* shown on the player while a petting pleases (Poogie_UpdatePetting); lives Poogie_JoyFrames frames */
};

/* A part of a model object (52 bytes, in the object's buffer), drawn by
 * ModelObj_DrawPart through its draw slot. */
typedef struct ModelObjPart {
    /* 0x00 */ s16 slot;            /* its draw slot (DrawSlot_Alloc; -1: none) */
    /* 0x02 */ s16 index;           /* its index in the object (ModelObj_DrawPart switches on it) */
    /* 0x04 */ Vec3f size;          /* from its first key curve */
    /* 0x10 */ Vec3f pos;
    /* 0x1C */ f32 alpha;           /* from a key curve, in the types that have one */
    /* 0x20 */ u8 unk20[4];
    /* 0x24 */ u16 angle;           /* its roll (ANGLE_TURN to a turn) */
    /* 0x26 */ s16 spin;            /* added to angle each frame */
    /* 0x28 */ f32 scale;
    /* 0x2C */ struct DrawSlot *draw;   /* DrawSlot_Get(slot); NULL: not drawn */
    /* 0x30 */ s16 frame;           /* counted up each frame from 0 or below: the part waits while it is 0 or less */
    /* 0x32 */ u16 driftSeed;       /* random: MODEL_OBJ_NPC_PUFF and MODEL_OBJ_SPARKLE drift sideways by it */
} ModelObjPart;
typedef char ModelObjPart_size_check[(sizeof(ModelObjPart) == 0x34) ? 1 : -1];

/* Per type, a table of each part's frames (NULL: gModelObjPartFrames),
 * then MODEL_OBJ_NPC_BURST's texture per part, which ModelObj_DrawPart
 * reads. One object: the second table starts within the first's last
 * 16 bytes. */
typedef struct ModelObjTables {
    u16 *partFrames[10];
    s16 burstTexIds[12];
} ModelObjTables;

/* A kind of the gallery's targets (gGalleryTargetKinds, 8 bytes each). */
typedef struct GalleryTargetKind {
    /* 0x0 */ u16 picture;          /* its picture (EntryPicture_GetUVf's id); the gallery picks the hit's cry by it */
    /* 0x2 */ s16 points;           /* the points it gives when downed */
    /* 0x4 */ u8 unk4;              /* 0 to 3, rising with the points; no reader seen */
    /* 0x5 */ u8 size;              /* its size class (gGalleryTargetSizes: the picture's side) */
    /* 0x6 */ u8 hits;              /* the hits it takes to go down */
    /* 0x7 */ u8 endsRound;         /* 1: downing it flips the whole board down */
} GalleryTargetKind;
typedef char GalleryTargetKind_size_check[(sizeof(GalleryTargetKind) == 0x8) ? 1 : -1];

extern GalleryTargetKind gGalleryTargetKinds[];

/*
 * The village mine's menu (TOWN_MODE_MINE, the view of gMenuWork that
 * Mine_OpenMenu and Mine_UpdateMenu use): the player gives the miner one
 * of the pickaxes held, he digs during the next quests (SaveData.mine),
 * and his find is handed over when he is next spoken to.
 */
#define MINE_PICKAXE_KINDS      3   /* the pickaxes the miner takes (ITEM_ID_MINE_TOOL_WEAK, _MID, _STRONG) */

typedef struct MineMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ u16 items[MINE_PICKAXE_KINDS + 1]; /* the pickaxes held (Mine_BuildPickaxeList), then 0: the list draws one empty row after them */
    /* 0x10 */ struct Em *miner;    /* the townsperson spoken to */
    /* 0x14 */ s16 cursor;
    /* 0x16 */ s16 count;
    /* 0x18 */ u8 toggle;           /* PAD_SQUARE toggles it: the pickaxe's description */
    /* 0x19 */ u8 state;            /* enum MineMenuState */
} MineMenu;

/* Mine_UpdateMenu's states (MineMenu.state). */
enum MineMenuState {
    MINE_MENU_TALK = 0,         /* the miner's opening words, then the list */
    MINE_MENU_SLIDE_IN = 1,     /* the pickaxe list's window slides in */
    MINE_MENU_CHOOSE = 2,       /* the pickaxe list */
    MINE_MENU_DIGGING = 3,      /* a pickaxe was given: his words, then close */
    MINE_MENU_REFUSED = 4,      /* nothing given: his words, then close */
    MINE_MENU_DONE_TALK = 5,    /* he has dug (MINE_DUG): his report */
    MINE_MENU_HAND_OVER = 6     /* his find goes to the pouch, if there is room */
};

/*
 * The ship menu (TOWN_MODE_SHIP_MENU, its view of gMenuWork): a
 * townsperson's two choices, to board (a camera tour, then the player is
 * taken to AREA_HUB_0x57) or to see the offers (OfferMenu_Open), the
 * second listed only when OfferMenu_BuildList finds some.
 */
typedef struct ShipMenu {
    /* 0x0 */ u8 *npc;          /* the townsperson spoken to (its Em) */
    /* 0x4 */ s16 cursor;       /* SHIP_CHOICE_* */
    /* 0x6 */ s16 count;        /* the choices listed: 1, or 2 with the offers */
    /* 0x8 */ s8 state;         /* enum ShipMenuState */
} ShipMenu;

#define SHIP_CHOICE_BOARD   0
#define SHIP_CHOICE_OFFERS  1

/* ShipMenu_Update's states (ShipMenu.state). */
enum ShipMenuState {
    SHIP_MENU_CHOOSE = 0,
    SHIP_MENU_CONFIRM = 1,      /* the townsperson asks to confirm the boarding */
    SHIP_MENU_LAST_WORDS = 2,   /* his last words; then the tour starts */
    SHIP_MENU_TOUR = 3,         /* the camera tour runs (ShipMenu_IsTouring) */
    SHIP_MENU_FADE = 4,         /* the screen fades out; then the area changes */
    SHIP_MENU_LEAVE = 5         /* his words on leaving, then the menu closes */
};




/*
 * The hunting records (the records menu, TOWN_MODE_RECORDS, in the menu
 * work area gMenuWork): the guides' four categories (gRecordGuideCats: a name, a list of 12-byte
 * entries and their count), the items and monsters seen (bits copied from
 * the save), pages of 8 or 10 rows.
 */
typedef struct RecordCat {
    /* 0x0 */ char *name;
    /* 0x4 */ u8 *list;
    /* 0x8 */ s32 count;
} RecordCat;

/* An entry of a records category (12 bytes): its pages of text. */
typedef struct RecordEntry {
    /* 0x0 */ char *name;
    /* 0x4 */ char **pages;
    /* 0x8 */ s16 pageCount;
    /* 0xA */ s16 unkA;
} RecordEntry;

typedef struct RecordMenu {
    /* 0x000 */ WinSlide win;
    /* 0x008 */ u8 unk8[8];
    /* 0x010 */ u32 itemBits[0x20]; /* items obtained (gSave.itemBits, a member's from Records_RecvItemBits; ItemBits_Test) */
    /* 0x090 */ u32 titleBits[8];   /* a bit per gHunterTitles entry (gSave.titles, a member's from Records_RecvTitleBits; Bits32_Test, Records_TallyTitles) */
    /* 0x0B0 */ u16 sizeMax[0x80];  /* per monster (gSave.sizeMax; a member's from Records_RecvMonsterRecords kind 0) */
    /* 0x1B0 */ u16 sizeMin[0x80];  /* per monster (gSave.sizeMin; Records_RecvMonsterRecords kind 1) */
    /* 0x2B0 */ u16 hunts[0x80];    /* per monster: gSave.hunted + captured (Records_RecvMonsterRecords kind 2) */
    /* 0x3B0 */ u16 ids[8];         /* the rows of the page shown (monster and item lists) */
    /* 0x3C0 */ s16 kind[9];        /* the menu's entries (Records_Open: up to 9) */
    /* 0x3D2 */ u8 equipBits[0x282]; /* the equipment list's flags packed (Bits_Pack of gSave's bladeFlags, gunFlags, armorFlags; a member's from Records_RecvEquipBits); Records_EquipInput clears 640 bytes */
    /* 0x654 */ void *rows[10];     /* the rows of the page shown (category lists) */
    /* 0x67C */ EquipItem items[8];
    /* 0x6DC */ s32 tally[9][2];    /* per item category: seen, total (Records_TallyTitles, Records_TallyItems) */
    /* 0x724 */ s32 equipTally[10]; /* pairs (owned, in all): per genre (Records_TallyWeapons) or per armour type (Records_TallyArmor); the owner's by message (Records_RecvEquipTally) */
    /* 0x74C */ char password[0xA];  /* the house password typed (Chat_OpenKbd, Kbd_Run; Records_HouseAccessInput) */
    /* 0x756 */ s16 rowCount;
    /* 0x758 */ s16 page;
    /* 0x75A */ s16 pages;
    /* 0x75C */ s16 subCursor;      /* a left-right cursor: the equipment level (below levelCount), a decoration's page */
    /* 0x75E */ s16 levelCount;     /* Equip_LevelMark's result (Records_EquipListInput) */
    /* 0x760 */ s16 genre;          /* the equipment genre (gEquipGroupTexts; Records_EquipInput) */
    /* 0x762 */ s16 genreCount;     /* the genres (5) */
    /* 0x764 */ s16 menuSel;        /* the menu's cursor: an index into kind (Records_MenuInput, Records_DrawMenu) */
    /* 0x766 */ s16 sel;            /* the category */
    /* 0x768 */ s16 cursor;
    /* 0x76A */ s16 menuCount;      /* the menu's entries */
    /* 0x76C */ s16 timer;           /* Records_MenuInput: frames left for a member's records */
    /* 0x76E */ u8 equipType;       /* the EquipItem type listed (Records_SetEquipPage): EQUIP_BLADE, EQUIP_GUN, or an armour type, EQUIP_HEAD to EQUIP_WAIST and EQUIP_LEGS (Records_EquipInput) */
    /* 0x76F */ s8 toggle;          /* PAD_SQUARE toggles it: the equipment's preview and level line, a title's description */
    /* 0x770 */ s8 houseAccess;     /* the house access setting chosen (HOUSE_ACCESS_*): gHouseAccess on opening, stored back on done */
    /* 0x771 */ u8 reqKind;         /* Records_MenuInput: the kind requested */
    /* 0x772 */ s8 mode;            /* Records_Update's state (switch) */
    /* 0x773 */ s8 step;            /* Records_ItemsInput: 0 the categories, 1 the item list */
    /* 0x774 */ s8 eventMark;       /* 1 when the kind's record word (gSave.recordCounts[40] to [43], set by the quest download) is 1 (Records_Update); the lists start a line lower while it is 0 */
} RecordMenu;
typedef char RecordMenu_size_check[(sizeof(RecordMenu) == 0x778) ? 1 : -1];

/* RecordMenu.kind[]: the records menu's entries (Records_Open lists them). */
enum RecordKind {
    RECORD_KIND_HOUSE_ACCESS = 0,   /* who may enter the house (online, the owner only) */
    RECORD_KIND_GUIDES = 1,         /* the guides' text pages (gRecordGuideCats) */
    RECORD_KIND_MONSTERS = 2,       /* the monsters' sizes and hunts */
    RECORD_KIND_ITEMS = 3,          /* the items obtained */
    RECORD_KIND_WEAPONS = 4,
    RECORD_KIND_ARMOR = 5,
    RECORD_KIND_TITLES = 6,         /* the owner only */
    RECORD_KIND_SCORES_A = 7,       /* the owner only, once an unlocking item is obtained */
    RECORD_KIND_SCORES_B = 8,       /* the same, with another item */
    RECORD_KIND_COUNT = 9
};

/* RecordMenu.mode: Records_Update's states. */
enum RecordsMode {
    RECORDS_MODE_OPEN = 0,          /* the window slides in */
    RECORDS_MODE_MENU = 1,          /* the menu of record kinds */
    RECORDS_MODE_TO_PAGE = 2,       /* the window slides out to a kind's screen */
    RECORDS_MODE_BACK = 3,          /* the window slides back in to the menu */
    RECORDS_MODE_ITEMS = 4,
    RECORDS_MODE_TITLES = 5,
    RECORDS_MODE_GUIDES = 6,
    RECORDS_MODE_HOUSE_ACCESS = 7,
    RECORDS_MODE_MONSTERS = 8,
    RECORDS_MODE_SCORES = 9,
    RECORDS_MODE_EQUIP = 10
};

/* RecordMenu.step while the menu or the equipment screen waits for the
 * house owner's records (the Records_Recv* functions). */
#define RECORDS_STEP_WAIT       1   /* asked the owner, waiting for the reply */
#define RECORDS_STEP_ARRIVED    3   /* on the menu: the reply arrived, the page opens */

/* RecordMenu.genre: the weapon genres of the records' equipment lists
 * (Weapon_IsInGenre). */
enum RecordsGenre {
    RECORDS_GENRE_GREAT_SWORDS = 0, /* great swords and long swords */
    RECORDS_GENRE_SMALL_SWORDS = 1, /* sword and shield, dual swords */
    RECORDS_GENRE_HAMMERS = 2,      /* hammers and hunting horns */
    RECORDS_GENRE_LANCES = 3,       /* lances and gunlances */
    RECORDS_GENRE_GUNS = 4,         /* the bowguns and anything else */
    RECORDS_GENRE_COUNT = 5
};

#define RECORDS_PAGE_ROWS       8       /* the rows of an equipment, item or title page */
#define MONSTER_RECORD_COUNT    0x4A    /* the monsters whose records the owner sends */
#define RECORDS_GUIDE_ROWS      10      /* the rows of a guide category's page */
#define RECORD_GUIDE_COUNT      4       /* the guides' categories (gRecordGuideCats) */
#define RECORDS_ITEM_CATEGORIES 8       /* the item list's categories (ItemData.category 1 to 8) */
#define HOUSE_PASSWORD_SIZE     9       /* gHousePassword: the password that lets visitors into the house */

/* The house access page's rows (RecordMenu.sel): the three settings
 * (HOUSE_ACCESS_*), then done. */
#define HOUSE_ACCESS_ROW_DONE   3
#define HOUSE_ACCESS_ROWS       4

/* Records_HouseAccessInput's steps (RecordMenu.step). */
enum HouseAccessStep {
    HOUSE_ACCESS_STEP_CHOOSE = 0,       /* the rows */
    HOUSE_ACCESS_STEP_TYPING = 1,       /* the keyboard is open for the password */
    HOUSE_ACCESS_STEP_EMPTY_NOTICE = 2, /* the password was empty: a message until PAD_CIRCLE */
    HOUSE_ACCESS_STEP_REOPEN = 3,       /* the keyboard opens again */
    HOUSE_ACCESS_STEP_SENDING = 4       /* the setting is being sent */
};
/* The equipment flags packed per type (Bits_Pack): as many as the blade
 * table has (BLADE_ID_COUNT) for every type. */
#define RECORDS_EQUIP_BITS      BLADE_ID_COUNT
/* The town message kind that asks the owner for one equipment type's
 * flags (TownMsg_SendRecordsAsk): 0xA plus the EquipItem type. */
#define RECORDS_REQ_EQUIP(type) (0xA + (type))

/*
 * A flock of birds in the town's hubs (a stage object of kind
 * LOBBY_OBJ_BIRD_FLOCK, BirdFlock_Spawn; its buf is a BirdFlock).
 * FLOCK_BIRDS birds sit on FLOCK_BIRD_SPOTS spots (gFlockBirdSpots), one
 * bird to a spot; each turns about on its spot, spreads its wings, flies off
 * and up, and comes back from far away to another free spot.
 */
#define FLOCK_BIRDS 6           /* the birds of a flock */
#define FLOCK_BIRD_SPOTS 16     /* the spots a bird can sit on (gFlockBirdSpots) */
#define FLOCK_SPOT_SLOTS 8      /* BirdFlock.spot's size: FLOCK_BIRDS used */
#define FLOCK_BIRD_SE_TAKE_OFF 0xE6 /* the monsters' bank (SE_SLOT_EM): the wings as a bird flies off */
#define FLOCK_BIRD_SE_CALL 0xE7     /* the monsters' bank: a bird's call as it comes in to land */

/* FlockBird.state */
enum FlockBirdState {
    FLOCK_BIRD_SITTING = 0,     /* on its spot, turning now and then */
    FLOCK_BIRD_ARRIVING = 1,    /* flying in to its spot, then landing */
    FLOCK_BIRD_LEAVING = 2      /* flying off and up */
};

/* The birds' animations (gFlockBirdAnims, FlockBird.anim). */
enum FlockBirdAnimId {
    FLOCK_BIRD_ANIM_SIT = 0,    /* no list: part 0 */
    FLOCK_BIRD_ANIM_FLY = 1,    /* parts 1-5, two frames each, looping */
    FLOCK_BIRD_ANIM_FLAP = 2,   /* parts 1-5, a frame each, looping: lifting off, landing */
    FLOCK_BIRD_ANIM_FOLD = 3,   /* parts 1-4 once: the wings fold after landing */
    FLOCK_BIRD_ANIM_SPREAD = 4  /* parts 4-1 once: the wings spread before flying off */
};

/* An animation of the flock's birds (gFlockBirdAnims, by enum
 * FlockBirdAnimId): the bird's material set of the common model and a list of
 * (part, frames) pairs; a part of -1 ends it (frames: -1 stop, else the pair
 * to go on with). */
typedef struct FlockBirdPair {
    s8 part;        /* the material shown; -1: the list's end */
    s8 frames;      /* frames it shows; at the end: -1 stop, else the pair to go on with */
} FlockBirdPair;
typedef struct FlockBirdAnim {
    s32 matSet;
    FlockBirdPair *list;
} FlockBirdAnim;

/* A bird of a flock, 76 bytes. */
typedef struct FlockBird {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ Vec3f vel;           /* the speeds: x, then z (in vel.y), then the height (in vel.z) */
    /* 0x18 */ f32 accel;           /* added to the climbing speed (vel.z) while it is under 15 */
    /* 0x1C */ u8 unk1C[2];
    /* 0x1E */ s16 timer;           /* frames: (1-5) * 30 sitting before it may fly off; the landing's and lift-off's frames */
    /* 0x20 */ s16 turnWait;        /* frames until the sitting bird may turn: 30 */
    /* 0x22 */ u16 ang;             /* the heading, ANGLE_TURN to a turn */
    /* 0x24 */ s32 color;           /* its tint (RS_COLOR2); -1: none */
    /* 0x28 */ FlockBirdPair *list; /* the animation's (part, frames) pairs */
    /* 0x2C */ f32 climb;           /* the height gained flying off */
    /* 0x30 */ s8 matSet;           /* the material set of the common model it is drawn with */
    /* 0x31 */ s8 state;            /* enum FlockBirdState */
    /* 0x32 */ s8 part;             /* the part shown (a material of the set) */
    /* 0x33 */ s8 frames;           /* left for this part; -1: stopped */
    /* 0x34 */ s8 idx;              /* in the list */
    /* 0x35 */ s8 dir;              /* a direction: ang is dir << 13 (of 8) or << 12 (of 16) */
    /* 0x36 */ s8 step;             /* within the state */
    /* 0x37 */ s8 spreadCount;      /* counted up as it spreads its wings, cleared as it settles; nothing reads it */
    /* 0x38 */ s8 called;           /* the arrival call played */
    /* 0x39 */ u8 unk39;
    /* 0x3A */ s8 anim;             /* enum FlockBirdAnimId */
    /* 0x3B */ u8 unk3B;
    /* 0x3C */ f32 unk3C;           /* 20 at each new spot; nothing reads it */
    /* 0x40 */ Vec3f dest;          /* its spot, 20 above */
} FlockBird;
typedef char FlockBird_size_check[(sizeof(FlockBird) == 0x4C) ? 1 : -1];

/* A flock (the buf of its StageObj, set up by BirdFlock_Init). */
typedef struct BirdFlock {
    /* 0x00 */ s8 spot[FLOCK_SPOT_SLOTS]; /* the spot of each bird; -1: none */
    /* 0x08 */ f32 scale;           /* the birds' size (1.2), also the factor of their flying speeds */
    /* 0x0C */ FlockBird bird[FLOCK_BIRDS];
} BirdFlock;

/* the lobby scene's object of LobbyFlock_Run (a sub_main object, Obj_Alloc(1); a view of the Obj):
 * 16 parts, each drawn by its own draw slot (LobbyFlock_DrawPart) */
typedef struct LobbyPart {
    /* 0x00 */ f32 vel[3];          /* its speed, in its own axes */
    /* 0x0C */ u16 rot[3];          /* angles, ANGLE_TURN to a turn; random at the start (LobbyFlock_DrawPart draws with rot[0], rot[1]) */
    /* 0x12 */ s16 spin;            /* rot[0]'s step per frame */
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 slot;            /* its draw slot (DrawSlot_Alloc), -1: none */
    /* 0x1C */ struct DrawSlot *draw;      /* DrawSlot_Get(slot) */
} LobbyPart;

typedef struct LobbyFlock {
    /* 0x00 */ u8 active;           /* Obj.active */
    /* 0x01 */ u8 visible;          /* Obj.visible */
    /* 0x02 */ u8 kind;             /* Obj.kind: 24 (LobbyFlock_Spawn) */
    /* 0x03 */ u8 type;             /* Obj.type: 0 */
    /* 0x04 */ u8 state;            /* Obj.state: LobbyFlock_Run's switch */
    /* 0x05 */ s8 unk5;             /* Obj.var0: Rand_Next(1) & 3 at the start */
    /* 0x06 */ u8 pad6[2];
    /* 0x08 */ s16 timer;           /* Obj.timer */
    /* 0x0A */ u8 padA[0xA];
    /* 0x14 */ void *draw;          /* Obj.draw: NULL (its parts draw through their slots) */
    /* 0x18 */ LobbyPart *parts;    /* Obj.buf: 16 */
    /* 0x1C */ u8 pad1C[4];
    /* 0x20 */ f32 pos[3];          /* Obj.pos */
    /* 0x2C */ u8 pad2C[0x40 - 0x2C];
} LobbyFlock;
typedef char LobbyFlock_size_check[(sizeof(LobbyFlock) == 0x40) ? 1 : -1]; /* an Obj */

/* The shooting gallery's other objects and the house meal scene. */

/*
 * A shot of the gallery (kind LOBBY_OBJ_GALLERY_SHOT). Type
 * GALLERY_SHOT_BULLET flies from the gun towards the aim until it hits
 * something or its time runs out; the gallery tests it against the targets
 * and the walls each frame (GalleryShot_GetCapsule). Where it hits a wall,
 * a flash and three puffs show (GalleryShot_SpawnWallHit).
 */
typedef struct GalleryShot {
    /* 0x00 */ f32 speed;           /* the bullet: units a frame; a puff: its rise a frame */
    /* 0x04 */ Vec3f start;         /* the bullet's start, ahead of the muzzle */
    /* 0x10 */ Vec3f aim;           /* the point aimed at */
    /* 0x1C */ Vec3f cur;           /* the bullet's position */
    /* 0x28 */ Vec3f prev;          /* its position the frame before */
    /* 0x34 */ Vec3f vel;           /* its move a frame */
    /* 0x40 */ u8 hit;              /* 1 once it hit something (GalleryShot_SetHit): it ends */
    /* 0x41 */ u8 pad41[3];
    /* 0x44 */ Vec3f scale;         /* the flash's and puffs' scale, from their keys */
    /* 0x50 */ f32 alpha;           /* their alpha, from their keys */
    /* 0x54 */ s16 roll;            /* their random roll (ANGLE_TURN to a turn) */
} GalleryShot;

/* The results' score (kind LOBBY_OBJ_GALLERY_SCORE): its value split into
 * decimal digits, shown one by one from the lowest. */
#define GALLERY_SCORE_DIGITS 10
typedef struct GalleryScore {
    /* 0x00 */ s16 digit[GALLERY_SCORE_DIGITS];    /* lowest first */
    /* 0x14 */ s16 value;
    /* 0x16 */ s16 shown;           /* the digits settled so far */
    /* 0x18 */ s16 rolling;         /* the digit rolling in the next place (0 to 9) */
    /* 0x1A */ s16 count;           /* the value's digits (at least one) */
    /* 0x1C */ s16 rollTimer;       /* frames since the rolling digit last stepped */
    /* 0x1E */ s16 y;               /* the screen row */
} GalleryScore;

/*
 * Particles thrown from an actor (kind LOBBY_OBJ_ACTOR_PARTICLES; owner the
 * actor they come from): six parts, each with its own draw slot. The
 * types (ACTOR_PARTICLES_*):
 * - SPRAY: small pale bits thrown fast from the owner's bone 12 or 20,
 *   thrown again on a timer, the spray turning on and off at random, while
 *   the owner stays in NPC_STATE_CONTEST_GRIP (the arm-wrestling grip,
 *   whose state function spawns them);
 * - HAIR: bits in the owner's hair colour (a Player) that fly up, fall and
 *   tumble, until the barber (em) passes frame 50 of its
 *   SERVICE_MOT_HAIR_CUT_NEXT;
 * - BARBER: three pairs of short effects at the barber's bones 6 and 9,
 *   for 30 frames.
 */
enum ActorParticlesType {
    ACTOR_PARTICLES_SPRAY = 0,
    ACTOR_PARTICLES_HAIR = 1,
    ACTOR_PARTICLES_BARBER = 2
};
#define ACTOR_PARTICLES 6   /* the parts of one object */
typedef struct ActorParticles {
    /* 0x000 */ struct Em *em;             /* HAIR and BARBER: the barber, whose bones and motion they follow */
    /* 0x004 */ Vec3f dir[ACTOR_PARTICLES];    /* the direction thrown (z turned by random x and y angles) */
    /* 0x04C */ Vec3f pos[ACTOR_PARTICLES];
    /* 0x094 */ Vec3f rot[ACTOR_PARTICLES];    /* degrees (ActorParticles_Draw) */
    /* 0x0DC */ Vec3f spin[ACTOR_PARTICLES];   /* added to rot every frame */
    /* 0x124 */ f32 speed[ACTOR_PARTICLES];
    /* 0x13C */ s8 timer[ACTOR_PARTICLES];     /* frames until the part is thrown again; BARBER: its life */
    /* 0x142 */ s8 frame[ACTOR_PARTICLES];     /* frames since thrown (the draw's key time) */
    /* 0x148 */ f32 vy[ACTOR_PARTICLES];       /* HAIR: the rise a frame (1 less each frame, down to -2) */
    /* 0x160 */ s16 slot[ACTOR_PARTICLES];     /* DrawSlot_Alloc */
    /* 0x16C */ DrawSlot *part[ACTOR_PARTICLES];   /* DrawSlot_Get(slot); NULL: none was free */
    /* 0x184 */ u8 on;              /* 1 while the parts move and draw (the spray toggles it) */
} ActorParticles;

/*
 * The points a downed target gives, rising from it (kind
 * LOBBY_OBJ_GALLERY_POINTS) for 6 frames.
 */
typedef struct GalleryPoints {
    /* 0x00 */ f32 x, y, z;         /* its screen position (Camera_ProjectPoint) */
    /* 0x0C */ u8 padC[4];
    /* 0x10 */ s16 value;           /* the points */
    /* 0x12 */ u16 age;             /* frames shown */
} GalleryPoints;

/* The meal scene's work area (gMenuWork, TOWN_MODE_MEAL_SCENE). */
typedef struct MealScene {
    /* 0x00 */ struct Em *npc;             /* the cook spoken to (MealScene_Open) */
    /* 0x04 */ struct Player *pl;          /* the local player */
    /* 0x08 */ s32 seatMask;        /* the bits of the two seats' spots, for the meal scene (NpcScene_Start) */
    /* 0x0C */ s16 plSpot;          /* the player's seat: an action spot of the house (Actor_MoveToSceneSpot) */
    /* 0x0E */ s16 npcSpot;         /* the cook's seat */
    /* 0x10 */ s16 timer;
    /* 0x12 */ s16 talk;            /* the cook's first line; +1 accepted, +2 declined, +3 after the meal, +4 already eaten */
    /* 0x14 */ s8 step;             /* MealScene_Update's step (MEAL_STEP_*) */
} MealScene;

/* MealScene_Update's steps (MealScene.step). */
enum MealStep {
    MEAL_STEP_OFFER = 0,        /* the cook's offer: accepted or declined */
    MEAL_STEP_ACCEPTED = 1,     /* the accepted line, then a fade out */
    MEAL_STEP_SEAT = 2,         /* faded out: both are seated */
    MEAL_STEP_SEATED = 3,       /* 3 frames, then a fade in, the kitchen camera and the cooking lesson */
    MEAL_STEP_COOKING = 4,      /* until the lesson is done */
    MEAL_STEP_SERVE = 5,        /* until the player's motion is PL_MOT_TOWN_SEATED; then the meal's effects */
    MEAL_STEP_EATING = 6,       /* until the player's motion ends */
    MEAL_STEP_EATEN = 7,        /* the same, then the cook's line after the meal */
    MEAL_STEP_THANKS = 8,       /* that line, then a fade out */
    MEAL_STEP_LEAVE = 9,        /* faded out: the cook leaves, the scene closes */
    MEAL_STEP_DECLINED = 10     /* the declined or already-eaten line, then the scene closes */
};

/* A meal's bonuses (gMealEffects: by the house's level, then by the cook's
 * group), copied into the scene's meal fields for the next quest. */
typedef struct MealEffect {
    s16 stamina;
    s8 health;
    s8 attack;
    s8 defense;
    u8 pad;
} MealEffect;


/*
 * The mug a drinking townsperson holds (kind LOBBY_OBJ_DRINK_MUG): the work
 * in its StageObj.buf. StageObj.type is its phase (enum DrinkMugPhase), and
 * in DRINK_MUG_HELD StageObj.var0 is DRINK_MUG_WAIT or DRINK_MUG_SWIVEL.
 * Angles named yaw or swivel are 16-bit (ANGLE_TURN to a turn); the headings are
 * radians.
 */
typedef struct DrinkMugWork {
    /* 0x00 */ struct EmBone *hand;     /* the townsperson's DRINK_MUG_BONE */
    /* 0x04 */ struct Em *npc;          /* the townsperson holding it */
    /* 0x08 */ s32 swivelGoal;          /* a swivel's random angle, 0 to 90 degrees */
    /* 0x0C */ s8 swivelFrames;         /* the swivel's frames left (1 to 3) */
    /* 0x0D */ s8 idleWait;             /* frames before the next swivel (5 to 14) */
    /* 0x0E */ s8 frameTimer;           /* the phases' second frame counter */
    /* 0x0F */ s8 animFrame;            /* the phase's frame: the material DrinkMug_Draw shows */
    /* 0x10 */ s16 swivelEnd;           /* swivelGoal - 45 degrees: the swivel's end, -45 to +45 */
    /* 0x12 */ u8 pad12[2];
    /* 0x14 */ f32 swivelStep;          /* added to swivelOffset each frame of a swivel */
    /* 0x18 */ u16 swivelOffset;        /* the held mug's angle from the townsperson's heading */
    /* 0x1A */ s16 yaw;                 /* swivelOffset + the townsperson's rotY: the held mug's yaw */
    /* 0x1C */ Vec3f orbit;             /* the offset from the townsperson (circling) or the hand (returning) */
    /* 0x28 */ Vec3f lastPos;           /* StageObj.pos at the start of the frame */
    /* 0x34 */ u8 atHand;               /* 1 once the returning mug has reached the hand */
    /* 0x35 */ u8 returning;            /* 1 once the contest's end has sent it back */
    /* 0x36 */ u8 pad36[2];
    /* 0x38 */ f32 flyHeading;          /* the flight's heading, turned by curveRate each frame */
    /* 0x3C */ f32 heading;             /* the yaw drawn while it hovers, settles, turns and rests */
    /* 0x40 */ s8 hopFrame;             /* the turn's hops: an 8-frame cycle */
    /* 0x41 */ s8 bobFrame;             /* the bob while it flies or hovers */
    /* 0x42 */ s8 bobRise;              /* the hover at the hand: frames of rising */
    /* 0x43 */ u8 turnFrames;           /* frames of the turn to the townsperson's heading */
    /* 0x44 */ s8 turnDir;              /* -1, 0 or 1: the turn's direction */
    /* 0x45 */ u8 pad45;
    /* 0x46 */ s16 curveRate;           /* the flight's turn a frame (16-bit angle) */
    /* 0x48 */ f32 speed;               /* the flight's distance a frame */
    /* 0x4C */ s16 backTimeout;         /* frames DrinkMug_IsBack waits before it reports the mug back */
} DrinkMugWork;

/* StageObj.type of the mug. */
enum DrinkMugPhase {
    DRINK_MUG_HELD = 0,     /* in the hand, swivelling now and then */
    DRINK_MUG_LIFT = 1,     /* the contest started: lifts off */
    DRINK_MUG_FLY_OUT = 2,  /* curves out and up to DRINK_MUG_ORBIT_RADIUS */
    DRINK_MUG_HOVER = 3,    /* hovers, over the hand once back */
    DRINK_MUG_CIRCLE = 4,   /* circles the townsperson */
    DRINK_MUG_RETURN = 5,   /* the contest ended: spirals back to the hand */
    DRINK_MUG_TURN = 6,     /* turns to the townsperson's heading with small hops */
    DRINK_MUG_REST = 7,     /* rests for StageObj.timer frames */
    DRINK_MUG_RESET = 8,    /* back to HELD, as at set-up */
    DRINK_MUG_SETTLE = 9    /* settles into the hand */
};
#define DRINK_MUG_WAIT 0        /* StageObj.var0 in DRINK_MUG_HELD: waiting for the next swivel */
#define DRINK_MUG_SWIVEL 1      /* swivelling */
#define DRINK_MUG_BONE 0xB      /* the townsperson's bone the mug is held at */
#define DRINK_MUG_HAND_OFFSET 14.0f     /* the mug sits this far ahead of the bone, along the heading */
#define DRINK_MUG_ORBIT_RADIUS 240.0f   /* the circle it flies round the townsperson */
/* The material sets of the common model (gCommonModels[0]) the mug draws. */
#define DRINK_MUG_MATSET_HELD 41
#define DRINK_MUG_MATSET_FLY 42
#define DRINK_MUG_MATSET_HOVER 43
#define DRINK_MUG_MATSET_LIFT 44

#endif
