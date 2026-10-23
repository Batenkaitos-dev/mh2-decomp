#ifndef BOX_MENU_H
#define BOX_MENU_H

#include "common.h"
#include "item.h"
#include "save.h"
#include "lobby_menu.h"

/*
 * The item box menu's work, gBoxWork (pointing at gBoxWorkMem (0x8B4380),
 * 112 bytes): BoxMenu_InitWork sets it up, BoxMenu_Update and the BoxMenu_*
 * sub-menus run on it, and BoxMenu_Draw and its helpers draw from it.
 */
/* ItemBoxMenu.state (BoxMenu_Update). */
enum BoxMenuState {
    BOX_STATE_LIST = 0,     /* the menu list */
    BOX_STATE_SUB = 1,      /* in the chosen sub-menu */
    BOX_STATE_CLOSING = 2   /* the window slides out */
};

/* ItemBoxMenu.menu: the menu list's entries. */
enum BoxMenuEntry {
    BOX_MENU_STORE = 0,     /* store pouch items (BoxMenu_StoreFromPouch) */
    BOX_MENU_TAKE = 1,      /* take items to the pouch (BoxMenu_TakeToPouch) */
    BOX_MENU_EQUIP = 2,     /* the equipment (BoxMenu_EquipMenu) */
    BOX_MENU_ARRANGE = 3,   /* arrange the box (BoxMenu_ArrangeMenu) */
    BOX_MENU_SELL = 4,      /* sell (BoxMenu_SellMenu) */
    BOX_MENU_COUNT = 5
};

/* ItemBoxMenu.kind outside the equipment menu: which box the cursor is in. */
enum BoxKind {
    BOX_KIND_ITEMS = 0,
    BOX_KIND_EQUIP = 1
};

/* ItemBoxMenu.kind in the equipment menu (BoxMenu_EquipMenu): its entries. */
enum EquipMenuEntry {
    EQUIP_MENU_PIECES = 0,       /* wear single pieces */
    EQUIP_MENU_EDIT_SET = 1,     /* edit a saved equipment set */
    EQUIP_MENU_WEAR_SET = 2,     /* wear a saved set */
    EQUIP_MENU_SAVE_SET = 3,     /* save the worn equipment as a set */
    EQUIP_MENU_SYMBOL_COLOR = 4  /* the symbol colour (only once one is earned) */
};

/* ItemBoxMenu.help: the help window over the list. */
enum BoxHelpMode {
    BOX_HELP_OFF = 0,
    BOX_HELP_OPEN = 1,          /* the item's description, or the piece's skills */
    BOX_HELP_SKILLS = 2,        /* the skills page of a set being edited (BoxMenu_EquipPickInput) */
    BOX_HELP_HIDE_LINE = 0x80,  /* bit: the help line is hidden while the window is up */
    BOX_HELP_DETAIL = 0x81      /* the piece's detail with its levels (PAD_SQUARE) */
};

/* ItemBoxMenu.cursorClass: the equipment at the cursor. */
enum BoxCursorClass {
    BOX_CLASS_ARMOR = 0,
    BOX_CLASS_WEAPON = 1,       /* a weapon (EQUIP_BLADE, EQUIP_GUN) */
    BOX_CLASS_OTHER_WEAPON = 2  /* a weapon of the other type than the one worn */
};

/* BoxMenu_StoreFromPouch's steps (ItemBoxMenu.step). */
enum BoxStoreStep {
    BOX_STORE_PICK = 0,     /* pick a pouch item */
    BOX_STORE_SHOW = 1,     /* the result's message, until PAD_CIRCLE */
    BOX_STORE_CLOSING = 2   /* the pouch window slides out */
};

/* BoxMenu_TakeToPouch's steps (ItemBoxMenu.step). */
enum BoxTakeStep {
    BOX_TAKE_PICK = 0,      /* pick a box item */
    BOX_TAKE_SLOT = 1,      /* pick a pouch slot */
    BOX_TAKE_AMOUNT = 2,    /* the amount to take */
    BOX_TAKE_SWAP = 3,      /* swap with the pouch slot's item (yes/no) */
    BOX_TAKE_RESULT = 4     /* the result's message */
};

/* ItemBoxMenu.part: the set editor's rows. */
#define BOX_PART_SAVE       6   /* the row that saves the set */
#define BOX_PART_COUNT      7

/* ItemBoxMenu.blink's restart value: the next frame's increment makes it 0. */
#define BOX_BLINK_RESTART   0xFF

/* The box lists' draw flags (BoxMenu_DrawItemList, BoxMenu_DrawEquipGrid and
 * their page bar, BoxMenu_DrawPageTabs). */
