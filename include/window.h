#ifndef WINDOW_H
#define WINDOW_H

#include "common.h"
#include "player.h"
#include "grid.h"
#include "chat_log.h"

/*
 * sub_main's menu windows (src/sub_main/menu_windows.c): a position, the
 * character size in pixels (charW, charH: Font_SetSpacing's), the text size in
 * characters (lineChars wide, lines high; a line is charH + 2 pixels), a frame
 * style in flags & 7 and colours. Window_DrawTextAlpha and the list windows read a text colour at
 * +8; Window_DrawText and the list panels read padX, padY there.
 */
/* The flags word of Window, Panel, ListWin and ListPanel. */
#define WINDOW_STYLE_MASK   0x0007  /* the frame style (Frame.style); above WINDOW_STYLE_MAX draws that */
#define WINDOW_STYLE_MAX    5
#define WINDOW_RULED        0x2000  /* ListPanel: rules between the entries, lines 2 pixels taller; ListWin: taller lines */
#define WINDOW_INSET        0x4000  /* the frame drawn inside the box (FRAME_INSET) */
#define WINDOW_FILL         0x8000  /* the background filled in color2 (FRAME_FILL), not tiled */

/* Frame.style: the style in the low bits, then two flags. */
#define FRAME_STYLE_MASK    0x7
#define FRAME_STYLE_CORNERS 4           /* styles 4 and 5: gFrameStyles2's corner pieces; 0-3 tile gFrameStyles' edges */
#define FRAME_INSET         0x40000000  /* the box shrunk by the corners' size, so the frame lies inside it */
#define FRAME_FILL          0x80000000  /* the background a rectangle in color2; else tiles of the window texture */

/* Menu_DrawArrow's and Chat_DrawArrow's flags. */
#define ARROW_MIRROR        0x1     /* pointing the other way (the texels swapped) */
#define ARROW_TEXRECT       0x2     /* drawn by TexRect_Draw, mode 2, not as a sprite */

/* Gfx_DrawMarker's flags: the window texture's marker icon (20 by 18 texels). */
#define MARKER_COLOR_MASK   0x3     /* the colour, gMarkerColors' index */
#define MARKER_CUE          0x4     /* a blinking cue to its right */
#define MARKER_ALERT        0x8     /* a red cell drawn over it */

/* A menu window (gChatFrameBox, gPersonalStatusPanels[], gEquipWindowBox...): position, size in cells, colour and flags. */
typedef struct Window {
    s16 x;                      /* 0x0 */
    s16 y;                      /* 0x2 */
    u8 charW;                   /* 0x4: a character width in pixels (Font_SetSpacing advance) */
    u8 charH;                   /* 0x5: a character height in pixels; a line is charH + 2 */
    u8 lineChars;               /* 0x6: characters per line (the text is charW * lineChars wide) */
    u8 lines;                   /* 0x7: text lines */
    s16 color;                  /* 0x8: text colour */
    u16 flags;                  /* 0xA */
    u32 frameColor;             /* 0xC: the frame's second colour (flags 0x8000; Window_DrawTextAlpha), Panel.color for Window_DrawText */
} Window;

typedef char Window_size_check[(sizeof(Window) == 0x10) ? 1 : -1];

/* A window frame as WindowFrame_Draw draws it. */
typedef struct Frame {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    s16 w;                      /* 0x04 */
    s16 h;                      /* 0x06 */
    u32 style;                  /* 0x08: 0-5; 0x80000000: color2 given */
    u32 color;                  /* 0x0C */
    u32 color2;                 /* 0x10 */
} Frame;

typedef char Frame_size_check[(sizeof(Frame) == 0x14) ? 1 : -1];

/* A menu window as Window_DrawText draws it (gHelpWindowBox, gChatFrameBox, gPersonalStatusBoxes[]...). */
typedef struct Panel {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    u8 charW;                   /* 0x04: a character width in pixels (Font_SetSpacing advance) */
    u8 charH;                   /* 0x05: a character height in pixels; a line is charH + 2 */
    u8 lineChars;               /* 0x06: characters per line (the text is charW * lineChars wide) */
    u8 lines;                   /* 0x07 */
    u8 padX;                    /* 0x08 */
    u8 padY;                    /* 0x09 */
    u16 flags;                  /* 0x0A: the frame style (WINDOW_STYLE_MASK, at most WINDOW_STYLE_MAX), WINDOW_INSET, WINDOW_FILL */
    u32 color;                  /* 0x0C: frame colours */
    u32 color2;                 /* 0x10 */
    s16 textColor;              /* 0x14: a text colour; with rgbText an RGBA for the whole word */
    u8 unk16;
    u8 rgbText;                 /* 0x17 */
} Panel;

