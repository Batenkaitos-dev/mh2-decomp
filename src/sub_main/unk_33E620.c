#include "common.h"
#include "window.h"
#include "chat_log.h"
#include "em.h"
#include "player.h"
#include "quest.h"
#include "sprite.h"
#include "scene.h"
#include "pad.h"
#include "item.h"
#include "prim.h"
#include "save.h"
#include "hud.h"
#include "syssave.h"
#include "sub_main/unk_33E620.h"   /* the texts this file uses (tools/content_gen.py) */

/* Built at optimization level 4: Hud_DrawRoundGauge, Map_DrawMain_Unk, Hud_DrawPouchItem,
 * Hud_DrawCompass and Hud_DrawCompassQuad match only at 4, Map_DrawZoomed only at 3 (a pair below),
 * the rest at both. The shared strings ("%2d" from 0x3426D0 to 0x34B610, "%s" from 0x34B610
 * to 0x352650) show that the file is one of the game's units. */
#pragma optimization_level 4

/* prototypes */
s32 Combine_IsKnown(s16 a, s16 b);
s32 Player_IsLocal(void *obj);
s32 Quest_IsVersus(void);
void Save_StorePouch(Player *p);
void Shake_Start(u8 id, u8 flags, f32 amp);
void Text_Draw(char *s);
void Text_SetSize(s32 size);
void Font_SetColor(s32 color);
void Font_SetPal(s32 a);
s32 Game_PauseState(void);
struct Recipe;
u8 Recipe_YieldBase(struct Recipe *r);
s32 Combine_IsKnown3(s32, s32, s32);
s32 Recipe_IsKnown(Recipe *r);
void Tex_Prepare(s32 a, s32 b);
void Hud_ResetMenuState(void);
void FieldMenu_DrawList(s8 flag);
void FieldMenu_Draw(void);
void InfoMenu_Reset(void);
void LocationInfo_Draw(void);
void ItemList_DrawPage(void);
void MonsterNote_DrawPage(void);
void QuestMenu_DrawPage(void);
void PhraseMenu_Draw(void);
s32 PouchDrop_Update(u16 buttons);
void Hud_DrawPlayerNames(void);
void Hud_DrawVersusMonsterBars(void);
void Hud_DrawMap(void);
void Hud_DrawPouchItem(void);
void Hud_DrawItemWindow(void);
void Hud_DrawPartyStatus(void);
void Hud_DrawClock(void);
void Hud_DrawWeaponBox(void);
void Hud_DrawCamEvent(void);
void Hud_DrawBowgun(void);
s32 Hud_GlowCond_Unk(void *);
void Hud_DrawGlow(s16, s16, s32, f32, f32);
void Hud_DrawBowgunAmmo(void);
void Pause_DrawWindow(void);
s32 ItemGet_Reserve(s32 arg0, u16 *arg1);
void Game_Pause(u8 arg0);
s32 Player_IsFree(Player *p);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
s32 MonsterNote_SetKnown(s32 arg0);
/* unit's own declaration: with the definition's s8 (s8 cur, s8 dir) (sub_main/unk_356380.c), MonsterList_Update: 36 of 118 words differ */
s8 MonsterNote_NextKnown(s32 cur, s32 dir);
void Chat_DrawHudLog(void);
void Chat_DrawLog(void);
void Gfx_SetBlend(s32 a, s32 b);
void Gfx_SetState63(s32 a);
void Tex_Bind(s32 a);
s32 Game_IsOnline(void);
s32 Quest_PhaseIsOver(s32 a);
s32 Quest_HasAdvice(void);
s32 Item_IsSpecial(u16 item);
s32 Gs_SetState(u8 reg, u32 v);
s32 Gs_GetState(u8 reg);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void PlaceInfo_Draw(void);
void MsgEvent_DrawPopup(void);
void BoxMenu_Draw(void);
s32 func_770310(void);
void GameMenu_Back(void);
void GameMenu_Draw(void);
void LobbyMember_DrawList(void);
void SaveOpt37F6_Open(void);
s32 SaveOpt37F6_Input(void); /* unit's own declaration: SaveOpt37F6_Input reads a0 = buttons (StatusMenu_Update's, left in a0 through FriendLimit_Update) */
s32 Lobby_LacksQuest(void);
s32 Lobby_HasQuest(void);
s32 EmBlangonga_InStateArmor(Em *em);
extern u8 gQuestMenuCursorMax[];
extern u8 gFieldMenuHelpLines[];
extern ListPanel gInfoMenuPanel;
extern char **gInfoMenuLabelSets[];
extern ListPanel gChatMenuPanel;
extern s32 gItemNames[];
extern s16 gItemGetWait;
extern u16 gItemGetSerial;
extern u8 gPhraseMenuCursor;
extern u8 gHelpShow;
extern u8 gHelpPage;
extern u16 gHelpLine;
extern u8 gPauseMenuReq;
extern u8 gPauseWinStep;
extern u8 gPauseWinFade;
extern u8 gChatInputMode;
u8 Shake_Flags(u8 id);
s32 Shake_Update(u8 id);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count);
f32 Math_Sin(f32 x);
f32 Math_Sqrtf(f32 x);
s32 Sprite_Draw(struct Sprite *s);
s32 sprintf(char *dst, char *fmt, ...);
void Text_PrintfColored(char *fmt, ...);
void Window_DrawText(void *p, s16 *pos, u8 *text);
void Sjis_Widen(char *src, char *dst);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
extern u16 gFrameTick;
extern QuestInfo *gQuestInfo;
extern u8 gChatHideNames;
extern s8 gHelpColumn;
struct ListWin;
void ListPanel_DrawDim(struct ListWin *win, char *title, s32 sel);
struct Window;
void Window_DrawTextB2(struct Window *w, u8 *text);
void Text_Printf(char *fmt, ...);
void *memcpy(void *d, void *s, u32 n);
/* end of prototypes */

typedef struct OptMenu {
    u8 pad0[0x4E];
    u8 cursor;          /* 0x4E */
    u8 pad4F[0x99 - 0x4F];
    u8 page;            /* 0x99: row of gOptionMenuRows */
    u8 count;           /* 0x9A */
    u8 opts[9];         /* 0x9B: the options' values */
} OptMenu;

typedef struct SpriteW {
    s16 x, y, w, h;
    u32 color;
    s32 uv0;
    s32 uv1;
} SpriteW;

typedef struct Vec3s {
    s16 x, y, z;
} Vec3s;

typedef struct Gauge {
    f32 x;              /* 0x00 */
    f32 len;            /* 0x04: the full length */
    s16 y;              /* 0x08 */
    s16 cur;            /* 0x0A */
    s16 max;            /* 0x0C */
    u8 unkE[2];
    u32 color;          /* 0x10 */
} Gauge;

typedef struct Line {
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    u32 color;
} Line;

typedef struct AlphaKey {
    s32 time;
    f32 value;
} AlphaKey;

typedef struct PosKey {
    s32 time;
    f32 x;
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
    u8 visible;         /* 0x01 */
    u8 alpha;           /* 0x02 */
    u8 start;           /* 0x03 */
    f32 x0;             /* 0x04 */
    f32 x1;             /* 0x08 */
    f32 y0;             /* 0x0C */
    f32 y1;             /* 0x10 */
    f32 bx;             /* 0x14 */
    s16 by;             /* 0x18 */
    u8 unk1A[2];
    FadeDef *def;       /* 0x1C */
} Fade;

typedef struct IconDef {
    s16 unk0;
    s16 w;              /* 0x2 */
    s16 h;              /* 0x4 */
    s16 x;              /* 0x6 */
    s32 uv0;            /* 0x8 */
    s32 uv1;            /* 0xC */
} IconDef;

typedef struct QuestListEntry {  /* the quest list gQuestListEntries (0x10 bytes each) */
    u8 unk0[0xC];
    char *name;         /* 0xC */
} QuestListEntry;

typedef struct Tri {
    s16 x0, y0;         /* 0x00 */
    s16 x1, y1;         /* 0x04 */
    s16 x2, y2;         /* 0x08 */
    u32 color;          /* 0x0C */
    s16 u0, v0;         /* 0x10 */
    s16 u1, v1;         /* 0x14 */
    s16 u2, v2;         /* 0x18 */
} Tri;

char *NetUser_Get(u8 id); /* unit's own declaration: the definition (lobby) is void *NetUser_Get(s32 arg0); the calls here match only with this one */
extern char gPlayerIds[][8];

/* unk_33E620's .rodata (0x46CFB8-0x46CFC8) */
#pragma rodata on
u32 D_0046CFB8[3] = {
#include "sub_main/0046CFB8.inc"
};
#pragma rodata off

void Hud_PrintPlayerName(Player *p)
{
    char *name = (char *)p->name;

    if (Game_IsOnline() == 1 && gChatHideNames != 0) {
        if (gScene.online == 0) {
            name = gPlayerIds[p->index];
        } else {
            name = NetUser_Get(p->index);
        }
    }
    Text_PrintfColored("%-8.8s", name);
}

s32 FieldMenu_Requested(Player *p, u16 btn)
{
    if (p->tradeMsgTimer != 0) {
        return 0;
    }
    if ((Quest_IsVersus() == 1) && (gQuest.inputLock > 0)) {
        return 0;
    }
    return (btn & 0xFFFF & 0x8000) != 0;
}

extern u16 D_006AE0B4;
extern u16 D_006AE0C8;
extern u8 gChatLines;
extern s32 gChatSendWait;
extern DrawSlot gHudDrawHooks[4];   /* the HUD draw hooks (unk_376C00.c) */
/* unit's own declaration: as the file's last declaration of it (u8 in pouch.c) */
s32 Pouch_Add(Player *p, u16 item, s16 n);
s32 Pouch_IsSlotUsable(Player *p, u16 slot);
u16 Menu_ReadDpad(u8 pad);
s32 func_00355E90(Player *p);
void Hud_UpdateMapPing(u16 buttons);
void Players_UpdatePresent(void);
u8 MsgEvent_PopupInput(u16 buttons); /* unit's own declaration: the definition (game) is u8 MsgEvent_PopupInput(s32 arg0); the calls here match only with this one */
s32 Field_ControlEnabled(void);
s32 Kbd_ActHotkeys(void);
s32 func_723BD0(Player *p);
void Chat_Update(u8 a);
void FieldMenu_Open(void);
void SupplyBox_Update(Player *p);
void Player_RunDeliveryMenu(Player *p);
void Quest_DeliveryMenu(Player *p);
void Hud_MapZoomInput(u16 arg0, u16 arg1);
void FieldMenu_Update(u16 buttons);
void Chat_SendInput(u16 buttons);
void Chat_UpdateWindow(u16 unused0);
void Pause_Update(void);
void Fade_UpdateAll(void);
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
s32 CamReq_Started(void);

/* The item HUD's per-frame update: returns the combine ingredients when the menu closes, eases
   the health shown, blinks the pouch and ammo slots, and runs the pouch/drop input. */
void ItemHud_Update(void)
{
    Player *p;
    u32 i;
    u16 btn;
    u16 pad;
    s32 r;

    if (gScene.eventRunning != 0) {
        return;
    }
    p = gHudState->player;
    if (Quest_PhaseIsOver(1) == 1) {
        if (gHudState->combineTaken != 0) {
            for (i = 0; i < 3; i++) {
                if (gHudState->combineItems[i] != 0xFFFF) {
                    Pouch_Add(p, gHudState->combineItems[i], 1);
                }
            }
            gHudState->combineTaken = 0;
        }
        gHudState->mode = 0;
        gHudState->supply = 0;
        gHudState->dropState = 0;
        gHudState->menuPhase = 0;
        gScene.menuOpen = 0;
        gChatInputMode = 0;
        return;
    }
    btn = D_006AE0B4 | (D_006AE0C8 & 0x3C00);
    pad = gPadState[0].held;
    Menu_ReadDpad(0);
    if (gHudState->camReqStarted != 0) {
        gHudState->dpadRepeat = 0;
        btn &= 0xBFFF;
        pad &= 0xBFFF;
    }
    switch (gHudState->phase) {
    case 0:
        gHudState->phase++;
        gHudState->unk00 = 1;
        gHudState->hp = p->hp;
        gHudState->hpRecoverable = p->hpRecoverable;
    case 1:
        if (gHudState->hp != p->hp) {
            if (gHudState->hp < p->hp) {
                gHudState->hp++;
            } else {
                gHudState->hp--;
            }
        }
        gHudState->hpRecoverable = p->hpRecoverable;
        gHudState->pouchBarShown = 0;
        if (gHudState->pouchCursor != p->pouchCursor) {
            gHudState->pouchCursor = p->pouchCursor;
            if (Pouch_IsSlotUsable(p, p->pouchCursor) == 1) {
                gHudState->blink60 = 2;
                if (p->pouchHudFlags & 4) {
                    gHudState->dir61 = -1;
                } else {
                    gHudState->dir61 = 1;
                }
            }
        }
        if (p->isGunner == 1) {
            if (gHudState->ammoSlot != p->ammoSlot) {
                gHudState->ammoSlot = p->ammoSlot;
                if (p->ammoSlot != 0xFF) {
                    gHudState->blink64 = 2;
                    if (p->pouchHudFlags & 0x10) {
                        gHudState->dir65 = -1;
                    } else {
                        gHudState->dir65 = 1;
                    }
                }
            }
        }
        gHudState->camEventStart = 0;
        if (func_00355E90(p) == 1) {
            if (gHudState->camEventFade >= 4) {
                gHudState->camEventStart = 1;
            }
            if (gHudState->camEventFade != 0) {
                gHudState->camEventFade--;
            }
        } else {
            gHudState->camEventFade = 4;
        }
        gHudState->hidden = 0;
        if (p->itemSelect == 0) {
            gHudState->hidden = gHudState->cockpitOff;
        }
        if (gHudState->glowTimer != 0) {
            gHudState->glowTimer--;
        }
        Hud_UpdateMapPing(pad);
        if (gHudState->supplyMsgTimer > 0) {
            gHudState->supplyMsgTimer--;
        }
        if (gHudState->supplyMoveTimer > 0) {
            gHudState->supplyMoveTimer--;
        }
        if (Game_IsOnline() == 1) {
            Players_UpdatePresent();
            gChatLines = 0;
            if (gChatSendWait <= 0) {
                gChatSendWait = 0;
            } else {
                gChatSendWait--;
            }
        }
        if ((gHudState->popupOpen = MsgEvent_PopupInput(btn)) == 0) {
            switch (gChatInputMode) {
            case 0:
                if (Game_PauseState() == 0 && p->itemSelect == 0 && p->tradeMsgTimer == 0 && Field_ControlEnabled() == 1) {
                    r = Kbd_ActHotkeys();
                    if ((btn & 0x100) || r == 1) {
                        if (func_723BD0(p) != 0) {
                            Sound_PlaySeFlat(0, 0x15, 0);
                        } else {
                            Chat_Update(r != 1);
                            btn = pad = gHudState->dpadRepeat = 0;
                        }
                    }
                }
                if (PouchDrop_Update(btn | gHudState->dpadRepeat) == 0) {
                    switch (gHudState->mode) {
                    case 0:
                        if (gChatInputMode == 0) {
                            if (FieldMenu_Requested(p, btn) == 1) {
                                FieldMenu_Open();
                                Sound_PlaySeFlat(0, 0x11, 0);
                            } else {
                                if (gHudState->supply != 0) {
                                    SupplyBox_Update(p);
                                } else if (gHudState->deliveryMenu != 0) {
                                    Player_RunDeliveryMenu(p);
                                } else if (gHudState->deliveryOpen != 0) {
                                    Quest_DeliveryMenu(p);
                                } else {
                                    Hud_MapZoomInput(btn, pad);
                                }
                                gHudState->supply = p->supplyBoxOpen;
                                gHudState->deliveryMenu = p->deliveryMenu;
                                gHudState->deliveryOpen = p->deliveryOpen;
                            }
                        }
                        break;
                    case 1:
                        FieldMenu_Update(btn | gHudState->dpadRepeat);
                        break;
                    }
                }
                Fade_UpdateAll();
                break;
            case 1:
                Chat_SendInput(btn);
                Hud_MapZoomInput(btn & 0xBFFF, pad & 0xBFFF);
                if (PouchDrop_Update(0) == 0) {
                    switch (gHudState->mode) {
                    case 1:
                        FieldMenu_Update(0);
                        break;
                    }
                }
                break;
            }
        }
        break;
    }
    Chat_UpdateWindow(btn);
    Pause_Update();
    OT_AddAt(gDrawListText6, &gHudDrawHooks[0], 0, 1);
    OT_AddAt(gDrawListText0b, &gHudDrawHooks[1], 0, 1);
    OT_AddAt(gDrawListText1, &gHudDrawHooks[2], 0, 1);
    OT_AddAt(gDrawListText2, &gHudDrawHooks[3], 0, 1);
    gHudState->camReqStarted = CamReq_Started();
}

extern u8 gChatLines;
extern s32 gChatSendWait;
extern s8 gNoticeFlags;
void Chat_InitMode(void);
u16 GameMenu_GetPad(void);
u16 Menu_ReadDpad(u8 pad);
void Players_UpdatePresent(void);
s32 Hud_FieldInputAllowed_Unk(void);
s32 Kbd_ActHotkeys(void);
void Chat_Update(u8 a);
s32 PouchDrop_UpdateSave(u16 buttons);
s32 GameMenu_Update(u16 pad);
void Chat_SendInput(u16 buttons);
void Chat_UpdateWindowsOnline(u16 buttons);
void Pause_Update(void);
void Fade_UpdateAll(void);
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);

/* The HUD's update: the pad buttons go to the menus (PouchDrop_UpdateSave) and the network (offline
 * none), then the chat and the draw lists. */
void Hud_Update(void)
{
    u16 buttons;
    s32 r;

    Chat_InitMode();
    buttons = GameMenu_GetPad();
    Menu_ReadDpad(0);
    switch (gHudState->phase) {
    case 0:
        (gHudState->phase)++;
        gHudState->unk00 = 1;
    case 1:
        if (Game_IsOnline() == 1) {
            Players_UpdatePresent();
            gChatLog.win.flags = 0;
            gChatLines = 0;
            if (gChatSendWait <= 0) {
                gChatSendWait = 0;
            } else {
                gChatSendWait--;
            }
        }
        switch (gChatInputMode) {
        case 0:
            if (gNoticeFlags == 0) {
                if (Hud_FieldInputAllowed_Unk() == 1) {
                    r = Kbd_ActHotkeys();
                    if ((buttons & 0x100) || r == 1) {
                        Chat_Update(r != 1);
                        buttons = gHudState->dpadRepeat = 0;
                    }
                }
                if (PouchDrop_UpdateSave((u16)(buttons | gHudState->dpadRepeat)) == 0) {
                    GameMenu_Update(buttons | gHudState->dpadRepeat);
                }
                Fade_UpdateAll();
            }
            break;
        case 1:
            Chat_SendInput(buttons);
            if (PouchDrop_UpdateSave(0) == 0) {
                GameMenu_Update(0);
            }
            break;
        }
    }
    Chat_UpdateWindowsOnline(buttons);
    Pause_Update();
    OT_AddAt(gDrawListText0b, &gHudDrawHooks[1], 0, 1);
    OT_AddAt(gDrawListText1, &gHudDrawHooks[2], 0, 1);
    OT_AddAt(gDrawListText2, &gHudDrawHooks[3], 0, 1);
}

void FieldMenu_Open(void)
{
    gScene.menuOpen = 1;
    gHudState->mode = 1;
    gHudState->menuPhase = 0;
    gHelpShow = 1;
    gHelpPage = 0;
    gHelpLine = (u16) gHudState->fieldMenuCursor;
    if (Game_IsOnline() != 1) {
        gHudState->fieldMenuCount = 8U;
        if (Quest_HasAdvice() == 1) {
            gHudState->fieldMenuCount = (u8) (gHudState->fieldMenuCount + 1);
        }
    } else {
        gHudState->fieldMenuCount = 7U;
    }
    Shake_Start(0U, 0U, 180.0f);
    gHudState->fadeActive = 0;
    Hud_ResetMenuState();
}

void Shake_Stop(void);
void FieldMenu_Close(void);
void FieldMenu_SetHelpLine(void);
void FieldMenu_ToggleCockpit(void);
void FieldMenu_Exit(void);
extern s32 (*gFieldMenuDecideOnline[])(void);
extern s32 (*gFieldMenuDecide[])(void);
extern u16 (*gFieldMenuInputOnline[])(s32);
extern u16 (*gFieldMenuInput[])(s32);

/* The menu top page's input: state 0 picks an entry (gFieldMenuDecide / gFieldMenuDecideOnline start it), state 1
 * runs the entry's input (gFieldMenuInput / gFieldMenuInputOnline), state 2 waits for the window to close. */
void FieldMenu_Update(u16 buttons)
{
    s32 r;
    u16 b;
    switch (gHudState->menuPhase) {
    case 0:
        if (Shake_Update(0) == 1) {
            buttons &= 0x3C40;
        }
        Menu_CursorUpDown(&gHudState->fieldMenuCursor, buttons, gHudState->fieldMenuCount);
        FieldMenu_SetHelpLine();
        if (buttons & 0x200) {
            FieldMenu_ToggleCockpit();
        }
        if (buttons & 0x20) {
            Shake_Stop();
            if (Game_IsOnline() == 1) {
                r = gFieldMenuDecideOnline[gHudState->fieldMenuCursor]();
            } else {
                r = gFieldMenuDecide[gHudState->fieldMenuCursor]();
            }
            if (r == 0) {
                (gHudState->menuPhase)++;
                gHudState->state = 0;
                Sound_PlaySeFlat(0, 0x13, 0);
            } else {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
        } else if (buttons & 0x8040) {
            FieldMenu_Close();
        }
        break;
    case 1:
        if (Game_IsOnline() == 1) {
            b = gFieldMenuInputOnline[gHudState->fieldMenuCursor](buttons);
        } else {
            b = gFieldMenuInput[gHudState->fieldMenuCursor](buttons);
        }
        if (b & 0x8000) {
            FieldMenu_Exit();
        } else if (b & 0x40) {
            gHudState->menuPhase = 0;
            gHelpShow = 1;
            gHelpPage = 0;
            FieldMenu_SetHelpLine();
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        break;
    case 2:
        if (Shake_Update(0) == 0) {
            FieldMenu_Exit();
        }
        break;
    }
}

void FieldMenu_Close(void)
{
    if (gScene.online != 0) {
        GameMenu_Back();
        return;
    }
    gHudState->menuPhase = 2;
    gHelpShow = 0;
    Shake_Start(0U, 0x80U, 180.0f);
    Sound_PlaySeFlat(0, 0x14, 0);
}

void FieldMenu_SetHelpLine(void)
{
    if (Game_IsOnline() == 1) {
        gHelpLine = (u16) gFieldMenuHelpLines[gHudState->fieldMenuCursor];
        return;
    }
    gHelpLine = (u16) gHudState->fieldMenuCursor;
}

void FieldMenu_ToggleCockpit(void)
{
    gHudState->cockpitOff ^= 1;
    Sound_PlaySeFlat(0, 0x13, 0);
}

extern ListPanel gFieldMenuPanel;
extern char *gFieldMenuItemsOffline[][9];
extern char *gFieldMenuItemsOnline[][7];
extern char *gFieldMenuTitle[];
void GameMenu_DrawList(s8 dim);
void *Shake_GetOffset(u8 id);
s32 Hud_MenuAllowed_Unk(void);
s32 ItemGet_IsReady(Player *p);
void FieldMenu_DrawCockpit(s16 row, s16 *pos);

void FieldMenu_DrawList(s8 flag)
{
    s16 *pos;
    s16 k;

    if (gScene.online != 0) {
        GameMenu_DrawList(flag);
        return;
    }
    Gfx_SetBlend(4, 5);
    pos = Shake_GetOffset(0);
    k = 0;
    if (Hud_MenuAllowed_Unk() != 1 || ItemGet_IsReady(NULL) != 1) {
        k = 1;
    }
    if (Game_IsOnline() != 1) {
        if (Game_PauseState() != 0) {
            k = 4;
        } else if (Quest_PhaseIsOver(0) == 1) {
            k += 2;
        }
        gFieldMenuPanel.items = gFieldMenuItemsOffline[k];
    } else {
        gFieldMenuPanel.items = gFieldMenuItemsOnline[k];
    }
    if (flag == 0) {
        gFieldMenuPanel.cursorColor = 0xA9182;
        FieldMenu_DrawCockpit(gHudState->fieldMenuCount, pos);
    } else {
        gFieldMenuPanel.cursorColor = 0x808080;
    }
    gFieldMenuPanel.lines = gHudState->fieldMenuCount;
    ListPanel_Draw(&gFieldMenuPanel, pos, gFieldMenuTitle[0], gHudState->fieldMenuCursor);
}

extern void (*gFieldMenuDrawOnline[])(void);
extern void (*gFieldMenuDraw[])(void);
s32 Shake_Value(u8 id);
void HelpWindow_DrawCurrent(void);

void FieldMenu_Draw(void)
{
    switch (gHudState->menuPhase) {
    case 0:
        FieldMenu_DrawList(0);
        break;
    case 1:
        if (Game_IsOnline() == 1) {
            gFieldMenuDrawOnline[gHudState->fieldMenuCursor]();
        } else {
            gFieldMenuDraw[gHudState->fieldMenuCursor]();
        }
        break;
    case 2:
        FieldMenu_DrawList(1);
        break;
    }
    if (Shake_Value(0) == 0 && gHelpShow != 0) {
        HelpWindow_DrawCurrent();
    }
}

extern Sprite gFieldMenuCockpitBox;
extern Icon gFieldMenuCockpitIcon;
extern char *gCockpitToggleTexts[];
void Draw_Icons(Icon *icons, u8 count);

void FieldMenu_DrawCockpit(s16 row, s16 *pos)
{
    s16 y = (row + 1) * 22 + 0x50;
    gFieldMenuCockpitBox.y = y;
    Window_DrawText(&gFieldMenuCockpitBox, (s16 *)pos, (u8 *)gCockpitToggleTexts[gHudState->cockpitOff]);
    gFieldMenuCockpitIcon.x = pos[0] + 0x1D3;
    gFieldMenuCockpitIcon.y = y - 1;
    Draw_Icons(&gFieldMenuCockpitIcon, 1);
}

s32 StatusMenu_Open(void *m)
{
    gHudState->menuStep = 0;
    gHudState->listCursor = 0;
    if (gScene.online == 0) {
        gHudState->cursorMax = 2;
    } else {
        gHudState->cursorMax = Game_IsOnline() == 1 ? 5 : 3;
    }
    gHelpLine = 0;
    gHelpShow = 1;
    gHelpPage = 9;
    return 0;
}

s32 PersonalStatus_Open(void);
u16 PersonalStatus_Update(u16 buttons);
s32 EquipStatus_Open(void);
u16 EquipStatus_Update(u16 buttons);
void CommentEdit_Open(void);
u16 Chat_InputLine(u16 buttons);
void TextColor_Open(void);
u16 TextColor_Input(u16 buttons); /* unit's own declaration: the definition (lobby) is s32 TextColor_Input(s32 arg0); the calls here match only with this one */
void FriendLimit_Open(void);
s32 FriendLimit_Update(void);

/* The item page's input (StatusMenu_Open sets it up): the entry (cursor +0x4B of +0xB5) and then
 * its own handler. */
u16 StatusMenu_Update(u16 buttons)
{
    switch (gHudState->menuStep) {
    case 0:
        if (buttons & 0x8000) {
            FieldMenu_Close();
            return 0;
        }
        Menu_CursorUpDown(&gHudState->listCursor, buttons, gHudState->cursorMax);
        gHelpLine = gHudState->listCursor;
        if (buttons & 0x20) {
            switch (gHudState->listCursor) {
            case 0:
                PersonalStatus_Open();
                break;
            case 1:
                EquipStatus_Open();
                break;
            case 2:
                CommentEdit_Open();
                break;
            case 3:
                TextColor_Open();
                break;
            case 4:
                FriendLimit_Open();
                break;
            }
            (gHudState->menuStep)++;
            Sound_PlaySeFlat(0, 0x13, 0);
        }
        break;
    case 1:
        switch (gHudState->listCursor) {
        case 0:
            buttons = PersonalStatus_Update(buttons);
            break;
        case 1:
            buttons = EquipStatus_Update(buttons);
            break;
        case 2:
            buttons = Chat_InputLine(buttons);
            break;
        case 3:
            buttons = TextColor_Input(buttons);
            break;
        case 4:
            buttons = FriendLimit_Update();
            break;
        }
        if (buttons & 0x8000) {
            FieldMenu_Close();
        } else if (buttons & 0x40) {
            gHudState->menuStep = 0;
            gHelpShow = 1;
            gHelpPage = 9;
            gHelpLine = gHudState->listCursor;
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        buttons = 0;
        break;
    }
    return buttons;
}

void FriendLimit_Open(void)
{
    SaveOpt37F6_Open();
}

s32 FriendLimit_Update(void)
{
    return SaveOpt37F6_Input();
}

void PersonalStatus_Draw(void);
void EquipStatus_Draw(void);
void CommentEdit_Draw(void);
void SaveOpt37F6_Draw(void);
extern ListPanel gStatusMenuPanel;

void StatusMenu_Draw(void)
{
    u8 listCursor;
    u8 menuStep;

    menuStep = gHudState->menuStep;
    switch (menuStep) {
    case 0:
        FieldMenu_DrawList(1);
        gStatusMenuPanel.lines = gHudState->cursorMax;
        gStatusMenuPanel.cursorColor = 0xFF0A9182;
        ListPanel_Draw(&gStatusMenuPanel, NULL, NULL, gHudState->listCursor);
        break;
    case 1:
        listCursor = gHudState->listCursor;
        switch (listCursor) {
        case 0:
            PersonalStatus_Draw();
            break;
        case 1:
            EquipStatus_Draw();
            break;
        case 2:
            CommentEdit_Draw();
            break;
        case 3:
            break;
        case 4:
            FieldMenu_DrawList(1);
            gStatusMenuPanel.cursorColor = 0x808080;
            ListPanel_Draw(&gStatusMenuPanel, NULL, NULL, gHudState->listCursor);
            SaveOpt37F6_Draw();
            break;
        }
        break;
    }
}

extern u8 gInfoMenuCounts[];
extern u8 gInfoMenuEntries[];

void InfoMenu_Reset(void)
{

    gHudState->menuStep = 0;
    if (gScene.online == 0) {
        gHudState->infoSet = 0U;
    } else if (Game_IsOnline() != 1) {
        gHudState->infoSet = 1U;
    } else {
        gHudState->infoSet = gScene.area != 0x83 ? 2 : 3;
    }
    gHudState->cursorMax = (u8) gInfoMenuCounts[gHudState->infoSet];
    gHudState->infoEntry = (u8) *(gHudState->listCursor + (gInfoMenuEntries + (gHudState->infoSet * 4)));
    gHelpShow = 1;
    gHelpPage = 0xA;
    gHelpLine = (u16) gHudState->infoEntry;
}

s32 InfoMenu_Open(void *m)
{
    gHudState->listCursor = 0;
    InfoMenu_Reset();
    gHudState->unk94 = 0;
    gHudState->combineRecipe = NULL;
    gHudState->noteMonster = 0;
    return 0;
}

s32 PlaceInfo_Open(void);
s32 LobbyMember_OpenList(void);
u16 LocationInfo_Update(u16 buttons);
u16 CombineList_Update(u16 buttons);
u16 MonsterList_Update(u16 buttons);
u16 QuestMenu_Update(u16 buttons);
u16 LobbyMember_ListInput(u16 pad);
s32 CombineList_Open(void);
s32 MonsterList_Open(void);
s32 QuestMenu_Open(); /* unit's own declaration: QuestMenu_Open ignores a0 (m), which holds &gHudState->infoEntry at InfoMenu_Update's call */
void FieldMenu_Close(void);

/* The first menu page's input: the entry (item box, equipment, ...) and then its own handler. */
u16 InfoMenu_Update(u16 buttons)
{
    switch (gHudState->menuStep) {
    case 0:
        if (buttons & 0x8000) {
            FieldMenu_Close();
            return 0;
        }
        Menu_CursorUpDown(&gHudState->listCursor, buttons, gHudState->cursorMax);
        gHudState->infoEntry = *(gHudState->listCursor + (gInfoMenuEntries + (gHudState->infoSet * 4)));
        gHelpLine = gHudState->infoEntry;
        if (buttons & 0x20) {
            switch (gHudState->infoEntry) {
            case 0:
                PlaceInfo_Open();
                break;
            case 1:
                CombineList_Open();
                break;
            case 2:
                MonsterList_Open();
                break;
            case 3:
                QuestMenu_Open();
                break;
            case 4:
                LobbyMember_OpenList();
                break;
            }
            (gHudState->menuStep)++;
            Sound_PlaySeFlat(0, 0x13, 0);
        }
        break;
    case 1:
        switch (gHudState->infoEntry) {
        case 0:
            buttons = LocationInfo_Update(buttons);
            break;
        case 1:
            buttons = CombineList_Update(buttons);
            break;
        case 2:
            buttons = MonsterList_Update(buttons);
            break;
        case 3:
            buttons = QuestMenu_Update(buttons);
            break;
        case 4:
            buttons = LobbyMember_ListInput(buttons);
            break;
        }
        if (buttons & 0x8000) {
            return 0x8000;
        }
        if (buttons & 0x40) {
            InfoMenu_Reset();
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        buttons = 0;
        break;
    }
    return buttons;
}

void InfoMenu_Draw(void)
{
    u8 infoEntry;
    u8 menuStep;

    menuStep = gHudState->menuStep;
    switch (menuStep) {
    case 0:
        FieldMenu_DrawList(1);
        gInfoMenuPanel.lines = gHudState->cursorMax;
        gInfoMenuPanel.items = gInfoMenuLabelSets[gHudState->infoSet];
        ListPanel_Draw(&gInfoMenuPanel, NULL, NULL, gHudState->listCursor);
        break;
    case 1:
        infoEntry = gHudState->infoEntry;
        switch (infoEntry) {
        case 0:
            LocationInfo_Draw();
            break;
        case 1:
            ItemList_DrawPage();
            break;
        case 2:
            MonsterNote_DrawPage();
            break;
        case 3:
            QuestMenu_DrawPage();
            break;
        case 4:
            LobbyMember_DrawList();
        }
        break;
    }
}

u16 PlaceInfo_Input(void); /* unit's own declaration: PlaceInfo_Input (s32 arg) reads a0 = buttons (LocationInfo_Update's u16 argument, still in a0; passing it adds a mask) */

u16 LocationInfo_Update(u16 buttons)
{
    if (gScene.online == 0) {
        return buttons;
    }
    return PlaceInfo_Input();
}

void LocationInfo_Draw(void)
{
    if (gScene.online != 0) {
        PlaceInfo_Draw();
    }
}

struct Grid;
s32 CombineList_Setup(struct Grid *g, s16 *out, s32 v);

s32 CombineList_Open(void)
{
    gHudState->grid.index = 0;
    gHudState->grid.rows = 5;
    CombineList_Setup(&gHudState->grid, (s16 *)gHudState->listItems, 5);
    gHudState->set = 0;
    gHudState->sets[0].recipe = NULL;
    gHelpShow = 0;
    Shake_Start(8, 0, 333.0f);
    return 0;
}

void CombineList_MoveCursor(struct Grid *g, s16 *out, u16 buttons);
s32 Recipe_IsListedFor(s16 item, s32 a);
struct RecipeHit;
u8 Combine_ListRecipesFor(struct RecipeHit *out, s16 item, s32 flags);

u16 CombineList_Update(u16 buttons)
{
    s32 i;
    u16 prev;
    u16 item;

    if (Shake_Flags(8) & 0x80) {
        if (Shake_Update(8) == 0) {
            return 0x8000;
        }
        return 0;
    }
    if ((u16)buttons & 0x8000) {
        Shake_Start(8, 0x80, 333.0f);
        gHelpShow = 0;
        Sound_PlaySeFlat(0, 0x14, 0);
        return 0;
    }
    gHelpShow = 1;
    if (Shake_Update(8) == 1) {
        buttons &= 0xC3BF;
        gHelpShow = 0;
    }
    prev = gHudState->grid.index;
    CombineList_MoveCursor(&gHudState->grid, (s16 *)gHudState->listItems, buttons);
    for (i = 0; i < gHudState->grid.rows; i++) {
        if (Recipe_IsListedFor((s16)gHudState->listItems[i], 4) != 1) {
            gHudState->listItems[i] = 0;
        }
    }
    item = gHudState->listItems[gHudState->grid.row];
    if (item != 0) {
        gHelpPage = 1;
        gHelpLine = item + 0x18;
        if (prev != gHudState->grid.index) {
            gHudState->sets[0].recipe = NULL;
            buttons = 0;
            gHudState->set = 0;
        }
        if (gHudState->sets[0].recipe == NULL) {
            gHudState->setCount = Combine_ListRecipesFor(gHudState->sets, item, gHudState->grid.unk0);
        }
        if (gHudState->setCount > 1 && ((u16)buttons & 0x200)) {
            (gHudState->set)++;
            if (gHudState->set >= gHudState->setCount) {
                gHudState->set = 0;
            }
            Sound_PlaySeFlat(0, 9, 0);
        }
    } else {
        gHelpPage = 0;
        gHelpLine = 0xFFFF;
        gHudState->sets[0].recipe = NULL;
        gHudState->set = 0;
    }
    return buttons;
}

/* A set in the list work area: its record and up to four values printed with it. */
extern ListPanel gItemListPanel;
extern Frame gItemListFrame[];
extern char *gCombineListFmt[];
extern char *gCombineListNoText;
extern char *gItemListLineTexts[];
extern char *gUnknownNameText;
extern char *gRecipeMaterialsFmt[];
extern char *gRecipeUnknownFmt;
extern char *gRecipeSwitchHint[];
extern char *gRecipeMaterialFmts[];
extern char *gRecipeRateLines[];
extern char *gRecipeRateLabels[];
extern char *gRecipeYieldLabels[];
struct Frame;
void WindowFrame_Draw(struct Frame *f, s16 *off);
struct Pair;
s32 Shake_CopyOffset(u8 id, struct Pair *pos);
s32 sprintf(char *dst, char *fmt, ...);
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Menu_DrawFrame(void *b, s16 *pos);
void Tex_Prepare(s32 a, s32 b);
void Tex_Bind(s32 a);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(char *fmt, ...);
void Text_DrawLinesColored(s16 x, s16 y, char **names, s16 h);
s32 Recipe_IsKnown(Recipe *r);
void Font_SetColor(s32 color);
void Text_Draw(char *s);
void Text_Printf(char *fmt, ...);
u32 strlen(const void *s);
s32 Sprite_Draw(struct Sprite *s);
extern s32 gItemNames[];
extern s32 gItemColors[];

/* The paged item list: the page's items with their icons, then the selected set's record.
   The recipe rows reuse num for the row's y and y for the (dead) row counter: the game's
   registers and step order come from that reuse. */
void ItemList_DrawPage(void)
{
    ItemData *d;
    Sprite s;
    char buf[0x30];
    s16 pos[2];
    s32 i;
    u16 *it;
    s16 num;
    s16 y;
    char *name;
    u8 n;
    RecipeHit *set;
    Recipe *rec;
    s32 rows;
    s16 w;

    Shake_CopyOffset(8, (struct Pair *)pos);
    sprintf(buf, gCombineListFmt[0], gHudState->grid.col + 1, gHudState->grid.cols);
    ListPanel_Draw(&gItemListPanel, pos, buf, gHudState->grid.row + 1);
    Menu_DrawFrame(&gItemListPanel, pos);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    Font_SetPos(pos[0] + 0x13B, 0x52);
    Text_PrintfColored(gCombineListNoText);
    y = 0x66;
    it = gHudState->listItems;
    num = gHudState->grid.col * gHudState->grid.rows + 1;
    if (gHudState->grid.col < gHudState->grid.cols - 1) {
        n = gHudState->grid.rows;
    } else {
        n = gHudState->grid.lastRows;
    }
    s.x = pos[0] + 0x172;
    s.w = 0x17;
    s.h = 0x12;
    for (i = n; i > 0; i--) {
        d = &gItemData[*it];
        if (*it != 0) {
            name = (char *)gItemNames[*it];
            s.u0 = (((d->icon + 1) & 7) << 5) + 1;
            s.v0 = (((d->icon + 1) >> 3) << 5) + 1;
            s.u1 = s.u0 + 0x1E;
            s.v1 = s.v0 + 0x1E;
        } else {
            name = gUnknownNameText;
            *(s32 *)&s.u0 = 0x810041;
            *(s32 *)&s.u1 = 0x9F005F;
        }
        Font_SetPos(pos[0] + 0x13B, y);
        Text_PrintfColored(gItemListLineTexts[0], num, name);
        s.y = y;
        s.color = gItemColors[d->color];
        Sprite_Draw((struct Sprite *)&s);
        it++;
        num++;
        y += 0x14;
    }
    WindowFrame_Draw(&gItemListFrame[0], pos);
    WindowFrame_Draw(&gItemListFrame[1], pos);
    Text_DrawLinesColored((s16)(pos[0] + 0x216), 0xE4, gRecipeRateLines, 0x32);
    set = &gHudState->sets[gHudState->set];
    rec = set->recipe;
    if (rec != NULL) {
        Font_SetPos(pos[0] + 0x13B, 0xDA);
        if (Recipe_IsKnown(rec) == 1) {
            Text_PrintfColored(gRecipeMaterialsFmt[0], gHudState->set + 1);
            num = 0xF8;
            rows = !(rec->flags & 0x80) ? 2 : 3;
            for (y = 0; rows > 0; rows--, y++, num += 0x14) {
                Font_SetPos(pos[0] + 0x13B, num);
                Text_PrintfColored(gRecipeMaterialFmts[y], gItemNames[set->info[y]]);
            }
            Font_SetColor(0x802B7F2B);
            Font_SetPos(pos[0] + 0x228, 0xF8);
            Text_Draw(gRecipeRateLabels[rec->rateClass]);
            w = strlen(gRecipeYieldLabels[rec->yieldClass]) * 9 / 2;
            Font_SetPos(0x23A - w + pos[0], 0x12A);
            Text_Draw(gRecipeYieldLabels[rec->yieldClass]);
        } else {
            Text_PrintfColored(gRecipeUnknownFmt, gHudState->set + 1);
        }
        if (gHudState->setCount > 1) {
            Font_SetPos(pos[0] + 0x13B, 0x13E);
            Font_SetColor(0x807F7F7F);
            Text_Printf(gRecipeSwitchHint[0]);
        }
    }
}

s32 MonsterList_Open(void)
{
    gHudState->noteMonster = MonsterNote_NextKnown(0, 0);
    gHudState->noteHelpHide = -0x80;
    gHelpPage = 0xA;
    gHelpLine = 5;
    gHelpShow = 0;
    Shake_Start(0xDU, 0U, 333.0f);
    return 0;
}

s32 MonsterNote_AnyKnown(void);
s32 Save_TestEventFlag(s32 flag);

u16 MonsterList_Update(u16 buttons)
{
    s8 v;
    s8 *p;

    gHelpShow = 0;
    if (Shake_Flags(0xD) & 0x80) {
        if (Shake_Update(0xD) == 0) {
            return 0x8000;
        }
        return 0;
    }
    if (buttons & 0x8000) {
        Shake_Start(0xD, 0x80, 333.0f);
        Sound_PlaySeFlat(0, 0x14, 0);
        return 0;
    }
    if (Shake_Update(0xD) == 1) {
        buttons = 0;
    }
    if (MonsterNote_AnyKnown() == 1) {
        if (Save_TestEventFlag(0xC4) == 1 && (buttons & 0x200)) {
            if (gHudState->noteHelpHide & 0x80) {
                gHudState->noteHelpHide &= 0x7F;
            } else {
                gHudState->noteHelpHide |= 0x80;
            }
            Sound_PlaySeFlat(0, 0x11, 0);
            buttons = 0;
        }
        if (!(gHudState->noteHelpHide & 0x80)) {
            gHelpShow = 1;
        }
        if ((u16)buttons & 0xC00) {
            v = MonsterNote_NextKnown(gHudState->noteMonster, (buttons & 0x800) ? -1 : 1);
            p = &gHudState->noteMonster;
            if (*p != v) {
                *p = v;
                Sound_PlaySeFlat(0, 0x17, 0);
            }
        }
    } else {
        gHudState->noteMonster = -1;
    }
    return buttons;
}

s32 MonsterNote_CountKnown(void);
u8 MonsterNote_Number(u8 m);
s32 Em_HasRandomSize(s16 id);
f32 Monster_SizeFromPercent(u8 kind, u16 x);
u16 Record_GroupHunted(SaveData *save, u8 group);
u16 Record_GroupCaptured(SaveData *save, u8 group);
u16 Record_GroupLargest(SaveData *save, u8 group, u8 *out);
u16 Record_GroupSmallest(SaveData *save, u8 group, u8 *out);
f64 fptodp(f32 x);
s32 MonsterNote_Index(u16 id);
u8 MonsterNote_GetListFlags(u16 id);
s32 Str_Length(char *s);
void Text_PrintfStyled(s32 a, const char *fmt, ...);
void Menu_DrawEntryPicture(MonsterNoteDef *e, s16 x, s16 y);
s32 SizeRecord_DrawLarge(MonsterNoteDef *r, s16 x, s16 y, s32 v);
s32 SizeRecord_DrawSmall(MonsterNoteDef *r, s16 x, s16 y, s32 v);
extern ListPanel gMonsterNotePanels[];
extern Panel gMonsterNoteHelpBoxes[];
extern char *gMonsterListTitle[];
extern char *gMonsterCountFmt;
extern char *gMonsterSizeFmt;
extern char *gMonsterNoSizeText[];
extern char *gArenaBaitTitle;
extern char *gArenaBaitNoBookText[];
extern char *gArenaCannotRaiseFmt;
extern char *gMonsterNoteLabels[];
extern char *gMonsterRecordLabels;
extern char *gArenaBaitHint[];
extern char *gMonsterListHint;
extern char *gArenaBaitLabels[];
extern char *gMonsterNoteNumFmt;
extern char *gMonsterNoteValueFmt[];
extern char *gMonsterNames[];
extern char *gEmClassNames[];
extern char *gEmDescTexts[];
extern u16 *gMonsterNoteMarks[]; /* per material set: 8-byte entries, the item first */

/* The monster record page: the selected monster's hunt counts and sizes, or its materials. */
void MonsterNote_DrawPage(void)
{
    char small[0x18];
    char big[0x10];
    Sprite s;
    char line[0x40];
    s16 pos[2];
    u8 crown;
    char *title;
    u8 page;
    u32 n;
    MonsterNoteDef *e;
    char *help;
    s32 k;
    u16 *mat;
    s32 i;
    s16 y;
    u8 flags;
    s32 v;
    f32 f;
    s32 w;

    Shake_CopyOffset(0xD, (struct Pair *)pos);
    if (gHudState->noteHelpHide & 0x80) {
        title = gMonsterListTitle[0];
        page = 0;
    } else {
        title = gArenaBaitTitle;
        page = 1;
    }
    ListPanel_Draw(&gMonsterNotePanels[page], pos, title, -1);
    Font_SetSpacing(0x12, 0x12);
    n = MonsterNote_CountKnown();
    if (n != 0) {
        Font_SetColor(0x807F7F7F);
        Font_SetPos(pos[0] + 0x21B, 0x3E);
        sprintf(small, gMonsterNoteNumFmt, MonsterNote_Number((u8)gHudState->noteMonster), n);
        Text_PrintfColored(gMonsterNoteValueFmt[0], small);
    }
    if (n > 1) {
        Menu_DrawFrame(&gMonsterNotePanels[page], pos);
    }
    if (gHudState->noteMonster >= 0) {
        e = &gSizeRecordTbl[gHudState->noteMonster];
        if (gHudState->noteHelpHide & 0x80) {
            Font_SetColor(0x807F7F28);
            Font_SetPos(pos[0] + 0x11F, 0x52);
            Text_PrintfStyled(2, gMonsterNoteLabels[0]);
            Font_SetColor(0x807F7F7F);
            Font_SetPos(pos[0] + 0x155, 0x52);
            Text_PrintfColored(gMonsterNames[e->emType]);
            Font_SetPos(pos[0] + 0x155, 0x66);
            Text_PrintfColored(gEmClassNames[e->emClass]);
            Font_SetPos(pos[0] + 0x19D, 0x7A);
            n = Record_GroupHunted(&gSave, e->emType);
            n += Record_GroupCaptured(&gSave, e->emType);
            if (n > 9999) {
                n = 9999;
            }
            Text_PrintfColored(gMonsterCountFmt, n);
            Font_SetPos(pos[0] + 0x11F, 0xBF);
            Text_PrintfStyled(1, gEmDescTexts[e->note]);
            if (Em_HasRandomSize(e->emType) == 1) {
                Font_SetColor(0x807F7F28);
                Font_SetPos(pos[0] + 0x11F, 0x8E);
                Text_PrintfStyled(2, gMonsterRecordLabels);
                if (n != 0) {
                    v = (s16)Record_GroupLargest(&gSave, e->emType, &crown);
                    f = Monster_SizeFromPercent(crown, (u16)v);
                    SizeRecord_DrawLarge(e, pos[0] + 0x179, 0x8E, v);
                    sprintf(small, gMonsterSizeFmt, fptodp(f));
                    v = (s16)Record_GroupSmallest(&gSave, e->emType, &crown);
                    f = Monster_SizeFromPercent(crown, (u16)v);
                    SizeRecord_DrawSmall(e, pos[0] + 0x179, 0xA2, v);
                    sprintf(big, gMonsterSizeFmt, fptodp(f));
                } else {
                    sprintf(small, gMonsterNoSizeText[0]);
                    sprintf(big, gMonsterNoSizeText[0]);
                }
                Font_SetColor(0x807F7F7F);
                Font_SetPos(pos[0] + 0x19D, 0x8E);
                Text_PrintfColored(small);
                Font_SetPos(pos[0] + 0x19D, 0xA2);
                Text_PrintfColored(big);
            }
            Menu_DrawEntryPicture(e, pos[0] + 0x1EF, 0x66);
            help = gArenaBaitHint[0];
        } else {
            k = MonsterNote_Index(e->emType);
            if (k != 0xFF) {
                Font_SetColor(0x807F7F28);
                Font_SetPos(pos[0] + 0x11F, 0x52);
                Text_PrintfStyled(2, gArenaBaitLabels[0]);
                Font_SetColor(0x807F7F7F);
                Font_SetPos(pos[0] + 0x155, 0x52);
                Text_PrintfColored(gMonsterNames[e->emType]);
                mat = gMonsterNoteMarks[k];
                flags = MonsterNote_GetListFlags(e->emType);
                y = 0x7A;
                Font_SetColor(0x807F7F7F);
                Tex_Prepare(1, 0x144);
                Tex_Bind(0x144);
                s.w = 0x17;
                s.x = pos[0] + 0x167;
                s.h = 0x12;
                for (i = 0; i < 5;) {
                    Font_SetPos(pos[0] + 0x18B, y);
                    if (flags & 1) {
                        ItemData *d;

                        Text_PrintfColored((char *)gItemNames[*mat]);
                        d = &gItemData[*mat];
                        s.u0 = (((d->icon + 1) & 7) << 5) + 1;
                        s.v0 = (((d->icon + 1) >> 3) << 5) + 1;
                        s.u1 = s.u0 + 0x1E;
                        s.v1 = s.v0 + 0x1E;
                        s.color = gItemColors[d->color];
                    } else {
                        Text_PrintfColored(gUnknownNameText);
                        *(s32 *)&s.u0 = 0x810041;
                        *(s32 *)&s.u1 = 0x9F005F;
                        s.color = gItemColors[0];
                    }
                    s.y = y;
                    Sprite_Draw((struct Sprite *)&s);
                    i++;
                    flags >>= 1;
                    mat += 4;
                    y += 0x14;
                }
            } else {
                Font_SetColor(0x802B7F7F);
                sprintf(line, gArenaCannotRaiseFmt, gMonsterNames[e->emType]);
                w = (s16)Str_Length(line) * 9 / 2;
                Font_SetPos(pos[0] + 0x1C1 - w, 0x7A);
                Text_Printf(line);
            }
            help = gMonsterListHint;
        }
        if (Save_TestEventFlag(0xC4) == 1) {
            Window_DrawText(&gMonsterNoteHelpBoxes[page], (s16 *)pos, (u8 *)help);
        }
    } else {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(pos[0] + 0x147, 0xA2);
        Text_Printf(gArenaBaitNoBookText[0]);
    }
}

void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
s32 EntryPicture_GetUV(s32 *uv0, s32 *uv1, u16 id);

/* The framed picture of entry e (e[0xD]: the picture, 0xFF none) at x, y. */
void Menu_DrawEntryPicture(MonsterNoteDef *e, s16 x, s16 y)
{
    SpriteW s;

    if (e->picture == 0xFF) {
        return;
    }
    Gfx_DrawLine(x + 3, y + 3, 0x5E, 0x4A, 0xFF493120);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    s.x = x;
    s.y = y;
    s.w = 0x5E;
    s.h = 6;
    s.color = 0xFF493120;
    s.uv0 = 0x810041;
    s.uv1 = 0x87007E;
    Sprite_Draw((struct Sprite *)&s);
    s.x = x + 6;
    s.y = y + 0x4A;
    s.uv0 = 0x86007D;
    s.uv1 = 0x800040;
    Sprite_Draw((struct Sprite *)&s);
    s.x = x;
    s.y = y + 6;
    s.w = 6;
    s.h = 0x4A;
    s.uv0 = 0x830081;
    s.uv1 = 0xBE0088;
    Sprite_Draw((struct Sprite *)&s);
    s.x = x + 0x5E;
    s.y = y;
    s.uv0 = 0xBD0087;
    s.uv1 = 0x820080;
    Sprite_Draw((struct Sprite *)&s);
    Gfx_SetBlend(4, 5);
    Gfx_SetState63(0);
    Tex_Prepare(1, 0x147);
    Tex_Bind(0x147);
    s.x = x + 0x12;
    s.w = 0x40;
    s.y = y + 8;
    s.h = 0x40;
    EntryPicture_GetUV(&s.uv0, &s.uv1, e->picture);
    s.color = 0xFFFFFFFF;
    Sprite_Draw((struct Sprite *)&s);
}

s32 SizeRecord_DrawLarge(MonsterNoteDef *r, s16 x, s16 y, s32 v);
s32 SizeRecord_DrawSmall(MonsterNoteDef *r, s16 x, s16 y, s32 v);
void Hud_DrawRoundGauge(s16 x, s16 y, s16 size, u8 flags);

s32 MonsterList_DrawLargeMark(u16 i, s16 x, s16 y, s32 v)
{
    return SizeRecord_DrawLarge(&gSizeRecordTbl[i], x, y, v);
}

s32 MonsterList_DrawSmallMark(u16 i, s16 x, s16 y, s32 v)
{
    return SizeRecord_DrawSmall(&gSizeRecordTbl[i], x, y, v);
}

/* A round gauge frame of `size` (25 when 0) at x, y, with its inner mark (flags 1 and 2: the
 * lit variants). */
void Hud_DrawRoundGauge(s16 x, s16 y, s16 size, u8 flags)
{
    SpriteW s;

    Gfx_SetState63(0);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    Gfx_SetBlend(4, 5);
    if (size == 0) {
        size = 25;
    }
    s.x = x;
    s.y = y;
    s.w = size;
    s.h = size * 18 / 25;
    s.color = 0xFFFFFFFF;
    if (flags & 1) {
        s.uv0 = 0xCA0064;
        s.uv1 = 0xD70078;
    } else {
        s.uv0 = 0xDD0064;
        s.uv1 = 0xEA0078;
    }
    Sprite_Draw((struct Sprite *)&s);
    s.x = x + size * 6 / 25;
    s.y = y + size * 7 / 25;
    s.w = size * 10 / 25;
    s.h = size * 7 / 25;
    if (flags & 2) {
        s.uv0 = 0xC8007E;
        s.uv1 = 0xCD0086;
    } else {
        s.uv0 = 0xD0007E;
        s.uv1 = 0xD50086;
    }
    Sprite_Draw((struct Sprite *)&s);
}

s32 SizeRecord_DrawLarge(MonsterNoteDef *r, s16 x, s16 y, s32 v)
{
    s16 n = v;
    u8 f;

    if (n < r->bigCrown) {
        return 0;
    }
    f = n >= r->goldCrown;
    Hud_DrawRoundGauge(x, y, 0, f);
    return (s16)(f + 1);
}

s32 SizeRecord_DrawSmall(MonsterNoteDef *r, s16 x, s16 y, s32 v)
{
    s16 n = v;

    if (n > r->smallCrown) {
        return 0;
    }
    Hud_DrawRoundGauge(x, y, 0, 3);
    return 1;
}

MonsterNoteDef *MonsterList_Find(s32 kind)
{
    MonsterNoteDef *rec;
    s32 i;

    rec = gSizeRecordTbl;
    i = 0;
loop_1:
    if (rec->emType == (kind & 0xFF)) {
        return rec;
    }
    i += 1;
    rec++;
    if (i >= 0x34) {
        return 0;
    }
    goto loop_1;
}

void MonsterList_OnItemGet(u16 item)
{
    MonsterNoteDef *rec;
    s32 n;

    rec = gSizeRecordTbl;
    n = 0x34;
    do {
        if (rec->knownItem == (item & 0xFFFF)) {
            MonsterNote_SetKnown((s32) rec->note);
        }
        n -= 1;
        rec++;
    } while (n > 0);
}

s32 QuestMenu_CheckInfo(void)
{
    if ((gScene.online == 1) && (Lobby_HasQuest() == 0)) {
        gHudState->hasQuestInfo = 0;
        return 0;
    }
    gHudState->hasQuestInfo = 1;
    return 1;
}

s32 QuestMenu_Open(void *m)
{
    QuestMenu_CheckInfo();
    gHudState->pageCursor = 0;
    gHudState->questMenuStep = 0;
    gHudState->questMenuOffline = (u8) (gScene.online == 0);
    gHudState->cursorMax = (u8) gQuestMenuCursorMax[gHudState->questMenuOffline];
    gHelpShow = 0;
    Shake_Start(5U, 0U, 280.0f);
    return 0;
}

u16 QuestMenu_Update(u16 buttons)
{
    gHelpShow = 0;
    switch (gHudState->questMenuStep) {
    case 0:
        Shake_Update(5);
        if (QuestMenu_CheckInfo() == 1) {
            Menu_CursorLeftRight(&gHudState->pageCursor, buttons & 0xC00, gHudState->cursorMax);
        }
        if (buttons & 0x8000) {
            (gHudState->questMenuStep)++;
            Shake_Start(5, 0x80, 280.0f);
            buttons = 0;
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        break;
    case 1:
        if (Shake_Update(5) == 0) {
            return 0x8000;
        }
        buttons = 0;
        break;
    }
    return buttons;
}

extern ListPanel gQuestMenuPanel;
extern u8 gQuestMenuKinds[][4];
extern char *gQuestStatusTitle[];
extern char *gNoQuestText;
extern char *gQuestMenuRecordTitles[];
extern char *gQuestMenuTitles[];
extern char *gQuestMenuItemLists[][17];
extern char *gStageNames[];
extern char *gQuestKindNames[];
s32 Quest_IsMustOrder(void);
s32 ArenaCup_QuestTaken(void);
s32 func_793C10(void);
void func_793BE0(void);
s32 Quest_IsDrill(void);
s32 QuestMenu_MapIndex_Unk(s32 a);
void QuestMenu_DrawParTime(s16 x);
void QuestMenu_DrawBestRecord(s16 x);
void QuestMenu_DrawTargets(s16 x);
void QuestMenu_DrawTimeLimit(s32 unused);
void QuestMenu_DrawNumber(s32 v);
void Text_DrawStars(s32 n);
void *QuestInfo_Get(s32 i);

/* Draws the quest info page of the pause menu (page kind gQuestMenuKinds[+0x93][+0x4E]): the quest's
   texts, its stars and rewards, or the page's own drawer. */
void QuestMenu_DrawPage(void)
{
    s16 *pos = Shake_GetOffset(5);
    s32 qkind;
    s32 kind;
    s8 vs;
    s16 idx;
    char buf[64];

    Gfx_SetBlend(4, 5);
    if (gHudState->hasQuestInfo == 0) {
        gQuestMenuPanel.items = NULL;
        ListPanel_Draw(&gQuestMenuPanel, pos, gQuestStatusTitle[0], -1);
        Font_SetPos(*pos + 0x167, 0xB6);
        Text_Draw(gNoQuestText);
        return;
    }
    vs = 0;
    if (gScene.online == 0) {
        if (Quest_IsVersus() == 1) {
            vs = 1;
        }
        qkind = Quest_IsMustOrder();
    } else {
        if (ArenaCup_QuestTaken() == 1) {
            vs = 2;
        }
        qkind = func_793C10();
    }
    kind = (s16)gQuestMenuKinds[gHudState->questMenuOffline][gHudState->pageCursor];
    switch (kind) {
    case 0:
        sprintf(buf, gQuestMenuRecordTitles[0]);
        break;
    default:
        sprintf(buf, gQuestMenuTitles[0], gHudState->pageCursor + 1, gHudState->cursorMax);
        break;
    }
    if (Quest_IsDrill() == 1) {
        gQuestMenuPanel.items = gQuestMenuItemLists[QuestMenu_MapIndex_Unk(kind)];
    } else {
        idx = kind;
        switch (kind) {
        case 1:
            if (qkind == 1) {
                idx = 8;
            }
            break;
        case 2:
            if (vs != 0) {
                idx = 7;
            } else if (qkind == 1) {
                idx = 9;
            }
            break;
        }
        gQuestMenuPanel.items = gQuestMenuItemLists[idx];
    }
    ListPanel_Draw(&gQuestMenuPanel, pos, buf, -1);
    Menu_DrawFrame(&gQuestMenuPanel, pos);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(*pos + 0x167, 0x66);
    Text_Draw(QuestInfo_Get(0));
    switch (kind) {
    case 0:
        QuestMenu_DrawTargets(*pos);
        break;
    case 1:
        if (qkind != 1) {
            Font_SetPos(*pos + 0x167, 0xB6);
            Text_Draw(QuestInfo_Get(1));
            Font_SetPos(*pos + 0x1C1, 0xCA);
            Text_DrawStars(gQuestInfo->stars);
            Font_SetPos(*pos + 0x167, 0x106);
            Text_Draw(QuestInfo_Get(2));
            Font_SetPos(*pos + 0x167, 0x142);
            Text_Draw(QuestInfo_Get(3));
            Font_SetPos(*pos + 0x167, 0x17E);
            Text_Draw(gQuestKindNames[gQuestInfo->terms]);
        } else {
            Font_SetPos(*pos + 0x167, 0xB6);
            Text_Draw(QuestInfo_Get(1));
            Font_SetPos(*pos + 0x167, 0xCA);
            Text_Draw(QuestInfo_Get(2));
            Font_SetPos(*pos + 0x167, 0xDE);
            Text_Draw(QuestInfo_Get(3));
            Font_SetPos(*pos + 0x1C1, 0x106);
            Text_DrawStars(gQuestInfo->stars);
            Font_SetPos(*pos + 0x167, 0x142);
            Text_Draw(gQuestKindNames[gQuestInfo->terms]);
        }
        break;
    case 2:
        if (Quest_IsDrill() == 1) {
            QuestMenu_DrawParTime(*pos);
            break;
        }
        Font_SetPos(*pos + 0x1C1, 0xA2);
        switch (vs) {
        case 0:
        case 1:
            QuestMenu_DrawTimeLimit(1);
            break;
        case 2:
            func_793BE0();
            break;
        }
        if (vs == 0) {
            Font_SetPos(*pos + 0x1C1, 0xB6);
            QuestMenu_DrawNumber(gQuestInfo->fee);
            if (qkind != 1) {
                Font_SetPos(*pos + 0x1E5, 0xCA);
                QuestMenu_DrawNumber(gQuestInfo->reward);
                Font_SetPos(*pos + 0x1E5, 0xDE);
                QuestMenu_DrawNumber(gQuestInfo->rewardSubA);
                Font_SetPos(*pos + 0x1E5, 0xF2);
                QuestMenu_DrawNumber(gQuestInfo->rewardSubB);
            } else {
                Font_SetPos(*pos + 0x1C1, 0xCA);
                QuestMenu_DrawNumber(gQuestInfo->reward);
            }
        } else {
            Font_SetPos(*pos + 0x1C1, 0xB6);
            QuestMenu_DrawNumber(0);
        }
        Font_SetPos(*pos + 0x167, 0x12E);
        Text_Draw(QuestInfo_Get(4));
        Font_SetPos(*pos + 0x167, 0x17E);
        Text_Draw(QuestInfo_Get(5));
        break;
    case 3:
        if (Quest_IsDrill() == 1) {
            QuestMenu_DrawBestRecord(*pos);
            break;
        }
        Font_SetPos(*pos + 0x1C1, 0xA2);
        Text_Draw(QuestInfo_Get(6));
        Font_SetPos(*pos + 0x1C1, 0xB6);
        Text_Draw(gStageNames[gQuestInfo->map]);
        Font_SetPos(*pos + 0x167, 0xF2);
        Text_Draw(QuestInfo_Get(7));
        break;
    }
}

extern char D_0046CFD0[];
extern char *gQuestMenuStatusTexts[];

void QuestMenu_DrawTimeLimit(s32 unused)
{
    char buf[4];
    u16 out[6];
    u16 *d;
    char *s;
    char c;

    sprintf(buf, D_0046CFD0, gQuestInfo->timeLimit / 1800);
    d = out;
    s = buf;
    c = *s;
    while (c != 0) {
        *d = ((c + 0x1F) << 8) | 0x82;
        s++;
        c = *s;
        d++;
    }
    *d = 0;
    Text_PrintfColored(gQuestMenuStatusTexts[0], out);
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_33E620", D_0046CFD0);

s32 Quest_GetTime(s32 which);

void QuestMenu_DrawTime(u8 which)
{
    char buf[8];
    u16 out[12];
    u16 *d;
    char *s;
    char c;
    s32 t;
    s32 m;

    t = Quest_GetTime(which) / 30;
    m = t / 60;
    t -= m * 60;
    sprintf(buf, "%02d:%02d", m, t);
    d = out;
    s = buf;
    c = *s;
    while (c != 0) {
        if (c != ':') {
            *d = ((c + 0x1F) << 8) | 0x82;
        } else {
            *d = 0x4681;
        }
        s++;
        c = *s;
        d++;
    }
    *d = 0;
    Text_Draw((char *)out);
}

void QuestMenu_DrawNumber(s32 v)
{
    u16 out[32];
    char buf[16];
    u16 *d;
    char *s;
    char c;

    sprintf(buf, D_0046CFD0, v);
    d = out;
    s = buf;
    c = *s;
    while (c != 0) {
        *d = ((c + 0x1F) << 8) | 0x82;
        s++;
        c = *s;
        d++;
    }
    d[0] = 0x9A82;
    d[1] = 0;
    Text_Draw((char *)out);
}

/* Draws n full-width underscores. */
void Text_DrawStars(s32 n)
{
    u16 buf[16];
    u16 *p = buf;

    while (n != 0) {
        *p = 0x9A81;
        n--;
        p++;
    }
    *p = 0;
    Text_Draw((char *)buf);
}

extern char D_0046CFF0[];

void QuestMenu_DrawCount(u16 a, u16 b, u8 flag)
{
    char buf[8];
    u16 out[12];
    u16 *d;
    char *s;
    char c;

    if (flag != 0) {
        sprintf(buf, "%2d/%d", a, b);
    } else {
        sprintf(buf, D_0046CFF0, a);
    }
    d = out;
    s = buf;
    c = *s;
    while (c != 0) {
        if (c == '/') {
            *d = 0x5E81;
        } else if (c == ' ') {
            *d = 0x4081;
        } else {
            *d = ((c + 0x1F) << 8) | 0x82;
        }
        s++;
        c = *s;
        d++;
    }
    *d = 0;
    Text_Draw((char *)out);
}

extern char *D_0044590C;

void QuestMenu_DrawRate(void)
{
    char buf[8];
    u16 out[12];
    u16 *d;
    char *s;
    char c;

    sprintf(buf, D_0046CFD0, (10 - gScene.fortStage) * 10);
    d = out;
    s = buf;
    c = *s;
    while (c != 0) {
        *d = ((c + 0x1F) << 8) | 0x82;
        s++;
        c = *s;
        d++;
    }
    *d = 0;
    Text_PrintfColored(D_0044590C, out);
}

extern char D_0046CFF8[];

void QuestMenu_DrawFaints(void)
{
    char out[24];
    char buf[8];
    Player *p;
    s32 i;
    u8 total;
    u8 n;
    s32 left;

    p = gPlayers;
    total = 0;
    for (i = 0; i < 4; i++, p++) {
        if (p->active != 0 && gScene.inQuest[i] == 1) {
            total += p->faints;
        }
    }
    if (gQuestInfo->penalty == 0) {
        sprintf(buf, D_0046CFD0, total);
        Sjis_Widen(buf, out);
        Text_Draw(out);
    } else {
        left = gQuestInfo->reward;
        n = 1;
        while (left > gQuestInfo->penalty) {
            left -= gQuestInfo->penalty;
            n++;
        }
        if (n < total) {
            total = n;
        }
        sprintf(buf, D_0046CFF8, total, n);
        Sjis_Widen(buf, out);
        Text_Draw(out);
    }
}

s32 Quest_IsMustOrder(void);
void Text_PrintfStyled(s32 a, const char *fmt, ...);
extern char *gQuestTargetLabels[];
extern char *D_00445914;
extern char *D_004458EC;
extern char *gQuestMenuTargetTexts[];
void QuestMenu_DrawTime(u8 which);
void QuestMenu_DrawCount(u16 a, u16 b, u8 flag);
void QuestMenu_DrawRate(void);
void QuestMenu_DrawFaints(void);
void *QuestInfo_Get(s32 i);
s32 Quest_PhaseIsOver(s32 a);



/* The quest page at x: the title (gQuestTargetLabels), the quest texts (QuestInfo_Get) and, outside
 * versus quests, the targets with their counts and the done marks. */
void QuestMenu_DrawTargets(s16 x)
{
    s16 lx = x;
    s32 n;
    s32 i;
    s16 y;
    s32 tx;

    i = Quest_IsMustOrder();    /* the quest kind: i is the loop counter only below */
    tx = lx + 0x167;
    Font_SetPos(tx, 0xA2);
    Text_PrintfStyled(0x2A, gQuestTargetLabels[Quest_IsVersus() != 1 ? i == 1 : 2]);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(tx, 0xB6);
    Text_Draw(QuestInfo_Get(1));
    if (Quest_IsVersus() != 1) {
        if (i != 1 || (gQuest.goals[1].flags & 0x10)) {
            Font_SetPos(tx, 0xF2);
            Text_Draw(QuestInfo_Get(2));
        }
        if (i != 1 || (gQuest.goals[2].flags & 0x10)) {
            Font_SetPos(tx, 0x12E);
            Text_Draw(QuestInfo_Get(3));
        }
        if (gQuest.flags & 0x10000000) {
            Font_SetPos(tx, 0x16A);
            QuestMenu_DrawRate();
        }
        Font_SetPos(tx, 0x17E);
        Text_Printf(D_00445914);
        Font_SetPos(lx + 0x1C1, 0x17E);
        QuestMenu_DrawFaints();
    }
    Font_SetPos(lx + 0x1C1, 0x192);
    QuestMenu_DrawTime(0);
    if (Quest_IsVersus() != 1) {
        for (i = 0, y = 0xCA; i < 3; i++, y += 0x3C) {
            if (gQuest.goals[i].flags & 0x10) {
                if ((gQuest.goals[i].flags & 3) && !(gQuest.goals[i].flags & 0x20) && Quest_PhaseIsOver(0) != 1) {
                    Font_SetColor(0x807F5230);
                    Font_SetPos(lx + 0x167, y);
                    Text_Draw(D_004458EC);
                    n = gQuest.goals2[i].count - gQuest.goals[i].count;
                    Font_SetColor(0x807F7F7F);
                    Font_SetPos(lx + 0x1E5, y);
                    QuestMenu_DrawCount(n, 0, 0);
                }
                if (gQuest.goals[i].flags & 0x20) {
                    Font_SetColor(0x807F212E);
                    Font_SetPos(lx + 0x167, y);
                    Text_Draw(gQuestMenuTargetTexts[0]);
                }
            }
        }
    }
}

s32 ChatMenu_Open(void)
{
    gHudState->menuStep = 0;
    gHudState->listCursor = 0;
    gHelpShow = 1;
    gHelpPage = 0xB;
    gHelpLine = 0;
    return 0;
}

s32 Chat_OpenWindow(void);
u16 Chat_LogScroll(u16 buttons);
s32 PhraseMenu_Open(); /* unit's own declaration: PhraseMenu_Open ignores a0 (m), which holds &gHudState->listCursor at ChatMenu_Update's call */
u16 PhraseMenu_Update(u16 buttons);

u16 ChatMenu_Update(u16 buttons)
{
    switch (gHudState->menuStep) {
    case 0:
        if (buttons & 0x8000) {
            FieldMenu_Close();
            return 0;
        }
        Menu_CursorUpDown(&gHudState->listCursor, buttons, 2);
        gHelpLine = gHudState->listCursor;
        if (buttons & 0x20) {
            switch (gHudState->listCursor) {
            case 0:
                Chat_OpenWindow();
                break;
            case 1:
                PhraseMenu_Open();
                break;
            }
            (gHudState->menuStep)++;
            Sound_PlaySeFlat(0, 0x13, 0);
        }
        break;
    case 1:
        switch (gHudState->listCursor) {
        case 0:
            buttons = Chat_LogScroll(buttons);
            break;
        case 1:
            buttons = PhraseMenu_Update(buttons);
            break;
        }
        if (buttons & 0x8000) {
            return 0x8000;
        }
        if (buttons & 0x40) {
            gHudState->menuStep = 0;
            gHelpShow = 1;
            gHelpPage = 0xB;
            gHelpLine = gHudState->listCursor;
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        buttons = 0;
        break;
    }
    return buttons;
}

void ChatMenu_Draw(void)
{
    u8 menuStep;
    u8 listCursor;

    menuStep = gHudState->menuStep;
    switch (menuStep) {
    case 0:
        FieldMenu_DrawList(1);
        ListPanel_Draw(&gChatMenuPanel, NULL, NULL, gHudState->listCursor);
        break;
    case 1:
        listCursor = gHudState->listCursor;
        switch (listCursor) {
        case 0:
            Chat_DrawLog();
            break;
        case 1:
            PhraseMenu_Draw();
        }
        break;
    }
}

s32 PhraseMenu_Open(void *m)
{
    gHudState->phraseStep = 0;
    gHelpPage = 3;
    gHelpShow = 0;
    gHelpLine = 0;
    gPhraseMenuCursor = 0;
    Shake_Start(0xEU, 0U, 300.0f);
    return 0;
}

u8 PhraseMenu_MoveCursor(u16 buttons, u8 cur);
s32 PhraseMenu_OpenKbd(u8 cur);
s8 PhraseMenu_KbdUpdate(u8 cur);

u16 PhraseMenu_Update(u16 buttons)
{
    if (Shake_Flags(0xE) & 0x80) {
        if (Shake_Update(0xE) == 0) {
            return 0x8000;
        }
        return 0;
    }
    switch (gHudState->phraseStep) {
    case 0:
        if (buttons & 0x8000) {
            Shake_Start(0xE, 0x80, 300.0f);
            gHelpShow = 0;
            Sound_PlaySeFlat(0, 0x14, 0);
            return 0;
        }
        gHelpShow = 1;
        if (Shake_Update(0xE) == 1) {
            buttons &= 0xFFBF;
            gHelpShow = 0;
        }
        if (!(buttons & 0x40)) {
            gHelpLine = 0;
            gPhraseMenuCursor = PhraseMenu_MoveCursor(buttons, gPhraseMenuCursor);
            if ((buttons & 0x20) && PhraseMenu_OpenKbd(gPhraseMenuCursor) == 1) {
                (gHudState->phraseStep)++;
            }
        }
        break;
    case 1:
        gHelpLine = 1;
        buttons &= 0x7FBF;
        if (PhraseMenu_KbdUpdate(gPhraseMenuCursor) != 0) {
            gHudState->phraseStep = 0;
        }
        break;
    }
    return buttons;
}

extern ListPanel gPhraseMenuPanel;
extern QuestListEntry gQuestListEntries[];
extern char *gPhraseEditFmt[];
s32 Shake_CopyOffset(u8 id, struct Pair *pos);
void Menu_DrawFrame(void *b, s16 *pos);
void Text_DrawTruncated(u8 n, char *src);

void PhraseMenu_Draw(void)
{
    s16 pos[2];
    char buf[44];
    s32 page;
    s32 i;
    s16 y;
    QuestListEntry *q;

    Shake_CopyOffset(0xE, (struct Pair *)pos);
    page = (s8)gPhraseMenuCursor % 6;
    sprintf(buf, gPhraseEditFmt[0], (s8)gPhraseMenuCursor / 6 + 1);
    if (gHudState->phraseStep == 0) {
        gPhraseMenuPanel.cursorColor = 0xFF0A9182;
    } else {
        gPhraseMenuPanel.cursorColor = 0xFF707070;
    }
    ListPanel_Draw(&gPhraseMenuPanel, pos, buf, page);
    Menu_DrawFrame(&gPhraseMenuPanel, pos);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F7F);
    q = &gQuestListEntries[(s8)gPhraseMenuCursor / 6 * 6];
    y = 0x54;
    i = 6;
    do {
        Font_SetPos(pos[0] + 0x1AF, y);
        Text_DrawTruncated(0x14, q->name);
        y += 0x16;
        q++;
    } while (--i != 0);
}

s32 PauseMenu_Open(void)
{
    if ((Game_IsOnline() != 1) && (Quest_PhaseIsOver(0) != 1) && (Game_PauseState() == 0)) {
        gHelpShow = 0;
        gPauseMenuReq = 1;
        Game_Pause(1U);
        return 0;
    }
    return 1;
}

void Game_Resume(u8 a);

u16 PauseMenu_Update(u16 buttons)
{
    if (buttons & 0x8060) {
        Game_Resume(1);
        return 0x8000;
    }
    return 0;
}

void Pause_Update(void)
{
    switch (gPauseWinStep) {
    case 2:
        gPauseWinFade += 1;
        if ((s32) gPauseWinFade > 4) {
            gPauseWinStep = 0;
            gPauseMenuReq = 0;
        }
        
    case 0:
        if ((gPauseMenuReq != 0) && (Game_PauseState() >= 2)) {
            gPauseWinStep = 1;
            gPauseWinFade = 4;
        }
        break;
    case 1:
        if (Game_PauseState() == 0) {
            gPauseWinFade = 0;
            gPauseWinStep += 1;
            break;
        }
        if (gPauseWinFade != 0) {
            gPauseWinFade -= 1;
        }
        break;
    }
}

void PauseMenu_Draw(void)
{
}

void PouchMenu_Back(void)
{
    gHudState->state = (u8) (gHudState->state - 1);
    Sound_PlaySeFlat(0, 0x14, 0);
    gHelpLine = gHudState->player->pouch[gHudState->slot].item + 0x18;
}

/* The count of the menu's item (+0x55, 1 up to the pouch count; +0x56 set at the maximum): left and
 * right by 1, the shoulder buttons by 10. */
void Menu_AdjustCount(u16 buttons)
{
    Player *p = gHudState->player;
    s16 n = 0;
    s16 max;

    if (buttons & 0x800) {
        n--;
    }
    if (buttons & 0x400) {
        n++;
    }
    if (buttons & 0x2000) {
        n += 10;
    }
    if (buttons & 0x1000) {
        n -= 10;
    }
    if (n != 0) {
        gHudState->atMax = 0;
        n += gHudState->count;
        if (n <= 0) {
            n = 1;
        }
        max = p->pouch[gHudState->slot].count;
        if (n >= max) {
            n = max;
            gHudState->atMax = 1;
        }
        if (gHudState->count != n) {
            gHudState->count = n;
            Sound_PlaySeFlat(0, 0x17, 0);
        }
    }
}

s32 Area_HasTownPcs(u16 a);

/* Defined without a prototype: PouchMenu_Draw passes it the player (it reads the one of
 * gHudState instead). */
s32 PouchMenu_CanTransfer(Player *p) /* p: unused, both callers pass the player */
{
    u16 item;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    item = (gHudState->player)->pouch[gHudState->slot].item;
    if (gItemData[item].rarity >= 3) {
        return 0;
    }
    if (gItemData[item].max == 0xFF) {
        return 0;
    }
    if (gScene.online != 0) {
        return Area_HasTownPcs(gScene.area);
    }
    return 1;
}

s32 PouchMenu_Open(void *m)
{
    gHudState->state = 0;
    gHudState->slot = 0;
    *(s16 *)&gHudState->action = 0;
    gHudState->target = 0;
    if (gScene.localPlayer == 0) {
        gHudState->target = 1;
    }
    gHudState->sorting = 0;
    gHelpShow = 0;
    gHelpPage = 1;
    gHelpLine = (gHudState->player)->pouch[gHudState->slot].item + 0x18;
    gHelpColumn = 0;
    Shake_Start(1, 0, 180.0f);
    return 0;
}

void Pouch_Compact(u8 box);
u16 PouchMenu_MoveCursor(u8 *cur, u16 buttons);
void PouchMenu_DecoCursor(u16 item, u16 buttons);
s32 Scene_InField(void);
void GameMenu_Clear(void);
void Pouch_MoveCursor(u8 *cur, u16 buttons);
s16 Pouch_ClearSlot(Player *p, s16 slot);
u8 YesNo_Toggle(u16 buttons, u8 sel, u16 mask);
s32 NetUser_State(s8 i);

/* The pouch menu, driven by buttons: pick a slot (or sort the pouch), pick drop, give or move,
 * then do it; giving offers the item to another player (Player.offerItem) and waits in state 3.
 * Returns the buttons left unused, 0x8000 once the menu has closed. */
u16 PouchMenu_Update(u16 buttons)
{
    Player *p = gHudState->player;
    s32 ok;
    s32 last;
    ItemStack t;

    if (gHudState->state != 4 && (buttons & 0x8000)) {
        Shake_Start(1, 0x80, 180.0f);
        gHudState->state = 4;
        Sound_PlaySeFlat(0, 0x14, 0);
    }
    gHelpShow = 1;
    switch (gHudState->state) {
    case 0:
        if (Shake_Update(1) == 1) {
            gHelpShow = 0;
            buttons &= 0x3C40;
        }
        gHelpLine = 6;
        if (gHudState->sorting != 0) {
            if (gHudState->yesNo == 0) {
                if (buttons & 0x20) {
                    Pouch_Compact(0);
                    Sound_PlaySeFlat(0, 0x13, 0);
                    gHudState->slot = gHudState->slot % 10;
                    gHudState->sorting = 0;
                } else {
                    if (buttons & 0x40) {
                        gHudState->yesNo = 1;
                        Sound_PlaySeFlat(0, 0x14, 0);
                    } else if (buttons & 0x400) {
                        gHudState->yesNo = 1;
                        Sound_PlaySeFlat(0, 0x17, 0);
                    }
                    buttons = 0;
                    break;
                }
            } else {
                if (buttons & 0x60) {
                    gHudState->sorting = 0;
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else {
                    if (buttons & 0x800) {
                        gHudState->yesNo = 0;
                        Sound_PlaySeFlat(0, 0x17, 0);
                    }
                    buttons = 0;
                    break;
                }
            }
            buttons = 0;
        } else {
            if (buttons & 0x40) {
                break;
            }
            if (buttons & 0x4000) {
                gHudState->sorting = 1;
                buttons = 0;
                gHudState->yesNo = 0;
                Sound_PlaySeFlat(0, 0xF, 0);
                break;
            }
        }
        gHelpColumn &= 3;
        PouchMenu_MoveCursor(&gHudState->slot, buttons);
        gHelpColumn |= 0x10;
        if (buttons & 0x20) {
            if (p->pouch[gHudState->slot].item != 0) {
                gHudState->state++;
                Sound_PlaySeFlat(0, 0x13, 0);
            } else {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
        }
        break;
    case 1:
        if (p->pouch[gHudState->slot].item == 0) {
            gHudState->state = 0;
            buttons &= ~0x40;
            break;
        }
        Menu_CursorUpDown(&gHudState->action, buttons, 3);
        gHelpColumn &= 3;
        PouchMenu_DecoCursor(p->pouch[gHudState->slot].item, buttons & 0x80);
        gHelpColumn |= 0x10;
        if (buttons & 0x40) {
            buttons &= ~0x40;
            gHudState->state--;
            Sound_PlaySeFlat(0, 0x14, 0);
        } else if (buttons & 0x20) {
            if (Player_IsFree(p) == 0) {
                Sound_PlaySeFlat(0, 0x15, 0);
            } else {
                gHudState->cancel = 0;
                switch (gHudState->action) {
                case 0:
                    gHudState->yesNo = 1;
                    gHudState->state++;
                    gHudState->step = 0;
                    Sound_PlaySeFlat(0, 0x13, 0);
                    break;
                case 1:
                    if (PouchMenu_CanTransfer(p) == 1) {
                        gHudState->count = 1;
                        gHudState->state++;
                        if (p->pouch[gHudState->slot].count != 1) {
                            gHudState->atMax = 0;
                            gHudState->step = 0;
                        } else {
                            gHudState->step = 1;
                        }
                        Sound_PlaySeFlat(0, 0x13, 0);
                    } else {
                        Sound_PlaySeFlat(0, 0x15, 0);
                    }
                    break;
                case 2:
                    gHudState->state++;
                    gHudState->dest = gHudState->slot;
                    Sound_PlaySeFlat(0, 0x13, 0);
                    break;
                }
            }
        }
        break;
    case 2:
        if (p->pouch[gHudState->slot].item == 0) {
            gHudState->state = 0;
            buttons = 0;
            gHelpLine = gHudState->player->pouch[gHudState->slot].item + 0x18;
        } else if (gHudState->cancel != 0) {
            gHudState->state = 1;
            buttons = 0;
            gHelpLine = gHudState->player->pouch[gHudState->slot].item + 0x18;
        } else if (Player_IsFree(p) == 0) {
            buttons = 0;
            gHudState->state = 1;
            gHelpLine = gHudState->player->pouch[gHudState->slot].item + 0x18;
        } else {
            switch (gHudState->action) {
            case 0:
                if (buttons & 0x40) {
                    buttons &= ~0x40;
                    PouchMenu_Back();
                } else {
                    gHelpLine = 0;
                    gHudState->yesNo = YesNo_Toggle(buttons, gHudState->yesNo, 0x3000);
                    if (buttons & 0x20) {
                        if (gHudState->yesNo == 0) {
                            Pouch_ClearSlot(p, gHudState->slot);
                            gHudState->state = 0;
                            gHudState->action = 0;
                            Sound_PlaySeFlat(0, 0x13, 0);
                        } else {
                            PouchMenu_Back();
                        }
                    }
                }
                break;
            case 1:
                switch (gHudState->step) {
                case 0:
                    if (buttons & 0x40) {
                        buttons &= ~0x40;
                        PouchMenu_Back();
                    } else {
                        gHelpLine = 3;
                        Menu_AdjustCount(buttons);
                        if (buttons & 0x20) {
                            gHelpLine = 1;
                            gHudState->step++;
                            Sound_PlaySeFlat(0, 0x13, 0);
                        }
                    }
                    break;
                case 1:
                    if (buttons & 0x40) {
                        buttons &= ~0x40;
                        if (p->pouch[gHudState->slot].count != 1 &&
                            gItemData[p->pouch[gHudState->slot].item].max != 0xFF) {
                            gHudState->step = 0;
                            Sound_PlaySeFlat(0, 0x14, 0);
                        } else {
                            PouchMenu_Back();
                        }
                    } else {
                        gHelpLine = 1;
                        last = gScene.online == 0 ? 3 : 9;
                        if (buttons & 0x2000) {
                            gHudState->target--;
                            if (gHudState->target < 0) {
                                gHudState->target = last;
                            }
                            if (gHudState->target == gScene.localPlayer) {
                                gHudState->target--;
                            }
                            if (gHudState->target < 0) {
                                gHudState->target = last;
                            }
                            Sound_PlaySeFlat(0, 0x17, 0);
                        } else if (buttons & 0x1000) {
                            gHudState->target++;
                            if (gHudState->target > last) {
                                gHudState->target = 0;
                            }
                            if (gHudState->target == gScene.localPlayer) {
                                gHudState->target++;
                            }
                            if (gHudState->target > last) {
                                gHudState->target = 0;
                            }
                            Sound_PlaySeFlat(0, 0x17, 0);
                        }
                        if (buttons & 0x20) {
                            if (gScene.online == 0) {
                                ok = gScene.inQuest[gHudState->target] == 1;
                            } else {
                                switch (NetUser_State(gHudState->target)) {
                                case 0:
                                    ok = 1;
                                    break;
                                case 1:
                                case 2:
                                    ok = 0;
                                    break;
                                }
                            }
                            if (ok == 1) {
                                buttons = 0;
                                p->offerTo = gHudState->target;
                                p->offerItem = p->pouch[gHudState->slot].item;
                                p->offerCount = gHudState->count;
                                p->offerRequest = 1;
                                gHudState->state = 3;
                            } else {
                                Sound_PlaySeFlat(0, 0x15, 0);
                            }
                        }
                    }
                    break;
                }
                break;
            case 2:
                if (buttons & 0x40) {
                    buttons &= ~0x40;
                    PouchMenu_Back();
                } else {
                    gHelpLine = 7;
                    Pouch_MoveCursor(&gHudState->dest, buttons);
                    if (buttons & 0x20) {
                        if (gHudState->slot != gHudState->dest) {
                            t = p->pouch[gHudState->slot];
                            p->pouch[gHudState->slot] = p->pouch[gHudState->dest];
                            p->pouch[gHudState->dest] = t;
                            if (Scene_InField() == 1) {
                                if (p->ammoSlot == gHudState->slot) {
                                    p->ammoSlot = gHudState->dest;
                                } else if (p->ammoSlot == gHudState->dest) {
                                    p->ammoSlot = gHudState->slot;
                                }
                                if (p->coatingSlot == gHudState->slot) {
                                    p->coatingSlot = gHudState->dest;
                                } else if (p->coatingSlot == gHudState->dest) {
                                    p->coatingSlot = gHudState->slot;
                                }
                            }
                            gHudState->slot = gHudState->dest;
                            gHudState->state = 0;
                            gHelpLine = gHudState->player->pouch[gHudState->slot].item + 0x18;
                            Sound_PlaySeFlat(1, 0x8B, 0);
                        } else {
                            Sound_PlaySeFlat(0, 0x15, 0);
                        }
                    }
                }
                break;
            default:
                gHudState->state = 0;
                break;
            }
        }
        break;
    case 3:
        if (p->offerStarted != 0) {
            if (gScene.online == 0) {
                FieldMenu_Exit();
            } else {
                GameMenu_Clear();
            }
            Sound_PlaySeFlat(0, 0x13, 0);
        } else {
            gHudState->state = 2;
            gHudState->action = 1;
            gHudState->step = 1;
            Sound_PlaySeFlat(0, 0x15, 0);
        }
        buttons = 0;
        break;
    case 4:
        if (Shake_Update(1) == 0) {
            return 0x8000;
        }
        gHelpShow = 0;
        buttons = 0;
        break;
    }
    return buttons;
}

s32 Menu_IsNotCursorSlot(s32 base)
{
    return (base & 0xFF) != *(u8 *)((u8 *)gHudState + 0x51); /* ->slot allocates the registers the other way */
}

s32 Combine_ItemHasRecipe(s16 item);
s32 Combine_ItemUsable(s16 item);

s32 CombineMenu_CheckFirst(u8 slot, u16 item)
{
    if (slot == 0xFF) {
        return 0;
    }
    if ((gHudState->player)->pouch[slot].item != item) {
        return 0;
    }
    if (gHudState->combineThree == 0) {
        if (Combine_ItemHasRecipe(item) == 0) {
            return 0;
        }
    } else if (Combine_ItemUsable(item) == 0) {
        return 0;
    }
    return 1;
}

struct Recipe *Combine_FindRecipe(s16 a, s16 b);
s32 Combine_CanUsePair(s16 a, s16 b);

s32 CombineMenu_CheckPair(u8 a, u8 b)
{
    Player *p = gHudState->player;

    if (a == 0xFF) {
        return 0;
    }
    if (b == a) {
        return 0;
    }
    if (gHudState->combineThree == 0) {
        gHudState->combineRecipe = Combine_FindRecipe(p->pouch[b].item, p->pouch[a].item);
        if (gHudState->combineRecipe == NULL) {
            return 0;
        }
        if (Recipe_IsKnown(gHudState->combineRecipe) != 0) {
            gHudState->combineResult = gHudState->combineRecipe->result;
        } else {
            gHudState->combineResult = -1;
        }
    } else if (Combine_CanUsePair(p->pouch[b].item, p->pouch[a].item) == 0) {
        return 0;
    }
    return 1;
}

Recipe *Combine_FindRecipe3(s16 a, s16 b, s16 c);

s32 CombineMenu_CheckTriple(u8 a, u8 b, u8 c)
{
    Player *p = gHudState->player;

    if (b == 0xFF) {
        return 0;
    }
    if (a == 0xFF) {
        return 0;
    }
    if (c == b) {
        return 0;
    }
    if (c == a) {
        return 0;
    }
    if (b == a) {
        return 0;
    }
    gHudState->combineRecipe = Combine_FindRecipe3(p->pouch[c].item, p->pouch[b].item, p->pouch[a].item);
    if (gHudState->combineRecipe == NULL) {
        return 0;
    }
    if (Recipe_IsKnown(gHudState->combineRecipe) != 0) {
        gHudState->combineResult = gHudState->combineRecipe->result;
    } else {
        gHudState->combineResult = -1;
    }
    return 1;
}

void CombineMenu_Reset(void)
{
    gHudState->unk88 = 0;
    gHudState->slot = 0;
    gHudState->combineThree = 0;
    gHudState->combineConfirm = 0;
    gHudState->combineSlots[2] = 0xFF;
    gHudState->combineSlots[1] = 0xFF;
    gHudState->combineSlots[0] = 0xFF;
    gHudState->combineItems[2] = 0xFFFF;
    gHudState->combineItems[1] = 0xFFFF;
    gHudState->combineItems[0] = 0xFFFF;
    gHudState->combineResult = -1;
    gHudState->combineRecipe = NULL;
    gHudState->fadeActive = 0;
}

s32 Hud_MenuAllowed_Unk(void)
{
    Player *p;

    p = gHudState->player;
    if (gScene.online == 0) {
        if (Player_IsFree(p) != 1) {
            return 0;
        }
        goto block_6;
    }
    if (Lobby_LacksQuest() == 0) {
        return 0;
    }
block_6:
    return 1;
}

s32 CombineMenu_IsPicked(s32 base)
{
    s32 a = base & 0xFFFF;
    HudState *m = gHudState;

    if (a == m->combineSlots[0] || a == m->combineSlots[1]) {
        return 1;
    }
    if (m->combineThree != 0 && a == m->combineSlots[2]) {
        return 1;
    }
    return 0;
}

s32 CombineMenu_ResultRoom(void *list)
{
    s32 room;
    s32 ret;
    Recipe *r;

    r = gHudState->combineRecipe;
    if (r != NULL) {
        if (*(u16 *)((u8 *)list + 0) != r->result) {
            return -1;
        }
        if (Recipe_IsKnown(r) == 0) {
            return -1;
        }
        room = *(&gItemData->max + (*(u16 *)((u8 *)list + 0) * 0x18)) - *(s16 *)((u8 *)list + 2);
        if (room <= 0) {
            return 0;
        }
        if (room < (Recipe_YieldBase(gHudState->combineRecipe) & 0xFF)) {
            ret = 1;
        } else {
            ret = 2;
        }
        return ret;
    }
    return -1;
}

s32 CombineMenu_IsReady(void)
{
    HudState *m = gHudState;

    if (m->combineItems[0] == 0xFFFF || m->combineItems[1] == 0xFFFF) {
        return 0;
    }
    if (m->combineThree != 0 && m->combineItems[2] == 0xFFFF) {
        return 0;
    }
    return 1;
}

s32 CombineMenu_HasSpecial(void)
{
    if (Item_IsSpecial((u16)gHudState->combineResult) == 1) {
        if (Item_IsSpecial(gHudState->combineItems[0]) != 0) {
            return 1;
        }
        if (Item_IsSpecial(gHudState->combineItems[1]) != 0) {
            return 1;
        }
        if ((gHudState->combineThree != 0) && (Item_IsSpecial(gHudState->combineItems[2]) != 0)) {
            return 1;
        }
        goto block_9;
    }
block_9:
    return 0;
}

s32 CombineMenu_IsKnown(void)
{
    if (gHudState->combineThree == 0) {
        return Combine_IsKnown((s16)gHudState->combineItems[0], (s16)gHudState->combineItems[1]);
    }
    return Combine_IsKnown3((s32) (s16)gHudState->combineItems[0], (s32) (s16)gHudState->combineItems[1], (s32) (s16)gHudState->combineItems[2]);
}

void Lobby_ClearConnected(void);

s32 CombineMenu_Open(void)
{
    if (Hud_MenuAllowed_Unk() != 1) {
        return 1;
    }
    gHudState->unk84 = -1;
    if ((s16)ItemGet_Reserve(0, (u16 *)&gHudState->unk84) < 0) {
        return 1;
    }
    if (gScene.online != 0) {
        Lobby_ClearConnected();
    }
    CombineMenu_Reset();
    gHudState->sorting = 0;
    gHelpShow = 1;
    gHelpPage = 1;
    gHelpLine = gHudState->combineThree + 0xE;
    gHelpColumn = 0;
    Shake_Start(1, 0, 180.0f);
    return 0;
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_33E620", D_0046CFF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_33E620", D_0046CFF8);

/* The combine menu's part of the work area (gHudState). */
typedef struct CombineWork {
    u8 pad00[7];
    u8 unk07;           /* 0x07: nonzero while a message is up */
    Player *player;     /* 0x08 */
    u8 pad0C[3];
    u8 unk0F;           /* 0x0F: the sort prompt's choice (0 yes) */
    u8 pad10[0x51 - 0x10];
    u8 unk51;           /* 0x51: the pouch cursor */
    u8 pad52[0x70 - 0x52];
    Recipe *recipe;     /* 0x70: the chosen recipe */
    u16 items[3];       /* 0x74: the chosen ingredients */
    u8 slots[3];        /* 0x7A: their pouch slots (0xFF none) */
    u8 three;           /* 0x7D: 1 for a three-ingredient combine */
    s16 timer;          /* 0x7E */
    s16 count;          /* 0x80: recipes found */
    u8 unk82;           /* 0x82: the quantity made */
    u8 unk83;           /* 0x83: the recipe is known (no fade) */
    s16 unk84;          /* 0x84 */
    s16 item;           /* 0x86: the item made (0xAE failure) */
    u8 state;           /* 0x88 */
    u8 sel;             /* 0x89: ingredient being chosen */
    u8 unk8A;           /* 0x8A: the confirm cursor */
    s8 unk8B;           /* 0x8B: 1 no room, 2 no room for the rest, 4 an ingredient gone */
    u8 pad8C[0xA5 - 0x8C];
    u8 unkA5;           /* 0xA5: items taken from the pouch */
    u8 unkA6;           /* 0xA6: the sort prompt is up */
} CombineWork;

#define CW ((CombineWork *)gHudState)

void CombineMenu_StartFade(s32 a);
void Recipe_SetKnown(Recipe *r, s16 *item);
s32 Recipe_CheckAllKnown(void);
s16 Combine_Do(u8 *count, Player *player, s16 a, s16 b, s32 mode);
s16 Combine_Do3(u8 *count, Player *player, s32 a, s32 b, s32 c, s32 mode);
s16 Pouch_FindSlot(Player *p, u16 item);
s16 Pouch_GiveItems(Player *p, u16 item, s16 key, u8 flags, s16 n);
void Item_PlayUseSound(Player *p, u16 item);
u8 Notice_Push(u8 a, s16 b, s16 c);
void Jingle_Play(void *obj, s32 id);
s16 Pouch_Space(Player *p, u16 item);

/* The combine menu's input step: the ingredient picks (state 1), the confirmation (2), the
   result (3-5) and the checks after it (6); returns the buttons left for the caller. */
u16 CombineMenu_Input(u16 btn)
{
    Player *p = CW->player;
    u16 ret;
    u16 b;

    if ((CW->state < 3 || CW->state == 5) && (btn & 0x8000)) {
        Shake_Start(1, 0x80, 180.0f);
        CW->state = 7;
        Sound_PlaySeFlat(0, 0x14, 0);
    }
    b = btn;
    ret = b & ~0x8000;
    if (CW->unk51 >= 20) {
        CW->unk51 = 0;
    }
    gHelpShow = 1;
    switch (CW->state) {
    case 0:
        if (Shake_Update(1) == 1) {
            gHelpShow = 0;
            btn &= 0x3C40;
        }
        gHelpLine = 6;
        if (CW->unkA6 != 0) {
            ret &= ~0x40;
            if (CW->unk0F == 0) {
                if (btn & 0x20) {
                    Pouch_Compact(0);
                    Sound_PlaySeFlat(0, 0x13, 0);
                    CW->unk51 = CW->unk51 % 10;
                    CW->unkA6 = 0;
                } else {
                    if (btn & 0x40) {
                        CW->unk0F = 1;
                        Sound_PlaySeFlat(0, 0x14, 0);
                    } else if (btn & 0x400) {
                        CW->unk0F = 1;
                        Sound_PlaySeFlat(0, 0x17, 0);
                    }
                    break;
                }
            } else {
                if (btn & 0x60) {
                    CW->unkA6 = 0;
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else {
                    if (btn & 0x800) {
                        CW->unk0F = 0;
                        Sound_PlaySeFlat(0, 0x17, 0);
                    }
                    break;
                }
            }
            btn = 0;
        } else if (btn & 0x40) {
            break;
        } else if (btn & 0x4000) {
            CW->unkA6 = 1;
            CW->unk0F = 0;
            Sound_PlaySeFlat(0, 0xF, 0);
            break;
        }
        Menu_CursorUpDown(&CW->three, btn, 2);
        b = btn;
        gHelpLine = CW->three + 0xE;
        Pouch_MoveCursor(&CW->unk51, b & 0xC00);
        if (b & 0x20) {
            CW->state++;
            CW->sel = 0;
            Sound_PlaySeFlat(0, 0x13, 0);
        }
        break;
    case 1:
        ret &= ~0x40;
        switch (CW->sel) {
        case 0:
            Pouch_MoveCursor(&CW->unk51, btn);
            if (b & 0x40) {
                CW->state = 0;
                gHelpLine = CW->three + 0xE;
                Sound_PlaySeFlat(0, 0x14, 0);
            } else if (b & 0x20) {
                if (CombineMenu_CheckFirst(CW->unk51, p->pouch[CW->unk51].item) == 1) {
                    CW->slots[0] = CW->unk51;
                    CW->items[0] = p->pouch[CW->slots[0]].item;
                    CW->items[1] = 0xFFFF;
                    CW->sel++;
                    Sound_PlaySeFlat(0, 0x13, 0);
                } else {
                    Sound_PlaySeFlat(0, 0x15, 0);
                }
            }
            break;
        case 1:
            if (CombineMenu_CheckFirst(CW->slots[0], CW->items[0]) == 1) {
                Pouch_MoveCursor(&CW->unk51, btn);
                if (b & 0x40) {
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else {
                    if (b & 0x20) {
                        if (CombineMenu_CheckPair(CW->unk51, CW->slots[0]) == 1) {
                            CW->slots[1] = CW->unk51;
                            CW->items[1] = p->pouch[CW->slots[1]].item;
                            CW->unk8A = 0;
                            if (CW->three == 0) {
                                CW->state++;
                            } else {
                                CW->items[2] = 0xFFFF;
                            }
                            CW->sel++;
                            Sound_PlaySeFlat(0, 0x13, 0);
                        } else {
                            Sound_PlaySeFlat(0, 0x15, 0);
                        }
                    }
                    break;
                }
            }
            CW->slots[0] = 0xFF;
            CW->items[0] = 0xFFFF;
            CW->sel = 0;
            break;
        case 2:
            if (CombineMenu_CheckFirst(CW->slots[0], CW->items[0]) != 1) {
                CW->slots[0] = 0xFF;
                CW->items[0] = 0xFFFF;
                CW->sel = 0;
                break;
            }
            if (CombineMenu_CheckFirst(CW->slots[1], CW->items[1]) == 1) {
                Pouch_MoveCursor(&CW->unk51, btn);
                if (b & 0x40) {
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else {
                    if (b & 0x20) {
                        if (CombineMenu_CheckTriple(CW->unk51, CW->slots[0], CW->slots[1]) == 1) {
                            CW->slots[2] = CW->unk51;
                            CW->items[2] = p->pouch[CW->slots[2]].item;
                            CW->state++;
                            CW->sel++;
                            Sound_PlaySeFlat(0, 0x13, 0);
                        } else {
                            Sound_PlaySeFlat(0, 0x15, 0);
                        }
                    }
                    break;
                }
            }
            CW->slots[1] = 0xFF;
            CW->items[1] = 0xFFFF;
            CW->sel = 1;
            break;
        }
        break;
    case 2:
        CW->unk8B = 0;
        CW->unk83 = 0;
        if (CombineMenu_CheckFirst(CW->slots[0], CW->items[0]) == 0) {
            CW->slots[0] = Pouch_FindSlot(p, CW->items[0]);
            if (CW->slots[0] == 0xFF) {
                CW->unk8B |= 4;
            }
        }
        if (CombineMenu_CheckFirst(CW->slots[1], CW->items[1]) == 0) {
            CW->slots[1] = Pouch_FindSlot(p, CW->items[1]);
            if (CW->slots[1] == 0xFF) {
                CW->unk8B |= 4;
            }
        }
        if (CW->three != 0 && CombineMenu_CheckFirst(CW->slots[2], CW->items[2]) == 0) {
            CW->slots[2] = Pouch_FindSlot(p, CW->items[2]);
            if (CW->slots[2] == 0xFF) {
                CW->unk8B |= 4;
            }
        }
        if (CW->unk8B == 0) {
            if (CombineMenu_IsKnown() != 0) {
                s16 n = Pouch_Space(p, CW->count);

                if (n == 0) {
                    if (CombineMenu_HasSpecial() == 0) {
                        CW->unk8B |= 1;
                        gHelpLine = 0xC;
                    }
                } else if (n < 0) {
                    if (p->pouch[CW->slots[0]].count > 1 && p->pouch[CW->slots[1]].count > 1) {
                        CW->unk8B |= 2;
                        gHelpLine = 0xD;
                    }
                }
                CW->unk83 = 1;
            }
        } else {
            gHelpLine = 0xB;
        }
        if (CW->unk8B == 0) {
            if (CW->count == -1) {
                gHelpLine = 0xA;
            } else {
                gHelpLine = CW->count + 0x18;
            }
        } else {
            CW->unk8A = 1;
            btn &= 0xCFFF;
        }
        b = btn;
        if (b & 0x20) {
            ret &= 0x7FBF;
            switch (CW->unk8A) {
            case 0:
                if (CW->unk8B == 0) {
                    if (CW->unk83 != 0) {
                        CW->timer = 1;
                    } else {
                        CW->timer = 30;
                        CombineMenu_StartFade(0);
                    }
                    CW->state++;
                    if (CW->three == 0) {
                        CW->item = Combine_Do(&CW->unk82, p, p->pouch[CW->slots[0]].item, p->pouch[CW->slots[1]].item, 0);
                    } else {
                        CW->item = Combine_Do3(&CW->unk82, p, (s16)p->pouch[CW->slots[0]].item, (s16)p->pouch[CW->slots[1]].item, (s16)p->pouch[CW->slots[2]].item, 0);
                    }
                    CW->unkA5 = 1;
                } else {
                    Sound_PlaySeFlat(0, 0x15, 0);
                }
                break;
            case 1:
                CombineMenu_Reset();
                return 0x40;
            }
        } else if (b & 0x8000) {
            CombineMenu_Reset();
            Sound_PlaySeFlat(0, 0x14, 0);
            return 0x8000;
        } else if (b & 0x40) {
            ret &= ~0x40;
            if (CW->slots[1] < 20) {
                CW->unk51 = *(u8 *)(CW->three + (u32)CW + 0x7B); /* slots[three + 1] */
            } else {
                CW->unk51 = 0;
            }
            CW->count = -1;
            CW->state = 1;
            if (CW->three == 0) {
                CW->slots[1] = 0xFF;
                CW->items[1] = 0xFFFF;
                CW->sel = 1;
            } else {
                CW->slots[2] = 0xFF;
                CW->items[2] = 0xFFFF;
                CW->sel = 2;
            }
            gHelpLine = CW->three + 0xE;
            Sound_PlaySeFlat(0, 0x14, 0);
        } else {
            Menu_CursorUpDown(&CW->unk8A, btn, 2);
            Pouch_MoveCursor(&CW->unk51, b & 0xC00);
        }
        break;
    case 3:
        ret &= 0x7FBF;
        CW->timer--;
        if (CW->timer > 0) {
            break;
        }
        if (CW->item > 0) {
            Recipe_SetKnown(CW->recipe, &CW->item);
            CW->count = CW->item;
            CombineMenu_StartFade(1);
            Recipe_CheckAllKnown();
        } else {
            CW->item = 0xAE;
            CW->unk82 = 1;
            CombineMenu_StartFade(2);
        }
        if (p->pouch[CW->slots[0]].item == 0) {
            CW->slots[0] = 0xFF;
        }
        if (p->pouch[CW->slots[1]].item == 0) {
            CW->slots[1] = 0xFF;
        }
        if (CW->three != 0 && p->pouch[CW->slots[2]].item == 0) {
            CW->slots[2] = 0xFF;
        }
        gHelpLine = CW->item + 0x18;
        CW->state++;
    case 4:
        ret &= 0x7FBF;
        switch (Pouch_GiveItems(p, CW->item, CW->unk84, 0, CW->unk82)) {
        case -1:
            break;
        case 0:
        case 1:
            if (CW->unk83 == 0) {
                if (CW->item != 0xAE) {
                    Item_PlayUseSound(p, CW->item);
                } else {
                    Jingle_Play(p, 10);
                }
            }
            if (CW->unk83 != 0) {
                CW->timer = 1;
            } else {
                CW->timer = 30;
            }
            CW->state++;
            CW->unkA5 = 0;
            break;
        case 2:
        case 3:
            Notice_Push(1, 3, CW->item);
            if (CW->unk83 != 0) {
                CW->timer = 1;
            } else {
                CW->timer = 30;
            }
            CW->state++;
            CW->unkA5 = 0;
            break;
        case 5:
            CW->unk51 = 0;
            CW->state = 6;
            CW->unkA5 = 0;
            break;
        }
        break;
    case 5:
        ret &= ~0x40;
        if (CW->timer != 0) {
            CW->timer--;
        } else if (b & 0x3FFF) {
            CW->state = 2;
            Sound_PlaySeFlat(0, 9, 0);
        }
        break;
    case 6:
        ret &= ~0x40;
        if (CW->unk07 != 0) {
            break;
        }
        if (CW->items[0] != p->pouch[CW->slots[0]].item) {
            CW->slots[0] = 0xFF;
        } else if (CW->items[1] != p->pouch[CW->slots[1]].item) {
            CW->slots[1] = 0xFF;
        } else if (CW->three != 0 && CW->items[2] != p->pouch[CW->slots[2]].item) {
            CW->slots[2] = 0xFF;
        }
        CW->state = 2;
        if (CW->three == 0) {
            if (CW->slots[0] != 0xFF && CW->slots[1] != 0xFF && Combine_FindRecipe(CW->items[0], CW->items[1]) == NULL) {
                CombineMenu_Reset();
                CW->state++;
            }
        } else if (CW->slots[0] != 0xFF && CW->slots[1] != 0xFF && CW->slots[2] != 0xFF
                   && Combine_FindRecipe3(CW->items[0], CW->items[1], CW->items[2]) == NULL) {
            CombineMenu_Reset();
            CW->state++;
            CW->three = 1;
        }
        break;
    case 7:
        if (Shake_Update(1) == 0) {
            return 0x8000;
        }
        gHelpShow = 0;
        ret = 0;
        break;
    }
    return ret;
}

typedef struct RecipeView {
    s16 item;
    s16 result;         /* 0x02 */
} RecipeView;

extern ListPanel gCombineKindPanel;
extern ListPanel gCombineMaterialPanel;
extern Panel gCombineResultWindow;
extern ListPanel gCombineDoPanel;
extern u32 D_004321C4[]; /* gCombineDoPanel's text colour as one RGBA word (textColor, unk16, rgbText) */
extern char *gYesNoTexts[];
extern char *gCombineSelectFmt[];
extern char *gCombineInfoLabels[];
extern char *gCombineUnknownText;
extern char *gCombineRateFmt[];
extern char *gCombineRateUnknownText;
extern char *gCombineSelectingText[];
extern char *gCombineSuccessLabels;
extern char *gCombineFailLabels[];
extern char *gCombine2MaterialLabels[];
extern char *gCombine3MaterialLabels[];
s16 Pouch_Count(Player *p, u16 item);
s32 Combine_RecipeRate(struct Recipe *r, s32 mode);
void *Recipe_YieldLabel(struct Recipe *r);
void CombineMenu_DrawItemName(u8 arg0, u16 arg1);
void ItemList_DrawPouch(s32 a, s32 b, s32 flags);

/* Draws the combine menu's info panel for the state at +0x88: the ingredient list, the chosen
   recipe's result, success rate and yield, or (states 5 and 6) the result item's counts. */
void CombineMenu_DrawInfo(void)
{
    Player *p = CW->player;
    s32 help;
    s32 flags;
    u16 n;
    u16 cnt;
    u16 max;

    Gfx_SetState63(0);
    Gfx_SetBlend(4, 5);
    help = 0;
    flags = 0x85;
    if (CW->state == 0) {
        if (CW->unkA6 != 0) {
            flags = 0x11;
            Font_SetSpacing(0x12, 0x12);
            Font_SetPos(0x1B0, 0x18E);
            Text_Printf(gYesNoTexts[CW->unk0F]);
        } else {
            if (Shake_Value(1) == 0) {
                ListPanel_Draw(&gCombineKindPanel, NULL, NULL, CW->three);
            }
            flags = 0xAD;
        }
    } else if (CW->state < 5) {
        gCombineMaterialPanel.lines = CW->three + 2;
        ListPanel_Draw(&gCombineMaterialPanel, NULL, NULL, -1);
        Font_SetSpacing(0x12, 0x12);
        Font_SetColor(0x807F7F28);
        Text_DrawLinesColored(0x10E, 0x3E, CW->three == 0 ? gCombine2MaterialLabels : gCombine3MaterialLabels, 0x27);
        Font_SetPos(0x10E, 0x50);
        if (CW->sel == 0) {
            Text_Printf(gCombineSelectingText[0]);
        } else {
            CombineMenu_DrawItemName(CW->slots[0], CW->items[0]);
            Font_SetPos(0x10E, 0x77);
            if (CW->sel == 1) {
                Text_Printf(gCombineSelectingText[0]);
            } else {
                CombineMenu_DrawItemName(CW->slots[1], CW->items[1]);
                if (CW->three > 0) {
                    Font_SetPos(0x10E, 0x9E);
                    if (CW->sel == 2) {
                        Text_Printf(gCombineSelectingText[0]);
                    } else {
                        CombineMenu_DrawItemName(CW->slots[2], CW->items[2]);
                    }
                }
            }
        }
        if (CW->state < 2) {
            help = 0xA9182;
            Font_SetColor(0x807F7F7F);
            Font_SetPos(0x132, 0x166);
            Text_PrintfColored(gCombineSelectFmt[0], gCombine3MaterialLabels[CW->sel]);
        }
        if (CW->state < 3) {
            flags = 0xD;
        }
    }
    ItemList_DrawPouch(CW->unk51, help, flags);
    if (CW->state >= 2 && CW->state < 5) {
        gCombineResultWindow.y = CW->three == 0 ? 0x9D : 0xC4;
        Window_DrawText(&gCombineResultWindow, (s16 *)NULL, (u8 *)gCombineInfoLabels[0]);
        Font_SetColor(0x807F7F7F);
        Font_SetPos(0x10E, gCombineResultWindow.y + 0x14);
        if (CW->count > 0) {
            Text_Draw((char *)gItemNames[((RecipeView *)CW->recipe)->result]);
        } else {
            Text_Draw(gCombineUnknownText);
        }
        Font_SetPos(0x168, gCombineResultWindow.y + 0x28);
        if (CW->count > 0) {
            Text_PrintfColored(gCombineRateFmt[0], (s8)Combine_RecipeRate(CW->recipe, 0));
        } else {
            Text_Draw(gCombineRateUnknownText);
        }
        Font_SetPos(0x168, gCombineResultWindow.y + 0x3C);
        Text_Draw(Recipe_YieldLabel(CW->recipe));
        if (CW->state == 2) {
            gCombineDoPanel.y = CW->three == 0 ? 0xFF : 0x125;
            D_004321C4[0] = CW->unk8B == 0 ? 0x807F7F7F : 0x80404040;
            ListPanel_Draw(&gCombineDoPanel, NULL, NULL, CW->unk8A);
        }
    }
    if (CW->state == 5 || CW->state == 6) {
        if (CW->item != 0xAE) {
            Window_DrawText(&gCombineResultWindow, (s16 *)NULL, (u8 *)gCombineSuccessLabels);
            n = CW->unk82;
            cnt = Pouch_Count(p, CW->item);
            max = gItemData[CW->item].max;
        } else {
            Window_DrawText(&gCombineResultWindow, (s16 *)NULL, (u8 *)gCombineFailLabels[0]);
            n = 1;
            cnt = Pouch_Count(p, 0xAE);
            max = gItemData[0xAE].max;
        }
        Font_SetColor(0x807F7F7F);
        Font_SetPos(0x10E, gCombineResultWindow.y + 0x14);
        Text_Draw((char *)gItemNames[CW->item]);
        Font_SetPos(0x168, gCombineResultWindow.y + 0x28);
        Text_PrintfColored(D_0046CFD0, n);
        Font_SetPos(0x168, gCombineResultWindow.y + 0x3C);
        Text_PrintfColored(D_0046CFF8, cnt, max);
    }
}

void CombineMenu_DrawItemName(u8 slot, u16 item)
{
    u32 color;

    if ((slot & 0xFF) != 0xFF) {
        color = 0x807F7F7F;
    } else {
        color = 0x80404040;
    }
    Font_SetColor(color);
    Text_Draw((char *) ((s8 *) gItemNames[item & 0xFFFF]));
}

void SkillList_Build(Player *p, Player *q);
void Player_RecomputeStats(Player *p);

s32 PersonalStatus_Open(void)
{
    if (gScene.online != 0) {
        Player_RecomputeStats(&gPlayers[gScene.localPlayer]);
    }
    SkillList_Build(gHudState->player, NULL);
    gHudState->pageCursor = 0;
    gHelpShow = 0;
    return 0;
}

u8 SkillList_PageCount(void);

u16 PersonalStatus_Update(u16 buttons)
{
    u8 n = SkillList_PageCount() + 1;

    if (buttons & 0xC00) {
        if (buttons & 0x400) {
            (gHudState->pageCursor)++;
            if (gHudState->pageCursor > n) {
                gHudState->pageCursor = 0;
            }
        } else if (gHudState->pageCursor == 0) {
            gHudState->pageCursor = n;
        } else {
            gHudState->pageCursor -= 1;
        }
        Sound_PlaySeFlat(0, 0x11, 0);
    }
    return buttons & 0x8040;
}

void PersonalStatus_DrawPage(struct Player *p, u8 b);

void PersonalStatus_Draw(void)
{
    PersonalStatus_DrawPage(gHudState->player, gHudState->pageCursor);
}

/* The equipment in menu slot `slot` (0 the weapon, 1-5 the armour of types 2-5 and 0) copied to
 * `out`; NULL when the slot is empty. */
static EquipItem *Player_GetEquipBySlot(EquipItem *out, u8 slot, Player *p)
{
    EquipItem *e;

    out->used = out->level = 0;
    switch (slot) {
    case 0:
        *out = p->equip[6];
        out->used = 1;
        return out;
    case 1:
        out->type = 2;
        break;
    case 2:
        out->type = 3;
        break;
    case 3:
        out->type = 4;
        break;
    case 4:
        out->type = 5;
        break;
    case 5:
        out->type = 0;
        break;
    default:
        return NULL;
    }
    e = &p->equip[out->type];
    if (e->id == 0) {
        return NULL;
    }
    out->used = 1;
    out->id = e->id;
    out->level = e->level;
    out->deco[0] = e->deco[0];
    out->deco[1] = e->deco[1];
    out->deco[2] = e->deco[2];
    return out;
}

s32 EquipStatus_Open(void)
{
    gHudState->pageCursor = 0;
    gHudState->equipCursor = 0;
    gHelpShow = 0;
    return 0;
}

u8 Equip_LevelMark(EquipItem *a, u8 flags, EquipItem *b);

u16 EquipStatus_Update(u16 buttons)
{
    u8 cursor;
    EquipItem e;
    EquipItem *pe;
    Player *p;

    p = gHudState->player;
    cursor = gHudState->pageCursor;
    Menu_CursorUpDown(&cursor, buttons, 6);
    if (cursor != gHudState->pageCursor) {
        gHudState->pageCursor = cursor;
        gHudState->equipCursor = 0;
    }
    pe = Player_GetEquipBySlot(&e, gHudState->pageCursor, p);
    if (pe != NULL) {
        gHudState->equipCursorMax = Equip_LevelMark(&e, 0, 0);
        Menu_CursorLeftRight(&gHudState->equipCursor, buttons & 0xC00, gHudState->equipCursorMax);
    }
    return buttons;
}

extern u8 gOptionMenuCounts[];

s32 OptionMenu_Open(void *m)
{
    gHudState->pageCursor = 0;
    gHelpPage = 0xC;
    gHelpShow = 0;
    if (gScene.online != 0) {
        if (gScene.area == 0x83) {
            gHudState->optionKind = 0;
        } else {
            gHudState->optionKind = Game_IsOnline() == 1 ? 1 : 2;
        }
    } else {
        gHudState->optionKind = Game_IsOnline() == 1 ? 3 : 4;
    }
    gHudState->optionCount = gOptionMenuCounts[gHudState->optionKind];
    Shake_Start(9, 0, 280.0f);
    return 0;
}

void Vib_Start(s32 port, s32 on);
extern u8 *gOptionMenuRows[];

/* The options menu's work (at gHudState). */
#define OPT ((OptMenu *)gHudState)

/* The options menu (vibration and the other settings): the cursor, the changed option, then the
 * settings copied to the game; 0x8040 of the buttons returned (0x8000 while shaking out). */
s32 OptionMenu_Update(u16 buttons)
{
    u8 k;

    gHelpShow = 0;
    if (Shake_Flags(9) & 0x80) {
        return Shake_Update(9) == 0 ? 0x8000 : 0;
    }
    if (buttons & 0x8000) {
        Shake_Start(9, 0x80, 333.0f);
        Sound_PlaySeFlat(0, 0x14, 0);
        return 0;
    }
    gHelpShow = 1;
    if (Shake_Update(9) == 1) {
        buttons &= 0xC3BF;
        gHelpShow = 0;
    }
    Menu_CursorUpDown(&OPT->cursor, buttons, OPT->count);
    k = gOptionMenuRows[OPT->page][OPT->cursor];
    gHelpLine = k;
    if (buttons & 0xC00) {
        switch (k) {
        case 1:
            if (OPT->opts[0] != 0) {
                Sound_PlaySeFlat(0, 0x15, 0);
                break;
            }
        case 0:
        case 3:
        case 5:
        case 7:
        case 8:
            *(u8 *)((u8)k + (s32)gHudState + 0x9B) ^= 1;
            Sound_PlaySeFlat(0, 0x17, 0);
            break;
        case 2:
        case 4:
        case 6:
            if (buttons & 0x800) {
                u8 *p = (u8 *)(k + (s32)gHudState + 0x9B);

                if (*p == 0) {
                    *p = 2;
                } else {
                    *p = *p - 1;
                }
            } else {
                (*(u8 *)(k + (s32)gHudState + 0x9B))++;
                if (*(u8 *)(k + (s32)gHudState + 0x9B) > 2) {
                    *(u8 *)(k + (s32)gHudState + 0x9B) = 0;
                }
            }
            Sound_PlaySeFlat(0, 0x17, 0);
            break;
        }
    }
    gScene.aimReverse = gSysSave.options.aimReverse = OPT->opts[2];
    gScene.camControl = gSysSave.options.camControl = OPT->opts[4];
    gScene.camFollow = gSysSave.options.camFollow = OPT->opts[3];
    gSysSave.options.chatSize = OPT->opts[6];
    if (OPT->opts[5] != gSysSave.options.vibration) {
        gSysSave.options.vibration = OPT->opts[5];
        if (gSysSave.options.vibration) {
            Vib_Start(0, 1);
        }
    }
    gChatHideNames = OPT->opts[7];
    gSysSave.options.chatBeep = OPT->opts[8];
    return (u16)(buttons & 0x8040);
}


extern char *gFieldOptionLabels[][9];
extern char *gFieldOptionValues[][3];
extern char D_00448198[];
extern char *gOptionMenuTitle[];
extern ListPanel gOptionMenuPanel;
s32 Str_Length(char *s);

/* Draws the options menu: the list of the page options (gOptionMenuPanel), each option value text
 * centred at the right (option 1 while option 0 is set: D_00448198), and the cursor arrows
 * (grey on that option, pulsing otherwise). */
void OptionMenu_Draw(void)
{
    s16 pos[2];
    SpriteW s;
    char *list[12];
    s16 w;
    s16 y;
    u8 *rows;
    s16 k;
    s16 i;
    char **names;

    Shake_CopyOffset(9, (struct Pair *)pos);
    Gfx_SetBlend(4, 5);
    rows = gOptionMenuRows[OPT->page];
    names = gFieldOptionLabels[OPT->opts[0]];
    for (i = 0; i < OPT->count; i++) {
        list[i] = names[rows[i]];
    }
    list[i] = NULL;
    gOptionMenuPanel.items = list;
    gOptionMenuPanel.lines = OPT->count;
    ListPanel_Draw(&gOptionMenuPanel, pos, gOptionMenuTitle[0], -1);
    Font_SetColor(0x807F7F28);
    y = 0x55;
    for (i = 0; i < OPT->count; i++) {
        k = rows[i];
        if (OPT->opts[0] == 0 || k != 1) {
            w = Str_Length(gFieldOptionValues[k][OPT->opts[k]]);
            Font_SetPos(pos[0] + 0x229 - w * 9 / 2, y);
            Text_Draw(gFieldOptionValues[k][OPT->opts[k]]);
        } else {
            Font_SetPos(pos[0] + 0x20E, y);
            Text_Printf(D_00448198);
        }
        y += 22;
    }
    s.w = 0x12;
    s.y = OPT->cursor * 22 + 0x55;
    s.h = 0x12;
    if (gScene.online != 0 || OPT->opts[0] == 0 || rows[OPT->cursor] != 1) {
        s.color = (((s8)(48.0f * Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10))) + 0xAF) << 8) | 0xF0200020;
    } else {
        s.color = 0xF0707070;
    }
    s.x = pos[0] + 0x1EE;
    s.uv0 = 0x1A00A6;
    s.uv1 = 0x2E0094;
    Sprite_Draw((struct Sprite *)&s);
    s.x = pos[0] + 0x252;
    *(s16 *)&s.uv0 = 0x94;
    *(s16 *)&s.uv1 = 0xA6;
    Sprite_Draw((struct Sprite *)&s);
}
#undef OPT

s32 ItemGet_IsReady(Player *p)
{
    if (p != NULL && Player_IsLocal(p) == 0) {
        return 0;
    }
    if (gHudState->dropState != 0) {
        return 0;
    }
    return gItemGetWait <= 0;
}

s32 ItemGet_Reserve(s32 pl, u16 *gotItem)
{
    u16 serial;

    if ((pl != 0) && (Player_IsLocal((Player *) pl) == 0)) {
        return -1;
    }
    if (gHudState->dropState != 0) {
        return -1;
    }
    if (gItemGetWait <= 0) {
        serial = gItemGetSerial + 1;
        gItemGetSerial = serial;
        *gotItem = serial;
        gItemGetWait = 4;
        return 0;
    }
    if (*gotItem == gItemGetSerial) {
        gItemGetWait = 4;
        return 0;
    }
    return -1;
}

void QuestRecord_Count(u16 a);
s16 Pouch_GiveItems(Player *p, u16 item, s16 key, u8 flags, s16 n);

/* Adds one item to p's pouch (Pouch_GiveItems, whose result it returns), offline also
 * through QuestRecord_Count. */
s16 Pouch_GiveItem(Player *p, u16 item, s16 key, u8 flags)
{
    if (gScene.online == 0) {
        QuestRecord_Count(item);
    }
    return Pouch_GiveItems(p, item, key, flags, 1);
}

/* unit's own declaration: as the file's last declaration of it (u8 in pouch.c) */
s32 Pouch_Add(Player *p, u16 item, s16 n);
void Item_PlayUseSound(Player *p, u16 item);
u8 Notice_Push(u8 a, s16 b, s16 c);
u8 Item_CarryKind(u16 item);

/* Adds n of item to p's pouch (the result of Pouch_Add, or -1 while the state is busy or `key` is
 * not the current request); flags 1: messages, 2: the use sound. */
s16 Pouch_GiveItems(Player *p, u16 item, s16 key, u8 flags, s16 n)
{
    u16 r;

    if (gHudState->dropState != 0) {
        return -1;
    }
    if (gItemGetWait > 0 && key != gItemGetSerial) {
        return -1;
    }
    r = Pouch_Add(p, item, n);
    gHudState->dropItem = 0;
    switch (r) {
    case 0:
    case 1:
        if (flags & 2) {
            Item_PlayUseSound(p, item);
        }
        if (flags & 1) {
            Notice_Push(1, 0, item);
        }
        if (gScene.online != 0) {
            Save_StorePouch(p);
        }
        break;
    case 2:
    case 3:
        if (flags & 1) {
            Notice_Push(1, 3, item);
        }
        break;
    case 5:
        if (Item_CarryKind(item) != 0) {
            if (flags & 1) {
                Notice_Push(0, 0xC, 0);
            }
            return 5;
        }
        gHudState->dropItem = item;
        gHudState->dropState = 1;
        gHudState->dropHelpLine = item;
        gHudState->dropCount = n;
        if (flags & 2) {
            Item_PlayUseSound(p, item);
        }
        gHudState->dropNotice = 0xFF;
        break;
    }
    return r;
}

u16 ItemGet_Result(void)
{
    u8 dropState;

    dropState = gHudState->dropState;
    if ((dropState == 0) && (dropState == 6)) {
        return 0U;
    }
    return gHudState->dropItem;
}

s32 Notice_IsQueued(u8 id);
void Pouch_MoveCursor(u8 *cur, u16 buttons);
s16 Pouch_ClearSlot(Player *p, s16 slot);
/* unit's own declaration: with the definition's u8 (Player *p, u16 item, s16 n) (sub_main/pouch.c), Pouch_GiveItems: 1 of 132 words differ */
s32 Pouch_Add(Player *p, u16 item, s16 n);
u8 YesNo_Toggle(u16 buttons, u8 sel, u16 mask);

/* The pouch's item-drop dialog, a state machine at +7: 1 posts the notice for the item at +0x5A
 * and waits for it, 2-3 show the yes/no choice (+0x57), 4 picks a pouch slot (+0x51), 5 confirms
 * (+0xF) and swaps the item in, 6 waits for the closing shake. Returns 0 when it is done. */
s32 PouchDrop_Update(u16 buttons)
{
    u8 *state = &gHudState->dropState;

    switch (*state) {
    case 0:
        if (gItemGetWait != 0) {
            gItemGetWait--;
        }
        return 0;
    case 1:
        if (gHudState->dropNotice == 0xFF) {
            gHudState->dropNotice = Notice_Push(1, 0, (s16)gHudState->dropItem);
        } else if (gHudState->dropNotice == gScene.bannerNotice) {
            (*state)++;
            gHudState->dropBlink = 6;
            return 1;
        } else if (Notice_IsQueued(gHudState->dropNotice) == 0) {
            (gHudState->dropState)++;
            return 1;
        }
        return 0;
    case 2:
        if (gHudState->dropBlink != 0) {
            (gHudState->dropBlink)--;
        }
        if (gHudState->dropNotice == gScene.bannerNotice) {
            return 1;
        }
        gHudState->dropBlink = 0;
        gHudState->dropChoice = 0;
        (gHudState->dropState)++;
        Shake_Start(1, 0, 200.0f);
        /* fall through */
    case 3:
        if (Shake_Update(1) == 1) {
            buttons &= 0x3C00;
        }
        gHudState->dropBlink = (gHudState->dropBlink + 1) & 0x7F;
        if (gHudState->dropBlink < 0x40) {
            gHudState->dropHelpLine = 0x14;
        } else {
            gHudState->dropHelpLine = gHudState->dropItem + 0x18;
        }
        Pouch_MoveCursor(&gHudState->slot, buttons & 0xC00);
        if (buttons & 0x3000) {
            gHudState->dropChoice = (gHudState->dropChoice + 1) & 1;
            Sound_PlaySeFlat(0, 0x17, 0);
        }
        if (buttons & 0x20) {
            switch (gHudState->dropChoice) {
            case 0:
                gHudState->dropHelpLine = 0x15;
                gHudState->dropShowSlot = 0;
                (gHudState->dropState)++;
                break;
            case 1:
                gHudState->dropState = 6;
                Shake_Start(1, 0x80, 180.0f);
                gHudState->dropItem = 0;
                break;
            }
            Sound_PlaySeFlat(0, 0x13, 0);
        }
        break;
    case 4:
        if (gHudState->dropShowSlot != 0) {
            if (buttons & 0x240) {
                gHudState->dropShowSlot = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
            }
            buttons &= ~0x40;
        } else if (buttons & 0x200) {
            gHudState->dropShowSlot = 1;
            Sound_PlaySeFlat(0, 9, 0);
        }
        Pouch_MoveCursor(&gHudState->slot, buttons);
        if (buttons & 0x20) {
            gHudState->dropHelpLine = 0x16;
            gHudState->dropShowSlot = 0;
            gHudState->yesNo = 1;
            (gHudState->dropState)++;
            Sound_PlaySeFlat(0, 0x13, 0);
        } else if (buttons & 0x40) {
            gHudState->dropHelpLine = 0x14;
            gHudState->dropBlink = 0;
            gHudState->dropState = 3;
            Sound_PlaySeFlat(0, 0x14, 0);
        } else if (gHudState->dropShowSlot == 0) {
            gHudState->dropHelpLine = 0x15;
        } else {
            gHudState->dropHelpLine = (gHudState->player)->pouch[gHudState->slot].item + 0x18;
        }
        break;
    case 5:
        gHudState->dropHelpLine = 0x16;
        gHudState->yesNo = YesNo_Toggle(buttons, gHudState->yesNo, 0x3000);
        if (buttons & 0x20) {
            if (gHudState->yesNo == 0) {
                Pouch_ClearSlot(gHudState->player, gHudState->slot);
                Pouch_Add(gHudState->player, gHudState->dropItem, gHudState->dropCount);
                gHudState->dropItem = 0;
                if (gScene.online != 0) {
                    Save_StorePouch(gHudState->player);
                }
                gHudState->dropState = 6;
                Sound_PlaySeFlat(0, 0x13, 0);
                Shake_Start(1, 0x80, 180.0f);
            } else {
                gHudState->dropState = 4;
                Sound_PlaySeFlat(0, 0x14, 0);
            }
        } else if (buttons & 0x40) {
            u8 *sel = &gHudState->yesNo;

            if (*sel == 0) {
                *sel = 1;
                Sound_PlaySeFlat(0, 0x17, 0);
            } else {
                gHudState->dropState = 4;
                Sound_PlaySeFlat(0, 0x14, 0);
            }
        }
        break;
    case 6:
        if (Shake_Update(1) == 0) {
            gHudState->dropState = 0;
            return 0;
        }
        break;
    }
    return 1;
}

s32 PouchDrop_UpdateSave(u16 buttons)
{
    s32 ret;

    ret = PouchDrop_Update(buttons);
    if (ret != 0) {
        Save_StorePouch(gHudState->player);
    }
    return ret;
}

void Hud_MapZoomInput(u16 btn, u16 unused)
{
    if ((gHudState->onlineHud == 0) && (gHudState->hidden == 0) && (btn & 0xFFFF & 0x4000)) {
        gHudState->mapZoomed = (u8) (gHudState->mapZoomed ^ 1);
    }
}

void QSync_SendState(u8 a, u8 b); /* gm_sub QSync_SendState */

void Hud_UpdateMapPing(u16 buttons)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        if (gHudState->pingTimer[i] >= 0) {
            if (gHudState->pingTimer[i] <= 99 && gHudState->pingTimer[i] % 20 == 0) {
                Sound_PlaySeFlat(0, gHudState->pingTimer[i] / 20 + 0x1B, 0);
            }
            gHudState->pingTimer[i]++;
            if (gHudState->pingTimer[i] > 105) {
                gHudState->pingTimer[i] = -1;
            }
        }
    }
    if (gHudState->pingTimer[gScene.localPlayer] < 0 && (buttons & 2)) {
        (gHudState->pingHold)--;
        if (gHudState->pingHold <= 0) {
            gHudState->pingTimer[gScene.localPlayer] = 0;
            QSync_SendState(9, gScene.localPlayer);
        }
        return;
    }
    gHudState->pingHold = 10;
}

void Hud_StartMapPing(s32 i)
{
    gHudState->pingTimer[i] = 0;
}

extern char D_0046D0C8[];
extern u32 gPlayerNameColors[];
extern char *gSideNames[];
s32 Obj_InCurrentArea(void *obj);
f32 Player_NameTagRaise(Player *p);
void Camera_ProjectPoint(f32 *out, f32 *pos);
void Mtx_Identity(f32 *m);

/* The other hunters' names above their heads (in their team colour, with the team name, in
 * versus quests). */
void Hud_DrawPlayerNames(void)
{
    char buf[32];
    f32 scr[4];
    f32 m[16];
    f32 pos[3];
    Player *p;
    char *name;
    s16 i;

    Font_SetSpacing(0x10, 0x10);
    Font_SetColor(0x807F7F7F);
    for (p = gPlayers, i = 0; i < 4; i++, p++) {
        if (p->index == gScene.localPlayer) {
            continue;
        }
        if (gScene.inQuest[i] != 1) {
            continue;
        }
        if (p->active == 0 || p->visible == 0) {
            continue;
        }
        if (!(u8)Obj_InCurrentArea(p)) {
            continue;
        }
        Mtx_Identity(m);
        Gs_SetState(0x1A, (s32)m);
        pos[0] = p->pos[0];
        pos[1] = 192.0f + p->pos[1] + Player_NameTagRaise(p);
        pos[2] = p->pos[2];
        Camera_ProjectPoint(scr, pos);
        if (scr[0] < 700.0f && !(scr[0] <= -60.0f) && scr[1] < 500.0f && !(scr[1] <= -20.0f) && !(scr[3] <= 0.0f)) {
            name = (char *)p->name;
            if (Game_IsOnline() == 1 && gChatHideNames != 0) {
                name = gPlayerIds[p->index];
            }
            if (Quest_IsVersus() == 1) {
                Font_SetColor(gPlayerNameColors[p->side]);
                sprintf(buf, D_0046D0C8, gSideNames[p->side], name);
                name = buf;
            }
            scr[0] -= ((s16)Str_Length(name) / 2) * 8;
            Font_SetPos(1.25f * scr[0], scr[1]);
            Text_Draw(name);
        }
    }
}

extern u8 gQuestExtra[];
Em *Em_FindOnSide(u8 side);
s32 Em_IsHiddenFromHud(Em *e);
void Hud_DrawVersusMonsterBar(Em *e, s16 x, s16 y, u32 color, u8 *name);

/* In versus quests, the two sides' monsters' health bars above them (green, yellow, red,
 * blinking under a fifth), named after their quest entries (gQuestExtra + 0x12). */
void Hud_DrawVersusMonsterBars(void)
{
    f32 scr[4];
    f32 m[16];
    f32 pos[3];
    Em *e;
    s32 i;
    s16 x;
    s16 y;
    u8 *n;
    u32 color;
    f32 r;

    if (Quest_IsVersus() != 1) {
        return;
    }
    Gfx_SetState63(1);
    Gfx_SetBlend(4, 5);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    for (i = 0, n = gQuestExtra; i < 2; i++, n += 0x1C) {
        if ((e = Em_FindOnSide(i + 1)) == NULL) {
            continue;
        }
        if (Em_IsHiddenFromHud(e) == 1) {
            continue;
        }
        Mtx_Identity(m);
        Gs_SetState(0x1A, (s32)m);
        pos[0] = e->posX;
        pos[1] = 450.0f + e->posY;
        pos[2] = e->posZ;
        Camera_ProjectPoint(scr, pos);
        if (scr[0] < 552.0f && !(scr[0] <= -40.0f) && scr[1] < 448.0f && !(scr[1] <= -30.0f) && !(scr[3] <= 0.0f)) {
            x = 640.0f * scr[0] / 512.0f - 52.0f;
            y = scr[1];
            r = (f32)e->hp / (f32)e->hpMax;
            if (!(r <= 0.6666667f)) {
                color = 0xFF00FF20;
            } else if (!(r <= 0.33333334f)) {
                color = 0xFFFFFF00;
            } else {
                color = 0xFFFF0000;
                if (r < 0.2f) {
                    color = ((s8)(112.0f * Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10))) + 0x8F) << 16 | 0xFF000000;
                }
            }
            Hud_DrawVersusMonsterBar(e, x, y, color, n + 0x12);
        }
    }
}

typedef struct Vec2s {
    s16 x;
    s16 y;
} Vec2s;

typedef struct SpriteUV {
    s16 x, y, w, h;
    u32 color;
    Vec2s uv0;
    Vec2s uv1;
} SpriteUV;

extern u32 gBlockGaugeSizes[];
extern SpriteUV D_00430A34[];
extern SpriteUV D_00430A36[];
extern SpriteUV D_00430A3C[];
extern u32 gBlockGaugeUVs[];

/* full is an s16: as an s32 it took s3 and the field pointers moved down. */
/* A gauge of v (up to 90) at x, y: a block (((SpriteUV *)gBlockGaugeSizes)) per 30, then a part of the next block
 * per 5 more. */
void Hud_DrawBlockGauge(s16 x, s16 y, u16 v, u32 color)
{
    SpriteUV s;
    s32 i;
    s16 rest;
    s16 full;
    s16 k;

    if (v > 90) {
        v = 90;
    }
    full = (s16)(v / 30);
    s.x = x + 10;
    s.y = y - 5;
    rest = v % 30;
    for (i = 0; i < full; i++) {
        s.w = ((SpriteUV *)gBlockGaugeSizes)[i].w;
        s.h = ((SpriteUV *)gBlockGaugeSizes)[i].h;
        s.color = color;
        s.uv0 = ((SpriteUV *)gBlockGaugeSizes)[i].uv0;
        s.uv1 = ((SpriteUV *)gBlockGaugeSizes)[i].uv1;
        s.y -= s.h;
        Sprite_Draw((struct Sprite *)&s);
        s.x += s.w;
        s.y += s.h;
    }
    k = rest / 10;
    if (k != 0) {
        s.w = D_00430A34[full].x;
        s.h = D_00430A36[full].x;
        s.color = color;
        s.uv0 = *(Vec2s *)&D_00430A3C[full].x;
        s.uv1 = *(Vec2s *)&((SpriteUV *)gBlockGaugeUVs)[full].x;
        s.w = s.w * k / 3;
        s.uv1.x = s.uv0.x + k * (s.uv1.x - s.uv0.x) / 3;
        s.y -= s.h;
        Sprite_Draw((struct Sprite *)&s);
    }
}

void Hud_DrawBlockGauge(s16 x, s16 y, u16 v, u32 color);
void Sprite_DrawStretch9(Sprite *src, s16 w, s16 h, s16 du, s16 dv);

/* The health bar box of versus monster e at (x, y): the frame, the bar in `color`, the anger
 * gauge (Hud_DrawBlockGauge: e[0x9A0], or blinking from e+0x808 while e[0x9A1] is set) and the side's
 * name with `name`. */
void Hud_DrawVersusMonsterBar(Em *e, s16 x, s16 y, u32 color, u8 *name)
{
    Sprite s;
    char buf[0x20];
    s16 n;
    f32 c;

    s.x = x - 10;
    s.y = y - 2;
    s.w = 0x7A;
    s.h = 0x14;
    *(s32 *)&s.u0 = 0x700038;
    *(s32 *)&s.u1 = 0x8D0056;
    s.color = 0xC0000000;
    Sprite_DrawStretch9(&s, 8, 8, 8, 8);
    s.color = color;
    s.h = 0x16;
    s.x = x - 0x16;
    s.w = 0x19;
    *(s32 *)&s.u0 = 0x770000;
    *(s32 *)&s.u1 = 0x8D0019;
    Sprite_Draw((struct Sprite *)&s);
    s.x += s.w;
    s.w = 0x64;
    s.u0 = 0x19;
    s.u1 = 0x23;
    Sprite_Draw((struct Sprite *)&s);
    s.x += s.w;
    s.w = 0xB;
    s.u0 = 0x23;
    s.u1 = 0x36;
    Sprite_Draw((struct Sprite *)&s);
    s.x = x - 0x14;
    s.y = y - 0xD;
    s.w = 0x1A;
    s.h = 0xA;
    *(s32 *)&s.u0 = 0x8F0016;
    *(s32 *)&s.u1 = 0x990030;
    Sprite_Draw((struct Sprite *)&s);
    if (e->rageStarted == 0) {
        if (e->versusGauge != 0xFF && e->versusGauge != 0) {
            Hud_DrawBlockGauge(x, y, e->versusGauge, 0xFFFFA040);
        }
    } else {
        n = (e->rageTimer - 450) / 30;
        if (n > 0) {
            c = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x1F) << 11));
            Hud_DrawBlockGauge(x, y, n, (((s8)(80.0f * c) + 0x50) << 8) | 0xFFFF0000 | ((s8)(32.0f * c) + 0x20));
        }
    }
    Font_SetSpacing(0x10, 0x10);
    Font_SetPos(x, y);
    Font_SetColor((u8)e->side == 1 ? 0x802B7F7F : 0x807F212E);
    sprintf(buf, D_0046D0C8, gSideNames[(u8)e->side], name);
    Text_Draw(buf);
}

extern char *gItemActionLabels[][3];
extern ListPanel gPouchMenuPanel;
extern ListPanel gPouchMenuPanel2;
void Menu_PlayerNameList(void);
extern char *gYesNoTexts[];
void ItemList_DrawPouch(s32 a, s32 b, s32 flags);
void Menu_DrawListCursor(s16 *off, s32 row, u32 color);
s32 PouchMenu_CanTransfer(Player *p);

/* Draws the pouch page for its state +0x50 (0: the list, with the page title when +0xA6 is set;
 * 1: the use/throw choice; 2-3: the sort and the others list; 4: no list) and marks the cursor's
 * row (+0x51) when state 3 shows the same page as +0xA7. */
void PouchMenu_Draw(void)
{
    Player *p;
    u32 color;
    s32 flags;
    s32 k;
    u8 v;
    s32 cur;
    u8 *sel;

    p = gHudState->player;
    v = gHudState->slot;
    switch (gHudState->state) {
    case 0:
        if (gHudState->sorting != 0) {
            flags = 0x10;
            color = 0;
            Font_SetSpacing(0x12, 0x12);
            Font_SetPos(0x1B0, 0x18E);
            Text_Printf(gYesNoTexts[gHudState->yesNo]);
        } else {
            if (gHudState->camReqStarted != 0) {
                flags = 0x88;
            } else {
                flags = 0xA8;
            }
            color = 0xA9182;
        }
        break;
    case 1:
        flags = 0;
        k = 2;
        if (Player_IsFree(p) == 1) {
            k = PouchMenu_CanTransfer(p) != 1;
        }
        gPouchMenuPanel.items = gItemActionLabels[k];
        gPouchMenuPanel.cursorColor = 0xA9182;
        ListPanel_Draw(&gPouchMenuPanel, NULL, NULL, gHudState->action);
        color = 0x808080;
        break;
    case 2:
    case 3:
        flags = 0;
        color = 0x808080;
        sel = &gHudState->action;
        switch (*sel) {
        case 0:
            gPouchMenuPanel.cursorColor = color;
            ListPanel_Draw(&gPouchMenuPanel, NULL, NULL, *sel);
            ListPanel_Draw(&gPouchMenuPanel2, NULL, NULL, gHudState->yesNo);
            break;
        case 1:
            Menu_PlayerNameList();
            break;
        case 2:
            gPouchMenuPanel.cursorColor = color;
            ListPanel_Draw(&gPouchMenuPanel, NULL, NULL, *sel);
            color = 0xA9182;
            v = gHudState->dest;
            flags = 0x48;
            break;
        }
        break;
    case 4:
        flags = 0x80;
        color = 0;
        break;
    }
    ItemList_DrawPouch(v, color, flags);
    if (flags & 0x40) {
        cur = gHudState->slot;
        if ((u8)(cur / 10) == gHudState->dest / 10) {
            Menu_DrawListCursor(NULL, cur - (u8)(cur / 10) * 10, 0x400000);
        }
    }
}

extern char D_0046D0D0[];
extern Panel gPlayerNameCountBox;
void Menu_DrawArrow(s16 x, s16 y, s16 w, s16 h, u32 color, u8 flags);

void PlayerNameList_DrawCount(void)
{
    char buf[32];

    sprintf(buf, D_0046D0D0, gHudState->count);
    Window_DrawText(&gPlayerNameCountBox, (s16 *)NULL, (u8 *)buf);
    Gfx_SetState63(1);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    if (gHudState->atMax == 0) {
        Menu_DrawArrow(0x18A, 0x3E, 0x18, 0x12, 0xFF20FF28, 0);
    }
    if (gHudState->count != 1) {
        Menu_DrawArrow(0x14C, 0x3E, 0x18, 0x12, 0xFF20FF28, 1);
    }
}

extern Panel gDeliveryCountBox;

void Delivery_DrawCount(void)
{
    char buf[32];

    sprintf(buf, D_0046D0D0, gHudState->count);
    Window_DrawText(&gDeliveryCountBox, (s16 *)NULL, (u8 *)buf);
    Gfx_SetState63(1);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    if (gHudState->atMax != gHudState->count) {
        Menu_DrawArrow(0x144, 0x50, 0x18, 0x12, 0xFF20FF28, 0);
    }
    if (gHudState->count != 1) {
        Menu_DrawArrow(0x106, 0x50, 0x18, 0x12, 0xFF20FF28, 1);
    }
}

extern char *gEmptyNameText;
extern ListPanel gPlayerNameListPanels[];
extern u8 gNetUsers[];
s32 NetUser_State(s8 i);
void PlayerNameList_DrawCount(void);
void Text_DrawLinesColored(s16 x, s16 y, char **names, s16 h);

/* The other players' names list (state +0x53: 0 builds it, PlayerNameList_DrawCount; 1 draws it), from
 * gPlayers offline or the lobby's members online; the cursor +0x54 skips the local player. */
void Menu_PlayerNameList(void)
{
    char *names[12];
    s32 i;
    s32 n;
    u8 *e;
    char **mp;
    s32 j;
    s32 m;
    s32 k;

    switch (gHudState->step) {
    case 1:
        if (gScene.online == 0) {
            for (i = 0, n = 0; i < 4; i++) {
                if (gScene.localPlayer == i) {
                    continue;
                }
                if (gScene.inQuest[i] == 1) {
                    names[n] = (char *)gPlayers[i].name;
                    if (Game_IsOnline() == 1 && gChatHideNames != 0) {
                        names[n] = gPlayerIds[i];
                    }
                } else {
                    names[n] = gEmptyNameText;
                }
                n++;
            }
            names[n] = NULL;
        } else {
            for (j = 0, m = 0, e = gNetUsers, mp = names; j < 10; j++, e += 0x11C) {
                if (gScene.localPlayer == j) {
                    continue;
                }
                switch (NetUser_State(j)) {
                case 0:
                    *mp = (char *)(e + 8);
                    if (Game_IsOnline() == 1 && gChatHideNames != 0) {
                        *mp = (char *)e;
                    }
                    break;
                case 1:
                case 2:
                    *mp = gEmptyNameText;
                    break;
                }
                mp++;
                m++;
            }
            names[m] = NULL;
        }
        k = gHudState->target;
        if (gScene.localPlayer < k) {
            k--;
        }
        ListPanel_Draw(&gPlayerNameListPanels[gScene.online], NULL, NULL, k);
        Font_SetColor(0x807F7F7F);
        Font_SetSpacing(0x12, 0x12);
        Text_DrawLinesColored(0x154, 0x3E, names, 0x15);
        break;
    case 0:
        PlayerNameList_DrawCount();
        break;
    }
}

struct SortPair;
void Sort_PairsDesc(struct SortPair *list, u32 n);
void ItemList_Draw(ItemStack *list, s32 a, s32 b, s32 flags);

void ItemList_DrawPouch(s32 a, s32 b, s32 flags)
{
    ItemStack list[20];
    ItemStack *src;
    ItemStack *dst;
    s32 i;
    s32 n;

    if (!(flags & 2)) {
        src = (gHudState->player)->pouch;
    } else {
        src = gSave.pouch;
    }
    if (flags & 0x10) {
        dst = list;
        n = 0;
        i = 20;
        do {
            if (src->item != 0) {
                n++;
                *dst = *src;
                dst++;
            }
            src++;
        } while (--i != 0);
        for (i = 20 - n; i != 0; i--) {
            dst->count = 0;
            dst->item = 0;
            dst++;
        }
        Sort_PairsDesc((struct SortPair *)list, n);
        src = list;
        a %= 10;
        b = 0;
        flags &= 0x11;
    }
    ItemList_Draw(src, a, b, flags);
}

extern ListPanel gItemListBox;
extern Panel gItemListTextBox;
extern char *gItemListTitles[];
extern char *gItemListTitleFmt[];
extern char *gItemListEmptyText;
extern char *gInfinityText[];
s32 Combine_ItemHasRecipeIn(s16 item, u8 flag);
void ButtonHint_DrawSort(u8 i);

/* Draws a page of 10 item stacks of list (the page of entry a) on the list panel, the cursor on a when b is set. The
 * copy list of list_in is needed for the registers. */
void ItemList_Draw(ItemStack *list_in, s32 a, s32 b, s32 flags)
{
    s16 y;
    s16 cur;
    s16 zero[2];
    s16 *pos;
    s16 x;
    s16 base;
    u32 color;
    ItemStack *list = list_in;
    s32 usable;
    char buf[0x2C];
    s32 mark;
    s16 k;
    s16 page;
    s32 x2;
    s16 i;

    page = a / 10;
    base = page * 10;
    Gfx_SetBlend(4, 5);
    if (b != 0) {
        gItemListBox.cursorColor = b;
        cur = a - base;
    } else {
        cur = -1;
    }
    pos = NULL;
    if (flags & 0x80) {
        if (Shake_Value(1) == 1) {
            pos = Shake_GetOffset(1);
            flags &= ~0x20;
        }
    }
    if (pos == NULL) {
        zero[1] = 0;
        zero[0] = 0;
        pos = zero;
    }
    sprintf(buf, gItemListTitleFmt[0], gItemListTitles[flags & 1], page + 1);
    ListPanel_Draw(&gItemListBox, pos, buf, cur);
    if (flags & 8) {
        Menu_DrawFrame(&gItemListBox, pos);
    }
    Gfx_SetState63(0);
    Font_SetSpacing(0x12, 0x12);
    i = 10;
    x = pos[0] + 0x1AF;
    list += base;
    y = pos[1] + 0x3E;
    x2 = x + 0x9E;
    usable = flags & 4;
    mark = flags & 0x40;
    for (; i > 0; i--, list++, base++) {
        y += 0x15;
        Font_SetPos(x, y);
        Font_SetColor(0x807F7F7F);
        color = 0x807F212E;
        if (usable) {
            if ((u8)CombineMenu_IsReady() == 0) {
                if (Combine_ItemHasRecipeIn((s16)list->item, gHudState->combineThree) == 0) {
                    Font_SetColor(0x80404040);
                    color = 0x80401017;
                } else if (CombineMenu_IsPicked((u16)base) == 1) {
                    Font_SetColor(0x807F7F28);
                }
            } else {
                k = CombineMenu_ResultRoom(list);
                if (k >= 0) {
                    switch (k) {
                    case 0:
                        Font_SetColor(0x807F212E);
                        break;
                    case 1:
                        Font_SetColor(0x807F5230);
                        break;
                    }
                } else if (CombineMenu_IsPicked((u16)base) == 1) {
                    Font_SetColor(0x807F7F28);
                } else {
                    Font_SetColor(0x80404040);
                    color = 0x80401017;
                }
            }
        } else if (mark) {
            if (Menu_IsNotCursorSlot((u8)base) != 1) {
                Font_SetColor(0x80404040);
                color = 0x80401017;
            }
        }
        if (list->item == 0) {
            Text_Draw(gItemListEmptyText);
        } else {
            Text_Draw((char *)gItemNames[list->item]);
            if (gItemData[list->item].max > 1) {
                Font_SetPos(x2, y);
                if (gItemData[list->item].max == 0xFF) {
                    Text_Draw(gInfinityText[0]);
                } else {
                    if (list->count >= gItemData[list->item].max) {
                        Font_SetColor(color);
                    }
                    Text_PrintfColored(D_0046CFF0, list->count);
                }
            }
        }
    }
    if (flags & 0x20) {
        Window_DrawText(&gItemListTextBox, (s16 *)NULL, (u8 *)NULL);
        ButtonHint_DrawSort(1);
    }
}

s32 Prim_FillRect(struct PrimLine *r);
extern ListPanel gItemListBox;

/* The blinking cursor frame around row `row` of the list box gItemListBox (moved by off). */
void Menu_DrawListCursor(s16 *off, s32 row, u32 color)
{
    ListPanel *b = (ListPanel *)&gItemListBox; /* the cast keeps the address in a register (not %lo per read) */
    Line l;
    s16 x;
    s16 y;
    s16 h;

    x = b->x;
    y = b->y;
    if (off != NULL) {
        x += off[0];
        y += off[1];
    }
    h = b->charH + 4;
    l.x0 = x - 2;
    l.x1 = x + b->charW * b->lineChars + 1;
    l.y0 = y + h * (row + 1) - 1;
    l.y1 = l.y0 + h + 2;
    l.color = color | ((((s8)(48.0f * Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10)))) + 0xBF) << 24);
    Prim_FillRect((struct PrimLine *)&l);
}

s32 func_786E50(Em *em);

/* A pulsing colour for the monster p: grey when its stateGroup is 5 or func_786E50 holds, else by
 * gHudState+0xAA (below 2) and p->combat (1). */
u32 Map_MonsterColor(Em *p)
{
    f32 c;
    u8 g;

    if (p->stateGroup == 5 || func_786E50(p) == 1) {
        g = (s8)(32.0f * Math_Sin(0.0000958738f * (u16)(*(u8 *)&gFrameTick << 8))) + 0x80;
        return g | ((g << 16) | 0xC0000000 | (g << 8));
    }
    if (gHudState->unkAA <= 1) {
        c = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10));
        return ((((s8)(54.0f * c) + 0x53) << 8) | 0xFFFA0000) | ((s8)(6.0f * c) + 0xDD);
    }
    if (p->combat == 1) {
        c = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0xF) << 12));
        return (((s8)(64.0f * c) + 0x48) << 8) | 0xFFF00000;
    }
    c = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10));
    return (((s8)(48.0f * c) + 0x80) << 8) | 0xFF1000E0;
}

void Map_DrawDot(u32 color, f32 x, f32 y, f32 r)
{
    SpriteW s;

    Gfx_SetState63(1);
    s.x = x - r;
    s.y = y - r;
    s.w = r + r;
    s.h = r + r;
    s.color = color;
    s.uv0 = 0xF00010;
    s.uv1 = 0x1000020;
    Sprite_Draw((struct Sprite *)&s);
}

void Map_DrawMarker(f32 x, f32 y)
{
    SpriteW s;
    f32 r = 8.0f;

    Tex_Prepare(1, 0x145);
    Tex_Bind(0x145);
    Gfx_SetState63(0);
    s.x = x - r;
    s.y = y - r;
    s.w = r + r;
    s.h = r + r;
    s.color = 0xFFFFFFFF;
    s.uv0 = 0xF00020;
    s.uv1 = 0xFF002F;
    Sprite_Draw((struct Sprite *)&s);
}

s32 Em_IsHiddenFromHud(Em *e)
{
    u8 kind;

    if (e->stateArmor != 0) {
        return 1;
    }
    kind = e->type;
    switch (kind) {
    case 0x1A:
    case 0x2C:
    case 0xE:
    case 0x2B:
        return e->stance == 4;
    case 0x33:
    case 0x3D:
        return EmBlangonga_InStateArmor(e);
    default:
        return 0;
    }
}

s32 Map_IsMonsterHidden(Em *e)
{
    u8 kind;

    kind = e->type;
    if (kind == 0x3B) {
        return 1;
    }
    if ((s32) gHudState->unkAA >= 4) {
        return 0;
    }
    if (e->stateArmor != 0) {
        return 1;
    }
    switch (kind) {
    case 0xF:
    case 0x2D:
        return (s8)e->ceilingTimer != 0;
    case 0x16:
        return func_770310();
    case 0x1A:
    case 0x2C:
    case 0xE:
    case 0x2B:
        return e->stance == 4;
    case 0x33:
    case 0x3D:
        return EmBlangonga_InStateArmor(e);
    default:
        return 0;
    }
}

void Mtx_Identity(f32 *m);
void Mtx_SetRotZYX(f32 *m, f32 x, f32 y, f32 z);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
s32 Prim_DrawTexTri(struct PrimTexTri *t);
extern u8 gMapIconKinds[];
extern u32 gMapMonsterIconColors[];
extern u8 gMapMonsterIconU[];

/* The map icon of monster e at (x, y) (scale s): a dot (Map_DrawDot) for kinds with bit 7 in
 * gMapIconKinds, else its icon turned by its heading, coloured by Map_MonsterColor. */
void Map_DrawMonsterIcon(Em *e, f32 x, f32 y, f32 s)
{
    Tri t;
    f32 m[16];
    f32 p[4][3];
    f32 v[3];
    u32 c;
    f32 h;
    f32 r;
    s16 i;
    u16 ang;
    s16 u;
    s16 u2;
    u8 id;

    id = gMapIconKinds[e->type];
    if (id == 0xFF) {
        return;
    }
    if (id & 0x80) {
        if (id & 0x40) {
            r = 5.0f * s;
            c = Map_MonsterColor(e);
        } else {
            r = 3.0f;
            c = gMapMonsterIconColors[id & 0xF];
        }
        Map_DrawDot(c, x, y, r);
        return;
    }
    if (Map_IsMonsterHidden(e) == 1) {
        return;
    }
    Gfx_SetState63(0);
    if (gHudState->unkAA <= 1) {
        id = 7;
        ang = 0;
        s *= 0.8f;
    } else {
        if (e->stance != 2) {
            id += 8;
        }
        ang = 0x18000 - e->rotY;
    }
    u = gMapMonsterIconU[id];
    u2 = u + 0x10;
    Mtx_Identity(m);
    Mtx_SetRotZYX(m, 0.0f, 0.0f, 6.2831855f * ang / 65536.0f);
    v[2] = 0.0f;
    h = 12.8f * s;
    for (i = 0; i < 4; i++) {
        v[0] = -h + 2.0f * h * (i / 2);
        v[1] = -h + 2.0f * h * (i & 1);
        Vec_Rotate(p[i], v, m);
        p[i][0] += x;
        p[i][1] += y;
    }
    t.x0 = p[0][0];
    t.y0 = p[0][1];
    t.x1 = p[1][0];
    t.y1 = p[1][1];
    t.x2 = p[2][0];
    t.y2 = p[2][1];
    t.u0 = u;
    t.v0 = 0xF0;
    t.v1 = 0x100;
    t.v2 = 0xF0;
    t.u1 = u;
    t.u2 = u2;
    t.color = Map_MonsterColor(e);
    Prim_DrawTexTri((struct PrimTexTri *)&t);
    t.x0 = p[3][0];
    t.y0 = p[3][1];
    t.u0 = u2;
    t.v0 = 0x100;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
}

extern u32 gPlayerMapColors[];
void Map_DrawPingGlow(s16 x, s16 y, s16 n, s32 c);

/* The map icon of player p at (x, y) (scale s), turned by its heading (the local player's own
 * icon, the others' in their colour gPlayerMapColors), flashing on its counter gHudState+0x3C (shown
 * by Map_DrawPingGlow; a quarter of it when p is hidden, +0x4B8 bit 11). */
void Map_DrawPlayerIcon(Player *p, f32 x, f32 y, f32 s)
{
    Tri t;
    f32 m[16];
    f32 pts[4][3];
    f32 v[3];
    f32 h;
    s32 hidden;
    s32 k;
    s16 i;
    s16 u;
    s16 u2;

    hidden = (p->statusIcons & 0x800) != 0;
    if (p->index != gScene.localPlayer && hidden == 1) {
        return;
    }
    Gfx_SetState63(0);
    Gfx_SetBlend(4, 5);
    Tex_Prepare(1, 0x145);
    Tex_Bind(0x145);
    if (p->index == gScene.localPlayer) {
        u = 0;
    } else {
        u = 0x50;
    }
    u2 = u + 0xF;
    Mtx_Identity(m);
    Mtx_SetRotZYX(m, 0.0f, 0.0f, 6.2831855f * (u16)(0x18000 - p->rot[1]) / 65536.0f);
    v[2] = 0.0f;
    h = 9.0f;
    for (i = 0; i < 4; i++) {
        v[0] = -h + 2.0f * h * (i / 2);
        v[1] = -h + 2.0f * h * (i & 1);
        Vec_Rotate(pts[i], v, m);
        pts[i][0] = x + s * pts[i][0];
        pts[i][1] = y + s * pts[i][1];
    }
    t.color = gPlayerMapColors[p->index & 3];
    k = gHudState->pingTimer[p->index];
    if (k >= 0 && k <= 99 && k % 20 == 0) {
        t.color = 0xFFFFFFFF;
    }
    t.x0 = pts[0][0];
    t.y0 = pts[0][1];
    t.x1 = pts[1][0];
    t.y1 = pts[1][1];
    t.x2 = pts[2][0];
    t.y2 = pts[2][1];
    t.u0 = u;
    t.v0 = 0xF0;
    t.v1 = 0xFF;
    t.v2 = 0xF0;
    t.u1 = u;
    t.u2 = u2;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
    t.x0 = pts[3][0];
    t.y0 = pts[3][1];
    t.u0 = u2;
    t.v0 = 0xFF;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
    if (gHudState->pingTimer[p->index] >= 0) {
        Gfx_SetState63(1);
        Gfx_SetBlend(4, 1);
        Tex_Prepare(1, 0x144);
        Tex_Bind(0x144);
        if (hidden != 1) {
            Map_DrawPingGlow(x, y, gHudState->pingTimer[p->index], gPlayerMapColors[p->index & 3]);
        } else {
            Map_DrawPingGlow(x, y, gHudState->pingTimer[p->index] / 4, gPlayerMapColors[p->index & 3]);
        }
    }
}

void Map_DrawPingGlow(s16 x, s16 y, s16 n, s32 c)
{
    s16 m;
    s32 k;

    if (n <= 19) {
        Hud_DrawGlow(x, y, c, 40.0f, 25 - n);
    } else if (n <= 84) {
        m = n % 20;
        Hud_DrawGlow(x, y, c, 40.0f, 25 - m);
        k = 5 - m;
        if (k >= 0) {
            Hud_DrawGlow(x, y, c, 40.0f, k);
        }
    } else {
        Hud_DrawGlow(x, y, c, 40.0f, 105 - n);
    }
}

/* The map part of the work area (gHudState). */
typedef struct MapWork {
    u8 pad00[8];
    Player *player;     /* 0x08 */
    u8 pad0C[0x34 - 0xC];
    f32 scaleX;         /* 0x34: world to map, x */
    f32 scaleZ;         /* 0x38: world to map, z */
    u8 pad3C[0xAA - 0x3C];
    s8 unkAA;           /* 0xAA */
    u8 padAB[0xB2 - 0xAB];
    u8 half;            /* 0xB2: the map half shown (0 whole, 1 left, 2 right) */
} MapWork;

#define MW ((MapWork *)gHudState)

extern f32 gMapDrawScales[][2];
u8 Player_DetectLevel(Player *p);
s32 Player_InMaplessArea(Player *p);
s32 Map_IsMaplessArea(u8 stage, u8 area);
s32 Map_PlayerIconKind(void *obj); /* unit's own declaration: the definition (unk_33E230.c) returns u8; u8 here drops the masks of two callers */
void Map_DrawPlayerIcon(Player *p, f32 x, f32 y, f32 s);

static void Map_GetPoint(f32 *x, f32 *y, f32 *z, s32 id);
AreaRecord *Stage_GetAreaRecord(s32 id); /* unit's own declaration: with the definition's AreaRecord *(u16 x) (sub_main/stage_data.c), Map_GetPosition 1 of 63 words differ */
s32 Player_HasMap(Player *p);
void Map_DrawDot(u32 color, f32 x, f32 y, f32 r);

/* The area map at x, y, scaled: the monsters' area marks, the map (the whole area when
 * Player_HasMap says so, else a window around the player), the camp mark, the monsters, the
 * other players and the local player. */
void Map_DrawArea(f32 x, s16 y, f32 scale)
{
    f32 mx;
    f32 my;
    f32 mz;
    Sprite s;
    Player *pl;
    Player *p;
    Em *e;
    Em *m;
    AreaRecord *r;
    f32 sx;
    f32 sz;
    f32 x0;
    f32 x1;
    f32 z0;
    f32 z1;
    s16 i;

    pl = MW->player;
    MW->unkAA = Player_DetectLevel(pl);
    MW->half = Player_InMaplessArea(pl);
    Tex_Prepare(1, 0x145);
    Tex_Bind(0x145);
    Gfx_SetState63(1);
    Gs_SetState(0x60, 0);
    sx = 320.0f * MW->scaleX * scale;
    sz = 320.0f * MW->scaleZ * scale;
    Gfx_SetBlend(4, 1);
    for (i = 20, e = (Em *)gEms; i > 0; i--, e++) {
        if ((u8)Map_PlayerIconKind(e) && e->showOnMap != 0) {
            Map_GetPoint(&mx, &my, &mz, e->area);
            mx *= scale;
            my *= scale;
            if (mz == 0.0f) {
                r = Stage_GetAreaRecord(e->area);
                mx = sx * (r->originX + 0.5f * r->sizeX);
                my = sz * (r->originZ + 0.5f * r->sizeZ);
                mz = 32.0f;
            }
            mx += x;
            my += y;
            Map_DrawDot(0x70C0C010, mx, my, mz * scale);
        }
    }
    Gfx_SetBlend(4, 5);
    Gfx_SetState63(0);
    if (Player_HasMap(pl) == 1) {
        s.y = y;
        s.x = x;
        s.h = s.w = 320.0f * scale;
        *(s32 *)&s.u0 = 0;
        *(s32 *)&s.u1 = 0xF000F0;
        switch (MW->half) {
        case 0:
            break;
        case 1:
            s.u1 = 0x78;
            break;
        case 2:
            s.u0 = 0x78;
            break;
        }
    } else {
        mx = 240.0f * (pl->mapPos[0] * MW->scaleX);
        my = 240.0f * (pl->mapPos[2] * MW->scaleZ);
        x0 = mx - 32.0f;
        x1 = 32.0f + mx;
        if (x0 < 0.0f) {
            x0 = 0.0f;
        }
        if (x1 > 240.0f) {
            x1 = 240.0f;
        }
        z0 = my - 32.0f;
        z1 = 32.0f + my;
        if (z0 < 0.0f) {
            z0 = 0.0f;
        }
        if (z1 > 240.0f) {
            z1 = 240.0f;
        }
        s.u0 = x0;
        s.v0 = z0;
        s.u1 = x1;
        s.v1 = z1;
        if (MW->half != 0) {
            s.u0 = x0 / 2.0f;
            s.u1 = x1 / 2.0f;
            if (MW->half == 2) {
                s.u0 += 0x78;
                s.u1 += 0x78;
            }
        }
        s.x = x + 1.3333334f * x0 * scale;
        s.y = y + (s16)(320.0f * z0 / 240.0f * scale);
        s.w = scale * (1.3333334f * (x1 - x0));
        s.h = scale * (320.0f * (z1 - z0) / 240.0f);
    }
    s.color = 0xDEFFFFFF;
    Sprite_Draw((struct Sprite *)&s);
    if (Quest_IsVersus() != 1) {
        if (MW->half == (u8)Map_IsMaplessArea(gScene.map, gScene.campArea)) {
            mx = sx * gMapDrawScales[gScene.map][0];
            my = sz * gMapDrawScales[gScene.map][1];
            Map_DrawMarker(x + mx, y + my);
        }
    }
    for (m = (Em *)gEms, i = 20; i > 0; i--, m++) {
        if ((u8)Map_PlayerIconKind(m)) {
            my = m->mapPos[2] * sz;
            if (my >= 0.0f) {
                mx = m->mapPos[0] * sx;
                Map_DrawMonsterIcon(m, x + mx, y + my, scale);
            }
        }
    }
    for (p = gPlayers, i = 0; i < gScene.playerCount; i++, p++) {
        if (p->active != 0 && i != gScene.localPlayer) {
            if (MW->half == (u8)Map_IsMaplessArea(gScene.map, p->area)) {
                mx = p->mapPos[0] * sx;
                my = p->mapPos[2] * sz;
                Map_DrawPlayerIcon(p, x + mx, y + my, scale);
            }
        }
    }
    p = MW->player;
    mx = p->mapPos[0] * sx;
    my = p->mapPos[2] * sz;
    Map_DrawPlayerIcon(p, x + mx, y + my, scale);
}

extern Vec3s gMapPoints[];
extern Vec3s gMapPoints80[];

static void Map_GetPoint(f32 *x, f32 *y, f32 *z, s32 id)
{
    Vec3s *p = &gMapPoints[id];

    if (gScene.map == 0xF && (id == 0x80 || id == 0x81)) {
        p = &gMapPoints80[id - 0x80];
    }
    *x = p->x;
    *y = p->y;
    *z = p->z;
}

/* The stages' map scale factors, x and y per stage (splat split the table in two). */
extern f32 gMapScales[];
extern f32 D_00430EA4[];
AreaRecord *Stage_GetAreaRecord(s32 id); /* unit's own declaration: with the definition's AreaRecord *(u16 x) (sub_main/stage_data.c), Map_GetPosition 1 of 63 words differ */

/* The map position of id (Map_GetPoint) scaled by the stage's factors; when its z is 0, the centre
 * of area id's rectangle instead (returns 0 then, else 1). */
/* Raw addresses kept here: the named form changes the code (gScene.map too). */
extern u8 D_006ADCE0;
s32 Map_GetPosition(f32 *x, f32 *y, s32 id)
{
    f32 z;
    AreaRecord *r;

    Map_GetPoint(x, y, &z, id);
    if (z != 0.0f) {
        *x = 0.003125f * *x * gMapScales[D_006ADCE0 + D_006ADCE0];
        *y = 0.003125f * *y * D_00430EA4[D_006ADCE0 + D_006ADCE0];
        return 1;
    }
    r = Stage_GetAreaRecord(id);
    *x = r->originX + 0.5f * r->sizeX;
    *y = r->originZ + 0.5f * r->sizeZ;
    return 0;
}

void Map_DrawArea(f32 x, s16 y, f32 scale);
s32 Season_Get(void);

void Map_DrawMain_Unk(void)
{
    Sprite s;
    u32 c;
    s16 k;

    Gfx_SetState63(1);
    Gfx_SetBlend(4, 5);
    Tex_Prepare(1, 0x154);
    Tex_Bind(0x154);
    s.x = 0;
    s.y = 0;
    s.w = 0x280;
    s.h = 0x1C0;
    *(s32 *)&s.u0 = 0;
    *(s32 *)&s.u1 = 0xBF00FF;
    s.color = 0xFFFFFFFF;
    Sprite_Draw((struct Sprite *)&s);
    k = (u8)Season_Get();
    switch (k) {
    case 0:
        c = 0xA0C6AD20;
        break;
    case 1:
        c = 0xA053ACCC;
        break;
    case 2:
        c = 0xA0398C34;
        break;
    }
    s.color = c;
    s.y = 0x10;
    s.h = 0x20;
    s.v0 = k * 21 + 0xC1;
    s.v1 = s.v0 + 0x14;
    Sprite_Draw((struct Sprite *)&s);
    s.y = 0x190;
    Sprite_Draw((struct Sprite *)&s);
    Map_DrawArea(160.0f, 0x40, 1.0f);
}

/* The zoomed map around the local player at x, y: the camp mark, the monsters and the other
 * players inside the view, then the local player. */
/* Level 3: at level 4 the products for the map position are scheduled otherwise (49 words
 * off with `0.75f * (5.0f * dz) + y`, 193 as written). The rest of the unit is at level 4. */
#pragma optimization_level 3
void Map_DrawZoomed(f32 x, s16 y)
{
    Sprite s;
    Player *p;
    Player *q;
    Em *e;
    f32 px;
    f32 pz;
    f32 ox;
    f32 oz;
    f32 dx;
    f32 dz;
    f32 mx;
    f32 my;
    u32 i;
    u8 *h;

    p = MW->player;
    MW->unkAA = Player_DetectLevel(p);
    MW->half = Player_InMaplessArea(p);
    Tex_Prepare(1, 0x145);
    Tex_Bind(0x145);
    Gfx_SetState63(0);
    Gs_SetState(0x60, 0);
    px = 240.0f * (p->mapPos[0] * MW->scaleX);
    pz = 240.0f * (p->mapPos[2] * MW->scaleZ);
    if (px < 22.4f) {
        ox = 0.0f;
    } else if (px > 217.6f) {
        ox = 195.2f;
    } else {
        ox = px - 22.4f;
    }
    if (pz < 22.4f) {
        oz = 0.0f;
    } else if (pz > 217.6f) {
        oz = 195.2f;
    } else {
        oz = pz - 22.4f;
    }
    Tex_Prepare(1, 0x145);
    Tex_Bind(0x145);
    s.x = x;
    s.y = y;
    s.w = 0xA8;
    s.h = 0xA8;
    s.u0 = ox;
    s.v0 = oz;
    s.u1 = 44.8f + ox;
    s.color = 0xC8FFFFFF;
    s.v1 = 44.8f + oz;
    h = &MW->half;
    if (*h != 0) {
        s.u0 /= 2;
        s.u1 /= 2;
        if (*h == 2) {
            s.u0 += 0x78;
            s.u1 += 0x78;
        }
    }
    Sprite_Draw((struct Sprite *)&s);
    if (Quest_IsVersus() != 1) {
        if (MW->half == (u8)Map_IsMaplessArea(gScene.map, gScene.campArea)) {
            dx = 240.0f * (MW->scaleX * gMapDrawScales[gScene.map][0]) - ox;
            if (dx >= 0.0f && dx <= 44.8f) {
                dz = 240.0f * (MW->scaleZ * gMapDrawScales[gScene.map][1]) - oz;
                if (dz >= 0.0f && dz <= 44.8f) {
                    mx = x + 0.75f * (5.0f * dx);
                    my = y + 0.75f * (5.0f * dz);
                    Map_DrawMarker(mx, my);
                }
            }
        }
    }
    for (e = (Em *)gEms, i = 0; i < 20; i++, e++) {
        if ((u8)Map_PlayerIconKind(e)) {
            dx = 240.0f * (e->mapPos[0] * MW->scaleX) - ox;
            if (dx >= 0.0f && dx <= 44.8f) {
                dz = 240.0f * (e->mapPos[2] * MW->scaleZ) - oz;
                if (dz >= 0.0f && dz <= 44.8f) {
                    mx = x + 0.75f * (5.0f * dx);
                    my = y + 0.75f * (5.0f * dz);
                    Map_DrawMonsterIcon(e, mx, my, 1.4f);
                }
            }
        }
    }
    for (q = gPlayers, i = 0; i < gScene.playerCount; i++, q++) {
        if (q->active != 0 && i != gScene.localPlayer) {
            if (MW->half == (u8)Map_IsMaplessArea(gScene.map, q->area)) {
                dx = 240.0f * (q->mapPos[0] * MW->scaleX) - ox;
                if (dx >= 0.0f && dx <= 44.8f) {
                    dz = 240.0f * (q->mapPos[2] * MW->scaleZ) - oz;
                    if (dz >= 0.0f && dz <= 44.8f) {
                        mx = x + 0.75f * (5.0f * dx);
                        my = y + 0.75f * (5.0f * dz);
                        Map_DrawPlayerIcon(q, mx, my, 1.0f);
                    }
                }
            }
        }
    }
    mx = x + 0.75f * (5.0f * (px - ox));
    my = y + 0.75f * (5.0f * (pz - oz));
    Map_DrawPlayerIcon(MW->player, mx, my, 1.0f);
}
#pragma optimization_level reset

void Map_DrawZoomed(f32 x, s16 y);
void Hud_DrawGlowEffect(void);
extern f32 gMapAreaOffsetX[];
extern s16 gMapAreaOffsetY[];

/* The map in its HUD corner: the area map, or the zoomed map when +0x46 is set. */
void Hud_DrawMap(void)
{
    s16 y;

    if (gHudState->onlineHud != 0) {
        return;
    }
    if (gHudState->hidden != 0) {
        return;
    }
    y = gChatInputMode == 0 ? 0x3C : 0xB6;
    if (gHudState->mapZoomed == 0) {
        Map_DrawArea(400.0f + gMapAreaOffsetX[gScene.map], y + gMapAreaOffsetY[gScene.map], 0.7f);
        Hud_DrawGlowEffect();
    } else {
        Map_DrawZoomed(456.0f, y);
    }
}

s32 Pouch_IsSlotUsable(Player *p, u16 slot);
void Sprite_DrawStretch3(Sprite *src, s16 w, s16 du);
void Sprite_DrawList(u8 *s, s32 n);
void Hud_DrawPouchSelector(void);
void Hud_DrawPouchBar(void);
extern u32 gHudSeasonColors[];
extern u32 gPouchBoxSprites[];
extern u32 D_00430B18[];
extern u32 D_00430B24[];
extern char D_0046D0D8[];
s32 Str_Length(char *s);

typedef struct PouchHud {
    u8 pad0[8];
    Player *player;     /* 0x08 */
    u8 pad0C[0x60 - 0xC];
    s8 blink60;         /* 0x60 */
    u8 pad61[3];
    s8 blink64;         /* 0x64 */
    u8 pad65[0x98 - 0x65];
    u8 hidden;          /* 0x98 */
    u8 pad99[0xB3 - 0x99];
    u8 state;           /* 0xB3 */
    s8 timer;           /* 0xB4 */
} PouchHud;

#define HUD ((PouchHud *)gHudState)

void Hud_DrawPouchItem(void)
{
    Player *p = HUD->player;
    u16 item;
    s32 n;

    if (p->active == 0 || p->visible == 0) {
        return;
    }
    if (HUD->hidden != 0) {
        return;
    }
    Gfx_SetState63(1);
    Gfx_SetBlend(4, 5);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    D_00430B18[0] = gHudSeasonColors[(u8)Season_Get()];
    Sprite_DrawStretch3((Sprite *)(u8 *)gPouchBoxSprites, 8, 8);
    Sprite_DrawList((u8 *)D_00430B24, 2);
    switch (HUD->state) {
    case 0:
        if (p->itemSelect == 0) {
            Hud_DrawPouchSelector();
            break;
        }
        HUD->state++;
        HUD->timer = 6;
    case 1:
        Hud_DrawPouchBar();
        if (p->itemSelect == 0) {
            HUD->state = 3;
            break;
        }
        HUD->timer -= 3;
        if (HUD->timer <= 0) {
            HUD->timer = 0;
            HUD->state++;
        }
        break;
    case 2:
        HUD->timer = 0;
        if (p->itemSelect == 0) {
            HUD->state = 3;
            HUD->timer = 3;
        }
        Hud_DrawPouchBar();
        break;
    case 3:
        if (p->itemSelect == 0) {
            HUD->timer += 3;
            if (HUD->timer >= 9) {
                HUD->timer = 9;
                HUD->state = 0;
                Hud_DrawPouchSelector();
                break;
            }
        } else {
            HUD->state = 1;
        }
        Hud_DrawPouchBar();
        break;
    }
    if (Pouch_IsSlotUsable(p, p->pouchCursor) == 1) {
        n = (u8)p->pouch[p->pouchCursor].count;
        item = p->pouch[p->pouchCursor].item;
        if (n > 0 && gItemData[item].max > 1) {
            Font_SetSpacing(0x12, 0x12);
            Font_SetPos(n > 9 ? 0x212 : 0x20D, 0x170);
            Font_SetColor(n >= gItemData[item].max ? 0x807F212E : 0x807F7F7F);
            Text_PrintfColored(D_0046CFF0, n);
        }
        Font_SetSpacing(0x14, 0x13);
        Font_SetPos(419.0f + (186.0f - Str_Length((char *)gItemNames[p->pouch[p->pouchCursor].item]) * 10) / 2.0f, 0x18C);
        Font_SetColor(0x807F7F7F);
        Text_PrintfColored(D_0046D0D8, gItemNames[item]);
    }
    if (HUD->blink60 > 0) {
        HUD->blink60--;
    }
    if (HUD->blink64 > 0) {
        HUD->blink64--;
    }
}

#undef HUD

s32 Pouch_IsSlotUsable(Player *p, u16 slot);
s32 Hud_DrawPouchSlotName(u8, s16);
void Hud_DrawPouchSlotIcon(u8 slot, s16 x, f32 sw, f32 sh);
void Sprite_DrawStretch3(Sprite *src, s16 w, s16 du);
void Sprite_DrawList(u8 *s, s32 n);
extern s32 gItemColors[];
extern u32 D_00430B4C[];
extern u32 gPouchBarSprites[];
extern s16 D_00430BF8[];
extern s16 D_00430C0C[];
extern u32 gPouchSelectorSprites[];
extern u8 D_00430C5C[];
extern u32 gPouchSelectorSpritesB[];
extern u8 gPouchSelectorCursor[];
extern s32 D_00430CF0[];
extern u8 D_00430CFC[];
extern s32 D_00430D04[];
extern u8 gPouchSelectorSpritesC[];
extern u8 gPouchSelIconsA[];
extern s16 D_00430E4A[];
extern u8 gPouchSelIconsB[];
extern s16 D_00430E5A[];

void Hud_DrawPouchSelector(void)
{
    Player *p;
    void *icons;
    s32 temp_s0;
    s32 var_s0;
    s32 temp_s0_2;
    u16 ammoSlot;

    p = gHudState->player;
    temp_s0 = p->isGunner != 1;
    Sprite_DrawList((u8 *) (u8 *)gPouchBarSprites, 6);
    temp_s0_2 = temp_s0 & 0xFF;
    if (temp_s0_2 != 0) {
        Sprite_Draw((struct Sprite *)D_00430BF8);
        icons = gPouchSelIconsA;
        D_00430E4A[0] = 0x175;
    } else {
        Sprite_Draw((struct Sprite *)D_00430C0C);
        icons = gPouchSelIconsB;
        D_00430E5A[0] = 0x173;
    }
    Sprite_DrawList((u8 *) (u8 *)D_00430B4C, 2);
    Sprite_DrawList((u8 *) (u8 *)gPouchSelectorSprites, 3);
    if (Pouch_IsSlotUsable((Player *) p, p->pouchCursor) == 1) {
        Tex_Prepare(1, 0x144);
        Tex_Bind(0x144);
        Hud_DrawPouchSlotIcon((u8) p->pouchCursor, 0x200, 1.0f, 1.0f);
    }
    if (temp_s0_2 == 0) {
        var_s0 = -0x7F7F80;
        if (p->ammoCount != 0) {
            Font_SetSpacing(0x14, 0x13);
            if (Hud_DrawPouchSlotName(*(u8 *)&p->ammoSlot, 0x130) == 1) {
                var_s0 = gItemColors[*(&gItemData->color + (p->pouch[p->ammoSlot].item * 0x18))];
            }
        }
        Tex_Prepare(1, 0x146);
        Tex_Bind(0x146);
        Sprite_DrawStretch3((Sprite *)D_00430C5C, 8, -8);
        Sprite_DrawList((u8 *) (u8 *)gPouchSelectorSpritesB, 6);
        if (p->weaponType != 0xA) {
            D_00430CF0[0] = var_s0;
            Sprite_Draw((struct Sprite *)gPouchSelectorCursor);
        } else {
            ammoSlot = p->ammoSlot;
            if (p->pouch[ammoSlot].item != 0x87) {
                if (ammoSlot == p->coatingSlot) {
                    Sprite_DrawList((u8 *) gPouchSelectorSpritesC, 2);
                }
                D_00430D04[0] = var_s0;
                Sprite_Draw((struct Sprite *)D_00430CFC);
            }
        }
    }
    if (p->pouchHudFlags & 1) {
        *(s16 *)((u8 *)icons + 0xA) = (s16) (*(s16 *)((u8 *)icons + 0xA) + 2);
    }
    Draw_Icons((void *) icons, 2U);
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_33E620", D_0046D0B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_33E620", D_0046D0B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_33E620", D_0046D0C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_33E620", D_0046D0C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_33E620", D_0046D0D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_33E620", D_0046D0D8);

extern Sprite D_00430BE4;
extern Sprite D_00430C48;
extern u8 D_00430D94[];
extern Icon gPouchIconFromA[];
extern Icon gPouchIconFromB[];
extern Icon gPouchBarIcons[];
extern s16 gPouchBarXA[];
extern s16 gPouchBarXB[];
extern s16 D_00432438[];
extern s16 D_00432440[];
s16 Pouch_CountSelectable(Player *p);
u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir);
u16 Bowgun_NextAmmoSlot(Player *p, u16 slot, u8 dir);
extern u32 gPouchBarFrame[];
void Hud_AmmoIconSprite(u8 *a, Sprite *s);
void Hud_DrawPouchBarEnds(s16 x, s16 y, s16 h);

#define HUD ((PouchHud *)gHudState)

/* The pouch item bar (state 1-3 of Hud_DrawPouchItem): the selected item and its
 * neighbours sliding in by the timer (+0xB4), the frame, and for gunners the ammo list. */
void Hud_DrawPouchBar(void)
{
    Sprite b;
    Sprite a;
    u16 slot[6];
    u8 ammo[4];
    Player *p;
    u8 melee;
    s32 n;
    s32 d;
    s32 t;
    s16 x0;
    s16 x1;
    s16 x2;
    s16 x3;
    s16 x4;
    s16 x5;
    s32 i;
    f32 sc;

    p = gHudState->player;
    melee = p->isGunner != 1;
    Sprite_Draw((struct Sprite *)gPouchBarSprites);
    a.color = 0xA0000000;
    *(s32 *)&a.u0 = 0x69007E;
    *(s32 *)&a.u1 = 0x8C00A1;
    b.color = 0xFF808080;
    *(s32 *)&b.u0 = 0x16008B;
    *(s32 *)&b.u1 = 0x4800BD;
    if (gHudState->blink60 != 0) {
        n = Pouch_CountSelectable(p);
        slot[0] = slot[1] = slot[2] = slot[3] = slot[4] = slot[5] = 0xFF;
        d = gHudState->dir61 >= 0;
        if (Pouch_IsSlotUsable(p, p->pouchCursor) == 0) {
            n++;
        } else {
            slot[d] = p->pouchCursor;
        }
        if (n >= 2) {
            slot[1 - d] = Pouch_NextSelectable(p, p->pouchCursor, d & 0xFF);
        }
        switch (n) {
        default:
            slot[2] = Pouch_NextSelectable(p, slot[0], 1);
            slot[3] = Pouch_NextSelectable(p, slot[1], 0);
            slot[4] = Pouch_NextSelectable(p, slot[2], 1);
            slot[5] = Pouch_NextSelectable(p, slot[3], 0);
            break;
        case 5:
            slot[2] = Pouch_NextSelectable(p, slot[0], 1);
            slot[3] = Pouch_NextSelectable(p, slot[1], 0);
            slot[4] = Pouch_NextSelectable(p, slot[2], 1);
            break;
        case 4:
            slot[3] = Pouch_NextSelectable(p, slot[1], 0);
        case 3:
            slot[2] = Pouch_NextSelectable(p, slot[0], 1);
            break;
        case 2:
        case 1:
        case 0:
            break;
        }
        t = HUD->timer;
        x0 = gPouchBarXB[0] + (0x1F6 - gPouchBarXB[0]) * t / 9;
        x1 = gPouchBarXB[1] + (0x20A - gPouchBarXB[1]) * t / 9;
        x2 = gPouchBarXB[2] + (0x1EA - gPouchBarXB[2]) * t / 9;
        x3 = gPouchBarXB[3] + (0x216 - gPouchBarXB[3]) * t / 9;
        x4 = gPouchBarXB[4] + (0x1DE - gPouchBarXB[4]) * t / 9;
        x5 = gPouchBarXB[5] + (0x222 - gPouchBarXB[5]) * t / 9;
        sc = 40 * 0.32;
        a.w = sc;
        a.y = 0x15E;
        a.h = 0x19;
        a.x = x4 - (a.w >> 1);
        Sprite_Draw((struct Sprite *)&a);
        a.x = x5 - (a.w >> 1);
        Sprite_Draw((struct Sprite *)&a);
        sc = 58 * 0.32;
        b.w = sc;
        b.y = 0x158;
        b.h = 0x25;
        b.x = x4 - (b.w >> 1);
        Sprite_Draw((struct Sprite *)&b);
        b.x = x5 - (b.w >> 1);
        Sprite_Draw((struct Sprite *)&b);
        Tex_Prepare(1, 0x144);
        Tex_Bind(0x144);
        if (slot[4] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[4], x4, 0.32f, 0.64f);
        }
        if (slot[5] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[5], x5, 0.32f, 0.64f);
        }
        Tex_Prepare(1, 0x146);
        Tex_Bind(0x146);
        sc = 40 * 0.76;
        a.w = sc;
        a.y = 0x15B;
        a.h = 0x1E;
        a.x = x2 - (a.w >> 1);
        Sprite_Draw((struct Sprite *)&a);
        a.x = x3 - (a.w >> 1);
        Sprite_Draw((struct Sprite *)&a);
        sc = 58 * 0.76;
        b.w = sc;
        b.y = 0x154;
        b.h = 0x2C;
        b.x = x2 - (b.w >> 1);
        Sprite_Draw((struct Sprite *)&b);
        b.x = x3 - (b.w >> 1);
        Sprite_Draw((struct Sprite *)&b);
        Tex_Prepare(1, 0x144);
        Tex_Bind(0x144);
        if (slot[2] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[2], x2, 0.76f, 0.76f);
        }
        if (slot[3] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[3], x3, 0.76f, 0.76f);
        }
        Tex_Prepare(1, 0x146);
        Tex_Bind(0x146);
        sc = 40 * 0.88;
        a.w = sc;
        a.y = 0x159;
        a.h = 0x23;
        a.x = x0 - (a.w >> 1);
        Sprite_Draw((struct Sprite *)&a);
        a.x = x1 - (a.w >> 1);
        Sprite_Draw((struct Sprite *)&a);
        sc = 58 * 0.88;
        b.w = sc;
        b.y = 0x151;
        b.h = 0x33;
        b.x = x0 - (b.w >> 1);
        Sprite_Draw((struct Sprite *)&b);
        b.x = x1 - (b.w >> 1);
        Sprite_Draw((struct Sprite *)&b);
        Sprite_Draw((struct Sprite *)&D_00430C48);
        Tex_Prepare(1, 0x144);
        Tex_Bind(0x144);
        if (slot[0] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[0], x0, 0.88f, 0.88f);
        }
        if (slot[1] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[1], x1, 0.88f, 0.88f);
        }
    } else {
        slot[0] = p->pouchCursor;
        slot[1] = slot[2] = slot[3] = slot[4] = 0xFF;
        n = Pouch_CountSelectable(p);
        if (Pouch_IsSlotUsable(p, p->pouchCursor) == 0) {
            n++;
            slot[0] = 0xFF;
        }
        switch (n) {
        default:
            slot[1] = Pouch_NextSelectable(p, p->pouchCursor, 1);
            slot[2] = Pouch_NextSelectable(p, p->pouchCursor, 0);
            slot[3] = Pouch_NextSelectable(p, slot[1], 1);
            slot[4] = Pouch_NextSelectable(p, slot[2], 0);
            break;
        case 4:
            slot[1] = Pouch_NextSelectable(p, p->pouchCursor, 1);
            slot[2] = Pouch_NextSelectable(p, p->pouchCursor, 0);
            slot[3] = Pouch_NextSelectable(p, slot[1], 1);
            break;
        case 3:
            slot[2] = Pouch_NextSelectable(p, p->pouchCursor, 0);
        case 2:
            slot[1] = Pouch_NextSelectable(p, p->pouchCursor, 1);
            break;
        case 1:
        case 0:
            break;
        }
        t = HUD->timer;
        x0 = gPouchBarXA[1] + (0x1E8 - gPouchBarXA[1]) * t / 9;
        x1 = gPouchBarXA[2] + (0x218 - gPouchBarXA[2]) * t / 9;
        x2 = gPouchBarXA[3] + (0x1DE - gPouchBarXA[3]) * t / 9;
        x3 = gPouchBarXA[4] + (0x222 - gPouchBarXA[4]) * t / 9;
        sc = 40 * 0.68;
        a.w = sc;
        a.y = 0x15D;
        a.h = 0x1B;
        a.x = x3 - (a.w >> 1);
        Sprite_Draw((struct Sprite *)&a);
        a.x = x2 - (a.w >> 1);
        Sprite_Draw((struct Sprite *)&a);
        sc = 58 * 0.68;
        b.w = sc;
        b.y = 0x157;
        b.h = 0x27;
        b.x = x3 - (b.w >> 1);
        Sprite_Draw((struct Sprite *)&b);
        b.x = x2 - (b.w >> 1);
        Sprite_Draw((struct Sprite *)&b);
        Tex_Prepare(1, 0x144);
        Tex_Bind(0x144);
        if (slot[4] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[4], x3, 0.68f, 0.68f);
        }
        if (slot[3] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[3], x2, 0.68f, 0.68f);
        }
        Tex_Prepare(1, 0x146);
        Tex_Bind(0x146);
        sc = 40 * 0.8;
        a.w = sc;
        a.y = 0x15A;
        a.h = 0x20;
        a.x = x1 - (a.w >> 1);
        Sprite_Draw((struct Sprite *)&a);
        a.x = x0 - (a.w >> 1);
        Sprite_Draw((struct Sprite *)&a);
        sc = 58 * 0.8;
        b.w = sc;
        b.y = 0x153;
        b.h = 0x2E;
        b.x = x1 - (b.w >> 1);
        Sprite_Draw((struct Sprite *)&b);
        b.x = x0 - (b.w >> 1);
        Sprite_Draw((struct Sprite *)&b);
        Tex_Prepare(1, 0x144);
        Tex_Bind(0x144);
        if (slot[2] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[2], x1, 0.8f, 0.8f);
        }
        if (slot[1] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[1], x0, 0.8f, 0.8f);
        }
        Tex_Prepare(1, 0x146);
        Tex_Bind(0x146);
        Sprite_DrawList((u8 *)D_00430B4C, 2);
        Sprite_Draw((struct Sprite *)&D_00430C48);
        Tex_Prepare(1, 0x144);
        Tex_Bind(0x144);
        if (slot[0] != 0xFF) {
            Hud_DrawPouchSlotIcon(slot[0], 0x200, 1.0f, 1.0f);
        }
    }
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    b.color = -1;
    if (HUD->state == 3) {
        b.w = 0x14;
        b.y = 0x160;
        b.h = 0x11;
        *(s32 *)&b.u0 = 0x9C00DF;
        *(s32 *)&b.u1 = 0xAD00CB;
        b.x = HUD->timer * -0x2B / 9 + 0x256;
        Sprite_Draw((struct Sprite *)&b);
        b.w = 0x24;
        b.y = 0x16F;
        b.h = 0x1D;
        *(s32 *)&b.u0 = 0xCF00B8;
        *(s32 *)&b.u1 = 0xEC00DC;
        b.x = HUD->timer * -0x2B / 9 + 0x23D;
    } else {
        b.w = 0x18;
        b.y = 0x16B;
        b.h = 0x19;
        *(s32 *)&b.u0 = 0xAE00CD;
        *(s32 *)&b.u1 = 0xC700E5;
        b.x = HUD->timer * -0x2B / 9 + 0x250;
        Sprite_Draw((struct Sprite *)&b);
        b.w = 0x24;
        b.y = 0x179;
        b.h = 0x13;
        *(s32 *)&b.u0 = 0xD900B8;
        *(s32 *)&b.u1 = 0xEC00DC;
        b.x = HUD->timer * -0x2B / 9 + 0x23D;
    }
    Sprite_Draw((struct Sprite *)&b);
    b.x -= 0x3F;
    b.w = 0x3F;
    b.y = 0x184;
    b.h = 7;
    *(s32 *)&b.u0 = 0x2200C1;
    *(s32 *)&b.u1 = 0x290100;
    Sprite_Draw((struct Sprite *)&b);
    while (b.x > 0x205) {
        b.x -= 0x3F;
        Sprite_Draw((struct Sprite *)&b);
    }
    b.w = b.x - 0x1C6;
    b.u0 = 0x102 - b.w;
    b.x = 0x1C6;
    Sprite_Draw((struct Sprite *)&b);
    if (melee) {
        Sprite_Draw((struct Sprite *)&D_00430BE4);
        b.w = 0x18;
        b.y = 0x16F;
        b.h = 0x18;
        *(s32 *)&b.u0 = 0x700056;
        *(s32 *)&b.u1 = 0x88006E;
        b.x = (D_00430BF8[0] - 0x19E) * HUD->timer / 9 + 0x19E;
    } else {
        b.w = 0x1A;
        b.y = 0x16B;
        b.h = 0x1B;
        *(s32 *)&b.u0 = 0xAD00E6;
        *(s32 *)&b.u1 = 0xC80100;
        b.x = (D_00430C0C[0] - 0x197) * HUD->timer / 9 + 0x197;
    }
    Sprite_Draw((struct Sprite *)&b);
    i = melee;
    if (!i) {
        b.w = 0x24;
        b.y = 0x144;
        b.h = 0x24;
        *(s32 *)&b.u0 = 0xEB00FF;
        *(s32 *)&b.u1 = 0xC700DB;
        b.x = HUD->timer * 0x2B / 9 + 0x19F;
        Sprite_Draw((struct Sprite *)&b);
        b.x += 0x24;
        b.w = 0x3F;
        b.y = 0x144;
        b.h = 7;
        *(s32 *)&b.u0 = 0x2200C1;
        *(s32 *)&b.u1 = 0x290100;
        while (b.x < 0x1FB) {
            Sprite_Draw((struct Sprite *)&b);
            b.x += 0x3F;
        }
        b.w = 0x23A - b.x;
        b.u1 = b.w + 0xC3;
        Sprite_Draw((struct Sprite *)&b);
    }
    n = 1;
    if (melee) {
        gPouchBarIcons[0].x = (gPouchIconFromA[0].x - 0x19E) * HUD->timer / 9 + 0x19E;
        gPouchBarIcons[0].y = 0x175;
    } else {
        gPouchBarIcons[0].x = (gPouchIconFromB[0].x - 0x18E) * HUD->timer / 9 + 0x18E;
        gPouchBarIcons[0].y = 0x171;
    }
    if (p->pouchHudFlags & 4) {
        gPouchBarIcons[0].y += 2;
    }
    if (HUD->state < 3) {
        n = 2;
        gPouchBarIcons[1].x = HUD->timer * -0x2B / 9 + 0x25F;
        gPouchBarIcons[1].y = 0x16F;
        if (p->pouchHudFlags & 2) {
            gPouchBarIcons[1].y += 2;
        }
    }
    Draw_Icons(gPouchBarIcons, n);
    if (!i) {
        Tex_Prepare(1, 0x146);
        Tex_Bind(0x146);
        Sprite_DrawStretch9((Sprite *)gPouchBarFrame, 8, 8, -8, -8);
        Sprite_DrawList(D_00430D94, 8);
        b.h = 0x18;
        b.v0 = 0x1A;
        b.v1 = 0x30;
        if (gHudState->blink64 != 0) {
            if (gHudState->dir65 < 0) {
                ammo[1] = Bowgun_NextAmmoSlot(p, p->ammoSlot, 1);
                ammo[0] = ammo[2] = ammo[3] = 0xFF;
                if (ammo[1] != 0xFF) {
                    if (p->pouch[ammo[1]].item == p->pouch[p->ammoSlot].item) {
                        ammo[1] = 0xFF;
                    } else {
                        Font_SetSpacing(0x14, 0x14);
                        if (Hud_DrawPouchSlotName(ammo[1], 0x125) == 0) {
                            ammo[1] = 0xFF;
                        } else {
                            u8 k = Bowgun_NextAmmoSlot(p, ammo[1], 1);
                            if (p->pouch[k].item != p->pouch[ammo[1]].item && p->pouch[k].item != p->pouch[p->ammoSlot].item) {
                                Font_SetSpacing(0x14, 0xA);
                                if (Hud_DrawPouchSlotName(k, 0x13C) == 1) {
                                    ammo[0] = k;
                                    k = Bowgun_NextAmmoSlot(p, p->ammoSlot, 0);
                                    if (p->pouch[k].item != p->pouch[ammo[0]].item && Hud_DrawPouchSlotName(k, 0x100) == 1) {
                                        ammo[3] = k;
                                    }
                                }
                            }
                        }
                    }
                    Font_SetSpacing(0x14, 0x14);
                    if (Hud_DrawPouchSlotName(*(u8 *)&p->ammoSlot, 0x10D) == 1) {
                        ammo[2] = p->ammoSlot;
                    }
                }
            } else {
                ammo[0] = Bowgun_NextAmmoSlot(p, p->ammoSlot, 1);
                ammo[1] = ammo[2] = ammo[3] = 0xFF;
                if (ammo[0] != 0xFF) {
                    if (p->pouch[ammo[0]].item == p->pouch[p->ammoSlot].item) {
                        ammo[0] = 0xFF;
                    } else {
                        Font_SetSpacing(0x14, 0xA);
                        if (Hud_DrawPouchSlotName(ammo[0], 0x13C) == 0) {
                            ammo[0] = 0xFF;
                        } else {
                            u8 k = Bowgun_NextAmmoSlot(p, p->ammoSlot, 0);
                            if (p->pouch[k].item != p->pouch[ammo[0]].item) {
                                Font_SetSpacing(0x14, 0x14);
                                if (Hud_DrawPouchSlotName(k, 0x10D) == 1) {
                                    ammo[2] = k;
                                    k = Bowgun_NextAmmoSlot(p, ammo[2], 0);
                                    if (p->pouch[k].item != p->pouch[ammo[0]].item) {
                                        Font_SetSpacing(0x14, 0xA);
                                        if (Hud_DrawPouchSlotName(k, 0x100) == 1) {
                                            ammo[3] = k;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Font_SetSpacing(0x14, 0x14);
                    if (Hud_DrawPouchSlotName(*(u8 *)&p->ammoSlot, 0x125) == 1) {
                        ammo[1] = p->ammoSlot;
                    }
                }
            }
            Hud_AmmoIconSprite((u8 *)p, &b);
            for (i = 3; i >= 0; i--) {
                if (p->weaponType == 10 && p->pouch[ammo[i]].item == 0x87) {
                    b.y += D_00432438[i];
                } else {
                    if (ammo[i] != 0xFF) {
                        b.color = gItemColors[gItemData[p->pouch[ammo[i]].item].color];
                    } else {
                        b.color = 0xFF707070;
                    }
                    b.h = D_00432440[i];
                    Sprite_Draw((struct Sprite *)&b);
                    b.y += D_00432438[i];
                }
            }
        } else {
            ammo[0] = Bowgun_NextAmmoSlot(p, p->ammoSlot, 1);
            ammo[1] = ammo[2] = 0xFF;
            if (ammo[0] != 0xFF) {
                Font_SetSpacing(0x14, 0x14);
                melee = Bowgun_NextAmmoSlot(p, p->ammoSlot, 0);
                if (p->pouch[melee].item != p->pouch[ammo[0]].item && Hud_DrawPouchSlotName(melee, 0xFF) == 1) {
                    ammo[2] = melee;
                }
                if (p->pouch[ammo[0]].item == p->pouch[p->ammoSlot].item) {
                    ammo[0] = 0xFF;
                } else if (Hud_DrawPouchSlotName(ammo[0], 0x12F) == 0) {
                    ammo[0] = 0xFF;
                }
                if (Hud_DrawPouchSlotName(*(u8 *)&p->ammoSlot, 0x117) == 1) {
                    ammo[1] = p->ammoSlot;
                }
            }
            Hud_AmmoIconSprite((u8 *)p, &b);
            for (i = 2; i >= 0; i--) {
                if (p->weaponType == 10) {
                    if (p->pouch[ammo[i]].item != 0x87) {
                        if (ammo[i] != 0xFF && ammo[i] == p->coatingSlot) {
                            Hud_DrawPouchBarEnds(0x1AB, b.y, b.h + 1);
                        }
                        goto draw;
                    }
                } else {
                draw:
                    if (ammo[i] != 0xFF) {
                        b.color = gItemColors[gItemData[p->pouch[ammo[i]].item].color];
                    } else {
                        b.color = 0xFF707070;
                    }
                    Sprite_Draw((struct Sprite *)&b);
                }
                b.y += 0x18;
            }
        }
        gHudState->pouchBarShown = 1;
    }
}

#undef HUD

extern Icon gHudButtonIcons[2];
extern u8 gHudButtonSprites[];

/* The two icons gHudButtonIcons (pressed a bit when Player +0x84A bits 0x10 and 8 are set) and the three sprites of gHudButtonSprites[0], when +0x5F of the state is set. */
void Hud_DrawButtonIcons(void)
{
    Player *p = gHudState->player;

    if (gHudState->pouchBarShown != 0) {
        gHudButtonIcons[0].y = 0xEA;
        if (p->pouchHudFlags & 0x10) {
            gHudButtonIcons[0].y += 2;
        }
        gHudButtonIcons[1].y = 0x140;
        if (p->pouchHudFlags & 8) {
            gHudButtonIcons[1].y += 2;
        }
        Draw_Icons(gHudButtonIcons, 2);
        Tex_Prepare(1, 0x146);
        Tex_Bind(0x146);
        Sprite_DrawList(gHudButtonSprites, 3);
    }
}

extern char *gPouchInfiniteFmt[];
s32 Str_Length(char *s);

/* The name and count of pouch slot `slot` (the loaded ammo highlighted) at height y; 0 when the
 * slot is empty or not usable. */
s32 Hud_DrawPouchSlotName(u8 slot, s16 y)
{
    Player *p = gHudState->player;
    ItemData *d;
    u16 item;
    u8 n;
    u32 color;
    char buf[0x20];

    if (slot == 0xFF) {
        return 0;
    }
    item = p->pouch[slot].item;
    d = &gItemData[item];
    if (!(d->flags & 6)) {
        return 0;
    }
    n = p->pouch[slot].count;
    if (n == 0) {
        return 0;
    }
    if (p->weaponType != 10) {
        color = (u8)d->ammoKind == p->savedAmmoType ? 0x807F7F28 : 0x807F7F7F;
    } else {
        color = slot == p->coatingSlot ? 0x807F7F28 : 0x807F7F7F;
    }
    Font_SetColor(color);
    if (item == 0x87) {
        sprintf(buf, (char *)gItemNames[item]);
    } else if (d->max == 0xFF) {
        sprintf(buf, gPouchInfiniteFmt[0], gItemNames[item]);
    } else {
        sprintf(buf, "%s %d", gItemNames[item], n);
    }
    Font_SetPos(419.0f + (186.0f - Str_Length(buf) * 10) / 2.0f, y);
    Text_Draw(buf);
    return 1;
}

s32 Item_IsUsable(u16 item);

/* The icon of pouch slot `slot` at x (scaled by sw, sh), marked when the item can't be used (or is
 * flagged in its data). */
/* icon is a u8: as an s32 it took s2 and the item*24 offset s1. */
void Hud_DrawPouchSlotIcon(u8 slot, s16 x, f32 sw, f32 sh)
{
    Sprite s;
    u8 color;
    u16 item;
    u8 icon;
    s8 mark;

    item = (gHudState->player)->pouch[slot].item;
    icon = gItemData[item].icon;
    if (icon == 0xFF) {
        return;
    }
    color = gItemData[item].color;
    mark = 0;
    if (Item_IsUsable(item) == 0) {
        color = 10;
        mark = -1;
    } else if (gItemData[item].flags2 & 1) {
        mark = 1;
    }
    s.w = 45.0f * sw;
    s.x = x - (s.w >> 1);
    s.h = 45.0f * sh;
    s.y = 0x16A - (s.h >> 1);
    s.u0 = ((icon + 1) & 7) * 32 + 1;
    s.v0 = ((icon + 1) >> 3) * 32 + 1;
    s.u1 = s.u0 + 0x1E;
    s.v1 = s.v0 + 0x1E;
    s.color = gItemColors[color];
    Sprite_Draw((struct Sprite *)&s);
    if (mark != 0) {
        if (mark > 0) {
            *(s32 *)&s.u0 = 0xE00080;
            *(s32 *)&s.u1 = 0x10000A0;
            s.color = 0xFFFFFFFF;
        } else {
            *(s32 *)&s.u0 = 0xE000A0;
            *(s32 *)&s.u1 = 0x10000C0;
            s.color = 0xC8FF0000;
        }
        Sprite_Draw((struct Sprite *)&s);
    }
}

extern Panel gItemWindowTextBox;
extern ListPanel gItemWindowDropPanel;
extern ListPanel gItemWindowPanel;
extern Panel gItemWindowNameBox[2];
extern char *gItemFullText[];
extern char *gItemGotFmt;
extern u32 gItemDropPanelColors[];
extern s32 gItemDropPouchColors[];
void ItemList_DrawPouch(s32 a, s32 b, s32 flags);
void HelpWindow_DrawLine(u8 a, u16 b);
void ButtonHint_DrawDetails(void);

void Hud_DrawItemWindow(void)
{
    char buf[0x40];
    s32 flags;
    s16 *pos;
    u8 st;

    if (gHudState->dropState <= 1) {
        return;
    }
    if (gHudState->dropState == 2) {
        if (gHudState->dropBlink == 0) {
            Window_DrawText(&gItemWindowTextBox, (s16 *)NULL, (u8 *)gItemFullText[0]);
        }
        return;
    }
    flags = 0x88;
    pos = Shake_GetOffset(1);
    st = gHudState->dropState;
    switch (st) {
    case 4:
    case 3:
        gItemWindowDropPanel.cursorColor = gItemDropPanelColors[st];
        ListPanel_Draw(&gItemWindowDropPanel, pos, NULL, gHudState->dropChoice);
        break;
    case 5:
        ListPanel_Draw(&gItemWindowPanel, NULL, NULL, gHudState->yesNo);
        break;
    case 6:
        flags = 0x80;
        break;
    }
    ItemList_DrawPouch(gHudState->slot, gItemDropPouchColors[gHudState->dropState], flags);
    if (Shake_Value(1) != 0) {
        return;
    }
    if (gHudState->dropCount <= 1) {
        Window_DrawText(&gItemWindowNameBox[0], (s16 *)NULL, (u8 *)gItemNames[gHudState->dropItem]);
    } else {
        sprintf(buf, gItemGotFmt, gItemNames[gHudState->dropItem], gHudState->dropCount);
        Window_DrawText(&gItemWindowNameBox[1], (s16 *)NULL, (u8 *)buf);
    }
    HelpWindow_DrawLine(1, (u16)gHudState->dropHelpLine);
    if (gHudState->dropState == 4 && gHudState->dropShowSlot == 0) {
        ButtonHint_DrawDetails();
    }
}

void Hud_DrawPlayerStatus(Player *p, s16 y, f32 scale);

void Hud_DrawPartyStatus(void)
{
    Player *p;
    u32 i;
    s16 y;

    Hud_DrawPlayerStatus(gHudState->player, 0x63, 15.0f);
    y = 0x77;
    p = gPlayers;
    for (i = 0; i < 4; i++, p++) {
        if (p->active != 0 && i != gScene.localPlayer && gScene.inQuest[i] == 1) {
            Hud_DrawPlayerStatus(p, y, 15.0f);
            y += 0x16;
        }
    }
}

extern u32 gHudSeasonColors[];
struct Pt;
void Hud_WeaponIconUV(u8 *e, struct Pt *uv0, struct Pt *uv1);
s32 Prim_DrawLine(PrimLine *l);
void Hud_DrawStatusIcon(u16 mask, s32 idx, u32 x, u16 y);

/* A player's HUD panel at x, y: for another player the name plate and health bar, then the
 * colour icon and name, the target mark when a monster in the player's area is on them (or in
 * versus), the status icon and the flashing counter ring. */
void Hud_DrawPlayerStatus(Player *p, s16 y, f32 x)
{
    Sprite s;
    PrimLine l;
    Em *e;
    f32 x1;
    f32 r;
    f32 lx;
    s32 on;
    s32 hidden;
    s32 i;
    s16 ty2;
    s32 ty;
    s16 cx;
    s16 cy;

    Gfx_SetState63(1);
    Gfx_SetBlend(4, 5);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    if (p->index != gScene.localPlayer) {
        s.x = 12.0f + x;
        s.y = y;
        s.w = 0x5B;
        s.h = 0x14;
        *(s32 *)&s.u0 = 0x700038;
        *(s32 *)&s.u1 = 0x8D0056;
        s.color = gHudSeasonColors[(u8)Season_Get()];
        Sprite_DrawStretch9(&s, 8, 8, 8, 8);
        s.color = 0xFF000000;
        s.y = y;
        s.h = 0x16;
        s.x = x;
        s.w = 0x19;
        *(s32 *)&s.u0 = 0x770000;
        *(s32 *)&s.u1 = 0x8D0019;
        Sprite_Draw((struct Sprite *)&s);
        s.x += s.w;
        s.w = 0x41;
        s.u0 = 0x19;
        s.u1 = 0x23;
        Sprite_Draw((struct Sprite *)&s);
        s.x += s.w;
        s.w = 0x13;
        s.u0 = 0x23;
        s.u1 = 0x36;
        Sprite_Draw((struct Sprite *)&s);
        r = (f32)p->hp / (f32)p->hpMax;
        lx = 22.0f + x;
        ty = y;
        l.x0 = lx;
        l.x1 = lx + 72.0f * r;
        l.y0 = l.y1 = ty + 0x12;
        l.color = 0xFF00FF00;
        Prim_DrawLine(&l);
        ty2 = y;
    } else {
        ty = y;
        ty2 = ty - 2;
    }
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    x1 = 11.0f + x;
    s.x = x1;
    s.y = ty + 2;
    s.w = 0x14;
    s.h = 0x12;
    s.color = gPlayerMapColors[p->index & 3];
    Hud_WeaponIconUV((u8 *)p, (struct Pt *)&s.u0, (struct Pt *)&s.u1);
    Sprite_Draw((struct Sprite *)&s);
    Font_SetSpacing(0x10, 0x10);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(33.0f + x, ty + 1);
    Hud_PrintPlayerName(p);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    on = 0;
    if (Quest_IsVersus() == 1) {
        on = 1;
    } else if (p->inBattle != 0) {
        on = 1;
    } else {
        for (e = (Em *)gEms, i = 0; i < 20; i++, e++) {
            if (e->active != 0 && e->area == p->area && e->type != 0xF && e->type != 0x2D) {
                if (e->showOnMap != 0 && e->curGoalKind == 1 && e->curGoalArg == 0 && p->index == e->curGoalNo
                    && e->combat == 1) {
                    on = 1;
                    break;
                }
                if (e->combatPlayers & (1 << p->index)) {
                    on = 1;
                    break;
                }
            }
        }
    }
    if (on > 0) {
        s.x = 109.0f + x;
        s.w = 0x19;
        s.y = ty2 + 2;
        s.h = 0x14;
        s.color = 0xFFFFFFFF;
        *(s32 *)&s.u0 = 0xED0078;
        *(s32 *)&s.u1 = 0xFF008C;
        Sprite_Draw((struct Sprite *)&s);
    }
    Hud_DrawStatusIcon(p->statusIcons, (u8)p->index, x, ty2);
    if (gHudState->pingTimer[p->index] >= 0) {
        hidden = (p->statusIcons & 0x800) != 0;
        if (p->index == gScene.localPlayer || hidden != 1) {
            Gfx_SetBlend(4, 1);
            Tex_Prepare(1, 0x144);
            Tex_Bind(0x144);
            cx = (s16)x1 + 10;
            cy = ty + 11;
            if (hidden != 1) {
                Map_DrawPingGlow(cx, cy, gHudState->pingTimer[p->index], gPlayerMapColors[p->index & 3]);
            } else {
                Map_DrawPingGlow(cx, cy, gHudState->pingTimer[p->index] / 4, gPlayerMapColors[p->index & 3]);
            }
        }
    }
}

typedef struct UVRect {
    Vec2s a;
    Vec2s b;
} UVRect;

extern UVRect gStatusIconUVs[];
#define TIMER(i) (((s16 *)((u8 *)gHudState + 0x10))[i])
#define SEL(i) (gHudState->statusIcon[i])

/* The status icon of slot idx: cycles through the set bits of mask (15 kinds), 20 frames each. */
/* TIMER and SEL are spelled so that the index is added first (k + base), as in the game. */
void Hud_DrawStatusIcon(u16 mask, s32 idx, u32 x, u16 y)
{
    SpriteUV s;
    UVRect *uv;
    s16 *tp;
    s32 t;

    if (mask & 0x7FFF) {
        u8 k = idx;

        tp = &TIMER(k);
        t = *tp;
        if (t < 0) {
            for (t = 0; t < 15U; t++) {
                if (!(mask & (1 << t)) == 0) {
                    SEL(k) = t;
                    TIMER(k) = 20;
                    break;
                }
            }
        } else if ((mask & (1 << SEL(k))) && t > 0) {
            *tp = t - 1;
        } else {
            for (t = 15; t != 0; t--) {
                (SEL(k))++;
                if (SEL(k) >= 15) {
                    SEL(k) = 0;
                }
                if (mask & (1 << SEL(k))) {
                    TIMER(k) = 20;
                    break;
                }
            }
        }
        s.x = 134.0f + x;
        s.y = y + 2;
        s.w = 0x19;
        s.h = 0x14;
        s.color = 0xFFFFFFFF;
        uv = &gStatusIconUVs[SEL(k)];
        s.uv0 = uv->a;
        s.uv1 = uv->b;
        Sprite_Draw((struct Sprite *)&s);
    } else {
        TIMER((u8)idx) = -1;
    }
}

#undef TIMER
#undef SEL

extern s16 D_005B3858;
extern s16 D_005B3850;
extern u8 D_005B3860;
extern u32 gClockColors[];
extern u8 D_004308C0[];
extern u32 D_004308C8[];
extern u32 D_004308DC[];
extern u32 gHudSeasonColors[];
s32 Quest_GetTime(s32 which);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Season_Get(void);
void Compass_DrawMarker(s16 a, s16 b, u8 kind);
void Sprite_DrawStretch9(Sprite *src, s16 w, s16 h, s16 du, s16 dv);
void Hud_DrawHealthStamina(void);
void Hud_DrawCompass(s32 angle, u8 kind);
u16 Clock_Angle(s32 x);
void Hud_DrawCompassQuad(s32 angle, s32 k);
void Sprite_DrawList(u8 *s, s32 n);
f32 Math_Sin(f32 x);

/* The quest clock: the compass markers, the clock face and its frame (colour by kind
 * Season_Get), its rim pulsing faster as the time left (+0x24) runs out, and the hands for the
 * time gone (+0x28 - +0x24), the hand at +0xAC catching up 0x16C a frame. */
void Hud_DrawClock(void)
{
    s32 total;
    Sprite s;
    s32 left;
    u32 color;
    u8 c;
    f32 f;
    u16 hand;
    u8 kind;
    s32 gone;

    Gfx_SetState63(1);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    if (gScene.questPhase == 2) {
        gHudState->timeLeft = Quest_GetTime(0);
        gHudState->timeTotal = Quest_GetTime(1);
    } else if (gScene.questPhase == 3 || gScene.questPhase == 4) {
        if (gQuest.flags & 0x2000) {
            gHudState->timeLeft = 0;
        }
    }
    left = gHudState->timeLeft;
    total = gHudState->timeTotal;
    kind = (u8)Season_Get();
    Compass_DrawMarker(0, D_005B3858, kind);
    Compass_DrawMarker(0, D_005B3858 + 0x4000, kind);
    Compass_DrawMarker(0, D_005B3858 + 0x8000, kind);
    Compass_DrawMarker(0, D_005B3858 + 0xC000, kind);
    Compass_DrawMarker(D_005B3850, 0x2000, kind);
    Compass_DrawMarker(D_005B3850, 0x6000, kind);
    Compass_DrawMarker(D_005B3850, -0x6000, kind);
    Compass_DrawMarker(D_005B3850, -0x2000, kind);
    Game_PauseState();
    s.x = 0x1B;
    s.y = 0x62;
    s.w = 0x5B;
    s.h = 0x14;
    *(s32 *)&s.u0 = 0x700038;
    *(s32 *)&s.u1 = 0x8D0056;
    s.color = gHudSeasonColors[kind];
    Sprite_DrawStretch9(&s, 8, 8, 8, 8);
    s.x = 0xF;
    s.y = 8;
    s.w = 0x6D;
    s.h = 0x6E;
    *(s32 *)&s.u0 = 0;
    *(s32 *)&s.u1 = 0x6E006D;
    s.color = gClockColors[kind];
    Sprite_Draw((struct Sprite *)&s);
    Hud_DrawHealthStamina();
    Gfx_SetBlend(4, 5);
    s.x = 0x1E;
    s.y = 0x47;
    s.w = s.h = 0xE;
    s.u0 = 0x70;
    s.v0 = kind * 16 + 0x5C;
    s.u1 = 0x7E;
    s.v1 = s.v0 + 0xE;
    s.color = gClockColors[kind];
    Sprite_Draw((struct Sprite *)&s);
    if (left > 9000) {
        color = 0xC07F7F7F;
    } else {
        if (left > 1800) {
            f = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x1F) << 11));
            c = (s8)(60.0f * f) + 0xC0;
        } else {
            f = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0xF) << 12));
            c = (s8)(76.0f * f) + 0xAC;
        }
        color = c << 8 | 0xC0FF0000 | c;
    }
    D_004308DC[0] = color;
    D_004308C8[0] = color;
    Sprite_DrawList(D_004308C0, 2);
    Gfx_SetState63(0);
    gone = total - left;
    if (gone < 0) {
        gone = 0;
    }
    hand = Clock_Angle(gone);
    if (hand < gHudState->clockHand) {
        gHudState->clockHand = hand;
    }
    Hud_DrawCompass((s16)(0x10000 - gHudState->clockHand), kind);
    if (D_005B3860 != 0) {
        Hud_DrawCompassQuad((s16)gHudState->clockHand, 0);
    } else {
        Hud_DrawCompassQuad((s16)Clock_Angle(gone), 0);
    }
    Hud_DrawCompassQuad((s16)Clock_Angle(total), 1);
    if (Game_PauseState() == 0 && gHudState->clockHand < hand) {
        gHudState->clockHand += 0x16C;
    }
}

/* A textured triangle as Prim_DrawTexTri draws it. */
extern u32 gCompassColors[];
extern f32 gCompassPoints[][2];
extern u8 gCompassUVs[][4];
void Math_GetSinCos(f32 *s, f32 *c, f32 a);
s32 Prim_DrawTexTri(struct PrimTexTri *t);

/* The compass: eight triangles around (0x42, 0x40), the points of gCompassPoints turned by `angle`
 * (0x10000 a turn); kind selects the colour. */
void Hud_DrawCompass(s32 angle, u8 kind)
{
    Tri t;
    f32 s;
    f32 c;
    f32 (*p)[2];
    s32 i;
    u8 (*uv)[4];

    Math_GetSinCos(&s, &c, 6.2831855f * angle / 65536.0f);
    t.x0 = 0x42;
    t.y0 = 0x40;
    t.color = gCompassColors[kind];
    t.u0 = 0x8A;
    t.v0 = 0x1C;
    p = gCompassPoints;
    uv = gCompassUVs;
    t.x2 = (s16)(0.5f + ((*p)[0] * c - (*p)[1] * s)) + 0x42;
    t.y2 = (s16)(0.5f + ((*p)[0] * s + (*p)[1] * c)) + 0x40;
    for (i = 8; i != 0; i--) {
        t.x1 = t.x2;
        t.y1 = t.y2;
        p++;
        t.x2 = (s16)(0.5f + ((*p)[0] * c - (*p)[1] * s)) + 0x42;
        t.y2 = (s16)(0.5f + ((*p)[0] * s + (*p)[1] * c)) + 0x40;
        t.u1 = (*uv)[0];
        t.v1 = (*uv)[1];
        t.u2 = (*uv)[2];
        t.v2 = (*uv)[3];
        uv++;
        Prim_DrawTexTri((struct PrimTexTri *)&t);
    }
}

extern u32 gCompassMarkerColors[];

/* A marker on the compass ring (radius 28 around (66, 64)) at angle b, pointing along a - b (kept in b): a
 * 12x12 textured square as two triangles, colour gCompassMarkerColors[kind]. */
void Compass_DrawMarker(s16 a, s16 b, u8 kind)
{
    Tri t;
    f32 s;
    f32 c;
    f32 cx;
    f32 cy;

    t.color = gCompassMarkerColors[kind];
    Math_GetSinCos(&s, &c, 6.2831855f * b / 65536.0f);
    cx = 66.0f - 28.0f * s;
    cy = 64.0f - 28.0f * c;
    b = (u16)(a - b);
    Math_GetSinCos(&s, &c, 6.2831855f * b / 65536.0f);
    t.x0 = (cx + -6.0f * c) - -6.0f * s;
    t.y0 = -6.0f * c + (cy + -6.0f * s);
    t.x1 = (cx + 6.0f * c) - -6.0f * s;
    t.y1 = -6.0f * c + (cy + 6.0f * s);
    t.x2 = (cx + -6.0f * c) - 6.0f * s;
    t.y2 = 6.0f * c + (cy + -6.0f * s);
    t.u0 = 0x9B;
    t.v0 = 1;
    t.u1 = 0xA7;
    t.v1 = 1;
    t.u2 = 0x9B;
    t.v2 = 0xD;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
    t.x0 = cx + 6.0f * c - 6.0f * s;
    t.y0 = cy + 6.0f * s + 6.0f * c;
    t.u0 = 0xA7;
    t.v0 = 0xD;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
}

u16 Clock_Angle(s32 x)
{
    return (x / 9000) * 0x1555;
}

/* A textured quad (two triangles) around (0x42, 0x40): four points and their UVs. */
typedef struct QuadDef {
    f32 p[4][2];        /* 0x00 */
    u8 uv[4];           /* 0x20: u0, v0, u1, v1 */
    u32 color;          /* 0x24 */
} QuadDef;

extern QuadDef gCompassQuads[];

/* Draws quad k of gCompassQuads turned by `angle` (0x10000 a turn). */
void Hud_DrawCompassQuad(s32 angle, s32 k)
{
    QuadDef *d = &gCompassQuads[k];
    Tri t;
    f32 s;
    f32 c;

    Math_GetSinCos(&s, &c, 0.0000958738f * angle);
    t.color = d->color;
    t.u0 = d->uv[0];
    t.v0 = d->uv[1];
    t.u1 = d->uv[2];
    t.v1 = d->uv[1];
    t.u2 = d->uv[0];
    t.v2 = d->uv[3];
    t.x0 = (s16)(0.5f + (d->p[0][0] * c - d->p[0][1] * s)) + 0x42;
    t.y0 = (s16)(0.5f + (d->p[0][0] * s + d->p[0][1] * c)) + 0x40;
    t.x1 = (s16)(0.5f + (d->p[1][0] * c - d->p[1][1] * s)) + 0x42;
    t.y1 = (s16)(0.5f + (d->p[1][0] * s + d->p[1][1] * c)) + 0x40;
    t.x2 = (s16)(0.5f + (d->p[2][0] * c - d->p[2][1] * s)) + 0x42;
    t.y2 = (s16)(0.5f + (d->p[2][0] * s + d->p[2][1] * c)) + 0x40;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
    t.u0 = d->uv[2];
    t.v0 = d->uv[3];
    t.x0 = (s16)(0.5f + (d->p[3][0] * c - d->p[3][1] * s)) + 0x42;
    t.y0 = (s16)(0.5f + (d->p[3][0] * s + d->p[3][1] * c)) + 0x40;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
}

void Hud_DrawGaugeFrame(Gauge *g, s32 thin);
void Hud_DrawGaugeFill(Gauge *g, s32 thin);
u32 Hud_HealthColor(s16 kind, Player *p);
u32 Hud_HealthBarColor(Player *p);
u32 Hud_StaminaColor(s16 kind, Player *p);
s32 Player_StaminaLocked(Player *p);
s16 Area_Temperature(u16 area);

/* Draws the local player health and stamina gauges: the frames, the recoverable (red) health,
 * the health in its colour (by the area kind), the stamina blinking red below 76 or pulsing
 * (Player_StaminaLocked) or in its colour. */
void Hud_DrawHealthStamina(void)
{
    Player *p = gHudState->player;
    u32 hpColor;
    u32 stColor;
    Gauge g;
    s16 kind;
    u8 v;
    f32 f;
    s32 r;
    s32 b;

    Gs_SetState(0x6C, 1);
    Gs_SetState(0x6D, 7);
    Gfx_SetState63(1);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    hpColor = 0xFF10C010;
    stColor = 0xFFF0F000;
    kind = Area_Temperature(p->area);
    switch (kind) {
    default:
        gHudState->staminaBlink = 0;
        gHudState->hpBlink = 0;
        break;
    case 0:
    case 1:
    case 3:
        hpColor = Hud_HealthColor(kind, p);
        break;
    case 2:
    case 4:
        stColor = Hud_StaminaColor(kind, p);
        hpColor = Hud_HealthBarColor(p);
        break;
    }
    g.x = 119.0f;
    g.y = 0x15;
    g.len = 288.0f;
    g.max = 100;
    g.cur = p->hpMax;
    g.color = 0xFF000000;
    Hud_DrawGaugeFrame(&g, 0);
    if (p->hp > 0) {
        if (gHudState->hp < p->hpRecoverable) {
            g.cur = p->hpRecoverable;
            g.color = 0xFFC01010;
            Hud_DrawGaugeFill(&g, 0);
        }
        g.cur = gHudState->hp;
        g.color = hpColor;
        Hud_DrawGaugeFill(&g, 0);
    }
    g.y = 0x24;
    g.max = 300;
    g.cur = p->staminaMax;
    g.color = 0xFF000000;
    Hud_DrawGaugeFrame(&g, 1);
    if (p->stamina < 0x4C) {
        v = (s16)(64.0f * Math_Sin(6.2831855f * ((gFrameTick % 45) * 8) / 360.0f)) + 0x40;
        g.color = v << 8 | 0xFFFF0000 | v;
    } else if (Player_StaminaLocked(p) == 0) {
        g.color = stColor;
    } else {
        f = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10));
        r = (u8)((s8)(18.0f * f) + 0xE7);
        b = r << 16 | 0xFF000000 | r << 8;
        g.color = (u8)((s8)(40.0f * f) + 0x9F) | b;
    }
    g.cur = p->stamina;
    Hud_DrawGaugeFill(&g, 1);
}

s32 func_003B5F20(Player *p);
s16 Player_CountSpecialItems(Player *p);

s32 Hud_HealthFlashes(void *p)
{
    if (Player_CountSpecialItems(p) > 0) {
        return 1;
    }
    if (*(u16 *)((u8 *)p + 0x4B8) & 1) {
        return 1;
    }
    if (func_003B5F20((Player *) p) == 1) {
        return 1;
    }
    return 0;
}

extern s16 gHudGaugeFlashSpeeds[][5];
u8 Player_GetSkill(Player *p, u8 group);
f32 Math_Cos(f32 x);

/* The colour of gauge `kind` (0, 1 or 3) of p: pulsing at a speed from the skill of group 0x1E
 * while p->unk870 is clear (or when Hud_HealthFlashes holds), else 0xFF10C010. */
u32 Hud_HealthColor(s16 kind, Player *p)
{
    u32 color = 0xFF10C010;
    s16 speed = 0;
    u8 col;
    u8 lvl;
    u8 skill;
    f32 c;

    if (p->coolDrinkTimer == 0) {
        skill = Player_GetSkill(p, 0x1E);
        if (skill != 0x86) {
            switch (skill) {
            case 0x85:
                lvl = 0;
                break;
            case 0x84:
                lvl = 1;
                break;
            case 0x87:
                lvl = 3;
                break;
            case 0x88:
                lvl = 4;
                break;
            default:
                lvl = 2;
                break;
            }
            switch (kind) {
            case 0:
                lvl = col = 0;
                break;
            case 1:
                col = 0;
                break;
            case 3:
                col = 1;
                break;
            }
            speed = gHudGaugeFlashSpeeds[col][lvl];
        }
    }
    if (speed == 0 && Hud_HealthFlashes(p) == 1) {
        speed = 0x100;
    }
    if (speed != 0) {
        gHudState->hpBlink += speed;
        c = Math_Cos(0.0000958738f * gHudState->hpBlink);
        color = (((s8)(-28.0f * c) + 0x2D) << 16) | 0xFF000000 | (((s8)(15.0f * c) + 0xB1) << 8) | ((s8)(-72.0f * c) + 0x58);
    } else {
        gHudState->hpBlink = 0;
    }
    return color;
}

f32 Math_Cos(f32 x);

u32 Hud_HealthBarColor(Player *p)
{
    u32 color = 0xFF10C010;
    f32 s;

    if (Hud_HealthFlashes(p) == 1) {
        gHudState->hpBlink += 0x100;
        s = Math_Cos(0.0000958738f * gHudState->hpBlink);
        color = (((s8)(-28.0f * s) + 0x2D) << 16) | 0xFF000000 | (((s8)(15.0f * s) + 0xB1) << 8) | ((s8)(-72.0f * s) + 0x58);
    } else {
        gHudState->hpBlink = 0;
    }
    return color;
}

/* The colour of gauge `kind` (2 or 4) of p: pulsing at a speed from the skill of group 0x1D while
 * p->unk872 is clear, else 0xFFF0F000. */
u32 Hud_StaminaColor(s16 kind, Player *p)
{
    u32 color = 0xFFF0F000;
    s16 speed = 0;
    u8 col;
    u8 lvl;
    u8 skill;
    f32 c;

    if (p->hotDrinkTimer == 0) {
        skill = Player_GetSkill(p, 0x1D);
        if (skill != 0x8B) {
            switch (skill) {
            case 0x8A:
                lvl = 0;
                break;
            case 0x89:
                lvl = 1;
                break;
            case 0x8C:
                lvl = 3;
                break;
            case 0x8D:
                lvl = 4;
                break;
            default:
                lvl = 2;
                break;
            }
            switch (kind) {
            case 2:
                col = 0;
                break;
            case 4:
                col = 1;
                break;
            }
            speed = gHudGaugeFlashSpeeds[col][lvl];
        }
    }
    if (speed != 0) {
        gHudState->staminaBlink += speed;
        c = Math_Cos(0.0000958738f * gHudState->staminaBlink);
        color = (((s8)(58.0f * c) + 0xB6) << 16) | 0xFF000000 | (((s8)(102.0f * c) + 0x8A) << 8) | ((s8)(-21.0f * c) + 0x15);
    } else {
        gHudState->staminaBlink = 0;
    }
    return color;
}

/* A gauge frame's texture piece: the st rectangle of its body (u0..v0) and end cap (u1, v1),
 * the body's height, cap's width and the frame's y offset. */
typedef struct GaugeFrame {
    f32 u0;             /* 0x00 */
    f32 v0;             /* 0x04 */
    f32 u1;             /* 0x08 */
    f32 v1;             /* 0x0C */
    u16 h;              /* 0x10 */
    u16 capw;           /* 0x12 */
    u16 dy;             /* 0x14 */
    u16 pad;
} GaugeFrame;

extern GaugeFrame gGaugeFrames[];
s32 Prim_DrawSpriteF(PrimSpriteF *sp);

/* Draws the frame of gauge g: body pieces 45 pixels wide along its length, then the end cap. */
void Hud_DrawGaugeFrame(Gauge *g, s32 thin)
{
    GaugeFrame *e = &gGaugeFrames[thin];
    u32 rest;
    u32 i;
    PrimSpriteF f;
    PrimSpriteF b;
    u32 n;
    f32 x;

    b.v[0].z = b.v[1].z = f.v[0].z = f.v[1].z = -0.9f;
    b.v[0].q = b.v[1].q = f.v[0].q = f.v[1].q = 1.0f;
    f.color = b.color = g->color;
    f.v[1].y = g->y + e->h + e->dy;
    f.v[0].y = f.v[1].y + e->h;
    b.v[0].y = g->y;
    b.v[1].y = b.v[0].y + e->h;
    f.st[0][1] = b.st[0][1] = e->v0;
    f.st[1][1] = b.st[1][1] = e->v0 + 0.00390625f * e->h;
    n = 1.1295f * (g->len * ((f32)g->cur / (f32)g->max));
    rest = n % 45;
    f.st[0][0] = b.st[0][0] = e->u0;
    f.st[1][0] = b.st[1][0] = 0.140625f + e->u0;
    x = g->x;
    for (i = n / 45; i != 0; i--) {
        f.v[0].x = x;
        x += 45.0f;
        f.v[1].x = x;
        Prim_DrawSpriteF(&f);
        b.v[0].x = f.v[0].x;
        b.v[1].x = f.v[1].x;
        Prim_DrawSpriteF(&b);
    }
    f.v[0].x = x;
    f.v[1].x = x + rest;
    f.st[1][0] = e->u0 + 0.003125f * rest;
    Prim_DrawSpriteF(&f);
    b.v[0].x = f.v[0].x;
    b.v[1].x = f.v[1].x;
    b.st[1][0] = f.st[1][0];
    Prim_DrawSpriteF(&b);
    b.v[0].x = 15.0f + b.v[1].x;
    b.v[1].y = b.v[0].y + e->capw;
    b.st[0][0] = 0.046875f + e->u1;
    b.st[1][0] = e->u1;
    b.st[0][1] = e->v1;
    b.st[1][1] = e->v1 + 0.00390625f * e->capw;
    Prim_DrawSpriteF(&b);
}

/* Draws the filled part of gauge g (3 or 5 pixels high). */
void Hud_DrawGaugeFill(Gauge *g, s32 thin)
{
    SpriteW s;
    f32 w;

    if (g->cur <= 0 || g->max <= 0) {
        return;
    }
    w = 1.1295f * (g->len * ((f32)g->cur / (f32)g->max));
    if (thin == 0) {
        s.y = g->y + 6;
        s.h = 5;
    } else {
        s.y = g->y + 5;
        s.h = 3;
    }
    s.x = g->x;
    s.w = w;
    s.color = g->color;
    s.uv0 = 0x400C4;
    s.uv1 = 0x500C6;
    Sprite_Draw((struct Sprite *)&s);
}

u8 Player_GetSharpnessLevel(Player *p);
void Hud_DrawHornNotes(Player *p);
extern u8 gSharpnessBarV[];
extern u32 gSharpnessGlowMasks[];
extern s16 D_00430958[];
extern s16 D_0043095A[];
extern u8 gLongSwordGaugeSprites[];
extern u8 gWeaponBoxSprite[];

/* The weapon box: the sharpness bar of the local player's level (glowing when gSharpnessGlowMasks has a
 * mask for it), then the kind's extras (7: a gauge of +0x776, 8: Hud_DrawHornNotes). */
void Hud_DrawWeaponBox(void)
{
    Player *p = gHudState->player;
    Sprite s;
    u8 lv;
    u32 mask;
    f32 a;

    Gfx_SetState63(0);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    lv = Player_GetSharpnessLevel(p);
    s.x = 0x70;
    s.y = 0x31;
    s.w = 0x50;
    s.h = 0xE;
    s.color = 0xFFFFFFFF;
    s.u0 = 0xC0;
    s.u1 = 0xFF;
    s.v0 = gSharpnessBarV[lv];
    s.v1 = s.v0 + 0xE;
    Sprite_Draw((struct Sprite *)&s);
    mask = gSharpnessGlowMasks[lv];
    if (mask != 0) {
        Gfx_SetState63(1);
        Gfx_SetBlend(4, 1);
        a = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x7F) << 9));
        s.color = ((((s8)(127.0f * a) + 0x7F) << 24) | 0xFFFFFF) & mask;
        s.x = 0x8D;
        s.w = 0x33;
        s.u0 = 0xD7;
        s.v0 -= 0x2A;
        s.v1 -= 0x2A;
        Sprite_Draw((struct Sprite *)&s);
        Gfx_SetBlend(4, 5);
    }
    switch (p->weaponType) {
    case 7:
        if (p->swordGauge > 0) {
            s.x = D_00430958[0];
            s.y = D_0043095A[0] + 4;
            s.w = p->swordGauge * 0x75 / 100;
            s.h = 2;
            s.color = 0xFFFF2020;
            *(s32 *)&s.u0 = 0x400C4;
            *(s32 *)&s.u1 = 0x500C6;
            Sprite_Draw((struct Sprite *)&s);
        }
        Sprite_DrawList(gLongSwordGaugeSprites, 8);
        break;
    case 8:
        Hud_DrawHornNotes(p);
    default:
        Sprite_DrawList(gWeaponBoxSprite, 1);
        break;
    }
}

typedef struct NoteHud {
    u8 pad0[0x101];
    u8 last;            /* 0x101: the note slot shown last */
    u8 anim;            /* 0x102: 6..1 while the new note slides in */
    u8 hist[5];         /* 0x103: the notes shown, newest first */
} NoteHud;

#define NH ((NoteHud *)gHudState)

extern u8 gHornNoteSprites[];
extern u32 gHornNoteColors[];
extern s16 gHornNoteYs[];

/* y0 caches the current row's height for the difference (the sum reads the table again: a local
 * that caches a field for some of its uses), and the interpolation reads the table as u16, as the
 * function's own x = (u16)(...) does. Without y0 the anim and difference registers swap.
 * FAKEMATCH: the history index is (u32)i, which only puts gHudState first in the addu (a permuter
 * find). */
void Hud_DrawHornNotes(Player *p)
{
    SpriteW s;
    u16 y0;
    s32 i;
    s32 x;
    f32 scale;
    u8 note;

    Sprite_DrawList(gHornNoteSprites, 2);
    note = p->hornNoteSlot;
    if (p->hornNotes[note] == 0) {
        NH->last = note;
        NH->anim = 0;
        NH->hist[4] = 0;
        NH->hist[3] = 0;
        NH->hist[2] = 0;
        NH->hist[1] = 0;
        NH->hist[0] = 0;
        return;
    }
    if (NH->last != note) {
        NH->last = note;
        NH->anim = 6;
        NH->hist[4] = NH->hist[3];
        NH->hist[3] = NH->hist[2];
        NH->hist[2] = NH->hist[1];
        NH->hist[1] = NH->hist[0];
        NH->hist[0] = p->hornNotes[p->hornNoteSlot];
    }
    s.uv0 = 0x9D0015;
    s.uv1 = 0xAC0025;
    if (NH->anim > 0) {
        s.w = 20;
        s.h = 15;
        x = (u16)(NH->anim * -24 / 6) + 0xF0;
        for (i = 1; i < 5; i++) {
            if (NH->hist[(u32)i] == 0) {
                break;
            }
            x += 0x18;
            y0 = (u16)gHornNoteYs[i];
            s.x = x;
            s.y = (u16)gHornNoteYs[i] + NH->anim * ((u16)gHornNoteYs[i - 1] - y0) / 6;
            s.color = gHornNoteColors[NH->hist[(u32)i]];
            if (i == 4) {
                s.color &= 0xFFFFFF;
                s.color |= NH->anim * 255 / 6 << 24;
            }
            Sprite_Draw((struct Sprite *)&s);
        }
        if (NH->anim > 3) {
            scale = 1.0f + 0.41666666f * (NH->anim - 3);
        } else {
            scale = 1.0f;
        }
        s.w = 20.0f * scale;
        s.h = 15.0f * scale;
        s.x = 0xFA - s.w / 2;
        s.y = gHornNoteYs[0] + 7 - s.h / 2;
        s.color = gHornNoteColors[NH->hist[0]];
        Sprite_Draw((struct Sprite *)&s);
        NH->anim--;
    } else {
        s.x = 0xF0;
        s.w = 20;
        s.h = 15;
        for (i = 0; i < 4; i++) {
            if (NH->hist[(u32)i] == 0) {
                break;
            }
            s.y = gHornNoteYs[i];
            s.color = gHornNoteColors[NH->hist[(u32)i]];
            Sprite_Draw((struct Sprite *)&s);
            s.x += 0x18;
        }
    }
}

#undef NH

/* A textured triangle with its texel pairs as words (Prim_DrawTexTri). */
typedef struct TriW {
    s16 x0, y0;         /* 0x00 */
    s16 x1, y1;         /* 0x04 */
    s16 x2, y2;         /* 0x08 */
    u32 color;          /* 0x0C */
    s32 uv0;            /* 0x10 */
    s32 uv1;            /* 0x14 */
    s32 uv2;            /* 0x18 */
} TriW;

s32 CamCtl_IsAiming(Player *pl);
s8 CamAim_GetState(s32 p, u8 *show, u8 *bar, f32 *pos);
struct TexRect;
void TexRect_Draw(struct TexRect *r, s8 mode);
extern u8 D_0043189C[];
extern s16 D_0043189E;
extern s16 gCamEventSizes[];
extern s16 D_004318A2[];
extern s16 gHudStepPulse[][2];
extern s16 D_00431876[];
extern s16 D_0043187A[];
extern s16 D_0043188A[];
extern s16 D_0043188E[];
extern u8 gCamEventSprites[];
extern s16 gCamEventTris[];
extern s16 D_004318D4[];
extern s16 D_004318EC[];
extern s16 D_00431908[];
extern s16 D_00431924[];

/* The camera event overlay: a bar of five sprites with a marker at the event's position, and
 * while the event shows, two sprites and two quads placed between their two positions of
 * gHudStepPulse by the step at +0x30 (0..3; they pulse at step 0), a sound when +0x33 is set. */
void Hud_DrawCamEvent(void)
{
    SpriteW s;
    TriW t;
    f32 pos;
    f32 f;
    u8 show;
    u8 bar;
    s32 p;
    s32 i;
    s16 (*q)[2];
    s32 k;
    s16 rk;

    p = (s32)gHudState->player;
    if (CamCtl_IsAiming((Player *)p) == 0) {
        gHudState->camEvent = -1;
        return;
    }
    Gfx_SetState63(1);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    gHudState->camEvent = CamAim_GetState(p, &show, &bar, &pos);
    if (bar != 0) {
        Sprite_DrawList(D_0043189C, 2);
        s.x = 0x107;
        s.y = D_0043189E;
        s.w = gCamEventSizes[0];
        s.h = D_004318A2[0];
        s.color = -1;
        s.uv0 = 0xC40000;
        s.uv1 = 0xD80014;
        i = 5;
        do {
            Sprite_Draw((struct Sprite *)&s);
            s.x += 0x18;
        } while (--i != 0);        s.x = 246.0f + 142.0f * pos;
        s.y = 0x15D;
        s.w = 0xC;
        s.h = 0xC;
        s.color = 0xFFFF0000;
        s.uv0 = 0x8F0061;
        s.uv1 = 0x9A006A;
        TexRect_Draw((struct TexRect *)&s, 2);
    }
    if (show != 0) {
        if (gHudState->camEventFade < 4) {
            k = (s16)gHudState->camEventFade;
            rk = 4 - k;
            if (gHudState->camEventFade == 0) {
                f = Math_Cos(0.0000958738f * (u16)((gHudState->camEventBlink & 0x1F) << 11));
                s.color = ((s8)(94.0f * f) + 0xA1) << 24 | 0xFF0000;
                (gHudState->camEventBlink)++;
            } else {
                s.color = 0xFFFF0000;
                gHudState->camEventBlink = 0;
            }
            q = gHudStepPulse;
            s.x = (q[0][0] * k + q[0][1] * rk) >> 2;
            s.y = D_00431876[0];
            s.w = 0x19;
            s.h = D_0043187A[0];
            s.uv0 = 0x8F0056;
            s.uv1 = 0x9A006A;
            Sprite_Draw((struct Sprite *)&s);
            s.x = (q[1][0] * k + q[1][1] * rk) >> 2;
            s.y = D_0043188A[0];
            s.w = 0x19;
            s.h = D_0043188E[0];
            s.uv0 = 0x8F0069;
            s.uv1 = 0x9A0055;
            Sprite_Draw((struct Sprite *)&s);
            q++;
            q++; /* two steps: MWCC folds `q += 2` into the offsets */
            t.color = s.color;
            t.x0 = t.x2 = gCamEventTris[0];
            t.x1 = D_004318D4[0];
            t.y0 = t.y1 = (q[0][0] * k + q[0][1] * rk) >> 2;
            t.y2 = t.y0 + 0xC;
            t.uv0 = 0x9A0061;
            t.uv1 = 0x8F0061;
            t.uv2 = 0x9A006A;
            Prim_DrawTexTri((struct PrimTexTri *)&t);
            t.x0 = t.x1;
            t.y0 = t.y2;
            t.uv0 = 0x8F006A;
            Prim_DrawTexTri((struct PrimTexTri *)&t);
            t.x0 = D_00431908[0];
            t.y2 = (q[1][0] * k + q[1][1] * rk) >> 2;
            t.y0 = t.y1 = t.y2 + 0xC;
            t.uv0 = 0x9A0061;
            Prim_DrawTexTri((struct PrimTexTri *)&t);
            t.x0 = t.x1;
            t.y0 = t.y2;
            t.uv0 = 0x8F006A;
            Prim_DrawTexTri((struct PrimTexTri *)&t);
            if ((u8)gHudState->camEventStart != 0) {
                Sound_PlaySeFlat(0, 0x20, 0);
            }
        }
        Sprite_DrawList(gCamEventSprites, 3);
        Prim_DrawTexTri((struct PrimTexTri *)gCamEventTris);
        Prim_DrawTexTri((struct PrimTexTri *)D_004318EC);
        Prim_DrawTexTri((struct PrimTexTri *)D_00431908);
        Prim_DrawTexTri((struct PrimTexTri *)D_00431924);
    }
}

s16 Player_IsState(void *obj, u8 a, u8 b);
u16 Bowgun_NextAmmoSlot(Player *p, u16 slot, u8 dir);
void Hud_DrawBlinkIcon(s32 i);
void Hud_DrawBowgunClip(Player *p, u8 kind);

/* The bowgun HUD (weapon kinds 1, 5 and 9): the clip and the reload prompts. */
void Hud_DrawBowgun(void)
{
    Player *p = gHudState->player;

    if (p->weaponType == 1 || p->weaponType == 5 || p->weaponType == 9) {
        Gfx_SetState63(0);
        Tex_Prepare(1, 0x146);
        Tex_Bind(0x146);
        if (p->weaponType == 9) {
            Hud_DrawBowgunClip(p, 2);
            if (p->weaponOut != 0) {
                if (p->roundsLoaded <= 0) {
                    Hud_DrawBlinkIcon(0);
                }
                if (Player_IsState(p, 1, 5) != 0) {
                    Hud_DrawBlinkIcon(1);
                }
                if (Player_IsState(p, 1, 0x1B) != 0) {
                    Hud_DrawBlinkIcon(1);
                }
            }
            return;
        }
        if (p->ammoCount == 0) {
            if (p->weaponOut != 0 && Bowgun_NextAmmoSlot(p, p->ammoSlot, 1) != 0xFF) {
                Hud_DrawBlinkIcon(2);
            }
            return;
        }
        Hud_DrawBowgunClip(p, gHudState->camEvent == 0);
        if (p->roundsLoaded <= 0 && p->weaponOut != 0) {
            Hud_DrawBlinkIcon(0);
        }
        if (p->motionId[0] == 0x57C) {
            Hud_DrawBlinkIcon(1);
        }
    }
}

extern IconDef gBlinkIconDefs[];

void Hud_DrawBlinkIcon(s32 i)
{
    SpriteW s;
    IconDef *d = &gBlinkIconDefs[i];
    u8 a;

    a = (s8)(96.0f * Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x1F) << 11))) + 0x9F;
    if (gHudState->camEvent != 0) {
        s.x = 0x78;
        s.y = 0x3A;
        s.color = (a << 24) | 0xFFFFFF;
        if ((gHudState->player)->weaponType == 9) {
            s.x += 0x64;
        }
    } else {
        s.x = d->x;
        s.y = 0xD5;
        s.color = 0xFF000000 | (a << 16) | (a << 8) | a;
    }
    s.w = d->w;
    s.h = d->h;
    s.uv0 = d->uv0;
    s.uv1 = d->uv1;
    Sprite_Draw((struct Sprite *)&s);
}

u8 Player_GetSharpnessLevel(Player *p);

/* The bowgun's clip: p->clipSize slots, the first p->roundsLoaded of them loaded (kind 0-2: the
 * position). */
void Hud_DrawBowgunClip(Player *p, u8 kind)
{
    SpriteW s;
    u32 i;

    s.color = 0xFFFFFFFF;
    switch (kind) {
    case 0:
        s.x = 0x78;
        s.y = 0x32;
        break;
    case 1:
        s.x = 0x13E - p->clipSize * 15 / 2;
        s.y = 0x123;
        break;
    case 2:
        if (Player_GetSharpnessLevel(p) == 0) {
            s.color = 0xFFFF0000;
        }
        s.x = 0xDC;
        s.y = 0x32;
        break;
    }
    s.w = 0xF;
    s.h = 0x14;
    for (i = 0; i < p->clipSize; i++) {
        if (i < p->roundsLoaded) {
            s.uv0 = 0xED00D8;
            s.uv1 = 0xFF00E4;
        } else {
            s.uv0 = 0xED00E4;
            s.uv1 = 0xFF00F0;
        }
        Sprite_Draw((struct Sprite *)&s);
        s.x += s.w;
    }
}

s32 Player_HasSkill(Player *p, u8 skill);

s32 Hud_GlowCond_Unk(void *arg0)
{
    Player *p;

    if (*(u8 *)((u8 *)arg0 + 0x532) != 0) {
        return 1;
    }
    p = gHudState->player;
    if ((*(u16 *)&p->detectTimer | *(u16 *)&p->actDetectTimer) != 0) {
        return 1;
    }
    return Player_HasSkill((Player *) p, 0xA2U);
}

void Hud_TriggerGlow(Em *em)
{
    if ((Hud_GlowCond_Unk((void *) em) != 0) && (gHudState->mode == 0) && (gHudState->onlineHud == 0) && (gHudState->hidden == 0)) {
        gHudState->glowTimer = 0x1A;
    }
}

void Hud_DrawGlowEffect(void)
{
    f32 t;
    u8 glowTimer;

    if (gHudState->glowTimer != 0) {
        Gfx_SetBlend(4, 1);
        Gfx_SetState63(1);
        Tex_Prepare(1, 0x144);
        Tex_Bind(0x144);
        glowTimer = gHudState->glowTimer;
        t = (f32) (u32) glowTimer;
        Hud_DrawGlow(0x200, 0xAC, 0xFFFFFF, 120.0f, t);
        Gfx_SetBlend(4, 5);
    }
}

/* A glow of size `size` fading with t (0-25) around x, y: four quarter sprites. */
void Hud_DrawGlow(s16 x, s16 y, s32 color, f32 size, f32 t)
{
    Sprite s;
    u8 alpha;
    s16 w;

    alpha = 51.0f * Math_Sqrtf(t);
    w = size * (0.291667014f + 0.1180555f * Math_Sqrtf(25.0f - t));
    s.color = (color & 0xFFFFFF) | (alpha << 24);
    s.w = w;
    s.h = w;
    s.x = x;
    s.y = y;
    *(s32 *)&s.u0 = 0xC100E1;
    *(s32 *)&s.u1 = 0xDF00FF;
    Sprite_Draw((struct Sprite *)&s);
    s.y = y - s.h;
    s.v0 = 0xDF;
    s.v1 = 0xC1;
    Sprite_Draw((struct Sprite *)&s);
    s.x = x - s.w;
    s.u0 = 0xFF;
    s.u1 = 0xE1;
    Sprite_Draw((struct Sprite *)&s);
    s.y = y;
    s.v0 = 0xC1;
    s.v1 = 0xDF;
    Sprite_Draw((struct Sprite *)&s);
}

extern Frame gSupplyBoxFrame;
extern char *gSupplySelectText[];
extern char *gSupplyPouchFullText;
extern char *gSupplyCannotCarryText[];
extern char *gSupplyInfiniteFmt;
extern char *gSupplyCountFmt[];
s32 QuestGoal_IsDone(s32 a);
s16 Pouch_Space(Player *p, u16 item);

/* The x of supply box slot i's column. */
#define SUPPLY_X(i) (313.0f + 36.0f * ((i) & 7))

/* The moving item's alpha reuses the u16 local item (a u32 local of its own gives other
 * registers for the Sprite field addresses the function keeps in s1-s3 and s6-s8): found by
 * reducing a round 16 permuter output. */
/* The supply box window, while it is open (Player.supplyBoxOpen): the slots' frames and items (a slot
 * taken is hidden, and so are the quest goals' slots until they are done), the item being taken
 * moving to the pouch, the cursor, then the selected item's name and count, and why it can't be
 * taken (the pouch is full, or has room for fewer). */
void SupplyBox_Draw(void)
{
    Player *p = gHudState->player;
    SceneState *ss = &gScene;
    Line r;
    u32 taken[2];
    Sprite s;
    char buf[32];
    char *msg;
    s16 i;
    s32 k;
    f32 t;
    f32 f;
    f32 sc;
    u8 alpha;
    u16 item;
    s16 n;
    s16 x;
    u8 cur;

    if (p->supplyBoxOpen == 0) {
        return;
    }
    if (gQuest.timedEvent == 0) {
        taken[0] = gScene.supplyTaken[0];
        taken[1] = gScene.supplyTaken[1];
    } else {
        taken[0] = gScene.supplyTaken[0] | 0xFFFFFF;
        taken[1] = gScene.supplyTaken[1] | 0xFFFFFF00;
    }
    if (QuestGoal_IsDone(1) != 1) {
        taken[0] |= 0xFF000000;
    }
    if (QuestGoal_IsDone(2) != 1) {
        taken[1] |= 0xFF;
    }
    Gs_SetState(0x60, 0);
    WindowFrame_Draw(&gSupplyBoxFrame, 0);
    r.color = 0xFF200000;
    for (i = 0; i < 40; i++) {
        r.x0 = (s16)SUPPLY_X(i) + 4;
        r.x1 = r.x0 + 0x20;
        r.y0 = (i >> 3) * 32 + 0x60;
        r.y1 = r.y0 + 0x1C;
        Prim_FillRect((struct PrimLine *)&r);
    }
    Gfx_SetState63(0);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    s.w = 0x28;
    s.h = 0x20;
    for (i = 0; i < 40; i++) {
        if (!(taken[i >> 5] & (1 << (i & 31)))) {
            if (ss->supply[i].item != 0 && ss->supply[i].count != 0) {
                ItemData *d = &gItemData[ss->supply[i].item];

                if (d->icon != 0xFF) {
                    k = d->icon + 1;
                    s.x = SUPPLY_X(i);
                    s.y = (i >> 3) * 32 + 0x5E;
                    s.u0 = (k & 7) * 32 + 1;
                    s.v0 = (k >> 3) * 32 + 1;
                    s.u1 = s.u0 + 0x1E;
                    s.v1 = s.v0 + 0x1E;
                    s.color = gItemColors[d->color];
                    Sprite_Draw((struct Sprite *)&s);
                }
            }
        }
    }
    if (gHudState->supplyMoveTimer > 0) {
        t = (f32)gHudState->supplyMoveTimer / (f32)gHudState->supplyMoveFrames;
        s.x = SUPPLY_X(gHudState->supplyMoveSlot) * t + 315.0f * (1.0f - t);
        s.y = t * ((gHudState->supplyMoveSlot >> 3) * 32 + 0x5E) + 284.0f * (1.0f - t);
        k = gItemData[gHudState->supplyMoveItem].icon + 1;
        s.u0 = (k & 7) * 32 + 1;
        s.v0 = (k >> 3) * 32 + 1;
        s.u1 = s.u0 + 0x1E;
        s.v1 = s.v0 + 0x1E;
        s.color = gItemColors[gItemData[gHudState->supplyMoveItem].color] & 0xFFFFFF;
        item = 45.0f * Math_Sqrtf(gHudState->supplyMoveTimer);
        s.color |= (item & 0xFF) << 24;
        Sprite_Draw((struct Sprite *)&s);
    }
    s.x = SUPPLY_X(p->menuCursor);
    s.y = (p->menuCursor >> 3) * 32 + 0x5E;
    *(s32 *)&s.u0 = 0;
    *(s32 *)&s.u1 = 0x200020;
    s.color = 0xFFFFFFFF;
    Sprite_Draw((struct Sprite *)&s);
    if (gHudState->supplyMsgTimer > 0) {
        f = gHudState->supplyMsgTimer & 0x1F;
        alpha = 45.0f * Math_Sqrtf(f);
        sc = 32.0f * (1.0f + 0.17960416f * Math_Sqrtf(31.0f - f));
        s.w = 1.25f * sc;
        s.h = sc;
        s.x = (s16)SUPPLY_X(gHudState->supplyFlashSlot) - ((s.w - 0x28) >> 1);
        s.y = (gHudState->supplyFlashSlot >> 3) * 32 + 0x5E - ((s.h - 0x20) >> 1);
        s.color = ((alpha & 0xFF) << 24) | 0xFFFFFF;
        Sprite_Draw((struct Sprite *)&s);
    }
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    s.x = 0x13B;
    s.y = 0x11C;
    s.w = 0x32;
    s.h = 0x32;
    *(s32 *)&s.u0 = 0xEC0078;
    *(s32 *)&s.u1 = 0x100008C;
    s.color = 0xFFFFFFFF;
    Sprite_Draw((struct Sprite *)&s);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(0x144, 0x46);
    Text_Draw(gSupplySelectText[0]);
    cur = p->menuCursor;
    item = (taken[cur >> 5] & (1 << (cur & 31))) ? 0 : ss->supply[cur].item;
    n = ss->supply[cur].count;
    if (item == 0 || n == 0) {
        sprintf(buf, D_0046D0D8, gItemNames[0]);
        Font_SetPos(0x1CB - Str_Length(buf) * 9 / 2, 0x10A);
        Text_Draw(buf);
        item = 0;
    } else {
        switch (gItemData[item].max) {
        case 1:
            sprintf(buf, (char *)gItemNames[item]);
            break;
        case 0xFF:
            sprintf(buf, gSupplyInfiniteFmt, gItemNames[item]);
            break;
        default:
            sprintf(buf, gSupplyCountFmt[0], gItemNames[item], n);
            break;
        }
        Font_SetPos(0x1CB - Str_Length(buf) * 9 / 2, 0x10A);
        Text_Draw(buf);
        x = -1;
        k = Pouch_Space(p, item);
        if (k < 0) {
            x = 0x18C;
            msg = gSupplyPouchFullText;
        } else if (k < n) {
            x = 0x17A;
            msg = gSupplyCannotCarryText[0];
        }
        if (x > 0) {
            Font_SetPos(x, 0x128);
            Font_SetColor(0x807F212E);
            Text_Draw(msg);
        }
    }
    HelpWindow_DrawLine(1, item + 0x18);
}

void Hud_StartSupplyMsg(u8 *p)
{
    gHudState->supplyMsgTimer = 0x1F;
    gHudState->supplyFlashSlot = p[0x81B];
}

void SupplyBox_StartMove(u16 a, u8 b)
{
    f32 dx;
    f32 dy;
    u8 n;

    gHudState->supplyMoveItem = a;
    gHudState->supplyMoveSlot = b;
    dx = 313.0f + 36.0f * (gHudState->supplyMoveSlot & 7) - 315.0f;
    dy = (gHudState->supplyMoveSlot >> 3) * 32 - 0xBE;
    n = 0.1f * Math_Sqrtf(dx * dx + dy * dy);
    gHudState->supplyMoveTimer = n;
    gHudState->supplyMoveFrames = n;
}

extern ListWin gTentMenuPanel;
extern char *gTentMenuLabels[];
extern char *gSubTargetHelpText[];
extern char *gQuestMenuHelpTexts[];
extern char *gYesNoTexts[];
extern Panel gSupplyBoxTextBox;
s32 Quest_CheckSubClearMode(void);

void SupplyBox_DrawWindow(void)
{
    Player *p = gHudState->player;
    s32 k;
    s32 cursor;
    s32 sel;
    s32 v;
    char *text;

    if (p->deliveryMenu == 0) {
        return;
    }
    switch (p->menuPage) {
    case 0:
        cursor = p->menuCursor;
        k = 0;
        if (Game_IsOnline() != 1) {
            k += 3;
            cursor += 3;
        }
        if (p->menuDelay != 0) {
            gTentMenuPanel.color = 0xA;
            sel = -1;
        } else {
            sel = p->menuCursor;
            gTentMenuPanel.color = 0;
            if (Quest_PhaseIsOver(0) == 1) {
                k += 12;
            } else if (Quest_CheckSubClearMode() != 1) {
                k += 6;
            }
        }
        gTentMenuPanel.items = &gTentMenuLabels[k];
        ListPanel_DrawDim(&gTentMenuPanel, 0, sel);
        HelpWindow_DrawLine(8, cursor);
        break;
    case 1:
        v = Game_IsOnline() != 1;
        switch (p->menuCursor) {
        default:
            return;
        case 1:
            text = gSubTargetHelpText[v];
            break;
        case 2:
            text = gQuestMenuHelpTexts[v];
            break;
        }
        Window_DrawTextB2((struct Window *)&gSupplyBoxTextBox, (u8 *)((s32)text));
        Font_SetPos(0x18F, 0x142);
        Text_Printf(gYesNoTexts[p->yesNoCursor]);
        break;
    }
}

extern char D_0046D0D8[];
extern char *gDeliveryMenuTexts[];
extern char *gDeliveryDoneText[];
extern char *gNotDeliveryText;
extern char *gDeliveryHelpText[];
extern ListPanel gDeliveryPanel;
extern ListPanel gDeliveryPanel2;
extern Panel gDeliveryTextBox;
s32 Quest_GetDeliveryTarget(Player *unused, s32 idx, ItemStack *out);
s32 QuestGoal_AllSubDone(void);
s16 Pouch_Count(Player *p, u16 item);

/* Draws the delivery page of the quest menu (page 2 first shows the goal list, page 1 the
 * Delivery_DrawCount part): the title and, for each of the three targets, its label and the item
 * with the count wanted, coloured by how many the pouch holds. */
void QuestMenu_DrawDelivery(void)
{
    Player *p = gHudState->player;
    ItemStack st;
    char buf[0x4C];
    s16 i;
    char *q;
    s8 n;
    s16 c;

    if (p->deliveryOpen == 0) {
        return;
    }
    switch (p->menuPage) {
    case 2:
        ListPanel_Draw(&gDeliveryPanel, NULL, NULL, gHudState->yesNo);
        Quest_GetDeliveryTarget(p, p->menuCursor, &st);
        if (p->menuCursor == 0 && st.count == gHudState->count && QuestGoal_AllSubDone() == 0) {
            Window_DrawTextB2((struct Window *)&gDeliveryTextBox, (u8 *)((s32)gDeliveryHelpText[0]));
        }
    case 1:
        Delivery_DrawCount();
    case 0:
        ListPanel_Draw(&gDeliveryPanel2, NULL, NULL, p->menuCursor + 1);
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(0x166, 0x50);
        Font_SetColor(0x807F7F28);
        Text_Printf(D_0046D0D8, gDeliveryMenuTexts[0]);
        for (i = 0; i < 3; i++) {
            Font_SetPos(0x166, (i + 1) * 22 + 0x50);
            Font_SetColor(0x807F7F7F);
            q = buf;
            n = 0;
            n += (s8)sprintf(q, D_0046D0D8, gDeliveryMenuTexts[i + 1]);
            switch (Quest_GetDeliveryTarget(&gPlayers[gScene.localPlayer], i, &st)) {
            case 0:
                sprintf(q + n, D_0046D0D8, gNotDeliveryText);
                Font_SetColor(0x80505050);
                break;
            case 1:
                sprintf(q + n, D_0046D0D8, gDeliveryDoneText[0]);
                Font_SetColor(0x802B7F7F);
                break;
            case 2:
                n += (s8)sprintf(q + n, D_0046D0D8, gItemNames[st.item]);
                if (st.count > 1) {
                    sprintf(q + n, TXT(0x0046D100) /* a full-width multiplication sign, then the count */, st.count);
                }
                c = Pouch_Count(p, st.item);
                if (c <= 0) {
                    Font_SetColor(0x80505050);
                } else if (c >= st.count) {
                    Font_SetColor(0x807F7F7F);
                } else {
                    Font_SetColor(0x807F5230);
                }
                break;
            }
            Text_Printf(D_0046D0D8, buf);
        }
        break;
    }
}

extern ListWin gEquipStatusPanel;
extern char *gEquipStatusTitle[];
void PersonalStatus_DrawEquipment(u8 *p);
void EquipWindow_DrawLocal(EquipItem *e, s16 x, s16 y, u8 flags, EquipItem *other);

void EquipStatus_Draw(void)
{
    EquipItem e;
    Line l;
    Player *p;
    EquipItem *pe;

    p = gHudState->player;
    ListPanel_DrawDim(&gEquipStatusPanel, gEquipStatusTitle[0], -1);
    PersonalStatus_DrawEquipment((u8 *)p);
    l.x0 = 0x178;
    l.x1 = 0x266;
    l.y0 = gHudState->pageCursor * 32 + 0x55;
    l.y1 = l.y0 + 0x20;
    l.color = (((s8)(48.0f * Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x3F) << 10))) + 0xBF) << 24) | 0xA9182;
    Prim_FillRect((struct PrimLine *)&l);
    pe = Player_GetEquipBySlot(&e, gHudState->pageCursor, p);
    EquipWindow_DrawLocal(pe, 0x132, 0x12A, gHudState->equipCursor, 0);
}

extern Sprite gBowgunAmmoSprite;
extern char *gAmmoTensFmt[];
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);

/* The bowgun's loaded ammo: the tens as a number, the ones as rounds. */
void Hud_DrawBowgunAmmo(void)
{
    SpriteW s;
    char buf[8];
    Player *p = gHudState->player;
    u8 n;
    u16 item;
    u8 tens;
    s32 ones;
    u32 color;

    Gfx_SetState63(0);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    if (p->coatingSlot != 0xFF) {
        n = p->pouch[p->coatingSlot].count;
        item = p->pouch[p->coatingSlot].item;
        color = gItemColors[gItemData[item].color];
        ones = n % 10;
        s.x = 0x78;
        s.y = 0x34;
        s.w = 0x1F;
        s.h = 0x15;
        s.uv0 = 0x2A00E8;
        s.uv1 = 0x3F0100;
        tens = n / 10;
        s.color = tens != 0 ? color : 0xFF808080;
        Sprite_Draw((struct Sprite *)&s);
        Font_SetSpacing(0x12, 0x12);
        sprintf(buf, gAmmoTensFmt[0], tens);
        Text_DrawShadow(0x98, 0x37, 1, 0, buf);
        s.x = 0xB4;
        s.y = 0x34;
        s.w = 0x14;
        s.h = 0x15;
        s.uv0 = 0xAD0004;
        s.uv1 = 0xC20014;
        s.color = color;
        while (ones != 0) {
            Sprite_Draw((struct Sprite *)&s);
            s.x += 0x14;
            ones--;
        }
    } else {
        Sprite_Draw((struct Sprite *)&gBowgunAmmoSprite);
    }
}

extern Panel gPauseTextBox;
extern char *gPausedText[];

void Pause_DrawWindow(void)
{
    s16 pos[2];

    if (gPauseWinStep != 0) {
        pos[0] = 0;
        pos[1] = gPauseWinFade * -56;
        Window_DrawText(&gPauseTextBox, (s16 *)pos, (u8 *)gPausedText[0]);
    }
}

void Hud_SaveGsState(void)
{
    gHudState->gsSave6C = Gs_GetState(0x6C);
    gHudState->gsSave6D = Gs_GetState(0x6D);
}

void Hud_RestoreGsState(void)
{
    Gs_SetState(0x6C, gHudState->gsSave6C);
    Gs_SetState(0x6D, gHudState->gsSave6D);
}

void Hud_DrawLayer0(void)
{
    Hud_SaveGsState();
    Gs_SetState(0x60, 0);
    Gfx_SetBlend(4, 5);
    Text_SetSize(6);
    Font_SetPal(0);
    if (Game_PauseState() == 0) {
        Hud_DrawVersusMonsterBars();
    }
    Hud_RestoreGsState();
}

void Hud_DrawLayer1(void)
{
    u8 temp_v1;

    Hud_SaveGsState();
    Gs_SetState(0x60, 0);
    Gfx_SetBlend(4, 5);
    Text_SetSize(0);
    Font_SetPal(0);
    if (Game_PauseState() == 0) {
        Hud_DrawCamEvent();
        if (gHudState->hidden == 0) {
            Hud_DrawClock();
            temp_v1 = gHudState->player->weaponType;
            switch (temp_v1) {
            case 10:
                Hud_DrawBowgunAmmo();
                break;
            case 1:
            case 5:
                Hud_DrawBowgun();
                break;
            case 9:
                Hud_DrawBowgun();
                
            default:
                Hud_DrawWeaponBox();
                break;
            }
            Hud_DrawPartyStatus();
            Hud_DrawPlayerNames();
        } else if (gHudState->camEvent == 0) {
            Hud_DrawBowgun();
        }
    }
    Pause_DrawWindow();
    Hud_RestoreGsState();
}

void Hud_DrawLayer2(void)
{
    void *temp_v0;

    Hud_SaveGsState();
    Gfx_SetBlend(4, 5);
    Text_SetSize(1);
    Font_SetPal(0);
    if (gHudState->popupOpen != 0) {
        MsgEvent_DrawPopup();
    } else if (gChatInputMode != 0) {
        Chat_DrawHudLog();
        Hud_DrawMap();
    } else if (gHudState->dropState != 0) {
        Hud_DrawItemWindow();
    } else if (gHudState->mode != 0) {
        FieldMenu_Draw();
    } else {
        temp_v0 = gHudState->player;
        if ((*(u8 *)((u8 *)temp_v0 + 0x8D5) | (*(u8 *)((u8 *)temp_v0 + 0x81A) | *(u8 *)((u8 *)temp_v0 + 0x8B1))) == 0) {
            Hud_DrawMap();
            Hud_DrawPouchItem();
        }
    }
    Hud_RestoreGsState();
}

s8 Kbd_IsOpen(void);
void Hud_DrawButtonIcons(void);
void SupplyBox_Draw(void);
void SupplyBox_DrawWindow(void);
void QuestMenu_DrawDelivery(void);
void Notice_Draw(void);
void Chat_DrawHud(void);
void Hud_FadeDraw(void);
void Chat_DrawKbdBox(s32 text);

void Hud_DrawLayer3(void)
{
    Hud_SaveGsState();
    Text_SetSize(2);
    Font_SetPal(0);
    if (gHudState->popupOpen == 0) {
        Hud_DrawButtonIcons();
        if (gChatInputMode == 0 && gHudState->dropState == 0 && gHudState->mode == 0) {
            SupplyBox_Draw();
            SupplyBox_DrawWindow();
            QuestMenu_DrawDelivery();
        }
        if (gNoticeFlags != 0) {
            Notice_Draw();
            gNoticeFlags = 0;
        } else {
            Chat_DrawHud();
        }
        if (gChatInputMode == 0) {
            Hud_FadeDraw();
        }
        if (Kbd_IsOpen() != 0) {
            Gfx_SetBlend(4, 5);
            Chat_DrawKbdBox(gChatInputMode != 0 ? 0xBDB59F6C : 0xBD6C6CC5);
        }
    }
    Hud_RestoreGsState();
}

void Hud_DrawLayer1B(void)
{
    Hud_SaveGsState();
    Gs_SetState(0x60, 0);
    Gfx_SetBlend(4, 5);
    Text_SetSize(0);
    Font_SetPal(0);
    Pause_DrawWindow();
    Hud_RestoreGsState();
}

void Hud_DrawLayer2B(void)
{
    Hud_SaveGsState();
    Gs_SetState(0x60, 0);
    Gfx_SetBlend(4, 5);
    Text_SetSize(1);
    Font_SetPal(0);
    if (gChatInputMode == 0) {
        if (gHudState->dropState != 0) {
            Hud_DrawItemWindow();
        } else if (gScene.menuOpen != 0) {
            GameMenu_Draw();
        }
        BoxMenu_Draw();
    }
    Hud_RestoreGsState();
}

extern u8 gChatSendTo;
void ChatLog_DrawFull(void);
void LobbyChat_DrawHud(void);

void Hud_DrawLayer3B(void)
{
    u32 c;
    Hud_SaveGsState();
    Text_SetSize(2);
    Font_SetPal(0);
    if (gNoticeFlags != 0) {
        Notice_Draw();
        gNoticeFlags = 0;
    }
    if (gChatInputMode != 0) {
        if (gChatSendTo == 0) {
            ChatLog_DrawFull();
        } else {
            Chat_DrawHudLog();
        }
    } else if (gChatSendTo != 0) {
        Chat_DrawHud();
    } else {
        LobbyChat_DrawHud();
    }
    if (gChatInputMode == 0) {
        Hud_FadeDraw();
    }
    if (Kbd_IsOpen() != 0) {
        Gfx_SetBlend(4, 5);
        c = 0xBD6C6CC5;
        if (gChatInputMode != 0) {
            c = gChatSendTo == 0 ? 0xBD6CC56C : 0xBDB59F6C;
        }
        Chat_DrawKbdBox(c);
    }
    Hud_RestoreGsState();
}

void Hud_FadeStart(s32 mode, s16 x, s16 y);

void CombineMenu_StartFade(s32 a)
{
    Hud_FadeStart(a, 0x10E, gHudState->combineThree == 0 ? 0xC3 : 0xEA);
}

extern Fade gHudFades[6];
void Jingle_Play(void *obj, s32 id);
extern FadeDef *gHudFadeSetsA[4];
extern FadeDef gHudFadeDefsB[2];
extern FadeDef *gHudFadeSetsC[4];
extern s16 gFadeOffsets[][3][2];

/* Starts the six fades of gHudFades (the gHudState block's fade state 0x8C-0x8E) in one of
 * three patterns at (x, y): 0 a staggered row (with a jingle), 1 three pairs jittered by the
 * gFadeOffsets table picked by the frame tick, 2 four at once. */
void Hud_FadeStart(s32 mode, s16 x, s16 y)
{
    Fade *e;
    f32 fx;
    u32 i;
    s32 j;
    s32 k;

    gHudState->fadeActive = 1;
    gHudState->fadeFrame = 1;
    gHudState->fadeMode = mode;
    switch (mode) {
    case 0:
        e = gHudFades;
        fx = x + 50;
        for (i = 0; i < 6; i++) {
            e->active = 1; e->visible = 0; e->start = i * 5; e->bx = fx; e->by = y; e->def = gHudFadeSetsA[i % 4];
            e++;
        }
        Jingle_Play(gHudState->player, 0x1C);
        break;
    case 1:
        e = gHudFades;
        k = (u8)gFrameTick * 3 >> 8;
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 2; j++) {
                e->active = 1; e->visible = 0; e->start = i * 3;
                e->bx = x + gFadeOffsets[k][i][0]; e->by = y + gFadeOffsets[k][i][1]; e->def = &gHudFadeDefsB[j];
                e++;
            }
        }
        Sound_PlaySeFlat(0, 0xE, 0);
        break;
    case 2:
        e = gHudFades;
        fx = x + 50;
        for (i = 0; i < 4; i++) {
            e->active = 1; e->visible = 0; e->start = 0; e->bx = fx; e->by = y; e->def = gHudFadeSetsC[i];
            e++;
        }
        gHudFades[5].active = 0;
        gHudFades[4].active = 0;
        Sound_PlaySeFlat(0, 0xD, 0);
        break;
    }
}

s32 Fade_ApplyPos(Fade *e, PosKey *k, s16 t);
s32 Fade_ApplyAlpha(Fade *e, AlphaKey *k, s16 t);

void Fade_UpdateAll(void)
{
    s16 t;
    Fade *e;
    s32 i;
    s32 n;

    e = gHudFades;
    n = 0;
    i = 6;
    do {
        if (e->active != 0) {
            e->visible = 0;
            n++;
            if (e->start < gHudState->fadeFrame) {
                t = gHudState->fadeFrame - e->start;
                e->visible = 1;
                Fade_ApplyPos(e, e->def->pos, t);
                Fade_ApplyAlpha(e, e->def->alpha, t);
            }
        }
        e++;
    } while (--i != 0);
    if (n != 0) {
        (gHudState->fadeFrame)++;
    } else {
        gHudState->fadeActive = 0;
    }
}

void Gfx_SetBlendAlphaAdd(s32 a);

/* Draws the active fades of gHudFades: each a textured quad (two triangles) of its
 * definition, turned by the definition angle (0x10000 a turn) and scaled to its rectangle. */
void Hud_FadeDraw(void)
{
    Fade *e;
    s32 i;
    Tri t;
    f32 s;
    f32 c;

    if (gHudState->fadeActive == 0) {
        return;
    }
    Gfx_SetState63(1);
    Gs_SetState(0x60, 0);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    e = gHudFades;
    i = 6;
    do {
        if (e->active != 0 && e->visible != 0) {
            Gfx_SetBlendAlphaAdd(e->def->tex);
            t.color = e->def->color | (e->alpha << 24);
            t.u0 = t.u2 = e->def->u + 1;
            t.v0 = t.v1 = e->def->v + 1;
            t.u1 = e->def->u + e->def->x1 - 1;
            t.v2 = e->def->v + e->def->y1 - 1;
            Math_GetSinCos(&s, &c, 6.2831855f * e->def->angle / 65536.0f);
            t.x0 = e->bx + e->x0 * c - e->y0 * s;
            t.y0 = e->by + (s16)(e->x0 * s + e->y0 * c);
            t.x1 = e->bx + e->x1 * c - e->y0 * s;
            t.y1 = e->by + (s16)(e->x1 * s + e->y0 * c);
            t.x2 = e->bx + e->x0 * c - e->y1 * s;
            t.y2 = e->by + (s16)(e->x0 * s + e->y1 * c);
            Prim_DrawTexTri((struct PrimTexTri *)&t);
            t.u0 = e->def->u + e->def->x1 - 1;
            t.v0 = e->def->v + e->def->y1 - 1;
            t.x0 = e->bx + e->x1 * c - e->y1 * s;
            t.y0 = e->by + (s16)(e->x1 * s + e->y1 * c);
            Prim_DrawTexTri((struct PrimTexTri *)&t);
        }
        e++;
    } while (--i != 0);
}

/* Interpolates the position keys of e's definition at time t into e's rectangle (x0-y1); 1 before
 * the first key, -1 (and e cleared) after the last one. */
s32 Fade_ApplyPos(Fade *e, PosKey *k, s16 t)
{
    PosKey *next;
    f32 x;
    f32 y;
    s32 dt;
    s32 dn;

    t = (s16)t;
    if (t < k->time) {
        return 1;
    }
    for (next = k + 1; next->time > 0; k = next, next++) {
        if (t <= next->time) {
            dt = t - k->time;
            dn = next->time - k->time;
            x = next->x - k->x;
            x = k->x + x * dt / dn;
            y = next->y - k->y;
            y = k->y + y * dt / dn;
            e->x0 = -e->def->x0 * x;
            e->x1 = x * (e->def->x1 - e->def->x0);
            e->y0 = -e->def->y0 * y;
            e->y1 = y * (e->def->y1 - e->def->y0);
            return 0;
        }
    }
    e->active = 0;
    return -1;
}

s32 Fade_ApplyAlpha(Fade *e, AlphaKey *k, s16 t)
{
    AlphaKey *next;
    f32 v;

    if (t < k->time) {
        return 1;
    }
    for (next = k + 1; next->time > 0; k = next, next++) {
        if (t <= next->time) {
            v = k->value;
            v += (next->value - v) * (f32)(t - k->time) / (f32)(next->time - k->time);
            e->alpha = 255.0f * v;
            return 0;
        }
    }
    e->active = 0;
    return -1;
}

s32 QuestMenu_Begin(void)
{
    QuestMenu_CheckInfo();
    gHudState->pageCursor = 0;
    gHudState->questMenuStep = 0;
    if (Game_IsOnline() == 0) {
        gHudState->cursorMax = 3;
    } else {
        gHudState->cursorMax = 4;
    }
    gHelpShow = 0;
    Shake_Start(5, 0, 280.0f);
    return 0;
}

extern char *gQuestInfoFormats[];

void QuestMenu_DrawWideNumber(s32 v)
{
    char out[0x40];
    char buf[0x40];

    sprintf(buf, gQuestInfoFormats[0], v);
    Sjis_Widen(buf, out);
    Text_Draw(out);
}

extern u16 D_006ADB48;
extern char *D_004458F4;
u16 Dojo_GetParMinutes(u16 id);
void *QuestInfo_Get(s32 i);
void QuestMenu_DrawTimeLimit(s32 unused);
void QuestMenu_DrawWideNumber(s32 v);

void QuestMenu_DrawParTime(s16 x)
{
    char buf[0x40];
    char out[0x40];
    u16 id;

    if (gScene.online == 1) {
        id = D_006ADB48;
    } else {
        id = gQuest.stage;
    }
    Font_SetPos(x + 0x1F7, 0xA2);
    QuestMenu_DrawTimeLimit(1);
    Font_SetPos(x + 0x1F7, 0xB6);
    sprintf(buf, D_004458F4, Dojo_GetParMinutes(id));
    Sjis_Widen(buf, out);
    Text_Draw(out);
    Font_SetPos(x + 0x1D3, 0xCA);
    QuestMenu_DrawWideNumber(gQuestInfo->reward);
    Font_SetPos(x + 0x1D3, 0xDE);
    QuestMenu_DrawWideNumber(gQuestInfo->rewardSubA);
    Font_SetPos(x + 0x1D3, 0xF2);
    QuestMenu_DrawWideNumber(gQuestInfo->rewardSubB);
    Font_SetPos(x + 0x167, 0x12E);
    Text_Draw(QuestInfo_Get(4));
    Font_SetPos(x + 0x167, 0x17E);
    Text_Draw(QuestInfo_Get(5));
}

extern char *gStageNames[];
extern char *gDojoNoRecordText[];
extern char *gDojoBestTimeFmt;
s32 Save_TestListBit(u16 id);
s32 Dojo_GetBestScore(u16 id);
s32 Dojo_GetBestTime(u16 id);
char *strcpy(char *dst, char *src);

void QuestMenu_DrawBestRecord(s16 x)
{
    char buf[0x40];
    char out[0x40];
    u16 id;
    s32 t;
    s32 m;

    if (gScene.online == 1) {
        id = D_006ADB48;
    } else {
        id = gQuest.stage;
    }
    Font_SetPos(x + 0x1C1, 0x8E);
    Text_Draw(QuestInfo_Get(6));
    Font_SetPos(x + 0x1C1, 0xA2);
    Text_Draw(gStageNames[gQuestInfo->map]);
    Font_SetPos(x + 0x1E5, 0xB6);
    if (Save_TestListBit(id) == 0) {
        Text_Draw(gDojoNoRecordText[0]);
    } else {
        QuestMenu_DrawWideNumber(Dojo_GetBestScore(id));
    }
    Font_SetPos(x + 0x1E5, 0xCA);
    if (Save_TestListBit(id) == 0) {
        strcpy(buf, gDojoNoRecordText[0]);
    } else {
        t = Dojo_GetBestTime(id) / 30;
        m = t / 60;
        t -= m * 60;
        sprintf(buf, gDojoBestTimeFmt, m, t);
    }
    Sjis_Widen(buf, out);
    Text_Draw(out);
    Font_SetPos(x + 0x167, 0xF2);
    Text_Draw(QuestInfo_Get(7));
}

extern ListPanel gQuestMenuPanel;
extern char *gQuestStatusTitle[];
extern char *gNoQuestText;
extern char *gQuestMenuTitles[];
extern char *D_00445904;
extern char *D_00445908;
extern char *gQuestMenuItemLists[][17];
extern char *gQuestKindNames[];
extern char gMemberNames[][17];
extern char gPlayerIds[][8];
extern char gMyName[];
extern char gMyUserId[];
s32 func_D64530(s32 i);
void Text_SetColor(s16 color);
void QuestMenu_DrawParTime(s16 x);
void QuestMenu_DrawBestRecord(s16 x);

/* draws the quest info window: page 0 the quest, 1 and 2 the sub-pages, 3 the
 * players (two strings each, the local one from gMyName/gMyUserId) */
void QuestMenu_DrawInfo(void)
{
    char buf[0x40];
    char out[0x40];
    s16 *pos;
    s32 i;
    s32 y1;
    s32 y2;
    s16 k;
    s16 page;
    u8 c;
    u8 *p;

    pos = Shake_GetOffset(5);
    Gfx_SetBlend(4, 5);
    if (gHudState->hasQuestInfo == 0) {
        gQuestMenuPanel.items = NULL;
        ListPanel_Draw(&gQuestMenuPanel, pos, gQuestStatusTitle[0], -1);
        Font_SetPos(pos[0] + 0x167, 0xB6);
        Text_Draw(gNoQuestText);
        return;
    }
    c = gHudState->pageCursor;
    sprintf(buf, gQuestMenuTitles[0], c + 1, gHudState->cursorMax);
    page = c;
    switch (page) {
    case 0:
        k = 1;
        break;
    case 1:
        k = 4;
        break;
    case 2:
        k = 5;
        break;
    case 3:
        k = 6;
        break;
    }
    gQuestMenuPanel.items = gQuestMenuItemLists[k];
    ListPanel_Draw(&gQuestMenuPanel, pos, buf, -1);
    Menu_DrawFrame(&gQuestMenuPanel, pos);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(pos[0] + 0x167, 0x66);
    Text_Draw(QuestInfo_Get(0));
    switch (page) {
    case 0:
        Font_SetPos(pos[0] + 0x167, 0xB6);
        Text_Draw(QuestInfo_Get(1));
        Font_SetPos(pos[0] + 0x1C1, 0xCA);
        Text_DrawStars(gQuestInfo->stars);
        Font_SetPos(pos[0] + 0x167, 0x106);
        Text_Draw(QuestInfo_Get(2));
        Font_SetPos(pos[0] + 0x167, 0x142);
        Text_Draw(QuestInfo_Get(3));
        Font_SetPos(pos[0] + 0x167, 0x17E);
        Text_Draw(gQuestKindNames[gQuestInfo->terms]);
        break;
    case 1:
        QuestMenu_DrawParTime(pos[0]);
        break;
    case 2:
        QuestMenu_DrawBestRecord(pos[0]);
        break;
    case 3:
        i = 0;
        if (i < gScene.playerCount) {
            p = (u8 *)&gScene;
            y1 = 0xA2;
            y2 = 0x64;
            do {
            if (((SceneState *)p)->inQuest[0] == 1) {
                Font_SetPos(pos[0] + 0x167, y1);
                Text_SetColor((u8)func_D64530(i));
                sprintf(buf, "%dP", i + 1);
                Sjis_Widen(buf, out);
                Text_Draw(out);
                Font_SetColor(0x807F7F7F);
                Font_SetPos(pos[0] + 0x167, y2 + 0x52);
                if (i == gScene.localPlayer) {
                    sprintf(buf, D_00445904, gMyName);
                } else {
                    sprintf(buf, D_00445904, gMemberNames[i]);
                }
                Sjis_Widen(buf, out);
                Text_Draw(out);
                Font_SetPos(pos[0] + 0x167, y2 + 0x66);
                if (i == gScene.localPlayer) {
                    sprintf(buf, D_00445908, gMyUserId);
                } else {
                    sprintf(buf, D_00445908, gPlayerIds[i]);
                }
                Sjis_Widen(buf, out);
                Text_Draw(out);
                y1 += 0x3C;
                y2 += 0x3C;
            }
            p++;
            } while (++i < gScene.playerCount);
        }
        break;
    }
}

s32 QuestMenu_MapIndex_Unk(s32 a)
{
    if (a == 2) {
        return 4;
    }
    if (a == 3) {
        a = 5;
    }
    return a;
}

void CommentEdit_Open(void)
{
    gHelpShow = 0;
    gHudState->kbdOpen = 0;
}

extern u8 *gLoadBuf;
void Kbd_SetPos(s32 a, f32 b);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
void Kbd_Cancel(void);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
void Chat_FilterWords(char *s);
s32 LobbyText_Update(void);

/* The chat input line: confirm opens the keyboard (the line saved), then the keyboard's result
 * (1: send, -1: cancelled, the line restored). */
u16 Chat_InputLine(u16 buttons)
{
    if ((buttons & 0x20) && gHudState->kbdOpen == 0) {
        memcpy(gLoadBuf, gSave.comment, 0x62);
        gHudState->kbdOpen = 1;
        Sound_PlaySeFlat(0, 0x13, 0);
        Kbd_SetPos(0x4B, 80.0f);
        Chat_OpenKbd(2, 0xE, 0x60, gSave.comment);
    } else if (gHudState->kbdOpen != 0) {
        if (gPadState[0].pressed & 0x4000) {
            Kbd_Cancel();
            gHudState->kbdOpen = 0;
            memcpy(gSave.comment, gLoadBuf, 0x62);
        } else {
            switch (Kbd_Run((char *)gSave.comment, gPadState[0].held, gPadState[0].pressed)) {
            case 0:
                break;
            case 1:
                Chat_FilterWords((char *)gSave.comment);
            close:
                gHudState->kbdOpen = 0;
                Kbd_Cancel();
                if (Game_IsOnline() == 1) {
                    LobbyText_Update();
                }
                break;
            case -1:
                memcpy(gSave.comment, gLoadBuf, 0x62);
                goto close;
            }
        }
    }
    return buttons;
}

extern ListPanel gCommentEditMenu __attribute__((aligned(16)));
extern ListWin gCommentEditPanel;
extern char *gCommentEditTitle[];
void Text_DrawWrapped33(s16 x, s16 y, s16 lineh, char *s);

void CommentEdit_Draw(void)
{
    ListPanel m = gCommentEditMenu;

    if (gHudState->kbdOpen != 0) {
        m.cursorColor = 0xFF707070;
    }
    ListPanel_DrawDim(&gCommentEditPanel, gCommentEditTitle[0], -1);
    ListPanel_Draw(&m, 0, 0, 0);
    Text_DrawWrapped33(0x132, 0x52, 0x12, (char *)gSave.comment);
}

/* unk_33E620's .data (0x4306C0-0x4326B8) */
extern char *gPouchMenuPanel2_6[];
extern char *gDeliveryPanel_7[];
extern char *D_00445888[];
extern char *gStatusMenuColors_1[];
extern char *gInfoMenuLabelsOffline[];
extern char *gInfoMenuLabels[];
extern char *gCombine2Text[];
extern char *gCombineDoText[];
extern char *gChatMenuPanel_7[];
extern char *gCommentEditText[];
extern u32 D_0046CFB8[];
extern u32 D_0046D0B0[];
extern u32 D_0046D0B8[];
extern u32 D_0046D0C0[];
extern u32 gHeapBlocks[];
void ChatMenu_Draw(void);
s32 ChatMenu_Open(void);
void CombineMenu_DrawInfo(void);
s32 CombineMenu_Open(void);
void OptionMenu_Draw(void);
s32 OptionMenu_Open(void *m);
void PauseMenu_Draw(void);
s32 PauseMenu_Open(void);
s32 PhraseMenu_Open(void *m);
void PouchMenu_Draw(void);
s32 PouchMenu_Open(void *m);
u16 PouchMenu_Update(u16);
u32 gPlayerMapColors[4] = {
#include "sub_main/004306C0.inc"
};
char *gEmptyNameText = (char *)D_0046CFB8;
UVRect gStatusIconUVs[15] = {
#include "sub_main/004306E0.inc"
};
u8 gQuestMenuCursorMax[8] = {
#include "sub_main/00430758.inc"
};
u8 gQuestMenuKinds[2][4] = {
#include "sub_main/00430760.inc"
};
u8 gOptionMenuCounts[8] = {
#include "sub_main/00430768.inc"
};
u8 gOptionMenuRows_0[8] = {
#include "sub_main/00430770.inc"
};
u8 gOptionMenuRows_1[8] = {
#include "sub_main/00430778.inc"
};
u8 gOptionMenuRows_2[8] = {
#include "sub_main/00430780.inc"
};
u8 gOptionMenuRows_3[8] = {
#include "sub_main/00430788.inc"
};
u8 gOptionMenuRows_4[16] = {
#include "sub_main/00430790.inc"
};
u8 *gOptionMenuRows[8] = {
    gOptionMenuRows_0, gOptionMenuRows_1, gOptionMenuRows_2, gOptionMenuRows_3,
    gOptionMenuRows_4, NULL, NULL, NULL,
};
QuadDef gCompassQuads[2] = {
#include "sub_main/004307C0.inc"
};
u32 gClockColors[4] = {
#include "sub_main/00430810.inc"
};
u32 gHudSeasonColors[4] = {
#include "sub_main/00430820.inc"
};
u32 gCompassMarkerColors[4] = {
#include "sub_main/00430830.inc"
};
u32 gCompassColors[4] = {
#include "sub_main/00430840.inc"
};
f32 gCompassPoints[10][2] = {
#include "sub_main/00430850.inc"
};
u8 gCompassUVs[8][4] = {
#include "sub_main/004308A0.inc"
};
u8 D_004308C0[48] = {
#include "sub_main/004308C0.inc"
};
s16 gHudGaugeFlashSpeeds[2][5] = {
#include "sub_main/004308F0.inc"
};
u8 gSharpnessBarV[8] = {
#include "sub_main/00430908.inc"
};
u32 gSharpnessGlowMasks[68] = {
#include "sub_main/00430910.inc"
};
s16 gHornNoteYs[8] = {
#include "sub_main/00430A20.inc"
};
u32 gBlockGaugeSizes[4] = {
#include "sub_main/00430A30.inc"
};
u32 gBlockGaugeUVs[12] = {
#include "sub_main/00430A40.inc"
};
s32 gItemColors[12] = {
#include "sub_main/00430A70.inc"
};
u32 gItemFontColors[12] = {
#include "sub_main/00430AA0.inc"
};
s32 gItemDropPouchColors[8] = {
#include "sub_main/00430AD0.inc"
};
u32 gItemDropPanelColors[8] = {
#include "sub_main/00430AF0.inc"
};
u32 gPouchBoxSprites[28] = {
#include "sub_main/00430B10.inc"
};
u32 gPouchBarSprites[40] = {
#include "sub_main/00430B80.inc"
};
u32 gPouchSelectorSprites[20] = {
#include "sub_main/00430C20.inc"
};
u32 gPouchSelectorSpritesB[30] = {
#include "sub_main/00430C70.inc"
};
u8 gPouchSelectorCursor[8] = {
#include "sub_main/00430CE8.inc"
};
s32 D_00430CF0[8] = {
#include "sub_main/00430CF0.inc"
};
u8 gPouchSelectorSpritesC[48] = {
#include "sub_main/00430D10.inc"
};
u8 gHudButtonSprites[64] = {
#include "sub_main/00430D40.inc"
};
u32 gPouchBarFrame[48] = {
#include "sub_main/00430D80.inc"
};
u8 gPouchSelIconsA[8] = {
#include "sub_main/00430E40.inc"
};
Icon gPouchIconFromA[1] = {
#include "sub_main/00430E48.inc"
};
u8 gPouchSelIconsB[8] = {
#include "sub_main/00430E50.inc"
};
Icon gPouchIconFromB[1] = {
#include "sub_main/00430E58.inc"
};
Icon gPouchBarIcons[2] = {
#include "sub_main/00430E60.inc"
};
Icon gHudButtonIcons[2] = {
#include "sub_main/00430E70.inc"
};
s16 gPouchBarXA[8] = {
#include "sub_main/00430E80.inc"
};
s16 gPouchBarXB[8] = {
#include "sub_main/00430E90.inc"
};
f32 gMapScales[52] = {
#include "sub_main/00430EA0.inc"
};
f32 gMapDrawScales[26][2] = {
#include "sub_main/00430F70.inc"
};
u8 gMapIconKinds[80] = {
#include "sub_main/00431040.inc"
};
u8 gMapMonsterIconU[16] = {
#include "sub_main/00431090.inc"
};
u32 gMapMonsterIconColors[4] = {
#include "sub_main/004310A0.inc"
};
Vec3s gMapPoints[200] = {
#include "sub_main/004310B0.inc"
};
Vec3s gMapPoints80[2] = {
#include "sub_main/00431560.inc"
};
MonsterNoteDef gSizeRecordTbl[52] = {
#include "sub_main/00431570.inc"
};
s16 gHudStepPulse[4][2] = {
#include "sub_main/00431850.inc"
};
u8 gCamEventSprites[64] = {
#include "sub_main/00431860.inc"
};
s16 gCamEventSizes[24] = {
#include "sub_main/004318A0.inc"
};
s16 gCamEventTris[56] = {
#include "sub_main/004318D0.inc"
};
IconDef gBlinkIconDefs[3] = {
#include "sub_main/00431940.inc"
};
Sprite gBowgunAmmoSprite = {
#include "sub_main/00431970.inc"
};
u32 gHudFadeSetsA_0_9[12] = {
#include "sub_main/00431990.inc"
};
u32 gHudFadeSetsA_1_9[12] = {
#include "sub_main/004319C0.inc"
};
u32 gHudFadeSetsA_2_5[12] = {
#include "sub_main/004319F0.inc"
};
u32 gHudFadeSetsA_3_5[12] = {
#include "sub_main/00431A20.inc"
};
u32 gHudFadeSetsA_0_10[12] = {
#include "sub_main/00431A50.inc"
};
u32 gHudFadeSetsA_1_10[12] = {
#include "sub_main/00431A80.inc"
};
u32 gHudFadeSetsA_2_6[12] = {
#include "sub_main/00431AB0.inc"
};
u32 gHudFadeSetsA_3_6[12] = {
#include "sub_main/00431AE0.inc"
};
FadeDef gHudFadeSetsA_0 = {
#include "sub_main/00431B10.inc"
};
FadeDef gHudFadeSetsA_1 = {
#include "sub_main/00431B30.inc"
};
u32 gHudFadeSetsA_2[8] = {
#include "sub_main/00431B50.inc"
};
u32 gHudFadeSetsA_3[8] = {
#include "sub_main/00431B70.inc"
};
FadeDef *gHudFadeSetsA[4] = {
    &gHudFadeSetsA_0, &gHudFadeSetsA_1, (FadeDef *)gHudFadeSetsA_2, (FadeDef *)gHudFadeSetsA_3,
};
u32 gHudFadeDefsB_0[16] = {
#include "sub_main/00431BA0.inc"
};
u32 gHudFadeDefsB_1[12] = {
#include "sub_main/00431BE0.inc"
};
u32 gHudFadeDefsB_0_a[8] = {
#include "sub_main/00431C10.inc"
};
u32 gHudFadeDefsB_1_a[8] = {
#include "sub_main/00431C30.inc"
};
FadeDef gHudFadeDefsB[2] = {
#include "sub_main/00431C50.inc"
};
u32 gHudFadeSetsC_0_9[16] = {
#include "sub_main/00431C90.inc"
};
u32 gHudFadeSetsC_1_9[16] = {
#include "sub_main/00431CD0.inc"
};
u32 gHudFadeSetsC_2_5[12] = {
#include "sub_main/00431D10.inc"
};
u32 gHudFadeSetsC_3_5[12] = {
#include "sub_main/00431D40.inc"
};
u32 gHudFadeSetsC_0_10[8] = {
#include "sub_main/00431D70.inc"
};
u32 gHudFadeSetsC_1_10[8] = {
#include "sub_main/00431D90.inc"
};
u32 gHudFadeSetsC_2_6[8] = {
#include "sub_main/00431DB0.inc"
};
u32 gHudFadeSetsC_3_6[8] = {
#include "sub_main/00431DD0.inc"
};
FadeDef gHudFadeSetsC_0 = {
#include "sub_main/00431DF0.inc"
};
FadeDef gHudFadeSetsC_1 = {
#include "sub_main/00431E10.inc"
};
u32 gHudFadeSetsC_2[8] = {
#include "sub_main/00431E30.inc"
};
u32 gHudFadeSetsC_3[8] = {
#include "sub_main/00431E50.inc"
};
FadeDef *gHudFadeSetsC[4] = {
    &gHudFadeSetsC_0, &gHudFadeSetsC_1, (FadeDef *)gHudFadeSetsC_2, (FadeDef *)gHudFadeSetsC_3,
};
/* The message-event list, the game overlay's (sub_main links game's names
 * link-only: tools/configure.py, LOADED_WITH). */
s32 MsgEvent_OpenList(void);
s32 MsgEvent_ListInput(u16 pad);
void MsgEvent_Draw(void);
s32 (*gFieldMenuDecide[12])(void) = {
    (s32 (*)(void))PouchMenu_Open, CombineMenu_Open, (s32 (*)(void))StatusMenu_Open, (s32 (*)(void))QuestMenu_Open,
    (s32 (*)(void))InfoMenu_Open, (s32 (*)(void))PhraseMenu_Open, (s32 (*)(void))OptionMenu_Open, PauseMenu_Open,
    MsgEvent_OpenList, NULL, NULL, NULL,
};
s32 (*gFieldMenuDecideOnline[8])(void) = {
    (s32 (*)(void))PouchMenu_Open, CombineMenu_Open, (s32 (*)(void))StatusMenu_Open, (s32 (*)(void))QuestMenu_Open,
    (s32 (*)(void))InfoMenu_Open, ChatMenu_Open, (s32 (*)(void))OptionMenu_Open, NULL,
};
u16 (*gFieldMenuInput[12])(s32) = {
    (u16 (*)(s32))PouchMenu_Update, (u16 (*)(s32))CombineMenu_Input, (u16 (*)(s32))StatusMenu_Update, (u16 (*)(s32))QuestMenu_Update,
    (u16 (*)(s32))InfoMenu_Update, (u16 (*)(s32))PhraseMenu_Update, (u16 (*)(s32))OptionMenu_Update, (u16 (*)(s32))PauseMenu_Update,
    (u16 (*)(s32))MsgEvent_ListInput, NULL, NULL, NULL,
};
u16 (*gFieldMenuInputOnline[8])(s32) = {
    (u16 (*)(s32))PouchMenu_Update, (u16 (*)(s32))CombineMenu_Input, (u16 (*)(s32))StatusMenu_Update, (u16 (*)(s32))QuestMenu_Update,
    (u16 (*)(s32))InfoMenu_Update, (u16 (*)(s32))ChatMenu_Update, (u16 (*)(s32))OptionMenu_Update, NULL,
};
u8 gFieldMenuHelpLines[16] = {
#include "sub_main/00431F20.inc"
};
ListPanel gFieldMenuPanel = {
#include "sub_main/00431F30.inc"
};
void (*gFieldMenuDraw[12])(void) = {
    PouchMenu_Draw, CombineMenu_DrawInfo, StatusMenu_Draw, QuestMenu_DrawPage,
    InfoMenu_Draw, PhraseMenu_Draw, OptionMenu_Draw, PauseMenu_Draw,
    MsgEvent_Draw, NULL, NULL, NULL,
};
void (*gFieldMenuDrawOnline[8])(void) = {
    PouchMenu_Draw, CombineMenu_DrawInfo, StatusMenu_Draw, QuestMenu_DrawPage,
    InfoMenu_Draw, ChatMenu_Draw, OptionMenu_Draw, NULL,
};
Sprite gFieldMenuCockpitBox = {
#include "sub_main/00431FA0.inc"
};
Icon gFieldMenuCockpitIcon = {
#include "sub_main/00431FB8.inc"
};
ListPanel gStatusMenuPanel = {
#include "sub_main/00431FC0.inc"
};
u8 gInfoMenuCounts[16] = {
#include "sub_main/00431FE0.inc"
};
u8 gInfoMenuEntries[16] = {
#include "sub_main/00431FF0.inc"
};
ListPanel gInfoMenuPanel = {
#include "sub_main/00432000.inc"
};
char **gInfoMenuLabelSets[4] = {
    gInfoMenuLabelsOffline, gInfoMenuLabels, gInfoMenuLabels, gInfoMenuLabels,
};
ListPanel gItemListPanel = {
#include "sub_main/00432030.inc"
};
Frame gItemListFrame[2] = {
#include "sub_main/00432050.inc"
};
ListPanel gMonsterNotePanels[2] = {
#include "sub_main/00432080.inc"
};
Panel gMonsterNoteHelpBoxes[2] = {
#include "sub_main/004320C0.inc"
};
ListPanel gQuestMenuPanel = {
#include "sub_main/004320F0.inc"
};
ListPanel gChatMenuPanel = {
#include "sub_main/00432110.inc"
};
ListPanel gPhraseMenuPanel = {
#include "sub_main/00432130.inc"
};
ListPanel gCombineKindPanel = {
#include "sub_main/00432150.inc"
};
ListPanel gCombineMaterialPanel = {
#include "sub_main/00432170.inc"
};
Panel gCombineResultWindow = {
#include "sub_main/00432190.inc"
};
ListPanel gCombineDoPanel = {
#include "sub_main/004321B0.inc"
};
ListPanel gOptionMenuPanel = {
#include "sub_main/004321D0.inc"
};
u32 gPlayerNameColors[8] = {
#include "sub_main/004321F0.inc"
};
char *gSideNames[8] = {
    (char *)D_0046D0B0, (char *)D_0046D0B8, (char *)D_0046D0C0, (char *)D_0046D0B8,
    (char *)D_0046D0C0, NULL, NULL, NULL,
};
ListPanel gPouchMenuPanel = {
#include "sub_main/00432230.inc"
};
ListPanel gPouchMenuPanel2 = {
#include "sub_main/00432250.inc"
};
ListPanel D_00432270[2] = {
#include "sub_main/00432270.inc"
};
ListPanel gDeliveryPanel = {
#include "sub_main/004322B0.inc"
};
ListPanel gPlayerNameListPanels[2] = {
#include "sub_main/004322D0.inc"
};
Panel gPlayerNameCountBox = {
#include "sub_main/00432310.inc"
};
Panel gDeliveryCountBox = {
#include "sub_main/00432330.inc"
};
ListPanel gItemListBox = {
#include "sub_main/00432350.inc"
};
Panel gItemListTextBox = {
#include "sub_main/00432370.inc"
};
f32 gMapAreaOffsetX[28] = {
#include "sub_main/00432390.inc"
};
s16 gMapAreaOffsetY[40] = {
#include "sub_main/00432400.inc"
};
Panel gItemWindowNameBox[2] = {
#include "sub_main/00432450.inc"
};
ListPanel gItemWindowDropPanel = {
#include "sub_main/00432480.inc"
};
ListPanel gItemWindowPanel = {
#include "sub_main/004324A0.inc"
};
Panel gItemWindowTextBox = {
#include "sub_main/004324C0.inc"
};
GaugeFrame gGaugeFrames[2] = {
#include "sub_main/004324E0.inc"
};
Frame gSupplyBoxFrame = {
#include "sub_main/00432510.inc"
};
ListWin gTentMenuPanel = {
#include "sub_main/00432530.inc"
};
Panel gSupplyBoxTextBox = {
#include "sub_main/00432550.inc"
};
ListPanel gDeliveryPanel2 = {
#include "sub_main/00432570.inc"
};
Panel gDeliveryTextBox = {
#include "sub_main/00432590.inc"
};
ListWin gEquipStatusPanel = {
#include "sub_main/004325B0.inc"
};
Panel gPauseTextBox = {
#include "sub_main/004325D0.inc"
};
s16 gFadeOffsets[4][3][2] = {
#include "sub_main/004325F0.inc"
};
ListWin gCommentEditPanel = {
#include "sub_main/00432620.inc"
};
ListPanel gCommentEditMenu __attribute__((aligned(16))) = {
#include "sub_main/00432640.inc"
};
u32 gWeaponIconUVs[22] = {
#include "sub_main/00432660.inc"
};