#define BOX_LIST_PAGE_MASK  0x7         /* the detail window's page, with BOX_LIST_DETAIL */
#define BOX_LIST_DETAIL     0x8         /* the cursor item's description, or the piece's detail window */
#define BOX_LIST_EQUIP      0x10        /* the equipment box, not the item box */
#define BOX_LIST_DIM        0x20        /* dimmed (alpha 96), the page bar's arrows grey */
#define BOX_LIST_MONEY      0x80        /* the money beside the list */
#define BOX_LIST_SORTED     0x100       /* the sorted copy (the sort prompt's preview): no cursor, no name */
#define BOX_LIST_WORN_ALT   0x200       /* worn pieces in the other colour */
#define BOX_LIST_ARMOR      0x400       /* armour: worn pieces marked */
#define BOX_LIST_WEAPONS    0x800000    /* weapons: worn ones marked */
#define BOX_LIST_PRICE      0x1000000   /* the cursor item's price */
#define BOX_LIST_UNFIT_ALT  0x2000000   /* the grid: worn weapons and pieces that don't fit the worn set in the other colour */
/* An item id in bits 13-22: only its stacks are bright, and no cursor shows. */
#define BOX_LIST_ITEM(flags)        (((flags) >> 13) & 0x3FF)
#define BOX_LIST_ITEM_FLAG(item)    (((item) & 0x3FF) << 13)

/* ItemBoxMenu.yesNo: the highlighted answer of a yes/no prompt. */
enum BoxYesNo {
    BOX_YES = 0,
    BOX_NO = 1,
    BOX_YESNO_NONE = 0xFF   /* no prompt */
};

typedef struct ItemBoxMenu {
    /* 0x00 */ u8 updated;          /* 1 once BoxMenu_Update has run through this frame: BoxMenu_Draw draws only then */
    /* 0x01 */ u8 cursorClass;      /* the equipment at the cursor (enum BoxCursorClass) */
    /* 0x02 */ u8 inputOn;          /* bit 0: the menu takes input (PAD_START toggles it) */
    /* 0x03 */ u8 yesNo;            /* a yes/no prompt's highlighted answer (enum BoxYesNo; gYesNoTexts, YesNo_Toggle) */
    /* 0x04 */ union {
        s32 steps;                  /* the four step bytes, cleared together */
        struct {
            u8 state;               /* enum BoxMenuState */
            u8 step;                /* the sub-menu's step */
            u8 sub;                 /* a step within the step */
            u8 sub2;                /* the set editor's step: 0 its rows, 1 a piece list, 2 the yes/no on dropping unfit armour; EquipSet_RenameInput's step */
        };
    };
    /* 0x08 */ union {
        s16 menuKind;               /* menu and kind, cleared together */
        struct {
            u8 menu;                /* the menu list cursor (enum BoxMenuEntry) */
            u8 kind;                /* enum BoxKind; in the equipment menu, its entry (enum EquipMenuEntry) */
        };
    };
    /* 0x0A */ u8 pick;             /* the symbol colour menu's cursor, into list */
    /* 0x0B */ u8 unkB;
    /* 0x0C */ struct Player *player; /* the local player (gPlayers + localPlayer) */
    /* 0x10 */ EquipItem *equipped;  /* the worn item of the type (on the set screen, the set's), NULL none */
    /* 0x14 */ EquipItem *boxItem;   /* the equipment box item at the cursor (&gSave.box[slots[1]]), NULL none */
    /* 0x18 */ s32 price;           /* the selling price (BoxMenu_SellMenu) */
    /* 0x1C */ u16 slots[2];         /* the item box and equipment box slots, indexed by kind ([0] also as slot); in a union, MWCC adds offset 0x1C before a variable index: the s32 clear is a cast */
    /* 0x20 */ u16 held;            /* the slot held for a swap (BoxMenu_ArrangeMenu); BoxMenu_DrawGridCursor: % 100 */
    /* 0x22 */ u8 pouch;            /* pouch slot */
    /* 0x23 */ u8 atMax;            /* the amount is the most that can move */
    /* 0x24 */ s16 amount;          /* amount to move (BoxMenu_InputAmount) */
    /* 0x26 */ u16 pouchItem;       /* the pouch item being stored (BoxMenu_StoreFromPouch) */
    /* 0x28 */ u8 levelSel;         /* a cursor (Menu_CursorLeftRight) below levelCount; on the set screens, the info page (PAD_SQUARE cycles 0-2, 2 the skills) */
    /* 0x29 */ u8 unk29;
    /* 0x2A */ u8 levelCount;       /* Equip_LevelMark's result: levelSel's limit */
    /* 0x2B */ u8 skillPage;        /* the skill view's page (Menu_CursorLeftRight; EquipDetail_Draw) */
    /* 0x2C */ u8 skillPages;       /* the skill view's pages (SkillView_Init, Equip_PreviewSkills) */
    /* 0x2D */ s8 msg;              /* a help line (gHelpLine), -1 none */
    /* 0x2E */ u8 help;             /* the help window's mode (enum BoxHelpMode; BoxMenu_DrawHelp) */
    /* 0x2F */ u8 blink;            /* a frame counter: 255 at open, (& 0x1F) < 20 shows the cursor */
    /* 0x30 */ u8 prompt;           /* a yes/no prompt is up (the sort: BoxMenu_SortPrompt; yesNo the answer) */
    /* 0x31 */ u8 canEquip;         /* the equipment at the cursor fits (gender, weapon class): the comparison
                                       marks it otherwise (flag 8); the help draws its skills (EquipDetail_Draw) */
    /* 0x32 */ u8 unk32;
    /* 0x33 */ u8 close;            /* BoxMenu_CloseStep's counter */
    /* 0x34 */ EquipSet cur;        /* the equipment set being edited */
    /* 0x54 */ u8 newSet;           /* 1 while the edited set has no weapon yet: cancelling the weapon list leaves the editor */
    /* 0x55 */ u8 set;              /* the equipment set (SaveData.equipSets) */
    /* 0x56 */ u8 setEntries;       /* the equipment set menu's entries: 4, or 5 with symbol colours */
    /* 0x57 */ u8 part;             /* the set editor's row: 0-5 a part (gBoxPartIdx gives its SAVE_EQUIP_* entry), 6 the row that saves the set */
    /* 0x58 */ u8 unk58[2];
    /* 0x5A */ u8 page;             /* BoxList_MoveCursor's page */
    /* 0x5B */ s8 pageCount;        /* the box list's pages: BoxMenu_DrawEquipPick shows page/pages above 1 */
    /* 0x5C */ u8 row;              /* BoxList_MoveCursor's row */
    /* 0x5D */ u8 count;            /* entries in list */
    /* 0x5E */ u8 list[12];         /* the earned symbol colours (BoxMenu_ListStyles), or the box list's entries */
    /* 0x6A */ u8 unk6A[6];
} ItemBoxMenu;
typedef char ItemBoxMenu_size_check[(sizeof(ItemBoxMenu) == 0x70) ? 1 : -1];

