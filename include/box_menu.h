#ifndef BOX_MENU_H
#define BOX_MENU_H

#include "common.h"
#include "item.h"
#include "save.h"

/*
 * The item box menu's work (gBoxWork -> gBoxWorkMem, D_008B4380, 0x70 bytes;
 * BoxMenu_InitWork, BoxMenu_Update and the BoxMenu_* sub-menus of
 * unk_760C70.c, their draw functions in unk_7683D0.c and unk_7682D0.c).
 */
typedef struct ItemBoxMenu {
    /* 0x00 */ u8 unk0;             /* cleared by BoxMenu_Update; the draw tests it */
    /* 0x01 */ u8 cursorClass;      /* the equipment at the cursor: 0 armour, 1 a weapon (types 6, 7),
                                       2 a weapon of the other type than the equipped one */
    /* 0x02 */ u8 inputOn;          /* bit 0: the menu takes input (pad 0x8000 toggles it) */
    /* 0x03 */ u8 yesNo;            /* a yes/no answer (gYesNoTexts, YesNo_Toggle), 0xFF none */
    /* 0x04 */ union {
        s32 steps;                  /* the four step bytes, cleared together */
        struct {
            u8 state;               /* BoxMenu_Update: 0 the menu list, 1 in a sub-menu, 2 closing */
            u8 step;                /* the sub-menu's step */
            u8 sub;                 /* a step within the step */
            u8 sub2;
        };
    };
    /* 0x08 */ union {
        s16 menuKind;               /* menu and kind, cleared together */
        struct {
            u8 menu;                /* the menu list cursor: 0 store, 1 take, 2 equip, 3 arrange, 4 sell */
            u8 kind;                /* 0 items, 1 equipment */
        };
    };
    /* 0x0A */ u8 pick;
    /* 0x0B */ u8 unkB;
    /* 0x0C */ struct Player *player; /* the local player (gPlayers + localPlayer) */
    /* 0x10 */ EquipItem *equipped;  /* the worn item of the type (on the set screen, the set's), NULL none */
    /* 0x14 */ EquipItem *boxItem;   /* the equipment box item at the cursor (&gSave.box[slots[1]]), NULL none */
    /* 0x18 */ s32 price;           /* the selling price (BoxMenu_SellMenu) */
    /* 0x1C */ u16 slots[2];         /* the item box and equipment box slots, indexed by kind ([0] also as slot); in a union, MWCC adds 0x1C before a variable index: the s32 clear is a cast */
    /* 0x20 */ u16 held;            /* the slot held for a swap (BoxMenu_ArrangeMenu); BoxMenu_DrawGridCursor: % 100 */
    /* 0x22 */ u8 pouch;            /* pouch slot */
    /* 0x23 */ u8 atMax;            /* the amount is the most that can move */
    /* 0x24 */ s16 amount;          /* amount to move (BoxMenu_InputAmount) */
    /* 0x26 */ u16 pouchItem;       /* the pouch item being stored (BoxMenu_StoreFromPouch) */
    /* 0x28 */ u8 levelSel;         /* a cursor (Menu_CursorLeftRight) below levelCount */
    /* 0x29 */ u8 unk29;
    /* 0x2A */ u8 levelCount;       /* Equip_LevelMark's result: levelSel's limit */
    /* 0x2B */ u8 skillPage;        /* the skill view's page (Menu_CursorLeftRight; EquipDetail_Draw) */
    /* 0x2C */ u8 skillPages;       /* the skill view's pages (SkillView_Init, Equip_PreviewSkills) */
    /* 0x2D */ s8 msg;              /* a help line (gHelpLine), -1 none */
    /* 0x2E */ u8 help;             /* the help line's mode (BoxMenu_DrawHelp; ^= 0x81, bit 7 with gHelpShow) */
    /* 0x2F */ u8 blink;            /* a frame counter: 0xFF at open, (& 0x1F) < 0x14 shows the cursor */
    /* 0x30 */ u8 prompt;           /* a yes/no prompt is up (the sort: BoxMenu_SortPrompt; yesNo the answer) */
    /* 0x31 */ u8 canEquip;         /* the equipment at the cursor fits (gender, weapon class): the comparison
                                       marks it otherwise (flag 8); the help draws its skills (EquipDetail_Draw) */
    /* 0x32 */ u8 unk32;
    /* 0x33 */ u8 close;            /* BoxMenu_CloseStep's counter */
    /* 0x34 */ EquipSet cur;        /* the equipment set being edited */
    /* 0x54 */ u8 unk54;
    /* 0x55 */ u8 set;              /* the equipment set (SaveData.equipSets) */
    /* 0x56 */ u8 setEntries;       /* the equipment set menu's entries: 4, or 5 with symbol colours */
    /* 0x57 */ u8 part;
    /* 0x58 */ u8 unk58[2];
    /* 0x5A */ u8 page;             /* BoxList_MoveCursor's page */
    /* 0x5B */ s8 pageCount;
    /* 0x5C */ u8 row;              /* BoxList_MoveCursor's row */
    /* 0x5D */ u8 count;            /* entries in list */
    /* 0x5E */ u8 list[12];
    /* 0x6A */ u8 unk6A[6];
} ItemBoxMenu;
typedef char ItemBoxMenu_size_check[(sizeof(ItemBoxMenu) == 0x70) ? 1 : -1];

extern ItemBoxMenu *gBoxWork;
extern ItemBoxMenu gBoxWorkMem;

#endif /* BOX_MENU_H */
