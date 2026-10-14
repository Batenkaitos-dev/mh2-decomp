/*
 * 0xD64560-0xD64C80: a unit with its own .bss (0xD7BD80) and callback tables
 * in .data; called from the character select flow (plsel.c).
 */
#include "common.h"
extern u8 gHelpShow;
extern u8 gHelpPage;
extern s8 gHelpColumn;
#include "scene.h"
#include "m2c_macros.h"
#include "plsel.h"
#include "player.h"
#include "save.h"
#include "qsync.h"


s32 func_00D64B50(void);
s32 func_00D64B60(s32 arg0);
void Plsel_DrawPouchPage(void);

void ItemList_DrawPouch(s32, s32, s32);
s32 Game_IsOnline(void);
void HelpWindow_DrawCurrent(void);
void Plsel_DrawPlayerMenuList(void);
void Plsel_DrawMenuPage4(void);

/* What gPlselMenu points to. */
typedef struct D64Obj {
    u8 unk00[0x08];
    Player *unk08;           /* 0x08 */
    u8 unk0C[0x48 - 0x0C];
    u8 unk48;                   /* 0x48 */
    u8 unk49;
    u8 unk4A;                   /* 0x4A */
    u8 unk4B[0x50 - 0x4B];
    u8 unk50;                   /* 0x50 */
    u8 unk51;                   /* 0x51 */
} D64Obj;

void *Hud_Init(void);
void *memcpy(void *, const void *, u32);
void Player_RecomputeStats(Player *);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Text_SetColor(s16 color);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Text_Printf(const char *fmt, ...);
extern u16 D_896AF0[];
extern s32 *D_896A80[];

void Gfx_SetBlend(s32, s32);
struct ListWin;
void ListPanel_DrawDim(struct ListWin *win, char *title, s32 sel);
extern char *D_446F90[];

s32 Plsel_ChatUpdate(void);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);

extern s16 gHelpLine;

/* .bss (0xD7BD80): a pointer. The next unit's .bss starts 16-aligned. */
D64Obj *gPlselMenu;

/* .data (0xD76060-0xD76100) */
ListPanel gPlselMenuPage4Panel = {
#include "plsel/00D76060.inc"
};
void EquipStatus_Draw(void); /* sub_main */
s32 EquipStatus_Open(void); /* sub_main */
u16 EquipStatus_Update(u16 buttons); /* sub_main */
void PersonalStatus_Draw(void); /* sub_main */
s32 PersonalStatus_Open(void); /* sub_main */
u16 PersonalStatus_Update(u16 buttons); /* sub_main */
s32 PouchMenu_Open(void *m); /* sub_main */
u16 PouchMenu_Update(u16 buttons); /* sub_main */
void QuestMenu_DrawInfo(void); /* sub_main */
u16 QuestMenu_Update(u16 buttons); /* sub_main */
u32 gPlselMenuUpdates[8] = {
#include "plsel/00D76080.inc"
};
u32 gPlselMenuInputs[6] = {
    (u32)PouchMenu_Update, (u32)PersonalStatus_Update, (u32)EquipStatus_Update, (u32)QuestMenu_Update,
    (u32)func_00D64B60, 0x00000000,
};
u8 gPlselMenuHelpA[8] = {
#include "plsel/00D760B8.inc"
};
u8 gPlselMenuHelpB[16] = {
#include "plsel/00D760C0.inc"
};
u32 gPlselMenuDraws[4] = {
    (u32)Plsel_DrawPouchPage, (u32)PersonalStatus_Draw, (u32)EquipStatus_Draw, (u32)QuestMenu_DrawInfo,
};
extern char *D_00446F70[];
ListWin gPlselMenuListBox = {
#include "plsel/00D760E0.inc"
};

void Plsel_ClosePlayerMenu(void) {
    gPlselMenu->unk48 = 0;
    gScene.menuOpen = 0;
    gHelpShow = 0;
}

void Plsel_OpenPlayerMenu(u16 idx) {
    Player *pl;

    gPlselMenu = Hud_Init();
    gPlselMenu->unk48 = 0;
    gScene.menuOpen = 1;
    gPlselMenu->unk4A = 0;
    gHelpShow = 1;
    gHelpPage = 0;
    if (Game_IsOnline() == 1) {
        gHelpLine = gPlselMenuHelpB[gPlselMenu->unk4A];
    } else {
        gHelpLine = gPlselMenuHelpA[gPlselMenu->unk4A];
    }
    gPlselMenu->unk08 = &gPlayers[idx];
    pl = gPlselMenu->unk08;
    memcpy((u8 *)pl + 0x82C, gSave.name, 0x12);
    gScene.mealAttack = gScene.mealDefense = gScene.mealHealth = gScene.mealStamina = 0;
    Player_RecomputeStats(pl);
    Sound_PlaySeFlat(0, 0x11, 0);
}