/*
 * The skill view's state at the start of gSkillPreview (160 bytes; SkillList_Build
 * fills the rest with the skill list). Equip_PreviewSkills fills it for one piece
 * tried on, SkillView_Init for a whole equipment set; EquipDetail_Draw draws it.
 */
typedef struct SkillPreview {
    /* 0x00 */ struct Player *compare;  /* the hunter the skill list is compared with, NULL none */
    /* 0x04 */ EquipItem item;          /* the piece previewed; item.used 0: none (a set's skills) */
    /* 0x10 */ u8 pages;                /* the skill list's pages (SkillList_PageCount) */
    /* 0x11 */ u8 wearable;             /* the hunter can wear item (Equip_DataWearable; 1 for a weapon) */
    /* 0x12 */ u8 torsoUp;              /* the tried armour has torso up (Player_ArmorHasTorsoUp) */
} SkillPreview;
extern SkillPreview gSkillPreview;

/* A player copied whole: the game's copy moves 16 bytes at a time (lq/sq),
 * as for a 16-aligned type (Player itself is 4-aligned). The previews copy
 * the hunter through it. */
typedef struct PlayerCopy {
    u8 b[0x970];
} __attribute__((aligned(16))) PlayerCopy;

/* The hunter as an equipment set would make them: the set screen's stats and
 * skills preview (EquipSet_RecalcStats). */
extern struct Player gPreviewPlayer;

/* The box list (BoxList_Build): the gSave.box slots of one equipment class, sorted or in box
 * order, shown seven to a page (BoxList_GetPage, BoxList_MoveCursor). */
typedef struct BoxList {
    u16 idx[500];       /* 0x000: indexes into gSave.box */
    u16 count;          /* 0x3E8 */
    u8 invalid;         /* 0x3EA: 1 when BoxList_Build was given a kind over 5 */
    u8 kind;            /* 0x3EB: the equipment class, gBoxTypeKinds[type] (ItemBoxMenu.part) */
    u8 sorted;          /* 0x3EC: by BoxEquip_SortKey */
    u8 pages;           /* 0x3ED */
    u8 lastCount;       /* 0x3EE: the entries on the last page */
} BoxList;
typedef char BoxList_size_check[(sizeof(BoxList) == 0x3F0) ? 1 : -1];
extern BoxList gBoxList;

/* A box list's page bar (BoxMenu_DrawPageTabs): gBoxPageTabs[0] is the item
 * list's, [1] the equipment grid's. The x offsets are from the bar's x. */
