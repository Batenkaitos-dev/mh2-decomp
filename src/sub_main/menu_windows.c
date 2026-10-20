/*
 * sub_main's menu windows, the notice window, chat and the status pages.
 *
 * - The menu text's tables come first (.rodata, generated from the game by
 *   tools/content_gen.py: only their names are in this file).
 * - The windows (window.h): WindowFrame_Draw draws a Frame in one of six
 *   styles from the window texture; ListPanel_Draw, Window_DrawText and the
 *   alpha variants draw a Panel, ListWin or Window with its lines; the menu
 *   arrows, stretched sprites and the target marker follow.
 * - Small helpers for the HUD's menus: the combine list's
 *   grid, the monster notes' known bits (SaveData.noteKnown), the pictures'
 *   texture cells and the help window.
 * - The notice window (chat_log.h's NOTICE_*): Notice_OpenText types a text
 *   out with time and sets up its prompt or list; Notice_Draw draws it.
 * - Chat: the software keyboard's input line (Chat_OpenInput,
 *   Chat_SendInput, sent by gChatSendTo, CHAT_SEND_*), the act tags that
 *   play gestures, the log ring gChat.lines and its windows; the lobby's
 *   own log is gChatLog.
 * - The status pages (PersonalStatus_DrawPage): the profile, the stats
 *   compared with another set of equipment, the equipment and the skill
 *   list (gSkillRecords).
 * - The equipment windows (EquipWindow_*): a piece of equipment's details
 *   page by page, its ammo, decorations and marks, previewed on the player
 *   (Player_PreviewWith).
 * - The file's data tables close it.
 */
#include "common.h"
#include "grid.h"
#include "window.h"
#include "chat_log.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "sprite.h"
#include "pad.h"
#include "syssave.h"
#include "save.h"
#include "item.h"
#include "sub_main/menu_windows.h"   /* the texts this file uses (tools/content_gen.py) */
#include "render_state.h"
#include "hud.h"
#include "prim.h"
#include "sound.h"
#include "area.h"
#include "texture.h"
#include "lobby_labels.h"

/* A line of the status page's skill list (gSkillRecords, 0x36 of them sorted by key, largest
 * first; gSkillCompareRecords holds the compared player's). */
typedef struct SkillRecord {
    u16 key;                    /* 0x0 */
    u8 used;                    /* 0x2: nonzero in use; 1 highlighted, 2 a heading line */
    u8 skill;                   /* 0x3 */
    s8 pts[6];                  /* 0x4: points from the weapon, armour 2-5, armour 0 (a heading: whether it has the skill) */
    s8 total;                   /* 0xA */
    s8 mark[6];                 /* 0xB: compared with the other player's record: 4 more points, 2 fewer */
    s8 totalMark;               /* 0x11: the same for the total */
} SkillRecord;

extern SkillRecord gSkillRecords[];




struct PrimLine;
s32 Prim_FillRect(struct PrimLine *r);


/* prototypes */
s32 Chat_CountLogRows(void);
void Hud_SetCancel(void);
struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, char *title, s32 sel, s32 alpha); /* unit's own declaration: the K&R definition's narrow parameters arrive promoted, as this prototype says (a narrow prototype is incompatible with it) */
void Window_DrawTextAlpha(Window *w, u8 *text, u8 alpha);
s32 Notice_OpenText(char *, u32, s32, s32, s32, char **);
s32 Chat_StripActTags(s8 *);
void Chat_DrawLogLines(u8, s16, u8, u8);
void Chat_DrawWindow(u8);
void Chat_DrawBox(s32 type, u8 idx, s32 text);
void Gfx_SetTexFilter(s32 a);
s32 Game_IsOnline(void);
void Player_RequestGesture(u8 a);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void ChatLog_ResetScroll(void);
extern u8 gEntryPictureIndex[];
extern u8 gWantedPictureCells[];
extern s32 gTalkEventTitle;
extern u8 gChatBeeped;
extern u8 gChatHidden;
extern u8 gChatHudColor;
extern u8 gHelpShow;
extern u16 gChatWinTimer;
extern u8 gChatWinOpen;
extern u8 gChatLogScroll;
extern u8 gChatLogAtEnd;
extern u8 gChatWindowSel;
extern u8 gChatSendTo;
extern u8 gChatWinState;
s32 Sprite_Draw(Sprite *s);
struct TexRect;
void TexRect_Draw(struct TexRect *r, s8 mode);
void Window_DrawText(void *p, s16 *pos, u8 *text);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_Draw(char *s);
void Text_PrintfStyled(s32 a, const char *fmt, ...);
void Text_PrintfColored(const char *fmt, ...);
void Chat_FilterWords(char *s);
void Chat_LogAdd(u8, s8 *, ChatMsg *, u8);
void Sprite_DrawStretch3(Sprite *src, s16 w, s16 du);
void Menu_GridCursor(Grid *g, u16 buttons);
s32 Recipe_IsListedFor(s16 n, s32 a);
extern Panel gEquipWindowBox;
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 tex);
extern u8 gChatLogHead;
extern u8 gChatLogCount;
extern u8 gChatWinHold;
void Text_SetColor(s16 color);
void Font_SetSpacing(u8 a, u8 b);
void Gfx_SetBlend(s32 a, s32 b);
void Text_Printf(const char *fmt, ...);
s32 sprintf(char *buf, const char *fmt, ...);
s32 Player_HasSkill(Player *p, u8 skill);
Player *Player_PreviewWith(Player *p, EquipItem *e);
extern u8 gChatTextSizes[];
extern u8 gChatLines;
extern u16 gFrameTick;
s32 Equip_CountDecos(EquipItem *e);
extern char gWindowFmtString[];
void WindowFrame_Draw(Frame *f, s16 *off);
void StatusPage_DrawWeaponClass(Player *pl);
/* end of prototypes */

/* A frame style of gFrameStyles (styles 0-3): tiled edges and corners from the icon texture (TEX_SLOT_ICONS). */
typedef struct FrameStyle {
    u8 cw, ch;                  /* 0x0: corner size */
    u16 cu, cv;                 /* 0x2: corner texel */
    u8 th, tw;                  /* 0x6: top/bottom edge tile height, width */
    u16 tu, tv;                 /* 0x8 */
    u8 sw, sh;                  /* 0xC: side edge tile width, height */
    u16 su, sv;                 /* 0xE */
} FrameStyle;

/* A frame style of gFrameStyles2 (styles 4-5): four corner pieces and lines. */
typedef struct FrameStyle2 {
    s16 ox, oy;                 /* 0x0: outset */
    u16 u, v;                   /* 0x4: corner texel */
    s16 w, h;                   /* 0x8: corner size */
    s16 ix, iy;                 /* 0xC: inset of the corner */
    s16 shadow;                 /* 0x10: shadow offset, 0 none */
} FrameStyle2;

extern FrameStyle gFrameStyles[];
extern FrameStyle2 gFrameStyles2[];
extern s16 D_0043ABA8[];
extern s16 D_0043ABAA[];
extern s16 D_0043ABAC[];
extern s16 D_0043ABAE[];

struct PrimLine;
s32 Prim_DrawLine(struct PrimLine *r);

/* The menu windows' texts and constants, read-only. */
#pragma rodata on
#pragma explicit_zero_data on
char gHelpTexts_1_8[8] = {
#include "sub_main/0046D110.inc"
};
#pragma explicit_zero_data reset
char D_0046D118[0x8] = {
#include "sub_main/0046D118.inc"
};
char D_0046D120[0x50] = {
#include "sub_main/0046D120.inc"
};
char D_0046D170[0x50] = {
#include "sub_main/0046D170.inc"
};
char D_0046D1C0[0x60] = {
#include "sub_main/0046D1C0.inc"
};
char D_0046D220[0x60] = {
#include "sub_main/0046D220.inc"
};
char D_0046D280[0x60] = {
#include "sub_main/0046D280.inc"
};
char D_0046D2E0[0x50] = {
#include "sub_main/0046D2E0.inc"
};
char D_0046D330[0x20] = {
#include "sub_main/0046D330.inc"
};
char D_0046D350[0x20] = {
#include "sub_main/0046D350.inc"
};
char D_0046D370[0x30] = {
#include "sub_main/0046D370.inc"
};
char D_0046D3A0[0x30] = {
#include "sub_main/0046D3A0.inc"
};
char D_0046D3D0[0x30] = {
#include "sub_main/0046D3D0.inc"
};
char D_0046D400[0x20] = {
#include "sub_main/0046D400.inc"
};
char D_0046D420[0x40] = {
#include "sub_main/0046D420.inc"
};
char D_0046D460[0x60] = {
#include "sub_main/0046D460.inc"
};
char D_0046D4C0[0x30] = {
#include "sub_main/0046D4C0.inc"
};
char D_0046D4F0[0x30] = {
#include "sub_main/0046D4F0.inc"
};
char D_0046D520[0x40] = {
#include "sub_main/0046D520.inc"
};
char D_0046D560[0x40] = {
#include "sub_main/0046D560.inc"
};
char D_0046D5A0[0x30] = {
#include "sub_main/0046D5A0.inc"
};
char D_0046D5D0[0x40] = {
#include "sub_main/0046D5D0.inc"
};
char D_0046D610[0x20] = {
#include "sub_main/0046D610.inc"
};
char D_0046D630[0x20] = {
#include "sub_main/0046D630.inc"
};
char D_0046D650[0x60] = {
#include "sub_main/0046D650.inc"
};
char D_0046D6B0[0x50] = {
#include "sub_main/0046D6B0.inc"
};
char D_0046D700[0x50] = {
#include "sub_main/0046D700.inc"
};
char D_0046D750[0x40] = {
#include "sub_main/0046D750.inc"
};
char D_0046D790[0x30] = {
#include "sub_main/0046D790.inc"
};
char D_0046D7C0[0x40] = {
#include "sub_main/0046D7C0.inc"
};
char D_0046D800[0x50] = {
#include "sub_main/0046D800.inc"
};
char D_0046D850[0x48] = {
#include "sub_main/0046D850.inc"
};
char D_0046D898[0xC] = {
#include "sub_main/0046D898.inc"
};
char D_0046D8B0[0x40] = {
#include "sub_main/0046D8B0.inc"
};
char D_0046D8F0[0x30] = {
#include "sub_main/0046D8F0.inc"
};
char D_0046D920[0x50] = {
#include "sub_main/0046D920.inc"
};
char D_0046D970[0x60] = {
#include "sub_main/0046D970.inc"
};
char D_0046D9D0[0x50] = {
#include "sub_main/0046D9D0.inc"
};
char D_0046DA20[0x50] = {
#include "sub_main/0046DA20.inc"
};
char D_0046DA70[0x50] = {
#include "sub_main/0046DA70.inc"
};
char D_0046DAC0[0x40] = {
#include "sub_main/0046DAC0.inc"
};
char D_0046DB00[0x60] = {
#include "sub_main/0046DB00.inc"
};
char D_0046DB60[0x50] = {
#include "sub_main/0046DB60.inc"
};
char D_0046DBB0[0x50] = {
#include "sub_main/0046DBB0.inc"
};
char D_0046DC00[0x40] = {
#include "sub_main/0046DC00.inc"
};
char D_0046DC40[0x30] = {
#include "sub_main/0046DC40.inc"
};
char D_0046DC70[0x30] = {
#include "sub_main/0046DC70.inc"
};
char D_0046DCA0[0x30] = {
#include "sub_main/0046DCA0.inc"
};
char D_0046DCD0[0x30] = {
#include "sub_main/0046DCD0.inc"
};
char D_0046DD00[0x30] = {
#include "sub_main/0046DD00.inc"
};
char D_0046DD30[0x50] = {
#include "sub_main/0046DD30.inc"
};
char D_0046DD80[0x40] = {
#include "sub_main/0046DD80.inc"
};
char D_0046DDC0[0x40] = {
#include "sub_main/0046DDC0.inc"
};
char D_0046DE00[0x50] = {
#include "sub_main/0046DE00.inc"
};
char D_0046DE50[0x40] = {
#include "sub_main/0046DE50.inc"
};
char D_0046DE90[0x50] = {
#include "sub_main/0046DE90.inc"
};
char D_0046DEE0[0x20] = {
#include "sub_main/0046DEE0.inc"
};
char D_0046DF00[0x40] = {
#include "sub_main/0046DF00.inc"
};
char D_0046DF40[0x30] = {
#include "sub_main/0046DF40.inc"
};
char D_0046DF70[0x40] = {
#include "sub_main/0046DF70.inc"
};
char D_0046DFB0[0x50] = {
#include "sub_main/0046DFB0.inc"
};
char D_0046E000[0x50] = {
#include "sub_main/0046E000.inc"
};
char D_0046E050[0x50] = {
#include "sub_main/0046E050.inc"
};
char D_0046E0A0[0x40] = {
#include "sub_main/0046E0A0.inc"
};
char D_0046E0E0[0x30] = {
#include "sub_main/0046E0E0.inc"
};
char D_0046E110[0x50] = {
#include "sub_main/0046E110.inc"
};
char D_0046E160[0x50] = {
#include "sub_main/0046E160.inc"
};
char D_0046E1B0[0x50] = {
#include "sub_main/0046E1B0.inc"
};
char D_0046E200[0x50] = {
#include "sub_main/0046E200.inc"
};
char D_0046E250[0x50] = {
#include "sub_main/0046E250.inc"
};
char D_0046E2A0[0x20] = {
#include "sub_main/0046E2A0.inc"
};
char D_0046E2C0[0x50] = {
#include "sub_main/0046E2C0.inc"
};
char D_0046E310[0x60] = {
#include "sub_main/0046E310.inc"
};
char D_0046E370[0x30] = {
#include "sub_main/0046E370.inc"
};
char D_0046E3A0[0x20] = {
#include "sub_main/0046E3A0.inc"
};
char D_0046E3C0[0x40] = {
#include "sub_main/0046E3C0.inc"
};
char D_0046E400[0x30] = {
#include "sub_main/0046E400.inc"
};
char D_0046E430[0x50] = {
#include "sub_main/0046E430.inc"
};
char D_0046E480[0x40] = {
#include "sub_main/0046E480.inc"
};
char D_0046E4C0[0x50] = {
#include "sub_main/0046E4C0.inc"
};
char D_0046E510[0x40] = {
#include "sub_main/0046E510.inc"
};
char D_0046E550[0x20] = {
#include "sub_main/0046E550.inc"
};
char D_0046E570[0x30] = {
#include "sub_main/0046E570.inc"
};
char D_0046E5A0[0x40] = {
#include "sub_main/0046E5A0.inc"
};
char D_0046E5E0[0x50] = {
#include "sub_main/0046E5E0.inc"
};
char D_0046E630[0x40] = {
#include "sub_main/0046E630.inc"
};
char D_0046E670[0x50] = {
#include "sub_main/0046E670.inc"
};
char D_0046E6C0[0x30] = {
#include "sub_main/0046E6C0.inc"
};
char D_0046E6F0[0x20] = {
#include "sub_main/0046E6F0.inc"
};
char D_0046E710[0x20] = {
#include "sub_main/0046E710.inc"
};
char D_0046E730[0x20] = {
#include "sub_main/0046E730.inc"
};
char D_0046E750[0x50] = {
#include "sub_main/0046E750.inc"
};
char D_0046E7A0[0x30] = {
#include "sub_main/0046E7A0.inc"
};
char D_0046E7D0[0x60] = {
#include "sub_main/0046E7D0.inc"
};
char D_0046E830[0x50] = {
#include "sub_main/0046E830.inc"
};
char D_0046E880[0x40] = {
#include "sub_main/0046E880.inc"
};
char D_0046E8C0[0x40] = {
#include "sub_main/0046E8C0.inc"
};
char D_0046E900[0x50] = {
#include "sub_main/0046E900.inc"
};
char D_0046E950[0x20] = {
#include "sub_main/0046E950.inc"
};
char D_0046E970[0x50] = {
#include "sub_main/0046E970.inc"
};
char D_0046E9C0[0x60] = {
#include "sub_main/0046E9C0.inc"
};
char D_0046EA20[0x50] = {
#include "sub_main/0046EA20.inc"
};
char D_0046EA70[0x40] = {
#include "sub_main/0046EA70.inc"
};
char D_0046EAB0[0x40] = {
#include "sub_main/0046EAB0.inc"
};
char D_0046EAF0[0x50] = {
#include "sub_main/0046EAF0.inc"
};
char D_0046EB40[0x40] = {
#include "sub_main/0046EB40.inc"
};
char D_0046EB80[0x40] = {
#include "sub_main/0046EB80.inc"
};
char D_0046EBC0[0x40] = {
#include "sub_main/0046EBC0.inc"
};
char D_0046EC00[0x40] = {
#include "sub_main/0046EC00.inc"
};
char D_0046EC40[0x40] = {
#include "sub_main/0046EC40.inc"
};
char D_0046EC80[0x40] = {
#include "sub_main/0046EC80.inc"
};
char D_0046ECC0[0x40] = {
#include "sub_main/0046ECC0.inc"
};
char D_0046ED00[0x40] = {
#include "sub_main/0046ED00.inc"
};
char D_0046ED40[0x50] = {
#include "sub_main/0046ED40.inc"
};
char D_0046ED90[0x50] = {
#include "sub_main/0046ED90.inc"
};
char D_0046EDE0[0x50] = {
#include "sub_main/0046EDE0.inc"
};
char D_0046EE30[0x50] = {
#include "sub_main/0046EE30.inc"
};
char D_0046EE80[0x50] = {
#include "sub_main/0046EE80.inc"
};
char D_0046EED0[0x50] = {
#include "sub_main/0046EED0.inc"
};
char D_0046EF20[0x50] = {
#include "sub_main/0046EF20.inc"
};
char D_0046EF70[0x40] = {
#include "sub_main/0046EF70.inc"
};
char D_0046EFB0[0x40] = {
#include "sub_main/0046EFB0.inc"
};
char D_0046EFF0[0x40] = {
#include "sub_main/0046EFF0.inc"
};
char D_0046F030[0x50] = {
#include "sub_main/0046F030.inc"
};
char D_0046F080[0x50] = {
#include "sub_main/0046F080.inc"
};
char D_0046F0D0[0x50] = {
#include "sub_main/0046F0D0.inc"
};
char D_0046F120[0x50] = {
#include "sub_main/0046F120.inc"
};
char D_0046F170[0x50] = {
#include "sub_main/0046F170.inc"
};
char D_0046F1C0[0x50] = {
#include "sub_main/0046F1C0.inc"
};
char D_0046F210[0x50] = {
#include "sub_main/0046F210.inc"
};
char D_0046F260[0x40] = {
#include "sub_main/0046F260.inc"
};
char D_0046F2A0[0x40] = {
#include "sub_main/0046F2A0.inc"
};
char D_0046F2E0[0x48] = {
#include "sub_main/0046F2E0.inc"
};
char D_0046F328[15] = {
#include "sub_main/0046F328.inc"
};
char D_0046F340[0x40] = {
#include "sub_main/0046F340.inc"
};
char D_0046F380[0x40] = {
#include "sub_main/0046F380.inc"
};
char D_0046F3C0[0x40] = {
#include "sub_main/0046F3C0.inc"
};
char D_0046F400[0x40] = {
#include "sub_main/0046F400.inc"
};
char D_0046F440[0x40] = {
#include "sub_main/0046F440.inc"
};
char D_0046F480[0x30] = {
#include "sub_main/0046F480.inc"
};
char D_0046F4B0[0x30] = {
#include "sub_main/0046F4B0.inc"
};
char D_0046F4E0[0x30] = {
#include "sub_main/0046F4E0.inc"
};
char D_0046F510[0x20] = {
#include "sub_main/0046F510.inc"
};
char D_0046F530[0x30] = {
#include "sub_main/0046F530.inc"
};
char D_0046F560[0x30] = {
#include "sub_main/0046F560.inc"
};
char D_0046F590[0x20] = {
#include "sub_main/0046F590.inc"
};
char D_0046F5B0[0x60] = {
#include "sub_main/0046F5B0.inc"
};
char D_0046F610[0x30] = {
#include "sub_main/0046F610.inc"
};
char D_0046F640[0x30] = {
#include "sub_main/0046F640.inc"
};
char D_0046F670[0x40] = {
#include "sub_main/0046F670.inc"
};
char D_0046F6B0[0x30] = {
#include "sub_main/0046F6B0.inc"
};
char D_0046F6E0[0x20] = {
#include "sub_main/0046F6E0.inc"
};
char D_0046F700[0x30] = {
#include "sub_main/0046F700.inc"
};
char D_0046F730[0x40] = {
#include "sub_main/0046F730.inc"
};
char D_0046F770[0x40] = {
#include "sub_main/0046F770.inc"
};
char D_0046F7B0[0x50] = {
#include "sub_main/0046F7B0.inc"
};
char D_0046F800[0x40] = {
#include "sub_main/0046F800.inc"
};
char D_0046F840[0x40] = {
#include "sub_main/0046F840.inc"
};
char D_0046F880[0x50] = {
#include "sub_main/0046F880.inc"
};
char D_0046F8D0[0x50] = {
#include "sub_main/0046F8D0.inc"
};
char D_0046F920[0x40] = {
#include "sub_main/0046F920.inc"
};
char D_0046F960[0x40] = {
#include "sub_main/0046F960.inc"
};
char D_0046F9A0[0x50] = {
#include "sub_main/0046F9A0.inc"
};
char D_0046F9F0[0x50] = {
#include "sub_main/0046F9F0.inc"
};
char D_0046FA40[0x40] = {
#include "sub_main/0046FA40.inc"
};
char D_0046FA80[0x40] = {
#include "sub_main/0046FA80.inc"
};
char D_0046FAC0[0x40] = {
#include "sub_main/0046FAC0.inc"
};
char D_0046FB00[0x50] = {
#include "sub_main/0046FB00.inc"
};
char D_0046FB50[0x20] = {
#include "sub_main/0046FB50.inc"
};
char D_0046FB70[0x20] = {
#include "sub_main/0046FB70.inc"
};
char D_0046FB90[0x50] = {
#include "sub_main/0046FB90.inc"
};
char D_0046FBE0[0x60] = {
#include "sub_main/0046FBE0.inc"
};
char D_0046FC40[0x50] = {
#include "sub_main/0046FC40.inc"
};
char D_0046FC90[0x50] = {
#include "sub_main/0046FC90.inc"
};
char D_0046FCE0[0x50] = {
#include "sub_main/0046FCE0.inc"
};
char D_0046FD30[0x20] = {
#include "sub_main/0046FD30.inc"
};
char D_0046FD50[0x40] = {
#include "sub_main/0046FD50.inc"
};
char D_0046FD90[0x50] = {
#include "sub_main/0046FD90.inc"
};
char D_0046FDE0[0x20] = {
#include "sub_main/0046FDE0.inc"
};
char D_0046FE00[0x50] = {
#include "sub_main/0046FE00.inc"
};
char D_0046FE50[0x20] = {
#include "sub_main/0046FE50.inc"
};
char D_0046FE70[0x20] = {
#include "sub_main/0046FE70.inc"
};
char D_0046FE90[0x20] = {
#include "sub_main/0046FE90.inc"
};
char D_0046FEB0[0x50] = {
#include "sub_main/0046FEB0.inc"
};
char D_0046FF00[0x30] = {
#include "sub_main/0046FF00.inc"
};
char D_0046FF30[0x40] = {
#include "sub_main/0046FF30.inc"
};
char D_0046FF70[0x50] = {
#include "sub_main/0046FF70.inc"
};
char gHelpTexts_1_216[64] = {
#include "sub_main/0046FFC0.inc"
};
u32 D_00470000[8] = {
#include "sub_main/00470000.inc"
};
char D_00470020[0x50] = {
#include "sub_main/00470020.inc"
};
char D_00470070[0x50] = {
#include "sub_main/00470070.inc"
};
char gHelpTexts_1_219[64] = {
#include "sub_main/004700C0.inc"
};
u32 D_00470100[4] = {
#include "sub_main/00470100.inc"
};
char D_00470110[0x20] = {
#include "sub_main/00470110.inc"
};
char D_00470130[0x20] = {
#include "sub_main/00470130.inc"
};
char D_00470150[0x20] = {
#include "sub_main/00470150.inc"
};
char D_00470170[0x30] = {
#include "sub_main/00470170.inc"
};
char D_004701A0[0x30] = {
#include "sub_main/004701A0.inc"
};
char gHelpTexts_1_225[48] = {
#include "sub_main/004701D0.inc"
};
u32 D_00470200[4] = {
#include "sub_main/00470200.inc"
};
char D_00470210[0x40] = {
#include "sub_main/00470210.inc"
};
char D_00470250[0x60] = {
#include "sub_main/00470250.inc"
};
char D_004702B0[0x50] = {
#include "sub_main/004702B0.inc"
};
char D_00470300[0x50] = {
#include "sub_main/00470300.inc"
};
char D_00470350[0x60] = {
#include "sub_main/00470350.inc"
};
char D_004703B0[0x50] = {
#include "sub_main/004703B0.inc"
};
char D_00470400[0x40] = {
#include "sub_main/00470400.inc"
};
char D_00470440[0x30] = {
#include "sub_main/00470440.inc"
};
char D_00470470[0x20] = {
#include "sub_main/00470470.inc"
};
char D_00470490[0x30] = {
#include "sub_main/00470490.inc"
};
char D_004704C0[0x20] = {
#include "sub_main/004704C0.inc"
};
char D_004704E0[0x60] = {
#include "sub_main/004704E0.inc"
};
char D_00470540[0x60] = {
#include "sub_main/00470540.inc"
};
char D_004705A0[0x30] = {
#include "sub_main/004705A0.inc"
};
char D_004705D0[0x40] = {
#include "sub_main/004705D0.inc"
};
char D_00470610[0x50] = {
#include "sub_main/00470610.inc"
};
char D_00470660[0x60] = {
#include "sub_main/00470660.inc"
};
char D_004706C0[0x50] = {
#include "sub_main/004706C0.inc"
};
char D_00470710[0x60] = {
#include "sub_main/00470710.inc"
};
char D_00470770[0x50] = {
#include "sub_main/00470770.inc"
};
char D_004707C0[0x50] = {
#include "sub_main/004707C0.inc"
};
char D_00470810[0x40] = {
#include "sub_main/00470810.inc"
};
char D_00470850[0x50] = {
#include "sub_main/00470850.inc"
};
char D_004708A0[0x50] = {
#include "sub_main/004708A0.inc"
};
char D_004708F0[0x60] = {
#include "sub_main/004708F0.inc"
};
char D_00470950[0x50] = {
#include "sub_main/00470950.inc"
};
char D_004709A0[0x50] = {
#include "sub_main/004709A0.inc"
};
char D_004709F0[0x50] = {
#include "sub_main/004709F0.inc"
};
char D_00470A40[0x50] = {
#include "sub_main/00470A40.inc"
};
char D_00470A90[0x50] = {
#include "sub_main/00470A90.inc"
};
char D_00470AE0[0x30] = {
#include "sub_main/00470AE0.inc"
};
char D_00470B10[0x40] = {
#include "sub_main/00470B10.inc"
};
char D_00470B50[0x20] = {
#include "sub_main/00470B50.inc"
};
char D_00470B70[0x20] = {
#include "sub_main/00470B70.inc"
};
char D_00470B90[0x20] = {
#include "sub_main/00470B90.inc"
};
char D_00470BB0[0x20] = {
#include "sub_main/00470BB0.inc"
};
char D_00470BD0[0x20] = {
#include "sub_main/00470BD0.inc"
};
char D_00470BF0[0x20] = {
#include "sub_main/00470BF0.inc"
};
char D_00470C10[0x30] = {
#include "sub_main/00470C10.inc"
};
char D_00470C40[0x30] = {
#include "sub_main/00470C40.inc"
};
char D_00470C70[0x40] = {
#include "sub_main/00470C70.inc"
};
char D_00470CB0[0x40] = {
#include "sub_main/00470CB0.inc"
};
char D_00470CF0[0x40] = {
#include "sub_main/00470CF0.inc"
};
char D_00470D30[0x50] = {
#include "sub_main/00470D30.inc"
};
char D_00470D80[0x50] = {
#include "sub_main/00470D80.inc"
};
char D_00470DD0[0x40] = {
#include "sub_main/00470DD0.inc"
};
char D_00470E10[0x30] = {
#include "sub_main/00470E10.inc"
};
char D_00470E40[0x40] = {
#include "sub_main/00470E40.inc"
};
char D_00470E80[0x30] = {
#include "sub_main/00470E80.inc"
};
char D_00470EB0[0x50] = {
#include "sub_main/00470EB0.inc"
};
char D_00470F00[0x48] = {
#include "sub_main/00470F00.inc"
};
char D_00470F48[0xC] = {
#include "sub_main/00470F48.inc"
};
char D_00470F60[0x50] = {
#include "sub_main/00470F60.inc"
};
char D_00470FB0[0x50] = {
#include "sub_main/00470FB0.inc"
};
char D_00471000[0x50] = {
#include "sub_main/00471000.inc"
};
char D_00471050[0x20] = {
#include "sub_main/00471050.inc"
};
char D_00471070[0x50] = {
#include "sub_main/00471070.inc"
};
char D_004710C0[0x20] = {
#include "sub_main/004710C0.inc"
};
char D_004710E0[0x50] = {
#include "sub_main/004710E0.inc"
};
char D_00471130[0x50] = {
#include "sub_main/00471130.inc"
};
char D_00471180[0x50] = {
#include "sub_main/00471180.inc"
};
char D_004711D0[0x40] = {
#include "sub_main/004711D0.inc"
};
char D_00471210[0x40] = {
#include "sub_main/00471210.inc"
};
char D_00471250[0x60] = {
#include "sub_main/00471250.inc"
};
char D_004712B0[0x50] = {
#include "sub_main/004712B0.inc"
};
char D_00471300[0x40] = {
#include "sub_main/00471300.inc"
};
char D_00471340[0x40] = {
#include "sub_main/00471340.inc"
};
char D_00471380[0x40] = {
#include "sub_main/00471380.inc"
};
char D_004713C0[0x40] = {
#include "sub_main/004713C0.inc"
};
char D_00471400[0x40] = {
#include "sub_main/00471400.inc"
};
char D_00471440[0x40] = {
#include "sub_main/00471440.inc"
};
char D_00471480[0x50] = {
#include "sub_main/00471480.inc"
};
char D_004714D0[0x50] = {
#include "sub_main/004714D0.inc"
};
char D_00471520[0x50] = {
#include "sub_main/00471520.inc"
};
char D_00471570[0x50] = {
#include "sub_main/00471570.inc"
};
char D_004715C0[0x40] = {
#include "sub_main/004715C0.inc"
};
char D_00471600[0x50] = {
#include "sub_main/00471600.inc"
};
char D_00471650[0x40] = {
#include "sub_main/00471650.inc"
};
char D_00471690[0x50] = {
#include "sub_main/00471690.inc"
};
char D_004716E0[0x40] = {
#include "sub_main/004716E0.inc"
};
char D_00471720[0x50] = {
#include "sub_main/00471720.inc"
};
char D_00471770[0x40] = {
#include "sub_main/00471770.inc"
};
char D_004717B0[0x50] = {
#include "sub_main/004717B0.inc"
};
char D_00471800[0x50] = {
#include "sub_main/00471800.inc"
};
char D_00471850[0x50] = {
#include "sub_main/00471850.inc"
};
char D_004718A0[0x40] = {
#include "sub_main/004718A0.inc"
};
char D_004718E0[0x40] = {
#include "sub_main/004718E0.inc"
};
char D_00471920[0x40] = {
#include "sub_main/00471920.inc"
};
char D_00471960[0x40] = {
#include "sub_main/00471960.inc"
};
char D_004719A0[0x40] = {
#include "sub_main/004719A0.inc"
};
char D_004719E0[0x40] = {
#include "sub_main/004719E0.inc"
};
char D_00471A20[0x40] = {
#include "sub_main/00471A20.inc"
};
char D_00471A60[0x50] = {
#include "sub_main/00471A60.inc"
};
char D_00471AB0[0x40] = {
#include "sub_main/00471AB0.inc"
};
char D_00471AF0[0x50] = {
#include "sub_main/00471AF0.inc"
};
char D_00471B40[0x40] = {
#include "sub_main/00471B40.inc"
};
char D_00471B80[0x60] = {
#include "sub_main/00471B80.inc"
};
char D_00471BE0[0x50] = {
#include "sub_main/00471BE0.inc"
};
char D_00471C30[0x60] = {
#include "sub_main/00471C30.inc"
};
char D_00471C90[0x60] = {
#include "sub_main/00471C90.inc"
};
char D_00471CF0[0x50] = {
#include "sub_main/00471CF0.inc"
};
char D_00471D40[0x60] = {
#include "sub_main/00471D40.inc"
};
char D_00471DA0[0x50] = {
#include "sub_main/00471DA0.inc"
};
char D_00471DF0[0x40] = {
#include "sub_main/00471DF0.inc"
};
char D_00471E30[0x50] = {
#include "sub_main/00471E30.inc"
};
char D_00471E80[0x60] = {
#include "sub_main/00471E80.inc"
};
char D_00471EE0[0x60] = {
#include "sub_main/00471EE0.inc"
};
char D_00471F40[0x50] = {
#include "sub_main/00471F40.inc"
};
char D_00471F90[0x30] = {
#include "sub_main/00471F90.inc"
};
char D_00471FC0[0x30] = {
#include "sub_main/00471FC0.inc"
};
char D_00471FF0[0x40] = {
#include "sub_main/00471FF0.inc"
};
char D_00472030[0x60] = {
#include "sub_main/00472030.inc"
};
char D_00472090[0x30] = {
#include "sub_main/00472090.inc"
};
char D_004720C0[0x40] = {
#include "sub_main/004720C0.inc"
};
char D_00472100[0x50] = {
#include "sub_main/00472100.inc"
};
char D_00472150[0x60] = {
#include "sub_main/00472150.inc"
};
char D_004721B0[0x50] = {
#include "sub_main/004721B0.inc"
};
char D_00472200[0x50] = {
#include "sub_main/00472200.inc"
};
char D_00472250[0x40] = {
#include "sub_main/00472250.inc"
};
char D_00472290[0x40] = {
#include "sub_main/00472290.inc"
};
char D_004722D0[0x50] = {
#include "sub_main/004722D0.inc"
};
char D_00472320[0x60] = {
#include "sub_main/00472320.inc"
};
char D_00472380[0x50] = {
#include "sub_main/00472380.inc"
};
char D_004723D0[0x20] = {
#include "sub_main/004723D0.inc"
};
char D_004723F0[0x40] = {
#include "sub_main/004723F0.inc"
};
char D_00472430[0x20] = {
#include "sub_main/00472430.inc"
};
char D_00472450[0x30] = {
#include "sub_main/00472450.inc"
};
char D_00472480[0x30] = {
#include "sub_main/00472480.inc"
};
char D_004724B0[0x40] = {
#include "sub_main/004724B0.inc"
};
char D_004724F0[0x50] = {
#include "sub_main/004724F0.inc"
};
char D_00472540[0x40] = {
#include "sub_main/00472540.inc"
};
char D_00472580[0x50] = {
#include "sub_main/00472580.inc"
};
char D_004725D0[0x50] = {
#include "sub_main/004725D0.inc"
};
char D_00472620[0x50] = {
#include "sub_main/00472620.inc"
};
char D_00472670[0x60] = {
#include "sub_main/00472670.inc"
};
char D_004726D0[0x50] = {
#include "sub_main/004726D0.inc"
};
char D_00472720[0x50] = {
#include "sub_main/00472720.inc"
};
char D_00472770[0x50] = {
#include "sub_main/00472770.inc"
};
char D_004727C0[0x50] = {
#include "sub_main/004727C0.inc"
};
char D_00472810[0x40] = {
#include "sub_main/00472810.inc"
};
char D_00472850[0x50] = {
#include "sub_main/00472850.inc"
};
char D_004728A0[0x50] = {
#include "sub_main/004728A0.inc"
};
char D_004728F0[0x50] = {
#include "sub_main/004728F0.inc"
};
char D_00472940[0x60] = {
#include "sub_main/00472940.inc"
};
char D_004729A0[0x40] = {
#include "sub_main/004729A0.inc"
};
char D_004729E0[0x50] = {
#include "sub_main/004729E0.inc"
};
char D_00472A30[0x50] = {
#include "sub_main/00472A30.inc"
};
char D_00472A80[0x60] = {
#include "sub_main/00472A80.inc"
};
char D_00472AE0[0x40] = {
#include "sub_main/00472AE0.inc"
};
char D_00472B20[0x50] = {
#include "sub_main/00472B20.inc"
};
char D_00472B70[0x60] = {
#include "sub_main/00472B70.inc"
};
char D_00472BD0[0x50] = {
#include "sub_main/00472BD0.inc"
};
char D_00472C20[0x50] = {
#include "sub_main/00472C20.inc"
};
char D_00472C70[0x60] = {
#include "sub_main/00472C70.inc"
};
char D_00472CD0[0x40] = {
#include "sub_main/00472CD0.inc"
};
char D_00472D10[0x50] = {
#include "sub_main/00472D10.inc"
};
char D_00472D60[0x40] = {
#include "sub_main/00472D60.inc"
};
char D_00472DA0[0x50] = {
#include "sub_main/00472DA0.inc"
};
char D_00472DF0[0x50] = {
#include "sub_main/00472DF0.inc"
};
char D_00472E40[0x50] = {
#include "sub_main/00472E40.inc"
};
char D_00472E90[0x30] = {
#include "sub_main/00472E90.inc"
};
char D_00472EC0[0x50] = {
#include "sub_main/00472EC0.inc"
};
char D_00472F10[0x50] = {
#include "sub_main/00472F10.inc"
};
char D_00472F60[0x50] = {
#include "sub_main/00472F60.inc"
};
char D_00472FB0[0x40] = {
#include "sub_main/00472FB0.inc"
};
char D_00472FF0[0x50] = {
#include "sub_main/00472FF0.inc"
};
char D_00473040[0x40] = {
#include "sub_main/00473040.inc"
};
char D_00473080[0x50] = {
#include "sub_main/00473080.inc"
};
char D_004730D0[0x50] = {
#include "sub_main/004730D0.inc"
};
char D_00473120[0x50] = {
#include "sub_main/00473120.inc"
};
char D_00473170[0x30] = {
#include "sub_main/00473170.inc"
};
char D_004731A0[0x40] = {
#include "sub_main/004731A0.inc"
};
char D_004731E0[0x50] = {
#include "sub_main/004731E0.inc"
};
char D_00473230[0x50] = {
#include "sub_main/00473230.inc"
};
char D_00473280[0x50] = {
#include "sub_main/00473280.inc"
};
char D_004732D0[0x50] = {
#include "sub_main/004732D0.inc"
};
char D_00473320[0x60] = {
#include "sub_main/00473320.inc"
};
char D_00473380[0x50] = {
#include "sub_main/00473380.inc"
};
char D_004733D0[0x50] = {
#include "sub_main/004733D0.inc"
};
char D_00473420[0x50] = {
#include "sub_main/00473420.inc"
};
char D_00473470[0x50] = {
#include "sub_main/00473470.inc"
};
char D_004734C0[0x40] = {
#include "sub_main/004734C0.inc"
};
char D_00473500[0x60] = {
#include "sub_main/00473500.inc"
};
char D_00473560[0x50] = {
#include "sub_main/00473560.inc"
};
char D_004735B0[0x50] = {
#include "sub_main/004735B0.inc"
};
char D_00473600[0x50] = {
#include "sub_main/00473600.inc"
};
char D_00473650[0x60] = {
#include "sub_main/00473650.inc"
};
char D_004736B0[0x60] = {
#include "sub_main/004736B0.inc"
};
char D_00473710[0x50] = {
#include "sub_main/00473710.inc"
};
char D_00473760[0x50] = {
#include "sub_main/00473760.inc"
};
char D_004737B0[0x40] = {
#include "sub_main/004737B0.inc"
};
char D_004737F0[0x50] = {
#include "sub_main/004737F0.inc"
};
char D_00473840[0x40] = {
#include "sub_main/00473840.inc"
};
char D_00473880[0x60] = {
#include "sub_main/00473880.inc"
};
char D_004738E0[0x50] = {
#include "sub_main/004738E0.inc"
};
char D_00473930[0x50] = {
#include "sub_main/00473930.inc"
};
char D_00473980[0x50] = {
#include "sub_main/00473980.inc"
};
char D_004739D0[0x50] = {
#include "sub_main/004739D0.inc"
};
char D_00473A20[0x50] = {
#include "sub_main/00473A20.inc"
};
char D_00473A70[0x60] = {
#include "sub_main/00473A70.inc"
};
char D_00473AD0[0x40] = {
#include "sub_main/00473AD0.inc"
};
char D_00473B10[0x50] = {
#include "sub_main/00473B10.inc"
};
char D_00473B60[0x60] = {
#include "sub_main/00473B60.inc"
};
char D_00473BC0[0x50] = {
#include "sub_main/00473BC0.inc"
};
char D_00473C10[0x50] = {
#include "sub_main/00473C10.inc"
};
char D_00473C60[0x40] = {
#include "sub_main/00473C60.inc"
};
char D_00473CA0[0x50] = {
#include "sub_main/00473CA0.inc"
};
char D_00473CF0[0x50] = {
#include "sub_main/00473CF0.inc"
};
char D_00473D40[0x50] = {
#include "sub_main/00473D40.inc"
};
char D_00473D90[0x60] = {
#include "sub_main/00473D90.inc"
};
char D_00473DF0[0x50] = {
#include "sub_main/00473DF0.inc"
};
char D_00473E40[0x50] = {
#include "sub_main/00473E40.inc"
};
char D_00473E90[0x50] = {
#include "sub_main/00473E90.inc"
};
char D_00473EE0[0x40] = {
#include "sub_main/00473EE0.inc"
};
char D_00473F20[0x50] = {
#include "sub_main/00473F20.inc"
};
char D_00473F70[0x50] = {
#include "sub_main/00473F70.inc"
};
char D_00473FC0[0x50] = {
#include "sub_main/00473FC0.inc"
};
char D_00474010[0x50] = {
#include "sub_main/00474010.inc"
};
char D_00474060[0x50] = {
#include "sub_main/00474060.inc"
};
char D_004740B0[0x50] = {
#include "sub_main/004740B0.inc"
};
char D_00474100[0x50] = {
#include "sub_main/00474100.inc"
};
char D_00474150[0x30] = {
#include "sub_main/00474150.inc"
};
char D_00474180[0x50] = {
#include "sub_main/00474180.inc"
};
char D_004741D0[0x50] = {
#include "sub_main/004741D0.inc"
};
char D_00474220[0x50] = {
#include "sub_main/00474220.inc"
};
char D_00474270[0x50] = {
#include "sub_main/00474270.inc"
};
char D_004742C0[0x50] = {
#include "sub_main/004742C0.inc"
};
char D_00474310[0x40] = {
#include "sub_main/00474310.inc"
};
char D_00474350[0x40] = {
#include "sub_main/00474350.inc"
};
char D_00474390[0x50] = {
#include "sub_main/00474390.inc"
};
char D_004743E0[0x50] = {
#include "sub_main/004743E0.inc"
};
char D_00474430[0x50] = {
#include "sub_main/00474430.inc"
};
char D_00474480[0x50] = {
#include "sub_main/00474480.inc"
};
char D_004744D0[0x60] = {
#include "sub_main/004744D0.inc"
};
char D_00474530[0x50] = {
#include "sub_main/00474530.inc"
};
char D_00474580[0x50] = {
#include "sub_main/00474580.inc"
};
char D_004745D0[0x50] = {
#include "sub_main/004745D0.inc"
};
char D_00474620[0x50] = {
#include "sub_main/00474620.inc"
};
char D_00474670[0x50] = {
#include "sub_main/00474670.inc"
};
char D_004746C0[0x60] = {
#include "sub_main/004746C0.inc"
};
char D_00474720[0x40] = {
#include "sub_main/00474720.inc"
};
char D_00474760[0x50] = {
#include "sub_main/00474760.inc"
};
char D_004747B0[0x50] = {
#include "sub_main/004747B0.inc"
};
char D_00474800[0x50] = {
#include "sub_main/00474800.inc"
};
char D_00474850[0x50] = {
#include "sub_main/00474850.inc"
};
char D_004748A0[0x60] = {
#include "sub_main/004748A0.inc"
};
char D_00474900[0x50] = {
#include "sub_main/00474900.inc"
};
char D_00474950[0x50] = {
#include "sub_main/00474950.inc"
};
char D_004749A0[0x60] = {
#include "sub_main/004749A0.inc"
};
char D_00474A00[0x60] = {
#include "sub_main/00474A00.inc"
};
char D_00474A60[0x50] = {
#include "sub_main/00474A60.inc"
};
char D_00474AB0[0x40] = {
#include "sub_main/00474AB0.inc"
};
char D_00474AF0[0x60] = {
#include "sub_main/00474AF0.inc"
};
char D_00474B50[0x50] = {
#include "sub_main/00474B50.inc"
};
char D_00474BA0[0x50] = {
#include "sub_main/00474BA0.inc"
};
char D_00474BF0[0x60] = {
#include "sub_main/00474BF0.inc"
};
char D_00474C50[0x60] = {
#include "sub_main/00474C50.inc"
};
char D_00474CB0[0x50] = {
#include "sub_main/00474CB0.inc"
};
char D_00474D00[0x50] = {
#include "sub_main/00474D00.inc"
};
char D_00474D50[0x50] = {
#include "sub_main/00474D50.inc"
};
char D_00474DA0[0x50] = {
#include "sub_main/00474DA0.inc"
};
char D_00474DF0[0x50] = {
#include "sub_main/00474DF0.inc"
};
char D_00474E40[0x50] = {
#include "sub_main/00474E40.inc"
};
char D_00474E90[0x40] = {
#include "sub_main/00474E90.inc"
};
char D_00474ED0[0x60] = {
#include "sub_main/00474ED0.inc"
};
char D_00474F30[0x50] = {
#include "sub_main/00474F30.inc"
};
char D_00474F80[0x50] = {
#include "sub_main/00474F80.inc"
};
char D_00474FD0[0x40] = {
#include "sub_main/00474FD0.inc"
};
char D_00475010[0x50] = {
#include "sub_main/00475010.inc"
};
char D_00475060[0x50] = {
#include "sub_main/00475060.inc"
};
char D_004750B0[0x60] = {
#include "sub_main/004750B0.inc"
};
char D_00475110[0x50] = {
#include "sub_main/00475110.inc"
};
char D_00475160[0x50] = {
#include "sub_main/00475160.inc"
};
char D_004751B0[0x50] = {
#include "sub_main/004751B0.inc"
};
char D_00475200[0x60] = {
#include "sub_main/00475200.inc"
};
char D_00475260[0x50] = {
#include "sub_main/00475260.inc"
};
char D_004752B0[0x50] = {
#include "sub_main/004752B0.inc"
};
char D_00475300[0x40] = {
#include "sub_main/00475300.inc"
};
char D_00475340[0x50] = {
#include "sub_main/00475340.inc"
};
char D_00475390[0x60] = {
#include "sub_main/00475390.inc"
};
char D_004753F0[0x60] = {
#include "sub_main/004753F0.inc"
};
char D_00475450[0x50] = {
#include "sub_main/00475450.inc"
};
char D_004754A0[0x40] = {
#include "sub_main/004754A0.inc"
};
char D_004754E0[0x60] = {
#include "sub_main/004754E0.inc"
};
char D_00475540[0x60] = {
#include "sub_main/00475540.inc"
};
char D_004755A0[0x50] = {
#include "sub_main/004755A0.inc"
};
char D_004755F0[0x50] = {
#include "sub_main/004755F0.inc"
};
char D_00475640[0x50] = {
#include "sub_main/00475640.inc"
};
char D_00475690[0x60] = {
#include "sub_main/00475690.inc"
};
char D_004756F0[0x50] = {
#include "sub_main/004756F0.inc"
};
char D_00475740[0x50] = {
#include "sub_main/00475740.inc"
};
char D_00475790[0x40] = {
#include "sub_main/00475790.inc"
};
char D_004757D0[0x50] = {
#include "sub_main/004757D0.inc"
};
char D_00475820[0x40] = {
#include "sub_main/00475820.inc"
};
char D_00475860[0x50] = {
#include "sub_main/00475860.inc"
};
char D_004758B0[0x50] = {
#include "sub_main/004758B0.inc"
};
char D_00475900[0x50] = {
#include "sub_main/00475900.inc"
};
char D_00475950[0x60] = {
#include "sub_main/00475950.inc"
};
char D_004759B0[0x60] = {
#include "sub_main/004759B0.inc"
};
char D_00475A10[0x50] = {
#include "sub_main/00475A10.inc"
};
char D_00475A60[0x40] = {
#include "sub_main/00475A60.inc"
};
char D_00475AA0[0x50] = {
#include "sub_main/00475AA0.inc"
};
char D_00475AF0[0x50] = {
#include "sub_main/00475AF0.inc"
};
char D_00475B40[0x40] = {
#include "sub_main/00475B40.inc"
};
char D_00475B80[0x30] = {
#include "sub_main/00475B80.inc"
};
char D_00475BB0[0x40] = {
#include "sub_main/00475BB0.inc"
};
char D_00475BF0[0x50] = {
#include "sub_main/00475BF0.inc"
};
char D_00475C40[0x50] = {
#include "sub_main/00475C40.inc"
};
char D_00475C90[0x50] = {
#include "sub_main/00475C90.inc"
};
char D_00475CE0[0x50] = {
#include "sub_main/00475CE0.inc"
};
char D_00475D30[0x50] = {
#include "sub_main/00475D30.inc"
};
char D_00475D80[0x50] = {
#include "sub_main/00475D80.inc"
};
char D_00475DD0[0x50] = {
#include "sub_main/00475DD0.inc"
};
char D_00475E20[0x50] = {
#include "sub_main/00475E20.inc"
};
char D_00475E70[0x50] = {
#include "sub_main/00475E70.inc"
};
char D_00475EC0[0x30] = {
#include "sub_main/00475EC0.inc"
};
char D_00475EF0[0x50] = {
#include "sub_main/00475EF0.inc"
};
char D_00475F40[0x50] = {
#include "sub_main/00475F40.inc"
};
char D_00475F90[0x30] = {
#include "sub_main/00475F90.inc"
};
char D_00475FC0[0x40] = {
#include "sub_main/00475FC0.inc"
};
char D_00476000[0x40] = {
#include "sub_main/00476000.inc"
};
char D_00476040[0x40] = {
#include "sub_main/00476040.inc"
};
char D_00476080[0x30] = {
#include "sub_main/00476080.inc"
};
char D_004760B0[0x40] = {
#include "sub_main/004760B0.inc"
};
char D_004760F0[0x40] = {
#include "sub_main/004760F0.inc"
};
char D_00476130[0x40] = {
#include "sub_main/00476130.inc"
};
char D_00476170[0x40] = {
#include "sub_main/00476170.inc"
};
char D_004761B0[0x40] = {
#include "sub_main/004761B0.inc"
};
char D_004761F0[0x50] = {
#include "sub_main/004761F0.inc"
};
char D_00476240[0x60] = {
#include "sub_main/00476240.inc"
};
char D_004762A0[0x50] = {
#include "sub_main/004762A0.inc"
};
char D_004762F0[0x40] = {
#include "sub_main/004762F0.inc"
};
char D_00476330[0x40] = {
#include "sub_main/00476330.inc"
};
char D_00476370[0x50] = {
#include "sub_main/00476370.inc"
};
char D_004763C0[0x50] = {
#include "sub_main/004763C0.inc"
};
char D_00476410[0x50] = {
#include "sub_main/00476410.inc"
};
char D_00476460[0x60] = {
#include "sub_main/00476460.inc"
};
char D_004764C0[0x50] = {
#include "sub_main/004764C0.inc"
};
char D_00476510[0x50] = {
#include "sub_main/00476510.inc"
};
char D_00476560[0x50] = {
#include "sub_main/00476560.inc"
};
char D_004765B0[0x50] = {
#include "sub_main/004765B0.inc"
};
char D_00476600[0x40] = {
#include "sub_main/00476600.inc"
};
char D_00476640[0x30] = {
#include "sub_main/00476640.inc"
};
char D_00476670[0x50] = {
#include "sub_main/00476670.inc"
};
char D_004766C0[0x40] = {
#include "sub_main/004766C0.inc"
};
char D_00476700[0x50] = {
#include "sub_main/00476700.inc"
};
char D_00476750[0x50] = {
#include "sub_main/00476750.inc"
};
char D_004767A0[0x50] = {
#include "sub_main/004767A0.inc"
};
char D_004767F0[0x50] = {
#include "sub_main/004767F0.inc"
};
char D_00476840[0x50] = {
#include "sub_main/00476840.inc"
};
char D_00476890[0x40] = {
#include "sub_main/00476890.inc"
};
char D_004768D0[0x50] = {
#include "sub_main/004768D0.inc"
};
char D_00476920[0x20] = {
#include "sub_main/00476920.inc"
};
char D_00476940[0x50] = {
#include "sub_main/00476940.inc"
};
char D_00476990[0x50] = {
#include "sub_main/00476990.inc"
};
char D_004769E0[0x60] = {
#include "sub_main/004769E0.inc"
};
char D_00476A40[0x50] = {
#include "sub_main/00476A40.inc"
};
char D_00476A90[0x30] = {
#include "sub_main/00476A90.inc"
};
char D_00476AC0[0x40] = {
#include "sub_main/00476AC0.inc"
};
char D_00476B00[0x50] = {
#include "sub_main/00476B00.inc"
};
char D_00476B50[0x50] = {
#include "sub_main/00476B50.inc"
};
char D_00476BA0[0x50] = {
#include "sub_main/00476BA0.inc"
};
char D_00476BF0[0x40] = {
#include "sub_main/00476BF0.inc"
};
char D_00476C30[0x40] = {
#include "sub_main/00476C30.inc"
};
char D_00476C70[0x50] = {
#include "sub_main/00476C70.inc"
};
char D_00476CC0[0x50] = {
#include "sub_main/00476CC0.inc"
};
char D_00476D10[0x50] = {
#include "sub_main/00476D10.inc"
};
char D_00476D60[0x50] = {
#include "sub_main/00476D60.inc"
};
char D_00476DB0[0x50] = {
#include "sub_main/00476DB0.inc"
};
char D_00476E00[0x50] = {
#include "sub_main/00476E00.inc"
};
char D_00476E50[0x50] = {
#include "sub_main/00476E50.inc"
};
char D_00476EA0[0x60] = {
#include "sub_main/00476EA0.inc"
};
char D_00476F00[0x40] = {
#include "sub_main/00476F00.inc"
};
char D_00476F40[0x50] = {
#include "sub_main/00476F40.inc"
};
char D_00476F90[0x50] = {
#include "sub_main/00476F90.inc"
};
char D_00476FE0[0x40] = {
#include "sub_main/00476FE0.inc"
};
char D_00477020[0x30] = {
#include "sub_main/00477020.inc"
};
char D_00477050[0x50] = {
#include "sub_main/00477050.inc"
};
char D_004770A0[0x40] = {
#include "sub_main/004770A0.inc"
};
char D_004770E0[0x30] = {
#include "sub_main/004770E0.inc"
};
char D_00477110[0x50] = {
#include "sub_main/00477110.inc"
};
char D_00477160[0x50] = {
#include "sub_main/00477160.inc"
};
char D_004771B0[0x60] = {
#include "sub_main/004771B0.inc"
};
char D_00477210[0x50] = {
#include "sub_main/00477210.inc"
};
char D_00477260[0x50] = {
#include "sub_main/00477260.inc"
};
char D_004772B0[0x50] = {
#include "sub_main/004772B0.inc"
};
char D_00477300[0x50] = {
#include "sub_main/00477300.inc"
};
char D_00477350[0x60] = {
#include "sub_main/00477350.inc"
};
char D_004773B0[0x40] = {
#include "sub_main/004773B0.inc"
};
char D_004773F0[0x40] = {
#include "sub_main/004773F0.inc"
};
char D_00477430[0x28] = {
#include "sub_main/00477430.inc"
};
char D_00477458[0xC] = {
#include "sub_main/00477458.inc"
};
char D_00477470[0x60] = {
#include "sub_main/00477470.inc"
};
char D_004774D0[0x50] = {
#include "sub_main/004774D0.inc"
};
char D_00477520[0x60] = {
#include "sub_main/00477520.inc"
};
char D_00477580[0x60] = {
#include "sub_main/00477580.inc"
};
char D_004775E0[0x60] = {
#include "sub_main/004775E0.inc"
};
char D_00477640[0x50] = {
#include "sub_main/00477640.inc"
};
char D_00477690[0x50] = {
#include "sub_main/00477690.inc"
};
char D_004776E0[0x60] = {
#include "sub_main/004776E0.inc"
};
char D_00477740[0x50] = {
#include "sub_main/00477740.inc"
};
char D_00477790[0x50] = {
#include "sub_main/00477790.inc"
};
char D_004777E0[0x50] = {
#include "sub_main/004777E0.inc"
};
char D_00477830[0x50] = {
#include "sub_main/00477830.inc"
};
char D_00477880[0x50] = {
#include "sub_main/00477880.inc"
};
char D_004778D0[0x50] = {
#include "sub_main/004778D0.inc"
};
char D_00477920[0x60] = {
#include "sub_main/00477920.inc"
};
char D_00477980[0x60] = {
#include "sub_main/00477980.inc"
};
char D_004779E0[0x50] = {
#include "sub_main/004779E0.inc"
};
char D_00477A30[0x50] = {
#include "sub_main/00477A30.inc"
};
char D_00477A80[0x50] = {
#include "sub_main/00477A80.inc"
};
char D_00477AD0[0x50] = {
#include "sub_main/00477AD0.inc"
};
char D_00477B20[0x50] = {
#include "sub_main/00477B20.inc"
};
char D_00477B70[0x60] = {
#include "sub_main/00477B70.inc"
};
char D_00477BD0[0x50] = {
#include "sub_main/00477BD0.inc"
};
char D_00477C20[0x50] = {
#include "sub_main/00477C20.inc"
};
char D_00477C70[0x60] = {
#include "sub_main/00477C70.inc"
};
char D_00477CD0[0x50] = {
#include "sub_main/00477CD0.inc"
};
char D_00477D20[0x50] = {
#include "sub_main/00477D20.inc"
};
char D_00477D70[0x50] = {
#include "sub_main/00477D70.inc"
};
char D_00477DC0[0x60] = {
#include "sub_main/00477DC0.inc"
};
char D_00477E20[0x60] = {
#include "sub_main/00477E20.inc"
};
char D_00477E80[0x60] = {
#include "sub_main/00477E80.inc"
};
char D_00477EE0[0x60] = {
#include "sub_main/00477EE0.inc"
};
char D_00477F40[0x40] = {
#include "sub_main/00477F40.inc"
};
char D_00477F80[0x30] = {
#include "sub_main/00477F80.inc"
};
char D_00477FB0[0x30] = {
#include "sub_main/00477FB0.inc"
};
char D_00477FE0[0x50] = {
#include "sub_main/00477FE0.inc"
};
char D_00478030[0x50] = {
#include "sub_main/00478030.inc"
};
char D_00478080[0x50] = {
#include "sub_main/00478080.inc"
};
char D_004780D0[0x30] = {
#include "sub_main/004780D0.inc"
};
char D_00478100[0x50] = {
#include "sub_main/00478100.inc"
};
char D_00478150[0x40] = {
#include "sub_main/00478150.inc"
};
char D_00478190[0x50] = {
#include "sub_main/00478190.inc"
};
char D_004781E0[0x30] = {
#include "sub_main/004781E0.inc"
};
char D_00478210[0x50] = {
#include "sub_main/00478210.inc"
};
char D_00478260[0x10] = {
#include "sub_main/00478260.inc"
};
char D_00478270[0x20] = {
#include "sub_main/00478270.inc"
};
char D_00478290[0x40] = {
#include "sub_main/00478290.inc"
};
char D_004782D0[0x40] = {
#include "sub_main/004782D0.inc"
};
char D_00478310[0x20] = {
#include "sub_main/00478310.inc"
};
char D_00478330[0x20] = {
#include "sub_main/00478330.inc"
};
char D_00478350[0x40] = {
#include "sub_main/00478350.inc"
};
char D_00478390[0x50] = {
#include "sub_main/00478390.inc"
};
char D_004783E0[0x50] = {
#include "sub_main/004783E0.inc"
};
char D_00478430[0x50] = {
#include "sub_main/00478430.inc"
};
char D_00478480[0x40] = {
#include "sub_main/00478480.inc"
};
char D_004784C0[0x50] = {
#include "sub_main/004784C0.inc"
};
char D_00478510[0x40] = {
#include "sub_main/00478510.inc"
};
char D_00478550[0x40] = {
#include "sub_main/00478550.inc"
};
char D_00478590[0x50] = {
#include "sub_main/00478590.inc"
};
char D_004785E0[0x40] = {
#include "sub_main/004785E0.inc"
};
char D_00478620[0x50] = {
#include "sub_main/00478620.inc"
};
char D_00478670[0x50] = {
#include "sub_main/00478670.inc"
};
char D_004786C0[0x50] = {
#include "sub_main/004786C0.inc"
};
char D_00478710[0x40] = {
#include "sub_main/00478710.inc"
};
char D_00478750[0x40] = {
#include "sub_main/00478750.inc"
};
char D_00478790[0x40] = {
#include "sub_main/00478790.inc"
};
char D_004787D0[0x50] = {
#include "sub_main/004787D0.inc"
};
char D_00478820[0x40] = {
#include "sub_main/00478820.inc"
};
char D_00478860[0x40] = {
#include "sub_main/00478860.inc"
};
char D_004788A0[0x40] = {
#include "sub_main/004788A0.inc"
};
char D_004788E0[0x40] = {
#include "sub_main/004788E0.inc"
};
char D_00478920[0x50] = {
#include "sub_main/00478920.inc"
};
char D_00478970[0x50] = {
#include "sub_main/00478970.inc"
};
char D_004789C0[0x50] = {
#include "sub_main/004789C0.inc"
};
char D_00478A10[0x50] = {
#include "sub_main/00478A10.inc"
};
char D_00478A60[0x50] = {
#include "sub_main/00478A60.inc"
};
char D_00478AB0[0x50] = {
#include "sub_main/00478AB0.inc"
};
char D_00478B00[0x50] = {
#include "sub_main/00478B00.inc"
};
char D_00478B50[0x40] = {
#include "sub_main/00478B50.inc"
};
char D_00478B90[0x40] = {
#include "sub_main/00478B90.inc"
};
char D_00478BD0[0x40] = {
#include "sub_main/00478BD0.inc"
};
char D_00478C10[0x50] = {
#include "sub_main/00478C10.inc"
};
char D_00478C60[0x50] = {
#include "sub_main/00478C60.inc"
};
char D_00478CB0[0x50] = {
#include "sub_main/00478CB0.inc"
};
char D_00478D00[0x50] = {
#include "sub_main/00478D00.inc"
};
char D_00478D50[0x50] = {
#include "sub_main/00478D50.inc"
};
char D_00478DA0[0x60] = {
#include "sub_main/00478DA0.inc"
};
char D_00478E00[0x50] = {
#include "sub_main/00478E00.inc"
};
char D_00478E50[0x60] = {
#include "sub_main/00478E50.inc"
};
char D_00478EB0[0x50] = {
#include "sub_main/00478EB0.inc"
};
char D_00478F00[0x50] = {
#include "sub_main/00478F00.inc"
};
char D_00478F50[0x50] = {
#include "sub_main/00478F50.inc"
};
char D_00478FA0[0x60] = {
#include "sub_main/00478FA0.inc"
};
char D_00479000[0x60] = {
#include "sub_main/00479000.inc"
};
char D_00479060[0x60] = {
#include "sub_main/00479060.inc"
};
char D_004790C0[0x60] = {
#include "sub_main/004790C0.inc"
};
char D_00479120[0x60] = {
#include "sub_main/00479120.inc"
};
char D_00479180[0x50] = {
#include "sub_main/00479180.inc"
};
char D_004791D0[0x40] = {
#include "sub_main/004791D0.inc"
};
char D_00479210[0x40] = {
#include "sub_main/00479210.inc"
};
char D_00479250[0x50] = {
#include "sub_main/00479250.inc"
};
char D_004792A0[0x40] = {
#include "sub_main/004792A0.inc"
};
char D_004792E0[0x60] = {
#include "sub_main/004792E0.inc"
};
char D_00479340[0x20] = {
#include "sub_main/00479340.inc"
};
char D_00479360[0x50] = {
#include "sub_main/00479360.inc"
};
char D_004793B0[0x60] = {
#include "sub_main/004793B0.inc"
};
char D_00479410[0x50] = {
#include "sub_main/00479410.inc"
};
char D_00479460[0x30] = {
#include "sub_main/00479460.inc"
};
char D_00479490[0x60] = {
#include "sub_main/00479490.inc"
};
char D_004794F0[0x50] = {
#include "sub_main/004794F0.inc"
};
char D_00479540[0x50] = {
#include "sub_main/00479540.inc"
};
char D_00479590[0x50] = {
#include "sub_main/00479590.inc"
};
char D_004795E0[0x50] = {
#include "sub_main/004795E0.inc"
};
char D_00479630[0x30] = {
#include "sub_main/00479630.inc"
};
char D_00479660[0x50] = {
#include "sub_main/00479660.inc"
};
char D_004796B0[0x50] = {
#include "sub_main/004796B0.inc"
};
char D_00479700[0x50] = {
#include "sub_main/00479700.inc"
};
char D_00479750[0x50] = {
#include "sub_main/00479750.inc"
};
char D_004797A0[0x50] = {
#include "sub_main/004797A0.inc"
};
char D_004797F0[0x50] = {
#include "sub_main/004797F0.inc"
};
char D_00479840[0x50] = {
#include "sub_main/00479840.inc"
};
char D_00479890[0x50] = {
#include "sub_main/00479890.inc"
};
char D_004798E0[0x50] = {
#include "sub_main/004798E0.inc"
};
char D_00479930[0x50] = {
#include "sub_main/00479930.inc"
};
char D_00479980[0x60] = {
#include "sub_main/00479980.inc"
};
char D_004799E0[0x40] = {
#include "sub_main/004799E0.inc"
};
char D_00479A20[0x40] = {
#include "sub_main/00479A20.inc"
};
char D_00479A60[0x40] = {
#include "sub_main/00479A60.inc"
};
char D_00479AA0[0x40] = {
#include "sub_main/00479AA0.inc"
};
char D_00479AE0[0x50] = {
#include "sub_main/00479AE0.inc"
};
char D_00479B30[0x40] = {
#include "sub_main/00479B30.inc"
};
char D_00479B70[0x50] = {
#include "sub_main/00479B70.inc"
};
char D_00479BC0[0x50] = {
#include "sub_main/00479BC0.inc"
};
char D_00479C10[0x40] = {
#include "sub_main/00479C10.inc"
};
char D_00479C50[0x40] = {
#include "sub_main/00479C50.inc"
};
char D_00479C90[0x40] = {
#include "sub_main/00479C90.inc"
};
char D_00479CD0[0x50] = {
#include "sub_main/00479CD0.inc"
};
char D_00479D20[0x50] = {
#include "sub_main/00479D20.inc"
};
char D_00479D70[0x40] = {
#include "sub_main/00479D70.inc"
};
char D_00479DB0[0x40] = {
#include "sub_main/00479DB0.inc"
};
char D_00479DF0[0x40] = {
#include "sub_main/00479DF0.inc"
};
char D_00479E30[0x40] = {
#include "sub_main/00479E30.inc"
};
char D_00479E70[0x40] = {
#include "sub_main/00479E70.inc"
};
char D_00479EB0[0x40] = {
#include "sub_main/00479EB0.inc"
};
char D_00479EF0[0x40] = {
#include "sub_main/00479EF0.inc"
};
char D_00479F30[0x40] = {
#include "sub_main/00479F30.inc"
};
char D_00479F70[0x40] = {
#include "sub_main/00479F70.inc"
};
char D_00479FB0[0x50] = {
#include "sub_main/00479FB0.inc"
};
char D_0047A000[0x40] = {
#include "sub_main/0047A000.inc"
};
char D_0047A040[0x50] = {
#include "sub_main/0047A040.inc"
};
char D_0047A090[0x50] = {
#include "sub_main/0047A090.inc"
};
char D_0047A0E0[0x50] = {
#include "sub_main/0047A0E0.inc"
};
char D_0047A130[0x50] = {
#include "sub_main/0047A130.inc"
};
char D_0047A180[0x50] = {
#include "sub_main/0047A180.inc"
};
char D_0047A1D0[0x40] = {
#include "sub_main/0047A1D0.inc"
};
char D_0047A210[0x50] = {
#include "sub_main/0047A210.inc"
};
char D_0047A260[0x50] = {
#include "sub_main/0047A260.inc"
};
char D_0047A2B0[0x50] = {
#include "sub_main/0047A2B0.inc"
};
char D_0047A300[0x40] = {
#include "sub_main/0047A300.inc"
};
char D_0047A340[0x50] = {
#include "sub_main/0047A340.inc"
};
char D_0047A390[0x30] = {
#include "sub_main/0047A390.inc"
};
char D_0047A3C0[0x40] = {
#include "sub_main/0047A3C0.inc"
};
char D_0047A400[0x40] = {
#include "sub_main/0047A400.inc"
};
char D_0047A440[0x20] = {
#include "sub_main/0047A440.inc"
};
char D_0047A460[0x20] = {
#include "sub_main/0047A460.inc"
};
char D_0047A480[0x20] = {
#include "sub_main/0047A480.inc"
};
char D_0047A4A0[0x20] = {
#include "sub_main/0047A4A0.inc"
};
char D_0047A4C0[0x20] = {
#include "sub_main/0047A4C0.inc"
};
char D_0047A4E0[0x30] = {
#include "sub_main/0047A4E0.inc"
};
char D_0047A510[0x20] = {
#include "sub_main/0047A510.inc"
};
char D_0047A530[0x20] = {
#include "sub_main/0047A530.inc"
};
char D_0047A550[0x20] = {
#include "sub_main/0047A550.inc"
};
char D_0047A570[0x20] = {
#include "sub_main/0047A570.inc"
};
char D_0047A590[0x20] = {
#include "sub_main/0047A590.inc"
};
char D_0047A5B0[0x20] = {
#include "sub_main/0047A5B0.inc"
};
char D_0047A5D0[0x20] = {
#include "sub_main/0047A5D0.inc"
};
char D_0047A5F0[0x20] = {
#include "sub_main/0047A5F0.inc"
};
char D_0047A610[0x20] = {
#include "sub_main/0047A610.inc"
};
char D_0047A630[0x30] = {
#include "sub_main/0047A630.inc"
};
char D_0047A660[0x20] = {
#include "sub_main/0047A660.inc"
};
char D_0047A680[0x30] = {
#include "sub_main/0047A680.inc"
};
char D_0047A6B0[0x20] = {
#include "sub_main/0047A6B0.inc"
};
char D_0047A6D0[0x30] = {
#include "sub_main/0047A6D0.inc"
};
char D_0047A700[0x30] = {
#include "sub_main/0047A700.inc"
};
char D_0047A730[0x30] = {
#include "sub_main/0047A730.inc"
};
char D_0047A760[0x30] = {
#include "sub_main/0047A760.inc"
};
char D_0047A790[0x30] = {
#include "sub_main/0047A790.inc"
};
char D_0047A7C0[0x20] = {
#include "sub_main/0047A7C0.inc"
};
char D_0047A7E0[0x30] = {
#include "sub_main/0047A7E0.inc"
};
char D_0047A810[0x20] = {
#include "sub_main/0047A810.inc"
};
char D_0047A830[0x20] = {
#include "sub_main/0047A830.inc"
};
char D_0047A850[0x20] = {
#include "sub_main/0047A850.inc"
};
char D_0047A870[0x30] = {
#include "sub_main/0047A870.inc"
};
char D_0047A8A0[0x30] = {
#include "sub_main/0047A8A0.inc"
};
char D_0047A8D0[0x30] = {
#include "sub_main/0047A8D0.inc"
};
char D_0047A900[0x30] = {
#include "sub_main/0047A900.inc"
};
char D_0047A930[0x30] = {
#include "sub_main/0047A930.inc"
};
char D_0047A960[0x40] = {
#include "sub_main/0047A960.inc"
};
char D_0047A9A0[0x40] = {
#include "sub_main/0047A9A0.inc"
};
char D_0047A9E0[0x40] = {
#include "sub_main/0047A9E0.inc"
};
char D_0047AA20[0x40] = {
#include "sub_main/0047AA20.inc"
};
char D_0047AA60[0x40] = {
#include "sub_main/0047AA60.inc"
};
char D_0047AAA0[0x40] = {
#include "sub_main/0047AAA0.inc"
};
char D_0047AAE0[0x40] = {
#include "sub_main/0047AAE0.inc"
};
char D_0047AB20[0x40] = {
#include "sub_main/0047AB20.inc"
};
char D_0047AB60[0x40] = {
#include "sub_main/0047AB60.inc"
};
char D_0047ABA0[0x40] = {
#include "sub_main/0047ABA0.inc"
};
char D_0047ABE0[0x40] = {
#include "sub_main/0047ABE0.inc"
};
char D_0047AC20[0x30] = {
#include "sub_main/0047AC20.inc"
};
char D_0047AC50[0x30] = {
#include "sub_main/0047AC50.inc"
};
char D_0047AC80[0x30] = {
#include "sub_main/0047AC80.inc"
};
char D_0047ACB0[0x30] = {
#include "sub_main/0047ACB0.inc"
};
char D_0047ACE0[0x20] = {
#include "sub_main/0047ACE0.inc"
};
char D_0047AD00[0x30] = {
#include "sub_main/0047AD00.inc"
};
char D_0047AD30[0x20] = {
#include "sub_main/0047AD30.inc"
};
char D_0047AD50[0x30] = {
#include "sub_main/0047AD50.inc"
};
char D_0047AD80[0x30] = {
#include "sub_main/0047AD80.inc"
};
char D_0047ADB0[0x30] = {
#include "sub_main/0047ADB0.inc"
};
char D_0047ADE0[0x30] = {
#include "sub_main/0047ADE0.inc"
};
char D_0047AE10[0x30] = {
#include "sub_main/0047AE10.inc"
};
char D_0047AE40[0x30] = {
#include "sub_main/0047AE40.inc"
};
char D_0047AE70[0x20] = {
#include "sub_main/0047AE70.inc"
};
char D_0047AE90[0x30] = {
#include "sub_main/0047AE90.inc"
};
char D_0047AEC0[0x30] = {
#include "sub_main/0047AEC0.inc"
};
char D_0047AEF0[0x30] = {
#include "sub_main/0047AEF0.inc"
};
char D_0047AF20[0x20] = {
#include "sub_main/0047AF20.inc"
};
char D_0047AF40[0x20] = {
#include "sub_main/0047AF40.inc"
};
char D_0047AF60[0x30] = {
#include "sub_main/0047AF60.inc"
};
char D_0047AF90[0x20] = {
#include "sub_main/0047AF90.inc"
};
char D_0047AFB0[0x20] = {
#include "sub_main/0047AFB0.inc"
};
char D_0047AFD0[0x30] = {
#include "sub_main/0047AFD0.inc"
};
char D_0047B000[0x20] = {
#include "sub_main/0047B000.inc"
};
char D_0047B020[0x20] = {
#include "sub_main/0047B020.inc"
};
char D_0047B040[0x30] = {
#include "sub_main/0047B040.inc"
};
char D_0047B070[0x30] = {
#include "sub_main/0047B070.inc"
};
char D_0047B0A0[0x30] = {
#include "sub_main/0047B0A0.inc"
};
char D_0047B0D0[0x20] = {
#include "sub_main/0047B0D0.inc"
};
char D_0047B0F0[0x20] = {
#include "sub_main/0047B0F0.inc"
};
char D_0047B110[0x20] = {
#include "sub_main/0047B110.inc"
};
char D_0047B130[0x20] = {
#include "sub_main/0047B130.inc"
};
char D_0047B150[0x20] = {
#include "sub_main/0047B150.inc"
};
char D_0047B170[0x20] = {
#include "sub_main/0047B170.inc"
};
char D_0047B190[0x20] = {
#include "sub_main/0047B190.inc"
};
char D_0047B1B0[0x20] = {
#include "sub_main/0047B1B0.inc"
};
char D_0047B1D0[0x20] = {
#include "sub_main/0047B1D0.inc"
};
char D_0047B1F0[0x20] = {
#include "sub_main/0047B1F0.inc"
};
char D_0047B210[0x20] = {
#include "sub_main/0047B210.inc"
};
char D_0047B230[0x10] = {
#include "sub_main/0047B230.inc"
};
char D_0047B240[0x20] = {
#include "sub_main/0047B240.inc"
};
char D_0047B260[0x20] = {
#include "sub_main/0047B260.inc"
};
char D_0047B280[0x20] = {
#include "sub_main/0047B280.inc"
};
char D_0047B2A0[0x20] = {
#include "sub_main/0047B2A0.inc"
};
char D_0047B2C0[0x20] = {
#include "sub_main/0047B2C0.inc"
};
char D_0047B2E0[0x20] = {
#include "sub_main/0047B2E0.inc"
};
char D_0047B300[0x20] = {
#include "sub_main/0047B300.inc"
};
char D_0047B320[0x20] = {
#include "sub_main/0047B320.inc"
};
char D_0047B340[0x20] = {
#include "sub_main/0047B340.inc"
};
char D_0047B360[0x20] = {
#include "sub_main/0047B360.inc"
};
char D_0047B380[0x20] = {
#include "sub_main/0047B380.inc"
};
char D_0047B3A0[0x20] = {
#include "sub_main/0047B3A0.inc"
};
char D_0047B3C0[0x20] = {
#include "sub_main/0047B3C0.inc"
};
char D_0047B3E0[0x20] = {
#include "sub_main/0047B3E0.inc"
};
char D_0047B400[0x20] = {
#include "sub_main/0047B400.inc"
};
char D_0047B420[0x20] = {
#include "sub_main/0047B420.inc"
};
char D_0047B440[0x10] = {
#include "sub_main/0047B440.inc"
};
char D_0047B450[0x20] = {
#include "sub_main/0047B450.inc"
};
char D_0047B470[0x20] = {
#include "sub_main/0047B470.inc"
};
char D_0047B490[0x20] = {
#include "sub_main/0047B490.inc"
};
char D_0047B4B0[0x20] = {
#include "sub_main/0047B4B0.inc"
};
char D_0047B4D0[0x20] = {
#include "sub_main/0047B4D0.inc"
};
char D_0047B4F0[0x20] = {
#include "sub_main/0047B4F0.inc"
};
char D_0047B510[0x20] = {
#include "sub_main/0047B510.inc"
};
char D_0047B530[0x20] = {
#include "sub_main/0047B530.inc"
};
char D_0047B550[0x20] = {
#include "sub_main/0047B550.inc"
};
char D_0047B570[0x20] = {
#include "sub_main/0047B570.inc"
};
char D_0047B590[0x20] = {
#include "sub_main/0047B590.inc"
};
char D_0047B5B0[0x20] = {
#include "sub_main/0047B5B0.inc"
};
char D_0047B5D0[0x20] = {
#include "sub_main/0047B5D0.inc"
};
char D_0047B5F0[0x20] = {
#include "sub_main/0047B5F0.inc"
};
char D_0047B610[0x20] = {
#include "sub_main/0047B610.inc"
};
char D_0047B630[0x20] = {
#include "sub_main/0047B630.inc"
};
char D_0047B650[0x20] = {
#include "sub_main/0047B650.inc"
};
char D_0047B670[0x20] = {
#include "sub_main/0047B670.inc"
};
char D_0047B690[0x20] = {
#include "sub_main/0047B690.inc"
};
char D_0047B6B0[0x10] = {
#include "sub_main/0047B6B0.inc"
};
char D_0047B6C0[0x20] = {
#include "sub_main/0047B6C0.inc"
};
char D_0047B6E0[0x20] = {
#include "sub_main/0047B6E0.inc"
};
char D_0047B700[0x20] = {
#include "sub_main/0047B700.inc"
};
char D_0047B720[0x20] = {
#include "sub_main/0047B720.inc"
};
char D_0047B740[0x20] = {
#include "sub_main/0047B740.inc"
};
char D_0047B760[0x20] = {
#include "sub_main/0047B760.inc"
};
char D_0047B780[0x20] = {
#include "sub_main/0047B780.inc"
};
char D_0047B7A0[0x20] = {
#include "sub_main/0047B7A0.inc"
};
char D_0047B7C0[0x20] = {
#include "sub_main/0047B7C0.inc"
};
char D_0047B7E0[0x20] = {
#include "sub_main/0047B7E0.inc"
};
char D_0047B800[0x20] = {
#include "sub_main/0047B800.inc"
};
char D_0047B820[0x20] = {
#include "sub_main/0047B820.inc"
};
char D_0047B840[0x20] = {
#include "sub_main/0047B840.inc"
};
char D_0047B860[0x20] = {
#include "sub_main/0047B860.inc"
};
char D_0047B880[0x20] = {
#include "sub_main/0047B880.inc"
};
char D_0047B8A0[0x20] = {
#include "sub_main/0047B8A0.inc"
};
char D_0047B8C0[0x20] = {
#include "sub_main/0047B8C0.inc"
};
char D_0047B8E0[0x20] = {
#include "sub_main/0047B8E0.inc"
};
char D_0047B900[0x20] = {
#include "sub_main/0047B900.inc"
};
char D_0047B920[0x20] = {
#include "sub_main/0047B920.inc"
};
char D_0047B940[0x20] = {
#include "sub_main/0047B940.inc"
};
char D_0047B960[0x20] = {
#include "sub_main/0047B960.inc"
};
char D_0047B980[0x20] = {
#include "sub_main/0047B980.inc"
};
char D_0047B9A0[0x20] = {
#include "sub_main/0047B9A0.inc"
};
char D_0047B9C0[0x20] = {
#include "sub_main/0047B9C0.inc"
};
char D_0047B9E0[0x20] = {
#include "sub_main/0047B9E0.inc"
};
char D_0047BA00[0x20] = {
#include "sub_main/0047BA00.inc"
};
char D_0047BA20[0x20] = {
#include "sub_main/0047BA20.inc"
};
char D_0047BA40[0x20] = {
#include "sub_main/0047BA40.inc"
};
char D_0047BA60[0x20] = {
#include "sub_main/0047BA60.inc"
};
char D_0047BA80[0x20] = {
#include "sub_main/0047BA80.inc"
};
char D_0047BAA0[0x20] = {
#include "sub_main/0047BAA0.inc"
};
char D_0047BAC0[0x20] = {
#include "sub_main/0047BAC0.inc"
};
char D_0047BAE0[0x20] = {
#include "sub_main/0047BAE0.inc"
};
char D_0047BB00[0x20] = {
#include "sub_main/0047BB00.inc"
};
char D_0047BB20[0x20] = {
#include "sub_main/0047BB20.inc"
};
char D_0047BB40[0x20] = {
#include "sub_main/0047BB40.inc"
};
char D_0047BB60[0x20] = {
#include "sub_main/0047BB60.inc"
};
char D_0047BB80[0x20] = {
#include "sub_main/0047BB80.inc"
};
char D_0047BBA0[0x20] = {
#include "sub_main/0047BBA0.inc"
};
char D_0047BBC0[0x20] = {
#include "sub_main/0047BBC0.inc"
};
char D_0047BBE0[0x20] = {
#include "sub_main/0047BBE0.inc"
};
char D_0047BC00[0x20] = {
#include "sub_main/0047BC00.inc"
};
char D_0047BC20[0x20] = {
#include "sub_main/0047BC20.inc"
};
char D_0047BC40[0x20] = {
#include "sub_main/0047BC40.inc"
};
char D_0047BC60[0x20] = {
#include "sub_main/0047BC60.inc"
};
char D_0047BC80[0x20] = {
#include "sub_main/0047BC80.inc"
};
char D_0047BCA0[0x20] = {
#include "sub_main/0047BCA0.inc"
};
char D_0047BCC0[0x20] = {
#include "sub_main/0047BCC0.inc"
};
char D_0047BCE0[0x20] = {
#include "sub_main/0047BCE0.inc"
};
char D_0047BD00[0x20] = {
#include "sub_main/0047BD00.inc"
};
char D_0047BD20[0x20] = {
#include "sub_main/0047BD20.inc"
};
char D_0047BD40[0x20] = {
#include "sub_main/0047BD40.inc"
};
char D_0047BD60[0x20] = {
#include "sub_main/0047BD60.inc"
};
char D_0047BD80[0x20] = {
#include "sub_main/0047BD80.inc"
};
char D_0047BDA0[0x20] = {
#include "sub_main/0047BDA0.inc"
};
char D_0047BDC0[0x20] = {
#include "sub_main/0047BDC0.inc"
};
char D_0047BDE0[0x20] = {
#include "sub_main/0047BDE0.inc"
};
char D_0047BE00[0x20] = {
#include "sub_main/0047BE00.inc"
};
char D_0047BE20[0x20] = {
#include "sub_main/0047BE20.inc"
};
char D_0047BE40[0x20] = {
#include "sub_main/0047BE40.inc"
};
char D_0047BE60[0x20] = {
#include "sub_main/0047BE60.inc"
};
char D_0047BE80[0x20] = {
#include "sub_main/0047BE80.inc"
};
char D_0047BEA0[0x20] = {
#include "sub_main/0047BEA0.inc"
};
char D_0047BEC0[0x20] = {
#include "sub_main/0047BEC0.inc"
};
char D_0047BEE0[0x20] = {
#include "sub_main/0047BEE0.inc"
};
char D_0047BF00[0x20] = {
#include "sub_main/0047BF00.inc"
};
char D_0047BF20[0x20] = {
#include "sub_main/0047BF20.inc"
};
char D_0047BF40[0x20] = {
#include "sub_main/0047BF40.inc"
};
char D_0047BF60[0x20] = {
#include "sub_main/0047BF60.inc"
};
char D_0047BF80[0x20] = {
#include "sub_main/0047BF80.inc"
};
char D_0047BFA0[0x20] = {
#include "sub_main/0047BFA0.inc"
};
char D_0047BFC0[0x20] = {
#include "sub_main/0047BFC0.inc"
};
char D_0047BFE0[0x20] = {
#include "sub_main/0047BFE0.inc"
};
char D_0047C000[0x20] = {
#include "sub_main/0047C000.inc"
};
char D_0047C020[0x20] = {
#include "sub_main/0047C020.inc"
};
char D_0047C040[0x20] = {
#include "sub_main/0047C040.inc"
};
char D_0047C060[0x20] = {
#include "sub_main/0047C060.inc"
};
char D_0047C080[0x20] = {
#include "sub_main/0047C080.inc"
};
char D_0047C0A0[0x20] = {
#include "sub_main/0047C0A0.inc"
};
char D_0047C0C0[0x20] = {
#include "sub_main/0047C0C0.inc"
};
char D_0047C0E0[0x20] = {
#include "sub_main/0047C0E0.inc"
};
char D_0047C100[0x20] = {
#include "sub_main/0047C100.inc"
};
char D_0047C120[0x20] = {
#include "sub_main/0047C120.inc"
};
char D_0047C140[0x20] = {
#include "sub_main/0047C140.inc"
};
char D_0047C160[0x20] = {
#include "sub_main/0047C160.inc"
};
char D_0047C180[0x20] = {
#include "sub_main/0047C180.inc"
};
char D_0047C1A0[0x20] = {
#include "sub_main/0047C1A0.inc"
};
char D_0047C1C0[0x20] = {
#include "sub_main/0047C1C0.inc"
};
char D_0047C1E0[0x20] = {
#include "sub_main/0047C1E0.inc"
};
char D_0047C200[0x20] = {
#include "sub_main/0047C200.inc"
};
char D_0047C220[0x20] = {
#include "sub_main/0047C220.inc"
};
char D_0047C240[0x20] = {
#include "sub_main/0047C240.inc"
};
char D_0047C260[0x20] = {
#include "sub_main/0047C260.inc"
};
char D_0047C280[0x20] = {
#include "sub_main/0047C280.inc"
};
char D_0047C2A0[0x20] = {
#include "sub_main/0047C2A0.inc"
};
char D_0047C2C0[0x20] = {
#include "sub_main/0047C2C0.inc"
};
char D_0047C2E0[0x20] = {
#include "sub_main/0047C2E0.inc"
};
char D_0047C300[0x20] = {
#include "sub_main/0047C300.inc"
};
char D_0047C320[0x20] = {
#include "sub_main/0047C320.inc"
};
char D_0047C340[0x20] = {
#include "sub_main/0047C340.inc"
};
char D_0047C360[0x20] = {
#include "sub_main/0047C360.inc"
};
char D_0047C380[0x20] = {
#include "sub_main/0047C380.inc"
};
char D_0047C3A0[0x20] = {
#include "sub_main/0047C3A0.inc"
};
char D_0047C3C0[0x20] = {
#include "sub_main/0047C3C0.inc"
};
char D_0047C3E0[0x20] = {
#include "sub_main/0047C3E0.inc"
};
char D_0047C400[0x20] = {
#include "sub_main/0047C400.inc"
};
char D_0047C420[0x20] = {
#include "sub_main/0047C420.inc"
};
char D_0047C440[0x20] = {
#include "sub_main/0047C440.inc"
};
char D_0047C460[0x20] = {
#include "sub_main/0047C460.inc"
};
char D_0047C480[0x20] = {
#include "sub_main/0047C480.inc"
};
char D_0047C4A0[0x20] = {
#include "sub_main/0047C4A0.inc"
};
char D_0047C4C0[0x20] = {
#include "sub_main/0047C4C0.inc"
};
char D_0047C4E0[0x20] = {
#include "sub_main/0047C4E0.inc"
};
char D_0047C500[0x20] = {
#include "sub_main/0047C500.inc"
};
char D_0047C520[0x20] = {
#include "sub_main/0047C520.inc"
};
char D_0047C540[0x20] = {
#include "sub_main/0047C540.inc"
};
char D_0047C560[0x20] = {
#include "sub_main/0047C560.inc"
};
char D_0047C580[0x20] = {
#include "sub_main/0047C580.inc"
};
char D_0047C5A0[0x20] = {
#include "sub_main/0047C5A0.inc"
};
char D_0047C5C0[0x20] = {
#include "sub_main/0047C5C0.inc"
};
char D_0047C5E0[0x20] = {
#include "sub_main/0047C5E0.inc"
};
char D_0047C600[0x20] = {
#include "sub_main/0047C600.inc"
};
char D_0047C620[0x20] = {
#include "sub_main/0047C620.inc"
};
char D_0047C640[0x20] = {
#include "sub_main/0047C640.inc"
};
char D_0047C660[0x20] = {
#include "sub_main/0047C660.inc"
};
char D_0047C680[0x20] = {
#include "sub_main/0047C680.inc"
};
char D_0047C6A0[0x20] = {
#include "sub_main/0047C6A0.inc"
};
char D_0047C6C0[0x20] = {
#include "sub_main/0047C6C0.inc"
};
char D_0047C6E0[0x20] = {
#include "sub_main/0047C6E0.inc"
};
char D_0047C700[0x20] = {
#include "sub_main/0047C700.inc"
};
char D_0047C720[0x20] = {
#include "sub_main/0047C720.inc"
};
char D_0047C740[0x20] = {
#include "sub_main/0047C740.inc"
};
char D_0047C760[0x20] = {
#include "sub_main/0047C760.inc"
};
char D_0047C780[0x20] = {
#include "sub_main/0047C780.inc"
};
char D_0047C7A0[0x20] = {
#include "sub_main/0047C7A0.inc"
};
char D_0047C7C0[0x20] = {
#include "sub_main/0047C7C0.inc"
};
char D_0047C7E0[0x20] = {
#include "sub_main/0047C7E0.inc"
};
char D_0047C800[0x20] = {
#include "sub_main/0047C800.inc"
};
char D_0047C820[0x20] = {
#include "sub_main/0047C820.inc"
};
char D_0047C840[0x20] = {
#include "sub_main/0047C840.inc"
};
char D_0047C860[0x10] = {
#include "sub_main/0047C860.inc"
};
char D_0047C870[0x20] = {
#include "sub_main/0047C870.inc"
};
char D_0047C890[0x20] = {
#include "sub_main/0047C890.inc"
};
char D_0047C8B0[0x20] = {
#include "sub_main/0047C8B0.inc"
};
char D_0047C8D0[0x20] = {
#include "sub_main/0047C8D0.inc"
};
char D_0047C8F0[0x20] = {
#include "sub_main/0047C8F0.inc"
};
char D_0047C910[0x20] = {
#include "sub_main/0047C910.inc"
};
char D_0047C930[0x20] = {
#include "sub_main/0047C930.inc"
};
char D_0047C950[0x20] = {
#include "sub_main/0047C950.inc"
};
char D_0047C970[0x20] = {
#include "sub_main/0047C970.inc"
};
char D_0047C990[0x20] = {
#include "sub_main/0047C990.inc"
};
char D_0047C9B0[0x20] = {
#include "sub_main/0047C9B0.inc"
};
char D_0047C9D0[0x20] = {
#include "sub_main/0047C9D0.inc"
};
char D_0047C9F0[0x20] = {
#include "sub_main/0047C9F0.inc"
};
char D_0047CA10[0x20] = {
#include "sub_main/0047CA10.inc"
};
char D_0047CA30[0x20] = {
#include "sub_main/0047CA30.inc"
};
char D_0047CA50[0x20] = {
#include "sub_main/0047CA50.inc"
};
char D_0047CA70[0x20] = {
#include "sub_main/0047CA70.inc"
};
char D_0047CA90[0x30] = {
#include "sub_main/0047CA90.inc"
};
char D_0047CAC0[0x20] = {
#include "sub_main/0047CAC0.inc"
};
char D_0047CAE0[0x20] = {
#include "sub_main/0047CAE0.inc"
};
char D_0047CB00[0x20] = {
#include "sub_main/0047CB00.inc"
};
char D_0047CB20[0x20] = {
#include "sub_main/0047CB20.inc"
};
char D_0047CB40[0x20] = {
#include "sub_main/0047CB40.inc"
};
char D_0047CB60[0x20] = {
#include "sub_main/0047CB60.inc"
};
char D_0047CB80[0x20] = {
#include "sub_main/0047CB80.inc"
};
char D_0047CBA0[0x20] = {
#include "sub_main/0047CBA0.inc"
};
char D_0047CBC0[0x30] = {
#include "sub_main/0047CBC0.inc"
};
char D_0047CBF0[0x20] = {
#include "sub_main/0047CBF0.inc"
};
char D_0047CC10[0x20] = {
#include "sub_main/0047CC10.inc"
};
char D_0047CC30[0x20] = {
#include "sub_main/0047CC30.inc"
};
char D_0047CC50[0x20] = {
#include "sub_main/0047CC50.inc"
};
char D_0047CC70[0x20] = {
#include "sub_main/0047CC70.inc"
};
char D_0047CC90[0x20] = {
#include "sub_main/0047CC90.inc"
};
char D_0047CCB0[0x20] = {
#include "sub_main/0047CCB0.inc"
};
char D_0047CCD0[0x20] = {
#include "sub_main/0047CCD0.inc"
};
char D_0047CCF0[0x20] = {
#include "sub_main/0047CCF0.inc"
};
char D_0047CD10[0x20] = {
#include "sub_main/0047CD10.inc"
};
char D_0047CD30[0x20] = {
#include "sub_main/0047CD30.inc"
};
char D_0047CD50[0x20] = {
#include "sub_main/0047CD50.inc"
};
char D_0047CD70[0x20] = {
#include "sub_main/0047CD70.inc"
};
char D_0047CD90[0x20] = {
#include "sub_main/0047CD90.inc"
};
char D_0047CDB0[0x20] = {
#include "sub_main/0047CDB0.inc"
};
char D_0047CDD0[0x20] = {
#include "sub_main/0047CDD0.inc"
};
char D_0047CDF0[0x20] = {
#include "sub_main/0047CDF0.inc"
};
char D_0047CE10[0x20] = {
#include "sub_main/0047CE10.inc"
};
char D_0047CE30[0x20] = {
#include "sub_main/0047CE30.inc"
};
char D_0047CE50[0x20] = {
#include "sub_main/0047CE50.inc"
};
char D_0047CE70[0x30] = {
#include "sub_main/0047CE70.inc"
};
char D_0047CEA0[0x20] = {
#include "sub_main/0047CEA0.inc"
};
char D_0047CEC0[0x20] = {
#include "sub_main/0047CEC0.inc"
};
char D_0047CEE0[0x20] = {
#include "sub_main/0047CEE0.inc"
};
char D_0047CF00[0x20] = {
#include "sub_main/0047CF00.inc"
};
char D_0047CF20[0x20] = {
#include "sub_main/0047CF20.inc"
};
char D_0047CF40[0x30] = {
#include "sub_main/0047CF40.inc"
};
char D_0047CF70[0x20] = {
#include "sub_main/0047CF70.inc"
};
char D_0047CF90[0x20] = {
#include "sub_main/0047CF90.inc"
};
char D_0047CFB0[0x20] = {
#include "sub_main/0047CFB0.inc"
};
char D_0047CFD0[0x20] = {
#include "sub_main/0047CFD0.inc"
};
char D_0047CFF0[0x20] = {
#include "sub_main/0047CFF0.inc"
};
char D_0047D010[0x20] = {
#include "sub_main/0047D010.inc"
};
char D_0047D030[0x20] = {
#include "sub_main/0047D030.inc"
};
char D_0047D050[0x20] = {
#include "sub_main/0047D050.inc"
};
char D_0047D070[0x20] = {
#include "sub_main/0047D070.inc"
};
char D_0047D090[0x20] = {
#include "sub_main/0047D090.inc"
};
char D_0047D0B0[0x20] = {
#include "sub_main/0047D0B0.inc"
};
char D_0047D0D0[0x20] = {
#include "sub_main/0047D0D0.inc"
};
char D_0047D0F0[0x20] = {
#include "sub_main/0047D0F0.inc"
};
char D_0047D110[0x20] = {
#include "sub_main/0047D110.inc"
};
char D_0047D130[0x20] = {
#include "sub_main/0047D130.inc"
};
char D_0047D150[0x20] = {
#include "sub_main/0047D150.inc"
};
char D_0047D170[0x20] = {
#include "sub_main/0047D170.inc"
};
char D_0047D190[0x30] = {
#include "sub_main/0047D190.inc"
};
char D_0047D1C0[0x20] = {
#include "sub_main/0047D1C0.inc"
};
char D_0047D1E0[0x30] = {
#include "sub_main/0047D1E0.inc"
};
char D_0047D210[0x20] = {
#include "sub_main/0047D210.inc"
};
char D_0047D230[0x20] = {
#include "sub_main/0047D230.inc"
};
char D_0047D250[0x20] = {
#include "sub_main/0047D250.inc"
};
char D_0047D270[0x20] = {
#include "sub_main/0047D270.inc"
};
char D_0047D290[0x20] = {
#include "sub_main/0047D290.inc"
};
char D_0047D2B0[0x20] = {
#include "sub_main/0047D2B0.inc"
};
char D_0047D2D0[0x20] = {
#include "sub_main/0047D2D0.inc"
};
char D_0047D2F0[0x20] = {
#include "sub_main/0047D2F0.inc"
};
char D_0047D310[0x20] = {
#include "sub_main/0047D310.inc"
};
char D_0047D330[0x20] = {
#include "sub_main/0047D330.inc"
};
char D_0047D350[0x20] = {
#include "sub_main/0047D350.inc"
};
char D_0047D370[0x20] = {
#include "sub_main/0047D370.inc"
};
char D_0047D390[0x20] = {
#include "sub_main/0047D390.inc"
};
char D_0047D3B0[0x20] = {
#include "sub_main/0047D3B0.inc"
};
char D_0047D3D0[0x20] = {
#include "sub_main/0047D3D0.inc"
};
char D_0047D3F0[0x20] = {
#include "sub_main/0047D3F0.inc"
};
char D_0047D410[0x20] = {
#include "sub_main/0047D410.inc"
};
char D_0047D430[0x20] = {
#include "sub_main/0047D430.inc"
};
char D_0047D450[0x20] = {
#include "sub_main/0047D450.inc"
};
char D_0047D470[0x20] = {
#include "sub_main/0047D470.inc"
};
char D_0047D490[0x20] = {
#include "sub_main/0047D490.inc"
};
char D_0047D4B0[0x20] = {
#include "sub_main/0047D4B0.inc"
};
char D_0047D4D0[0x20] = {
#include "sub_main/0047D4D0.inc"
};
char D_0047D4F0[0x20] = {
#include "sub_main/0047D4F0.inc"
};
char D_0047D510[0x20] = {
#include "sub_main/0047D510.inc"
};
char D_0047D530[0x20] = {
#include "sub_main/0047D530.inc"
};
char D_0047D550[0x20] = {
#include "sub_main/0047D550.inc"
};
char D_0047D570[0x20] = {
#include "sub_main/0047D570.inc"
};
char D_0047D590[0x20] = {
#include "sub_main/0047D590.inc"
};
char D_0047D5B0[0x20] = {
#include "sub_main/0047D5B0.inc"
};
char D_0047D5D0[0x20] = {
#include "sub_main/0047D5D0.inc"
};
char D_0047D5F0[0x20] = {
#include "sub_main/0047D5F0.inc"
};
char D_0047D610[0x20] = {
#include "sub_main/0047D610.inc"
};
char D_0047D630[0x20] = {
#include "sub_main/0047D630.inc"
};
char D_0047D650[0x20] = {
#include "sub_main/0047D650.inc"
};
char D_0047D670[0x20] = {
#include "sub_main/0047D670.inc"
};
char D_0047D690[0x20] = {
#include "sub_main/0047D690.inc"
};
char D_0047D6B0[0x20] = {
#include "sub_main/0047D6B0.inc"
};
char D_0047D6D0[0x20] = {
#include "sub_main/0047D6D0.inc"
};
char D_0047D6F0[0x20] = {
#include "sub_main/0047D6F0.inc"
};
char D_0047D710[0x20] = {
#include "sub_main/0047D710.inc"
};
char D_0047D730[0x20] = {
#include "sub_main/0047D730.inc"
};
char D_0047D750[0x20] = {
#include "sub_main/0047D750.inc"
};
char D_0047D770[0x20] = {
#include "sub_main/0047D770.inc"
};
char D_0047D790[0x20] = {
#include "sub_main/0047D790.inc"
};
char D_0047D7B0[0x20] = {
#include "sub_main/0047D7B0.inc"
};
char D_0047D7D0[0x20] = {
#include "sub_main/0047D7D0.inc"
};
char D_0047D7F0[0x20] = {
#include "sub_main/0047D7F0.inc"
};
char D_0047D810[0x20] = {
#include "sub_main/0047D810.inc"
};
char D_0047D830[0x20] = {
#include "sub_main/0047D830.inc"
};
char D_0047D850[0x20] = {
#include "sub_main/0047D850.inc"
};
char D_0047D870[0x20] = {
#include "sub_main/0047D870.inc"
};
char D_0047D890[0x20] = {
#include "sub_main/0047D890.inc"
};
char D_0047D8B0[0x20] = {
#include "sub_main/0047D8B0.inc"
};
char D_0047D8D0[0x20] = {
#include "sub_main/0047D8D0.inc"
};
char D_0047D8F0[0x20] = {
#include "sub_main/0047D8F0.inc"
};
char D_0047D910[0x20] = {
#include "sub_main/0047D910.inc"
};
char D_0047D930[0x20] = {
#include "sub_main/0047D930.inc"
};
char D_0047D950[0x20] = {
#include "sub_main/0047D950.inc"
};
char D_0047D970[0x20] = {
#include "sub_main/0047D970.inc"
};
char D_0047D990[0x20] = {
#include "sub_main/0047D990.inc"
};
char D_0047D9B0[0x20] = {
#include "sub_main/0047D9B0.inc"
};
char D_0047D9D0[0x20] = {
#include "sub_main/0047D9D0.inc"
};
char D_0047D9F0[0x20] = {
#include "sub_main/0047D9F0.inc"
};
char D_0047DA10[0x20] = {
#include "sub_main/0047DA10.inc"
};
char D_0047DA30[0x20] = {
#include "sub_main/0047DA30.inc"
};
char D_0047DA50[0x20] = {
#include "sub_main/0047DA50.inc"
};
char D_0047DA70[0x20] = {
#include "sub_main/0047DA70.inc"
};
char D_0047DA90[0x20] = {
#include "sub_main/0047DA90.inc"
};
char D_0047DAB0[0x20] = {
#include "sub_main/0047DAB0.inc"
};
char D_0047DAD0[0x20] = {
#include "sub_main/0047DAD0.inc"
};
char D_0047DAF0[0x20] = {
#include "sub_main/0047DAF0.inc"
};
char D_0047DB10[0x20] = {
#include "sub_main/0047DB10.inc"
};
char D_0047DB30[0x20] = {
#include "sub_main/0047DB30.inc"
};
char D_0047DB50[0x20] = {
#include "sub_main/0047DB50.inc"
};
char D_0047DB70[0x20] = {
#include "sub_main/0047DB70.inc"
};
char D_0047DB90[0x20] = {
#include "sub_main/0047DB90.inc"
};
char D_0047DBB0[0x20] = {
#include "sub_main/0047DBB0.inc"
};
char D_0047DBD0[0x20] = {
#include "sub_main/0047DBD0.inc"
};
char D_0047DBF0[0x20] = {
#include "sub_main/0047DBF0.inc"
};
char D_0047DC10[0x20] = {
#include "sub_main/0047DC10.inc"
};
char D_0047DC30[0x20] = {
#include "sub_main/0047DC30.inc"
};
char D_0047DC50[0x20] = {
#include "sub_main/0047DC50.inc"
};
char D_0047DC70[0x20] = {
#include "sub_main/0047DC70.inc"
};
char D_0047DC90[0x20] = {
#include "sub_main/0047DC90.inc"
};
char D_0047DCB0[0x20] = {
#include "sub_main/0047DCB0.inc"
};
u32 D_0047DCD0[8] = {
#include "sub_main/0047DCD0.inc"
};
char D_0047DCF0[0x20] = {
#include "sub_main/0047DCF0.inc"
};
char D_0047DD10[0x20] = {
#include "sub_main/0047DD10.inc"
};
char D_0047DD30[0x20] = {
#include "sub_main/0047DD30.inc"
};
char D_0047DD50[0x20] = {
#include "sub_main/0047DD50.inc"
};
char D_0047DD70[0x20] = {
#include "sub_main/0047DD70.inc"
};
char D_0047DD90[0x20] = {
#include "sub_main/0047DD90.inc"
};
char D_0047DDB0[0x20] = {
#include "sub_main/0047DDB0.inc"
};
char D_0047DDD0[0x20] = {
#include "sub_main/0047DDD0.inc"
};
char D_0047DDF0[0x20] = {
#include "sub_main/0047DDF0.inc"
};
char D_0047DE10[0x20] = {
#include "sub_main/0047DE10.inc"
};
char D_0047DE30[0x20] = {
#include "sub_main/0047DE30.inc"
};
char D_0047DE50[0x20] = {
#include "sub_main/0047DE50.inc"
};
char D_0047DE70[0x20] = {
#include "sub_main/0047DE70.inc"
};
char D_0047DE90[0x20] = {
#include "sub_main/0047DE90.inc"
};
char D_0047DEB0[0x20] = {
#include "sub_main/0047DEB0.inc"
};
char D_0047DED0[0x20] = {
#include "sub_main/0047DED0.inc"
};
char D_0047DEF0[0x20] = {
#include "sub_main/0047DEF0.inc"
};
char D_0047DF10[0x20] = {
#include "sub_main/0047DF10.inc"
};
char D_0047DF30[0x20] = {
#include "sub_main/0047DF30.inc"
};
char D_0047DF50[0x20] = {
#include "sub_main/0047DF50.inc"
};
char D_0047DF70[0x20] = {
#include "sub_main/0047DF70.inc"
};
char D_0047DF90[0x20] = {
#include "sub_main/0047DF90.inc"
};
char D_0047DFB0[0x20] = {
#include "sub_main/0047DFB0.inc"
};
char D_0047DFD0[0x20] = {
#include "sub_main/0047DFD0.inc"
};
char D_0047DFF0[0x20] = {
#include "sub_main/0047DFF0.inc"
};
char D_0047E010[0x20] = {
#include "sub_main/0047E010.inc"
};
char D_0047E030[0x20] = {
#include "sub_main/0047E030.inc"
};
char D_0047E050[0x20] = {
#include "sub_main/0047E050.inc"
};
char D_0047E070[0x20] = {
#include "sub_main/0047E070.inc"
};
char D_0047E090[0x20] = {
#include "sub_main/0047E090.inc"
};
char D_0047E0B0[0x20] = {
#include "sub_main/0047E0B0.inc"
};
char D_0047E0D0[0x20] = {
#include "sub_main/0047E0D0.inc"
};
char D_0047E0F0[0x20] = {
#include "sub_main/0047E0F0.inc"
};
char D_0047E110[0x20] = {
#include "sub_main/0047E110.inc"
};
char D_0047E130[0x20] = {
#include "sub_main/0047E130.inc"
};
char D_0047E150[0x20] = {
#include "sub_main/0047E150.inc"
};
char D_0047E170[0x20] = {
#include "sub_main/0047E170.inc"
};
char D_0047E190[0x20] = {
#include "sub_main/0047E190.inc"
};
char D_0047E1B0[0x20] = {
#include "sub_main/0047E1B0.inc"
};
char D_0047E1D0[0x20] = {
#include "sub_main/0047E1D0.inc"
};
char D_0047E1F0[0x20] = {
#include "sub_main/0047E1F0.inc"
};
char D_0047E210[0x20] = {
#include "sub_main/0047E210.inc"
};
char D_0047E230[0x20] = {
#include "sub_main/0047E230.inc"
};
char D_0047E250[0x20] = {
#include "sub_main/0047E250.inc"
};
char D_0047E270[0x20] = {
#include "sub_main/0047E270.inc"
};
char D_0047E290[0x20] = {
#include "sub_main/0047E290.inc"
};
char D_0047E2B0[0x20] = {
#include "sub_main/0047E2B0.inc"
};
char D_0047E2D0[0x20] = {
#include "sub_main/0047E2D0.inc"
};
char D_0047E2F0[0x20] = {
#include "sub_main/0047E2F0.inc"
};
char D_0047E310[0x10] = {
#include "sub_main/0047E310.inc"
};
char D_0047E320[0x20] = {
#include "sub_main/0047E320.inc"
};
char D_0047E340[0x20] = {
#include "sub_main/0047E340.inc"
};
char D_0047E360[0x20] = {
#include "sub_main/0047E360.inc"
};
char D_0047E380[0x20] = {
#include "sub_main/0047E380.inc"
};
char D_0047E3A0[0x20] = {
#include "sub_main/0047E3A0.inc"
};
char D_0047E3C0[0x20] = {
#include "sub_main/0047E3C0.inc"
};
char D_0047E3E0[0x20] = {
#include "sub_main/0047E3E0.inc"
};
char D_0047E400[0x20] = {
#include "sub_main/0047E400.inc"
};
char D_0047E420[0x20] = {
#include "sub_main/0047E420.inc"
};
char D_0047E440[0x20] = {
#include "sub_main/0047E440.inc"
};
char D_0047E460[0x20] = {
#include "sub_main/0047E460.inc"
};
char D_0047E480[0x20] = {
#include "sub_main/0047E480.inc"
};
char D_0047E4A0[0x20] = {
#include "sub_main/0047E4A0.inc"
};
char D_0047E4C0[0x20] = {
#include "sub_main/0047E4C0.inc"
};
char D_0047E4E0[0x20] = {
#include "sub_main/0047E4E0.inc"
};
char D_0047E500[0x20] = {
#include "sub_main/0047E500.inc"
};
char D_0047E520[0x20] = {
#include "sub_main/0047E520.inc"
};
char D_0047E540[0x20] = {
#include "sub_main/0047E540.inc"
};
char D_0047E560[0x20] = {
#include "sub_main/0047E560.inc"
};
char D_0047E580[0x20] = {
#include "sub_main/0047E580.inc"
};
char D_0047E5A0[0x20] = {
#include "sub_main/0047E5A0.inc"
};
char D_0047E5C0[0x20] = {
#include "sub_main/0047E5C0.inc"
};
char D_0047E5E0[0x20] = {
#include "sub_main/0047E5E0.inc"
};
char D_0047E600[0x20] = {
#include "sub_main/0047E600.inc"
};
char D_0047E620[0x20] = {
#include "sub_main/0047E620.inc"
};
char D_0047E640[0x20] = {
#include "sub_main/0047E640.inc"
};
char D_0047E660[0x10] = {
#include "sub_main/0047E660.inc"
};
char D_0047E670[0x20] = {
#include "sub_main/0047E670.inc"
};
char D_0047E690[0x20] = {
#include "sub_main/0047E690.inc"
};
char D_0047E6B0[0x20] = {
#include "sub_main/0047E6B0.inc"
};
char D_0047E6D0[0x20] = {
#include "sub_main/0047E6D0.inc"
};
char D_0047E6F0[0x20] = {
#include "sub_main/0047E6F0.inc"
};
char D_0047E710[0x20] = {
#include "sub_main/0047E710.inc"
};
char D_0047E730[0x20] = {
#include "sub_main/0047E730.inc"
};
char D_0047E750[0x20] = {
#include "sub_main/0047E750.inc"
};
char D_0047E770[0x20] = {
#include "sub_main/0047E770.inc"
};
char D_0047E790[0x20] = {
#include "sub_main/0047E790.inc"
};
char D_0047E7B0[0x20] = {
#include "sub_main/0047E7B0.inc"
};
char D_0047E7D0[0x10] = {
#include "sub_main/0047E7D0.inc"
};
char D_0047E7E0[0x20] = {
#include "sub_main/0047E7E0.inc"
};
char D_0047E800[0x20] = {
#include "sub_main/0047E800.inc"
};
char D_0047E820[0x20] = {
#include "sub_main/0047E820.inc"
};
char D_0047E840[0x20] = {
#include "sub_main/0047E840.inc"
};
char D_0047E860[0x20] = {
#include "sub_main/0047E860.inc"
};
char D_0047E880[0x20] = {
#include "sub_main/0047E880.inc"
};
char D_0047E8A0[0x20] = {
#include "sub_main/0047E8A0.inc"
};
char D_0047E8C0[0x20] = {
#include "sub_main/0047E8C0.inc"
};
char D_0047E8E0[0x20] = {
#include "sub_main/0047E8E0.inc"
};
char D_0047E900[0x20] = {
#include "sub_main/0047E900.inc"
};
char D_0047E920[0x20] = {
#include "sub_main/0047E920.inc"
};
char D_0047E940[0x20] = {
#include "sub_main/0047E940.inc"
};
char D_0047E960[0x20] = {
#include "sub_main/0047E960.inc"
};
char D_0047E980[0x20] = {
#include "sub_main/0047E980.inc"
};
char D_0047E9A0[0x20] = {
#include "sub_main/0047E9A0.inc"
};
char D_0047E9C0[0x20] = {
#include "sub_main/0047E9C0.inc"
};
char D_0047E9E0[0x20] = {
#include "sub_main/0047E9E0.inc"
};
char D_0047EA00[0x20] = {
#include "sub_main/0047EA00.inc"
};
char D_0047EA20[0x20] = {
#include "sub_main/0047EA20.inc"
};
char D_0047EA40[0x20] = {
#include "sub_main/0047EA40.inc"
};
char D_0047EA60[0x20] = {
#include "sub_main/0047EA60.inc"
};
char D_0047EA80[0x20] = {
#include "sub_main/0047EA80.inc"
};
char D_0047EAA0[0x20] = {
#include "sub_main/0047EAA0.inc"
};
char D_0047EAC0[0x20] = {
#include "sub_main/0047EAC0.inc"
};
char D_0047EAE0[0x20] = {
#include "sub_main/0047EAE0.inc"
};
char D_0047EB00[0x20] = {
#include "sub_main/0047EB00.inc"
};
char D_0047EB20[0x20] = {
#include "sub_main/0047EB20.inc"
};
char D_0047EB40[0x20] = {
#include "sub_main/0047EB40.inc"
};
char D_0047EB60[0x20] = {
#include "sub_main/0047EB60.inc"
};
char D_0047EB80[0x20] = {
#include "sub_main/0047EB80.inc"
};
char D_0047EBA0[0x20] = {
#include "sub_main/0047EBA0.inc"
};
char D_0047EBC0[0x20] = {
#include "sub_main/0047EBC0.inc"
};
char D_0047EBE0[0x20] = {
#include "sub_main/0047EBE0.inc"
};
char D_0047EC00[0x20] = {
#include "sub_main/0047EC00.inc"
};
char D_0047EC20[0x20] = {
#include "sub_main/0047EC20.inc"
};
char D_0047EC40[0x20] = {
#include "sub_main/0047EC40.inc"
};
char D_0047EC60[0x20] = {
#include "sub_main/0047EC60.inc"
};
char D_0047EC80[0x20] = {
#include "sub_main/0047EC80.inc"
};
char D_0047ECA0[0x20] = {
#include "sub_main/0047ECA0.inc"
};
char D_0047ECC0[0x20] = {
#include "sub_main/0047ECC0.inc"
};
char D_0047ECE0[0x20] = {
#include "sub_main/0047ECE0.inc"
};
char D_0047ED00[0x20] = {
#include "sub_main/0047ED00.inc"
};
char D_0047ED20[0x20] = {
#include "sub_main/0047ED20.inc"
};
char D_0047ED40[0x20] = {
#include "sub_main/0047ED40.inc"
};
char D_0047ED60[0x10] = {
#include "sub_main/0047ED60.inc"
};
char D_0047ED70[0x20] = {
#include "sub_main/0047ED70.inc"
};
char D_0047ED90[0x20] = {
#include "sub_main/0047ED90.inc"
};
char D_0047EDB0[0x20] = {
#include "sub_main/0047EDB0.inc"
};
char D_0047EDD0[0x20] = {
#include "sub_main/0047EDD0.inc"
};
char D_0047EDF0[0x20] = {
#include "sub_main/0047EDF0.inc"
};
char D_0047EE10[0x20] = {
#include "sub_main/0047EE10.inc"
};
char D_0047EE30[0x20] = {
#include "sub_main/0047EE30.inc"
};
char D_0047EE50[0x20] = {
#include "sub_main/0047EE50.inc"
};
char D_0047EE70[0x20] = {
#include "sub_main/0047EE70.inc"
};
char D_0047EE90[0x20] = {
#include "sub_main/0047EE90.inc"
};
char D_0047EEB0[0x20] = {
#include "sub_main/0047EEB0.inc"
};
char D_0047EED0[0x20] = {
#include "sub_main/0047EED0.inc"
};
char D_0047EEF0[0x20] = {
#include "sub_main/0047EEF0.inc"
};
char D_0047EF10[0x20] = {
#include "sub_main/0047EF10.inc"
};
char D_0047EF30[0x20] = {
#include "sub_main/0047EF30.inc"
};
char D_0047EF50[0x20] = {
#include "sub_main/0047EF50.inc"
};
char D_0047EF70[0x20] = {
#include "sub_main/0047EF70.inc"
};
char D_0047EF90[0x20] = {
#include "sub_main/0047EF90.inc"
};
char D_0047EFB0[0x20] = {
#include "sub_main/0047EFB0.inc"
};
char D_0047EFD0[0x20] = {
#include "sub_main/0047EFD0.inc"
};
char D_0047EFF0[0x20] = {
#include "sub_main/0047EFF0.inc"
};
char D_0047F010[0x20] = {
#include "sub_main/0047F010.inc"
};
char D_0047F030[0x20] = {
#include "sub_main/0047F030.inc"
};
char D_0047F050[0x20] = {
#include "sub_main/0047F050.inc"
};
char D_0047F070[0x20] = {
#include "sub_main/0047F070.inc"
};
char D_0047F090[0x20] = {
#include "sub_main/0047F090.inc"
};
char D_0047F0B0[0x20] = {
#include "sub_main/0047F0B0.inc"
};
char D_0047F0D0[0x20] = {
#include "sub_main/0047F0D0.inc"
};
char D_0047F0F0[0x20] = {
#include "sub_main/0047F0F0.inc"
};
char D_0047F110[0x20] = {
#include "sub_main/0047F110.inc"
};
char D_0047F130[0x20] = {
#include "sub_main/0047F130.inc"
};
char D_0047F150[0x20] = {
#include "sub_main/0047F150.inc"
};
char D_0047F170[0x20] = {
#include "sub_main/0047F170.inc"
};
char D_0047F190[0x20] = {
#include "sub_main/0047F190.inc"
};
char D_0047F1B0[0x10] = {
#include "sub_main/0047F1B0.inc"
};
char D_0047F1C0[0x20] = {
#include "sub_main/0047F1C0.inc"
};
char D_0047F1E0[0x20] = {
#include "sub_main/0047F1E0.inc"
};
char D_0047F200[0x20] = {
#include "sub_main/0047F200.inc"
};
char D_0047F220[0x20] = {
#include "sub_main/0047F220.inc"
};
char D_0047F240[0x20] = {
#include "sub_main/0047F240.inc"
};
char D_0047F260[0x20] = {
#include "sub_main/0047F260.inc"
};
char D_0047F280[0x20] = {
#include "sub_main/0047F280.inc"
};
char D_0047F2A0[0x20] = {
#include "sub_main/0047F2A0.inc"
};
char D_0047F2C0[0x20] = {
#include "sub_main/0047F2C0.inc"
};
char D_0047F2E0[0x20] = {
#include "sub_main/0047F2E0.inc"
};
char D_0047F300[0x20] = {
#include "sub_main/0047F300.inc"
};
char D_0047F320[0x20] = {
#include "sub_main/0047F320.inc"
};
char D_0047F340[0x20] = {
#include "sub_main/0047F340.inc"
};
char D_0047F360[0x20] = {
#include "sub_main/0047F360.inc"
};
char D_0047F380[0x20] = {
#include "sub_main/0047F380.inc"
};
char D_0047F3A0[0x20] = {
#include "sub_main/0047F3A0.inc"
};
char D_0047F3C0[0x20] = {
#include "sub_main/0047F3C0.inc"
};
char D_0047F3E0[0x20] = {
#include "sub_main/0047F3E0.inc"
};
char D_0047F400[0x20] = {
#include "sub_main/0047F400.inc"
};
char D_0047F420[0x20] = {
#include "sub_main/0047F420.inc"
};
char D_0047F440[0x20] = {
#include "sub_main/0047F440.inc"
};
char D_0047F460[0x20] = {
#include "sub_main/0047F460.inc"
};
char D_0047F480[0x20] = {
#include "sub_main/0047F480.inc"
};
char D_0047F4A0[0x20] = {
#include "sub_main/0047F4A0.inc"
};
char D_0047F4C0[0x20] = {
#include "sub_main/0047F4C0.inc"
};
char D_0047F4E0[0x20] = {
#include "sub_main/0047F4E0.inc"
};
char D_0047F500[0x20] = {
#include "sub_main/0047F500.inc"
};
char D_0047F520[0x20] = {
#include "sub_main/0047F520.inc"
};
char D_0047F540[0x20] = {
#include "sub_main/0047F540.inc"
};
char D_0047F560[0x20] = {
#include "sub_main/0047F560.inc"
};
char D_0047F580[0x20] = {
#include "sub_main/0047F580.inc"
};
char D_0047F5A0[0x20] = {
#include "sub_main/0047F5A0.inc"
};
char D_0047F5C0[0x20] = {
#include "sub_main/0047F5C0.inc"
};
char D_0047F5E0[0x20] = {
#include "sub_main/0047F5E0.inc"
};
char D_0047F600[0x20] = {
#include "sub_main/0047F600.inc"
};
char D_0047F620[0x20] = {
#include "sub_main/0047F620.inc"
};
char D_0047F640[0x20] = {
#include "sub_main/0047F640.inc"
};
char D_0047F660[0x20] = {
#include "sub_main/0047F660.inc"
};
char D_0047F680[0x20] = {
#include "sub_main/0047F680.inc"
};
char D_0047F6A0[0x20] = {
#include "sub_main/0047F6A0.inc"
};
char D_0047F6C0[0x20] = {
#include "sub_main/0047F6C0.inc"
};
char D_0047F6E0[0x20] = {
#include "sub_main/0047F6E0.inc"
};
char D_0047F700[0x20] = {
#include "sub_main/0047F700.inc"
};
char D_0047F720[0x20] = {
#include "sub_main/0047F720.inc"
};
char D_0047F740[0x20] = {
#include "sub_main/0047F740.inc"
};
char D_0047F760[0x20] = {
#include "sub_main/0047F760.inc"
};
char D_0047F780[0x20] = {
#include "sub_main/0047F780.inc"
};
char D_0047F7A0[0x20] = {
#include "sub_main/0047F7A0.inc"
};
char D_0047F7C0[0x20] = {
#include "sub_main/0047F7C0.inc"
};
char D_0047F7E0[0x20] = {
#include "sub_main/0047F7E0.inc"
};
char D_0047F800[0x20] = {
#include "sub_main/0047F800.inc"
};
char D_0047F820[0x20] = {
#include "sub_main/0047F820.inc"
};
char D_0047F840[0x20] = {
#include "sub_main/0047F840.inc"
};
char D_0047F860[0x20] = {
#include "sub_main/0047F860.inc"
};
char D_0047F880[0x20] = {
#include "sub_main/0047F880.inc"
};
char D_0047F8A0[0x20] = {
#include "sub_main/0047F8A0.inc"
};
char D_0047F8C0[0x20] = {
#include "sub_main/0047F8C0.inc"
};
char D_0047F8E0[0x20] = {
#include "sub_main/0047F8E0.inc"
};
char D_0047F900[0x20] = {
#include "sub_main/0047F900.inc"
};
char D_0047F920[0x20] = {
#include "sub_main/0047F920.inc"
};
char D_0047F940[0x20] = {
#include "sub_main/0047F940.inc"
};
char D_0047F960[0x20] = {
#include "sub_main/0047F960.inc"
};
char D_0047F980[0x20] = {
#include "sub_main/0047F980.inc"
};
char D_0047F9A0[0x20] = {
#include "sub_main/0047F9A0.inc"
};
char D_0047F9C0[0x20] = {
#include "sub_main/0047F9C0.inc"
};
char D_0047F9E0[0x20] = {
#include "sub_main/0047F9E0.inc"
};
char D_0047FA00[0x20] = {
#include "sub_main/0047FA00.inc"
};
char D_0047FA20[0x20] = {
#include "sub_main/0047FA20.inc"
};
char D_0047FA40[0x20] = {
#include "sub_main/0047FA40.inc"
};
char D_0047FA60[0x20] = {
#include "sub_main/0047FA60.inc"
};
char D_0047FA80[0x20] = {
#include "sub_main/0047FA80.inc"
};
char D_0047FAA0[0x20] = {
#include "sub_main/0047FAA0.inc"
};
char D_0047FAC0[0x20] = {
#include "sub_main/0047FAC0.inc"
};
char D_0047FAE0[0x20] = {
#include "sub_main/0047FAE0.inc"
};
char D_0047FB00[0x20] = {
#include "sub_main/0047FB00.inc"
};
char D_0047FB20[0x20] = {
#include "sub_main/0047FB20.inc"
};
char D_0047FB40[0x20] = {
#include "sub_main/0047FB40.inc"
};
char D_0047FB60[0x20] = {
#include "sub_main/0047FB60.inc"
};
char D_0047FB80[0x20] = {
#include "sub_main/0047FB80.inc"
};
char D_0047FBA0[0x20] = {
#include "sub_main/0047FBA0.inc"
};
char D_0047FBC0[0x20] = {
#include "sub_main/0047FBC0.inc"
};
char D_0047FBE0[0x20] = {
#include "sub_main/0047FBE0.inc"
};
char D_0047FC00[0x20] = {
#include "sub_main/0047FC00.inc"
};
char D_0047FC20[0x20] = {
#include "sub_main/0047FC20.inc"
};
char D_0047FC40[0x20] = {
#include "sub_main/0047FC40.inc"
};
char D_0047FC60[0x20] = {
#include "sub_main/0047FC60.inc"
};
char D_0047FC80[0x20] = {
#include "sub_main/0047FC80.inc"
};
char D_0047FCA0[0x20] = {
#include "sub_main/0047FCA0.inc"
};
char D_0047FCC0[0x20] = {
#include "sub_main/0047FCC0.inc"
};
char D_0047FCE0[0x20] = {
#include "sub_main/0047FCE0.inc"
};
char D_0047FD00[0x20] = {
#include "sub_main/0047FD00.inc"
};
char D_0047FD20[0x20] = {
#include "sub_main/0047FD20.inc"
};
char D_0047FD40[0x20] = {
#include "sub_main/0047FD40.inc"
};
char D_0047FD60[0x20] = {
#include "sub_main/0047FD60.inc"
};
char D_0047FD80[0x20] = {
#include "sub_main/0047FD80.inc"
};
char D_0047FDA0[0x20] = {
#include "sub_main/0047FDA0.inc"
};
char D_0047FDC0[0x20] = {
#include "sub_main/0047FDC0.inc"
};
char D_0047FDE0[0x20] = {
#include "sub_main/0047FDE0.inc"
};
char D_0047FE00[0x20] = {
#include "sub_main/0047FE00.inc"
};
char D_0047FE20[0x20] = {
#include "sub_main/0047FE20.inc"
};
char D_0047FE40[0x20] = {
#include "sub_main/0047FE40.inc"
};
char D_0047FE60[0x20] = {
#include "sub_main/0047FE60.inc"
};
char D_0047FE80[0x20] = {
#include "sub_main/0047FE80.inc"
};
char D_0047FEA0[0x20] = {
#include "sub_main/0047FEA0.inc"
};
char D_0047FEC0[0x20] = {
#include "sub_main/0047FEC0.inc"
};
char D_0047FEE0[0x20] = {
#include "sub_main/0047FEE0.inc"
};
char D_0047FF00[0x20] = {
#include "sub_main/0047FF00.inc"
};
char D_0047FF20[0x20] = {
#include "sub_main/0047FF20.inc"
};
char D_0047FF40[0x20] = {
#include "sub_main/0047FF40.inc"
};
char D_0047FF60[0x20] = {
#include "sub_main/0047FF60.inc"
};
char D_0047FF80[0x20] = {
#include "sub_main/0047FF80.inc"
};
char D_0047FFA0[0x20] = {
#include "sub_main/0047FFA0.inc"
};
char D_0047FFC0[0x20] = {
#include "sub_main/0047FFC0.inc"
};
char D_0047FFE0[0x20] = {
#include "sub_main/0047FFE0.inc"
};
char D_00480000[0x20] = {
#include "sub_main/00480000.inc"
};
char D_00480020[0x20] = {
#include "sub_main/00480020.inc"
};
char D_00480040[0x20] = {
#include "sub_main/00480040.inc"
};
char D_00480060[0x20] = {
#include "sub_main/00480060.inc"
};
char D_00480080[0x20] = {
#include "sub_main/00480080.inc"
};
char D_004800A0[0x20] = {
#include "sub_main/004800A0.inc"
};
char D_004800C0[0x20] = {
#include "sub_main/004800C0.inc"
};
char D_004800E0[0x20] = {
#include "sub_main/004800E0.inc"
};
char D_00480100[0x20] = {
#include "sub_main/00480100.inc"
};
char D_00480120[0x20] = {
#include "sub_main/00480120.inc"
};
char D_00480140[0x20] = {
#include "sub_main/00480140.inc"
};
char D_00480160[0x20] = {
#include "sub_main/00480160.inc"
};
char D_00480180[0x20] = {
#include "sub_main/00480180.inc"
};
char D_004801A0[0x20] = {
#include "sub_main/004801A0.inc"
};
char D_004801C0[0x20] = {
#include "sub_main/004801C0.inc"
};
char D_004801E0[0x20] = {
#include "sub_main/004801E0.inc"
};
char D_00480200[0x20] = {
#include "sub_main/00480200.inc"
};
char D_00480220[0x20] = {
#include "sub_main/00480220.inc"
};
char D_00480240[0x20] = {
#include "sub_main/00480240.inc"
};
char D_00480260[0x20] = {
#include "sub_main/00480260.inc"
};
char D_00480280[0x20] = {
#include "sub_main/00480280.inc"
};
char D_004802A0[0x20] = {
#include "sub_main/004802A0.inc"
};
char D_004802C0[0x20] = {
#include "sub_main/004802C0.inc"
};
char D_004802E0[0x20] = {
#include "sub_main/004802E0.inc"
};
char D_00480300[0x20] = {
#include "sub_main/00480300.inc"
};
char D_00480320[0x20] = {
#include "sub_main/00480320.inc"
};
char D_00480340[0x20] = {
#include "sub_main/00480340.inc"
};
char D_00480360[0x20] = {
#include "sub_main/00480360.inc"
};
char D_00480380[0x20] = {
#include "sub_main/00480380.inc"
};
char D_004803A0[0x20] = {
#include "sub_main/004803A0.inc"
};
char D_004803C0[0x20] = {
#include "sub_main/004803C0.inc"
};
char D_004803E0[0x20] = {
#include "sub_main/004803E0.inc"
};
char D_00480400[0x20] = {
#include "sub_main/00480400.inc"
};
char D_00480420[0x20] = {
#include "sub_main/00480420.inc"
};
char D_00480440[0x20] = {
#include "sub_main/00480440.inc"
};
char D_00480460[0x20] = {
#include "sub_main/00480460.inc"
};
char D_00480480[0x20] = {
#include "sub_main/00480480.inc"
};
char D_004804A0[0x20] = {
#include "sub_main/004804A0.inc"
};
char D_004804C0[0x20] = {
#include "sub_main/004804C0.inc"
};
char D_004804E0[0x20] = {
#include "sub_main/004804E0.inc"
};
char D_00480500[0x20] = {
#include "sub_main/00480500.inc"
};
char D_00480520[0x20] = {
#include "sub_main/00480520.inc"
};
char D_00480540[0x20] = {
#include "sub_main/00480540.inc"
};
char D_00480560[0x20] = {
#include "sub_main/00480560.inc"
};
char D_00480580[0x20] = {
#include "sub_main/00480580.inc"
};
char D_004805A0[0x20] = {
#include "sub_main/004805A0.inc"
};
char D_004805C0[0x20] = {
#include "sub_main/004805C0.inc"
};
char D_004805E0[0x20] = {
#include "sub_main/004805E0.inc"
};
char D_00480600[0x20] = {
#include "sub_main/00480600.inc"
};
char D_00480620[0x20] = {
#include "sub_main/00480620.inc"
};
char D_00480640[0x20] = {
#include "sub_main/00480640.inc"
};
char D_00480660[0x20] = {
#include "sub_main/00480660.inc"
};
char D_00480680[0x20] = {
#include "sub_main/00480680.inc"
};
char D_004806A0[0x20] = {
#include "sub_main/004806A0.inc"
};
char D_004806C0[0x20] = {
#include "sub_main/004806C0.inc"
};
char D_004806E0[0x20] = {
#include "sub_main/004806E0.inc"
};
char D_00480700[0x20] = {
#include "sub_main/00480700.inc"
};
char D_00480720[0x20] = {
#include "sub_main/00480720.inc"
};
char D_00480740[0x20] = {
#include "sub_main/00480740.inc"
};
char D_00480760[0x20] = {
#include "sub_main/00480760.inc"
};
char D_00480780[0x20] = {
#include "sub_main/00480780.inc"
};
char D_004807A0[0x20] = {
#include "sub_main/004807A0.inc"
};
char D_004807C0[0x20] = {
#include "sub_main/004807C0.inc"
};
char D_004807E0[0x20] = {
#include "sub_main/004807E0.inc"
};
char D_00480800[0x20] = {
#include "sub_main/00480800.inc"
};
char D_00480820[0x20] = {
#include "sub_main/00480820.inc"
};
char D_00480840[0x20] = {
#include "sub_main/00480840.inc"
};
char D_00480860[0x20] = {
#include "sub_main/00480860.inc"
};
char D_00480880[0x20] = {
#include "sub_main/00480880.inc"
};
char D_004808A0[0x20] = {
#include "sub_main/004808A0.inc"
};
char D_004808C0[0x20] = {
#include "sub_main/004808C0.inc"
};
char D_004808E0[0x20] = {
#include "sub_main/004808E0.inc"
};
char D_00480900[0x20] = {
#include "sub_main/00480900.inc"
};
char D_00480920[0x20] = {
#include "sub_main/00480920.inc"
};
char D_00480940[0x20] = {
#include "sub_main/00480940.inc"
};
char D_00480960[0x20] = {
#include "sub_main/00480960.inc"
};
char D_00480980[0x20] = {
#include "sub_main/00480980.inc"
};
char D_004809A0[0x20] = {
#include "sub_main/004809A0.inc"
};
char D_004809C0[0x20] = {
#include "sub_main/004809C0.inc"
};
char D_004809E0[0x20] = {
#include "sub_main/004809E0.inc"
};
char D_00480A00[0x20] = {
#include "sub_main/00480A00.inc"
};
char D_00480A20[0x20] = {
#include "sub_main/00480A20.inc"
};
char D_00480A40[0x20] = {
#include "sub_main/00480A40.inc"
};
char D_00480A60[0x20] = {
#include "sub_main/00480A60.inc"
};
char D_00480A80[0x20] = {
#include "sub_main/00480A80.inc"
};
char D_00480AA0[0x20] = {
#include "sub_main/00480AA0.inc"
};
char D_00480AC0[0x20] = {
#include "sub_main/00480AC0.inc"
};
char D_00480AE0[0x20] = {
#include "sub_main/00480AE0.inc"
};
char D_00480B00[0x20] = {
#include "sub_main/00480B00.inc"
};
char D_00480B20[0x20] = {
#include "sub_main/00480B20.inc"
};
char D_00480B40[0x20] = {
#include "sub_main/00480B40.inc"
};
char D_00480B60[0x20] = {
#include "sub_main/00480B60.inc"
};
char D_00480B80[0x20] = {
#include "sub_main/00480B80.inc"
};
char D_00480BA0[0x20] = {
#include "sub_main/00480BA0.inc"
};
char D_00480BC0[0x20] = {
#include "sub_main/00480BC0.inc"
};
char D_00480BE0[0x20] = {
#include "sub_main/00480BE0.inc"
};
char D_00480C00[0x20] = {
#include "sub_main/00480C00.inc"
};
char D_00480C20[0x20] = {
#include "sub_main/00480C20.inc"
};
char D_00480C40[0x20] = {
#include "sub_main/00480C40.inc"
};
char D_00480C60[0x20] = {
#include "sub_main/00480C60.inc"
};
char D_00480C80[0x20] = {
#include "sub_main/00480C80.inc"
};
char D_00480CA0[0x20] = {
#include "sub_main/00480CA0.inc"
};
char D_00480CC0[0x20] = {
#include "sub_main/00480CC0.inc"
};
char D_00480CE0[0x20] = {
#include "sub_main/00480CE0.inc"
};
char D_00480D00[0x20] = {
#include "sub_main/00480D00.inc"
};
char D_00480D20[0x20] = {
#include "sub_main/00480D20.inc"
};
char D_00480D40[0x30] = {
#include "sub_main/00480D40.inc"
};
char D_00480D70[0x30] = {
#include "sub_main/00480D70.inc"
};
char D_00480DA0[0x20] = {
#include "sub_main/00480DA0.inc"
};
char D_00480DC0[0x30] = {
#include "sub_main/00480DC0.inc"
};
char D_00480DF0[0x30] = {
#include "sub_main/00480DF0.inc"
};
char D_00480E20[0x20] = {
#include "sub_main/00480E20.inc"
};
char D_00480E40[0x20] = {
#include "sub_main/00480E40.inc"
};
char D_00480E60[0x20] = {
#include "sub_main/00480E60.inc"
};
char D_00480E80[0x20] = {
#include "sub_main/00480E80.inc"
};
char D_00480EA0[0x20] = {
#include "sub_main/00480EA0.inc"
};
char D_00480EC0[0x20] = {
#include "sub_main/00480EC0.inc"
};
char D_00480EE0[0x20] = {
#include "sub_main/00480EE0.inc"
};
char D_00480F00[0x20] = {
#include "sub_main/00480F00.inc"
};
char D_00480F20[0x20] = {
#include "sub_main/00480F20.inc"
};
char D_00480F40[0x20] = {
#include "sub_main/00480F40.inc"
};
char D_00480F60[0x20] = {
#include "sub_main/00480F60.inc"
};
char D_00480F80[0x20] = {
#include "sub_main/00480F80.inc"
};
char D_00480FA0[0x20] = {
#include "sub_main/00480FA0.inc"
};
char D_00480FC0[0x20] = {
#include "sub_main/00480FC0.inc"
};
char D_00480FE0[0x20] = {
#include "sub_main/00480FE0.inc"
};
char D_00481000[0x20] = {
#include "sub_main/00481000.inc"
};
char D_00481020[0x20] = {
#include "sub_main/00481020.inc"
};
char D_00481040[0x20] = {
#include "sub_main/00481040.inc"
};
char D_00481060[0x20] = {
#include "sub_main/00481060.inc"
};
char D_00481080[0x20] = {
#include "sub_main/00481080.inc"
};
char D_004810A0[0x20] = {
#include "sub_main/004810A0.inc"
};
char D_004810C0[0x20] = {
#include "sub_main/004810C0.inc"
};
char D_004810E0[0x20] = {
#include "sub_main/004810E0.inc"
};
char D_00481100[0x20] = {
#include "sub_main/00481100.inc"
};
char D_00481120[0x20] = {
#include "sub_main/00481120.inc"
};
char D_00481140[0x20] = {
#include "sub_main/00481140.inc"
};
char D_00481160[0x20] = {
#include "sub_main/00481160.inc"
};
char D_00481180[0x20] = {
#include "sub_main/00481180.inc"
};
char D_004811A0[0x20] = {
#include "sub_main/004811A0.inc"
};
char D_004811C0[0x20] = {
#include "sub_main/004811C0.inc"
};
char D_004811E0[0x20] = {
#include "sub_main/004811E0.inc"
};
char D_00481200[0x20] = {
#include "sub_main/00481200.inc"
};
char D_00481220[0x20] = {
#include "sub_main/00481220.inc"
};
char D_00481240[0x20] = {
#include "sub_main/00481240.inc"
};
char D_00481260[0x10] = {
#include "sub_main/00481260.inc"
};
char D_00481270[0x20] = {
#include "sub_main/00481270.inc"
};
char D_00481290[0x20] = {
#include "sub_main/00481290.inc"
};
char D_004812B0[0x20] = {
#include "sub_main/004812B0.inc"
};
char D_004812D0[0x20] = {
#include "sub_main/004812D0.inc"
};
char D_004812F0[0x30] = {
#include "sub_main/004812F0.inc"
};
char D_00481320[0x20] = {
#include "sub_main/00481320.inc"
};
char D_00481340[0x20] = {
#include "sub_main/00481340.inc"
};
char D_00481360[0x20] = {
#include "sub_main/00481360.inc"
};
char D_00481380[0x20] = {
#include "sub_main/00481380.inc"
};
char D_004813A0[0x20] = {
#include "sub_main/004813A0.inc"
};
char D_004813C0[0x20] = {
#include "sub_main/004813C0.inc"
};
char D_004813E0[0x20] = {
#include "sub_main/004813E0.inc"
};
char D_00481400[0x20] = {
#include "sub_main/00481400.inc"
};
char D_00481420[0x20] = {
#include "sub_main/00481420.inc"
};
char D_00481440[0x20] = {
#include "sub_main/00481440.inc"
};
char D_00481460[0x20] = {
#include "sub_main/00481460.inc"
};
char D_00481480[0x20] = {
#include "sub_main/00481480.inc"
};
char D_004814A0[0x20] = {
#include "sub_main/004814A0.inc"
};
char D_004814C0[0x20] = {
#include "sub_main/004814C0.inc"
};
char D_004814E0[0x10] = {
#include "sub_main/004814E0.inc"
};
char D_004814F0[0x20] = {
#include "sub_main/004814F0.inc"
};
char D_00481510[0x10] = {
#include "sub_main/00481510.inc"
};
char D_00481520[0x20] = {
#include "sub_main/00481520.inc"
};
char D_00481540[0x20] = {
#include "sub_main/00481540.inc"
};
char D_00481560[0x20] = {
#include "sub_main/00481560.inc"
};
char D_00481580[0x20] = {
#include "sub_main/00481580.inc"
};
char D_004815A0[0x20] = {
#include "sub_main/004815A0.inc"
};
char D_004815C0[0x20] = {
#include "sub_main/004815C0.inc"
};
char D_004815E0[0x20] = {
#include "sub_main/004815E0.inc"
};
char D_00481600[0x20] = {
#include "sub_main/00481600.inc"
};
char D_00481620[0x30] = {
#include "sub_main/00481620.inc"
};
char D_00481650[0x20] = {
#include "sub_main/00481650.inc"
};
char D_00481670[0x20] = {
#include "sub_main/00481670.inc"
};
char D_00481690[0x10] = {
#include "sub_main/00481690.inc"
};
char D_004816A0[0x20] = {
#include "sub_main/004816A0.inc"
};
char D_004816C0[0x20] = {
#include "sub_main/004816C0.inc"
};
char D_004816E0[0x20] = {
#include "sub_main/004816E0.inc"
};
char D_00481700[0x20] = {
#include "sub_main/00481700.inc"
};
char D_00481720[0x20] = {
#include "sub_main/00481720.inc"
};
char D_00481740[0x10] = {
#include "sub_main/00481740.inc"
};
char D_00481750[0x20] = {
#include "sub_main/00481750.inc"
};
char D_00481770[0x20] = {
#include "sub_main/00481770.inc"
};
char D_00481790[0x20] = {
#include "sub_main/00481790.inc"
};
char D_004817B0[0x20] = {
#include "sub_main/004817B0.inc"
};
char D_004817D0[0x20] = {
#include "sub_main/004817D0.inc"
};
char D_004817F0[0x20] = {
#include "sub_main/004817F0.inc"
};
char D_00481810[0x20] = {
#include "sub_main/00481810.inc"
};
char D_00481830[0x20] = {
#include "sub_main/00481830.inc"
};
char D_00481850[0x20] = {
#include "sub_main/00481850.inc"
};
char D_00481870[0x20] = {
#include "sub_main/00481870.inc"
};
char D_00481890[0x20] = {
#include "sub_main/00481890.inc"
};
char D_004818B0[0x20] = {
#include "sub_main/004818B0.inc"
};
char D_004818D0[0x20] = {
#include "sub_main/004818D0.inc"
};
char D_004818F0[0x20] = {
#include "sub_main/004818F0.inc"
};
char D_00481910[0x20] = {
#include "sub_main/00481910.inc"
};
char D_00481930[0x20] = {
#include "sub_main/00481930.inc"
};
char D_00481950[0x20] = {
#include "sub_main/00481950.inc"
};
char D_00481970[0x20] = {
#include "sub_main/00481970.inc"
};
char D_00481990[0x20] = {
#include "sub_main/00481990.inc"
};
char D_004819B0[0x20] = {
#include "sub_main/004819B0.inc"
};
char D_004819D0[0x20] = {
#include "sub_main/004819D0.inc"
};
char D_004819F0[0x20] = {
#include "sub_main/004819F0.inc"
};
char D_00481A10[0x20] = {
#include "sub_main/00481A10.inc"
};
char D_00481A30[0x20] = {
#include "sub_main/00481A30.inc"
};
char D_00481A50[0x20] = {
#include "sub_main/00481A50.inc"
};
char D_00481A70[0x20] = {
#include "sub_main/00481A70.inc"
};
char D_00481A90[0x20] = {
#include "sub_main/00481A90.inc"
};
char D_00481AB0[0x20] = {
#include "sub_main/00481AB0.inc"
};
char D_00481AD0[0x30] = {
#include "sub_main/00481AD0.inc"
};
char D_00481B00[0x20] = {
#include "sub_main/00481B00.inc"
};
char D_00481B20[0x20] = {
#include "sub_main/00481B20.inc"
};
char D_00481B40[0x20] = {
#include "sub_main/00481B40.inc"
};
char D_00481B60[0x20] = {
#include "sub_main/00481B60.inc"
};
char D_00481B80[0x20] = {
#include "sub_main/00481B80.inc"
};
char D_00481BA0[0x20] = {
#include "sub_main/00481BA0.inc"
};
char D_00481BC0[0x20] = {
#include "sub_main/00481BC0.inc"
};
char D_00481BE0[0x20] = {
#include "sub_main/00481BE0.inc"
};
char D_00481C00[0x20] = {
#include "sub_main/00481C00.inc"
};
char D_00481C20[0x20] = {
#include "sub_main/00481C20.inc"
};
char D_00481C40[0x20] = {
#include "sub_main/00481C40.inc"
};
char D_00481C60[0x30] = {
#include "sub_main/00481C60.inc"
};
char D_00481C90[0x20] = {
#include "sub_main/00481C90.inc"
};
char D_00481CB0[0x20] = {
#include "sub_main/00481CB0.inc"
};
char D_00481CD0[0x20] = {
#include "sub_main/00481CD0.inc"
};
char D_00481CF0[0x20] = {
#include "sub_main/00481CF0.inc"
};
char D_00481D10[0x20] = {
#include "sub_main/00481D10.inc"
};
char D_00481D30[0x20] = {
#include "sub_main/00481D30.inc"
};
char D_00481D50[0x20] = {
#include "sub_main/00481D50.inc"
};
char D_00481D70[0x30] = {
#include "sub_main/00481D70.inc"
};
char D_00481DA0[0x30] = {
#include "sub_main/00481DA0.inc"
};
char D_00481DD0[0x20] = {
#include "sub_main/00481DD0.inc"
};
char D_00481DF0[0x20] = {
#include "sub_main/00481DF0.inc"
};
char D_00481E10[0x30] = {
#include "sub_main/00481E10.inc"
};
char D_00481E40[0x20] = {
#include "sub_main/00481E40.inc"
};
char D_00481E60[0x20] = {
#include "sub_main/00481E60.inc"
};
char D_00481E80[0x20] = {
#include "sub_main/00481E80.inc"
};
char D_00481EA0[0x30] = {
#include "sub_main/00481EA0.inc"
};
char D_00481ED0[0x30] = {
#include "sub_main/00481ED0.inc"
};
char D_00481F00[0x20] = {
#include "sub_main/00481F00.inc"
};
char D_00481F20[0x20] = {
#include "sub_main/00481F20.inc"
};
char D_00481F40[0x20] = {
#include "sub_main/00481F40.inc"
};
char D_00481F60[0x20] = {
#include "sub_main/00481F60.inc"
};
char D_00481F80[0x20] = {
#include "sub_main/00481F80.inc"
};
char D_00481FA0[0x20] = {
#include "sub_main/00481FA0.inc"
};
char D_00481FC0[0x30] = {
#include "sub_main/00481FC0.inc"
};
char D_00481FF0[0x30] = {
#include "sub_main/00481FF0.inc"
};
char D_00482020[0x20] = {
#include "sub_main/00482020.inc"
};
char D_00482040[0x20] = {
#include "sub_main/00482040.inc"
};
char D_00482060[0x30] = {
#include "sub_main/00482060.inc"
};
char D_00482090[0x30] = {
#include "sub_main/00482090.inc"
};
char D_004820C0[0x30] = {
#include "sub_main/004820C0.inc"
};
char D_004820F0[0x20] = {
#include "sub_main/004820F0.inc"
};
char D_00482110[0x20] = {
#include "sub_main/00482110.inc"
};
char D_00482130[0x30] = {
#include "sub_main/00482130.inc"
};
char D_00482160[0x30] = {
#include "sub_main/00482160.inc"
};
char D_00482190[0x20] = {
#include "sub_main/00482190.inc"
};
char D_004821B0[0x20] = {
#include "sub_main/004821B0.inc"
};
char D_004821D0[0x20] = {
#include "sub_main/004821D0.inc"
};
char D_004821F0[0x20] = {
#include "sub_main/004821F0.inc"
};
char D_00482210[0x20] = {
#include "sub_main/00482210.inc"
};
char D_00482230[0x20] = {
#include "sub_main/00482230.inc"
};
char D_00482250[0x20] = {
#include "sub_main/00482250.inc"
};
char D_00482270[0x20] = {
#include "sub_main/00482270.inc"
};
char D_00482290[0x20] = {
#include "sub_main/00482290.inc"
};
char D_004822B0[0x20] = {
#include "sub_main/004822B0.inc"
};
char D_004822D0[0x30] = {
#include "sub_main/004822D0.inc"
};
char D_00482300[0x20] = {
#include "sub_main/00482300.inc"
};
char D_00482320[0x20] = {
#include "sub_main/00482320.inc"
};
char D_00482340[0x20] = {
#include "sub_main/00482340.inc"
};
char D_00482360[0x20] = {
#include "sub_main/00482360.inc"
};
char D_00482380[0x20] = {
#include "sub_main/00482380.inc"
};
char D_004823A0[0x30] = {
#include "sub_main/004823A0.inc"
};
char D_004823D0[0x20] = {
#include "sub_main/004823D0.inc"
};
char D_004823F0[0x20] = {
#include "sub_main/004823F0.inc"
};
char D_00482410[0x20] = {
#include "sub_main/00482410.inc"
};
char D_00482430[0x20] = {
#include "sub_main/00482430.inc"
};
char D_00482450[0x20] = {
#include "sub_main/00482450.inc"
};
char D_00482470[0x30] = {
#include "sub_main/00482470.inc"
};
char D_004824A0[0x20] = {
#include "sub_main/004824A0.inc"
};
char D_004824C0[0x20] = {
#include "sub_main/004824C0.inc"
};
char D_004824E0[0x20] = {
#include "sub_main/004824E0.inc"
};
char D_00482500[0x30] = {
#include "sub_main/00482500.inc"
};
char D_00482530[0x20] = {
#include "sub_main/00482530.inc"
};
char D_00482550[0x20] = {
#include "sub_main/00482550.inc"
};
char D_00482570[0x20] = {
#include "sub_main/00482570.inc"
};
char D_00482590[0x20] = {
#include "sub_main/00482590.inc"
};
char D_004825B0[0x20] = {
#include "sub_main/004825B0.inc"
};
char D_004825D0[0x30] = {
#include "sub_main/004825D0.inc"
};
char D_00482600[0x20] = {
#include "sub_main/00482600.inc"
};
char D_00482620[0x20] = {
#include "sub_main/00482620.inc"
};
char D_00482640[0x20] = {
#include "sub_main/00482640.inc"
};
char D_00482660[0x20] = {
#include "sub_main/00482660.inc"
};
char D_00482680[0x20] = {
#include "sub_main/00482680.inc"
};
char D_004826A0[0x20] = {
#include "sub_main/004826A0.inc"
};
char D_004826C0[0x30] = {
#include "sub_main/004826C0.inc"
};
char D_004826F0[0x20] = {
#include "sub_main/004826F0.inc"
};
char D_00482710[0x20] = {
#include "sub_main/00482710.inc"
};
char D_00482730[0x20] = {
#include "sub_main/00482730.inc"
};
char D_00482750[0x20] = {
#include "sub_main/00482750.inc"
};
char D_00482770[0x20] = {
#include "sub_main/00482770.inc"
};
char D_00482790[0x20] = {
#include "sub_main/00482790.inc"
};
char D_004827B0[0x20] = {
#include "sub_main/004827B0.inc"
};
char D_004827D0[0x20] = {
#include "sub_main/004827D0.inc"
};
char D_004827F0[0x20] = {
#include "sub_main/004827F0.inc"
};
char D_00482810[0x20] = {
#include "sub_main/00482810.inc"
};
char D_00482830[0x20] = {
#include "sub_main/00482830.inc"
};
char D_00482850[0x20] = {
#include "sub_main/00482850.inc"
};
char D_00482870[0x30] = {
#include "sub_main/00482870.inc"
};
char D_004828A0[0x20] = {
#include "sub_main/004828A0.inc"
};
char D_004828C0[0x20] = {
#include "sub_main/004828C0.inc"
};
char D_004828E0[0x20] = {
#include "sub_main/004828E0.inc"
};
char D_00482900[0x20] = {
#include "sub_main/00482900.inc"
};
char D_00482920[0x20] = {
#include "sub_main/00482920.inc"
};
char D_00482940[0x20] = {
#include "sub_main/00482940.inc"
};
char D_00482960[0x20] = {
#include "sub_main/00482960.inc"
};
char D_00482980[0x20] = {
#include "sub_main/00482980.inc"
};
char D_004829A0[0x20] = {
#include "sub_main/004829A0.inc"
};
char D_004829C0[0x20] = {
#include "sub_main/004829C0.inc"
};
char D_004829E0[0x20] = {
#include "sub_main/004829E0.inc"
};
char D_00482A00[0x20] = {
#include "sub_main/00482A00.inc"
};
char D_00482A20[0x20] = {
#include "sub_main/00482A20.inc"
};
char D_00482A40[0x20] = {
#include "sub_main/00482A40.inc"
};
char D_00482A60[0x20] = {
#include "sub_main/00482A60.inc"
};
char D_00482A80[0x20] = {
#include "sub_main/00482A80.inc"
};
char D_00482AA0[0x20] = {
#include "sub_main/00482AA0.inc"
};
char D_00482AC0[0x20] = {
#include "sub_main/00482AC0.inc"
};
char D_00482AE0[0x20] = {
#include "sub_main/00482AE0.inc"
};
char D_00482B00[0x20] = {
#include "sub_main/00482B00.inc"
};
char D_00482B20[0x20] = {
#include "sub_main/00482B20.inc"
};
char D_00482B40[0x20] = {
#include "sub_main/00482B40.inc"
};
char D_00482B60[0x20] = {
#include "sub_main/00482B60.inc"
};
char D_00482B80[0x20] = {
#include "sub_main/00482B80.inc"
};
char D_00482BA0[0x20] = {
#include "sub_main/00482BA0.inc"
};
char D_00482BC0[0x20] = {
#include "sub_main/00482BC0.inc"
};
char D_00482BE0[0x20] = {
#include "sub_main/00482BE0.inc"
};
char D_00482C00[0x20] = {
#include "sub_main/00482C00.inc"
};
char D_00482C20[0x20] = {
#include "sub_main/00482C20.inc"
};
char D_00482C40[0x20] = {
#include "sub_main/00482C40.inc"
};
char D_00482C60[0x20] = {
#include "sub_main/00482C60.inc"
};
char D_00482C80[0x20] = {
#include "sub_main/00482C80.inc"
};
char D_00482CA0[0x20] = {
#include "sub_main/00482CA0.inc"
};
char D_00482CC0[0x20] = {
#include "sub_main/00482CC0.inc"
};
char D_00482CE0[0x20] = {
#include "sub_main/00482CE0.inc"
};
char D_00482D00[0x20] = {
#include "sub_main/00482D00.inc"
};
char D_00482D20[0x20] = {
#include "sub_main/00482D20.inc"
};
char D_00482D40[0x20] = {
#include "sub_main/00482D40.inc"
};
char D_00482D60[0x20] = {
#include "sub_main/00482D60.inc"
};
char D_00482D80[0x20] = {
#include "sub_main/00482D80.inc"
};
char D_00482DA0[0x20] = {
#include "sub_main/00482DA0.inc"
};
char D_00482DC0[0x20] = {
#include "sub_main/00482DC0.inc"
};
char D_00482DE0[0x20] = {
#include "sub_main/00482DE0.inc"
};
char D_00482E00[0x20] = {
#include "sub_main/00482E00.inc"
};
char D_00482E20[0x20] = {
#include "sub_main/00482E20.inc"
};
char D_00482E40[0x20] = {
#include "sub_main/00482E40.inc"
};
char D_00482E60[0x20] = {
#include "sub_main/00482E60.inc"
};
char D_00482E80[0x20] = {
#include "sub_main/00482E80.inc"
};
char D_00482EA0[0x30] = {
#include "sub_main/00482EA0.inc"
};
char D_00482ED0[0x20] = {
#include "sub_main/00482ED0.inc"
};
char D_00482EF0[0x20] = {
#include "sub_main/00482EF0.inc"
};
char D_00482F10[0x20] = {
#include "sub_main/00482F10.inc"
};
char D_00482F30[0x20] = {
#include "sub_main/00482F30.inc"
};
char D_00482F50[0x20] = {
#include "sub_main/00482F50.inc"
};
char D_00482F70[0x20] = {
#include "sub_main/00482F70.inc"
};
char D_00482F90[0x20] = {
#include "sub_main/00482F90.inc"
};
char D_00482FB0[0x20] = {
#include "sub_main/00482FB0.inc"
};
char D_00482FD0[0x20] = {
#include "sub_main/00482FD0.inc"
};
char D_00482FF0[0x20] = {
#include "sub_main/00482FF0.inc"
};
char D_00483010[0x20] = {
#include "sub_main/00483010.inc"
};
char D_00483030[0x20] = {
#include "sub_main/00483030.inc"
};
char D_00483050[0x20] = {
#include "sub_main/00483050.inc"
};
char D_00483070[0x20] = {
#include "sub_main/00483070.inc"
};
char D_00483090[0x20] = {
#include "sub_main/00483090.inc"
};
char D_004830B0[0x20] = {
#include "sub_main/004830B0.inc"
};
char D_004830D0[0x20] = {
#include "sub_main/004830D0.inc"
};
char D_004830F0[0x20] = {
#include "sub_main/004830F0.inc"
};
char D_00483110[0x20] = {
#include "sub_main/00483110.inc"
};
char D_00483130[0x20] = {
#include "sub_main/00483130.inc"
};
char D_00483150[0x20] = {
#include "sub_main/00483150.inc"
};
char D_00483170[0x20] = {
#include "sub_main/00483170.inc"
};
char D_00483190[0x20] = {
#include "sub_main/00483190.inc"
};
char D_004831B0[0x20] = {
#include "sub_main/004831B0.inc"
};
char D_004831D0[0x20] = {
#include "sub_main/004831D0.inc"
};
char D_004831F0[0x20] = {
#include "sub_main/004831F0.inc"
};
char D_00483210[0x20] = {
#include "sub_main/00483210.inc"
};
char D_00483230[0x20] = {
#include "sub_main/00483230.inc"
};
char D_00483250[0x20] = {
#include "sub_main/00483250.inc"
};
char D_00483270[0x20] = {
#include "sub_main/00483270.inc"
};
char D_00483290[0x20] = {
#include "sub_main/00483290.inc"
};
char D_004832B0[0x20] = {
#include "sub_main/004832B0.inc"
};
char D_004832D0[0x20] = {
#include "sub_main/004832D0.inc"
};
char D_004832F0[0x20] = {
#include "sub_main/004832F0.inc"
};
char D_00483310[0x20] = {
#include "sub_main/00483310.inc"
};
char D_00483330[0x20] = {
#include "sub_main/00483330.inc"
};
char D_00483350[0x20] = {
#include "sub_main/00483350.inc"
};
char D_00483370[0x20] = {
#include "sub_main/00483370.inc"
};
char D_00483390[0x20] = {
#include "sub_main/00483390.inc"
};
char D_004833B0[0x20] = {
#include "sub_main/004833B0.inc"
};
char D_004833D0[0x20] = {
#include "sub_main/004833D0.inc"
};
char D_004833F0[0x20] = {
#include "sub_main/004833F0.inc"
};
char D_00483410[0x20] = {
#include "sub_main/00483410.inc"
};
char D_00483430[0x20] = {
#include "sub_main/00483430.inc"
};
char D_00483450[0x20] = {
#include "sub_main/00483450.inc"
};
char D_00483470[0x20] = {
#include "sub_main/00483470.inc"
};
char D_00483490[0x30] = {
#include "sub_main/00483490.inc"
};
char D_004834C0[0x20] = {
#include "sub_main/004834C0.inc"
};
char D_004834E0[0x20] = {
#include "sub_main/004834E0.inc"
};
char D_00483500[0x30] = {
#include "sub_main/00483500.inc"
};
char D_00483530[0x20] = {
#include "sub_main/00483530.inc"
};
char D_00483550[0x20] = {
#include "sub_main/00483550.inc"
};
char D_00483570[0x20] = {
#include "sub_main/00483570.inc"
};
char D_00483590[0x30] = {
#include "sub_main/00483590.inc"
};
char D_004835C0[0x20] = {
#include "sub_main/004835C0.inc"
};
char D_004835E0[0x20] = {
#include "sub_main/004835E0.inc"
};
char D_00483600[0x30] = {
#include "sub_main/00483600.inc"
};
char D_00483630[0x20] = {
#include "sub_main/00483630.inc"
};
char D_00483650[0x30] = {
#include "sub_main/00483650.inc"
};
char D_00483680[0x20] = {
#include "sub_main/00483680.inc"
};
char D_004836A0[0x20] = {
#include "sub_main/004836A0.inc"
};
char D_004836C0[0x20] = {
#include "sub_main/004836C0.inc"
};
char D_004836E0[0x20] = {
#include "sub_main/004836E0.inc"
};
char D_00483700[0x20] = {
#include "sub_main/00483700.inc"
};
char D_00483720[0x20] = {
#include "sub_main/00483720.inc"
};
char D_00483740[0x20] = {
#include "sub_main/00483740.inc"
};
char D_00483760[0x20] = {
#include "sub_main/00483760.inc"
};
char D_00483780[0x20] = {
#include "sub_main/00483780.inc"
};
char D_004837A0[0x20] = {
#include "sub_main/004837A0.inc"
};
char D_004837C0[0x20] = {
#include "sub_main/004837C0.inc"
};
char D_004837E0[0x20] = {
#include "sub_main/004837E0.inc"
};
char D_00483800[0x20] = {
#include "sub_main/00483800.inc"
};
char D_00483820[0x20] = {
#include "sub_main/00483820.inc"
};
char D_00483840[0x20] = {
#include "sub_main/00483840.inc"
};
char D_00483860[0x20] = {
#include "sub_main/00483860.inc"
};
char D_00483880[0x30] = {
#include "sub_main/00483880.inc"
};
char D_004838B0[0x30] = {
#include "sub_main/004838B0.inc"
};
char D_004838E0[0x20] = {
#include "sub_main/004838E0.inc"
};
char D_00483900[0x20] = {
#include "sub_main/00483900.inc"
};
char D_00483920[0x30] = {
#include "sub_main/00483920.inc"
};
char D_00483950[0x20] = {
#include "sub_main/00483950.inc"
};
char D_00483970[0x20] = {
#include "sub_main/00483970.inc"
};
char D_00483990[0x20] = {
#include "sub_main/00483990.inc"
};
char D_004839B0[0x20] = {
#include "sub_main/004839B0.inc"
};
char D_004839D0[0x20] = {
#include "sub_main/004839D0.inc"
};
char D_004839F0[0x20] = {
#include "sub_main/004839F0.inc"
};
char D_00483A10[0x20] = {
#include "sub_main/00483A10.inc"
};
char D_00483A30[0x20] = {
#include "sub_main/00483A30.inc"
};
char D_00483A50[0x20] = {
#include "sub_main/00483A50.inc"
};
char D_00483A70[0x20] = {
#include "sub_main/00483A70.inc"
};
char D_00483A90[0x20] = {
#include "sub_main/00483A90.inc"
};
char D_00483AB0[0x20] = {
#include "sub_main/00483AB0.inc"
};
char D_00483AD0[0x20] = {
#include "sub_main/00483AD0.inc"
};
char D_00483AF0[0x20] = {
#include "sub_main/00483AF0.inc"
};
char D_00483B10[0x20] = {
#include "sub_main/00483B10.inc"
};
char D_00483B30[0x20] = {
#include "sub_main/00483B30.inc"
};
char D_00483B50[0x20] = {
#include "sub_main/00483B50.inc"
};
char D_00483B70[0x20] = {
#include "sub_main/00483B70.inc"
};
char D_00483B90[0x20] = {
#include "sub_main/00483B90.inc"
};
char D_00483BB0[0x20] = {
#include "sub_main/00483BB0.inc"
};
char D_00483BD0[0x20] = {
#include "sub_main/00483BD0.inc"
};
char D_00483BF0[0x20] = {
#include "sub_main/00483BF0.inc"
};
char D_00483C10[0x20] = {
#include "sub_main/00483C10.inc"
};
char D_00483C30[0x20] = {
#include "sub_main/00483C30.inc"
};
char D_00483C50[0x20] = {
#include "sub_main/00483C50.inc"
};
char D_00483C70[0x20] = {
#include "sub_main/00483C70.inc"
};
char D_00483C90[0x20] = {
#include "sub_main/00483C90.inc"
};
char D_00483CB0[0x20] = {
#include "sub_main/00483CB0.inc"
};
char D_00483CD0[0x20] = {
#include "sub_main/00483CD0.inc"
};
char D_00483CF0[0x18] = {
#include "sub_main/00483CF0.inc"
};
char D_00483D08[13] = {
#include "sub_main/00483D08.inc"
};
char D_00483D20[0x20] = {
#include "sub_main/00483D20.inc"
};
char D_00483D40[0x20] = {
#include "sub_main/00483D40.inc"
};
char D_00483D60[0x20] = {
#include "sub_main/00483D60.inc"
};
char D_00483D80[0x20] = {
#include "sub_main/00483D80.inc"
};
char D_00483DA0[0x20] = {
#include "sub_main/00483DA0.inc"
};
char D_00483DC0[0x20] = {
#include "sub_main/00483DC0.inc"
};
char D_00483DE0[0x20] = {
#include "sub_main/00483DE0.inc"
};
char D_00483E00[0x20] = {
#include "sub_main/00483E00.inc"
};
char D_00483E20[0x20] = {
#include "sub_main/00483E20.inc"
};
char D_00483E40[0x20] = {
#include "sub_main/00483E40.inc"
};
char D_00483E60[0x20] = {
#include "sub_main/00483E60.inc"
};
char D_00483E80[0x20] = {
#include "sub_main/00483E80.inc"
};
char D_00483EA0[0x20] = {
#include "sub_main/00483EA0.inc"
};
char D_00483EC0[0x20] = {
#include "sub_main/00483EC0.inc"
};
char D_00483EE0[0x20] = {
#include "sub_main/00483EE0.inc"
};
char D_00483F00[0x10] = {
#include "sub_main/00483F00.inc"
};
char D_00483F10[0x20] = {
#include "sub_main/00483F10.inc"
};
char D_00483F30[0x20] = {
#include "sub_main/00483F30.inc"
};
char D_00483F50[0x20] = {
#include "sub_main/00483F50.inc"
};
char D_00483F70[0x20] = {
#include "sub_main/00483F70.inc"
};
char D_00483F90[0x20] = {
#include "sub_main/00483F90.inc"
};
char D_00483FB0[0x20] = {
#include "sub_main/00483FB0.inc"
};
char D_00483FD0[0x20] = {
#include "sub_main/00483FD0.inc"
};
char D_00483FF0[0x20] = {
#include "sub_main/00483FF0.inc"
};
char D_00484010[0x20] = {
#include "sub_main/00484010.inc"
};
char D_00484030[0x20] = {
#include "sub_main/00484030.inc"
};
char D_00484050[0x20] = {
#include "sub_main/00484050.inc"
};
char D_00484070[0x20] = {
#include "sub_main/00484070.inc"
};
char D_00484090[0x20] = {
#include "sub_main/00484090.inc"
};
char D_004840B0[0x20] = {
#include "sub_main/004840B0.inc"
};
char D_004840D0[0x20] = {
#include "sub_main/004840D0.inc"
};
char D_004840F0[0x20] = {
#include "sub_main/004840F0.inc"
};
char D_00484110[0x20] = {
#include "sub_main/00484110.inc"
};
char D_00484130[0x20] = {
#include "sub_main/00484130.inc"
};
char D_00484150[0x20] = {
#include "sub_main/00484150.inc"
};
char D_00484170[0x20] = {
#include "sub_main/00484170.inc"
};
char D_00484190[0x20] = {
#include "sub_main/00484190.inc"
};
char D_004841B0[0x20] = {
#include "sub_main/004841B0.inc"
};
char D_004841D0[0x20] = {
#include "sub_main/004841D0.inc"
};
char D_004841F0[0x20] = {
#include "sub_main/004841F0.inc"
};
char D_00484210[0x20] = {
#include "sub_main/00484210.inc"
};
char D_00484230[0x20] = {
#include "sub_main/00484230.inc"
};
char D_00484250[0x20] = {
#include "sub_main/00484250.inc"
};
char D_00484270[0x20] = {
#include "sub_main/00484270.inc"
};
char D_00484290[0x20] = {
#include "sub_main/00484290.inc"
};
char D_004842B0[0x20] = {
#include "sub_main/004842B0.inc"
};
char D_004842D0[0x20] = {
#include "sub_main/004842D0.inc"
};
char D_004842F0[0x20] = {
#include "sub_main/004842F0.inc"
};
char D_00484310[0x20] = {
#include "sub_main/00484310.inc"
};
char D_00484330[0x20] = {
#include "sub_main/00484330.inc"
};
char D_00484350[0x20] = {
#include "sub_main/00484350.inc"
};
char D_00484370[0x20] = {
#include "sub_main/00484370.inc"
};
char D_00484390[0x20] = {
#include "sub_main/00484390.inc"
};
char D_004843B0[0x20] = {
#include "sub_main/004843B0.inc"
};
char D_004843D0[0x20] = {
#include "sub_main/004843D0.inc"
};
char D_004843F0[0x20] = {
#include "sub_main/004843F0.inc"
};
char D_00484410[0x20] = {
#include "sub_main/00484410.inc"
};
char D_00484430[0x20] = {
#include "sub_main/00484430.inc"
};
char D_00484450[0x20] = {
#include "sub_main/00484450.inc"
};
char D_00484470[0x20] = {
#include "sub_main/00484470.inc"
};
char D_00484490[0x20] = {
#include "sub_main/00484490.inc"
};
char D_004844B0[0x20] = {
#include "sub_main/004844B0.inc"
};
char D_004844D0[0x20] = {
#include "sub_main/004844D0.inc"
};
char D_004844F0[0x20] = {
#include "sub_main/004844F0.inc"
};
char D_00484510[0x20] = {
#include "sub_main/00484510.inc"
};
char D_00484530[0x20] = {
#include "sub_main/00484530.inc"
};
char D_00484550[0x20] = {
#include "sub_main/00484550.inc"
};
char D_00484570[0x20] = {
#include "sub_main/00484570.inc"
};
char D_00484590[0x10] = {
#include "sub_main/00484590.inc"
};
char D_004845A0[0x20] = {
#include "sub_main/004845A0.inc"
};
char D_004845C0[0x20] = {
#include "sub_main/004845C0.inc"
};
char D_004845E0[0x20] = {
#include "sub_main/004845E0.inc"
};
char D_00484600[0x20] = {
#include "sub_main/00484600.inc"
};
char D_00484620[0x20] = {
#include "sub_main/00484620.inc"
};
char D_00484640[0x20] = {
#include "sub_main/00484640.inc"
};
char D_00484660[0x20] = {
#include "sub_main/00484660.inc"
};
char D_00484680[0x20] = {
#include "sub_main/00484680.inc"
};
char D_004846A0[0x20] = {
#include "sub_main/004846A0.inc"
};
char D_004846C0[0x20] = {
#include "sub_main/004846C0.inc"
};
char D_004846E0[0x20] = {
#include "sub_main/004846E0.inc"
};
char D_00484700[0x20] = {
#include "sub_main/00484700.inc"
};
char D_00484720[0x20] = {
#include "sub_main/00484720.inc"
};
char D_00484740[0x20] = {
#include "sub_main/00484740.inc"
};
char D_00484760[0x20] = {
#include "sub_main/00484760.inc"
};
char D_00484780[0x20] = {
#include "sub_main/00484780.inc"
};
char D_004847A0[0x20] = {
#include "sub_main/004847A0.inc"
};
char D_004847C0[0x20] = {
#include "sub_main/004847C0.inc"
};
char D_004847E0[0x20] = {
#include "sub_main/004847E0.inc"
};
char D_00484800[0x20] = {
#include "sub_main/00484800.inc"
};
char D_00484820[0x20] = {
#include "sub_main/00484820.inc"
};
char D_00484840[0x20] = {
#include "sub_main/00484840.inc"
};
char D_00484860[0x20] = {
#include "sub_main/00484860.inc"
};
char D_00484880[0x20] = {
#include "sub_main/00484880.inc"
};
char D_004848A0[0x20] = {
#include "sub_main/004848A0.inc"
};
char D_004848C0[0x20] = {
#include "sub_main/004848C0.inc"
};
char D_004848E0[0x20] = {
#include "sub_main/004848E0.inc"
};
char D_00484900[0x20] = {
#include "sub_main/00484900.inc"
};
char D_00484920[0x20] = {
#include "sub_main/00484920.inc"
};
char D_00484940[0x20] = {
#include "sub_main/00484940.inc"
};
char D_00484960[0x20] = {
#include "sub_main/00484960.inc"
};
char D_00484980[0x20] = {
#include "sub_main/00484980.inc"
};
char D_004849A0[0x10] = {
#include "sub_main/004849A0.inc"
};
char D_004849B0[0x20] = {
#include "sub_main/004849B0.inc"
};
char D_004849D0[0x20] = {
#include "sub_main/004849D0.inc"
};
char D_004849F0[0x20] = {
#include "sub_main/004849F0.inc"
};
char D_00484A10[0x20] = {
#include "sub_main/00484A10.inc"
};
char D_00484A30[0x20] = {
#include "sub_main/00484A30.inc"
};
char D_00484A50[0x20] = {
#include "sub_main/00484A50.inc"
};
char D_00484A70[0x20] = {
#include "sub_main/00484A70.inc"
};
char D_00484A90[0x20] = {
#include "sub_main/00484A90.inc"
};
char D_00484AB0[0x20] = {
#include "sub_main/00484AB0.inc"
};
char D_00484AD0[0x10] = {
#include "sub_main/00484AD0.inc"
};
char D_00484AE0[0x20] = {
#include "sub_main/00484AE0.inc"
};
char D_00484B00[0x20] = {
#include "sub_main/00484B00.inc"
};
char D_00484B20[0x10] = {
#include "sub_main/00484B20.inc"
};
char D_00484B30[0x20] = {
#include "sub_main/00484B30.inc"
};
char D_00484B50[0x20] = {
#include "sub_main/00484B50.inc"
};
char D_00484B70[0x20] = {
#include "sub_main/00484B70.inc"
};
char D_00484B90[0x20] = {
#include "sub_main/00484B90.inc"
};
char D_00484BB0[0x20] = {
#include "sub_main/00484BB0.inc"
};
char D_00484BD0[0x20] = {
#include "sub_main/00484BD0.inc"
};
char D_00484BF0[0x20] = {
#include "sub_main/00484BF0.inc"
};
char D_00484C10[0x20] = {
#include "sub_main/00484C10.inc"
};
char D_00484C30[0x20] = {
#include "sub_main/00484C30.inc"
};
char D_00484C50[0x20] = {
#include "sub_main/00484C50.inc"
};
char D_00484C70[0x20] = {
#include "sub_main/00484C70.inc"
};
char D_00484C90[0x20] = {
#include "sub_main/00484C90.inc"
};
char D_00484CB0[0x20] = {
#include "sub_main/00484CB0.inc"
};
char D_00484CD0[0x20] = {
#include "sub_main/00484CD0.inc"
};
char D_00484CF0[0x20] = {
#include "sub_main/00484CF0.inc"
};
char D_00484D10[0x20] = {
#include "sub_main/00484D10.inc"
};
char D_00484D30[0x10] = {
#include "sub_main/00484D30.inc"
};
char D_00484D40[0x20] = {
#include "sub_main/00484D40.inc"
};
char D_00484D60[0x20] = {
#include "sub_main/00484D60.inc"
};
char D_00484D80[0x20] = {
#include "sub_main/00484D80.inc"
};
char D_00484DA0[0x20] = {
#include "sub_main/00484DA0.inc"
};
char D_00484DC0[0x20] = {
#include "sub_main/00484DC0.inc"
};
char D_00484DE0[0x20] = {
#include "sub_main/00484DE0.inc"
};
char D_00484E00[0x20] = {
#include "sub_main/00484E00.inc"
};
char D_00484E20[0x20] = {
#include "sub_main/00484E20.inc"
};
char D_00484E40[0x20] = {
#include "sub_main/00484E40.inc"
};
char D_00484E60[0x20] = {
#include "sub_main/00484E60.inc"
};
char D_00484E80[0x20] = {
#include "sub_main/00484E80.inc"
};
char D_00484EA0[0x20] = {
#include "sub_main/00484EA0.inc"
};
char D_00484EC0[0x20] = {
#include "sub_main/00484EC0.inc"
};
char D_00484EE0[0x20] = {
#include "sub_main/00484EE0.inc"
};
char D_00484F00[0x20] = {
#include "sub_main/00484F00.inc"
};
char D_00484F20[0x20] = {
#include "sub_main/00484F20.inc"
};
char D_00484F40[0x20] = {
#include "sub_main/00484F40.inc"
};
char D_00484F60[0x20] = {
#include "sub_main/00484F60.inc"
};
char D_00484F80[0x20] = {
#include "sub_main/00484F80.inc"
};
char D_00484FA0[0x20] = {
#include "sub_main/00484FA0.inc"
};
char D_00484FC0[0x20] = {
#include "sub_main/00484FC0.inc"
};
char D_00484FE0[0x20] = {
#include "sub_main/00484FE0.inc"
};
char D_00485000[0x20] = {
#include "sub_main/00485000.inc"
};
char D_00485020[0x20] = {
#include "sub_main/00485020.inc"
};
char D_00485040[0x20] = {
#include "sub_main/00485040.inc"
};
char D_00485060[0x20] = {
#include "sub_main/00485060.inc"
};
char D_00485080[0x20] = {
#include "sub_main/00485080.inc"
};
char D_004850A0[0x20] = {
#include "sub_main/004850A0.inc"
};
char D_004850C0[0x20] = {
#include "sub_main/004850C0.inc"
};
char D_004850E0[0x20] = {
#include "sub_main/004850E0.inc"
};
char D_00485100[0x20] = {
#include "sub_main/00485100.inc"
};
char D_00485120[0x20] = {
#include "sub_main/00485120.inc"
};
char D_00485140[0x20] = {
#include "sub_main/00485140.inc"
};
char D_00485160[0x20] = {
#include "sub_main/00485160.inc"
};
char D_00485180[0x20] = {
#include "sub_main/00485180.inc"
};
char D_004851A0[0x20] = {
#include "sub_main/004851A0.inc"
};
char D_004851C0[0x20] = {
#include "sub_main/004851C0.inc"
};
char D_004851E0[0x20] = {
#include "sub_main/004851E0.inc"
};
char D_00485200[0x20] = {
#include "sub_main/00485200.inc"
};
char D_00485220[0x20] = {
#include "sub_main/00485220.inc"
};
char D_00485240[0x20] = {
#include "sub_main/00485240.inc"
};
char D_00485260[0x20] = {
#include "sub_main/00485260.inc"
};
char D_00485280[0x20] = {
#include "sub_main/00485280.inc"
};
char D_004852A0[0x20] = {
#include "sub_main/004852A0.inc"
};
char D_004852C0[0x20] = {
#include "sub_main/004852C0.inc"
};
char D_004852E0[0x20] = {
#include "sub_main/004852E0.inc"
};
char D_00485300[0x20] = {
#include "sub_main/00485300.inc"
};
char D_00485320[0x20] = {
#include "sub_main/00485320.inc"
};
char D_00485340[0x20] = {
#include "sub_main/00485340.inc"
};
char D_00485360[0x20] = {
#include "sub_main/00485360.inc"
};
char D_00485380[0x20] = {
#include "sub_main/00485380.inc"
};
char D_004853A0[0x20] = {
#include "sub_main/004853A0.inc"
};
char D_004853C0[0x20] = {
#include "sub_main/004853C0.inc"
};
char D_004853E0[0x20] = {
#include "sub_main/004853E0.inc"
};
char D_00485400[0x20] = {
#include "sub_main/00485400.inc"
};
char D_00485420[0x20] = {
#include "sub_main/00485420.inc"
};
char D_00485440[0x20] = {
#include "sub_main/00485440.inc"
};
char D_00485460[0x20] = {
#include "sub_main/00485460.inc"
};
char D_00485480[0x20] = {
#include "sub_main/00485480.inc"
};
char D_004854A0[0x20] = {
#include "sub_main/004854A0.inc"
};
char D_004854C0[0x20] = {
#include "sub_main/004854C0.inc"
};
char D_004854E0[0x20] = {
#include "sub_main/004854E0.inc"
};
char D_00485500[0x20] = {
#include "sub_main/00485500.inc"
};
char D_00485520[0x20] = {
#include "sub_main/00485520.inc"
};
char D_00485540[0x20] = {
#include "sub_main/00485540.inc"
};
char D_00485560[0x20] = {
#include "sub_main/00485560.inc"
};
char D_00485580[0x20] = {
#include "sub_main/00485580.inc"
};
char D_004855A0[0x20] = {
#include "sub_main/004855A0.inc"
};
char D_004855C0[0x20] = {
#include "sub_main/004855C0.inc"
};
char D_004855E0[0x20] = {
#include "sub_main/004855E0.inc"
};
char D_00485600[0x20] = {
#include "sub_main/00485600.inc"
};
char D_00485620[0x20] = {
#include "sub_main/00485620.inc"
};
char D_00485640[0x20] = {
#include "sub_main/00485640.inc"
};
char D_00485660[0x20] = {
#include "sub_main/00485660.inc"
};
char D_00485680[0x20] = {
#include "sub_main/00485680.inc"
};
char D_004856A0[0x10] = {
#include "sub_main/004856A0.inc"
};
char D_004856B0[0x20] = {
#include "sub_main/004856B0.inc"
};
char D_004856D0[0x20] = {
#include "sub_main/004856D0.inc"
};
char D_004856F0[0x20] = {
#include "sub_main/004856F0.inc"
};
char D_00485710[0x20] = {
#include "sub_main/00485710.inc"
};
char D_00485730[0x20] = {
#include "sub_main/00485730.inc"
};
char D_00485750[0x20] = {
#include "sub_main/00485750.inc"
};
char D_00485770[0x20] = {
#include "sub_main/00485770.inc"
};
char D_00485790[0x18] = {
#include "sub_main/00485790.inc"
};
char D_004857A8[15] = {
#include "sub_main/004857A8.inc"
};
char D_004857C0[0x20] = {
#include "sub_main/004857C0.inc"
};
char D_004857E0[0x20] = {
#include "sub_main/004857E0.inc"
};
char D_00485800[0x20] = {
#include "sub_main/00485800.inc"
};
char D_00485820[0x20] = {
#include "sub_main/00485820.inc"
};
char D_00485840[0x20] = {
#include "sub_main/00485840.inc"
};
char D_00485860[0x20] = {
#include "sub_main/00485860.inc"
};
char D_00485880[0x20] = {
#include "sub_main/00485880.inc"
};
char D_004858A0[0x20] = {
#include "sub_main/004858A0.inc"
};
char D_004858C0[0x20] = {
#include "sub_main/004858C0.inc"
};
char D_004858E0[0x20] = {
#include "sub_main/004858E0.inc"
};
char D_00485900[0x20] = {
#include "sub_main/00485900.inc"
};
char D_00485920[0x20] = {
#include "sub_main/00485920.inc"
};
char D_00485940[0x20] = {
#include "sub_main/00485940.inc"
};
char D_00485960[0x20] = {
#include "sub_main/00485960.inc"
};
char D_00485980[0x20] = {
#include "sub_main/00485980.inc"
};
char D_004859A0[0x20] = {
#include "sub_main/004859A0.inc"
};
char D_004859C0[0x20] = {
#include "sub_main/004859C0.inc"
};
char D_004859E0[0x20] = {
#include "sub_main/004859E0.inc"
};
char D_00485A00[0x20] = {
#include "sub_main/00485A00.inc"
};
char D_00485A20[0x20] = {
#include "sub_main/00485A20.inc"
};
char D_00485A40[0x20] = {
#include "sub_main/00485A40.inc"
};
char D_00485A60[0x20] = {
#include "sub_main/00485A60.inc"
};
char D_00485A80[0x20] = {
#include "sub_main/00485A80.inc"
};
char D_00485AA0[0x20] = {
#include "sub_main/00485AA0.inc"
};
char D_00485AC0[0x20] = {
#include "sub_main/00485AC0.inc"
};
char D_00485AE0[0x20] = {
#include "sub_main/00485AE0.inc"
};
char D_00485B00[0x20] = {
#include "sub_main/00485B00.inc"
};
char D_00485B20[0x20] = {
#include "sub_main/00485B20.inc"
};
char D_00485B40[0x20] = {
#include "sub_main/00485B40.inc"
};
char D_00485B60[0x20] = {
#include "sub_main/00485B60.inc"
};
char D_00485B80[0x20] = {
#include "sub_main/00485B80.inc"
};
char D_00485BA0[0x20] = {
#include "sub_main/00485BA0.inc"
};
char D_00485BC0[0x20] = {
#include "sub_main/00485BC0.inc"
};
char D_00485BE0[0x20] = {
#include "sub_main/00485BE0.inc"
};
char D_00485C00[0x20] = {
#include "sub_main/00485C00.inc"
};
char D_00485C20[0x20] = {
#include "sub_main/00485C20.inc"
};
char D_00485C40[0x20] = {
#include "sub_main/00485C40.inc"
};
char D_00485C60[0x20] = {
#include "sub_main/00485C60.inc"
};
char D_00485C80[0x20] = {
#include "sub_main/00485C80.inc"
};
char D_00485CA0[0x20] = {
#include "sub_main/00485CA0.inc"
};
char D_00485CC0[0x20] = {
#include "sub_main/00485CC0.inc"
};
char D_00485CE0[0x20] = {
#include "sub_main/00485CE0.inc"
};
char D_00485D00[0x20] = {
#include "sub_main/00485D00.inc"
};
char D_00485D20[0x20] = {
#include "sub_main/00485D20.inc"
};
char D_00485D40[0x20] = {
#include "sub_main/00485D40.inc"
};
char D_00485D60[0x20] = {
#include "sub_main/00485D60.inc"
};
char D_00485D80[0x20] = {
#include "sub_main/00485D80.inc"
};
char D_00485DA0[0x20] = {
#include "sub_main/00485DA0.inc"
};
char D_00485DC0[0x20] = {
#include "sub_main/00485DC0.inc"
};
char D_00485DE0[0x20] = {
#include "sub_main/00485DE0.inc"
};
char D_00485E00[0x20] = {
#include "sub_main/00485E00.inc"
};
char D_00485E20[0x20] = {
#include "sub_main/00485E20.inc"
};
char D_00485E40[0x20] = {
#include "sub_main/00485E40.inc"
};
char D_00485E60[0x20] = {
#include "sub_main/00485E60.inc"
};
char D_00485E80[0x10] = {
#include "sub_main/00485E80.inc"
};
char D_00485E90[0x20] = {
#include "sub_main/00485E90.inc"
};
char D_00485EB0[0x20] = {
#include "sub_main/00485EB0.inc"
};
char D_00485ED0[0x20] = {
#include "sub_main/00485ED0.inc"
};
char D_00485EF0[0x20] = {
#include "sub_main/00485EF0.inc"
};
char D_00485F10[0x20] = {
#include "sub_main/00485F10.inc"
};
char D_00485F30[0x20] = {
#include "sub_main/00485F30.inc"
};
char D_00485F50[0x20] = {
#include "sub_main/00485F50.inc"
};
char D_00485F70[0x20] = {
#include "sub_main/00485F70.inc"
};
char D_00485F90[0x20] = {
#include "sub_main/00485F90.inc"
};
char D_00485FB0[0x20] = {
#include "sub_main/00485FB0.inc"
};
char D_00485FD0[0x20] = {
#include "sub_main/00485FD0.inc"
};
char D_00485FF0[0x20] = {
#include "sub_main/00485FF0.inc"
};
char D_00486010[0x20] = {
#include "sub_main/00486010.inc"
};
char D_00486030[0x20] = {
#include "sub_main/00486030.inc"
};
char D_00486050[0x20] = {
#include "sub_main/00486050.inc"
};
char D_00486070[0x20] = {
#include "sub_main/00486070.inc"
};
char D_00486090[0x20] = {
#include "sub_main/00486090.inc"
};
char D_004860B0[0x20] = {
#include "sub_main/004860B0.inc"
};
char D_004860D0[0x20] = {
#include "sub_main/004860D0.inc"
};
char D_004860F0[0x20] = {
#include "sub_main/004860F0.inc"
};
char D_00486110[0x20] = {
#include "sub_main/00486110.inc"
};
char D_00486130[0x20] = {
#include "sub_main/00486130.inc"
};
char D_00486150[0x20] = {
#include "sub_main/00486150.inc"
};
char D_00486170[0x20] = {
#include "sub_main/00486170.inc"
};
char D_00486190[0x20] = {
#include "sub_main/00486190.inc"
};
char D_004861B0[0x20] = {
#include "sub_main/004861B0.inc"
};
char D_004861D0[0x20] = {
#include "sub_main/004861D0.inc"
};
char D_004861F0[0x20] = {
#include "sub_main/004861F0.inc"
};
char D_00486210[0x10] = {
#include "sub_main/00486210.inc"
};
char D_00486220[0x20] = {
#include "sub_main/00486220.inc"
};
char D_00486240[0x20] = {
#include "sub_main/00486240.inc"
};
char D_00486260[0x20] = {
#include "sub_main/00486260.inc"
};
char D_00486280[0x20] = {
#include "sub_main/00486280.inc"
};
char D_004862A0[0x20] = {
#include "sub_main/004862A0.inc"
};
char D_004862C0[0x20] = {
#include "sub_main/004862C0.inc"
};
char D_004862E0[0x20] = {
#include "sub_main/004862E0.inc"
};
char D_00486300[0x20] = {
#include "sub_main/00486300.inc"
};
char D_00486320[0x20] = {
#include "sub_main/00486320.inc"
};
char D_00486340[0x20] = {
#include "sub_main/00486340.inc"
};
char D_00486360[0x20] = {
#include "sub_main/00486360.inc"
};
char D_00486380[0x10] = {
#include "sub_main/00486380.inc"
};
char D_00486390[0x20] = {
#include "sub_main/00486390.inc"
};
char D_004863B0[0x20] = {
#include "sub_main/004863B0.inc"
};
char D_004863D0[0x20] = {
#include "sub_main/004863D0.inc"
};
char D_004863F0[0x20] = {
#include "sub_main/004863F0.inc"
};
char D_00486410[0x20] = {
#include "sub_main/00486410.inc"
};
char D_00486430[0x20] = {
#include "sub_main/00486430.inc"
};
char D_00486450[0x20] = {
#include "sub_main/00486450.inc"
};
char D_00486470[0x20] = {
#include "sub_main/00486470.inc"
};
char D_00486490[0x20] = {
#include "sub_main/00486490.inc"
};
char D_004864B0[0x20] = {
#include "sub_main/004864B0.inc"
};
char D_004864D0[0x20] = {
#include "sub_main/004864D0.inc"
};
char D_004864F0[0x20] = {
#include "sub_main/004864F0.inc"
};
char D_00486510[0x20] = {
#include "sub_main/00486510.inc"
};
char D_00486530[0x20] = {
#include "sub_main/00486530.inc"
};
char D_00486550[0x20] = {
#include "sub_main/00486550.inc"
};
char D_00486570[0x20] = {
#include "sub_main/00486570.inc"
};
char D_00486590[0x20] = {
#include "sub_main/00486590.inc"
};
char D_004865B0[0x20] = {
#include "sub_main/004865B0.inc"
};
char D_004865D0[0x20] = {
#include "sub_main/004865D0.inc"
};
char D_004865F0[0x20] = {
#include "sub_main/004865F0.inc"
};
char D_00486610[0x20] = {
#include "sub_main/00486610.inc"
};
char D_00486630[0x20] = {
#include "sub_main/00486630.inc"
};
char D_00486650[0x20] = {
#include "sub_main/00486650.inc"
};
char D_00486670[0x10] = {
#include "sub_main/00486670.inc"
};
char D_00486680[0x20] = {
#include "sub_main/00486680.inc"
};
char D_004866A0[0x20] = {
#include "sub_main/004866A0.inc"
};
char D_004866C0[0x10] = {
#include "sub_main/004866C0.inc"
};
char D_004866D0[0x20] = {
#include "sub_main/004866D0.inc"
};
char D_004866F0[0x20] = {
#include "sub_main/004866F0.inc"
};
char D_00486710[0x20] = {
#include "sub_main/00486710.inc"
};
char D_00486730[0x20] = {
#include "sub_main/00486730.inc"
};
char D_00486750[0x20] = {
#include "sub_main/00486750.inc"
};
char D_00486770[0x20] = {
#include "sub_main/00486770.inc"
};
char D_00486790[0x20] = {
#include "sub_main/00486790.inc"
};
char D_004867B0[0x20] = {
#include "sub_main/004867B0.inc"
};
char D_004867D0[0x20] = {
#include "sub_main/004867D0.inc"
};
char D_004867F0[0x20] = {
#include "sub_main/004867F0.inc"
};
char D_00486810[0x20] = {
#include "sub_main/00486810.inc"
};
char D_00486830[0x20] = {
#include "sub_main/00486830.inc"
};
char D_00486850[0x20] = {
#include "sub_main/00486850.inc"
};
char D_00486870[0x20] = {
#include "sub_main/00486870.inc"
};
char D_00486890[0x10] = {
#include "sub_main/00486890.inc"
};
char D_004868A0[0x20] = {
#include "sub_main/004868A0.inc"
};
char D_004868C0[0x20] = {
#include "sub_main/004868C0.inc"
};
char D_004868E0[0x20] = {
#include "sub_main/004868E0.inc"
};
char D_00486900[0x20] = {
#include "sub_main/00486900.inc"
};
char D_00486920[0x20] = {
#include "sub_main/00486920.inc"
};
char D_00486940[0x20] = {
#include "sub_main/00486940.inc"
};
char D_00486960[0x20] = {
#include "sub_main/00486960.inc"
};
char D_00486980[0x20] = {
#include "sub_main/00486980.inc"
};
char D_004869A0[0x20] = {
#include "sub_main/004869A0.inc"
};
char D_004869C0[0x20] = {
#include "sub_main/004869C0.inc"
};
char D_004869E0[0x20] = {
#include "sub_main/004869E0.inc"
};
char D_00486A00[0x20] = {
#include "sub_main/00486A00.inc"
};
char D_00486A20[0x20] = {
#include "sub_main/00486A20.inc"
};
char D_00486A40[0x20] = {
#include "sub_main/00486A40.inc"
};
char D_00486A60[0x20] = {
#include "sub_main/00486A60.inc"
};
char D_00486A80[0x20] = {
#include "sub_main/00486A80.inc"
};
char D_00486AA0[0x20] = {
#include "sub_main/00486AA0.inc"
};
char D_00486AC0[0x20] = {
#include "sub_main/00486AC0.inc"
};
char D_00486AE0[0x20] = {
#include "sub_main/00486AE0.inc"
};
char D_00486B00[0x20] = {
#include "sub_main/00486B00.inc"
};
char D_00486B20[0x20] = {
#include "sub_main/00486B20.inc"
};
char D_00486B40[0x20] = {
#include "sub_main/00486B40.inc"
};
char D_00486B60[0x20] = {
#include "sub_main/00486B60.inc"
};
char D_00486B80[0x20] = {
#include "sub_main/00486B80.inc"
};
char D_00486BA0[0x20] = {
#include "sub_main/00486BA0.inc"
};
char D_00486BC0[0x20] = {
#include "sub_main/00486BC0.inc"
};
char D_00486BE0[0x20] = {
#include "sub_main/00486BE0.inc"
};
char D_00486C00[0x20] = {
#include "sub_main/00486C00.inc"
};
char D_00486C20[0x20] = {
#include "sub_main/00486C20.inc"
};
char D_00486C40[0x20] = {
#include "sub_main/00486C40.inc"
};
char D_00486C60[0x20] = {
#include "sub_main/00486C60.inc"
};
char D_00486C80[0x20] = {
#include "sub_main/00486C80.inc"
};
char D_00486CA0[0x20] = {
#include "sub_main/00486CA0.inc"
};
char D_00486CC0[0x20] = {
#include "sub_main/00486CC0.inc"
};
char D_00486CE0[0x20] = {
#include "sub_main/00486CE0.inc"
};
char D_00486D00[0x20] = {
#include "sub_main/00486D00.inc"
};
char D_00486D20[0x20] = {
#include "sub_main/00486D20.inc"
};
char D_00486D40[0x20] = {
#include "sub_main/00486D40.inc"
};
char D_00486D60[0x20] = {
#include "sub_main/00486D60.inc"
};
char D_00486D80[0x20] = {
#include "sub_main/00486D80.inc"
};
char D_00486DA0[0x20] = {
#include "sub_main/00486DA0.inc"
};
char D_00486DC0[0x10] = {
#include "sub_main/00486DC0.inc"
};
char D_00486DD0[0x20] = {
#include "sub_main/00486DD0.inc"
};
char D_00486DF0[0x20] = {
#include "sub_main/00486DF0.inc"
};
char D_00486E10[0x20] = {
#include "sub_main/00486E10.inc"
};
char D_00486E30[0x20] = {
#include "sub_main/00486E30.inc"
};
char D_00486E50[0x20] = {
#include "sub_main/00486E50.inc"
};
char D_00486E70[0x20] = {
#include "sub_main/00486E70.inc"
};
char D_00486E90[0x20] = {
#include "sub_main/00486E90.inc"
};
char D_00486EB0[0x20] = {
#include "sub_main/00486EB0.inc"
};
char D_00486ED0[0x20] = {
#include "sub_main/00486ED0.inc"
};
char D_00486EF0[0x20] = {
#include "sub_main/00486EF0.inc"
};
char D_00486F10[0x20] = {
#include "sub_main/00486F10.inc"
};
char D_00486F30[0x20] = {
#include "sub_main/00486F30.inc"
};
char D_00486F50[0x20] = {
#include "sub_main/00486F50.inc"
};
char D_00486F70[0x20] = {
#include "sub_main/00486F70.inc"
};
char D_00486F90[0x20] = {
#include "sub_main/00486F90.inc"
};
char D_00486FB0[0x20] = {
#include "sub_main/00486FB0.inc"
};
char D_00486FD0[0x20] = {
#include "sub_main/00486FD0.inc"
};
char D_00486FF0[0x20] = {
#include "sub_main/00486FF0.inc"
};
char D_00487010[0x20] = {
#include "sub_main/00487010.inc"
};
char D_00487030[0x10] = {
#include "sub_main/00487030.inc"
};
char D_00487040[0x20] = {
#include "sub_main/00487040.inc"
};
char D_00487060[0x20] = {
#include "sub_main/00487060.inc"
};
char D_00487080[0x20] = {
#include "sub_main/00487080.inc"
};
char D_004870A0[0x20] = {
#include "sub_main/004870A0.inc"
};
char D_004870C0[0x20] = {
#include "sub_main/004870C0.inc"
};
char D_004870E0[0x20] = {
#include "sub_main/004870E0.inc"
};
char D_00487100[0x20] = {
#include "sub_main/00487100.inc"
};
char D_00487120[0x20] = {
#include "sub_main/00487120.inc"
};
char D_00487140[0x20] = {
#include "sub_main/00487140.inc"
};
char D_00487160[0x20] = {
#include "sub_main/00487160.inc"
};
char D_00487180[0x10] = {
#include "sub_main/00487180.inc"
};
char D_00487190[0x20] = {
#include "sub_main/00487190.inc"
};
char D_004871B0[0x20] = {
#include "sub_main/004871B0.inc"
};
char D_004871D0[0x20] = {
#include "sub_main/004871D0.inc"
};
char D_004871F0[0x20] = {
#include "sub_main/004871F0.inc"
};
char D_00487210[0x18] = {
#include "sub_main/00487210.inc"
};
char D_00487228[0xC] = {
#include "sub_main/00487228.inc"
};
char D_00487240[0x20] = {
#include "sub_main/00487240.inc"
};
char D_00487260[0x20] = {
#include "sub_main/00487260.inc"
};
char D_00487280[0x20] = {
#include "sub_main/00487280.inc"
};
char D_004872A0[0x20] = {
#include "sub_main/004872A0.inc"
};
char D_004872C0[0x20] = {
#include "sub_main/004872C0.inc"
};
char D_004872E0[0x20] = {
#include "sub_main/004872E0.inc"
};
char D_00487300[0x10] = {
#include "sub_main/00487300.inc"
};
char D_00487310[0x20] = {
#include "sub_main/00487310.inc"
};
char D_00487330[0x20] = {
#include "sub_main/00487330.inc"
};
char D_00487350[0x20] = {
#include "sub_main/00487350.inc"
};
char D_00487370[0x20] = {
#include "sub_main/00487370.inc"
};
char D_00487390[0x20] = {
#include "sub_main/00487390.inc"
};
char D_004873B0[0x20] = {
#include "sub_main/004873B0.inc"
};
char D_004873D0[0x20] = {
#include "sub_main/004873D0.inc"
};
char D_004873F0[0x20] = {
#include "sub_main/004873F0.inc"
};
char D_00487410[0x20] = {
#include "sub_main/00487410.inc"
};
char D_00487430[0x20] = {
#include "sub_main/00487430.inc"
};
char D_00487450[0x20] = {
#include "sub_main/00487450.inc"
};
char D_00487470[0x20] = {
#include "sub_main/00487470.inc"
};
char D_00487490[0x20] = {
#include "sub_main/00487490.inc"
};
char D_004874B0[0x20] = {
#include "sub_main/004874B0.inc"
};
char D_004874D0[0x20] = {
#include "sub_main/004874D0.inc"
};
char D_004874F0[0x20] = {
#include "sub_main/004874F0.inc"
};
char D_00487510[0x10] = {
#include "sub_main/00487510.inc"
};
char D_00487520[0x20] = {
#include "sub_main/00487520.inc"
};
char D_00487540[0x20] = {
#include "sub_main/00487540.inc"
};
char D_00487560[0x20] = {
#include "sub_main/00487560.inc"
};
char D_00487580[0x20] = {
#include "sub_main/00487580.inc"
};
char D_004875A0[0x20] = {
#include "sub_main/004875A0.inc"
};
char D_004875C0[0x20] = {
#include "sub_main/004875C0.inc"
};
char D_004875E0[0x20] = {
#include "sub_main/004875E0.inc"
};
char D_00487600[0x20] = {
#include "sub_main/00487600.inc"
};
char D_00487620[0x20] = {
#include "sub_main/00487620.inc"
};
char D_00487640[0x20] = {
#include "sub_main/00487640.inc"
};
char D_00487660[0x20] = {
#include "sub_main/00487660.inc"
};
char D_00487680[0x20] = {
#include "sub_main/00487680.inc"
};
char D_004876A0[0x20] = {
#include "sub_main/004876A0.inc"
};
char D_004876C0[0x20] = {
#include "sub_main/004876C0.inc"
};
char D_004876E0[0x20] = {
#include "sub_main/004876E0.inc"
};
char D_00487700[0x20] = {
#include "sub_main/00487700.inc"
};
char D_00487720[0x20] = {
#include "sub_main/00487720.inc"
};
char D_00487740[0x20] = {
#include "sub_main/00487740.inc"
};
char D_00487760[0x20] = {
#include "sub_main/00487760.inc"
};
char D_00487780[0x20] = {
#include "sub_main/00487780.inc"
};
char D_004877A0[0x20] = {
#include "sub_main/004877A0.inc"
};
char D_004877C0[0x20] = {
#include "sub_main/004877C0.inc"
};
char D_004877E0[0x20] = {
#include "sub_main/004877E0.inc"
};
char D_00487800[0x20] = {
#include "sub_main/00487800.inc"
};
char D_00487820[0x20] = {
#include "sub_main/00487820.inc"
};
char D_00487840[0x20] = {
#include "sub_main/00487840.inc"
};
char D_00487860[0x20] = {
#include "sub_main/00487860.inc"
};
char D_00487880[0x20] = {
#include "sub_main/00487880.inc"
};
char D_004878A0[0x20] = {
#include "sub_main/004878A0.inc"
};
char D_004878C0[0x20] = {
#include "sub_main/004878C0.inc"
};
char D_004878E0[0x20] = {
#include "sub_main/004878E0.inc"
};
char D_00487900[0x20] = {
#include "sub_main/00487900.inc"
};
char D_00487920[0x20] = {
#include "sub_main/00487920.inc"
};
char D_00487940[0x20] = {
#include "sub_main/00487940.inc"
};
char D_00487960[0x20] = {
#include "sub_main/00487960.inc"
};
char D_00487980[0x20] = {
#include "sub_main/00487980.inc"
};
char D_004879A0[0x20] = {
#include "sub_main/004879A0.inc"
};
char D_004879C0[0x20] = {
#include "sub_main/004879C0.inc"
};
char D_004879E0[0x20] = {
#include "sub_main/004879E0.inc"
};
char D_00487A00[0x20] = {
#include "sub_main/00487A00.inc"
};
char D_00487A20[0x20] = {
#include "sub_main/00487A20.inc"
};
char D_00487A40[0x20] = {
#include "sub_main/00487A40.inc"
};
char D_00487A60[0x20] = {
#include "sub_main/00487A60.inc"
};
char D_00487A80[0x20] = {
#include "sub_main/00487A80.inc"
};
char D_00487AA0[0x20] = {
#include "sub_main/00487AA0.inc"
};
char D_00487AC0[0x20] = {
#include "sub_main/00487AC0.inc"
};
char D_00487AE0[0x20] = {
#include "sub_main/00487AE0.inc"
};
char D_00487B00[0x20] = {
#include "sub_main/00487B00.inc"
};
char D_00487B20[0x20] = {
#include "sub_main/00487B20.inc"
};
char D_00487B40[0x20] = {
#include "sub_main/00487B40.inc"
};
char D_00487B60[0x20] = {
#include "sub_main/00487B60.inc"
};
char D_00487B80[0x20] = {
#include "sub_main/00487B80.inc"
};
char D_00487BA0[0x20] = {
#include "sub_main/00487BA0.inc"
};
char D_00487BC0[0x20] = {
#include "sub_main/00487BC0.inc"
};
char D_00487BE0[0x20] = {
#include "sub_main/00487BE0.inc"
};
char D_00487C00[0x20] = {
#include "sub_main/00487C00.inc"
};
char D_00487C20[0x20] = {
#include "sub_main/00487C20.inc"
};
char D_00487C40[0x20] = {
#include "sub_main/00487C40.inc"
};
char D_00487C60[0x10] = {
#include "sub_main/00487C60.inc"
};
char D_00487C70[0x20] = {
#include "sub_main/00487C70.inc"
};
char D_00487C90[0x20] = {
#include "sub_main/00487C90.inc"
};
char D_00487CB0[0x20] = {
#include "sub_main/00487CB0.inc"
};
char D_00487CD0[0x20] = {
#include "sub_main/00487CD0.inc"
};
char D_00487CF0[0x20] = {
#include "sub_main/00487CF0.inc"
};
char D_00487D10[0x20] = {
#include "sub_main/00487D10.inc"
};
char D_00487D30[0x20] = {
#include "sub_main/00487D30.inc"
};
char D_00487D50[0x20] = {
#include "sub_main/00487D50.inc"
};
char D_00487D70[0x20] = {
#include "sub_main/00487D70.inc"
};
char D_00487D90[0x20] = {
#include "sub_main/00487D90.inc"
};
char D_00487DB0[0x20] = {
#include "sub_main/00487DB0.inc"
};
char D_00487DD0[0x20] = {
#include "sub_main/00487DD0.inc"
};
char D_00487DF0[0x20] = {
#include "sub_main/00487DF0.inc"
};
char D_00487E10[0x20] = {
#include "sub_main/00487E10.inc"
};
char D_00487E30[0x20] = {
#include "sub_main/00487E30.inc"
};
char D_00487E50[0x20] = {
#include "sub_main/00487E50.inc"
};
char D_00487E70[0x20] = {
#include "sub_main/00487E70.inc"
};
char D_00487E90[0x20] = {
#include "sub_main/00487E90.inc"
};
char D_00487EB0[0x20] = {
#include "sub_main/00487EB0.inc"
};
char D_00487ED0[0x20] = {
#include "sub_main/00487ED0.inc"
};
char D_00487EF0[0x20] = {
#include "sub_main/00487EF0.inc"
};
char D_00487F10[0x20] = {
#include "sub_main/00487F10.inc"
};
char D_00487F30[0x20] = {
#include "sub_main/00487F30.inc"
};
char D_00487F50[0x20] = {
#include "sub_main/00487F50.inc"
};
char D_00487F70[0x20] = {
#include "sub_main/00487F70.inc"
};
char D_00487F90[0x20] = {
#include "sub_main/00487F90.inc"
};
char D_00487FB0[0x10] = {
#include "sub_main/00487FB0.inc"
};
char D_00487FC0[0x20] = {
#include "sub_main/00487FC0.inc"
};
char D_00487FE0[0x20] = {
#include "sub_main/00487FE0.inc"
};
char D_00488000[0x20] = {
#include "sub_main/00488000.inc"
};
char D_00488020[0x20] = {
#include "sub_main/00488020.inc"
};
char D_00488040[0x20] = {
#include "sub_main/00488040.inc"
};
char D_00488060[0x20] = {
#include "sub_main/00488060.inc"
};
char D_00488080[0x20] = {
#include "sub_main/00488080.inc"
};
char D_004880A0[0x20] = {
#include "sub_main/004880A0.inc"
};
char D_004880C0[0x20] = {
#include "sub_main/004880C0.inc"
};
char D_004880E0[0x20] = {
#include "sub_main/004880E0.inc"
};
char D_00488100[0x20] = {
#include "sub_main/00488100.inc"
};
char D_00488120[0x20] = {
#include "sub_main/00488120.inc"
};
char D_00488140[0x20] = {
#include "sub_main/00488140.inc"
};
char D_00488160[0x20] = {
#include "sub_main/00488160.inc"
};
char D_00488180[0x20] = {
#include "sub_main/00488180.inc"
};
char D_004881A0[0x20] = {
#include "sub_main/004881A0.inc"
};
char D_004881C0[0x20] = {
#include "sub_main/004881C0.inc"
};
char D_004881E0[0x20] = {
#include "sub_main/004881E0.inc"
};
char D_00488200[0x20] = {
#include "sub_main/00488200.inc"
};
char D_00488220[0x20] = {
#include "sub_main/00488220.inc"
};
char D_00488240[0x20] = {
#include "sub_main/00488240.inc"
};
char D_00488260[0x20] = {
#include "sub_main/00488260.inc"
};
char D_00488280[0x20] = {
#include "sub_main/00488280.inc"
};
char D_004882A0[0x20] = {
#include "sub_main/004882A0.inc"
};
char D_004882C0[0x20] = {
#include "sub_main/004882C0.inc"
};
char D_004882E0[0x20] = {
#include "sub_main/004882E0.inc"
};
char D_00488300[0x20] = {
#include "sub_main/00488300.inc"
};
char D_00488320[0x20] = {
#include "sub_main/00488320.inc"
};
char D_00488340[0x20] = {
#include "sub_main/00488340.inc"
};
char D_00488360[0x20] = {
#include "sub_main/00488360.inc"
};
char D_00488380[0x20] = {
#include "sub_main/00488380.inc"
};
char D_004883A0[0x20] = {
#include "sub_main/004883A0.inc"
};
char D_004883C0[0x20] = {
#include "sub_main/004883C0.inc"
};
char D_004883E0[0x20] = {
#include "sub_main/004883E0.inc"
};
char D_00488400[0x20] = {
#include "sub_main/00488400.inc"
};
char D_00488420[0x10] = {
#include "sub_main/00488420.inc"
};
char D_00488430[0x20] = {
#include "sub_main/00488430.inc"
};
char D_00488450[0x20] = {
#include "sub_main/00488450.inc"
};
char D_00488470[0x20] = {
#include "sub_main/00488470.inc"
};
char D_00488490[0x20] = {
#include "sub_main/00488490.inc"
};
char D_004884B0[0x20] = {
#include "sub_main/004884B0.inc"
};
char D_004884D0[0x20] = {
#include "sub_main/004884D0.inc"
};
char D_004884F0[0x20] = {
#include "sub_main/004884F0.inc"
};
char D_00488510[0x20] = {
#include "sub_main/00488510.inc"
};
char D_00488530[0x20] = {
#include "sub_main/00488530.inc"
};
char D_00488550[0x20] = {
#include "sub_main/00488550.inc"
};
char D_00488570[0x20] = {
#include "sub_main/00488570.inc"
};
char D_00488590[0x20] = {
#include "sub_main/00488590.inc"
};
char D_004885B0[0x20] = {
#include "sub_main/004885B0.inc"
};
char D_004885D0[0x20] = {
#include "sub_main/004885D0.inc"
};
char D_004885F0[0x20] = {
#include "sub_main/004885F0.inc"
};
char D_00488610[0x20] = {
#include "sub_main/00488610.inc"
};
char D_00488630[0x20] = {
#include "sub_main/00488630.inc"
};
char D_00488650[0x20] = {
#include "sub_main/00488650.inc"
};
char D_00488670[0x20] = {
#include "sub_main/00488670.inc"
};
char D_00488690[0x20] = {
#include "sub_main/00488690.inc"
};
char D_004886B0[0x20] = {
#include "sub_main/004886B0.inc"
};
char D_004886D0[0x20] = {
#include "sub_main/004886D0.inc"
};
char D_004886F0[0x20] = {
#include "sub_main/004886F0.inc"
};
char D_00488710[0x20] = {
#include "sub_main/00488710.inc"
};
char D_00488730[0x20] = {
#include "sub_main/00488730.inc"
};
char D_00488750[0x20] = {
#include "sub_main/00488750.inc"
};
char D_00488770[0x20] = {
#include "sub_main/00488770.inc"
};
char D_00488790[0x20] = {
#include "sub_main/00488790.inc"
};
char D_004887B0[0x20] = {
#include "sub_main/004887B0.inc"
};
char D_004887D0[0x20] = {
#include "sub_main/004887D0.inc"
};
char D_004887F0[0x20] = {
#include "sub_main/004887F0.inc"
};
char D_00488810[0x20] = {
#include "sub_main/00488810.inc"
};
char D_00488830[0x20] = {
#include "sub_main/00488830.inc"
};
char D_00488850[0x20] = {
#include "sub_main/00488850.inc"
};
char D_00488870[0x20] = {
#include "sub_main/00488870.inc"
};
char D_00488890[0x20] = {
#include "sub_main/00488890.inc"
};
char D_004888B0[0x20] = {
#include "sub_main/004888B0.inc"
};
char D_004888D0[0x20] = {
#include "sub_main/004888D0.inc"
};
char D_004888F0[0x20] = {
#include "sub_main/004888F0.inc"
};
char D_00488910[0x20] = {
#include "sub_main/00488910.inc"
};
char D_00488930[0x20] = {
#include "sub_main/00488930.inc"
};
char D_00488950[0x20] = {
#include "sub_main/00488950.inc"
};
char D_00488970[0x20] = {
#include "sub_main/00488970.inc"
};
char D_00488990[0x20] = {
#include "sub_main/00488990.inc"
};
char D_004889B0[0x20] = {
#include "sub_main/004889B0.inc"
};
char D_004889D0[0x20] = {
#include "sub_main/004889D0.inc"
};
char D_004889F0[0x20] = {
#include "sub_main/004889F0.inc"
};
char D_00488A10[0x20] = {
#include "sub_main/00488A10.inc"
};
char D_00488A30[0x20] = {
#include "sub_main/00488A30.inc"
};
char D_00488A50[0x20] = {
#include "sub_main/00488A50.inc"
};
char D_00488A70[0x20] = {
#include "sub_main/00488A70.inc"
};
char D_00488A90[0x20] = {
#include "sub_main/00488A90.inc"
};
char D_00488AB0[0x20] = {
#include "sub_main/00488AB0.inc"
};
char D_00488AD0[0x20] = {
#include "sub_main/00488AD0.inc"
};
char D_00488AF0[0x20] = {
#include "sub_main/00488AF0.inc"
};
char D_00488B10[0x20] = {
#include "sub_main/00488B10.inc"
};
char D_00488B30[0x20] = {
#include "sub_main/00488B30.inc"
};
char D_00488B50[0x20] = {
#include "sub_main/00488B50.inc"
};
char D_00488B70[0x20] = {
#include "sub_main/00488B70.inc"
};
char D_00488B90[0x20] = {
#include "sub_main/00488B90.inc"
};
char D_00488BB0[0x20] = {
#include "sub_main/00488BB0.inc"
};
char D_00488BD0[0x20] = {
#include "sub_main/00488BD0.inc"
};
char D_00488BF0[0x20] = {
#include "sub_main/00488BF0.inc"
};
char D_00488C10[0x20] = {
#include "sub_main/00488C10.inc"
};
char D_00488C30[0x20] = {
#include "sub_main/00488C30.inc"
};
char D_00488C50[0x20] = {
#include "sub_main/00488C50.inc"
};
char D_00488C70[0x20] = {
#include "sub_main/00488C70.inc"
};
char D_00488C90[0x20] = {
#include "sub_main/00488C90.inc"
};
char D_00488CB0[0x20] = {
#include "sub_main/00488CB0.inc"
};
char D_00488CD0[0x20] = {
#include "sub_main/00488CD0.inc"
};
char D_00488CF0[0x20] = {
#include "sub_main/00488CF0.inc"
};
char D_00488D10[0x20] = {
#include "sub_main/00488D10.inc"
};
char D_00488D30[0x20] = {
#include "sub_main/00488D30.inc"
};
char D_00488D50[0x20] = {
#include "sub_main/00488D50.inc"
};
char D_00488D70[0x20] = {
#include "sub_main/00488D70.inc"
};
char D_00488D90[0x20] = {
#include "sub_main/00488D90.inc"
};
char D_00488DB0[0x20] = {
#include "sub_main/00488DB0.inc"
};
char D_00488DD0[0x20] = {
#include "sub_main/00488DD0.inc"
};
char D_00488DF0[0x20] = {
#include "sub_main/00488DF0.inc"
};
char D_00488E10[0x20] = {
#include "sub_main/00488E10.inc"
};
char D_00488E30[0x20] = {
#include "sub_main/00488E30.inc"
};
char D_00488E50[0x20] = {
#include "sub_main/00488E50.inc"
};
char D_00488E70[0x20] = {
#include "sub_main/00488E70.inc"
};
char D_00488E90[0x20] = {
#include "sub_main/00488E90.inc"
};
char D_00488EB0[0x20] = {
#include "sub_main/00488EB0.inc"
};
char D_00488ED0[0x20] = {
#include "sub_main/00488ED0.inc"
};
char D_00488EF0[0x20] = {
#include "sub_main/00488EF0.inc"
};
char D_00488F10[0x20] = {
#include "sub_main/00488F10.inc"
};
char D_00488F30[0x20] = {
#include "sub_main/00488F30.inc"
};
char D_00488F50[0x20] = {
#include "sub_main/00488F50.inc"
};
char D_00488F70[0x20] = {
#include "sub_main/00488F70.inc"
};
char D_00488F90[0x20] = {
#include "sub_main/00488F90.inc"
};
char D_00488FB0[0x20] = {
#include "sub_main/00488FB0.inc"
};
char D_00488FD0[0x20] = {
#include "sub_main/00488FD0.inc"
};
char D_00488FF0[0x20] = {
#include "sub_main/00488FF0.inc"
};
char D_00489010[0x20] = {
#include "sub_main/00489010.inc"
};
char D_00489030[0x20] = {
#include "sub_main/00489030.inc"
};
char D_00489050[0x20] = {
#include "sub_main/00489050.inc"
};
char D_00489070[0x20] = {
#include "sub_main/00489070.inc"
};
char D_00489090[0x20] = {
#include "sub_main/00489090.inc"
};
char D_004890B0[0x20] = {
#include "sub_main/004890B0.inc"
};
char D_004890D0[0x20] = {
#include "sub_main/004890D0.inc"
};
char D_004890F0[0x20] = {
#include "sub_main/004890F0.inc"
};
char D_00489110[0x20] = {
#include "sub_main/00489110.inc"
};
char D_00489130[0x20] = {
#include "sub_main/00489130.inc"
};
char D_00489150[0x20] = {
#include "sub_main/00489150.inc"
};
char D_00489170[0x20] = {
#include "sub_main/00489170.inc"
};
char D_00489190[0x20] = {
#include "sub_main/00489190.inc"
};
char D_004891B0[0x20] = {
#include "sub_main/004891B0.inc"
};
char D_004891D0[0x20] = {
#include "sub_main/004891D0.inc"
};
char D_004891F0[0x20] = {
#include "sub_main/004891F0.inc"
};
char D_00489210[0x20] = {
#include "sub_main/00489210.inc"
};
char D_00489230[0x10] = {
#include "sub_main/00489230.inc"
};
char D_00489240[0x20] = {
#include "sub_main/00489240.inc"
};
char D_00489260[0x20] = {
#include "sub_main/00489260.inc"
};
char D_00489280[0x20] = {
#include "sub_main/00489280.inc"
};
char D_004892A0[0x20] = {
#include "sub_main/004892A0.inc"
};
char D_004892C0[0x20] = {
#include "sub_main/004892C0.inc"
};
char D_004892E0[0x20] = {
#include "sub_main/004892E0.inc"
};
char D_00489300[0x20] = {
#include "sub_main/00489300.inc"
};
char D_00489320[0x20] = {
#include "sub_main/00489320.inc"
};
char D_00489340[0x20] = {
#include "sub_main/00489340.inc"
};
char D_00489360[0x20] = {
#include "sub_main/00489360.inc"
};
char D_00489380[0x20] = {
#include "sub_main/00489380.inc"
};
char D_004893A0[0x20] = {
#include "sub_main/004893A0.inc"
};
char D_004893C0[0x20] = {
#include "sub_main/004893C0.inc"
};
char D_004893E0[0x20] = {
#include "sub_main/004893E0.inc"
};
char D_00489400[0x20] = {
#include "sub_main/00489400.inc"
};
char D_00489420[0x20] = {
#include "sub_main/00489420.inc"
};
char D_00489440[0x20] = {
#include "sub_main/00489440.inc"
};
char D_00489460[0x20] = {
#include "sub_main/00489460.inc"
};
char D_00489480[0x20] = {
#include "sub_main/00489480.inc"
};
char D_004894A0[0x20] = {
#include "sub_main/004894A0.inc"
};
char D_004894C0[0x20] = {
#include "sub_main/004894C0.inc"
};
char D_004894E0[0x20] = {
#include "sub_main/004894E0.inc"
};
char D_00489500[0x20] = {
#include "sub_main/00489500.inc"
};
char D_00489520[0x20] = {
#include "sub_main/00489520.inc"
};
char D_00489540[0x20] = {
#include "sub_main/00489540.inc"
};
char D_00489560[0x20] = {
#include "sub_main/00489560.inc"
};
char D_00489580[0x20] = {
#include "sub_main/00489580.inc"
};
char D_004895A0[0x20] = {
#include "sub_main/004895A0.inc"
};
char D_004895C0[0x20] = {
#include "sub_main/004895C0.inc"
};
char D_004895E0[0x20] = {
#include "sub_main/004895E0.inc"
};
char D_00489600[0x20] = {
#include "sub_main/00489600.inc"
};
char D_00489620[0x20] = {
#include "sub_main/00489620.inc"
};
char D_00489640[0x20] = {
#include "sub_main/00489640.inc"
};
char D_00489660[0x20] = {
#include "sub_main/00489660.inc"
};
char D_00489680[0x20] = {
#include "sub_main/00489680.inc"
};
char D_004896A0[0x20] = {
#include "sub_main/004896A0.inc"
};
char D_004896C0[0x20] = {
#include "sub_main/004896C0.inc"
};
char D_004896E0[0x20] = {
#include "sub_main/004896E0.inc"
};
char D_00489700[0x20] = {
#include "sub_main/00489700.inc"
};
char D_00489720[0x20] = {
#include "sub_main/00489720.inc"
};
char D_00489740[0x20] = {
#include "sub_main/00489740.inc"
};
char D_00489760[0x20] = {
#include "sub_main/00489760.inc"
};
char D_00489780[0x20] = {
#include "sub_main/00489780.inc"
};
char D_004897A0[0x20] = {
#include "sub_main/004897A0.inc"
};
char D_004897C0[0x20] = {
#include "sub_main/004897C0.inc"
};
char D_004897E0[0x20] = {
#include "sub_main/004897E0.inc"
};
char D_00489800[0x20] = {
#include "sub_main/00489800.inc"
};
char D_00489820[0x10] = {
#include "sub_main/00489820.inc"
};
char D_00489830[0x20] = {
#include "sub_main/00489830.inc"
};
char D_00489850[0x20] = {
#include "sub_main/00489850.inc"
};
char D_00489870[0x20] = {
#include "sub_main/00489870.inc"
};
char D_00489890[0x20] = {
#include "sub_main/00489890.inc"
};
char D_004898B0[0x20] = {
#include "sub_main/004898B0.inc"
};
char D_004898D0[0x20] = {
#include "sub_main/004898D0.inc"
};
char D_004898F0[0x20] = {
#include "sub_main/004898F0.inc"
};
char D_00489910[0x20] = {
#include "sub_main/00489910.inc"
};
char D_00489930[0x20] = {
#include "sub_main/00489930.inc"
};
char D_00489950[0x20] = {
#include "sub_main/00489950.inc"
};
char D_00489970[0x10] = {
#include "sub_main/00489970.inc"
};
char D_00489980[0x20] = {
#include "sub_main/00489980.inc"
};
char D_004899A0[0x20] = {
#include "sub_main/004899A0.inc"
};
char D_004899C0[0x20] = {
#include "sub_main/004899C0.inc"
};
char D_004899E0[0x20] = {
#include "sub_main/004899E0.inc"
};
char D_00489A00[0x20] = {
#include "sub_main/00489A00.inc"
};
char D_00489A20[0x20] = {
#include "sub_main/00489A20.inc"
};
char D_00489A40[0x18] = {
#include "sub_main/00489A40.inc"
};
char D_00489A58[13] = {
#include "sub_main/00489A58.inc"
};
char D_00489A70[0x20] = {
#include "sub_main/00489A70.inc"
};
char D_00489A90[0x20] = {
#include "sub_main/00489A90.inc"
};
char D_00489AB0[0x20] = {
#include "sub_main/00489AB0.inc"
};
char D_00489AD0[0x20] = {
#include "sub_main/00489AD0.inc"
};
char D_00489AF0[0x20] = {
#include "sub_main/00489AF0.inc"
};
char D_00489B10[0x20] = {
#include "sub_main/00489B10.inc"
};
char D_00489B30[0x20] = {
#include "sub_main/00489B30.inc"
};
char D_00489B50[0x20] = {
#include "sub_main/00489B50.inc"
};
char D_00489B70[0x20] = {
#include "sub_main/00489B70.inc"
};
char D_00489B90[0x20] = {
#include "sub_main/00489B90.inc"
};
char D_00489BB0[0x20] = {
#include "sub_main/00489BB0.inc"
};
char D_00489BD0[0x20] = {
#include "sub_main/00489BD0.inc"
};
char D_00489BF0[0x20] = {
#include "sub_main/00489BF0.inc"
};
char D_00489C10[0x20] = {
#include "sub_main/00489C10.inc"
};
char D_00489C30[0x20] = {
#include "sub_main/00489C30.inc"
};
char D_00489C50[0x20] = {
#include "sub_main/00489C50.inc"
};
char D_00489C70[0x20] = {
#include "sub_main/00489C70.inc"
};
char D_00489C90[0x20] = {
#include "sub_main/00489C90.inc"
};
char D_00489CB0[0x20] = {
#include "sub_main/00489CB0.inc"
};
char D_00489CD0[0x20] = {
#include "sub_main/00489CD0.inc"
};
char D_00489CF0[0x20] = {
#include "sub_main/00489CF0.inc"
};
char D_00489D10[0x20] = {
#include "sub_main/00489D10.inc"
};
char D_00489D30[0x20] = {
#include "sub_main/00489D30.inc"
};
char D_00489D50[0x20] = {
#include "sub_main/00489D50.inc"
};
char D_00489D70[0x20] = {
#include "sub_main/00489D70.inc"
};
char D_00489D90[0x20] = {
#include "sub_main/00489D90.inc"
};
char D_00489DB0[0x20] = {
#include "sub_main/00489DB0.inc"
};
char D_00489DD0[0x20] = {
#include "sub_main/00489DD0.inc"
};
char D_00489DF0[0x20] = {
#include "sub_main/00489DF0.inc"
};
char D_00489E10[0x20] = {
#include "sub_main/00489E10.inc"
};
char D_00489E30[0x20] = {
#include "sub_main/00489E30.inc"
};
char D_00489E50[0x20] = {
#include "sub_main/00489E50.inc"
};
char D_00489E70[0x20] = {
#include "sub_main/00489E70.inc"
};
char D_00489E90[0x20] = {
#include "sub_main/00489E90.inc"
};
char D_00489EB0[0x20] = {
#include "sub_main/00489EB0.inc"
};
char D_00489ED0[0x20] = {
#include "sub_main/00489ED0.inc"
};
char D_00489EF0[0x20] = {
#include "sub_main/00489EF0.inc"
};
char D_00489F10[0x20] = {
#include "sub_main/00489F10.inc"
};
char D_00489F30[0x10] = {
#include "sub_main/00489F30.inc"
};
char D_00489F40[0x20] = {
#include "sub_main/00489F40.inc"
};
char D_00489F60[0x20] = {
#include "sub_main/00489F60.inc"
};
char D_00489F80[0x20] = {
#include "sub_main/00489F80.inc"
};
char D_00489FA0[0x20] = {
#include "sub_main/00489FA0.inc"
};
char D_00489FC0[0x20] = {
#include "sub_main/00489FC0.inc"
};
char D_00489FE0[0x20] = {
#include "sub_main/00489FE0.inc"
};
char D_0048A000[0x20] = {
#include "sub_main/0048A000.inc"
};
char D_0048A020[0x20] = {
#include "sub_main/0048A020.inc"
};
char D_0048A040[0x20] = {
#include "sub_main/0048A040.inc"
};
char D_0048A060[0x20] = {
#include "sub_main/0048A060.inc"
};
char D_0048A080[0x20] = {
#include "sub_main/0048A080.inc"
};
char D_0048A0A0[0x20] = {
#include "sub_main/0048A0A0.inc"
};
char D_0048A0C0[0x20] = {
#include "sub_main/0048A0C0.inc"
};
char D_0048A0E0[0x20] = {
#include "sub_main/0048A0E0.inc"
};
char D_0048A100[0x20] = {
#include "sub_main/0048A100.inc"
};
char D_0048A120[0x20] = {
#include "sub_main/0048A120.inc"
};
char D_0048A140[0x20] = {
#include "sub_main/0048A140.inc"
};
char D_0048A160[0x20] = {
#include "sub_main/0048A160.inc"
};
char D_0048A180[0x20] = {
#include "sub_main/0048A180.inc"
};
char D_0048A1A0[0x20] = {
#include "sub_main/0048A1A0.inc"
};
char D_0048A1C0[0x20] = {
#include "sub_main/0048A1C0.inc"
};
char D_0048A1E0[0x20] = {
#include "sub_main/0048A1E0.inc"
};
char D_0048A200[0x20] = {
#include "sub_main/0048A200.inc"
};
char D_0048A220[0x20] = {
#include "sub_main/0048A220.inc"
};
char D_0048A240[0x20] = {
#include "sub_main/0048A240.inc"
};
char D_0048A260[0x20] = {
#include "sub_main/0048A260.inc"
};
char D_0048A280[0x20] = {
#include "sub_main/0048A280.inc"
};
char D_0048A2A0[0x20] = {
#include "sub_main/0048A2A0.inc"
};
char D_0048A2C0[0x20] = {
#include "sub_main/0048A2C0.inc"
};
char D_0048A2E0[0x20] = {
#include "sub_main/0048A2E0.inc"
};
char D_0048A300[0x20] = {
#include "sub_main/0048A300.inc"
};
char D_0048A320[0x20] = {
#include "sub_main/0048A320.inc"
};
char D_0048A340[0x20] = {
#include "sub_main/0048A340.inc"
};
char D_0048A360[0x20] = {
#include "sub_main/0048A360.inc"
};
char D_0048A380[0x20] = {
#include "sub_main/0048A380.inc"
};
char D_0048A3A0[0x20] = {
#include "sub_main/0048A3A0.inc"
};
char D_0048A3C0[0x20] = {
#include "sub_main/0048A3C0.inc"
};
char D_0048A3E0[0x10] = {
#include "sub_main/0048A3E0.inc"
};
char D_0048A3F0[0x20] = {
#include "sub_main/0048A3F0.inc"
};
char D_0048A410[0x20] = {
#include "sub_main/0048A410.inc"
};
char D_0048A430[0x20] = {
#include "sub_main/0048A430.inc"
};
char D_0048A450[0x10] = {
#include "sub_main/0048A450.inc"
};
char D_0048A460[0x20] = {
#include "sub_main/0048A460.inc"
};
char D_0048A480[0x20] = {
#include "sub_main/0048A480.inc"
};
char D_0048A4A0[0x20] = {
#include "sub_main/0048A4A0.inc"
};
char D_0048A4C0[0x20] = {
#include "sub_main/0048A4C0.inc"
};
char D_0048A4E0[0x20] = {
#include "sub_main/0048A4E0.inc"
};
char D_0048A500[0x20] = {
#include "sub_main/0048A500.inc"
};
char D_0048A520[0x20] = {
#include "sub_main/0048A520.inc"
};
char D_0048A540[0x20] = {
#include "sub_main/0048A540.inc"
};
char D_0048A560[0x20] = {
#include "sub_main/0048A560.inc"
};
char D_0048A580[0x20] = {
#include "sub_main/0048A580.inc"
};
char D_0048A5A0[0x20] = {
#include "sub_main/0048A5A0.inc"
};
char D_0048A5C0[0x20] = {
#include "sub_main/0048A5C0.inc"
};
char D_0048A5E0[0x20] = {
#include "sub_main/0048A5E0.inc"
};
char D_0048A600[0x20] = {
#include "sub_main/0048A600.inc"
};
char D_0048A620[0x20] = {
#include "sub_main/0048A620.inc"
};
char D_0048A640[0x20] = {
#include "sub_main/0048A640.inc"
};
char D_0048A660[0x20] = {
#include "sub_main/0048A660.inc"
};
char D_0048A680[0x20] = {
#include "sub_main/0048A680.inc"
};
char D_0048A6A0[0x20] = {
#include "sub_main/0048A6A0.inc"
};
char D_0048A6C0[0x20] = {
#include "sub_main/0048A6C0.inc"
};
char D_0048A6E0[0x20] = {
#include "sub_main/0048A6E0.inc"
};
char D_0048A700[0x20] = {
#include "sub_main/0048A700.inc"
};
char D_0048A720[0x20] = {
#include "sub_main/0048A720.inc"
};
char D_0048A740[0x20] = {
#include "sub_main/0048A740.inc"
};
char D_0048A760[0x20] = {
#include "sub_main/0048A760.inc"
};
char D_0048A780[0x20] = {
#include "sub_main/0048A780.inc"
};
char D_0048A7A0[0x20] = {
#include "sub_main/0048A7A0.inc"
};
char D_0048A7C0[0x20] = {
#include "sub_main/0048A7C0.inc"
};
char D_0048A7E0[0x20] = {
#include "sub_main/0048A7E0.inc"
};
char D_0048A800[0x20] = {
#include "sub_main/0048A800.inc"
};
char D_0048A820[0x20] = {
#include "sub_main/0048A820.inc"
};
char D_0048A840[0x20] = {
#include "sub_main/0048A840.inc"
};
char D_0048A860[0x20] = {
#include "sub_main/0048A860.inc"
};
char D_0048A880[0x20] = {
#include "sub_main/0048A880.inc"
};
char D_0048A8A0[0x20] = {
#include "sub_main/0048A8A0.inc"
};
char D_0048A8C0[0x20] = {
#include "sub_main/0048A8C0.inc"
};
char D_0048A8E0[0x20] = {
#include "sub_main/0048A8E0.inc"
};
char D_0048A900[0x20] = {
#include "sub_main/0048A900.inc"
};
char D_0048A920[0x20] = {
#include "sub_main/0048A920.inc"
};
char D_0048A940[0x20] = {
#include "sub_main/0048A940.inc"
};
char D_0048A960[0x20] = {
#include "sub_main/0048A960.inc"
};
char D_0048A980[0x20] = {
#include "sub_main/0048A980.inc"
};
char D_0048A9A0[0x20] = {
#include "sub_main/0048A9A0.inc"
};
char D_0048A9C0[0x20] = {
#include "sub_main/0048A9C0.inc"
};
char D_0048A9E0[0x20] = {
#include "sub_main/0048A9E0.inc"
};
char D_0048AA00[0x20] = {
#include "sub_main/0048AA00.inc"
};
char D_0048AA20[0x20] = {
#include "sub_main/0048AA20.inc"
};
char D_0048AA40[0x20] = {
#include "sub_main/0048AA40.inc"
};
char D_0048AA60[0x20] = {
#include "sub_main/0048AA60.inc"
};
char D_0048AA80[0x20] = {
#include "sub_main/0048AA80.inc"
};
char D_0048AAA0[0x20] = {
#include "sub_main/0048AAA0.inc"
};
char D_0048AAC0[0x20] = {
#include "sub_main/0048AAC0.inc"
};
char D_0048AAE0[0x10] = {
#include "sub_main/0048AAE0.inc"
};
char D_0048AAF0[0x20] = {
#include "sub_main/0048AAF0.inc"
};
char D_0048AB10[0x20] = {
#include "sub_main/0048AB10.inc"
};
char D_0048AB30[0x20] = {
#include "sub_main/0048AB30.inc"
};
char D_0048AB50[0x20] = {
#include "sub_main/0048AB50.inc"
};
char D_0048AB70[0x20] = {
#include "sub_main/0048AB70.inc"
};
char D_0048AB90[0x20] = {
#include "sub_main/0048AB90.inc"
};
char D_0048ABB0[0x20] = {
#include "sub_main/0048ABB0.inc"
};
char D_0048ABD0[0x20] = {
#include "sub_main/0048ABD0.inc"
};
char D_0048ABF0[0x20] = {
#include "sub_main/0048ABF0.inc"
};
char D_0048AC10[0x20] = {
#include "sub_main/0048AC10.inc"
};
char D_0048AC30[0x20] = {
#include "sub_main/0048AC30.inc"
};
char D_0048AC50[0x20] = {
#include "sub_main/0048AC50.inc"
};
char D_0048AC70[0x20] = {
#include "sub_main/0048AC70.inc"
};
char D_0048AC90[0x20] = {
#include "sub_main/0048AC90.inc"
};
char D_0048ACB0[0x20] = {
#include "sub_main/0048ACB0.inc"
};
char D_0048ACD0[0x20] = {
#include "sub_main/0048ACD0.inc"
};
char D_0048ACF0[0x20] = {
#include "sub_main/0048ACF0.inc"
};
char D_0048AD10[0x20] = {
#include "sub_main/0048AD10.inc"
};
char D_0048AD30[0x20] = {
#include "sub_main/0048AD30.inc"
};
char D_0048AD50[0x20] = {
#include "sub_main/0048AD50.inc"
};
char D_0048AD70[0x20] = {
#include "sub_main/0048AD70.inc"
};
char D_0048AD90[0x20] = {
#include "sub_main/0048AD90.inc"
};
char D_0048ADB0[0x20] = {
#include "sub_main/0048ADB0.inc"
};
char D_0048ADD0[0x20] = {
#include "sub_main/0048ADD0.inc"
};
char D_0048ADF0[0x20] = {
#include "sub_main/0048ADF0.inc"
};
char D_0048AE10[0x20] = {
#include "sub_main/0048AE10.inc"
};
char D_0048AE30[0x20] = {
#include "sub_main/0048AE30.inc"
};
char D_0048AE50[0x20] = {
#include "sub_main/0048AE50.inc"
};
char D_0048AE70[0x20] = {
#include "sub_main/0048AE70.inc"
};
char D_0048AE90[0x20] = {
#include "sub_main/0048AE90.inc"
};
char D_0048AEB0[0x20] = {
#include "sub_main/0048AEB0.inc"
};
char D_0048AED0[0x20] = {
#include "sub_main/0048AED0.inc"
};
char D_0048AEF0[0x20] = {
#include "sub_main/0048AEF0.inc"
};
char D_0048AF10[0x20] = {
#include "sub_main/0048AF10.inc"
};
char D_0048AF30[0x20] = {
#include "sub_main/0048AF30.inc"
};
char D_0048AF50[0x20] = {
#include "sub_main/0048AF50.inc"
};
char D_0048AF70[0x20] = {
#include "sub_main/0048AF70.inc"
};
char D_0048AF90[0x20] = {
#include "sub_main/0048AF90.inc"
};
char D_0048AFB0[0x20] = {
#include "sub_main/0048AFB0.inc"
};
char D_0048AFD0[0x20] = {
#include "sub_main/0048AFD0.inc"
};
char D_0048AFF0[0x20] = {
#include "sub_main/0048AFF0.inc"
};
char D_0048B010[0x20] = {
#include "sub_main/0048B010.inc"
};
char D_0048B030[0x20] = {
#include "sub_main/0048B030.inc"
};
char D_0048B050[0x20] = {
#include "sub_main/0048B050.inc"
};
char D_0048B070[0x20] = {
#include "sub_main/0048B070.inc"
};
char D_0048B090[0x20] = {
#include "sub_main/0048B090.inc"
};
char D_0048B0B0[0x20] = {
#include "sub_main/0048B0B0.inc"
};
char D_0048B0D0[0x20] = {
#include "sub_main/0048B0D0.inc"
};
char D_0048B0F0[0x20] = {
#include "sub_main/0048B0F0.inc"
};
char D_0048B110[0x20] = {
#include "sub_main/0048B110.inc"
};
char D_0048B130[0x20] = {
#include "sub_main/0048B130.inc"
};
char D_0048B150[0x20] = {
#include "sub_main/0048B150.inc"
};
char D_0048B170[0x20] = {
#include "sub_main/0048B170.inc"
};
char D_0048B190[0x20] = {
#include "sub_main/0048B190.inc"
};
char D_0048B1B0[0x20] = {
#include "sub_main/0048B1B0.inc"
};
char D_0048B1D0[0x20] = {
#include "sub_main/0048B1D0.inc"
};
char D_0048B1F0[0x20] = {
#include "sub_main/0048B1F0.inc"
};
char D_0048B210[0x20] = {
#include "sub_main/0048B210.inc"
};
char D_0048B230[0x20] = {
#include "sub_main/0048B230.inc"
};
char D_0048B250[0x20] = {
#include "sub_main/0048B250.inc"
};
char D_0048B270[0x20] = {
#include "sub_main/0048B270.inc"
};
char D_0048B290[0x20] = {
#include "sub_main/0048B290.inc"
};
char D_0048B2B0[0x20] = {
#include "sub_main/0048B2B0.inc"
};
char D_0048B2D0[0x20] = {
#include "sub_main/0048B2D0.inc"
};
char D_0048B2F0[0x20] = {
#include "sub_main/0048B2F0.inc"
};
char D_0048B310[0x20] = {
#include "sub_main/0048B310.inc"
};
char D_0048B330[0x20] = {
#include "sub_main/0048B330.inc"
};
char D_0048B350[0x10] = {
#include "sub_main/0048B350.inc"
};
char D_0048B360[0x20] = {
#include "sub_main/0048B360.inc"
};
char D_0048B380[0x20] = {
#include "sub_main/0048B380.inc"
};
char D_0048B3A0[0x20] = {
#include "sub_main/0048B3A0.inc"
};
char D_0048B3C0[0x20] = {
#include "sub_main/0048B3C0.inc"
};
char D_0048B3E0[0x20] = {
#include "sub_main/0048B3E0.inc"
};
char D_0048B400[0x20] = {
#include "sub_main/0048B400.inc"
};
char D_0048B420[0x20] = {
#include "sub_main/0048B420.inc"
};
char D_0048B440[0x20] = {
#include "sub_main/0048B440.inc"
};
char D_0048B460[0x20] = {
#include "sub_main/0048B460.inc"
};
char D_0048B480[0x20] = {
#include "sub_main/0048B480.inc"
};
char D_0048B4A0[0x20] = {
#include "sub_main/0048B4A0.inc"
};
char D_0048B4C0[0x20] = {
#include "sub_main/0048B4C0.inc"
};
char D_0048B4E0[0x20] = {
#include "sub_main/0048B4E0.inc"
};
char D_0048B500[0x20] = {
#include "sub_main/0048B500.inc"
};
char D_0048B520[0x20] = {
#include "sub_main/0048B520.inc"
};
char D_0048B540[0x20] = {
#include "sub_main/0048B540.inc"
};
char D_0048B560[0x20] = {
#include "sub_main/0048B560.inc"
};
char D_0048B580[0x20] = {
#include "sub_main/0048B580.inc"
};
char D_0048B5A0[0x20] = {
#include "sub_main/0048B5A0.inc"
};
char D_0048B5C0[0x20] = {
#include "sub_main/0048B5C0.inc"
};
char D_0048B5E0[0x20] = {
#include "sub_main/0048B5E0.inc"
};
char D_0048B600[0x20] = {
#include "sub_main/0048B600.inc"
};
char D_0048B620[0x20] = {
#include "sub_main/0048B620.inc"
};
char D_0048B640[0x20] = {
#include "sub_main/0048B640.inc"
};
char D_0048B660[0x20] = {
#include "sub_main/0048B660.inc"
};
char D_0048B680[0x20] = {
#include "sub_main/0048B680.inc"
};
char D_0048B6A0[0x20] = {
#include "sub_main/0048B6A0.inc"
};
char D_0048B6C0[0x20] = {
#include "sub_main/0048B6C0.inc"
};
char D_0048B6E0[0x20] = {
#include "sub_main/0048B6E0.inc"
};
char D_0048B700[0x20] = {
#include "sub_main/0048B700.inc"
};
char D_0048B720[0x20] = {
#include "sub_main/0048B720.inc"
};
char D_0048B740[0x20] = {
#include "sub_main/0048B740.inc"
};
char D_0048B760[0x20] = {
#include "sub_main/0048B760.inc"
};
char D_0048B780[0x20] = {
#include "sub_main/0048B780.inc"
};
char D_0048B7A0[0x20] = {
#include "sub_main/0048B7A0.inc"
};
char D_0048B7C0[0x20] = {
#include "sub_main/0048B7C0.inc"
};
char D_0048B7E0[0x20] = {
#include "sub_main/0048B7E0.inc"
};
char D_0048B800[0x20] = {
#include "sub_main/0048B800.inc"
};
char D_0048B820[0x20] = {
#include "sub_main/0048B820.inc"
};
char D_0048B840[0x20] = {
#include "sub_main/0048B840.inc"
};
char D_0048B860[0x20] = {
#include "sub_main/0048B860.inc"
};
char D_0048B880[0x20] = {
#include "sub_main/0048B880.inc"
};
char D_0048B8A0[0x20] = {
#include "sub_main/0048B8A0.inc"
};
char D_0048B8C0[0x10] = {
#include "sub_main/0048B8C0.inc"
};
char D_0048B8D0[0x20] = {
#include "sub_main/0048B8D0.inc"
};
char D_0048B8F0[0x20] = {
#include "sub_main/0048B8F0.inc"
};
char D_0048B910[0x20] = {
#include "sub_main/0048B910.inc"
};
char D_0048B930[0x20] = {
#include "sub_main/0048B930.inc"
};
char D_0048B950[0x20] = {
#include "sub_main/0048B950.inc"
};
char D_0048B970[0x20] = {
#include "sub_main/0048B970.inc"
};
char D_0048B990[0x20] = {
#include "sub_main/0048B990.inc"
};
char D_0048B9B0[0x20] = {
#include "sub_main/0048B9B0.inc"
};
char D_0048B9D0[0x20] = {
#include "sub_main/0048B9D0.inc"
};
char D_0048B9F0[0x20] = {
#include "sub_main/0048B9F0.inc"
};
char D_0048BA10[0x20] = {
#include "sub_main/0048BA10.inc"
};
char D_0048BA30[0x20] = {
#include "sub_main/0048BA30.inc"
};
char D_0048BA50[0x20] = {
#include "sub_main/0048BA50.inc"
};
char D_0048BA70[0x20] = {
#include "sub_main/0048BA70.inc"
};
char D_0048BA90[0x20] = {
#include "sub_main/0048BA90.inc"
};
char D_0048BAB0[0x20] = {
#include "sub_main/0048BAB0.inc"
};
char D_0048BAD0[0x20] = {
#include "sub_main/0048BAD0.inc"
};
char D_0048BAF0[0x20] = {
#include "sub_main/0048BAF0.inc"
};
char D_0048BB10[0x20] = {
#include "sub_main/0048BB10.inc"
};
char D_0048BB30[0x20] = {
#include "sub_main/0048BB30.inc"
};
char D_0048BB50[0x20] = {
#include "sub_main/0048BB50.inc"
};
char D_0048BB70[0x20] = {
#include "sub_main/0048BB70.inc"
};
char D_0048BB90[0x20] = {
#include "sub_main/0048BB90.inc"
};
char D_0048BBB0[0x20] = {
#include "sub_main/0048BBB0.inc"
};
char D_0048BBD0[0x20] = {
#include "sub_main/0048BBD0.inc"
};
char D_0048BBF0[0x20] = {
#include "sub_main/0048BBF0.inc"
};
char D_0048BC10[0x20] = {
#include "sub_main/0048BC10.inc"
};
char D_0048BC30[0x20] = {
#include "sub_main/0048BC30.inc"
};
char D_0048BC50[0x20] = {
#include "sub_main/0048BC50.inc"
};
char D_0048BC70[0x20] = {
#include "sub_main/0048BC70.inc"
};
char D_0048BC90[0x20] = {
#include "sub_main/0048BC90.inc"
};
char D_0048BCB0[0x10] = {
#include "sub_main/0048BCB0.inc"
};
char D_0048BCC0[0x20] = {
#include "sub_main/0048BCC0.inc"
};
char D_0048BCE0[0x20] = {
#include "sub_main/0048BCE0.inc"
};
char D_0048BD00[0x20] = {
#include "sub_main/0048BD00.inc"
};
char D_0048BD20[0x20] = {
#include "sub_main/0048BD20.inc"
};
char D_0048BD40[0x20] = {
#include "sub_main/0048BD40.inc"
};
char D_0048BD60[0x20] = {
#include "sub_main/0048BD60.inc"
};
char D_0048BD80[0x20] = {
#include "sub_main/0048BD80.inc"
};
char D_0048BDA0[0x20] = {
#include "sub_main/0048BDA0.inc"
};
char D_0048BDC0[0x10] = {
#include "sub_main/0048BDC0.inc"
};
char D_0048BDD0[0x20] = {
#include "sub_main/0048BDD0.inc"
};
char D_0048BDF0[0x20] = {
#include "sub_main/0048BDF0.inc"
};
char D_0048BE10[0x20] = {
#include "sub_main/0048BE10.inc"
};
char D_0048BE30[0x20] = {
#include "sub_main/0048BE30.inc"
};
char D_0048BE50[0x20] = {
#include "sub_main/0048BE50.inc"
};
char D_0048BE70[0x20] = {
#include "sub_main/0048BE70.inc"
};
char D_0048BE90[0x20] = {
#include "sub_main/0048BE90.inc"
};
char D_0048BEB0[0x20] = {
#include "sub_main/0048BEB0.inc"
};
char D_0048BED0[0x20] = {
#include "sub_main/0048BED0.inc"
};
char D_0048BEF0[0x20] = {
#include "sub_main/0048BEF0.inc"
};
char D_0048BF10[0x20] = {
#include "sub_main/0048BF10.inc"
};
char D_0048BF30[0x20] = {
#include "sub_main/0048BF30.inc"
};
char D_0048BF50[0x10] = {
#include "sub_main/0048BF50.inc"
};
char D_0048BF60[0x20] = {
#include "sub_main/0048BF60.inc"
};
char D_0048BF80[0x20] = {
#include "sub_main/0048BF80.inc"
};
char D_0048BFA0[0x20] = {
#include "sub_main/0048BFA0.inc"
};
char D_0048BFC0[0x20] = {
#include "sub_main/0048BFC0.inc"
};
char D_0048BFE0[0x20] = {
#include "sub_main/0048BFE0.inc"
};
char D_0048C000[0x20] = {
#include "sub_main/0048C000.inc"
};
char D_0048C020[0x20] = {
#include "sub_main/0048C020.inc"
};
char D_0048C040[0x20] = {
#include "sub_main/0048C040.inc"
};
char D_0048C060[0x20] = {
#include "sub_main/0048C060.inc"
};
char D_0048C080[0x20] = {
#include "sub_main/0048C080.inc"
};
char D_0048C0A0[0x20] = {
#include "sub_main/0048C0A0.inc"
};
char D_0048C0C0[0x20] = {
#include "sub_main/0048C0C0.inc"
};
char D_0048C0E0[0x20] = {
#include "sub_main/0048C0E0.inc"
};
char D_0048C100[0x20] = {
#include "sub_main/0048C100.inc"
};
char D_0048C120[0x20] = {
#include "sub_main/0048C120.inc"
};
char D_0048C140[0x20] = {
#include "sub_main/0048C140.inc"
};
char D_0048C160[0x20] = {
#include "sub_main/0048C160.inc"
};
char D_0048C180[0x20] = {
#include "sub_main/0048C180.inc"
};
char D_0048C1A0[0x20] = {
#include "sub_main/0048C1A0.inc"
};
char D_0048C1C0[0x10] = {
#include "sub_main/0048C1C0.inc"
};
char D_0048C1D0[0x20] = {
#include "sub_main/0048C1D0.inc"
};
char D_0048C1F0[0x10] = {
#include "sub_main/0048C1F0.inc"
};
char D_0048C200[0x20] = {
#include "sub_main/0048C200.inc"
};
char D_0048C220[0x20] = {
#include "sub_main/0048C220.inc"
};
char D_0048C240[0x20] = {
#include "sub_main/0048C240.inc"
};
char D_0048C260[0x20] = {
#include "sub_main/0048C260.inc"
};
char D_0048C280[0x20] = {
#include "sub_main/0048C280.inc"
};
char D_0048C2A0[0x20] = {
#include "sub_main/0048C2A0.inc"
};
char D_0048C2C0[0x20] = {
#include "sub_main/0048C2C0.inc"
};
char D_0048C2E0[0x20] = {
#include "sub_main/0048C2E0.inc"
};
char D_0048C300[0x20] = {
#include "sub_main/0048C300.inc"
};
char D_0048C320[0x20] = {
#include "sub_main/0048C320.inc"
};
char D_0048C340[0x20] = {
#include "sub_main/0048C340.inc"
};
char D_0048C360[0x10] = {
#include "sub_main/0048C360.inc"
};
char D_0048C370[0x20] = {
#include "sub_main/0048C370.inc"
};
char D_0048C390[0x20] = {
#include "sub_main/0048C390.inc"
};
char D_0048C3B0[0x20] = {
#include "sub_main/0048C3B0.inc"
};
char D_0048C3D0[0x20] = {
#include "sub_main/0048C3D0.inc"
};
char D_0048C3F0[0x20] = {
#include "sub_main/0048C3F0.inc"
};
char D_0048C410[0x20] = {
#include "sub_main/0048C410.inc"
};
char D_0048C430[0x20] = {
#include "sub_main/0048C430.inc"
};
char D_0048C450[0x20] = {
#include "sub_main/0048C450.inc"
};
char D_0048C470[0x20] = {
#include "sub_main/0048C470.inc"
};
char D_0048C490[0x20] = {
#include "sub_main/0048C490.inc"
};
char D_0048C4B0[0x10] = {
#include "sub_main/0048C4B0.inc"
};
char D_0048C4C0[0x20] = {
#include "sub_main/0048C4C0.inc"
};
char D_0048C4E0[0x20] = {
#include "sub_main/0048C4E0.inc"
};
char D_0048C500[0x20] = {
#include "sub_main/0048C500.inc"
};
char D_0048C520[0x20] = {
#include "sub_main/0048C520.inc"
};
char D_0048C540[0x20] = {
#include "sub_main/0048C540.inc"
};
char D_0048C560[0x20] = {
#include "sub_main/0048C560.inc"
};
char D_0048C580[0x20] = {
#include "sub_main/0048C580.inc"
};
char D_0048C5A0[0x20] = {
#include "sub_main/0048C5A0.inc"
};
char D_0048C5C0[0x20] = {
#include "sub_main/0048C5C0.inc"
};
char D_0048C5E0[0x20] = {
#include "sub_main/0048C5E0.inc"
};
char D_0048C600[0x20] = {
#include "sub_main/0048C600.inc"
};
char D_0048C620[0x20] = {
#include "sub_main/0048C620.inc"
};
char D_0048C640[0x20] = {
#include "sub_main/0048C640.inc"
};
char D_0048C660[0x20] = {
#include "sub_main/0048C660.inc"
};
char D_0048C680[0x20] = {
#include "sub_main/0048C680.inc"
};
char D_0048C6A0[0x20] = {
#include "sub_main/0048C6A0.inc"
};
char D_0048C6C0[0x20] = {
#include "sub_main/0048C6C0.inc"
};
char D_0048C6E0[0x20] = {
#include "sub_main/0048C6E0.inc"
};
char D_0048C700[0x20] = {
#include "sub_main/0048C700.inc"
};
char D_0048C720[0x20] = {
#include "sub_main/0048C720.inc"
};
char D_0048C740[0x20] = {
#include "sub_main/0048C740.inc"
};
char D_0048C760[0x20] = {
#include "sub_main/0048C760.inc"
};
char D_0048C780[0x20] = {
#include "sub_main/0048C780.inc"
};
char D_0048C7A0[0x20] = {
#include "sub_main/0048C7A0.inc"
};
char D_0048C7C0[0x20] = {
#include "sub_main/0048C7C0.inc"
};
char D_0048C7E0[0x20] = {
#include "sub_main/0048C7E0.inc"
};
char D_0048C800[0x20] = {
#include "sub_main/0048C800.inc"
};
char D_0048C820[0x20] = {
#include "sub_main/0048C820.inc"
};
char D_0048C840[0x20] = {
#include "sub_main/0048C840.inc"
};
char D_0048C860[0x18] = {
#include "sub_main/0048C860.inc"
};
char D_0048C878[15] = {
#include "sub_main/0048C878.inc"
};
char D_0048C890[0x20] = {
#include "sub_main/0048C890.inc"
};
char D_0048C8B0[0x20] = {
#include "sub_main/0048C8B0.inc"
};
char D_0048C8D0[0x20] = {
#include "sub_main/0048C8D0.inc"
};
char D_0048C8F0[0x20] = {
#include "sub_main/0048C8F0.inc"
};
char D_0048C910[0x20] = {
#include "sub_main/0048C910.inc"
};
char D_0048C930[0x20] = {
#include "sub_main/0048C930.inc"
};
char D_0048C950[0x20] = {
#include "sub_main/0048C950.inc"
};
char D_0048C970[0x20] = {
#include "sub_main/0048C970.inc"
};
char D_0048C990[0x20] = {
#include "sub_main/0048C990.inc"
};
char D_0048C9B0[0x20] = {
#include "sub_main/0048C9B0.inc"
};
char D_0048C9D0[0x20] = {
#include "sub_main/0048C9D0.inc"
};
char D_0048C9F0[0x20] = {
#include "sub_main/0048C9F0.inc"
};
char D_0048CA10[0x20] = {
#include "sub_main/0048CA10.inc"
};
char D_0048CA30[0x20] = {
#include "sub_main/0048CA30.inc"
};
char D_0048CA50[0x20] = {
#include "sub_main/0048CA50.inc"
};
char D_0048CA70[0x20] = {
#include "sub_main/0048CA70.inc"
};
char D_0048CA90[0x20] = {
#include "sub_main/0048CA90.inc"
};
char D_0048CAB0[0x20] = {
#include "sub_main/0048CAB0.inc"
};
char D_0048CAD0[0x20] = {
#include "sub_main/0048CAD0.inc"
};
char D_0048CAF0[0x20] = {
#include "sub_main/0048CAF0.inc"
};
char D_0048CB10[0x20] = {
#include "sub_main/0048CB10.inc"
};
char D_0048CB30[0x20] = {
#include "sub_main/0048CB30.inc"
};
char D_0048CB50[0x20] = {
#include "sub_main/0048CB50.inc"
};
char D_0048CB70[0x20] = {
#include "sub_main/0048CB70.inc"
};
char D_0048CB90[0x20] = {
#include "sub_main/0048CB90.inc"
};
char D_0048CBB0[0x20] = {
#include "sub_main/0048CBB0.inc"
};
char D_0048CBD0[0x20] = {
#include "sub_main/0048CBD0.inc"
};
char D_0048CBF0[0x20] = {
#include "sub_main/0048CBF0.inc"
};
char D_0048CC10[0x20] = {
#include "sub_main/0048CC10.inc"
};
char D_0048CC30[0x20] = {
#include "sub_main/0048CC30.inc"
};
char D_0048CC50[0x20] = {
#include "sub_main/0048CC50.inc"
};
char D_0048CC70[0x20] = {
#include "sub_main/0048CC70.inc"
};
char D_0048CC90[0x20] = {
#include "sub_main/0048CC90.inc"
};
char D_0048CCB0[0x20] = {
#include "sub_main/0048CCB0.inc"
};
char D_0048CCD0[0x20] = {
#include "sub_main/0048CCD0.inc"
};
char D_0048CCF0[0x20] = {
#include "sub_main/0048CCF0.inc"
};
char D_0048CD10[0x20] = {
#include "sub_main/0048CD10.inc"
};
char D_0048CD30[0x20] = {
#include "sub_main/0048CD30.inc"
};
char D_0048CD50[0x20] = {
#include "sub_main/0048CD50.inc"
};
char D_0048CD70[0x20] = {
#include "sub_main/0048CD70.inc"
};
char D_0048CD90[0x20] = {
#include "sub_main/0048CD90.inc"
};
char D_0048CDB0[0x20] = {
#include "sub_main/0048CDB0.inc"
};
char D_0048CDD0[0x20] = {
#include "sub_main/0048CDD0.inc"
};
char D_0048CDF0[0x20] = {
#include "sub_main/0048CDF0.inc"
};
char D_0048CE10[0x20] = {
#include "sub_main/0048CE10.inc"
};
char D_0048CE30[0x20] = {
#include "sub_main/0048CE30.inc"
};
char D_0048CE50[0x20] = {
#include "sub_main/0048CE50.inc"
};
char D_0048CE70[0x20] = {
#include "sub_main/0048CE70.inc"
};
char D_0048CE90[0x20] = {
#include "sub_main/0048CE90.inc"
};
char D_0048CEB0[0x20] = {
#include "sub_main/0048CEB0.inc"
};
char D_0048CED0[0x20] = {
#include "sub_main/0048CED0.inc"
};
char D_0048CEF0[0x20] = {
#include "sub_main/0048CEF0.inc"
};
char D_0048CF10[0x20] = {
#include "sub_main/0048CF10.inc"
};
char D_0048CF30[0x20] = {
#include "sub_main/0048CF30.inc"
};
char D_0048CF50[0x20] = {
#include "sub_main/0048CF50.inc"
};
char D_0048CF70[0x20] = {
#include "sub_main/0048CF70.inc"
};
char D_0048CF90[0x20] = {
#include "sub_main/0048CF90.inc"
};
char D_0048CFB0[0x20] = {
#include "sub_main/0048CFB0.inc"
};
char D_0048CFD0[0x20] = {
#include "sub_main/0048CFD0.inc"
};
char D_0048CFF0[0x20] = {
#include "sub_main/0048CFF0.inc"
};
char D_0048D010[0x20] = {
#include "sub_main/0048D010.inc"
};
char D_0048D030[0x20] = {
#include "sub_main/0048D030.inc"
};
char D_0048D050[0x20] = {
#include "sub_main/0048D050.inc"
};
char D_0048D070[0x20] = {
#include "sub_main/0048D070.inc"
};
char D_0048D090[0x20] = {
#include "sub_main/0048D090.inc"
};
char D_0048D0B0[0x20] = {
#include "sub_main/0048D0B0.inc"
};
char D_0048D0D0[0x20] = {
#include "sub_main/0048D0D0.inc"
};
char D_0048D0F0[0x20] = {
#include "sub_main/0048D0F0.inc"
};
char D_0048D110[0x20] = {
#include "sub_main/0048D110.inc"
};
char D_0048D130[0x10] = {
#include "sub_main/0048D130.inc"
};
char D_0048D140[0x20] = {
#include "sub_main/0048D140.inc"
};
char D_0048D160[0x20] = {
#include "sub_main/0048D160.inc"
};
char D_0048D180[0x20] = {
#include "sub_main/0048D180.inc"
};
char D_0048D1A0[0x20] = {
#include "sub_main/0048D1A0.inc"
};
char D_0048D1C0[0x20] = {
#include "sub_main/0048D1C0.inc"
};
char D_0048D1E0[0x20] = {
#include "sub_main/0048D1E0.inc"
};
char D_0048D200[0x20] = {
#include "sub_main/0048D200.inc"
};
char D_0048D220[0x20] = {
#include "sub_main/0048D220.inc"
};
char D_0048D240[0x20] = {
#include "sub_main/0048D240.inc"
};
char D_0048D260[0x20] = {
#include "sub_main/0048D260.inc"
};
char D_0048D280[0x20] = {
#include "sub_main/0048D280.inc"
};
char D_0048D2A0[0x20] = {
#include "sub_main/0048D2A0.inc"
};
char D_0048D2C0[0x20] = {
#include "sub_main/0048D2C0.inc"
};
char D_0048D2E0[0x20] = {
#include "sub_main/0048D2E0.inc"
};
char D_0048D300[0x20] = {
#include "sub_main/0048D300.inc"
};
char D_0048D320[0x20] = {
#include "sub_main/0048D320.inc"
};
char D_0048D340[0x20] = {
#include "sub_main/0048D340.inc"
};
char D_0048D360[0x20] = {
#include "sub_main/0048D360.inc"
};
char D_0048D380[0x20] = {
#include "sub_main/0048D380.inc"
};
char D_0048D3A0[0x20] = {
#include "sub_main/0048D3A0.inc"
};
char D_0048D3C0[0x20] = {
#include "sub_main/0048D3C0.inc"
};
char D_0048D3E0[0x20] = {
#include "sub_main/0048D3E0.inc"
};
char D_0048D400[0x20] = {
#include "sub_main/0048D400.inc"
};
char D_0048D420[0x20] = {
#include "sub_main/0048D420.inc"
};
char D_0048D440[0x20] = {
#include "sub_main/0048D440.inc"
};
char D_0048D460[0x20] = {
#include "sub_main/0048D460.inc"
};
char D_0048D480[0x20] = {
#include "sub_main/0048D480.inc"
};
char D_0048D4A0[0x20] = {
#include "sub_main/0048D4A0.inc"
};
char D_0048D4C0[0x20] = {
#include "sub_main/0048D4C0.inc"
};
char D_0048D4E0[0x20] = {
#include "sub_main/0048D4E0.inc"
};
char D_0048D500[0x20] = {
#include "sub_main/0048D500.inc"
};
char D_0048D520[0x20] = {
#include "sub_main/0048D520.inc"
};
char D_0048D540[0x20] = {
#include "sub_main/0048D540.inc"
};
char D_0048D560[0x20] = {
#include "sub_main/0048D560.inc"
};
char D_0048D580[0x20] = {
#include "sub_main/0048D580.inc"
};
char D_0048D5A0[0x20] = {
#include "sub_main/0048D5A0.inc"
};
char D_0048D5C0[0x20] = {
#include "sub_main/0048D5C0.inc"
};
char D_0048D5E0[0x20] = {
#include "sub_main/0048D5E0.inc"
};
char D_0048D600[0x20] = {
#include "sub_main/0048D600.inc"
};
char D_0048D620[0x10] = {
#include "sub_main/0048D620.inc"
};
char D_0048D630[0x20] = {
#include "sub_main/0048D630.inc"
};
char D_0048D650[0x20] = {
#include "sub_main/0048D650.inc"
};
char D_0048D670[0x20] = {
#include "sub_main/0048D670.inc"
};
char D_0048D690[0x20] = {
#include "sub_main/0048D690.inc"
};
char D_0048D6B0[0x20] = {
#include "sub_main/0048D6B0.inc"
};
char D_0048D6D0[0x20] = {
#include "sub_main/0048D6D0.inc"
};
char D_0048D6F0[0x20] = {
#include "sub_main/0048D6F0.inc"
};
char D_0048D710[0x20] = {
#include "sub_main/0048D710.inc"
};
char D_0048D730[0x20] = {
#include "sub_main/0048D730.inc"
};
char D_0048D750[0x20] = {
#include "sub_main/0048D750.inc"
};
char D_0048D770[0x20] = {
#include "sub_main/0048D770.inc"
};
char D_0048D790[0x20] = {
#include "sub_main/0048D790.inc"
};
char D_0048D7B0[0x20] = {
#include "sub_main/0048D7B0.inc"
};
char D_0048D7D0[0x20] = {
#include "sub_main/0048D7D0.inc"
};
char D_0048D7F0[0x20] = {
#include "sub_main/0048D7F0.inc"
};
char D_0048D810[0x20] = {
#include "sub_main/0048D810.inc"
};
char D_0048D830[0x20] = {
#include "sub_main/0048D830.inc"
};
char D_0048D850[0x20] = {
#include "sub_main/0048D850.inc"
};
char D_0048D870[0x20] = {
#include "sub_main/0048D870.inc"
};
char D_0048D890[0x20] = {
#include "sub_main/0048D890.inc"
};
char D_0048D8B0[0x20] = {
#include "sub_main/0048D8B0.inc"
};
char D_0048D8D0[0x20] = {
#include "sub_main/0048D8D0.inc"
};
char D_0048D8F0[0x20] = {
#include "sub_main/0048D8F0.inc"
};
char D_0048D910[0x20] = {
#include "sub_main/0048D910.inc"
};
char D_0048D930[0x20] = {
#include "sub_main/0048D930.inc"
};
char D_0048D950[0x20] = {
#include "sub_main/0048D950.inc"
};
char D_0048D970[0x20] = {
#include "sub_main/0048D970.inc"
};
char D_0048D990[0x20] = {
#include "sub_main/0048D990.inc"
};
char D_0048D9B0[0x20] = {
#include "sub_main/0048D9B0.inc"
};
char D_0048D9D0[0x20] = {
#include "sub_main/0048D9D0.inc"
};
char D_0048D9F0[0x20] = {
#include "sub_main/0048D9F0.inc"
};
char D_0048DA10[0x20] = {
#include "sub_main/0048DA10.inc"
};
char D_0048DA30[0x20] = {
#include "sub_main/0048DA30.inc"
};
char D_0048DA50[0x20] = {
#include "sub_main/0048DA50.inc"
};
char D_0048DA70[0x20] = {
#include "sub_main/0048DA70.inc"
};
char D_0048DA90[0x20] = {
#include "sub_main/0048DA90.inc"
};
char D_0048DAB0[0x20] = {
#include "sub_main/0048DAB0.inc"
};
char D_0048DAD0[0x10] = {
#include "sub_main/0048DAD0.inc"
};
char D_0048DAE0[0x20] = {
#include "sub_main/0048DAE0.inc"
};
char D_0048DB00[0x20] = {
#include "sub_main/0048DB00.inc"
};
char D_0048DB20[0x20] = {
#include "sub_main/0048DB20.inc"
};
char D_0048DB40[0x20] = {
#include "sub_main/0048DB40.inc"
};
char D_0048DB60[0x20] = {
#include "sub_main/0048DB60.inc"
};
char D_0048DB80[0x20] = {
#include "sub_main/0048DB80.inc"
};
char D_0048DBA0[0x20] = {
#include "sub_main/0048DBA0.inc"
};
char D_0048DBC0[0x20] = {
#include "sub_main/0048DBC0.inc"
};
char D_0048DBE0[0x20] = {
#include "sub_main/0048DBE0.inc"
};
char D_0048DC00[0x20] = {
#include "sub_main/0048DC00.inc"
};
char D_0048DC20[0x20] = {
#include "sub_main/0048DC20.inc"
};
char D_0048DC40[0x20] = {
#include "sub_main/0048DC40.inc"
};
char D_0048DC60[0x20] = {
#include "sub_main/0048DC60.inc"
};
char D_0048DC80[0x20] = {
#include "sub_main/0048DC80.inc"
};
char D_0048DCA0[0x20] = {
#include "sub_main/0048DCA0.inc"
};
char D_0048DCC0[0x20] = {
#include "sub_main/0048DCC0.inc"
};
char D_0048DCE0[0x20] = {
#include "sub_main/0048DCE0.inc"
};
char D_0048DD00[0x20] = {
#include "sub_main/0048DD00.inc"
};
char D_0048DD20[0x20] = {
#include "sub_main/0048DD20.inc"
};
char D_0048DD40[0x20] = {
#include "sub_main/0048DD40.inc"
};
char D_0048DD60[0x20] = {
#include "sub_main/0048DD60.inc"
};
char D_0048DD80[0x20] = {
#include "sub_main/0048DD80.inc"
};
char D_0048DDA0[0x20] = {
#include "sub_main/0048DDA0.inc"
};
char D_0048DDC0[0x20] = {
#include "sub_main/0048DDC0.inc"
};
char D_0048DDE0[0x20] = {
#include "sub_main/0048DDE0.inc"
};
char D_0048DE00[0x20] = {
#include "sub_main/0048DE00.inc"
};
char D_0048DE20[0x20] = {
#include "sub_main/0048DE20.inc"
};
char D_0048DE40[0x20] = {
#include "sub_main/0048DE40.inc"
};
char D_0048DE60[0x20] = {
#include "sub_main/0048DE60.inc"
};
char D_0048DE80[0x20] = {
#include "sub_main/0048DE80.inc"
};
char D_0048DEA0[0x20] = {
#include "sub_main/0048DEA0.inc"
};
char D_0048DEC0[0x20] = {
#include "sub_main/0048DEC0.inc"
};
char D_0048DEE0[0x20] = {
#include "sub_main/0048DEE0.inc"
};
char D_0048DF00[0x20] = {
#include "sub_main/0048DF00.inc"
};
char D_0048DF20[0x20] = {
#include "sub_main/0048DF20.inc"
};
char D_0048DF40[0x20] = {
#include "sub_main/0048DF40.inc"
};
char D_0048DF60[0x20] = {
#include "sub_main/0048DF60.inc"
};
char D_0048DF80[0x20] = {
#include "sub_main/0048DF80.inc"
};
char D_0048DFA0[0x20] = {
#include "sub_main/0048DFA0.inc"
};
char D_0048DFC0[0x20] = {
#include "sub_main/0048DFC0.inc"
};
char D_0048DFE0[0x20] = {
#include "sub_main/0048DFE0.inc"
};
char D_0048E000[0x20] = {
#include "sub_main/0048E000.inc"
};
char D_0048E020[0x20] = {
#include "sub_main/0048E020.inc"
};
char D_0048E040[0x20] = {
#include "sub_main/0048E040.inc"
};
char D_0048E060[0x20] = {
#include "sub_main/0048E060.inc"
};
char D_0048E080[0x20] = {
#include "sub_main/0048E080.inc"
};
char D_0048E0A0[0x20] = {
#include "sub_main/0048E0A0.inc"
};
char D_0048E0C0[0x20] = {
#include "sub_main/0048E0C0.inc"
};
char D_0048E0E0[0x20] = {
#include "sub_main/0048E0E0.inc"
};
char D_0048E100[0x20] = {
#include "sub_main/0048E100.inc"
};
char D_0048E120[0x20] = {
#include "sub_main/0048E120.inc"
};
char D_0048E140[0x20] = {
#include "sub_main/0048E140.inc"
};
char D_0048E160[0x20] = {
#include "sub_main/0048E160.inc"
};
char D_0048E180[0x20] = {
#include "sub_main/0048E180.inc"
};
char D_0048E1A0[0x20] = {
#include "sub_main/0048E1A0.inc"
};
char D_0048E1C0[0x20] = {
#include "sub_main/0048E1C0.inc"
};
char D_0048E1E0[0x20] = {
#include "sub_main/0048E1E0.inc"
};
char D_0048E200[0x20] = {
#include "sub_main/0048E200.inc"
};
char D_0048E220[0x20] = {
#include "sub_main/0048E220.inc"
};
char D_0048E240[0x20] = {
#include "sub_main/0048E240.inc"
};
char D_0048E260[0x20] = {
#include "sub_main/0048E260.inc"
};
char D_0048E280[0x20] = {
#include "sub_main/0048E280.inc"
};
char D_0048E2A0[0x20] = {
#include "sub_main/0048E2A0.inc"
};
char D_0048E2C0[0x20] = {
#include "sub_main/0048E2C0.inc"
};
char D_0048E2E0[0x20] = {
#include "sub_main/0048E2E0.inc"
};
char D_0048E300[0x10] = {
#include "sub_main/0048E300.inc"
};
char D_0048E310[0x20] = {
#include "sub_main/0048E310.inc"
};
char D_0048E330[0x20] = {
#include "sub_main/0048E330.inc"
};
char D_0048E350[0x20] = {
#include "sub_main/0048E350.inc"
};
char D_0048E370[0x20] = {
#include "sub_main/0048E370.inc"
};
char D_0048E390[0x20] = {
#include "sub_main/0048E390.inc"
};
char D_0048E3B0[0x20] = {
#include "sub_main/0048E3B0.inc"
};
char D_0048E3D0[0x10] = {
#include "sub_main/0048E3D0.inc"
};
char D_0048E3E0[0x20] = {
#include "sub_main/0048E3E0.inc"
};
char D_0048E400[0x20] = {
#include "sub_main/0048E400.inc"
};
char D_0048E420[0x20] = {
#include "sub_main/0048E420.inc"
};
char D_0048E440[0x20] = {
#include "sub_main/0048E440.inc"
};
char D_0048E460[0x20] = {
#include "sub_main/0048E460.inc"
};
char D_0048E480[0x20] = {
#include "sub_main/0048E480.inc"
};
char D_0048E4A0[0x10] = {
#include "sub_main/0048E4A0.inc"
};
char D_0048E4B0[0x20] = {
#include "sub_main/0048E4B0.inc"
};
char D_0048E4D0[0x20] = {
#include "sub_main/0048E4D0.inc"
};
char D_0048E4F0[0x20] = {
#include "sub_main/0048E4F0.inc"
};
char D_0048E510[0x20] = {
#include "sub_main/0048E510.inc"
};
char D_0048E530[0x20] = {
#include "sub_main/0048E530.inc"
};
char D_0048E550[0x20] = {
#include "sub_main/0048E550.inc"
};
char D_0048E570[0x20] = {
#include "sub_main/0048E570.inc"
};
char D_0048E590[0x20] = {
#include "sub_main/0048E590.inc"
};
char D_0048E5B0[0x20] = {
#include "sub_main/0048E5B0.inc"
};
char D_0048E5D0[0x20] = {
#include "sub_main/0048E5D0.inc"
};
char D_0048E5F0[0x20] = {
#include "sub_main/0048E5F0.inc"
};
char D_0048E610[0x20] = {
#include "sub_main/0048E610.inc"
};
char D_0048E630[0x20] = {
#include "sub_main/0048E630.inc"
};
char D_0048E650[0x20] = {
#include "sub_main/0048E650.inc"
};
char D_0048E670[0x20] = {
#include "sub_main/0048E670.inc"
};
char D_0048E690[0x20] = {
#include "sub_main/0048E690.inc"
};
char D_0048E6B0[0x20] = {
#include "sub_main/0048E6B0.inc"
};
char D_0048E6D0[0x10] = {
#include "sub_main/0048E6D0.inc"
};
char D_0048E6E0[0x20] = {
#include "sub_main/0048E6E0.inc"
};
char D_0048E700[0x20] = {
#include "sub_main/0048E700.inc"
};
char D_0048E720[0x20] = {
#include "sub_main/0048E720.inc"
};
char D_0048E740[0x20] = {
#include "sub_main/0048E740.inc"
};
char D_0048E760[0x20] = {
#include "sub_main/0048E760.inc"
};
char D_0048E780[0x20] = {
#include "sub_main/0048E780.inc"
};
char D_0048E7A0[0x18] = {
#include "sub_main/0048E7A0.inc"
};
char D_0048E7B8[15] = {
#include "sub_main/0048E7B8.inc"
};
char D_0048E7D0[0x20] = {
#include "sub_main/0048E7D0.inc"
};
char D_0048E7F0[0x20] = {
#include "sub_main/0048E7F0.inc"
};
char D_0048E810[0x20] = {
#include "sub_main/0048E810.inc"
};
char D_0048E830[0x20] = {
#include "sub_main/0048E830.inc"
};
char D_0048E850[0x20] = {
#include "sub_main/0048E850.inc"
};
char D_0048E870[0x20] = {
#include "sub_main/0048E870.inc"
};
char D_0048E890[0x20] = {
#include "sub_main/0048E890.inc"
};
char D_0048E8B0[0x20] = {
#include "sub_main/0048E8B0.inc"
};
char D_0048E8D0[0x20] = {
#include "sub_main/0048E8D0.inc"
};
char D_0048E8F0[0x20] = {
#include "sub_main/0048E8F0.inc"
};
char D_0048E910[0x20] = {
#include "sub_main/0048E910.inc"
};
char D_0048E930[0x20] = {
#include "sub_main/0048E930.inc"
};
char D_0048E950[0x20] = {
#include "sub_main/0048E950.inc"
};
char D_0048E970[0x20] = {
#include "sub_main/0048E970.inc"
};
char D_0048E990[0x20] = {
#include "sub_main/0048E990.inc"
};
char D_0048E9B0[0x20] = {
#include "sub_main/0048E9B0.inc"
};
char D_0048E9D0[0x20] = {
#include "sub_main/0048E9D0.inc"
};
char D_0048E9F0[0x20] = {
#include "sub_main/0048E9F0.inc"
};
char D_0048EA10[0x20] = {
#include "sub_main/0048EA10.inc"
};
char D_0048EA30[0x20] = {
#include "sub_main/0048EA30.inc"
};
char D_0048EA50[0x20] = {
#include "sub_main/0048EA50.inc"
};
char D_0048EA70[0x20] = {
#include "sub_main/0048EA70.inc"
};
char D_0048EA90[0x20] = {
#include "sub_main/0048EA90.inc"
};
char D_0048EAB0[0x20] = {
#include "sub_main/0048EAB0.inc"
};
char D_0048EAD0[0x20] = {
#include "sub_main/0048EAD0.inc"
};
char D_0048EAF0[0x20] = {
#include "sub_main/0048EAF0.inc"
};
char D_0048EB10[0x20] = {
#include "sub_main/0048EB10.inc"
};
char D_0048EB30[0x20] = {
#include "sub_main/0048EB30.inc"
};
char D_0048EB50[0x20] = {
#include "sub_main/0048EB50.inc"
};
char D_0048EB70[0x20] = {
#include "sub_main/0048EB70.inc"
};
char D_0048EB90[0x20] = {
#include "sub_main/0048EB90.inc"
};
char D_0048EBB0[0x20] = {
#include "sub_main/0048EBB0.inc"
};
char D_0048EBD0[0x20] = {
#include "sub_main/0048EBD0.inc"
};
char D_0048EBF0[0x20] = {
#include "sub_main/0048EBF0.inc"
};
char D_0048EC10[0x20] = {
#include "sub_main/0048EC10.inc"
};
char D_0048EC30[0x20] = {
#include "sub_main/0048EC30.inc"
};
char D_0048EC50[0x20] = {
#include "sub_main/0048EC50.inc"
};
char D_0048EC70[0x20] = {
#include "sub_main/0048EC70.inc"
};
char D_0048EC90[0x20] = {
#include "sub_main/0048EC90.inc"
};
char D_0048ECB0[0x20] = {
#include "sub_main/0048ECB0.inc"
};
char D_0048ECD0[0x20] = {
#include "sub_main/0048ECD0.inc"
};
char D_0048ECF0[0x20] = {
#include "sub_main/0048ECF0.inc"
};
char D_0048ED10[0x20] = {
#include "sub_main/0048ED10.inc"
};
char D_0048ED30[0x20] = {
#include "sub_main/0048ED30.inc"
};
char D_0048ED50[0x20] = {
#include "sub_main/0048ED50.inc"
};
char D_0048ED70[0x20] = {
#include "sub_main/0048ED70.inc"
};
char D_0048ED90[0x20] = {
#include "sub_main/0048ED90.inc"
};
char D_0048EDB0[0x20] = {
#include "sub_main/0048EDB0.inc"
};
char D_0048EDD0[0x20] = {
#include "sub_main/0048EDD0.inc"
};
char D_0048EDF0[0x20] = {
#include "sub_main/0048EDF0.inc"
};
char D_0048EE10[0x20] = {
#include "sub_main/0048EE10.inc"
};
char D_0048EE30[0x20] = {
#include "sub_main/0048EE30.inc"
};
char D_0048EE50[0x20] = {
#include "sub_main/0048EE50.inc"
};
char D_0048EE70[0x20] = {
#include "sub_main/0048EE70.inc"
};
char D_0048EE90[0x20] = {
#include "sub_main/0048EE90.inc"
};
char D_0048EEB0[0x20] = {
#include "sub_main/0048EEB0.inc"
};
char D_0048EED0[0x20] = {
#include "sub_main/0048EED0.inc"
};
char D_0048EEF0[0x20] = {
#include "sub_main/0048EEF0.inc"
};
char D_0048EF10[0x20] = {
#include "sub_main/0048EF10.inc"
};
char D_0048EF30[0x20] = {
#include "sub_main/0048EF30.inc"
};
char D_0048EF50[0x20] = {
#include "sub_main/0048EF50.inc"
};
char D_0048EF70[0x20] = {
#include "sub_main/0048EF70.inc"
};
char D_0048EF90[0x20] = {
#include "sub_main/0048EF90.inc"
};
char D_0048EFB0[0x20] = {
#include "sub_main/0048EFB0.inc"
};
char D_0048EFD0[0x20] = {
#include "sub_main/0048EFD0.inc"
};
char D_0048EFF0[16] = {
#include "sub_main/0048EFF0.inc"
};
u32 D_0048F000[4] = {
#include "sub_main/0048F000.inc"
};
char D_0048F010[0x20] = {
#include "sub_main/0048F010.inc"
};
char D_0048F030[0x20] = {
#include "sub_main/0048F030.inc"
};
char D_0048F050[0x20] = {
#include "sub_main/0048F050.inc"
};
char D_0048F070[0x10] = {
#include "sub_main/0048F070.inc"
};
char D_0048F080[0x10] = {
#include "sub_main/0048F080.inc"
};
char D_0048F090[0x20] = {
#include "sub_main/0048F090.inc"
};
char D_0048F0B0[0x20] = {
#include "sub_main/0048F0B0.inc"
};
char D_0048F0D0[0x20] = {
#include "sub_main/0048F0D0.inc"
};
char D_0048F0F0[0x20] = {
#include "sub_main/0048F0F0.inc"
};
char D_0048F110[0x20] = {
#include "sub_main/0048F110.inc"
};
char D_0048F130[0x20] = {
#include "sub_main/0048F130.inc"
};
char D_0048F150[0x20] = {
#include "sub_main/0048F150.inc"
};
char D_0048F170[0x20] = {
#include "sub_main/0048F170.inc"
};
char D_0048F190[0x20] = {
#include "sub_main/0048F190.inc"
};
char D_0048F1B0[0x20] = {
#include "sub_main/0048F1B0.inc"
};
char D_0048F1D0[0x20] = {
#include "sub_main/0048F1D0.inc"
};
char D_0048F1F0[0x20] = {
#include "sub_main/0048F1F0.inc"
};
char D_0048F210[0x20] = {
#include "sub_main/0048F210.inc"
};
char D_0048F230[0x20] = {
#include "sub_main/0048F230.inc"
};
char D_0048F250[0x20] = {
#include "sub_main/0048F250.inc"
};
char D_0048F270[0x20] = {
#include "sub_main/0048F270.inc"
};
char D_0048F290[0x20] = {
#include "sub_main/0048F290.inc"
};
char D_0048F2B0[0x20] = {
#include "sub_main/0048F2B0.inc"
};
char D_0048F2D0[0x20] = {
#include "sub_main/0048F2D0.inc"
};
char D_0048F2F0[0x20] = {
#include "sub_main/0048F2F0.inc"
};
char D_0048F310[0x20] = {
#include "sub_main/0048F310.inc"
};
char D_0048F330[0x20] = {
#include "sub_main/0048F330.inc"
};
char D_0048F350[0x20] = {
#include "sub_main/0048F350.inc"
};
char D_0048F370[0x20] = {
#include "sub_main/0048F370.inc"
};
char D_0048F390[0x20] = {
#include "sub_main/0048F390.inc"
};
char D_0048F3B0[0x20] = {
#include "sub_main/0048F3B0.inc"
};
char D_0048F3D0[0x20] = {
#include "sub_main/0048F3D0.inc"
};
char D_0048F3F0[0x20] = {
#include "sub_main/0048F3F0.inc"
};
char D_0048F410[0x20] = {
#include "sub_main/0048F410.inc"
};
char D_0048F430[0x20] = {
#include "sub_main/0048F430.inc"
};
char D_0048F450[0x20] = {
#include "sub_main/0048F450.inc"
};
char D_0048F470[0x20] = {
#include "sub_main/0048F470.inc"
};
char D_0048F490[0x20] = {
#include "sub_main/0048F490.inc"
};
char D_0048F4B0[0x20] = {
#include "sub_main/0048F4B0.inc"
};
char D_0048F4D0[0x20] = {
#include "sub_main/0048F4D0.inc"
};
char D_0048F4F0[0x20] = {
#include "sub_main/0048F4F0.inc"
};
char D_0048F510[0x20] = {
#include "sub_main/0048F510.inc"
};
char D_0048F530[0x20] = {
#include "sub_main/0048F530.inc"
};
char D_0048F550[0x20] = {
#include "sub_main/0048F550.inc"
};
char D_0048F570[0x20] = {
#include "sub_main/0048F570.inc"
};
char D_0048F590[0x20] = {
#include "sub_main/0048F590.inc"
};
char D_0048F5B0[0x20] = {
#include "sub_main/0048F5B0.inc"
};
char D_0048F5D0[0x20] = {
#include "sub_main/0048F5D0.inc"
};
char D_0048F5F0[0x20] = {
#include "sub_main/0048F5F0.inc"
};
char D_0048F610[0x20] = {
#include "sub_main/0048F610.inc"
};
char D_0048F630[0x20] = {
#include "sub_main/0048F630.inc"
};
char D_0048F650[0x20] = {
#include "sub_main/0048F650.inc"
};
char D_0048F670[0x20] = {
#include "sub_main/0048F670.inc"
};
char D_0048F690[0x20] = {
#include "sub_main/0048F690.inc"
};
char D_0048F6B0[0x20] = {
#include "sub_main/0048F6B0.inc"
};
char D_0048F6D0[0x20] = {
#include "sub_main/0048F6D0.inc"
};
char D_0048F6F0[0x20] = {
#include "sub_main/0048F6F0.inc"
};
char D_0048F710[0x20] = {
#include "sub_main/0048F710.inc"
};
char D_0048F730[0x10] = {
#include "sub_main/0048F730.inc"
};
char D_0048F740[0x20] = {
#include "sub_main/0048F740.inc"
};
char D_0048F760[0x20] = {
#include "sub_main/0048F760.inc"
};
char D_0048F780[0x20] = {
#include "sub_main/0048F780.inc"
};
char D_0048F7A0[0x20] = {
#include "sub_main/0048F7A0.inc"
};
char D_0048F7C0[0x20] = {
#include "sub_main/0048F7C0.inc"
};
char D_0048F7E0[0x20] = {
#include "sub_main/0048F7E0.inc"
};
char D_0048F800[0x20] = {
#include "sub_main/0048F800.inc"
};
char D_0048F820[0x20] = {
#include "sub_main/0048F820.inc"
};
char D_0048F840[0x20] = {
#include "sub_main/0048F840.inc"
};
char D_0048F860[0x20] = {
#include "sub_main/0048F860.inc"
};
char D_0048F880[0x20] = {
#include "sub_main/0048F880.inc"
};
char D_0048F8A0[0x20] = {
#include "sub_main/0048F8A0.inc"
};
char D_0048F8C0[0x20] = {
#include "sub_main/0048F8C0.inc"
};
char D_0048F8E0[0x20] = {
#include "sub_main/0048F8E0.inc"
};
char D_0048F900[0x20] = {
#include "sub_main/0048F900.inc"
};
char D_0048F920[0x20] = {
#include "sub_main/0048F920.inc"
};
char D_0048F940[0x20] = {
#include "sub_main/0048F940.inc"
};
char D_0048F960[0x20] = {
#include "sub_main/0048F960.inc"
};
char D_0048F980[0x20] = {
#include "sub_main/0048F980.inc"
};
char D_0048F9A0[0x20] = {
#include "sub_main/0048F9A0.inc"
};
char D_0048F9C0[0x20] = {
#include "sub_main/0048F9C0.inc"
};
char D_0048F9E0[0x20] = {
#include "sub_main/0048F9E0.inc"
};
char D_0048FA00[0x20] = {
#include "sub_main/0048FA00.inc"
};
char D_0048FA20[0x20] = {
#include "sub_main/0048FA20.inc"
};
char D_0048FA40[0x20] = {
#include "sub_main/0048FA40.inc"
};
char D_0048FA60[0x20] = {
#include "sub_main/0048FA60.inc"
};
char D_0048FA80[0x20] = {
#include "sub_main/0048FA80.inc"
};
char D_0048FAA0[0x20] = {
#include "sub_main/0048FAA0.inc"
};
char D_0048FAC0[0x20] = {
#include "sub_main/0048FAC0.inc"
};
char D_0048FAE0[0x20] = {
#include "sub_main/0048FAE0.inc"
};
char D_0048FB00[0x20] = {
#include "sub_main/0048FB00.inc"
};
char D_0048FB20[0x20] = {
#include "sub_main/0048FB20.inc"
};
char D_0048FB40[0x20] = {
#include "sub_main/0048FB40.inc"
};
char D_0048FB60[0x20] = {
#include "sub_main/0048FB60.inc"
};
char D_0048FB80[0x20] = {
#include "sub_main/0048FB80.inc"
};
char D_0048FBA0[0x20] = {
#include "sub_main/0048FBA0.inc"
};
char D_0048FBC0[0x20] = {
#include "sub_main/0048FBC0.inc"
};
char D_0048FBE0[0x20] = {
#include "sub_main/0048FBE0.inc"
};
char D_0048FC00[0x20] = {
#include "sub_main/0048FC00.inc"
};
char D_0048FC20[0x20] = {
#include "sub_main/0048FC20.inc"
};
char D_0048FC40[0x20] = {
#include "sub_main/0048FC40.inc"
};
char D_0048FC60[0x20] = {
#include "sub_main/0048FC60.inc"
};
char D_0048FC80[0x20] = {
#include "sub_main/0048FC80.inc"
};
char D_0048FCA0[0x20] = {
#include "sub_main/0048FCA0.inc"
};
char D_0048FCC0[0x20] = {
#include "sub_main/0048FCC0.inc"
};
char D_0048FCE0[0x20] = {
#include "sub_main/0048FCE0.inc"
};
char D_0048FD00[0x20] = {
#include "sub_main/0048FD00.inc"
};
char D_0048FD20[0x20] = {
#include "sub_main/0048FD20.inc"
};
char D_0048FD40[0x20] = {
#include "sub_main/0048FD40.inc"
};
char D_0048FD60[0x20] = {
#include "sub_main/0048FD60.inc"
};
char D_0048FD80[0x20] = {
#include "sub_main/0048FD80.inc"
};
char D_0048FDA0[0x20] = {
#include "sub_main/0048FDA0.inc"
};
char D_0048FDC0[0x20] = {
#include "sub_main/0048FDC0.inc"
};
char D_0048FDE0[0x20] = {
#include "sub_main/0048FDE0.inc"
};
char D_0048FE00[0x20] = {
#include "sub_main/0048FE00.inc"
};
char D_0048FE20[0x20] = {
#include "sub_main/0048FE20.inc"
};
char D_0048FE40[0x20] = {
#include "sub_main/0048FE40.inc"
};
char D_0048FE60[0x20] = {
#include "sub_main/0048FE60.inc"
};
char D_0048FE80[0x20] = {
#include "sub_main/0048FE80.inc"
};
char D_0048FEA0[0x20] = {
#include "sub_main/0048FEA0.inc"
};
char D_0048FEC0[0x20] = {
#include "sub_main/0048FEC0.inc"
};
char D_0048FEE0[0x20] = {
#include "sub_main/0048FEE0.inc"
};
char D_0048FF00[0x20] = {
#include "sub_main/0048FF00.inc"
};
char D_0048FF20[0x20] = {
#include "sub_main/0048FF20.inc"
};
char D_0048FF40[0x20] = {
#include "sub_main/0048FF40.inc"
};
char D_0048FF60[0x20] = {
#include "sub_main/0048FF60.inc"
};
char D_0048FF80[0x20] = {
#include "sub_main/0048FF80.inc"
};
char D_0048FFA0[0x20] = {
#include "sub_main/0048FFA0.inc"
};
char D_0048FFC0[0x20] = {
#include "sub_main/0048FFC0.inc"
};
char D_0048FFE0[0x20] = {
#include "sub_main/0048FFE0.inc"
};
char D_00490000[0x20] = {
#include "sub_main/00490000.inc"
};
char D_00490020[0x20] = {
#include "sub_main/00490020.inc"
};
char D_00490040[8] = {
#include "sub_main/00490040.inc"
};
u32 D_00490048[3] = {
#include "sub_main/00490048.inc"
};
char D_00490060[0x20] = {
#include "sub_main/00490060.inc"
};
char D_00490080[0x20] = {
#include "sub_main/00490080.inc"
};
char D_004900A0[0x20] = {
#include "sub_main/004900A0.inc"
};
char D_004900C0[0x20] = {
#include "sub_main/004900C0.inc"
};
char D_004900E0[0x20] = {
#include "sub_main/004900E0.inc"
};
char D_00490100[0x20] = {
#include "sub_main/00490100.inc"
};
char D_00490120[0x20] = {
#include "sub_main/00490120.inc"
};
char D_00490140[0x20] = {
#include "sub_main/00490140.inc"
};
char D_00490160[0x20] = {
#include "sub_main/00490160.inc"
};
char D_00490180[0x20] = {
#include "sub_main/00490180.inc"
};
char D_004901A0[0x20] = {
#include "sub_main/004901A0.inc"
};
char D_004901C0[0x20] = {
#include "sub_main/004901C0.inc"
};
char D_004901E0[0x20] = {
#include "sub_main/004901E0.inc"
};
char D_00490200[0x20] = {
#include "sub_main/00490200.inc"
};
char D_00490220[0x20] = {
#include "sub_main/00490220.inc"
};
char D_00490240[0x20] = {
#include "sub_main/00490240.inc"
};
char D_00490260[0x20] = {
#include "sub_main/00490260.inc"
};
char D_00490280[0x20] = {
#include "sub_main/00490280.inc"
};
char D_004902A0[0x20] = {
#include "sub_main/004902A0.inc"
};
char D_004902C0[0x20] = {
#include "sub_main/004902C0.inc"
};
char D_004902E0[0x20] = {
#include "sub_main/004902E0.inc"
};
char D_00490300[0x20] = {
#include "sub_main/00490300.inc"
};
char D_00490320[0x20] = {
#include "sub_main/00490320.inc"
};
char D_00490340[0x20] = {
#include "sub_main/00490340.inc"
};
char D_00490360[0x20] = {
#include "sub_main/00490360.inc"
};
char D_00490380[0x20] = {
#include "sub_main/00490380.inc"
};
char D_004903A0[0x20] = {
#include "sub_main/004903A0.inc"
};
char D_004903C0[0x20] = {
#include "sub_main/004903C0.inc"
};
char D_004903E0[0x20] = {
#include "sub_main/004903E0.inc"
};
char D_00490400[0x20] = {
#include "sub_main/00490400.inc"
};
char D_00490420[0x20] = {
#include "sub_main/00490420.inc"
};
char D_00490440[0x20] = {
#include "sub_main/00490440.inc"
};
char D_00490460[0x20] = {
#include "sub_main/00490460.inc"
};
char D_00490480[0x20] = {
#include "sub_main/00490480.inc"
};
char D_004904A0[0x20] = {
#include "sub_main/004904A0.inc"
};
char D_004904C0[0x20] = {
#include "sub_main/004904C0.inc"
};
char D_004904E0[0x20] = {
#include "sub_main/004904E0.inc"
};
char D_00490500[0x18] = {
#include "sub_main/00490500.inc"
};
char D_00490518[0xC] = {
#include "sub_main/00490518.inc"
};
char D_00490530[0x20] = {
#include "sub_main/00490530.inc"
};
char D_00490550[0x20] = {
#include "sub_main/00490550.inc"
};
char D_00490570[0x10] = {
#include "sub_main/00490570.inc"
};
char D_00490580[0x20] = {
#include "sub_main/00490580.inc"
};
char D_004905A0[0x20] = {
#include "sub_main/004905A0.inc"
};
char D_004905C0[0x20] = {
#include "sub_main/004905C0.inc"
};
char D_004905E0[0x20] = {
#include "sub_main/004905E0.inc"
};
char D_00490600[0x20] = {
#include "sub_main/00490600.inc"
};
char D_00490620[0x20] = {
#include "sub_main/00490620.inc"
};
char D_00490640[0x20] = {
#include "sub_main/00490640.inc"
};
char D_00490660[0x20] = {
#include "sub_main/00490660.inc"
};
char D_00490680[0x20] = {
#include "sub_main/00490680.inc"
};
char D_004906A0[0x20] = {
#include "sub_main/004906A0.inc"
};
char D_004906C0[0x20] = {
#include "sub_main/004906C0.inc"
};
char D_004906E0[0x20] = {
#include "sub_main/004906E0.inc"
};
char D_00490700[0x20] = {
#include "sub_main/00490700.inc"
};
char D_00490720[0x20] = {
#include "sub_main/00490720.inc"
};
char D_00490740[0x20] = {
#include "sub_main/00490740.inc"
};
char D_00490760[0x20] = {
#include "sub_main/00490760.inc"
};
char D_00490780[0x20] = {
#include "sub_main/00490780.inc"
};
char D_004907A0[0x20] = {
#include "sub_main/004907A0.inc"
};
char D_004907C0[0x20] = {
#include "sub_main/004907C0.inc"
};
char D_004907E0[0x20] = {
#include "sub_main/004907E0.inc"
};
char D_00490800[0x20] = {
#include "sub_main/00490800.inc"
};
char D_00490820[0x20] = {
#include "sub_main/00490820.inc"
};
char D_00490840[0x20] = {
#include "sub_main/00490840.inc"
};
char D_00490860[0x20] = {
#include "sub_main/00490860.inc"
};
char D_00490880[0x20] = {
#include "sub_main/00490880.inc"
};
#pragma rodata off

#pragma optimization_level 4
/* Draws a window frame, moved by off[0], off[1] when off is given, from the window texture
 * (TEX_SLOT_ICONS). Styles 0-3: the edges of gFrameStyles[style] tiled along the sides (FRAME_INSET
 * first shrinks the box by the corners' size), then the four corners, each piece drawn twice
 * (piece, and mirror flipped for the opposite side). Styles 4-5: the four corners of
 * gFrameStyles2[style - FRAME_STYLE_CORNERS], each over a shadow when the style has one.
 * Then the background: a rectangle in color2 with FRAME_FILL, else tiles of about 17
 * pixels; and for styles 4-5 the four border lines between the corners. Built at
 * optimization level 4: the file's level 3 does not give this function's code. */
void WindowFrame_Draw(Frame *frame, s16 *off)
{
    u8 style;
    PrimLine fill;
    PrimLine border;
    Sprite piece;
    Sprite mirror;
    s16 x;
    u16 uEnd;
    s16 y;
    s16 width;
    s16 height;
    FrameStyle *tiles;
    FrameStyle2 *corners;
    u32 rows;
    u32 cols;
    u16 vEnd;
    f32 nextX;
    f32 nextY;
    f32 step;
    f32 stepX;
    s32 end;
    s16 cornerDx;
    s16 cornerDy;
    s32 endY;
    s32 endX;
    u32 i;
    s16 lineInsetX;
    s16 lineInsetY;

    Gfx_SetTexFilter(1);
    Gfx_SetBlend(4, 5);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    x = frame->x;
    y = frame->y;
    width = frame->w;
    height = frame->h;
    if (off != NULL) {
        x += off[0];
        y += off[1];
    }
    style = frame->style & FRAME_STYLE_MASK;
    if (style < FRAME_STYLE_CORNERS) {
        tiles = &gFrameStyles[style];
        if (frame->style & FRAME_INSET) {
            x += tiles->cw;
            y += tiles->ch;
            width -= (s16)(tiles->cw + tiles->cw);
            height -= (s16)(tiles->ch + tiles->ch);
        }
        piece.color = mirror.color = frame->color;
        piece.u0 = mirror.u0 = tiles->tu;
        piece.u1 = mirror.u1 = tiles->tu + tiles->tw;
        piece.v0 = tiles->tv;
        piece.v1 = tiles->tv + tiles->th;
        mirror.v0 = tiles->tv + tiles->th - 1;
        mirror.v1 = tiles->tv - 1;
        piece.y = y - tiles->th;
        mirror.y = y + height;
        piece.h = mirror.h = tiles->th;
        if (tiles->tw > 2) {
            i = width / tiles->tw;
            if (width % tiles->tw > tiles->tw >> 1) {
                i++;
            }
            nextX = x;
            step = (f32)width / i;
            end = nextX + width;
            while (i != 0) {
                i--;
                piece.x = mirror.x = nextX;
                nextX = end - step * i;
                piece.w = mirror.w = (s16)nextX - piece.x;
                Sprite_Draw(&piece);
                Sprite_Draw(&mirror);
            }
        } else {
            piece.x = mirror.x = x;
            piece.w = mirror.w = width;
            Sprite_Draw(&piece);
            Sprite_Draw(&mirror);
        }
        piece.u0 = tiles->su;
        piece.u1 = tiles->su + tiles->sw;
        mirror.u0 = tiles->su + tiles->sw - 1;
        mirror.u1 = tiles->su - 1;
        piece.v0 = mirror.v0 = tiles->sv;
        piece.v1 = mirror.v1 = tiles->sv + tiles->sh;
        piece.x = x - tiles->sw;
        mirror.x = x + width;
        piece.w = mirror.w = tiles->sw;
        if (tiles->sh > 2) {
            i = height / tiles->sh;
            if (tiles->sh >> 1 < height % tiles->sh) {
                i++;
            }
            nextY = y;
            step = (f32)height / i;
            end = nextY + height;
            while (i != 0) {
                i--;
                piece.y = mirror.y = nextY;
                nextY = end - step * i;
                piece.h = mirror.h = (s16)nextY - piece.y;
                Sprite_Draw(&piece);
                Sprite_Draw(&mirror);
            }
            height = piece.y + piece.h - y;
        } else {
            piece.y = mirror.y = y;
            piece.h = mirror.h = height;
            Sprite_Draw(&piece);
            Sprite_Draw(&mirror);
        }
        piece.w = tiles->cw;
        piece.h = tiles->ch;
        piece.u0 = tiles->cu;
        piece.u1 = tiles->cu + tiles->cw;
        piece.v0 = tiles->cv;
        piece.v1 = tiles->cv + tiles->ch;
        piece.x = x - tiles->cw;
        piece.y = y - tiles->ch;
        Sprite_Draw(&piece);
        piece.u0 = tiles->cu + tiles->cw - 1;
        piece.u1 = tiles->cu - 1;
        piece.x = x + width;
        Sprite_Draw(&piece);
        piece.v0 = tiles->cv + tiles->ch - 1;
        piece.v1 = tiles->cv - 1;
        piece.y = y + height;
        Sprite_Draw(&piece);
        piece.u0 = tiles->cu;
        piece.u1 = tiles->cu + tiles->cw;
        piece.x = x - tiles->cw;
        Sprite_Draw(&piece);
    } else {
        corners = &gFrameStyles2[style - FRAME_STYLE_CORNERS];
        x -= corners->ox;
        y -= corners->oy;
        width += (s16)(corners->ox + corners->ox);
        height += (s16)(corners->oy + corners->oy);
        if (frame->style & FRAME_INSET) {
            x += corners->ix;
            y += corners->iy;
            width -= (s16)(corners->ix + corners->ix);
            height -= (s16)(corners->iy + corners->iy);
        }
        cornerDx = width - (corners->w - corners->ix);
        cornerDy = height - (corners->h - corners->iy);
        piece.w = corners->w;
        piece.h = corners->h;
        uEnd = corners->u + corners->w;
        vEnd = corners->v + corners->h;
        piece.u0 = corners->u;
        piece.v0 = corners->v;
        piece.u1 = uEnd;
        piece.v1 = vEnd;
        if (corners->shadow != 0) {
            piece.x = corners->shadow + (x - corners->ix);
            piece.y = corners->shadow + (y - corners->iy);
            piece.color = 0xE8000000;
            Sprite_Draw(&piece);
        }
        piece.x = x - corners->ix;
        piece.y = y - corners->iy;
        piece.color = frame->color;
        Sprite_Draw(&piece);
        piece.u0 = uEnd - 1;
        piece.u1 = corners->u - 1;
        if (corners->shadow != 0) {
            piece.x = corners->shadow + (x + cornerDx);
            piece.y = corners->shadow + (y - corners->iy);
            piece.color = 0xE8000000;
            Sprite_Draw(&piece);
        }
        piece.x = x + cornerDx;
        piece.y = y - corners->iy;
        piece.color = frame->color;
        Sprite_Draw(&piece);
        piece.v0 = vEnd - 1;
        piece.v1 = corners->v - 1;
        if (corners->shadow != 0) {
            piece.x = corners->shadow + (x + cornerDx);
            piece.y = corners->shadow + (y + cornerDy);
            piece.color = 0xE8000000;
            Sprite_Draw(&piece);
        }
        piece.x = x + cornerDx;
        piece.y = y + cornerDy;
        piece.color = frame->color;
        Sprite_Draw(&piece);
        piece.u0 = corners->u;
        piece.u1 = uEnd;
        if (corners->shadow != 0) {
            piece.x = corners->shadow + (x - corners->ix);
            piece.y = corners->shadow + (y + cornerDy);
            piece.color = 0xE8000000;
            Sprite_Draw(&piece);
        }
        piece.x = x - corners->ix;
        piece.y = y + cornerDy;
        piece.color = frame->color;
        Sprite_Draw(&piece);
    }
    if (frame->style & FRAME_FILL) {
        fill.x0 = x;
        fill.x1 = x + width;
        fill.y1 = y + height;
        fill.y0 = y;
        fill.color = frame->color2;
        Prim_FillRect(&fill);
    } else {
        *(u32 *)&piece.u0 = TEX_UV(0xEC, 0xBC);
        *(u32 *)&piece.u1 = TEX_UV(0xFD, 0xCD);
        piece.color = frame->color2;
        cols = width / 17;
        if (width % 17 > 8) {
            cols++;
        }
        stepX = (f32)width / cols;
        rows = height / 17;
        endX = (f32)x + (f32)width;
        if (height % 17 > 8) {
            rows++;
        }
        nextY = y;
        step = (f32)height / rows;
        endY = nextY + height;
        while (rows != 0) {
            rows--;
            piece.y = nextY;
            nextY = endY - step * rows;
            piece.h = (s16)nextY - piece.y;
            i = cols;
            nextX = x;
            while (i != 0) {
                i--;
                piece.x = nextX;
                nextX = endX - stepX * i;
                piece.w = (s16)nextX - piece.x;
                Sprite_Draw(&piece);
            }
        }
    }
    if (style == 4 || style == 5) {
        lineInsetY = gFrameStyles2[0].h - gFrameStyles2[0].iy;
        border.color = frame->color;
        lineInsetX = gFrameStyles2[0].w - gFrameStyles2[0].ix;
        border.x0 = x + lineInsetX;
        border.x1 = x + width - lineInsetX;
        border.y0 = border.y1 = y;
        Prim_DrawLine(&border);
        border.y0 = border.y1 = y + height;
        Prim_DrawLine(&border);
        border.x0 = border.x1 = x;
        border.y0 = y + lineInsetY;
        border.y1 = y + height - lineInsetY;
        Prim_DrawLine(&border);
        border.x0 = border.x1 = x + width;
        Prim_DrawLine(&border);
    }
}
#pragma optimization_level reset


void ListPanel_DrawCursorBar(s32 x0, s32 x1, s16 y, s16 lh, s32 sel, s32 kind);
s32 Prim_DrawLine(struct PrimLine *r);
void Font_SetPal(s32 a);

/* Draws a list panel moved by pos: its frame, a title line on a band when title is set, the
 * cursor on entry sel (none when negative), the entries and, with WINDOW_RULED, rules between
 * them. For the match one local, k, is the panel's y, then the line height, then the rules'
 * counter, and lineY is both the text lines' y and the rules' y. */
void ListPanel_Draw(ListPanel *panel, s16 *pos, char *title, s16 sel)
{
    Frame frame;
    PrimLine band;
    PrimLine rule;
    s32 left;
    s32 top;
    u8 style;
    s16 lineY;
    s16 k;
    s16 lineH;
    s16 rows;
    char **item;
    s16 x;

    x = panel->x;
    k = panel->y;
    if (pos != NULL) {
        x += pos[0];
        k += pos[1];
    }
    style = panel->flags & WINDOW_STYLE_MASK;
    if (style > WINDOW_STYLE_MAX) {
        style = WINDOW_STYLE_MAX;
    }
    frame.style = style & FRAME_STYLE_MASK;
    if (panel->flags & WINDOW_INSET) {
        frame.style |= FRAME_INSET;
    }
    if (panel->flags & WINDOW_FILL) {
        frame.style |= FRAME_FILL;
    }
    lineH = panel->charH + 2;
    if (panel->flags & WINDOW_RULED) {
        lineH += 2;
    }
    rows = panel->lines;
    if (title != NULL) {
        rows++;
    }
    frame.x = (left = x) - panel->padX;
    frame.y = (top = k) - panel->padY;
    frame.w = panel->padX + panel->padX + panel->charW * panel->lineChars;
    k = lineH;
    frame.h = panel->padY + panel->padY + k * rows;
    frame.color = panel->color;
    frame.color2 = panel->color2;
    WindowFrame_Draw(&frame, 0);
    if (sel >= 0) {
        if (title != NULL) {
            sel++;
        }
        ListPanel_DrawCursorBar((left - 2) - (f32)panel->padX, (f32)panel->padX + (left + panel->charW * panel->lineChars + 1), top + 1, lineH, sel, panel->cursorColor);
    }
    if (panel->items != NULL || title != NULL) {
        Font_SetSpacing(panel->charW, panel->charH);
        if (panel->rgbText == 0) {
            Text_SetColor(panel->textColor);
        } else {
            Font_SetPal(0);
            Font_SetColor(*(u32 *)&panel->textColor);
        }
        lineY = top + 1;
        if (title != NULL) {
            band.x0 = left - panel->padX;
            band.x1 = left + panel->charW * panel->lineChars + panel->padX;
            band.y0 = lineY;
            band.y1 = lineY + k - 1;
            band.color = (panel->color & 0xFFFFFF) | 0x58000000;
            Prim_FillRect(&band);
            Font_SetPos(x, lineY);
            Text_Printf(title);
            lineY += lineH;
            rows--;
        }
        item = panel->items;
        if (item != NULL && rows > 0) {
            do {
                if (*item == NULL) {
                    break;
                }
                Font_SetPos(x, lineY);
                Text_Printf(gWindowFmtString, *item);
                rows--;
                item++;
                lineY += lineH;
            } while (rows > 0);
        }
    }
    if (panel->flags & WINDOW_RULED) {
        rule.x0 = left + 1;
        lineY = top + k - 1;
        rule.x1 = left + panel->charW * panel->lineChars;
        rule.color = panel->color;
        if (title != NULL) {
            lineY += lineH;
        }
        for (k = panel->lines - 1; k > 0; k--, lineY += lineH) {
            rule.y1 = lineY;
            rule.y0 = lineY;
            Prim_DrawLine(&rule);
        }
    }
}

void Window_DrawSides(Window *b, s16 *pos, u32 color);

/* Draws a window's two side pieces in a fixed colour (0xF020DF20; the green byte is worked
 * out from a float local, as the match needs). */
void Menu_DrawFrame(void *win, s16 *pos)
{
    f32 alpha = 48.0f;

    Window_DrawSides(win, pos, (((s8)alpha + 0xAF) << 8) | 0xF0200020);
}

/* Draws the two side pieces of a window's frame (with shadows), at its position moved by pos; they blink with gFrameTick. */
void Window_DrawSides(Window *win, s16 *pos, u32 color)
{
    Sprite piece;
    s16 x;
    s16 y;
    s16 blink;

    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    x = win->x;
    y = win->y;
    blink = (gFrameTick & 0x10) == 0 ? 3 : 8;
    if (pos != NULL) {
        x += pos[0];
        y += pos[1];
    }
    piece.w = win->charW;
    piece.h = win->charH + 4;
    *(u32 *)&piece.u0 = TEX_UV(0x71, 0x6E);
    *(u32 *)&piece.u1 = TEX_UV(0x7D, 0x7D);
    piece.x = x - blink + 1;
    piece.y = y;
    piece.color = color & 0xFF000000;
    Sprite_Draw(&piece);
    piece.x -= 2;
    piece.y -= 1;
    piece.color = color;
    Sprite_Draw(&piece);
    piece.u0 = 0x7C;
    piece.u1 = 0x70;
    piece.x = blink + (x + win->charW * win->lineChars) - win->charW + 1;
    piece.y = y;
    piece.color = color & 0xFF000000;
    Sprite_Draw(&piece);
    piece.x -= 2;
    piece.y -= 1;
    piece.color = color;
    Sprite_Draw(&piece);
}

void Font_SetPal(s32 a);

/* Draws a menu window (its frame, moved by pos) and its text line by line. */
void Window_DrawText(void *window, s16 *pos, u8 *text)
{
    Panel *panel = window;
    char *src = (char *)text;
    Frame frame;
    char buf[128];
    s16 linesLeft;
    s16 lineH;
    s16 x;
    s16 y;
    u8 style;
    char *dst;
    s16 lineY;

    x = panel->x;
    y = panel->y;
    if (pos != NULL) {
        x += pos[0];
        y += pos[1];
    }
    style = panel->flags & WINDOW_STYLE_MASK;
    if (style > WINDOW_STYLE_MAX) {
        style = WINDOW_STYLE_MAX;
    }
    frame.style = style & FRAME_STYLE_MASK;
    if (panel->flags & WINDOW_INSET) {
        frame.style |= FRAME_INSET;
    }
    if (panel->flags & WINDOW_FILL) {
        frame.style |= FRAME_FILL;
    }
    lineH = panel->charH + 2;
    frame.x = x - panel->padX;
    frame.y = y - panel->padY;
    frame.w = panel->padX + panel->padX + panel->charW * panel->lineChars;
    frame.h = panel->padY + panel->padY + lineH * panel->lines;
    frame.color = panel->color;
    frame.color2 = panel->color2;
    WindowFrame_Draw(&frame, 0);
    if (src != NULL) {
        Font_SetSpacing(panel->charW, panel->charH);
        if (panel->rgbText == 0) {
            Text_SetColor(panel->textColor);
        } else {
            Font_SetPal(0);
            Font_SetColor(*(u32 *)&panel->textColor);
        }
        lineY = y;
        for (linesLeft = panel->lines; linesLeft > 0; linesLeft--) {
            for (dst = buf;; src++, dst++) {
                if (*src == '\n') {
                    *dst = 0;
                    src++;
                    break;
                }
                *dst = *src;
                if (*src == 0) {
                    linesLeft = 0;
                    break;
                }
            }
            Font_SetPos(x, lineY);
            Text_Printf(gWindowFmtString, buf);
            lineY += lineH;
        }
    }
}

/* ListPanel_DrawAlpha with alpha 0xB2. */
void ListPanel_DrawDim(struct ListWin *win, char *title, s32 sel)
{
    ListPanel_DrawAlpha(win, title, sel, 0xB2);
}


void ListPanel_DrawCursorBar(s32 x0, s32 x1, s16 y, s16 lh, s32 sel, s32 kind);
extern u32 gPanelFrameColor;

/* Draws a list window with alpha: its frame, a title line (highlighted) when title is set, the cursor on
 * entry sel (none when negative) and the entries. A K&R definition: its callers see it without a prototype. */
void ListPanel_DrawAlpha(win, title, sel, alpha)
    ListWin *win;
    char *title;
    s32 sel;
    u8 alpha;
{
    Frame frame;
    PrimLine band;
    f32 x;
    f32 cellW;
    s16 top;
    s16 lineH;
    u8 style;
    s16 rows;
    char **item;

    Gfx_SetTexFilter(1);
    Gfx_SetBlend(4, 5);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    x = win->x;
    cellW = win->charW;
    style = win->flags & WINDOW_STYLE_MASK;
    if (style > WINDOW_STYLE_MAX) {
        style = WINDOW_STYLE_MAX;
    }
    if (win->flags & WINDOW_RULED) {
        top = win->y - 2;
        lineH = win->charH + 4;
    } else {
        top = win->y - 1;
        lineH = win->charH + 2;
    }
    frame.x = win->x;
    frame.y = top;
    frame.w = win->charW * win->lineChars;
    frame.h = lineH * win->lines;
    frame.color = (gPanelFrameColor & 0xFFFFFF) | (alpha << 24);
    frame.color2 = (alpha << 24) | 0x305040;
    frame.style = style & FRAME_STYLE_MASK;
    if (win->flags & WINDOW_FILL) {
        frame.style |= FRAME_FILL;
        frame.color2 = win->color2;
    }
    if (title != NULL) {
        frame.h += lineH;
    }
    WindowFrame_Draw(&frame, 0);
    if (title != NULL) {
        band.x0 = x;
        band.x1 = x + cellW * win->lineChars;
        band.y1 = top + lineH;
        band.color = 0x30FFFFFF;
        band.y0 = top;
        Prim_FillRect(&band);
    }
    if (sel >= 0) {
        if (title != NULL) {
            sel++;
        }
        ListPanel_DrawCursorBar(x - 2.0f, 2.0f + (x + cellW * win->lineChars), win->y, lineH, sel, win->cursorColor);
    }
    if (win->items != NULL) {
        top = win->y;
        Gfx_SetBlend(4, 5);
        Font_SetSpacing(win->charW, win->charH);
        Text_SetColor(win->color);
        if (title != NULL) {
            Font_SetPos(win->x, top);
            Text_Printf(title);
            top += lineH;
        }
        for (rows = win->lines, item = win->items; rows > 0; item++, rows--) {
            if (*item == NULL) {
                break;
            }
            Font_SetPos(win->x, top);
            Text_Printf(gWindowFmtString, *item);
            top += lineH;
        }
    } else if (title != NULL) {
        Gfx_SetBlend(4, 5);
        Font_SetSpacing(win->charW, win->charH);
        Text_SetColor(win->color);
        Font_SetPos(win->x, win->y);
        Text_Printf(title);
    }
}

/* Window_DrawTextAlpha with alpha 0xB2. */
void Window_DrawTextB2(Window *w, u8 *text)
{
    Window_DrawTextAlpha(w, text, 0xB2);
}

extern u32 gPanelFrameColor;

/* Draws a window (its frame in style flags & 7, at most 5) with alpha, and its text line by line. */
void Window_DrawTextAlpha(Window *win, u8 *text, u8 alpha)
{
    Frame frame;
    u8 buf[128];
    s16 linesLeft;
    s16 lineH;
    s16 y;
    u8 style;
    u8 *dst;
    s16 left;
    s16 top;

    style = win->flags & WINDOW_STYLE_MASK;
    if (style > WINDOW_STYLE_MAX) {
        style = WINDOW_STYLE_MAX;
    }
    top = win->y - 1;
    lineH = win->charH + 2;
    left = win->x;
    frame.x = left;
    frame.y = top;
    frame.w = win->charW * win->lineChars;
    frame.h = lineH * win->lines;
    frame.color = (gPanelFrameColor & 0xFFFFFF) | (alpha << 24);
    frame.color2 = (alpha << 24) | 0x305040;
    frame.style = style & FRAME_STYLE_MASK;
    if (win->flags & WINDOW_FILL) {
        frame.style |= FRAME_FILL;
        frame.color2 = win->frameColor;
    }
    WindowFrame_Draw(&frame, 0);
    if (text != NULL) {
        y = win->y;
        Gfx_SetBlend(4, 5);
        Font_SetSpacing(win->charW, win->charH);
        Text_SetColor(win->color);
        for (linesLeft = win->lines; linesLeft > 0; linesLeft--) {
            for (dst = buf;; text++, dst++) {
                if (*text == '\n') {
                    *dst = 0;
                    text++;
                    break;
                }
                *dst = *text;
                if (*text == 0) {
                    linesLeft = 0;
                    break;
                }
            }
            Font_SetPos(win->x, y);
            Text_Printf(gWindowFmtString, buf);
            y += lineH;
        }
    }
}

/* Draws a sprite stretched across in three parts: caps w wide showing du texels at each end, and the middle. */
void Sprite_DrawStretch3(Sprite *src, s16 capW, s16 capU)
{
    Sprite part;

    part = *src;
    part.w = capW;
    part.u1 = part.u0 + capU;
    Sprite_Draw(&part);
    part.x += part.w;
    part.w = src->w - (capW + capW);
    part.u0 = src->u0 + capU;
    part.u1 = src->u1 - capU;
    Sprite_Draw(&part);
    part.x += part.w;
    part.w = capW;
    part.u0 = src->u1 - capU;
    part.u1 = src->u1;
    Sprite_Draw(&part);
}

/* Draws a sprite stretched in both directions in nine parts: rows h high showing dv texels at the top and
 * bottom, and the middle row, each drawn by Sprite_DrawStretch3 (caps w wide showing du texels). */
void Sprite_DrawStretch9(Sprite *src, s16 capW, s16 capH, s16 capU, s16 capV)
{
    Sprite row;

    row = *src;
    row.h = capH;
    row.v1 = row.v0 + capV;
    Sprite_DrawStretch3(&row, capW, capU);
    row.y += row.h;
    row.h = src->h - (capH + capH);
    row.v0 = src->v0 + capV;
    row.v1 = src->v1 - capV;
    Sprite_DrawStretch3(&row, capW, capU);
    row.y += row.h;
    row.h = capH;
    row.v0 = src->v1 - capV;
    row.v1 = src->v1;
    Sprite_DrawStretch3(&row, capW, capU);
}

/* Draws count sprites from an array. */
void Sprite_DrawList(Sprite *sprites, s32 count)
{
    while (count != 0) {
        Sprite_Draw(sprites);
        count--;
        sprites++;
    }
}

/* Draws a menu arrow (texels 0x94-0xA6, 0x1A-0x2E of the bound texture) in the box x, y, w, h.
 * flags: ARROW_MIRROR points it the other way, ARROW_TEXRECT draws it with TexRect_Draw's
 * mode 2 instead of as a sprite. */
void Menu_DrawArrow(s16 x, s16 y, s16 width, s16 height, u32 color, u8 flags)
{
    Sprite arrow;

    arrow.v0 = 0x1A;
    arrow.y = y;
    arrow.v1 = 0x2E;
    arrow.h = height;
    arrow.color = color;
    if (flags & ARROW_MIRROR) {
        arrow.u0 = 0xA6;
        arrow.u1 = 0x94;
    } else {
        arrow.u0 = 0x94;
        arrow.u1 = 0xA6;
    }
    if (flags & ARROW_TEXRECT) {
        arrow.x = x;
        arrow.w = width;
        TexRect_Draw((struct TexRect *)&arrow, 2);
    } else {
        arrow.x = x;
        arrow.w = width;
        Sprite_Draw(&arrow);
    }
}

/* Draws the chat window's arrow (the side piece of Window_DrawSides, texels 0x71-0x7D,
 * 0x6E-0x7D) in the box x, y, w, h; flags as for Menu_DrawArrow. The lobby draws its
 * scroll arrows with it. */
void Chat_DrawArrow(s16 x, s16 y, s16 width, s16 height, u32 color, u8 flags)
{
    Sprite arrow;

    arrow.v0 = 0x6E;
    arrow.y = y;
    arrow.v1 = 0x7D;
    arrow.h = height;
    arrow.color = color;
    if (flags & ARROW_MIRROR) {
        arrow.u0 = 0x71;
        arrow.u1 = 0x7D;
    } else {
        arrow.u0 = 0x7D;
        arrow.u1 = 0x71;
    }
    if (flags & ARROW_TEXRECT) {
        arrow.x = x;
        arrow.w = width;
        TexRect_Draw((struct TexRect *)&arrow, 2);
    } else {
        arrow.x = x;
        arrow.w = width;
        Sprite_Draw(&arrow);
    }
}

/* A piece of the target marker (gTargetMarkerPieces, 10 bytes). */
typedef struct MarkPiece {
    s16 dx;                     /* 0x0 */
    s16 dy;                     /* 0x2 */
    s16 w;                      /* 0x4 */
    s16 h;                      /* 0x6 */
    s8 mode;                    /* 0x8: TexRect_Draw's */
} MarkPiece;

extern MarkPiece gTargetMarkerPieces[4];
void Mtx_Identity(f32 *m);
s32 Gs_SetState(u8 reg, u32 v);
void Camera_ProjectPoint(f32 *out, f32 *v);

/* Draws the target marker at a world position, when it projects on the screen. */
void Hud_DrawTargetMarker(f32 *pos)
{
    u32 i;
    MarkPiece *piece;
    s16 x;
    s16 y;
    f32 mtx[16];
    f32 screen[4];
    Sprite spr;

    Mtx_Identity(mtx);
    Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
    Camera_ProjectPoint(screen, pos);
    if (screen[3] <= 0.0f) {
        return;
    }
    x = 1.25f * screen[0];
    if (x < -0x1C || x > 0x29C) {
        return;
    }
    y = screen[1];
    if (y < -0x14 || y > 0x1D4) {
        return;
    }
    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    spr.color = 0xE4FF0000;
    spr.x = x - 1;
    spr.y = y - 1;
    spr.w = 3;
    spr.h = 3;
    *(u32 *)&spr.u0 = TEX_UV(0x70, 0x92);
    *(u32 *)&spr.u1 = TEX_UV(0x72, 0x94);
    Sprite_Draw(&spr);
    *(u32 *)&spr.u0 = TEX_UV(0x6D, 0x8F);
    *(u32 *)&spr.u1 = TEX_UV(0xC0, 0x9A);
    piece = gTargetMarkerPieces;
    for (i = 4; i != 0; i--) {
        spr.x = x + piece->dx;
        spr.y = y + piece->dy;
        spr.w = piece->w;
        spr.h = piece->h;
        TexRect_Draw((struct TexRect *)&spr, piece->mode);
        piece++;
    }
}

/* Sets up a grid over the items Recipe_IsListedFor lists for filter (items 1 to 0x398), for
 * the cursor at grid->index, and writes the items of the cursor's column to out (zeros after
 * them). Returns 0 when there are none or the cursor is past them, else 1. */
s32 CombineList_Setup(Grid *grid, s16 *out, s32 filter)
{
    s32 i;
    s32 listed;
    s32 start;
    s32 end;
    s32 col;
    s32 item;
    s32 rows;

    grid->filter = filter;
    for (i = 0; i < (s32)grid->rows; i++) {
        out[i] = 0;
    }
    rows = grid->rows;
    col = grid->index / rows;
    start = rows * col;
    end = start + rows;
    for (item = 1, listed = 0; item < 0x399; item++) {
        if (Recipe_IsListedFor(item, filter) == 1) {
            if (listed >= start && listed < end) {
                *out++ = item;
            }
            listed++;
        }
    }
    if (listed == 0) {
        return 0;
    }
    if (listed <= grid->index) {
        return 0;
    }
    grid->cols = (listed + (grid->rows - 1)) / grid->rows;
    grid->lastRows = listed - grid->rows * (grid->cols - 1);
    grid->col = col;
    grid->row = grid->index % grid->rows;
    return 1;
}

/* Moves a grid's cursor by the buttons and writes the items of its column (as
 * CombineList_Setup lists them, with the grid's filter) to out. */
void CombineList_MoveCursor(Grid *grid, s16 *out, u16 buttons)
{
    s32 i;
    s32 start;
    s32 end;
    s16 listed;
    s32 item;

    Menu_GridCursor(grid, buttons);
    for (i = 0; i < grid->rows; i++) {
        out[i] = 0;
    }
    start = grid->rows * grid->col;
    end = start + (grid->col < grid->cols - 1 ? grid->rows : grid->lastRows);
    for (item = 1, listed = 0; listed < end; item++) {
        if (Recipe_IsListedFor(item, grid->filter) == 1) {
            if (listed >= start && listed < end) {
                *out++ = item;
            }
            listed++;
        }
    }
}

/* 1 if any monster note is known (gSave.noteKnown). */
s32 MonsterNote_AnyKnown(void)
{
    s32 i;

    for (i = 0; i < 6; i++) {
        if (gSave.noteKnown[i] != 0) {
            return 1;
        }
    }
    return (gSave.noteKnown[i] & 0xF) != 0;   /* the last 4 of MONSTER_NOTE_COUNT */
}

/* Marks monster note `kind` known; returns 0 for a number past MONSTER_NOTE_COUNT, else 1.
 * The byte's address is worked out first, so that the load and store share it. */
s32 MonsterNote_SetKnown(s32 kind)
{
    s32 note;
    u8 *bits;

    note = kind & 0xFF;
    if (note >= MONSTER_NOTE_COUNT) {
        return 0;
    }
    bits = (u8 *)gSave.noteKnown + (note >> 3);
    *bits |= (1 << (note & 7)) & 0xFF;
    return 1;
}

/* The number of monster notes known. */
s32 MonsterNote_CountKnown(void)
{
    u32 i;
    s32 count;

    count = 0;
    for (i = 0; i < MONSTER_NOTE_COUNT; i++) {
        if (gSave.noteKnown[i >> 3] & (1 << (i & 7))) {
            count++;
        }
    }
    return count;
}

/* The next known monster note from cur in the direction dir (wrapping round), cur itself
 * when it is negative (from note 0); -1 when none is known. */
s8 MonsterNote_NextKnown(s8 cur, s8 dir)
{
    s32 i;

    if (MonsterNote_AnyKnown() == 1) {
        if (cur < 0) {
            cur = 0;
        } else {
            cur += dir;
            if (cur > MONSTER_NOTE_COUNT - 1) {
                cur = 0;
            }
            if (cur < 0) {
                cur = MONSTER_NOTE_COUNT - 1;
            }
        }
        for (i = MONSTER_NOTE_COUNT; i != 0; i--) {
            if (!!(gSave.noteKnown[cur >> 3] & (1 << (cur & 7)))) {
                return cur;
            }
            if (dir >= 0) {
                cur++;
                if (cur > MONSTER_NOTE_COUNT - 1) {
                    cur = 0;
                }
            } else if (cur <= 0) {
                cur = MONSTER_NOTE_COUNT - 1;
            } else {
                cur--;
            }
        }
    }
    return -1;
}

/* The number shown for monster note `limit`: 1 plus the known notes before it. */
u8 MonsterNote_Number(u8 limit)
{
    s32 i;
    s32 number;

    if (limit > MONSTER_NOTE_COUNT) {
        limit = MONSTER_NOTE_COUNT;
    }
    number = 1;
    for (i = 0; i < limit; i++) {
        if (gSave.noteKnown[i >> 3] & (1 << (i & 7))) {
            number++;
        }
    }
    return number;
}

/* The texture coordinates (0 to 1) of picture id's cell in the pictures' texture, a grid of
 * 42-pixel cells six across (gEntryPictureIndex gives the cell). For a picture without a
 * cell (0xFF), the stand-in's and 0; else 1. */
s32 EntryPicture_GetUVf(f32 *uv0, f32 *uv1, u16 id)
{
    u8 cell;
    s32 x;
    s32 y;

    cell = gEntryPictureIndex[id];
    if (cell == 0xFF) {
        uv0[0] = 43.0f / 256;
        uv0[1] = 211.0f / 256;
        uv1[0] = 83.0f / 256;
        uv1[1] = 251.0f / 256;
        return 0;
    }
    x = cell % 6 * 42;
    uv0[0] = (1.0f / 256) * (x + 1);
    y = cell / 6 * 42;
    uv0[1] = (1.0f / 256) * (y + 1);
    uv1[0] = (1.0f / 256) * (x + 41);
    uv1[1] = (1.0f / 256) * (y + 41);
    return 1;
}

/* EntryPicture_GetUVf in texels: the cell's corners as TEX_UV words (a Sprite's u0, v0 and
 * u1, v1). The monster notes and the lobby draw their pictures with it. */
s32 EntryPicture_GetUV(s32 *uv0, s32 *uv1, u16 id)
{
    s32 texV;
    s32 texU;
    u8 *entry;
    u8 cell;

    entry = &gEntryPictureIndex[id];
    cell = *entry;
    if (cell == 0xFF) {
        *uv0 = TEX_UV(0x2B, 0xD3);
        *uv1 = TEX_UV(0x53, 0xFB);
        return 0;
    }
    texU = ((s32) cell % 6) * 0x2A;
    texV = ((*entry / 6) + (cell >> 0x1F)) * 0x2A;
    *uv0 = ((texV + 1) << 0x10) + (texU + 1);
    *uv1 = ((texV + 0x29) << 0x10) + (texU + 0x29);
    return 1;
}

/* EntryPicture_GetUV through gWantedPictureCells: the corners of a wanted entry's picture in
 * the pictures' texture (TEX_SLOT_PICTURES). The market's wanted lists draw their monsters'
 * pictures with it (id 0 the locked picture). */
s32 WantedPicture_GetUV(s32 *uv0, s32 *uv1, u16 id)
{
    s32 texV;
    s32 texU;
    u8 *entry;
    u8 cell;

    entry = &gWantedPictureCells[id];
    cell = *entry;
    if (cell == 0xFF) {
        *uv0 = TEX_UV(0x2B, 0xD3);
        *uv1 = TEX_UV(0x53, 0xFB);
        return 0;
    }
    texU = ((s32) cell % 6) * 0x2A;
    texV = ((*entry / 6) + (cell >> 0x1F)) * 0x2A;
    *uv0 = ((texV + 1) << 0x10) + (texU + 1);
    *uv1 = ((texV + 0x29) << 0x10) + (texU + 0x29);
    return 1;
}

void HelpWindow_Draw(u8 a, u16 b, s8 c);
extern u8 gHelpPage;
extern u16 gHelpLine;
extern s8 gHelpColumn;

/* Draws the help window at the page, line and column it is on. */
void HelpWindow_DrawCurrent(void)
{
    HelpWindow_Draw(gHelpPage, gHelpLine, gHelpColumn);
}

void HelpWindow_Draw(u8 a, u16 b, s8 c);

/* Draws the help window's page and line without a column cursor. */
void HelpWindow_DrawLine(u8 page, u16 line)
{
    HelpWindow_Draw(page, line, -1);
}

extern Panel gHelpWindowBox;
extern char **gHelpTexts[];
extern u32 gItemColors[];
extern char *gRarityFmt[];
void Deco_DrawDetails(u16 item, s16 x, s16 y, s8 c, u8 pal);
s16 Rarity_Color(u8 i);

/* Draws the help window with line `line` of help page `page` (HELP_LINE_NONE: empty). An
 * item's line on HELP_PAGE_ITEMS (HELP_ITEM_LINE(item)) shows instead the item's
 * decoration details (Deco_DrawDetails, in view `column`), its icon (with the mark that
 * ItemData.flags2 bit 0 asks for) and its rarity. */
void HelpWindow_Draw(u8 page, u16 line, s8 column)
{
    Sprite spr;
    ItemData *data;
    char *text;
    s32 item;

    if (page > HELP_PAGE_OPTIONS) {
        return;
    }
    if (page == HELP_PAGE_ITEMS && line >= HELP_ITEM_LINE(0)) {
        Window_DrawText(&gHelpWindowBox, 0, (u8 *)0);
        if (line > HELP_ITEM_LINE(0)) {
            item = line - HELP_ITEM_LINE(0);
            Deco_DrawDetails(item, 0x168, 0x167, column, 0);
            data = &gItemData[item];
            Tex_Prepare(1, 0x144);
            Tex_Bind(0x144);
            spr.x = 0x132;
            spr.y = 0x166;
            spr.w = 0x32;
            spr.h = 0x28;
            spr.u0 = ((data->icon + 1) & 7) * 32 + 1;
            spr.v0 = ((data->icon + 1) >> 3) * 32 + 1;
            spr.u1 = spr.u0 + 0x1E;
            spr.v1 = spr.v0 + 0x1E;
            spr.color = gItemColors[data->color];
            Sprite_Draw(&spr);
            if (data->flags2 & 1) {
                *(u32 *)&spr.u0 = TEX_UV(0x80, 0xE0);
                *(u32 *)&spr.u1 = TEX_UV(0xA0, 0x100);
                spr.color = 0xFFFFFFFF;
                Sprite_Draw(&spr);
            }
            Font_SetSpacing(0x10, 0x10);
            Font_SetPos(0x132, 0x192);
            Text_SetColor(Rarity_Color(data->rarity));
            Text_PrintfColored(gRarityFmt[0], data->rarity + 1);
        }
    } else {
        text = NULL;
        if (line != HELP_LINE_NONE) {
            text = gHelpTexts[page][line];
        }
        Window_DrawText(&gHelpWindowBox, 0, (u8 *)text);
    }
}

extern u32 gDecoDetailColors[][3];
extern Icon gDecoDetailIcon;
extern char *gDecoModeFmt[];
extern char *gDecoDetailLabels[];
extern char *gHunterClassTexts[];
extern char *gRangeClassTexts[];
extern char *gNoSkillText[];
extern char *gDecoSkillPtsFmt[];
extern s16 gDecoDetailPos[4][2];
extern char *gSkillNames[];
void Draw_Icons(Icon *icons, u8 n);
void Text_DrawLinesColored(s16 x, s16 y, char **lines, s16 dy);
void Text_DrawDigit(u8 n);
void Hud_DrawTab(s16 x, s16 y, u8 flags, u8 lvl, u32 color);

/* Draws one page of an item's decoration details at x, y, in the colours
 * gDecoDetailColors[pal]. view & 3 is the page: 0 the item's description, 1 who can use it
 * (the hunter class from DecoData.flags 4 and 8, the range class from 1 and 2) and its size,
 * 2 its skills and points (skills 0x29-0x2B show no points). view & 0x10 draws the page
 * number with an icon instead of a tab. A negative view, or an item that is not a
 * decoration, gives the description. */
void Deco_DrawDetails(u16 item, s16 x, s16 y, s8 view, u8 pal)
{
    DecoData *record;
    u32 *colors;
    u8 page;
    u8 *skill;
    u32 i;
    s16 *pos;
    s32 top;
    s32 textY;
    s32 left;
    s32 textX;
    u8 kind;
    ItemData *data;

    colors = gDecoDetailColors[pal];
    data = &gItemData[item];
    if (data->deco == 0 || view < 0) {
        page = 0;
    } else {
        record = &gDecoTable[data->deco];
        page = view & 3;
        Tex_Prepare(1, TEX_SLOT_ICONS);
        Tex_Bind(TEX_SLOT_ICONS);
        if (view & 0x10) {
            Font_SetColor(colors[0]);
            Font_SetPos(x + 0xAB, y + 0x29);
            Text_Printf(gDecoModeFmt[0], page + 1);
            gDecoDetailIcon.x = x + 0x93;
            gDecoDetailIcon.y = y + 0x28;
            Draw_Icons(&gDecoDetailIcon, 1);
        } else {
            Hud_DrawTab(x - 0x34, y - 0x3B, (page | 0x40) + 1, 3, colors[0]);
        }
        Font_SetSpacing(0xF, 0x12);
    }
    switch (page) {
    case 0:
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(x, y);
        Font_SetColor(colors[0]);
        Text_Printf(gHelpTexts[HELP_PAGE_ITEMS][HELP_ITEM_LINE(item)]);
        break;
    case 1:
        Font_SetColor(colors[1]);
        Text_DrawLinesColored(x, y, gDecoDetailLabels, 0x14);
        Font_SetColor(colors[2]);
        textX = x + 0x69;
        textY = y;
        Font_SetPos(textX, textY);
        if ((record->flags & 0xC) == 0xC) {
            kind = 2;
        } else if (record->flags & 4) {
            kind = 0;
        } else if (record->flags & 8) {
            kind = 1;
        } else {
            kind = 3;
        }
        Text_Draw(gHunterClassTexts[kind]);
        Font_SetPos(textX, textY + 0x13);
        if ((record->flags & 3) == 3) {
            kind = 2;
        } else if (record->flags & 1) {
            kind = 0;
        } else if (record->flags & 2) {
            kind = 1;
        } else {
            kind = 3;
        }
        Text_Draw(gRangeClassTexts[kind]);
        Font_SetPos(textX, textY + 0x26);
        Text_DrawDigit(record->size);
        break;
    case 2:
        Font_SetColor(colors[0]);
        skill = record->skills[0];
        top = y;
        left = x;
        for (i = 0, pos = gDecoDetailPos[0]; i < 4; i++, pos += 2, skill += 2) {
            Font_SetPos(left + pos[0], top + pos[1]);
            if (skill[0] == 0) {
                if (i == 0) {
                    Text_Draw(gNoSkillText[0]);
                }
                break;
            }
            Text_Draw(gSkillNames[skill[0]]);
            if (skill[0] != 0x29 && skill[0] != 0x2A && skill[0] != 0x2B) {
                Font_SetPos(left + pos[0] + 0x4B, top + pos[1]);
                Text_PrintfColored(gDecoSkillPtsFmt[0], (s8)skill[1]);
            }
        }
        break;
    }
}

/* Notice_OpenText with the conversation's title (gTalkEventTitle) and no list. */
s32 Notice_OpenTalk(char *text, u32 width, s32 mode, s32 sel)
{
    if (mode > NOTICE_MODE_CLOSE) {
        return -1;
    }
    return Notice_OpenText(text, width, mode, sel, gTalkEventTitle, NULL);
}

/* Notice_OpenText with no list; the lobby's and the game overlay's conversations open their
 * texts with it, an NPC's name as the title. */
s32 Notice_OpenTextN(char *text, u32 width, s32 mode, s32 sel, s32 title)
{
    if (mode > NOTICE_MODE_CLOSE) {
        return -1;
    }
    return Notice_OpenText(text, width, mode, sel, title, NULL);
}

extern u8 gChatInputMode;
extern s16 gNoticeShownChars;
extern s8 gNoticeFlags;
extern s8 gNoticeNextSePlayed;
extern u32 gNoticeWidth;
extern char **gNoticeList;
extern s32 gNoticeTitle;
extern u8 gNoticeSel;
extern s8 D_0043C73A;
s32 Text_NextChar(char *out, char *s, char **next);

/* Opens the notice window (Notice_Draw) on text under the title (a char pointer, 0 none),
 * and shows as much of it as width allows. The callers raise width with time, so the text
 * types out: each character costs 3, a full stop or question mark 15 and a comma 8 (a pause
 * after them). A sound plays for each new character. Returns 1 while text remains; once it
 * is all shown, sets up what mode asks for (NOTICE_MODE_*) and returns 0. -1 while the chat
 * keyboard is open, or for a list mode without a list. */
s32 Notice_OpenText(char *text, u32 width, s32 mode, s32 sel, s32 title, char **list)
{
    char ch[4];
    s16 kind;
    u32 budget;
    u32 chars;
    u32 cost;
    u16 code;

    if (gChatInputMode != 0) {
        return -1;
    }
    if (mode == NOTICE_MODE_CLOSE) {
        gNoticeFlags = NOTICE_CLOSED;
        return 0;
    }
    gNoticeTitle = title;
    if (gNoticeWidth > width || gChat.notice != text) {
        gNoticeShownChars = 0;
        gNoticeNextSePlayed = 0;
    }
    gNoticeWidth = width;
    gNoticeFlags = NOTICE_OPEN;
    budget = width;
    chars = 0;
    gChat.notice = text;
    for (;;) {
        kind = Text_NextChar(ch, text, &text);
        if (kind == 4) {
            break;
        }
        if (ch[0] == '\n') {
            continue;
        }
        chars++;
        cost = 3;
        if (kind == 1) {
            code = (ch[0] << 8) + (u8)ch[1];
            if (code == 0x8142 || code == 0x8148) {
                cost = 15;
            } else if (code == 0x8141) {
                cost = 8;
            }
        } else if (kind == 0) {
            if (ch[0] == '.' || ch[0] == '?') {
                cost = 15;
            }
        }
        if (cost > budget) {
            if (chars > gNoticeShownChars) {
                if (gScene.inLobby == 0) {
                    Sound_PlaySeFlat(6, 0x1D, D_0043C73A);
                } else {
                    Sound_PlaySeFlat(1, 0x84, 0);
                }
            }
            gNoticeShownChars = chars;
            return 1;
        }
        budget -= cost;
    }
    if (chars > gNoticeShownChars) {
        if (gScene.inLobby == 0) {
            Sound_PlaySeFlat(6, 0x1D, D_0043C73A);
        } else {
            Sound_PlaySeFlat(1, 0x84, 0);
        }
    }
    gNoticeShownChars = chars;
    switch (mode) {
    case NOTICE_MODE_PROMPT:
        gNoticeFlags |= NOTICE_PROMPT;
        if (sel == 0) {
            gNoticeFlags |= NOTICE_PROMPT_YES;
        }
        gNoticeSel = sel;
        gNoticeList = NULL;
        break;
    case NOTICE_MODE_NEXT:
        gNoticeFlags |= NOTICE_NEXT;
        if (gNoticeNextSePlayed == 0) {
            gNoticeNextSePlayed++;
            if (gScene.inLobby == 0) {
                Sound_PlaySeFlat(6, 0x1F, D_0043C73A);
            } else {
                Sound_PlaySeFlat(1, 0x86, 0);
            }
        }
        break;
    case NOTICE_MODE_LIST:
        if (list == NULL) {
            return -1;
        }
        gNoticeList = list;
        gNoticeSel = sel;
        gNoticeFlags |= NOTICE_LIST;
        break;
    case NOTICE_MODE_PICKED:
        if (list == NULL) {
            return -1;
        }
        gNoticeList = list;
        gNoticeSel = sel;
        gNoticeFlags |= NOTICE_LIST_PICKED;
        break;
    }
    return 0;
}

/* The first pad's held and pressed buttons for the menus, with the right stick's directions
 * added as the D-pad's buttons (one direction each). */
void Pad_GetMenuButtons(u16 *held, u16 *pressed)
{
    *held = gPadState[0].held;
    *pressed = gPadState[0].pressed;
    if (gPadState[0].dir & PAD_RSTICK_UP) {
        *held |= PAD_UP;
    } else if (gPadState[0].dir & PAD_RSTICK_DOWN) {
        *held |= PAD_DOWN;
    } else if (gPadState[0].dir & PAD_RSTICK_LEFT) {
        *held |= PAD_LEFT;
    } else if (gPadState[0].dir & PAD_RSTICK_RIGHT) {
        *held |= PAD_RIGHT;
    }
    if (gPadState[0].dirPressed & PAD_RSTICK_UP) {
        *pressed |= PAD_UP;
    } else if (gPadState[0].dirPressed & PAD_RSTICK_DOWN) {
        *pressed |= PAD_DOWN;
    } else if (gPadState[0].dirPressed & PAD_RSTICK_LEFT) {
        *pressed |= PAD_LEFT;
    } else if (gPadState[0].dirPressed & PAD_RSTICK_RIGHT) {
        *pressed |= PAD_RIGHT;
    }
}

s8 Kbd_Run(char *out, s32 b, s16 flags); /* unit's own declaration: declared here as Chat_RunKbd calls it; with the definition's s8 (char *dst, s16 held, s16 pressed), the call compiles differently */

/* Runs the chat keyboard for a frame (Kbd_Run): its result, or 0xFF (cancelled) with an
 * empty line when SELECT is pressed. */
u8 Chat_RunKbd(char *out, s32 held, s16 pressed)
{
    if (pressed & PAD_SELECT) {
        *out = 0;
        return 0xFF;
    }
    return Kbd_Run(out, held, pressed);
}

void Kbd_SetStartActive(u8 v);
/* unit's own declaration: declared here as Chat_OpenKbd calls it; with the definition's void (s32 kind, u8 layout, u16 max, char *text), the call compiles differently */
void Kbd_Open(u8 a, s32 b, u16 c, char *text);
void Chat_SetTarget(s8 target);

/* Opens the software keyboard for chat: Kbd_Open's kind is flags' low byte, and
 * CHAT_KBD_INACTIVE and CHAT_KBD_TARGETED set its focus and the chat target. The lobby
 * opens its text fields with it too. */
void Chat_OpenKbd(s32 flags, s32 layout, u16 maxChars, void *text)
{
    Kbd_SetStartActive(!(flags & CHAT_KBD_INACTIVE));
    Kbd_Open(flags, layout, maxChars, text);
    if (flags & CHAT_KBD_TARGETED) {
        Chat_SetTarget(0);
    } else {
        Chat_SetTarget(-1);
    }
}

s32 TownPc_IsSeatedState(u8 a, u8 b);
s32 CamFocus_IsOn(void);
s32 BaitMenu_IsOpen(void);
s32 Hud_SupplyBoxOpen(void);
s32 Hud_SupplyBoxActive(void);
s32 Hud_DeliveryOpen(void);

/* 1 if the local player may chat now: in the town only online, not while seated at the town's PC, a camera
 * focus or the bait menu; outside it, not while the supply box or the delivery menu is open. */
s32 Chat_IsAvailable(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    if (gScene.inLobby != 0) {
        if (Game_IsOnline() == 0) {
            return 0;
        }
        if (TownPc_IsSeatedState(pl->stateGroup, pl->state) == 1 || CamFocus_IsOn() == 1) {
            return 0;
        }
        if (BaitMenu_IsOpen() == 1) {
            return 0;
        }
    } else if (Hud_SupplyBoxOpen() == 1 || Hud_SupplyBoxActive() == 1 || Hud_DeliveryOpen() == 1) {
        return 0;
    }
    return 1;
}

/* Sets gChatSendTo: offline CHAT_SEND_LOCAL. Online, CHAT_SEND_PARTY while gScene.inLobby
 * is 0; else the chat window's choice (gChatWindowSel), or CHAT_SEND_LOBBY in AREA_HUB_0x83. */
void Chat_PickSendTarget(void)
{
    if (Game_IsOnline() == 1) {
        if (gScene.inLobby != 0) {
            if (gScene.area == AREA_HUB_0x83) {
                gChatSendTo = CHAT_SEND_LOBBY;
                return;
            }
            gChatSendTo = gChatWindowSel;
            return;
        }
        gChatSendTo = CHAT_SEND_PARTY;
        return;
    }
    gChatSendTo = CHAT_SEND_LOCAL;
}

extern s32 gChatSendWait;
extern u8 gChatInputMode;
void Kbd_SetPos(s32 n, f32 f);
void Kbd_SetSubmitWait(s32 arg0);
void Chat_SendInput(u16 a);

/* Opens the chat input line on the software keyboard, sized for the target's window (0x2C
 * characters, the lobby's 0x3C), and runs its first frame. active 0 opens it without the
 * focus. */
void Chat_OpenInput(u8 active)
{
    u32 flags;
    s32 maxChars;

    flags = 0;
    if (Chat_IsAvailable() != 1) {
        flags = CHAT_KBD_UNAVAILABLE;
    }
    switch (gChatSendTo) {
    case CHAT_SEND_TARGETS:
    case CHAT_SEND_PARTY:
        flags |= CHAT_KBD_TARGETED;
    case CHAT_SEND_LOCAL:
        gChatWinOpen = gChatWinTimer = 0;
        maxChars = 0x2C;
        break;
    case CHAT_SEND_LOBBY:
        gChatLog.win.open = gChatLog.win.timer = 0;
        maxChars = 0x3C;
        break;
    }
    if (active == 0) {
        flags |= CHAT_KBD_INACTIVE;
    }
    Kbd_SetPos(0x4B, 80.0f);
    Chat_OpenKbd(flags, 0xE, maxChars, 0);
    Kbd_SetSubmitWait(gChatSendWait);
    gChatInputMode++;
    Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
    Chat_SendInput(0);
}

s32 Chat_FilterIfNotBlank(u8 *s);
void Chat_PlayAct(s32 arg0);
void Chat_AddReceived(u8 arg0, s8 *arg1);
void Chat_ResetScroll(void);
char *strcpy(char *dst, char *src);
s32 Chat_SetRecipients(s8 *msg);
void Chat_ClearLog(void);
void func_792A50(u8 who, s8 *msg, s32 c);
void Chat_SendToTargets(u8 who, s8 *msg, u8 to); /* unit's own declaration: the definition (lobby) is void Chat_SendToTargets(s32 arg0, void *arg1, s32 arg2); the calls here match only with this one */
void QSync_SendChat(u8 who, s32 a, s8 *msg, u8 to);
void Kbd_Cancel(void);
extern s8 gChatSystemRecord[];
extern char *gChatTargetTexts[];
extern ChatMsg gChatSendRecord;    /* the record of a line sent offline (CHAT_SEND_LOCAL) */
extern u8 gChatMsgClass;
extern u16 gChatRecipients;

/* Runs the chat input for a frame. When it ends with a line, sends it by gChatSendTo
 * (CHAT_SEND_*) after the word filter, plays an act tag's gesture, or logs why the
 * recipients are wrong (gChatSystemRecord); then closes the input. While it runs the chat
 * windows stay closed. */
void Chat_SendInput(u16 unused)
{
    s8 text[0x40];
    s16 held;
    s16 pressed;
    Player *pl;
    s8 result;
    s8 recipients;
    u8 to;

    Pad_GetMenuButtons((u16 *)&held, (u16 *)&pressed);
    text[0] = 0;
    result = (s8)Chat_RunKbd((char *)text, held, pressed);
    if (result != 0) {
        if (text[0] != 0 && result > 0 && Chat_FilterIfNotBlank((u8 *)text) != 0) {
            recipients = Chat_SetRecipients(text);
            if (recipients < 0) {
                strcpy(((ChatMsg *)gChatSystemRecord)->text, gChatTargetTexts[(s8)(1 - recipients)]);
                Chat_AddReceived(0xFF, gChatSystemRecord);
            } else {
                Chat_PlayAct((s32)text);
                if (text[0] != 0) {
                    gChatSendWait = 0x3C;
                    pl = &gPlayers[gScene.localPlayer];
                    switch (gChatSendTo) {
                    case CHAT_SEND_LOBBY:
                        func_792A50(pl->index, text, 0);
                        break;
                    case CHAT_SEND_TARGETS:
                        Chat_SendToTargets(pl->index, text, gChatMsgClass);
                        break;
                    case CHAT_SEND_PARTY:
                        switch (gChatMsgClass) {
                        default:
                            to = 0xFF;
                            break;
                        case 1:
                            to = gChatRecipients & 0xF;
                            break;
                        }
                        Chat_LogAdd(pl->index, text, NULL, 0);
                        QSync_SendChat(pl->index, 1, text, to);
                        gChatWinOpen = 1;
                        gChatWinTimer = 0x12C;
                        break;
                    case CHAT_SEND_LOCAL:
                        gChatSendRecord.name[0] = 0;
                        strcpy(gChatSendRecord.text, (char *)text);
                        Chat_ClearLog();
                        Chat_AddReceived(gScene.localPlayer, (s8 *)&gChatSendRecord);
                        gChatSendWait = 0;
                        break;
                    }
                }
            }
        }
        Kbd_Cancel();
        gChatInputMode = 0;
        Chat_ResetScroll();
    } else if (gChatSendTo == CHAT_SEND_LOBBY) {
        gChatLog.win.open = gChatLog.win.timer = 0;
    } else {
        gChatWinOpen = gChatWinTimer = 0;
    }
}

/* Strips the act tags from a chat line, and has the local player play the first one's
 * gesture (1-12; Player_RequestGesture) when the scene allows it. */
void Chat_PlayAct(s32 text)
{
    s32 tags;
    u32 act;

    tags = Chat_StripActTags((s8 *) text);
    act = tags & 0xF;
    if ((tags != 0) && (act != 0) && (act < 0xDU) && ((Game_IsOnline() == 1) || (gScene.inLobby == 0))) {
        Player_RequestGesture(act & 0xFF);
        Hud_SetCancel();
    }
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/menu_windows", gStatusFmtInt);
INCLUDE_RODATA("asm/sub_main/nonmatchings/menu_windows", gWindowFmtString);

s8 *Str_Find(s8 *s, s8 *t);
s8 Chat_ParseActTag(s8 *s);

/* Blanks the "<actNN>" tags of a chat message (Chat_ParseActTag) and closes the gaps; returns the first tag's number, 0 if none. */
s32 Chat_StripActTags(s8 *src)
{
    s8 *dst = src;
    s32 found = 0;
    s8 *tag;
    s8 act;
    s8 *scan;

    while (*src != 0) {
        if ((tag = Str_Find(src, (s8 *)"<act")) == NULL) {
            break;
        }
        act = Chat_ParseActTag(tag);
        if (act >= 0) {
            if (found == 0) {
                found = act;
            }
            tag += 3;
        }
        src = tag + 4;
    }
    if (found != 0) {
        for (scan = dst; ; scan++) {
            if (*scan == 1) {
                if (scan >= dst) {
                    dst = scan + 1;
                }
                while (*dst == 1) {
                    dst++;
                }
                *scan = *dst;
                *dst = 1;
                dst++;
            }
            if (*scan == 0) {
                break;
            }
        }
    }
    return found;
}

/* Reads the gesture number 1-12 of an act tag at tag (two digits at 4-5, then '>'), and
 * blanks the tag with 1s for Chat_StripActTags; -1 if it isn't one. */
s8 Chat_ParseActTag(s8 *tag)
{
    s8 act;

    if (tag[6] == '>' && tag[4] >= '0' && tag[4] <= '1') {
        act = (tag[4] - '0') * 10;
        if (tag[5] >= '0' && tag[5] <= '9') {
            act += (s8)(tag[5] - '0');
            if (act > 0 && act <= 12) {
                tag[0] = tag[1] = tag[2] = tag[3] = tag[4] = tag[5] = tag[6] = 1;
                return act;
            }
        }
    }
    return -1;
}

/* 1 if str starts with prefix. Kept for the match: the loop is written with gotos; as a plain loop it compiles differently. */
static s32 Str_HasPrefix(s8 *str, s8 *prefix)
{
    s8 *scan;
    s8 *rest;
    s8 ch;

    scan = str;
    rest = prefix;
    ch = *rest;
    if (ch != 0) {
loop_2:
        if (*scan != ch) {
            return 0;
        }
        rest += 1;
        ch = *rest;
        scan += 1;
        if (ch == 0) {
            goto block_6;
        }
        goto loop_2;
    }
block_6:
    return 1;
}

/* The first place in s where t starts (Str_HasPrefix compares the rest). */
s8 *Str_Find(s8 *str, s8 *sub)
{
    for (; *str != 0; str++) {
        if (*str == *sub && Str_HasPrefix(str + 1, sub + 1) == 1) {
            return str;
        }
    }
    return NULL;
}

char *strcpy(char *dst, char *src);
extern u8 gChatWhoColors[];
extern u8 gChatClassColors[];


extern u8 gChatMsgClass;
extern char gMyUserId[];

/* Adds a message from player `who` to the chat ring (gChat.lines), with the sender and
 * colours from header (without one, the local player's), wrapped into at most two rows of
 * 0x16 bytes (0x1E when wide), a Shift-JIS pair never split. */
void Chat_LogAdd(u8 who, s8 *msg, ChatMsg *header, u8 wide)
{
    ChatLine *line;
    s32 i;
    s32 left;
    s32 width;
    s8 *dst;
    s8 *row;
    s32 ch;

    if (*msg == 0) {
        return;
    }
    line = &gChat.lines[gChatLogHead];
    line->who = who;
    if (header != NULL) {
        line->color[0] = header->cls;
        line->color[1] = header->color[0];
        line->color[2] = header->color[1];
        line->color[3] = header->color[2];
        strcpy((char *)line->id, header->id);
        strcpy((char *)line->name, header->name);
    } else {
        line->color[0] = 0;
        line->color[1] = line->color[2] = gChatWhoColors[line->who];
        line->color[3] = gChatClassColors[gChatMsgClass];
        strcpy((char *)line->id, gMyUserId);
        strcpy((char *)line->name, (char *)gPlayers[line->who].name);
    }
    gChatLogHead += 1;
    gChatLogCount += 1;
    gChatLogHead &= 0x3F;
    if (gChatLogCount > 0x40) {
        gChatLogCount = 0x40;
    }
    line->rows = 0;
    width = wide == 0 ? 0x16 : 0x1E;
    for (i = 0, row = line->text[0]; i < 2; i++, row += 0x18) {
        dst = row;
        if (*msg == 0) {
            *row = 0;
            return;
        }
        left = width;
        row[width] = 0;
        do {
            if (*msg == 0) {
                *dst = 0;
                line->rows++;
                return;
            }
            ch = *msg & 0xFF;
            if ((ch >= 0x80 && ch <= 0x9F) || (ch >= 0xE0 && ch <= 0xFF)) {
                *dst = ch;
                dst[1] = msg[1];
                left -= 2;
                msg += 2;
                dst += 2;
            } else {
                *dst = ch;
                msg++;
                dst++;
                left--;
            }
        } while (left > 0);
        *dst = 0;
        line->rows++;
    }
}

void Chat_Beep(void);

/* Logs a received ChatMsg (after the word filter), scrolls the log on when it is full, and
 * opens the chat window for 0x12C frames (beeping when the line is another player's). The
 * text is reached as msg + 0x1C (ChatMsg.text): through the member, MWCC keeps the address
 * in a register and the code differs. */
void Chat_AddReceived(u8 who, s8 *msg)
{
    Chat_FilterWords((char *) (msg + 0x1C));    /* ChatMsg.text; see above */
    Chat_LogAdd(who, (s8 *) (msg + 0x1C), (ChatMsg *) msg, 0);
    if (Chat_CountLogRows() > 11U) {
        gChatLogScroll += 1;
        if ((s32) gChatLogScroll > 0x3F) {
            gChatLogScroll = 0x3F;
        }
    }
    if (((who & 0xFF) != gScene.localPlayer) && (gChatWinOpen == 0)) {
        Chat_Beep();
    }
    gChatWinOpen = 1;
    gChatWinTimer = 0x12C;
}

/* Lets the next message beep again. */
void Chat_ClearBeep(void)
{
    gChatBeeped = 0;
}

/* Plays the chat's beep (sound 0x18), once until Chat_ClearBeep. */
void Chat_Beep(void)
{
    if (gChatBeeped == 0) {
        Sound_PlaySeFlat(0, 0x18, 0);
        gChatBeeped = 1;
    }
}

extern s8 gPlayerIds[][8];

/* Logs a line from a player of this console (the party's messages, through qnet_sync.c)
 * with a beep; kind 0xFF gives it the system class colour. */
void Chat_LogAddLocal(u8 who, u8 kind, char *msg)
{
    ChatMsg header;
    u8 cls;

    Chat_FilterWords(msg);
    if (kind == 0xFF) {
        cls = 3;
    } else {
        cls = 1;
    }
    header.cls = 0;
    header.color[0] = header.color[1] = gChatWhoColors[who];
    header.color[2] = gChatClassColors[cls];
    strcpy(header.name, (char *)gPlayers[who].name);
    strcpy(header.id, (char *)gPlayerIds[who]);
    Chat_LogAdd(who, (s8 *)msg, &header, 0);
    if (Chat_CountLogRows() > 11U) {
        gChatLogScroll++;
        if (gChatLogScroll > 0x3F) {
            gChatLogScroll = 0x3F;
        }
    }
    gChatWinOpen = 1;
    gChatWinTimer = 0x12C;
    Sound_PlaySeFlat(0, 0x18, 0);
}

/* Scrolls the chat log to its newest lines (the lobby's log for CHAT_SEND_LOBBY). */
void Chat_ResetScroll(void)
{
    if (gChatSendTo == CHAT_SEND_LOBBY) {
        ChatLog_ResetScroll();
        return;
    }
    gChatLogScroll = 0;
    if (Chat_CountLogRows() < 0xCU) {
        gChatLogAtEnd = 1;
        return;
    }
    gChatLogAtEnd = 0;
}

/* Online, hides the help line and scrolls the chat log to its end for the chat window the
 * lobby opens; returns 0. Offline returns 1 and does nothing. */
s32 Chat_OpenWindow(void)
{
    if (Game_IsOnline() == 0) {
        return 1;
    }
    gHelpShow = 0;
    Chat_ResetScroll();
    return 0;
}

s32 Chat_LogRowsBefore(u8 skip);

/* Scrolls the field chat's log by the buttons while it is open for scrolling: PAD_UP goes
 * back a line, PAD_DOWN on (sets the CHAT_WIN_* arrows); PAD_START closes it and is
 * returned alone. Returns the buttons otherwise. */
u16 Chat_LogScroll(u16 buttons)
{
    if (buttons & PAD_START) {
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        return PAD_START;
    }
    gChat.helpShow = 0;
    gChat.win.flags = CHAT_WIN_SCROLLING;
    if (Chat_CountLogRows() > 11U) {
        if (gChat.win.scroll != 0) {
            gChat.win.flags |= CHAT_WIN_MORE_AFTER;
        }
        if (Chat_LogRowsBefore(gChat.win.scroll) > 11U) {
            gChat.win.flags |= CHAT_WIN_MORE_BEFORE;
            if ((buttons & PAD_UP) && gChat.win.scroll < gChat.win.count - 1) {
                gChat.win.atEnd = 0;
                gChat.win.scroll++;
                gChat.win.flags |= CHAT_WIN_STEP_BACK;
                Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                if (Chat_LogRowsBefore(gChat.win.scroll) < 12U) {
                    gChat.win.atEnd = 1;
                }
            }
        } else {
            gChat.win.atEnd = 1;
        }
        if ((buttons & PAD_DOWN) && gChat.win.scroll > 0) {
            gChat.win.scroll--;
            gChat.win.flags |= CHAT_WIN_STEP_ON;
            Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
            gChat.win.atEnd = 0;
        }
    }
    if (gChat.win.scroll == 0) {
        gChat.win.open = gChat.win.timer = 0;
    }
    return buttons;
}

/* The rows of the chat log lines older than the newest `skip` ones. */
s32 Chat_LogRowsBefore(u8 skip)
{
    s32 rows = 0;
    s32 i = gChatLogHead - 1 - skip;
    s32 n = gChatLogCount - skip;

    for (; n != 0; n--, i--) {
        ChatLine *line = &gChat.lines[i & 0x3F];
        rows += line->rows;
        if (line->id[0] != 0) {
            rows++;
        }
    }
    return rows;
}

/* Draws the chat box of the send target and the newest log lines, and stops the window's
 * timer. */
void Chat_DrawHudLog(void)
{
    u8 chatSize;

    chatSize = gSysSave.options.chatSize;
    Chat_DrawBox((s32) gChatSendTo, chatSize, 0);
    Chat_DrawLogLines(0U, 0, 0U, chatSize);
    gChatWinTimer = 0;
}

extern Panel gChatFrameBox;

/* Draws the window gChatFrameBox w wide, as tall as w * 18 / 21 lines, at the bottom of the screen; then restores its size. */
void Chat_DrawFrame(s32 text, u8 size, s32 color)
{
    s16 pos[2];
    s32 cellH;

    gChatFrameBox.charW = size;
    cellH = size * 18 / 21;
    pos[0] = 0;
    pos[1] = 0x1A4 - (cellH + 1) * 12;
    gChatFrameBox.flags = ((color + 2) & WINDOW_STYLE_MASK) | WINDOW_FILL;
    gChatFrameBox.charH = cellH - 1;
    Window_DrawText(&gChatFrameBox, pos, (u8 *)text);
    gChatFrameBox.charW = 0x15;
    gChatFrameBox.charH = 0x11;
}

extern u8 gChatHideNames;
extern char *gChatTypeTexts;

/* Formats a log line's sender row into buf: the name, or the id when gChatHideNames is set. */
void Chat_FormatSender(char *buf, ChatLine *line)
{
    if (gChatHideNames == 0) {
        sprintf(buf, gChatTypeTexts, line->name);
    } else {
        sprintf(buf, gChatTypeTexts, line->id);
    }
}

void Text_DrawShadow2(s16 x, s16 y, s32 c1, s32 c2, char *s);

/* Draws the chat log in text size `size` moved down by dy: scrolled back (the newest `scroll` lines skipped)
 * from the bottom up when it fills the window and not at its end, else from the oldest line down; lines
 * with an id get a sender row (Chat_FormatSender), rows with a wide last character a narrower font. */
void Chat_DrawLogLines(u8 scroll, s16 dy, u8 atEnd, u8 size)
{
    s32 k;
    char sender[0x20];
    s32 i;
    s32 left;
    s16 top;
    ChatLine *entry;
    s16 narrowW;
    s16 lineH;
    s16 charH;
    ChatLine *line;
    s8 *fwdRow;
    s32 count;
    s16 y;
    s8 *row;
    s16 offY;

    if (gChatLogCount == 0) {
        return;
    }
    size = gChatTextSizes[size];
    narrowW = (s16)size * 20 / 21;
    charH = (s16)size * 18 / 21;
    Font_SetSpacing(size, charH);
    lineH = charH + 1;
    offY = dy * size / 21;
    if (Chat_CountLogRows() > 11U && atEnd == 0) {
        y = 0x1A4 - lineH;
        top = 0x1A4 - lineH * 11;
        i = gChatLogHead - 1 - scroll;
        left = gChatLogCount - scroll;
        for (; left != 0; left--, i--) {
            line = &gChat.lines[i &= 0x3F];
            Text_SetColor(line->color[3]);
            k = line->rows - 1;
            if (k >= 0) {
                row = line->text[k];
                do {
                    Font_SetPos(0x18, y + offY);
                    if (row[0x16] != 0) {
                        Font_SetSpacing(narrowW, charH);
                        Text_Draw((char *)row);
                        Font_SetSpacing(size, charH);
                    } else {
                        Text_Draw((char *)row);
                    }
                    y -= lineH;
                    if (y < top) {
                        return;
                    }
                    k--;
                    row -= 0x18;
                } while (k >= 0);
            }
            if (line->id[0] != 0) {
                Chat_FormatSender(sender, line);
                Text_DrawShadow2(0x18, y + offY, 1, line->color[2], sender);
                y -= lineH;
                if (y < top) {
                    return;
                }
            }
        }
    } else {
        count = gChatLogCount;
        i = gChatLogHead - count;
        y = 0x1A4 - lineH * 11;
        for (; count != 0; count--, i++) {
            entry = &gChat.lines[i &= 0x3F];
            if (entry->id[0] != 0) {
                Chat_FormatSender(sender, entry);
                Text_DrawShadow2(0x18, y + offY, 1, entry->color[2], sender);
                y += lineH;
                if (y >= 0x1A4) {
                    return;
                }
            }
            Text_SetColor(entry->color[3]);
            k = 0;
            if (k < entry->rows) {
                fwdRow = entry->text[0];
                do {
                    Font_SetPos(0x18, y + offY);
                    if (fwdRow[0x16] != 0) {
                        Font_SetSpacing(narrowW, charH);
                        Text_Draw((char *)fwdRow);
                        Font_SetSpacing(size, charH);
                    } else {
                        Text_Draw((char *)fwdRow);
                    }
                    y += lineH;
                    if (y >= 0x1A4) {
                        return;
                    }
                    k++;
                    fwdRow += 0x18;
                } while (k < entry->rows);
            }
        }
    }
}

f32 Math_Sin(f32 x);

/* The chat log window (as wide as gChatTextSizes[gSysSave.options.chatSize]), its blinking scroll arrows (gChatLines flags) and its lines. */
void Chat_DrawLog(void)
{
    u8 size;
    s16 x;
    u32 pulse;
    u16 phase;
    u32 upColor;
    s16 y;

    Gfx_SetTexFilter(0);
    size = gChatTextSizes[(s8)gSysSave.options.chatSize];
    Chat_DrawFrame(0, size, 1);
    phase = (gFrameTick & 0x1F) << 11;
    pulse = ((s8)(48.0f * Math_Sin((6.2831855f / 65536.0f) * phase)) + 0xCF) << 24 | 0x1ACC8E;
    x = 7.0f + (size * 11) / 2;
    y = 0x194 - (size * 18 / 21 + 1) * 12;
    if (gChatLines & CHAT_WIN_MORE_BEFORE) {
        upColor = pulse;
        if (gChatLines & CHAT_WIN_STEP_BACK) {
            y -= 2;
        }
    } else {
        upColor = 0xD8707070;
    }
    Chat_DrawArrow(x, y, 0x1E, 0x18, upColor, ARROW_TEXRECT);
    y = 0x19E;
    if (gChatLines & CHAT_WIN_MORE_AFTER) {
        if (gChatLines & CHAT_WIN_STEP_ON) {
            y += 2;
        }
    } else {
        pulse = 0xD8707070;
    }
    Chat_DrawArrow(x, y, 0x1E, 0x18, pulse, ARROW_MIRROR | ARROW_TEXRECT);
    Chat_DrawLogLines(gChatLogScroll, -10, gChatLogAtEnd, (s8)gSysSave.options.chatSize);
}

s8 Kbd_IsOpen(void);
s32 Town_IsPlayerHeld(void);
s32 Kbd_IsModArrow(void);

/* 1 when the player asks to switch chat windows: PAD_L1, or the keyboard's arrow key
 * (Kbd_IsModArrow); never while the keyboard is open or the town holds the player. */
s32 Chat_ToggleRequested(u16 buttons)
{
    if (Kbd_IsOpen() != 0) {
        return 0;
    }
    if (Town_IsPlayerHeld() == 1) {
        return 0;
    }
    if (buttons & PAD_L1) {
        return 1;
    }
    return Kbd_IsModArrow();
}

s32 ChatLog_Close(void);

/* Online in the town: opens the current chat window (gChatWindowSel: 0 the gChatLog one, 1 the gChatWinTimer one) for 0x12C frames,
 * or if it is open, closes it and switches to the other. */
void Chat_ToggleWindow(u16 buttons)
{
    ChatWindow *window;
    s32 closed;
    s16 timer;

    if (Game_IsOnline() == 1 && gScene.inLobby != 0 && gScene.area != AREA_HUB_0x83 && Chat_ToggleRequested(buttons) == 1) {
        if (gChatWinHold > 0) {
            switch (gChatWinState) {
            case 2:
            case 3:
                return;
            }
        }
        closed = 0;
        timer = 0x12C;
        if (gChatWindowSel == 0) {
            window = &gChatLog.win;
            if (gChatLog.win.timer != 0 || (window->flags & CHAT_WIN_SCROLLING)) {
                window->timer = 0;
                if (window->flags & CHAT_WIN_SCROLLING) {
                    window->flags = 0;
                    closed = 1;
                    timer = 0;
                }
                gChatWindowSel = 1;
                window = &gChat.win;
                gChatSendTo = CHAT_SEND_TARGETS;
                if (closed) {
                    Chat_OpenWindow();
                    window->flags = CHAT_WIN_SCROLLING;
                    window->open = 0;
                }
            }
        } else {
            window = &gChat.win;
            if (window->timer != 0 || (window->flags & CHAT_WIN_SCROLLING)) {
                window->timer = 0;
                if (window->flags & CHAT_WIN_SCROLLING) {
                    window->flags = 0;
                    closed = 1;
                    timer = 0;
                }
                gChatWindowSel = 0;
                window = &gChatLog.win;
                gChatSendTo = CHAT_SEND_LOBBY;
                if (closed) {
                    ChatLog_Close();
                    window->flags = CHAT_WIN_SCROLLING;
                    window->open = 0;
                }
            }
        }
        window->timer = timer;
        Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
    }
}

extern s8 gNoticeFlags;

/* Online: updates the chat windows by the buttons (PAD_L2 opens the lobby's log window or
 * flips gChatLog.compact while it is open; Chat_ToggleWindow switches windows) and counts
 * the current window's timer down. While chat is hidden, a notice is up or the log is
 * scrolled, gChatWinState is 3. */
void Chat_UpdateWindowsOnline(u16 buttons)
{
    ChatWindow *window;
    u8 pressed;

    if (gChatHidden != 0 || (gNoticeFlags != 0 && gNoticeFlags != NOTICE_CLOSED)) {
        if (gChatWinOpen == 0) {
            gChatWinTimer = 0;
        }
        if (gChatLog.win.open == 0) {
            gChatLog.win.timer = 0;
        }
        gChatWinState = 3;
        return;
    }
    Chat_ToggleWindow(buttons);
    if (gChatSendTo == CHAT_SEND_LOBBY) {
        window = &gChatLog.win;
    } else {
        window = &gChat.win;
    }
    if (window->flags & CHAT_WIN_SCROLLING) {
        gChatWinState = 3;
        return;
    }
    pressed = 0;
    if (Town_IsPlayerHeld() != 1 && (buttons & PAD_L2)) {
        pressed = 1;
    }
    if (gChatWinHold > 0) {
        gChatWinHold--;
        if (gChatWinState == 3) {
            if (gChatWinOpen == 0) {
                gChatWinTimer = 0;
            }
            if (gChatLog.win.open == 0) {
                gChatLog.win.timer = 0;
            }
            return;
        }
        if (gChatSendTo == CHAT_SEND_LOBBY) {
            switch (gChatWinState) {
            case 0:
                if (pressed != 0) {
                    if (gChatLog.win.timer != 0) {
                        gChatLog.compact ^= 1;
                    }
                    gChatLog.win.timer = 0x12C;
                    Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
                }
                break;
            case 1:
                if (gChatLog.win.open == 0) {
                    if (pressed != 0 && gChatLog.win.timer > 0 && gChatLog.win.timer != 0) {
                        gChatLog.win.timer = 0;
                        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                    }
                    break;
                }
                if (pressed != 0) {
                    gChatLog.win.timer = 0x12C;
                    Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
                    break;
                }
            case 2:
                if (gChatLog.win.open == 0 && gChatLog.win.timer != 0) {
                    gChatLog.win.timer = 0;
                    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                }
            default:
                return;
            }
        }
    } else if (gChatSendTo == CHAT_SEND_LOBBY && pressed != 0) {
        if (gChatLog.win.timer != 0) {
            gChatLog.compact ^= 1;
        }
        gChatLog.win.timer = 0x12C;
        Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
    }
    gChatWinState = 0;
    window->open = 0;
    if (window->timer > 0) {
        window->timer--;
    }
}

/* Updates the chat window's state (gChatWinState) and counts its timer down. */
void Chat_UpdateWindow(u16 unused0)
{
    ChatWindow *window;

    if (gChatHidden != 0 || (gNoticeFlags != 0 && gNoticeFlags != NOTICE_CLOSED)) {
        if (gChatWinOpen == 0) {
            gChatWinTimer = 0;
        }
        gChatWinState = 3;
        return;
    }
    window = &gChat.win;
    if (window->flags & CHAT_WIN_SCROLLING) {
        gChatWinState = 3;
        return;
    }
    if (gChatWinHold > 0) {
        gChatWinHold--;
        if (gChatWinState == 3) {
            if (gChatWinOpen == 0) {
                gChatWinTimer = 0;
            }
            return;
        }
    }
    gChatWinState = 0;
    window->open = 0;
    if (window->timer > 0) {
        window->timer--;
    }
}

/* Draws the chat on the HUD: the chat window for gChatWinState 3 and up while it is open,
 * else, while its timer runs, the box and newest lines in the HUD's chat size
 * (gChatHudColor's low bits, when set, override the option). */
void Chat_DrawHud(void)
{
    u8 sizeIndex;
    u8 size;

    if ((gChatWinOpen != 0) && ((s32) gChatWinState >= 3)) {
        Chat_DrawWindow(gChatWinState);
        return;
    }
    if (gChatWinTimer != 0) {
        Gfx_SetTexFilter(0);
        if (gChatHudColor == 0) {
            size = gSysSave.options.chatSize;
        } else {
            size = gChatHudColor & 0x7F;
        }
        sizeIndex = size & 0xFF;
        Chat_DrawBox((s32) gChatSendTo, sizeIndex, 0);
        Chat_DrawLogLines(0U, 0, 0U, sizeIndex);
    }
}

s8 Kbd_IsOpen(void);
s32 Town_IsPlayerHeld(void);

/* 0 while the keyboard is open, or in the town while it holds the player; else 1. */
s32 Chat_CanDraw(void)
{
    if (Kbd_IsOpen() != 0) {
        return 0;
    }
    if (gScene.inLobby != 0 && Town_IsPlayerHeld() == 1) {
        return 0;
    }
    return 1;
}

extern Panel gChatLogWindow[];
extern Panel gChatRecvWindow;
extern char *gChatRecvText[];
extern char *gChatOpenText;
void Gfx_DrawMarker(s16 x, s16 y, s16 w, s16 h, s32 flags);

/* Draws a chat window (kind 1: the log, 2 and 3: the input line, 3 as wide as gChatTextSizes[gSysSave.options.chatSize]) and its marker, with the blinking cue (colour 0 for the lobby's chat, 1 for the others). */
void Chat_DrawWindow(u8 kind)
{
    Panel *win;
    u8 size;
    s32 markerFlags;
    char *text;

    if (kind != 0) {
        size = (kind == 3) ? gChatTextSizes[(s8)gSysSave.options.chatSize] : 0x15;
        if (kind == 1) {
            win = &gChatRecvWindow;
            if (Chat_CanDraw() == 1) {
                text = gChatOpenText;
            } else {
                text = gChatRecvText[0];
            }
        } else {
            win = gChatLogWindow;
            win->y = (kind == 2) ? 0x190 : 0x192 - (size * 18 / 21 + 1) * 13;
            win->charW = size;
            win->charH = size * 18 / 21;
            text = gChatRecvText[0];
        }
        if (gChatSendTo == CHAT_SEND_LOBBY) {
            markerFlags = MARKER_CUE | 0;
            win->color = 0xBD6CC56C;
        } else {
            win->color = 0xBDB59F6C;
            markerFlags = MARKER_CUE | 1;
        }
        Window_DrawText(win, 0, (u8 *)((s32)text));
        Gfx_DrawMarker(win->x, win->y, size, size * 18 / 21, markerFlags);
    }
}

/* Hides the chat windows (gChatHidden) and stops the current one's timer. */
void Chat_HideWindows(void)
{
    gChatHidden = 1;
    if (gChatSendTo == CHAT_SEND_LOBBY) {
        gChatLog.win.timer = 0;
        return;
    }
    gChatWinTimer = 0;
}

/* Lets the chat windows show again. */
void Chat_ShowWindows(void)
{
    gChatHidden = 0;
}

s32 NetUser_State(s8 i);
extern u8 gOthersPresentMask;
extern u8 gOthersPresentCount;

/* The other players present: a bit mask in gOthersPresentMask and their number in gOthersPresentCount. */
void Players_UpdatePresent(void)
{
    u32 i;

    gOthersPresentCount = 0;
    gOthersPresentMask = 0;
    if (gScene.inLobby == 0) {
        for (i = 0; i < 4; i++) {
            if (gScene.localPlayer != i && gScene.inQuest[i] == 1) {
                gOthersPresentMask |= (u8)(1 << i);
                gOthersPresentCount++;
            }
        }
    } else {
        for (i = 0; i < 10; i++) {
            if (gScene.localPlayer != i && NetUser_State(i) == 0) {
                gOthersPresentMask |= (u8)(1 << i);
                gOthersPresentCount++;
            }
        }
    }
}

extern s16 gNoticeShownChars;
extern char **gNoticeList;
extern s32 gNoticeTitle;
extern u8 gNoticeSel;
extern u8 gChatSizeLineGaps[];
extern char *gNoticeYesSelText;
extern char *gNoticeNoSelText[];
extern char *gNoticeNextText;
s32 Text_NextChar(char *out, char *s, char **next);
void Text_PrintCodes(s16 max, const char *fmt, ...);

/* Draws the notice window (nothing with NOTICE_CLOSED): the part of gChat.notice shown so
 * far, wrapped at 0x16 half-width characters, then by gNoticeFlags the yes/no prompt, the
 * more-text mark, the list gNoticeList with its selected entry gNoticeSel in brackets, or
 * that entry alone. */
void Notice_Draw(void)
{
    s32 i;
    char ch[4];
    char *src;
    s16 kind;
    s16 lineH;
    s32 charH;
    u8 size;
    s32 charsLeft;
    s32 widthLeft;
    s32 chars;
    s16 y;
    s16 step;
    char *line;

    if (gNoticeFlags & NOTICE_CLOSED) {
        return;
    }
    size = gChatTextSizes[gSysSave.options.chatSize];
    Chat_DrawBox(0, gSysSave.options.chatSize, gNoticeTitle);
    charH = size * 18 / 21;
    Font_SetSpacing(size, charH);
    Text_SetColor(0);
    lineH = charH + 1;
    y = 0x1A4 - lineH * 11U;    /* 11 lines up from the bottom; the count is unsigned for the match */
    i = gSysSave.options.chatSize;
    step = lineH + gChatSizeLineGaps[i];
    charsLeft = gNoticeShownChars;
    src = gChat.notice;
    if (charsLeft > 0) {
        do {
            line = src;
            widthLeft = 0x16;
            chars = 0;
            while (charsLeft > 0 && widthLeft > 0) {
                kind = Text_NextChar(ch, src, &src);
                if (ch[0] == '\n') {
                    break;
                }
                charsLeft--;
                chars++;
                if (kind == 1 || kind == 2) {
                    widthLeft--;
                }
                widthLeft--;
            }
            Font_SetPos(0x18, y);
            Text_PrintCodes(chars, line);
            y += step;
        } while (charsLeft > 0);
    }
    y += step;
    if (gNoticeFlags & NOTICE_PROMPT) {
        Font_SetPos(0x18, y);
        if (gNoticeFlags & NOTICE_PROMPT_YES) {
            Text_Printf(gNoticeYesSelText);
        } else {
            Text_Printf(gNoticeNoSelText[0]);
        }
    }
    if (gNoticeFlags & NOTICE_NEXT) {
        Text_SetColor(2);
        Font_SetPos(size * 132 / 21 + 0x18, y);
        Text_Draw(gNoticeNextText);
    }
    if (gNoticeFlags & NOTICE_LIST) {
        Font_SetPos(0x18, y);
        for (i = 0; i < 11; i++) {
            if (gNoticeList[i] == NULL) {
                break;
            }
            Font_SetPos(0x18, y);
            if (i == gNoticeSel) {
                Text_Printf(TXT(0x004908B8) /* a colour code, then the name in full-width corner brackets */, gNoticeList[i]);
            } else {
                Text_Printf("~C00  %s", gNoticeList[i]);
            }
            y += step;
        }
    }
    if (gNoticeFlags & NOTICE_LIST_PICKED) {
        Font_SetPos(0x18, y);
        Text_Printf(gNoticeList[gNoticeSel]);
    }
}

struct Player;

extern char *gStatusPageFmt[];
extern ListPanel gPersonalStatusPanels[];
extern Panel gPersonalStatusBoxes[];
extern char *gStatusBoxTitles[];
u8 SkillList_PageCount(void);
struct ListPanel;
void ListPanel_Draw(ListPanel *b, s16 *pos, char *title, s16 sel);
void Menu_DrawFrame(void *b, s16 *pos);
s16 StatusPage_DrawHeader(struct Player *p, s16 x, s16 y);
s16 StatusPage_DrawSymbolColor(s16 x, s16 y);
s16 StatusPage_DrawRank(s16 x, s16 y);
s16 StatusPage_DrawArenaRecord(s16 x, s16 y);
s16 StatusPage_DrawOnline(s16 x, s16 y);
void Text_DrawWrapped33(s16 x, s16 y, s16 lineh, char *s);
void StatusPage_DrawStats(Player *p, Player *q, s32 x, s32 y);
u8 Player_ArmorHasTorsoUp(Player *p);
void SkillList_DrawPage(s16 x, s16 y, u8 page, u8 mark);
void PersonalStatus_DrawEquipment(Player *pl);
void Player_DrawSkillsPlain(Player *a, s32 b, s16 c); /* unit's own declaration: declared here as EquipWindow_DrawDetails calls it; with the definition's void (Player *a, s32 b, s16 c), the call compiles differently */

/* Draws page `page` of the status menu for pl, its title showing the page and the page
 * count: 0 the profile (name, ranks, the hunter's comment), 1 the equipment and the stats,
 * 2-7 the skill list's pages with the skills in effect. */
void PersonalStatus_DrawPage(struct Player *pl, u8 page)
{
    char buf[64];
    s16 y;
    u8 pages;

    pages = SkillList_PageCount() + 2;
    sprintf(buf, gStatusPageFmt[0], page + 1, pages);
    ListPanel_Draw(&gPersonalStatusPanels[page], NULL, buf, -1);
    Menu_DrawFrame(&gPersonalStatusPanels[page], 0);
    Window_DrawText(&gPersonalStatusBoxes[page], 0, (u8 *)gStatusBoxTitles[page]);
    Font_SetSpacing(0x12, 0x12);
    switch (page) {
    case 0:
        y = StatusPage_DrawHeader(pl, 0x132, 0x52);
        y = StatusPage_DrawSymbolColor(0x132, y);
        y = StatusPage_DrawRank(0x132, y);
        y = StatusPage_DrawArenaRecord(0x132, y);
        StatusPage_DrawOnline(0x132, y);
        Text_DrawWrapped33(0x136, 0x165, 0x14, (char *)gSave.comment);
        break;
    case 1:
        PersonalStatus_DrawEquipment(pl);
        StatusPage_DrawStats(pl, 0, 0x132, 0x126);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        SkillList_DrawPage(0x132, 0x3E, page - 2, Player_ArmorHasTorsoUp(pl));
        Player_DrawSkillsPlain(pl, 0x132, 0x126);
        break;
    }
}

s32 Quest_IsDrill(void);
extern char *gStatusHeadLabels[];
extern char *gPointsFmt[];
extern char *gMoneyFmt;

/* The first lines of the status page: a heading picked by Game_IsOnline and Quest_IsDrill
 * (gStatusHeadLabels), the hunter's name, online an id line (the user id, or outside the town
 * the player's preset id), the drill's points or the money, the hunter's title and the
 * weapon class; returns the next line's y. The last lines' x + 0x48U is unsigned so that
 * MWCC doesn't reuse valueX there (the match needs it). */
s16 StatusPage_DrawHeader(struct Player *pl, s16 x, s16 y)
{
    s32 label;
    s32 valueX;
    s32 left;

    Font_SetColor(STATUS_COLOR_LABEL);
    Font_SetPos(x, y);
    label = 0;
    if (Game_IsOnline() == 1) {
        label++;
    }
    if (Quest_IsDrill() == 1) {
        label += 2;
    }
    Text_PrintfStyled(2, gStatusHeadLabels[label]);
    Font_SetColor(STATUS_COLOR_VALUE);
    left = x;
    valueX = left + 0x48;
    Font_SetPos(valueX, y);
    Text_Draw((char *)pl->name);
    y += 0x14;
    if (Game_IsOnline() == 1) {
        Font_SetPos(valueX, y);
        if (Quest_IsDrill() == 1 && gScene.allLocal != 0) {
            Text_Draw(gMyUserId);
        } else if (gScene.inLobby == 0) {
            Text_Draw((char *)gScene.userIds[pl->index]);
        } else {
            Text_Draw(gMyUserId);
        }
        y += 0x14;
    }
    if (Quest_IsDrill() == 1) {
        Font_SetPos(left + 0x7E, y);
        Text_PrintfColored(gPointsFmt[0], gSave.points);
    } else {
        Font_SetPos(valueX, y);
        Text_PrintfColored(gMoneyFmt, gSave.money);
    }
    Font_SetPos(left + 0x48U, y + 0x14);
    Text_Draw(gHunterTitles[gSave.title].text);
    Font_SetPos(left + 0x48U, y + 0x28);
    StatusPage_DrawWeaponClass(pl);
    return y + 0x3C;
}

s32 StatusPage_HasSymbolColor(void);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
extern char *gSymbolColorLabel[];
extern char *gSymbolColorNames[];

/* A line of the status page, when StatusPage_HasSymbolColor: the label and the name of
 * gSave.symbolColor; returns the next line's y. */
s16 StatusPage_DrawSymbolColor(s16 x, s16 y)
{
    if (StatusPage_HasSymbolColor() == 1) {
        Font_SetColor(STATUS_COLOR_LABEL);
        Font_SetPos(x, y);
        Text_Draw(gSymbolColorLabel[0]);
        Font_SetColor(STATUS_COLOR_VALUE);
        Font_SetPos(x + 0x90, y);
        Text_Draw(gSymbolColorNames[gSave.symbolColor]);
        y += 0x14;
    }
    return y;
}

void Save_GetRankInfo(SaveData *s, u16 *hr, u32 *pts, u32 *next);
void Text_PrintfColored(const char *fmt, ...);
extern char *gHunterRankLabels;
extern char *gRankPointsFmt[];
extern char gStatusFmtInt[];

/* The hunter rank lines of the status page, once the hunter has a rank: the rank, then the
 * rank points and the points to the next rank (the points alone at rank 999); returns the
 * next line's y. */
s16 StatusPage_DrawRank(s16 x, s16 y)
{
    u16 hr;
    u32 points;
    u32 nextPoints;

    if (gSave.hr > 0) {
        Font_SetColor(STATUS_COLOR_LABEL);
        Font_SetPos(x, y);
        Text_PrintfStyled(2, gHunterRankLabels);
        Save_GetRankInfo(&gSave, &hr, &points, &nextPoints);
        Font_SetColor(STATUS_COLOR_VALUE);
        Font_SetPos(x + 0x48, y);
        Text_PrintfColored(gStatusFmtInt, hr);
        Font_SetPos(x + 0x48, y + 0x14);
        if (hr < 999) {
            Text_PrintfColored(gRankPointsFmt[0], points, nextPoints);
        } else {
            Text_PrintfColored(gStatusFmtInt, points);
        }
        y += 0x28;
    }
    return y;
}

s32 Save_TestEventFlag(s32 n);
void Text_PrintfColored(const char *fmt, ...);
extern char *gArenaRecordLabels[];
extern char *gArenaRecordFmt[];
extern char *gArenaTimeFmt;

/* The arena record lines of the status page, once event flag 0xC4 is set: the fights, wins
 * and losses, and the total time in minutes and seconds; returns the next line's y. */
s16 StatusPage_DrawArenaRecord(s16 x, s16 y)
{
    if (Save_TestEventFlag(0xC4) == 1) {
        Font_SetColor(STATUS_COLOR_LABEL);
        Font_SetPos(x, y);
        Text_PrintfStyled(2, gArenaRecordLabels[0]);
        Font_SetColor(STATUS_COLOR_VALUE);
        Font_SetPos(x + 0x5A, y);
        Text_PrintfColored(gArenaRecordFmt[0], gSave.versusWins + gSave.versusLosses, gSave.versusWins, gSave.versusLosses);
        Font_SetPos(x + 0xA2, y + 0x14);
        Text_PrintfColored(gArenaTimeFmt, gSave.versusMinutes / 60, gSave.versusMinutes % 60);
        y += 0x28;
    }
    return y;
}

void Profile_DrawStatus(s16 x, s16 y); /* unit's own declaration: the definition (lobby) is void Profile_DrawStatus(s32 x, s32 y); the calls here match only with this one */
void Profile_DrawRestrict(s16 x, s16 y);
extern char *gOnlineCardLabels;

/* The online lines of the status page (online, in the town): the profile's status
 * and friend limit (the lobby's Profile_Draw*); returns the next line's y. */
s16 StatusPage_DrawOnline(s16 x, s16 y)
{
    if (Game_IsOnline() == 1 && gScene.inLobby != 0) {
        Font_SetColor(STATUS_COLOR_LABEL);
        Font_SetPos(x, y);
        Text_PrintfStyled(2, gOnlineCardLabels);
        Font_SetColor(STATUS_COLOR_VALUE);
        Profile_DrawStatus(x + 0x48, y);
        Profile_DrawRestrict(x + 0xA2, y + 0x14);
        y += 0x28;
    }
    return y;
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/menu_windows", gStatusFmtInt4);
INCLUDE_RODATA("asm/sub_main/nonmatchings/menu_windows", gStatusFmtInt3);

extern char gStatusFmtInt4[];
extern char gStatusFmtInt3[];
extern char *gStatusStatLabels[];
u32 Player_ShownAttack(Player *p);


/* The stats block of the status page: the labels, then pl's health, stamina / 3, attack,
 * defense and five resistances, each coloured against other's (STATUS_COLOR_BETTER, _WORSE,
 * or _VALUE; all _VALUE without other). For the match the positions are ints holding u16
 * values, and the three row offsets that both columns use are locals. */
void StatusPage_DrawStats(Player *pl, Player *other, s32 x, s32 y)
{
    u32 hpColor;
    u32 staminaColor;
    u32 attackColor;
    s32 rightX;
    u32 attack;
    u32 resist4Color;
    s32 top;
    s32 left;
    s16 leftX;
    s32 row1Y;
    s32 row2Y;
    s32 row3Y;
    u32 otherAttack;
    u32 resist3Color;
    u32 resist2Color;
    u32 resist1Color;
    u32 resist0Color;
    u32 defenseColor;

    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(STATUS_COLOR_LABEL);
    Text_DrawLinesColored(x, y, gStatusStatLabels, 0x14);
    attack = Player_ShownAttack(pl);
    hpColor = staminaColor = attackColor = defenseColor = resist0Color = resist1Color = resist2Color = resist3Color = resist4Color = STATUS_COLOR_VALUE;
    if (other != NULL) {
        if (other->hpMax < pl->hpMax) {
            hpColor = STATUS_COLOR_BETTER;
        } else if (other->hpMax > pl->hpMax) {
            hpColor = STATUS_COLOR_WORSE;
        }
        if (other->staminaMax < pl->staminaMax) {
            staminaColor = STATUS_COLOR_BETTER;
        } else if (other->staminaMax > pl->staminaMax) {
            staminaColor = STATUS_COLOR_WORSE;
        }
        otherAttack = Player_ShownAttack(other);
        if (attack > otherAttack) {
            attackColor = STATUS_COLOR_BETTER;
        } else if (attack < otherAttack) {
            attackColor = STATUS_COLOR_WORSE;
        }
        if (other->defense < pl->defense) {
            defenseColor = STATUS_COLOR_BETTER;
        } else if (other->defense > pl->defense) {
            defenseColor = STATUS_COLOR_WORSE;
        }
        if (other->resist[0] < pl->resist[0]) {
            resist0Color = STATUS_COLOR_BETTER;
        } else if (other->resist[0] > pl->resist[0]) {
            resist0Color = STATUS_COLOR_WORSE;
        }
        if (other->resist[1] < pl->resist[1]) {
            resist1Color = STATUS_COLOR_BETTER;
        } else if (other->resist[1] > pl->resist[1]) {
            resist1Color = STATUS_COLOR_WORSE;
        }
        if (other->resist[2] < pl->resist[2]) {
            resist2Color = STATUS_COLOR_BETTER;
        } else if (other->resist[2] > pl->resist[2]) {
            resist2Color = STATUS_COLOR_WORSE;
        }
        if (other->resist[3] < pl->resist[3]) {
            resist3Color = STATUS_COLOR_BETTER;
        } else if (other->resist[3] > pl->resist[3]) {
            resist3Color = STATUS_COLOR_WORSE;
        }
        if (other->resist[4] < pl->resist[4]) {
            resist4Color = STATUS_COLOR_BETTER;
        } else if (other->resist[4] > pl->resist[4]) {
            resist4Color = STATUS_COLOR_WORSE;
        }
    }
    Font_SetColor(hpColor);
    top = (u16)y;
    left = (u16)x;
    leftX = (u16)(left + 0x5A);
    Font_SetPos(leftX, top);
    Text_PrintfColored(gStatusFmtInt4, pl->hpMax);
    Font_SetColor(staminaColor);
    row1Y = top + 0x14;
    Font_SetPos(leftX, row1Y);
    Text_PrintfColored(gStatusFmtInt4, pl->staminaMax / 3);
    Font_SetColor(attackColor);
    row2Y = top + 0x28;
    Font_SetPos(leftX, row2Y);
    Text_PrintfColored(gStatusFmtInt4, attack);
    Font_SetColor(defenseColor);
    row3Y = top + 0x3C;
    Font_SetPos(leftX, row3Y);
    Text_PrintfColored(gStatusFmtInt4, pl->defense);
    Font_SetColor(resist0Color);
    rightX = (u16)(left + 0x5A) + 0x90;
    Font_SetPos(rightX, top);
    Text_PrintfColored(gStatusFmtInt3, (s16)pl->resist[0]);
    Font_SetColor(resist1Color);
    Font_SetPos(rightX, row1Y);
    Text_PrintfColored(gStatusFmtInt3, (s16)pl->resist[1]);
    Font_SetColor(resist2Color);
    Font_SetPos(rightX, row2Y);
    Text_PrintfColored(gStatusFmtInt3, (s16)pl->resist[2]);
    Font_SetColor(resist3Color);
    Font_SetPos(rightX, row3Y);
    Text_PrintfColored(gStatusFmtInt3, (s16)pl->resist[3]);
    Font_SetColor(resist4Color);
    Font_SetPos(rightX, top + 0x50);
    Text_PrintfColored(gStatusFmtInt3, (s16)pl->resist[4]);
}

extern s32 gWeaponClassMultipliers[];
u8 Weapon_GetClass(u8 a, u16 weapon);

/* The attack power shown for a player: attack times the weapon class's multiplier / 100. */
u32 Player_ShownAttack(Player *pl)
{
    return (u16)(pl->attack * gWeaponClassMultipliers[Weapon_GetClass(pl->equip[6].type, pl->equip[6].id)] / 100);
}

void StatusPage_DrawEquipment(EquipItem *weapon, EquipItem *eq);

/* The equipment page's icons and names for a player. */
void PersonalStatus_DrawEquipment(Player *pl)
{
    StatusPage_DrawEquipment(&pl->equip[EQUIP_SLOT_WEAPON], pl->equip);
}


struct Pt;
void Equip_GetIconUV(EquipItem *e, struct Pt *uv0, struct Pt *uv1);
u8 Equip_GetRarity(u8 type, u16 id);
char *Equip_GetName(u8 type, u16 id);
u32 Equip_Color(u8 idx, u8 alpha, s32 alt);
extern u8 gStatusEquipIconU[];
extern char *gEquipWeaponFmt[];
extern char *gEquipArmorFmt;
extern char **gArmorNameTables[];

/* The equipment page of the status screen: the weapon's and the armour pieces' icons in their colours,
 * the weapon's value (Equip_GetName) and a text per armour piece. */
void StatusPage_DrawEquipment(EquipItem *weapon, EquipItem *equip)
{
    Sprite spr;
    ArmorData *armor[5];
    u16 ids[5];
    u32 i;
    s16 y;
    s32 textY;

    armor[0] = &gArmorHeadData[ids[0] = equip[2].id];
    armor[1] = &gArmorBodyData[ids[1] = equip[3].id];
    armor[2] = &gArmorArmsData[ids[2] = equip[4].id];
    armor[3] = &gArmorWaistData[ids[3] = equip[5].id];
    armor[4] = &gArmorLegsData[ids[4] = equip[0].id];
    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    spr.x = 0x137;
    spr.w = 0x28;
    spr.h = 0x20;
    y = 0x55;
    spr.y = y;
    Equip_GetIconUV(weapon, (struct Pt *)&spr.u0, (struct Pt *)&spr.u1);
    spr.color = Equip_Color(Equip_GetRarity(weapon->type, weapon->id), 0xFF, 0);
    Sprite_Draw(&spr);
    spr.v0 = 0x121;
    spr.v1 = 0x13F;
    for (i = 0; i < 5; i++) {
        y += 0x20;
        spr.y = y;
        spr.u0 = gStatusEquipIconU[i];
        spr.u1 = spr.u0 + 0x20;
        spr.color = Equip_Color(armor[i]->rarity, 0xFF, 0);
        Sprite_Draw(&spr);
    }
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(0x168, 0x5C);
    Text_Printf(gEquipWeaponFmt[0], Equip_GetName(weapon->type, weapon->id));
    for (i = 0, textY = 0x7C; i < 5; textY += 0x20, i++) {
        Font_SetPos(0x168, textY);
        Text_Printf(gEquipArmorFmt, gArmorNameTables[i][ids[i]]);
    }
}

u8 Weapon_GetClass(u8 a, u16 weapon);
extern char *gWeaponTypeNames[];

/* Draws the name of a player's weapon class. */
void StatusPage_DrawWeaponClass(Player *pl)
{
    Text_Draw(gWeaponTypeNames[Weapon_GetClass(pl->equip[EQUIP_SLOT_WEAPON].type, pl->equip[EQUIP_SLOT_WEAPON].id)]);
}

void Player_DrawSkills(Player *p, s32 x, s16 y, s16 *colors);

/* Player_DrawSkills with all skills in colour 0. */
void Player_DrawSkillsPlain(Player *pl, s32 x, s16 y)
{
    s16 colors[5];

    colors[4] = 0;
    colors[3] = 0;
    colors[2] = 0;
    colors[1] = 0;
    colors[0] = 0;
    Player_DrawSkills(pl, x, y, colors);
}

extern char *gSkillEffectNames[];
s32 Player_SkillApplies(Player *p, u8 slot);

/* Draws a player's five skill effects, one per line from (x, y + 0x14), in colors[slot] (text
 * colour 9 for a skill that doesn't apply to the equipment, Player_SkillApplies), with an
 * arrow before colours 0xE and 2 (the equipment preview's gained and lost skills); the
 * no-skill text when there are none. */
void Player_DrawSkills(Player *pl, s32 x, s16 y, s16 *colors)
{
    u32 i;
    s16 lineY;
    s32 textX;
    s32 markX;

    Font_SetSpacing(0x12, 0x12);
    if ((pl->skills[0] | pl->skills[1] | pl->skills[2] | pl->skills[3] | pl->skills[4]) == 0) {
        Text_SetColor(0);
        Font_SetPos(x, y + 0x14);
        Text_Draw(gSkillEffectNames[0]);
        return;
    }
    lineY = y + 0x14;
    textX = (s16)x;
    for (i = 0, markX = textX - 8; i < 5; i++, colors++) {
        if (pl->skills[i] != 0) {
            if (Player_SkillApplies(pl, i) == 1) {
                Text_SetColor(*colors);
            } else {
                Text_SetColor(9);
            }
            Font_SetPos(textX, lineY);
            Text_Draw(gSkillEffectNames[pl->skills[i]]);
            if (*colors == 0xE || *colors == 2) {
                Menu_DrawArrow(markX, lineY, 8, 0x12, 0xC0D0D0D0, 0);
            }
            lineY += 0x14;
        }
    }
}

/* A blade's status value (poison, paralysis, sleep or an element), -1 when it has none. */
s32 BladeData_StatusValue(BladeData *blade)
{
    return blade->statusType != 0 ? blade->statusValue : -1;
}

s32 Player_GetSkillAmmoMask(Player *p);
extern u8 gGunnerNoSkillGroups[];
extern u8 gBowNoSkillGroups[];
extern u8 gBladeNoSkillGroups[];

/* 1 if the skill tree of a player's skill slot applies to the player's weapon: gunners
 * (Player.weaponFlags 2) don't get the trees in gGunnerNoSkillGroups (gBowNoSkillGroups for
 * a bow, weapon class 10), nor the ammo trees (0x26, 0xB, 0x17, 0x2F) for ammo their gun
 * can't load, and tree 0x34 not with skill 0x33 nor for a bow; blademasters (weaponFlags 1)
 * don't get gBladeNoSkillGroups', nor 0x2F without a status on the blade. Trees 4 and 5
 * need weaponFlags 4 (tree 4 also a class-1 weapon with level bit 0x80). */
s32 Player_SkillApplies(Player *pl, u8 slot)
{
    u8 group;
    u8 *list;
    EquipData *data;
    s32 mask;

    group = pl->skillGroups[slot];
    if (pl->weaponFlags & 2) {
        if (group == 0x34) {
            if (Player_HasSkill(pl, 0x33) == 1) {
                return 0;
            }
            if (Weapon_GetClass(pl->equip[6].type, pl->equip[6].id) != 10) {
                return 1;
            }
            return 0;
        }
        list = gGunnerNoSkillGroups;
        do {
            if (*list == group) {
                return 0;
            }
            list++;
        } while (*list != 0);
        if (Weapon_GetClass(pl->equip[6].type, pl->equip[6].id) != 10) {
            data = Equip_GetData(&pl->equip[6]);
            mask = data->gun.ammoMask | Player_GetSkillAmmoMask(pl);
            if (group == 0x26) {
                if (!(mask & 7)) {
                    return 0;
                }
            } else if (group == 0xB) {
                if (!(mask & 0x38)) {
                    return 0;
                }
            } else if (group == 0x17) {
                if (!(mask & 0x1C0)) {
                    return 0;
                }
            } else if (group == 0x2F) {
                if (!(mask & 0xFC00000)) {
                    return 0;
                }
            }
        } else {
            list = gBowNoSkillGroups;
            do {
                if (*list == group) {
                    return 0;
                }
                list++;
            } while (*list != 0);
        }
    }
    if (pl->weaponFlags & 1) {
        if (group == 0x2F) {
            return (s16)BladeData_StatusValue(&Equip_GetData(&pl->equip[6])->blade) > 0;
        }
        list = gBladeNoSkillGroups;
        do {
            if (*list == group) {
                return 0;
            }
            list++;
        } while (*list != 0);
    }
    if (!(pl->weaponFlags & 4)) {
        switch (group) {
        case 5:
            return 0;
        case 4:
            if (Weapon_GetClass(pl->equip[6].type, pl->equip[6].id) == 1 && (pl->equip[6].level & 0x80)) {
                break;
            }
            return 0;
        }
    }
    return 1;
}

extern ArmorData *gArmorTables[];
extern u16 gArmorEquipSlots[];

/* 1 if a piece of the player's armour has skill tree 0x29, 0x2A or 0x2B (the torso up
 * trees, which double the torso's points; gSkillNames). */
u8 Player_ArmorHasTorsoUp(Player *pl)
{
    s32 i;
    s32 j;
    u8 *tree;
    ArmorData *armor;

    for (i = 0; i < 5; i++) {
        u16 k = gArmorEquipSlots[i];

        armor = gArmorTables[k];
        armor += pl->equip[k].id;
        tree = &armor->skills[0].skill;
        for (j = 5; j > 0; j--) {
            if ((u32)(*tree - 0x29) <= 1 || *tree == 0x2B) {
                return 1;
            }
            tree += 2;
        }
    }
    return 0;
}

extern char *gSkillListHeader[];
extern char *gSkillNameFmt[];
extern char *gSkillColFmt;
extern char *gSkillPartPtsFmt[];
extern char *gSkillTotalFmt;
extern char *gSkillRowFmt[];
extern char *gSkillPartMarks[];
extern char *gSkillNames[];

/* Draws page `page` of the skill list (nine gSkillRecords) from (x, y): per skill its name,
 * its points per piece with the comparison marks, and its total (a heading line for a
 * record with used & 2); with mark, a box behind the torso column (torso up). */
void SkillList_DrawPage(s16 x, s16 y, u8 page, u8 mark)
{
    SkillRecord *rec;
    char buf[128];
    PrimLine box;
    u32 i;
    s16 textColor;
    u32 j;
    s16 lineY;
    s16 shown;
    s32 top;

    Font_SetSpacing(0x10, 0x12);
    top = y;
    Font_SetPos(x, top + 0x14);
    Font_SetColor(STATUS_COLOR_LABEL);
    Text_PrintfColored(gSkillListHeader[0]);
    lineY = top + 0x28;
    shown = 0;
    for (rec = &gSkillRecords[page * 9], i = 9; i != 0; lineY += 0x14, i--, rec++) {
        if (rec->used == 0) {
            break;
        }
        if (!(rec->used & 2)) {
            shown++;
            textColor = (rec->used & 1) ? 3 : 0;
            sprintf(buf, gSkillNameFmt[0], gSkillNames[rec->skill]);
            for (j = 0; j < 6; j++) {
                if (rec->pts[j] == 0) {
                    sprintf(buf, gSkillColFmt, buf, textColor);
                } else {
                    sprintf(buf, gSkillPartPtsFmt[0], buf, textColor, rec->mark[j], rec->pts[j]);
                }
            }
            Text_SetColor(textColor);
            Font_SetPos(x, lineY);
            Text_Printf(gSkillTotalFmt, buf, textColor, rec->totalMark, rec->total);
        } else {
            Text_SetColor(-1);
            Font_SetPos(x, lineY);
            Text_Printf(gSkillRowFmt[0], gSkillNames[rec->skill], gSkillPartMarks[rec->pts[0]], gSkillPartMarks[rec->pts[1]],
                        gSkillPartMarks[rec->pts[2]], gSkillPartMarks[rec->pts[3]], gSkillPartMarks[rec->pts[4]], gSkillPartMarks[rec->pts[5]]);
        }
    }
    if (shown > 0 && mark != 0) {
        box.color = 0xB2003E74;
        box.x0 = x + 0x98;
        box.x1 = x + 0xB1;
        box.y0 = top + 0x29;
        box.y1 = top + 0x27 + shown * 0x14;
        Prim_FillRect(&box);
    }
}

/* The skill list's page count: nine records a page (at most 0x2E records counted), at least 1. */
u8 SkillList_PageCount(void)
{
    u32 count;
    SkillRecord *rec;

    for (count = 0, rec = gSkillRecords; count < 0x2E; count++, rec++) {
        if (rec->used == 0) {
            break;
        }
    }
    if (count == 0) {
        return 1;
    }
    return (count - 1) / 9 + 1;
}

extern SkillRecord gSkillCompareRecords[];
s32 SkillList_MakeRecord(Player *p, SkillRecord *r, Player *q);
void SkillList_Insert(Player *a, u8 b, Player *q);

/* Builds the skill lists over the skill trees 1-0x51: other's records in
 * gSkillCompareRecords (when other is set; not the torso up trees), then pl's, compared
 * with other's, sorted into gSkillRecords. */
void SkillList_Build(Player *pl, Player *other)
{
    SkillRecord *rec;
    s32 left;
    s8 tree;

    if (other != NULL) {
        rec = gSkillCompareRecords;
        left = 0x36;
        for (tree = 1; tree < 0x52; tree++) {
            if (tree == 0x29 || tree == 0x2A || tree == 0x2B) {
                continue;
            }
            rec->skill = tree;
            if (SkillList_MakeRecord(other, rec, NULL) == 1) {
                left--;
                rec++;
                if (left <= 0) {
                    break;
                }
            }
        }
        while (left > 0) {
            rec->used = 0;
            left--;
            rec++;
        }
    }
    for (rec = gSkillRecords, left = 0x36; left > 0; left--, rec++) {
        rec->used = 0;
    }
    for (tree = 1; tree < 0x52; tree++) {
        SkillList_Insert(pl, tree, other);
    }
}

s32 SkillList_MakeRecord(Player *p, SkillRecord *r, Player *q);

/* Makes pl's record of skill tree `tree` (SkillList_MakeRecord, compared with other) and
 * inserts it into gSkillRecords by its key, when it has points and there is room. */
void SkillList_Insert(Player *pl, u8 tree, Player *other)
{
    SkillRecord rec;
    SkillRecord *slot;
    SkillRecord *free;
    SkillRecord *src;
    s32 i;
    s32 j;

    rec.skill = tree;
    if (SkillList_MakeRecord(pl, &rec, other) != 1) {
        return;
    }
    for (i = 0x36, free = gSkillRecords; i > 0; i--, free++) {
        if (free->used == 0) {
            free++;
            for (slot = gSkillRecords, j = 0x36; j > 0; j--, slot++) {
                if (slot->used != 0) {
                    if (slot->key > rec.key) {
                        continue;
                    }
                    src = free - 1;
                    do {
                        free--;
                        src--;
                        *free = *src;
                    } while (src > slot);
                }
                *slot = rec;
                return;
            }
            return;
        }
    }
}

s32 SkillList_FillPieces(Player *p, SkillRecord *r, Player *q);
s8 Equip_GetSkillPoints(EquipItem *e, u8 kind);
s8 Player_GetTorsoSkillPoints(Player *p, u8 kind);
u8 Player_GetSkill(Player *p, u8 kind);

/* Makes pl's record of skill tree rec->skill: its points per piece (weapon, head, torso,
 * arms, waist, legs) and in total, and its sort key (active skills first, then by total);
 * with other, marks each against other's record (gSkillCompareRecords). 0 when no piece
 * gives points. The torso up trees (0x29-0x2B) are SkillList_FillPieces's. */
s32 SkillList_MakeRecord(Player *pl, SkillRecord *rec, Player *other)
{
    SkillRecord *cmp;
    u32 n;
    u32 i;

    if ((u32)(rec->skill - 0x29) <= 1 || rec->skill == 0x2B) {
        return SkillList_FillPieces(pl, rec, other);
    }
    rec->pts[0] = Equip_GetSkillPoints(&pl->equip[6], rec->skill);
    rec->pts[1] = Equip_GetSkillPoints(&pl->equip[2], rec->skill);
    rec->pts[2] = Player_GetTorsoSkillPoints(pl, rec->skill);
    rec->pts[3] = Equip_GetSkillPoints(&pl->equip[4], rec->skill);
    rec->pts[4] = Equip_GetSkillPoints(&pl->equip[5], rec->skill);
    rec->pts[5] = Equip_GetSkillPoints(&pl->equip[0], rec->skill);
    if ((rec->pts[0] | rec->pts[1] | rec->pts[2] | rec->pts[3] | rec->pts[4] | rec->pts[5]) == 0) {
        return 0;
    }
    rec->used = 0x80;
    rec->total = rec->pts[0] + rec->pts[1] + rec->pts[2] + rec->pts[3] + rec->pts[4] + rec->pts[5];
    rec->key = 0;
    if (Player_GetSkill(pl, rec->skill) != 0) {
        rec->used |= 1;
        rec->key += 0x8000;
        rec->mark[0] = rec->mark[1] = rec->mark[2] = rec->mark[3] = rec->mark[4] = rec->mark[5] = rec->totalMark = 3;
    } else {
        rec->mark[0] = rec->mark[1] = rec->mark[2] = rec->mark[3] = rec->mark[4] = rec->mark[5] = 0;
        rec->totalMark = 6;
    }
    if (rec->total >= 0) {
        rec->key += (rec->total << 7) + 0x4000;
    } else {
        rec->key += -rec->total << 7;
    }
    rec->key += rec->skill;
    if (other != NULL) {
        for (cmp = gSkillCompareRecords, n = 0x36; n != 0; n--, cmp++) {
            if (cmp->used == 0) {
                break;
            }
            if (rec->skill == cmp->skill) {
                for (i = 0; i < 6; i++) {
                    if (rec->pts[i] > cmp->pts[i]) {
                        rec->mark[i] = 4;
                    } else if (cmp->pts[i] > rec->pts[i]) {
                        rec->mark[i] = 2;
                    }
                }
                if (rec->total > cmp->total) {
                    rec->totalMark = 4;
                } else if (cmp->total > rec->total) {
                    rec->totalMark = 2;
                }
                return 1;
            }
        }
        for (i = 0; i < 6; i++) {
            if (rec->pts[i] > 0) {
                rec->mark[i] = 4;
            } else if (rec->pts[i] < 0) {
                rec->mark[i] = 2;
            }
        }
        if (rec->total > 0) {
            rec->totalMark = 4;
        } else if (rec->total < 0) {
            rec->totalMark = 2;
        }
    }
    return 1;
}

s8 Equip_GetSkillPoints(EquipItem *e, u8 kind);

/* Fills r with the pieces of equipment giving points to skill r->skill; 0 if none does. */
s32 SkillList_FillPieces(Player *pl, SkillRecord *rec, Player *other)
{
    rec->pts[0] = Equip_GetSkillPoints(&pl->equip[6], rec->skill) > 0;
    rec->pts[1] = Equip_GetSkillPoints(&pl->equip[2], rec->skill) > 0;
    rec->pts[2] = Equip_GetSkillPoints(&pl->equip[3], rec->skill) > 0;
    rec->pts[3] = Equip_GetSkillPoints(&pl->equip[4], rec->skill) > 0;
    rec->pts[4] = Equip_GetSkillPoints(&pl->equip[5], rec->skill) > 0;
    rec->pts[5] = Equip_GetSkillPoints(&pl->equip[0], rec->skill) > 0;
    if ((rec->pts[0] | rec->pts[1] | rec->pts[2] | rec->pts[3] | rec->pts[4] | rec->pts[5]) == 0) {
        return 0;
    }
    rec->used = 0x82;
    rec->total = 0;
    rec->key = 0;
    return 1;
}

void Player_CalcSkills(Player *p);
/* the calls cast to s16: this unit's code takes the value as s16 */
s32 Player_GetHpSkillBonus(Player *p);
void Player_CalcAttack(Player *p);
void Player_CalcDefense(Player *p);
void Player_CalcResists(Player *p);

/* Recomputes a player's skills, maximum health and stamina, and attack, defense and resistances. */
void Player_RecomputeStats(Player *pl)
{
    s16 health;

    Player_CalcSkills(pl);
    health = gScene.mealHealth + 100 + (s16)Player_GetHpSkillBonus(pl);
    if (health > 150) {
        health = 150;
    }
    pl->hpMax = health;
    pl->hp = health;
    pl->staminaMax = gScene.mealStamina + 300;
    if (pl->staminaMax <= 75) {
        pl->staminaMax = 75;
    } else if (pl->staminaMax > 450) {
        pl->staminaMax = 450;
    }
    pl->attackDrug = gScene.mealAttack;
    pl->attackBuff = 0;
    Player_CalcAttack(pl);
    pl->defenseDrug = gScene.mealDefense;
    pl->defenseBuff = 0;
    Player_CalcDefense(pl);
    Player_CalcResists(pl);
}

/* A code for a piece of equipment (compared with other): 0 none, 1 an id from 900, else 2 blade / 4 gun / 0x8003 armour,
 * +3 | 0x8000 for weapons (+2 for armour) when either has decorations. */
u16 Equip_Code(EquipItem *item, EquipItem *other)
{
    u16 deco;
    u16 code;

    if (item == NULL) {
        return 0;
    }
    if (item->id == 0) {
        return 0;
    }
    if (item->id >= 900) {
        return 1;
    }
    deco = 0;
    if (Equip_CountDecos(item) != 0) {
        deco = 1;
    } else if (other != NULL && other->id != 0 && Equip_CountDecos(other) != 0) {
        deco = 1;
    }
    switch (item->type) {
    case 7:
        code = 4;
        break;
    case 6:
        code = 2;
        break;
    default:
        code = 0x8003;
        if (deco & 0xFFFF) {
            code += 2;
        }
        return code;
    }
    if (deco & 0xFFFF) {
        code += 3;
        code |= 0x8000;
    }
    return code;
}

/* The level (low 3 bits of Equip_Code's code) of a piece of equipment, or the higher of two (a and b),
 * plus one when one has decorations (without b: with flags 0x10, or 0x20 and decorations). */
u8 Equip_LevelMark(EquipItem *item, u8 flags, EquipItem *other)
{
    u16 lvl;
    u16 bit;
    u16 mark;

    if (item != NULL && item->id == 0) {
        item = NULL;
    }
    if (other != NULL && other->id == 0) {
        other = NULL;
    }
    lvl = Equip_Code(item, other);
    bit = lvl & 0x8000;
    lvl &= 7;
    if (other != NULL) {
        mark = Equip_Code(other, item);
        bit |= mark & 0x8000;
        if ((u16)(mark & 7) > lvl) {
            lvl = (u16)(mark & 7);
        }
        if (bit & 0xFFFF) {
            lvl++;
        }
        return lvl;
    }
    if ((flags & 0x10) || ((bit & 0xFFFF) && (flags & 0x20))) {
        lvl++;
    }
    return lvl;
}

u8 Equip_GetClass(EquipItem *e);
/* The detail line lists of the equipment window (8 bytes each, a line kind per byte, 3 ends the list). */
extern u8 gEqDetailBlade[];
extern u8 gEqDetailBladeDeco[];
extern u8 gEqDetailBladeVsGun[];
extern u8 gEqDetailBladeVsGunDeco[];
extern u8 gEqDetailGun[];
extern u8 gEqDetailGunDeco[];
extern u8 gEqDetailBow[];
extern u8 gEqDetailBowDeco[];
extern u8 gEqDetailOnlyBlade[];
extern u8 gEqDetailOnlyBladeDeco[];
extern u8 gEqDetailOnlyGun[];
extern u8 gEqDetailOnlyGunDeco[];
extern u8 gEqDetailArmor[];
extern u8 gEqDetailArmorDeco[];
extern u8 gEqDetailOnlyArmor[];
extern u8 gEqDetailOnlyArmorDeco[];

/* Picks the list of detail lines to show for a piece of equipment a (a blade, a gun, a bowgun of class 10 or armour),
 * compared with b; lists with decoration lines when either has decorations. */
u8 *EquipWindow_PickDetails(EquipItem *item, u8 flags, EquipItem *other)
{
    u8 *details;
    s32 deco;
    s32 type;

    if (item == NULL) {
        if (other == NULL) {
            return gEqDetailOnlyGun;
        }
        deco = Equip_CountDecos(other);
        switch (other->type) {
        case 6:
            return deco == 0 ? gEqDetailOnlyBlade : gEqDetailOnlyBladeDeco;
        case 7:
            return deco == 0 ? gEqDetailOnlyGun : gEqDetailOnlyGunDeco;
        default:
            return deco == 0 ? gEqDetailOnlyArmor : gEqDetailOnlyArmorDeco;
        }
    }
    deco = Equip_CountDecos(item);
    if (other != NULL) {
        deco |= Equip_CountDecos(other);
        type = other->type;
    } else {
        type = -1;
    }
    switch (item->type) {
    case 6:
        switch (type) {
        default:
            details = deco == 0 ? gEqDetailBlade : gEqDetailBladeDeco;
            break;
        case 7:
            details = deco == 0 ? gEqDetailBladeVsGun : gEqDetailBladeVsGunDeco;
            break;
        }
        break;
    case 7:
        switch (Equip_GetClass(item)) {
        default:
            details = deco == 0 ? gEqDetailGun : gEqDetailGunDeco;
            break;
        case 10:
            details = deco == 0 ? gEqDetailBow : gEqDetailBowDeco;
            break;
        }
        break;
    default:
        details = deco == 0 ? gEqDetailArmor : gEqDetailArmorDeco;
        break;
    }
    return details;
}

void EquipWindow_Draw(Player *p, EquipItem *e, s16 x, s16 y, u8 flags, EquipItem *other, u8 c);

/* EquipWindow_Draw for the local player, in window colour 0xB2. */
void EquipWindow_DrawLocal(EquipItem *item, s16 x, s16 y, u8 flags, EquipItem *other)
{
    EquipWindow_Draw(&gPlayers[gScene.localPlayer], item, x, y, flags, other, 0xB2);
}

void Hud_DrawTab(s16 x, s16 y, u8 flags, u8 lvl, u32 color);
void EquipWindow_DrawDetails(Player *p, EquipItem *e, s16 x, s16 y, u8 flags, EquipItem *other);
u8 Equip_LevelMark(EquipItem *a, u8 flags, EquipItem *b);
extern char *gUnknownEquipText[];

/* The window of a piece of equipment in the equipment menus, its frame at alpha `alpha`
 * (the background at half), with its level mark and details. */
void EquipWindow_Draw(Player *pl, EquipItem *item, s16 x, s16 y, u8 flags, EquipItem *other, u8 alpha)
{
    s16 pos[2];
    u8 frame;
    u8 lvl;
    u8 col;
    u8 band;

    if (item != NULL && item->id == 0) {
        item = NULL;
    }
    if (other != NULL && other->id == 0) {
        other = NULL;
    }
    ((u8 *)&gEquipWindowBox.color)[3] = alpha;
    ((u8 *)&gEquipWindowBox.color2)[3] = alpha >> 1;
    frame = flags;
    col = (frame & 8) == 0 ? 0 : 10;
    pos[0] = x;
    pos[1] = y;
    lvl = frame & 7;
    Window_DrawText(&gEquipWindowBox, pos, (u8 *)0);
    if (item != NULL && item->id >= 900) {
        Text_SetColor(col);
        Font_SetPos(x + 0x36, y + 10);
        Text_Draw(gUnknownEquipText[0]);
        return;
    }
    if (!(frame & 0x80)) {
        band = Equip_LevelMark(item, flags, other);
        if (band > 0) {
            Hud_DrawTab(x, y, (frame & 0x40) | (lvl + 1), band, 0x807F7F7F);
        }
    }
    EquipWindow_DrawDetails(pl, item, x, y, flags, other);
}

/* Draws the equipment window's empty box at x, y, its frame at alpha `alpha` and its
 * background at half; the alphas are the top bytes of the Panel's colours. */
void StatusPage_DrawBox(s32 x, s16 y, s32 alpha)
{
    s16 pos[2];

    ((u8 *)&gEquipWindowBox.color)[3] = alpha;          /* the frame colour's alpha */
    pos[0] = x;
    pos[1] = y;
    ((u8 *)&gEquipWindowBox.color2)[3] = (u8)alpha >> 1;  /* the background's, half */
    Window_DrawText(&gEquipWindowBox, pos, (u8 *)0);
}

/* 1 if two pieces of equipment are the same: type, id, level and decorations. */
static s32 EquipItem_Equal(EquipItem *left, EquipItem *right)
{
    if (left->type != right->type) {
        return 0;
    }
    if (left->id != right->id) {
        return 0;
    }
    if (left->level != right->level) {
        return 0;
    }
    if (left->deco[0] != right->deco[0]) {
        return 0;
    }
    if (left->deco[1] != right->deco[1]) {
        return 0;
    }
    return left->deco[2] == right->deco[2];
}

extern u8 gWeaponKindClass[];
u16 Equip_GetKind(EquipItem *e);
u8 WeaponClass_IsGunner(u8 cls);
u8 WeaponType_Flags(u8 cls);
u8 Equip_GetBladeSpecial(EquipItem *e);

/* Puts a weapon in a player's weapon slot and updates what depends on it. */
void Player_SetWeapon(Player *pl, EquipItem *item)
{
    pl->equip[6] = *item;
    pl->weaponKind = Equip_GetKind(&pl->equip[6]);
    pl->weaponType = gWeaponKindClass[pl->weaponKind];
    pl->isGunner = WeaponClass_IsGunner(pl->weaponType);
    pl->weaponFlags = WeaponType_Flags(pl->weaponType);
    pl->bladeSpecial = Equip_GetBladeSpecial(&pl->equip[6]);
}

typedef struct PlayerQ {
    Player p;
} __attribute__((aligned(16))) PlayerQ;
extern Player gPlayerPreview;
void Player_RemoveUnwearable(Player *p);
void Player_CalcSharpnessMax(Player *p);
u8 Player_GetSharpnessLevel(Player *p);

/* A copy of a player (gPlayerPreview) with a piece of equipment in place, for the equipment menus' previews. */
Player *Player_PreviewWith(Player *pl, EquipItem *item)
{
    *(PlayerQ *)&gPlayerPreview = *(PlayerQ *)pl;
    if (gScene.inLobby != 0) {
        if (item->type == 7 || item->type == 6) {
            if (EquipItem_Equal(&pl->equip[6], item) == 0) {
                Player_SetWeapon(&gPlayerPreview, item);
                if (pl->equip[6].type != item->type) {
                    Player_RemoveUnwearable(&gPlayerPreview);
                }
            }
        }
        Player_CalcSkills(&gPlayerPreview);
    }
    if (item->type == 6) {
        Player_CalcSharpnessMax(&gPlayerPreview);
        gPlayerPreview.sharpness = gPlayerPreview.sharpnessMax;
        gPlayerPreview.sharpnessLevel = Player_GetSharpnessLevel(&gPlayerPreview);
    }
    return &gPlayerPreview;
}

/* A line of the bowgun ammo list: its kind, up to three ammo types and their bits. */
typedef struct AmmoLine {
    u8 kind;        /* 0x00: 2 or 3 entries, else 1 */
    u8 ammo[3];     /* 0x01: Bowgun_ClipSize's ammo types */
    u32 mask[3];    /* 0x04: the bits of the three entries */
    u32 lineMask;   /* 0x10: the bit of the line itself */
} AmmoLine;

extern AmmoLine gAmmoLines[];
extern char *gAmmoNameFmt[];
extern char *gAmmoCount1Fmt;
extern char *gAmmoLineFormats[];
extern char *gAmmoCount3Fmt;
extern char *gAmmoNames[];
u8 Bowgun_ClipSize(GunData *gun, u8 kind);

/* Prints ammo line `line`: its name, then each entry's clip size (+ extra) in colour 4 when
 * the entry's bit is set in `loaded` (or extra is not 0), else 5; entries not in `have` and the
 * line when not in `have` stay colour 10. */
void Bowgun_DrawAmmoLine(GunData *gun, u32 have, u32 loaded, u8 line, u8 extra)
{
    AmmoLine *row;
    u8 nameColor;
    u8 sepColor;
    u8 level1Color;
    u8 clips[4];
    u8 sep2Color;
    u8 sep3Color;
    u8 level3Color;
    u8 level2Color;
    char buf[0x2C];

    row = &gAmmoLines[line];
    *(s32 *)clips = 0;
    level1Color = sep2Color = level2Color = level3Color = sep3Color = nameColor = sepColor = 10;
    if (have & row->lineMask) {
        nameColor = (loaded & row->lineMask) ? 4 : 5;
        sepColor = 5;
    }
    if (have & row->mask[0]) {
        clips[0] = extra + Bowgun_ClipSize(gun, row->ammo[0]);
        level1Color = (loaded & row->mask[0]) || extra != 0 ? 4 : 5;
    }
    if (have & row->mask[1]) {
        clips[1] = extra + Bowgun_ClipSize(gun, row->ammo[1]);
        level2Color = (loaded & row->mask[1]) || extra != 0 ? 4 : 5;
        sep2Color = 5;
    }
    if (have & row->mask[2]) {
        clips[2] = extra + Bowgun_ClipSize(gun, row->ammo[2]);
        level3Color = (loaded & row->mask[2]) || extra != 0 ? 4 : 5;
        sep3Color = 5;
    }
    switch (row->kind) {
    case 3:
        sprintf(buf, gAmmoCount3Fmt, level1Color, clips[0], sep2Color, level2Color, clips[1], sep3Color, level3Color, clips[2]);
        break;
    case 2:
        sprintf(buf, gAmmoLineFormats[0], level1Color, clips[0], sep2Color, level2Color, clips[1]);
        break;
    default:
        sprintf(buf, gAmmoCount1Fmt, level1Color, clips[0]);
        break;
    }
    Text_PrintfColored(gAmmoNameFmt[0], nameColor, gAmmoNames[line], sepColor, buf);
}

void Equip_GetIconUV(EquipItem *e, struct Pt *uv0, struct Pt *uv1);
/* unit's own declaration: declared here as Equip_DrawIcon and StatusPage_DrawEquipment call it; with the definition's s32 (u8 type, u16 id), the calls compile differently */
u8 Equip_GetRarity(u8 type, u16 id);
u32 Equip_Color(u8 idx, u8 alpha, s32 alt);

/* Draws the icon of a piece of equipment at (x + 5, y), in its colour (grey with flags 8). */
void Equip_DrawIcon(EquipItem *item, s16 x, s16 y, u8 flags)
{
    Sprite spr;

    spr.x = x + 5;
    spr.y = y;
    spr.w = 0x28;
    spr.h = 0x20;
    Equip_GetIconUV(item, (struct Pt *)&spr.u0, (struct Pt *)&spr.u1);
    if (!(flags & 8)) {
        spr.color = Equip_Color(Equip_GetRarity(item->type, item->id), 0xFF, 0);
    } else {
        spr.color = 0xFF808080;
    }
    Sprite_Draw(&spr);
}

void Text_DrawLines(s16 x, s16 y, char **lines, s16 dy);
void EquipMenu_DrawSharpness(Player *p, EquipItem *e, s16 x, s16 y);
void EquipMenu_DrawElement(EquipItem *e, s16 x, s16 y, EquipItem *cmp);
void EquipMenu_DrawSkills(EquipItem *e, s16 x, s32 y, EquipItem *cmp, u8 flags); /* unit's own declaration: declared here as EquipWindow_DrawDetails calls it; with the definition's void (EquipItem *e, s16 x, s32 y, EquipItem *cmp, u8 flags), the call compiles differently */
s32 Equip_DataWearable(Player *p, EquipItem *e);
u32 Armor_GetDefense(EquipItem *item);
s16 Gun_GetAttack(EquipItem *e);
void SkillList_DrawWithEquip(Player *p, EquipItem *e, s16 x, s16 y);
void EquipWindow_DrawWeaponValue(Player *p, EquipItem *e, s16 x, s16 y, EquipItem *other);
void EquipWindow_DrawSlots(EquipItem *e, s16 x, s16 y, u8 plain);
void EquipWindow_DrawDecorations(EquipItem *e, s16 x, s16 y, u8 flag);
void EquipWindow_DrawWeaponKind(EquipItem *e, s16 x, s16 y);
void EquipWindow_DrawMarks(EquipItem *e, s16 x, s16 y);
void EquipWindow_DrawRow(Player *p, EquipItem *e, s16 x, s16 y);
void EquipWindow_DrawItemList(GunData *gun, s16 x, s16 y);
extern char *gBladeDescTexts[][4];
extern char *gGunDescTexts[][4];
extern char *gArmorDescTexts[][4];
extern char *gActiveSkillsTitle;
extern char *gNoEquipText;
extern char *gHunterTypeLabels[];
extern char *gEquipTypeLabels[];
extern char *gBladeDetailLabels[];
extern char *gBowgunDetailLabels[];
extern char *gBowDetailLabels[];
extern char *gArmorStatLabels[];
extern char *gGunReloadTexts[];
extern char *gGunRecoilTexts[];
extern char *gEquipNameFmt[];
extern char *gScopeVariableText;
extern char *gScopeFixedText[];
extern char *gLongBarrelText;
extern char *gPowerBarrelText[];
extern char *gSilencerText;
extern char *gShieldText[];
extern char *gDashText;
extern char *gAmmoHeaderText[];
extern char *gAmmoClipLabel;
extern char *gAttackPerfText;
extern char *gDecoDetailTitle[];

/* Draws the details page of the equipment window for e (compared with other): weapon attack, sharpness, element,
 * gun options, armour defence and resistances, names, skills, bowgun ammo, decorations. Kept for the match: the
 * locals are declared at the top and shared by the cases, as the game's stack layout shows (d, atk, def, names, then
 * details, rank, base, rankCol, and six colours that the blade, gun and armour cases all use); each case starts by
 * resetting the colours (unused in the blade case, as is its d[9] compare); and a buffer of 17 to 32 bytes sits below
 * the two EquipItem copies. The bowgun case takes the preview player into p before drawing the ammo. Built at
 * optimization level 4, unlike the rest of the file: at the file's level the code comes out far from the game's. */
#pragma optimization_level 4
/* Draws a bowgun's ammo lines in text colour `color`: the ammo it loads, with what pl's
 * skills add (Player_GetSkillAmmoMask) marked, and one more round per clip with SKILL_LOAD_UP.
 * kind picks the page (4 the first lines and their heading). */
static inline void EquipWindow_DrawAmmo(Player *pl, EquipItem *item, s32 x, s32 y, u8 kind, u8 color)
{
    u32 extra;
    u32 have;
    u8 plus;
    GunData *data;

    data = &Equip_GetData(item)->gun;
    have = data->ammoMask;
    extra = ~have & Player_GetSkillAmmoMask(pl);
    plus = 0;
    have |= extra;
    if (Player_HasSkill(pl, SKILL_LOAD_UP) == 1) {
        plus = 1;
    }
    Text_SetColor(color);
    Font_SetPos(x, y);
    if (kind == 4) {
        Text_Printf(gAmmoHeaderText[0]);
        Font_SetPos(x, y + 0x14);
        Bowgun_DrawAmmoLine(data, have, extra, 0, plus);
        Font_SetPos(x, y + 0x28);
        Bowgun_DrawAmmoLine(data, have, extra, 1, plus);
        Font_SetPos(x, y + 0x3C);
        Bowgun_DrawAmmoLine(data, have, extra, 2, plus);
        Font_SetPos(x, y + 0x50);
        Bowgun_DrawAmmoLine(data, have, extra, 3, plus);
        Font_SetPos(x, y + 0x64);
        Bowgun_DrawAmmoLine(data, have, extra, 4, plus);
        Font_SetPos(x + 0xB4, y + 0x14);
        Bowgun_DrawAmmoLine(data, have, extra, 5, plus);
        Font_SetPos(x + 0xB4, y + 0x28);
        Bowgun_DrawAmmoLine(data, have, extra, 6, plus);
        Font_SetPos(x + 0xB4, y + 0x3C);
        Bowgun_DrawAmmoLine(data, have, extra, 7, plus);
        Font_SetPos(x + 0xB4, y + 0x50);
        Bowgun_DrawAmmoLine(data, have, extra, 8, plus);
    } else {
        Text_Printf(gAmmoClipLabel);
        Font_SetPos(x, y + 0x14);
        Bowgun_DrawAmmoLine(data, have, extra, 9, plus);
        Font_SetPos(x, y + 0x28);
        Bowgun_DrawAmmoLine(data, have, extra, 10, plus);
        Font_SetPos(x, y + 0x3C);
        Bowgun_DrawAmmoLine(data, have, extra, 11, plus);
        Font_SetPos(x, y + 0x50);
        Bowgun_DrawAmmoLine(data, have, extra, 12, plus);
        Font_SetPos(x, y + 0x64);
        Bowgun_DrawAmmoLine(data, have, extra, 13, plus);
        Font_SetPos(x + 0x99, y + 0x14);
        Bowgun_DrawAmmoLine(data, have, extra, 14, plus);
        Font_SetPos(x + 0x99, y + 0x28);
        Bowgun_DrawAmmoLine(data, have, extra, 15, plus);
        Font_SetPos(x + 0x99, y + 0x3C);
        Bowgun_DrawAmmoLine(data, have, extra, 16, plus);
        Font_SetPos(x + 0x99, y + 0x50);
        Bowgun_DrawAmmoLine(data, have, extra, 17, plus);
    }
}

/* Draws one page of a piece of equipment's details at x, y; the page's kind comes from
 * EquipWindow_PickDetails's table for the item, at flags & 7: 0 its name, kind, marks and
 * (a bowgun) its parts; 1 who can use it, its values (compared with other) and its slots;
 * 2 its skill points (EquipMenu_DrawSkills); 3 the skills in effect; 4-5 the ammo pages;
 * 6-7 the weapon's own rows and item list; 8-9 its decorations; 10 the no-equipment text.
 * flags 8 greys the page without a comparison, flags 0x20 drops the comparison. */
void EquipWindow_DrawDetails(Player *pl, EquipItem *item, s16 x, s16 y, u8 flags, EquipItem *other)
{
    EquipData *data;
    u32 atk;
    u16 def;
    char **names;
    u8 *details;
    u8 rank;
    s32 cls;
    u8 col;
    u8 base;
    u8 rankCol;
    u8 kind;
    u8 cSil;
    u8 cLvl;
    u8 cAff;
    u8 cOpt;
    EquipData *otherData;
    u8 cAtk;
    u8 cRng;
    char **lines;
    char buf[32];
    EquipItem itemCopy;
    EquipItem otherCopy;

    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    details = EquipWindow_PickDetails(item, flags, other);
    if (flags & 0x20) {
        other = NULL;
    }
    if (item != NULL) {
        rank = Equip_GetRarity(item->type, item->id);
        cls = Equip_GetClass(item);
    } else {
        rank = 0;
        cls = 0;
    }
    if (!(flags & 8)) {
        base = 5;
        col = 0;
        rankCol = Rarity_Color(rank);
    } else {
        col = 10;
        base = 10;
        other = NULL;
        rankCol = 10;
    }
    Text_SetColor(base);
    kind = details[flags & 7];
    switch (kind) {
    case 0:
        Equip_DrawIcon(item, x, y, flags);
        Font_SetPos(x + 0x2D, y + 0xA);
        Text_Printf(gEquipNameFmt[0], rankCol, Equip_GetName(item->type, item->id));
        switch (item->type) {
        case 6: {
            u32 oatk;

            data = Equip_GetData(item);
            atk = (u16)((u16)data->blade.attack * gWeaponClassMultipliers[cls] / 100);
            cAtk = cOpt = cAff = cLvl = cSil = cRng = 0;
            if (other != NULL) {
                if (item->type == other->type) {
                    otherData = Equip_GetData(other);
                    oatk = (u16)((u16)otherData->blade.attack * gWeaponClassMultipliers[Equip_GetClass(other)] / 100);
                    if (otherData->blade.sharpMax < data->blade.sharpMax) {
                        cAff = 4;
                    } else if (data->blade.sharpMax < otherData->blade.sharpMax) {
                        cAff = 2;
                    }
                } else {
                    oatk = (u16)(Gun_GetAttack(other) * gWeaponClassMultipliers[Equip_GetClass(other)] / 100);
                }
                if (oatk < atk) {
                    cAtk = 4;
                } else if (atk < oatk) {
                    cAtk = 2;
                }
            }
            Text_SetColor(cAtk);
            Font_SetPos(x + 0x48, y + 0x28);
            Text_PrintfColored(gStatusFmtInt4, atk);
            EquipMenu_DrawSharpness(pl, item, x + 0x9D, y + 0x3E);
            EquipMenu_DrawElement(item, x, y + 0x50, other);
            switch (cls) {
            case 9:
                EquipWindow_DrawWeaponKind(item, x, y);
                break;
            case 8:
                EquipWindow_DrawMarks(item, x, y);
                break;
            }
            lines = gBladeDetailLabels;
            break;
        }
        case 7: {
            EquipData *otherData;
            u32 oatk;

            data = Equip_GetData(item);
            atk = (u16)(gWeaponClassMultipliers[cls] * Gun_GetAttack(item) / 100);
            cAtk = cAff = cLvl = cSil = cOpt = cRng = 0;
            if (other != NULL) {
                if (item->type == other->type) {
                    otherData = Equip_GetData(other);
                    oatk = (u16)(gWeaponClassMultipliers[Equip_GetClass(other)] * Gun_GetAttack(other) / 100);
                    if (otherData->gun.reload < data->gun.reload) {
                        cAff = 4;
                    } else if (data->gun.reload < otherData->gun.reload) {
                        cAff = 2;
                    }
                    buf[0] = other->level & 0xF;
                    buf[1] = item->level & 0xF;
                    if (buf[0] < buf[1]) {
                        cLvl = 4;
                    } else if (buf[1] < buf[0]) {
                        cLvl = 2;
                    }
                    if (other->level & 0x40) {
                        if (!(item->level & 0x40)) {
                            cSil = 2;
                        }
                    } else if (item->level & 0x40) {
                        cSil = 4;
                    }
                    if (other->level & 0xB0) {
                        if (!(item->level & 0xB0)) {
                            cOpt = 2;
                        }
                    } else if (item->level & 0xB0) {
                        cOpt = 4;
                    }
                    if (otherData->gun.recoil < data->gun.recoil) {
                        cRng = 4;
                    } else if (data->gun.recoil < otherData->gun.recoil) {
                        cRng = 2;
                    }
                } else {
                    otherData = Equip_GetData(other);
                    oatk = (u16)((u16)otherData->blade.attack * gWeaponClassMultipliers[Equip_GetClass(other)] / 100);
                }
                if (oatk < atk) {
                    cAtk = 4;
                } else if (atk < oatk) {
                    cAtk = 2;
                }
            }
            Text_SetColor(cAtk);
            Font_SetPos(x + 0x48, y + 0x28);
            Text_PrintfColored(gStatusFmtInt4, atk);
            if (cls != 10) {
                Text_SetColor(cAff);
                Font_SetPos(x + 0x5A, y + 0x3C);
                Text_Draw(gGunReloadTexts[data->gun.reload]);
                Text_SetColor(cRng);
                Font_SetPos(x + 0x5A, y + 0x50);
                Text_Draw(gGunRecoilTexts[data->gun.recoil]);
                Text_SetColor(cLvl);
                Font_SetPos(x + 0x6C, y + 0x64);
                buf[0] = 0x82;
                buf[1] = (item->level & 0xF) + 0x50;
                buf[2] = 0;
                Text_Draw(buf);
                Font_SetSpacing(0x10, 0x12);
                Text_SetColor(cSil);
                Font_SetPos(x + 0xAF, y + 0x28);
                if (item->level & 0x40) {
                    Text_Draw(gScopeVariableText);
                } else {
                    Text_Draw(gScopeFixedText[0]);
                }
                Font_SetSpacing(0x12, 0x12);
                Text_SetColor(cOpt);
                Font_SetPos(x + 0xC4, y + 0x3C);
                if (cls == 1) {
                    if (item->level & 0x20) {
                        Text_Draw(gPowerBarrelText[0]);
                    } else if (item->level & 0x80) {
                        Text_Draw(gShieldText[0]);
                    }
                } else {
                    if (item->level & 0x20) {
                        Text_Draw(gLongBarrelText);
                    } else if (item->level & 0x10) {
                        Text_Draw(gSilencerText);
                    }
                }
                EquipMenu_DrawElement(item, x + 0xC6, y + 0x50, other);
                lines = gBowgunDetailLabels;
            } else {
                /* FAKEMATCH: x + 0. The game passes an int x here, re-extended from the case's (s32)x copy (the
                 * other calls pass the s16 x as it is); a zero layout offset beside x + 0xC6 above is the one form
                 * found. Plain x: 10 words off, (s32)x: 15. */
                EquipMenu_DrawElement(item, x + 0, y + 0x3C, other);
                lines = gBowDetailLabels;
            }
            break;
        }
        default: {
            ArmorData *data;
            ArmorData *otherData;
            s32 odef;

            data = &Equip_GetData(item)->armor;
            def = (u8)Armor_GetDefense(item);
            cAtk = cOpt = cAff = cLvl = cSil = cRng = col;
            if (other != NULL && item->type == other->type) {
                otherData = &Equip_GetData(other)->armor;
                odef = (u8)Armor_GetDefense(other);
                if (odef < def) {
                    cAtk = 4;
                } else if (def < odef) {
                    cAtk = 2;
                }
                if (otherData->resist[0] < data->resist[0]) {
                    cOpt = 4;
                } else if (data->resist[0] < otherData->resist[0]) {
                    cOpt = 2;
                }
                if (otherData->resist[1] < data->resist[1]) {
                    cAff = 4;
                } else if (data->resist[1] < otherData->resist[1]) {
                    cAff = 2;
                }
                if (otherData->resist[2] < data->resist[2]) {
                    cLvl = 4;
                } else if (data->resist[2] < otherData->resist[2]) {
                    cLvl = 2;
                }
                if (otherData->resist[3] < data->resist[3]) {
                    cSil = 4;
                } else if (data->resist[3] < otherData->resist[3]) {
                    cSil = 2;
                }
                if (otherData->resist[4] < data->resist[4]) {
                    cRng = 4;
                } else if (data->resist[4] < otherData->resist[4]) {
                    cRng = 2;
                }
            }
            Text_SetColor(col);
            Font_SetPos(x + 0x48, y + 0x28);
            Text_PrintfColored(gStatusFmtInt3, item->level + 1);
            Text_SetColor(cAtk);
            Font_SetPos(x + 0xD8, y + 0x28);
            Text_PrintfColored(gStatusFmtInt3, def);
            Text_SetColor(cOpt);
            Font_SetPos(x + 0x48, y + 0x3C);
            Text_PrintfColored(gStatusFmtInt3, data->resist[0]);
            Text_SetColor(cAff);
            Font_SetPos(x + 0xD8, y + 0x3C);
            Text_PrintfColored(gStatusFmtInt3, data->resist[1]);
            Text_SetColor(cLvl);
            Font_SetPos(x + 0x48, y + 0x50);
            Text_PrintfColored(gStatusFmtInt3, data->resist[2]);
            Text_SetColor(cSil);
            Font_SetPos(x + 0xD8, y + 0x50);
            Text_PrintfColored(gStatusFmtInt3, data->resist[3]);
            Text_SetColor(cRng);
            Font_SetPos(x + 0x48, y + 0x64);
            Text_PrintfColored(gStatusFmtInt3, data->resist[4]);
            lines = gArmorStatLabels;
            break;
        }
        }
        Text_SetColor(base);
        Text_DrawLines(x, y + 0x28, lines, 0x14);
        break;
    case 1: {
        s32 id;
        u8 k;
        u8 armorFlags;

        Equip_DrawIcon(item, x, y + 0xE, flags);
        id = item->id;
        switch (item->type) {
        case 7:
            Text_DrawLines(x, y + 0x3C, gHunterTypeLabels, 0x14);
            names = gGunDescTexts[id];
            break;
        case 6:
            Text_DrawLines(x, y + 0x3C, gHunterTypeLabels, 0x14);
            names = gBladeDescTexts[id];
            break;
        case 0:
            id += 0xED;
        case 5:
            id += 0xEE;
        case 4:
            id += 0xF3;
        case 3:
            id += 0xFD;
        case 2:
            Text_DrawLines(x, y + 0x3C, gEquipTypeLabels, 0x14);
            names = gArmorDescTexts[id];
            break;
        }
        Text_SetColor(col);
        Text_DrawLines(x + 0x36, y, names, 0x14);
        Font_SetPos(x + 0x90, y + 0x3C);
        if (item->id != 0) {
            switch (item->type) {
            case 6:
            case 7:
                k = item->type - 6;
                break;
            default:
                armorFlags = Equip_GetData(item)->armor.flags;
                if ((armorFlags & 0xC) == 0xC) {
                    k = 2;
                } else if (armorFlags & 4) {
                    k = 0;
                } else {
                    k = 1;
                }
                break;
            }
            Text_Draw(gHunterClassTexts[k]);
        } else {
            Text_Draw(gDashText);
        }
        Font_SetPos(x + 0x48, y + 0x50);
        if (item->id != 0) {
            Text_SetColor(rankCol);
            buf[0] = 0x82;
            buf[2] = 0;
            buf[1] = rank + 0x50;
            Text_Draw(buf);
        } else {
            Text_Draw(gDashText);
        }
        EquipWindow_DrawWeaponValue(pl, item, x + 0x90, y + 0x50, other);
        EquipWindow_DrawSlots(item, x + 0x5A, y + 0x64, (flags & 8) != 0);
        break;
    }
    case 2: {
        /* unit's own declaration: the game passes y as a halfword; with the definition's void (EquipItem *e, s16 x, s32 y, EquipItem *cmp, u8 flags), EquipWindow_DrawDetails compiles differently */
        void EquipMenu_DrawSkills(EquipItem *item, s16 x, s16 y, EquipItem *cmp, u8 flags);

        EquipMenu_DrawSkills(item, x, y, other, flags);
        break;
    }
    case 3: {
        /* unit's own declaration: the game passes the second argument as a halfword; with the definition's void (Player *a, s32 b, s16 c), EquipWindow_DrawDetails compiles differently */
        void Player_DrawSkillsPlain(Player *a, s16 b, s16 c);
        u32 textY;
        u32 textX;

        textY = y;
        textX = x;
        Font_SetPos(textX, textY);
        Text_Draw(gActiveSkillsTitle);
        if (item != NULL) {
            if (item->type != 7 && item->type != 6 && !(u8)Equip_DataWearable(pl, item)) {
                Font_SetColor(0x80404040);
                flags |= 8;
            }
            if (!(flags & 8)) {
                if (!EquipItem_Equal(&pl->equip[item->type], item)) {
                    if (gScene.inLobby) {
                        SkillList_DrawWithEquip(pl, item, x, y);
                    }
                } else {
                    Player_DrawSkillsPlain(pl, x, y);
                }
            } else if (gScene.inLobby) {
                itemCopy = *item;
                itemCopy.id = 0;
                itemCopy.deco[2] = 0;
                itemCopy.deco[1] = 0;
                itemCopy.deco[0] = 0;
                SkillList_DrawWithEquip(pl, &itemCopy, x, y);
            } else {
                Font_SetPos(textX, textY + 0x14);
                Text_Draw(gSkillEffectNames[0]);
            }
        } else if (other != NULL && gScene.inLobby) {
            otherCopy = *other;
            otherCopy.id = 0;
            otherCopy.deco[2] = 0;
            otherCopy.deco[1] = 0;
            otherCopy.deco[0] = 0;
            SkillList_DrawWithEquip(pl, &otherCopy, x, y);
        } else {
            Player_DrawSkillsPlain(pl, x, y);
        }
        break;
    }
    case 4:
    case 5:
        pl = Player_PreviewWith(pl, item);
        EquipWindow_DrawAmmo(pl, item, x, y, kind, col);
        break;
    case 6:
    case 7: {
        GunData *data;

        Text_SetColor(col);
        Font_SetPos(x, y);
        Text_Draw(gAttackPerfText);
        data = &Equip_GetData(item)->gun;
        if (kind == 6) {
            EquipWindow_DrawRow(pl, item, x, y);
        } else {
            EquipWindow_DrawItemList(data, x, y);
        }
        break;
    }
    case 8:
    case 9:
        Text_SetColor(base);
        Font_SetPos(x, y);
        Text_Draw(gDecoDetailTitle[0]);
        Text_SetColor(col);
        EquipWindow_DrawDecorations(item, x, y + 0x14, kind - 8);
        break;
    case 10:
        Text_SetColor(col);
        Font_SetPos(x + 0x36, y + 0xA);
        Text_Draw(gNoEquipText);
        break;
    }
}
#pragma optimization_level reset

s16 Player_GetWeaponStat(Player *p, u8 kind);
void Sjis_Widen(char *src, char *dst);
extern char *gAffinityLabel[];
extern char *gPercentFmt;

/* The weapon's value 8 (Player_GetWeaponStat; only for guns or from sharpness 3) for the player with it, coloured
 * against the one of other (higher: 0x802B7F7F, lower: 0x807F212E) or when a skill 0x23-0x25 applies. */
void EquipWindow_DrawWeaponValue(Player *pl, EquipItem *item, s16 x, s16 y, EquipItem *other)
{
    char out[24];
    char buf[8];
    Player *preview;
    u32 color;
    s16 value;
    s16 otherValue;

    if (item->type == 7 || item->type == 6) {
        color = 0x807F7F7F;
        preview = Player_PreviewWith(pl, item);
        value = 0;
        if (item->type == 7 || preview->sharpnessLevel >= 3) {
            value = Player_GetWeaponStat(preview, WSTAT_AFFINITY);
            if (Player_HasSkill(preview, 0x23) == 1 || Player_HasSkill(preview, 0x24) == 1 || Player_HasSkill(preview, 0x25) == 1) {
                color = 0x802B7F7F;
            }
        }
        if (other != NULL) {
            preview = Player_PreviewWith(pl, other);
            otherValue = 0;
            if (other->type == 7 || preview->sharpnessLevel >= 3) {
                otherValue = Player_GetWeaponStat(preview, WSTAT_AFFINITY);
            }
            color = 0x807F7F7F;
            if (value > otherValue) {
                color = 0x802B7F7F;
            } else if (otherValue > value) {
                color = 0x807F212E;
            }
        }
        Font_SetColor(STATUS_COLOR_LABEL);
        Font_SetPos(x, y);
        Text_PrintfColored(gAffinityLabel[0]);
        Font_SetColor(color);
        Font_SetPos(x + 0x48, y);
        sprintf(buf, gPercentFmt, value);
        Sjis_Widen(buf, out);
        Text_PrintfColored(out);
    }
}

u8 Equip_GetSlotCount(EquipItem *item);
s32 Equip_FreeSlots(EquipItem *e);
void Equip_ListSlots(EquipItem *e, u16 *out);
extern u32 gItemFontColors[];

/* The decoration slots of a piece of equipment from (x, y): a filled mark per decoration (in its colour unless plain),
 * a hollow one per free slot, a dash for the rest of the 3. */
void EquipWindow_DrawSlots(EquipItem *item, s16 x, s16 y, u8 plain)
{
    u16 deco[3];
    s32 i;
    u8 slots;
    s32 used;

    slots = Equip_GetSlotCount(item);
    used = slots - Equip_FreeSlots(item);
    Equip_ListSlots(item, deco);
    Font_SetColor(0x80505050);
    for (i = 0; i < used; i++) {
        if (!(plain & 0xFF)) {
            Font_SetColor(gItemFontColors[gItemData[deco[i]].color]);
        }
        Font_SetPos(x, y);
        Text_Draw(TXT(0x00490960) /* a full-width black circle */); /* a filled circle */
        x += 0x14;
    }
    Font_SetColor(0x80505050);
    for (; i < slots; i++) {
        Font_SetPos(x, y);
        Text_Draw(TXT(0x00490968) /* a full-width white circle */); /* a hollow circle */
        x += 0x14;
    }
    for (; i < 3; i++) {
        Font_SetPos(x, y);
        Text_Draw(TXT(0x00490970) /* a full-width minus sign */); /* a dash */
        x += 0x14;
    }
}

DecoData *Item_GetDecoration(u16 item);

/* The lines a decoration's skills take in the equipment window, two skills a line; 0 for no
 * decoration. */
s32 Decoration_SkillRows(u16 item)
{
    u8 *skill;
    s32 i;
    s32 n;

    if (item == 0) {
        return 0;
    }
    skill = Item_GetDecoration(item)->skills[0];
    for (i = n = 0; i < 4; i++) {
        if (*skill == 0) {
            break;
        }
        n++;
        skill += 2;
    }
    return (n + 1) >> 1;
}

extern u8 gDecoRowLayouts[];
extern char *gItemNames[];
extern char *gSkillNames[];
extern char *gDecoNameFmt;
extern char *gDecoSkillFmt[];

/* The decorations of a piece of equipment and their skills: the combination of its decoration sizes
 * (Decoration_SkillRows, base 3) picks the entries of gDecoRowLayouts (low bits: how many to list; with flag, the high
 * nibble: one slot); each with its name and up to four skills, two per line. */
void EquipWindow_DrawDecorations(EquipItem *item, s16 x, s16 y, u8 flag)
{
    u8 *data[2];
    char *name[2];
    s32 i;
    u8 *skill;
    s32 k;
    s32 n;
    u8 **dataPtr;
    char **namePtr;
    s32 left;
    s32 slot;
    s32 scale;
    s32 j;
    s32 id;

    for (i = 0, k = 0, scale = 1; i < 3; i++) {
        if (item->deco[i] != 0) {
            k += scale * Decoration_SkillRows(item->deco[i]);
        }
        scale *= 3;
    }
    data[0] = data[1] = NULL;
    if (flag == 0) {
        switch (gDecoRowLayouts[k] & 3) {
        case 2:
            data[1] = (u8 *)Item_GetDecoration(item->deco[1]);
            name[1] = gItemNames[item->deco[1]];
            /* fall through */
        case 1:
            data[0] = (u8 *)Item_GetDecoration(item->deco[0]);
            name[0] = gItemNames[item->deco[0]];
            break;
        }
    } else {
        j = gDecoRowLayouts[k] >> 4;
        if (j != 0) {
            id = item->deco[j];
            data[0] = (u8 *)Item_GetDecoration(id);
            name[0] = gItemNames[id];
        }
    }
    for (n = 0, dataPtr = data, namePtr = name, left = x; n < 2; n++, dataPtr++, namePtr++) {
        if (*dataPtr == NULL) {
            break;
        }
        Font_SetPos(x, y);
        Text_PrintfColored(gDecoNameFmt, *namePtr);
        y += 0x14;
        skill = *dataPtr + 8;
        for (slot = 0; slot < 4; slot++, skill += 2) {
            if (*skill == 0) {
                break;
            }
            Font_SetPos(left + (slot & 1) * 0xA2, y + (slot >> 1) * 0x14);
            if (*skill != 0x29 && *skill != 0x2A && *skill != 0x2B) {
                Text_PrintfColored(gDecoSkillFmt[0], gSkillNames[*skill], (s8)skill[1]);
            } else {
                Text_Draw(gSkillNames[*skill]);
            }
        }
        y += ((slot + 1) >> 1) * 0x14;
    }
}

u8 Equip_GetBladeSpecial(EquipItem *e);
extern u8 gShellTypeOfSpecial[];
extern char *gShellTypeTexts[];

/* Draws the name of a weapon's (Equip_GetBladeSpecial) kind at (x + 0x7E, y + 0x28). */
void EquipWindow_DrawWeaponKind(EquipItem *item, s16 x, s16 y)
{
    Font_SetColor(STATUS_COLOR_VALUE);
    Font_SetPos(x + 0x7E, y + 0x28);
    Text_Draw(gShellTypeTexts[gShellTypeOfSpecial[Equip_GetBladeSpecial(item)]]);
}

u8 Equip_GetBladeSpecial(EquipItem *e);
extern char *gHornNotesLabel;
extern u8 gBladeSpecialMarks[][3];
extern u32 gHornNoteColors[];

/* A label and three coloured marks (gBladeSpecialMarks by Equip_GetBladeSpecial's kind) for a weapon. */
void EquipWindow_DrawMarks(EquipItem *item, s16 x, s16 y)
{
    Sprite spr;
    s32 i;
    u8 *marks;

    Font_SetColor(STATUS_COLOR_VALUE);
    Font_SetPos(x + 0x6C, y + 0x28);
    Text_Draw(gHornNotesLabel);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    spr.x = x + 0xB4;
    spr.y = y + 0x29;
    spr.w = 0x14;
    spr.h = 0x10;
    *(u32 *)&spr.u0 = 0xB400AC;
    *(u32 *)&spr.u1 = 0xC400BC;
    marks = gBladeSpecialMarks[Equip_GetBladeSpecial(item)];
    for (i = 3; i > 0; i--) {
        spr.color = gHornNoteColors[*marks];
        Sprite_Draw(&spr);
        marks++;
        spr.x += 0x1A;
    }
}

extern char *gEquipNameFmt[];
extern s16 gBowgunClipShots[][4];
extern char *gBowShotTexts[];

/* A heading and the three (four with SKILL_LOAD_UP) entries gBowgunClipShots of a piece of equipment's row 0xF,
 * for the player as it would be with it (Player_PreviewWith). */
void EquipWindow_DrawRow(Player *pl, EquipItem *item, s16 x, s16 y)
{
    GunData *data;
    s32 i;
    s32 n;
    s32 k;

    pl = Player_PreviewWith(pl, item);
    data = &Equip_GetData(item)->gun;
    n = 3;
    k = 0x14;
    if (Player_HasSkill(pl, SKILL_LOAD_UP) == 1) {
        n++;
        k = 0x15;
    }
    Font_SetPos(x, y + 0x18);
    Font_SetColor(0x807F5230);
    Text_PrintfStyled(3, gEquipNameFmt[k]);
    Font_SetColor(STATUS_COLOR_LABEL);
    for (i = 0; i < n; i++, x += 0x48) {
        Font_SetPos(x, y + 0x44);
        Text_PrintfColored(gBowShotTexts[gBowgunClipShots[data->clipRow][i]]);
    }
}

extern char *gBowCoatingsLabel;
extern u16 gEquipListItemBits[4];
extern u16 gEquipListItems[4];
extern char *gItemNames[];

/* A heading, then the names of the items gEquipListItems whose bits gEquipListItemBits are set in o's 0x14, one per line. */
void EquipWindow_DrawItemList(GunData *gun, s16 x, s16 y)
{
    s32 i;

    Font_SetColor(0x807F5230);
    Font_SetPos(x, y + 0x14);
    Text_Draw(gBowCoatingsLabel);
    Font_SetColor(STATUS_COLOR_LABEL);
    y += 0x28;
    for (i = 0; i < 4; i++) {
        if (gun->ammoMask & gEquipListItemBits[i]) {
            Font_SetPos(x, y);
            Text_PrintfColored(gItemNames[gEquipListItems[i]]);
            y += 0x14;
        }
    }
}

/* The menu windows' initialised data. */
extern char *gHelpTexts_0[];
extern char *D_00447610[];
extern char *gHelpTexts_3[];
extern char *D_00447620[];
extern char *D_00447630[];
extern char *gHelpTexts_9[];
extern char *gHelpTexts_10[];
extern char *gHelpTexts_11[];
extern char *gHelpTexts_12[];
extern char D_00448200[];
extern char D_00448210[];
extern char D_00448230[];
extern char D_00448250[];
extern char D_00448280[];
extern char D_004482A0[];
extern char D_004482C0[];
extern char D_004482D0[];
extern char D_004482F0[];
extern char D_00448320[];
extern char D_00448340[];
extern char D_00448360[];
extern char D_00448380[];
extern char D_004483C0[];
extern char D_004483E0[];
extern char *gHelpTexts_6[];
u8 gStatusEquipIconU[8] = {
#include "sub_main/004326B8.inc"
};
s32 gWeaponClassMultipliers[12] = {
#include "sub_main/004326C0.inc"
};
u32 gHornNoteColors[8] = {
#include "sub_main/004326F0.inc"
};
u8 gChatWhoColors[8] = {
#include "sub_main/00432710.inc"
};
u8 gChatTextSizes[8] = {
#include "sub_main/00432718.inc"
};
u8 gChatSizeLineGaps[16] = {
#include "sub_main/00432720.inc"
};
s8 gChatSystemRecord[304] = {
#include "sub_main/00432730.inc"
};
u8 gEntryPictureIndex[80] = {
#include "sub_main/00432860.inc"
};
u8 gWantedPictureCells[80] = {
#include "sub_main/004328B0.inc"
};
u8 gEqDetailBlade[8] = {
#include "sub_main/00432900.inc"
};
u8 gEqDetailBladeDeco[8] = {
#include "sub_main/00432908.inc"
};
u8 gEqDetailBladeVsGun[8] = {
#include "sub_main/00432910.inc"
};
u8 gEqDetailBladeVsGunDeco[8] = {
#include "sub_main/00432918.inc"
};
u8 gEqDetailGun[8] = {
#include "sub_main/00432920.inc"
};
u8 gEqDetailGunDeco[8] = {
#include "sub_main/00432928.inc"
};
u8 gEqDetailBow[8] = {
#include "sub_main/00432930.inc"
};
u8 gEqDetailBowDeco[8] = {
#include "sub_main/00432938.inc"
};
u8 gEqDetailOnlyBlade[8] = {
#include "sub_main/00432940.inc"
};
u8 gEqDetailOnlyBladeDeco[8] = {
#include "sub_main/00432948.inc"
};
u8 gEqDetailOnlyGun[8] = {
#include "sub_main/00432950.inc"
};
u8 gEqDetailOnlyGunDeco[8] = {
#include "sub_main/00432958.inc"
};
u8 gEqDetailArmor[8] = {
#include "sub_main/00432960.inc"
};
u8 gEqDetailArmorDeco[8] = {
#include "sub_main/00432968.inc"
};
u8 gEqDetailOnlyArmor[8] = {
#include "sub_main/00432970.inc"
};
u8 gEqDetailOnlyArmorDeco[8] = {
#include "sub_main/00432978.inc"
};
u16 gEquipListItems[4] = {
#include "sub_main/00432980.inc"
};
u16 gEquipListItemBits[4] = {
#include "sub_main/00432988.inc"
};
u8 gDecoRowLayouts[16] = {
#include "sub_main/00432990.inc"
};
s16 gDecoDetailPos[4][2] = {
#include "sub_main/004329A0.inc"
};
extern char D_004481B0[];
extern char D_004481E0[];
char *gHelpTexts_1[948] = {
    D_004481B0, D_004481E0, D_00448200, D_00448210,
    D_00448230, D_00448250, D_00448280, D_004482A0,
    gHelpTexts_1_8, gHelpTexts_1_8, D_004482C0, D_004482D0,
    D_004482F0, D_00448320, D_00448340, D_00448360,
    gHelpTexts_1_8, gHelpTexts_1_8, gHelpTexts_1_8, gHelpTexts_1_8,
    D_00448380, D_004483C0, D_004483E0, gHelpTexts_1_8,
    D_0046D118, D_0046D120, D_0046D170, D_0046D1C0,
    D_0046D220, D_0046D280, D_0046D2E0, D_0046D330,
    D_0046D350, D_0046D370, D_0046D3A0, D_0046D3D0,
    D_0046D400, D_0046D420, D_0046D460, D_0046D4C0,
    D_0046D4F0, D_0046D520, D_0046D560, D_0046D5A0,
    D_0046D5D0, D_0046D610, D_0046D630, D_0046D650,
    D_0046D6B0, D_0046D700, D_0046D750, D_0046D790,
    D_0046D7C0, D_0046D800, D_0046D850, D_0046D898,
    D_0046D8B0, D_0046D8F0, D_0046D920, D_0046D970,
    D_0046D9D0, D_0046DA20, D_0046DA70, D_0046DAC0,
    D_0046DB00, D_0046DB60, D_0046DBB0, D_0046DC00,
    D_0046DC40, D_0046DC70, D_0046DCA0, D_0046DCD0,
    D_0046DD00, D_0046DD30, D_0046DD80, D_0046DDC0,
    D_0046DE00, D_0046DE50, D_0046DE90, D_0046DEE0,
    D_0046DF00, D_0046DF40, D_0046DF70, D_0046DFB0,
    D_0046E000, D_0046E050, D_0046E0A0, D_0046D898,
    D_0046E0E0, D_0046E110, D_0046E160, D_0046E1B0,
    D_0046E200, D_0046E250, D_0046E2A0, D_0046E2C0,
    D_0046E310, D_0046E370, D_0046E3A0, D_0046E3C0,
    D_0046E400, D_0046E430, D_0046E480, D_0046E4C0,
    D_0046E510, D_0046E550, D_0046E570, D_0046E5A0,
    D_0046E5E0, D_0046E630, D_0046E670, D_0046D898,
    D_0046E6C0, D_0046E6F0, D_0046E710, D_0046E730,
    D_0046E750, D_0046E7A0, D_0046E7D0, D_0046E830,
    D_0046E880, D_0046E8C0, D_0046E900, D_0046E950,
    D_0046E970, D_0046E9C0, D_0046EA20, D_0046EA70,
    D_0046EAB0, D_0046EAF0, D_0046EB40, D_0046EB80,
    D_0046EBC0, D_0046EC00, D_0046EC40, D_0046EC80,
    D_0046ECC0, D_0046ED00, D_0046ED40, D_0046ED90,
    D_0046EDE0, D_0046EE30, D_0046EE80, D_0046EED0,
    D_0046EF20, D_0046EF70, D_0046EFB0, D_0046EFF0,
    D_0046F030, D_0046F080, D_0046F0D0, D_0046F120,
    D_0046F170, D_0046F1C0, D_0046F210, D_0046F260,
    D_0046F2A0, D_0046F2E0, D_0046D898, D_0046F328,
    D_0046F340, D_0046F380, D_0046F3C0, D_0046F400,
    D_0046F440, D_0046D898, D_0046F480, D_0046F4B0,
    D_0046F4E0, D_0046F510, D_0046F530, D_0046F560,
    D_0046F590, D_0046F5B0, D_0046F610, D_0046F640,
    D_0046F670, D_0046F6B0, D_0046F6E0, D_0046F700,
    D_0046F730, D_0046F770, D_0046F7B0, D_0046F800,
    D_0046F840, D_0046F880, D_0046F8D0, D_0046F920,
    D_0046F960, D_0046F9A0, D_0046F9F0, D_0046FA40,
    D_0046FA80, D_0046FAC0, D_0046D898, D_0046D898,
    D_0046FB00, D_0046FB50, D_0046FB70, D_0046FB90,
    D_0046FBE0, D_0046FC40, D_0046FC90, D_0046FCE0,
    D_0046FD30, D_0046FD50, D_0046FD90, D_0046FDE0,
    D_0046FE00, D_0046FE50, D_0046FE70, D_0046FE90,
    D_0046FEB0, D_0046FF00, D_0046FF30, D_0046FF70,
    gHelpTexts_1_216, D_00470020, D_00470070, gHelpTexts_1_219,
    D_00470110, D_00470130, D_00470150, D_00470170,
    D_004701A0, gHelpTexts_1_225, D_00470210, D_00470250,
    D_004702B0, D_00470300, D_00470350, D_004703B0,
    D_00470400, D_00470440, D_00470470, D_00470490,
    D_004704C0, D_004704E0, D_00470540, D_004705A0,
    D_004705D0, D_00470610, D_00470660, D_004706C0,
    D_00470710, D_00470770, D_004707C0, D_0046D898,
    D_00470810, D_00470850, D_004708A0, D_004708F0,
    D_00470950, D_0046D898, D_004709A0, D_004709F0,
    D_00470A40, D_00470A90, D_00470AE0, D_00470B10,
    D_00470B50, D_00470B70, D_00470B90, D_00470BB0,
    D_00470BD0, D_00470BF0, D_00470C10, D_00470C40,
    D_00470C70, D_00470CB0, D_00470CF0, D_00470D30,
    D_00470D80, D_00470DD0, D_00470E10, D_00470E40,
    D_00470E80, D_00470EB0, D_00470F00, D_00470F48,
    D_00470F60, D_00470FB0, D_00471000, D_00471050,
    D_00471070, D_004710C0, D_004710E0, D_00471130,
    D_00471180, D_0046D898, D_004711D0, D_00471210,
    D_00471250, D_004712B0, D_00471300, D_00471340,
    D_00471380, D_004713C0, D_00471400, D_00471440,
    D_00471480, D_004714D0, D_00471520, D_00471570,
    D_004715C0, D_00471600, D_00471650, D_00471690,
    D_004716E0, D_00471720, D_00471770, D_004717B0,
    D_00471800, D_00471850, D_004718A0, D_004718E0,
    D_00471920, D_00471960, D_004719A0, D_004719E0,
    D_00471A20, D_00471A60, D_00471AB0, D_00471AF0,
    D_00471B40, D_0046D898, D_00471B80, D_00471BE0,
    D_00471C30, D_0046D898, D_00471C90, D_0046D898,
    D_00471CF0, D_00471D40, D_00471DA0, D_00471DF0,
    D_00471E30, D_00471E80, D_00471EE0, D_00471F40,
    D_00471F90, D_00471FC0, D_00471FF0, D_00472030,
    D_00472090, D_004720C0, D_00472100, D_00472150,
    D_004721B0, D_00472200, D_00472250, D_00472290,
    D_004722D0, D_00472320, D_00472380, D_004723D0,
    D_004723F0, D_00472430, D_00472450, D_00472480,
    D_004724B0, D_004724F0, D_00472540, D_00472580,
    D_004725D0, D_00472620, D_00472670, D_004726D0,
    D_00472720, D_00472770, D_004727C0, D_00472810,
    D_00472850, D_004728A0, D_004728F0, D_00472940,
    D_004729A0, D_004729E0, D_00472A30, D_00472A80,
    D_00472AE0, D_00472B20, D_00472B70, D_00472BD0,
    D_00472C20, D_00472C70, D_00472CD0, D_00472D10,
    D_00472D60, D_00472DA0, D_00472DF0, D_00472E40,
    D_00472E90, D_00472EC0, D_00472F10, D_00472F60,
    D_00472FB0, D_00472FF0, D_00473040, D_00473080,
    D_004730D0, D_00473120, D_00473170, D_004731A0,
    D_004731E0, D_00473230, D_00473280, D_004732D0,
    D_00473320, D_00473380, D_004733D0, D_00473420,
    D_00473470, D_004734C0, D_00473500, D_00473560,
    D_004735B0, D_00473600, D_00473650, D_004736B0,
    D_00473710, D_00473760, D_004737B0, D_004737F0,
    D_00473840, D_00473880, D_004738E0, D_00473930,
    D_00473980, D_004739D0, D_00473A20, D_00473A70,
    D_00473AD0, D_00473B10, D_00473B60, D_004736B0,
    D_00473BC0, D_00473C10, D_00473C60, D_00473CA0,
    D_00473CF0, D_00473D40, D_00473D90, D_00473DF0,
    D_00473E40, D_00473E90, D_00473EE0, D_00473F20,
    D_00473F70, D_00473FC0, D_00474010, D_00474060,
    D_004740B0, D_00474100, D_00474150, D_00474180,
    D_004741D0, D_00474220, D_00474270, D_004742C0,
    D_00474310, D_00474350, D_00474390, D_004743E0,
    D_00474430, D_00474480, D_004744D0, D_00474530,
    D_00474580, D_004745D0, D_00474620, D_00474670,
    D_004746C0, D_00474720, D_00474760, D_004747B0,
    D_00474800, D_00474850, D_004748A0, D_00474900,
    D_00474950, D_004749A0, D_00474A00, D_00474A60,
    D_00474AB0, D_00474AF0, D_00474A00, D_00474B50,
    D_00474BA0, D_00474BF0, D_00474C50, D_00474CB0,
    D_00474D00, D_00474D50, D_00474DA0, D_00474DF0,
    D_00474E40, D_00474E90, D_00474ED0, D_00474F30,
    D_00474F80, D_00474FD0, D_00475010, D_00475060,
    D_004750B0, D_00475110, D_00475160, D_004751B0,
    D_00475200, D_00475260, D_004752B0, D_00475300,
    D_00475340, D_00475390, D_004753F0, D_00475450,
    D_004754A0, D_004754E0, D_00475540, D_004755A0,
    D_004755F0, D_00475640, D_00475690, D_004756F0,
    D_00475740, D_00475790, D_004757D0, D_00475820,
    D_00475860, D_0046D898, D_0046D898, D_004758B0,
    D_00475900, D_00475950, D_004759B0, D_00475A10,
    D_00475A60, D_00475AA0, D_00475AF0, D_00475B40,
    D_00475B80, D_00475BB0, D_00475BF0, D_00475C40,
    D_0046D898, D_00475C90, D_00475CE0, D_00475D30,
    D_00475D80, D_00475DD0, D_00475E20, D_00475E70,
    D_00475EC0, D_00475EF0, D_00475F40, D_00475F90,
    D_00475FC0, D_00476000, D_0046D898, D_00476040,
    D_00476080, D_004760B0, D_004760F0, D_00476130,
    D_00476170, D_004761B0, D_0046D898, D_004761F0,
    D_00476240, D_004762A0, D_004762F0, D_0046D898,
    D_00476330, D_00476370, D_004763C0, D_00476410,
    D_00476460, D_004764C0, D_00476510, D_00476560,
    D_004765B0, D_00476600, D_00476640, D_00476670,
    D_004766C0, D_00476700, D_00476750, D_004767A0,
    D_004767F0, D_00476840, D_00476890, D_004768D0,
    D_0046D898, D_00476920, D_00476940, D_00476990,
    D_004769E0, D_00476A40, D_00476A90, D_00476AC0,
    D_00476B00, D_00476B50, D_00476BA0, D_00476BF0,
    D_0046D898, D_00476C30, D_00476C70, D_00476CC0,
    D_00476D10, D_00476D60, D_00476DB0, D_00476E00,
    D_00476E50, D_00476EA0, D_00476F00, D_00476F40,
    D_00476F90, D_0046D898, D_00476FE0, D_00477020,
    D_00477050, D_004770A0, D_004770E0, D_00477110,
    D_00477160, D_004771B0, D_00477210, D_00477260,
    D_004772B0, D_00477300, D_00477350, D_004773B0,
    D_004773F0, D_00477430, D_00477458, D_00477470,
    D_00477470, D_00477470, D_00477470, D_00477470,
    D_004774D0, D_004774D0, D_004774D0, D_004774D0,
    D_004774D0, D_00477520, D_00477580, D_004775E0,
    D_00477640, D_00477690, D_004776E0, D_00477740,
    D_00477790, D_004777E0, D_00477830, D_00477880,
    D_004778D0, D_004778D0, D_00477920, D_00477980,
    D_004779E0, D_00477A30, D_00477A30, D_00477A80,
    D_00477AD0, D_00477B20, D_00477B70, D_00477BD0,
    D_00477C20, D_00477C70, D_00477CD0, D_00477D20,
    D_00477D70, D_00477DC0, D_00477E20, D_0046D898,
    D_0046D898, D_0046D898, D_0046D898, D_00477E80,
    D_00477EE0, D_00477F40, D_00477F80, D_00477FB0,
    D_00477FE0, D_00478030, D_00478080, D_004780D0,
    D_00478100, D_00478150, D_00478190, D_004781E0,
    D_00478210, D_00478260, D_00478270, D_00478290,
    D_004782D0, D_00478310, D_00478330, D_00478350,
    D_00478390, D_004783E0, D_00478430, D_00478480,
    D_004784C0, D_00478510, D_00478550, D_00478590,
    D_004785E0, D_00478620, D_00478670, D_004786C0,
    D_00478710, D_00478750, D_00478790, D_004787D0,
    D_00478820, D_00478860, D_004788A0, D_004788E0,
    D_00478920, D_0046D898, D_00478970, D_004789C0,
    D_00478A10, D_00478A60, D_00478A60, D_00478A60,
    D_00478A60, D_00478AB0, D_00478B00, D_00478B50,
    D_00478B90, D_00478BD0, D_00478C10, D_00478C10,
    D_00478C10, D_00478C10, D_00478C10, D_00478C10,
    D_00478C60, D_00478CB0, D_00478D00, D_00478D50,
    D_00478DA0, D_00478E00, D_00478E50, D_00478EB0,
    D_00478F00, D_00478F50, D_00478FA0, D_00479000,
    D_00479060, D_004790C0, D_00479120, D_00479180,
    D_004791D0, D_00479210, D_00479250, D_004792A0,
    D_004792E0, D_00479340, D_00479360, D_004793B0,
    D_00479410, D_00479460, D_00479490, D_004794F0,
    D_00479540, D_00479590, D_004795E0, D_00479630,
    D_00479660, D_004796B0, D_00479700, D_00479750,
    D_004797A0, D_004797F0, D_00479840, D_00479890,
    D_004798E0, D_00479930, D_00479980, D_004799E0,
    D_00479A20, D_00479A60, D_00479AA0, D_00479AE0,
    D_00479B30, D_00479B70, D_00479BC0, D_00479C10,
    D_00479C50, D_00479C90, D_00479CD0, D_00479D20,
    D_00479D70, D_00479DB0, D_00479DF0, D_00479E30,
    D_00479E70, D_00479EB0, D_00479EF0, D_00479F30,
    D_00479F70, D_00479FB0, D_0047A000, D_0047A040,
    D_0047A090, D_0047A0E0, D_0047A130, D_0047A180,
    D_0047A1D0, D_0047A210, D_0047A260, D_0047A2B0,
    D_0047A300, D_0047A340, D_0047A390, D_0047A390,
    D_0047A3C0, D_0047A400, D_0047A440, D_0047A460,
    D_0047A480, D_0047A480, D_0047A4A0, D_0047A4C0,
    D_0047A4E0, D_0047A4E0, D_0047A510, D_0047A530,
    D_0047A550, D_0047A570, D_0047A590, D_0047A5B0,
    D_0047A5D0, D_0047A5F0, D_0047A5F0, D_0047A610,
    D_0047A630, D_0047A660, D_0047A680, D_0047A6B0,
    D_0047A6D0, D_0047A700, D_0047A730, D_0047A760,
    D_0047A790, D_0047A7C0, D_0047A7E0, D_0047A810,
    D_0047A830, D_0047A850, D_0047A870, D_0047A8A0,
    D_0047A8D0, D_0047A900, D_0047A930, D_0047A960,
    D_0047A9A0, D_0047A9E0, D_0047AA20, D_0047AA60,
    D_0047AAA0, D_0047AAE0, D_0047AB20, D_0047AB60,
    D_0047ABA0, D_0047ABE0, D_0047AC20, D_0047AC50,
    D_0047AC80, D_0047ACB0, D_0047ACE0, D_0047AD00,
    D_0047AD30, D_0047AD50, D_0047AD80, D_0047ADB0,
    D_0047ADE0, D_0047AE10, D_0047AE40, D_0047AE70,
    D_0047AE70, D_0047AE90, D_0047AEC0, D_0047AEF0,
    D_0047AEF0, D_0047AF20, D_0047AF40, D_0047AF60,
    D_0047AF90, D_0047AFB0, D_0047AFD0, D_0047B000,
    D_0047B020, D_0047B040, D_0047A460, D_0047A4A0,
    D_0047A4A0, D_0047A4C0, D_0047A570, D_0047A590,
    D_0047A5B0, D_0047A5F0, D_0047A630, D_0047A660,
    D_0047A6B0, D_0047A7E0, D_0047A830, D_0047A870,
    D_0047A9A0, D_0047AAE0, D_0047AB20, D_0047AB60,
    D_0047ABA0, D_0047ABE0, D_0047B070, D_0047B0A0,
    D_0047ACE0, D_0047AD00, D_0047AD30, D_0047AD50,
    D_0047AD80, D_0047ADB0, D_0047ADE0, D_0047AE10,
    D_0047AE40, D_0047AE70, D_0047AE90, D_0047AEC0,
    D_0047AF40, D_0047AFD0, D_0047A660, D_0047A6B0,
    D_0047A700, D_0047ACB0, D_0047AD50, D_0047AD80,
    D_0047ADB0, D_0047ADE0, D_0047AE10, D_0047AE40,
    D_0047AFD0, NULL, NULL, NULL,
};
/* Entries 4 and 5 are the lobby overlay's lists: the game menu's and the item box's help lines. */
char **gHelpTexts[16] = {
    gHelpTexts_0, gHelpTexts_1, D_00447610, gHelpTexts_3,
    gGameMenuHelpTexts, gBoxHelpTexts, gHelpTexts_6, D_00447620,
    D_00447630, gHelpTexts_9, gHelpTexts_10, gHelpTexts_11,
    gHelpTexts_12, NULL, NULL, NULL,
};
char *gBladeDescTexts[486][4] = {
    D_0047B0D0, gHelpTexts_1_8, gHelpTexts_1_8, NULL,
    D_0047B0F0, D_0047B110, D_0047B130, NULL,
    D_0047B0F0, D_0047B110, D_0047B130, NULL,
    D_0047B150, D_0047B170, D_0047B190, NULL,
    D_0047B1B0, D_0047B1D0, D_0047B1F0, NULL,
    D_0047B210, D_0047B230, D_0047B240, NULL,
    D_0047B150, D_0047B170, D_0047B190, NULL,
    D_0047B260, D_0047B280, D_0047B2A0, NULL,
    D_0047B2C0, D_0047B2E0, D_0047B300, NULL,
    D_0047B320, D_0047B340, D_0047B360, NULL,
    D_0047B380, D_0047B3A0, D_0047B3C0, NULL,
    D_0047B380, D_0047B3A0, D_0047B3C0, NULL,
    D_0047B380, D_0047B3A0, D_0047B3C0, NULL,
    D_0047B3E0, D_0047B400, D_0047B420, NULL,
    D_0047B3E0, D_0047B400, D_0047B420, NULL,
    D_0047B3E0, D_0047B400, D_0047B420, NULL,
    D_0047B440, D_0047B450, D_0047B470, NULL,
    D_0047B440, D_0047B450, D_0047B470, NULL,
    D_0047B490, D_0047B4B0, D_0047B4D0, NULL,
    D_0047B4F0, D_0047B510, D_0047B530, NULL,
    D_0047B490, D_0047B4B0, D_0047B4D0, NULL,
    D_0047B550, D_0047B570, D_0047B590, NULL,
    D_0047B5B0, D_0047B5D0, D_0047B5F0, NULL,
    D_0047B550, D_0047B570, D_0047B590, NULL,
    D_0047B610, D_0047B630, D_0047B650, NULL,
    D_0047B610, D_0047B630, D_0047B650, NULL,
    D_0047B670, D_0047B690, D_0047B6B0, NULL,
    D_0047B670, D_0047B690, D_0047B6B0, NULL,
    D_0047B6C0, D_0047B110, D_0047B130, NULL,
    D_0047B6C0, D_0047B110, D_0047B130, NULL,
    D_0047B6E0, D_0047B700, D_0047B720, NULL,
    D_0047B440, D_0047B450, D_0047B470, NULL,
    D_0047B6E0, D_0047B700, D_0047B720, NULL,
    D_0047B740, D_0047B760, D_0047B780, NULL,
    D_0047B7A0, D_0047B7C0, D_0047B7E0, NULL,
    D_0047B800, D_0047B820, D_0047B840, NULL,
    D_0047B860, D_0047B880, D_0047B8A0, NULL,
    D_0047B8C0, D_0047B8E0, D_0047B900, NULL,
    D_0047B920, D_0047B940, D_0047B960, NULL,
    D_0047B980, D_0047B9A0, D_0047B9C0, NULL,
    D_0047B9E0, D_0047BA00, D_0047BA20, NULL,
    D_0047B980, D_0047B9A0, D_0047B9C0, NULL,
    D_0047BA40, D_0047BA60, D_0047BA80, NULL,
    D_0047BA40, D_0047BA60, D_0047BA80, NULL,
    D_0047BAA0, D_0047BAC0, D_0047BAE0, NULL,
    D_0047BAA0, D_0047BAC0, D_0047BAE0, NULL,
    D_0047B9E0, D_0047BA00, D_0047BA20, NULL,
    D_0047BB00, D_0047BB20, D_0047BB40, NULL,
    D_0047BB60, gHelpTexts_1_8, gHelpTexts_1_8, NULL,
    D_0047BB80, D_0047BBA0, D_0047BBC0, NULL,
    D_0047BBE0, D_0047BC00, D_0047BC20, NULL,
    D_0047BBE0, D_0047BC00, D_0047BC20, NULL,
    D_0047BC40, D_0047BC60, D_0047BC80, NULL,
    D_0047BCA0, D_0047BCC0, D_0047BCE0, NULL,
    D_0047BC40, D_0047BC60, D_0047BC80, NULL,
    D_0047BD00, D_0047BD20, D_0047BD40, NULL,
    D_0047BD00, D_0047BD20, D_0047BD40, NULL,
    D_0047B150, D_0047BD60, gHelpTexts_1_8, NULL,
    D_0047BD80, D_0047BDA0, D_0047BDC0, NULL,
    D_0047B150, D_0047BD60, gHelpTexts_1_8, NULL,
    D_0047BDE0, D_0047BE00, D_0047BE20, NULL,
    D_0047BE40, D_0047BE60, D_0047BE80, NULL,
    D_0047BEA0, D_0047BEC0, D_0047BEE0, NULL,
    D_0047BF00, D_0047BF20, D_0047BF40, NULL,
    D_0047BF60, D_0047BF80, D_0047BFA0, NULL,
    D_0047BF60, D_0047BF80, D_0047BFA0, NULL,
    D_0047BFC0, D_0047BFE0, D_0047C000, NULL,
    D_0047BFC0, D_0047BFE0, D_0047C000, NULL,
    D_0047C020, D_0047C040, D_0047C060, NULL,
    D_0047C080, D_0047C0A0, D_0047C0C0, NULL,
    D_0047C0E0, D_0047C100, D_0047C120, NULL,
    D_0047C140, D_0047C160, D_0047C180, NULL,
    D_0047C0E0, D_0047C100, D_0047C120, NULL,
    D_0047C1A0, D_0047C1C0, D_0047C1E0, NULL,
    D_0047C200, D_0047C220, D_0047C240, NULL,
    D_0047C260, D_0047C280, D_0047C2A0, NULL,
    D_0047C2C0, D_0047C2E0, D_0047C300, NULL,
    D_0047C320, D_0047C340, D_0047C360, NULL,
    D_0047C380, D_0047BD20, D_0047BD40, NULL,
    D_0047C380, D_0047BD20, D_0047BD40, NULL,
    D_0047C3A0, D_0047BD60, D_0047C3C0, NULL,
    D_0047C3E0, D_0047C400, D_0047C420, NULL,
    D_0047C3A0, D_0047BD60, D_0047C440, NULL,
    D_0047C460, D_0047C480, D_0047C4A0, NULL,
    D_0047C460, D_0047C480, D_0047C4A0, NULL,
    D_0047C4C0, D_0047C4E0, D_0047C500, NULL,
    D_0047C260, D_0047C280, D_0047C2A0, NULL,
    D_0047C1A0, D_0047C1C0, D_0047C1E0, NULL,
    D_0047C520, D_0047C540, D_0047C560, NULL,
    D_0047C580, D_0047C5A0, D_0047C5C0, NULL,
    D_0047C5E0, D_0047C600, D_0047C620, NULL,
    D_0047C640, D_0047C660, D_0047C680, NULL,
    D_0047C640, D_0047C660, D_0047C680, NULL,
    D_0047C6A0, D_0047C6C0, D_0047C6E0, NULL,
    D_0047C700, D_0047C720, D_0047C740, NULL,
    D_0047C760, D_0047C780, D_0047BAE0, NULL,
    D_0047C760, D_0047C780, D_0047BAE0, NULL,
    D_0047C7A0, D_0047C7C0, D_0047C7E0, NULL,
    D_0047C800, D_0047C820, D_0047C840, NULL,
    D_0047C860, D_0047C870, gHelpTexts_1_8, NULL,
    D_0047C860, D_0047C870, gHelpTexts_1_8, NULL,
    D_0047C890, D_0047C8B0, D_0047C8D0, NULL,
    D_0047C890, D_0047C8B0, D_0047C8D0, NULL,
    D_0047C8F0, D_0047C910, D_0047C930, NULL,
    D_0047C8F0, D_0047C910, D_0047C930, NULL,
    D_0047C950, D_0047C970, gHelpTexts_1_8, NULL,
    D_0047C990, D_0047C9B0, D_0047C9D0, NULL,
    D_0047C950, D_0047C970, gHelpTexts_1_8, NULL,
    D_0047C9F0, D_0047CA10, D_0047CA30, NULL,
    D_0047C9F0, D_0047CA10, D_0047CA30, NULL,
    D_0047CA50, D_0047CA70, D_0047CA90, NULL,
    D_0047CAC0, D_0047CAE0, D_0047CB00, NULL,
    D_0047CB20, D_0047CB40, D_0047CB60, NULL,
    D_0047CA50, D_0047CA70, D_0047CA90, NULL,
    D_0047CB80, D_0047CBA0, D_0047CBC0, NULL,
    D_0047CBF0, D_0047CC10, D_0047CC30, NULL,
    D_0047CC50, D_0047CC70, D_0047CC90, NULL,
    D_0047CBF0, D_0047CC10, D_0047CC30, NULL,
    D_0047C5E0, D_0047C600, D_0047CCB0, NULL,
    D_0047C5E0, D_0047C600, D_0047CCB0, NULL,
    D_0047CCD0, D_0047CCF0, D_0047CD10, NULL,
    D_0047CD30, D_0047CD50, D_0047CD70, NULL,
    D_0047CCD0, D_0047CCF0, D_0047CD10, NULL,
    D_0047B980, D_0047CD90, D_0047B9C0, NULL,
    D_0047CD30, D_0047CD50, D_0047CDB0, NULL,
    D_0047CDD0, D_0047CDF0, D_0047CE10, NULL,
    D_0047CE30, D_0047CE50, D_0047CE70, NULL,
    D_0047CEA0, D_0047C910, D_0047C930, NULL,
    D_0047CEA0, D_0047C910, D_0047C930, NULL,
    D_0047CEC0, D_0047CEE0, D_0047C3C0, NULL,
    D_0047CF00, D_0047CF20, D_0047CF40, NULL,
    D_0047CEC0, D_0047CEE0, D_0047B720, NULL,
    D_0047CF70, D_0047CF90, D_0047CFB0, NULL,
    D_0047CFD0, D_0047CFF0, D_0047D010, NULL,
    D_0047D030, D_0047D050, D_0047D070, NULL,
    D_0047D090, D_0047D0B0, D_0047D0D0, NULL,
    D_0047D0F0, D_0047D110, D_0047D130, NULL,
    D_0047D150, D_0047D170, D_0047D190, NULL,
    D_0047D1C0, D_0047D1E0, D_0047D210, NULL,
    D_0047D230, D_0047D250, D_0047D270, NULL,
    D_0047D290, D_0047D2B0, D_0047D2D0, NULL,
    D_0047D230, D_0047D250, D_0047D270, NULL,
    D_0047D2F0, D_0047D310, D_0047D330, NULL,
    D_0047D350, D_0047D370, D_0047D390, NULL,
    D_0047D3B0, D_0047D3D0, D_0047D3F0, NULL,
    D_0047D410, D_0047D430, D_0047D450, NULL,
    D_0047D3B0, D_0047D3D0, D_0047D3F0, NULL,
    D_0047D470, D_0047D490, D_0047D4B0, NULL,
    D_0047C6A0, D_0047D4D0, D_0047C6E0, NULL,
    D_0047C700, D_0047D4F0, D_0047C740, NULL,
    D_0047D510, D_0047D530, D_0047BAE0, NULL,
    D_0047D510, D_0047D530, D_0047BAE0, NULL,
    D_0047D410, D_0047D430, D_0047D450, NULL,
    D_0047D550, D_0047D570, D_0047D590, NULL,
    D_0047D5B0, D_0047D5D0, D_0047D5F0, NULL,
    D_0047D610, D_0047D630, D_0047D650, NULL,
    D_0047D670, D_0047D690, D_0047D6B0, NULL,
    D_0047D670, D_0047D690, D_0047D6B0, NULL,
    D_0047D6D0, D_0047D6F0, gHelpTexts_1_8, NULL,
    D_0047D6D0, D_0047D6F0, gHelpTexts_1_8, NULL,
    D_0047D6D0, D_0047D6F0, gHelpTexts_1_8, NULL,
    D_0047D710, D_0047D730, D_0047D750, NULL,
    D_0047D770, D_0047D790, D_0047D7B0, NULL,
    D_0047D7D0, D_0047D7F0, D_0047D810, NULL,
    D_0047D830, D_0047D850, D_0047D870, NULL,
    D_0047D890, D_0047D8B0, D_0047D8D0, NULL,
    D_0047D8F0, D_0047D910, D_0047D930, NULL,
    D_0047D950, D_0047D970, D_0047D990, NULL,
    D_0047D7D0, D_0047D7F0, D_0047D810, NULL,
    D_0047D950, D_0047D970, D_0047D990, NULL,
    D_0047D9B0, D_0047D9D0, D_0047D9F0, NULL,
    D_0047DA10, D_0047DA30, D_0047DA50, NULL,
    D_0047D9B0, D_0047D9D0, D_0047D9F0, NULL,
    D_0047DA70, D_0047DA90, D_0047DAB0, NULL,
    D_0047DAD0, D_0047DAF0, D_0047DB10, NULL,
    D_0047DA70, D_0047DA90, D_0047DAB0, NULL,
    D_0047DAD0, D_0047DAF0, D_0047DB10, NULL,
    D_0047DB30, D_0047DB50, D_0047DB70, NULL,
    D_0047DB90, D_0047D690, D_0047D6B0, NULL,
    D_0047DB90, D_0047D690, D_0047D6B0, NULL,
    D_0047DBB0, D_0047DBD0, D_0047B190, NULL,
    D_0047DBB0, D_0047DBD0, D_0047B190, NULL,
    D_0047DBF0, D_0047DC10, D_0047DC30, NULL,
    D_0047DC50, D_0047DC70, D_0047DC90, NULL,
    D_0047DCB0, (char *)D_0047DCD0, D_0047DCF0, NULL,
    D_0047DC50, D_0047DC70, D_0047DC90, NULL,
    D_0047DD10, D_0047DD30, D_0047DD50, NULL,
    D_0047DD70, D_0047DD90, D_0047DDB0, NULL,
    D_0047DDD0, D_0047DDF0, D_0047DE10, NULL,
    D_0047DD70, D_0047DD90, D_0047DDB0, NULL,
    D_0047DE30, D_0047DE50, D_0047DE70, NULL,
    D_0047DE90, D_0047DEB0, D_0047DED0, NULL,
    D_0047DEF0, D_0047DF10, D_0047DF30, NULL,
    D_0047DE90, D_0047DEB0, D_0047DED0, NULL,
    D_0047DEF0, D_0047DF10, D_0047DF30, NULL,
    D_0047DF50, D_0047DF70, D_0047DF90, NULL,
    D_0047DF50, D_0047DF70, D_0047DF90, NULL,
    D_0047DFB0, D_0047D530, D_0047BAE0, NULL,
    D_0047DFB0, D_0047D530, D_0047BAE0, NULL,
    D_0047DFD0, D_0047DFF0, D_0047E010, NULL,
    D_0047E030, D_0047E050, D_0047E070, NULL,
    D_0047E090, D_0047E0B0, D_0047E0D0, NULL,
    D_0047E090, D_0047E0B0, D_0047E0D0, NULL,
    D_0047D5B0, D_0047D5D0, D_0047E0F0, NULL,
    D_0047C2C0, D_0047E110, D_0047E130, NULL,
    D_0047E150, D_0047E170, D_0047E190, NULL,
    D_0047E1B0, D_0047E1D0, D_0047E1F0, NULL,
    D_0047E210, D_0047E230, D_0047E250, NULL,
    D_0047E270, D_0047E290, D_0047E2B0, NULL,
    D_0047E2D0, D_0047E2F0, D_0047E310, NULL,
    D_0047E320, D_0047E340, D_0047E360, NULL,
    D_0047B8C0, D_0047B8E0, D_0047B900, NULL,
    D_0047C580, D_0047C5A0, D_0047C5C0, NULL,
    D_0047C020, D_0047C040, D_0047C060, NULL,
    D_0047C5E0, D_0047C600, D_0047C620, NULL,
    D_0047E380, D_0047E3A0, D_0047E3C0, NULL,
    D_0047E3E0, D_0047E400, D_0047E420, NULL,
    D_0047E440, D_0047E460, D_0047E480, NULL,
    D_0047D0F0, D_0047D110, D_0047D130, NULL,
    D_0047D5B0, D_0047D5D0, D_0047D5F0, NULL,
    D_0047E4A0, D_0047E4C0, D_0047E4E0, NULL,
    D_0047E500, D_0047E520, D_0047E540, NULL,
    D_0047D5B0, D_0047D5D0, D_0047E0F0, NULL,
    D_0047E560, D_0047E580, D_0047E5A0, NULL,
    D_0047E5C0, D_0047E5E0, D_0047E600, NULL,
    D_0047E620, D_0047E640, D_0047E660, NULL,
    D_0047E670, D_0047E690, D_0047E6B0, NULL,
    D_0047E6D0, D_0047E6F0, D_0047E710, NULL,
    D_0047E730, D_0047E750, D_0047E770, NULL,
    D_0047E790, D_0047E7B0, D_0047E7D0, NULL,
    D_0047D610, D_0047D630, D_0047D650, NULL,
    D_0047E620, D_0047E640, D_0047E660, NULL,
    D_0047E7E0, D_0047E800, D_0047E820, NULL,
    D_0047E840, D_0047E860, D_0047E880, NULL,
    D_0047E8A0, D_0047E8C0, D_0047E8E0, NULL,
    D_0047E900, D_0047E920, D_0047E940, NULL,
    D_0047E960, D_0047E980, D_0047E9A0, NULL,
    D_0047E9C0, D_0047E9E0, D_0047EA00, NULL,
    D_0047E960, D_0047E980, D_0047E9A0, NULL,
    D_0047EA20, D_0047EA40, D_0047EA60, NULL,
    D_0047EA20, D_0047EA40, D_0047EA60, NULL,
    D_0047EA80, D_0047EAA0, D_0047EAC0, NULL,
    D_0047C2C0, D_0047EAE0, D_0047EB00, NULL,
    D_0047EA80, D_0047EAA0, D_0047EAC0, NULL,
    D_0047C6A0, D_0047EB20, D_0047C6E0, NULL,
    D_0047EB40, D_0047EB60, D_0047EB80, NULL,
    D_0047EBA0, D_0047EBC0, D_0047EBE0, NULL,
    D_0047EBA0, D_0047EBC0, D_0047EBE0, NULL,
    D_0047EC00, D_0047EC20, D_0047EC40, NULL,
    D_0047EC00, D_0047EC20, D_0047EC40, NULL,
    D_0047EC60, D_0047EC80, D_0047ECA0, NULL,
    D_0047EC60, D_0047EC80, D_0047ECA0, NULL,
    D_0047ECC0, D_0047ECE0, D_0047ED00, NULL,
    D_0047ECC0, D_0047ECE0, D_0047ED00, NULL,
    D_0047ED20, D_0047ED40, D_0047ED60, NULL,
    D_0047ED20, D_0047ED40, D_0047ED60, NULL,
    D_0047ED20, D_0047ED40, D_0047ED60, NULL,
    D_0047ED70, D_0047ED90, D_0047EDB0, NULL,
    D_0047ED70, D_0047ED90, D_0047EDB0, NULL,
    D_0047EDD0, D_0047EDF0, D_0047EE10, NULL,
    D_0047EDD0, D_0047EDF0, D_0047EE10, NULL,
    D_0047B260, D_0047B280, D_0047B2A0, NULL,
    D_0047BD80, D_0047BDA0, D_0047BDC0, NULL,
    D_0047EE30, D_0047EE50, D_0047EE70, NULL,
    D_0047D8F0, D_0047D910, D_0047D930, NULL,
    D_0047EE90, D_0047EEB0, D_0047EED0, NULL,
    D_0047EEF0, D_0047EF10, D_0047EF30, NULL,
    D_0047EF50, D_0047EF70, D_0047EF90, NULL,
    D_0047C2C0, D_0047EFB0, D_0047EFD0, NULL,
    D_0047EFF0, D_0047F010, D_0047F030, NULL,
    D_0047F050, D_0047F070, D_0047F090, NULL,
    D_0047F0B0, D_0047F0D0, D_0047F0F0, NULL,
    D_0047F0B0, D_0047F0D0, D_0047F0F0, NULL,
    D_0047F110, D_0047F130, D_0047F150, NULL,
    D_0047E270, D_0047E290, D_0047E2B0, NULL,
    D_0047F170, D_0047F190, D_0047F1B0, NULL,
    D_0047F1C0, D_0047F1E0, D_0047F200, NULL,
    D_0047F220, D_0047F240, D_0047F260, NULL,
    D_0047F280, D_0047F2A0, D_0047F2C0, NULL,
    D_0047F1C0, D_0047F1E0, D_0047F200, NULL,
    D_0047F2E0, D_0047F300, D_0047F320, NULL,
    D_0047F340, D_0047F360, D_0047F380, NULL,
    D_0047F3A0, D_0047F360, D_0047F380, NULL,
    D_0047F3C0, D_0047F3E0, D_0047F400, NULL,
    D_0047F3C0, D_0047F3E0, D_0047F400, NULL,
    D_0047E270, D_0047E290, D_0047E2B0, NULL,
    D_0047E320, D_0047E340, D_0047E360, NULL,
    D_0047F420, D_0047F440, D_0047F460, NULL,
    D_0047F480, D_0047F4A0, D_0047F4C0, NULL,
    D_0047F4E0, D_0047F500, D_0047F520, NULL,
    D_0047BAA0, D_0047F540, D_0047F560, NULL,
    D_0047BAA0, D_0047F540, D_0047F560, NULL,
    D_0047F580, D_0047F5A0, D_0047F5C0, NULL,
    D_0047F5E0, D_0047F600, D_0047F620, NULL,
    D_0047F640, D_0047F660, D_0047F680, NULL,
    D_0047F6A0, D_0047F6C0, D_0047F6E0, NULL,
    D_0047F700, D_0047F720, D_0047F740, NULL,
    D_0047F700, D_0047F720, D_0047F740, NULL,
    D_0047F760, D_0047F780, D_0047F7A0, NULL,
    D_0047F7C0, D_0047F7E0, D_0047F800, NULL,
    D_0047F820, D_0047F840, D_0047F860, NULL,
    D_0047F820, D_0047F840, D_0047F860, NULL,
    D_0047F880, D_0047F8A0, D_0047F8C0, NULL,
    D_0047F8E0, D_0047F900, D_0047F920, NULL,
    D_0047F940, D_0047F960, D_0047F980, NULL,
    D_0047F9A0, D_0047F9C0, D_0047F9E0, NULL,
    D_0047FA00, D_0047FA20, D_0047FA40, NULL,
    D_0047FA60, D_0047FA80, D_0047FAA0, NULL,
    D_0047F480, D_0047F4A0, D_0047FAC0, NULL,
    D_0047FAE0, D_0047FB00, D_0047FB20, NULL,
    D_0047C760, D_0047FB40, D_0047F560, NULL,
    D_0047C760, D_0047FB40, D_0047F560, NULL,
    D_0047FA60, D_0047FA80, D_0047FAA0, NULL,
    D_0047FB60, D_0047FB80, D_0047FBA0, NULL,
    D_0047FBC0, D_0047FBE0, D_0047FC00, NULL,
    D_0047FC20, D_0047FC40, D_0047FC60, NULL,
    D_0047FC80, D_0047FCA0, D_0047FCC0, NULL,
    D_0047FCE0, D_0047FD00, D_0047FD20, NULL,
    D_0047FD40, D_0047FD60, D_0047FD80, NULL,
    D_0047FDA0, D_0047FDC0, D_0047FDE0, NULL,
    D_0047FE00, D_0047FE20, D_0047FE40, NULL,
    D_0047C2C0, D_0047FE60, D_0047FE80, NULL,
    D_0047FEA0, D_0047FEC0, D_0047FEE0, NULL,
    D_0047FF00, D_0047FF20, D_0047FF40, NULL,
    D_0047FF60, D_0047FF80, D_0047FFA0, NULL,
    D_0047FF60, D_0047FF80, D_0047FFA0, NULL,
    D_0047FFC0, D_0047FFE0, D_00480000, NULL,
    D_0047FFC0, D_0047FFE0, D_00480000, NULL,
    D_00480020, D_00480040, D_00480060, NULL,
    D_00480080, D_004800A0, D_004800C0, NULL,
    D_0047F480, D_004800E0, D_00480100, NULL,
    D_0047F4E0, D_00480120, D_00480140, NULL,
    D_0047D510, D_00480160, D_0047F560, NULL,
    D_0047D510, D_00480160, D_0047F560, NULL,
    D_00480180, D_004801A0, D_004801C0, NULL,
    D_004801E0, D_00480200, D_00480220, NULL,
    D_00480240, D_00480260, D_00480280, NULL,
    D_00480080, D_004800A0, D_004800C0, NULL,
    D_004802A0, D_004802C0, D_004802E0, NULL,
    D_004802A0, D_004802C0, D_004802E0, NULL,
    D_0047E560, D_0047E580, D_0047E5A0, NULL,
    D_00480300, D_00480320, D_00480340, NULL,
    D_00480360, D_00480380, D_004803A0, NULL,
    D_004803C0, D_004803E0, D_00480400, NULL,
    D_00480420, D_00480440, D_00480460, NULL,
    D_00480480, D_004804A0, D_004804C0, NULL,
    D_004804E0, D_00480500, D_00480520, NULL,
    D_00480540, D_00480560, D_00480580, NULL,
    D_004805A0, D_004805C0, D_004805E0, NULL,
    D_00480600, D_00480620, D_00480640, NULL,
    D_00480660, D_00480680, D_004806A0, NULL,
    D_00480660, D_00480680, D_004806A0, NULL,
    D_004806C0, D_004806E0, D_00480700, NULL,
    D_0047F480, D_0047F4A0, D_00480720, NULL,
    D_0047F4E0, D_00480740, D_00480760, NULL,
    D_0047E670, D_0047E690, D_0047E6B0, NULL,
    D_0047D830, D_0047D850, D_0047D870, NULL,
    D_00480780, D_004807A0, D_004807C0, NULL,
    D_0047EB40, D_0047EB60, D_0047EB80, NULL,
    D_004807E0, D_00480800, D_00480820, NULL,
    D_00480840, D_00480860, D_00480880, NULL,
    D_004808A0, D_004808C0, D_004808E0, NULL,
    D_00480900, D_0047F540, D_0047BAE0, NULL,
    D_00480900, D_0047F540, D_0047BAE0, NULL,
    D_004805A0, D_004805C0, D_004805E0, NULL,
    D_00480920, D_00480940, D_00480960, NULL,
    D_0047F3A0, D_0047F360, D_0047F380, NULL,
    D_0047F340, D_0047F360, D_0047F380, NULL,
    D_0047EE30, D_0047EE50, D_0047EE70, NULL,
    D_0047C080, D_0047C0A0, D_0047C0C0, NULL,
    D_00480980, D_004809A0, D_004809C0, NULL,
    D_004809E0, D_00480A00, D_00480A20, NULL,
    D_00480A40, D_00480A60, D_00480A80, NULL,
    D_00480A40, D_00480A60, D_00480A80, NULL,
    D_00480AA0, D_00480AC0, D_00480AE0, NULL,
    D_00480B00, D_00480B20, D_00480B40, NULL,
    D_0047E6D0, D_0047E6F0, D_0047E710, NULL,
    D_00480B60, D_00480B80, D_00480BA0, NULL,
    D_00480BC0, D_00480BE0, D_00480C00, NULL,
    D_00480C20, D_00480C40, D_00480C60, NULL,
    D_00480C80, D_00480CA0, D_00480CC0, NULL,
    D_00480CE0, D_00480D00, D_00480D20, NULL,
    D_0047F580, D_0047F5A0, D_0047F5C0, NULL,
    D_0047B5B0, D_0047B5D0, D_0047B5F0, NULL,
    D_0047F0B0, D_0047F0D0, D_0047F0F0, NULL,
    D_0047EEF0, D_0047EF10, D_0047EF30, NULL,
    D_0047EF50, D_0047EF70, D_0047EF90, NULL,
    D_0047F110, D_0047F130, D_0047F150, NULL,
    D_0047BB80, D_0047BBA0, D_0047BBC0, NULL,
    D_0047B4F0, D_0047B510, D_0047B530, NULL,
    D_004809E0, D_00480A00, D_00480A20, NULL,
    D_00480A40, D_00480A60, D_00480A80, NULL,
    D_0047F640, D_0047F660, D_0047F680, NULL,
    D_0047F6A0, D_0047F6C0, D_0047F6E0, NULL,
    D_0047F700, D_0047F720, D_0047F740, NULL,
    D_0047F940, D_0047F960, D_0047F980, NULL,
    D_0047F880, D_0047F8A0, D_0047F8C0, NULL,
    D_0047FA60, D_0047FA80, D_0047FAA0, NULL,
    D_0047FBC0, D_0047FBE0, D_0047FC00, NULL,
    D_0047FC20, D_0047FC40, D_0047FC60, NULL,
    D_0047FC80, D_0047FCA0, D_0047FCC0, NULL,
    D_0047FCE0, D_0047FD00, D_0047FD20, NULL,
    D_0047CA50, D_0047CA70, D_00480D40, NULL,
    D_0047D030, D_0047D050, D_0047D070, NULL,
    D_0047CFD0, D_0047CFF0, D_0047D010, NULL,
    D_0047CE30, D_0047CE50, D_00480D70, NULL,
    D_0047CD30, D_0047CD50, D_00480DA0, NULL,
    D_0047D150, D_0047D170, D_00480DC0, NULL,
    D_0047CF00, D_0047CF20, D_00480DF0, NULL,
    D_0047CC50, D_0047CC70, D_0047CC90, NULL,
    D_0047D1C0, D_0047D1E0, D_0047D210, NULL,
    D_0047CB80, D_0047CBA0, D_0047CBC0, NULL,
    D_0047B920, D_0047B940, D_0047B960, NULL,
    D_00480B60, D_00480B80, D_00480BA0, NULL,
    D_0047F3C0, D_0047F3E0, D_0047F400, NULL,
    D_0047B9E0, D_0047BA00, D_0047BA20, NULL,
    D_0047C2C0, D_0047EFB0, D_0047EFD0, NULL,
    D_00480B00, D_00480B20, D_00480B40, NULL,
    D_0047B740, D_0047B760, D_0047B780, NULL,
    D_0047BCA0, D_0047BCC0, D_0047BCE0, NULL,
    D_0047EFF0, D_0047F010, D_0047F030, NULL,
    D_00480BC0, D_00480BE0, D_00480C00, NULL,
    D_00480C80, D_00480CA0, D_00480CC0, NULL,
    D_0047B1B0, D_0047B1D0, D_0047B1F0, NULL,
    D_0047BE40, D_0047BE60, D_0047BE80, NULL,
    D_0047C260, D_0047C280, D_0047C2A0, NULL,
    D_0047C640, D_0047C660, D_0047C680, NULL,
    D_0047C200, D_0047C220, D_0047C240, NULL,
    D_0047C2C0, D_0047C2E0, D_0047C300, NULL,
    D_0047E380, D_0047E3A0, D_0047E3C0, NULL,
    D_0047BEA0, D_0047BEC0, D_0047BEE0, NULL,
    D_0047C3E0, D_0047C400, D_0047C420, NULL,
    D_0047C520, D_0047C540, D_0047C560, NULL,
    D_0047C7A0, D_0047C7C0, D_0047C7E0, NULL,
    D_00480E20, D_00480E40, D_00480E60, NULL,
    D_00480E80, D_00480EA0, D_00480EC0, NULL,
    D_00480EE0, D_00480F00, D_00480F20, NULL,
    D_0047C140, D_0047C160, D_0047C180, NULL,
    D_00480F40, D_00480F60, D_00480F80, NULL,
    D_0047D2F0, D_0047D310, D_0047D330, NULL,
    D_0047FF60, D_0047FF80, D_0047FFA0, NULL,
    D_0047D410, D_0047D430, D_0047D450, NULL,
    D_0047C2C0, D_0047FE60, D_0047FE80, NULL,
    D_0047C990, D_0047C9B0, D_0047C9D0, NULL,
    D_00480020, D_00480040, D_00480060, NULL,
    D_00480180, D_004801A0, D_004801C0, NULL,
    D_00480240, D_00480260, D_00480280, NULL,
    D_0047D290, D_0047D2B0, D_0047D2D0, NULL,
    D_0047E210, D_0047E230, D_0047E250, NULL,
    D_0047DA10, D_0047DA30, D_0047DA50, NULL,
    D_0047DAD0, D_0047DAF0, D_0047DB10, NULL,
    D_0047DDD0, D_0047DDF0, D_0047DE10, NULL,
    D_0047DE90, D_0047DEB0, D_0047DED0, NULL,
    D_0047DFD0, D_0047DFF0, D_0047E010, NULL,
    D_0047C2C0, D_0047E110, D_0047E130, NULL,
    D_0047E4A0, D_0047E4C0, D_0047E4E0, NULL,
    D_0047E9C0, D_0047E9E0, D_0047EA00, NULL,
    D_004802A0, D_004802C0, D_004802E0, NULL,
    D_0047C2C0, D_0047EAE0, D_0047EB00, NULL,
    D_004803C0, D_004803E0, D_00480400, NULL,
    D_004804E0, D_00480500, D_00480520, NULL,
    D_00480540, D_00480560, D_00480580, NULL,
    D_00480FA0, D_00480FC0, D_00480FE0, NULL,
    D_0047E900, D_0047E920, D_0047E940, NULL,
    D_00481000, D_00481020, D_00481040, NULL,
    D_00480C80, D_00480CA0, D_00480CC0, NULL,
    D_0047F0B0, D_0047F0D0, D_0047F0F0, NULL,
    D_0047CC50, D_0047CC70, D_00481060, NULL,
    D_0047D8F0, D_0047D910, D_0047D930, NULL,
    D_004806C0, D_004806E0, D_00480700, NULL,
    D_0047FC80, D_0047FCA0, D_0047FCC0, NULL,
    D_004808C0, D_00481080, D_004810A0, NULL,
    D_004810C0, D_004810E0, D_00481100, NULL,
    D_0047D5B0, D_0047D5D0, D_00481120, NULL,
    D_00481140, D_00481160, D_00481180, NULL,
    D_0047D5B0, D_0047D5D0, D_00481120, NULL,
    D_004811A0, D_004811C0, D_004811E0, NULL,
    D_0047C320, D_0047C340, D_0047C360, NULL,
    D_0047B5B0, D_0047B5D0, D_0047B5F0, NULL,
    D_0047BB60, gHelpTexts_1_8, gHelpTexts_1_8, NULL,
    D_0047DDD0, D_0047DDF0, D_0047DE10, NULL,
    D_004803C0, D_004803E0, D_00480400, NULL,
    D_004811A0, D_004811C0, D_004811E0, NULL,
    D_0047FF60, D_0047FF80, D_0047FFA0, NULL,
    D_00480CE0, D_00481200, D_00481220, NULL,
    D_00481240, D_0047D0B0, D_0047D0D0, NULL,
};
char *gGunDescTexts[128][4] = {
    D_00481260, NULL, NULL, NULL,
    D_00481270, D_00481290, D_004812B0, NULL,
    D_004812D0, D_004812F0, D_00481320, NULL,
    D_00481340, D_00481360, D_00481380, NULL,
    D_004813A0, D_004813C0, D_004813E0, NULL,
    D_004813A0, D_00481400, D_00481420, NULL,
    D_00481440, D_00481460, D_00481480, NULL,
    D_004814A0, D_004814C0, D_004814E0, NULL,
    D_004814F0, D_004814C0, D_00481510, NULL,
    D_00481520, D_00481540, D_00481560, NULL,
    D_00481580, D_004815A0, D_004815C0, NULL,
    D_004815E0, D_00481600, D_00481620, NULL,
    D_00481650, D_00481670, D_00481690, NULL,
    D_004816A0, D_004816C0, D_004816E0, NULL,
    D_00481700, D_00481720, D_00481740, NULL,
    D_00481750, D_00481770, D_00481790, NULL,
    D_004817B0, D_004817D0, D_004817F0, NULL,
    D_00481810, D_00481830, D_00481850, NULL,
    D_00481870, D_00481890, D_004818B0, NULL,
    D_004818D0, D_004818F0, D_00481910, NULL,
    D_0047C2C0, D_00481930, D_00481950, NULL,
    D_00481970, D_00481990, D_004819B0, NULL,
    D_004819D0, D_004819F0, D_00481A10, NULL,
    D_00481A30, D_00481A50, D_00481690, NULL,
    D_00481A70, D_00481A90, D_00481AB0, NULL,
    D_00481AD0, D_00481B00, D_00481B20, NULL,
    D_00481B40, D_00481B60, D_00481B80, NULL,
    D_00481BA0, D_00481BC0, D_00481BE0, NULL,
    D_00481C00, D_00481C20, D_00481C40, NULL,
    D_00481C60, D_00481C90, D_00481CB0, NULL,
    D_00481CD0, D_00481CF0, D_00481D10, NULL,
    D_00481D30, D_00481D50, D_00481D70, NULL,
    D_00481DA0, D_00481DD0, D_00481DF0, NULL,
    D_00481E10, D_00481C90, D_00481CB0, NULL,
    D_00481CD0, D_00481E40, D_00481E60, NULL,
    D_00481D30, D_00481E80, D_00481EA0, NULL,
    D_00481ED0, D_00481F00, D_00481F20, NULL,
    D_00481F40, D_00481F60, D_00481F80, NULL,
    D_00481FA0, D_00481FC0, D_00481FF0, NULL,
    D_00482020, D_00482040, D_00482060, NULL,
    D_00482090, D_004820C0, D_004820F0, NULL,
    D_00482110, D_00482130, D_00482160, NULL,
    D_00482190, D_004821B0, D_004821D0, NULL,
    D_004821F0, D_00482210, D_00482230, NULL,
    D_00482250, D_00482270, D_00482290, NULL,
    D_004822B0, D_004822D0, D_00482300, NULL,
    D_00482320, D_00482340, D_00482360, NULL,
    D_00482380, D_004823A0, D_004823D0, NULL,
    D_004823F0, D_00482410, D_00482430, NULL,
    D_00482450, D_00482470, D_004824A0, NULL,
    D_004824C0, D_004824E0, D_00482500, NULL,
    D_00482530, D_00482550, D_00482570, NULL,
    D_00482590, D_004825B0, D_004825D0, NULL,
    D_00482600, D_00482620, D_00482640, NULL,
    D_00482660, D_00482680, D_004826A0, NULL,
    D_0047F480, D_0047F4A0, D_004826C0, NULL,
    D_0047FAE0, D_004826F0, D_00482710, NULL,
    D_00482730, D_00482750, D_00482770, NULL,
    D_00482790, D_004827B0, D_004827D0, NULL,
    D_004827F0, D_00482810, D_004827D0, NULL,
    D_00482830, D_00482850, D_00482870, NULL,
    D_004828A0, D_004828C0, D_004828E0, NULL,
    D_00482900, D_00482920, D_00482940, NULL,
    D_00482960, D_00482980, D_004829A0, NULL,
    D_004829C0, D_004829E0, D_00482A00, NULL,
    D_00482A20, D_00482A40, D_00482A60, NULL,
    D_00482A80, D_00482AA0, D_00482AC0, NULL,
    D_00482AE0, D_00482B00, D_00482B20, NULL,
    D_00482B40, D_00482B60, D_00482B80, NULL,
    D_00482BA0, D_00482BC0, D_00482BE0, NULL,
    D_00482C00, D_00482C20, D_00482C40, NULL,
    D_00482C60, D_00482C80, D_00482CA0, NULL,
    D_00482CC0, D_00482CE0, D_00482D00, NULL,
    D_00482D20, D_00482D40, D_00482D60, NULL,
    D_00482D80, D_00482DA0, D_00482DC0, NULL,
    D_00482DE0, D_00482E00, D_00482E20, NULL,
    D_00482E40, D_00481B60, D_00482E60, NULL,
    D_00482E80, D_00482EA0, D_00482ED0, NULL,
    D_00482EF0, D_00482F10, D_00482F30, NULL,
    D_00482F50, D_00482F70, D_00482F90, NULL,
    D_00482FB0, D_00482FD0, D_00482FF0, NULL,
    D_00483010, D_00483030, D_00483050, NULL,
    D_00483070, D_00483090, D_004830B0, NULL,
    D_004830D0, D_004830F0, D_00483110, NULL,
    D_00483130, D_00483150, D_00483170, NULL,
    D_00483190, D_004831B0, D_004831D0, NULL,
    D_004831F0, D_00483210, D_00483230, NULL,
    D_00483250, D_00483270, D_00483290, NULL,
    D_004832B0, D_004832D0, D_004832F0, NULL,
    D_00483310, D_00483330, D_00483350, NULL,
    D_00483370, D_00483390, D_004833B0, NULL,
    D_004833D0, D_004833F0, D_00483410, NULL,
    D_0047C2C0, D_00483430, D_00483450, NULL,
    D_00483470, D_00483490, D_004834C0, NULL,
    D_004834E0, D_00483500, D_00483530, NULL,
    D_00483550, D_00483570, D_00483590, NULL,
    D_004835C0, D_004835E0, D_00483600, NULL,
    D_00483630, D_00483650, D_00483680, NULL,
    D_004836A0, D_004836C0, D_004836E0, NULL,
    D_00483700, D_00483720, D_00483740, NULL,
    D_00483760, D_00483780, D_004837A0, NULL,
    D_004837C0, D_004837E0, D_00483800, NULL,
    D_00483820, D_00483840, D_00483860, NULL,
    D_00483880, D_004838B0, D_004838E0, NULL,
    D_00483900, D_00483920, D_00483950, NULL,
    D_00483970, D_00483990, D_004839B0, NULL,
    D_004839D0, D_004839F0, D_00483A10, NULL,
    D_00483A30, D_00483A50, D_00483A70, NULL,
    D_00482EF0, D_00482F10, D_00482F30, NULL,
    D_00482EF0, D_00482F10, D_00482F30, NULL,
    D_00482F50, D_00482F70, D_00482F90, NULL,
    D_00482F50, D_00482F70, D_00482F90, NULL,
    D_00482FB0, D_00482FD0, D_00482FF0, NULL,
    D_00483010, D_00483030, D_00483050, NULL,
    D_00483070, D_00483090, D_004830B0, NULL,
    D_004830D0, D_004830F0, D_00483110, NULL,
    D_00483130, D_00483150, D_00483170, NULL,
    D_00483130, D_00483150, D_00483170, NULL,
    D_00483190, D_004831B0, D_004831D0, NULL,
    D_004831F0, D_00483210, D_00483230, NULL,
    D_00483250, D_00483270, D_00483290, NULL,
    D_00483A30, D_00483A50, D_00483A70, NULL,
    D_00483A90, D_00483AB0, D_00483AD0, NULL,
    D_00483AF0, D_00483B10, D_00483B30, NULL,
    D_00483B50, D_00483B70, D_00483B90, NULL,
    D_00483BB0, D_00483BD0, D_00483BF0, NULL,
    D_00483C10, D_00483C30, D_00483C50, NULL,
    D_00483C70, D_00483C90, D_00483CB0, NULL,
};
char *gArmorDescTexts[1207][4] = {
    D_0047B0D0, NULL, NULL, NULL,
    D_00483CD0, D_00483CF0, D_00483D08, NULL,
    D_00483D20, D_00483D40, D_00483D60, NULL,
    D_00483D80, D_00483DA0, D_00483DC0, NULL,
    D_00483D80, D_00483DE0, D_00483E00, NULL,
    D_00483E20, D_00483E40, D_00483E60, NULL,
    D_00483E80, D_00483EA0, D_00483EC0, NULL,
    D_00483EE0, D_00483F00, D_00483F10, NULL,
    D_00483F30, D_00483F50, D_00483F70, NULL,
    D_00483D80, D_00483F90, D_00483FB0, NULL,
    D_00483D80, D_00483FD0, D_00483FF0, NULL,
    D_00484010, D_00484030, D_00484050, NULL,
    D_00484010, D_00484070, D_00484090, NULL,
    D_004840B0, D_004840D0, D_004840F0, NULL,
    D_004840B0, D_00484110, D_00484130, NULL,
    D_00484150, D_00484170, D_00484190, NULL,
    D_004841B0, D_004841D0, D_004841F0, NULL,
    D_00484210, D_00483E40, D_00484230, NULL,
    D_00484250, D_00484270, D_00484290, NULL,
    D_004842B0, D_004842D0, D_004842F0, NULL,
    D_00484310, D_00484330, D_00484350, NULL,
    D_00484370, D_00484390, D_004843B0, NULL,
    D_00484370, D_004843D0, D_004843F0, NULL,
    D_00484410, D_00484430, D_00484450, NULL,
    D_00484410, D_00484470, D_00484490, NULL,
    D_004844B0, D_004844D0, D_004844F0, NULL,
    D_00484510, D_00484530, D_00484550, NULL,
    D_00484570, D_00484590, NULL, NULL,
    D_00484570, D_004845A0, NULL, NULL,
    D_004845C0, D_004845E0, D_00484600, NULL,
    D_00484620, D_00484640, D_00484660, NULL,
    D_00484680, D_004846A0, D_004846C0, NULL,
    D_004846E0, D_00484700, D_00484720, NULL,
    D_00484740, D_00484760, D_00484780, NULL,
    D_004847A0, D_004847C0, D_004847E0, NULL,
    D_004847A0, D_00484800, D_00484820, NULL,
    D_00484840, D_00484860, D_00484880, NULL,
    D_004848A0, D_004848C0, D_004848E0, NULL,
    D_00484900, D_00484920, D_00484940, NULL,
    D_00484960, D_00484980, D_004849A0, NULL,
    D_004849B0, D_004849D0, D_004849F0, NULL,
    D_004849B0, D_00484A10, D_00484A30, NULL,
    D_00484A50, D_00484A70, D_00484A90, NULL,
    D_00484AB0, D_00484AD0, NULL, NULL,
    D_00484AE0, D_00484B00, D_00484B20, NULL,
    D_00484B30, D_00484B50, D_00484B70, NULL,
    D_00484B90, D_00484BB0, D_00484BD0, NULL,
    D_00484BF0, D_00484C10, D_00484C30, NULL,
    D_00484C50, D_00484C70, D_00484C90, NULL,
    D_00484CB0, D_00484CD0, D_00484CF0, NULL,
    D_00484D10, D_00484D30, D_00484D40, NULL,
    D_00484D60, D_00484D30, D_00484D80, NULL,
    D_00484DA0, D_00484DC0, D_00484DE0, NULL,
    D_00484E00, D_00484E20, D_00484E40, NULL,
    D_00484E00, D_00484E60, D_00484E80, NULL,
    D_00484E00, D_00484EA0, D_00484EC0, NULL,
    D_00484E00, D_00484EE0, D_00484F00, NULL,
    D_00484F20, D_00484F40, D_00484F60, NULL,
    D_00484F20, D_00484F40, D_00484F60, NULL,
    D_00484F80, D_00484FA0, D_00484FC0, NULL,
    D_00484F80, D_00484FE0, D_00485000, NULL,
    D_00485020, D_00485040, D_00485060, NULL,
    D_00485080, D_004850A0, D_004850C0, NULL,
    D_004850E0, D_00485100, D_00485120, NULL,
    D_00485140, D_00485160, D_00485180, NULL,
    D_004851A0, D_004851C0, D_004851E0, NULL,
    D_00485200, D_00485220, D_00485240, NULL,
    D_00485260, D_00485280, D_004852A0, NULL,
    D_00485260, D_004852C0, D_004852E0, NULL,
    D_00485300, D_00485320, D_00485340, NULL,
    D_00485300, D_00485360, D_00485380, NULL,
    D_004853A0, D_004853C0, D_004853E0, NULL,
    D_00485400, D_00485420, D_00485440, NULL,
    D_00483CD0, D_00483CF0, D_00483D08, NULL,
    D_00483D20, D_00483D40, D_00483D60, NULL,
    D_00483D80, D_00483DA0, D_00483DC0, NULL,
    D_00483D80, D_00483DE0, D_00483E00, NULL,
    D_00483E20, D_00483E40, D_00483E60, NULL,
    D_00483E80, D_00483EA0, D_00483EC0, NULL,
    D_00483EE0, D_00483F00, D_00483F10, NULL,
    D_00483F30, D_00483F50, D_00483F70, NULL,
    D_00483D80, D_00483F90, D_00483FB0, NULL,
    D_00483D80, D_00483FD0, D_00483FF0, NULL,
    D_00484010, D_00484030, D_00484050, NULL,
    D_00484010, D_00484070, D_00484090, NULL,
    D_004840B0, D_004840D0, D_004840F0, NULL,
    D_004840B0, D_00484110, D_00484130, NULL,
    D_00484150, D_00484170, D_00484190, NULL,
    D_004841B0, D_004841D0, D_004841F0, NULL,
    D_00484210, D_00483E40, D_00484230, NULL,
    D_00484250, D_00484270, D_00484290, NULL,
    D_004842B0, D_004842D0, D_004842F0, NULL,
    D_00485460, D_00484330, D_00484350, NULL,
    D_00484370, D_00484390, D_004843B0, NULL,
    D_00484370, D_004843D0, D_004843F0, NULL,
    D_00484410, D_00484430, D_00484450, NULL,
    D_00484410, D_00484470, D_00484490, NULL,
    D_00485480, D_004854A0, D_004854C0, NULL,
    D_004854E0, D_00485500, D_00485520, NULL,
    D_00484570, D_00484590, NULL, NULL,
    D_00484570, D_004845A0, NULL, NULL,
    D_00485540, D_00485560, D_00485580, NULL,
    D_004855A0, D_004855C0, D_004855E0, NULL,
    D_00484680, D_004846A0, D_004846C0, NULL,
    D_004846E0, D_00484700, D_00484720, NULL,
    D_004847A0, D_004847C0, D_004847E0, NULL,
    D_004847A0, D_00484800, D_00484820, NULL,
    D_00484840, D_00484860, D_00484880, NULL,
    D_004848A0, D_004848C0, D_004848E0, NULL,
    D_00484900, D_00484920, D_00484940, NULL,
    D_00484960, D_00484980, D_004849A0, NULL,
    D_004849B0, D_004849D0, D_004849F0, NULL,
    D_004849B0, D_00484A10, D_00484A30, NULL,
    D_00485600, D_00485620, D_00485640, NULL,
    D_00485660, D_00485680, D_004856A0, NULL,
    D_004856B0, D_004856D0, D_004856F0, NULL,
    D_00485710, D_00485730, D_00485750, NULL,
    D_00485770, D_00485790, D_004857A8, NULL,
    D_004857C0, D_004857E0, D_00485800, NULL,
    D_00485260, D_00485280, D_004852A0, NULL,
    D_00485260, D_004852C0, D_004852E0, NULL,
    D_00485300, D_00485320, D_00485340, NULL,
    D_00485300, D_00485360, D_00485380, NULL,
    D_00485820, D_00485840, D_00485860, NULL,
    D_00485880, D_004858A0, D_004858C0, NULL,
    D_004858E0, D_00485900, D_00485920, NULL,
    D_00485940, D_00485960, D_00485980, NULL,
    D_004859A0, D_004859C0, D_004859E0, NULL,
    D_00485A00, D_00485A20, D_00485A40, NULL,
    D_00483CD0, D_00483CF0, D_00483D08, NULL,
    D_00483D20, D_00483D40, D_00483D60, NULL,
    D_00483D80, D_00483DA0, D_00483DC0, NULL,
    D_00483D80, D_00483DE0, D_00483E00, NULL,
    D_00483E20, D_00483E40, D_00483E60, NULL,
    D_00483E80, D_00483EA0, D_00483EC0, NULL,
    D_00483EE0, D_00483F00, D_00483F10, NULL,
    D_00483F30, D_00483F50, D_00483F70, NULL,
    D_00483D80, D_00483F90, D_00483FB0, NULL,
    D_00483D80, D_00483FD0, D_00483FF0, NULL,
    D_00484010, D_00484030, D_00484050, NULL,
    D_00484010, D_00484070, D_00484090, NULL,
    D_004840B0, D_004840D0, D_004840F0, NULL,
    D_004840B0, D_00484110, D_00484130, NULL,
    D_00484150, D_00484170, D_00484190, NULL,
    D_004841B0, D_004841D0, D_004841F0, NULL,
    D_00484210, D_00483E40, D_00484230, NULL,
    D_00484250, D_00484270, D_00484290, NULL,
    D_00485A60, D_00485A80, D_00485AA0, NULL,
    D_00485AC0, D_00485AE0, D_00485B00, NULL,
    D_004842B0, D_004842D0, D_004842F0, NULL,
    D_00484310, D_00484330, D_00484350, NULL,
    D_00484370, D_00484390, D_004843B0, NULL,
    D_00484370, D_004843D0, D_004843F0, NULL,
    D_00484410, D_00484430, D_00484450, NULL,
    D_00484410, D_00484470, D_00484490, NULL,
    D_004844B0, D_004844D0, D_004844F0, NULL,
    D_00484510, D_00484530, D_00484550, NULL,
    D_00484570, D_00485B20, D_00485B40, NULL,
    D_00484570, D_00485B60, D_00485B80, NULL,
    D_004845C0, D_004845E0, D_00484600, NULL,
    D_00484620, D_00484640, D_00484660, NULL,
    D_00484680, D_004846A0, D_004846C0, NULL,
    D_004846E0, D_00484700, D_00484720, NULL,
    D_004847A0, D_004847C0, D_004847E0, NULL,
    D_004847A0, D_00484800, D_00484820, NULL,
    D_00484840, D_00484860, D_00484880, NULL,
    D_004848A0, D_004848C0, D_004848E0, NULL,
    D_00484900, D_00484920, D_00484940, NULL,
    D_00484960, D_00484980, D_004849A0, NULL,
    D_004849B0, D_004849D0, D_004849F0, NULL,
    D_004849B0, D_00484A10, D_00484A30, NULL,
    D_00485BA0, D_00485BC0, D_00485BE0, NULL,
    D_00485BA0, D_00485BC0, D_00485BE0, NULL,
    D_00485C00, D_00485C20, D_00485C40, NULL,
    D_00485C60, D_00485C80, D_00485CA0, NULL,
    D_00485CC0, D_00485CE0, D_00485D00, NULL,
    D_00485D20, D_00485D40, D_00485D60, NULL,
    D_00485260, D_00485280, D_004852A0, NULL,
    D_00485260, D_004852C0, D_004852E0, NULL,
    D_00485300, D_00485320, D_00485340, NULL,
    D_00485300, D_00485360, D_00485380, NULL,
    D_004853A0, D_004853C0, D_004853E0, NULL,
    D_00485400, D_00485420, D_00485440, NULL,
    D_00485D80, D_00485DA0, D_00485DC0, NULL,
    D_00485DE0, D_00485E00, D_00485E20, NULL,
    D_00485D80, D_00485DA0, D_00485DC0, NULL,
    D_00485DE0, D_00485E00, D_00485E20, NULL,
    D_00485D80, D_00485DA0, D_00485DC0, NULL,
    D_00485DE0, D_00485E00, D_00485E20, NULL,
    D_00485E40, D_00485E60, D_00485E80, NULL,
    D_00485E90, D_00485EB0, D_00485ED0, NULL,
    D_00485E40, D_00485E60, D_00485E80, NULL,
    D_00485E90, D_00485EB0, D_00485ED0, NULL,
    D_00485E40, D_00485E60, D_00485E80, NULL,
    D_00485E90, D_00485EB0, D_00485ED0, NULL,
    D_00485EF0, D_00485F10, D_00485F30, NULL,
    D_00485EF0, D_00485F50, D_00485F70, NULL,
    D_00485EF0, D_00485F10, D_00485F30, NULL,
    D_00485EF0, D_00485F50, D_00485F70, NULL,
    D_00485EF0, D_00485F10, D_00485F30, NULL,
    D_00485EF0, D_00485F50, D_00485F70, NULL,
    D_00485F90, D_00485FB0, D_00485FD0, NULL,
    D_00485FF0, D_00486010, D_00486030, NULL,
    D_00485F90, D_00485FB0, D_00485FD0, NULL,
    D_00485FF0, D_00486010, D_00486030, NULL,
    D_00485F90, D_00485FB0, D_00485FD0, NULL,
    D_00485FF0, D_00486010, D_00486030, NULL,
    D_00486050, D_00486070, D_00486090, NULL,
    D_004860B0, D_004860D0, D_004860F0, NULL,
    D_00486050, D_00486070, D_00486090, NULL,
    D_004860B0, D_004860D0, D_004860F0, NULL,
    D_00486110, D_00486130, D_00486150, NULL,
    D_00486170, D_00486190, D_004861B0, NULL,
    D_004861D0, D_004861F0, D_00486210, NULL,
    D_00486220, D_00486240, D_00486260, NULL,
    D_00486280, D_004862A0, D_004862C0, NULL,
    D_004862E0, D_00486300, D_00486320, NULL,
    D_00486340, D_00486360, D_00486380, NULL,
    D_00486390, D_004863B0, D_004863D0, NULL,
    D_004863F0, D_00486410, D_00486430, NULL,
    D_00486450, D_00486470, D_00486490, NULL,
    D_004864B0, D_004864D0, D_004864F0, NULL,
    D_00486510, D_00486530, D_00486550, NULL,
    D_00486570, D_00486590, D_004865B0, NULL,
    D_004865D0, D_004865F0, D_00486610, NULL,
    D_00486630, D_00486650, D_00486670, NULL,
    D_00486630, D_00486650, D_00486670, NULL,
    D_00484A50, D_00484A70, D_00484A90, NULL,
    D_00484CB0, D_00484CD0, D_00484CF0, NULL,
    D_00486680, D_004866A0, D_004866C0, NULL,
    D_00485CE0, D_00485D00, D_004866D0, NULL,
    D_004866F0, D_00486710, D_00486730, NULL,
    D_00486750, D_00486770, D_00486790, NULL,
    D_004867B0, D_004867D0, D_004867F0, NULL,
    D_00484E00, D_00486810, D_00486830, NULL,
    D_00485C00, D_00485C20, D_00485C40, NULL,
    D_00485C60, D_00485C80, D_00485CA0, NULL,
    D_00485CC0, D_00485CE0, D_00485D00, NULL,
    D_00485D20, D_00485D40, D_00485D60, NULL,
    D_00486850, D_00486870, D_00486890, NULL,
    D_004868A0, D_004868C0, D_004868E0, NULL,
    D_00486900, D_00486920, D_00486940, NULL,
    D_00486960, D_00486980, D_004869A0, NULL,
    D_004869C0, D_004869E0, D_00486A00, NULL,
    D_00486A20, D_00486A40, D_00486A60, NULL,
    D_00486A80, D_00486AA0, D_00486AC0, NULL,
    D_00486AE0, D_00486B00, D_00486B20, NULL,
    D_00486B40, D_00486B60, D_00486B80, NULL,
    D_00486BA0, D_00486BC0, D_00486BE0, NULL,
    D_00486C00, D_00486C20, D_00486C40, NULL,
    D_00486C60, D_00486C80, D_00486CA0, NULL,
    D_00486CC0, D_00486CE0, D_00486D00, NULL,
    D_00486D20, D_00486D40, D_00486D60, NULL,
    D_0047B0D0, NULL, NULL, NULL,
    D_00486D80, D_00486DA0, D_00486DC0, NULL,
    D_00486DD0, D_00486DF0, D_00486E10, NULL,
    D_00486E30, D_00486E50, D_00486E70, NULL,
    D_00486E30, D_00486E90, D_00486EB0, NULL,
    D_00486ED0, D_00486EF0, D_00486F10, NULL,
    D_00486F30, D_00486F50, D_00486F70, NULL,
    D_00486F90, D_00486FB0, D_00486FD0, NULL,
    D_00486FF0, D_00487010, D_00487030, NULL,
    D_00486E30, D_00487040, D_00487060, NULL,
    D_00486E30, D_00487080, D_004870A0, NULL,
    D_00484010, D_004870C0, D_004870E0, NULL,
    D_00484010, D_00487100, D_00487120, NULL,
    D_00487140, D_00487160, D_00487180, NULL,
    D_00487140, D_00487190, D_004871B0, NULL,
    D_004871D0, D_004871F0, D_00487210, NULL,
    D_004841B0, D_00487228, D_00487240, NULL,
    D_00487260, D_00487280, D_004872A0, NULL,
    D_004872C0, D_004872E0, D_00487300, NULL,
    D_00487310, D_00487330, D_00487350, NULL,
    D_00487370, D_00487390, D_004873B0, NULL,
    D_004873D0, D_004873F0, D_00487410, NULL,
    D_00487430, D_00487450, D_00487470, NULL,
    D_00487490, D_004874B0, D_004874D0, NULL,
    D_00487490, D_004874F0, D_00487510, NULL,
    D_00484410, D_00487520, D_00487540, NULL,
    D_00487560, D_00487580, D_004875A0, NULL,
    D_004875C0, D_004875E0, D_00487600, NULL,
    D_00487620, D_00487640, D_00487660, NULL,
    D_00487620, D_00487680, D_004876A0, NULL,
    D_004876C0, D_004876E0, D_00487700, NULL,
    D_004876C0, D_00487720, D_00487740, NULL,
    D_00487760, D_00487780, D_004877A0, NULL,
    D_004877C0, D_004877E0, D_00487800, NULL,
    D_00487820, D_00487840, D_00487860, NULL,
    D_00487880, D_004878A0, D_004878C0, NULL,
    D_00484740, D_004878E0, D_00487900, NULL,
    D_00487920, D_00487940, D_00487960, NULL,
    D_004847A0, D_00487980, D_004879A0, NULL,
    D_004847A0, D_004879C0, D_004879E0, NULL,
    D_00487A00, D_00487A20, D_00487A40, NULL,
    D_00487A60, D_00487A80, D_00487AA0, NULL,
    D_00487AC0, D_00487AE0, D_00487B00, NULL,
    D_00484960, D_00487B20, D_00487B40, NULL,
    D_004849B0, D_00487B60, D_00487B80, NULL,
    D_00487BA0, D_00487BC0, D_00487BE0, NULL,
    D_00484A50, D_00487C00, D_00484A90, NULL,
    D_00487C20, D_00487C40, D_00487C60, NULL,
    D_00487C70, D_00487C90, D_00487CB0, NULL,
    D_00487CD0, D_00487CF0, D_00487D10, NULL,
    D_00484C50, D_00487D30, D_00487D50, NULL,
    D_00487D70, D_00487228, D_00487D90, NULL,
    D_00487DB0, D_00487DD0, D_00487DF0, NULL,
    D_00487E10, D_00487E30, D_00487E50, NULL,
    D_00487E70, D_00487E90, D_00487EB0, NULL,
    D_00487E70, D_00487E90, D_00487EB0, NULL,
    D_00484F80, D_00487ED0, D_00487EF0, NULL,
    D_00484F80, D_00487ED0, D_00487EF0, NULL,
    D_00487F10, D_00487F30, D_00487F50, NULL,
    D_00487F70, D_00487F90, D_00487FB0, NULL,
    D_00487FC0, D_00487FE0, D_00488000, NULL,
    D_00488020, D_00488040, D_00488060, NULL,
    D_00485260, D_00488080, D_004880A0, NULL,
    D_00485260, D_004880C0, D_004880E0, NULL,
    D_00485300, D_00488100, D_00488120, NULL,
    D_00485300, D_00488140, D_00488160, NULL,
    D_00488180, D_004881A0, D_004881C0, NULL,
    D_004881E0, D_00488200, D_00488220, NULL,
    D_00486D80, D_00486DA0, D_00486DC0, NULL,
    D_00486DD0, D_00486DF0, D_00486E10, NULL,
    D_00486E30, D_00486E50, D_00486E70, NULL,
    D_00486E30, D_00486E90, D_00486EB0, NULL,
    D_00486ED0, D_00486EF0, D_00486F10, NULL,
    D_00486F30, D_00486F50, D_00486F70, NULL,
    D_00486F90, D_00486FB0, D_00486FD0, NULL,
    D_00486FF0, D_00487010, D_00487030, NULL,
    D_00486E30, D_00487040, D_00487060, NULL,
    D_00486E30, D_00487080, D_004870A0, NULL,
    D_00484010, D_004870C0, D_004870E0, NULL,
    D_00484010, D_00487100, D_00487120, NULL,
    D_00487140, D_00487160, D_00487180, NULL,
    D_00487140, D_00487190, D_004871B0, NULL,
    D_004871D0, D_004871F0, D_00487210, NULL,
    D_004841B0, D_00487228, D_00487240, NULL,
    D_00487260, D_00487280, D_004872A0, NULL,
    D_004872C0, D_004872E0, D_00487300, NULL,
    D_00487310, D_00487330, D_00487350, NULL,
    D_00487370, D_00487390, D_004873B0, NULL,
    D_004873D0, D_004873F0, D_00487410, NULL,
    D_00488240, D_00487450, D_00487470, NULL,
    D_00487490, D_004874B0, D_004874D0, NULL,
    D_00487490, D_004874F0, D_00487510, NULL,
    D_00484410, D_00487520, D_00487540, NULL,
    D_00487560, D_00487580, D_004875A0, NULL,
    D_004875C0, D_004875E0, D_00487600, NULL,
    D_00488260, D_00488280, D_004882A0, NULL,
    D_004882C0, D_004882E0, D_00488300, NULL,
    D_004876C0, D_004876E0, D_00487700, NULL,
    D_004876C0, D_00487720, D_00487740, NULL,
    D_00488320, D_00488340, D_00488360, NULL,
    D_00488380, D_004883A0, D_004883C0, NULL,
    D_00487820, D_00487840, D_00487860, NULL,
    D_00487880, D_004878A0, D_004878C0, NULL,
    D_004847A0, D_00487980, D_004879A0, NULL,
    D_004847A0, D_004879C0, D_004879E0, NULL,
    D_00487A00, D_00487A20, D_00487A40, NULL,
    D_00487A60, D_00487A80, D_00487AA0, NULL,
    D_00487AC0, D_00487AE0, D_00487B00, NULL,
    D_00484960, D_00487B20, D_00487B40, NULL,
    D_004849B0, D_00487B60, D_00487B80, NULL,
    D_00487BA0, D_00487BC0, D_00487BE0, NULL,
    D_00485600, D_00485620, D_004883E0, NULL,
    D_00485660, D_00488400, D_00488420, NULL,
    D_00488430, D_00488450, D_00488470, NULL,
    D_004856B0, D_00488490, D_004884B0, NULL,
    D_004884D0, D_004884F0, D_00488510, NULL,
    D_004857C0, D_00488530, D_00488550, NULL,
    D_00485260, D_00488080, D_004880A0, NULL,
    D_00485260, D_004880C0, D_004880E0, NULL,
    D_00485300, D_00488100, D_00488120, NULL,
    D_00485300, D_00488140, D_00488160, NULL,
    D_00488570, D_00488590, D_004885B0, NULL,
    D_004885D0, D_004885F0, D_00488610, NULL,
    D_00488630, D_00488650, D_00488670, NULL,
    D_00488690, D_004886B0, D_004886D0, NULL,
    D_004886F0, D_00488710, D_00488730, NULL,
    D_00488750, D_00488770, D_00488790, NULL,
    D_00486D80, D_00486DA0, D_00486DC0, NULL,
    D_00486DD0, D_00486DF0, D_00486E10, NULL,
    D_00486E30, D_00486E50, D_00486E70, NULL,
    D_00486E30, D_00486E90, D_00486EB0, NULL,
    D_00486ED0, D_00486EF0, D_00486F10, NULL,
    D_00486F30, D_00486F50, D_00486F70, NULL,
    D_00486F90, D_00486FB0, D_00486FD0, NULL,
    D_00486FF0, D_00487010, D_00487030, NULL,
    D_00486E30, D_00487040, D_00487060, NULL,
    D_00486E30, D_00487080, D_004870A0, NULL,
    D_00484010, D_004870C0, D_004870E0, NULL,
    D_00484010, D_00487100, D_00487120, NULL,
    D_00487140, D_00487160, D_00487180, NULL,
    D_00487140, D_00487190, D_004871B0, NULL,
    D_004871D0, D_004871F0, D_00487210, NULL,
    D_004841B0, D_00487228, D_00487240, NULL,
    D_00487260, D_00487280, D_004872A0, NULL,
    D_004872C0, D_004872E0, D_00487300, NULL,
    D_00487310, D_00487330, D_00487350, NULL,
    D_00487370, D_00487390, D_004873B0, NULL,
    D_004873D0, D_004873F0, D_00487410, NULL,
    D_00487430, D_00487450, D_00487470, NULL,
    D_00487490, D_004874B0, D_004874D0, NULL,
    D_00487490, D_004874F0, D_00487510, NULL,
    D_00484410, D_00487520, D_00487540, NULL,
    D_00487560, D_00487580, D_004875A0, NULL,
    D_004875C0, D_004875E0, D_00487600, NULL,
    D_00487620, D_00487640, D_00487660, NULL,
    D_00487620, D_00487680, D_004876A0, NULL,
    D_004876C0, D_004876E0, D_00487700, NULL,
    D_004876C0, D_00487720, D_00487740, NULL,
    D_00487760, D_00487780, D_004877A0, NULL,
    D_004877C0, D_004877E0, D_00487800, NULL,
    D_00487820, D_00487840, D_00487860, NULL,
    D_00487880, D_004878A0, D_004878C0, NULL,
    D_004847A0, D_00487980, D_004879A0, NULL,
    D_004847A0, D_004879C0, D_004879E0, NULL,
    D_00487A00, D_00487A20, D_00487A40, NULL,
    D_00487A60, D_00487A80, D_00487AA0, NULL,
    D_00487AC0, D_00487AE0, D_00487B00, NULL,
    D_00484960, D_00487B20, D_00487B40, NULL,
    D_004849B0, D_00487B60, D_00487B80, NULL,
    D_00487BA0, D_00487BC0, D_00487BE0, NULL,
    D_004887B0, D_004887D0, D_004887F0, NULL,
    D_004887B0, D_004887D0, D_00488810, NULL,
    D_00488830, D_00488850, D_00488870, NULL,
    D_00485D20, D_00488890, D_004888B0, NULL,
    D_004888D0, D_004888F0, D_00488910, NULL,
    D_00488930, D_00488950, D_00488970, NULL,
    D_00485260, D_00488080, D_004880A0, NULL,
    D_00485260, D_004880C0, D_004880E0, NULL,
    D_00485300, D_00488100, D_00488120, NULL,
    D_00485300, D_00488140, D_00488160, NULL,
    D_00488180, D_004881A0, D_004881C0, NULL,
    D_004881E0, D_00488200, D_00488220, NULL,
    D_00488990, D_004889B0, D_004889D0, NULL,
    D_004889F0, D_00488A10, D_00488A30, NULL,
    D_00488990, D_004889B0, D_004889D0, NULL,
    D_004889F0, D_00488A10, D_00488A30, NULL,
    D_00488990, D_004889B0, D_004889D0, NULL,
    D_004889F0, D_00488A10, D_00488A30, NULL,
    D_00485E40, D_00488A50, D_00488A70, NULL,
    D_00485E40, D_00488A90, D_00488AB0, NULL,
    D_00485E40, D_00488A50, D_00488A70, NULL,
    D_00485E40, D_00488A90, D_00488AB0, NULL,
    D_00485E40, D_00488A50, D_00488A70, NULL,
    D_00485E40, D_00488A90, D_00488AB0, NULL,
    D_00485EF0, D_00488AD0, D_00488AF0, NULL,
    D_00488B10, D_00488B30, D_00488B50, NULL,
    D_00485EF0, D_00488AD0, D_00488AF0, NULL,
    D_00488B10, D_00488B30, D_00488B50, NULL,
    D_00485EF0, D_00488AD0, D_00488AF0, NULL,
    D_00488B10, D_00488B30, D_00488B50, NULL,
    D_00488B70, D_00488B90, D_00488BB0, NULL,
    D_00485FF0, D_00488BD0, D_00488BF0, NULL,
    D_00488B70, D_00488B90, D_00488BB0, NULL,
    D_00485FF0, D_00488BD0, D_00488BF0, NULL,
    D_00488B70, D_00488B90, D_00488BB0, NULL,
    D_00485FF0, D_00488BD0, D_00488BF0, NULL,
    D_00488C10, D_00488C30, D_00488C50, NULL,
    D_00488C70, D_00488C90, D_00488CB0, NULL,
    D_00488C10, D_00488C30, D_00488C50, NULL,
    D_00488C70, D_00488C90, D_00488CB0, NULL,
    D_00488CD0, D_00488CF0, D_00488D10, NULL,
    D_00488D30, D_00488D50, D_00488D70, NULL,
    D_00488D90, D_00488DB0, D_00488DD0, NULL,
    D_00488D90, D_00488DF0, D_00488E10, NULL,
    D_00488E30, D_00488E50, D_00488E70, NULL,
    D_00488E90, D_00488EB0, D_00488ED0, NULL,
    D_00488EF0, D_00488F10, D_00488F30, NULL,
    D_00488F50, D_00488F70, D_00488F90, NULL,
    D_00488FB0, D_00488FD0, D_00488FF0, NULL,
    D_00489010, D_00489030, D_00489050, NULL,
    D_00489070, D_00489090, D_004890B0, NULL,
    D_004890D0, D_004890F0, D_00489110, NULL,
    D_00489130, D_00489150, D_00489170, NULL,
    D_00489190, D_004891B0, D_004891D0, NULL,
    D_004891F0, D_00489210, D_00489230, NULL,
    D_004891F0, D_00489210, D_00489230, NULL,
    D_00484A50, D_00487C00, D_00484A90, NULL,
    D_00487D70, D_00487228, D_00489240, NULL,
    D_00489260, D_00489280, NULL, NULL,
    D_004892A0, D_004892C0, D_004892E0, NULL,
    D_00489300, D_00489320, D_00489340, NULL,
    D_00489360, D_00489380, D_004893A0, NULL,
    D_004893C0, D_004893E0, D_00489400, NULL,
    D_00489420, D_00489440, D_00489460, NULL,
    D_00489480, D_004894A0, D_004894C0, NULL,
    D_004894E0, D_00489280, NULL, NULL,
    D_00488830, D_00488850, D_00488870, NULL,
    D_00485D20, D_00488890, D_004888B0, NULL,
    D_004888D0, D_004888F0, D_00488910, NULL,
    D_00488930, D_00488950, D_00488970, NULL,
    D_00489500, D_00489520, D_00489540, NULL,
    D_00489500, D_00489560, D_00489580, NULL,
    D_004895A0, D_004895C0, D_004895E0, NULL,
    D_0047B0D0, NULL, NULL, NULL,
    D_00486D80, D_00489600, D_00489620, NULL,
    D_00489640, D_00489660, D_00489680, NULL,
    D_004896A0, D_004896C0, D_004896E0, NULL,
    D_00489700, D_00489720, D_00489740, NULL,
    D_00486ED0, D_00489760, D_00489780, NULL,
    D_004897A0, D_004897C0, D_004897E0, NULL,
    D_00483EE0, D_00489800, D_00489820, NULL,
    D_00489830, D_00489850, D_00489870, NULL,
    D_004896A0, D_00489890, D_004898B0, NULL,
    D_004896A0, D_004898D0, D_004898F0, NULL,
    D_00484010, D_00489910, D_00489930, NULL,
    D_00484010, D_00489950, D_00489970, NULL,
    D_00487140, D_00489980, D_004899A0, NULL,
    D_004899C0, D_004899E0, D_00489A00, NULL,
    D_00489A20, D_00489A40, D_00489A58, NULL,
    D_004841B0, D_00489A70, D_00489A90, NULL,
    D_00487260, D_00489AB0, D_00489AD0, NULL,
    D_00489AF0, D_00489B10, D_00489B30, NULL,
    D_00489B50, D_00489B70, D_00489B90, NULL,
    D_00487370, D_00489BB0, D_00489BD0, NULL,
    D_004842B0, D_00489BF0, D_004842F0, NULL,
    D_00489C10, D_00489C30, D_00489C50, NULL,
    D_00489C70, D_00489C90, D_00489CB0, NULL,
    D_00489C70, D_00489CD0, D_00489CF0, NULL,
    D_00489D10, D_00489D30, D_00489D50, NULL,
    D_00484410, D_00489D70, D_00489D90, NULL,
    D_00489DB0, D_00489DD0, D_00489DF0, NULL,
    D_00489E10, D_00489E30, D_00489E50, NULL,
    D_00489E10, D_00489E70, D_00489E90, NULL,
    D_00489EB0, D_00489ED0, D_00489EF0, NULL,
    D_00489EB0, D_00489F10, D_00489F30, NULL,
    D_00489F40, D_00489F60, D_00489F80, NULL,
    D_00489FA0, D_00489FC0, D_00489FE0, NULL,
    D_00487820, D_0048A000, D_0048A020, NULL,
    D_00487880, D_0048A040, D_0048A060, NULL,
    D_0048A080, D_0048A0A0, D_0048A0C0, NULL,
    D_0048A0E0, D_0048A100, D_0048A120, NULL,
    D_004847A0, D_0048A140, D_0048A160, NULL,
    D_004847A0, D_0048A180, D_0048A1A0, NULL,
    D_0048A1C0, D_0048A1E0, D_0048A200, NULL,
    D_0048A1C0, D_0048A1E0, D_0048A220, NULL,
    D_0048A240, D_0048A260, D_0048A280, NULL,
    D_0048A2A0, D_0048A2C0, D_0048A2E0, NULL,
    D_004849B0, D_0048A300, D_0048A320, NULL,
    D_004849B0, D_0048A340, D_0048A360, NULL,
    D_00484A50, D_0048A380, D_00484A90, NULL,
    D_0048A3A0, D_0048A3C0, D_0048A3E0, NULL,
    D_0048A3F0, D_0048A410, D_0048A430, NULL,
    D_00487D70, D_0048A450, D_0048A460, NULL,
    D_00487DB0, D_0048A480, D_0048A4A0, NULL,
    D_00487E10, D_0048A4C0, D_0048A4E0, NULL,
    D_00487E70, D_00487E90, D_0048A500, NULL,
    D_00487E70, D_00487E90, D_0048A500, NULL,
    D_00484F80, D_0048A520, D_0048A540, NULL,
    D_00484F80, D_0048A560, D_0048A580, NULL,
    D_0048A5A0, D_0048A5C0, D_0048A5E0, NULL,
    D_0048A600, D_0048A620, D_0048A640, NULL,
    D_0048A660, D_0048A680, D_0048A6A0, NULL,
    D_0048A6C0, D_0048A6E0, D_0048A700, NULL,
    D_00485260, D_0048A720, D_0048A740, NULL,
    D_00485260, D_0048A760, D_0048A780, NULL,
    D_00485300, D_0048A7A0, D_0048A7C0, NULL,
    D_00485300, D_0048A7E0, D_0048A800, NULL,
    D_0048A820, D_0048A840, D_0048A860, NULL,
    D_0048A880, D_0048A8A0, D_0048A8C0, NULL,
    D_00486D80, D_00489600, D_00489620, NULL,
    D_00489640, D_00489660, D_00489680, NULL,
    D_004896A0, D_004896C0, D_004896E0, NULL,
    D_00489700, D_00489720, D_00489740, NULL,
    D_00486ED0, D_00489760, D_00489780, NULL,
    D_004897A0, D_004897C0, D_004897E0, NULL,
    D_00483EE0, D_00489800, D_00489820, NULL,
    D_00489830, D_00489850, D_00489870, NULL,
    D_004896A0, D_00489890, D_004898B0, NULL,
    D_004896A0, D_004898D0, D_004898F0, NULL,
    D_00484010, D_00489910, D_00489930, NULL,
    D_00484010, D_00489950, D_00489970, NULL,
    D_00487140, D_00489980, D_004899A0, NULL,
    D_004899C0, D_004899E0, D_00489A00, NULL,
    D_00489A20, D_00489A40, D_00489A58, NULL,
    D_004841B0, D_00489A70, D_00489A90, NULL,
    D_00487260, D_00489AB0, D_00489AD0, NULL,
    D_00489AF0, D_00489B10, D_00489B30, NULL,
    D_00489B50, D_0048A8E0, D_00489B90, NULL,
    D_00487370, D_00489BB0, D_00489BD0, NULL,
    D_004842B0, D_00489BF0, D_004842F0, NULL,
    D_0048A900, D_00489C30, D_00489C50, NULL,
    D_00489C70, D_00489C90, D_00489CB0, NULL,
    D_00489C70, D_00489CD0, D_00489CF0, NULL,
    D_00489D10, D_00489D30, D_00489D50, NULL,
    D_00484410, D_00489D70, D_00489D90, NULL,
    D_00489DB0, D_00489DD0, D_00489DF0, NULL,
    D_0048A920, D_0048A940, D_0048A960, NULL,
    D_0048A980, D_0048A9A0, D_0048A9C0, NULL,
    D_00489EB0, D_00489ED0, D_00489EF0, NULL,
    D_00489EB0, D_00489F10, D_00489F30, NULL,
    D_0048A9E0, D_0048AA00, D_0048AA20, NULL,
    D_0048AA40, D_0048AA60, D_0048AA80, NULL,
    D_00487820, D_0048A000, D_0048A020, NULL,
    D_00487880, D_0048A040, D_0048A060, NULL,
    D_004847A0, D_0048A140, D_0048A160, NULL,
    D_004847A0, D_0048A180, D_0048A1A0, NULL,
    D_0048A1C0, D_0048A1E0, D_0048A200, NULL,
    D_0048A1C0, D_0048A1E0, D_0048A220, NULL,
    D_0048A240, D_0048A260, D_0048A280, NULL,
    D_0048A2A0, D_0048A2C0, D_0048A2E0, NULL,
    D_004849B0, D_0048A300, D_0048A320, NULL,
    D_004849B0, D_0048A340, D_0048A360, NULL,
    D_00485600, D_00485620, D_0048AAA0, NULL,
    D_00485660, D_0048AAC0, D_0048AAE0, NULL,
    D_004856B0, D_0048AAF0, D_0048AB10, NULL,
    D_0048AB30, D_0048AB50, D_0048AB70, NULL,
    D_0048AB90, D_0048ABB0, D_0048ABD0, NULL,
    D_0048ABF0, D_0048AC10, D_0048AC30, NULL,
    D_00485260, D_0048A720, D_0048A740, NULL,
    D_00485260, D_0048A760, D_0048A780, NULL,
    D_00485300, D_0048A7A0, D_0048A7C0, NULL,
    D_00485300, D_0048A7E0, D_0048A800, NULL,
    D_0048AC50, D_0048AC70, D_0048AC90, NULL,
    D_0048ACB0, D_0048ACD0, D_0048ACF0, NULL,
    D_0048AD10, D_0048AD30, D_0048AD50, NULL,
    D_0048AD70, D_0048AD90, D_0048ADB0, NULL,
    D_0048ADD0, D_0048ADF0, D_0048AE10, NULL,
    D_0048AE30, D_0048AE50, D_0048AE70, NULL,
    D_00486D80, D_00489600, D_00489620, NULL,
    D_00489640, D_00489660, D_00489680, NULL,
    D_004896A0, D_004896C0, D_004896E0, NULL,
    D_00489700, D_00489720, D_00489740, NULL,
    D_00486ED0, D_00489760, D_00489780, NULL,
    D_004897A0, D_004897C0, D_004897E0, NULL,
    D_00483EE0, D_00489800, D_00489820, NULL,
    D_00489830, D_00489850, D_00489870, NULL,
    D_004896A0, D_00489890, D_004898B0, NULL,
    D_004896A0, D_004898D0, D_004898F0, NULL,
    D_00484010, D_00489910, D_00489930, NULL,
    D_00484010, D_00489950, D_00489970, NULL,
    D_00487140, D_00489980, D_004899A0, NULL,
    D_004899C0, D_004899E0, D_00489A00, NULL,
    D_00489A20, D_00489A40, D_00489A58, NULL,
    D_004841B0, D_00489A70, D_00489A90, NULL,
    D_00487260, D_00489AB0, D_00489AD0, NULL,
    D_00489AF0, D_00489B10, D_00489B30, NULL,
    D_00489B50, D_00489B70, D_00489B90, NULL,
    D_00487370, D_00489BB0, D_00489BD0, NULL,
    D_004842B0, D_00489BF0, D_004842F0, NULL,
    D_00489C10, D_00489C30, D_00489C50, NULL,
    D_00489C70, D_00489C90, D_00489CB0, NULL,
    D_00489C70, D_00489CD0, D_00489CF0, NULL,
    D_00489D10, D_00489D30, D_00489D50, NULL,
    D_00484410, D_00489D70, D_00489D90, NULL,
    D_00489DB0, D_00489DD0, D_00489DF0, NULL,
    D_00489E10, D_00489E30, D_00489E50, NULL,
    D_00489E10, D_00489E70, D_00489E90, NULL,
    D_00489EB0, D_00489ED0, D_00489EF0, NULL,
    D_00489EB0, D_00489F10, D_00489F30, NULL,
    D_00489F40, D_00489F60, D_00489F80, NULL,
    D_00489FA0, D_00489FC0, D_00489FE0, NULL,
    D_00487820, D_0048A000, D_0048A020, NULL,
    D_00487880, D_0048A040, D_0048A060, NULL,
    D_004847A0, D_0048A140, D_0048A160, NULL,
    D_004847A0, D_0048A180, D_0048A1A0, NULL,
    D_0048A1C0, D_0048A1E0, D_0048A200, NULL,
    D_0048A1C0, D_0048A1E0, D_0048A220, NULL,
    D_0048A240, D_0048A260, D_0048A280, NULL,
    D_0048A2A0, D_0048A2C0, D_0048A2E0, NULL,
    D_004849B0, D_0048A300, D_0048A320, NULL,
    D_004849B0, D_0048A340, D_0048A360, NULL,
    D_0048AE90, D_0048AEB0, D_0048AED0, NULL,
    D_0048AEF0, D_0048AEB0, D_0048AED0, NULL,
    D_0048AF10, D_0048AF30, D_0048AF50, NULL,
    D_0048AF70, D_0048AF90, D_0048AFB0, NULL,
    D_0048AFD0, D_0048AFF0, D_0048B010, NULL,
    D_0048AFD0, D_0048B030, D_0048B050, NULL,
    D_00485260, D_0048A720, D_0048A740, NULL,
    D_00485260, D_0048A760, D_0048A780, NULL,
    D_00485300, D_0048A7A0, D_0048A7C0, NULL,
    D_00485300, D_0048A7E0, D_0048A800, NULL,
    D_0048A820, D_0048A840, D_0048A860, NULL,
    D_0048A880, D_0048A8A0, D_0048A8C0, NULL,
    D_0048B070, D_0048B090, D_0048B0B0, NULL,
    D_0048B0D0, D_0048B0F0, D_0048B110, NULL,
    D_0048B070, D_0048B090, D_0048B0B0, NULL,
    D_0048B0D0, D_0048B0F0, D_0048B110, NULL,
    D_0048B070, D_0048B090, D_0048B0B0, NULL,
    D_0048B0D0, D_0048B0F0, D_0048B110, NULL,
    D_00485E40, D_0048B130, D_0048B150, NULL,
    D_0048B170, D_0048B190, D_0048B1B0, NULL,
    D_00485E40, D_0048B130, D_0048B150, NULL,
    D_0048B170, D_0048B190, D_0048B1B0, NULL,
    D_00485E40, D_0048B130, D_0048B150, NULL,
    D_0048B170, D_0048B190, D_0048B1B0, NULL,
    D_0048B1D0, D_0048B1F0, D_0048B210, NULL,
    D_00485EF0, D_0048B230, D_0048B250, NULL,
    D_0048B1D0, D_0048B1F0, D_0048B210, NULL,
    D_00485EF0, D_0048B230, D_0048B250, NULL,
    D_0048B1D0, D_0048B1F0, D_0048B210, NULL,
    D_00485EF0, D_0048B230, D_0048B250, NULL,
    D_0048B270, D_0048B290, D_0048B2B0, NULL,
    D_0048B270, D_0048B2D0, D_0048B2F0, NULL,
    D_0048B270, D_0048B290, D_0048B2B0, NULL,
    D_0048B270, D_0048B2D0, D_0048B2F0, NULL,
    D_0048B270, D_0048B290, D_0048B2B0, NULL,
    D_0048B270, D_0048B2D0, D_0048B2F0, NULL,
    D_0048B310, D_0048B330, D_0048B350, NULL,
    D_0048B360, D_0048B380, D_0048B3A0, NULL,
    D_0048B310, D_0048B330, D_0048B350, NULL,
    D_0048B360, D_0048B380, D_0048B3A0, NULL,
    D_0048B3C0, D_0048B3E0, D_0048B400, NULL,
    D_00486170, D_0048B420, D_0048B440, NULL,
    D_0048B460, D_0048B480, D_0048B4A0, NULL,
    D_0048B4C0, D_0048B4E0, D_0048B500, NULL,
    D_0048B520, D_0048B540, D_0048B560, NULL,
    D_0048B580, D_0048B5A0, D_0048B5C0, NULL,
    D_0048B5E0, D_0048B600, D_0048B620, NULL,
    D_0048B640, D_0048B660, D_0048B680, NULL,
    D_0048B6A0, D_0048B6C0, D_0048B6E0, NULL,
    D_0048B700, D_0048B5A0, D_0048B720, NULL,
    D_0048B740, D_0048B760, D_0048B780, NULL,
    D_0048B740, D_0048B760, D_0048B7A0, NULL,
    D_0048B7C0, D_0048B7E0, D_0048B800, NULL,
    D_0048B820, D_0048B840, D_0048B860, NULL,
    D_0048B880, D_0048B8A0, D_0048B8C0, NULL,
    D_0048B880, D_0048B8A0, D_0048B8C0, NULL,
    D_00484A50, D_0048A380, D_00484A90, NULL,
    D_00487D70, D_0048A450, D_0048A460, NULL,
    D_0048B8D0, D_0048B8F0, D_0048B910, NULL,
    D_0048B930, D_0048B950, D_0048B970, NULL,
    D_0048B990, D_0048B9B0, D_0048B9D0, NULL,
    D_0048B9F0, D_0048BA10, D_0048BA30, NULL,
    D_0048BA50, D_0048BA70, D_0048BA90, NULL,
    D_0048AF10, D_0048AF30, D_0048AF50, NULL,
    D_0048AF70, D_0048AF90, D_0048AFB0, NULL,
    D_0048BAB0, D_0048AFF0, D_0048B010, NULL,
    D_0048AFD0, D_0048B030, D_0048B050, NULL,
    D_0048BAD0, D_0048BAF0, D_0048BB10, NULL,
    D_0048BB30, D_0048BB50, D_0048BB70, NULL,
    D_0048BB90, D_0048BBB0, D_0048BBD0, NULL,
    D_0047B0D0, NULL, NULL, NULL,
    D_00486D80, D_0048BBF0, D_0048BC10, NULL,
    D_0048BC30, D_0048BC50, D_0048BC70, NULL,
    D_0048BC90, D_0048BCB0, D_0048BCC0, NULL,
    D_0048BCE0, D_0048BD00, D_0048BD20, NULL,
    D_0048BD40, D_0048BD60, D_0048BD80, NULL,
    D_0048BD40, D_0048BDA0, D_0048BDC0, NULL,
    D_0048BDD0, D_0048BDF0, D_0048BE10, NULL,
    D_0048BE30, D_0048BE50, D_0048BE70, NULL,
    D_00483EE0, D_0048BE90, D_0048BEB0, NULL,
    D_0048BED0, D_0048BEF0, D_0048BF10, NULL,
    D_0048BD40, D_0048BF30, D_0048BF50, NULL,
    D_0048BD40, D_0048BF60, D_0048BF80, NULL,
    D_0048BFA0, D_0048BFC0, D_0048BFE0, NULL,
    D_0048C000, D_0048C020, D_0048C040, NULL,
    D_0048C060, D_0048C080, D_0048C0A0, NULL,
    D_0048C0C0, D_0048C0E0, D_0048C100, NULL,
    D_0048C120, D_0048C140, D_0048C160, NULL,
    D_0048C180, D_0048C1A0, D_0048C1C0, NULL,
    D_004841B0, D_0048C1D0, D_0048C1F0, NULL,
    D_00487260, D_0048C200, D_0048C220, NULL,
    D_0048C240, D_0048C260, D_0048C280, NULL,
    D_0048C2A0, D_0048C2C0, D_0048C2E0, NULL,
    D_00487370, D_0048C300, D_0048C320, NULL,
    D_00487370, D_0048C340, D_0048C360, NULL,
    D_0048C370, D_0048C390, D_0048C3B0, NULL,
    D_0048C3D0, D_0048C3F0, D_0048C410, NULL,
    D_0048C430, D_0048C450, D_0048C470, NULL,
    D_0048C430, D_0048C490, D_0048C4B0, NULL,
    D_00484410, D_0048C4C0, D_0048C4E0, NULL,
    D_00484410, D_0048C500, D_0048C520, NULL,
    D_0048C540, D_0048C560, D_0048C580, NULL,
    D_0048C5A0, D_0048C5C0, D_0048C5E0, NULL,
    D_0048C600, D_0048C620, D_0048C640, NULL,
    D_0048C660, D_004876E0, D_00487700, NULL,
    D_0048C660, D_0048C680, D_0048C6A0, NULL,
    D_0048C6C0, D_0048C6E0, D_0048C700, NULL,
    D_0048C720, D_0048C740, D_0048C760, NULL,
    D_004847A0, D_0048C780, D_0048C7A0, NULL,
    D_004847A0, D_0048C7C0, D_0048C7E0, NULL,
    D_0048C800, D_0048C820, D_0048C840, NULL,
    D_00487A00, D_0048C860, D_0048C878, NULL,
    D_00484960, D_0048C890, D_0048C8B0, NULL,
    D_0048C8D0, D_0048C8F0, D_0048C910, NULL,
    D_004849B0, D_0048C930, D_0048C950, NULL,
    D_004849B0, D_0048C970, D_0048C990, NULL,
    D_00484A50, D_0048C9B0, D_00484A90, NULL,
    D_0048C9D0, D_0048C9F0, D_0048CA10, NULL,
    D_0048CA30, D_0048CA50, D_0048CA70, NULL,
    D_00484C50, D_0048CA90, D_0048CAB0, NULL,
    D_0048CAD0, D_00484F40, D_00484F60, NULL,
    D_0048CAD0, D_00484F40, D_00484F60, NULL,
    D_00484F80, D_0048CAF0, D_0048CB10, NULL,
    D_00484F80, D_0048CB30, D_0048CB50, NULL,
    D_0048CB70, D_0048CB90, D_0048CBB0, NULL,
    D_00487F70, D_0048CBD0, D_0048CBF0, NULL,
    D_0048CC10, D_0048CC30, D_0048CC50, NULL,
    D_0048CC70, D_0048CC90, D_0048CCB0, NULL,
    D_0048CCD0, D_0048CCF0, D_0048CD10, NULL,
    D_0048CCD0, D_0048CD30, D_0048CD50, NULL,
    D_00485300, D_0048CD70, D_0048CD90, NULL,
    D_00485300, D_0048CDB0, D_0048CDD0, NULL,
    D_0048CDF0, D_0048CE10, D_0048A7C0, NULL,
    D_0048CE30, D_0048CE50, D_0048CE70, NULL,
    D_0048CE90, D_0048CEB0, D_0048CED0, NULL,
    D_0048CEF0, D_0048CF10, D_0048CF30, NULL,
    D_00486D80, D_0048BBF0, D_0048BC10, NULL,
    D_0048BC90, D_0048BCB0, D_0048BCC0, NULL,
    D_0048BD40, D_0048BD60, D_0048BD80, NULL,
    D_0048BD40, D_0048BDA0, D_0048BDC0, NULL,
    D_0048BDD0, D_0048BDF0, D_0048BE10, NULL,
    D_0048BE30, D_0048BE50, D_0048BE70, NULL,
    D_00483EE0, D_0048BE90, D_0048BEB0, NULL,
    D_0048BED0, D_0048BEF0, D_0048BF10, NULL,
    D_0048BD40, D_0048BF30, D_0048BF50, NULL,
    D_0048BD40, D_0048BF60, D_0048BF80, NULL,
    D_0048C000, D_0048C020, D_0048C040, NULL,
    D_0048C060, D_0048C080, D_0048C0A0, NULL,
    D_0048C0C0, D_0048C0E0, D_0048C100, NULL,
    D_0048C120, D_0048C140, D_0048C160, NULL,
    D_0048C180, D_0048C1A0, D_0048C1C0, NULL,
    D_004841B0, D_0048C1D0, D_0048C1F0, NULL,
    D_00487260, D_0048C200, D_0048C220, NULL,
    D_0048C240, D_0048C260, D_0048C280, NULL,
    D_00487370, D_0048C300, D_0048C320, NULL,
    D_00487370, D_0048C340, D_0048C360, NULL,
    D_0048C370, D_0048C390, D_0048C3B0, NULL,
    D_0048CF50, D_0048C3F0, D_0048C410, NULL,
    D_0048C430, D_0048C450, D_0048C470, NULL,
    D_0048C430, D_0048C490, D_0048C4B0, NULL,
    D_00484410, D_0048C4C0, D_0048C4E0, NULL,
    D_00484410, D_0048C500, D_0048C520, NULL,
    D_0048C540, D_0048C560, D_0048C580, NULL,
    D_0048CF70, D_0048CF90, D_0048CFB0, NULL,
    D_0048CFD0, D_0048CFF0, D_0048D010, NULL,
    D_0048C660, D_004876E0, D_00487700, NULL,
    D_0048C660, D_0048C680, D_0048C6A0, NULL,
    D_0048D030, D_0048D050, D_0048D070, NULL,
    D_0048D090, D_0048D0B0, D_0048D0D0, NULL,
    D_004847A0, D_0048C780, D_0048C7A0, NULL,
    D_004847A0, D_0048C7C0, D_0048C7E0, NULL,
    D_0048C800, D_0048C820, D_0048C840, NULL,
    D_00487A00, D_0048C860, D_0048C878, NULL,
    D_00484960, D_0048C890, D_0048C8B0, NULL,
    D_0048C8D0, D_0048C8F0, D_0048C910, NULL,
    D_004849B0, D_0048C930, D_0048C950, NULL,
    D_004849B0, D_0048C970, D_0048C990, NULL,
    D_00485600, D_00485620, D_0048D0F0, NULL,
    D_00485660, D_0048D110, D_0048D130, NULL,
    D_0048D140, D_0048D160, D_0048D180, NULL,
    D_0048D1A0, D_0048D1C0, D_0048D1E0, NULL,
    D_00485770, D_0048D200, D_0048D220, NULL,
    D_004857C0, D_0048D240, D_0048D260, NULL,
    D_0048CCD0, D_0048CCF0, D_0048CD10, NULL,
    D_0048CCD0, D_0048CD30, D_0048CD50, NULL,
    D_00485300, D_0048CD70, D_0048CD90, NULL,
    D_00485300, D_0048CDB0, D_0048CDD0, NULL,
    D_0048D280, D_0048D2A0, D_0048D2C0, NULL,
    D_0048D2E0, D_0048D300, D_0048D320, NULL,
    D_0048D340, D_0048D360, D_0048D380, NULL,
    D_0048D3A0, D_0048D3C0, D_0048D3E0, NULL,
    D_0048D400, D_0048D420, D_0048D440, NULL,
    D_0048D460, D_0048D480, D_0048D4A0, NULL,
    D_00486D80, D_0048BBF0, D_0048BC10, NULL,
    D_0048BC90, D_0048BCB0, D_0048BCC0, NULL,
    D_0048BD40, D_0048BD60, D_0048BD80, NULL,
    D_0048BD40, D_0048BDA0, D_0048BDC0, NULL,
    D_0048BDD0, D_0048BDF0, D_0048BE10, NULL,
    D_0048BE30, D_0048BE50, D_0048BE70, NULL,
    D_00483EE0, D_0048BE90, D_0048BEB0, NULL,
    D_0048BED0, D_0048BEF0, D_0048BF10, NULL,
    D_0048BD40, D_0048BF30, D_0048BF50, NULL,
    D_0048BD40, D_0048BF60, D_0048BF80, NULL,
    D_0048C000, D_0048C020, D_0048C040, NULL,
    D_0048C060, D_0048C080, D_0048C0A0, NULL,
    D_0048C0C0, D_0048C0E0, D_0048C100, NULL,
    D_0048C120, D_0048C140, D_0048C160, NULL,
    D_0048C180, D_0048C1A0, D_0048C1C0, NULL,
    D_004841B0, D_0048C1D0, D_0048C1F0, NULL,
    D_00487260, D_0048C200, D_0048C220, NULL,
    D_0048C240, D_0048C260, D_0048C280, NULL,
    D_00487370, D_0048C300, D_0048C320, NULL,
    D_00487370, D_0048C340, D_0048C360, NULL,
    D_0048C370, D_0048C390, D_0048C3B0, NULL,
    D_0048C3D0, D_0048C3F0, D_0048C410, NULL,
    D_0048C430, D_0048C450, D_0048C470, NULL,
    D_0048C430, D_0048C490, D_0048C4B0, NULL,
    D_00484410, D_0048C4C0, D_0048C4E0, NULL,
    D_00484410, D_0048C500, D_0048C520, NULL,
    D_0048C540, D_0048C560, D_0048C580, NULL,
    D_0048C5A0, D_0048C5C0, D_0048C5E0, NULL,
    D_0048C600, D_0048C620, D_0048C640, NULL,
    D_0048C660, D_004876E0, D_00487700, NULL,
    D_0048C660, D_0048C680, D_0048C6A0, NULL,
    D_0048C6C0, D_0048C6E0, D_0048C700, NULL,
    D_0048C720, D_0048C740, D_0048C760, NULL,
    D_004847A0, D_0048C780, D_0048C7A0, NULL,
    D_004847A0, D_0048C7C0, D_0048C7E0, NULL,
    D_0048C800, D_0048C820, D_0048C840, NULL,
    D_00487A00, D_0048C860, D_0048C878, NULL,
    D_00484960, D_0048C890, D_0048C8B0, NULL,
    D_0048C8D0, D_0048C8F0, D_0048C910, NULL,
    D_004849B0, D_0048C930, D_0048C950, NULL,
    D_004849B0, D_0048C970, D_0048C990, NULL,
    D_00485BA0, D_0048D4C0, D_00485BE0, NULL,
    D_00485BA0, D_0048D4C0, D_00485BE0, NULL,
    D_0048D4E0, D_0048D500, D_0048D520, NULL,
    D_00485C60, D_0048D540, D_0048D560, NULL,
    D_0048D580, D_0048D5A0, D_0048D5C0, NULL,
    D_0048D5E0, D_0048D600, D_0048D620, NULL,
    D_0048CCD0, D_0048CCF0, D_0048CD10, NULL,
    D_0048CCD0, D_0048CD30, D_0048CD50, NULL,
    D_00485300, D_0048CD70, D_0048CD90, NULL,
    D_00485300, D_0048CDB0, D_0048CDD0, NULL,
    D_0048CE90, D_0048CEB0, D_0048CED0, NULL,
    D_0048CEF0, D_0048CF10, D_0048CF30, NULL,
    D_0048D630, D_0048D650, D_0048D670, NULL,
    D_0048D690, D_0048D6B0, D_0048D6D0, NULL,
    D_0048D630, D_0048D650, D_0048D670, NULL,
    D_0048D690, D_0048D6B0, D_0048D6D0, NULL,
    D_0048D630, D_0048D650, D_0048D670, NULL,
    D_0048D690, D_0048D6B0, D_0048D6D0, NULL,
    D_0048D6F0, D_0048D710, D_0048D730, NULL,
    D_00485E40, D_0048D750, D_0048D770, NULL,
    D_0048D6F0, D_0048D710, D_0048D730, NULL,
    D_00485E40, D_0048D750, D_0048D770, NULL,
    D_0048D6F0, D_0048D710, D_0048D730, NULL,
    D_00485E40, D_0048D750, D_0048D770, NULL,
    D_00485EF0, D_0048D790, D_0048D7B0, NULL,
    D_00485EF0, D_0048D7D0, D_0048D7F0, NULL,
    D_00485EF0, D_0048D790, D_0048D7B0, NULL,
    D_00485EF0, D_0048D7D0, D_0048D7F0, NULL,
    D_00485EF0, D_0048D790, D_0048D7B0, NULL,
    D_00485EF0, D_0048D7D0, D_0048D7F0, NULL,
    D_0048D810, D_0048D830, D_0048D850, NULL,
    D_0048D870, D_0048D890, D_0048D8B0, NULL,
    D_0048D810, D_0048D830, D_0048D850, NULL,
    D_0048D870, D_0048D890, D_0048D8B0, NULL,
    D_0048D810, D_0048D830, D_0048D850, NULL,
    D_0048D870, D_0048D890, D_0048D8B0, NULL,
    D_0048D8D0, D_0048D8F0, D_0048D910, NULL,
    D_0048D930, D_0048D950, D_0048D970, NULL,
    D_0048D8D0, D_0048D8F0, D_0048D910, NULL,
    D_0048D930, D_0048D950, D_0048D970, NULL,
    D_0048D990, D_0048D9B0, D_0048D9D0, NULL,
    D_0048D9F0, D_0048DA10, D_0048DA30, NULL,
    D_0048DA50, D_0048DA70, D_0048DA90, NULL,
    D_0048DAB0, D_0048DAD0, D_0048DAE0, NULL,
    D_0048DB00, D_0048DB20, D_0048DB40, NULL,
    D_0048DB60, D_0048DB80, D_0048DBA0, NULL,
    D_0048DBC0, D_0048DBE0, D_0048DC00, NULL,
    D_0048DC20, D_0048DC40, D_0048DC60, NULL,
    D_0048DC80, D_0048DCA0, D_0048DCC0, NULL,
    D_0048DCE0, D_0048DD00, D_0048DD20, NULL,
    D_0048DD40, D_0048DD60, D_0048DD80, NULL,
    D_0048DD40, D_0048DD60, D_0048DDA0, NULL,
    D_0048DDC0, D_0048DDE0, D_0048DE00, NULL,
    D_0048DE20, D_0048DE40, D_0048DE60, NULL,
    D_0048DE80, D_0048DEA0, D_0048DEC0, NULL,
    D_0048DE80, D_0048DEA0, D_0048DEC0, NULL,
    D_0048DEE0, D_0048DF00, D_0048DF20, NULL,
    D_0048DEE0, D_0048DF40, D_0048DF60, NULL,
    D_00484A50, D_0048C9B0, D_00484A90, NULL,
    D_0048DF80, D_0048DFA0, D_0048DFC0, NULL,
    D_0048DFE0, D_0048E000, D_0048E020, NULL,
    D_0048E040, D_0048E060, D_0048E080, NULL,
    D_0048E0A0, D_0048E0C0, D_0048E0E0, NULL,
    D_0048E100, D_0048E120, D_0048E140, NULL,
    D_0048D4E0, D_0048D500, D_0048D520, NULL,
    D_00485C60, D_0048D540, D_0048D560, NULL,
    D_0048D580, D_0048D5A0, D_0048D5C0, NULL,
    D_0048D5E0, D_0048D600, D_0048D620, NULL,
    D_00484740, D_0048E160, D_0048E180, NULL,
    D_0048A0E0, D_0048E1A0, D_0048E1C0, NULL,
    D_0048E1E0, D_0048E200, D_0048E220, NULL,
    D_0048E1E0, D_0048E240, D_0048E260, NULL,
    D_0048E280, D_0048E2A0, D_0048E2C0, NULL,
    D_0047B0D0, NULL, NULL, NULL,
    D_0048E2E0, D_0048E300, D_0048E310, NULL,
    D_0048E330, D_0048E350, D_0048E370, NULL,
    D_0048E390, D_0048E3B0, D_0048E3D0, NULL,
    D_0048E390, D_0048E3E0, D_0048E400, NULL,
    D_0048E420, D_0048E440, D_0048E460, NULL,
    D_0048E420, D_0048E480, D_0048E4A0, NULL,
    D_0048E4B0, D_0048E4D0, D_0048E4F0, NULL,
    D_0048E510, D_0048E530, D_0048E550, NULL,
    D_0048E390, D_0048E570, D_0048E590, NULL,
    D_0048E390, D_0048E5B0, D_0048E5D0, NULL,
    D_0048E5F0, D_0048E610, D_0048E630, NULL,
    D_0048E650, D_0048E670, D_0048E690, NULL,
    D_00484010, D_0048E6B0, D_0048E6D0, NULL,
    D_0048E6E0, D_0048E700, D_0048E720, NULL,
    D_00487140, D_0048E740, D_0048E760, NULL,
    D_0048E780, D_0048E7A0, D_0048E7B8, NULL,
    D_004841B0, D_0048E7D0, D_0048E7F0, NULL,
    D_0048E810, D_0048E830, D_0048E850, NULL,
    D_0048E870, D_0048E890, D_0048E8B0, NULL,
    D_0048E8D0, D_0048E8F0, D_0048E910, NULL,
    D_0048E930, D_0048E950, D_0048E970, NULL,
    D_0048C2A0, D_0048E990, D_0048E9B0, NULL,
    D_0048E9D0, D_0048E9F0, D_0048E910, NULL,
    D_0048EA10, D_0048EA30, D_0048EA50, NULL,
    D_0048EA70, D_0048EA90, D_0048EAB0, NULL,
    D_0048EAD0, D_0048EAF0, D_0048EB10, NULL,
    D_0048EB30, D_0048EB50, D_0048EB70, NULL,
    D_00484410, D_0048EB90, D_0048EBB0, NULL,
    D_0048EBD0, D_0048EBF0, D_0048EC10, NULL,
    D_0048EC30, D_0048EC50, D_0048EC70, NULL,
    D_0048EC90, D_00489ED0, D_00489EF0, NULL,
    D_0048EC90, D_00489F10, D_00489F30, NULL,
    D_0048ECB0, D_0048ECD0, D_0048ECF0, NULL,
    D_0048ED10, D_0048ED30, D_0048ED50, NULL,
    D_00487820, D_0048ED70, D_0048ED90, NULL,
    D_00487880, D_004878A0, D_0048EDB0, NULL,
    D_004847A0, D_0048EDD0, D_0048EDF0, NULL,
    D_004847A0, D_0048EE10, D_0048EE30, NULL,
    D_0048EE50, D_0048EE70, D_0048EE90, NULL,
    D_0048EEB0, D_0048EED0, D_0048EEF0, NULL,
    D_00484960, D_0048EF10, D_0048EF30, NULL,
    D_00484960, D_0048EF50, D_0048EF70, NULL,
    D_004849B0, D_0048EF90, D_0048EFB0, NULL,
    D_004849B0, D_0048EFD0, D_0048EFF0, NULL,
    D_00484A50, D_0048F010, D_00484A90, NULL,
    D_0048F030, D_0048F050, D_0048F070, NULL,
    D_00487D70, D_0048F080, D_0048F090, NULL,
    D_00487DB0, D_0048F0B0, D_0048F0D0, NULL,
    D_00487E10, D_0048F0F0, D_0048F110, NULL,
    D_00487E70, D_00487E90, D_0048F130, NULL,
    D_00487E70, D_00487E90, D_0048F130, NULL,
    D_00484F80, D_0048F150, D_0048F170, NULL,
    D_00484F80, D_0048F190, D_0048F1B0, NULL,
    D_0048A5A0, D_0048F1D0, D_0048F1F0, NULL,
    D_00487F70, D_0048F210, D_0048F230, NULL,
    D_0048F250, D_0048F270, D_0048F290, NULL,
    D_0048F2B0, D_0048F2D0, D_0048F2F0, NULL,
    D_00485260, D_0048F310, D_0048F330, NULL,
    D_00485260, D_0048F350, D_0048F370, NULL,
    D_00485300, D_0048F390, D_0048F3B0, NULL,
    D_00485300, D_0048F3D0, D_0048F3F0, NULL,
    D_0048F410, D_0048F430, D_0048F450, NULL,
    D_0048F470, D_0048F430, D_0048F450, NULL,
    D_0048F490, D_0048F4B0, D_0048F4D0, NULL,
    D_0048F4F0, D_0048F510, D_0048F530, NULL,
    D_0048E2E0, D_0048E300, D_0048E310, NULL,
    D_0048E330, D_0048E350, D_0048E370, NULL,
    D_0048E390, D_0048E3B0, D_0048E3D0, NULL,
    D_0048E390, D_0048E3E0, D_0048E400, NULL,
    D_0048E420, D_0048E440, D_0048E460, NULL,
    D_0048E420, D_0048E480, D_0048E4A0, NULL,
    D_0048E4B0, D_0048E4D0, D_0048E4F0, NULL,
    D_0048E510, D_0048E530, D_0048E550, NULL,
    D_0048E390, D_0048E570, D_0048E590, NULL,
    D_0048E390, D_0048E5B0, D_0048E5D0, NULL,
    D_0048E650, D_0048E670, D_0048E690, NULL,
    D_00484010, D_0048E6B0, D_0048E6D0, NULL,
    D_0048E6E0, D_0048E700, D_0048E720, NULL,
    D_00487140, D_0048E740, D_0048E760, NULL,
    D_0048E780, D_0048E7A0, D_0048E7B8, NULL,
    D_004841B0, D_0048E7D0, D_0048E7F0, NULL,
    D_0048E870, D_0048E890, D_0048E8B0, NULL,
    D_0048E8D0, D_0048E8F0, D_0048E910, NULL,
    D_0048F550, D_0048E9F0, D_0048E910, NULL,
    D_0048EA10, D_0048EA30, D_0048EA50, NULL,
    D_0048EA70, D_0048EA90, D_0048EAB0, NULL,
    D_0048EAD0, D_0048EAF0, D_0048EB10, NULL,
    D_0048EB30, D_0048EB50, D_0048EB70, NULL,
    D_00484410, D_0048EB90, D_0048EBB0, NULL,
    D_0048F570, D_0048F590, D_0048F5B0, NULL,
    D_0048F5D0, D_0048F5F0, D_0048F610, NULL,
    D_0048EC90, D_00489ED0, D_00489EF0, NULL,
    D_0048EC90, D_00489F10, D_00489F30, NULL,
    D_0048F630, D_0048F650, D_0048F670, NULL,
    D_0048F690, D_0048F6B0, D_0048F6D0, NULL,
    D_00487820, D_0048ED70, D_0048ED90, NULL,
    D_00487880, D_004878A0, D_0048EDB0, NULL,
    D_004847A0, D_0048EDD0, D_0048EDF0, NULL,
    D_004847A0, D_0048EE10, D_0048EE30, NULL,
    D_0048EE50, D_0048EE70, D_0048EE90, NULL,
    D_0048EEB0, D_0048EED0, D_0048EEF0, NULL,
    D_00484960, D_0048EF10, D_0048EF30, NULL,
    D_00484960, D_0048EF50, D_0048EF70, NULL,
    D_004849B0, D_0048EF90, D_0048EFB0, NULL,
    D_004849B0, D_0048EFD0, D_0048EFF0, NULL,
    D_00485600, D_00485620, D_0048F6F0, NULL,
    D_00485660, D_0048F710, D_0048F730, NULL,
    D_0048F740, D_0048F760, D_0048F780, NULL,
    D_0048F740, D_0048F7A0, D_0048F7C0, NULL,
    D_0048AB90, D_0048F7E0, D_0048F800, NULL,
    D_0048ABF0, D_0048F820, D_0048F840, NULL,
    D_00485260, D_0048F310, D_0048F330, NULL,
    D_00485260, D_0048F350, D_0048F370, NULL,
    D_00485300, D_0048F390, D_0048F3B0, NULL,
    D_00485300, D_0048F3D0, D_0048F3F0, NULL,
    D_0048F860, D_0048F880, D_0048F8A0, NULL,
    D_0048F8C0, D_0048F8E0, D_0048F900, NULL,
    D_0048F920, D_0048F940, D_0048F960, NULL,
    D_0048F980, D_0048F9A0, D_0048F9C0, NULL,
    D_0048F9E0, D_0048FA00, D_0048FA20, NULL,
    D_0048FA40, D_0048FA60, D_0048FA80, NULL,
    D_0048E2E0, D_0048E300, D_0048E310, NULL,
    D_0048E330, D_0048E350, D_0048E370, NULL,
    D_0048E390, D_0048E3B0, D_0048E3D0, NULL,
    D_0048E390, D_0048E3E0, D_0048E400, NULL,
    D_0048E420, D_0048E440, D_0048E460, NULL,
    D_0048E420, D_0048E480, D_0048E4A0, NULL,
    D_0048E4B0, D_0048E4D0, D_0048E4F0, NULL,
    D_0048E510, D_0048E530, D_0048E550, NULL,
    D_0048E390, D_0048E570, D_0048E590, NULL,
    D_0048E390, D_0048E5B0, D_0048E5D0, NULL,
    D_0048E650, D_0048E670, D_0048E690, NULL,
    D_00484010, D_0048E6B0, D_0048E6D0, NULL,
    D_0048E6E0, D_0048E700, D_0048E720, NULL,
    D_00487140, D_0048E740, D_0048E760, NULL,
    D_0048E780, D_0048E7A0, D_0048E7B8, NULL,
    D_004841B0, D_0048E7D0, D_0048E7F0, NULL,
    D_0048E870, D_0048E890, D_0048E8B0, NULL,
    D_0048E8D0, D_0048E8F0, D_0048E910, NULL,
    D_0048FAA0, D_0048FAC0, D_0048FAE0, NULL,
    D_0048FB00, D_0048FB20, D_0048FB40, NULL,
    D_0048E9D0, D_0048E9F0, D_0048E910, NULL,
    D_0048EA10, D_0048EA30, D_0048EA50, NULL,
    D_0048EA70, D_0048EA90, D_0048EAB0, NULL,
    D_0048EAD0, D_0048EAF0, D_0048EB10, NULL,
    D_0048EB30, D_0048EB50, D_0048EB70, NULL,
    D_00484410, D_0048EB90, D_0048EBB0, NULL,
    D_0048EBD0, D_0048EBF0, D_0048EC10, NULL,
    D_0048EC30, D_0048EC50, D_0048EC70, NULL,
    D_0048EC90, D_00489ED0, D_00489EF0, NULL,
    D_0048EC90, D_00489F10, D_00489F30, NULL,
    D_0048ECB0, D_0048ECD0, D_0048ECF0, NULL,
    D_0048ED10, D_0048ED30, D_0048ED50, NULL,
    D_00487820, D_0048ED70, D_0048ED90, NULL,
    D_00487880, D_004878A0, D_0048EDB0, NULL,
    D_004847A0, D_0048EDD0, D_0048EDF0, NULL,
    D_004847A0, D_0048EE10, D_0048EE30, NULL,
    D_0048EE50, D_0048EE70, D_0048EE90, NULL,
    D_0048EEB0, D_0048EED0, D_0048EEF0, NULL,
    D_00484960, D_0048EF10, D_0048EF30, NULL,
    D_00484960, D_0048EF50, D_0048EF70, NULL,
    D_004849B0, D_0048EF90, D_0048EFB0, NULL,
    D_004849B0, D_0048EFD0, D_0048EFF0, NULL,
    D_004887B0, D_0048FB60, D_004887F0, NULL,
    D_004887B0, D_0048FB60, D_00488810, NULL,
    D_004856B0, D_0048FB80, D_0048FBA0, NULL,
    D_0048FBC0, D_0048FBE0, D_0048FC00, NULL,
    D_0048FC20, D_0048FC40, D_0048FC60, NULL,
    D_0048FC20, D_0048FC80, D_0048FCA0, NULL,
    D_00485260, D_0048F310, D_0048F330, NULL,
    D_00485260, D_0048F350, D_0048F370, NULL,
    D_00485300, D_0048F390, D_0048F3B0, NULL,
    D_00485300, D_0048F3D0, D_0048F3F0, NULL,
    D_0048F490, D_0048F4B0, D_0048F4D0, NULL,
    D_0048F4F0, D_0048F510, D_0048F530, NULL,
    D_0048FCC0, D_0048FCE0, D_0048FD00, NULL,
    D_0048FD20, D_0048FD40, D_0048FD60, NULL,
    D_0048FCC0, D_0048FCE0, D_0048FD00, NULL,
    D_0048FD20, D_0048FD40, D_0048FD60, NULL,
    D_0048FCC0, D_0048FCE0, D_0048FD00, NULL,
    D_0048FD20, D_0048FD40, D_0048FD60, NULL,
    D_0048FD80, D_0048FDA0, D_0048FDC0, NULL,
    D_0048FD80, D_0048FDE0, D_0048FE00, NULL,
    D_0048FD80, D_0048FDA0, D_0048FDC0, NULL,
    D_0048FD80, D_0048FDE0, D_0048FE00, NULL,
    D_0048FD80, D_0048FDA0, D_0048FDC0, NULL,
    D_0048FD80, D_0048FDE0, D_0048FE00, NULL,
    D_0048FE20, D_0048FE40, D_0048FE60, NULL,
    D_00485EF0, D_0048FE80, D_0048FEA0, NULL,
    D_0048FE20, D_0048FE40, D_0048FE60, NULL,
    D_00485EF0, D_0048FE80, D_0048FEA0, NULL,
    D_0048FE20, D_0048FE40, D_0048FE60, NULL,
    D_00485EF0, D_0048FE80, D_0048FEA0, NULL,
    D_00485F90, D_0048FEC0, D_0048FEE0, NULL,
    D_0048FF00, D_0048FF20, D_0048FF40, NULL,
    D_00485F90, D_0048FEC0, D_0048FEE0, NULL,
    D_0048FF00, D_0048FF20, D_0048FF40, NULL,
    D_00485F90, D_0048FEC0, D_0048FEE0, NULL,
    D_0048FF00, D_0048FF20, D_0048FF40, NULL,
    D_0048FF60, D_0048FF80, D_0048FFA0, NULL,
    D_0048FFC0, D_0048FFE0, D_00490000, NULL,
    D_0048FF60, D_0048FF80, D_0048FFA0, NULL,
    D_0048FFC0, D_0048FFE0, D_00490000, NULL,
    D_0048B3C0, D_00490020, D_00490040, NULL,
    D_00490060, D_00490080, D_004900A0, NULL,
    D_004900C0, D_004900E0, D_00490100, NULL,
    D_00490120, D_00490140, D_00490160, NULL,
    D_00490180, D_004901A0, D_004901C0, NULL,
    D_004901E0, D_00490200, D_00490220, NULL,
    D_00490240, D_00490260, D_00490280, NULL,
    D_004902A0, D_004902C0, D_004902E0, NULL,
    D_00490300, D_00490320, D_00490340, NULL,
    D_00490360, D_00490200, D_00490220, NULL,
    D_00490380, D_004903A0, D_004903C0, NULL,
    D_00490380, D_004903E0, D_00490400, NULL,
    D_00490420, D_00490440, D_00490460, NULL,
    D_00490480, D_004904A0, D_004904C0, NULL,
    D_004904E0, D_00490500, D_00490518, NULL,
    D_004904E0, D_00490500, D_00490518, NULL,
    D_00484A50, D_0048F010, D_00484A90, NULL,
    D_00487D70, D_0048F080, D_0048F090, NULL,
    D_00490530, D_00490550, D_00490570, NULL,
    D_00490580, D_004905A0, D_004905C0, NULL,
    D_004905E0, D_00490600, D_00490620, NULL,
    D_00490640, D_00490660, D_00490680, NULL,
    D_004906A0, D_004906C0, D_004906E0, NULL,
    D_004856B0, D_0048FB80, D_0048FBA0, NULL,
    D_0048FBC0, D_0048FBE0, D_0048FC00, NULL,
    D_0048FC20, D_0048FC40, D_0048FC60, NULL,
    D_0048FC20, D_0048FC80, D_0048FCA0, NULL,
    D_00490700, D_00490720, D_00490740, NULL,
    D_00484740, D_00490760, D_00490780, NULL,
    D_004907A0, D_004907C0, D_004907E0, NULL,
    D_00490800, D_004907C0, D_00490820, NULL,
    D_00490840, D_00490860, D_00490880, NULL,
};
u32 gIconUVs[32] = {
#include "sub_main/0043AA90.inc"
};
u32 gEquipColors[10] = {
#include "sub_main/0043AB10.inc"
};
u8 gRarityColorSlots[14] = {
#include "sub_main/0043AB38.inc"
};
u8 gRarityColors[8] = {
#include "sub_main/0043AB48.inc"
};
FrameStyle gFrameStyles[4] = {
#include "sub_main/0043AB50.inc"
};
FrameStyle2 gFrameStyles2[2] = {
#include "sub_main/0043ABA0.inc"
};
u32 gPanelFrameColor = {
#include "sub_main/0043ABC8.inc"
};
MarkPiece gTargetMarkerPieces[4] = {
#include "sub_main/0043ABD0.inc"
};
Panel gHelpWindowBox = {
#include "sub_main/0043AC00.inc"
};
u32 gDecoDetailColors[2][3] = {
#include "sub_main/0043AC20.inc"
};
Icon gDecoDetailIcon = {
#include "sub_main/0043AC38.inc"
};
Panel gChatFrameBox = {
#include "sub_main/0043AC40.inc"
};
/* [0] the chat input line (Chat_DrawWindow kinds 2 and 3), [1] gChatRecvWindow (an alias) the log. */
Panel gChatLogWindow[2] = {
#include "sub_main/0043AC60.inc"
};
ListPanel gPersonalStatusPanels[8] = {
#include "sub_main/0043AC90.inc"
};
Panel gPersonalStatusBoxes[8] = {
#include "sub_main/0043AD90.inc"
};
u8 gGunnerNoSkillGroups[40] = {
#include "sub_main/0043AE50.inc"
};
u16 gArmorEquipSlots[6] = {
#include "sub_main/0043AE78.inc"
};
Panel gEquipWindowBox = {
#include "sub_main/0043AE90.inc"
};
AmmoLine gAmmoLines[18] = {
#include "sub_main/0043AEB0.inc"
};
u8 gShellTypeOfSpecial[12] = {
#include "sub_main/0043B018.inc"
};
