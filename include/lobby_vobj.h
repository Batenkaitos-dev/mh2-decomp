#ifndef LOBBY_VOBJ_H
#define LOBBY_VOBJ_H

#include "common.h"
#include "sprite.h"
#include "item.h"
#include "menu_task.h"
#include "stageobj.h"
#include "lobby_globals.h"
#include "lobby_menu.h"

/*
 * A village object with up to 15 members (lobby 0x6B8AC0-0x6B9B80): a
 * sub_main object (StageObj_Alloc(1), type 0x20, update VObj_Update) whose
 * work block keeps, per member, a kind (an index into gVObjKinds), a
 * timer, an angle (u16, 0x10000 a turn) and flags.
 */
typedef struct Vec3f {
    f32 x, y, z;
} Vec3f;

typedef struct VObjData {
    /* 0x000 */ Vec3f spots[15];   /* the spots the members stand on (VObj_Init: five per row from gVObjRowBases) */
    /* 0x0B4 */ s8 timer[16];       /* counts down (VObj_TickTimer); set from VKind.timer */
    /* 0x0C4 */ u16 ang[15];        /* 0x10000 a turn (VObj_GetPartAngle: in radians) */
    /* 0x0E2 */ u16 heading[16];    /* per member, facing the centre (+0x8000); [15] the spin's angle */
    /* 0x102 */ u8 flag[15];
    /* 0x111 */ u8 unk111[15];
    /* 0x120 */ s16 unk120[16];
    /* 0x140 */ Vec3f pos;
    /* 0x14C */ s32 spin;            /* the turn per frame (0x10000 a turn) added to ang and heading[15] */
    /* 0x150 */ u32 period;          /* frames between steps (VObj_Init: 6, 8, 12, 16) */
    /* 0x154 */ u8 unk154;
    /* 0x155 */ u8 unk155;
    /* 0x156 */ u8 delayLeft;       /* frames left (counted down; reset from delay) */
    /* 0x157 */ s8 delay;           /* the frames between the members' steps (VObj_Spawn's f) */
    /* 0x158 */ u8 finished;        /* 1 when done (VObj_IsFinished, VObj_IsCleared) */
    /* 0x159 */ u8 unk159[3];
    /* 0x15C */ u8 *kinds;          /* a kind (gVObjKinds index) per member */
    /* 0x160 */ u8 *spotIdx;       /* a spot (index into spots) per member (VObj_Spawn) */
    /* 0x164 */ s16 slot[16];        /* a draw slot per member */
    /* 0x184 */ DrawSlot *parts[15];
} VObjData;
typedef char VObjData_size_check[(sizeof(VObjData) == 0x1C0) ? 1 : -1];

/* A village object is a sub_main stage object (include/stageobj.h): its
 * buf is the VObjData of its members. */
typedef StageObj VObj;

/* A member kind (gVObjKinds, 8 bytes each). */
typedef struct VKind {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
    /* 0x6 */ u8 timer;
    /* 0x7 */ u8 unk7;
} VKind;
typedef char VKind_size_check[(sizeof(VKind) == 0x8) ? 1 : -1];

extern VKind gVObjKinds[];

/*
 * The pickaxe menu of the village mine (lobby 0x6B59E0-0x6B6120): the
 * pickaxes held (items 0x96-0x98), a cursor, and a toggle (button 0x200).
 */
typedef struct PickMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ u16 items[6];
    /* 0x14 */ s16 cursor;
    /* 0x16 */ s16 count;
    /* 0x18 */ u8 toggle;
} PickMenu;