void Plsel_PlayerMenuBack(void) {
    gPlselMenu->unk48 = 0;
    gHelpShow = 1;
    gHelpPage = 0;
    if (Game_IsOnline() == 1) {
        gHelpLine = gPlselMenuHelpB[gPlselMenu->unk4A];
    } else {
        gHelpLine = gPlselMenuHelpA[gPlselMenu->unk4A];
    }
}

void Plsel_PlayerMenuUpdate(u16 pad) {
    s32 r;
    u16 ret;

    if (Plsel_ChatUpdate() == 1) {
        return;
    }
    switch (gPlselMenu->unk48) {
    case 0:
        Menu_CursorUpDown(&gPlselMenu->unk4A, pad, 5);
        if (Game_IsOnline() == 1) {
            gHelpLine = gPlselMenuHelpB[gPlselMenu->unk4A];
        } else {
            gHelpLine = gPlselMenuHelpA[gPlselMenu->unk4A];
        }
        if (pad & 0x20) {
            r = ((s32 (*)(void))gPlselMenuUpdates[gPlselMenu->unk4A])();
            gPlselMenu->unk08 = &gPlayers[gQuestSync.pick];
            if (r == 0) {
                gPlselMenu->unk48++;
                gPlselMenu->unk50 = 0;
                Sound_PlaySeFlat(0, 0x13, 0);
            } else {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
        } else if (pad & 0x8040) {
            Plsel_ClosePlayerMenu();
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        break;
    case 1:
        if (gPlselMenu->unk4A == 0) {
            pad &= 0xBFDF;
        }
        ret = ((s32 (*)(s32))gPlselMenuInputs[gPlselMenu->unk4A])(pad);
        if (ret & 0x8000) {
            Plsel_ClosePlayerMenu();
            Sound_PlaySeFlat(0, 0x14, 0);
        } else if (ret & 0x40) {
            Plsel_PlayerMenuBack();
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        break;
    }
}

extern char *gFieldMenuTitle[];
void Plsel_DrawPlayerMenuList(void) {
    ListWin b;

    Gfx_SetBlend(4, 5);
    b = gPlselMenuListBox;
    if (Game_IsOnline() == 1) {
        b.items = D_446F90;
    }
    ListPanel_DrawDim(&b, gFieldMenuTitle[0], gPlselMenu->unk4A);
}

void Plsel_DrawPlayerMenu(void) {
    D64Obj *o;

    o = gPlselMenu;
    switch (o->unk48) {
    case 0:
        Plsel_DrawPlayerMenuList();
        break;
    case 1:
        switch (o->unk4A) {
        case 0:
        case 1:
        case 2:
        case 3:
            ((void (*)(void))gPlselMenuDraws[o->unk4A])();
            break;
        case 4:
            Plsel_DrawMenuPage4();
            return;
        }
        break;
    }
    if (gHelpShow != 0) {
        HelpWindow_DrawCurrent();
    }
}

void Plsel_DrawPouchPage(void) {
    D64Obj *o;
    u16 v;

    gHelpShow = 1;
    gHelpPage = 1;
    o = gPlselMenu;
    v = (s16)o->unk08->pouch[o->unk51].item;
    gHelpColumn = 0;
    gHelpLine = v + 0x18;
    ItemList_DrawPouch(o->unk51, 0xA9182, 8);
}

s32 func_00D64B50(void) {
    return 0;
}

s32 func_00D64B60(s32 arg0) {
    return arg0;
}

extern s32 D_00446F80;
extern s32 D_00446FA0;
void Plsel_DrawMenuPage4(void) {
    s32 y;
    s32 *list;
    s32 i;
    u16 *p;
    u16 v;

    if (Game_IsOnline() == 0) {
        ListPanel_Draw(&gPlselMenuPage4Panel, 0, (char *) D_00446F80, -1);
    } else {
        ListPanel_Draw(&gPlselMenuPage4Panel, 0, (char *) D_00446FA0, -1);
    }
    i = 0;
    p = D_896AF0;
    v = *p;
    while (v != 0) {
        if (v == gQuest.stage) {
            break;
        }
        p++;
        v = *p;
        i++;
    }
    list = D_896A80[i];
    Text_SetColor(0);
    y = 0x56;
    Font_SetSpacing(0x12, 0x12);
    while (*list != 0) {
        Font_SetPos(0x148, y);
        Text_Printf((const char *)*list);
        list++;
        y += 0x14;
    }
}