typedef char Panel_size_check[(sizeof(Panel) == 0x18) ? 1 : -1];

/* A list window (ListPanel_DrawAlpha): a Window with its entries and cursor. 28 bytes: lobby
 * NetMode_DrawPart and plsel Plsel_DrawPlayerMenuList copy its 28 bytes into a local they draw, and
 * every object is followed by the next at +0x20 even when that one is 8-aligned
 * (select gGalleryListPanel); nothing reads +0x18. */
typedef struct ListWin {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    u8 charW;                   /* 0x04: a character width in pixels (Font_SetSpacing advance) */
    u8 charH;                   /* 0x05: a character height in pixels; a line is charH + 2 */
    u8 lineChars;               /* 0x06: characters per line (the text is charW * lineChars wide) */
    u8 lines;                   /* 0x07 */
    s16 color;                  /* 0x08: text colour */
    u16 flags;                  /* 0x0A: the frame style (WINDOW_STYLE_MASK), WINDOW_RULED taller lines, WINDOW_FILL color2 given */
    char **items;               /* 0x0C: NULL-terminated */
    s32 cursorColor;            /* 0x10: the cursor bar colour (0xRRGGBB; ListPanel_DrawCursorBar pulses its alpha) */
    u32 color2;                 /* 0x14 */
    s32 unk18;                  /* 0x18: 0 in every object */
} ListWin;

typedef char ListWin_size_check[(sizeof(ListWin) == 0x1C) ? 1 : -1];

/* A list panel (ListPanel_Draw): a Panel with a cursor kind and its entries. */
typedef struct ListPanel {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    u8 charW;                   /* 0x04: a character width in pixels (Font_SetSpacing advance) */
    u8 charH;                   /* 0x05: a character height in pixels; a line is charH + 2 */
    u8 lineChars;               /* 0x06: characters per line (the text is charW * lineChars wide) */
    u8 lines;                   /* 0x07 */
    u8 padX;                    /* 0x08 */
    u8 padY;                    /* 0x09 */
    u16 flags;                  /* 0x0A: the frame style (WINDOW_STYLE_MASK), WINDOW_RULED lines between the entries, WINDOW_INSET, WINDOW_FILL */
    u32 color;                  /* 0x0C */
    u32 color2;                 /* 0x10 */
    s16 textColor;              /* 0x14 */
    u8 unk16;
    u8 rgbText;                 /* 0x17 */
    s32 cursorColor;            /* 0x18: the cursor bar colour (0xRRGGBB; ListPanel_DrawCursorBar pulses its alpha) */
    char **items;               /* 0x1C: NULL-terminated */
} ListPanel;

typedef char ListPanel_size_check[(sizeof(ListPanel) == 0x20) ? 1 : -1];

