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
 *
 * One file: the data of its parts lie in another order than their code (the windows' frame
 * styles, gFrameStyles, after tables that the later code reads; the window's format string,
 * gWindowFmtString, after the later code's literals), so no cut inside it keeps each side's
 * data together.
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
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "camera.h"
#include "fishing.h"
#include "kbd.h"
#include "shake.h"
#include "text.h"
#include "bait_menu.h"
#include "box_skill_lists.h"
#include "chat_phrases.h"
#include "draw_helpers.h"
#include "engine_math.h"
#include "equip_parts.h"
#include "font.h"
#include "game_util.h"
#include "lobby_queries.h"
#include "menu_util.h"
#include "town_frame.h"
#include "town_player_states.h"
#include "sprite_queue.h"

/* A line of the status page's skill list (gSkillRecords, 54 of them sorted by key, largest
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


/* prototypes */
struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, char *title, s32 sel, s32 alpha); /* unit's own declaration: the K&R definition's narrow parameters arrive promoted, as this prototype says (a narrow prototype is incompatible with it) */
void Window_DrawTextAlpha(Window *w, u8 *text, u8 alpha);
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
struct TexRect;
extern Panel gEquipWindowBox;
extern u8 gChatLogHead;
extern u8 gChatLogCount;
extern u8 gChatWinHold;
s32 sprintf(char *buf, const char *fmt, ...);
extern u8 gChatLines;
extern u16 gFrameTick;
extern char gWindowFmtString[];
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


/* The menu windows' texts and constants, read-only. */
#pragma rodata on
#pragma explicit_zero_data on
char gHelpTexts_1_8[8] = {
#include "sub_main/0046D110.inc"
};
#pragma explicit_zero_data reset
char gHelpTexts_1_22[8] = {
#include "sub_main/0046D118.inc"
};
char gHelpTexts_1_23[80] = {
#include "sub_main/0046D120.inc"
};
char gHelpTexts_1_24[80] = {
#include "sub_main/0046D170.inc"
};
char gHelpTexts_1_25[96] = {
#include "sub_main/0046D1C0.inc"
};
char gHelpTexts_1_26[96] = {
#include "sub_main/0046D220.inc"
};
char gHelpTexts_1_27[96] = {
#include "sub_main/0046D280.inc"
};
char gHelpTexts_1_28[80] = {
#include "sub_main/0046D2E0.inc"
};
char gHelpTexts_1_29[32] = {
#include "sub_main/0046D330.inc"
};
char gHelpTexts_1_30[32] = {
#include "sub_main/0046D350.inc"
};
char gHelpTexts_1_31[48] = {
#include "sub_main/0046D370.inc"
};
char gHelpTexts_1_32[48] = {
#include "sub_main/0046D3A0.inc"
};
char gHelpTexts_1_33[48] = {
#include "sub_main/0046D3D0.inc"
};
char gHelpTexts_1_34[32] = {
#include "sub_main/0046D400.inc"
};
char gHelpTexts_1_35[64] = {
#include "sub_main/0046D420.inc"
};
char gHelpTexts_1_36[96] = {
#include "sub_main/0046D460.inc"
};
char gHelpTexts_1_37[48] = {
#include "sub_main/0046D4C0.inc"
};
char gHelpTexts_1_38[48] = {
#include "sub_main/0046D4F0.inc"
};
char gHelpTexts_1_39[64] = {
#include "sub_main/0046D520.inc"
};
char gHelpTexts_1_40[64] = {
#include "sub_main/0046D560.inc"
};
char gHelpTexts_1_41[48] = {
#include "sub_main/0046D5A0.inc"
};
char gHelpTexts_1_42[64] = {
#include "sub_main/0046D5D0.inc"
};
char gHelpTexts_1_43[32] = {
#include "sub_main/0046D610.inc"
};
char gHelpTexts_1_44[32] = {
#include "sub_main/0046D630.inc"
};
char gHelpTexts_1_45[96] = {
#include "sub_main/0046D650.inc"
};
char gHelpTexts_1_46[80] = {
#include "sub_main/0046D6B0.inc"
};
char gHelpTexts_1_47[80] = {
#include "sub_main/0046D700.inc"
};
char gHelpTexts_1_48[64] = {
#include "sub_main/0046D750.inc"
};
char gHelpTexts_1_49[48] = {
#include "sub_main/0046D790.inc"
};
char gHelpTexts_1_50[64] = {
#include "sub_main/0046D7C0.inc"
};
char gHelpTexts_1_51[80] = {
#include "sub_main/0046D800.inc"
};
char gHelpTexts_1_52[72] = {
#include "sub_main/0046D850.inc"
};
char D_0046D898[12] = {
#include "sub_main/0046D898.inc"
};
char gHelpTexts_1_54[64] = {
#include "sub_main/0046D8B0.inc"
};
char gHelpTexts_1_55[48] = {
#include "sub_main/0046D8F0.inc"
};
char gHelpTexts_1_56[80] = {
#include "sub_main/0046D920.inc"
};
char gHelpTexts_1_57[96] = {
#include "sub_main/0046D970.inc"
};
char gHelpTexts_1_58[80] = {
#include "sub_main/0046D9D0.inc"
};
char gHelpTexts_1_59[80] = {
#include "sub_main/0046DA20.inc"
};
char gHelpTexts_1_60[80] = {
#include "sub_main/0046DA70.inc"
};
char gHelpTexts_1_61[64] = {
#include "sub_main/0046DAC0.inc"
};
char gHelpTexts_1_62[96] = {
#include "sub_main/0046DB00.inc"
};
char gHelpTexts_1_63[80] = {
#include "sub_main/0046DB60.inc"
};
char gHelpTexts_1_64[80] = {
#include "sub_main/0046DBB0.inc"
};
char gHelpTexts_1_65[64] = {
#include "sub_main/0046DC00.inc"
};
char gHelpTexts_1_66[48] = {
#include "sub_main/0046DC40.inc"
};
char gHelpTexts_1_67[48] = {
#include "sub_main/0046DC70.inc"
};
char gHelpTexts_1_68[48] = {
#include "sub_main/0046DCA0.inc"
};
char gHelpTexts_1_69[48] = {
#include "sub_main/0046DCD0.inc"
};
char gHelpTexts_1_70[48] = {
#include "sub_main/0046DD00.inc"
};
char gHelpTexts_1_71[80] = {
#include "sub_main/0046DD30.inc"
};
char gHelpTexts_1_72[64] = {
#include "sub_main/0046DD80.inc"
};
char gHelpTexts_1_73[64] = {
#include "sub_main/0046DDC0.inc"
};
char gHelpTexts_1_74[80] = {
#include "sub_main/0046DE00.inc"
};
char gHelpTexts_1_75[64] = {
#include "sub_main/0046DE50.inc"
};
char gHelpTexts_1_76[80] = {
#include "sub_main/0046DE90.inc"
};
char gHelpTexts_1_77[32] = {
#include "sub_main/0046DEE0.inc"
};
char gHelpTexts_1_78[64] = {
#include "sub_main/0046DF00.inc"
};
char gHelpTexts_1_79[48] = {
#include "sub_main/0046DF40.inc"
};
char gHelpTexts_1_80[64] = {
#include "sub_main/0046DF70.inc"
};
char gHelpTexts_1_81[80] = {
#include "sub_main/0046DFB0.inc"
};
char gHelpTexts_1_82[80] = {
#include "sub_main/0046E000.inc"
};
char gHelpTexts_1_83[80] = {
#include "sub_main/0046E050.inc"
};
char gHelpTexts_1_84[64] = {
#include "sub_main/0046E0A0.inc"
};
char gHelpTexts_1_86[48] = {
#include "sub_main/0046E0E0.inc"
};
char gHelpTexts_1_87[80] = {
#include "sub_main/0046E110.inc"
};
char gHelpTexts_1_88[80] = {
#include "sub_main/0046E160.inc"
};
char gHelpTexts_1_89[80] = {
#include "sub_main/0046E1B0.inc"
};
char gHelpTexts_1_90[80] = {
#include "sub_main/0046E200.inc"
};
char gHelpTexts_1_91[80] = {
#include "sub_main/0046E250.inc"
};
char gHelpTexts_1_92[32] = {
#include "sub_main/0046E2A0.inc"
};
char gHelpTexts_1_93[80] = {
#include "sub_main/0046E2C0.inc"
};
char gHelpTexts_1_94[96] = {
#include "sub_main/0046E310.inc"
};
char gHelpTexts_1_95[48] = {
#include "sub_main/0046E370.inc"
};
char gHelpTexts_1_96[32] = {
#include "sub_main/0046E3A0.inc"
};
char gHelpTexts_1_97[64] = {
#include "sub_main/0046E3C0.inc"
};
char gHelpTexts_1_98[48] = {
#include "sub_main/0046E400.inc"
};
char gHelpTexts_1_99[80] = {
#include "sub_main/0046E430.inc"
};
char gHelpTexts_1_100[64] = {
#include "sub_main/0046E480.inc"
};
char gHelpTexts_1_101[80] = {
#include "sub_main/0046E4C0.inc"
};
char gHelpTexts_1_102[64] = {
#include "sub_main/0046E510.inc"
};
char gHelpTexts_1_103[32] = {
#include "sub_main/0046E550.inc"
};
char gHelpTexts_1_104[48] = {
#include "sub_main/0046E570.inc"
};
char gHelpTexts_1_105[64] = {
#include "sub_main/0046E5A0.inc"
};
char gHelpTexts_1_106[80] = {
#include "sub_main/0046E5E0.inc"
};
char gHelpTexts_1_107[64] = {
#include "sub_main/0046E630.inc"
};
char gHelpTexts_1_108[80] = {
#include "sub_main/0046E670.inc"
};
char gHelpTexts_1_110[48] = {
#include "sub_main/0046E6C0.inc"
};
char gHelpTexts_1_111[32] = {
#include "sub_main/0046E6F0.inc"
};
char gHelpTexts_1_112[32] = {
#include "sub_main/0046E710.inc"
};
char gHelpTexts_1_113[32] = {
#include "sub_main/0046E730.inc"
};
char gHelpTexts_1_114[80] = {
#include "sub_main/0046E750.inc"
};
char gHelpTexts_1_115[48] = {
#include "sub_main/0046E7A0.inc"
};
char gHelpTexts_1_116[96] = {
#include "sub_main/0046E7D0.inc"
};
char gHelpTexts_1_117[80] = {
#include "sub_main/0046E830.inc"
};
char gHelpTexts_1_118[64] = {
#include "sub_main/0046E880.inc"
};
char gHelpTexts_1_119[64] = {
#include "sub_main/0046E8C0.inc"
};
char gHelpTexts_1_120[80] = {
#include "sub_main/0046E900.inc"
};
char gHelpTexts_1_121[32] = {
#include "sub_main/0046E950.inc"
};
char gHelpTexts_1_122[80] = {
#include "sub_main/0046E970.inc"
};
char gHelpTexts_1_123[96] = {
#include "sub_main/0046E9C0.inc"
};
char gHelpTexts_1_124[80] = {
#include "sub_main/0046EA20.inc"
};
char gHelpTexts_1_125[64] = {
#include "sub_main/0046EA70.inc"
};
char gHelpTexts_1_126[64] = {
#include "sub_main/0046EAB0.inc"
};
char gHelpTexts_1_127[80] = {
#include "sub_main/0046EAF0.inc"
};
char gHelpTexts_1_128[64] = {
#include "sub_main/0046EB40.inc"
};
char gHelpTexts_1_129[64] = {
#include "sub_main/0046EB80.inc"
};
char gHelpTexts_1_130[64] = {
#include "sub_main/0046EBC0.inc"
};
char gHelpTexts_1_131[64] = {
#include "sub_main/0046EC00.inc"
};
char gHelpTexts_1_132[64] = {
#include "sub_main/0046EC40.inc"
};
char gHelpTexts_1_133[64] = {
#include "sub_main/0046EC80.inc"
};
char gHelpTexts_1_134[64] = {
#include "sub_main/0046ECC0.inc"
};
char gHelpTexts_1_135[64] = {
#include "sub_main/0046ED00.inc"
};
char gHelpTexts_1_136[80] = {
#include "sub_main/0046ED40.inc"
};
char gHelpTexts_1_137[80] = {
#include "sub_main/0046ED90.inc"
};
char gHelpTexts_1_138[80] = {
#include "sub_main/0046EDE0.inc"
};
char gHelpTexts_1_139[80] = {
#include "sub_main/0046EE30.inc"
};
char gHelpTexts_1_140[80] = {
#include "sub_main/0046EE80.inc"
};
char gHelpTexts_1_141[80] = {
#include "sub_main/0046EED0.inc"
};
char gHelpTexts_1_142[80] = {
#include "sub_main/0046EF20.inc"
};
char gHelpTexts_1_143[64] = {
#include "sub_main/0046EF70.inc"
};
char gHelpTexts_1_144[64] = {
#include "sub_main/0046EFB0.inc"
};
char gHelpTexts_1_145[64] = {
#include "sub_main/0046EFF0.inc"
};
char gHelpTexts_1_146[80] = {
#include "sub_main/0046F030.inc"
};
char gHelpTexts_1_147[80] = {
#include "sub_main/0046F080.inc"
};
char gHelpTexts_1_148[80] = {
#include "sub_main/0046F0D0.inc"
};
char gHelpTexts_1_149[80] = {
#include "sub_main/0046F120.inc"
};
char gHelpTexts_1_150[80] = {
#include "sub_main/0046F170.inc"
};
char gHelpTexts_1_151[80] = {
#include "sub_main/0046F1C0.inc"
};
char gHelpTexts_1_152[80] = {
#include "sub_main/0046F210.inc"
};
char gHelpTexts_1_153[64] = {
#include "sub_main/0046F260.inc"
};
char gHelpTexts_1_154[64] = {
#include "sub_main/0046F2A0.inc"
};
char gHelpTexts_1_155[72] = {
#include "sub_main/0046F2E0.inc"
};
char gHelpTexts_1_157[15] = {
#include "sub_main/0046F328.inc"
};
char gHelpTexts_1_158[64] = {
#include "sub_main/0046F340.inc"
};
char gHelpTexts_1_159[64] = {
#include "sub_main/0046F380.inc"
};
char gHelpTexts_1_160[64] = {
#include "sub_main/0046F3C0.inc"
};
char gHelpTexts_1_161[64] = {
#include "sub_main/0046F400.inc"
};
char gHelpTexts_1_162[64] = {
#include "sub_main/0046F440.inc"
};
char gHelpTexts_1_164[48] = {
#include "sub_main/0046F480.inc"
};
char gHelpTexts_1_165[48] = {
#include "sub_main/0046F4B0.inc"
};
char gHelpTexts_1_166[48] = {
#include "sub_main/0046F4E0.inc"
};
char gHelpTexts_1_167[32] = {
#include "sub_main/0046F510.inc"
};
char gHelpTexts_1_168[48] = {
#include "sub_main/0046F530.inc"
};
char gHelpTexts_1_169[48] = {
#include "sub_main/0046F560.inc"
};
char gHelpTexts_1_170[32] = {
#include "sub_main/0046F590.inc"
};
char gHelpTexts_1_171[96] = {
#include "sub_main/0046F5B0.inc"
};
char gHelpTexts_1_172[48] = {
#include "sub_main/0046F610.inc"
};
char gHelpTexts_1_173[48] = {
#include "sub_main/0046F640.inc"
};
char gHelpTexts_1_174[64] = {
#include "sub_main/0046F670.inc"
};
char gHelpTexts_1_175[48] = {
#include "sub_main/0046F6B0.inc"
};
char gHelpTexts_1_176[32] = {
#include "sub_main/0046F6E0.inc"
};
char gHelpTexts_1_177[48] = {
#include "sub_main/0046F700.inc"
};
char gHelpTexts_1_178[64] = {
#include "sub_main/0046F730.inc"
};
char gHelpTexts_1_179[64] = {
#include "sub_main/0046F770.inc"
};
char gHelpTexts_1_180[80] = {
#include "sub_main/0046F7B0.inc"
};
char gHelpTexts_1_181[64] = {
#include "sub_main/0046F800.inc"
};
char gHelpTexts_1_182[64] = {
#include "sub_main/0046F840.inc"
};
char gHelpTexts_1_183[80] = {
#include "sub_main/0046F880.inc"
};
char gHelpTexts_1_184[80] = {
#include "sub_main/0046F8D0.inc"
};
char gHelpTexts_1_185[64] = {
#include "sub_main/0046F920.inc"
};
char gHelpTexts_1_186[64] = {
#include "sub_main/0046F960.inc"
};
char gHelpTexts_1_187[80] = {
#include "sub_main/0046F9A0.inc"
};
char gHelpTexts_1_188[80] = {
#include "sub_main/0046F9F0.inc"
};
char gHelpTexts_1_189[64] = {
#include "sub_main/0046FA40.inc"
};
char gHelpTexts_1_190[64] = {
#include "sub_main/0046FA80.inc"
};
char gHelpTexts_1_191[64] = {
#include "sub_main/0046FAC0.inc"
};
char gHelpTexts_1_194[80] = {
#include "sub_main/0046FB00.inc"
};
char gHelpTexts_1_195[32] = {
#include "sub_main/0046FB50.inc"
};
char gHelpTexts_1_196[32] = {
#include "sub_main/0046FB70.inc"
};
char gHelpTexts_1_197[80] = {
#include "sub_main/0046FB90.inc"
};
char gHelpTexts_1_198[96] = {
#include "sub_main/0046FBE0.inc"
};
char gHelpTexts_1_199[80] = {
#include "sub_main/0046FC40.inc"
};
char gHelpTexts_1_200[80] = {
#include "sub_main/0046FC90.inc"
};
char gHelpTexts_1_201[80] = {
#include "sub_main/0046FCE0.inc"
};
char gHelpTexts_1_202[32] = {
#include "sub_main/0046FD30.inc"
};
char gHelpTexts_1_203[64] = {
#include "sub_main/0046FD50.inc"
};
char gHelpTexts_1_204[80] = {
#include "sub_main/0046FD90.inc"
};
char gHelpTexts_1_205[32] = {
#include "sub_main/0046FDE0.inc"
};
char gHelpTexts_1_206[80] = {
#include "sub_main/0046FE00.inc"
};
char gHelpTexts_1_207[32] = {
#include "sub_main/0046FE50.inc"
};
char gHelpTexts_1_208[32] = {
#include "sub_main/0046FE70.inc"
};
char gHelpTexts_1_209[32] = {
#include "sub_main/0046FE90.inc"
};
char gHelpTexts_1_210[80] = {
#include "sub_main/0046FEB0.inc"
};
char gHelpTexts_1_211[48] = {
#include "sub_main/0046FF00.inc"
};
char gHelpTexts_1_212[64] = {
#include "sub_main/0046FF30.inc"
};
char gHelpTexts_1_213[80] = {
#include "sub_main/0046FF70.inc"
};
char gHelpTexts_1_216[64] = {
#include "sub_main/0046FFC0.inc"
};
char D_00470000[32] = {
#include "sub_main/00470000.inc"
};
char gHelpTexts_1_215[80] = {
#include "sub_main/00470020.inc"
};
char D_00470070[80] = {
#include "sub_main/00470070.inc"
};
char gHelpTexts_1_219[64] = {
#include "sub_main/004700C0.inc"
};
char D_00470100[16] = {
#include "sub_main/00470100.inc"
};
char gHelpTexts_1_218[32] = {
#include "sub_main/00470110.inc"
};
char D_00470130[32] = {
#include "sub_main/00470130.inc"
};
char gHelpTexts_1_220[32] = {
#include "sub_main/00470150.inc"
};
char gHelpTexts_1_221[48] = {
#include "sub_main/00470170.inc"
};
char gHelpTexts_1_222[48] = {
#include "sub_main/004701A0.inc"
};
char gHelpTexts_1_225[48] = {
#include "sub_main/004701D0.inc"
};
char D_00470200[16] = {
#include "sub_main/00470200.inc"
};
char gHelpTexts_1_224[64] = {
#include "sub_main/00470210.inc"
};
char D_00470250[96] = {
#include "sub_main/00470250.inc"
};
char gHelpTexts_1_226[80] = {
#include "sub_main/004702B0.inc"
};
char gHelpTexts_1_227[80] = {
#include "sub_main/00470300.inc"
};
char gHelpTexts_1_228[96] = {
#include "sub_main/00470350.inc"
};
char gHelpTexts_1_229[80] = {
#include "sub_main/004703B0.inc"
};
char gHelpTexts_1_230[64] = {
#include "sub_main/00470400.inc"
};
char gHelpTexts_1_231[48] = {
#include "sub_main/00470440.inc"
};
char gHelpTexts_1_232[32] = {
#include "sub_main/00470470.inc"
};
char gHelpTexts_1_233[48] = {
#include "sub_main/00470490.inc"
};
char gHelpTexts_1_234[32] = {
#include "sub_main/004704C0.inc"
};
char gHelpTexts_1_235[96] = {
#include "sub_main/004704E0.inc"
};
char gHelpTexts_1_236[96] = {
#include "sub_main/00470540.inc"
};
char gHelpTexts_1_237[48] = {
#include "sub_main/004705A0.inc"
};
char gHelpTexts_1_238[64] = {
#include "sub_main/004705D0.inc"
};
char gHelpTexts_1_239[80] = {
#include "sub_main/00470610.inc"
};
char gHelpTexts_1_240[96] = {
#include "sub_main/00470660.inc"
};
char gHelpTexts_1_241[80] = {
#include "sub_main/004706C0.inc"
};
char gHelpTexts_1_242[96] = {
#include "sub_main/00470710.inc"
};
char gHelpTexts_1_243[80] = {
#include "sub_main/00470770.inc"
};
char gHelpTexts_1_244[80] = {
#include "sub_main/004707C0.inc"
};
char gHelpTexts_1_246[64] = {
#include "sub_main/00470810.inc"
};
char gHelpTexts_1_247[80] = {
#include "sub_main/00470850.inc"
};
char gHelpTexts_1_248[80] = {
#include "sub_main/004708A0.inc"
};
char gHelpTexts_1_249[96] = {
#include "sub_main/004708F0.inc"
};
char gHelpTexts_1_250[80] = {
#include "sub_main/00470950.inc"
};
char gHelpTexts_1_252[80] = {
#include "sub_main/004709A0.inc"
};
char gHelpTexts_1_253[80] = {
#include "sub_main/004709F0.inc"
};
char gHelpTexts_1_254[80] = {
#include "sub_main/00470A40.inc"
};
char gHelpTexts_1_255[80] = {
#include "sub_main/00470A90.inc"
};
char gHelpTexts_1_256[48] = {
#include "sub_main/00470AE0.inc"
};
char gHelpTexts_1_257[64] = {
#include "sub_main/00470B10.inc"
};
char gHelpTexts_1_258[32] = {
#include "sub_main/00470B50.inc"
};
char gHelpTexts_1_259[32] = {
#include "sub_main/00470B70.inc"
};
char gHelpTexts_1_260[32] = {
#include "sub_main/00470B90.inc"
};
char gHelpTexts_1_261[32] = {
#include "sub_main/00470BB0.inc"
};
char gHelpTexts_1_262[32] = {
#include "sub_main/00470BD0.inc"
};
char gHelpTexts_1_263[32] = {
#include "sub_main/00470BF0.inc"
};
char gHelpTexts_1_264[48] = {
#include "sub_main/00470C10.inc"
};
char gHelpTexts_1_265[48] = {
#include "sub_main/00470C40.inc"
};
char gHelpTexts_1_266[64] = {
#include "sub_main/00470C70.inc"
};
char gHelpTexts_1_267[64] = {
#include "sub_main/00470CB0.inc"
};
char gHelpTexts_1_268[64] = {
#include "sub_main/00470CF0.inc"
};
char gHelpTexts_1_269[80] = {
#include "sub_main/00470D30.inc"
};
char gHelpTexts_1_270[80] = {
#include "sub_main/00470D80.inc"
};
char gHelpTexts_1_271[64] = {
#include "sub_main/00470DD0.inc"
};
char gHelpTexts_1_272[48] = {
#include "sub_main/00470E10.inc"
};
char gHelpTexts_1_273[64] = {
#include "sub_main/00470E40.inc"
};
char gHelpTexts_1_274[48] = {
#include "sub_main/00470E80.inc"
};
char gHelpTexts_1_275[80] = {
#include "sub_main/00470EB0.inc"
};
char gHelpTexts_1_276[72] = {
#include "sub_main/00470F00.inc"
};
char gHelpTexts_1_277[12] = {
#include "sub_main/00470F48.inc"
};
char gHelpTexts_1_278[80] = {
#include "sub_main/00470F60.inc"
};
char gHelpTexts_1_279[80] = {
#include "sub_main/00470FB0.inc"
};
char gHelpTexts_1_280[80] = {
#include "sub_main/00471000.inc"
};
char gHelpTexts_1_281[32] = {
#include "sub_main/00471050.inc"
};
char gHelpTexts_1_282[80] = {
#include "sub_main/00471070.inc"
};
char gHelpTexts_1_283[32] = {
#include "sub_main/004710C0.inc"
};
char gHelpTexts_1_284[80] = {
#include "sub_main/004710E0.inc"
};
char gHelpTexts_1_285[80] = {
#include "sub_main/00471130.inc"
};
char gHelpTexts_1_286[80] = {
#include "sub_main/00471180.inc"
};
char gHelpTexts_1_288[64] = {
#include "sub_main/004711D0.inc"
};
char gHelpTexts_1_289[64] = {
#include "sub_main/00471210.inc"
};
char gHelpTexts_1_290[96] = {
#include "sub_main/00471250.inc"
};
char gHelpTexts_1_291[80] = {
#include "sub_main/004712B0.inc"
};
char gHelpTexts_1_292[64] = {
#include "sub_main/00471300.inc"
};
char gHelpTexts_1_293[64] = {
#include "sub_main/00471340.inc"
};
char gHelpTexts_1_294[64] = {
#include "sub_main/00471380.inc"
};
char gHelpTexts_1_295[64] = {
#include "sub_main/004713C0.inc"
};
char gHelpTexts_1_296[64] = {
#include "sub_main/00471400.inc"
};
char gHelpTexts_1_297[64] = {
#include "sub_main/00471440.inc"
};
char gHelpTexts_1_298[80] = {
#include "sub_main/00471480.inc"
};
char gHelpTexts_1_299[80] = {
#include "sub_main/004714D0.inc"
};
char gHelpTexts_1_300[80] = {
#include "sub_main/00471520.inc"
};
char gHelpTexts_1_301[80] = {
#include "sub_main/00471570.inc"
};
char gHelpTexts_1_302[64] = {
#include "sub_main/004715C0.inc"
};
char gHelpTexts_1_303[80] = {
#include "sub_main/00471600.inc"
};
char gHelpTexts_1_304[64] = {
#include "sub_main/00471650.inc"
};
char gHelpTexts_1_305[80] = {
#include "sub_main/00471690.inc"
};
char gHelpTexts_1_306[64] = {
#include "sub_main/004716E0.inc"
};
char gHelpTexts_1_307[80] = {
#include "sub_main/00471720.inc"
};
char gHelpTexts_1_308[64] = {
#include "sub_main/00471770.inc"
};
char gHelpTexts_1_309[80] = {
#include "sub_main/004717B0.inc"
};
char gHelpTexts_1_310[80] = {
#include "sub_main/00471800.inc"
};
char gHelpTexts_1_311[80] = {
#include "sub_main/00471850.inc"
};
char gHelpTexts_1_312[64] = {
#include "sub_main/004718A0.inc"
};
char gHelpTexts_1_313[64] = {
#include "sub_main/004718E0.inc"
};
char gHelpTexts_1_314[64] = {
#include "sub_main/00471920.inc"
};
char gHelpTexts_1_315[64] = {
#include "sub_main/00471960.inc"
};
char gHelpTexts_1_316[64] = {
#include "sub_main/004719A0.inc"
};
char gHelpTexts_1_317[64] = {
#include "sub_main/004719E0.inc"
};
char gHelpTexts_1_318[64] = {
#include "sub_main/00471A20.inc"
};
char gHelpTexts_1_319[80] = {
#include "sub_main/00471A60.inc"
};
char gHelpTexts_1_320[64] = {
#include "sub_main/00471AB0.inc"
};
char gHelpTexts_1_321[80] = {
#include "sub_main/00471AF0.inc"
};
char gHelpTexts_1_322[64] = {
#include "sub_main/00471B40.inc"
};
char gHelpTexts_1_324[96] = {
#include "sub_main/00471B80.inc"
};
char gHelpTexts_1_325[80] = {
#include "sub_main/00471BE0.inc"
};
char gHelpTexts_1_326[96] = {
#include "sub_main/00471C30.inc"
};
char gHelpTexts_1_328[96] = {
#include "sub_main/00471C90.inc"
};
char gHelpTexts_1_330[80] = {
#include "sub_main/00471CF0.inc"
};
char gHelpTexts_1_331[96] = {
#include "sub_main/00471D40.inc"
};
char gHelpTexts_1_332[80] = {
#include "sub_main/00471DA0.inc"
};
char gHelpTexts_1_333[64] = {
#include "sub_main/00471DF0.inc"
};
char gHelpTexts_1_334[80] = {
#include "sub_main/00471E30.inc"
};
char gHelpTexts_1_335[96] = {
#include "sub_main/00471E80.inc"
};
char gHelpTexts_1_336[96] = {
#include "sub_main/00471EE0.inc"
};
char gHelpTexts_1_337[80] = {
#include "sub_main/00471F40.inc"
};
char gHelpTexts_1_338[48] = {
#include "sub_main/00471F90.inc"
};
char gHelpTexts_1_339[48] = {
#include "sub_main/00471FC0.inc"
};
char gHelpTexts_1_340[64] = {
#include "sub_main/00471FF0.inc"
};
char gHelpTexts_1_341[96] = {
#include "sub_main/00472030.inc"
};
char gHelpTexts_1_342[48] = {
#include "sub_main/00472090.inc"
};
char gHelpTexts_1_343[64] = {
#include "sub_main/004720C0.inc"
};
char gHelpTexts_1_344[80] = {
#include "sub_main/00472100.inc"
};
char gHelpTexts_1_345[96] = {
#include "sub_main/00472150.inc"
};
char gHelpTexts_1_346[80] = {
#include "sub_main/004721B0.inc"
};
char gHelpTexts_1_347[80] = {
#include "sub_main/00472200.inc"
};
char gHelpTexts_1_348[64] = {
#include "sub_main/00472250.inc"
};
char gHelpTexts_1_349[64] = {
#include "sub_main/00472290.inc"
};
char gHelpTexts_1_350[80] = {
#include "sub_main/004722D0.inc"
};
char gHelpTexts_1_351[96] = {
#include "sub_main/00472320.inc"
};
char gHelpTexts_1_352[80] = {
#include "sub_main/00472380.inc"
};
char gHelpTexts_1_353[32] = {
#include "sub_main/004723D0.inc"
};
char gHelpTexts_1_354[64] = {
#include "sub_main/004723F0.inc"
};
char gHelpTexts_1_355[32] = {
#include "sub_main/00472430.inc"
};
char gHelpTexts_1_356[48] = {
#include "sub_main/00472450.inc"
};
char gHelpTexts_1_357[48] = {
#include "sub_main/00472480.inc"
};
char gHelpTexts_1_358[64] = {
#include "sub_main/004724B0.inc"
};
char gHelpTexts_1_359[80] = {
#include "sub_main/004724F0.inc"
};
char gHelpTexts_1_360[64] = {
#include "sub_main/00472540.inc"
};
char gHelpTexts_1_361[80] = {
#include "sub_main/00472580.inc"
};
char gHelpTexts_1_362[80] = {
#include "sub_main/004725D0.inc"
};
char gHelpTexts_1_363[80] = {
#include "sub_main/00472620.inc"
};
char gHelpTexts_1_364[96] = {
#include "sub_main/00472670.inc"
};
char gHelpTexts_1_365[80] = {
#include "sub_main/004726D0.inc"
};
char gHelpTexts_1_366[80] = {
#include "sub_main/00472720.inc"
};
char gHelpTexts_1_367[80] = {
#include "sub_main/00472770.inc"
};
char gHelpTexts_1_368[80] = {
#include "sub_main/004727C0.inc"
};
char gHelpTexts_1_369[64] = {
#include "sub_main/00472810.inc"
};
char gHelpTexts_1_370[80] = {
#include "sub_main/00472850.inc"
};
char gHelpTexts_1_371[80] = {
#include "sub_main/004728A0.inc"
};
char gHelpTexts_1_372[80] = {
#include "sub_main/004728F0.inc"
};
char gHelpTexts_1_373[96] = {
#include "sub_main/00472940.inc"
};
char gHelpTexts_1_374[64] = {
#include "sub_main/004729A0.inc"
};
char gHelpTexts_1_375[80] = {
#include "sub_main/004729E0.inc"
};
char gHelpTexts_1_376[80] = {
#include "sub_main/00472A30.inc"
};
char gHelpTexts_1_377[96] = {
#include "sub_main/00472A80.inc"
};
char gHelpTexts_1_378[64] = {
#include "sub_main/00472AE0.inc"
};
char gHelpTexts_1_379[80] = {
#include "sub_main/00472B20.inc"
};
char gHelpTexts_1_380[96] = {
#include "sub_main/00472B70.inc"
};
char gHelpTexts_1_381[80] = {
#include "sub_main/00472BD0.inc"
};
char gHelpTexts_1_382[80] = {
#include "sub_main/00472C20.inc"
};
char gHelpTexts_1_383[96] = {
#include "sub_main/00472C70.inc"
};
char gHelpTexts_1_384[64] = {
#include "sub_main/00472CD0.inc"
};
char gHelpTexts_1_385[80] = {
#include "sub_main/00472D10.inc"
};
char gHelpTexts_1_386[64] = {
#include "sub_main/00472D60.inc"
};
char gHelpTexts_1_387[80] = {
#include "sub_main/00472DA0.inc"
};
char gHelpTexts_1_388[80] = {
#include "sub_main/00472DF0.inc"
};
char gHelpTexts_1_389[80] = {
#include "sub_main/00472E40.inc"
};
char gHelpTexts_1_390[48] = {
#include "sub_main/00472E90.inc"
};
char gHelpTexts_1_391[80] = {
#include "sub_main/00472EC0.inc"
};
char gHelpTexts_1_392[80] = {
#include "sub_main/00472F10.inc"
};
char gHelpTexts_1_393[80] = {
#include "sub_main/00472F60.inc"
};
char gHelpTexts_1_394[64] = {
#include "sub_main/00472FB0.inc"
};
char gHelpTexts_1_395[80] = {
#include "sub_main/00472FF0.inc"
};
char gHelpTexts_1_396[64] = {
#include "sub_main/00473040.inc"
};
char gHelpTexts_1_397[80] = {
#include "sub_main/00473080.inc"
};
char gHelpTexts_1_398[80] = {
#include "sub_main/004730D0.inc"
};
char gHelpTexts_1_399[80] = {
#include "sub_main/00473120.inc"
};
char gHelpTexts_1_400[48] = {
#include "sub_main/00473170.inc"
};
char gHelpTexts_1_401[64] = {
#include "sub_main/004731A0.inc"
};
char gHelpTexts_1_402[80] = {
#include "sub_main/004731E0.inc"
};
char gHelpTexts_1_403[80] = {
#include "sub_main/00473230.inc"
};
char gHelpTexts_1_404[80] = {
#include "sub_main/00473280.inc"
};
char gHelpTexts_1_405[80] = {
#include "sub_main/004732D0.inc"
};
char gHelpTexts_1_406[96] = {
#include "sub_main/00473320.inc"
};
char gHelpTexts_1_407[80] = {
#include "sub_main/00473380.inc"
};
char gHelpTexts_1_408[80] = {
#include "sub_main/004733D0.inc"
};
char gHelpTexts_1_409[80] = {
#include "sub_main/00473420.inc"
};
char gHelpTexts_1_410[80] = {
#include "sub_main/00473470.inc"
};
char gHelpTexts_1_411[64] = {
#include "sub_main/004734C0.inc"
};
char gHelpTexts_1_412[96] = {
#include "sub_main/00473500.inc"
};
char gHelpTexts_1_413[80] = {
#include "sub_main/00473560.inc"
};
char gHelpTexts_1_414[80] = {
#include "sub_main/004735B0.inc"
};
char gHelpTexts_1_415[80] = {
#include "sub_main/00473600.inc"
};
char gHelpTexts_1_416[96] = {
#include "sub_main/00473650.inc"
};
char D_004736B0[96] = {
#include "sub_main/004736B0.inc"
};
char gHelpTexts_1_418[80] = {
#include "sub_main/00473710.inc"
};
char gHelpTexts_1_419[80] = {
#include "sub_main/00473760.inc"
};
char gHelpTexts_1_420[64] = {
#include "sub_main/004737B0.inc"
};
char gHelpTexts_1_421[80] = {
#include "sub_main/004737F0.inc"
};
char gHelpTexts_1_422[64] = {
#include "sub_main/00473840.inc"
};
char gHelpTexts_1_423[96] = {
#include "sub_main/00473880.inc"
};
char gHelpTexts_1_424[80] = {
#include "sub_main/004738E0.inc"
};
char gHelpTexts_1_425[80] = {
#include "sub_main/00473930.inc"
};
char gHelpTexts_1_426[80] = {
#include "sub_main/00473980.inc"
};
char gHelpTexts_1_427[80] = {
#include "sub_main/004739D0.inc"
};
char gHelpTexts_1_428[80] = {
#include "sub_main/00473A20.inc"
};
char gHelpTexts_1_429[96] = {
#include "sub_main/00473A70.inc"
};
char gHelpTexts_1_430[64] = {
#include "sub_main/00473AD0.inc"
};
char gHelpTexts_1_431[80] = {
#include "sub_main/00473B10.inc"
};
char gHelpTexts_1_432[96] = {
#include "sub_main/00473B60.inc"
};
char gHelpTexts_1_434[80] = {
#include "sub_main/00473BC0.inc"
};
char gHelpTexts_1_435[80] = {
#include "sub_main/00473C10.inc"
};
char gHelpTexts_1_436[64] = {
#include "sub_main/00473C60.inc"
};
char gHelpTexts_1_437[80] = {
#include "sub_main/00473CA0.inc"
};
char gHelpTexts_1_438[80] = {
#include "sub_main/00473CF0.inc"
};
char gHelpTexts_1_439[80] = {
#include "sub_main/00473D40.inc"
};
char gHelpTexts_1_440[96] = {
#include "sub_main/00473D90.inc"
};
char gHelpTexts_1_441[80] = {
#include "sub_main/00473DF0.inc"
};
char gHelpTexts_1_442[80] = {
#include "sub_main/00473E40.inc"
};
char gHelpTexts_1_443[80] = {
#include "sub_main/00473E90.inc"
};
char gHelpTexts_1_444[64] = {
#include "sub_main/00473EE0.inc"
};
char gHelpTexts_1_445[80] = {
#include "sub_main/00473F20.inc"
};
char gHelpTexts_1_446[80] = {
#include "sub_main/00473F70.inc"
};
char gHelpTexts_1_447[80] = {
#include "sub_main/00473FC0.inc"
};
char gHelpTexts_1_448[80] = {
#include "sub_main/00474010.inc"
};
char gHelpTexts_1_449[80] = {
#include "sub_main/00474060.inc"
};
char gHelpTexts_1_450[80] = {
#include "sub_main/004740B0.inc"
};
char gHelpTexts_1_451[80] = {
#include "sub_main/00474100.inc"
};
char gHelpTexts_1_452[48] = {
#include "sub_main/00474150.inc"
};
char gHelpTexts_1_453[80] = {
#include "sub_main/00474180.inc"
};
char gHelpTexts_1_454[80] = {
#include "sub_main/004741D0.inc"
};
char gHelpTexts_1_455[80] = {
#include "sub_main/00474220.inc"
};
char gHelpTexts_1_456[80] = {
#include "sub_main/00474270.inc"
};
char gHelpTexts_1_457[80] = {
#include "sub_main/004742C0.inc"
};
char gHelpTexts_1_458[64] = {
#include "sub_main/00474310.inc"
};
char gHelpTexts_1_459[64] = {
#include "sub_main/00474350.inc"
};
char gHelpTexts_1_460[80] = {
#include "sub_main/00474390.inc"
};
char gHelpTexts_1_461[80] = {
#include "sub_main/004743E0.inc"
};
char gHelpTexts_1_462[80] = {
#include "sub_main/00474430.inc"
};
char gHelpTexts_1_463[80] = {
#include "sub_main/00474480.inc"
};
char gHelpTexts_1_464[96] = {
#include "sub_main/004744D0.inc"
};
char gHelpTexts_1_465[80] = {
#include "sub_main/00474530.inc"
};
char gHelpTexts_1_466[80] = {
#include "sub_main/00474580.inc"
};
char gHelpTexts_1_467[80] = {
#include "sub_main/004745D0.inc"
};
char gHelpTexts_1_468[80] = {
#include "sub_main/00474620.inc"
};
char gHelpTexts_1_469[80] = {
#include "sub_main/00474670.inc"
};
char gHelpTexts_1_470[96] = {
#include "sub_main/004746C0.inc"
};
char gHelpTexts_1_471[64] = {
#include "sub_main/00474720.inc"
};
char gHelpTexts_1_472[80] = {
#include "sub_main/00474760.inc"
};
char gHelpTexts_1_473[80] = {
#include "sub_main/004747B0.inc"
};
char gHelpTexts_1_474[80] = {
#include "sub_main/00474800.inc"
};
char gHelpTexts_1_475[80] = {
#include "sub_main/00474850.inc"
};
char gHelpTexts_1_476[96] = {
#include "sub_main/004748A0.inc"
};
char gHelpTexts_1_477[80] = {
#include "sub_main/00474900.inc"
};
char gHelpTexts_1_478[80] = {
#include "sub_main/00474950.inc"
};
char gHelpTexts_1_479[96] = {
#include "sub_main/004749A0.inc"
};
char D_00474A00[96] = {
#include "sub_main/00474A00.inc"
};
char gHelpTexts_1_481[80] = {
#include "sub_main/00474A60.inc"
};
char gHelpTexts_1_482[64] = {
#include "sub_main/00474AB0.inc"
};
char gHelpTexts_1_483[96] = {
#include "sub_main/00474AF0.inc"
};
char gHelpTexts_1_485[80] = {
#include "sub_main/00474B50.inc"
};
char gHelpTexts_1_486[80] = {
#include "sub_main/00474BA0.inc"
};
char gHelpTexts_1_487[96] = {
#include "sub_main/00474BF0.inc"
};
char gHelpTexts_1_488[96] = {
#include "sub_main/00474C50.inc"
};
char gHelpTexts_1_489[80] = {
#include "sub_main/00474CB0.inc"
};
char gHelpTexts_1_490[80] = {
#include "sub_main/00474D00.inc"
};
char gHelpTexts_1_491[80] = {
#include "sub_main/00474D50.inc"
};
char gHelpTexts_1_492[80] = {
#include "sub_main/00474DA0.inc"
};
char gHelpTexts_1_493[80] = {
#include "sub_main/00474DF0.inc"
};
char gHelpTexts_1_494[80] = {
#include "sub_main/00474E40.inc"
};
char gHelpTexts_1_495[64] = {
#include "sub_main/00474E90.inc"
};
char gHelpTexts_1_496[96] = {
#include "sub_main/00474ED0.inc"
};
char gHelpTexts_1_497[80] = {
#include "sub_main/00474F30.inc"
};
char gHelpTexts_1_498[80] = {
#include "sub_main/00474F80.inc"
};
char gHelpTexts_1_499[64] = {
#include "sub_main/00474FD0.inc"
};
char gHelpTexts_1_500[80] = {
#include "sub_main/00475010.inc"
};
char gHelpTexts_1_501[80] = {
#include "sub_main/00475060.inc"
};
char gHelpTexts_1_502[96] = {
#include "sub_main/004750B0.inc"
};
char gHelpTexts_1_503[80] = {
#include "sub_main/00475110.inc"
};
char gHelpTexts_1_504[80] = {
#include "sub_main/00475160.inc"
};
char gHelpTexts_1_505[80] = {
#include "sub_main/004751B0.inc"
};
char gHelpTexts_1_506[96] = {
#include "sub_main/00475200.inc"
};
char gHelpTexts_1_507[80] = {
#include "sub_main/00475260.inc"
};
char gHelpTexts_1_508[80] = {
#include "sub_main/004752B0.inc"
};
char gHelpTexts_1_509[64] = {
#include "sub_main/00475300.inc"
};
char gHelpTexts_1_510[80] = {
#include "sub_main/00475340.inc"
};
char gHelpTexts_1_511[96] = {
#include "sub_main/00475390.inc"
};
char gHelpTexts_1_512[96] = {
#include "sub_main/004753F0.inc"
};
char gHelpTexts_1_513[80] = {
#include "sub_main/00475450.inc"
};
char gHelpTexts_1_514[64] = {
#include "sub_main/004754A0.inc"
};
char gHelpTexts_1_515[96] = {
#include "sub_main/004754E0.inc"
};
char gHelpTexts_1_516[96] = {
#include "sub_main/00475540.inc"
};
char gHelpTexts_1_517[80] = {
#include "sub_main/004755A0.inc"
};
char gHelpTexts_1_518[80] = {
#include "sub_main/004755F0.inc"
};
char gHelpTexts_1_519[80] = {
#include "sub_main/00475640.inc"
};
char gHelpTexts_1_520[96] = {
#include "sub_main/00475690.inc"
};
char gHelpTexts_1_521[80] = {
#include "sub_main/004756F0.inc"
};
char gHelpTexts_1_522[80] = {
#include "sub_main/00475740.inc"
};
char gHelpTexts_1_523[64] = {
#include "sub_main/00475790.inc"
};
char gHelpTexts_1_524[80] = {
#include "sub_main/004757D0.inc"
};
char gHelpTexts_1_525[64] = {
#include "sub_main/00475820.inc"
};
char gHelpTexts_1_526[80] = {
#include "sub_main/00475860.inc"
};
char gHelpTexts_1_529[80] = {
#include "sub_main/004758B0.inc"
};
char gHelpTexts_1_530[80] = {
#include "sub_main/00475900.inc"
};
char gHelpTexts_1_531[96] = {
#include "sub_main/00475950.inc"
};
char gHelpTexts_1_532[96] = {
#include "sub_main/004759B0.inc"
};
char gHelpTexts_1_533[80] = {
#include "sub_main/00475A10.inc"
};
char gHelpTexts_1_534[64] = {
#include "sub_main/00475A60.inc"
};
char gHelpTexts_1_535[80] = {
#include "sub_main/00475AA0.inc"
};
char gHelpTexts_1_536[80] = {
#include "sub_main/00475AF0.inc"
};
char gHelpTexts_1_537[64] = {
#include "sub_main/00475B40.inc"
};
char gHelpTexts_1_538[48] = {
#include "sub_main/00475B80.inc"
};
char gHelpTexts_1_539[64] = {
#include "sub_main/00475BB0.inc"
};
char gHelpTexts_1_540[80] = {
#include "sub_main/00475BF0.inc"
};
char gHelpTexts_1_541[80] = {
#include "sub_main/00475C40.inc"
};
char gHelpTexts_1_543[80] = {
#include "sub_main/00475C90.inc"
};
char gHelpTexts_1_544[80] = {
#include "sub_main/00475CE0.inc"
};
char gHelpTexts_1_545[80] = {
#include "sub_main/00475D30.inc"
};
char gHelpTexts_1_546[80] = {
#include "sub_main/00475D80.inc"
};
char gHelpTexts_1_547[80] = {
#include "sub_main/00475DD0.inc"
};
char gHelpTexts_1_548[80] = {
#include "sub_main/00475E20.inc"
};
char gHelpTexts_1_549[80] = {
#include "sub_main/00475E70.inc"
};
char gHelpTexts_1_550[48] = {
#include "sub_main/00475EC0.inc"
};
char gHelpTexts_1_551[80] = {
#include "sub_main/00475EF0.inc"
};
char gHelpTexts_1_552[80] = {
#include "sub_main/00475F40.inc"
};
char gHelpTexts_1_553[48] = {
#include "sub_main/00475F90.inc"
};
char gHelpTexts_1_554[64] = {
#include "sub_main/00475FC0.inc"
};
char gHelpTexts_1_555[64] = {
#include "sub_main/00476000.inc"
};
char gHelpTexts_1_557[64] = {
#include "sub_main/00476040.inc"
};
char gHelpTexts_1_558[48] = {
#include "sub_main/00476080.inc"
};
char gHelpTexts_1_559[64] = {
#include "sub_main/004760B0.inc"
};
char gHelpTexts_1_560[64] = {
#include "sub_main/004760F0.inc"
};
char gHelpTexts_1_561[64] = {
#include "sub_main/00476130.inc"
};
char gHelpTexts_1_562[64] = {
#include "sub_main/00476170.inc"
};
char gHelpTexts_1_563[64] = {
#include "sub_main/004761B0.inc"
};
char gHelpTexts_1_565[80] = {
#include "sub_main/004761F0.inc"
};
char gHelpTexts_1_566[96] = {
#include "sub_main/00476240.inc"
};
char gHelpTexts_1_567[80] = {
#include "sub_main/004762A0.inc"
};
char gHelpTexts_1_568[64] = {
#include "sub_main/004762F0.inc"
};
char gHelpTexts_1_570[64] = {
#include "sub_main/00476330.inc"
};
char gHelpTexts_1_571[80] = {
#include "sub_main/00476370.inc"
};
char gHelpTexts_1_572[80] = {
#include "sub_main/004763C0.inc"
};
char gHelpTexts_1_573[80] = {
#include "sub_main/00476410.inc"
};
char gHelpTexts_1_574[96] = {
#include "sub_main/00476460.inc"
};
char gHelpTexts_1_575[80] = {
#include "sub_main/004764C0.inc"
};
char gHelpTexts_1_576[80] = {
#include "sub_main/00476510.inc"
};
char gHelpTexts_1_577[80] = {
#include "sub_main/00476560.inc"
};
char gHelpTexts_1_578[80] = {
#include "sub_main/004765B0.inc"
};
char gHelpTexts_1_579[64] = {
#include "sub_main/00476600.inc"
};
char gHelpTexts_1_580[48] = {
#include "sub_main/00476640.inc"
};
char gHelpTexts_1_581[80] = {
#include "sub_main/00476670.inc"
};
char gHelpTexts_1_582[64] = {
#include "sub_main/004766C0.inc"
};
char gHelpTexts_1_583[80] = {
#include "sub_main/00476700.inc"
};
char gHelpTexts_1_584[80] = {
#include "sub_main/00476750.inc"
};
char gHelpTexts_1_585[80] = {
#include "sub_main/004767A0.inc"
};
char gHelpTexts_1_586[80] = {
#include "sub_main/004767F0.inc"
};
char gHelpTexts_1_587[80] = {
#include "sub_main/00476840.inc"
};
char gHelpTexts_1_588[64] = {
#include "sub_main/00476890.inc"
};
char gHelpTexts_1_589[80] = {
#include "sub_main/004768D0.inc"
};
char gHelpTexts_1_591[32] = {
#include "sub_main/00476920.inc"
};
char gHelpTexts_1_592[80] = {
#include "sub_main/00476940.inc"
};
char gHelpTexts_1_593[80] = {
#include "sub_main/00476990.inc"
};
char gHelpTexts_1_594[96] = {
#include "sub_main/004769E0.inc"
};
char gHelpTexts_1_595[80] = {
#include "sub_main/00476A40.inc"
};
char gHelpTexts_1_596[48] = {
#include "sub_main/00476A90.inc"
};
char gHelpTexts_1_597[64] = {
#include "sub_main/00476AC0.inc"
};
char gHelpTexts_1_598[80] = {
#include "sub_main/00476B00.inc"
};
char gHelpTexts_1_599[80] = {
#include "sub_main/00476B50.inc"
};
char gHelpTexts_1_600[80] = {
#include "sub_main/00476BA0.inc"
};
char gHelpTexts_1_601[64] = {
#include "sub_main/00476BF0.inc"
};
char gHelpTexts_1_603[64] = {
#include "sub_main/00476C30.inc"
};
char gHelpTexts_1_604[80] = {
#include "sub_main/00476C70.inc"
};
char gHelpTexts_1_605[80] = {
#include "sub_main/00476CC0.inc"
};
char gHelpTexts_1_606[80] = {
#include "sub_main/00476D10.inc"
};
char gHelpTexts_1_607[80] = {
#include "sub_main/00476D60.inc"
};
char gHelpTexts_1_608[80] = {
#include "sub_main/00476DB0.inc"
};
char gHelpTexts_1_609[80] = {
#include "sub_main/00476E00.inc"
};
char gHelpTexts_1_610[80] = {
#include "sub_main/00476E50.inc"
};
char gHelpTexts_1_611[96] = {
#include "sub_main/00476EA0.inc"
};
char gHelpTexts_1_612[64] = {
#include "sub_main/00476F00.inc"
};
char gHelpTexts_1_613[80] = {
#include "sub_main/00476F40.inc"
};
char gHelpTexts_1_614[80] = {
#include "sub_main/00476F90.inc"
};
char gHelpTexts_1_616[64] = {
#include "sub_main/00476FE0.inc"
};
char gHelpTexts_1_617[48] = {
#include "sub_main/00477020.inc"
};
char gHelpTexts_1_618[80] = {
#include "sub_main/00477050.inc"
};
char gHelpTexts_1_619[64] = {
#include "sub_main/004770A0.inc"
};
char gHelpTexts_1_620[48] = {
#include "sub_main/004770E0.inc"
};
char gHelpTexts_1_621[80] = {
#include "sub_main/00477110.inc"
};
char gHelpTexts_1_622[80] = {
#include "sub_main/00477160.inc"
};
char gHelpTexts_1_623[96] = {
#include "sub_main/004771B0.inc"
};
char gHelpTexts_1_624[80] = {
#include "sub_main/00477210.inc"
};
char gHelpTexts_1_625[80] = {
#include "sub_main/00477260.inc"
};
char gHelpTexts_1_626[80] = {
#include "sub_main/004772B0.inc"
};
char gHelpTexts_1_627[80] = {
#include "sub_main/00477300.inc"
};
char gHelpTexts_1_628[96] = {
#include "sub_main/00477350.inc"
};
char gHelpTexts_1_629[64] = {
#include "sub_main/004773B0.inc"
};
char gHelpTexts_1_630[64] = {
#include "sub_main/004773F0.inc"
};
char gHelpTexts_1_631[40] = {
#include "sub_main/00477430.inc"
};
char gHelpTexts_1_632[12] = {
#include "sub_main/00477458.inc"
};
char D_00477470[96] = {
#include "sub_main/00477470.inc"
};
char D_004774D0[80] = {
#include "sub_main/004774D0.inc"
};
char gHelpTexts_1_643[96] = {
#include "sub_main/00477520.inc"
};
char gHelpTexts_1_644[96] = {
#include "sub_main/00477580.inc"
};
char gHelpTexts_1_645[96] = {
#include "sub_main/004775E0.inc"
};
char gHelpTexts_1_646[80] = {
#include "sub_main/00477640.inc"
};
char gHelpTexts_1_647[80] = {
#include "sub_main/00477690.inc"
};
char gHelpTexts_1_648[96] = {
#include "sub_main/004776E0.inc"
};
char gHelpTexts_1_649[80] = {
#include "sub_main/00477740.inc"
};
char gHelpTexts_1_650[80] = {
#include "sub_main/00477790.inc"
};
char gHelpTexts_1_651[80] = {
#include "sub_main/004777E0.inc"
};
char gHelpTexts_1_652[80] = {
#include "sub_main/00477830.inc"
};
char gHelpTexts_1_653[80] = {
#include "sub_main/00477880.inc"
};
char D_004778D0[80] = {
#include "sub_main/004778D0.inc"
};
char gHelpTexts_1_656[96] = {
#include "sub_main/00477920.inc"
};
char gHelpTexts_1_657[96] = {
#include "sub_main/00477980.inc"
};
char gHelpTexts_1_658[80] = {
#include "sub_main/004779E0.inc"
};
char D_00477A30[80] = {
#include "sub_main/00477A30.inc"
};
char gHelpTexts_1_661[80] = {
#include "sub_main/00477A80.inc"
};
char gHelpTexts_1_662[80] = {
#include "sub_main/00477AD0.inc"
};
char gHelpTexts_1_663[80] = {
#include "sub_main/00477B20.inc"
};
char gHelpTexts_1_664[96] = {
#include "sub_main/00477B70.inc"
};
char gHelpTexts_1_665[80] = {
#include "sub_main/00477BD0.inc"
};
char gHelpTexts_1_666[80] = {
#include "sub_main/00477C20.inc"
};
char gHelpTexts_1_667[96] = {
#include "sub_main/00477C70.inc"
};
char gHelpTexts_1_668[80] = {
#include "sub_main/00477CD0.inc"
};
char gHelpTexts_1_669[80] = {
#include "sub_main/00477D20.inc"
};
char gHelpTexts_1_670[80] = {
#include "sub_main/00477D70.inc"
};
char gHelpTexts_1_671[96] = {
#include "sub_main/00477DC0.inc"
};
char gHelpTexts_1_672[96] = {
#include "sub_main/00477E20.inc"
};
char gHelpTexts_1_677[96] = {
#include "sub_main/00477E80.inc"
};
char gHelpTexts_1_678[96] = {
#include "sub_main/00477EE0.inc"
};
char gHelpTexts_1_679[64] = {
#include "sub_main/00477F40.inc"
};
char gHelpTexts_1_680[48] = {
#include "sub_main/00477F80.inc"
};
char gHelpTexts_1_681[48] = {
#include "sub_main/00477FB0.inc"
};
char gHelpTexts_1_682[80] = {
#include "sub_main/00477FE0.inc"
};
char gHelpTexts_1_683[80] = {
#include "sub_main/00478030.inc"
};
char gHelpTexts_1_684[80] = {
#include "sub_main/00478080.inc"
};
char gHelpTexts_1_685[48] = {
#include "sub_main/004780D0.inc"
};
char gHelpTexts_1_686[80] = {
#include "sub_main/00478100.inc"
};
char gHelpTexts_1_687[64] = {
#include "sub_main/00478150.inc"
};
char gHelpTexts_1_688[80] = {
#include "sub_main/00478190.inc"
};
char gHelpTexts_1_689[48] = {
#include "sub_main/004781E0.inc"
};
char gHelpTexts_1_690[80] = {
#include "sub_main/00478210.inc"
};
char gHelpTexts_1_691[16] = {
#include "sub_main/00478260.inc"
};
char gHelpTexts_1_692[32] = {
#include "sub_main/00478270.inc"
};
char gHelpTexts_1_693[64] = {
#include "sub_main/00478290.inc"
};
char gHelpTexts_1_694[64] = {
#include "sub_main/004782D0.inc"
};
char gHelpTexts_1_695[32] = {
#include "sub_main/00478310.inc"
};
char gHelpTexts_1_696[32] = {
#include "sub_main/00478330.inc"
};
char gHelpTexts_1_697[64] = {
#include "sub_main/00478350.inc"
};
char gHelpTexts_1_698[80] = {
#include "sub_main/00478390.inc"
};
char gHelpTexts_1_699[80] = {
#include "sub_main/004783E0.inc"
};
char gHelpTexts_1_700[80] = {
#include "sub_main/00478430.inc"
};
char gHelpTexts_1_701[64] = {
#include "sub_main/00478480.inc"
};
char gHelpTexts_1_702[80] = {
#include "sub_main/004784C0.inc"
};
char gHelpTexts_1_703[64] = {
#include "sub_main/00478510.inc"
};
char gHelpTexts_1_704[64] = {
#include "sub_main/00478550.inc"
};
char gHelpTexts_1_705[80] = {
#include "sub_main/00478590.inc"
};
char gHelpTexts_1_706[64] = {
#include "sub_main/004785E0.inc"
};
char gHelpTexts_1_707[80] = {
#include "sub_main/00478620.inc"
};
char gHelpTexts_1_708[80] = {
#include "sub_main/00478670.inc"
};
char gHelpTexts_1_709[80] = {
#include "sub_main/004786C0.inc"
};
char gHelpTexts_1_710[64] = {
#include "sub_main/00478710.inc"
};
char gHelpTexts_1_711[64] = {
#include "sub_main/00478750.inc"
};
char gHelpTexts_1_712[64] = {
#include "sub_main/00478790.inc"
};
char gHelpTexts_1_713[80] = {
#include "sub_main/004787D0.inc"
};
char gHelpTexts_1_714[64] = {
#include "sub_main/00478820.inc"
};
char gHelpTexts_1_715[64] = {
#include "sub_main/00478860.inc"
};
char gHelpTexts_1_716[64] = {
#include "sub_main/004788A0.inc"
};
char gHelpTexts_1_717[64] = {
#include "sub_main/004788E0.inc"
};
char gHelpTexts_1_718[80] = {
#include "sub_main/00478920.inc"
};
char gHelpTexts_1_720[80] = {
#include "sub_main/00478970.inc"
};
char gHelpTexts_1_721[80] = {
#include "sub_main/004789C0.inc"
};
char gHelpTexts_1_722[80] = {
#include "sub_main/00478A10.inc"
};
char D_00478A60[80] = {
#include "sub_main/00478A60.inc"
};
char gHelpTexts_1_727[80] = {
#include "sub_main/00478AB0.inc"
};
char gHelpTexts_1_728[80] = {
#include "sub_main/00478B00.inc"
};
char gHelpTexts_1_729[64] = {
#include "sub_main/00478B50.inc"
};
char gHelpTexts_1_730[64] = {
#include "sub_main/00478B90.inc"
};
char gHelpTexts_1_731[64] = {
#include "sub_main/00478BD0.inc"
};
char D_00478C10[80] = {
#include "sub_main/00478C10.inc"
};
char gHelpTexts_1_738[80] = {
#include "sub_main/00478C60.inc"
};
char gHelpTexts_1_739[80] = {
#include "sub_main/00478CB0.inc"
};
char gHelpTexts_1_740[80] = {
#include "sub_main/00478D00.inc"
};
char gHelpTexts_1_741[80] = {
#include "sub_main/00478D50.inc"
};
char gHelpTexts_1_742[96] = {
#include "sub_main/00478DA0.inc"
};
char gHelpTexts_1_743[80] = {
#include "sub_main/00478E00.inc"
};
char gHelpTexts_1_744[96] = {
#include "sub_main/00478E50.inc"
};
char gHelpTexts_1_745[80] = {
#include "sub_main/00478EB0.inc"
};
char gHelpTexts_1_746[80] = {
#include "sub_main/00478F00.inc"
};
char gHelpTexts_1_747[80] = {
#include "sub_main/00478F50.inc"
};
char gHelpTexts_1_748[96] = {
#include "sub_main/00478FA0.inc"
};
char gHelpTexts_1_749[96] = {
#include "sub_main/00479000.inc"
};
char gHelpTexts_1_750[96] = {
#include "sub_main/00479060.inc"
};
char gHelpTexts_1_751[96] = {
#include "sub_main/004790C0.inc"
};
char gHelpTexts_1_752[96] = {
#include "sub_main/00479120.inc"
};
char gHelpTexts_1_753[80] = {
#include "sub_main/00479180.inc"
};
char gHelpTexts_1_754[64] = {
#include "sub_main/004791D0.inc"
};
char gHelpTexts_1_755[64] = {
#include "sub_main/00479210.inc"
};
char gHelpTexts_1_756[80] = {
#include "sub_main/00479250.inc"
};
char gHelpTexts_1_757[64] = {
#include "sub_main/004792A0.inc"
};
char gHelpTexts_1_758[96] = {
#include "sub_main/004792E0.inc"
};
char gHelpTexts_1_759[32] = {
#include "sub_main/00479340.inc"
};
char gHelpTexts_1_760[80] = {
#include "sub_main/00479360.inc"
};
char gHelpTexts_1_761[96] = {
#include "sub_main/004793B0.inc"
};
char gHelpTexts_1_762[80] = {
#include "sub_main/00479410.inc"
};
char gHelpTexts_1_763[48] = {
#include "sub_main/00479460.inc"
};
char gHelpTexts_1_764[96] = {
#include "sub_main/00479490.inc"
};
char gHelpTexts_1_765[80] = {
#include "sub_main/004794F0.inc"
};
char gHelpTexts_1_766[80] = {
#include "sub_main/00479540.inc"
};
char gHelpTexts_1_767[80] = {
#include "sub_main/00479590.inc"
};
char gHelpTexts_1_768[80] = {
#include "sub_main/004795E0.inc"
};
char gHelpTexts_1_769[48] = {
#include "sub_main/00479630.inc"
};
char gHelpTexts_1_770[80] = {
#include "sub_main/00479660.inc"
};
char gHelpTexts_1_771[80] = {
#include "sub_main/004796B0.inc"
};
char gHelpTexts_1_772[80] = {
#include "sub_main/00479700.inc"
};
char gHelpTexts_1_773[80] = {
#include "sub_main/00479750.inc"
};
char gHelpTexts_1_774[80] = {
#include "sub_main/004797A0.inc"
};
char gHelpTexts_1_775[80] = {
#include "sub_main/004797F0.inc"
};
char gHelpTexts_1_776[80] = {
#include "sub_main/00479840.inc"
};
char gHelpTexts_1_777[80] = {
#include "sub_main/00479890.inc"
};
char gHelpTexts_1_778[80] = {
#include "sub_main/004798E0.inc"
};
char gHelpTexts_1_779[80] = {
#include "sub_main/00479930.inc"
};
char gHelpTexts_1_780[96] = {
#include "sub_main/00479980.inc"
};
char gHelpTexts_1_781[64] = {
#include "sub_main/004799E0.inc"
};
char gHelpTexts_1_782[64] = {
#include "sub_main/00479A20.inc"
};
char gHelpTexts_1_783[64] = {
#include "sub_main/00479A60.inc"
};
char gHelpTexts_1_784[64] = {
#include "sub_main/00479AA0.inc"
};
char gHelpTexts_1_785[80] = {
#include "sub_main/00479AE0.inc"
};
char gHelpTexts_1_786[64] = {
#include "sub_main/00479B30.inc"
};
char gHelpTexts_1_787[80] = {
#include "sub_main/00479B70.inc"
};
char gHelpTexts_1_788[80] = {
#include "sub_main/00479BC0.inc"
};
char gHelpTexts_1_789[64] = {
#include "sub_main/00479C10.inc"
};
char gHelpTexts_1_790[64] = {
#include "sub_main/00479C50.inc"
};
char gHelpTexts_1_791[64] = {
#include "sub_main/00479C90.inc"
};
char gHelpTexts_1_792[80] = {
#include "sub_main/00479CD0.inc"
};
char gHelpTexts_1_793[80] = {
#include "sub_main/00479D20.inc"
};
char gHelpTexts_1_794[64] = {
#include "sub_main/00479D70.inc"
};
char gHelpTexts_1_795[64] = {
#include "sub_main/00479DB0.inc"
};
char gHelpTexts_1_796[64] = {
#include "sub_main/00479DF0.inc"
};
char gHelpTexts_1_797[64] = {
#include "sub_main/00479E30.inc"
};
char gHelpTexts_1_798[64] = {
#include "sub_main/00479E70.inc"
};
char gHelpTexts_1_799[64] = {
#include "sub_main/00479EB0.inc"
};
char gHelpTexts_1_800[64] = {
#include "sub_main/00479EF0.inc"
};
char gHelpTexts_1_801[64] = {
#include "sub_main/00479F30.inc"
};
char gHelpTexts_1_802[64] = {
#include "sub_main/00479F70.inc"
};
char gHelpTexts_1_803[80] = {
#include "sub_main/00479FB0.inc"
};
char gHelpTexts_1_804[64] = {
#include "sub_main/0047A000.inc"
};
char gHelpTexts_1_805[80] = {
#include "sub_main/0047A040.inc"
};
char gHelpTexts_1_806[80] = {
#include "sub_main/0047A090.inc"
};
char gHelpTexts_1_807[80] = {
#include "sub_main/0047A0E0.inc"
};
char gHelpTexts_1_808[80] = {
#include "sub_main/0047A130.inc"
};
char gHelpTexts_1_809[80] = {
#include "sub_main/0047A180.inc"
};
char gHelpTexts_1_810[64] = {
#include "sub_main/0047A1D0.inc"
};
char gHelpTexts_1_811[80] = {
#include "sub_main/0047A210.inc"
};
char gHelpTexts_1_812[80] = {
#include "sub_main/0047A260.inc"
};
char gHelpTexts_1_813[80] = {
#include "sub_main/0047A2B0.inc"
};
char gHelpTexts_1_814[64] = {
#include "sub_main/0047A300.inc"
};
char gHelpTexts_1_815[80] = {
#include "sub_main/0047A340.inc"
};
char D_0047A390[48] = {
#include "sub_main/0047A390.inc"
};
char gHelpTexts_1_818[64] = {
#include "sub_main/0047A3C0.inc"
};
char gHelpTexts_1_819[64] = {
#include "sub_main/0047A400.inc"
};
char gHelpTexts_1_820[32] = {
#include "sub_main/0047A440.inc"
};
char D_0047A460[32] = {
#include "sub_main/0047A460.inc"
};
char D_0047A480[32] = {
#include "sub_main/0047A480.inc"
};
char D_0047A4A0[32] = {
#include "sub_main/0047A4A0.inc"
};
char D_0047A4C0[32] = {
#include "sub_main/0047A4C0.inc"
};
char D_0047A4E0[48] = {
#include "sub_main/0047A4E0.inc"
};
char gHelpTexts_1_828[32] = {
#include "sub_main/0047A510.inc"
};
char gHelpTexts_1_829[32] = {
#include "sub_main/0047A530.inc"
};
char gHelpTexts_1_830[32] = {
#include "sub_main/0047A550.inc"
};
char D_0047A570[32] = {
#include "sub_main/0047A570.inc"
};
char D_0047A590[32] = {
#include "sub_main/0047A590.inc"
};
char D_0047A5B0[32] = {
#include "sub_main/0047A5B0.inc"
};
char gHelpTexts_1_834[32] = {
#include "sub_main/0047A5D0.inc"
};
char D_0047A5F0[32] = {
#include "sub_main/0047A5F0.inc"
};
char gHelpTexts_1_837[32] = {
#include "sub_main/0047A610.inc"
};
char D_0047A630[48] = {
#include "sub_main/0047A630.inc"
};
char D_0047A660[32] = {
#include "sub_main/0047A660.inc"
};
char gHelpTexts_1_840[48] = {
#include "sub_main/0047A680.inc"
};
char D_0047A6B0[32] = {
#include "sub_main/0047A6B0.inc"
};
char gHelpTexts_1_842[48] = {
#include "sub_main/0047A6D0.inc"
};
char D_0047A700[48] = {
#include "sub_main/0047A700.inc"
};
char gHelpTexts_1_844[48] = {
#include "sub_main/0047A730.inc"
};
char gHelpTexts_1_845[48] = {
#include "sub_main/0047A760.inc"
};
char gHelpTexts_1_846[48] = {
#include "sub_main/0047A790.inc"
};
char gHelpTexts_1_847[32] = {
#include "sub_main/0047A7C0.inc"
};
char D_0047A7E0[48] = {
#include "sub_main/0047A7E0.inc"
};
char gHelpTexts_1_849[32] = {
#include "sub_main/0047A810.inc"
};
char D_0047A830[32] = {
#include "sub_main/0047A830.inc"
};
char gHelpTexts_1_851[32] = {
#include "sub_main/0047A850.inc"
};
char D_0047A870[48] = {
#include "sub_main/0047A870.inc"
};
char gHelpTexts_1_853[48] = {
#include "sub_main/0047A8A0.inc"
};
char gHelpTexts_1_854[48] = {
#include "sub_main/0047A8D0.inc"
};
char gHelpTexts_1_855[48] = {
#include "sub_main/0047A900.inc"
};
char gHelpTexts_1_856[48] = {
#include "sub_main/0047A930.inc"
};
char gHelpTexts_1_857[64] = {
#include "sub_main/0047A960.inc"
};
char D_0047A9A0[64] = {
#include "sub_main/0047A9A0.inc"
};
char gHelpTexts_1_859[64] = {
#include "sub_main/0047A9E0.inc"
};
char gHelpTexts_1_860[64] = {
#include "sub_main/0047AA20.inc"
};
char gHelpTexts_1_861[64] = {
#include "sub_main/0047AA60.inc"
};
char gHelpTexts_1_862[64] = {
#include "sub_main/0047AAA0.inc"
};
char D_0047AAE0[64] = {
#include "sub_main/0047AAE0.inc"
};
char D_0047AB20[64] = {
#include "sub_main/0047AB20.inc"
};
char D_0047AB60[64] = {
#include "sub_main/0047AB60.inc"
};
char D_0047ABA0[64] = {
#include "sub_main/0047ABA0.inc"
};
char D_0047ABE0[64] = {
#include "sub_main/0047ABE0.inc"
};
char gHelpTexts_1_868[48] = {
#include "sub_main/0047AC20.inc"
};
char gHelpTexts_1_869[48] = {
#include "sub_main/0047AC50.inc"
};
char gHelpTexts_1_870[48] = {
#include "sub_main/0047AC80.inc"
};
char D_0047ACB0[48] = {
#include "sub_main/0047ACB0.inc"
};
char D_0047ACE0[32] = {
#include "sub_main/0047ACE0.inc"
};
char D_0047AD00[48] = {
#include "sub_main/0047AD00.inc"
};
char D_0047AD30[32] = {
#include "sub_main/0047AD30.inc"
};
char D_0047AD50[48] = {
#include "sub_main/0047AD50.inc"
};
char D_0047AD80[48] = {
#include "sub_main/0047AD80.inc"
};
char D_0047ADB0[48] = {
#include "sub_main/0047ADB0.inc"
};
char D_0047ADE0[48] = {
#include "sub_main/0047ADE0.inc"
};
char D_0047AE10[48] = {
#include "sub_main/0047AE10.inc"
};
char D_0047AE40[48] = {
#include "sub_main/0047AE40.inc"
};
char D_0047AE70[32] = {
#include "sub_main/0047AE70.inc"
};
char D_0047AE90[48] = {
#include "sub_main/0047AE90.inc"
};
char D_0047AEC0[48] = {
#include "sub_main/0047AEC0.inc"
};
char D_0047AEF0[48] = {
#include "sub_main/0047AEF0.inc"
};
char gHelpTexts_1_887[32] = {
#include "sub_main/0047AF20.inc"
};
char D_0047AF40[32] = {
#include "sub_main/0047AF40.inc"
};
char gHelpTexts_1_889[48] = {
#include "sub_main/0047AF60.inc"
};
char gHelpTexts_1_890[32] = {
#include "sub_main/0047AF90.inc"
};
char gHelpTexts_1_891[32] = {
#include "sub_main/0047AFB0.inc"
};
char D_0047AFD0[48] = {
#include "sub_main/0047AFD0.inc"
};
char gHelpTexts_1_893[32] = {
#include "sub_main/0047B000.inc"
};
char gHelpTexts_1_894[32] = {
#include "sub_main/0047B020.inc"
};
char gHelpTexts_1_895[48] = {
#include "sub_main/0047B040.inc"
};
char gHelpTexts_1_916[48] = {
#include "sub_main/0047B070.inc"
};
char gHelpTexts_1_917[48] = {
#include "sub_main/0047B0A0.inc"
};
char D_0047B0D0[32] = {
#include "sub_main/0047B0D0.inc"
};
char D_0047B0F0[32] = {
#include "sub_main/0047B0F0.inc"
};
char D_0047B110[32] = {
#include "sub_main/0047B110.inc"
};
char D_0047B130[32] = {
#include "sub_main/0047B130.inc"
};
char D_0047B150[32] = {
#include "sub_main/0047B150.inc"
};
char D_0047B170[32] = {
#include "sub_main/0047B170.inc"
};
char D_0047B190[32] = {
#include "sub_main/0047B190.inc"
};
char D_0047B1B0[32] = {
#include "sub_main/0047B1B0.inc"
};
char D_0047B1D0[32] = {
#include "sub_main/0047B1D0.inc"
};
char D_0047B1F0[32] = {
#include "sub_main/0047B1F0.inc"
};
char gBladeDescTexts_19[32] = {
#include "sub_main/0047B210.inc"
};
char gBladeDescTexts_20[16] = {
#include "sub_main/0047B230.inc"
};
char gBladeDescTexts_21[32] = {
#include "sub_main/0047B240.inc"
};
char D_0047B260[32] = {
#include "sub_main/0047B260.inc"
};
char D_0047B280[32] = {
#include "sub_main/0047B280.inc"
};
char D_0047B2A0[32] = {
#include "sub_main/0047B2A0.inc"
};
char gBladeDescTexts_31[32] = {
#include "sub_main/0047B2C0.inc"
};
char gBladeDescTexts_32[32] = {
#include "sub_main/0047B2E0.inc"
};
char gBladeDescTexts_33[32] = {
#include "sub_main/0047B300.inc"
};
char gBladeDescTexts_35[32] = {
#include "sub_main/0047B320.inc"
};
char gBladeDescTexts_36[32] = {
#include "sub_main/0047B340.inc"
};
char gBladeDescTexts_37[32] = {
#include "sub_main/0047B360.inc"
};
char D_0047B380[32] = {
#include "sub_main/0047B380.inc"
};
char D_0047B3A0[32] = {
#include "sub_main/0047B3A0.inc"
};
char D_0047B3C0[32] = {
#include "sub_main/0047B3C0.inc"
};
char D_0047B3E0[32] = {
#include "sub_main/0047B3E0.inc"
};
char D_0047B400[32] = {
#include "sub_main/0047B400.inc"
};
char D_0047B420[32] = {
#include "sub_main/0047B420.inc"
};
char D_0047B440[16] = {
#include "sub_main/0047B440.inc"
};
char D_0047B450[32] = {
#include "sub_main/0047B450.inc"
};
char D_0047B470[32] = {
#include "sub_main/0047B470.inc"
};
char D_0047B490[32] = {
#include "sub_main/0047B490.inc"
};
char D_0047B4B0[32] = {
#include "sub_main/0047B4B0.inc"
};
char D_0047B4D0[32] = {
#include "sub_main/0047B4D0.inc"
};
char D_0047B4F0[32] = {
#include "sub_main/0047B4F0.inc"
};
char D_0047B510[32] = {
#include "sub_main/0047B510.inc"
};
char D_0047B530[32] = {
#include "sub_main/0047B530.inc"
};
char D_0047B550[32] = {
#include "sub_main/0047B550.inc"
};
char D_0047B570[32] = {
#include "sub_main/0047B570.inc"
};
char D_0047B590[32] = {
#include "sub_main/0047B590.inc"
};
char D_0047B5B0[32] = {
#include "sub_main/0047B5B0.inc"
};
char D_0047B5D0[32] = {
#include "sub_main/0047B5D0.inc"
};
char D_0047B5F0[32] = {
#include "sub_main/0047B5F0.inc"
};
char D_0047B610[32] = {
#include "sub_main/0047B610.inc"
};
char D_0047B630[32] = {
#include "sub_main/0047B630.inc"
};
char D_0047B650[32] = {
#include "sub_main/0047B650.inc"
};
char D_0047B670[32] = {
#include "sub_main/0047B670.inc"
};
char D_0047B690[32] = {
#include "sub_main/0047B690.inc"
};
char D_0047B6B0[16] = {
#include "sub_main/0047B6B0.inc"
};
char D_0047B6C0[32] = {
#include "sub_main/0047B6C0.inc"
};
char D_0047B6E0[32] = {
#include "sub_main/0047B6E0.inc"
};
char D_0047B700[32] = {
#include "sub_main/0047B700.inc"
};
char D_0047B720[32] = {
#include "sub_main/0047B720.inc"
};
char D_0047B740[32] = {
#include "sub_main/0047B740.inc"
};
char D_0047B760[32] = {
#include "sub_main/0047B760.inc"
};
char D_0047B780[32] = {
#include "sub_main/0047B780.inc"
};
char gBladeDescTexts_135[32] = {
#include "sub_main/0047B7A0.inc"
};
char gBladeDescTexts_136[32] = {
#include "sub_main/0047B7C0.inc"
};
char gBladeDescTexts_137[32] = {
#include "sub_main/0047B7E0.inc"
};
char gBladeDescTexts_139[32] = {
#include "sub_main/0047B800.inc"
};
char gBladeDescTexts_140[32] = {
#include "sub_main/0047B820.inc"
};
char gBladeDescTexts_141[32] = {
#include "sub_main/0047B840.inc"
};
char gBladeDescTexts_143[32] = {
#include "sub_main/0047B860.inc"
};
char gBladeDescTexts_144[32] = {
#include "sub_main/0047B880.inc"
};
char gBladeDescTexts_145[32] = {
#include "sub_main/0047B8A0.inc"
};
char D_0047B8C0[32] = {
#include "sub_main/0047B8C0.inc"
};
char D_0047B8E0[32] = {
#include "sub_main/0047B8E0.inc"
};
char D_0047B900[32] = {
#include "sub_main/0047B900.inc"
};
char D_0047B920[32] = {
#include "sub_main/0047B920.inc"
};
char D_0047B940[32] = {
#include "sub_main/0047B940.inc"
};
char D_0047B960[32] = {
#include "sub_main/0047B960.inc"
};
char D_0047B980[32] = {
#include "sub_main/0047B980.inc"
};
char D_0047B9A0[32] = {
#include "sub_main/0047B9A0.inc"
};
char D_0047B9C0[32] = {
#include "sub_main/0047B9C0.inc"
};
char D_0047B9E0[32] = {
#include "sub_main/0047B9E0.inc"
};
char D_0047BA00[32] = {
#include "sub_main/0047BA00.inc"
};
char D_0047BA20[32] = {
#include "sub_main/0047BA20.inc"
};
char D_0047BA40[32] = {
#include "sub_main/0047BA40.inc"
};
char D_0047BA60[32] = {
#include "sub_main/0047BA60.inc"
};
char D_0047BA80[32] = {
#include "sub_main/0047BA80.inc"
};
char D_0047BAA0[32] = {
#include "sub_main/0047BAA0.inc"
};
char D_0047BAC0[32] = {
#include "sub_main/0047BAC0.inc"
};
char D_0047BAE0[32] = {
#include "sub_main/0047BAE0.inc"
};
char gBladeDescTexts_187[32] = {
#include "sub_main/0047BB00.inc"
};
char gBladeDescTexts_188[32] = {
#include "sub_main/0047BB20.inc"
};
char gBladeDescTexts_189[32] = {
#include "sub_main/0047BB40.inc"
};
char D_0047BB60[32] = {
#include "sub_main/0047BB60.inc"
};
char D_0047BB80[32] = {
#include "sub_main/0047BB80.inc"
};
char D_0047BBA0[32] = {
#include "sub_main/0047BBA0.inc"
};
char D_0047BBC0[32] = {
#include "sub_main/0047BBC0.inc"
};
char D_0047BBE0[32] = {
#include "sub_main/0047BBE0.inc"
};
char D_0047BC00[32] = {
#include "sub_main/0047BC00.inc"
};
char D_0047BC20[32] = {
#include "sub_main/0047BC20.inc"
};
char D_0047BC40[32] = {
#include "sub_main/0047BC40.inc"
};
char D_0047BC60[32] = {
#include "sub_main/0047BC60.inc"
};
char D_0047BC80[32] = {
#include "sub_main/0047BC80.inc"
};
char D_0047BCA0[32] = {
#include "sub_main/0047BCA0.inc"
};
char D_0047BCC0[32] = {
#include "sub_main/0047BCC0.inc"
};
char D_0047BCE0[32] = {
#include "sub_main/0047BCE0.inc"
};
char D_0047BD00[32] = {
#include "sub_main/0047BD00.inc"
};
char D_0047BD20[32] = {
#include "sub_main/0047BD20.inc"
};
char D_0047BD40[32] = {
#include "sub_main/0047BD40.inc"
};
char D_0047BD60[32] = {
#include "sub_main/0047BD60.inc"
};
char D_0047BD80[32] = {
#include "sub_main/0047BD80.inc"
};
char D_0047BDA0[32] = {
#include "sub_main/0047BDA0.inc"
};
char D_0047BDC0[32] = {
#include "sub_main/0047BDC0.inc"
};
char gBladeDescTexts_239[32] = {
#include "sub_main/0047BDE0.inc"
};
char gBladeDescTexts_240[32] = {
#include "sub_main/0047BE00.inc"
};
char gBladeDescTexts_241[32] = {
#include "sub_main/0047BE20.inc"
};
char D_0047BE40[32] = {
#include "sub_main/0047BE40.inc"
};
char D_0047BE60[32] = {
#include "sub_main/0047BE60.inc"
};
char D_0047BE80[32] = {
#include "sub_main/0047BE80.inc"
};
char D_0047BEA0[32] = {
#include "sub_main/0047BEA0.inc"
};
char D_0047BEC0[32] = {
#include "sub_main/0047BEC0.inc"
};
char D_0047BEE0[32] = {
#include "sub_main/0047BEE0.inc"
};
char gBladeDescTexts_251[32] = {
#include "sub_main/0047BF00.inc"
};
char gBladeDescTexts_252[32] = {
#include "sub_main/0047BF20.inc"
};
char gBladeDescTexts_253[32] = {
#include "sub_main/0047BF40.inc"
};
char D_0047BF60[32] = {
#include "sub_main/0047BF60.inc"
};
char D_0047BF80[32] = {
#include "sub_main/0047BF80.inc"
};
char D_0047BFA0[32] = {
#include "sub_main/0047BFA0.inc"
};
char D_0047BFC0[32] = {
#include "sub_main/0047BFC0.inc"
};
char D_0047BFE0[32] = {
#include "sub_main/0047BFE0.inc"
};
char D_0047C000[32] = {
#include "sub_main/0047C000.inc"
};
char D_0047C020[32] = {
#include "sub_main/0047C020.inc"
};
char D_0047C040[32] = {
#include "sub_main/0047C040.inc"
};
char D_0047C060[32] = {
#include "sub_main/0047C060.inc"
};
char D_0047C080[32] = {
#include "sub_main/0047C080.inc"
};
char D_0047C0A0[32] = {
#include "sub_main/0047C0A0.inc"
};
char D_0047C0C0[32] = {
#include "sub_main/0047C0C0.inc"
};
char D_0047C0E0[32] = {
#include "sub_main/0047C0E0.inc"
};
char D_0047C100[32] = {
#include "sub_main/0047C100.inc"
};
char D_0047C120[32] = {
#include "sub_main/0047C120.inc"
};
char D_0047C140[32] = {
#include "sub_main/0047C140.inc"
};
char D_0047C160[32] = {
#include "sub_main/0047C160.inc"
};
char D_0047C180[32] = {
#include "sub_main/0047C180.inc"
};
char D_0047C1A0[32] = {
#include "sub_main/0047C1A0.inc"
};
char D_0047C1C0[32] = {
#include "sub_main/0047C1C0.inc"
};
char D_0047C1E0[32] = {
#include "sub_main/0047C1E0.inc"
};
char D_0047C200[32] = {
#include "sub_main/0047C200.inc"
};
char D_0047C220[32] = {
#include "sub_main/0047C220.inc"
};
char D_0047C240[32] = {
#include "sub_main/0047C240.inc"
};
char D_0047C260[32] = {
#include "sub_main/0047C260.inc"
};
char D_0047C280[32] = {
#include "sub_main/0047C280.inc"
};
char D_0047C2A0[32] = {
#include "sub_main/0047C2A0.inc"
};
char D_0047C2C0[32] = {
#include "sub_main/0047C2C0.inc"
};
char D_0047C2E0[32] = {
#include "sub_main/0047C2E0.inc"
};
char D_0047C300[32] = {
#include "sub_main/0047C300.inc"
};
char D_0047C320[32] = {
#include "sub_main/0047C320.inc"
};
char D_0047C340[32] = {
#include "sub_main/0047C340.inc"
};
char D_0047C360[32] = {
#include "sub_main/0047C360.inc"
};
char D_0047C380[32] = {
#include "sub_main/0047C380.inc"
};
char D_0047C3A0[32] = {
#include "sub_main/0047C3A0.inc"
};
char D_0047C3C0[32] = {
#include "sub_main/0047C3C0.inc"
};
char D_0047C3E0[32] = {
#include "sub_main/0047C3E0.inc"
};
char D_0047C400[32] = {
#include "sub_main/0047C400.inc"
};
char D_0047C420[32] = {
#include "sub_main/0047C420.inc"
};
char gBladeDescTexts_329[32] = {
#include "sub_main/0047C440.inc"
};
char D_0047C460[32] = {
#include "sub_main/0047C460.inc"
};
char D_0047C480[32] = {
#include "sub_main/0047C480.inc"
};
char D_0047C4A0[32] = {
#include "sub_main/0047C4A0.inc"
};
char gBladeDescTexts_339[32] = {
#include "sub_main/0047C4C0.inc"
};
char gBladeDescTexts_340[32] = {
#include "sub_main/0047C4E0.inc"
};
char gBladeDescTexts_341[32] = {
#include "sub_main/0047C500.inc"
};
char D_0047C520[32] = {
#include "sub_main/0047C520.inc"
};
char D_0047C540[32] = {
#include "sub_main/0047C540.inc"
};
char D_0047C560[32] = {
#include "sub_main/0047C560.inc"
};
char D_0047C580[32] = {
#include "sub_main/0047C580.inc"
};
char D_0047C5A0[32] = {
#include "sub_main/0047C5A0.inc"
};
char D_0047C5C0[32] = {
#include "sub_main/0047C5C0.inc"
};
char D_0047C5E0[32] = {
#include "sub_main/0047C5E0.inc"
};
char D_0047C600[32] = {
#include "sub_main/0047C600.inc"
};
char D_0047C620[32] = {
#include "sub_main/0047C620.inc"
};
char D_0047C640[32] = {
#include "sub_main/0047C640.inc"
};
char D_0047C660[32] = {
#include "sub_main/0047C660.inc"
};
char D_0047C680[32] = {
#include "sub_main/0047C680.inc"
};
char D_0047C6A0[32] = {
#include "sub_main/0047C6A0.inc"
};
char gBladeDescTexts_372[32] = {
#include "sub_main/0047C6C0.inc"
};
char D_0047C6E0[32] = {
#include "sub_main/0047C6E0.inc"
};
char D_0047C700[32] = {
#include "sub_main/0047C700.inc"
};
char gBladeDescTexts_376[32] = {
#include "sub_main/0047C720.inc"
};
char D_0047C740[32] = {
#include "sub_main/0047C740.inc"
};
char D_0047C760[32] = {
#include "sub_main/0047C760.inc"
};
char D_0047C780[32] = {
#include "sub_main/0047C780.inc"
};
char D_0047C7A0[32] = {
#include "sub_main/0047C7A0.inc"
};
char D_0047C7C0[32] = {
#include "sub_main/0047C7C0.inc"
};
char D_0047C7E0[32] = {
#include "sub_main/0047C7E0.inc"
};
char gBladeDescTexts_391[32] = {
#include "sub_main/0047C800.inc"
};
char gBladeDescTexts_392[32] = {
#include "sub_main/0047C820.inc"
};
char gBladeDescTexts_393[32] = {
#include "sub_main/0047C840.inc"
};
char D_0047C860[16] = {
#include "sub_main/0047C860.inc"
};
char D_0047C870[32] = {
#include "sub_main/0047C870.inc"
};
char D_0047C890[32] = {
#include "sub_main/0047C890.inc"
};
char D_0047C8B0[32] = {
#include "sub_main/0047C8B0.inc"
};
char D_0047C8D0[32] = {
#include "sub_main/0047C8D0.inc"
};
char D_0047C8F0[32] = {
#include "sub_main/0047C8F0.inc"
};
char D_0047C910[32] = {
#include "sub_main/0047C910.inc"
};
char D_0047C930[32] = {
#include "sub_main/0047C930.inc"
};
char D_0047C950[32] = {
#include "sub_main/0047C950.inc"
};
char D_0047C970[32] = {
#include "sub_main/0047C970.inc"
};
char D_0047C990[32] = {
#include "sub_main/0047C990.inc"
};
char D_0047C9B0[32] = {
#include "sub_main/0047C9B0.inc"
};
char D_0047C9D0[32] = {
#include "sub_main/0047C9D0.inc"
};
char D_0047C9F0[32] = {
#include "sub_main/0047C9F0.inc"
};
char D_0047CA10[32] = {
#include "sub_main/0047CA10.inc"
};
char D_0047CA30[32] = {
#include "sub_main/0047CA30.inc"
};
char D_0047CA50[32] = {
#include "sub_main/0047CA50.inc"
};
char D_0047CA70[32] = {
#include "sub_main/0047CA70.inc"
};
char D_0047CA90[48] = {
#include "sub_main/0047CA90.inc"
};
char gBladeDescTexts_443[32] = {
#include "sub_main/0047CAC0.inc"
};
char gBladeDescTexts_444[32] = {
#include "sub_main/0047CAE0.inc"
};
char gBladeDescTexts_445[32] = {
#include "sub_main/0047CB00.inc"
};
char gBladeDescTexts_447[32] = {
#include "sub_main/0047CB20.inc"
};
char gBladeDescTexts_448[32] = {
#include "sub_main/0047CB40.inc"
};
char gBladeDescTexts_449[32] = {
#include "sub_main/0047CB60.inc"
};
char D_0047CB80[32] = {
#include "sub_main/0047CB80.inc"
};
char D_0047CBA0[32] = {
#include "sub_main/0047CBA0.inc"
};
char D_0047CBC0[48] = {
#include "sub_main/0047CBC0.inc"
};
char D_0047CBF0[32] = {
#include "sub_main/0047CBF0.inc"
};
char D_0047CC10[32] = {
#include "sub_main/0047CC10.inc"
};
char D_0047CC30[32] = {
#include "sub_main/0047CC30.inc"
};
char D_0047CC50[32] = {
#include "sub_main/0047CC50.inc"
};
char D_0047CC70[32] = {
#include "sub_main/0047CC70.inc"
};
char D_0047CC90[32] = {
#include "sub_main/0047CC90.inc"
};
char D_0047CCB0[32] = {
#include "sub_main/0047CCB0.inc"
};
char D_0047CCD0[32] = {
#include "sub_main/0047CCD0.inc"
};
char D_0047CCF0[32] = {
#include "sub_main/0047CCF0.inc"
};
char D_0047CD10[32] = {
#include "sub_main/0047CD10.inc"
};
char D_0047CD30[32] = {
#include "sub_main/0047CD30.inc"
};
char D_0047CD50[32] = {
#include "sub_main/0047CD50.inc"
};
char gBladeDescTexts_485[32] = {
#include "sub_main/0047CD70.inc"
};
char gBladeDescTexts_492[32] = {
#include "sub_main/0047CD90.inc"
};
char gBladeDescTexts_497[32] = {
#include "sub_main/0047CDB0.inc"
};
char gBladeDescTexts_499[32] = {
#include "sub_main/0047CDD0.inc"
};
char gBladeDescTexts_500[32] = {
#include "sub_main/0047CDF0.inc"
};
char gBladeDescTexts_501[32] = {
#include "sub_main/0047CE10.inc"
};
char D_0047CE30[32] = {
#include "sub_main/0047CE30.inc"
};
char D_0047CE50[32] = {
#include "sub_main/0047CE50.inc"
};
char gBladeDescTexts_505[48] = {
#include "sub_main/0047CE70.inc"
};
char D_0047CEA0[32] = {
#include "sub_main/0047CEA0.inc"
};
char D_0047CEC0[32] = {
#include "sub_main/0047CEC0.inc"
};
char D_0047CEE0[32] = {
#include "sub_main/0047CEE0.inc"
};
char D_0047CF00[32] = {
#include "sub_main/0047CF00.inc"
};
char D_0047CF20[32] = {
#include "sub_main/0047CF20.inc"
};
char gBladeDescTexts_521[48] = {
#include "sub_main/0047CF40.inc"
};
char gBladeDescTexts_527[32] = {
#include "sub_main/0047CF70.inc"
};
char gBladeDescTexts_528[32] = {
#include "sub_main/0047CF90.inc"
};
char gBladeDescTexts_529[32] = {
#include "sub_main/0047CFB0.inc"
};
char D_0047CFD0[32] = {
#include "sub_main/0047CFD0.inc"
};
char D_0047CFF0[32] = {
#include "sub_main/0047CFF0.inc"
};
char D_0047D010[32] = {
#include "sub_main/0047D010.inc"
};
char D_0047D030[32] = {
#include "sub_main/0047D030.inc"
};
char D_0047D050[32] = {
#include "sub_main/0047D050.inc"
};
char D_0047D070[32] = {
#include "sub_main/0047D070.inc"
};
char gBladeDescTexts_539[32] = {
#include "sub_main/0047D090.inc"
};
char D_0047D0B0[32] = {
#include "sub_main/0047D0B0.inc"
};
char D_0047D0D0[32] = {
#include "sub_main/0047D0D0.inc"
};
char D_0047D0F0[32] = {
#include "sub_main/0047D0F0.inc"
};
char D_0047D110[32] = {
#include "sub_main/0047D110.inc"
};
char D_0047D130[32] = {
#include "sub_main/0047D130.inc"
};
char D_0047D150[32] = {
#include "sub_main/0047D150.inc"
};
char D_0047D170[32] = {
#include "sub_main/0047D170.inc"
};
char gBladeDescTexts_549[48] = {
#include "sub_main/0047D190.inc"
};
char D_0047D1C0[32] = {
#include "sub_main/0047D1C0.inc"
};
char D_0047D1E0[48] = {
#include "sub_main/0047D1E0.inc"
};
char D_0047D210[32] = {
#include "sub_main/0047D210.inc"
};
char D_0047D230[32] = {
#include "sub_main/0047D230.inc"
};
char D_0047D250[32] = {
#include "sub_main/0047D250.inc"
};
char D_0047D270[32] = {
#include "sub_main/0047D270.inc"
};
char D_0047D290[32] = {
#include "sub_main/0047D290.inc"
};
char D_0047D2B0[32] = {
#include "sub_main/0047D2B0.inc"
};
char D_0047D2D0[32] = {
#include "sub_main/0047D2D0.inc"
};
char D_0047D2F0[32] = {
#include "sub_main/0047D2F0.inc"
};
char D_0047D310[32] = {
#include "sub_main/0047D310.inc"
};
char D_0047D330[32] = {
#include "sub_main/0047D330.inc"
};
char gBladeDescTexts_571[32] = {
#include "sub_main/0047D350.inc"
};
char gBladeDescTexts_572[32] = {
#include "sub_main/0047D370.inc"
};
char gBladeDescTexts_573[32] = {
#include "sub_main/0047D390.inc"
};
char D_0047D3B0[32] = {
#include "sub_main/0047D3B0.inc"
};
char D_0047D3D0[32] = {
#include "sub_main/0047D3D0.inc"
};
char D_0047D3F0[32] = {
#include "sub_main/0047D3F0.inc"
};
char D_0047D410[32] = {
#include "sub_main/0047D410.inc"
};
char D_0047D430[32] = {
#include "sub_main/0047D430.inc"
};
char D_0047D450[32] = {
#include "sub_main/0047D450.inc"
};
char gBladeDescTexts_587[32] = {
#include "sub_main/0047D470.inc"
};
char gBladeDescTexts_588[32] = {
#include "sub_main/0047D490.inc"
};
char gBladeDescTexts_589[32] = {
#include "sub_main/0047D4B0.inc"
};
char gBladeDescTexts_592[32] = {
#include "sub_main/0047D4D0.inc"
};
char gBladeDescTexts_596[32] = {
#include "sub_main/0047D4F0.inc"
};
char D_0047D510[32] = {
#include "sub_main/0047D510.inc"
};
char D_0047D530[32] = {
#include "sub_main/0047D530.inc"
};
char gBladeDescTexts_611[32] = {
#include "sub_main/0047D550.inc"
};
char gBladeDescTexts_612[32] = {
#include "sub_main/0047D570.inc"
};
char gBladeDescTexts_613[32] = {
#include "sub_main/0047D590.inc"
};
char D_0047D5B0[32] = {
#include "sub_main/0047D5B0.inc"
};
char D_0047D5D0[32] = {
#include "sub_main/0047D5D0.inc"
};
char D_0047D5F0[32] = {
#include "sub_main/0047D5F0.inc"
};
char D_0047D610[32] = {
#include "sub_main/0047D610.inc"
};
char D_0047D630[32] = {
#include "sub_main/0047D630.inc"
};
char D_0047D650[32] = {
#include "sub_main/0047D650.inc"
};
char D_0047D670[32] = {
#include "sub_main/0047D670.inc"
};
char D_0047D690[32] = {
#include "sub_main/0047D690.inc"
};
char D_0047D6B0[32] = {
#include "sub_main/0047D6B0.inc"
};
char D_0047D6D0[32] = {
#include "sub_main/0047D6D0.inc"
};
char D_0047D6F0[32] = {
#include "sub_main/0047D6F0.inc"
};
char gBladeDescTexts_643[32] = {
#include "sub_main/0047D710.inc"
};
char gBladeDescTexts_644[32] = {
#include "sub_main/0047D730.inc"
};
char gBladeDescTexts_645[32] = {
#include "sub_main/0047D750.inc"
};
char gBladeDescTexts_647[32] = {
#include "sub_main/0047D770.inc"
};
char gBladeDescTexts_648[32] = {
#include "sub_main/0047D790.inc"
};
char gBladeDescTexts_649[32] = {
#include "sub_main/0047D7B0.inc"
};
char D_0047D7D0[32] = {
#include "sub_main/0047D7D0.inc"
};
char D_0047D7F0[32] = {
#include "sub_main/0047D7F0.inc"
};
char D_0047D810[32] = {
#include "sub_main/0047D810.inc"
};
char D_0047D830[32] = {
#include "sub_main/0047D830.inc"
};
char D_0047D850[32] = {
#include "sub_main/0047D850.inc"
};
char D_0047D870[32] = {
#include "sub_main/0047D870.inc"
};
char gBladeDescTexts_659[32] = {
#include "sub_main/0047D890.inc"
};
char gBladeDescTexts_660[32] = {
#include "sub_main/0047D8B0.inc"
};
char gBladeDescTexts_661[32] = {
#include "sub_main/0047D8D0.inc"
};
char D_0047D8F0[32] = {
#include "sub_main/0047D8F0.inc"
};
char D_0047D910[32] = {
#include "sub_main/0047D910.inc"
};
char D_0047D930[32] = {
#include "sub_main/0047D930.inc"
};
char D_0047D950[32] = {
#include "sub_main/0047D950.inc"
};
char D_0047D970[32] = {
#include "sub_main/0047D970.inc"
};
char D_0047D990[32] = {
#include "sub_main/0047D990.inc"
};
char D_0047D9B0[32] = {
#include "sub_main/0047D9B0.inc"
};
char D_0047D9D0[32] = {
#include "sub_main/0047D9D0.inc"
};
char D_0047D9F0[32] = {
#include "sub_main/0047D9F0.inc"
};
char D_0047DA10[32] = {
#include "sub_main/0047DA10.inc"
};
char D_0047DA30[32] = {
#include "sub_main/0047DA30.inc"
};
char D_0047DA50[32] = {
#include "sub_main/0047DA50.inc"
};
char D_0047DA70[32] = {
#include "sub_main/0047DA70.inc"
};
char D_0047DA90[32] = {
#include "sub_main/0047DA90.inc"
};
char D_0047DAB0[32] = {
#include "sub_main/0047DAB0.inc"
};
char D_0047DAD0[32] = {
#include "sub_main/0047DAD0.inc"
};
char D_0047DAF0[32] = {
#include "sub_main/0047DAF0.inc"
};
char D_0047DB10[32] = {
#include "sub_main/0047DB10.inc"
};
char gBladeDescTexts_707[32] = {
#include "sub_main/0047DB30.inc"
};
char gBladeDescTexts_708[32] = {
#include "sub_main/0047DB50.inc"
};
char gBladeDescTexts_709[32] = {
#include "sub_main/0047DB70.inc"
};
char D_0047DB90[32] = {
#include "sub_main/0047DB90.inc"
};
char D_0047DBB0[32] = {
#include "sub_main/0047DBB0.inc"
};
char D_0047DBD0[32] = {
#include "sub_main/0047DBD0.inc"
};
char gBladeDescTexts_727[32] = {
#include "sub_main/0047DBF0.inc"
};
char gBladeDescTexts_728[32] = {
#include "sub_main/0047DC10.inc"
};
char gBladeDescTexts_729[32] = {
#include "sub_main/0047DC30.inc"
};
char D_0047DC50[32] = {
#include "sub_main/0047DC50.inc"
};
char D_0047DC70[32] = {
#include "sub_main/0047DC70.inc"
};
char D_0047DC90[32] = {
#include "sub_main/0047DC90.inc"
};
char gBladeDescTexts_735[32] = {
#include "sub_main/0047DCB0.inc"
};
char gBladeDescTexts_736[32] = {
#include "sub_main/0047DCD0.inc"
};
char gBladeDescTexts_737[32] = {
#include "sub_main/0047DCF0.inc"
};
char gBladeDescTexts_743[32] = {
#include "sub_main/0047DD10.inc"
};
char gBladeDescTexts_744[32] = {
#include "sub_main/0047DD30.inc"
};
char gBladeDescTexts_745[32] = {
#include "sub_main/0047DD50.inc"
};
char D_0047DD70[32] = {
#include "sub_main/0047DD70.inc"
};
char D_0047DD90[32] = {
#include "sub_main/0047DD90.inc"
};
char D_0047DDB0[32] = {
#include "sub_main/0047DDB0.inc"
};
char D_0047DDD0[32] = {
#include "sub_main/0047DDD0.inc"
};
char D_0047DDF0[32] = {
#include "sub_main/0047DDF0.inc"
};
char D_0047DE10[32] = {
#include "sub_main/0047DE10.inc"
};
char gBladeDescTexts_759[32] = {
#include "sub_main/0047DE30.inc"
};
char gBladeDescTexts_760[32] = {
#include "sub_main/0047DE50.inc"
};
char gBladeDescTexts_761[32] = {
#include "sub_main/0047DE70.inc"
};
char D_0047DE90[32] = {
#include "sub_main/0047DE90.inc"
};
char D_0047DEB0[32] = {
#include "sub_main/0047DEB0.inc"
};
char D_0047DED0[32] = {
#include "sub_main/0047DED0.inc"
};
char D_0047DEF0[32] = {
#include "sub_main/0047DEF0.inc"
};
char D_0047DF10[32] = {
#include "sub_main/0047DF10.inc"
};
char D_0047DF30[32] = {
#include "sub_main/0047DF30.inc"
};
char D_0047DF50[32] = {
#include "sub_main/0047DF50.inc"
};
char D_0047DF70[32] = {
#include "sub_main/0047DF70.inc"
};
char D_0047DF90[32] = {
#include "sub_main/0047DF90.inc"
};
char D_0047DFB0[32] = {
#include "sub_main/0047DFB0.inc"
};
char D_0047DFD0[32] = {
#include "sub_main/0047DFD0.inc"
};
char D_0047DFF0[32] = {
#include "sub_main/0047DFF0.inc"
};
char D_0047E010[32] = {
#include "sub_main/0047E010.inc"
};
char gBladeDescTexts_799[32] = {
#include "sub_main/0047E030.inc"
};
char gBladeDescTexts_800[32] = {
#include "sub_main/0047E050.inc"
};
char gBladeDescTexts_801[32] = {
#include "sub_main/0047E070.inc"
};
char D_0047E090[32] = {
#include "sub_main/0047E090.inc"
};
char D_0047E0B0[32] = {
#include "sub_main/0047E0B0.inc"
};
char D_0047E0D0[32] = {
#include "sub_main/0047E0D0.inc"
};
char D_0047E0F0[32] = {
#include "sub_main/0047E0F0.inc"
};
char D_0047E110[32] = {
#include "sub_main/0047E110.inc"
};
char D_0047E130[32] = {
#include "sub_main/0047E130.inc"
};
char gBladeDescTexts_819[32] = {
#include "sub_main/0047E150.inc"
};
char gBladeDescTexts_820[32] = {
#include "sub_main/0047E170.inc"
};
char gBladeDescTexts_821[32] = {
#include "sub_main/0047E190.inc"
};
char gBladeDescTexts_823[32] = {
#include "sub_main/0047E1B0.inc"
};
char gBladeDescTexts_824[32] = {
#include "sub_main/0047E1D0.inc"
};
char gBladeDescTexts_825[32] = {
#include "sub_main/0047E1F0.inc"
};
char D_0047E210[32] = {
#include "sub_main/0047E210.inc"
};
char D_0047E230[32] = {
#include "sub_main/0047E230.inc"
};
char D_0047E250[32] = {
#include "sub_main/0047E250.inc"
};
char D_0047E270[32] = {
#include "sub_main/0047E270.inc"
};
char D_0047E290[32] = {
#include "sub_main/0047E290.inc"
};
char D_0047E2B0[32] = {
#include "sub_main/0047E2B0.inc"
};
char gBladeDescTexts_835[32] = {
#include "sub_main/0047E2D0.inc"
};
char gBladeDescTexts_836[32] = {
#include "sub_main/0047E2F0.inc"
};
char gBladeDescTexts_837[16] = {
#include "sub_main/0047E310.inc"
};
char D_0047E320[32] = {
#include "sub_main/0047E320.inc"
};
char D_0047E340[32] = {
#include "sub_main/0047E340.inc"
};
char D_0047E360[32] = {
#include "sub_main/0047E360.inc"
};
char D_0047E380[32] = {
#include "sub_main/0047E380.inc"
};
char D_0047E3A0[32] = {
#include "sub_main/0047E3A0.inc"
};
char D_0047E3C0[32] = {
#include "sub_main/0047E3C0.inc"
};
char gBladeDescTexts_863[32] = {
#include "sub_main/0047E3E0.inc"
};
char gBladeDescTexts_864[32] = {
#include "sub_main/0047E400.inc"
};
char gBladeDescTexts_865[32] = {
#include "sub_main/0047E420.inc"
};
char gBladeDescTexts_867[32] = {
#include "sub_main/0047E440.inc"
};
char gBladeDescTexts_868[32] = {
#include "sub_main/0047E460.inc"
};
char gBladeDescTexts_869[32] = {
#include "sub_main/0047E480.inc"
};
char D_0047E4A0[32] = {
#include "sub_main/0047E4A0.inc"
};
char D_0047E4C0[32] = {
#include "sub_main/0047E4C0.inc"
};
char D_0047E4E0[32] = {
#include "sub_main/0047E4E0.inc"
};
char gBladeDescTexts_883[32] = {
#include "sub_main/0047E500.inc"
};
char gBladeDescTexts_884[32] = {
#include "sub_main/0047E520.inc"
};
char gBladeDescTexts_885[32] = {
#include "sub_main/0047E540.inc"
};
char D_0047E560[32] = {
#include "sub_main/0047E560.inc"
};
char D_0047E580[32] = {
#include "sub_main/0047E580.inc"
};
char D_0047E5A0[32] = {
#include "sub_main/0047E5A0.inc"
};
char gBladeDescTexts_895[32] = {
#include "sub_main/0047E5C0.inc"
};
char gBladeDescTexts_896[32] = {
#include "sub_main/0047E5E0.inc"
};
char gBladeDescTexts_897[32] = {
#include "sub_main/0047E600.inc"
};
char D_0047E620[32] = {
#include "sub_main/0047E620.inc"
};
char D_0047E640[32] = {
#include "sub_main/0047E640.inc"
};
char D_0047E660[16] = {
#include "sub_main/0047E660.inc"
};
char D_0047E670[32] = {
#include "sub_main/0047E670.inc"
};
char D_0047E690[32] = {
#include "sub_main/0047E690.inc"
};
char D_0047E6B0[32] = {
#include "sub_main/0047E6B0.inc"
};
char D_0047E6D0[32] = {
#include "sub_main/0047E6D0.inc"
};
char D_0047E6F0[32] = {
#include "sub_main/0047E6F0.inc"
};
char D_0047E710[32] = {
#include "sub_main/0047E710.inc"
};
char gBladeDescTexts_911[32] = {
#include "sub_main/0047E730.inc"
};
char gBladeDescTexts_912[32] = {
#include "sub_main/0047E750.inc"
};
char gBladeDescTexts_913[32] = {
#include "sub_main/0047E770.inc"
};
char gBladeDescTexts_915[32] = {
#include "sub_main/0047E790.inc"
};
char gBladeDescTexts_916[32] = {
#include "sub_main/0047E7B0.inc"
};
char gBladeDescTexts_917[16] = {
#include "sub_main/0047E7D0.inc"
};
char gBladeDescTexts_927[32] = {
#include "sub_main/0047E7E0.inc"
};
char gBladeDescTexts_928[32] = {
#include "sub_main/0047E800.inc"
};
char gBladeDescTexts_929[32] = {
#include "sub_main/0047E820.inc"
};
char gBladeDescTexts_931[32] = {
#include "sub_main/0047E840.inc"
};
char gBladeDescTexts_932[32] = {
#include "sub_main/0047E860.inc"
};
char gBladeDescTexts_933[32] = {
#include "sub_main/0047E880.inc"
};
char gBladeDescTexts_935[32] = {
#include "sub_main/0047E8A0.inc"
};
char gBladeDescTexts_936[32] = {
#include "sub_main/0047E8C0.inc"
};
char gBladeDescTexts_937[32] = {
#include "sub_main/0047E8E0.inc"
};
char D_0047E900[32] = {
#include "sub_main/0047E900.inc"
};
char D_0047E920[32] = {
#include "sub_main/0047E920.inc"
};
char D_0047E940[32] = {
#include "sub_main/0047E940.inc"
};
char D_0047E960[32] = {
#include "sub_main/0047E960.inc"
};
char D_0047E980[32] = {
#include "sub_main/0047E980.inc"
};
char D_0047E9A0[32] = {
#include "sub_main/0047E9A0.inc"
};
char D_0047E9C0[32] = {
#include "sub_main/0047E9C0.inc"
};
char D_0047E9E0[32] = {
#include "sub_main/0047E9E0.inc"
};
char D_0047EA00[32] = {
#include "sub_main/0047EA00.inc"
};
char D_0047EA20[32] = {
#include "sub_main/0047EA20.inc"
};
char D_0047EA40[32] = {
#include "sub_main/0047EA40.inc"
};
char D_0047EA60[32] = {
#include "sub_main/0047EA60.inc"
};
char D_0047EA80[32] = {
#include "sub_main/0047EA80.inc"
};
char D_0047EAA0[32] = {
#include "sub_main/0047EAA0.inc"
};
char D_0047EAC0[32] = {
#include "sub_main/0047EAC0.inc"
};
char D_0047EAE0[32] = {
#include "sub_main/0047EAE0.inc"
};
char D_0047EB00[32] = {
#include "sub_main/0047EB00.inc"
};
char gBladeDescTexts_976[32] = {
#include "sub_main/0047EB20.inc"
};
char D_0047EB40[32] = {
#include "sub_main/0047EB40.inc"
};
char D_0047EB60[32] = {
#include "sub_main/0047EB60.inc"
};
char D_0047EB80[32] = {
#include "sub_main/0047EB80.inc"
};
char D_0047EBA0[32] = {
#include "sub_main/0047EBA0.inc"
};
char D_0047EBC0[32] = {
#include "sub_main/0047EBC0.inc"
};
char D_0047EBE0[32] = {
#include "sub_main/0047EBE0.inc"
};
char D_0047EC00[32] = {
#include "sub_main/0047EC00.inc"
};
char D_0047EC20[32] = {
#include "sub_main/0047EC20.inc"
};
char D_0047EC40[32] = {
#include "sub_main/0047EC40.inc"
};
char D_0047EC60[32] = {
#include "sub_main/0047EC60.inc"
};
char D_0047EC80[32] = {
#include "sub_main/0047EC80.inc"
};
char D_0047ECA0[32] = {
#include "sub_main/0047ECA0.inc"
};
char D_0047ECC0[32] = {
#include "sub_main/0047ECC0.inc"
};
char D_0047ECE0[32] = {
#include "sub_main/0047ECE0.inc"
};
char D_0047ED00[32] = {
#include "sub_main/0047ED00.inc"
};
char D_0047ED20[32] = {
#include "sub_main/0047ED20.inc"
};
char D_0047ED40[32] = {
#include "sub_main/0047ED40.inc"
};
char D_0047ED60[16] = {
#include "sub_main/0047ED60.inc"
};
char D_0047ED70[32] = {
#include "sub_main/0047ED70.inc"
};
char D_0047ED90[32] = {
#include "sub_main/0047ED90.inc"
};
char D_0047EDB0[32] = {
#include "sub_main/0047EDB0.inc"
};
char D_0047EDD0[32] = {
#include "sub_main/0047EDD0.inc"
};
char D_0047EDF0[32] = {
#include "sub_main/0047EDF0.inc"
};
char D_0047EE10[32] = {
#include "sub_main/0047EE10.inc"
};
char D_0047EE30[32] = {
#include "sub_main/0047EE30.inc"
};
char D_0047EE50[32] = {
#include "sub_main/0047EE50.inc"
};
char D_0047EE70[32] = {
#include "sub_main/0047EE70.inc"
};
char gBladeDescTexts_1059[32] = {
#include "sub_main/0047EE90.inc"
};
char gBladeDescTexts_1060[32] = {
#include "sub_main/0047EEB0.inc"
};
char gBladeDescTexts_1061[32] = {
#include "sub_main/0047EED0.inc"
};
char D_0047EEF0[32] = {
#include "sub_main/0047EEF0.inc"
};
char D_0047EF10[32] = {
#include "sub_main/0047EF10.inc"
};
char D_0047EF30[32] = {
#include "sub_main/0047EF30.inc"
};
char D_0047EF50[32] = {
#include "sub_main/0047EF50.inc"
};
char D_0047EF70[32] = {
#include "sub_main/0047EF70.inc"
};
char D_0047EF90[32] = {
#include "sub_main/0047EF90.inc"
};
char D_0047EFB0[32] = {
#include "sub_main/0047EFB0.inc"
};
char D_0047EFD0[32] = {
#include "sub_main/0047EFD0.inc"
};
char D_0047EFF0[32] = {
#include "sub_main/0047EFF0.inc"
};
char D_0047F010[32] = {
#include "sub_main/0047F010.inc"
};
char D_0047F030[32] = {
#include "sub_main/0047F030.inc"
};
char gBladeDescTexts_1079[32] = {
#include "sub_main/0047F050.inc"
};
char gBladeDescTexts_1080[32] = {
#include "sub_main/0047F070.inc"
};
char gBladeDescTexts_1081[32] = {
#include "sub_main/0047F090.inc"
};
char D_0047F0B0[32] = {
#include "sub_main/0047F0B0.inc"
};
char D_0047F0D0[32] = {
#include "sub_main/0047F0D0.inc"
};
char D_0047F0F0[32] = {
#include "sub_main/0047F0F0.inc"
};
char D_0047F110[32] = {
#include "sub_main/0047F110.inc"
};
char D_0047F130[32] = {
#include "sub_main/0047F130.inc"
};
char D_0047F150[32] = {
#include "sub_main/0047F150.inc"
};
char gBladeDescTexts_1099[32] = {
#include "sub_main/0047F170.inc"
};
char gBladeDescTexts_1100[32] = {
#include "sub_main/0047F190.inc"
};
char gBladeDescTexts_1101[16] = {
#include "sub_main/0047F1B0.inc"
};
char D_0047F1C0[32] = {
#include "sub_main/0047F1C0.inc"
};
char D_0047F1E0[32] = {
#include "sub_main/0047F1E0.inc"
};
char D_0047F200[32] = {
#include "sub_main/0047F200.inc"
};
char gBladeDescTexts_1107[32] = {
#include "sub_main/0047F220.inc"
};
char gBladeDescTexts_1108[32] = {
#include "sub_main/0047F240.inc"
};
char gBladeDescTexts_1109[32] = {
#include "sub_main/0047F260.inc"
};
char gBladeDescTexts_1111[32] = {
#include "sub_main/0047F280.inc"
};
char gBladeDescTexts_1112[32] = {
#include "sub_main/0047F2A0.inc"
};
char gBladeDescTexts_1113[32] = {
#include "sub_main/0047F2C0.inc"
};
char gBladeDescTexts_1119[32] = {
#include "sub_main/0047F2E0.inc"
};
char gBladeDescTexts_1120[32] = {
#include "sub_main/0047F300.inc"
};
char gBladeDescTexts_1121[32] = {
#include "sub_main/0047F320.inc"
};
char D_0047F340[32] = {
#include "sub_main/0047F340.inc"
};
char D_0047F360[32] = {
#include "sub_main/0047F360.inc"
};
char D_0047F380[32] = {
#include "sub_main/0047F380.inc"
};
char D_0047F3A0[32] = {
#include "sub_main/0047F3A0.inc"
};
char D_0047F3C0[32] = {
#include "sub_main/0047F3C0.inc"
};
char D_0047F3E0[32] = {
#include "sub_main/0047F3E0.inc"
};
char D_0047F400[32] = {
#include "sub_main/0047F400.inc"
};
char gBladeDescTexts_1147[32] = {
#include "sub_main/0047F420.inc"
};
char gBladeDescTexts_1148[32] = {
#include "sub_main/0047F440.inc"
};
char gBladeDescTexts_1149[32] = {
#include "sub_main/0047F460.inc"
};
char D_0047F480[32] = {
#include "sub_main/0047F480.inc"
};
char D_0047F4A0[32] = {
#include "sub_main/0047F4A0.inc"
};
char gBladeDescTexts_1153[32] = {
#include "sub_main/0047F4C0.inc"
};
char D_0047F4E0[32] = {
#include "sub_main/0047F4E0.inc"
};
char gBladeDescTexts_1156[32] = {
#include "sub_main/0047F500.inc"
};
char gBladeDescTexts_1157[32] = {
#include "sub_main/0047F520.inc"
};
char D_0047F540[32] = {
#include "sub_main/0047F540.inc"
};
char D_0047F560[32] = {
#include "sub_main/0047F560.inc"
};
char D_0047F580[32] = {
#include "sub_main/0047F580.inc"
};
char D_0047F5A0[32] = {
#include "sub_main/0047F5A0.inc"
};
char D_0047F5C0[32] = {
#include "sub_main/0047F5C0.inc"
};
char gBladeDescTexts_1171[32] = {
#include "sub_main/0047F5E0.inc"
};
char gBladeDescTexts_1172[32] = {
#include "sub_main/0047F600.inc"
};
char gBladeDescTexts_1173[32] = {
#include "sub_main/0047F620.inc"
};
char D_0047F640[32] = {
#include "sub_main/0047F640.inc"
};
char D_0047F660[32] = {
#include "sub_main/0047F660.inc"
};
char D_0047F680[32] = {
#include "sub_main/0047F680.inc"
};
char D_0047F6A0[32] = {
#include "sub_main/0047F6A0.inc"
};
char D_0047F6C0[32] = {
#include "sub_main/0047F6C0.inc"
};
char D_0047F6E0[32] = {
#include "sub_main/0047F6E0.inc"
};
char D_0047F700[32] = {
#include "sub_main/0047F700.inc"
};
char D_0047F720[32] = {
#include "sub_main/0047F720.inc"
};
char D_0047F740[32] = {
#include "sub_main/0047F740.inc"
};
char gBladeDescTexts_1191[32] = {
#include "sub_main/0047F760.inc"
};
char gBladeDescTexts_1192[32] = {
#include "sub_main/0047F780.inc"
};
char gBladeDescTexts_1193[32] = {
#include "sub_main/0047F7A0.inc"
};
char gBladeDescTexts_1195[32] = {
#include "sub_main/0047F7C0.inc"
};
char gBladeDescTexts_1196[32] = {
#include "sub_main/0047F7E0.inc"
};
char gBladeDescTexts_1197[32] = {
#include "sub_main/0047F800.inc"
};
char D_0047F820[32] = {
#include "sub_main/0047F820.inc"
};
char D_0047F840[32] = {
#include "sub_main/0047F840.inc"
};
char D_0047F860[32] = {
#include "sub_main/0047F860.inc"
};
char D_0047F880[32] = {
#include "sub_main/0047F880.inc"
};
char D_0047F8A0[32] = {
#include "sub_main/0047F8A0.inc"
};
char D_0047F8C0[32] = {
#include "sub_main/0047F8C0.inc"
};
char gBladeDescTexts_1211[32] = {
#include "sub_main/0047F8E0.inc"
};
char gBladeDescTexts_1212[32] = {
#include "sub_main/0047F900.inc"
};
char gBladeDescTexts_1213[32] = {
#include "sub_main/0047F920.inc"
};
char D_0047F940[32] = {
#include "sub_main/0047F940.inc"
};
char D_0047F960[32] = {
#include "sub_main/0047F960.inc"
};
char D_0047F980[32] = {
#include "sub_main/0047F980.inc"
};
char gBladeDescTexts_1219[32] = {
#include "sub_main/0047F9A0.inc"
};
char gBladeDescTexts_1220[32] = {
#include "sub_main/0047F9C0.inc"
};
char gBladeDescTexts_1221[32] = {
#include "sub_main/0047F9E0.inc"
};
char gBladeDescTexts_1223[32] = {
#include "sub_main/0047FA00.inc"
};
char gBladeDescTexts_1224[32] = {
#include "sub_main/0047FA20.inc"
};
char gBladeDescTexts_1225[32] = {
#include "sub_main/0047FA40.inc"
};
char D_0047FA60[32] = {
#include "sub_main/0047FA60.inc"
};
char D_0047FA80[32] = {
#include "sub_main/0047FA80.inc"
};
char D_0047FAA0[32] = {
#include "sub_main/0047FAA0.inc"
};
char gBladeDescTexts_1233[32] = {
#include "sub_main/0047FAC0.inc"
};
char D_0047FAE0[32] = {
#include "sub_main/0047FAE0.inc"
};
char gBladeDescTexts_1236[32] = {
#include "sub_main/0047FB00.inc"
};
char gBladeDescTexts_1237[32] = {
#include "sub_main/0047FB20.inc"
};
char D_0047FB40[32] = {
#include "sub_main/0047FB40.inc"
};
char gBladeDescTexts_1251[32] = {
#include "sub_main/0047FB60.inc"
};
char gBladeDescTexts_1252[32] = {
#include "sub_main/0047FB80.inc"
};
char gBladeDescTexts_1253[32] = {
#include "sub_main/0047FBA0.inc"
};
char D_0047FBC0[32] = {
#include "sub_main/0047FBC0.inc"
};
char D_0047FBE0[32] = {
#include "sub_main/0047FBE0.inc"
};
char D_0047FC00[32] = {
#include "sub_main/0047FC00.inc"
};
char D_0047FC20[32] = {
#include "sub_main/0047FC20.inc"
};
char D_0047FC40[32] = {
#include "sub_main/0047FC40.inc"
};
char D_0047FC60[32] = {
#include "sub_main/0047FC60.inc"
};
char D_0047FC80[32] = {
#include "sub_main/0047FC80.inc"
};
char D_0047FCA0[32] = {
#include "sub_main/0047FCA0.inc"
};
char D_0047FCC0[32] = {
#include "sub_main/0047FCC0.inc"
};
char D_0047FCE0[32] = {
#include "sub_main/0047FCE0.inc"
};
char D_0047FD00[32] = {
#include "sub_main/0047FD00.inc"
};
char D_0047FD20[32] = {
#include "sub_main/0047FD20.inc"
};
char gBladeDescTexts_1271[32] = {
#include "sub_main/0047FD40.inc"
};
char gBladeDescTexts_1272[32] = {
#include "sub_main/0047FD60.inc"
};
char gBladeDescTexts_1273[32] = {
#include "sub_main/0047FD80.inc"
};
char gBladeDescTexts_1275[32] = {
#include "sub_main/0047FDA0.inc"
};
char gBladeDescTexts_1276[32] = {
#include "sub_main/0047FDC0.inc"
};
char gBladeDescTexts_1277[32] = {
#include "sub_main/0047FDE0.inc"
};
char gBladeDescTexts_1279[32] = {
#include "sub_main/0047FE00.inc"
};
char gBladeDescTexts_1280[32] = {
#include "sub_main/0047FE20.inc"
};
char gBladeDescTexts_1281[32] = {
#include "sub_main/0047FE40.inc"
};
char D_0047FE60[32] = {
#include "sub_main/0047FE60.inc"
};
char D_0047FE80[32] = {
#include "sub_main/0047FE80.inc"
};
char gBladeDescTexts_1287[32] = {
#include "sub_main/0047FEA0.inc"
};
char gBladeDescTexts_1288[32] = {
#include "sub_main/0047FEC0.inc"
};
char gBladeDescTexts_1289[32] = {
#include "sub_main/0047FEE0.inc"
};
char gBladeDescTexts_1291[32] = {
#include "sub_main/0047FF00.inc"
};
char gBladeDescTexts_1292[32] = {
#include "sub_main/0047FF20.inc"
};
char gBladeDescTexts_1293[32] = {
#include "sub_main/0047FF40.inc"
};
char D_0047FF60[32] = {
#include "sub_main/0047FF60.inc"
};
char D_0047FF80[32] = {
#include "sub_main/0047FF80.inc"
};
char D_0047FFA0[32] = {
#include "sub_main/0047FFA0.inc"
};
char D_0047FFC0[32] = {
#include "sub_main/0047FFC0.inc"
};
char D_0047FFE0[32] = {
#include "sub_main/0047FFE0.inc"
};
char D_00480000[32] = {
#include "sub_main/00480000.inc"
};
char D_00480020[32] = {
#include "sub_main/00480020.inc"
};
char D_00480040[32] = {
#include "sub_main/00480040.inc"
};
char D_00480060[32] = {
#include "sub_main/00480060.inc"
};
char D_00480080[32] = {
#include "sub_main/00480080.inc"
};
char D_004800A0[32] = {
#include "sub_main/004800A0.inc"
};
char D_004800C0[32] = {
#include "sub_main/004800C0.inc"
};
char gBladeDescTexts_1320[32] = {
#include "sub_main/004800E0.inc"
};
char gBladeDescTexts_1321[32] = {
#include "sub_main/00480100.inc"
};
char gBladeDescTexts_1324[32] = {
#include "sub_main/00480120.inc"
};
char gBladeDescTexts_1325[32] = {
#include "sub_main/00480140.inc"
};
char D_00480160[32] = {
#include "sub_main/00480160.inc"
};
char D_00480180[32] = {
#include "sub_main/00480180.inc"
};
char D_004801A0[32] = {
#include "sub_main/004801A0.inc"
};
char D_004801C0[32] = {
#include "sub_main/004801C0.inc"
};
char gBladeDescTexts_1339[32] = {
#include "sub_main/004801E0.inc"
};
char gBladeDescTexts_1340[32] = {
#include "sub_main/00480200.inc"
};
char gBladeDescTexts_1341[32] = {
#include "sub_main/00480220.inc"
};
char D_00480240[32] = {
#include "sub_main/00480240.inc"
};
char D_00480260[32] = {
#include "sub_main/00480260.inc"
};
char D_00480280[32] = {
#include "sub_main/00480280.inc"
};
char D_004802A0[32] = {
#include "sub_main/004802A0.inc"
};
char D_004802C0[32] = {
#include "sub_main/004802C0.inc"
};
char D_004802E0[32] = {
#include "sub_main/004802E0.inc"
};
char gBladeDescTexts_1363[32] = {
#include "sub_main/00480300.inc"
};
char gBladeDescTexts_1364[32] = {
#include "sub_main/00480320.inc"
};
char gBladeDescTexts_1365[32] = {
#include "sub_main/00480340.inc"
};
char gBladeDescTexts_1367[32] = {
#include "sub_main/00480360.inc"
};
char gBladeDescTexts_1368[32] = {
#include "sub_main/00480380.inc"
};
char gBladeDescTexts_1369[32] = {
#include "sub_main/004803A0.inc"
};
char D_004803C0[32] = {
#include "sub_main/004803C0.inc"
};
char D_004803E0[32] = {
#include "sub_main/004803E0.inc"
};
char D_00480400[32] = {
#include "sub_main/00480400.inc"
};
char gBladeDescTexts_1375[32] = {
#include "sub_main/00480420.inc"
};
char gBladeDescTexts_1376[32] = {
#include "sub_main/00480440.inc"
};
char gBladeDescTexts_1377[32] = {
#include "sub_main/00480460.inc"
};
char gBladeDescTexts_1379[32] = {
#include "sub_main/00480480.inc"
};
char gBladeDescTexts_1380[32] = {
#include "sub_main/004804A0.inc"
};
char gBladeDescTexts_1381[32] = {
#include "sub_main/004804C0.inc"
};
char D_004804E0[32] = {
#include "sub_main/004804E0.inc"
};
char D_00480500[32] = {
#include "sub_main/00480500.inc"
};
char D_00480520[32] = {
#include "sub_main/00480520.inc"
};
char D_00480540[32] = {
#include "sub_main/00480540.inc"
};
char D_00480560[32] = {
#include "sub_main/00480560.inc"
};
char D_00480580[32] = {
#include "sub_main/00480580.inc"
};
char D_004805A0[32] = {
#include "sub_main/004805A0.inc"
};
char D_004805C0[32] = {
#include "sub_main/004805C0.inc"
};
char D_004805E0[32] = {
#include "sub_main/004805E0.inc"
};
char gBladeDescTexts_1395[32] = {
#include "sub_main/00480600.inc"
};
char gBladeDescTexts_1396[32] = {
#include "sub_main/00480620.inc"
};
char gBladeDescTexts_1397[32] = {
#include "sub_main/00480640.inc"
};
char D_00480660[32] = {
#include "sub_main/00480660.inc"
};
char D_00480680[32] = {
#include "sub_main/00480680.inc"
};
char D_004806A0[32] = {
#include "sub_main/004806A0.inc"
};
char D_004806C0[32] = {
#include "sub_main/004806C0.inc"
};
char D_004806E0[32] = {
#include "sub_main/004806E0.inc"
};
char D_00480700[32] = {
#include "sub_main/00480700.inc"
};
char gBladeDescTexts_1413[32] = {
#include "sub_main/00480720.inc"
};
char gBladeDescTexts_1416[32] = {
#include "sub_main/00480740.inc"
};
char gBladeDescTexts_1417[32] = {
#include "sub_main/00480760.inc"
};
char gBladeDescTexts_1427[32] = {
#include "sub_main/00480780.inc"
};
char gBladeDescTexts_1428[32] = {
#include "sub_main/004807A0.inc"
};
char gBladeDescTexts_1429[32] = {
#include "sub_main/004807C0.inc"
};
char gBladeDescTexts_1435[32] = {
#include "sub_main/004807E0.inc"
};
char gBladeDescTexts_1436[32] = {
#include "sub_main/00480800.inc"
};
char gBladeDescTexts_1437[32] = {
#include "sub_main/00480820.inc"
};
char gBladeDescTexts_1439[32] = {
#include "sub_main/00480840.inc"
};
char gBladeDescTexts_1440[32] = {
#include "sub_main/00480860.inc"
};
char gBladeDescTexts_1441[32] = {
#include "sub_main/00480880.inc"
};
char gBladeDescTexts_1443[32] = {
#include "sub_main/004808A0.inc"
};
char D_004808C0[32] = {
#include "sub_main/004808C0.inc"
};
char gBladeDescTexts_1445[32] = {
#include "sub_main/004808E0.inc"
};
char D_00480900[32] = {
#include "sub_main/00480900.inc"
};
char gBladeDescTexts_1459[32] = {
#include "sub_main/00480920.inc"
};
char gBladeDescTexts_1460[32] = {
#include "sub_main/00480940.inc"
};
char gBladeDescTexts_1461[32] = {
#include "sub_main/00480960.inc"
};
char gBladeDescTexts_1479[32] = {
#include "sub_main/00480980.inc"
};
char gBladeDescTexts_1480[32] = {
#include "sub_main/004809A0.inc"
};
char gBladeDescTexts_1481[32] = {
#include "sub_main/004809C0.inc"
};
char D_004809E0[32] = {
#include "sub_main/004809E0.inc"
};
char D_00480A00[32] = {
#include "sub_main/00480A00.inc"
};
char D_00480A20[32] = {
#include "sub_main/00480A20.inc"
};
char D_00480A40[32] = {
#include "sub_main/00480A40.inc"
};
char D_00480A60[32] = {
#include "sub_main/00480A60.inc"
};
char D_00480A80[32] = {
#include "sub_main/00480A80.inc"
};
char gBladeDescTexts_1495[32] = {
#include "sub_main/00480AA0.inc"
};
char gBladeDescTexts_1496[32] = {
#include "sub_main/00480AC0.inc"
};
char gBladeDescTexts_1497[32] = {
#include "sub_main/00480AE0.inc"
};
char D_00480B00[32] = {
#include "sub_main/00480B00.inc"
};
char D_00480B20[32] = {
#include "sub_main/00480B20.inc"
};
char D_00480B40[32] = {
#include "sub_main/00480B40.inc"
};
char D_00480B60[32] = {
#include "sub_main/00480B60.inc"
};
char D_00480B80[32] = {
#include "sub_main/00480B80.inc"
};
char D_00480BA0[32] = {
#include "sub_main/00480BA0.inc"
};
char D_00480BC0[32] = {
#include "sub_main/00480BC0.inc"
};
char D_00480BE0[32] = {
#include "sub_main/00480BE0.inc"
};
char D_00480C00[32] = {
#include "sub_main/00480C00.inc"
};
char gBladeDescTexts_1515[32] = {
#include "sub_main/00480C20.inc"
};
char gBladeDescTexts_1516[32] = {
#include "sub_main/00480C40.inc"
};
char gBladeDescTexts_1517[32] = {
#include "sub_main/00480C60.inc"
};
char D_00480C80[32] = {
#include "sub_main/00480C80.inc"
};
char D_00480CA0[32] = {
#include "sub_main/00480CA0.inc"
};
char D_00480CC0[32] = {
#include "sub_main/00480CC0.inc"
};
char D_00480CE0[32] = {
#include "sub_main/00480CE0.inc"
};
char gBladeDescTexts_1524[32] = {
#include "sub_main/00480D00.inc"
};
char gBladeDescTexts_1525[32] = {
#include "sub_main/00480D20.inc"
};
char gBladeDescTexts_1609[48] = {
#include "sub_main/00480D40.inc"
};
char gBladeDescTexts_1621[48] = {
#include "sub_main/00480D70.inc"
};
char gBladeDescTexts_1625[32] = {
#include "sub_main/00480DA0.inc"
};
char gBladeDescTexts_1629[48] = {
#include "sub_main/00480DC0.inc"
};
char gBladeDescTexts_1633[48] = {
#include "sub_main/00480DF0.inc"
};
char gBladeDescTexts_1735[32] = {
#include "sub_main/00480E20.inc"
};
char gBladeDescTexts_1736[32] = {
#include "sub_main/00480E40.inc"
};
char gBladeDescTexts_1737[32] = {
#include "sub_main/00480E60.inc"
};
char gBladeDescTexts_1739[32] = {
#include "sub_main/00480E80.inc"
};
char gBladeDescTexts_1740[32] = {
#include "sub_main/00480EA0.inc"
};
char gBladeDescTexts_1741[32] = {
#include "sub_main/00480EC0.inc"
};
char gBladeDescTexts_1743[32] = {
#include "sub_main/00480EE0.inc"
};
char gBladeDescTexts_1744[32] = {
#include "sub_main/00480F00.inc"
};
char gBladeDescTexts_1745[32] = {
#include "sub_main/00480F20.inc"
};
char gBladeDescTexts_1751[32] = {
#include "sub_main/00480F40.inc"
};
char gBladeDescTexts_1752[32] = {
#include "sub_main/00480F60.inc"
};
char gBladeDescTexts_1753[32] = {
#include "sub_main/00480F80.inc"
};
char gBladeDescTexts_1847[32] = {
#include "sub_main/00480FA0.inc"
};
char gBladeDescTexts_1848[32] = {
#include "sub_main/00480FC0.inc"
};
char gBladeDescTexts_1849[32] = {
#include "sub_main/00480FE0.inc"
};
char gBladeDescTexts_1855[32] = {
#include "sub_main/00481000.inc"
};
char gBladeDescTexts_1856[32] = {
#include "sub_main/00481020.inc"
};
char gBladeDescTexts_1857[32] = {
#include "sub_main/00481040.inc"
};
char gBladeDescTexts_1869[32] = {
#include "sub_main/00481060.inc"
};
char gBladeDescTexts_1884[32] = {
#include "sub_main/00481080.inc"
};
char gBladeDescTexts_1885[32] = {
#include "sub_main/004810A0.inc"
};
char gBladeDescTexts_1887[32] = {
#include "sub_main/004810C0.inc"
};
char gBladeDescTexts_1888[32] = {
#include "sub_main/004810E0.inc"
};
char gBladeDescTexts_1889[32] = {
#include "sub_main/00481100.inc"
};
char D_00481120[32] = {
#include "sub_main/00481120.inc"
};
char gBladeDescTexts_1895[32] = {
#include "sub_main/00481140.inc"
};
char gBladeDescTexts_1896[32] = {
#include "sub_main/00481160.inc"
};
char gBladeDescTexts_1897[32] = {
#include "sub_main/00481180.inc"
};
char D_004811A0[32] = {
#include "sub_main/004811A0.inc"
};
char D_004811C0[32] = {
#include "sub_main/004811C0.inc"
};
char D_004811E0[32] = {
#include "sub_main/004811E0.inc"
};
char gBladeDescTexts_1936[32] = {
#include "sub_main/00481200.inc"
};
char gBladeDescTexts_1937[32] = {
#include "sub_main/00481220.inc"
};
char gBladeDescTexts_1939[32] = {
#include "sub_main/00481240.inc"
};
char gGunDescTexts_0[16] = {
#include "sub_main/00481260.inc"
};
char gGunDescTexts_4[32] = {
#include "sub_main/00481270.inc"
};
char gGunDescTexts_5[32] = {
#include "sub_main/00481290.inc"
};
char gGunDescTexts_6[32] = {
#include "sub_main/004812B0.inc"
};
char gGunDescTexts_8[32] = {
#include "sub_main/004812D0.inc"
};
char gGunDescTexts_9[48] = {
#include "sub_main/004812F0.inc"
};
char gGunDescTexts_10[32] = {
#include "sub_main/00481320.inc"
};
char gGunDescTexts_12[32] = {
#include "sub_main/00481340.inc"
};
char gGunDescTexts_13[32] = {
#include "sub_main/00481360.inc"
};
char gGunDescTexts_14[32] = {
#include "sub_main/00481380.inc"
};
char D_004813A0[32] = {
#include "sub_main/004813A0.inc"
};
char gGunDescTexts_17[32] = {
#include "sub_main/004813C0.inc"
};
char gGunDescTexts_18[32] = {
#include "sub_main/004813E0.inc"
};
char gGunDescTexts_21[32] = {
#include "sub_main/00481400.inc"
};
char gGunDescTexts_22[32] = {
#include "sub_main/00481420.inc"
};
char gGunDescTexts_24[32] = {
#include "sub_main/00481440.inc"
};
char gGunDescTexts_25[32] = {
#include "sub_main/00481460.inc"
};
char gGunDescTexts_26[32] = {
#include "sub_main/00481480.inc"
};
char gGunDescTexts_28[32] = {
#include "sub_main/004814A0.inc"
};
char D_004814C0[32] = {
#include "sub_main/004814C0.inc"
};
char gGunDescTexts_30[16] = {
#include "sub_main/004814E0.inc"
};
char gGunDescTexts_32[32] = {
#include "sub_main/004814F0.inc"
};
char gGunDescTexts_34[16] = {
#include "sub_main/00481510.inc"
};
char gGunDescTexts_36[32] = {
#include "sub_main/00481520.inc"
};
char gGunDescTexts_37[32] = {
#include "sub_main/00481540.inc"
};
char gGunDescTexts_38[32] = {
#include "sub_main/00481560.inc"
};
char gGunDescTexts_40[32] = {
#include "sub_main/00481580.inc"
};
char gGunDescTexts_41[32] = {
#include "sub_main/004815A0.inc"
};
char gGunDescTexts_42[32] = {
#include "sub_main/004815C0.inc"
};
char gGunDescTexts_44[32] = {
#include "sub_main/004815E0.inc"
};
char gGunDescTexts_45[32] = {
#include "sub_main/00481600.inc"
};
char gGunDescTexts_46[48] = {
#include "sub_main/00481620.inc"
};
char gGunDescTexts_48[32] = {
#include "sub_main/00481650.inc"
};
char gGunDescTexts_49[32] = {
#include "sub_main/00481670.inc"
};
char D_00481690[16] = {
#include "sub_main/00481690.inc"
};
char gGunDescTexts_52[32] = {
#include "sub_main/004816A0.inc"
};
char gGunDescTexts_53[32] = {
#include "sub_main/004816C0.inc"
};
char gGunDescTexts_54[32] = {
#include "sub_main/004816E0.inc"
};
char gGunDescTexts_56[32] = {
#include "sub_main/00481700.inc"
};
char gGunDescTexts_57[32] = {
#include "sub_main/00481720.inc"
};
char gGunDescTexts_58[16] = {
#include "sub_main/00481740.inc"
};
char gGunDescTexts_60[32] = {
#include "sub_main/00481750.inc"
};
char gGunDescTexts_61[32] = {
#include "sub_main/00481770.inc"
};
char gGunDescTexts_62[32] = {
#include "sub_main/00481790.inc"
};
char gGunDescTexts_64[32] = {
#include "sub_main/004817B0.inc"
};
char gGunDescTexts_65[32] = {
#include "sub_main/004817D0.inc"
};
char gGunDescTexts_66[32] = {
#include "sub_main/004817F0.inc"
};
char gGunDescTexts_68[32] = {
#include "sub_main/00481810.inc"
};
char gGunDescTexts_69[32] = {
#include "sub_main/00481830.inc"
};
char gGunDescTexts_70[32] = {
#include "sub_main/00481850.inc"
};
char gGunDescTexts_72[32] = {
#include "sub_main/00481870.inc"
};
char gGunDescTexts_73[32] = {
#include "sub_main/00481890.inc"
};
char gGunDescTexts_74[32] = {
#include "sub_main/004818B0.inc"
};
char gGunDescTexts_76[32] = {
#include "sub_main/004818D0.inc"
};
char gGunDescTexts_77[32] = {
#include "sub_main/004818F0.inc"
};
char gGunDescTexts_78[32] = {
#include "sub_main/00481910.inc"
};
char gGunDescTexts_81[32] = {
#include "sub_main/00481930.inc"
};
char gGunDescTexts_82[32] = {
#include "sub_main/00481950.inc"
};
char gGunDescTexts_84[32] = {
#include "sub_main/00481970.inc"
};
char gGunDescTexts_85[32] = {
#include "sub_main/00481990.inc"
};
char gGunDescTexts_86[32] = {
#include "sub_main/004819B0.inc"
};
char gGunDescTexts_88[32] = {
#include "sub_main/004819D0.inc"
};
char gGunDescTexts_89[32] = {
#include "sub_main/004819F0.inc"
};
char gGunDescTexts_90[32] = {
#include "sub_main/00481A10.inc"
};
char gGunDescTexts_92[32] = {
#include "sub_main/00481A30.inc"
};
char gGunDescTexts_93[32] = {
#include "sub_main/00481A50.inc"
};
char gGunDescTexts_96[32] = {
#include "sub_main/00481A70.inc"
};
char gGunDescTexts_97[32] = {
#include "sub_main/00481A90.inc"
};
char gGunDescTexts_98[32] = {
#include "sub_main/00481AB0.inc"
};
char gGunDescTexts_100[48] = {
#include "sub_main/00481AD0.inc"
};
char gGunDescTexts_101[32] = {
#include "sub_main/00481B00.inc"
};
char gGunDescTexts_102[32] = {
#include "sub_main/00481B20.inc"
};
char gGunDescTexts_104[32] = {
#include "sub_main/00481B40.inc"
};
char D_00481B60[32] = {
#include "sub_main/00481B60.inc"
};
char gGunDescTexts_106[32] = {
#include "sub_main/00481B80.inc"
};
char gGunDescTexts_108[32] = {
#include "sub_main/00481BA0.inc"
};
char gGunDescTexts_109[32] = {
#include "sub_main/00481BC0.inc"
};
char gGunDescTexts_110[32] = {
#include "sub_main/00481BE0.inc"
};
char gGunDescTexts_112[32] = {
#include "sub_main/00481C00.inc"
};
char gGunDescTexts_113[32] = {
#include "sub_main/00481C20.inc"
};
char gGunDescTexts_114[32] = {
#include "sub_main/00481C40.inc"
};
char gGunDescTexts_116[48] = {
#include "sub_main/00481C60.inc"
};
char D_00481C90[32] = {
#include "sub_main/00481C90.inc"
};
char D_00481CB0[32] = {
#include "sub_main/00481CB0.inc"
};
char D_00481CD0[32] = {
#include "sub_main/00481CD0.inc"
};
char gGunDescTexts_121[32] = {
#include "sub_main/00481CF0.inc"
};
char gGunDescTexts_122[32] = {
#include "sub_main/00481D10.inc"
};
char D_00481D30[32] = {
#include "sub_main/00481D30.inc"
};
char gGunDescTexts_125[32] = {
#include "sub_main/00481D50.inc"
};
char gGunDescTexts_126[48] = {
#include "sub_main/00481D70.inc"
};
char gGunDescTexts_128[48] = {
#include "sub_main/00481DA0.inc"
};
char gGunDescTexts_129[32] = {
#include "sub_main/00481DD0.inc"
};
char gGunDescTexts_130[32] = {
#include "sub_main/00481DF0.inc"
};
char gGunDescTexts_132[48] = {
#include "sub_main/00481E10.inc"
};
char gGunDescTexts_137[32] = {
#include "sub_main/00481E40.inc"
};
char gGunDescTexts_138[32] = {
#include "sub_main/00481E60.inc"
};
char gGunDescTexts_141[32] = {
#include "sub_main/00481E80.inc"
};
char gGunDescTexts_142[48] = {
#include "sub_main/00481EA0.inc"
};
char gGunDescTexts_144[48] = {
#include "sub_main/00481ED0.inc"
};
char gGunDescTexts_145[32] = {
#include "sub_main/00481F00.inc"
};
char gGunDescTexts_146[32] = {
#include "sub_main/00481F20.inc"
};
char gGunDescTexts_148[32] = {
#include "sub_main/00481F40.inc"
};
char gGunDescTexts_149[32] = {
#include "sub_main/00481F60.inc"
};
char gGunDescTexts_150[32] = {
#include "sub_main/00481F80.inc"
};
char gGunDescTexts_152[32] = {
#include "sub_main/00481FA0.inc"
};
char gGunDescTexts_153[48] = {
#include "sub_main/00481FC0.inc"
};
char gGunDescTexts_154[48] = {
#include "sub_main/00481FF0.inc"
};
char gGunDescTexts_156[32] = {
#include "sub_main/00482020.inc"
};
char gGunDescTexts_157[32] = {
#include "sub_main/00482040.inc"
};
char gGunDescTexts_158[48] = {
#include "sub_main/00482060.inc"
};
char gGunDescTexts_160[48] = {
#include "sub_main/00482090.inc"
};
char gGunDescTexts_161[48] = {
#include "sub_main/004820C0.inc"
};
char gGunDescTexts_162[32] = {
#include "sub_main/004820F0.inc"
};
char gGunDescTexts_164[32] = {
#include "sub_main/00482110.inc"
};
char gGunDescTexts_165[48] = {
#include "sub_main/00482130.inc"
};
char gGunDescTexts_166[48] = {
#include "sub_main/00482160.inc"
};
char gGunDescTexts_168[32] = {
#include "sub_main/00482190.inc"
};
char gGunDescTexts_169[32] = {
#include "sub_main/004821B0.inc"
};
char gGunDescTexts_170[32] = {
#include "sub_main/004821D0.inc"
};
char gGunDescTexts_172[32] = {
#include "sub_main/004821F0.inc"
};
char gGunDescTexts_173[32] = {
#include "sub_main/00482210.inc"
};
char gGunDescTexts_174[32] = {
#include "sub_main/00482230.inc"
};
char gGunDescTexts_176[32] = {
#include "sub_main/00482250.inc"
};
char gGunDescTexts_177[32] = {
#include "sub_main/00482270.inc"
};
char gGunDescTexts_178[32] = {
#include "sub_main/00482290.inc"
};
char gGunDescTexts_180[32] = {
#include "sub_main/004822B0.inc"
};
char gGunDescTexts_181[48] = {
#include "sub_main/004822D0.inc"
};
char gGunDescTexts_182[32] = {
#include "sub_main/00482300.inc"
};
char gGunDescTexts_184[32] = {
#include "sub_main/00482320.inc"
};
char gGunDescTexts_185[32] = {
#include "sub_main/00482340.inc"
};
char gGunDescTexts_186[32] = {
#include "sub_main/00482360.inc"
};
char gGunDescTexts_188[32] = {
#include "sub_main/00482380.inc"
};
char gGunDescTexts_189[48] = {
#include "sub_main/004823A0.inc"
};
char gGunDescTexts_190[32] = {
#include "sub_main/004823D0.inc"
};
char gGunDescTexts_192[32] = {
#include "sub_main/004823F0.inc"
};
char gGunDescTexts_193[32] = {
#include "sub_main/00482410.inc"
};
char gGunDescTexts_194[32] = {
#include "sub_main/00482430.inc"
};
char gGunDescTexts_196[32] = {
#include "sub_main/00482450.inc"
};
char gGunDescTexts_197[48] = {
#include "sub_main/00482470.inc"
};
char gGunDescTexts_198[32] = {
#include "sub_main/004824A0.inc"
};
char gGunDescTexts_200[32] = {
#include "sub_main/004824C0.inc"
};
char gGunDescTexts_201[32] = {
#include "sub_main/004824E0.inc"
};
char gGunDescTexts_202[48] = {
#include "sub_main/00482500.inc"
};
char gGunDescTexts_204[32] = {
#include "sub_main/00482530.inc"
};
char gGunDescTexts_205[32] = {
#include "sub_main/00482550.inc"
};
char gGunDescTexts_206[32] = {
#include "sub_main/00482570.inc"
};
char gGunDescTexts_208[32] = {
#include "sub_main/00482590.inc"
};
char gGunDescTexts_209[32] = {
#include "sub_main/004825B0.inc"
};
char gGunDescTexts_210[48] = {
#include "sub_main/004825D0.inc"
};
char gGunDescTexts_212[32] = {
#include "sub_main/00482600.inc"
};
char gGunDescTexts_213[32] = {
#include "sub_main/00482620.inc"
};
char gGunDescTexts_214[32] = {
#include "sub_main/00482640.inc"
};
char gGunDescTexts_216[32] = {
#include "sub_main/00482660.inc"
};
char gGunDescTexts_217[32] = {
#include "sub_main/00482680.inc"
};
char gGunDescTexts_218[32] = {
#include "sub_main/004826A0.inc"
};
char gGunDescTexts_222[48] = {
#include "sub_main/004826C0.inc"
};
char gGunDescTexts_225[32] = {
#include "sub_main/004826F0.inc"
};
char gGunDescTexts_226[32] = {
#include "sub_main/00482710.inc"
};
char gGunDescTexts_228[32] = {
#include "sub_main/00482730.inc"
};
char gGunDescTexts_229[32] = {
#include "sub_main/00482750.inc"
};
char gGunDescTexts_230[32] = {
#include "sub_main/00482770.inc"
};
char gGunDescTexts_232[32] = {
#include "sub_main/00482790.inc"
};
char gGunDescTexts_233[32] = {
#include "sub_main/004827B0.inc"
};
char D_004827D0[32] = {
#include "sub_main/004827D0.inc"
};
char gGunDescTexts_236[32] = {
#include "sub_main/004827F0.inc"
};
char gGunDescTexts_237[32] = {
#include "sub_main/00482810.inc"
};
char gGunDescTexts_240[32] = {
#include "sub_main/00482830.inc"
};
char gGunDescTexts_241[32] = {
#include "sub_main/00482850.inc"
};
char gGunDescTexts_242[48] = {
#include "sub_main/00482870.inc"
};
char gGunDescTexts_244[32] = {
#include "sub_main/004828A0.inc"
};
char gGunDescTexts_245[32] = {
#include "sub_main/004828C0.inc"
};
char gGunDescTexts_246[32] = {
#include "sub_main/004828E0.inc"
};
char gGunDescTexts_248[32] = {
#include "sub_main/00482900.inc"
};
char gGunDescTexts_249[32] = {
#include "sub_main/00482920.inc"
};
char gGunDescTexts_250[32] = {
#include "sub_main/00482940.inc"
};
char gGunDescTexts_252[32] = {
#include "sub_main/00482960.inc"
};
char gGunDescTexts_253[32] = {
#include "sub_main/00482980.inc"
};
char gGunDescTexts_254[32] = {
#include "sub_main/004829A0.inc"
};
char gGunDescTexts_256[32] = {
#include "sub_main/004829C0.inc"
};
char gGunDescTexts_257[32] = {
#include "sub_main/004829E0.inc"
};
char gGunDescTexts_258[32] = {
#include "sub_main/00482A00.inc"
};
char gGunDescTexts_260[32] = {
#include "sub_main/00482A20.inc"
};
char gGunDescTexts_261[32] = {
#include "sub_main/00482A40.inc"
};
char gGunDescTexts_262[32] = {
#include "sub_main/00482A60.inc"
};
char gGunDescTexts_264[32] = {
#include "sub_main/00482A80.inc"
};
char gGunDescTexts_265[32] = {
#include "sub_main/00482AA0.inc"
};
char gGunDescTexts_266[32] = {
#include "sub_main/00482AC0.inc"
};
char gGunDescTexts_268[32] = {
#include "sub_main/00482AE0.inc"
};
char gGunDescTexts_269[32] = {
#include "sub_main/00482B00.inc"
};
char gGunDescTexts_270[32] = {
#include "sub_main/00482B20.inc"
};
char gGunDescTexts_272[32] = {
#include "sub_main/00482B40.inc"
};
char gGunDescTexts_273[32] = {
#include "sub_main/00482B60.inc"
};
char gGunDescTexts_274[32] = {
#include "sub_main/00482B80.inc"
};
char gGunDescTexts_276[32] = {
#include "sub_main/00482BA0.inc"
};
char gGunDescTexts_277[32] = {
#include "sub_main/00482BC0.inc"
};
char gGunDescTexts_278[32] = {
#include "sub_main/00482BE0.inc"
};
char gGunDescTexts_280[32] = {
#include "sub_main/00482C00.inc"
};
char gGunDescTexts_281[32] = {
#include "sub_main/00482C20.inc"
};
char gGunDescTexts_282[32] = {
#include "sub_main/00482C40.inc"
};
char gGunDescTexts_284[32] = {
#include "sub_main/00482C60.inc"
};
char gGunDescTexts_285[32] = {
#include "sub_main/00482C80.inc"
};
char gGunDescTexts_286[32] = {
#include "sub_main/00482CA0.inc"
};
char gGunDescTexts_288[32] = {
#include "sub_main/00482CC0.inc"
};
char gGunDescTexts_289[32] = {
#include "sub_main/00482CE0.inc"
};
char gGunDescTexts_290[32] = {
#include "sub_main/00482D00.inc"
};
char gGunDescTexts_292[32] = {
#include "sub_main/00482D20.inc"
};
char gGunDescTexts_293[32] = {
#include "sub_main/00482D40.inc"
};
char gGunDescTexts_294[32] = {
#include "sub_main/00482D60.inc"
};
char gGunDescTexts_296[32] = {
#include "sub_main/00482D80.inc"
};
char gGunDescTexts_297[32] = {
#include "sub_main/00482DA0.inc"
};
char gGunDescTexts_298[32] = {
#include "sub_main/00482DC0.inc"
};
char gGunDescTexts_300[32] = {
#include "sub_main/00482DE0.inc"
};
char gGunDescTexts_301[32] = {
#include "sub_main/00482E00.inc"
};
char gGunDescTexts_302[32] = {
#include "sub_main/00482E20.inc"
};
char gGunDescTexts_304[32] = {
#include "sub_main/00482E40.inc"
};
char gGunDescTexts_306[32] = {
#include "sub_main/00482E60.inc"
};
char gGunDescTexts_308[32] = {
#include "sub_main/00482E80.inc"
};
char gGunDescTexts_309[48] = {
#include "sub_main/00482EA0.inc"
};
char gGunDescTexts_310[32] = {
#include "sub_main/00482ED0.inc"
};
char D_00482EF0[32] = {
#include "sub_main/00482EF0.inc"
};
char D_00482F10[32] = {
#include "sub_main/00482F10.inc"
};
char D_00482F30[32] = {
#include "sub_main/00482F30.inc"
};
char D_00482F50[32] = {
#include "sub_main/00482F50.inc"
};
char D_00482F70[32] = {
#include "sub_main/00482F70.inc"
};
char D_00482F90[32] = {
#include "sub_main/00482F90.inc"
};
char D_00482FB0[32] = {
#include "sub_main/00482FB0.inc"
};
char D_00482FD0[32] = {
#include "sub_main/00482FD0.inc"
};
char D_00482FF0[32] = {
#include "sub_main/00482FF0.inc"
};
char D_00483010[32] = {
#include "sub_main/00483010.inc"
};
char D_00483030[32] = {
#include "sub_main/00483030.inc"
};
char D_00483050[32] = {
#include "sub_main/00483050.inc"
};
char D_00483070[32] = {
#include "sub_main/00483070.inc"
};
char D_00483090[32] = {
#include "sub_main/00483090.inc"
};
char D_004830B0[32] = {
#include "sub_main/004830B0.inc"
};
char D_004830D0[32] = {
#include "sub_main/004830D0.inc"
};
char D_004830F0[32] = {
#include "sub_main/004830F0.inc"
};
char D_00483110[32] = {
#include "sub_main/00483110.inc"
};
char D_00483130[32] = {
#include "sub_main/00483130.inc"
};
char D_00483150[32] = {
#include "sub_main/00483150.inc"
};
char D_00483170[32] = {
#include "sub_main/00483170.inc"
};
char D_00483190[32] = {
#include "sub_main/00483190.inc"
};
char D_004831B0[32] = {
#include "sub_main/004831B0.inc"
};
char D_004831D0[32] = {
#include "sub_main/004831D0.inc"
};
char D_004831F0[32] = {
#include "sub_main/004831F0.inc"
};
char D_00483210[32] = {
#include "sub_main/00483210.inc"
};
char D_00483230[32] = {
#include "sub_main/00483230.inc"
};
char D_00483250[32] = {
#include "sub_main/00483250.inc"
};
char D_00483270[32] = {
#include "sub_main/00483270.inc"
};
char D_00483290[32] = {
#include "sub_main/00483290.inc"
};
char gGunDescTexts_352[32] = {
#include "sub_main/004832B0.inc"
};
char gGunDescTexts_353[32] = {
#include "sub_main/004832D0.inc"
};
char gGunDescTexts_354[32] = {
#include "sub_main/004832F0.inc"
};
char gGunDescTexts_356[32] = {
#include "sub_main/00483310.inc"
};
char gGunDescTexts_357[32] = {
#include "sub_main/00483330.inc"
};
char gGunDescTexts_358[32] = {
#include "sub_main/00483350.inc"
};
char gGunDescTexts_360[32] = {
#include "sub_main/00483370.inc"
};
char gGunDescTexts_361[32] = {
#include "sub_main/00483390.inc"
};
char gGunDescTexts_362[32] = {
#include "sub_main/004833B0.inc"
};
char gGunDescTexts_364[32] = {
#include "sub_main/004833D0.inc"
};
char gGunDescTexts_365[32] = {
#include "sub_main/004833F0.inc"
};
char gGunDescTexts_366[32] = {
#include "sub_main/00483410.inc"
};
char gGunDescTexts_369[32] = {
#include "sub_main/00483430.inc"
};
char gGunDescTexts_370[32] = {
#include "sub_main/00483450.inc"
};
char gGunDescTexts_372[32] = {
#include "sub_main/00483470.inc"
};
char gGunDescTexts_373[48] = {
#include "sub_main/00483490.inc"
};
char gGunDescTexts_374[32] = {
#include "sub_main/004834C0.inc"
};
char gGunDescTexts_376[32] = {
#include "sub_main/004834E0.inc"
};
char gGunDescTexts_377[48] = {
#include "sub_main/00483500.inc"
};
char gGunDescTexts_378[32] = {
#include "sub_main/00483530.inc"
};
char gGunDescTexts_380[32] = {
#include "sub_main/00483550.inc"
};
char gGunDescTexts_381[32] = {
#include "sub_main/00483570.inc"
};
char gGunDescTexts_382[48] = {
#include "sub_main/00483590.inc"
};
char gGunDescTexts_384[32] = {
#include "sub_main/004835C0.inc"
};
char gGunDescTexts_385[32] = {
#include "sub_main/004835E0.inc"
};
char gGunDescTexts_386[48] = {
#include "sub_main/00483600.inc"
};
char gGunDescTexts_388[32] = {
#include "sub_main/00483630.inc"
};
char gGunDescTexts_389[48] = {
#include "sub_main/00483650.inc"
};
char gGunDescTexts_390[32] = {
#include "sub_main/00483680.inc"
};
char gGunDescTexts_392[32] = {
#include "sub_main/004836A0.inc"
};
char gGunDescTexts_393[32] = {
#include "sub_main/004836C0.inc"
};
char gGunDescTexts_394[32] = {
#include "sub_main/004836E0.inc"
};
char gGunDescTexts_396[32] = {
#include "sub_main/00483700.inc"
};
char gGunDescTexts_397[32] = {
#include "sub_main/00483720.inc"
};
char gGunDescTexts_398[32] = {
#include "sub_main/00483740.inc"
};
char gGunDescTexts_400[32] = {
#include "sub_main/00483760.inc"
};
char gGunDescTexts_401[32] = {
#include "sub_main/00483780.inc"
};
char gGunDescTexts_402[32] = {
#include "sub_main/004837A0.inc"
};
char gGunDescTexts_404[32] = {
#include "sub_main/004837C0.inc"
};
char gGunDescTexts_405[32] = {
#include "sub_main/004837E0.inc"
};
char gGunDescTexts_406[32] = {
#include "sub_main/00483800.inc"
};
char gGunDescTexts_408[32] = {
#include "sub_main/00483820.inc"
};
char gGunDescTexts_409[32] = {
#include "sub_main/00483840.inc"
};
char gGunDescTexts_410[32] = {
#include "sub_main/00483860.inc"
};
char gGunDescTexts_412[48] = {
#include "sub_main/00483880.inc"
};
char gGunDescTexts_413[48] = {
#include "sub_main/004838B0.inc"
};
char gGunDescTexts_414[32] = {
#include "sub_main/004838E0.inc"
};
char gGunDescTexts_416[32] = {
#include "sub_main/00483900.inc"
};
char gGunDescTexts_417[48] = {
#include "sub_main/00483920.inc"
};
char gGunDescTexts_418[32] = {
#include "sub_main/00483950.inc"
};
char gGunDescTexts_420[32] = {
#include "sub_main/00483970.inc"
};
char gGunDescTexts_421[32] = {
#include "sub_main/00483990.inc"
};
char gGunDescTexts_422[32] = {
#include "sub_main/004839B0.inc"
};
char gGunDescTexts_424[32] = {
#include "sub_main/004839D0.inc"
};
char gGunDescTexts_425[32] = {
#include "sub_main/004839F0.inc"
};
char gGunDescTexts_426[32] = {
#include "sub_main/00483A10.inc"
};
char D_00483A30[32] = {
#include "sub_main/00483A30.inc"
};
char D_00483A50[32] = {
#include "sub_main/00483A50.inc"
};
char D_00483A70[32] = {
#include "sub_main/00483A70.inc"
};
char gGunDescTexts_488[32] = {
#include "sub_main/00483A90.inc"
};
char gGunDescTexts_489[32] = {
#include "sub_main/00483AB0.inc"
};
char gGunDescTexts_490[32] = {
#include "sub_main/00483AD0.inc"
};
char gGunDescTexts_492[32] = {
#include "sub_main/00483AF0.inc"
};
char gGunDescTexts_493[32] = {
#include "sub_main/00483B10.inc"
};
char gGunDescTexts_494[32] = {
#include "sub_main/00483B30.inc"
};
char gGunDescTexts_496[32] = {
#include "sub_main/00483B50.inc"
};
char gGunDescTexts_497[32] = {
#include "sub_main/00483B70.inc"
};
char gGunDescTexts_498[32] = {
#include "sub_main/00483B90.inc"
};
char gGunDescTexts_500[32] = {
#include "sub_main/00483BB0.inc"
};
char gGunDescTexts_501[32] = {
#include "sub_main/00483BD0.inc"
};
char gGunDescTexts_502[32] = {
#include "sub_main/00483BF0.inc"
};
char gGunDescTexts_504[32] = {
#include "sub_main/00483C10.inc"
};
char gGunDescTexts_505[32] = {
#include "sub_main/00483C30.inc"
};
char gGunDescTexts_506[32] = {
#include "sub_main/00483C50.inc"
};
char gGunDescTexts_508[32] = {
#include "sub_main/00483C70.inc"
};
char gGunDescTexts_509[32] = {
#include "sub_main/00483C90.inc"
};
char gGunDescTexts_510[32] = {
#include "sub_main/00483CB0.inc"
};
char D_00483CD0[32] = {
#include "sub_main/00483CD0.inc"
};
char D_00483CF0[24] = {
#include "sub_main/00483CF0.inc"
};
char D_00483D08[13] = {
#include "sub_main/00483D08.inc"
};
char D_00483D20[32] = {
#include "sub_main/00483D20.inc"
};
char D_00483D40[32] = {
#include "sub_main/00483D40.inc"
};
char D_00483D60[32] = {
#include "sub_main/00483D60.inc"
};
char D_00483D80[32] = {
#include "sub_main/00483D80.inc"
};
char D_00483DA0[32] = {
#include "sub_main/00483DA0.inc"
};
char D_00483DC0[32] = {
#include "sub_main/00483DC0.inc"
};
char D_00483DE0[32] = {
#include "sub_main/00483DE0.inc"
};
char D_00483E00[32] = {
#include "sub_main/00483E00.inc"
};
char D_00483E20[32] = {
#include "sub_main/00483E20.inc"
};
char D_00483E40[32] = {
#include "sub_main/00483E40.inc"
};
char D_00483E60[32] = {
#include "sub_main/00483E60.inc"
};
char D_00483E80[32] = {
#include "sub_main/00483E80.inc"
};
char D_00483EA0[32] = {
#include "sub_main/00483EA0.inc"
};
char D_00483EC0[32] = {
#include "sub_main/00483EC0.inc"
};
char D_00483EE0[32] = {
#include "sub_main/00483EE0.inc"
};
char D_00483F00[16] = {
#include "sub_main/00483F00.inc"
};
char D_00483F10[32] = {
#include "sub_main/00483F10.inc"
};
char D_00483F30[32] = {
#include "sub_main/00483F30.inc"
};
char D_00483F50[32] = {
#include "sub_main/00483F50.inc"
};
char D_00483F70[32] = {
#include "sub_main/00483F70.inc"
};
char D_00483F90[32] = {
#include "sub_main/00483F90.inc"
};
char D_00483FB0[32] = {
#include "sub_main/00483FB0.inc"
};
char D_00483FD0[32] = {
#include "sub_main/00483FD0.inc"
};
char D_00483FF0[32] = {
#include "sub_main/00483FF0.inc"
};
char D_00484010[32] = {
#include "sub_main/00484010.inc"
};
char D_00484030[32] = {
#include "sub_main/00484030.inc"
};
char D_00484050[32] = {
#include "sub_main/00484050.inc"
};
char D_00484070[32] = {
#include "sub_main/00484070.inc"
};
char D_00484090[32] = {
#include "sub_main/00484090.inc"
};
char D_004840B0[32] = {
#include "sub_main/004840B0.inc"
};
char D_004840D0[32] = {
#include "sub_main/004840D0.inc"
};
char D_004840F0[32] = {
#include "sub_main/004840F0.inc"
};
char D_00484110[32] = {
#include "sub_main/00484110.inc"
};
char D_00484130[32] = {
#include "sub_main/00484130.inc"
};
char D_00484150[32] = {
#include "sub_main/00484150.inc"
};
char D_00484170[32] = {
#include "sub_main/00484170.inc"
};
char D_00484190[32] = {
#include "sub_main/00484190.inc"
};
char D_004841B0[32] = {
#include "sub_main/004841B0.inc"
};
char D_004841D0[32] = {
#include "sub_main/004841D0.inc"
};
char D_004841F0[32] = {
#include "sub_main/004841F0.inc"
};
char D_00484210[32] = {
#include "sub_main/00484210.inc"
};
char D_00484230[32] = {
#include "sub_main/00484230.inc"
};
char D_00484250[32] = {
#include "sub_main/00484250.inc"
};
char D_00484270[32] = {
#include "sub_main/00484270.inc"
};
char D_00484290[32] = {
#include "sub_main/00484290.inc"
};
char D_004842B0[32] = {
#include "sub_main/004842B0.inc"
};
char D_004842D0[32] = {
#include "sub_main/004842D0.inc"
};
char D_004842F0[32] = {
#include "sub_main/004842F0.inc"
};
char D_00484310[32] = {
#include "sub_main/00484310.inc"
};
char D_00484330[32] = {
#include "sub_main/00484330.inc"
};
char D_00484350[32] = {
#include "sub_main/00484350.inc"
};
char D_00484370[32] = {
#include "sub_main/00484370.inc"
};
char D_00484390[32] = {
#include "sub_main/00484390.inc"
};
char D_004843B0[32] = {
#include "sub_main/004843B0.inc"
};
char D_004843D0[32] = {
#include "sub_main/004843D0.inc"
};
char D_004843F0[32] = {
#include "sub_main/004843F0.inc"
};
char D_00484410[32] = {
#include "sub_main/00484410.inc"
};
char D_00484430[32] = {
#include "sub_main/00484430.inc"
};
char D_00484450[32] = {
#include "sub_main/00484450.inc"
};
char D_00484470[32] = {
#include "sub_main/00484470.inc"
};
char D_00484490[32] = {
#include "sub_main/00484490.inc"
};
char D_004844B0[32] = {
#include "sub_main/004844B0.inc"
};
char D_004844D0[32] = {
#include "sub_main/004844D0.inc"
};
char D_004844F0[32] = {
#include "sub_main/004844F0.inc"
};
char D_00484510[32] = {
#include "sub_main/00484510.inc"
};
char D_00484530[32] = {
#include "sub_main/00484530.inc"
};
char D_00484550[32] = {
#include "sub_main/00484550.inc"
};
char D_00484570[32] = {
#include "sub_main/00484570.inc"
};
char D_00484590[16] = {
#include "sub_main/00484590.inc"
};
char D_004845A0[32] = {
#include "sub_main/004845A0.inc"
};
char D_004845C0[32] = {
#include "sub_main/004845C0.inc"
};
char D_004845E0[32] = {
#include "sub_main/004845E0.inc"
};
char D_00484600[32] = {
#include "sub_main/00484600.inc"
};
char D_00484620[32] = {
#include "sub_main/00484620.inc"
};
char D_00484640[32] = {
#include "sub_main/00484640.inc"
};
char D_00484660[32] = {
#include "sub_main/00484660.inc"
};
char D_00484680[32] = {
#include "sub_main/00484680.inc"
};
char D_004846A0[32] = {
#include "sub_main/004846A0.inc"
};
char D_004846C0[32] = {
#include "sub_main/004846C0.inc"
};
char D_004846E0[32] = {
#include "sub_main/004846E0.inc"
};
char D_00484700[32] = {
#include "sub_main/00484700.inc"
};
char D_00484720[32] = {
#include "sub_main/00484720.inc"
};
char D_00484740[32] = {
#include "sub_main/00484740.inc"
};
char gArmorDescTexts_128[32] = {
#include "sub_main/00484760.inc"
};
char gArmorDescTexts_129[32] = {
#include "sub_main/00484780.inc"
};
char D_004847A0[32] = {
#include "sub_main/004847A0.inc"
};
char D_004847C0[32] = {
#include "sub_main/004847C0.inc"
};
char D_004847E0[32] = {
#include "sub_main/004847E0.inc"
};
char D_00484800[32] = {
#include "sub_main/00484800.inc"
};
char D_00484820[32] = {
#include "sub_main/00484820.inc"
};
char D_00484840[32] = {
#include "sub_main/00484840.inc"
};
char D_00484860[32] = {
#include "sub_main/00484860.inc"
};
char D_00484880[32] = {
#include "sub_main/00484880.inc"
};
char D_004848A0[32] = {
#include "sub_main/004848A0.inc"
};
char D_004848C0[32] = {
#include "sub_main/004848C0.inc"
};
char D_004848E0[32] = {
#include "sub_main/004848E0.inc"
};
char D_00484900[32] = {
#include "sub_main/00484900.inc"
};
char D_00484920[32] = {
#include "sub_main/00484920.inc"
};
char D_00484940[32] = {
#include "sub_main/00484940.inc"
};
char D_00484960[32] = {
#include "sub_main/00484960.inc"
};
char D_00484980[32] = {
#include "sub_main/00484980.inc"
};
char D_004849A0[16] = {
#include "sub_main/004849A0.inc"
};
char D_004849B0[32] = {
#include "sub_main/004849B0.inc"
};
char D_004849D0[32] = {
#include "sub_main/004849D0.inc"
};
char D_004849F0[32] = {
#include "sub_main/004849F0.inc"
};
char D_00484A10[32] = {
#include "sub_main/00484A10.inc"
};
char D_00484A30[32] = {
#include "sub_main/00484A30.inc"
};
char D_00484A50[32] = {
#include "sub_main/00484A50.inc"
};
char D_00484A70[32] = {
#include "sub_main/00484A70.inc"
};
char D_00484A90[32] = {
#include "sub_main/00484A90.inc"
};
char gArmorDescTexts_167[32] = {
#include "sub_main/00484AB0.inc"
};
char gArmorDescTexts_168[16] = {
#include "sub_main/00484AD0.inc"
};
char gArmorDescTexts_171[32] = {
#include "sub_main/00484AE0.inc"
};
char gArmorDescTexts_172[32] = {
#include "sub_main/00484B00.inc"
};
char gArmorDescTexts_173[16] = {
#include "sub_main/00484B20.inc"
};
char gArmorDescTexts_175[32] = {
#include "sub_main/00484B30.inc"
};
char gArmorDescTexts_176[32] = {
#include "sub_main/00484B50.inc"
};
char gArmorDescTexts_177[32] = {
#include "sub_main/00484B70.inc"
};
char gArmorDescTexts_179[32] = {
#include "sub_main/00484B90.inc"
};
char gArmorDescTexts_180[32] = {
#include "sub_main/00484BB0.inc"
};
char gArmorDescTexts_181[32] = {
#include "sub_main/00484BD0.inc"
};
char gArmorDescTexts_183[32] = {
#include "sub_main/00484BF0.inc"
};
char gArmorDescTexts_184[32] = {
#include "sub_main/00484C10.inc"
};
char gArmorDescTexts_185[32] = {
#include "sub_main/00484C30.inc"
};
char D_00484C50[32] = {
#include "sub_main/00484C50.inc"
};
char gArmorDescTexts_188[32] = {
#include "sub_main/00484C70.inc"
};
char gArmorDescTexts_189[32] = {
#include "sub_main/00484C90.inc"
};
char D_00484CB0[32] = {
#include "sub_main/00484CB0.inc"
};
char D_00484CD0[32] = {
#include "sub_main/00484CD0.inc"
};
char D_00484CF0[32] = {
#include "sub_main/00484CF0.inc"
};
char gArmorDescTexts_195[32] = {
#include "sub_main/00484D10.inc"
};
char D_00484D30[16] = {
#include "sub_main/00484D30.inc"
};
char gArmorDescTexts_197[32] = {
#include "sub_main/00484D40.inc"
};
char gArmorDescTexts_199[32] = {
#include "sub_main/00484D60.inc"
};
char gArmorDescTexts_201[32] = {
#include "sub_main/00484D80.inc"
};
char gArmorDescTexts_203[32] = {
#include "sub_main/00484DA0.inc"
};
char gArmorDescTexts_204[32] = {
#include "sub_main/00484DC0.inc"
};
char gArmorDescTexts_205[32] = {
#include "sub_main/00484DE0.inc"
};
char D_00484E00[32] = {
#include "sub_main/00484E00.inc"
};
char gArmorDescTexts_208[32] = {
#include "sub_main/00484E20.inc"
};
char gArmorDescTexts_209[32] = {
#include "sub_main/00484E40.inc"
};
char gArmorDescTexts_212[32] = {
#include "sub_main/00484E60.inc"
};
char gArmorDescTexts_213[32] = {
#include "sub_main/00484E80.inc"
};
char gArmorDescTexts_216[32] = {
#include "sub_main/00484EA0.inc"
};
char gArmorDescTexts_217[32] = {
#include "sub_main/00484EC0.inc"
};
char gArmorDescTexts_220[32] = {
#include "sub_main/00484EE0.inc"
};
char gArmorDescTexts_221[32] = {
#include "sub_main/00484F00.inc"
};
char D_00484F20[32] = {
#include "sub_main/00484F20.inc"
};
char D_00484F40[32] = {
#include "sub_main/00484F40.inc"
};
char D_00484F60[32] = {
#include "sub_main/00484F60.inc"
};
char D_00484F80[32] = {
#include "sub_main/00484F80.inc"
};
char gArmorDescTexts_232[32] = {
#include "sub_main/00484FA0.inc"
};
char gArmorDescTexts_233[32] = {
#include "sub_main/00484FC0.inc"
};
char gArmorDescTexts_236[32] = {
#include "sub_main/00484FE0.inc"
};
char gArmorDescTexts_237[32] = {
#include "sub_main/00485000.inc"
};
char gArmorDescTexts_239[32] = {
#include "sub_main/00485020.inc"
};
char gArmorDescTexts_240[32] = {
#include "sub_main/00485040.inc"
};
char gArmorDescTexts_241[32] = {
#include "sub_main/00485060.inc"
};
char gArmorDescTexts_243[32] = {
#include "sub_main/00485080.inc"
};
char gArmorDescTexts_244[32] = {
#include "sub_main/004850A0.inc"
};
char gArmorDescTexts_245[32] = {
#include "sub_main/004850C0.inc"
};
char gArmorDescTexts_247[32] = {
#include "sub_main/004850E0.inc"
};
char gArmorDescTexts_248[32] = {
#include "sub_main/00485100.inc"
};
char gArmorDescTexts_249[32] = {
#include "sub_main/00485120.inc"
};
char gArmorDescTexts_251[32] = {
#include "sub_main/00485140.inc"
};
char gArmorDescTexts_252[32] = {
#include "sub_main/00485160.inc"
};
char gArmorDescTexts_253[32] = {
#include "sub_main/00485180.inc"
};
char gArmorDescTexts_255[32] = {
#include "sub_main/004851A0.inc"
};
char gArmorDescTexts_256[32] = {
#include "sub_main/004851C0.inc"
};
char gArmorDescTexts_257[32] = {
#include "sub_main/004851E0.inc"
};
char gArmorDescTexts_259[32] = {
#include "sub_main/00485200.inc"
};
char gArmorDescTexts_260[32] = {
#include "sub_main/00485220.inc"
};
char gArmorDescTexts_261[32] = {
#include "sub_main/00485240.inc"
};
char D_00485260[32] = {
#include "sub_main/00485260.inc"
};
char D_00485280[32] = {
#include "sub_main/00485280.inc"
};
char D_004852A0[32] = {
#include "sub_main/004852A0.inc"
};
char D_004852C0[32] = {
#include "sub_main/004852C0.inc"
};
char D_004852E0[32] = {
#include "sub_main/004852E0.inc"
};
char D_00485300[32] = {
#include "sub_main/00485300.inc"
};
char D_00485320[32] = {
#include "sub_main/00485320.inc"
};
char D_00485340[32] = {
#include "sub_main/00485340.inc"
};
char D_00485360[32] = {
#include "sub_main/00485360.inc"
};
char D_00485380[32] = {
#include "sub_main/00485380.inc"
};
char D_004853A0[32] = {
#include "sub_main/004853A0.inc"
};
char D_004853C0[32] = {
#include "sub_main/004853C0.inc"
};
char D_004853E0[32] = {
#include "sub_main/004853E0.inc"
};
char D_00485400[32] = {
#include "sub_main/00485400.inc"
};
char D_00485420[32] = {
#include "sub_main/00485420.inc"
};
char D_00485440[32] = {
#include "sub_main/00485440.inc"
};
char gArmorDescTexts_363[32] = {
#include "sub_main/00485460.inc"
};
char gArmorDescTexts_383[32] = {
#include "sub_main/00485480.inc"
};
char gArmorDescTexts_384[32] = {
#include "sub_main/004854A0.inc"
};
char gArmorDescTexts_385[32] = {
#include "sub_main/004854C0.inc"
};
char gArmorDescTexts_387[32] = {
#include "sub_main/004854E0.inc"
};
char gArmorDescTexts_388[32] = {
#include "sub_main/00485500.inc"
};
char gArmorDescTexts_389[32] = {
#include "sub_main/00485520.inc"
};
char gArmorDescTexts_399[32] = {
#include "sub_main/00485540.inc"
};
char gArmorDescTexts_400[32] = {
#include "sub_main/00485560.inc"
};
char gArmorDescTexts_401[32] = {
#include "sub_main/00485580.inc"
};
char gArmorDescTexts_403[32] = {
#include "sub_main/004855A0.inc"
};
char gArmorDescTexts_404[32] = {
#include "sub_main/004855C0.inc"
};
char gArmorDescTexts_405[32] = {
#include "sub_main/004855E0.inc"
};
char D_00485600[32] = {
#include "sub_main/00485600.inc"
};
char D_00485620[32] = {
#include "sub_main/00485620.inc"
};
char gArmorDescTexts_449[32] = {
#include "sub_main/00485640.inc"
};
char D_00485660[32] = {
#include "sub_main/00485660.inc"
};
char gArmorDescTexts_452[32] = {
#include "sub_main/00485680.inc"
};
char gArmorDescTexts_453[16] = {
#include "sub_main/004856A0.inc"
};
char D_004856B0[32] = {
#include "sub_main/004856B0.inc"
};
char gArmorDescTexts_456[32] = {
#include "sub_main/004856D0.inc"
};
char gArmorDescTexts_457[32] = {
#include "sub_main/004856F0.inc"
};
char gArmorDescTexts_459[32] = {
#include "sub_main/00485710.inc"
};
char gArmorDescTexts_460[32] = {
#include "sub_main/00485730.inc"
};
char gArmorDescTexts_461[32] = {
#include "sub_main/00485750.inc"
};
char D_00485770[32] = {
#include "sub_main/00485770.inc"
};
char gArmorDescTexts_464[24] = {
#include "sub_main/00485790.inc"
};
char gArmorDescTexts_465[15] = {
#include "sub_main/004857A8.inc"
};
char D_004857C0[32] = {
#include "sub_main/004857C0.inc"
};
char gArmorDescTexts_468[32] = {
#include "sub_main/004857E0.inc"
};
char gArmorDescTexts_469[32] = {
#include "sub_main/00485800.inc"
};
char gArmorDescTexts_487[32] = {
#include "sub_main/00485820.inc"
};
char gArmorDescTexts_488[32] = {
#include "sub_main/00485840.inc"
};
char gArmorDescTexts_489[32] = {
#include "sub_main/00485860.inc"
};
char gArmorDescTexts_491[32] = {
#include "sub_main/00485880.inc"
};
char gArmorDescTexts_492[32] = {
#include "sub_main/004858A0.inc"
};
char gArmorDescTexts_493[32] = {
#include "sub_main/004858C0.inc"
};
char gArmorDescTexts_495[32] = {
#include "sub_main/004858E0.inc"
};
char gArmorDescTexts_496[32] = {
#include "sub_main/00485900.inc"
};
char gArmorDescTexts_497[32] = {
#include "sub_main/00485920.inc"
};
char gArmorDescTexts_499[32] = {
#include "sub_main/00485940.inc"
};
char gArmorDescTexts_500[32] = {
#include "sub_main/00485960.inc"
};
char gArmorDescTexts_501[32] = {
#include "sub_main/00485980.inc"
};
char gArmorDescTexts_503[32] = {
#include "sub_main/004859A0.inc"
};
char gArmorDescTexts_504[32] = {
#include "sub_main/004859C0.inc"
};
char gArmorDescTexts_505[32] = {
#include "sub_main/004859E0.inc"
};
char gArmorDescTexts_507[32] = {
#include "sub_main/00485A00.inc"
};
char gArmorDescTexts_508[32] = {
#include "sub_main/00485A20.inc"
};
char gArmorDescTexts_509[32] = {
#include "sub_main/00485A40.inc"
};
char gArmorDescTexts_583[32] = {
#include "sub_main/00485A60.inc"
};
char gArmorDescTexts_584[32] = {
#include "sub_main/00485A80.inc"
};
char gArmorDescTexts_585[32] = {
#include "sub_main/00485AA0.inc"
};
char gArmorDescTexts_587[32] = {
#include "sub_main/00485AC0.inc"
};
char gArmorDescTexts_588[32] = {
#include "sub_main/00485AE0.inc"
};
char gArmorDescTexts_589[32] = {
#include "sub_main/00485B00.inc"
};
char gArmorDescTexts_624[32] = {
#include "sub_main/00485B20.inc"
};
char gArmorDescTexts_625[32] = {
#include "sub_main/00485B40.inc"
};
char gArmorDescTexts_628[32] = {
#include "sub_main/00485B60.inc"
};
char gArmorDescTexts_629[32] = {
#include "sub_main/00485B80.inc"
};
char D_00485BA0[32] = {
#include "sub_main/00485BA0.inc"
};
char D_00485BC0[32] = {
#include "sub_main/00485BC0.inc"
};
char D_00485BE0[32] = {
#include "sub_main/00485BE0.inc"
};
char D_00485C00[32] = {
#include "sub_main/00485C00.inc"
};
char D_00485C20[32] = {
#include "sub_main/00485C20.inc"
};
char D_00485C40[32] = {
#include "sub_main/00485C40.inc"
};
char D_00485C60[32] = {
#include "sub_main/00485C60.inc"
};
char D_00485C80[32] = {
#include "sub_main/00485C80.inc"
};
char D_00485CA0[32] = {
#include "sub_main/00485CA0.inc"
};
char D_00485CC0[32] = {
#include "sub_main/00485CC0.inc"
};
char D_00485CE0[32] = {
#include "sub_main/00485CE0.inc"
};
char D_00485D00[32] = {
#include "sub_main/00485D00.inc"
};
char D_00485D20[32] = {
#include "sub_main/00485D20.inc"
};
char D_00485D40[32] = {
#include "sub_main/00485D40.inc"
};
char D_00485D60[32] = {
#include "sub_main/00485D60.inc"
};
char D_00485D80[32] = {
#include "sub_main/00485D80.inc"
};
char D_00485DA0[32] = {
#include "sub_main/00485DA0.inc"
};
char D_00485DC0[32] = {
#include "sub_main/00485DC0.inc"
};
char D_00485DE0[32] = {
#include "sub_main/00485DE0.inc"
};
char D_00485E00[32] = {
#include "sub_main/00485E00.inc"
};
char D_00485E20[32] = {
#include "sub_main/00485E20.inc"
};
char D_00485E40[32] = {
#include "sub_main/00485E40.inc"
};
char D_00485E60[32] = {
#include "sub_main/00485E60.inc"
};
char D_00485E80[16] = {
#include "sub_main/00485E80.inc"
};
char D_00485E90[32] = {
#include "sub_main/00485E90.inc"
};
char D_00485EB0[32] = {
#include "sub_main/00485EB0.inc"
};
char D_00485ED0[32] = {
#include "sub_main/00485ED0.inc"
};
char D_00485EF0[32] = {
#include "sub_main/00485EF0.inc"
};
char D_00485F10[32] = {
#include "sub_main/00485F10.inc"
};
char D_00485F30[32] = {
#include "sub_main/00485F30.inc"
};
char D_00485F50[32] = {
#include "sub_main/00485F50.inc"
};
char D_00485F70[32] = {
#include "sub_main/00485F70.inc"
};
char D_00485F90[32] = {
#include "sub_main/00485F90.inc"
};
char D_00485FB0[32] = {
#include "sub_main/00485FB0.inc"
};
char D_00485FD0[32] = {
#include "sub_main/00485FD0.inc"
};
char D_00485FF0[32] = {
#include "sub_main/00485FF0.inc"
};
char D_00486010[32] = {
#include "sub_main/00486010.inc"
};
char D_00486030[32] = {
#include "sub_main/00486030.inc"
};
char D_00486050[32] = {
#include "sub_main/00486050.inc"
};
char D_00486070[32] = {
#include "sub_main/00486070.inc"
};
char D_00486090[32] = {
#include "sub_main/00486090.inc"
};
char D_004860B0[32] = {
#include "sub_main/004860B0.inc"
};
char D_004860D0[32] = {
#include "sub_main/004860D0.inc"
};
char D_004860F0[32] = {
#include "sub_main/004860F0.inc"
};
char gArmorDescTexts_839[32] = {
#include "sub_main/00486110.inc"
};
char gArmorDescTexts_840[32] = {
#include "sub_main/00486130.inc"
};
char gArmorDescTexts_841[32] = {
#include "sub_main/00486150.inc"
};
char D_00486170[32] = {
#include "sub_main/00486170.inc"
};
char gArmorDescTexts_844[32] = {
#include "sub_main/00486190.inc"
};
char gArmorDescTexts_845[32] = {
#include "sub_main/004861B0.inc"
};
char gArmorDescTexts_847[32] = {
#include "sub_main/004861D0.inc"
};
char gArmorDescTexts_848[32] = {
#include "sub_main/004861F0.inc"
};
char gArmorDescTexts_849[16] = {
#include "sub_main/00486210.inc"
};
char gArmorDescTexts_851[32] = {
#include "sub_main/00486220.inc"
};
char gArmorDescTexts_852[32] = {
#include "sub_main/00486240.inc"
};
char gArmorDescTexts_853[32] = {
#include "sub_main/00486260.inc"
};
char gArmorDescTexts_855[32] = {
#include "sub_main/00486280.inc"
};
char gArmorDescTexts_856[32] = {
#include "sub_main/004862A0.inc"
};
char gArmorDescTexts_857[32] = {
#include "sub_main/004862C0.inc"
};
char gArmorDescTexts_859[32] = {
#include "sub_main/004862E0.inc"
};
char gArmorDescTexts_860[32] = {
#include "sub_main/00486300.inc"
};
char gArmorDescTexts_861[32] = {
#include "sub_main/00486320.inc"
};
char gArmorDescTexts_863[32] = {
#include "sub_main/00486340.inc"
};
char gArmorDescTexts_864[32] = {
#include "sub_main/00486360.inc"
};
char gArmorDescTexts_865[16] = {
#include "sub_main/00486380.inc"
};
char gArmorDescTexts_867[32] = {
#include "sub_main/00486390.inc"
};
char gArmorDescTexts_868[32] = {
#include "sub_main/004863B0.inc"
};
char gArmorDescTexts_869[32] = {
#include "sub_main/004863D0.inc"
};
char gArmorDescTexts_871[32] = {
#include "sub_main/004863F0.inc"
};
char gArmorDescTexts_872[32] = {
#include "sub_main/00486410.inc"
};
char gArmorDescTexts_873[32] = {
#include "sub_main/00486430.inc"
};
char gArmorDescTexts_875[32] = {
#include "sub_main/00486450.inc"
};
char gArmorDescTexts_876[32] = {
#include "sub_main/00486470.inc"
};
char gArmorDescTexts_877[32] = {
#include "sub_main/00486490.inc"
};
char gArmorDescTexts_879[32] = {
#include "sub_main/004864B0.inc"
};
char gArmorDescTexts_880[32] = {
#include "sub_main/004864D0.inc"
};
char gArmorDescTexts_881[32] = {
#include "sub_main/004864F0.inc"
};
char gArmorDescTexts_883[32] = {
#include "sub_main/00486510.inc"
};
char gArmorDescTexts_884[32] = {
#include "sub_main/00486530.inc"
};
char gArmorDescTexts_885[32] = {
#include "sub_main/00486550.inc"
};
char gArmorDescTexts_887[32] = {
#include "sub_main/00486570.inc"
};
char gArmorDescTexts_888[32] = {
#include "sub_main/00486590.inc"
};
char gArmorDescTexts_889[32] = {
#include "sub_main/004865B0.inc"
};
char gArmorDescTexts_891[32] = {
#include "sub_main/004865D0.inc"
};
char gArmorDescTexts_892[32] = {
#include "sub_main/004865F0.inc"
};
char gArmorDescTexts_893[32] = {
#include "sub_main/00486610.inc"
};
char D_00486630[32] = {
#include "sub_main/00486630.inc"
};
char D_00486650[32] = {
#include "sub_main/00486650.inc"
};
char D_00486670[16] = {
#include "sub_main/00486670.inc"
};
char gArmorDescTexts_911[32] = {
#include "sub_main/00486680.inc"
};
char gArmorDescTexts_912[32] = {
#include "sub_main/004866A0.inc"
};
char gArmorDescTexts_913[16] = {
#include "sub_main/004866C0.inc"
};
char gArmorDescTexts_917[32] = {
#include "sub_main/004866D0.inc"
};
char gArmorDescTexts_919[32] = {
#include "sub_main/004866F0.inc"
};
char gArmorDescTexts_920[32] = {
#include "sub_main/00486710.inc"
};
char gArmorDescTexts_921[32] = {
#include "sub_main/00486730.inc"
};
char gArmorDescTexts_923[32] = {
#include "sub_main/00486750.inc"
};
char gArmorDescTexts_924[32] = {
#include "sub_main/00486770.inc"
};
char gArmorDescTexts_925[32] = {
#include "sub_main/00486790.inc"
};
char gArmorDescTexts_927[32] = {
#include "sub_main/004867B0.inc"
};
char gArmorDescTexts_928[32] = {
#include "sub_main/004867D0.inc"
};
char gArmorDescTexts_929[32] = {
#include "sub_main/004867F0.inc"
};
char gArmorDescTexts_932[32] = {
#include "sub_main/00486810.inc"
};
char gArmorDescTexts_933[32] = {
#include "sub_main/00486830.inc"
};
char gArmorDescTexts_951[32] = {
#include "sub_main/00486850.inc"
};
char gArmorDescTexts_952[32] = {
#include "sub_main/00486870.inc"
};
char gArmorDescTexts_953[16] = {
#include "sub_main/00486890.inc"
};
char gArmorDescTexts_955[32] = {
#include "sub_main/004868A0.inc"
};
char gArmorDescTexts_956[32] = {
#include "sub_main/004868C0.inc"
};
char gArmorDescTexts_957[32] = {
#include "sub_main/004868E0.inc"
};
char gArmorDescTexts_959[32] = {
#include "sub_main/00486900.inc"
};
char gArmorDescTexts_960[32] = {
#include "sub_main/00486920.inc"
};
char gArmorDescTexts_961[32] = {
#include "sub_main/00486940.inc"
};
char gArmorDescTexts_963[32] = {
#include "sub_main/00486960.inc"
};
char gArmorDescTexts_964[32] = {
#include "sub_main/00486980.inc"
};
char gArmorDescTexts_965[32] = {
#include "sub_main/004869A0.inc"
};
char gArmorDescTexts_967[32] = {
#include "sub_main/004869C0.inc"
};
char gArmorDescTexts_968[32] = {
#include "sub_main/004869E0.inc"
};
char gArmorDescTexts_969[32] = {
#include "sub_main/00486A00.inc"
};
char gArmorDescTexts_971[32] = {
#include "sub_main/00486A20.inc"
};
char gArmorDescTexts_972[32] = {
#include "sub_main/00486A40.inc"
};
char gArmorDescTexts_973[32] = {
#include "sub_main/00486A60.inc"
};
char gArmorDescTexts_975[32] = {
#include "sub_main/00486A80.inc"
};
char gArmorDescTexts_976[32] = {
#include "sub_main/00486AA0.inc"
};
char gArmorDescTexts_977[32] = {
#include "sub_main/00486AC0.inc"
};
char gArmorDescTexts_979[32] = {
#include "sub_main/00486AE0.inc"
};
char gArmorDescTexts_980[32] = {
#include "sub_main/00486B00.inc"
};
char gArmorDescTexts_981[32] = {
#include "sub_main/00486B20.inc"
};
char gArmorDescTexts_983[32] = {
#include "sub_main/00486B40.inc"
};
char gArmorDescTexts_984[32] = {
#include "sub_main/00486B60.inc"
};
char gArmorDescTexts_985[32] = {
#include "sub_main/00486B80.inc"
};
char gArmorDescTexts_987[32] = {
#include "sub_main/00486BA0.inc"
};
char gArmorDescTexts_988[32] = {
#include "sub_main/00486BC0.inc"
};
char gArmorDescTexts_989[32] = {
#include "sub_main/00486BE0.inc"
};
char gArmorDescTexts_991[32] = {
#include "sub_main/00486C00.inc"
};
char gArmorDescTexts_992[32] = {
#include "sub_main/00486C20.inc"
};
char gArmorDescTexts_993[32] = {
#include "sub_main/00486C40.inc"
};
char gArmorDescTexts_995[32] = {
#include "sub_main/00486C60.inc"
};
char gArmorDescTexts_996[32] = {
#include "sub_main/00486C80.inc"
};
char gArmorDescTexts_997[32] = {
#include "sub_main/00486CA0.inc"
};
char gArmorDescTexts_999[32] = {
#include "sub_main/00486CC0.inc"
};
char gArmorDescTexts_1000[32] = {
#include "sub_main/00486CE0.inc"
};
char gArmorDescTexts_1001[32] = {
#include "sub_main/00486D00.inc"
};
char gArmorDescTexts_1003[32] = {
#include "sub_main/00486D20.inc"
};
char gArmorDescTexts_1004[32] = {
#include "sub_main/00486D40.inc"
};
char gArmorDescTexts_1005[32] = {
#include "sub_main/00486D60.inc"
};
char D_00486D80[32] = {
#include "sub_main/00486D80.inc"
};
char D_00486DA0[32] = {
#include "sub_main/00486DA0.inc"
};
char D_00486DC0[16] = {
#include "sub_main/00486DC0.inc"
};
char D_00486DD0[32] = {
#include "sub_main/00486DD0.inc"
};
char D_00486DF0[32] = {
#include "sub_main/00486DF0.inc"
};
char D_00486E10[32] = {
#include "sub_main/00486E10.inc"
};
char D_00486E30[32] = {
#include "sub_main/00486E30.inc"
};
char D_00486E50[32] = {
#include "sub_main/00486E50.inc"
};
char D_00486E70[32] = {
#include "sub_main/00486E70.inc"
};
char D_00486E90[32] = {
#include "sub_main/00486E90.inc"
};
char D_00486EB0[32] = {
#include "sub_main/00486EB0.inc"
};
char D_00486ED0[32] = {
#include "sub_main/00486ED0.inc"
};
char D_00486EF0[32] = {
#include "sub_main/00486EF0.inc"
};
char D_00486F10[32] = {
#include "sub_main/00486F10.inc"
};
char D_00486F30[32] = {
#include "sub_main/00486F30.inc"
};
char D_00486F50[32] = {
#include "sub_main/00486F50.inc"
};
char D_00486F70[32] = {
#include "sub_main/00486F70.inc"
};
char D_00486F90[32] = {
#include "sub_main/00486F90.inc"
};
char D_00486FB0[32] = {
#include "sub_main/00486FB0.inc"
};
char D_00486FD0[32] = {
#include "sub_main/00486FD0.inc"
};
char D_00486FF0[32] = {
#include "sub_main/00486FF0.inc"
};
char D_00487010[32] = {
#include "sub_main/00487010.inc"
};
char D_00487030[16] = {
#include "sub_main/00487030.inc"
};
char D_00487040[32] = {
#include "sub_main/00487040.inc"
};
char D_00487060[32] = {
#include "sub_main/00487060.inc"
};
char D_00487080[32] = {
#include "sub_main/00487080.inc"
};
char D_004870A0[32] = {
#include "sub_main/004870A0.inc"
};
char D_004870C0[32] = {
#include "sub_main/004870C0.inc"
};
char D_004870E0[32] = {
#include "sub_main/004870E0.inc"
};
char D_00487100[32] = {
#include "sub_main/00487100.inc"
};
char D_00487120[32] = {
#include "sub_main/00487120.inc"
};
char D_00487140[32] = {
#include "sub_main/00487140.inc"
};
char D_00487160[32] = {
#include "sub_main/00487160.inc"
};
char D_00487180[16] = {
#include "sub_main/00487180.inc"
};
char D_00487190[32] = {
#include "sub_main/00487190.inc"
};
char D_004871B0[32] = {
#include "sub_main/004871B0.inc"
};
char D_004871D0[32] = {
#include "sub_main/004871D0.inc"
};
char D_004871F0[32] = {
#include "sub_main/004871F0.inc"
};
char D_00487210[24] = {
#include "sub_main/00487210.inc"
};
char D_00487228[12] = {
#include "sub_main/00487228.inc"
};
char D_00487240[32] = {
#include "sub_main/00487240.inc"
};
char D_00487260[32] = {
#include "sub_main/00487260.inc"
};
char D_00487280[32] = {
#include "sub_main/00487280.inc"
};
char D_004872A0[32] = {
#include "sub_main/004872A0.inc"
};
char D_004872C0[32] = {
#include "sub_main/004872C0.inc"
};
char D_004872E0[32] = {
#include "sub_main/004872E0.inc"
};
char D_00487300[16] = {
#include "sub_main/00487300.inc"
};
char D_00487310[32] = {
#include "sub_main/00487310.inc"
};
char D_00487330[32] = {
#include "sub_main/00487330.inc"
};
char D_00487350[32] = {
#include "sub_main/00487350.inc"
};
char D_00487370[32] = {
#include "sub_main/00487370.inc"
};
char D_00487390[32] = {
#include "sub_main/00487390.inc"
};
char D_004873B0[32] = {
#include "sub_main/004873B0.inc"
};
char D_004873D0[32] = {
#include "sub_main/004873D0.inc"
};
char D_004873F0[32] = {
#include "sub_main/004873F0.inc"
};
char D_00487410[32] = {
#include "sub_main/00487410.inc"
};
char D_00487430[32] = {
#include "sub_main/00487430.inc"
};
char D_00487450[32] = {
#include "sub_main/00487450.inc"
};
char D_00487470[32] = {
#include "sub_main/00487470.inc"
};
char D_00487490[32] = {
#include "sub_main/00487490.inc"
};
char D_004874B0[32] = {
#include "sub_main/004874B0.inc"
};
char D_004874D0[32] = {
#include "sub_main/004874D0.inc"
};
char D_004874F0[32] = {
#include "sub_main/004874F0.inc"
};
char D_00487510[16] = {
#include "sub_main/00487510.inc"
};
char D_00487520[32] = {
#include "sub_main/00487520.inc"
};
char D_00487540[32] = {
#include "sub_main/00487540.inc"
};
char D_00487560[32] = {
#include "sub_main/00487560.inc"
};
char D_00487580[32] = {
#include "sub_main/00487580.inc"
};
char D_004875A0[32] = {
#include "sub_main/004875A0.inc"
};
char D_004875C0[32] = {
#include "sub_main/004875C0.inc"
};
char D_004875E0[32] = {
#include "sub_main/004875E0.inc"
};
char D_00487600[32] = {
#include "sub_main/00487600.inc"
};
char D_00487620[32] = {
#include "sub_main/00487620.inc"
};
char D_00487640[32] = {
#include "sub_main/00487640.inc"
};
char D_00487660[32] = {
#include "sub_main/00487660.inc"
};
char D_00487680[32] = {
#include "sub_main/00487680.inc"
};
char D_004876A0[32] = {
#include "sub_main/004876A0.inc"
};
char D_004876C0[32] = {
#include "sub_main/004876C0.inc"
};
char D_004876E0[32] = {
#include "sub_main/004876E0.inc"
};
char D_00487700[32] = {
#include "sub_main/00487700.inc"
};
char D_00487720[32] = {
#include "sub_main/00487720.inc"
};
char D_00487740[32] = {
#include "sub_main/00487740.inc"
};
char D_00487760[32] = {
#include "sub_main/00487760.inc"
};
char D_00487780[32] = {
#include "sub_main/00487780.inc"
};
char D_004877A0[32] = {
#include "sub_main/004877A0.inc"
};
char D_004877C0[32] = {
#include "sub_main/004877C0.inc"
};
char D_004877E0[32] = {
#include "sub_main/004877E0.inc"
};
char D_00487800[32] = {
#include "sub_main/00487800.inc"
};
char D_00487820[32] = {
#include "sub_main/00487820.inc"
};
char D_00487840[32] = {
#include "sub_main/00487840.inc"
};
char D_00487860[32] = {
#include "sub_main/00487860.inc"
};
char D_00487880[32] = {
#include "sub_main/00487880.inc"
};
char D_004878A0[32] = {
#include "sub_main/004878A0.inc"
};
char D_004878C0[32] = {
#include "sub_main/004878C0.inc"
};
char gArmorDescTexts_1152[32] = {
#include "sub_main/004878E0.inc"
};
char gArmorDescTexts_1153[32] = {
#include "sub_main/00487900.inc"
};
char gArmorDescTexts_1155[32] = {
#include "sub_main/00487920.inc"
};
char gArmorDescTexts_1156[32] = {
#include "sub_main/00487940.inc"
};
char gArmorDescTexts_1157[32] = {
#include "sub_main/00487960.inc"
};
char D_00487980[32] = {
#include "sub_main/00487980.inc"
};
char D_004879A0[32] = {
#include "sub_main/004879A0.inc"
};
char D_004879C0[32] = {
#include "sub_main/004879C0.inc"
};
char D_004879E0[32] = {
#include "sub_main/004879E0.inc"
};
char D_00487A00[32] = {
#include "sub_main/00487A00.inc"
};
char D_00487A20[32] = {
#include "sub_main/00487A20.inc"
};
char D_00487A40[32] = {
#include "sub_main/00487A40.inc"
};
char D_00487A60[32] = {
#include "sub_main/00487A60.inc"
};
char D_00487A80[32] = {
#include "sub_main/00487A80.inc"
};
char D_00487AA0[32] = {
#include "sub_main/00487AA0.inc"
};
char D_00487AC0[32] = {
#include "sub_main/00487AC0.inc"
};
char D_00487AE0[32] = {
#include "sub_main/00487AE0.inc"
};
char D_00487B00[32] = {
#include "sub_main/00487B00.inc"
};
char D_00487B20[32] = {
#include "sub_main/00487B20.inc"
};
char D_00487B40[32] = {
#include "sub_main/00487B40.inc"
};
char D_00487B60[32] = {
#include "sub_main/00487B60.inc"
};
char D_00487B80[32] = {
#include "sub_main/00487B80.inc"
};
char D_00487BA0[32] = {
#include "sub_main/00487BA0.inc"
};
char D_00487BC0[32] = {
#include "sub_main/00487BC0.inc"
};
char D_00487BE0[32] = {
#include "sub_main/00487BE0.inc"
};
char D_00487C00[32] = {
#include "sub_main/00487C00.inc"
};
char gArmorDescTexts_1195[32] = {
#include "sub_main/00487C20.inc"
};
char gArmorDescTexts_1196[32] = {
#include "sub_main/00487C40.inc"
};
char gArmorDescTexts_1197[16] = {
#include "sub_main/00487C60.inc"
};
char gArmorDescTexts_1199[32] = {
#include "sub_main/00487C70.inc"
};
char gArmorDescTexts_1200[32] = {
#include "sub_main/00487C90.inc"
};
char gArmorDescTexts_1201[32] = {
#include "sub_main/00487CB0.inc"
};
char gArmorDescTexts_1203[32] = {
#include "sub_main/00487CD0.inc"
};
char gArmorDescTexts_1204[32] = {
#include "sub_main/00487CF0.inc"
};
char gArmorDescTexts_1205[32] = {
#include "sub_main/00487D10.inc"
};
char gArmorDescTexts_1208[32] = {
#include "sub_main/00487D30.inc"
};
char gArmorDescTexts_1209[32] = {
#include "sub_main/00487D50.inc"
};
char D_00487D70[32] = {
#include "sub_main/00487D70.inc"
};
char gArmorDescTexts_1213[32] = {
#include "sub_main/00487D90.inc"
};
char D_00487DB0[32] = {
#include "sub_main/00487DB0.inc"
};
char gArmorDescTexts_1216[32] = {
#include "sub_main/00487DD0.inc"
};
char gArmorDescTexts_1217[32] = {
#include "sub_main/00487DF0.inc"
};
char D_00487E10[32] = {
#include "sub_main/00487E10.inc"
};
char gArmorDescTexts_1220[32] = {
#include "sub_main/00487E30.inc"
};
char gArmorDescTexts_1221[32] = {
#include "sub_main/00487E50.inc"
};
char D_00487E70[32] = {
#include "sub_main/00487E70.inc"
};
char D_00487E90[32] = {
#include "sub_main/00487E90.inc"
};
char D_00487EB0[32] = {
#include "sub_main/00487EB0.inc"
};
char D_00487ED0[32] = {
#include "sub_main/00487ED0.inc"
};
char D_00487EF0[32] = {
#include "sub_main/00487EF0.inc"
};
char gArmorDescTexts_1239[32] = {
#include "sub_main/00487F10.inc"
};
char gArmorDescTexts_1240[32] = {
#include "sub_main/00487F30.inc"
};
char gArmorDescTexts_1241[32] = {
#include "sub_main/00487F50.inc"
};
char D_00487F70[32] = {
#include "sub_main/00487F70.inc"
};
char gArmorDescTexts_1244[32] = {
#include "sub_main/00487F90.inc"
};
char gArmorDescTexts_1245[16] = {
#include "sub_main/00487FB0.inc"
};
char gArmorDescTexts_1247[32] = {
#include "sub_main/00487FC0.inc"
};
char gArmorDescTexts_1248[32] = {
#include "sub_main/00487FE0.inc"
};
char gArmorDescTexts_1249[32] = {
#include "sub_main/00488000.inc"
};
char gArmorDescTexts_1251[32] = {
#include "sub_main/00488020.inc"
};
char gArmorDescTexts_1252[32] = {
#include "sub_main/00488040.inc"
};
char gArmorDescTexts_1253[32] = {
#include "sub_main/00488060.inc"
};
char D_00488080[32] = {
#include "sub_main/00488080.inc"
};
char D_004880A0[32] = {
#include "sub_main/004880A0.inc"
};
char D_004880C0[32] = {
#include "sub_main/004880C0.inc"
};
char D_004880E0[32] = {
#include "sub_main/004880E0.inc"
};
char D_00488100[32] = {
#include "sub_main/00488100.inc"
};
char D_00488120[32] = {
#include "sub_main/00488120.inc"
};
char D_00488140[32] = {
#include "sub_main/00488140.inc"
};
char D_00488160[32] = {
#include "sub_main/00488160.inc"
};
char D_00488180[32] = {
#include "sub_main/00488180.inc"
};
char D_004881A0[32] = {
#include "sub_main/004881A0.inc"
};
char D_004881C0[32] = {
#include "sub_main/004881C0.inc"
};
char D_004881E0[32] = {
#include "sub_main/004881E0.inc"
};
char D_00488200[32] = {
#include "sub_main/00488200.inc"
};
char D_00488220[32] = {
#include "sub_main/00488220.inc"
};
char gArmorDescTexts_1363[32] = {
#include "sub_main/00488240.inc"
};
char gArmorDescTexts_1387[32] = {
#include "sub_main/00488260.inc"
};
char gArmorDescTexts_1388[32] = {
#include "sub_main/00488280.inc"
};
char gArmorDescTexts_1389[32] = {
#include "sub_main/004882A0.inc"
};
char gArmorDescTexts_1391[32] = {
#include "sub_main/004882C0.inc"
};
char gArmorDescTexts_1392[32] = {
#include "sub_main/004882E0.inc"
};
char gArmorDescTexts_1393[32] = {
#include "sub_main/00488300.inc"
};
char gArmorDescTexts_1403[32] = {
#include "sub_main/00488320.inc"
};
char gArmorDescTexts_1404[32] = {
#include "sub_main/00488340.inc"
};
char gArmorDescTexts_1405[32] = {
#include "sub_main/00488360.inc"
};
char gArmorDescTexts_1407[32] = {
#include "sub_main/00488380.inc"
};
char gArmorDescTexts_1408[32] = {
#include "sub_main/004883A0.inc"
};
char gArmorDescTexts_1409[32] = {
#include "sub_main/004883C0.inc"
};
char gArmorDescTexts_1453[32] = {
#include "sub_main/004883E0.inc"
};
char gArmorDescTexts_1456[32] = {
#include "sub_main/00488400.inc"
};
char gArmorDescTexts_1457[16] = {
#include "sub_main/00488420.inc"
};
char gArmorDescTexts_1459[32] = {
#include "sub_main/00488430.inc"
};
char gArmorDescTexts_1460[32] = {
#include "sub_main/00488450.inc"
};
char gArmorDescTexts_1461[32] = {
#include "sub_main/00488470.inc"
};
char gArmorDescTexts_1464[32] = {
#include "sub_main/00488490.inc"
};
char gArmorDescTexts_1465[32] = {
#include "sub_main/004884B0.inc"
};
char gArmorDescTexts_1467[32] = {
#include "sub_main/004884D0.inc"
};
char gArmorDescTexts_1468[32] = {
#include "sub_main/004884F0.inc"
};
char gArmorDescTexts_1469[32] = {
#include "sub_main/00488510.inc"
};
char gArmorDescTexts_1472[32] = {
#include "sub_main/00488530.inc"
};
char gArmorDescTexts_1473[32] = {
#include "sub_main/00488550.inc"
};
char gArmorDescTexts_1491[32] = {
#include "sub_main/00488570.inc"
};
char gArmorDescTexts_1492[32] = {
#include "sub_main/00488590.inc"
};
char gArmorDescTexts_1493[32] = {
#include "sub_main/004885B0.inc"
};
char gArmorDescTexts_1495[32] = {
#include "sub_main/004885D0.inc"
};
char gArmorDescTexts_1496[32] = {
#include "sub_main/004885F0.inc"
};
char gArmorDescTexts_1497[32] = {
#include "sub_main/00488610.inc"
};
char gArmorDescTexts_1499[32] = {
#include "sub_main/00488630.inc"
};
char gArmorDescTexts_1500[32] = {
#include "sub_main/00488650.inc"
};
char gArmorDescTexts_1501[32] = {
#include "sub_main/00488670.inc"
};
char gArmorDescTexts_1503[32] = {
#include "sub_main/00488690.inc"
};
char gArmorDescTexts_1504[32] = {
#include "sub_main/004886B0.inc"
};
char gArmorDescTexts_1505[32] = {
#include "sub_main/004886D0.inc"
};
char gArmorDescTexts_1507[32] = {
#include "sub_main/004886F0.inc"
};
char gArmorDescTexts_1508[32] = {
#include "sub_main/00488710.inc"
};
char gArmorDescTexts_1509[32] = {
#include "sub_main/00488730.inc"
};
char gArmorDescTexts_1511[32] = {
#include "sub_main/00488750.inc"
};
char gArmorDescTexts_1512[32] = {
#include "sub_main/00488770.inc"
};
char gArmorDescTexts_1513[32] = {
#include "sub_main/00488790.inc"
};
char D_004887B0[32] = {
#include "sub_main/004887B0.inc"
};
char D_004887D0[32] = {
#include "sub_main/004887D0.inc"
};
char D_004887F0[32] = {
#include "sub_main/004887F0.inc"
};
char D_00488810[32] = {
#include "sub_main/00488810.inc"
};
char D_00488830[32] = {
#include "sub_main/00488830.inc"
};
char D_00488850[32] = {
#include "sub_main/00488850.inc"
};
char D_00488870[32] = {
#include "sub_main/00488870.inc"
};
char D_00488890[32] = {
#include "sub_main/00488890.inc"
};
char D_004888B0[32] = {
#include "sub_main/004888B0.inc"
};
char D_004888D0[32] = {
#include "sub_main/004888D0.inc"
};
char D_004888F0[32] = {
#include "sub_main/004888F0.inc"
};
char D_00488910[32] = {
#include "sub_main/00488910.inc"
};
char D_00488930[32] = {
#include "sub_main/00488930.inc"
};
char D_00488950[32] = {
#include "sub_main/00488950.inc"
};
char D_00488970[32] = {
#include "sub_main/00488970.inc"
};
char D_00488990[32] = {
#include "sub_main/00488990.inc"
};
char D_004889B0[32] = {
#include "sub_main/004889B0.inc"
};
char D_004889D0[32] = {
#include "sub_main/004889D0.inc"
};
char D_004889F0[32] = {
#include "sub_main/004889F0.inc"
};
char D_00488A10[32] = {
#include "sub_main/00488A10.inc"
};
char D_00488A30[32] = {
#include "sub_main/00488A30.inc"
};
char D_00488A50[32] = {
#include "sub_main/00488A50.inc"
};
char D_00488A70[32] = {
#include "sub_main/00488A70.inc"
};
char D_00488A90[32] = {
#include "sub_main/00488A90.inc"
};
char D_00488AB0[32] = {
#include "sub_main/00488AB0.inc"
};
char D_00488AD0[32] = {
#include "sub_main/00488AD0.inc"
};
char D_00488AF0[32] = {
#include "sub_main/00488AF0.inc"
};
char D_00488B10[32] = {
#include "sub_main/00488B10.inc"
};
char D_00488B30[32] = {
#include "sub_main/00488B30.inc"
};
char D_00488B50[32] = {
#include "sub_main/00488B50.inc"
};
char D_00488B70[32] = {
#include "sub_main/00488B70.inc"
};
char D_00488B90[32] = {
#include "sub_main/00488B90.inc"
};
char D_00488BB0[32] = {
#include "sub_main/00488BB0.inc"
};
char D_00488BD0[32] = {
#include "sub_main/00488BD0.inc"
};
char D_00488BF0[32] = {
#include "sub_main/00488BF0.inc"
};
char D_00488C10[32] = {
#include "sub_main/00488C10.inc"
};
char D_00488C30[32] = {
#include "sub_main/00488C30.inc"
};
char D_00488C50[32] = {
#include "sub_main/00488C50.inc"
};
char D_00488C70[32] = {
#include "sub_main/00488C70.inc"
};
char D_00488C90[32] = {
#include "sub_main/00488C90.inc"
};
char D_00488CB0[32] = {
#include "sub_main/00488CB0.inc"
};
char gArmorDescTexts_1847[32] = {
#include "sub_main/00488CD0.inc"
};
char gArmorDescTexts_1848[32] = {
#include "sub_main/00488CF0.inc"
};
char gArmorDescTexts_1849[32] = {
#include "sub_main/00488D10.inc"
};
char gArmorDescTexts_1851[32] = {
#include "sub_main/00488D30.inc"
};
char gArmorDescTexts_1852[32] = {
#include "sub_main/00488D50.inc"
};
char gArmorDescTexts_1853[32] = {
#include "sub_main/00488D70.inc"
};
char D_00488D90[32] = {
#include "sub_main/00488D90.inc"
};
char gArmorDescTexts_1856[32] = {
#include "sub_main/00488DB0.inc"
};
char gArmorDescTexts_1857[32] = {
#include "sub_main/00488DD0.inc"
};
char gArmorDescTexts_1860[32] = {
#include "sub_main/00488DF0.inc"
};
char gArmorDescTexts_1861[32] = {
#include "sub_main/00488E10.inc"
};
char gArmorDescTexts_1863[32] = {
#include "sub_main/00488E30.inc"
};
char gArmorDescTexts_1864[32] = {
#include "sub_main/00488E50.inc"
};
char gArmorDescTexts_1865[32] = {
#include "sub_main/00488E70.inc"
};
char gArmorDescTexts_1867[32] = {
#include "sub_main/00488E90.inc"
};
char gArmorDescTexts_1868[32] = {
#include "sub_main/00488EB0.inc"
};
char gArmorDescTexts_1869[32] = {
#include "sub_main/00488ED0.inc"
};
char gArmorDescTexts_1871[32] = {
#include "sub_main/00488EF0.inc"
};
char gArmorDescTexts_1872[32] = {
#include "sub_main/00488F10.inc"
};
char gArmorDescTexts_1873[32] = {
#include "sub_main/00488F30.inc"
};
char gArmorDescTexts_1875[32] = {
#include "sub_main/00488F50.inc"
};
char gArmorDescTexts_1876[32] = {
#include "sub_main/00488F70.inc"
};
char gArmorDescTexts_1877[32] = {
#include "sub_main/00488F90.inc"
};
char gArmorDescTexts_1879[32] = {
#include "sub_main/00488FB0.inc"
};
char gArmorDescTexts_1880[32] = {
#include "sub_main/00488FD0.inc"
};
char gArmorDescTexts_1881[32] = {
#include "sub_main/00488FF0.inc"
};
char gArmorDescTexts_1883[32] = {
#include "sub_main/00489010.inc"
};
char gArmorDescTexts_1884[32] = {
#include "sub_main/00489030.inc"
};
char gArmorDescTexts_1885[32] = {
#include "sub_main/00489050.inc"
};
char gArmorDescTexts_1887[32] = {
#include "sub_main/00489070.inc"
};
char gArmorDescTexts_1888[32] = {
#include "sub_main/00489090.inc"
};
char gArmorDescTexts_1889[32] = {
#include "sub_main/004890B0.inc"
};
char gArmorDescTexts_1891[32] = {
#include "sub_main/004890D0.inc"
};
char gArmorDescTexts_1892[32] = {
#include "sub_main/004890F0.inc"
};
char gArmorDescTexts_1893[32] = {
#include "sub_main/00489110.inc"
};
char gArmorDescTexts_1895[32] = {
#include "sub_main/00489130.inc"
};
char gArmorDescTexts_1896[32] = {
#include "sub_main/00489150.inc"
};
char gArmorDescTexts_1897[32] = {
#include "sub_main/00489170.inc"
};
char gArmorDescTexts_1899[32] = {
#include "sub_main/00489190.inc"
};
char gArmorDescTexts_1900[32] = {
#include "sub_main/004891B0.inc"
};
char gArmorDescTexts_1901[32] = {
#include "sub_main/004891D0.inc"
};
char D_004891F0[32] = {
#include "sub_main/004891F0.inc"
};
char D_00489210[32] = {
#include "sub_main/00489210.inc"
};
char D_00489230[16] = {
#include "sub_main/00489230.inc"
};
char gArmorDescTexts_1917[32] = {
#include "sub_main/00489240.inc"
};
char gArmorDescTexts_1919[32] = {
#include "sub_main/00489260.inc"
};
char D_00489280[32] = {
#include "sub_main/00489280.inc"
};
char gArmorDescTexts_1923[32] = {
#include "sub_main/004892A0.inc"
};
char gArmorDescTexts_1924[32] = {
#include "sub_main/004892C0.inc"
};
char gArmorDescTexts_1925[32] = {
#include "sub_main/004892E0.inc"
};
char gArmorDescTexts_1927[32] = {
#include "sub_main/00489300.inc"
};
char gArmorDescTexts_1928[32] = {
#include "sub_main/00489320.inc"
};
char gArmorDescTexts_1929[32] = {
#include "sub_main/00489340.inc"
};
char gArmorDescTexts_1931[32] = {
#include "sub_main/00489360.inc"
};
char gArmorDescTexts_1932[32] = {
#include "sub_main/00489380.inc"
};
char gArmorDescTexts_1933[32] = {
#include "sub_main/004893A0.inc"
};
char gArmorDescTexts_1935[32] = {
#include "sub_main/004893C0.inc"
};
char gArmorDescTexts_1936[32] = {
#include "sub_main/004893E0.inc"
};
char gArmorDescTexts_1937[32] = {
#include "sub_main/00489400.inc"
};
char gArmorDescTexts_1939[32] = {
#include "sub_main/00489420.inc"
};
char gArmorDescTexts_1940[32] = {
#include "sub_main/00489440.inc"
};
char gArmorDescTexts_1941[32] = {
#include "sub_main/00489460.inc"
};
char gArmorDescTexts_1943[32] = {
#include "sub_main/00489480.inc"
};
char gArmorDescTexts_1944[32] = {
#include "sub_main/004894A0.inc"
};
char gArmorDescTexts_1945[32] = {
#include "sub_main/004894C0.inc"
};
char gArmorDescTexts_1947[32] = {
#include "sub_main/004894E0.inc"
};
char D_00489500[32] = {
#include "sub_main/00489500.inc"
};
char gArmorDescTexts_1968[32] = {
#include "sub_main/00489520.inc"
};
char gArmorDescTexts_1969[32] = {
#include "sub_main/00489540.inc"
};
char gArmorDescTexts_1972[32] = {
#include "sub_main/00489560.inc"
};
char gArmorDescTexts_1973[32] = {
#include "sub_main/00489580.inc"
};
char gArmorDescTexts_1975[32] = {
#include "sub_main/004895A0.inc"
};
char gArmorDescTexts_1976[32] = {
#include "sub_main/004895C0.inc"
};
char gArmorDescTexts_1977[32] = {
#include "sub_main/004895E0.inc"
};
char D_00489600[32] = {
#include "sub_main/00489600.inc"
};
char D_00489620[32] = {
#include "sub_main/00489620.inc"
};
char D_00489640[32] = {
#include "sub_main/00489640.inc"
};
char D_00489660[32] = {
#include "sub_main/00489660.inc"
};
char D_00489680[32] = {
#include "sub_main/00489680.inc"
};
char D_004896A0[32] = {
#include "sub_main/004896A0.inc"
};
char D_004896C0[32] = {
#include "sub_main/004896C0.inc"
};
char D_004896E0[32] = {
#include "sub_main/004896E0.inc"
};
char D_00489700[32] = {
#include "sub_main/00489700.inc"
};
char D_00489720[32] = {
#include "sub_main/00489720.inc"
};
char D_00489740[32] = {
#include "sub_main/00489740.inc"
};
char D_00489760[32] = {
#include "sub_main/00489760.inc"
};
char D_00489780[32] = {
#include "sub_main/00489780.inc"
};
char D_004897A0[32] = {
#include "sub_main/004897A0.inc"
};
char D_004897C0[32] = {
#include "sub_main/004897C0.inc"
};
char D_004897E0[32] = {
#include "sub_main/004897E0.inc"
};
char D_00489800[32] = {
#include "sub_main/00489800.inc"
};
char D_00489820[16] = {
#include "sub_main/00489820.inc"
};
char D_00489830[32] = {
#include "sub_main/00489830.inc"
};
char D_00489850[32] = {
#include "sub_main/00489850.inc"
};
char D_00489870[32] = {
#include "sub_main/00489870.inc"
};
char D_00489890[32] = {
#include "sub_main/00489890.inc"
};
char D_004898B0[32] = {
#include "sub_main/004898B0.inc"
};
char D_004898D0[32] = {
#include "sub_main/004898D0.inc"
};
char D_004898F0[32] = {
#include "sub_main/004898F0.inc"
};
char D_00489910[32] = {
#include "sub_main/00489910.inc"
};
char D_00489930[32] = {
#include "sub_main/00489930.inc"
};
char D_00489950[32] = {
#include "sub_main/00489950.inc"
};
char D_00489970[16] = {
#include "sub_main/00489970.inc"
};
char D_00489980[32] = {
#include "sub_main/00489980.inc"
};
char D_004899A0[32] = {
#include "sub_main/004899A0.inc"
};
char D_004899C0[32] = {
#include "sub_main/004899C0.inc"
};
char D_004899E0[32] = {
#include "sub_main/004899E0.inc"
};
char D_00489A00[32] = {
#include "sub_main/00489A00.inc"
};
char D_00489A20[32] = {
#include "sub_main/00489A20.inc"
};
char D_00489A40[24] = {
#include "sub_main/00489A40.inc"
};
char D_00489A58[13] = {
#include "sub_main/00489A58.inc"
};
char D_00489A70[32] = {
#include "sub_main/00489A70.inc"
};
char D_00489A90[32] = {
#include "sub_main/00489A90.inc"
};
char D_00489AB0[32] = {
#include "sub_main/00489AB0.inc"
};
char D_00489AD0[32] = {
#include "sub_main/00489AD0.inc"
};
char D_00489AF0[32] = {
#include "sub_main/00489AF0.inc"
};
char D_00489B10[32] = {
#include "sub_main/00489B10.inc"
};
char D_00489B30[32] = {
#include "sub_main/00489B30.inc"
};
char D_00489B50[32] = {
#include "sub_main/00489B50.inc"
};
char D_00489B70[32] = {
#include "sub_main/00489B70.inc"
};
char D_00489B90[32] = {
#include "sub_main/00489B90.inc"
};
char D_00489BB0[32] = {
#include "sub_main/00489BB0.inc"
};
char D_00489BD0[32] = {
#include "sub_main/00489BD0.inc"
};
char D_00489BF0[32] = {
#include "sub_main/00489BF0.inc"
};
char D_00489C10[32] = {
#include "sub_main/00489C10.inc"
};
char D_00489C30[32] = {
#include "sub_main/00489C30.inc"
};
char D_00489C50[32] = {
#include "sub_main/00489C50.inc"
};
char D_00489C70[32] = {
#include "sub_main/00489C70.inc"
};
char D_00489C90[32] = {
#include "sub_main/00489C90.inc"
};
char D_00489CB0[32] = {
#include "sub_main/00489CB0.inc"
};
char D_00489CD0[32] = {
#include "sub_main/00489CD0.inc"
};
char D_00489CF0[32] = {
#include "sub_main/00489CF0.inc"
};
char D_00489D10[32] = {
#include "sub_main/00489D10.inc"
};
char D_00489D30[32] = {
#include "sub_main/00489D30.inc"
};
char D_00489D50[32] = {
#include "sub_main/00489D50.inc"
};
char D_00489D70[32] = {
#include "sub_main/00489D70.inc"
};
char D_00489D90[32] = {
#include "sub_main/00489D90.inc"
};
char D_00489DB0[32] = {
#include "sub_main/00489DB0.inc"
};
char D_00489DD0[32] = {
#include "sub_main/00489DD0.inc"
};
char D_00489DF0[32] = {
#include "sub_main/00489DF0.inc"
};
char D_00489E10[32] = {
#include "sub_main/00489E10.inc"
};
char D_00489E30[32] = {
#include "sub_main/00489E30.inc"
};
char D_00489E50[32] = {
#include "sub_main/00489E50.inc"
};
char D_00489E70[32] = {
#include "sub_main/00489E70.inc"
};
char D_00489E90[32] = {
#include "sub_main/00489E90.inc"
};
char D_00489EB0[32] = {
#include "sub_main/00489EB0.inc"
};
char D_00489ED0[32] = {
#include "sub_main/00489ED0.inc"
};
char D_00489EF0[32] = {
#include "sub_main/00489EF0.inc"
};
char D_00489F10[32] = {
#include "sub_main/00489F10.inc"
};
char D_00489F30[16] = {
#include "sub_main/00489F30.inc"
};
char D_00489F40[32] = {
#include "sub_main/00489F40.inc"
};
char D_00489F60[32] = {
#include "sub_main/00489F60.inc"
};
char D_00489F80[32] = {
#include "sub_main/00489F80.inc"
};
char D_00489FA0[32] = {
#include "sub_main/00489FA0.inc"
};
char D_00489FC0[32] = {
#include "sub_main/00489FC0.inc"
};
char D_00489FE0[32] = {
#include "sub_main/00489FE0.inc"
};
char D_0048A000[32] = {
#include "sub_main/0048A000.inc"
};
char D_0048A020[32] = {
#include "sub_main/0048A020.inc"
};
char D_0048A040[32] = {
#include "sub_main/0048A040.inc"
};
char D_0048A060[32] = {
#include "sub_main/0048A060.inc"
};
char gArmorDescTexts_2123[32] = {
#include "sub_main/0048A080.inc"
};
char gArmorDescTexts_2124[32] = {
#include "sub_main/0048A0A0.inc"
};
char gArmorDescTexts_2125[32] = {
#include "sub_main/0048A0C0.inc"
};
char D_0048A0E0[32] = {
#include "sub_main/0048A0E0.inc"
};
char gArmorDescTexts_2128[32] = {
#include "sub_main/0048A100.inc"
};
char gArmorDescTexts_2129[32] = {
#include "sub_main/0048A120.inc"
};
char D_0048A140[32] = {
#include "sub_main/0048A140.inc"
};
char D_0048A160[32] = {
#include "sub_main/0048A160.inc"
};
char D_0048A180[32] = {
#include "sub_main/0048A180.inc"
};
char D_0048A1A0[32] = {
#include "sub_main/0048A1A0.inc"
};
char D_0048A1C0[32] = {
#include "sub_main/0048A1C0.inc"
};
char D_0048A1E0[32] = {
#include "sub_main/0048A1E0.inc"
};
char D_0048A200[32] = {
#include "sub_main/0048A200.inc"
};
char D_0048A220[32] = {
#include "sub_main/0048A220.inc"
};
char D_0048A240[32] = {
#include "sub_main/0048A240.inc"
};
char D_0048A260[32] = {
#include "sub_main/0048A260.inc"
};
char D_0048A280[32] = {
#include "sub_main/0048A280.inc"
};
char D_0048A2A0[32] = {
#include "sub_main/0048A2A0.inc"
};
char D_0048A2C0[32] = {
#include "sub_main/0048A2C0.inc"
};
char D_0048A2E0[32] = {
#include "sub_main/0048A2E0.inc"
};
char D_0048A300[32] = {
#include "sub_main/0048A300.inc"
};
char D_0048A320[32] = {
#include "sub_main/0048A320.inc"
};
char D_0048A340[32] = {
#include "sub_main/0048A340.inc"
};
char D_0048A360[32] = {
#include "sub_main/0048A360.inc"
};
char D_0048A380[32] = {
#include "sub_main/0048A380.inc"
};
char gArmorDescTexts_2167[32] = {
#include "sub_main/0048A3A0.inc"
};
char gArmorDescTexts_2168[32] = {
#include "sub_main/0048A3C0.inc"
};
char gArmorDescTexts_2169[16] = {
#include "sub_main/0048A3E0.inc"
};
char gArmorDescTexts_2171[32] = {
#include "sub_main/0048A3F0.inc"
};
char gArmorDescTexts_2172[32] = {
#include "sub_main/0048A410.inc"
};
char gArmorDescTexts_2173[32] = {
#include "sub_main/0048A430.inc"
};
char D_0048A450[16] = {
#include "sub_main/0048A450.inc"
};
char D_0048A460[32] = {
#include "sub_main/0048A460.inc"
};
char gArmorDescTexts_2180[32] = {
#include "sub_main/0048A480.inc"
};
char gArmorDescTexts_2181[32] = {
#include "sub_main/0048A4A0.inc"
};
char gArmorDescTexts_2184[32] = {
#include "sub_main/0048A4C0.inc"
};
char gArmorDescTexts_2185[32] = {
#include "sub_main/0048A4E0.inc"
};
char D_0048A500[32] = {
#include "sub_main/0048A500.inc"
};
char gArmorDescTexts_2196[32] = {
#include "sub_main/0048A520.inc"
};
char gArmorDescTexts_2197[32] = {
#include "sub_main/0048A540.inc"
};
char gArmorDescTexts_2200[32] = {
#include "sub_main/0048A560.inc"
};
char gArmorDescTexts_2201[32] = {
#include "sub_main/0048A580.inc"
};
char D_0048A5A0[32] = {
#include "sub_main/0048A5A0.inc"
};
char gArmorDescTexts_2204[32] = {
#include "sub_main/0048A5C0.inc"
};
char gArmorDescTexts_2205[32] = {
#include "sub_main/0048A5E0.inc"
};
char gArmorDescTexts_2207[32] = {
#include "sub_main/0048A600.inc"
};
char gArmorDescTexts_2208[32] = {
#include "sub_main/0048A620.inc"
};
char gArmorDescTexts_2209[32] = {
#include "sub_main/0048A640.inc"
};
char gArmorDescTexts_2211[32] = {
#include "sub_main/0048A660.inc"
};
char gArmorDescTexts_2212[32] = {
#include "sub_main/0048A680.inc"
};
char gArmorDescTexts_2213[32] = {
#include "sub_main/0048A6A0.inc"
};
char gArmorDescTexts_2215[32] = {
#include "sub_main/0048A6C0.inc"
};
char gArmorDescTexts_2216[32] = {
#include "sub_main/0048A6E0.inc"
};
char gArmorDescTexts_2217[32] = {
#include "sub_main/0048A700.inc"
};
char D_0048A720[32] = {
#include "sub_main/0048A720.inc"
};
char D_0048A740[32] = {
#include "sub_main/0048A740.inc"
};
char D_0048A760[32] = {
#include "sub_main/0048A760.inc"
};
char D_0048A780[32] = {
#include "sub_main/0048A780.inc"
};
char D_0048A7A0[32] = {
#include "sub_main/0048A7A0.inc"
};
char D_0048A7C0[32] = {
#include "sub_main/0048A7C0.inc"
};
char D_0048A7E0[32] = {
#include "sub_main/0048A7E0.inc"
};
char D_0048A800[32] = {
#include "sub_main/0048A800.inc"
};
char D_0048A820[32] = {
#include "sub_main/0048A820.inc"
};
char D_0048A840[32] = {
#include "sub_main/0048A840.inc"
};
char D_0048A860[32] = {
#include "sub_main/0048A860.inc"
};
char D_0048A880[32] = {
#include "sub_main/0048A880.inc"
};
char D_0048A8A0[32] = {
#include "sub_main/0048A8A0.inc"
};
char D_0048A8C0[32] = {
#include "sub_main/0048A8C0.inc"
};
char gArmorDescTexts_2316[32] = {
#include "sub_main/0048A8E0.inc"
};
char gArmorDescTexts_2327[32] = {
#include "sub_main/0048A900.inc"
};
char gArmorDescTexts_2351[32] = {
#include "sub_main/0048A920.inc"
};
char gArmorDescTexts_2352[32] = {
#include "sub_main/0048A940.inc"
};
char gArmorDescTexts_2353[32] = {
#include "sub_main/0048A960.inc"
};
char gArmorDescTexts_2355[32] = {
#include "sub_main/0048A980.inc"
};
char gArmorDescTexts_2356[32] = {
#include "sub_main/0048A9A0.inc"
};
char gArmorDescTexts_2357[32] = {
#include "sub_main/0048A9C0.inc"
};
char gArmorDescTexts_2367[32] = {
#include "sub_main/0048A9E0.inc"
};
char gArmorDescTexts_2368[32] = {
#include "sub_main/0048AA00.inc"
};
char gArmorDescTexts_2369[32] = {
#include "sub_main/0048AA20.inc"
};
char gArmorDescTexts_2371[32] = {
#include "sub_main/0048AA40.inc"
};
char gArmorDescTexts_2372[32] = {
#include "sub_main/0048AA60.inc"
};
char gArmorDescTexts_2373[32] = {
#include "sub_main/0048AA80.inc"
};
char gArmorDescTexts_2417[32] = {
#include "sub_main/0048AAA0.inc"
};
char gArmorDescTexts_2420[32] = {
#include "sub_main/0048AAC0.inc"
};
char gArmorDescTexts_2421[16] = {
#include "sub_main/0048AAE0.inc"
};
char gArmorDescTexts_2424[32] = {
#include "sub_main/0048AAF0.inc"
};
char gArmorDescTexts_2425[32] = {
#include "sub_main/0048AB10.inc"
};
char gArmorDescTexts_2427[32] = {
#include "sub_main/0048AB30.inc"
};
char gArmorDescTexts_2428[32] = {
#include "sub_main/0048AB50.inc"
};
char gArmorDescTexts_2429[32] = {
#include "sub_main/0048AB70.inc"
};
char D_0048AB90[32] = {
#include "sub_main/0048AB90.inc"
};
char gArmorDescTexts_2432[32] = {
#include "sub_main/0048ABB0.inc"
};
char gArmorDescTexts_2433[32] = {
#include "sub_main/0048ABD0.inc"
};
char D_0048ABF0[32] = {
#include "sub_main/0048ABF0.inc"
};
char gArmorDescTexts_2436[32] = {
#include "sub_main/0048AC10.inc"
};
char gArmorDescTexts_2437[32] = {
#include "sub_main/0048AC30.inc"
};
char gArmorDescTexts_2455[32] = {
#include "sub_main/0048AC50.inc"
};
char gArmorDescTexts_2456[32] = {
#include "sub_main/0048AC70.inc"
};
char gArmorDescTexts_2457[32] = {
#include "sub_main/0048AC90.inc"
};
char gArmorDescTexts_2459[32] = {
#include "sub_main/0048ACB0.inc"
};
char gArmorDescTexts_2460[32] = {
#include "sub_main/0048ACD0.inc"
};
char gArmorDescTexts_2461[32] = {
#include "sub_main/0048ACF0.inc"
};
char gArmorDescTexts_2463[32] = {
#include "sub_main/0048AD10.inc"
};
char gArmorDescTexts_2464[32] = {
#include "sub_main/0048AD30.inc"
};
char gArmorDescTexts_2465[32] = {
#include "sub_main/0048AD50.inc"
};
char gArmorDescTexts_2467[32] = {
#include "sub_main/0048AD70.inc"
};
char gArmorDescTexts_2468[32] = {
#include "sub_main/0048AD90.inc"
};
char gArmorDescTexts_2469[32] = {
#include "sub_main/0048ADB0.inc"
};
char gArmorDescTexts_2471[32] = {
#include "sub_main/0048ADD0.inc"
};
char gArmorDescTexts_2472[32] = {
#include "sub_main/0048ADF0.inc"
};
char gArmorDescTexts_2473[32] = {
#include "sub_main/0048AE10.inc"
};
char gArmorDescTexts_2475[32] = {
#include "sub_main/0048AE30.inc"
};
char gArmorDescTexts_2476[32] = {
#include "sub_main/0048AE50.inc"
};
char gArmorDescTexts_2477[32] = {
#include "sub_main/0048AE70.inc"
};
char gArmorDescTexts_2651[32] = {
#include "sub_main/0048AE90.inc"
};
char D_0048AEB0[32] = {
#include "sub_main/0048AEB0.inc"
};
char D_0048AED0[32] = {
#include "sub_main/0048AED0.inc"
};
char gArmorDescTexts_2655[32] = {
#include "sub_main/0048AEF0.inc"
};
char D_0048AF10[32] = {
#include "sub_main/0048AF10.inc"
};
char D_0048AF30[32] = {
#include "sub_main/0048AF30.inc"
};
char D_0048AF50[32] = {
#include "sub_main/0048AF50.inc"
};
char D_0048AF70[32] = {
#include "sub_main/0048AF70.inc"
};
char D_0048AF90[32] = {
#include "sub_main/0048AF90.inc"
};
char D_0048AFB0[32] = {
#include "sub_main/0048AFB0.inc"
};
char D_0048AFD0[32] = {
#include "sub_main/0048AFD0.inc"
};
char D_0048AFF0[32] = {
#include "sub_main/0048AFF0.inc"
};
char D_0048B010[32] = {
#include "sub_main/0048B010.inc"
};
char D_0048B030[32] = {
#include "sub_main/0048B030.inc"
};
char D_0048B050[32] = {
#include "sub_main/0048B050.inc"
};
char D_0048B070[32] = {
#include "sub_main/0048B070.inc"
};
char D_0048B090[32] = {
#include "sub_main/0048B090.inc"
};
char D_0048B0B0[32] = {
#include "sub_main/0048B0B0.inc"
};
char D_0048B0D0[32] = {
#include "sub_main/0048B0D0.inc"
};
char D_0048B0F0[32] = {
#include "sub_main/0048B0F0.inc"
};
char D_0048B110[32] = {
#include "sub_main/0048B110.inc"
};
char D_0048B130[32] = {
#include "sub_main/0048B130.inc"
};
char D_0048B150[32] = {
#include "sub_main/0048B150.inc"
};
char D_0048B170[32] = {
#include "sub_main/0048B170.inc"
};
char D_0048B190[32] = {
#include "sub_main/0048B190.inc"
};
char D_0048B1B0[32] = {
#include "sub_main/0048B1B0.inc"
};
char D_0048B1D0[32] = {
#include "sub_main/0048B1D0.inc"
};
char D_0048B1F0[32] = {
#include "sub_main/0048B1F0.inc"
};
char D_0048B210[32] = {
#include "sub_main/0048B210.inc"
};
char D_0048B230[32] = {
#include "sub_main/0048B230.inc"
};
char D_0048B250[32] = {
#include "sub_main/0048B250.inc"
};
char D_0048B270[32] = {
#include "sub_main/0048B270.inc"
};
char D_0048B290[32] = {
#include "sub_main/0048B290.inc"
};
char D_0048B2B0[32] = {
#include "sub_main/0048B2B0.inc"
};
char D_0048B2D0[32] = {
#include "sub_main/0048B2D0.inc"
};
char D_0048B2F0[32] = {
#include "sub_main/0048B2F0.inc"
};
char D_0048B310[32] = {
#include "sub_main/0048B310.inc"
};
char D_0048B330[32] = {
#include "sub_main/0048B330.inc"
};
char D_0048B350[16] = {
#include "sub_main/0048B350.inc"
};
char D_0048B360[32] = {
#include "sub_main/0048B360.inc"
};
char D_0048B380[32] = {
#include "sub_main/0048B380.inc"
};
char D_0048B3A0[32] = {
#include "sub_main/0048B3A0.inc"
};
char D_0048B3C0[32] = {
#include "sub_main/0048B3C0.inc"
};
char gArmorDescTexts_2812[32] = {
#include "sub_main/0048B3E0.inc"
};
char gArmorDescTexts_2813[32] = {
#include "sub_main/0048B400.inc"
};
char gArmorDescTexts_2816[32] = {
#include "sub_main/0048B420.inc"
};
char gArmorDescTexts_2817[32] = {
#include "sub_main/0048B440.inc"
};
char gArmorDescTexts_2819[32] = {
#include "sub_main/0048B460.inc"
};
char gArmorDescTexts_2820[32] = {
#include "sub_main/0048B480.inc"
};
char gArmorDescTexts_2821[32] = {
#include "sub_main/0048B4A0.inc"
};
char gArmorDescTexts_2823[32] = {
#include "sub_main/0048B4C0.inc"
};
char gArmorDescTexts_2824[32] = {
#include "sub_main/0048B4E0.inc"
};
char gArmorDescTexts_2825[32] = {
#include "sub_main/0048B500.inc"
};
char gArmorDescTexts_2827[32] = {
#include "sub_main/0048B520.inc"
};
char gArmorDescTexts_2828[32] = {
#include "sub_main/0048B540.inc"
};
char gArmorDescTexts_2829[32] = {
#include "sub_main/0048B560.inc"
};
char gArmorDescTexts_2831[32] = {
#include "sub_main/0048B580.inc"
};
char D_0048B5A0[32] = {
#include "sub_main/0048B5A0.inc"
};
char gArmorDescTexts_2833[32] = {
#include "sub_main/0048B5C0.inc"
};
char gArmorDescTexts_2835[32] = {
#include "sub_main/0048B5E0.inc"
};
char gArmorDescTexts_2836[32] = {
#include "sub_main/0048B600.inc"
};
char gArmorDescTexts_2837[32] = {
#include "sub_main/0048B620.inc"
};
char gArmorDescTexts_2839[32] = {
#include "sub_main/0048B640.inc"
};
char gArmorDescTexts_2840[32] = {
#include "sub_main/0048B660.inc"
};
char gArmorDescTexts_2841[32] = {
#include "sub_main/0048B680.inc"
};
char gArmorDescTexts_2843[32] = {
#include "sub_main/0048B6A0.inc"
};
char gArmorDescTexts_2844[32] = {
#include "sub_main/0048B6C0.inc"
};
char gArmorDescTexts_2845[32] = {
#include "sub_main/0048B6E0.inc"
};
char gArmorDescTexts_2847[32] = {
#include "sub_main/0048B700.inc"
};
char gArmorDescTexts_2849[32] = {
#include "sub_main/0048B720.inc"
};
char D_0048B740[32] = {
#include "sub_main/0048B740.inc"
};
char D_0048B760[32] = {
#include "sub_main/0048B760.inc"
};
char gArmorDescTexts_2853[32] = {
#include "sub_main/0048B780.inc"
};
char gArmorDescTexts_2857[32] = {
#include "sub_main/0048B7A0.inc"
};
char gArmorDescTexts_2859[32] = {
#include "sub_main/0048B7C0.inc"
};
char gArmorDescTexts_2860[32] = {
#include "sub_main/0048B7E0.inc"
};
char gArmorDescTexts_2861[32] = {
#include "sub_main/0048B800.inc"
};
char gArmorDescTexts_2863[32] = {
#include "sub_main/0048B820.inc"
};
char gArmorDescTexts_2864[32] = {
#include "sub_main/0048B840.inc"
};
char gArmorDescTexts_2865[32] = {
#include "sub_main/0048B860.inc"
};
char D_0048B880[32] = {
#include "sub_main/0048B880.inc"
};
char D_0048B8A0[32] = {
#include "sub_main/0048B8A0.inc"
};
char D_0048B8C0[16] = {
#include "sub_main/0048B8C0.inc"
};
char gArmorDescTexts_2883[32] = {
#include "sub_main/0048B8D0.inc"
};
char gArmorDescTexts_2884[32] = {
#include "sub_main/0048B8F0.inc"
};
char gArmorDescTexts_2885[32] = {
#include "sub_main/0048B910.inc"
};
char gArmorDescTexts_2887[32] = {
#include "sub_main/0048B930.inc"
};
char gArmorDescTexts_2888[32] = {
#include "sub_main/0048B950.inc"
};
char gArmorDescTexts_2889[32] = {
#include "sub_main/0048B970.inc"
};
char gArmorDescTexts_2891[32] = {
#include "sub_main/0048B990.inc"
};
char gArmorDescTexts_2892[32] = {
#include "sub_main/0048B9B0.inc"
};
char gArmorDescTexts_2893[32] = {
#include "sub_main/0048B9D0.inc"
};
char gArmorDescTexts_2895[32] = {
#include "sub_main/0048B9F0.inc"
};
char gArmorDescTexts_2896[32] = {
#include "sub_main/0048BA10.inc"
};
char gArmorDescTexts_2897[32] = {
#include "sub_main/0048BA30.inc"
};
char gArmorDescTexts_2899[32] = {
#include "sub_main/0048BA50.inc"
};
char gArmorDescTexts_2900[32] = {
#include "sub_main/0048BA70.inc"
};
char gArmorDescTexts_2901[32] = {
#include "sub_main/0048BA90.inc"
};
char gArmorDescTexts_2911[32] = {
#include "sub_main/0048BAB0.inc"
};
char gArmorDescTexts_2919[32] = {
#include "sub_main/0048BAD0.inc"
};
char gArmorDescTexts_2920[32] = {
#include "sub_main/0048BAF0.inc"
};
char gArmorDescTexts_2921[32] = {
#include "sub_main/0048BB10.inc"
};
char gArmorDescTexts_2923[32] = {
#include "sub_main/0048BB30.inc"
};
char gArmorDescTexts_2924[32] = {
#include "sub_main/0048BB50.inc"
};
char gArmorDescTexts_2925[32] = {
#include "sub_main/0048BB70.inc"
};
char gArmorDescTexts_2927[32] = {
#include "sub_main/0048BB90.inc"
};
char gArmorDescTexts_2928[32] = {
#include "sub_main/0048BBB0.inc"
};
char gArmorDescTexts_2929[32] = {
#include "sub_main/0048BBD0.inc"
};
char D_0048BBF0[32] = {
#include "sub_main/0048BBF0.inc"
};
char D_0048BC10[32] = {
#include "sub_main/0048BC10.inc"
};
char gArmorDescTexts_2939[32] = {
#include "sub_main/0048BC30.inc"
};
char gArmorDescTexts_2940[32] = {
#include "sub_main/0048BC50.inc"
};
char gArmorDescTexts_2941[32] = {
#include "sub_main/0048BC70.inc"
};
char D_0048BC90[32] = {
#include "sub_main/0048BC90.inc"
};
char D_0048BCB0[16] = {
#include "sub_main/0048BCB0.inc"
};
char D_0048BCC0[32] = {
#include "sub_main/0048BCC0.inc"
};
char gArmorDescTexts_2947[32] = {
#include "sub_main/0048BCE0.inc"
};
char gArmorDescTexts_2948[32] = {
#include "sub_main/0048BD00.inc"
};
char gArmorDescTexts_2949[32] = {
#include "sub_main/0048BD20.inc"
};
char D_0048BD40[32] = {
#include "sub_main/0048BD40.inc"
};
char D_0048BD60[32] = {
#include "sub_main/0048BD60.inc"
};
char D_0048BD80[32] = {
#include "sub_main/0048BD80.inc"
};
char D_0048BDA0[32] = {
#include "sub_main/0048BDA0.inc"
};
char D_0048BDC0[16] = {
#include "sub_main/0048BDC0.inc"
};
char D_0048BDD0[32] = {
#include "sub_main/0048BDD0.inc"
};
char D_0048BDF0[32] = {
#include "sub_main/0048BDF0.inc"
};
char D_0048BE10[32] = {
#include "sub_main/0048BE10.inc"
};
char D_0048BE30[32] = {
#include "sub_main/0048BE30.inc"
};
char D_0048BE50[32] = {
#include "sub_main/0048BE50.inc"
};
char D_0048BE70[32] = {
#include "sub_main/0048BE70.inc"
};
char D_0048BE90[32] = {
#include "sub_main/0048BE90.inc"
};
char D_0048BEB0[32] = {
#include "sub_main/0048BEB0.inc"
};
char D_0048BED0[32] = {
#include "sub_main/0048BED0.inc"
};
char D_0048BEF0[32] = {
#include "sub_main/0048BEF0.inc"
};
char D_0048BF10[32] = {
#include "sub_main/0048BF10.inc"
};
char D_0048BF30[32] = {
#include "sub_main/0048BF30.inc"
};
char D_0048BF50[16] = {
#include "sub_main/0048BF50.inc"
};
char D_0048BF60[32] = {
#include "sub_main/0048BF60.inc"
};
char D_0048BF80[32] = {
#include "sub_main/0048BF80.inc"
};
char gArmorDescTexts_2983[32] = {
#include "sub_main/0048BFA0.inc"
};
char gArmorDescTexts_2984[32] = {
#include "sub_main/0048BFC0.inc"
};
char gArmorDescTexts_2985[32] = {
#include "sub_main/0048BFE0.inc"
};
char D_0048C000[32] = {
#include "sub_main/0048C000.inc"
};
char D_0048C020[32] = {
#include "sub_main/0048C020.inc"
};
char D_0048C040[32] = {
#include "sub_main/0048C040.inc"
};
char D_0048C060[32] = {
#include "sub_main/0048C060.inc"
};
char D_0048C080[32] = {
#include "sub_main/0048C080.inc"
};
char D_0048C0A0[32] = {
#include "sub_main/0048C0A0.inc"
};
char D_0048C0C0[32] = {
#include "sub_main/0048C0C0.inc"
};
char D_0048C0E0[32] = {
#include "sub_main/0048C0E0.inc"
};
char D_0048C100[32] = {
#include "sub_main/0048C100.inc"
};
char D_0048C120[32] = {
#include "sub_main/0048C120.inc"
};
char D_0048C140[32] = {
#include "sub_main/0048C140.inc"
};
char D_0048C160[32] = {
#include "sub_main/0048C160.inc"
};
char D_0048C180[32] = {
#include "sub_main/0048C180.inc"
};
char D_0048C1A0[32] = {
#include "sub_main/0048C1A0.inc"
};
char D_0048C1C0[16] = {
#include "sub_main/0048C1C0.inc"
};
char D_0048C1D0[32] = {
#include "sub_main/0048C1D0.inc"
};
char D_0048C1F0[16] = {
#include "sub_main/0048C1F0.inc"
};
char D_0048C200[32] = {
#include "sub_main/0048C200.inc"
};
char D_0048C220[32] = {
#include "sub_main/0048C220.inc"
};
char D_0048C240[32] = {
#include "sub_main/0048C240.inc"
};
char D_0048C260[32] = {
#include "sub_main/0048C260.inc"
};
char D_0048C280[32] = {
#include "sub_main/0048C280.inc"
};
char D_0048C2A0[32] = {
#include "sub_main/0048C2A0.inc"
};
char gArmorDescTexts_3020[32] = {
#include "sub_main/0048C2C0.inc"
};
char gArmorDescTexts_3021[32] = {
#include "sub_main/0048C2E0.inc"
};
char D_0048C300[32] = {
#include "sub_main/0048C300.inc"
};
char D_0048C320[32] = {
#include "sub_main/0048C320.inc"
};
char D_0048C340[32] = {
#include "sub_main/0048C340.inc"
};
char D_0048C360[16] = {
#include "sub_main/0048C360.inc"
};
char D_0048C370[32] = {
#include "sub_main/0048C370.inc"
};
char D_0048C390[32] = {
#include "sub_main/0048C390.inc"
};
char D_0048C3B0[32] = {
#include "sub_main/0048C3B0.inc"
};
char D_0048C3D0[32] = {
#include "sub_main/0048C3D0.inc"
};
char D_0048C3F0[32] = {
#include "sub_main/0048C3F0.inc"
};
char D_0048C410[32] = {
#include "sub_main/0048C410.inc"
};
char D_0048C430[32] = {
#include "sub_main/0048C430.inc"
};
char D_0048C450[32] = {
#include "sub_main/0048C450.inc"
};
char D_0048C470[32] = {
#include "sub_main/0048C470.inc"
};
char D_0048C490[32] = {
#include "sub_main/0048C490.inc"
};
char D_0048C4B0[16] = {
#include "sub_main/0048C4B0.inc"
};
char D_0048C4C0[32] = {
#include "sub_main/0048C4C0.inc"
};
char D_0048C4E0[32] = {
#include "sub_main/0048C4E0.inc"
};
char D_0048C500[32] = {
#include "sub_main/0048C500.inc"
};
char D_0048C520[32] = {
#include "sub_main/0048C520.inc"
};
char D_0048C540[32] = {
#include "sub_main/0048C540.inc"
};
char D_0048C560[32] = {
#include "sub_main/0048C560.inc"
};
char D_0048C580[32] = {
#include "sub_main/0048C580.inc"
};
char D_0048C5A0[32] = {
#include "sub_main/0048C5A0.inc"
};
char D_0048C5C0[32] = {
#include "sub_main/0048C5C0.inc"
};
char D_0048C5E0[32] = {
#include "sub_main/0048C5E0.inc"
};
char D_0048C600[32] = {
#include "sub_main/0048C600.inc"
};
char D_0048C620[32] = {
#include "sub_main/0048C620.inc"
};
char D_0048C640[32] = {
#include "sub_main/0048C640.inc"
};
char D_0048C660[32] = {
#include "sub_main/0048C660.inc"
};
char D_0048C680[32] = {
#include "sub_main/0048C680.inc"
};
char D_0048C6A0[32] = {
#include "sub_main/0048C6A0.inc"
};
char D_0048C6C0[32] = {
#include "sub_main/0048C6C0.inc"
};
char D_0048C6E0[32] = {
#include "sub_main/0048C6E0.inc"
};
char D_0048C700[32] = {
#include "sub_main/0048C700.inc"
};
char D_0048C720[32] = {
#include "sub_main/0048C720.inc"
};
char D_0048C740[32] = {
#include "sub_main/0048C740.inc"
};
char D_0048C760[32] = {
#include "sub_main/0048C760.inc"
};
char D_0048C780[32] = {
#include "sub_main/0048C780.inc"
};
char D_0048C7A0[32] = {
#include "sub_main/0048C7A0.inc"
};
char D_0048C7C0[32] = {
#include "sub_main/0048C7C0.inc"
};
char D_0048C7E0[32] = {
#include "sub_main/0048C7E0.inc"
};
char D_0048C800[32] = {
#include "sub_main/0048C800.inc"
};
char D_0048C820[32] = {
#include "sub_main/0048C820.inc"
};
char D_0048C840[32] = {
#include "sub_main/0048C840.inc"
};
char D_0048C860[24] = {
#include "sub_main/0048C860.inc"
};
char D_0048C878[15] = {
#include "sub_main/0048C878.inc"
};
char D_0048C890[32] = {
#include "sub_main/0048C890.inc"
};
char D_0048C8B0[32] = {
#include "sub_main/0048C8B0.inc"
};
char D_0048C8D0[32] = {
#include "sub_main/0048C8D0.inc"
};
char D_0048C8F0[32] = {
#include "sub_main/0048C8F0.inc"
};
char D_0048C910[32] = {
#include "sub_main/0048C910.inc"
};
char D_0048C930[32] = {
#include "sub_main/0048C930.inc"
};
char D_0048C950[32] = {
#include "sub_main/0048C950.inc"
};
char D_0048C970[32] = {
#include "sub_main/0048C970.inc"
};
char D_0048C990[32] = {
#include "sub_main/0048C990.inc"
};
char D_0048C9B0[32] = {
#include "sub_main/0048C9B0.inc"
};
char gArmorDescTexts_3119[32] = {
#include "sub_main/0048C9D0.inc"
};
char gArmorDescTexts_3120[32] = {
#include "sub_main/0048C9F0.inc"
};
char gArmorDescTexts_3121[32] = {
#include "sub_main/0048CA10.inc"
};
char gArmorDescTexts_3123[32] = {
#include "sub_main/0048CA30.inc"
};
char gArmorDescTexts_3124[32] = {
#include "sub_main/0048CA50.inc"
};
char gArmorDescTexts_3125[32] = {
#include "sub_main/0048CA70.inc"
};
char gArmorDescTexts_3128[32] = {
#include "sub_main/0048CA90.inc"
};
char gArmorDescTexts_3129[32] = {
#include "sub_main/0048CAB0.inc"
};
char D_0048CAD0[32] = {
#include "sub_main/0048CAD0.inc"
};
char gArmorDescTexts_3140[32] = {
#include "sub_main/0048CAF0.inc"
};
char gArmorDescTexts_3141[32] = {
#include "sub_main/0048CB10.inc"
};
char gArmorDescTexts_3144[32] = {
#include "sub_main/0048CB30.inc"
};
char gArmorDescTexts_3145[32] = {
#include "sub_main/0048CB50.inc"
};
char gArmorDescTexts_3147[32] = {
#include "sub_main/0048CB70.inc"
};
char gArmorDescTexts_3148[32] = {
#include "sub_main/0048CB90.inc"
};
char gArmorDescTexts_3149[32] = {
#include "sub_main/0048CBB0.inc"
};
char gArmorDescTexts_3152[32] = {
#include "sub_main/0048CBD0.inc"
};
char gArmorDescTexts_3153[32] = {
#include "sub_main/0048CBF0.inc"
};
char gArmorDescTexts_3155[32] = {
#include "sub_main/0048CC10.inc"
};
char gArmorDescTexts_3156[32] = {
#include "sub_main/0048CC30.inc"
};
char gArmorDescTexts_3157[32] = {
#include "sub_main/0048CC50.inc"
};
char gArmorDescTexts_3159[32] = {
#include "sub_main/0048CC70.inc"
};
char gArmorDescTexts_3160[32] = {
#include "sub_main/0048CC90.inc"
};
char gArmorDescTexts_3161[32] = {
#include "sub_main/0048CCB0.inc"
};
char D_0048CCD0[32] = {
#include "sub_main/0048CCD0.inc"
};
char D_0048CCF0[32] = {
#include "sub_main/0048CCF0.inc"
};
char D_0048CD10[32] = {
#include "sub_main/0048CD10.inc"
};
char D_0048CD30[32] = {
#include "sub_main/0048CD30.inc"
};
char D_0048CD50[32] = {
#include "sub_main/0048CD50.inc"
};
char D_0048CD70[32] = {
#include "sub_main/0048CD70.inc"
};
char D_0048CD90[32] = {
#include "sub_main/0048CD90.inc"
};
char D_0048CDB0[32] = {
#include "sub_main/0048CDB0.inc"
};
char D_0048CDD0[32] = {
#include "sub_main/0048CDD0.inc"
};
char gArmorDescTexts_3179[32] = {
#include "sub_main/0048CDF0.inc"
};
char gArmorDescTexts_3180[32] = {
#include "sub_main/0048CE10.inc"
};
char gArmorDescTexts_3183[32] = {
#include "sub_main/0048CE30.inc"
};
char gArmorDescTexts_3184[32] = {
#include "sub_main/0048CE50.inc"
};
char gArmorDescTexts_3185[32] = {
#include "sub_main/0048CE70.inc"
};
char D_0048CE90[32] = {
#include "sub_main/0048CE90.inc"
};
char D_0048CEB0[32] = {
#include "sub_main/0048CEB0.inc"
};
char D_0048CED0[32] = {
#include "sub_main/0048CED0.inc"
};
char D_0048CEF0[32] = {
#include "sub_main/0048CEF0.inc"
};
char D_0048CF10[32] = {
#include "sub_main/0048CF10.inc"
};
char D_0048CF30[32] = {
#include "sub_main/0048CF30.inc"
};
char gArmorDescTexts_3279[32] = {
#include "sub_main/0048CF50.inc"
};
char gArmorDescTexts_3303[32] = {
#include "sub_main/0048CF70.inc"
};
char gArmorDescTexts_3304[32] = {
#include "sub_main/0048CF90.inc"
};
char gArmorDescTexts_3305[32] = {
#include "sub_main/0048CFB0.inc"
};
char gArmorDescTexts_3307[32] = {
#include "sub_main/0048CFD0.inc"
};
char gArmorDescTexts_3308[32] = {
#include "sub_main/0048CFF0.inc"
};
char gArmorDescTexts_3309[32] = {
#include "sub_main/0048D010.inc"
};
char gArmorDescTexts_3319[32] = {
#include "sub_main/0048D030.inc"
};
char gArmorDescTexts_3320[32] = {
#include "sub_main/0048D050.inc"
};
char gArmorDescTexts_3321[32] = {
#include "sub_main/0048D070.inc"
};
char gArmorDescTexts_3323[32] = {
#include "sub_main/0048D090.inc"
};
char gArmorDescTexts_3324[32] = {
#include "sub_main/0048D0B0.inc"
};
char gArmorDescTexts_3325[32] = {
#include "sub_main/0048D0D0.inc"
};
char gArmorDescTexts_3361[32] = {
#include "sub_main/0048D0F0.inc"
};
char gArmorDescTexts_3364[32] = {
#include "sub_main/0048D110.inc"
};
char gArmorDescTexts_3365[16] = {
#include "sub_main/0048D130.inc"
};
char gArmorDescTexts_3367[32] = {
#include "sub_main/0048D140.inc"
};
char gArmorDescTexts_3368[32] = {
#include "sub_main/0048D160.inc"
};
char gArmorDescTexts_3369[32] = {
#include "sub_main/0048D180.inc"
};
char gArmorDescTexts_3371[32] = {
#include "sub_main/0048D1A0.inc"
};
char gArmorDescTexts_3372[32] = {
#include "sub_main/0048D1C0.inc"
};
char gArmorDescTexts_3373[32] = {
#include "sub_main/0048D1E0.inc"
};
char gArmorDescTexts_3376[32] = {
#include "sub_main/0048D200.inc"
};
char gArmorDescTexts_3377[32] = {
#include "sub_main/0048D220.inc"
};
char gArmorDescTexts_3380[32] = {
#include "sub_main/0048D240.inc"
};
char gArmorDescTexts_3381[32] = {
#include "sub_main/0048D260.inc"
};
char gArmorDescTexts_3399[32] = {
#include "sub_main/0048D280.inc"
};
char gArmorDescTexts_3400[32] = {
#include "sub_main/0048D2A0.inc"
};
char gArmorDescTexts_3401[32] = {
#include "sub_main/0048D2C0.inc"
};
char gArmorDescTexts_3403[32] = {
#include "sub_main/0048D2E0.inc"
};
char gArmorDescTexts_3404[32] = {
#include "sub_main/0048D300.inc"
};
char gArmorDescTexts_3405[32] = {
#include "sub_main/0048D320.inc"
};
char gArmorDescTexts_3407[32] = {
#include "sub_main/0048D340.inc"
};
char gArmorDescTexts_3408[32] = {
#include "sub_main/0048D360.inc"
};
char gArmorDescTexts_3409[32] = {
#include "sub_main/0048D380.inc"
};
char gArmorDescTexts_3411[32] = {
#include "sub_main/0048D3A0.inc"
};
char gArmorDescTexts_3412[32] = {
#include "sub_main/0048D3C0.inc"
};
char gArmorDescTexts_3413[32] = {
#include "sub_main/0048D3E0.inc"
};
char gArmorDescTexts_3415[32] = {
#include "sub_main/0048D400.inc"
};
char gArmorDescTexts_3416[32] = {
#include "sub_main/0048D420.inc"
};
char gArmorDescTexts_3417[32] = {
#include "sub_main/0048D440.inc"
};
char gArmorDescTexts_3419[32] = {
#include "sub_main/0048D460.inc"
};
char gArmorDescTexts_3420[32] = {
#include "sub_main/0048D480.inc"
};
char gArmorDescTexts_3421[32] = {
#include "sub_main/0048D4A0.inc"
};
char D_0048D4C0[32] = {
#include "sub_main/0048D4C0.inc"
};
char D_0048D4E0[32] = {
#include "sub_main/0048D4E0.inc"
};
char D_0048D500[32] = {
#include "sub_main/0048D500.inc"
};
char D_0048D520[32] = {
#include "sub_main/0048D520.inc"
};
char D_0048D540[32] = {
#include "sub_main/0048D540.inc"
};
char D_0048D560[32] = {
#include "sub_main/0048D560.inc"
};
char D_0048D580[32] = {
#include "sub_main/0048D580.inc"
};
char D_0048D5A0[32] = {
#include "sub_main/0048D5A0.inc"
};
char D_0048D5C0[32] = {
#include "sub_main/0048D5C0.inc"
};
char D_0048D5E0[32] = {
#include "sub_main/0048D5E0.inc"
};
char D_0048D600[32] = {
#include "sub_main/0048D600.inc"
};
char D_0048D620[16] = {
#include "sub_main/0048D620.inc"
};
char D_0048D630[32] = {
#include "sub_main/0048D630.inc"
};
char D_0048D650[32] = {
#include "sub_main/0048D650.inc"
};
char D_0048D670[32] = {
#include "sub_main/0048D670.inc"
};
char D_0048D690[32] = {
#include "sub_main/0048D690.inc"
};
char D_0048D6B0[32] = {
#include "sub_main/0048D6B0.inc"
};
char D_0048D6D0[32] = {
#include "sub_main/0048D6D0.inc"
};
char D_0048D6F0[32] = {
#include "sub_main/0048D6F0.inc"
};
char D_0048D710[32] = {
#include "sub_main/0048D710.inc"
};
char D_0048D730[32] = {
#include "sub_main/0048D730.inc"
};
char D_0048D750[32] = {
#include "sub_main/0048D750.inc"
};
char D_0048D770[32] = {
#include "sub_main/0048D770.inc"
};
char D_0048D790[32] = {
#include "sub_main/0048D790.inc"
};
char D_0048D7B0[32] = {
#include "sub_main/0048D7B0.inc"
};
char D_0048D7D0[32] = {
#include "sub_main/0048D7D0.inc"
};
char D_0048D7F0[32] = {
#include "sub_main/0048D7F0.inc"
};
char D_0048D810[32] = {
#include "sub_main/0048D810.inc"
};
char D_0048D830[32] = {
#include "sub_main/0048D830.inc"
};
char D_0048D850[32] = {
#include "sub_main/0048D850.inc"
};
char D_0048D870[32] = {
#include "sub_main/0048D870.inc"
};
char D_0048D890[32] = {
#include "sub_main/0048D890.inc"
};
char D_0048D8B0[32] = {
#include "sub_main/0048D8B0.inc"
};
char D_0048D8D0[32] = {
#include "sub_main/0048D8D0.inc"
};
char D_0048D8F0[32] = {
#include "sub_main/0048D8F0.inc"
};
char D_0048D910[32] = {
#include "sub_main/0048D910.inc"
};
char D_0048D930[32] = {
#include "sub_main/0048D930.inc"
};
char D_0048D950[32] = {
#include "sub_main/0048D950.inc"
};
char D_0048D970[32] = {
#include "sub_main/0048D970.inc"
};
char gArmorDescTexts_3747[32] = {
#include "sub_main/0048D990.inc"
};
char gArmorDescTexts_3748[32] = {
#include "sub_main/0048D9B0.inc"
};
char gArmorDescTexts_3749[32] = {
#include "sub_main/0048D9D0.inc"
};
char gArmorDescTexts_3751[32] = {
#include "sub_main/0048D9F0.inc"
};
char gArmorDescTexts_3752[32] = {
#include "sub_main/0048DA10.inc"
};
char gArmorDescTexts_3753[32] = {
#include "sub_main/0048DA30.inc"
};
char gArmorDescTexts_3755[32] = {
#include "sub_main/0048DA50.inc"
};
char gArmorDescTexts_3756[32] = {
#include "sub_main/0048DA70.inc"
};
char gArmorDescTexts_3757[32] = {
#include "sub_main/0048DA90.inc"
};
char gArmorDescTexts_3759[32] = {
#include "sub_main/0048DAB0.inc"
};
char gArmorDescTexts_3760[16] = {
#include "sub_main/0048DAD0.inc"
};
char gArmorDescTexts_3761[32] = {
#include "sub_main/0048DAE0.inc"
};
char gArmorDescTexts_3763[32] = {
#include "sub_main/0048DB00.inc"
};
char gArmorDescTexts_3764[32] = {
#include "sub_main/0048DB20.inc"
};
char gArmorDescTexts_3765[32] = {
#include "sub_main/0048DB40.inc"
};
char gArmorDescTexts_3767[32] = {
#include "sub_main/0048DB60.inc"
};
char gArmorDescTexts_3768[32] = {
#include "sub_main/0048DB80.inc"
};
char gArmorDescTexts_3769[32] = {
#include "sub_main/0048DBA0.inc"
};
char gArmorDescTexts_3771[32] = {
#include "sub_main/0048DBC0.inc"
};
char gArmorDescTexts_3772[32] = {
#include "sub_main/0048DBE0.inc"
};
char gArmorDescTexts_3773[32] = {
#include "sub_main/0048DC00.inc"
};
char gArmorDescTexts_3775[32] = {
#include "sub_main/0048DC20.inc"
};
char gArmorDescTexts_3776[32] = {
#include "sub_main/0048DC40.inc"
};
char gArmorDescTexts_3777[32] = {
#include "sub_main/0048DC60.inc"
};
char gArmorDescTexts_3779[32] = {
#include "sub_main/0048DC80.inc"
};
char gArmorDescTexts_3780[32] = {
#include "sub_main/0048DCA0.inc"
};
char gArmorDescTexts_3781[32] = {
#include "sub_main/0048DCC0.inc"
};
char gArmorDescTexts_3783[32] = {
#include "sub_main/0048DCE0.inc"
};
char gArmorDescTexts_3784[32] = {
#include "sub_main/0048DD00.inc"
};
char gArmorDescTexts_3785[32] = {
#include "sub_main/0048DD20.inc"
};
char D_0048DD40[32] = {
#include "sub_main/0048DD40.inc"
};
char D_0048DD60[32] = {
#include "sub_main/0048DD60.inc"
};
char gArmorDescTexts_3789[32] = {
#include "sub_main/0048DD80.inc"
};
char gArmorDescTexts_3793[32] = {
#include "sub_main/0048DDA0.inc"
};
char gArmorDescTexts_3795[32] = {
#include "sub_main/0048DDC0.inc"
};
char gArmorDescTexts_3796[32] = {
#include "sub_main/0048DDE0.inc"
};
char gArmorDescTexts_3797[32] = {
#include "sub_main/0048DE00.inc"
};
char gArmorDescTexts_3799[32] = {
#include "sub_main/0048DE20.inc"
};
char gArmorDescTexts_3800[32] = {
#include "sub_main/0048DE40.inc"
};
char gArmorDescTexts_3801[32] = {
#include "sub_main/0048DE60.inc"
};
char D_0048DE80[32] = {
#include "sub_main/0048DE80.inc"
};
char D_0048DEA0[32] = {
#include "sub_main/0048DEA0.inc"
};
char D_0048DEC0[32] = {
#include "sub_main/0048DEC0.inc"
};
char D_0048DEE0[32] = {
#include "sub_main/0048DEE0.inc"
};
char gArmorDescTexts_3812[32] = {
#include "sub_main/0048DF00.inc"
};
char gArmorDescTexts_3813[32] = {
#include "sub_main/0048DF20.inc"
};
char gArmorDescTexts_3816[32] = {
#include "sub_main/0048DF40.inc"
};
char gArmorDescTexts_3817[32] = {
#include "sub_main/0048DF60.inc"
};
char gArmorDescTexts_3823[32] = {
#include "sub_main/0048DF80.inc"
};
char gArmorDescTexts_3824[32] = {
#include "sub_main/0048DFA0.inc"
};
char gArmorDescTexts_3825[32] = {
#include "sub_main/0048DFC0.inc"
};
char gArmorDescTexts_3827[32] = {
#include "sub_main/0048DFE0.inc"
};
char gArmorDescTexts_3828[32] = {
#include "sub_main/0048E000.inc"
};
char gArmorDescTexts_3829[32] = {
#include "sub_main/0048E020.inc"
};
char gArmorDescTexts_3831[32] = {
#include "sub_main/0048E040.inc"
};
char gArmorDescTexts_3832[32] = {
#include "sub_main/0048E060.inc"
};
char gArmorDescTexts_3833[32] = {
#include "sub_main/0048E080.inc"
};
char gArmorDescTexts_3835[32] = {
#include "sub_main/0048E0A0.inc"
};
char gArmorDescTexts_3836[32] = {
#include "sub_main/0048E0C0.inc"
};
char gArmorDescTexts_3837[32] = {
#include "sub_main/0048E0E0.inc"
};
char gArmorDescTexts_3839[32] = {
#include "sub_main/0048E100.inc"
};
char gArmorDescTexts_3840[32] = {
#include "sub_main/0048E120.inc"
};
char gArmorDescTexts_3841[32] = {
#include "sub_main/0048E140.inc"
};
char gArmorDescTexts_3860[32] = {
#include "sub_main/0048E160.inc"
};
char gArmorDescTexts_3861[32] = {
#include "sub_main/0048E180.inc"
};
char gArmorDescTexts_3864[32] = {
#include "sub_main/0048E1A0.inc"
};
char gArmorDescTexts_3865[32] = {
#include "sub_main/0048E1C0.inc"
};
char D_0048E1E0[32] = {
#include "sub_main/0048E1E0.inc"
};
char gArmorDescTexts_3868[32] = {
#include "sub_main/0048E200.inc"
};
char gArmorDescTexts_3869[32] = {
#include "sub_main/0048E220.inc"
};
char gArmorDescTexts_3872[32] = {
#include "sub_main/0048E240.inc"
};
char gArmorDescTexts_3873[32] = {
#include "sub_main/0048E260.inc"
};
char gArmorDescTexts_3875[32] = {
#include "sub_main/0048E280.inc"
};
char gArmorDescTexts_3876[32] = {
#include "sub_main/0048E2A0.inc"
};
char gArmorDescTexts_3877[32] = {
#include "sub_main/0048E2C0.inc"
};
char D_0048E2E0[32] = {
#include "sub_main/0048E2E0.inc"
};
char D_0048E300[16] = {
#include "sub_main/0048E300.inc"
};
char D_0048E310[32] = {
#include "sub_main/0048E310.inc"
};
char D_0048E330[32] = {
#include "sub_main/0048E330.inc"
};
char D_0048E350[32] = {
#include "sub_main/0048E350.inc"
};
char D_0048E370[32] = {
#include "sub_main/0048E370.inc"
};
char D_0048E390[32] = {
#include "sub_main/0048E390.inc"
};
char D_0048E3B0[32] = {
#include "sub_main/0048E3B0.inc"
};
char D_0048E3D0[16] = {
#include "sub_main/0048E3D0.inc"
};
char D_0048E3E0[32] = {
#include "sub_main/0048E3E0.inc"
};
char D_0048E400[32] = {
#include "sub_main/0048E400.inc"
};
char D_0048E420[32] = {
#include "sub_main/0048E420.inc"
};
char D_0048E440[32] = {
#include "sub_main/0048E440.inc"
};
char D_0048E460[32] = {
#include "sub_main/0048E460.inc"
};
char D_0048E480[32] = {
#include "sub_main/0048E480.inc"
};
char D_0048E4A0[16] = {
#include "sub_main/0048E4A0.inc"
};
char D_0048E4B0[32] = {
#include "sub_main/0048E4B0.inc"
};
char D_0048E4D0[32] = {
#include "sub_main/0048E4D0.inc"
};
char D_0048E4F0[32] = {
#include "sub_main/0048E4F0.inc"
};
char D_0048E510[32] = {
#include "sub_main/0048E510.inc"
};
char D_0048E530[32] = {
#include "sub_main/0048E530.inc"
};
char D_0048E550[32] = {
#include "sub_main/0048E550.inc"
};
char D_0048E570[32] = {
#include "sub_main/0048E570.inc"
};
char D_0048E590[32] = {
#include "sub_main/0048E590.inc"
};
char D_0048E5B0[32] = {
#include "sub_main/0048E5B0.inc"
};
char D_0048E5D0[32] = {
#include "sub_main/0048E5D0.inc"
};
char gArmorDescTexts_3923[32] = {
#include "sub_main/0048E5F0.inc"
};
char gArmorDescTexts_3924[32] = {
#include "sub_main/0048E610.inc"
};
char gArmorDescTexts_3925[32] = {
#include "sub_main/0048E630.inc"
};
char D_0048E650[32] = {
#include "sub_main/0048E650.inc"
};
char D_0048E670[32] = {
#include "sub_main/0048E670.inc"
};
char D_0048E690[32] = {
#include "sub_main/0048E690.inc"
};
char D_0048E6B0[32] = {
#include "sub_main/0048E6B0.inc"
};
char D_0048E6D0[16] = {
#include "sub_main/0048E6D0.inc"
};
char D_0048E6E0[32] = {
#include "sub_main/0048E6E0.inc"
};
char D_0048E700[32] = {
#include "sub_main/0048E700.inc"
};
char D_0048E720[32] = {
#include "sub_main/0048E720.inc"
};
char D_0048E740[32] = {
#include "sub_main/0048E740.inc"
};
char D_0048E760[32] = {
#include "sub_main/0048E760.inc"
};
char D_0048E780[32] = {
#include "sub_main/0048E780.inc"
};
char D_0048E7A0[24] = {
#include "sub_main/0048E7A0.inc"
};
char D_0048E7B8[15] = {
#include "sub_main/0048E7B8.inc"
};
char D_0048E7D0[32] = {
#include "sub_main/0048E7D0.inc"
};
char D_0048E7F0[32] = {
#include "sub_main/0048E7F0.inc"
};
char gArmorDescTexts_3951[32] = {
#include "sub_main/0048E810.inc"
};
char gArmorDescTexts_3952[32] = {
#include "sub_main/0048E830.inc"
};
char gArmorDescTexts_3953[32] = {
#include "sub_main/0048E850.inc"
};
char D_0048E870[32] = {
#include "sub_main/0048E870.inc"
};
char D_0048E890[32] = {
#include "sub_main/0048E890.inc"
};
char D_0048E8B0[32] = {
#include "sub_main/0048E8B0.inc"
};
char D_0048E8D0[32] = {
#include "sub_main/0048E8D0.inc"
};
char D_0048E8F0[32] = {
#include "sub_main/0048E8F0.inc"
};
char D_0048E910[32] = {
#include "sub_main/0048E910.inc"
};
char gArmorDescTexts_3963[32] = {
#include "sub_main/0048E930.inc"
};
char gArmorDescTexts_3964[32] = {
#include "sub_main/0048E950.inc"
};
char gArmorDescTexts_3965[32] = {
#include "sub_main/0048E970.inc"
};
char gArmorDescTexts_3968[32] = {
#include "sub_main/0048E990.inc"
};
char gArmorDescTexts_3969[32] = {
#include "sub_main/0048E9B0.inc"
};
char D_0048E9D0[32] = {
#include "sub_main/0048E9D0.inc"
};
char D_0048E9F0[32] = {
#include "sub_main/0048E9F0.inc"
};
char D_0048EA10[32] = {
#include "sub_main/0048EA10.inc"
};
char D_0048EA30[32] = {
#include "sub_main/0048EA30.inc"
};
char D_0048EA50[32] = {
#include "sub_main/0048EA50.inc"
};
char D_0048EA70[32] = {
#include "sub_main/0048EA70.inc"
};
char D_0048EA90[32] = {
#include "sub_main/0048EA90.inc"
};
char D_0048EAB0[32] = {
#include "sub_main/0048EAB0.inc"
};
char D_0048EAD0[32] = {
#include "sub_main/0048EAD0.inc"
};
char D_0048EAF0[32] = {
#include "sub_main/0048EAF0.inc"
};
char D_0048EB10[32] = {
#include "sub_main/0048EB10.inc"
};
char D_0048EB30[32] = {
#include "sub_main/0048EB30.inc"
};
char D_0048EB50[32] = {
#include "sub_main/0048EB50.inc"
};
char D_0048EB70[32] = {
#include "sub_main/0048EB70.inc"
};
char D_0048EB90[32] = {
#include "sub_main/0048EB90.inc"
};
char D_0048EBB0[32] = {
#include "sub_main/0048EBB0.inc"
};
char D_0048EBD0[32] = {
#include "sub_main/0048EBD0.inc"
};
char D_0048EBF0[32] = {
#include "sub_main/0048EBF0.inc"
};
char D_0048EC10[32] = {
#include "sub_main/0048EC10.inc"
};
char D_0048EC30[32] = {
#include "sub_main/0048EC30.inc"
};
char D_0048EC50[32] = {
#include "sub_main/0048EC50.inc"
};
char D_0048EC70[32] = {
#include "sub_main/0048EC70.inc"
};
char D_0048EC90[32] = {
#include "sub_main/0048EC90.inc"
};
char D_0048ECB0[32] = {
#include "sub_main/0048ECB0.inc"
};
char D_0048ECD0[32] = {
#include "sub_main/0048ECD0.inc"
};
char D_0048ECF0[32] = {
#include "sub_main/0048ECF0.inc"
};
char D_0048ED10[32] = {
#include "sub_main/0048ED10.inc"
};
char D_0048ED30[32] = {
#include "sub_main/0048ED30.inc"
};
char D_0048ED50[32] = {
#include "sub_main/0048ED50.inc"
};
char D_0048ED70[32] = {
#include "sub_main/0048ED70.inc"
};
char D_0048ED90[32] = {
#include "sub_main/0048ED90.inc"
};
char D_0048EDB0[32] = {
#include "sub_main/0048EDB0.inc"
};
char D_0048EDD0[32] = {
#include "sub_main/0048EDD0.inc"
};
char D_0048EDF0[32] = {
#include "sub_main/0048EDF0.inc"
};
char D_0048EE10[32] = {
#include "sub_main/0048EE10.inc"
};
char D_0048EE30[32] = {
#include "sub_main/0048EE30.inc"
};
char D_0048EE50[32] = {
#include "sub_main/0048EE50.inc"
};
char D_0048EE70[32] = {
#include "sub_main/0048EE70.inc"
};
char D_0048EE90[32] = {
#include "sub_main/0048EE90.inc"
};
char D_0048EEB0[32] = {
#include "sub_main/0048EEB0.inc"
};
char D_0048EED0[32] = {
#include "sub_main/0048EED0.inc"
};
char D_0048EEF0[32] = {
#include "sub_main/0048EEF0.inc"
};
char D_0048EF10[32] = {
#include "sub_main/0048EF10.inc"
};
char D_0048EF30[32] = {
#include "sub_main/0048EF30.inc"
};
char D_0048EF50[32] = {
#include "sub_main/0048EF50.inc"
};
char D_0048EF70[32] = {
#include "sub_main/0048EF70.inc"
};
char D_0048EF90[32] = {
#include "sub_main/0048EF90.inc"
};
char D_0048EFB0[32] = {
#include "sub_main/0048EFB0.inc"
};
char D_0048EFD0[32] = {
#include "sub_main/0048EFD0.inc"
};
char D_0048EFF0[16] = {
#include "sub_main/0048EFF0.inc"
};
char D_0048F000[16] = {
#include "sub_main/0048F000.inc"
};
char D_0048F010[32] = {
#include "sub_main/0048F010.inc"
};
char gArmorDescTexts_4063[32] = {
#include "sub_main/0048F030.inc"
};
char gArmorDescTexts_4064[32] = {
#include "sub_main/0048F050.inc"
};
char gArmorDescTexts_4065[16] = {
#include "sub_main/0048F070.inc"
};
char D_0048F080[16] = {
#include "sub_main/0048F080.inc"
};
char D_0048F090[32] = {
#include "sub_main/0048F090.inc"
};
char gArmorDescTexts_4072[32] = {
#include "sub_main/0048F0B0.inc"
};
char gArmorDescTexts_4073[32] = {
#include "sub_main/0048F0D0.inc"
};
char gArmorDescTexts_4076[32] = {
#include "sub_main/0048F0F0.inc"
};
char gArmorDescTexts_4077[32] = {
#include "sub_main/0048F110.inc"
};
char D_0048F130[32] = {
#include "sub_main/0048F130.inc"
};
char gArmorDescTexts_4088[32] = {
#include "sub_main/0048F150.inc"
};
char gArmorDescTexts_4089[32] = {
#include "sub_main/0048F170.inc"
};
char gArmorDescTexts_4092[32] = {
#include "sub_main/0048F190.inc"
};
char gArmorDescTexts_4093[32] = {
#include "sub_main/0048F1B0.inc"
};
char gArmorDescTexts_4096[32] = {
#include "sub_main/0048F1D0.inc"
};
char gArmorDescTexts_4097[32] = {
#include "sub_main/0048F1F0.inc"
};
char gArmorDescTexts_4100[32] = {
#include "sub_main/0048F210.inc"
};
char gArmorDescTexts_4101[32] = {
#include "sub_main/0048F230.inc"
};
char gArmorDescTexts_4103[32] = {
#include "sub_main/0048F250.inc"
};
char gArmorDescTexts_4104[32] = {
#include "sub_main/0048F270.inc"
};
char gArmorDescTexts_4105[32] = {
#include "sub_main/0048F290.inc"
};
char gArmorDescTexts_4107[32] = {
#include "sub_main/0048F2B0.inc"
};
char gArmorDescTexts_4108[32] = {
#include "sub_main/0048F2D0.inc"
};
char gArmorDescTexts_4109[32] = {
#include "sub_main/0048F2F0.inc"
};
char D_0048F310[32] = {
#include "sub_main/0048F310.inc"
};
char D_0048F330[32] = {
#include "sub_main/0048F330.inc"
};
char D_0048F350[32] = {
#include "sub_main/0048F350.inc"
};
char D_0048F370[32] = {
#include "sub_main/0048F370.inc"
};
char D_0048F390[32] = {
#include "sub_main/0048F390.inc"
};
char D_0048F3B0[32] = {
#include "sub_main/0048F3B0.inc"
};
char D_0048F3D0[32] = {
#include "sub_main/0048F3D0.inc"
};
char D_0048F3F0[32] = {
#include "sub_main/0048F3F0.inc"
};
char gArmorDescTexts_4127[32] = {
#include "sub_main/0048F410.inc"
};
char D_0048F430[32] = {
#include "sub_main/0048F430.inc"
};
char D_0048F450[32] = {
#include "sub_main/0048F450.inc"
};
char gArmorDescTexts_4131[32] = {
#include "sub_main/0048F470.inc"
};
char D_0048F490[32] = {
#include "sub_main/0048F490.inc"
};
char D_0048F4B0[32] = {
#include "sub_main/0048F4B0.inc"
};
char D_0048F4D0[32] = {
#include "sub_main/0048F4D0.inc"
};
char D_0048F4F0[32] = {
#include "sub_main/0048F4F0.inc"
};
char D_0048F510[32] = {
#include "sub_main/0048F510.inc"
};
char D_0048F530[32] = {
#include "sub_main/0048F530.inc"
};
char gArmorDescTexts_4215[32] = {
#include "sub_main/0048F550.inc"
};
char gArmorDescTexts_4239[32] = {
#include "sub_main/0048F570.inc"
};
char gArmorDescTexts_4240[32] = {
#include "sub_main/0048F590.inc"
};
char gArmorDescTexts_4241[32] = {
#include "sub_main/0048F5B0.inc"
};
char gArmorDescTexts_4243[32] = {
#include "sub_main/0048F5D0.inc"
};
char gArmorDescTexts_4244[32] = {
#include "sub_main/0048F5F0.inc"
};
char gArmorDescTexts_4245[32] = {
#include "sub_main/0048F610.inc"
};
char gArmorDescTexts_4255[32] = {
#include "sub_main/0048F630.inc"
};
char gArmorDescTexts_4256[32] = {
#include "sub_main/0048F650.inc"
};
char gArmorDescTexts_4257[32] = {
#include "sub_main/0048F670.inc"
};
char gArmorDescTexts_4259[32] = {
#include "sub_main/0048F690.inc"
};
char gArmorDescTexts_4260[32] = {
#include "sub_main/0048F6B0.inc"
};
char gArmorDescTexts_4261[32] = {
#include "sub_main/0048F6D0.inc"
};
char gArmorDescTexts_4305[32] = {
#include "sub_main/0048F6F0.inc"
};
char gArmorDescTexts_4308[32] = {
#include "sub_main/0048F710.inc"
};
char gArmorDescTexts_4309[16] = {
#include "sub_main/0048F730.inc"
};
char D_0048F740[32] = {
#include "sub_main/0048F740.inc"
};
char gArmorDescTexts_4312[32] = {
#include "sub_main/0048F760.inc"
};
char gArmorDescTexts_4313[32] = {
#include "sub_main/0048F780.inc"
};
char gArmorDescTexts_4316[32] = {
#include "sub_main/0048F7A0.inc"
};
char gArmorDescTexts_4317[32] = {
#include "sub_main/0048F7C0.inc"
};
char gArmorDescTexts_4320[32] = {
#include "sub_main/0048F7E0.inc"
};
char gArmorDescTexts_4321[32] = {
#include "sub_main/0048F800.inc"
};
char gArmorDescTexts_4324[32] = {
#include "sub_main/0048F820.inc"
};
char gArmorDescTexts_4325[32] = {
#include "sub_main/0048F840.inc"
};
char gArmorDescTexts_4343[32] = {
#include "sub_main/0048F860.inc"
};
char gArmorDescTexts_4344[32] = {
#include "sub_main/0048F880.inc"
};
char gArmorDescTexts_4345[32] = {
#include "sub_main/0048F8A0.inc"
};
char gArmorDescTexts_4347[32] = {
#include "sub_main/0048F8C0.inc"
};
char gArmorDescTexts_4348[32] = {
#include "sub_main/0048F8E0.inc"
};
char gArmorDescTexts_4349[32] = {
#include "sub_main/0048F900.inc"
};
char gArmorDescTexts_4351[32] = {
#include "sub_main/0048F920.inc"
};
char gArmorDescTexts_4352[32] = {
#include "sub_main/0048F940.inc"
};
char gArmorDescTexts_4353[32] = {
#include "sub_main/0048F960.inc"
};
char gArmorDescTexts_4355[32] = {
#include "sub_main/0048F980.inc"
};
char gArmorDescTexts_4356[32] = {
#include "sub_main/0048F9A0.inc"
};
char gArmorDescTexts_4357[32] = {
#include "sub_main/0048F9C0.inc"
};
char gArmorDescTexts_4359[32] = {
#include "sub_main/0048F9E0.inc"
};
char gArmorDescTexts_4360[32] = {
#include "sub_main/0048FA00.inc"
};
char gArmorDescTexts_4361[32] = {
#include "sub_main/0048FA20.inc"
};
char gArmorDescTexts_4363[32] = {
#include "sub_main/0048FA40.inc"
};
char gArmorDescTexts_4364[32] = {
#include "sub_main/0048FA60.inc"
};
char gArmorDescTexts_4365[32] = {
#include "sub_main/0048FA80.inc"
};
char gArmorDescTexts_4439[32] = {
#include "sub_main/0048FAA0.inc"
};
char gArmorDescTexts_4440[32] = {
#include "sub_main/0048FAC0.inc"
};
char gArmorDescTexts_4441[32] = {
#include "sub_main/0048FAE0.inc"
};
char gArmorDescTexts_4443[32] = {
#include "sub_main/0048FB00.inc"
};
char gArmorDescTexts_4444[32] = {
#include "sub_main/0048FB20.inc"
};
char gArmorDescTexts_4445[32] = {
#include "sub_main/0048FB40.inc"
};
char D_0048FB60[32] = {
#include "sub_main/0048FB60.inc"
};
char D_0048FB80[32] = {
#include "sub_main/0048FB80.inc"
};
char D_0048FBA0[32] = {
#include "sub_main/0048FBA0.inc"
};
char D_0048FBC0[32] = {
#include "sub_main/0048FBC0.inc"
};
char D_0048FBE0[32] = {
#include "sub_main/0048FBE0.inc"
};
char D_0048FC00[32] = {
#include "sub_main/0048FC00.inc"
};
char D_0048FC20[32] = {
#include "sub_main/0048FC20.inc"
};
char D_0048FC40[32] = {
#include "sub_main/0048FC40.inc"
};
char D_0048FC60[32] = {
#include "sub_main/0048FC60.inc"
};
char D_0048FC80[32] = {
#include "sub_main/0048FC80.inc"
};
char D_0048FCA0[32] = {
#include "sub_main/0048FCA0.inc"
};
char D_0048FCC0[32] = {
#include "sub_main/0048FCC0.inc"
};
char D_0048FCE0[32] = {
#include "sub_main/0048FCE0.inc"
};
char D_0048FD00[32] = {
#include "sub_main/0048FD00.inc"
};
char D_0048FD20[32] = {
#include "sub_main/0048FD20.inc"
};
char D_0048FD40[32] = {
#include "sub_main/0048FD40.inc"
};
char D_0048FD60[32] = {
#include "sub_main/0048FD60.inc"
};
char D_0048FD80[32] = {
#include "sub_main/0048FD80.inc"
};
char D_0048FDA0[32] = {
#include "sub_main/0048FDA0.inc"
};
char D_0048FDC0[32] = {
#include "sub_main/0048FDC0.inc"
};
char D_0048FDE0[32] = {
#include "sub_main/0048FDE0.inc"
};
char D_0048FE00[32] = {
#include "sub_main/0048FE00.inc"
};
char D_0048FE20[32] = {
#include "sub_main/0048FE20.inc"
};
char D_0048FE40[32] = {
#include "sub_main/0048FE40.inc"
};
char D_0048FE60[32] = {
#include "sub_main/0048FE60.inc"
};
char D_0048FE80[32] = {
#include "sub_main/0048FE80.inc"
};
char D_0048FEA0[32] = {
#include "sub_main/0048FEA0.inc"
};
char D_0048FEC0[32] = {
#include "sub_main/0048FEC0.inc"
};
char D_0048FEE0[32] = {
#include "sub_main/0048FEE0.inc"
};
char D_0048FF00[32] = {
#include "sub_main/0048FF00.inc"
};
char D_0048FF20[32] = {
#include "sub_main/0048FF20.inc"
};
char D_0048FF40[32] = {
#include "sub_main/0048FF40.inc"
};
char D_0048FF60[32] = {
#include "sub_main/0048FF60.inc"
};
char D_0048FF80[32] = {
#include "sub_main/0048FF80.inc"
};
char D_0048FFA0[32] = {
#include "sub_main/0048FFA0.inc"
};
char D_0048FFC0[32] = {
#include "sub_main/0048FFC0.inc"
};
char D_0048FFE0[32] = {
#include "sub_main/0048FFE0.inc"
};
char D_00490000[32] = {
#include "sub_main/00490000.inc"
};
char gArmorDescTexts_4696[32] = {
#include "sub_main/00490020.inc"
};
char gArmorDescTexts_4697[8] = {
#include "sub_main/00490040.inc"
};
char D_00490048[12] = {
#include "sub_main/00490048.inc"
};
char gArmorDescTexts_4699[32] = {
#include "sub_main/00490060.inc"
};
char gArmorDescTexts_4700[32] = {
#include "sub_main/00490080.inc"
};
char gArmorDescTexts_4701[32] = {
#include "sub_main/004900A0.inc"
};
char gArmorDescTexts_4703[32] = {
#include "sub_main/004900C0.inc"
};
char gArmorDescTexts_4704[32] = {
#include "sub_main/004900E0.inc"
};
char gArmorDescTexts_4705[32] = {
#include "sub_main/00490100.inc"
};
char gArmorDescTexts_4707[32] = {
#include "sub_main/00490120.inc"
};
char gArmorDescTexts_4708[32] = {
#include "sub_main/00490140.inc"
};
char gArmorDescTexts_4709[32] = {
#include "sub_main/00490160.inc"
};
char gArmorDescTexts_4711[32] = {
#include "sub_main/00490180.inc"
};
char gArmorDescTexts_4712[32] = {
#include "sub_main/004901A0.inc"
};
char gArmorDescTexts_4713[32] = {
#include "sub_main/004901C0.inc"
};
char gArmorDescTexts_4715[32] = {
#include "sub_main/004901E0.inc"
};
char D_00490200[32] = {
#include "sub_main/00490200.inc"
};
char D_00490220[32] = {
#include "sub_main/00490220.inc"
};
char gArmorDescTexts_4719[32] = {
#include "sub_main/00490240.inc"
};
char gArmorDescTexts_4720[32] = {
#include "sub_main/00490260.inc"
};
char gArmorDescTexts_4721[32] = {
#include "sub_main/00490280.inc"
};
char gArmorDescTexts_4723[32] = {
#include "sub_main/004902A0.inc"
};
char gArmorDescTexts_4724[32] = {
#include "sub_main/004902C0.inc"
};
char gArmorDescTexts_4725[32] = {
#include "sub_main/004902E0.inc"
};
char gArmorDescTexts_4727[32] = {
#include "sub_main/00490300.inc"
};
char gArmorDescTexts_4728[32] = {
#include "sub_main/00490320.inc"
};
char gArmorDescTexts_4729[32] = {
#include "sub_main/00490340.inc"
};
char gArmorDescTexts_4731[32] = {
#include "sub_main/00490360.inc"
};
char D_00490380[32] = {
#include "sub_main/00490380.inc"
};
char gArmorDescTexts_4736[32] = {
#include "sub_main/004903A0.inc"
};
char gArmorDescTexts_4737[32] = {
#include "sub_main/004903C0.inc"
};
char gArmorDescTexts_4740[32] = {
#include "sub_main/004903E0.inc"
};
char gArmorDescTexts_4741[32] = {
#include "sub_main/00490400.inc"
};
char gArmorDescTexts_4743[32] = {
#include "sub_main/00490420.inc"
};
char gArmorDescTexts_4744[32] = {
#include "sub_main/00490440.inc"
};
char gArmorDescTexts_4745[32] = {
#include "sub_main/00490460.inc"
};
char gArmorDescTexts_4747[32] = {
#include "sub_main/00490480.inc"
};
char gArmorDescTexts_4748[32] = {
#include "sub_main/004904A0.inc"
};
char gArmorDescTexts_4749[32] = {
#include "sub_main/004904C0.inc"
};
char D_004904E0[32] = {
#include "sub_main/004904E0.inc"
};
char D_00490500[24] = {
#include "sub_main/00490500.inc"
};
char D_00490518[12] = {
#include "sub_main/00490518.inc"
};
char gArmorDescTexts_4767[32] = {
#include "sub_main/00490530.inc"
};
char gArmorDescTexts_4768[32] = {
#include "sub_main/00490550.inc"
};
char gArmorDescTexts_4769[16] = {
#include "sub_main/00490570.inc"
};
char gArmorDescTexts_4771[32] = {
#include "sub_main/00490580.inc"
};
char gArmorDescTexts_4772[32] = {
#include "sub_main/004905A0.inc"
};
char gArmorDescTexts_4773[32] = {
#include "sub_main/004905C0.inc"
};
char gArmorDescTexts_4775[32] = {
#include "sub_main/004905E0.inc"
};
char gArmorDescTexts_4776[32] = {
#include "sub_main/00490600.inc"
};
char gArmorDescTexts_4777[32] = {
#include "sub_main/00490620.inc"
};
char gArmorDescTexts_4779[32] = {
#include "sub_main/00490640.inc"
};
char gArmorDescTexts_4780[32] = {
#include "sub_main/00490660.inc"
};
char gArmorDescTexts_4781[32] = {
#include "sub_main/00490680.inc"
};
char gArmorDescTexts_4783[32] = {
#include "sub_main/004906A0.inc"
};
char gArmorDescTexts_4784[32] = {
#include "sub_main/004906C0.inc"
};
char gArmorDescTexts_4785[32] = {
#include "sub_main/004906E0.inc"
};
char gArmorDescTexts_4803[32] = {
#include "sub_main/00490700.inc"
};
char gArmorDescTexts_4804[32] = {
#include "sub_main/00490720.inc"
};
char gArmorDescTexts_4805[32] = {
#include "sub_main/00490740.inc"
};
char gArmorDescTexts_4808[32] = {
#include "sub_main/00490760.inc"
};
char gArmorDescTexts_4809[32] = {
#include "sub_main/00490780.inc"
};
char gArmorDescTexts_4811[32] = {
#include "sub_main/004907A0.inc"
};
char D_004907C0[32] = {
#include "sub_main/004907C0.inc"
};
char gArmorDescTexts_4813[32] = {
#include "sub_main/004907E0.inc"
};
char gArmorDescTexts_4815[32] = {
#include "sub_main/00490800.inc"
};
char gArmorDescTexts_4817[32] = {
#include "sub_main/00490820.inc"
};
char gArmorDescTexts_4819[32] = {
#include "sub_main/00490840.inc"
};
char gArmorDescTexts_4820[32] = {
#include "sub_main/00490860.inc"
};
char gArmorDescTexts_4821[32] = {
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

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
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
        *(u32 *)&piece.u0 = TEX_UV(236, 188);
        *(u32 *)&piece.u1 = TEX_UV(253, 205);
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

/* Draws a window's two side pieces in a fixed colour (0xF020DF20; the green byte is worked
 * out from a float local, as the match needs). */
void Menu_DrawFrame(void *win, s16 *pos)
{
    f32 alpha = 48.0f;

    Window_DrawSides(win, pos, (((s8)alpha + 175) << 8) | 0xF0200020);
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
    *(u32 *)&piece.u0 = TEX_UV(113, 110);
    *(u32 *)&piece.u1 = TEX_UV(125, 125);
    piece.x = x - blink + 1;
    piece.y = y;
    piece.color = color & 0xFF000000;
    Sprite_Draw(&piece);
    piece.x -= 2;
    piece.y -= 1;
    piece.color = color;
    Sprite_Draw(&piece);
    piece.u0 = 124;
    piece.u1 = 112;
    piece.x = blink + (x + win->charW * win->lineChars) - win->charW + 1;
    piece.y = y;
    piece.color = color & 0xFF000000;
    Sprite_Draw(&piece);
    piece.x -= 2;
    piece.y -= 1;
    piece.color = color;
    Sprite_Draw(&piece);
}


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

/* ListPanel_DrawAlpha with alpha 178. */
void ListPanel_DrawDim(struct ListWin *win, char *title, s32 sel)
{
    ListPanel_DrawAlpha(win, title, sel, 178);
}


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

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
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
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
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
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        Font_SetSpacing(win->charW, win->charH);
        Text_SetColor(win->color);
        Font_SetPos(win->x, win->y);
        Text_Printf(title);
    }
}

/* Window_DrawTextAlpha with alpha 178. */
void Window_DrawTextB2(Window *w, u8 *text)
{
    Window_DrawTextAlpha(w, text, 178);
}


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
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
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

/* Draws a menu arrow (texels 148-166, 26-46 of the bound texture) in the box x, y, w, h.
 * flags: ARROW_MIRROR points it the other way, ARROW_TEXRECT draws it with TexRect_Draw's
 * mode 2 instead of as a sprite. */
void Menu_DrawArrow(s16 x, s16 y, s16 width, s16 height, u32 color, u8 flags)
{
    Sprite arrow;

    arrow.v0 = 26;
    arrow.y = y;
    arrow.v1 = 46;
    arrow.h = height;
    arrow.color = color;
    if (flags & ARROW_MIRROR) {
        arrow.u0 = 166;
        arrow.u1 = 148;
    } else {
        arrow.u0 = 148;
        arrow.u1 = 166;
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

/* Draws the chat window's arrow (the side piece of Window_DrawSides, texels 113-125,
 * 110-125) in the box x, y, w, h; flags as for Menu_DrawArrow. The lobby draws its
 * scroll arrows with it. */
void Chat_DrawArrow(s16 x, s16 y, s16 width, s16 height, u32 color, u8 flags)
{
    Sprite arrow;

    arrow.v0 = 110;
    arrow.y = y;
    arrow.v1 = 125;
    arrow.h = height;
    arrow.color = color;
    if (flags & ARROW_MIRROR) {
        arrow.u0 = 113;
        arrow.u1 = 125;
    } else {
        arrow.u0 = 125;
        arrow.u1 = 113;
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
    if (x < -28 || x > 668) {
        return;
    }
    y = screen[1];
    if (y < -20 || y > 468) {
        return;
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    spr.color = 0xE4FF0000;
    spr.x = x - 1;
    spr.y = y - 1;
    spr.w = 3;
    spr.h = 3;
    *(u32 *)&spr.u0 = TEX_UV(112, 146);
    *(u32 *)&spr.u1 = TEX_UV(114, 148);
    Sprite_Draw(&spr);
    *(u32 *)&spr.u0 = TEX_UV(109, 143);
    *(u32 *)&spr.u1 = TEX_UV(192, 154);
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

/* Sets up a grid over the items Recipe_IsListedFor lists for filter (items 1 to 920), for
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
    for (item = 1, listed = 0; item < 921; item++) {
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
 * cell (255), the stand-in's and 0; else 1. */
s32 EntryPicture_GetUVf(f32 *uv0, f32 *uv1, u16 id)
{
    u8 cell;
    s32 x;
    s32 y;

    cell = gEntryPictureIndex[id];
    if (cell == 255) {
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
    if (cell == 255) {
        *uv0 = TEX_UV(43, 211);
        *uv1 = TEX_UV(83, 251);
        return 0;
    }
    texU = ((s32) cell % 6) * 42;
    texV = ((*entry / 6) + (cell >> 31)) * 42;
    *uv0 = ((texV + 1) << 16) + (texU + 1);
    *uv1 = ((texV + 41) << 16) + (texU + 41);
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
    if (cell == 255) {
        *uv0 = TEX_UV(43, 211);
        *uv1 = TEX_UV(83, 251);
        return 0;
    }
    texU = ((s32) cell % 6) * 42;
    texV = ((*entry / 6) + (cell >> 31)) * 42;
    *uv0 = ((texV + 1) << 16) + (texU + 1);
    *uv1 = ((texV + 41) << 16) + (texU + 41);
    return 1;
}

extern u8 gHelpPage;
extern u16 gHelpLine;
extern s8 gHelpColumn;

/* Draws the help window at the page, line and column it is on. */
void HelpWindow_DrawCurrent(void)
{
    HelpWindow_Draw(gHelpPage, gHelpLine, gHelpColumn);
}

/* Draws the help window's page and line without a column cursor. */
void HelpWindow_DrawLine(u8 page, u16 line)
{
    HelpWindow_Draw(page, line, -1);
}

extern Panel gHelpWindowBox;
extern char **gHelpTexts[];
extern u32 gItemColors[];

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
            Deco_DrawDetails(item, 360, 359, column, 0);
            data = &gItemData[item];
            Tex_Prepare(1, 324);
            Tex_Bind(324);
            spr.x = 306;
            spr.y = 358;
            spr.w = 50;
            spr.h = 40;
            spr.u0 = ((data->icon + 1) & 7) * 32 + 1;
            spr.v0 = ((data->icon + 1) >> 3) * 32 + 1;
            spr.u1 = spr.u0 + 30;
            spr.v1 = spr.v0 + 30;
            spr.color = gItemColors[data->color];
            Sprite_Draw(&spr);
            if (data->flags2 & 1) {
                *(u32 *)&spr.u0 = TEX_UV(128, 224);
                *(u32 *)&spr.u1 = TEX_UV(160, 256);
                spr.color = 0xFFFFFFFF;
                Sprite_Draw(&spr);
            }
            Font_SetSpacing(16, 16);
            Font_SetPos(306, 402);
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
extern s16 gDecoDetailPos[4][2];

/* Draws one page of an item's decoration details at x, y, in the colours
 * gDecoDetailColors[pal]. view & 3 is the page: 0 the item's description, 1 who can use it
 * (the hunter class from DecoData.flags 4 and 8, the range class from 1 and 2) and its size,
 * 2 its skills and points (skill trees 41-43 show no points). view & 0x10 draws the page
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
            Font_SetPos(x + 171, y + 41);
            Text_Printf(gDecoModeFmt[0], page + 1);
            gDecoDetailIcon.x = x + 147;
            gDecoDetailIcon.y = y + 40;
            Draw_Icons(&gDecoDetailIcon, 1);
        } else {
            Hud_DrawTab(x - 52, y - 59, (page | 0x40) + 1, 3, colors[0]);
        }
        Font_SetSpacing(15, 18);
    }
    switch (page) {
    case 0:
        Font_SetSpacing(18, 18);
        Font_SetPos(x, y);
        Font_SetColor(colors[0]);
        Text_Printf(gHelpTexts[HELP_PAGE_ITEMS][HELP_ITEM_LINE(item)]);
        break;
    case 1:
        Font_SetColor(colors[1]);
        Text_DrawLinesColored(x, y, gDecoDetailLabels, 20);
        Font_SetColor(colors[2]);
        textX = x + 105;
        textY = y;
        Font_SetPos(textX, textY);
        if ((record->flags & 0xC) == 12) {
            kind = 2;
        } else if (record->flags & 4) {
            kind = 0;
        } else if (record->flags & 8) {
            kind = 1;
        } else {
            kind = 3;
        }
        Text_Draw(gHunterClassTexts[kind]);
        Font_SetPos(textX, textY + 19);
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
        Font_SetPos(textX, textY + 38);
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
            if (skill[0] != 41 && skill[0] != 42 && skill[0] != 43) {
                Font_SetPos(left + pos[0] + 75, top + pos[1]);
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
                    Sound_PlaySeFlat(6, 29, gSeBankByType[EM_KIND_TRADER]);
                } else {
                    Sound_PlaySeFlat(1, 132, 0);
                }
            }
            gNoticeShownChars = chars;
            return 1;
        }
        budget -= cost;
    }
    if (chars > gNoticeShownChars) {
        if (gScene.inLobby == 0) {
            Sound_PlaySeFlat(6, 29, gSeBankByType[EM_KIND_TRADER]);
        } else {
            Sound_PlaySeFlat(1, 132, 0);
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
                Sound_PlaySeFlat(6, 31, gSeBankByType[EM_KIND_TRADER]);
            } else {
                Sound_PlaySeFlat(1, 134, 0);
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

/* Runs the chat keyboard for a frame (Kbd_Run): its result, or 255 (cancelled) with an
 * empty line when SELECT is pressed. */
u8 Chat_RunKbd(char *out, s32 held, s16 pressed)
{
    if (pressed & PAD_SELECT) {
        *out = 0;
        return 255;
    }
    return Kbd_Run(out, held, pressed);
}

/* unit's own declaration: declared here as Chat_OpenKbd calls it; with the definition's void (s32 kind, u8 layout, u16 max, char *text), the call compiles differently */
void Kbd_Open(u8 a, s32 b, u16 c, char *text);

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
 * is 0; else the chat window's choice (gChatWindowSel), or CHAT_SEND_LOBBY in AREA_ONLINE_VILLAGE. */
void Chat_PickSendTarget(void)
{
    if (Game_IsOnline() == 1) {
        if (gScene.inLobby != 0) {
            if (gScene.area == AREA_ONLINE_VILLAGE) {
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

/* Opens the chat input line on the software keyboard, sized for the target's window (44
 * characters, the lobby's 60), and runs its first frame. active 0 opens it without the
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
        maxChars = 44;
        break;
    case CHAT_SEND_LOBBY:
        gChatLog.win.open = gChatLog.win.timer = 0;
        maxChars = 60;
        break;
    }
    if (active == 0) {
        flags |= CHAT_KBD_INACTIVE;
    }
    Kbd_SetPos(75, 80.0f);
    Chat_OpenKbd(flags, 14, maxChars, 0);
    Kbd_SetSubmitWait(gChatSendWait);
    gChatInputMode++;
    Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
    Chat_SendInput(0);
}

char *strcpy(char *dst, char *src);
void func_792A50(u8 who, s8 *msg, s32 c);
void Chat_SendToTargets(u8 who, s8 *msg, u8 to); /* unit's own declaration: the definition (lobby) is void Chat_SendToTargets(s32 arg0, void *arg1, s32 arg2); the calls here match only with this one */
void QSync_SendChat(u8 who, s32 a, s8 *msg, u8 to);
extern s8 gChatSystemRecord[];
extern ChatMsg gChatSendRecord;    /* the record of a line sent offline (CHAT_SEND_LOCAL) */
extern u8 gChatMsgClass;
extern u16 gChatRecipients;

/* Runs the chat input for a frame. When it ends with a line, sends it by gChatSendTo
 * (CHAT_SEND_*) after the word filter, plays an act tag's gesture, or logs why the
 * recipients are wrong (gChatSystemRecord); then closes the input. While it runs the chat
 * windows stay closed. */
void Chat_SendInput(u16 unused)
{
    s8 text[64];
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
                Chat_AddReceived(255, gChatSystemRecord);
            } else {
                Chat_PlayAct((s32)text);
                if (text[0] != 0) {
                    gChatSendWait = 60;
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
                            to = 255;
                            break;
                        case 1:
                            to = gChatRecipients & 0xF;
                            break;
                        }
                        Chat_LogAdd(pl->index, text, NULL, 0);
                        QSync_SendChat(pl->index, 1, text, to);
                        gChatWinOpen = 1;
                        gChatWinTimer = 300;
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
    if ((tags != 0) && (act != 0) && (act < 13U) && ((Game_IsOnline() == 1) || (gScene.inLobby == 0))) {
        Player_RequestGesture(act & 0xFF);
        Hud_SetCancel();
    }
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/menu_windows", gStatusFmtInt);
INCLUDE_RODATA("asm/sub_main/nonmatchings/menu_windows", gWindowFmtString);

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

extern u8 gChatWhoColors[];
extern u8 gChatClassColors[];


extern char gMyUserId[];

/* Adds a message from player `who` to the chat ring (gChat.lines), with the sender and
 * colours from header (without one, the local player's), wrapped into at most two rows of
 * 22 bytes (30 when wide), a Shift-JIS pair never split. */
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
    if (gChatLogCount > 64) {
        gChatLogCount = 64;
    }
    line->rows = 0;
    width = wide == 0 ? 22 : 30;
    for (i = 0, row = line->text[0]; i < 2; i++, row += 24) {
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
            if ((ch >= 128 && ch <= 159) || (ch >= 224 && ch <= 255)) {
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

/* Logs a received ChatMsg (after the word filter), scrolls the log on when it is full, and
 * opens the chat window for 300 frames (beeping when the line is another player's). The
 * text is reached as msg + 28 (ChatMsg.text): through the member, MWCC keeps the address
 * in a register and the code differs. */
void Chat_AddReceived(u8 who, s8 *msg)
{
    Chat_FilterWords((char *) (msg + 28));    /* ChatMsg.text; see above */
    Chat_LogAdd(who, (s8 *) (msg + 28), (ChatMsg *) msg, 0);
    if (Chat_CountLogRows() > 11U) {
        gChatLogScroll += 1;
        if ((s32) gChatLogScroll > 63) {
            gChatLogScroll = 63;
        }
    }
    if (((who & 0xFF) != gScene.localPlayer) && (gChatWinOpen == 0)) {
        Chat_Beep();
    }
    gChatWinOpen = 1;
    gChatWinTimer = 300;
}

/* Lets the next message beep again. */
void Chat_ClearBeep(void)
{
    gChatBeeped = 0;
}

/* Plays the chat's beep (SE_SYS_SEND), once until Chat_ClearBeep. */
void Chat_Beep(void)
{
    if (gChatBeeped == 0) {
        Sound_PlaySeFlat(0, SE_SYS_SEND, 0);
        gChatBeeped = 1;
    }
}

extern s8 gPlayerIds[][8];

/* Logs a line from a player of this console (the party's messages, through qnet_sync.c)
 * with a beep; kind 255 gives it the system class colour. */
void Chat_LogAddLocal(u8 who, u8 kind, char *msg)
{
    ChatMsg header;
    u8 cls;

    Chat_FilterWords(msg);
    if (kind == 255) {
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
        if (gChatLogScroll > 63) {
            gChatLogScroll = 63;
        }
    }
    gChatWinOpen = 1;
    gChatWinTimer = 300;
    Sound_PlaySeFlat(0, SE_SYS_SEND, 0);
}

/* Scrolls the chat log to its newest lines (the lobby's log for CHAT_SEND_LOBBY). */
void Chat_ResetScroll(void)
{
    if (gChatSendTo == CHAT_SEND_LOBBY) {
        ChatLog_ResetScroll();
        return;
    }
    gChatLogScroll = 0;
    if (Chat_CountLogRows() < 12U) {
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
    pos[1] = 420 - (cellH + 1) * 12;
    gChatFrameBox.flags = ((color + 2) & WINDOW_STYLE_MASK) | WINDOW_FILL;
    gChatFrameBox.charH = cellH - 1;
    Window_DrawText(&gChatFrameBox, pos, (u8 *)text);
    gChatFrameBox.charW = 21;
    gChatFrameBox.charH = 17;
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

/* Draws the chat log in text size `size` moved down by dy: scrolled back (the newest `scroll` lines skipped)
 * from the bottom up when it fills the window and not at its end, else from the oldest line down; lines
 * with an id get a sender row (Chat_FormatSender), rows with a wide last character a narrower font. */
void Chat_DrawLogLines(u8 scroll, s16 dy, u8 atEnd, u8 size)
{
    s32 k;
    char sender[32];
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
        y = 420 - lineH;
        top = 420 - lineH * 11;
        i = gChatLogHead - 1 - scroll;
        left = gChatLogCount - scroll;
        for (; left != 0; left--, i--) {
            line = &gChat.lines[i &= 0x3F];
            Text_SetColor(line->color[3]);
            k = line->rows - 1;
            if (k >= 0) {
                row = line->text[k];
                do {
                    Font_SetPos(24, y + offY);
                    if (row[22] != 0) {
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
                    row -= 24;
                } while (k >= 0);
            }
            if (line->id[0] != 0) {
                Chat_FormatSender(sender, line);
                Text_DrawShadow2(24, y + offY, 1, line->color[2], sender);
                y -= lineH;
                if (y < top) {
                    return;
                }
            }
        }
    } else {
        count = gChatLogCount;
        i = gChatLogHead - count;
        y = 420 - lineH * 11;
        for (; count != 0; count--, i++) {
            entry = &gChat.lines[i &= 0x3F];
            if (entry->id[0] != 0) {
                Chat_FormatSender(sender, entry);
                Text_DrawShadow2(24, y + offY, 1, entry->color[2], sender);
                y += lineH;
                if (y >= 420) {
                    return;
                }
            }
            Text_SetColor(entry->color[3]);
            k = 0;
            if (k < entry->rows) {
                fwdRow = entry->text[0];
                do {
                    Font_SetPos(24, y + offY);
                    if (fwdRow[22] != 0) {
                        Font_SetSpacing(narrowW, charH);
                        Text_Draw((char *)fwdRow);
                        Font_SetSpacing(size, charH);
                    } else {
                        Text_Draw((char *)fwdRow);
                    }
                    y += lineH;
                    if (y >= 420) {
                        return;
                    }
                    k++;
                    fwdRow += 24;
                } while (k < entry->rows);
            }
        }
    }
}

/* The chat log window (as wide as gChatTextSizes[gSysSave.options.chatSize]), its blinking scroll arrows (gChatLines flags) and its lines. */
void Chat_DrawLog(void)
{
    u8 size;
    s16 x;
    u32 pulse;
    u16 phase;
    u32 upColor;
    s16 y;

    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    size = gChatTextSizes[(s8)gSysSave.options.chatSize];
    Chat_DrawFrame(0, size, 1);
    phase = (gFrameTick & 0x1F) << 11;
    pulse = ((s8)(48.0f * Math_Sin((6.2831855f / 65536.0f) * phase)) + 207) << 24 | 0x1ACC8E;
    x = 7.0f + (size * 11) / 2;
    y = 404 - (size * 18 / 21 + 1) * 12;
    if (gChatLines & CHAT_WIN_MORE_BEFORE) {
        upColor = pulse;
        if (gChatLines & CHAT_WIN_STEP_BACK) {
            y -= 2;
        }
    } else {
        upColor = 0xD8707070;
    }
    Chat_DrawArrow(x, y, 30, 24, upColor, ARROW_TEXRECT);
    y = 414;
    if (gChatLines & CHAT_WIN_MORE_AFTER) {
        if (gChatLines & CHAT_WIN_STEP_ON) {
            y += 2;
        }
    } else {
        pulse = 0xD8707070;
    }
    Chat_DrawArrow(x, y, 30, 24, pulse, ARROW_MIRROR | ARROW_TEXRECT);
    Chat_DrawLogLines(gChatLogScroll, -10, gChatLogAtEnd, (s8)gSysSave.options.chatSize);
}

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

/* Online in the town: opens the current chat window (gChatWindowSel: 0 the gChatLog one, 1 the gChatWinTimer one) for 300 frames,
 * or if it is open, closes it and switches to the other. */
void Chat_ToggleWindow(u16 buttons)
{
    ChatWindow *window;
    s32 closed;
    s16 timer;

    if (Game_IsOnline() == 1 && gScene.inLobby != 0 && gScene.area != AREA_ONLINE_VILLAGE && Chat_ToggleRequested(buttons) == 1) {
        if (gChatWinHold > 0) {
            switch (gChatWinState) {
            case 2:
            case 3:
                return;
            }
        }
        closed = 0;
        timer = 300;
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
                    gChatLog.win.timer = 300;
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
                    gChatLog.win.timer = 300;
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
        gChatLog.win.timer = 300;
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
        Gfx_SetTexFilter(GFX_TEX_NEAREST);
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
extern char *gChatOpenText;

/* Draws a chat window (kind 1: the log, 2 and 3: the input line, 3 as wide as gChatTextSizes[gSysSave.options.chatSize]) and its marker, with the blinking cue (colour 0 for the lobby's chat, 1 for the others). */
void Chat_DrawWindow(u8 kind)
{
    Panel *win;
    u8 size;
    s32 markerFlags;
    char *text;

    if (kind != 0) {
        size = (kind == 3) ? gChatTextSizes[(s8)gSysSave.options.chatSize] : 21;
        if (kind == 1) {
            win = &gChatRecvWindow;
            if (Chat_CanDraw() == 1) {
                text = gChatOpenText;
            } else {
                text = gChatRecvText[0];
            }
        } else {
            win = gChatLogWindow;
            win->y = (kind == 2) ? 400 : 402 - (size * 18 / 21 + 1) * 13;
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

extern u8 gChatSizeLineGaps[];
extern char *gNoticeYesSelText;
extern char *gNoticeNextText;

/* Draws the notice window (nothing with NOTICE_CLOSED): the part of gChat.notice shown so
 * far, wrapped at 22 half-width characters, then by gNoticeFlags the yes/no prompt, the
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
    y = 420 - lineH * 11U;    /* 11 lines up from the bottom; the count is unsigned for the match */
    i = gSysSave.options.chatSize;
    step = lineH + gChatSizeLineGaps[i];
    charsLeft = gNoticeShownChars;
    src = gChat.notice;
    if (charsLeft > 0) {
        do {
            line = src;
            widthLeft = 22;
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
            Font_SetPos(24, y);
            Text_PrintCodes(chars, line);
            y += step;
        } while (charsLeft > 0);
    }
    y += step;
    if (gNoticeFlags & NOTICE_PROMPT) {
        Font_SetPos(24, y);
        if (gNoticeFlags & NOTICE_PROMPT_YES) {
            Text_Printf(gNoticeYesSelText);
        } else {
            Text_Printf(gNoticeNoSelText[0]);
        }
    }
    if (gNoticeFlags & NOTICE_NEXT) {
        Text_SetColor(2);
        Font_SetPos(size * 132 / 21 + 24, y);
        Text_Draw(gNoticeNextText);
    }
    if (gNoticeFlags & NOTICE_LIST) {
        Font_SetPos(24, y);
        for (i = 0; i < 11; i++) {
            if (gNoticeList[i] == NULL) {
                break;
            }
            Font_SetPos(24, y);
            if (i == gNoticeSel) {
                Text_Printf(TXT(0x004908B8) /* a colour code, then the name in full-width corner brackets */, gNoticeList[i]);
            } else {
                Text_Printf("~C00  %s", gNoticeList[i]);
            }
            y += step;
        }
    }
    if (gNoticeFlags & NOTICE_LIST_PICKED) {
        Font_SetPos(24, y);
        Text_Printf(gNoticeList[gNoticeSel]);
    }
}

struct Player;

extern ListPanel gPersonalStatusPanels[];
extern Panel gPersonalStatusBoxes[];
struct ListPanel;
void ListPanel_Draw(ListPanel *b, s16 *pos, char *title, s16 sel);
void SkillList_DrawPage(s16 x, s16 y, u8 page, u8 mark);
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
    Font_SetSpacing(18, 18);
    switch (page) {
    case 0:
        y = StatusPage_DrawHeader(pl, 306, 82);
        y = StatusPage_DrawSymbolColor(306, y);
        y = StatusPage_DrawRank(306, y);
        y = StatusPage_DrawArenaRecord(306, y);
        StatusPage_DrawOnline(306, y);
        Text_DrawWrapped33(310, 357, 20, (char *)gSave.comment);
        break;
    case 1:
        PersonalStatus_DrawEquipment(pl);
        StatusPage_DrawStats(pl, 0, 306, 294);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        SkillList_DrawPage(306, 62, page - 2, Player_ArmorHasTorsoUp(pl));
        Player_DrawSkillsPlain(pl, 306, 294);
        break;
    }
}

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
    valueX = left + 72;
    Font_SetPos(valueX, y);
    Text_Draw((char *)pl->name);
    y += 20;
    if (Game_IsOnline() == 1) {
        Font_SetPos(valueX, y);
        if (Quest_IsDrill() == 1 && gScene.allLocal != 0) {
            Text_Draw(gMyUserId);
        } else if (gScene.inLobby == 0) {
            Text_Draw((char *)gScene.userIds[pl->index]);
        } else {
            Text_Draw(gMyUserId);
        }
        y += 20;
    }
    if (Quest_IsDrill() == 1) {
        Font_SetPos(left + 126, y);
        Text_PrintfColored(gPointsFmt[0], gSave.points);
    } else {
        Font_SetPos(valueX, y);
        Text_PrintfColored(gMoneyFmt, gSave.money);
    }
    Font_SetPos(left + 72U, y + 20);
    Text_Draw(gHunterTitles[gSave.title].text);
    Font_SetPos(left + 72U, y + 40);
    StatusPage_DrawWeaponClass(pl);
    return y + 60;
}

/* A line of the status page, when StatusPage_HasSymbolColor: the label and the name of
 * gSave.symbolColor; returns the next line's y. */
s16 StatusPage_DrawSymbolColor(s16 x, s16 y)
{
    if (StatusPage_HasSymbolColor() == 1) {
        Font_SetColor(STATUS_COLOR_LABEL);
        Font_SetPos(x, y);
        Text_Draw(gSymbolColorLabel[0]);
        Font_SetColor(STATUS_COLOR_VALUE);
        Font_SetPos(x + 144, y);
        Text_Draw(gSymbolColorNames[gSave.symbolColor]);
        y += 20;
    }
    return y;
}

extern char *gHunterRankLabels;
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
        Font_SetPos(x + 72, y);
        Text_PrintfColored(gStatusFmtInt, hr);
        Font_SetPos(x + 72, y + 20);
        if (hr < 999) {
            Text_PrintfColored(gRankPointsFmt[0], points, nextPoints);
        } else {
            Text_PrintfColored(gStatusFmtInt, points);
        }
        y += 40;
    }
    return y;
}

extern char *gArenaTimeFmt;

/* The arena record lines of the status page, once event flag 0xC4 is set: the fights, wins
 * and losses, and the total time in minutes and seconds; returns the next line's y. */
s16 StatusPage_DrawArenaRecord(s16 x, s16 y)
{
    if (Save_TestEventFlag(196) == 1) {
        Font_SetColor(STATUS_COLOR_LABEL);
        Font_SetPos(x, y);
        Text_PrintfStyled(2, gArenaRecordLabels[0]);
        Font_SetColor(STATUS_COLOR_VALUE);
        Font_SetPos(x + 90, y);
        Text_PrintfColored(gArenaRecordFmt[0], gSave.versusWins + gSave.versusLosses, gSave.versusWins, gSave.versusLosses);
        Font_SetPos(x + 162, y + 20);
        Text_PrintfColored(gArenaTimeFmt, gSave.versusMinutes / 60, gSave.versusMinutes % 60);
        y += 40;
    }
    return y;
}

void Profile_DrawStatus(s16 x, s16 y); /* unit's own declaration: the definition (lobby) is void Profile_DrawStatus(s32 x, s32 y); the calls here match only with this one */
void Profile_DrawFriendLimit(s16 x, s16 y);
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
        Profile_DrawStatus(x + 72, y);
        Profile_DrawFriendLimit(x + 162, y + 20);
        y += 40;
    }
    return y;
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/menu_windows", gStatusFmtInt4);
INCLUDE_RODATA("asm/sub_main/nonmatchings/menu_windows", gStatusFmtInt3);

extern char gStatusFmtInt4[];
extern char gStatusFmtInt3[];


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

    Font_SetSpacing(18, 18);
    Font_SetColor(STATUS_COLOR_LABEL);
    Text_DrawLinesColored(x, y, gStatusStatLabels, 20);
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
    leftX = (u16)(left + 90);
    Font_SetPos(leftX, top);
    Text_PrintfColored(gStatusFmtInt4, pl->hpMax);
    Font_SetColor(staminaColor);
    row1Y = top + 20;
    Font_SetPos(leftX, row1Y);
    Text_PrintfColored(gStatusFmtInt4, pl->staminaMax / 3);
    Font_SetColor(attackColor);
    row2Y = top + 40;
    Font_SetPos(leftX, row2Y);
    Text_PrintfColored(gStatusFmtInt4, attack);
    Font_SetColor(defenseColor);
    row3Y = top + 60;
    Font_SetPos(leftX, row3Y);
    Text_PrintfColored(gStatusFmtInt4, pl->defense);
    Font_SetColor(resist0Color);
    rightX = (u16)(left + 90) + 144;
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
    Font_SetPos(rightX, top + 80);
    Text_PrintfColored(gStatusFmtInt3, (s16)pl->resist[4]);
}

/* The attack power shown for a player: attack times the weapon class's multiplier / 100. */
u32 Player_ShownAttack(Player *pl)
{
    return (u16)(pl->attack * gWeaponClassMultipliers[Weapon_GetClass(pl->equip[6].type, pl->equip[6].id)] / 100);
}

/* The equipment page's icons and names for a player. */
void PersonalStatus_DrawEquipment(Player *pl)
{
    StatusPage_DrawEquipment(&pl->equip[EQUIP_SLOT_WEAPON], pl->equip);
}


struct Pt;
u8 Equip_GetRarity(u8 type, u16 id);
u32 Equip_Color(u8 idx, u8 alpha, s32 alt);
extern u8 gStatusEquipIconU[];
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
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    spr.x = 311;
    spr.w = 40;
    spr.h = 32;
    y = 85;
    spr.y = y;
    Equip_GetIconUV(weapon, (struct Pt *)&spr.u0, (struct Pt *)&spr.u1);
    spr.color = Equip_Color(Equip_GetRarity(weapon->type, weapon->id), 255, 0);
    Sprite_Draw(&spr);
    spr.v0 = 289;
    spr.v1 = 319;
    for (i = 0; i < 5; i++) {
        y += 32;
        spr.y = y;
        spr.u0 = gStatusEquipIconU[i];
        spr.u1 = spr.u0 + 32;
        spr.color = Equip_Color(armor[i]->rarity, 255, 0);
        Sprite_Draw(&spr);
    }
    Font_SetSpacing(18, 18);
    Font_SetPos(360, 92);
    Text_Printf(gEquipWeaponFmt[0], Equip_GetName(weapon->type, weapon->id));
    for (i = 0, textY = 124; i < 5; textY += 32, i++) {
        Font_SetPos(360, textY);
        Text_Printf(gEquipArmorFmt, gArmorNameTables[i][ids[i]]);
    }
}

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

/* Draws a player's five skill effects, one per line from (x, y + 20), in colors[slot] (text
 * colour 9 for a skill that doesn't apply to the equipment, Player_SkillApplies), with an
 * arrow before text colours 14 and 2 (the equipment preview's gained and lost skills); the
 * no-skill text when there are none. */
void Player_DrawSkills(Player *pl, s32 x, s16 y, s16 *colors)
{
    u32 i;
    s16 lineY;
    s32 textX;
    s32 markX;

    Font_SetSpacing(18, 18);
    if ((pl->skills[0] | pl->skills[1] | pl->skills[2] | pl->skills[3] | pl->skills[4]) == 0) {
        Text_SetColor(0);
        Font_SetPos(x, y + 20);
        Text_Draw(gSkillEffectNames[0]);
        return;
    }
    lineY = y + 20;
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
            if (*colors == 14 || *colors == 2) {
                Menu_DrawArrow(markX, lineY, 8, 18, 0xC0D0D0D0, 0);
            }
            lineY += 20;
        }
    }
}

/* A blade's status value (poison, paralysis, sleep or an element), -1 when it has none. */
s32 BladeData_StatusValue(BladeData *blade)
{
    return blade->statusType != 0 ? blade->statusValue : -1;
}

extern u8 gGunnerNoSkillGroups[];
extern u8 gBowNoSkillGroups[];
extern u8 gBladeNoSkillGroups[];

/* 1 if the skill tree of a player's skill slot applies to the player's weapon: gunners
 * (Player.weaponFlags 2) don't get the trees in gGunnerNoSkillGroups (gBowNoSkillGroups for
 * a bow, weapon class 10), nor the ammo trees (38, 11, 23 and 47) for ammo their gun
 * can't load, and tree 52 not with SKILL_FREE_SHOTS nor for a bow; blademasters (weaponFlags 1)
 * don't get gBladeNoSkillGroups', nor 47 without a status on the blade. Trees 4 and 5
 * need weaponFlags 4 (tree 4 also a class-1 weapon with level bit 0x80). */
s32 Player_SkillApplies(Player *pl, u8 slot)
{
    u8 group;
    u8 *list;
    EquipData *data;
    s32 mask;

    group = pl->skillGroups[slot];
    if (pl->weaponFlags & 2) {
        if (group == 52) {
            if (Player_HasSkill(pl, SKILL_FREE_SHOTS) == 1) {
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
            if (group == 38) {
                if (!(mask & 7)) {
                    return 0;
                }
            } else if (group == 11) {
                if (!(mask & 0x38)) {
                    return 0;
                }
            } else if (group == 23) {
                if (!(mask & 0x1C0)) {
                    return 0;
                }
            } else if (group == 47) {
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
        if (group == 47) {
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

extern u16 gArmorEquipSlots[];

/* 1 if a piece of the player's armour has skill tree 41, 42 or 43 (the torso up
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
            if ((u32)(*tree - 41) <= 1 || *tree == 43) {
                return 1;
            }
            tree += 2;
        }
    }
    return 0;
}

extern char *gSkillColFmt;
extern char *gSkillTotalFmt;

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

    Font_SetSpacing(16, 18);
    top = y;
    Font_SetPos(x, top + 20);
    Font_SetColor(STATUS_COLOR_LABEL);
    Text_PrintfColored(gSkillListHeader[0]);
    lineY = top + 40;
    shown = 0;
    for (rec = &gSkillRecords[page * 9], i = 9; i != 0; lineY += 20, i--, rec++) {
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
        box.x0 = x + 152;
        box.x1 = x + 177;
        box.y0 = top + 41;
        box.y1 = top + 39 + shown * 20;
        Prim_FillRect(&box);
    }
}

/* The skill list's page count: nine records a page (at most 46 records counted), at least 1. */
u8 SkillList_PageCount(void)
{
    u32 count;
    SkillRecord *rec;

    for (count = 0, rec = gSkillRecords; count < 46; count++, rec++) {
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
        left = 54;
        for (tree = 1; tree < 82; tree++) {
            if (tree == 41 || tree == 42 || tree == 43) {
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
    for (rec = gSkillRecords, left = 54; left > 0; left--, rec++) {
        rec->used = 0;
    }
    for (tree = 1; tree < 82; tree++) {
        SkillList_Insert(pl, tree, other);
    }
}


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
    for (i = 54, free = gSkillRecords; i > 0; i--, free++) {
        if (free->used == 0) {
            free++;
            for (slot = gSkillRecords, j = 54; j > 0; j--, slot++) {
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
u8 Player_GetSkill(Player *p, u8 kind);

/* Makes pl's record of skill tree rec->skill: its points per piece (weapon, head, torso,
 * arms, waist, legs) and in total, and its sort key (active skills first, then by total);
 * with other, marks each against other's record (gSkillCompareRecords). 0 when no piece
 * gives points. The torso up trees (41-43) are SkillList_FillPieces's. */
s32 SkillList_MakeRecord(Player *pl, SkillRecord *rec, Player *other)
{
    SkillRecord *cmp;
    u32 n;
    u32 i;

    if ((u32)(rec->skill - 41) <= 1 || rec->skill == 43) {
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
    rec->used = 128;
    rec->total = rec->pts[0] + rec->pts[1] + rec->pts[2] + rec->pts[3] + rec->pts[4] + rec->pts[5];
    rec->key = 0;
    if (Player_GetSkill(pl, rec->skill) != 0) {
        rec->used |= 1;
        rec->key += 32768;
        rec->mark[0] = rec->mark[1] = rec->mark[2] = rec->mark[3] = rec->mark[4] = rec->mark[5] = rec->totalMark = 3;
    } else {
        rec->mark[0] = rec->mark[1] = rec->mark[2] = rec->mark[3] = rec->mark[4] = rec->mark[5] = 0;
        rec->totalMark = 6;
    }
    if (rec->total >= 0) {
        rec->key += (rec->total << 7) + 16384;
    } else {
        rec->key += -rec->total << 7;
    }
    rec->key += rec->skill;
    if (other != NULL) {
        for (cmp = gSkillCompareRecords, n = 54; n != 0; n--, cmp++) {
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
    rec->used = 130;
    rec->total = 0;
    rec->key = 0;
    return 1;
}

/* the calls cast to s16: this unit's code takes the value as s16 */

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
        code = 32771;
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
 * plus one when one has decorations (without b: with flag 0x10, or flag 0x20 and decorations). */
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
    EquipWindow_Draw(&gPlayers[gScene.localPlayer], item, x, y, flags, other, 178);
}

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
        Font_SetPos(x + 54, y + 10);
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
extern char *gAmmoCount1Fmt;
extern char *gAmmoCount3Fmt;

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
    char buf[44];

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

/* unit's own declaration: declared here as Equip_DrawIcon and StatusPage_DrawEquipment call it; with the definition's s32 (u8 type, u16 id), the calls compile differently */

/* Draws the icon of a piece of equipment at (x + 5, y), in its colour (grey with flags 8). */
void Equip_DrawIcon(EquipItem *item, s16 x, s16 y, u8 flags)
{
    Sprite spr;

    spr.x = x + 5;
    spr.y = y;
    spr.w = 40;
    spr.h = 32;
    Equip_GetIconUV(item, (struct Pt *)&spr.u0, (struct Pt *)&spr.u1);
    if (!(flags & 8)) {
        spr.color = Equip_Color(Equip_GetRarity(item->type, item->id), 255, 0);
    } else {
        spr.color = 0xFF808080;
    }
    Sprite_Draw(&spr);
}

void EquipMenu_DrawSkills(EquipItem *e, s16 x, s32 y, EquipItem *cmp, u8 flags); /* unit's own declaration: declared here as EquipWindow_DrawDetails calls it; with the definition's void (EquipItem *e, s16 x, s32 y, EquipItem *cmp, u8 flags), the call compiles differently */
s32 Equip_DataWearable(Player *p, EquipItem *e);
extern char *gBladeDescTexts[][4];
extern char *gGunDescTexts[][4];
extern char *gArmorDescTexts[][4];
extern char *gActiveSkillsTitle;
extern char *gNoEquipText;
extern char *gBladeDetailLabels[];
extern char *gBowgunDetailLabels[];
extern char *gScopeVariableText;
extern char *gLongBarrelText;
extern char *gSilencerText;
extern char *gDashText;
extern char *gAmmoClipLabel;
extern char *gAttackPerfText;

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
        Font_SetPos(x, y + 20);
        Bowgun_DrawAmmoLine(data, have, extra, 0, plus);
        Font_SetPos(x, y + 40);
        Bowgun_DrawAmmoLine(data, have, extra, 1, plus);
        Font_SetPos(x, y + 60);
        Bowgun_DrawAmmoLine(data, have, extra, 2, plus);
        Font_SetPos(x, y + 80);
        Bowgun_DrawAmmoLine(data, have, extra, 3, plus);
        Font_SetPos(x, y + 100);
        Bowgun_DrawAmmoLine(data, have, extra, 4, plus);
        Font_SetPos(x + 180, y + 20);
        Bowgun_DrawAmmoLine(data, have, extra, 5, plus);
        Font_SetPos(x + 180, y + 40);
        Bowgun_DrawAmmoLine(data, have, extra, 6, plus);
        Font_SetPos(x + 180, y + 60);
        Bowgun_DrawAmmoLine(data, have, extra, 7, plus);
        Font_SetPos(x + 180, y + 80);
        Bowgun_DrawAmmoLine(data, have, extra, 8, plus);
    } else {
        Text_Printf(gAmmoClipLabel);
        Font_SetPos(x, y + 20);
        Bowgun_DrawAmmoLine(data, have, extra, 9, plus);
        Font_SetPos(x, y + 40);
        Bowgun_DrawAmmoLine(data, have, extra, 10, plus);
        Font_SetPos(x, y + 60);
        Bowgun_DrawAmmoLine(data, have, extra, 11, plus);
        Font_SetPos(x, y + 80);
        Bowgun_DrawAmmoLine(data, have, extra, 12, plus);
        Font_SetPos(x, y + 100);
        Bowgun_DrawAmmoLine(data, have, extra, 13, plus);
        Font_SetPos(x + 153, y + 20);
        Bowgun_DrawAmmoLine(data, have, extra, 14, plus);
        Font_SetPos(x + 153, y + 40);
        Bowgun_DrawAmmoLine(data, have, extra, 15, plus);
        Font_SetPos(x + 153, y + 60);
        Bowgun_DrawAmmoLine(data, have, extra, 16, plus);
        Font_SetPos(x + 153, y + 80);
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

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
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
        Font_SetPos(x + 45, y + 10);
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
            Font_SetPos(x + 72, y + 40);
            Text_PrintfColored(gStatusFmtInt4, atk);
            EquipMenu_DrawSharpness(pl, item, x + 157, y + 62);
            EquipMenu_DrawElement(item, x, y + 80, other);
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
            Font_SetPos(x + 72, y + 40);
            Text_PrintfColored(gStatusFmtInt4, atk);
            if (cls != 10) {
                Text_SetColor(cAff);
                Font_SetPos(x + 90, y + 60);
                Text_Draw(gGunReloadTexts[data->gun.reload]);
                Text_SetColor(cRng);
                Font_SetPos(x + 90, y + 80);
                Text_Draw(gGunRecoilTexts[data->gun.recoil]);
                Text_SetColor(cLvl);
                Font_SetPos(x + 108, y + 100);
                buf[0] = 130;
                buf[1] = (item->level & 0xF) + 80;
                buf[2] = 0;
                Text_Draw(buf);
                Font_SetSpacing(16, 18);
                Text_SetColor(cSil);
                Font_SetPos(x + 175, y + 40);
                if (item->level & 0x40) {
                    Text_Draw(gScopeVariableText);
                } else {
                    Text_Draw(gScopeFixedText[0]);
                }
                Font_SetSpacing(18, 18);
                Text_SetColor(cOpt);
                Font_SetPos(x + 196, y + 60);
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
                EquipMenu_DrawElement(item, x + 198, y + 80, other);
                lines = gBowgunDetailLabels;
            } else {
                /* FAKEMATCH: x + 0. The game passes an int x here, re-extended from the case's (s32)x copy (the
                 * other calls pass the s16 x as it is); a zero layout offset beside x + 198 above is the one form
                 * found. Plain x: 10 words off, (s32)x: 15. */
                EquipMenu_DrawElement(item, x + 0, y + 60, other);
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
            Font_SetPos(x + 72, y + 40);
            Text_PrintfColored(gStatusFmtInt3, item->level + 1);
            Text_SetColor(cAtk);
            Font_SetPos(x + 216, y + 40);
            Text_PrintfColored(gStatusFmtInt3, def);
            Text_SetColor(cOpt);
            Font_SetPos(x + 72, y + 60);
            Text_PrintfColored(gStatusFmtInt3, data->resist[0]);
            Text_SetColor(cAff);
            Font_SetPos(x + 216, y + 60);
            Text_PrintfColored(gStatusFmtInt3, data->resist[1]);
            Text_SetColor(cLvl);
            Font_SetPos(x + 72, y + 80);
            Text_PrintfColored(gStatusFmtInt3, data->resist[2]);
            Text_SetColor(cSil);
            Font_SetPos(x + 216, y + 80);
            Text_PrintfColored(gStatusFmtInt3, data->resist[3]);
            Text_SetColor(cRng);
            Font_SetPos(x + 72, y + 100);
            Text_PrintfColored(gStatusFmtInt3, data->resist[4]);
            lines = gArmorStatLabels;
            break;
        }
        }
        Text_SetColor(base);
        Text_DrawLines(x, y + 40, lines, 20);
        break;
    case 1: {
        s32 id;
        u8 k;
        u8 armorFlags;

        Equip_DrawIcon(item, x, y + 14, flags);
        id = item->id;
        switch (item->type) {
        case 7:
            Text_DrawLines(x, y + 60, gHunterTypeLabels, 20);
            names = gGunDescTexts[id];
            break;
        case 6:
            Text_DrawLines(x, y + 60, gHunterTypeLabels, 20);
            names = gBladeDescTexts[id];
            break;
        case 0:
            id += 237;
        case 5:
            id += 238;
        case 4:
            id += 243;
        case 3:
            id += 253;
        case 2:
            Text_DrawLines(x, y + 60, gEquipTypeLabels, 20);
            names = gArmorDescTexts[id];
            break;
        }
        Text_SetColor(col);
        Text_DrawLines(x + 54, y, names, 20);
        Font_SetPos(x + 144, y + 60);
        if (item->id != 0) {
            switch (item->type) {
            case 6:
            case 7:
                k = item->type - 6;
                break;
            default:
                armorFlags = Equip_GetData(item)->armor.flags;
                if ((armorFlags & 0xC) == 12) {
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
        Font_SetPos(x + 72, y + 80);
        if (item->id != 0) {
            Text_SetColor(rankCol);
            buf[0] = 130;
            buf[2] = 0;
            buf[1] = rank + 80;
            Text_Draw(buf);
        } else {
            Text_Draw(gDashText);
        }
        EquipWindow_DrawWeaponValue(pl, item, x + 144, y + 80, other);
        EquipWindow_DrawSlots(item, x + 90, y + 100, (flags & 8) != 0);
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
                Font_SetPos(textX, textY + 20);
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
        EquipWindow_DrawDecorations(item, x, y + 20, kind - 8);
        break;
    case 10:
        Text_SetColor(col);
        Font_SetPos(x + 54, y + 10);
        Text_Draw(gNoEquipText);
        break;
    }
}
#pragma optimization_level reset

extern char *gPercentFmt;

/* The weapon's value 8 (Player_GetWeaponStat; only for guns or from sharpness 3) for the player with it, coloured
 * against the one of other (higher: 0x802B7F7F, lower: 0x807F212E) or when SKILL_AFFINITY_PLUS5, _PLUS10_NO_BOUNCE or _PLUS15_NO_BOUNCE applies. */
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
            if (Player_HasSkill(preview, SKILL_AFFINITY_PLUS5) == 1 || Player_HasSkill(preview, SKILL_AFFINITY_PLUS10_NO_BOUNCE) == 1 || Player_HasSkill(preview, SKILL_AFFINITY_PLUS15_NO_BOUNCE) == 1) {
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
        Font_SetPos(x + 72, y);
        sprintf(buf, gPercentFmt, value);
        Sjis_Widen(buf, out);
        Text_PrintfColored(out);
    }
}

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
        x += 20;
    }
    Font_SetColor(0x80505050);
    for (; i < slots; i++) {
        Font_SetPos(x, y);
        Text_Draw(TXT(0x00490968) /* a full-width white circle */); /* a hollow circle */
        x += 20;
    }
    for (; i < 3; i++) {
        Font_SetPos(x, y);
        Text_Draw(TXT(0x00490970) /* a full-width minus sign */); /* a dash */
        x += 20;
    }
}

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
extern char *gDecoNameFmt;

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
        y += 20;
        skill = *dataPtr + 8;
        for (slot = 0; slot < 4; slot++, skill += 2) {
            if (*skill == 0) {
                break;
            }
            Font_SetPos(left + (slot & 1) * 162, y + (slot >> 1) * 20);
            if (*skill != 41 && *skill != 42 && *skill != 43) {
                Text_PrintfColored(gDecoSkillFmt[0], gSkillNames[*skill], (s8)skill[1]);
            } else {
                Text_Draw(gSkillNames[*skill]);
            }
        }
        y += ((slot + 1) >> 1) * 20;
    }
}

extern u8 gShellTypeOfSpecial[];

/* Draws the name of a weapon's (Equip_GetBladeSpecial) kind at (x + 126, y + 40). */
void EquipWindow_DrawWeaponKind(EquipItem *item, s16 x, s16 y)
{
    Font_SetColor(STATUS_COLOR_VALUE);
    Font_SetPos(x + 126, y + 40);
    Text_Draw(gShellTypeTexts[gShellTypeOfSpecial[Equip_GetBladeSpecial(item)]]);
}

extern char *gHornNotesLabel;
extern u8 gBladeSpecialMarks[][3];

/* A label and three coloured marks (gBladeSpecialMarks by Equip_GetBladeSpecial's kind) for a weapon. */
void EquipWindow_DrawMarks(EquipItem *item, s16 x, s16 y)
{
    Sprite spr;
    s32 i;
    u8 *marks;

    Font_SetColor(STATUS_COLOR_VALUE);
    Font_SetPos(x + 108, y + 40);
    Text_Draw(gHornNotesLabel);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    spr.x = x + 180;
    spr.y = y + 41;
    spr.w = 20;
    spr.h = 16;
    *(u32 *)&spr.u0 = 0xB400AC;
    *(u32 *)&spr.u1 = 0xC400BC;
    marks = gBladeSpecialMarks[Equip_GetBladeSpecial(item)];
    for (i = 3; i > 0; i--) {
        spr.color = gHornNoteColors[*marks];
        Sprite_Draw(&spr);
        marks++;
        spr.x += 26;
    }
}

extern s16 gBowgunClipShots[][4];

/* A heading and the three (four with SKILL_LOAD_UP) entries gBowgunClipShots of a piece of equipment's row 15,
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
    k = 20;
    if (Player_HasSkill(pl, SKILL_LOAD_UP) == 1) {
        n++;
        k = 21;
    }
    Font_SetPos(x, y + 24);
    Font_SetColor(0x807F5230);
    Text_PrintfStyled(3, gEquipNameFmt[k]);
    Font_SetColor(STATUS_COLOR_LABEL);
    for (i = 0; i < n; i++, x += 72) {
        Font_SetPos(x, y + 68);
        Text_PrintfColored(gBowShotTexts[gBowgunClipShots[data->clipRow][i]]);
    }
}

extern char *gBowCoatingsLabel;
extern u16 gEquipListItemBits[4];
extern u16 gEquipListItems[4];

/* A heading, then the names of the items gEquipListItems whose bits gEquipListItemBits are set in the gun's ammoMask, one per line. */
void EquipWindow_DrawItemList(GunData *gun, s16 x, s16 y)
{
    s32 i;

    Font_SetColor(0x807F5230);
    Font_SetPos(x, y + 20);
    Text_Draw(gBowCoatingsLabel);
    Font_SetColor(STATUS_COLOR_LABEL);
    y += 40;
    for (i = 0; i < 4; i++) {
        if (gun->ammoMask & gEquipListItemBits[i]) {
            Font_SetPos(x, y);
            Text_PrintfColored(gItemNames[gEquipListItems[i]]);
            y += 20;
        }
    }
}

/* The menu windows' initialised data. */
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
char *gHelpTexts_1[948] = {
    D_004481B0, D_004481E0, gHelpTexts_1_0, gHelpTexts_1_1,
    gHelpTexts_1_2, gHelpTexts_1_3, gHelpTexts_1_4, gHelpTexts_1_5,
    gHelpTexts_1_8, gHelpTexts_1_8, D_004482C0, gHelpTexts_1_9,
    gHelpTexts_1_10, gHelpTexts_1_11, gHelpTexts_1_12, gHelpTexts_1_13,
    gHelpTexts_1_8, gHelpTexts_1_8, gHelpTexts_1_8, gHelpTexts_1_8,
    gHelpTexts_1_18, gHelpTexts_1_19, gHelpTexts_1_20, gHelpTexts_1_8,
    gHelpTexts_1_22, gHelpTexts_1_23, gHelpTexts_1_24, gHelpTexts_1_25,
    gHelpTexts_1_26, gHelpTexts_1_27, gHelpTexts_1_28, gHelpTexts_1_29,
    gHelpTexts_1_30, gHelpTexts_1_31, gHelpTexts_1_32, gHelpTexts_1_33,
    gHelpTexts_1_34, gHelpTexts_1_35, gHelpTexts_1_36, gHelpTexts_1_37,
    gHelpTexts_1_38, gHelpTexts_1_39, gHelpTexts_1_40, gHelpTexts_1_41,
    gHelpTexts_1_42, gHelpTexts_1_43, gHelpTexts_1_44, gHelpTexts_1_45,
    gHelpTexts_1_46, gHelpTexts_1_47, gHelpTexts_1_48, gHelpTexts_1_49,
    gHelpTexts_1_50, gHelpTexts_1_51, gHelpTexts_1_52, D_0046D898,
    gHelpTexts_1_54, gHelpTexts_1_55, gHelpTexts_1_56, gHelpTexts_1_57,
    gHelpTexts_1_58, gHelpTexts_1_59, gHelpTexts_1_60, gHelpTexts_1_61,
    gHelpTexts_1_62, gHelpTexts_1_63, gHelpTexts_1_64, gHelpTexts_1_65,
    gHelpTexts_1_66, gHelpTexts_1_67, gHelpTexts_1_68, gHelpTexts_1_69,
    gHelpTexts_1_70, gHelpTexts_1_71, gHelpTexts_1_72, gHelpTexts_1_73,
    gHelpTexts_1_74, gHelpTexts_1_75, gHelpTexts_1_76, gHelpTexts_1_77,
    gHelpTexts_1_78, gHelpTexts_1_79, gHelpTexts_1_80, gHelpTexts_1_81,
    gHelpTexts_1_82, gHelpTexts_1_83, gHelpTexts_1_84, D_0046D898,
    gHelpTexts_1_86, gHelpTexts_1_87, gHelpTexts_1_88, gHelpTexts_1_89,
    gHelpTexts_1_90, gHelpTexts_1_91, gHelpTexts_1_92, gHelpTexts_1_93,
    gHelpTexts_1_94, gHelpTexts_1_95, gHelpTexts_1_96, gHelpTexts_1_97,
    gHelpTexts_1_98, gHelpTexts_1_99, gHelpTexts_1_100, gHelpTexts_1_101,
    gHelpTexts_1_102, gHelpTexts_1_103, gHelpTexts_1_104, gHelpTexts_1_105,
    gHelpTexts_1_106, gHelpTexts_1_107, gHelpTexts_1_108, D_0046D898,
    gHelpTexts_1_110, gHelpTexts_1_111, gHelpTexts_1_112, gHelpTexts_1_113,
    gHelpTexts_1_114, gHelpTexts_1_115, gHelpTexts_1_116, gHelpTexts_1_117,
    gHelpTexts_1_118, gHelpTexts_1_119, gHelpTexts_1_120, gHelpTexts_1_121,
    gHelpTexts_1_122, gHelpTexts_1_123, gHelpTexts_1_124, gHelpTexts_1_125,
    gHelpTexts_1_126, gHelpTexts_1_127, gHelpTexts_1_128, gHelpTexts_1_129,
    gHelpTexts_1_130, gHelpTexts_1_131, gHelpTexts_1_132, gHelpTexts_1_133,
    gHelpTexts_1_134, gHelpTexts_1_135, gHelpTexts_1_136, gHelpTexts_1_137,
    gHelpTexts_1_138, gHelpTexts_1_139, gHelpTexts_1_140, gHelpTexts_1_141,
    gHelpTexts_1_142, gHelpTexts_1_143, gHelpTexts_1_144, gHelpTexts_1_145,
    gHelpTexts_1_146, gHelpTexts_1_147, gHelpTexts_1_148, gHelpTexts_1_149,
    gHelpTexts_1_150, gHelpTexts_1_151, gHelpTexts_1_152, gHelpTexts_1_153,
    gHelpTexts_1_154, gHelpTexts_1_155, D_0046D898, gHelpTexts_1_157,
    gHelpTexts_1_158, gHelpTexts_1_159, gHelpTexts_1_160, gHelpTexts_1_161,
    gHelpTexts_1_162, D_0046D898, gHelpTexts_1_164, gHelpTexts_1_165,
    gHelpTexts_1_166, gHelpTexts_1_167, gHelpTexts_1_168, gHelpTexts_1_169,
    gHelpTexts_1_170, gHelpTexts_1_171, gHelpTexts_1_172, gHelpTexts_1_173,
    gHelpTexts_1_174, gHelpTexts_1_175, gHelpTexts_1_176, gHelpTexts_1_177,
    gHelpTexts_1_178, gHelpTexts_1_179, gHelpTexts_1_180, gHelpTexts_1_181,
    gHelpTexts_1_182, gHelpTexts_1_183, gHelpTexts_1_184, gHelpTexts_1_185,
    gHelpTexts_1_186, gHelpTexts_1_187, gHelpTexts_1_188, gHelpTexts_1_189,
    gHelpTexts_1_190, gHelpTexts_1_191, D_0046D898, D_0046D898,
    gHelpTexts_1_194, gHelpTexts_1_195, gHelpTexts_1_196, gHelpTexts_1_197,
    gHelpTexts_1_198, gHelpTexts_1_199, gHelpTexts_1_200, gHelpTexts_1_201,
    gHelpTexts_1_202, gHelpTexts_1_203, gHelpTexts_1_204, gHelpTexts_1_205,
    gHelpTexts_1_206, gHelpTexts_1_207, gHelpTexts_1_208, gHelpTexts_1_209,
    gHelpTexts_1_210, gHelpTexts_1_211, gHelpTexts_1_212, gHelpTexts_1_213,
    gHelpTexts_1_216, gHelpTexts_1_215, D_00470070, gHelpTexts_1_219,
    gHelpTexts_1_218, D_00470130, gHelpTexts_1_220, gHelpTexts_1_221,
    gHelpTexts_1_222, gHelpTexts_1_225, gHelpTexts_1_224, D_00470250,
    gHelpTexts_1_226, gHelpTexts_1_227, gHelpTexts_1_228, gHelpTexts_1_229,
    gHelpTexts_1_230, gHelpTexts_1_231, gHelpTexts_1_232, gHelpTexts_1_233,
    gHelpTexts_1_234, gHelpTexts_1_235, gHelpTexts_1_236, gHelpTexts_1_237,
    gHelpTexts_1_238, gHelpTexts_1_239, gHelpTexts_1_240, gHelpTexts_1_241,
    gHelpTexts_1_242, gHelpTexts_1_243, gHelpTexts_1_244, D_0046D898,
    gHelpTexts_1_246, gHelpTexts_1_247, gHelpTexts_1_248, gHelpTexts_1_249,
    gHelpTexts_1_250, D_0046D898, gHelpTexts_1_252, gHelpTexts_1_253,
    gHelpTexts_1_254, gHelpTexts_1_255, gHelpTexts_1_256, gHelpTexts_1_257,
    gHelpTexts_1_258, gHelpTexts_1_259, gHelpTexts_1_260, gHelpTexts_1_261,
    gHelpTexts_1_262, gHelpTexts_1_263, gHelpTexts_1_264, gHelpTexts_1_265,
    gHelpTexts_1_266, gHelpTexts_1_267, gHelpTexts_1_268, gHelpTexts_1_269,
    gHelpTexts_1_270, gHelpTexts_1_271, gHelpTexts_1_272, gHelpTexts_1_273,
    gHelpTexts_1_274, gHelpTexts_1_275, gHelpTexts_1_276, gHelpTexts_1_277,
    gHelpTexts_1_278, gHelpTexts_1_279, gHelpTexts_1_280, gHelpTexts_1_281,
    gHelpTexts_1_282, gHelpTexts_1_283, gHelpTexts_1_284, gHelpTexts_1_285,
    gHelpTexts_1_286, D_0046D898, gHelpTexts_1_288, gHelpTexts_1_289,
    gHelpTexts_1_290, gHelpTexts_1_291, gHelpTexts_1_292, gHelpTexts_1_293,
    gHelpTexts_1_294, gHelpTexts_1_295, gHelpTexts_1_296, gHelpTexts_1_297,
    gHelpTexts_1_298, gHelpTexts_1_299, gHelpTexts_1_300, gHelpTexts_1_301,
    gHelpTexts_1_302, gHelpTexts_1_303, gHelpTexts_1_304, gHelpTexts_1_305,
    gHelpTexts_1_306, gHelpTexts_1_307, gHelpTexts_1_308, gHelpTexts_1_309,
    gHelpTexts_1_310, gHelpTexts_1_311, gHelpTexts_1_312, gHelpTexts_1_313,
    gHelpTexts_1_314, gHelpTexts_1_315, gHelpTexts_1_316, gHelpTexts_1_317,
    gHelpTexts_1_318, gHelpTexts_1_319, gHelpTexts_1_320, gHelpTexts_1_321,
    gHelpTexts_1_322, D_0046D898, gHelpTexts_1_324, gHelpTexts_1_325,
    gHelpTexts_1_326, D_0046D898, gHelpTexts_1_328, D_0046D898,
    gHelpTexts_1_330, gHelpTexts_1_331, gHelpTexts_1_332, gHelpTexts_1_333,
    gHelpTexts_1_334, gHelpTexts_1_335, gHelpTexts_1_336, gHelpTexts_1_337,
    gHelpTexts_1_338, gHelpTexts_1_339, gHelpTexts_1_340, gHelpTexts_1_341,
    gHelpTexts_1_342, gHelpTexts_1_343, gHelpTexts_1_344, gHelpTexts_1_345,
    gHelpTexts_1_346, gHelpTexts_1_347, gHelpTexts_1_348, gHelpTexts_1_349,
    gHelpTexts_1_350, gHelpTexts_1_351, gHelpTexts_1_352, gHelpTexts_1_353,
    gHelpTexts_1_354, gHelpTexts_1_355, gHelpTexts_1_356, gHelpTexts_1_357,
    gHelpTexts_1_358, gHelpTexts_1_359, gHelpTexts_1_360, gHelpTexts_1_361,
    gHelpTexts_1_362, gHelpTexts_1_363, gHelpTexts_1_364, gHelpTexts_1_365,
    gHelpTexts_1_366, gHelpTexts_1_367, gHelpTexts_1_368, gHelpTexts_1_369,
    gHelpTexts_1_370, gHelpTexts_1_371, gHelpTexts_1_372, gHelpTexts_1_373,
    gHelpTexts_1_374, gHelpTexts_1_375, gHelpTexts_1_376, gHelpTexts_1_377,
    gHelpTexts_1_378, gHelpTexts_1_379, gHelpTexts_1_380, gHelpTexts_1_381,
    gHelpTexts_1_382, gHelpTexts_1_383, gHelpTexts_1_384, gHelpTexts_1_385,
    gHelpTexts_1_386, gHelpTexts_1_387, gHelpTexts_1_388, gHelpTexts_1_389,
    gHelpTexts_1_390, gHelpTexts_1_391, gHelpTexts_1_392, gHelpTexts_1_393,
    gHelpTexts_1_394, gHelpTexts_1_395, gHelpTexts_1_396, gHelpTexts_1_397,
    gHelpTexts_1_398, gHelpTexts_1_399, gHelpTexts_1_400, gHelpTexts_1_401,
    gHelpTexts_1_402, gHelpTexts_1_403, gHelpTexts_1_404, gHelpTexts_1_405,
    gHelpTexts_1_406, gHelpTexts_1_407, gHelpTexts_1_408, gHelpTexts_1_409,
    gHelpTexts_1_410, gHelpTexts_1_411, gHelpTexts_1_412, gHelpTexts_1_413,
    gHelpTexts_1_414, gHelpTexts_1_415, gHelpTexts_1_416, D_004736B0,
    gHelpTexts_1_418, gHelpTexts_1_419, gHelpTexts_1_420, gHelpTexts_1_421,
    gHelpTexts_1_422, gHelpTexts_1_423, gHelpTexts_1_424, gHelpTexts_1_425,
    gHelpTexts_1_426, gHelpTexts_1_427, gHelpTexts_1_428, gHelpTexts_1_429,
    gHelpTexts_1_430, gHelpTexts_1_431, gHelpTexts_1_432, D_004736B0,
    gHelpTexts_1_434, gHelpTexts_1_435, gHelpTexts_1_436, gHelpTexts_1_437,
    gHelpTexts_1_438, gHelpTexts_1_439, gHelpTexts_1_440, gHelpTexts_1_441,
    gHelpTexts_1_442, gHelpTexts_1_443, gHelpTexts_1_444, gHelpTexts_1_445,
    gHelpTexts_1_446, gHelpTexts_1_447, gHelpTexts_1_448, gHelpTexts_1_449,
    gHelpTexts_1_450, gHelpTexts_1_451, gHelpTexts_1_452, gHelpTexts_1_453,
    gHelpTexts_1_454, gHelpTexts_1_455, gHelpTexts_1_456, gHelpTexts_1_457,
    gHelpTexts_1_458, gHelpTexts_1_459, gHelpTexts_1_460, gHelpTexts_1_461,
    gHelpTexts_1_462, gHelpTexts_1_463, gHelpTexts_1_464, gHelpTexts_1_465,
    gHelpTexts_1_466, gHelpTexts_1_467, gHelpTexts_1_468, gHelpTexts_1_469,
    gHelpTexts_1_470, gHelpTexts_1_471, gHelpTexts_1_472, gHelpTexts_1_473,
    gHelpTexts_1_474, gHelpTexts_1_475, gHelpTexts_1_476, gHelpTexts_1_477,
    gHelpTexts_1_478, gHelpTexts_1_479, D_00474A00, gHelpTexts_1_481,
    gHelpTexts_1_482, gHelpTexts_1_483, D_00474A00, gHelpTexts_1_485,
    gHelpTexts_1_486, gHelpTexts_1_487, gHelpTexts_1_488, gHelpTexts_1_489,
    gHelpTexts_1_490, gHelpTexts_1_491, gHelpTexts_1_492, gHelpTexts_1_493,
    gHelpTexts_1_494, gHelpTexts_1_495, gHelpTexts_1_496, gHelpTexts_1_497,
    gHelpTexts_1_498, gHelpTexts_1_499, gHelpTexts_1_500, gHelpTexts_1_501,
    gHelpTexts_1_502, gHelpTexts_1_503, gHelpTexts_1_504, gHelpTexts_1_505,
    gHelpTexts_1_506, gHelpTexts_1_507, gHelpTexts_1_508, gHelpTexts_1_509,
    gHelpTexts_1_510, gHelpTexts_1_511, gHelpTexts_1_512, gHelpTexts_1_513,
    gHelpTexts_1_514, gHelpTexts_1_515, gHelpTexts_1_516, gHelpTexts_1_517,
    gHelpTexts_1_518, gHelpTexts_1_519, gHelpTexts_1_520, gHelpTexts_1_521,
    gHelpTexts_1_522, gHelpTexts_1_523, gHelpTexts_1_524, gHelpTexts_1_525,
    gHelpTexts_1_526, D_0046D898, D_0046D898, gHelpTexts_1_529,
    gHelpTexts_1_530, gHelpTexts_1_531, gHelpTexts_1_532, gHelpTexts_1_533,
    gHelpTexts_1_534, gHelpTexts_1_535, gHelpTexts_1_536, gHelpTexts_1_537,
    gHelpTexts_1_538, gHelpTexts_1_539, gHelpTexts_1_540, gHelpTexts_1_541,
    D_0046D898, gHelpTexts_1_543, gHelpTexts_1_544, gHelpTexts_1_545,
    gHelpTexts_1_546, gHelpTexts_1_547, gHelpTexts_1_548, gHelpTexts_1_549,
    gHelpTexts_1_550, gHelpTexts_1_551, gHelpTexts_1_552, gHelpTexts_1_553,
    gHelpTexts_1_554, gHelpTexts_1_555, D_0046D898, gHelpTexts_1_557,
    gHelpTexts_1_558, gHelpTexts_1_559, gHelpTexts_1_560, gHelpTexts_1_561,
    gHelpTexts_1_562, gHelpTexts_1_563, D_0046D898, gHelpTexts_1_565,
    gHelpTexts_1_566, gHelpTexts_1_567, gHelpTexts_1_568, D_0046D898,
    gHelpTexts_1_570, gHelpTexts_1_571, gHelpTexts_1_572, gHelpTexts_1_573,
    gHelpTexts_1_574, gHelpTexts_1_575, gHelpTexts_1_576, gHelpTexts_1_577,
    gHelpTexts_1_578, gHelpTexts_1_579, gHelpTexts_1_580, gHelpTexts_1_581,
    gHelpTexts_1_582, gHelpTexts_1_583, gHelpTexts_1_584, gHelpTexts_1_585,
    gHelpTexts_1_586, gHelpTexts_1_587, gHelpTexts_1_588, gHelpTexts_1_589,
    D_0046D898, gHelpTexts_1_591, gHelpTexts_1_592, gHelpTexts_1_593,
    gHelpTexts_1_594, gHelpTexts_1_595, gHelpTexts_1_596, gHelpTexts_1_597,
    gHelpTexts_1_598, gHelpTexts_1_599, gHelpTexts_1_600, gHelpTexts_1_601,
    D_0046D898, gHelpTexts_1_603, gHelpTexts_1_604, gHelpTexts_1_605,
    gHelpTexts_1_606, gHelpTexts_1_607, gHelpTexts_1_608, gHelpTexts_1_609,
    gHelpTexts_1_610, gHelpTexts_1_611, gHelpTexts_1_612, gHelpTexts_1_613,
    gHelpTexts_1_614, D_0046D898, gHelpTexts_1_616, gHelpTexts_1_617,
    gHelpTexts_1_618, gHelpTexts_1_619, gHelpTexts_1_620, gHelpTexts_1_621,
    gHelpTexts_1_622, gHelpTexts_1_623, gHelpTexts_1_624, gHelpTexts_1_625,
    gHelpTexts_1_626, gHelpTexts_1_627, gHelpTexts_1_628, gHelpTexts_1_629,
    gHelpTexts_1_630, gHelpTexts_1_631, gHelpTexts_1_632, D_00477470,
    D_00477470, D_00477470, D_00477470, D_00477470,
    D_004774D0, D_004774D0, D_004774D0, D_004774D0,
    D_004774D0, gHelpTexts_1_643, gHelpTexts_1_644, gHelpTexts_1_645,
    gHelpTexts_1_646, gHelpTexts_1_647, gHelpTexts_1_648, gHelpTexts_1_649,
    gHelpTexts_1_650, gHelpTexts_1_651, gHelpTexts_1_652, gHelpTexts_1_653,
    D_004778D0, D_004778D0, gHelpTexts_1_656, gHelpTexts_1_657,
    gHelpTexts_1_658, D_00477A30, D_00477A30, gHelpTexts_1_661,
    gHelpTexts_1_662, gHelpTexts_1_663, gHelpTexts_1_664, gHelpTexts_1_665,
    gHelpTexts_1_666, gHelpTexts_1_667, gHelpTexts_1_668, gHelpTexts_1_669,
    gHelpTexts_1_670, gHelpTexts_1_671, gHelpTexts_1_672, D_0046D898,
    D_0046D898, D_0046D898, D_0046D898, gHelpTexts_1_677,
    gHelpTexts_1_678, gHelpTexts_1_679, gHelpTexts_1_680, gHelpTexts_1_681,
    gHelpTexts_1_682, gHelpTexts_1_683, gHelpTexts_1_684, gHelpTexts_1_685,
    gHelpTexts_1_686, gHelpTexts_1_687, gHelpTexts_1_688, gHelpTexts_1_689,
    gHelpTexts_1_690, gHelpTexts_1_691, gHelpTexts_1_692, gHelpTexts_1_693,
    gHelpTexts_1_694, gHelpTexts_1_695, gHelpTexts_1_696, gHelpTexts_1_697,
    gHelpTexts_1_698, gHelpTexts_1_699, gHelpTexts_1_700, gHelpTexts_1_701,
    gHelpTexts_1_702, gHelpTexts_1_703, gHelpTexts_1_704, gHelpTexts_1_705,
    gHelpTexts_1_706, gHelpTexts_1_707, gHelpTexts_1_708, gHelpTexts_1_709,
    gHelpTexts_1_710, gHelpTexts_1_711, gHelpTexts_1_712, gHelpTexts_1_713,
    gHelpTexts_1_714, gHelpTexts_1_715, gHelpTexts_1_716, gHelpTexts_1_717,
    gHelpTexts_1_718, D_0046D898, gHelpTexts_1_720, gHelpTexts_1_721,
    gHelpTexts_1_722, D_00478A60, D_00478A60, D_00478A60,
    D_00478A60, gHelpTexts_1_727, gHelpTexts_1_728, gHelpTexts_1_729,
    gHelpTexts_1_730, gHelpTexts_1_731, D_00478C10, D_00478C10,
    D_00478C10, D_00478C10, D_00478C10, D_00478C10,
    gHelpTexts_1_738, gHelpTexts_1_739, gHelpTexts_1_740, gHelpTexts_1_741,
    gHelpTexts_1_742, gHelpTexts_1_743, gHelpTexts_1_744, gHelpTexts_1_745,
    gHelpTexts_1_746, gHelpTexts_1_747, gHelpTexts_1_748, gHelpTexts_1_749,
    gHelpTexts_1_750, gHelpTexts_1_751, gHelpTexts_1_752, gHelpTexts_1_753,
    gHelpTexts_1_754, gHelpTexts_1_755, gHelpTexts_1_756, gHelpTexts_1_757,
    gHelpTexts_1_758, gHelpTexts_1_759, gHelpTexts_1_760, gHelpTexts_1_761,
    gHelpTexts_1_762, gHelpTexts_1_763, gHelpTexts_1_764, gHelpTexts_1_765,
    gHelpTexts_1_766, gHelpTexts_1_767, gHelpTexts_1_768, gHelpTexts_1_769,
    gHelpTexts_1_770, gHelpTexts_1_771, gHelpTexts_1_772, gHelpTexts_1_773,
    gHelpTexts_1_774, gHelpTexts_1_775, gHelpTexts_1_776, gHelpTexts_1_777,
    gHelpTexts_1_778, gHelpTexts_1_779, gHelpTexts_1_780, gHelpTexts_1_781,
    gHelpTexts_1_782, gHelpTexts_1_783, gHelpTexts_1_784, gHelpTexts_1_785,
    gHelpTexts_1_786, gHelpTexts_1_787, gHelpTexts_1_788, gHelpTexts_1_789,
    gHelpTexts_1_790, gHelpTexts_1_791, gHelpTexts_1_792, gHelpTexts_1_793,
    gHelpTexts_1_794, gHelpTexts_1_795, gHelpTexts_1_796, gHelpTexts_1_797,
    gHelpTexts_1_798, gHelpTexts_1_799, gHelpTexts_1_800, gHelpTexts_1_801,
    gHelpTexts_1_802, gHelpTexts_1_803, gHelpTexts_1_804, gHelpTexts_1_805,
    gHelpTexts_1_806, gHelpTexts_1_807, gHelpTexts_1_808, gHelpTexts_1_809,
    gHelpTexts_1_810, gHelpTexts_1_811, gHelpTexts_1_812, gHelpTexts_1_813,
    gHelpTexts_1_814, gHelpTexts_1_815, D_0047A390, D_0047A390,
    gHelpTexts_1_818, gHelpTexts_1_819, gHelpTexts_1_820, D_0047A460,
    D_0047A480, D_0047A480, D_0047A4A0, D_0047A4C0,
    D_0047A4E0, D_0047A4E0, gHelpTexts_1_828, gHelpTexts_1_829,
    gHelpTexts_1_830, D_0047A570, D_0047A590, D_0047A5B0,
    gHelpTexts_1_834, D_0047A5F0, D_0047A5F0, gHelpTexts_1_837,
    D_0047A630, D_0047A660, gHelpTexts_1_840, D_0047A6B0,
    gHelpTexts_1_842, D_0047A700, gHelpTexts_1_844, gHelpTexts_1_845,
    gHelpTexts_1_846, gHelpTexts_1_847, D_0047A7E0, gHelpTexts_1_849,
    D_0047A830, gHelpTexts_1_851, D_0047A870, gHelpTexts_1_853,
    gHelpTexts_1_854, gHelpTexts_1_855, gHelpTexts_1_856, gHelpTexts_1_857,
    D_0047A9A0, gHelpTexts_1_859, gHelpTexts_1_860, gHelpTexts_1_861,
    gHelpTexts_1_862, D_0047AAE0, D_0047AB20, D_0047AB60,
    D_0047ABA0, D_0047ABE0, gHelpTexts_1_868, gHelpTexts_1_869,
    gHelpTexts_1_870, D_0047ACB0, D_0047ACE0, D_0047AD00,
    D_0047AD30, D_0047AD50, D_0047AD80, D_0047ADB0,
    D_0047ADE0, D_0047AE10, D_0047AE40, D_0047AE70,
    D_0047AE70, D_0047AE90, D_0047AEC0, D_0047AEF0,
    D_0047AEF0, gHelpTexts_1_887, D_0047AF40, gHelpTexts_1_889,
    gHelpTexts_1_890, gHelpTexts_1_891, D_0047AFD0, gHelpTexts_1_893,
    gHelpTexts_1_894, gHelpTexts_1_895, D_0047A460, D_0047A4A0,
    D_0047A4A0, D_0047A4C0, D_0047A570, D_0047A590,
    D_0047A5B0, D_0047A5F0, D_0047A630, D_0047A660,
    D_0047A6B0, D_0047A7E0, D_0047A830, D_0047A870,
    D_0047A9A0, D_0047AAE0, D_0047AB20, D_0047AB60,
    D_0047ABA0, D_0047ABE0, gHelpTexts_1_916, gHelpTexts_1_917,
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
    gBladeDescTexts_19, gBladeDescTexts_20, gBladeDescTexts_21, NULL,
    D_0047B150, D_0047B170, D_0047B190, NULL,
    D_0047B260, D_0047B280, D_0047B2A0, NULL,
    gBladeDescTexts_31, gBladeDescTexts_32, gBladeDescTexts_33, NULL,
    gBladeDescTexts_35, gBladeDescTexts_36, gBladeDescTexts_37, NULL,
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
    gBladeDescTexts_135, gBladeDescTexts_136, gBladeDescTexts_137, NULL,
    gBladeDescTexts_139, gBladeDescTexts_140, gBladeDescTexts_141, NULL,
    gBladeDescTexts_143, gBladeDescTexts_144, gBladeDescTexts_145, NULL,
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
    gBladeDescTexts_187, gBladeDescTexts_188, gBladeDescTexts_189, NULL,
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
    gBladeDescTexts_239, gBladeDescTexts_240, gBladeDescTexts_241, NULL,
    D_0047BE40, D_0047BE60, D_0047BE80, NULL,
    D_0047BEA0, D_0047BEC0, D_0047BEE0, NULL,
    gBladeDescTexts_251, gBladeDescTexts_252, gBladeDescTexts_253, NULL,
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
    D_0047C3A0, D_0047BD60, gBladeDescTexts_329, NULL,
    D_0047C460, D_0047C480, D_0047C4A0, NULL,
    D_0047C460, D_0047C480, D_0047C4A0, NULL,
    gBladeDescTexts_339, gBladeDescTexts_340, gBladeDescTexts_341, NULL,
    D_0047C260, D_0047C280, D_0047C2A0, NULL,
    D_0047C1A0, D_0047C1C0, D_0047C1E0, NULL,
    D_0047C520, D_0047C540, D_0047C560, NULL,
    D_0047C580, D_0047C5A0, D_0047C5C0, NULL,
    D_0047C5E0, D_0047C600, D_0047C620, NULL,
    D_0047C640, D_0047C660, D_0047C680, NULL,
    D_0047C640, D_0047C660, D_0047C680, NULL,
    D_0047C6A0, gBladeDescTexts_372, D_0047C6E0, NULL,
    D_0047C700, gBladeDescTexts_376, D_0047C740, NULL,
    D_0047C760, D_0047C780, D_0047BAE0, NULL,
    D_0047C760, D_0047C780, D_0047BAE0, NULL,
    D_0047C7A0, D_0047C7C0, D_0047C7E0, NULL,
    gBladeDescTexts_391, gBladeDescTexts_392, gBladeDescTexts_393, NULL,
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
    gBladeDescTexts_443, gBladeDescTexts_444, gBladeDescTexts_445, NULL,
    gBladeDescTexts_447, gBladeDescTexts_448, gBladeDescTexts_449, NULL,
    D_0047CA50, D_0047CA70, D_0047CA90, NULL,
    D_0047CB80, D_0047CBA0, D_0047CBC0, NULL,
    D_0047CBF0, D_0047CC10, D_0047CC30, NULL,
    D_0047CC50, D_0047CC70, D_0047CC90, NULL,
    D_0047CBF0, D_0047CC10, D_0047CC30, NULL,
    D_0047C5E0, D_0047C600, D_0047CCB0, NULL,
    D_0047C5E0, D_0047C600, D_0047CCB0, NULL,
    D_0047CCD0, D_0047CCF0, D_0047CD10, NULL,
    D_0047CD30, D_0047CD50, gBladeDescTexts_485, NULL,
    D_0047CCD0, D_0047CCF0, D_0047CD10, NULL,
    D_0047B980, gBladeDescTexts_492, D_0047B9C0, NULL,
    D_0047CD30, D_0047CD50, gBladeDescTexts_497, NULL,
    gBladeDescTexts_499, gBladeDescTexts_500, gBladeDescTexts_501, NULL,
    D_0047CE30, D_0047CE50, gBladeDescTexts_505, NULL,
    D_0047CEA0, D_0047C910, D_0047C930, NULL,
    D_0047CEA0, D_0047C910, D_0047C930, NULL,
    D_0047CEC0, D_0047CEE0, D_0047C3C0, NULL,
    D_0047CF00, D_0047CF20, gBladeDescTexts_521, NULL,
    D_0047CEC0, D_0047CEE0, D_0047B720, NULL,
    gBladeDescTexts_527, gBladeDescTexts_528, gBladeDescTexts_529, NULL,
    D_0047CFD0, D_0047CFF0, D_0047D010, NULL,
    D_0047D030, D_0047D050, D_0047D070, NULL,
    gBladeDescTexts_539, D_0047D0B0, D_0047D0D0, NULL,
    D_0047D0F0, D_0047D110, D_0047D130, NULL,
    D_0047D150, D_0047D170, gBladeDescTexts_549, NULL,
    D_0047D1C0, D_0047D1E0, D_0047D210, NULL,
    D_0047D230, D_0047D250, D_0047D270, NULL,
    D_0047D290, D_0047D2B0, D_0047D2D0, NULL,
    D_0047D230, D_0047D250, D_0047D270, NULL,
    D_0047D2F0, D_0047D310, D_0047D330, NULL,
    gBladeDescTexts_571, gBladeDescTexts_572, gBladeDescTexts_573, NULL,
    D_0047D3B0, D_0047D3D0, D_0047D3F0, NULL,
    D_0047D410, D_0047D430, D_0047D450, NULL,
    D_0047D3B0, D_0047D3D0, D_0047D3F0, NULL,
    gBladeDescTexts_587, gBladeDescTexts_588, gBladeDescTexts_589, NULL,
    D_0047C6A0, gBladeDescTexts_592, D_0047C6E0, NULL,
    D_0047C700, gBladeDescTexts_596, D_0047C740, NULL,
    D_0047D510, D_0047D530, D_0047BAE0, NULL,
    D_0047D510, D_0047D530, D_0047BAE0, NULL,
    D_0047D410, D_0047D430, D_0047D450, NULL,
    gBladeDescTexts_611, gBladeDescTexts_612, gBladeDescTexts_613, NULL,
    D_0047D5B0, D_0047D5D0, D_0047D5F0, NULL,
    D_0047D610, D_0047D630, D_0047D650, NULL,
    D_0047D670, D_0047D690, D_0047D6B0, NULL,
    D_0047D670, D_0047D690, D_0047D6B0, NULL,
    D_0047D6D0, D_0047D6F0, gHelpTexts_1_8, NULL,
    D_0047D6D0, D_0047D6F0, gHelpTexts_1_8, NULL,
    D_0047D6D0, D_0047D6F0, gHelpTexts_1_8, NULL,
    gBladeDescTexts_643, gBladeDescTexts_644, gBladeDescTexts_645, NULL,
    gBladeDescTexts_647, gBladeDescTexts_648, gBladeDescTexts_649, NULL,
    D_0047D7D0, D_0047D7F0, D_0047D810, NULL,
    D_0047D830, D_0047D850, D_0047D870, NULL,
    gBladeDescTexts_659, gBladeDescTexts_660, gBladeDescTexts_661, NULL,
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
    gBladeDescTexts_707, gBladeDescTexts_708, gBladeDescTexts_709, NULL,
    D_0047DB90, D_0047D690, D_0047D6B0, NULL,
    D_0047DB90, D_0047D690, D_0047D6B0, NULL,
    D_0047DBB0, D_0047DBD0, D_0047B190, NULL,
    D_0047DBB0, D_0047DBD0, D_0047B190, NULL,
    gBladeDescTexts_727, gBladeDescTexts_728, gBladeDescTexts_729, NULL,
    D_0047DC50, D_0047DC70, D_0047DC90, NULL,
    gBladeDescTexts_735, gBladeDescTexts_736, gBladeDescTexts_737, NULL,
    D_0047DC50, D_0047DC70, D_0047DC90, NULL,
    gBladeDescTexts_743, gBladeDescTexts_744, gBladeDescTexts_745, NULL,
    D_0047DD70, D_0047DD90, D_0047DDB0, NULL,
    D_0047DDD0, D_0047DDF0, D_0047DE10, NULL,
    D_0047DD70, D_0047DD90, D_0047DDB0, NULL,
    gBladeDescTexts_759, gBladeDescTexts_760, gBladeDescTexts_761, NULL,
    D_0047DE90, D_0047DEB0, D_0047DED0, NULL,
    D_0047DEF0, D_0047DF10, D_0047DF30, NULL,
    D_0047DE90, D_0047DEB0, D_0047DED0, NULL,
    D_0047DEF0, D_0047DF10, D_0047DF30, NULL,
    D_0047DF50, D_0047DF70, D_0047DF90, NULL,
    D_0047DF50, D_0047DF70, D_0047DF90, NULL,
    D_0047DFB0, D_0047D530, D_0047BAE0, NULL,
    D_0047DFB0, D_0047D530, D_0047BAE0, NULL,
    D_0047DFD0, D_0047DFF0, D_0047E010, NULL,
    gBladeDescTexts_799, gBladeDescTexts_800, gBladeDescTexts_801, NULL,
    D_0047E090, D_0047E0B0, D_0047E0D0, NULL,
    D_0047E090, D_0047E0B0, D_0047E0D0, NULL,
    D_0047D5B0, D_0047D5D0, D_0047E0F0, NULL,
    D_0047C2C0, D_0047E110, D_0047E130, NULL,
    gBladeDescTexts_819, gBladeDescTexts_820, gBladeDescTexts_821, NULL,
    gBladeDescTexts_823, gBladeDescTexts_824, gBladeDescTexts_825, NULL,
    D_0047E210, D_0047E230, D_0047E250, NULL,
    D_0047E270, D_0047E290, D_0047E2B0, NULL,
    gBladeDescTexts_835, gBladeDescTexts_836, gBladeDescTexts_837, NULL,
    D_0047E320, D_0047E340, D_0047E360, NULL,
    D_0047B8C0, D_0047B8E0, D_0047B900, NULL,
    D_0047C580, D_0047C5A0, D_0047C5C0, NULL,
    D_0047C020, D_0047C040, D_0047C060, NULL,
    D_0047C5E0, D_0047C600, D_0047C620, NULL,
    D_0047E380, D_0047E3A0, D_0047E3C0, NULL,
    gBladeDescTexts_863, gBladeDescTexts_864, gBladeDescTexts_865, NULL,
    gBladeDescTexts_867, gBladeDescTexts_868, gBladeDescTexts_869, NULL,
    D_0047D0F0, D_0047D110, D_0047D130, NULL,
    D_0047D5B0, D_0047D5D0, D_0047D5F0, NULL,
    D_0047E4A0, D_0047E4C0, D_0047E4E0, NULL,
    gBladeDescTexts_883, gBladeDescTexts_884, gBladeDescTexts_885, NULL,
    D_0047D5B0, D_0047D5D0, D_0047E0F0, NULL,
    D_0047E560, D_0047E580, D_0047E5A0, NULL,
    gBladeDescTexts_895, gBladeDescTexts_896, gBladeDescTexts_897, NULL,
    D_0047E620, D_0047E640, D_0047E660, NULL,
    D_0047E670, D_0047E690, D_0047E6B0, NULL,
    D_0047E6D0, D_0047E6F0, D_0047E710, NULL,
    gBladeDescTexts_911, gBladeDescTexts_912, gBladeDescTexts_913, NULL,
    gBladeDescTexts_915, gBladeDescTexts_916, gBladeDescTexts_917, NULL,
    D_0047D610, D_0047D630, D_0047D650, NULL,
    D_0047E620, D_0047E640, D_0047E660, NULL,
    gBladeDescTexts_927, gBladeDescTexts_928, gBladeDescTexts_929, NULL,
    gBladeDescTexts_931, gBladeDescTexts_932, gBladeDescTexts_933, NULL,
    gBladeDescTexts_935, gBladeDescTexts_936, gBladeDescTexts_937, NULL,
    D_0047E900, D_0047E920, D_0047E940, NULL,
    D_0047E960, D_0047E980, D_0047E9A0, NULL,
    D_0047E9C0, D_0047E9E0, D_0047EA00, NULL,
    D_0047E960, D_0047E980, D_0047E9A0, NULL,
    D_0047EA20, D_0047EA40, D_0047EA60, NULL,
    D_0047EA20, D_0047EA40, D_0047EA60, NULL,
    D_0047EA80, D_0047EAA0, D_0047EAC0, NULL,
    D_0047C2C0, D_0047EAE0, D_0047EB00, NULL,
    D_0047EA80, D_0047EAA0, D_0047EAC0, NULL,
    D_0047C6A0, gBladeDescTexts_976, D_0047C6E0, NULL,
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
    gBladeDescTexts_1059, gBladeDescTexts_1060, gBladeDescTexts_1061, NULL,
    D_0047EEF0, D_0047EF10, D_0047EF30, NULL,
    D_0047EF50, D_0047EF70, D_0047EF90, NULL,
    D_0047C2C0, D_0047EFB0, D_0047EFD0, NULL,
    D_0047EFF0, D_0047F010, D_0047F030, NULL,
    gBladeDescTexts_1079, gBladeDescTexts_1080, gBladeDescTexts_1081, NULL,
    D_0047F0B0, D_0047F0D0, D_0047F0F0, NULL,
    D_0047F0B0, D_0047F0D0, D_0047F0F0, NULL,
    D_0047F110, D_0047F130, D_0047F150, NULL,
    D_0047E270, D_0047E290, D_0047E2B0, NULL,
    gBladeDescTexts_1099, gBladeDescTexts_1100, gBladeDescTexts_1101, NULL,
    D_0047F1C0, D_0047F1E0, D_0047F200, NULL,
    gBladeDescTexts_1107, gBladeDescTexts_1108, gBladeDescTexts_1109, NULL,
    gBladeDescTexts_1111, gBladeDescTexts_1112, gBladeDescTexts_1113, NULL,
    D_0047F1C0, D_0047F1E0, D_0047F200, NULL,
    gBladeDescTexts_1119, gBladeDescTexts_1120, gBladeDescTexts_1121, NULL,
    D_0047F340, D_0047F360, D_0047F380, NULL,
    D_0047F3A0, D_0047F360, D_0047F380, NULL,
    D_0047F3C0, D_0047F3E0, D_0047F400, NULL,
    D_0047F3C0, D_0047F3E0, D_0047F400, NULL,
    D_0047E270, D_0047E290, D_0047E2B0, NULL,
    D_0047E320, D_0047E340, D_0047E360, NULL,
    gBladeDescTexts_1147, gBladeDescTexts_1148, gBladeDescTexts_1149, NULL,
    D_0047F480, D_0047F4A0, gBladeDescTexts_1153, NULL,
    D_0047F4E0, gBladeDescTexts_1156, gBladeDescTexts_1157, NULL,
    D_0047BAA0, D_0047F540, D_0047F560, NULL,
    D_0047BAA0, D_0047F540, D_0047F560, NULL,
    D_0047F580, D_0047F5A0, D_0047F5C0, NULL,
    gBladeDescTexts_1171, gBladeDescTexts_1172, gBladeDescTexts_1173, NULL,
    D_0047F640, D_0047F660, D_0047F680, NULL,
    D_0047F6A0, D_0047F6C0, D_0047F6E0, NULL,
    D_0047F700, D_0047F720, D_0047F740, NULL,
    D_0047F700, D_0047F720, D_0047F740, NULL,
    gBladeDescTexts_1191, gBladeDescTexts_1192, gBladeDescTexts_1193, NULL,
    gBladeDescTexts_1195, gBladeDescTexts_1196, gBladeDescTexts_1197, NULL,
    D_0047F820, D_0047F840, D_0047F860, NULL,
    D_0047F820, D_0047F840, D_0047F860, NULL,
    D_0047F880, D_0047F8A0, D_0047F8C0, NULL,
    gBladeDescTexts_1211, gBladeDescTexts_1212, gBladeDescTexts_1213, NULL,
    D_0047F940, D_0047F960, D_0047F980, NULL,
    gBladeDescTexts_1219, gBladeDescTexts_1220, gBladeDescTexts_1221, NULL,
    gBladeDescTexts_1223, gBladeDescTexts_1224, gBladeDescTexts_1225, NULL,
    D_0047FA60, D_0047FA80, D_0047FAA0, NULL,
    D_0047F480, D_0047F4A0, gBladeDescTexts_1233, NULL,
    D_0047FAE0, gBladeDescTexts_1236, gBladeDescTexts_1237, NULL,
    D_0047C760, D_0047FB40, D_0047F560, NULL,
    D_0047C760, D_0047FB40, D_0047F560, NULL,
    D_0047FA60, D_0047FA80, D_0047FAA0, NULL,
    gBladeDescTexts_1251, gBladeDescTexts_1252, gBladeDescTexts_1253, NULL,
    D_0047FBC0, D_0047FBE0, D_0047FC00, NULL,
    D_0047FC20, D_0047FC40, D_0047FC60, NULL,
    D_0047FC80, D_0047FCA0, D_0047FCC0, NULL,
    D_0047FCE0, D_0047FD00, D_0047FD20, NULL,
    gBladeDescTexts_1271, gBladeDescTexts_1272, gBladeDescTexts_1273, NULL,
    gBladeDescTexts_1275, gBladeDescTexts_1276, gBladeDescTexts_1277, NULL,
    gBladeDescTexts_1279, gBladeDescTexts_1280, gBladeDescTexts_1281, NULL,
    D_0047C2C0, D_0047FE60, D_0047FE80, NULL,
    gBladeDescTexts_1287, gBladeDescTexts_1288, gBladeDescTexts_1289, NULL,
    gBladeDescTexts_1291, gBladeDescTexts_1292, gBladeDescTexts_1293, NULL,
    D_0047FF60, D_0047FF80, D_0047FFA0, NULL,
    D_0047FF60, D_0047FF80, D_0047FFA0, NULL,
    D_0047FFC0, D_0047FFE0, D_00480000, NULL,
    D_0047FFC0, D_0047FFE0, D_00480000, NULL,
    D_00480020, D_00480040, D_00480060, NULL,
    D_00480080, D_004800A0, D_004800C0, NULL,
    D_0047F480, gBladeDescTexts_1320, gBladeDescTexts_1321, NULL,
    D_0047F4E0, gBladeDescTexts_1324, gBladeDescTexts_1325, NULL,
    D_0047D510, D_00480160, D_0047F560, NULL,
    D_0047D510, D_00480160, D_0047F560, NULL,
    D_00480180, D_004801A0, D_004801C0, NULL,
    gBladeDescTexts_1339, gBladeDescTexts_1340, gBladeDescTexts_1341, NULL,
    D_00480240, D_00480260, D_00480280, NULL,
    D_00480080, D_004800A0, D_004800C0, NULL,
    D_004802A0, D_004802C0, D_004802E0, NULL,
    D_004802A0, D_004802C0, D_004802E0, NULL,
    D_0047E560, D_0047E580, D_0047E5A0, NULL,
    gBladeDescTexts_1363, gBladeDescTexts_1364, gBladeDescTexts_1365, NULL,
    gBladeDescTexts_1367, gBladeDescTexts_1368, gBladeDescTexts_1369, NULL,
    D_004803C0, D_004803E0, D_00480400, NULL,
    gBladeDescTexts_1375, gBladeDescTexts_1376, gBladeDescTexts_1377, NULL,
    gBladeDescTexts_1379, gBladeDescTexts_1380, gBladeDescTexts_1381, NULL,
    D_004804E0, D_00480500, D_00480520, NULL,
    D_00480540, D_00480560, D_00480580, NULL,
    D_004805A0, D_004805C0, D_004805E0, NULL,
    gBladeDescTexts_1395, gBladeDescTexts_1396, gBladeDescTexts_1397, NULL,
    D_00480660, D_00480680, D_004806A0, NULL,
    D_00480660, D_00480680, D_004806A0, NULL,
    D_004806C0, D_004806E0, D_00480700, NULL,
    D_0047F480, D_0047F4A0, gBladeDescTexts_1413, NULL,
    D_0047F4E0, gBladeDescTexts_1416, gBladeDescTexts_1417, NULL,
    D_0047E670, D_0047E690, D_0047E6B0, NULL,
    D_0047D830, D_0047D850, D_0047D870, NULL,
    gBladeDescTexts_1427, gBladeDescTexts_1428, gBladeDescTexts_1429, NULL,
    D_0047EB40, D_0047EB60, D_0047EB80, NULL,
    gBladeDescTexts_1435, gBladeDescTexts_1436, gBladeDescTexts_1437, NULL,
    gBladeDescTexts_1439, gBladeDescTexts_1440, gBladeDescTexts_1441, NULL,
    gBladeDescTexts_1443, D_004808C0, gBladeDescTexts_1445, NULL,
    D_00480900, D_0047F540, D_0047BAE0, NULL,
    D_00480900, D_0047F540, D_0047BAE0, NULL,
    D_004805A0, D_004805C0, D_004805E0, NULL,
    gBladeDescTexts_1459, gBladeDescTexts_1460, gBladeDescTexts_1461, NULL,
    D_0047F3A0, D_0047F360, D_0047F380, NULL,
    D_0047F340, D_0047F360, D_0047F380, NULL,
    D_0047EE30, D_0047EE50, D_0047EE70, NULL,
    D_0047C080, D_0047C0A0, D_0047C0C0, NULL,
    gBladeDescTexts_1479, gBladeDescTexts_1480, gBladeDescTexts_1481, NULL,
    D_004809E0, D_00480A00, D_00480A20, NULL,
    D_00480A40, D_00480A60, D_00480A80, NULL,
    D_00480A40, D_00480A60, D_00480A80, NULL,
    gBladeDescTexts_1495, gBladeDescTexts_1496, gBladeDescTexts_1497, NULL,
    D_00480B00, D_00480B20, D_00480B40, NULL,
    D_0047E6D0, D_0047E6F0, D_0047E710, NULL,
    D_00480B60, D_00480B80, D_00480BA0, NULL,
    D_00480BC0, D_00480BE0, D_00480C00, NULL,
    gBladeDescTexts_1515, gBladeDescTexts_1516, gBladeDescTexts_1517, NULL,
    D_00480C80, D_00480CA0, D_00480CC0, NULL,
    D_00480CE0, gBladeDescTexts_1524, gBladeDescTexts_1525, NULL,
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
    D_0047CA50, D_0047CA70, gBladeDescTexts_1609, NULL,
    D_0047D030, D_0047D050, D_0047D070, NULL,
    D_0047CFD0, D_0047CFF0, D_0047D010, NULL,
    D_0047CE30, D_0047CE50, gBladeDescTexts_1621, NULL,
    D_0047CD30, D_0047CD50, gBladeDescTexts_1625, NULL,
    D_0047D150, D_0047D170, gBladeDescTexts_1629, NULL,
    D_0047CF00, D_0047CF20, gBladeDescTexts_1633, NULL,
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
    gBladeDescTexts_1735, gBladeDescTexts_1736, gBladeDescTexts_1737, NULL,
    gBladeDescTexts_1739, gBladeDescTexts_1740, gBladeDescTexts_1741, NULL,
    gBladeDescTexts_1743, gBladeDescTexts_1744, gBladeDescTexts_1745, NULL,
    D_0047C140, D_0047C160, D_0047C180, NULL,
    gBladeDescTexts_1751, gBladeDescTexts_1752, gBladeDescTexts_1753, NULL,
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
    gBladeDescTexts_1847, gBladeDescTexts_1848, gBladeDescTexts_1849, NULL,
    D_0047E900, D_0047E920, D_0047E940, NULL,
    gBladeDescTexts_1855, gBladeDescTexts_1856, gBladeDescTexts_1857, NULL,
    D_00480C80, D_00480CA0, D_00480CC0, NULL,
    D_0047F0B0, D_0047F0D0, D_0047F0F0, NULL,
    D_0047CC50, D_0047CC70, gBladeDescTexts_1869, NULL,
    D_0047D8F0, D_0047D910, D_0047D930, NULL,
    D_004806C0, D_004806E0, D_00480700, NULL,
    D_0047FC80, D_0047FCA0, D_0047FCC0, NULL,
    D_004808C0, gBladeDescTexts_1884, gBladeDescTexts_1885, NULL,
    gBladeDescTexts_1887, gBladeDescTexts_1888, gBladeDescTexts_1889, NULL,
    D_0047D5B0, D_0047D5D0, D_00481120, NULL,
    gBladeDescTexts_1895, gBladeDescTexts_1896, gBladeDescTexts_1897, NULL,
    D_0047D5B0, D_0047D5D0, D_00481120, NULL,
    D_004811A0, D_004811C0, D_004811E0, NULL,
    D_0047C320, D_0047C340, D_0047C360, NULL,
    D_0047B5B0, D_0047B5D0, D_0047B5F0, NULL,
    D_0047BB60, gHelpTexts_1_8, gHelpTexts_1_8, NULL,
    D_0047DDD0, D_0047DDF0, D_0047DE10, NULL,
    D_004803C0, D_004803E0, D_00480400, NULL,
    D_004811A0, D_004811C0, D_004811E0, NULL,
    D_0047FF60, D_0047FF80, D_0047FFA0, NULL,
    D_00480CE0, gBladeDescTexts_1936, gBladeDescTexts_1937, NULL,
    gBladeDescTexts_1939, D_0047D0B0, D_0047D0D0, NULL,
};
char *gGunDescTexts[128][4] = {
    gGunDescTexts_0, NULL, NULL, NULL,
    gGunDescTexts_4, gGunDescTexts_5, gGunDescTexts_6, NULL,
    gGunDescTexts_8, gGunDescTexts_9, gGunDescTexts_10, NULL,
    gGunDescTexts_12, gGunDescTexts_13, gGunDescTexts_14, NULL,
    D_004813A0, gGunDescTexts_17, gGunDescTexts_18, NULL,
    D_004813A0, gGunDescTexts_21, gGunDescTexts_22, NULL,
    gGunDescTexts_24, gGunDescTexts_25, gGunDescTexts_26, NULL,
    gGunDescTexts_28, D_004814C0, gGunDescTexts_30, NULL,
    gGunDescTexts_32, D_004814C0, gGunDescTexts_34, NULL,
    gGunDescTexts_36, gGunDescTexts_37, gGunDescTexts_38, NULL,
    gGunDescTexts_40, gGunDescTexts_41, gGunDescTexts_42, NULL,
    gGunDescTexts_44, gGunDescTexts_45, gGunDescTexts_46, NULL,
    gGunDescTexts_48, gGunDescTexts_49, D_00481690, NULL,
    gGunDescTexts_52, gGunDescTexts_53, gGunDescTexts_54, NULL,
    gGunDescTexts_56, gGunDescTexts_57, gGunDescTexts_58, NULL,
    gGunDescTexts_60, gGunDescTexts_61, gGunDescTexts_62, NULL,
    gGunDescTexts_64, gGunDescTexts_65, gGunDescTexts_66, NULL,
    gGunDescTexts_68, gGunDescTexts_69, gGunDescTexts_70, NULL,
    gGunDescTexts_72, gGunDescTexts_73, gGunDescTexts_74, NULL,
    gGunDescTexts_76, gGunDescTexts_77, gGunDescTexts_78, NULL,
    D_0047C2C0, gGunDescTexts_81, gGunDescTexts_82, NULL,
    gGunDescTexts_84, gGunDescTexts_85, gGunDescTexts_86, NULL,
    gGunDescTexts_88, gGunDescTexts_89, gGunDescTexts_90, NULL,
    gGunDescTexts_92, gGunDescTexts_93, D_00481690, NULL,
    gGunDescTexts_96, gGunDescTexts_97, gGunDescTexts_98, NULL,
    gGunDescTexts_100, gGunDescTexts_101, gGunDescTexts_102, NULL,
    gGunDescTexts_104, D_00481B60, gGunDescTexts_106, NULL,
    gGunDescTexts_108, gGunDescTexts_109, gGunDescTexts_110, NULL,
    gGunDescTexts_112, gGunDescTexts_113, gGunDescTexts_114, NULL,
    gGunDescTexts_116, D_00481C90, D_00481CB0, NULL,
    D_00481CD0, gGunDescTexts_121, gGunDescTexts_122, NULL,
    D_00481D30, gGunDescTexts_125, gGunDescTexts_126, NULL,
    gGunDescTexts_128, gGunDescTexts_129, gGunDescTexts_130, NULL,
    gGunDescTexts_132, D_00481C90, D_00481CB0, NULL,
    D_00481CD0, gGunDescTexts_137, gGunDescTexts_138, NULL,
    D_00481D30, gGunDescTexts_141, gGunDescTexts_142, NULL,
    gGunDescTexts_144, gGunDescTexts_145, gGunDescTexts_146, NULL,
    gGunDescTexts_148, gGunDescTexts_149, gGunDescTexts_150, NULL,
    gGunDescTexts_152, gGunDescTexts_153, gGunDescTexts_154, NULL,
    gGunDescTexts_156, gGunDescTexts_157, gGunDescTexts_158, NULL,
    gGunDescTexts_160, gGunDescTexts_161, gGunDescTexts_162, NULL,
    gGunDescTexts_164, gGunDescTexts_165, gGunDescTexts_166, NULL,
    gGunDescTexts_168, gGunDescTexts_169, gGunDescTexts_170, NULL,
    gGunDescTexts_172, gGunDescTexts_173, gGunDescTexts_174, NULL,
    gGunDescTexts_176, gGunDescTexts_177, gGunDescTexts_178, NULL,
    gGunDescTexts_180, gGunDescTexts_181, gGunDescTexts_182, NULL,
    gGunDescTexts_184, gGunDescTexts_185, gGunDescTexts_186, NULL,
    gGunDescTexts_188, gGunDescTexts_189, gGunDescTexts_190, NULL,
    gGunDescTexts_192, gGunDescTexts_193, gGunDescTexts_194, NULL,
    gGunDescTexts_196, gGunDescTexts_197, gGunDescTexts_198, NULL,
    gGunDescTexts_200, gGunDescTexts_201, gGunDescTexts_202, NULL,
    gGunDescTexts_204, gGunDescTexts_205, gGunDescTexts_206, NULL,
    gGunDescTexts_208, gGunDescTexts_209, gGunDescTexts_210, NULL,
    gGunDescTexts_212, gGunDescTexts_213, gGunDescTexts_214, NULL,
    gGunDescTexts_216, gGunDescTexts_217, gGunDescTexts_218, NULL,
    D_0047F480, D_0047F4A0, gGunDescTexts_222, NULL,
    D_0047FAE0, gGunDescTexts_225, gGunDescTexts_226, NULL,
    gGunDescTexts_228, gGunDescTexts_229, gGunDescTexts_230, NULL,
    gGunDescTexts_232, gGunDescTexts_233, D_004827D0, NULL,
    gGunDescTexts_236, gGunDescTexts_237, D_004827D0, NULL,
    gGunDescTexts_240, gGunDescTexts_241, gGunDescTexts_242, NULL,
    gGunDescTexts_244, gGunDescTexts_245, gGunDescTexts_246, NULL,
    gGunDescTexts_248, gGunDescTexts_249, gGunDescTexts_250, NULL,
    gGunDescTexts_252, gGunDescTexts_253, gGunDescTexts_254, NULL,
    gGunDescTexts_256, gGunDescTexts_257, gGunDescTexts_258, NULL,
    gGunDescTexts_260, gGunDescTexts_261, gGunDescTexts_262, NULL,
    gGunDescTexts_264, gGunDescTexts_265, gGunDescTexts_266, NULL,
    gGunDescTexts_268, gGunDescTexts_269, gGunDescTexts_270, NULL,
    gGunDescTexts_272, gGunDescTexts_273, gGunDescTexts_274, NULL,
    gGunDescTexts_276, gGunDescTexts_277, gGunDescTexts_278, NULL,
    gGunDescTexts_280, gGunDescTexts_281, gGunDescTexts_282, NULL,
    gGunDescTexts_284, gGunDescTexts_285, gGunDescTexts_286, NULL,
    gGunDescTexts_288, gGunDescTexts_289, gGunDescTexts_290, NULL,
    gGunDescTexts_292, gGunDescTexts_293, gGunDescTexts_294, NULL,
    gGunDescTexts_296, gGunDescTexts_297, gGunDescTexts_298, NULL,
    gGunDescTexts_300, gGunDescTexts_301, gGunDescTexts_302, NULL,
    gGunDescTexts_304, D_00481B60, gGunDescTexts_306, NULL,
    gGunDescTexts_308, gGunDescTexts_309, gGunDescTexts_310, NULL,
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
    gGunDescTexts_352, gGunDescTexts_353, gGunDescTexts_354, NULL,
    gGunDescTexts_356, gGunDescTexts_357, gGunDescTexts_358, NULL,
    gGunDescTexts_360, gGunDescTexts_361, gGunDescTexts_362, NULL,
    gGunDescTexts_364, gGunDescTexts_365, gGunDescTexts_366, NULL,
    D_0047C2C0, gGunDescTexts_369, gGunDescTexts_370, NULL,
    gGunDescTexts_372, gGunDescTexts_373, gGunDescTexts_374, NULL,
    gGunDescTexts_376, gGunDescTexts_377, gGunDescTexts_378, NULL,
    gGunDescTexts_380, gGunDescTexts_381, gGunDescTexts_382, NULL,
    gGunDescTexts_384, gGunDescTexts_385, gGunDescTexts_386, NULL,
    gGunDescTexts_388, gGunDescTexts_389, gGunDescTexts_390, NULL,
    gGunDescTexts_392, gGunDescTexts_393, gGunDescTexts_394, NULL,
    gGunDescTexts_396, gGunDescTexts_397, gGunDescTexts_398, NULL,
    gGunDescTexts_400, gGunDescTexts_401, gGunDescTexts_402, NULL,
    gGunDescTexts_404, gGunDescTexts_405, gGunDescTexts_406, NULL,
    gGunDescTexts_408, gGunDescTexts_409, gGunDescTexts_410, NULL,
    gGunDescTexts_412, gGunDescTexts_413, gGunDescTexts_414, NULL,
    gGunDescTexts_416, gGunDescTexts_417, gGunDescTexts_418, NULL,
    gGunDescTexts_420, gGunDescTexts_421, gGunDescTexts_422, NULL,
    gGunDescTexts_424, gGunDescTexts_425, gGunDescTexts_426, NULL,
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
    gGunDescTexts_488, gGunDescTexts_489, gGunDescTexts_490, NULL,
    gGunDescTexts_492, gGunDescTexts_493, gGunDescTexts_494, NULL,
    gGunDescTexts_496, gGunDescTexts_497, gGunDescTexts_498, NULL,
    gGunDescTexts_500, gGunDescTexts_501, gGunDescTexts_502, NULL,
    gGunDescTexts_504, gGunDescTexts_505, gGunDescTexts_506, NULL,
    gGunDescTexts_508, gGunDescTexts_509, gGunDescTexts_510, NULL,
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
    D_00484740, gArmorDescTexts_128, gArmorDescTexts_129, NULL,
    D_004847A0, D_004847C0, D_004847E0, NULL,
    D_004847A0, D_00484800, D_00484820, NULL,
    D_00484840, D_00484860, D_00484880, NULL,
    D_004848A0, D_004848C0, D_004848E0, NULL,
    D_00484900, D_00484920, D_00484940, NULL,
    D_00484960, D_00484980, D_004849A0, NULL,
    D_004849B0, D_004849D0, D_004849F0, NULL,
    D_004849B0, D_00484A10, D_00484A30, NULL,
    D_00484A50, D_00484A70, D_00484A90, NULL,
    gArmorDescTexts_167, gArmorDescTexts_168, NULL, NULL,
    gArmorDescTexts_171, gArmorDescTexts_172, gArmorDescTexts_173, NULL,
    gArmorDescTexts_175, gArmorDescTexts_176, gArmorDescTexts_177, NULL,
    gArmorDescTexts_179, gArmorDescTexts_180, gArmorDescTexts_181, NULL,
    gArmorDescTexts_183, gArmorDescTexts_184, gArmorDescTexts_185, NULL,
    D_00484C50, gArmorDescTexts_188, gArmorDescTexts_189, NULL,
    D_00484CB0, D_00484CD0, D_00484CF0, NULL,
    gArmorDescTexts_195, D_00484D30, gArmorDescTexts_197, NULL,
    gArmorDescTexts_199, D_00484D30, gArmorDescTexts_201, NULL,
    gArmorDescTexts_203, gArmorDescTexts_204, gArmorDescTexts_205, NULL,
    D_00484E00, gArmorDescTexts_208, gArmorDescTexts_209, NULL,
    D_00484E00, gArmorDescTexts_212, gArmorDescTexts_213, NULL,
    D_00484E00, gArmorDescTexts_216, gArmorDescTexts_217, NULL,
    D_00484E00, gArmorDescTexts_220, gArmorDescTexts_221, NULL,
    D_00484F20, D_00484F40, D_00484F60, NULL,
    D_00484F20, D_00484F40, D_00484F60, NULL,
    D_00484F80, gArmorDescTexts_232, gArmorDescTexts_233, NULL,
    D_00484F80, gArmorDescTexts_236, gArmorDescTexts_237, NULL,
    gArmorDescTexts_239, gArmorDescTexts_240, gArmorDescTexts_241, NULL,
    gArmorDescTexts_243, gArmorDescTexts_244, gArmorDescTexts_245, NULL,
    gArmorDescTexts_247, gArmorDescTexts_248, gArmorDescTexts_249, NULL,
    gArmorDescTexts_251, gArmorDescTexts_252, gArmorDescTexts_253, NULL,
    gArmorDescTexts_255, gArmorDescTexts_256, gArmorDescTexts_257, NULL,
    gArmorDescTexts_259, gArmorDescTexts_260, gArmorDescTexts_261, NULL,
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
    gArmorDescTexts_363, D_00484330, D_00484350, NULL,
    D_00484370, D_00484390, D_004843B0, NULL,
    D_00484370, D_004843D0, D_004843F0, NULL,
    D_00484410, D_00484430, D_00484450, NULL,
    D_00484410, D_00484470, D_00484490, NULL,
    gArmorDescTexts_383, gArmorDescTexts_384, gArmorDescTexts_385, NULL,
    gArmorDescTexts_387, gArmorDescTexts_388, gArmorDescTexts_389, NULL,
    D_00484570, D_00484590, NULL, NULL,
    D_00484570, D_004845A0, NULL, NULL,
    gArmorDescTexts_399, gArmorDescTexts_400, gArmorDescTexts_401, NULL,
    gArmorDescTexts_403, gArmorDescTexts_404, gArmorDescTexts_405, NULL,
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
    D_00485600, D_00485620, gArmorDescTexts_449, NULL,
    D_00485660, gArmorDescTexts_452, gArmorDescTexts_453, NULL,
    D_004856B0, gArmorDescTexts_456, gArmorDescTexts_457, NULL,
    gArmorDescTexts_459, gArmorDescTexts_460, gArmorDescTexts_461, NULL,
    D_00485770, gArmorDescTexts_464, gArmorDescTexts_465, NULL,
    D_004857C0, gArmorDescTexts_468, gArmorDescTexts_469, NULL,
    D_00485260, D_00485280, D_004852A0, NULL,
    D_00485260, D_004852C0, D_004852E0, NULL,
    D_00485300, D_00485320, D_00485340, NULL,
    D_00485300, D_00485360, D_00485380, NULL,
    gArmorDescTexts_487, gArmorDescTexts_488, gArmorDescTexts_489, NULL,
    gArmorDescTexts_491, gArmorDescTexts_492, gArmorDescTexts_493, NULL,
    gArmorDescTexts_495, gArmorDescTexts_496, gArmorDescTexts_497, NULL,
    gArmorDescTexts_499, gArmorDescTexts_500, gArmorDescTexts_501, NULL,
    gArmorDescTexts_503, gArmorDescTexts_504, gArmorDescTexts_505, NULL,
    gArmorDescTexts_507, gArmorDescTexts_508, gArmorDescTexts_509, NULL,
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
    gArmorDescTexts_583, gArmorDescTexts_584, gArmorDescTexts_585, NULL,
    gArmorDescTexts_587, gArmorDescTexts_588, gArmorDescTexts_589, NULL,
    D_004842B0, D_004842D0, D_004842F0, NULL,
    D_00484310, D_00484330, D_00484350, NULL,
    D_00484370, D_00484390, D_004843B0, NULL,
    D_00484370, D_004843D0, D_004843F0, NULL,
    D_00484410, D_00484430, D_00484450, NULL,
    D_00484410, D_00484470, D_00484490, NULL,
    D_004844B0, D_004844D0, D_004844F0, NULL,
    D_00484510, D_00484530, D_00484550, NULL,
    D_00484570, gArmorDescTexts_624, gArmorDescTexts_625, NULL,
    D_00484570, gArmorDescTexts_628, gArmorDescTexts_629, NULL,
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
    gArmorDescTexts_839, gArmorDescTexts_840, gArmorDescTexts_841, NULL,
    D_00486170, gArmorDescTexts_844, gArmorDescTexts_845, NULL,
    gArmorDescTexts_847, gArmorDescTexts_848, gArmorDescTexts_849, NULL,
    gArmorDescTexts_851, gArmorDescTexts_852, gArmorDescTexts_853, NULL,
    gArmorDescTexts_855, gArmorDescTexts_856, gArmorDescTexts_857, NULL,
    gArmorDescTexts_859, gArmorDescTexts_860, gArmorDescTexts_861, NULL,
    gArmorDescTexts_863, gArmorDescTexts_864, gArmorDescTexts_865, NULL,
    gArmorDescTexts_867, gArmorDescTexts_868, gArmorDescTexts_869, NULL,
    gArmorDescTexts_871, gArmorDescTexts_872, gArmorDescTexts_873, NULL,
    gArmorDescTexts_875, gArmorDescTexts_876, gArmorDescTexts_877, NULL,
    gArmorDescTexts_879, gArmorDescTexts_880, gArmorDescTexts_881, NULL,
    gArmorDescTexts_883, gArmorDescTexts_884, gArmorDescTexts_885, NULL,
    gArmorDescTexts_887, gArmorDescTexts_888, gArmorDescTexts_889, NULL,
    gArmorDescTexts_891, gArmorDescTexts_892, gArmorDescTexts_893, NULL,
    D_00486630, D_00486650, D_00486670, NULL,
    D_00486630, D_00486650, D_00486670, NULL,
    D_00484A50, D_00484A70, D_00484A90, NULL,
    D_00484CB0, D_00484CD0, D_00484CF0, NULL,
    gArmorDescTexts_911, gArmorDescTexts_912, gArmorDescTexts_913, NULL,
    D_00485CE0, D_00485D00, gArmorDescTexts_917, NULL,
    gArmorDescTexts_919, gArmorDescTexts_920, gArmorDescTexts_921, NULL,
    gArmorDescTexts_923, gArmorDescTexts_924, gArmorDescTexts_925, NULL,
    gArmorDescTexts_927, gArmorDescTexts_928, gArmorDescTexts_929, NULL,
    D_00484E00, gArmorDescTexts_932, gArmorDescTexts_933, NULL,
    D_00485C00, D_00485C20, D_00485C40, NULL,
    D_00485C60, D_00485C80, D_00485CA0, NULL,
    D_00485CC0, D_00485CE0, D_00485D00, NULL,
    D_00485D20, D_00485D40, D_00485D60, NULL,
    gArmorDescTexts_951, gArmorDescTexts_952, gArmorDescTexts_953, NULL,
    gArmorDescTexts_955, gArmorDescTexts_956, gArmorDescTexts_957, NULL,
    gArmorDescTexts_959, gArmorDescTexts_960, gArmorDescTexts_961, NULL,
    gArmorDescTexts_963, gArmorDescTexts_964, gArmorDescTexts_965, NULL,
    gArmorDescTexts_967, gArmorDescTexts_968, gArmorDescTexts_969, NULL,
    gArmorDescTexts_971, gArmorDescTexts_972, gArmorDescTexts_973, NULL,
    gArmorDescTexts_975, gArmorDescTexts_976, gArmorDescTexts_977, NULL,
    gArmorDescTexts_979, gArmorDescTexts_980, gArmorDescTexts_981, NULL,
    gArmorDescTexts_983, gArmorDescTexts_984, gArmorDescTexts_985, NULL,
    gArmorDescTexts_987, gArmorDescTexts_988, gArmorDescTexts_989, NULL,
    gArmorDescTexts_991, gArmorDescTexts_992, gArmorDescTexts_993, NULL,
    gArmorDescTexts_995, gArmorDescTexts_996, gArmorDescTexts_997, NULL,
    gArmorDescTexts_999, gArmorDescTexts_1000, gArmorDescTexts_1001, NULL,
    gArmorDescTexts_1003, gArmorDescTexts_1004, gArmorDescTexts_1005, NULL,
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
    D_00484740, gArmorDescTexts_1152, gArmorDescTexts_1153, NULL,
    gArmorDescTexts_1155, gArmorDescTexts_1156, gArmorDescTexts_1157, NULL,
    D_004847A0, D_00487980, D_004879A0, NULL,
    D_004847A0, D_004879C0, D_004879E0, NULL,
    D_00487A00, D_00487A20, D_00487A40, NULL,
    D_00487A60, D_00487A80, D_00487AA0, NULL,
    D_00487AC0, D_00487AE0, D_00487B00, NULL,
    D_00484960, D_00487B20, D_00487B40, NULL,
    D_004849B0, D_00487B60, D_00487B80, NULL,
    D_00487BA0, D_00487BC0, D_00487BE0, NULL,
    D_00484A50, D_00487C00, D_00484A90, NULL,
    gArmorDescTexts_1195, gArmorDescTexts_1196, gArmorDescTexts_1197, NULL,
    gArmorDescTexts_1199, gArmorDescTexts_1200, gArmorDescTexts_1201, NULL,
    gArmorDescTexts_1203, gArmorDescTexts_1204, gArmorDescTexts_1205, NULL,
    D_00484C50, gArmorDescTexts_1208, gArmorDescTexts_1209, NULL,
    D_00487D70, D_00487228, gArmorDescTexts_1213, NULL,
    D_00487DB0, gArmorDescTexts_1216, gArmorDescTexts_1217, NULL,
    D_00487E10, gArmorDescTexts_1220, gArmorDescTexts_1221, NULL,
    D_00487E70, D_00487E90, D_00487EB0, NULL,
    D_00487E70, D_00487E90, D_00487EB0, NULL,
    D_00484F80, D_00487ED0, D_00487EF0, NULL,
    D_00484F80, D_00487ED0, D_00487EF0, NULL,
    gArmorDescTexts_1239, gArmorDescTexts_1240, gArmorDescTexts_1241, NULL,
    D_00487F70, gArmorDescTexts_1244, gArmorDescTexts_1245, NULL,
    gArmorDescTexts_1247, gArmorDescTexts_1248, gArmorDescTexts_1249, NULL,
    gArmorDescTexts_1251, gArmorDescTexts_1252, gArmorDescTexts_1253, NULL,
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
    gArmorDescTexts_1363, D_00487450, D_00487470, NULL,
    D_00487490, D_004874B0, D_004874D0, NULL,
    D_00487490, D_004874F0, D_00487510, NULL,
    D_00484410, D_00487520, D_00487540, NULL,
    D_00487560, D_00487580, D_004875A0, NULL,
    D_004875C0, D_004875E0, D_00487600, NULL,
    gArmorDescTexts_1387, gArmorDescTexts_1388, gArmorDescTexts_1389, NULL,
    gArmorDescTexts_1391, gArmorDescTexts_1392, gArmorDescTexts_1393, NULL,
    D_004876C0, D_004876E0, D_00487700, NULL,
    D_004876C0, D_00487720, D_00487740, NULL,
    gArmorDescTexts_1403, gArmorDescTexts_1404, gArmorDescTexts_1405, NULL,
    gArmorDescTexts_1407, gArmorDescTexts_1408, gArmorDescTexts_1409, NULL,
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
    D_00485600, D_00485620, gArmorDescTexts_1453, NULL,
    D_00485660, gArmorDescTexts_1456, gArmorDescTexts_1457, NULL,
    gArmorDescTexts_1459, gArmorDescTexts_1460, gArmorDescTexts_1461, NULL,
    D_004856B0, gArmorDescTexts_1464, gArmorDescTexts_1465, NULL,
    gArmorDescTexts_1467, gArmorDescTexts_1468, gArmorDescTexts_1469, NULL,
    D_004857C0, gArmorDescTexts_1472, gArmorDescTexts_1473, NULL,
    D_00485260, D_00488080, D_004880A0, NULL,
    D_00485260, D_004880C0, D_004880E0, NULL,
    D_00485300, D_00488100, D_00488120, NULL,
    D_00485300, D_00488140, D_00488160, NULL,
    gArmorDescTexts_1491, gArmorDescTexts_1492, gArmorDescTexts_1493, NULL,
    gArmorDescTexts_1495, gArmorDescTexts_1496, gArmorDescTexts_1497, NULL,
    gArmorDescTexts_1499, gArmorDescTexts_1500, gArmorDescTexts_1501, NULL,
    gArmorDescTexts_1503, gArmorDescTexts_1504, gArmorDescTexts_1505, NULL,
    gArmorDescTexts_1507, gArmorDescTexts_1508, gArmorDescTexts_1509, NULL,
    gArmorDescTexts_1511, gArmorDescTexts_1512, gArmorDescTexts_1513, NULL,
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
    gArmorDescTexts_1847, gArmorDescTexts_1848, gArmorDescTexts_1849, NULL,
    gArmorDescTexts_1851, gArmorDescTexts_1852, gArmorDescTexts_1853, NULL,
    D_00488D90, gArmorDescTexts_1856, gArmorDescTexts_1857, NULL,
    D_00488D90, gArmorDescTexts_1860, gArmorDescTexts_1861, NULL,
    gArmorDescTexts_1863, gArmorDescTexts_1864, gArmorDescTexts_1865, NULL,
    gArmorDescTexts_1867, gArmorDescTexts_1868, gArmorDescTexts_1869, NULL,
    gArmorDescTexts_1871, gArmorDescTexts_1872, gArmorDescTexts_1873, NULL,
    gArmorDescTexts_1875, gArmorDescTexts_1876, gArmorDescTexts_1877, NULL,
    gArmorDescTexts_1879, gArmorDescTexts_1880, gArmorDescTexts_1881, NULL,
    gArmorDescTexts_1883, gArmorDescTexts_1884, gArmorDescTexts_1885, NULL,
    gArmorDescTexts_1887, gArmorDescTexts_1888, gArmorDescTexts_1889, NULL,
    gArmorDescTexts_1891, gArmorDescTexts_1892, gArmorDescTexts_1893, NULL,
    gArmorDescTexts_1895, gArmorDescTexts_1896, gArmorDescTexts_1897, NULL,
    gArmorDescTexts_1899, gArmorDescTexts_1900, gArmorDescTexts_1901, NULL,
    D_004891F0, D_00489210, D_00489230, NULL,
    D_004891F0, D_00489210, D_00489230, NULL,
    D_00484A50, D_00487C00, D_00484A90, NULL,
    D_00487D70, D_00487228, gArmorDescTexts_1917, NULL,
    gArmorDescTexts_1919, D_00489280, NULL, NULL,
    gArmorDescTexts_1923, gArmorDescTexts_1924, gArmorDescTexts_1925, NULL,
    gArmorDescTexts_1927, gArmorDescTexts_1928, gArmorDescTexts_1929, NULL,
    gArmorDescTexts_1931, gArmorDescTexts_1932, gArmorDescTexts_1933, NULL,
    gArmorDescTexts_1935, gArmorDescTexts_1936, gArmorDescTexts_1937, NULL,
    gArmorDescTexts_1939, gArmorDescTexts_1940, gArmorDescTexts_1941, NULL,
    gArmorDescTexts_1943, gArmorDescTexts_1944, gArmorDescTexts_1945, NULL,
    gArmorDescTexts_1947, D_00489280, NULL, NULL,
    D_00488830, D_00488850, D_00488870, NULL,
    D_00485D20, D_00488890, D_004888B0, NULL,
    D_004888D0, D_004888F0, D_00488910, NULL,
    D_00488930, D_00488950, D_00488970, NULL,
    D_00489500, gArmorDescTexts_1968, gArmorDescTexts_1969, NULL,
    D_00489500, gArmorDescTexts_1972, gArmorDescTexts_1973, NULL,
    gArmorDescTexts_1975, gArmorDescTexts_1976, gArmorDescTexts_1977, NULL,
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
    gArmorDescTexts_2123, gArmorDescTexts_2124, gArmorDescTexts_2125, NULL,
    D_0048A0E0, gArmorDescTexts_2128, gArmorDescTexts_2129, NULL,
    D_004847A0, D_0048A140, D_0048A160, NULL,
    D_004847A0, D_0048A180, D_0048A1A0, NULL,
    D_0048A1C0, D_0048A1E0, D_0048A200, NULL,
    D_0048A1C0, D_0048A1E0, D_0048A220, NULL,
    D_0048A240, D_0048A260, D_0048A280, NULL,
    D_0048A2A0, D_0048A2C0, D_0048A2E0, NULL,
    D_004849B0, D_0048A300, D_0048A320, NULL,
    D_004849B0, D_0048A340, D_0048A360, NULL,
    D_00484A50, D_0048A380, D_00484A90, NULL,
    gArmorDescTexts_2167, gArmorDescTexts_2168, gArmorDescTexts_2169, NULL,
    gArmorDescTexts_2171, gArmorDescTexts_2172, gArmorDescTexts_2173, NULL,
    D_00487D70, D_0048A450, D_0048A460, NULL,
    D_00487DB0, gArmorDescTexts_2180, gArmorDescTexts_2181, NULL,
    D_00487E10, gArmorDescTexts_2184, gArmorDescTexts_2185, NULL,
    D_00487E70, D_00487E90, D_0048A500, NULL,
    D_00487E70, D_00487E90, D_0048A500, NULL,
    D_00484F80, gArmorDescTexts_2196, gArmorDescTexts_2197, NULL,
    D_00484F80, gArmorDescTexts_2200, gArmorDescTexts_2201, NULL,
    D_0048A5A0, gArmorDescTexts_2204, gArmorDescTexts_2205, NULL,
    gArmorDescTexts_2207, gArmorDescTexts_2208, gArmorDescTexts_2209, NULL,
    gArmorDescTexts_2211, gArmorDescTexts_2212, gArmorDescTexts_2213, NULL,
    gArmorDescTexts_2215, gArmorDescTexts_2216, gArmorDescTexts_2217, NULL,
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
    D_00489B50, gArmorDescTexts_2316, D_00489B90, NULL,
    D_00487370, D_00489BB0, D_00489BD0, NULL,
    D_004842B0, D_00489BF0, D_004842F0, NULL,
    gArmorDescTexts_2327, D_00489C30, D_00489C50, NULL,
    D_00489C70, D_00489C90, D_00489CB0, NULL,
    D_00489C70, D_00489CD0, D_00489CF0, NULL,
    D_00489D10, D_00489D30, D_00489D50, NULL,
    D_00484410, D_00489D70, D_00489D90, NULL,
    D_00489DB0, D_00489DD0, D_00489DF0, NULL,
    gArmorDescTexts_2351, gArmorDescTexts_2352, gArmorDescTexts_2353, NULL,
    gArmorDescTexts_2355, gArmorDescTexts_2356, gArmorDescTexts_2357, NULL,
    D_00489EB0, D_00489ED0, D_00489EF0, NULL,
    D_00489EB0, D_00489F10, D_00489F30, NULL,
    gArmorDescTexts_2367, gArmorDescTexts_2368, gArmorDescTexts_2369, NULL,
    gArmorDescTexts_2371, gArmorDescTexts_2372, gArmorDescTexts_2373, NULL,
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
    D_00485600, D_00485620, gArmorDescTexts_2417, NULL,
    D_00485660, gArmorDescTexts_2420, gArmorDescTexts_2421, NULL,
    D_004856B0, gArmorDescTexts_2424, gArmorDescTexts_2425, NULL,
    gArmorDescTexts_2427, gArmorDescTexts_2428, gArmorDescTexts_2429, NULL,
    D_0048AB90, gArmorDescTexts_2432, gArmorDescTexts_2433, NULL,
    D_0048ABF0, gArmorDescTexts_2436, gArmorDescTexts_2437, NULL,
    D_00485260, D_0048A720, D_0048A740, NULL,
    D_00485260, D_0048A760, D_0048A780, NULL,
    D_00485300, D_0048A7A0, D_0048A7C0, NULL,
    D_00485300, D_0048A7E0, D_0048A800, NULL,
    gArmorDescTexts_2455, gArmorDescTexts_2456, gArmorDescTexts_2457, NULL,
    gArmorDescTexts_2459, gArmorDescTexts_2460, gArmorDescTexts_2461, NULL,
    gArmorDescTexts_2463, gArmorDescTexts_2464, gArmorDescTexts_2465, NULL,
    gArmorDescTexts_2467, gArmorDescTexts_2468, gArmorDescTexts_2469, NULL,
    gArmorDescTexts_2471, gArmorDescTexts_2472, gArmorDescTexts_2473, NULL,
    gArmorDescTexts_2475, gArmorDescTexts_2476, gArmorDescTexts_2477, NULL,
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
    gArmorDescTexts_2651, D_0048AEB0, D_0048AED0, NULL,
    gArmorDescTexts_2655, D_0048AEB0, D_0048AED0, NULL,
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
    D_0048B3C0, gArmorDescTexts_2812, gArmorDescTexts_2813, NULL,
    D_00486170, gArmorDescTexts_2816, gArmorDescTexts_2817, NULL,
    gArmorDescTexts_2819, gArmorDescTexts_2820, gArmorDescTexts_2821, NULL,
    gArmorDescTexts_2823, gArmorDescTexts_2824, gArmorDescTexts_2825, NULL,
    gArmorDescTexts_2827, gArmorDescTexts_2828, gArmorDescTexts_2829, NULL,
    gArmorDescTexts_2831, D_0048B5A0, gArmorDescTexts_2833, NULL,
    gArmorDescTexts_2835, gArmorDescTexts_2836, gArmorDescTexts_2837, NULL,
    gArmorDescTexts_2839, gArmorDescTexts_2840, gArmorDescTexts_2841, NULL,
    gArmorDescTexts_2843, gArmorDescTexts_2844, gArmorDescTexts_2845, NULL,
    gArmorDescTexts_2847, D_0048B5A0, gArmorDescTexts_2849, NULL,
    D_0048B740, D_0048B760, gArmorDescTexts_2853, NULL,
    D_0048B740, D_0048B760, gArmorDescTexts_2857, NULL,
    gArmorDescTexts_2859, gArmorDescTexts_2860, gArmorDescTexts_2861, NULL,
    gArmorDescTexts_2863, gArmorDescTexts_2864, gArmorDescTexts_2865, NULL,
    D_0048B880, D_0048B8A0, D_0048B8C0, NULL,
    D_0048B880, D_0048B8A0, D_0048B8C0, NULL,
    D_00484A50, D_0048A380, D_00484A90, NULL,
    D_00487D70, D_0048A450, D_0048A460, NULL,
    gArmorDescTexts_2883, gArmorDescTexts_2884, gArmorDescTexts_2885, NULL,
    gArmorDescTexts_2887, gArmorDescTexts_2888, gArmorDescTexts_2889, NULL,
    gArmorDescTexts_2891, gArmorDescTexts_2892, gArmorDescTexts_2893, NULL,
    gArmorDescTexts_2895, gArmorDescTexts_2896, gArmorDescTexts_2897, NULL,
    gArmorDescTexts_2899, gArmorDescTexts_2900, gArmorDescTexts_2901, NULL,
    D_0048AF10, D_0048AF30, D_0048AF50, NULL,
    D_0048AF70, D_0048AF90, D_0048AFB0, NULL,
    gArmorDescTexts_2911, D_0048AFF0, D_0048B010, NULL,
    D_0048AFD0, D_0048B030, D_0048B050, NULL,
    gArmorDescTexts_2919, gArmorDescTexts_2920, gArmorDescTexts_2921, NULL,
    gArmorDescTexts_2923, gArmorDescTexts_2924, gArmorDescTexts_2925, NULL,
    gArmorDescTexts_2927, gArmorDescTexts_2928, gArmorDescTexts_2929, NULL,
    D_0047B0D0, NULL, NULL, NULL,
    D_00486D80, D_0048BBF0, D_0048BC10, NULL,
    gArmorDescTexts_2939, gArmorDescTexts_2940, gArmorDescTexts_2941, NULL,
    D_0048BC90, D_0048BCB0, D_0048BCC0, NULL,
    gArmorDescTexts_2947, gArmorDescTexts_2948, gArmorDescTexts_2949, NULL,
    D_0048BD40, D_0048BD60, D_0048BD80, NULL,
    D_0048BD40, D_0048BDA0, D_0048BDC0, NULL,
    D_0048BDD0, D_0048BDF0, D_0048BE10, NULL,
    D_0048BE30, D_0048BE50, D_0048BE70, NULL,
    D_00483EE0, D_0048BE90, D_0048BEB0, NULL,
    D_0048BED0, D_0048BEF0, D_0048BF10, NULL,
    D_0048BD40, D_0048BF30, D_0048BF50, NULL,
    D_0048BD40, D_0048BF60, D_0048BF80, NULL,
    gArmorDescTexts_2983, gArmorDescTexts_2984, gArmorDescTexts_2985, NULL,
    D_0048C000, D_0048C020, D_0048C040, NULL,
    D_0048C060, D_0048C080, D_0048C0A0, NULL,
    D_0048C0C0, D_0048C0E0, D_0048C100, NULL,
    D_0048C120, D_0048C140, D_0048C160, NULL,
    D_0048C180, D_0048C1A0, D_0048C1C0, NULL,
    D_004841B0, D_0048C1D0, D_0048C1F0, NULL,
    D_00487260, D_0048C200, D_0048C220, NULL,
    D_0048C240, D_0048C260, D_0048C280, NULL,
    D_0048C2A0, gArmorDescTexts_3020, gArmorDescTexts_3021, NULL,
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
    gArmorDescTexts_3119, gArmorDescTexts_3120, gArmorDescTexts_3121, NULL,
    gArmorDescTexts_3123, gArmorDescTexts_3124, gArmorDescTexts_3125, NULL,
    D_00484C50, gArmorDescTexts_3128, gArmorDescTexts_3129, NULL,
    D_0048CAD0, D_00484F40, D_00484F60, NULL,
    D_0048CAD0, D_00484F40, D_00484F60, NULL,
    D_00484F80, gArmorDescTexts_3140, gArmorDescTexts_3141, NULL,
    D_00484F80, gArmorDescTexts_3144, gArmorDescTexts_3145, NULL,
    gArmorDescTexts_3147, gArmorDescTexts_3148, gArmorDescTexts_3149, NULL,
    D_00487F70, gArmorDescTexts_3152, gArmorDescTexts_3153, NULL,
    gArmorDescTexts_3155, gArmorDescTexts_3156, gArmorDescTexts_3157, NULL,
    gArmorDescTexts_3159, gArmorDescTexts_3160, gArmorDescTexts_3161, NULL,
    D_0048CCD0, D_0048CCF0, D_0048CD10, NULL,
    D_0048CCD0, D_0048CD30, D_0048CD50, NULL,
    D_00485300, D_0048CD70, D_0048CD90, NULL,
    D_00485300, D_0048CDB0, D_0048CDD0, NULL,
    gArmorDescTexts_3179, gArmorDescTexts_3180, D_0048A7C0, NULL,
    gArmorDescTexts_3183, gArmorDescTexts_3184, gArmorDescTexts_3185, NULL,
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
    gArmorDescTexts_3279, D_0048C3F0, D_0048C410, NULL,
    D_0048C430, D_0048C450, D_0048C470, NULL,
    D_0048C430, D_0048C490, D_0048C4B0, NULL,
    D_00484410, D_0048C4C0, D_0048C4E0, NULL,
    D_00484410, D_0048C500, D_0048C520, NULL,
    D_0048C540, D_0048C560, D_0048C580, NULL,
    gArmorDescTexts_3303, gArmorDescTexts_3304, gArmorDescTexts_3305, NULL,
    gArmorDescTexts_3307, gArmorDescTexts_3308, gArmorDescTexts_3309, NULL,
    D_0048C660, D_004876E0, D_00487700, NULL,
    D_0048C660, D_0048C680, D_0048C6A0, NULL,
    gArmorDescTexts_3319, gArmorDescTexts_3320, gArmorDescTexts_3321, NULL,
    gArmorDescTexts_3323, gArmorDescTexts_3324, gArmorDescTexts_3325, NULL,
    D_004847A0, D_0048C780, D_0048C7A0, NULL,
    D_004847A0, D_0048C7C0, D_0048C7E0, NULL,
    D_0048C800, D_0048C820, D_0048C840, NULL,
    D_00487A00, D_0048C860, D_0048C878, NULL,
    D_00484960, D_0048C890, D_0048C8B0, NULL,
    D_0048C8D0, D_0048C8F0, D_0048C910, NULL,
    D_004849B0, D_0048C930, D_0048C950, NULL,
    D_004849B0, D_0048C970, D_0048C990, NULL,
    D_00485600, D_00485620, gArmorDescTexts_3361, NULL,
    D_00485660, gArmorDescTexts_3364, gArmorDescTexts_3365, NULL,
    gArmorDescTexts_3367, gArmorDescTexts_3368, gArmorDescTexts_3369, NULL,
    gArmorDescTexts_3371, gArmorDescTexts_3372, gArmorDescTexts_3373, NULL,
    D_00485770, gArmorDescTexts_3376, gArmorDescTexts_3377, NULL,
    D_004857C0, gArmorDescTexts_3380, gArmorDescTexts_3381, NULL,
    D_0048CCD0, D_0048CCF0, D_0048CD10, NULL,
    D_0048CCD0, D_0048CD30, D_0048CD50, NULL,
    D_00485300, D_0048CD70, D_0048CD90, NULL,
    D_00485300, D_0048CDB0, D_0048CDD0, NULL,
    gArmorDescTexts_3399, gArmorDescTexts_3400, gArmorDescTexts_3401, NULL,
    gArmorDescTexts_3403, gArmorDescTexts_3404, gArmorDescTexts_3405, NULL,
    gArmorDescTexts_3407, gArmorDescTexts_3408, gArmorDescTexts_3409, NULL,
    gArmorDescTexts_3411, gArmorDescTexts_3412, gArmorDescTexts_3413, NULL,
    gArmorDescTexts_3415, gArmorDescTexts_3416, gArmorDescTexts_3417, NULL,
    gArmorDescTexts_3419, gArmorDescTexts_3420, gArmorDescTexts_3421, NULL,
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
    gArmorDescTexts_3747, gArmorDescTexts_3748, gArmorDescTexts_3749, NULL,
    gArmorDescTexts_3751, gArmorDescTexts_3752, gArmorDescTexts_3753, NULL,
    gArmorDescTexts_3755, gArmorDescTexts_3756, gArmorDescTexts_3757, NULL,
    gArmorDescTexts_3759, gArmorDescTexts_3760, gArmorDescTexts_3761, NULL,
    gArmorDescTexts_3763, gArmorDescTexts_3764, gArmorDescTexts_3765, NULL,
    gArmorDescTexts_3767, gArmorDescTexts_3768, gArmorDescTexts_3769, NULL,
    gArmorDescTexts_3771, gArmorDescTexts_3772, gArmorDescTexts_3773, NULL,
    gArmorDescTexts_3775, gArmorDescTexts_3776, gArmorDescTexts_3777, NULL,
    gArmorDescTexts_3779, gArmorDescTexts_3780, gArmorDescTexts_3781, NULL,
    gArmorDescTexts_3783, gArmorDescTexts_3784, gArmorDescTexts_3785, NULL,
    D_0048DD40, D_0048DD60, gArmorDescTexts_3789, NULL,
    D_0048DD40, D_0048DD60, gArmorDescTexts_3793, NULL,
    gArmorDescTexts_3795, gArmorDescTexts_3796, gArmorDescTexts_3797, NULL,
    gArmorDescTexts_3799, gArmorDescTexts_3800, gArmorDescTexts_3801, NULL,
    D_0048DE80, D_0048DEA0, D_0048DEC0, NULL,
    D_0048DE80, D_0048DEA0, D_0048DEC0, NULL,
    D_0048DEE0, gArmorDescTexts_3812, gArmorDescTexts_3813, NULL,
    D_0048DEE0, gArmorDescTexts_3816, gArmorDescTexts_3817, NULL,
    D_00484A50, D_0048C9B0, D_00484A90, NULL,
    gArmorDescTexts_3823, gArmorDescTexts_3824, gArmorDescTexts_3825, NULL,
    gArmorDescTexts_3827, gArmorDescTexts_3828, gArmorDescTexts_3829, NULL,
    gArmorDescTexts_3831, gArmorDescTexts_3832, gArmorDescTexts_3833, NULL,
    gArmorDescTexts_3835, gArmorDescTexts_3836, gArmorDescTexts_3837, NULL,
    gArmorDescTexts_3839, gArmorDescTexts_3840, gArmorDescTexts_3841, NULL,
    D_0048D4E0, D_0048D500, D_0048D520, NULL,
    D_00485C60, D_0048D540, D_0048D560, NULL,
    D_0048D580, D_0048D5A0, D_0048D5C0, NULL,
    D_0048D5E0, D_0048D600, D_0048D620, NULL,
    D_00484740, gArmorDescTexts_3860, gArmorDescTexts_3861, NULL,
    D_0048A0E0, gArmorDescTexts_3864, gArmorDescTexts_3865, NULL,
    D_0048E1E0, gArmorDescTexts_3868, gArmorDescTexts_3869, NULL,
    D_0048E1E0, gArmorDescTexts_3872, gArmorDescTexts_3873, NULL,
    gArmorDescTexts_3875, gArmorDescTexts_3876, gArmorDescTexts_3877, NULL,
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
    gArmorDescTexts_3923, gArmorDescTexts_3924, gArmorDescTexts_3925, NULL,
    D_0048E650, D_0048E670, D_0048E690, NULL,
    D_00484010, D_0048E6B0, D_0048E6D0, NULL,
    D_0048E6E0, D_0048E700, D_0048E720, NULL,
    D_00487140, D_0048E740, D_0048E760, NULL,
    D_0048E780, D_0048E7A0, D_0048E7B8, NULL,
    D_004841B0, D_0048E7D0, D_0048E7F0, NULL,
    gArmorDescTexts_3951, gArmorDescTexts_3952, gArmorDescTexts_3953, NULL,
    D_0048E870, D_0048E890, D_0048E8B0, NULL,
    D_0048E8D0, D_0048E8F0, D_0048E910, NULL,
    gArmorDescTexts_3963, gArmorDescTexts_3964, gArmorDescTexts_3965, NULL,
    D_0048C2A0, gArmorDescTexts_3968, gArmorDescTexts_3969, NULL,
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
    gArmorDescTexts_4063, gArmorDescTexts_4064, gArmorDescTexts_4065, NULL,
    D_00487D70, D_0048F080, D_0048F090, NULL,
    D_00487DB0, gArmorDescTexts_4072, gArmorDescTexts_4073, NULL,
    D_00487E10, gArmorDescTexts_4076, gArmorDescTexts_4077, NULL,
    D_00487E70, D_00487E90, D_0048F130, NULL,
    D_00487E70, D_00487E90, D_0048F130, NULL,
    D_00484F80, gArmorDescTexts_4088, gArmorDescTexts_4089, NULL,
    D_00484F80, gArmorDescTexts_4092, gArmorDescTexts_4093, NULL,
    D_0048A5A0, gArmorDescTexts_4096, gArmorDescTexts_4097, NULL,
    D_00487F70, gArmorDescTexts_4100, gArmorDescTexts_4101, NULL,
    gArmorDescTexts_4103, gArmorDescTexts_4104, gArmorDescTexts_4105, NULL,
    gArmorDescTexts_4107, gArmorDescTexts_4108, gArmorDescTexts_4109, NULL,
    D_00485260, D_0048F310, D_0048F330, NULL,
    D_00485260, D_0048F350, D_0048F370, NULL,
    D_00485300, D_0048F390, D_0048F3B0, NULL,
    D_00485300, D_0048F3D0, D_0048F3F0, NULL,
    gArmorDescTexts_4127, D_0048F430, D_0048F450, NULL,
    gArmorDescTexts_4131, D_0048F430, D_0048F450, NULL,
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
    gArmorDescTexts_4215, D_0048E9F0, D_0048E910, NULL,
    D_0048EA10, D_0048EA30, D_0048EA50, NULL,
    D_0048EA70, D_0048EA90, D_0048EAB0, NULL,
    D_0048EAD0, D_0048EAF0, D_0048EB10, NULL,
    D_0048EB30, D_0048EB50, D_0048EB70, NULL,
    D_00484410, D_0048EB90, D_0048EBB0, NULL,
    gArmorDescTexts_4239, gArmorDescTexts_4240, gArmorDescTexts_4241, NULL,
    gArmorDescTexts_4243, gArmorDescTexts_4244, gArmorDescTexts_4245, NULL,
    D_0048EC90, D_00489ED0, D_00489EF0, NULL,
    D_0048EC90, D_00489F10, D_00489F30, NULL,
    gArmorDescTexts_4255, gArmorDescTexts_4256, gArmorDescTexts_4257, NULL,
    gArmorDescTexts_4259, gArmorDescTexts_4260, gArmorDescTexts_4261, NULL,
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
    D_00485600, D_00485620, gArmorDescTexts_4305, NULL,
    D_00485660, gArmorDescTexts_4308, gArmorDescTexts_4309, NULL,
    D_0048F740, gArmorDescTexts_4312, gArmorDescTexts_4313, NULL,
    D_0048F740, gArmorDescTexts_4316, gArmorDescTexts_4317, NULL,
    D_0048AB90, gArmorDescTexts_4320, gArmorDescTexts_4321, NULL,
    D_0048ABF0, gArmorDescTexts_4324, gArmorDescTexts_4325, NULL,
    D_00485260, D_0048F310, D_0048F330, NULL,
    D_00485260, D_0048F350, D_0048F370, NULL,
    D_00485300, D_0048F390, D_0048F3B0, NULL,
    D_00485300, D_0048F3D0, D_0048F3F0, NULL,
    gArmorDescTexts_4343, gArmorDescTexts_4344, gArmorDescTexts_4345, NULL,
    gArmorDescTexts_4347, gArmorDescTexts_4348, gArmorDescTexts_4349, NULL,
    gArmorDescTexts_4351, gArmorDescTexts_4352, gArmorDescTexts_4353, NULL,
    gArmorDescTexts_4355, gArmorDescTexts_4356, gArmorDescTexts_4357, NULL,
    gArmorDescTexts_4359, gArmorDescTexts_4360, gArmorDescTexts_4361, NULL,
    gArmorDescTexts_4363, gArmorDescTexts_4364, gArmorDescTexts_4365, NULL,
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
    gArmorDescTexts_4439, gArmorDescTexts_4440, gArmorDescTexts_4441, NULL,
    gArmorDescTexts_4443, gArmorDescTexts_4444, gArmorDescTexts_4445, NULL,
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
    D_0048B3C0, gArmorDescTexts_4696, gArmorDescTexts_4697, NULL,
    gArmorDescTexts_4699, gArmorDescTexts_4700, gArmorDescTexts_4701, NULL,
    gArmorDescTexts_4703, gArmorDescTexts_4704, gArmorDescTexts_4705, NULL,
    gArmorDescTexts_4707, gArmorDescTexts_4708, gArmorDescTexts_4709, NULL,
    gArmorDescTexts_4711, gArmorDescTexts_4712, gArmorDescTexts_4713, NULL,
    gArmorDescTexts_4715, D_00490200, D_00490220, NULL,
    gArmorDescTexts_4719, gArmorDescTexts_4720, gArmorDescTexts_4721, NULL,
    gArmorDescTexts_4723, gArmorDescTexts_4724, gArmorDescTexts_4725, NULL,
    gArmorDescTexts_4727, gArmorDescTexts_4728, gArmorDescTexts_4729, NULL,
    gArmorDescTexts_4731, D_00490200, D_00490220, NULL,
    D_00490380, gArmorDescTexts_4736, gArmorDescTexts_4737, NULL,
    D_00490380, gArmorDescTexts_4740, gArmorDescTexts_4741, NULL,
    gArmorDescTexts_4743, gArmorDescTexts_4744, gArmorDescTexts_4745, NULL,
    gArmorDescTexts_4747, gArmorDescTexts_4748, gArmorDescTexts_4749, NULL,
    D_004904E0, D_00490500, D_00490518, NULL,
    D_004904E0, D_00490500, D_00490518, NULL,
    D_00484A50, D_0048F010, D_00484A90, NULL,
    D_00487D70, D_0048F080, D_0048F090, NULL,
    gArmorDescTexts_4767, gArmorDescTexts_4768, gArmorDescTexts_4769, NULL,
    gArmorDescTexts_4771, gArmorDescTexts_4772, gArmorDescTexts_4773, NULL,
    gArmorDescTexts_4775, gArmorDescTexts_4776, gArmorDescTexts_4777, NULL,
    gArmorDescTexts_4779, gArmorDescTexts_4780, gArmorDescTexts_4781, NULL,
    gArmorDescTexts_4783, gArmorDescTexts_4784, gArmorDescTexts_4785, NULL,
    D_004856B0, D_0048FB80, D_0048FBA0, NULL,
    D_0048FBC0, D_0048FBE0, D_0048FC00, NULL,
    D_0048FC20, D_0048FC40, D_0048FC60, NULL,
    D_0048FC20, D_0048FC80, D_0048FCA0, NULL,
    gArmorDescTexts_4803, gArmorDescTexts_4804, gArmorDescTexts_4805, NULL,
    D_00484740, gArmorDescTexts_4808, gArmorDescTexts_4809, NULL,
    gArmorDescTexts_4811, D_004907C0, gArmorDescTexts_4813, NULL,
    gArmorDescTexts_4815, D_004907C0, gArmorDescTexts_4817, NULL,
    gArmorDescTexts_4819, gArmorDescTexts_4820, gArmorDescTexts_4821, NULL,
};
IconUV gIconUVs[16] = {
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