typedef struct BoxTab {
    /* 0x00 */ f32 tabW;        /* the tab sprites' width */
    /* 0x04 */ f32 leftX;       /* the left tab sprite's x */
    /* 0x08 */ f32 rightX;      /* the right tab sprite's x */
    /* 0x0C */ s16 arrowW;      /* the arrows' width */
    /* 0x0E */ s16 rightArrowX; /* the right arrow's x (the left one is at the bar's x) */
    /* 0x10 */ s16 textX;       /* the page number's x */
    /* 0x12 */ u8 framed;       /* nonzero: the bar's window (gBoxPageTabsWin) is drawn under it */
} BoxTab;
typedef char BoxTab_size_check[(sizeof(BoxTab) == 0x14) ? 1 : -1];
void BoxMenu_DrawGridCursor(void);
void BoxMenu_DrawItemList(s16 cur, u32 flags, f32 listX);
void BoxMenu_DrawPageTabs(s16 x, s16 y, s16 page, u32 flags, s32 alpha, u8 kind);
s8 BoxList_Build(u8 kind, u8 sorted);
void BoxList_InsertSorted(u16 *list, u16 idx, u16 n);
s16 BoxList_MoveCursor(u8 *row, u8 *page, u16 pad);
void EquipDetail_Draw(Player *pl, s16 x, s16 y, u32 flags);
void EquipDetail_DrawNone(s16 x, s16 y);
void BoxMenu_DrawControls(s32 where);
void BoxMenu_DrawActions(s32 dimmed);
void BoxMenu_Draw(void);
void BoxMenu_DrawStore(void);
void BoxMenu_DrawTake(void);
void BoxMenu_DrawEquip(void);
u32 Color_Half(u32 color);
void BoxMenu_DrawListHeader(u8 *equipSet, s32 flags, u8 which);
u8 BoxMenu_DrawEquipList(u16 *slot, s32 part, s32 flags);
void EquipInfo_DrawPage(s16 y, s32 hasItem, s32 page, Player *player);
void BoxMenu_DrawEquipInfo(u16 *slot, s32 part, s32 flags, Player *player);
void BoxMenu_DrawEquipPick(EquipIdx *cur, s8 sel, u32 flags);
void BoxMenu_DrawArrange(void);
void BoxMenu_DrawSell(void);
void BoxMenu_DrawHelp(s32 helpMode, s32 pouchSlot);
void BoxMenu_DrawAmount(void);
void BoxMenu_DrawZenny(void);
void BoxMenu_DrawYesNo(void);
void *Box_BuildSortedList(u8 equip);
void Box_ApplySort(u8 equip);
u32 BoxEquip_SortKey(EquipItem *item);
void BoxEquip_ShellSort(EquipItem *list, u32 n);
u16 Box_FindEquip(EquipItem *box, EquipItem *item);
extern BoxTab gBoxPageTabs[2];

extern ItemBoxMenu *gBoxWork;
extern ItemBoxMenu gBoxWorkMem;

void BoxMenu_InitWork(void);
s32 BoxMenu_Open(void);
s32 BoxMenu_Update(u16 pad);
void BoxMenu_MoveCursor(u16 *cur, u16 pad);
u16 BoxMenu_StoreFromPouch(u16 pad);
u16 BoxMenu_TakeToPouch(u16 pad);
u16 BoxMenu_EquipMenu(u16 pad);
void EquipSet_RecalcStats(EquipIdx *slots, s32 compareTo);
s32 EquipSet_RenameInput(u16 pad);
void Equip_SetEquipped(EquipIdx *set);
u8 BoxMenu_CloseStep(u16 pad, u8 step);
u16 BoxMenu_EquipPickInput(u16 *setIdx, u16 pad, s32 *ok, s32 flags);
void BoxMenu_UpdateSkillPreview(void);
u8 Equip_PreviewSkills(Player *player, EquipItem *tried);
u8 SkillView_Init(void *pl, s32 compareWith);
u8 EquipSet_RemoveSlot(u16 slot, s32 testOnly);
u8 EquipSet_DropUnfit(EquipIdx *out, EquipIdx *set, s32 flags);
u16 BoxMenu_EquipListInput(u16 *setIdx, u16 pad, s32 *ok, s32 flags);
u16 BoxMenu_ArrangeMenu(u16 pad);
u16 BoxMenu_SellMenu(u16 pad);
s32 ItemBox_SlotHasItem(u16 slot);
s32 EquipBox_TryRemove(u16 slot);
s32 BoxMenu_CanTakeSeveral(void);
void BoxMenu_InputAmount(u16 pad, u8 noLimit);
void Equip_SwapSlots(u16 first, u16 second);
u16 BoxMenu_Input(u16 pad);
u16 BoxMenu_SortPrompt(u16 pad, u8 kind);
s32 Equip_ArmorFits(EquipItem *item, u16 *slot);
s32 Box_YesNoInput(u16 *pad);
extern char gBoxEmptyText[]; /* an empty string */

#endif /* BOX_MENU_H */