/*
 * The hunting records (lobby 0x6B0570-0x6B5300, in the menu work area
 * gMenuWork): four categories (gRecordCats: a name, a list of 12-byte
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
    /* 0x010 */ u32 itemBits[0x20]; /* items obtained (gSave.itemBits, a member's from Records_RecvMsg2C; ItemBits_Test) */
    /* 0x090 */ u32 titleBits[8];   /* a bit per gHunterTitles entry (gSave.unk3660, a member's from Records_RecvMsg2D; Bits32_Test, Records_TallyTitles) */
    /* 0x0B0 */ u16 sizeMax[0x80];  /* per monster (gSave.sizeMax; a member's from Records_RecvMsg2E kind 0) */
    /* 0x1B0 */ u16 sizeMin[0x80];  /* per monster (gSave.sizeMin; Records_RecvMsg2E kind 1) */
    /* 0x2B0 */ u16 hunts[0x80];    /* per monster: gSave.hunted + captured (Records_RecvMsg2E kind 2) */
    /* 0x3B0 */ u16 ids[8];         /* the rows of the page shown (monster and item lists) */
    /* 0x3C0 */ s16 kind[9];        /* the menu's entries (Records_Open: up to 9) */
    /* 0x3D2 */ u8 equipBits[0x282]; /* the equipment list's flags packed (Bits_Pack of gSave's bladeFlags, gunFlags, armorFlags; a member's from Records_RecvPage); Records_EquipInput clears 0x280 bytes */
    /* 0x654 */ void *rows[10];     /* the rows of the page shown (category lists) */
    /* 0x67C */ EquipItem items[8];
    /* 0x6DC */ s32 tally[9][2];    /* per item category: seen, total (Records_TallyTitles, Records_TallyItems) */
    /* 0x724 */ s32 equipTally[10]; /* Records_TallyWeapons/Armor, or a member's (TownMsg_Send2B) */
    /* 0x74C */ char name[0xA];      /* Records_InviteInput: the text entered (Chat_OpenKbd) */
    /* 0x756 */ s16 rowCount;
    /* 0x758 */ s16 page;
    /* 0x75A */ s16 pages;
    /* 0x75C */ s16 subCursor;      /* a left-right cursor: the equipment level (below levelCount), a decoration's page */
    /* 0x75E */ s16 levelCount;     /* Equip_LevelMark's result (Records_EquipListInput) */
    /* 0x760 */ s16 genre;          /* the equipment genre (gEquipGroupTexts; Records_EquipInput) */
    /* 0x762 */ s16 genreCount;     /* the genres (5) */
    /* 0x764 */ s16 menuSel;        /* the menu's cursor: an index into kind (Records_LoadInput, Records_DrawMenu) */
    /* 0x766 */ s16 sel;            /* the category */
    /* 0x768 */ s16 cursor;
    /* 0x76A */ s16 menuCount;      /* the menu's entries */
    /* 0x76C */ s16 timer;           /* Records_LoadInput: frames left for a member's records */
    /* 0x76E */ u8 equipType;       /* the EquipItem type listed (Records_LoadEquipPage): 6 blades, 7 guns, 2-5 and 0 the armour genres (Records_EquipInput) */
    /* 0x76F */ s8 toggle;          /* button 0x200 in the monster list */
    /* 0x770 */ s8 prevSel;         /* Records_InviteInput: sel before the confirm */
    /* 0x771 */ u8 reqKind;         /* Records_LoadInput: the kind requested */
    /* 0x772 */ s8 mode;            /* Records_Update's state (switch) */
    /* 0x773 */ s8 step;            /* Records_ItemsInput: 0 the categories, 1 the item list */
    /* 0x774 */ s8 unk774;
} RecordMenu;
typedef char RecordMenu_size_check[(sizeof(RecordMenu) == 0x778) ? 1 : -1];

extern RecordCat gRecordCats[];

/* An animation of the village birds (D_007DD1C0, by kind): a mesh and a list
 * of (part, frames) pairs; a part of -1 ends it (frames: -1 stop, else the
 * pair to go on with). */
typedef struct VBirdAnim {
    s32 mesh;
    s8 *list;
} VBirdAnim;

/*
 * A bird of a village flock (lobby 0x6BD5E0-0x6BE640; probably birds: each
 * sits on one of 16 spots, flaps through 5 parts, and flies off 8000 units
 * away and 4000 up), 0x4C bytes.
 */
typedef struct VBird {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ Vec3f vel;
    /* 0x18 */ f32 accel;           /* added to vel.y while it is under 15 */
    /* 0x1C */ u8 unk1C[2];
    /* 0x1E */ s16 timer;           /* frames: (1-5) * 30 at a new spot */
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u16 ang;             /* 0x10000 a turn */
    /* 0x24 */ s32 color;           /* Gs_SetState(0x67, ...); -1: none */
    /* 0x28 */ s8 *list;            /* the animation (part, frames) pairs */
    /* 0x2C */ f32 climb;           /* the height gained flying off */
    /* 0x30 */ s8 mesh;             /* MdlMesh index */
    /* 0x31 */ s8 state;            /* 0 sitting, 1 arriving, 2 flying off */
    /* 0x32 */ s8 part;             /* the part shown */
    /* 0x33 */ s8 frames;           /* left for this part; -1: stopped */
    /* 0x34 */ s8 idx;              /* in the list */
    /* 0x35 */ s8 dir;              /* a direction: ang is dir << 13 (of 8) or << 12 (of 16) */
    /* 0x36 */ s8 step;             /* within the state */
    /* 0x37 */ s8 unk37;
    /* 0x38 */ s8 called;           /* the arrival sound played */
    /* 0x39 */ u8 unk39;
    /* 0x3A */ s8 anim;             /* D_007DD1C0 index */
    /* 0x3B */ u8 unk3B;
    /* 0x3C */ f32 unk3C;
    /* 0x40 */ Vec3f dest;          /* its spot, 20 above */
} VBird;
typedef char VBird_size_check[(sizeof(VBird) == 0x4C) ? 1 : -1];

/* A flock (the data of a VObj, set up by VFlock_Init): 6 birds on 6 of the
 * 16 spots of gVBirdSpots. */
typedef struct VFlock {
    /* 0x00 */ s8 spot[8];          /* the spot of each bird; -1: none */
    /* 0x08 */ f32 scale;
    /* 0x0C */ VBird bird[6];
} VFlock;

/* the lobby scene's object of LobbyFlock_Run (a sub_main object, Obj_Alloc(1); a view of the Obj):
 * 16 parts, each drawn by its own draw slot (LobbyActor_DrawPart) */
typedef struct LobbyPart {
    /* 0x00 */ f32 vel[3];          /* its speed, in its own axes */
    /* 0x0C */ u16 rot[3];          /* angles, 0x10000 a turn; random at the start (LobbyActor_DrawPart draws with rot[0], rot[1]) */
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
    /* 0x02 */ u8 kind;             /* Obj.kind: 0x18 (LobbyFlock_Spawn) */
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

#endif