void WindowFrame_Draw(Frame *frame, s16 *off);
void Menu_DrawFrame(void *win, s16 *pos);
void Window_DrawSides(Window *win, s16 *pos, u32 color);
void Window_DrawText(void *window, s16 *pos, u8 *text);
void ListPanel_DrawDim(struct ListWin *win, char *title, s32 sel);
void Window_DrawTextB2(Window *w, u8 *text);
void Sprite_DrawStretch3(Sprite *src, s16 capW, s16 capU);
void Sprite_DrawStretch9(Sprite *src, s16 capW, s16 capH, s16 capU, s16 capV);
void Sprite_DrawList(Sprite *sprites, s32 count);
void Menu_DrawArrow(s16 x, s16 y, s16 width, s16 height, u32 color, u8 flags);
void Chat_DrawArrow(s16 x, s16 y, s16 width, s16 height, u32 color, u8 flags);
void Hud_DrawTargetMarker(f32 *pos);
s32 CombineList_Setup(Grid *grid, s16 *out, s32 filter);
void CombineList_MoveCursor(Grid *grid, s16 *out, u16 buttons);
s32 MonsterNote_AnyKnown(void);
s32 MonsterNote_SetKnown(s32 kind);
s32 MonsterNote_CountKnown(void);
u8 MonsterNote_Number(u8 limit);
s32 EntryPicture_GetUVf(f32 *uv0, f32 *uv1, u16 id);
s32 EntryPicture_GetUV(s32 *uv0, s32 *uv1, u16 id);
s32 WantedPicture_GetUV(s32 *uv0, s32 *uv1, u16 id);
void HelpWindow_DrawCurrent(void);
void HelpWindow_DrawLine(u8 page, u16 line);
void HelpWindow_Draw(u8 page, u16 line, s8 column);
void Deco_DrawDetails(u16 item, s16 x, s16 y, s8 view, u8 pal);
s32 Notice_OpenTalk(char *text, u32 width, s32 mode, s32 sel);
s32 Notice_OpenText(char *text, u32 width, s32 mode, s32 sel, s32 title, char **list);
void Chat_OpenKbd(s32 flags, s32 layout, u16 maxChars, void *text);
s32 Chat_IsAvailable(void);
void Chat_PickSendTarget(void);
void Chat_OpenInput(u8 active);
void Chat_SendInput(u16 unused);
void Chat_PlayAct(s32 text);
s32 Chat_StripActTags(s8 *src);
s8 Chat_ParseActTag(s8 *tag);
s8 *Str_Find(s8 *str, s8 *sub);
void Chat_LogAdd(u8 who, s8 *msg, ChatMsg *header, u8 wide);
void Chat_AddReceived(u8 who, s8 *msg);
void Chat_ClearBeep(void);
void Chat_Beep(void);
void Chat_LogAddLocal(u8 who, u8 kind, char *msg);
void Chat_ResetScroll(void);
s32 Chat_OpenWindow(void);
u16 Chat_LogScroll(u16 buttons);
s32 Chat_LogRowsBefore(u8 skip);
void Chat_DrawHudLog(void);
void Chat_DrawLogLines(u8 scroll, s16 dy, u8 atEnd, u8 size);
void Chat_DrawLog(void);
void Chat_UpdateWindowsOnline(u16 buttons);
void Chat_UpdateWindow(u16 unused0);
void Chat_DrawHud(void);
void Chat_DrawWindow(u8 kind);
void Chat_HideWindows(void);
void Chat_ShowWindows(void);
void Players_UpdatePresent(void);
void Notice_Draw(void);
void PersonalStatus_DrawPage(struct Player *pl, u8 page);
s16 StatusPage_DrawHeader(struct Player *pl, s16 x, s16 y);
s16 StatusPage_DrawSymbolColor(s16 x, s16 y);
s16 StatusPage_DrawRank(s16 x, s16 y);
s16 StatusPage_DrawArenaRecord(s16 x, s16 y);
s16 StatusPage_DrawOnline(s16 x, s16 y);
void StatusPage_DrawStats(Player *pl, Player *other, s32 x, s32 y);
u32 Player_ShownAttack(Player *pl);
void PersonalStatus_DrawEquipment(Player *pl);
void StatusPage_DrawEquipment(EquipItem *weapon, EquipItem *equip);
void StatusPage_DrawWeaponClass(Player *pl);
s32 Player_SkillApplies(Player *pl, u8 slot);
u8 Player_ArmorHasTorsoUp(Player *pl);
u8 SkillList_PageCount(void);
void SkillList_Build(Player *pl, Player *other);
void SkillList_Insert(Player *pl, u8 tree, Player *other);
void Player_RecomputeStats(Player *pl);
u8 Equip_LevelMark(EquipItem *item, u8 flags, EquipItem *other);
void EquipWindow_DrawLocal(EquipItem *item, s16 x, s16 y, u8 flags, EquipItem *other);
void StatusPage_DrawBox(s32 x, s16 y, s32 alpha);
void Player_SetWeapon(Player *pl, EquipItem *item);
Player *Player_PreviewWith(Player *pl, EquipItem *item);
void EquipWindow_DrawDetails(Player *pl, EquipItem *item, s16 x, s16 y, u8 flags, EquipItem *other);
void EquipWindow_DrawWeaponValue(Player *pl, EquipItem *item, s16 x, s16 y, EquipItem *other);
void EquipWindow_DrawSlots(EquipItem *item, s16 x, s16 y, u8 plain);
void EquipWindow_DrawDecorations(EquipItem *item, s16 x, s16 y, u8 flag);
void EquipWindow_DrawWeaponKind(EquipItem *item, s16 x, s16 y);
void EquipWindow_DrawMarks(EquipItem *item, s16 x, s16 y);
void EquipWindow_DrawRow(Player *pl, EquipItem *item, s16 x, s16 y);
void EquipWindow_DrawItemList(GunData *gun, s16 x, s16 y);
extern s32 gWeaponClassMultipliers[12];
extern u32 gHornNoteColors[8];
extern u8 gChatTextSizes[8]; /* the chat's text sizes, by gSysSave.options.chatSize (sub_main) */
extern u32 gEquipColors[10];
extern u8 gRarityColorSlots[14];
extern u8 gRarityColors[8];

#endif
