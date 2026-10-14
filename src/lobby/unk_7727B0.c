/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "game_menu.h"
#include "m2c_macros.h"
#include "sprite.h"
#include "window.h"

s32 IdList_Find(s8 *list, s32 n, void *id);
struct Rec18;
s32 IdList_Remove(struct Rec18 *r, u8 idx, u8 n);
extern M2C_UNK gChatBlockList;
void Draw_Icons(Icon *icons, u8 count);
void Window_DrawText(void *p, s16 *pos, u8 *text);
extern Panel gMutedTitleWindow;
extern s16 gMutedTitleIcons[];
extern char *gMutedLabelTexts[];
s32 IdList_Count(s8 *p, u8 n);

s32 MuteList_Remove(ChatSpeaker *e) {
    s32 idx;

    idx = IdList_Find((s8 *) &gChatBlockList, 0x14, e);
    if (idx != -1) {
        return IdList_Remove((struct Rec18 *) &gChatBlockList, idx & 0xFF, 0x14);
    }
    return 0;
}

u8 ChatPage_CountMuted(void)
{
    gSubMenuWork->mutedCount = IdList_Count((s8 *) &gChatBlockList, 0x14);
    gSubMenuWork->mutedGrid.index = 0;
    gSubMenuWork->mutedGrid.rows = 10;
    gSubMenuWork->mutedGrid.row = 0;
    gSubMenuWork->mutedGrid.col = 0;
    gSubMenuWork->mutedGrid.cols = (gSubMenuWork->mutedCount + 9) / 10;
    gSubMenuWork->mutedGrid.lastRows = gSubMenuWork->mutedCount - (gSubMenuWork->mutedGrid.cols - 1) * 10;
    return gSubMenuWork->mutedCount;
}

struct Pair;
s32 Shake_CopyOffset(u8 id, struct Pair *out);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Menu_DrawFrame(void *b, s16 *pos);
void Gfx_DrawGauge(s16 x, s16 y, s16 w, s16 h, s32 flags);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(const char *fmt, ...);
s32 sprintf(char *, const char *, ...);
void Sjis_Widen(char *src, char *dst);
void Text_Draw(char *s);
void Chat_DrawMutedTitle(s16 *pos);
void HunterInfo_DrawIfOpen(void);
s32 Shake_Value(u8);
extern ListPanel gSpeakerPanel;
extern ListPanel gSpeakerListPanel;
extern ListPanel gSpeakerHeadPanel;
extern char *gChatUserActTexts[][3];
extern char *gSpeakerListTexts[];
extern char *D_0080CFA4;
extern char *gSpeakerHeadTexts[];
extern char *gNoSpeakerTexts[];
extern char *D_0080CFB4;
extern char *gMutedListTexts[];
extern char *D_0080CFBC;
extern char *D_0080CFC4;

/* a hunter record of the saved list (0x18 bytes) */
typedef struct HunterRec {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x10];
} HunterRec;
/* The hunter list screen (gSubMenuWork): the members found or the saved list
 * (gChatBlockList), 10 per page. In both loops i counts the rows down and n
 * is the entry index. m stays a byte view of gSubMenuWork (GameMenu): with
 * GameMenu *m (every field read as m->field, round 31 session 44), MWCC
 * loads the grid's column count (m[0x6B5], m[0x6C1]) twice where the game
 * loads it once into a3, even with those three reads left as byte reads;
 * the rest matches as fields. */
void Chat_DrawSpeakers(void)
{
    char text[0x20];
    char num[0x10];
    char line[0xC];
    s16 pos[2];
    s32 i;
    u32 color;
    s32 k;
    s32 sel;
    s16 y;
    s32 n;
    u8 *m;

    Shake_CopyOffset(0xF, (struct Pair *) pos);
    m = (u8 *)gSubMenuWork;
    switch (m[6]) {
    case 0:
        color = 0xFF0A9182;
        switch (m[7]) {
        case 1:
            if (m[*(u16 *)(m + 0x6B0) + 0x12] == 0) {
                n = m[0x11] < 20;
                k = 0;
                if (!n) {
                    k = 1;
                }
            } else {
                k = 2;
            }
            gSpeakerListPanel.items = gChatUserActTexts[k];
            ListPanel_Draw(&gSpeakerListPanel, pos, NULL, m[0xF]);
            color = 0xFF808080;
        case 0:
            m = (u8 *)gSubMenuWork;
            sel = m[0x6B5] != 0 ? m[0x6B3] + 1 : -1;
            if (m[0x6B5] <= 0) {
                sprintf(text, gSpeakerListTexts[0]);
            } else {
                sprintf(text, D_0080CFA4, m[0x6B4] + 1, m[0x6B5]);
            }
            gSpeakerPanel.cursorColor = color;
            ListPanel_Draw(&gSpeakerPanel, pos, text, sel);
            if (gSubMenuWork->speakerGrid.cols != 0) {
                if (gSubMenuWork->speakerGrid.cols > 1) {
                    Menu_DrawFrame(&gSpeakerPanel, pos);
                }
                Font_SetColor(0x807F7F28);
                Font_SetPos(pos[0] + 0x1C1, 0x52);
                Text_PrintfColored(gSpeakerHeadTexts[0]);
                Font_SetColor(0x807F7F7F);
                y = 0x66;
                i = 10;
                n = gSubMenuWork->speakerGrid.col * 10;
                do {
                    m = (u8 *)gSubMenuWork;
                    if (((s8 *)m)[n * 0x19 + 0x53] == 0) {
                        break;
                    }
                    if ((m + n)[0x12] != 0) {
                        Gfx_DrawGauge(pos[0] + 0x1A3, y, 0x19, 0x14, 8);
                    }
                    Font_SetPos(pos[0] + 0x1C1, y);
                    m = (u8 *)gSubMenuWork + n * 0x19;
                    Text_PrintfColored("%-8.8s   %-6.6s", m + 0x5B, m + 0x53);
                    y += 0x14;
                    i--;
                    n++;
                } while (i > 0);
            } else {
                Font_SetColor(0x802B7F7F);
                Font_SetPos(pos[0] + 0x1B8, 0x78);
                Text_PrintfColored(gNoSpeakerTexts[0]);
            }
            Chat_DrawMutedTitle(pos);
            break;
        case 2:
            switch (m[0xF]) {
            case 0:
                HunterInfo_DrawIfOpen();
            }
            break;
        }
        break;
    case 1:
        switch (m[7]) {
        case 1:
            ListPanel_Draw(&gSpeakerHeadPanel, NULL, NULL, 0);
        case 0:
            m = (u8 *)gSubMenuWork;
            sel = m[0x6C1] != 0 ? m[0x6BF] + 1 : -1;
            if (m[0x6C1] <= 0) {
                sprintf(text, D_0080CFB4);
            } else {
                sprintf(text, gMutedListTexts[0], m[0x6C0] + 1, m[0x6C1]);
            }
            gSpeakerPanel.cursorColor = 0xFF0A9182;
            ListPanel_Draw(&gSpeakerPanel, pos, text, sel);
            if (gSubMenuWork->mutedGrid.cols != 0) {
                if (gSubMenuWork->mutedGrid.cols > 1) {
                    Menu_DrawFrame(&gSpeakerPanel, pos);
                }
                Font_SetColor(0x807F7F28);
                Font_SetPos(pos[0] + 0x1C1, 0x52);
                Text_PrintfColored(gSpeakerHeadTexts[0]);
                Font_SetColor(0x807F7F7F);
                y = 0x66;
                i = 10;
                n = gSubMenuWork->mutedGrid.col * 10;
                do {
                    if (((HunterRec *)&gChatBlockList)[n].id[0] == 0) {
                        break;
                    }
                    Gfx_DrawGauge(pos[0] + 0x1A3, y, 0x19, 0x14, 8);
                    Font_SetPos(pos[0] + 0x1C1, y);
                    Text_PrintfColored("%-8.8s   %-6.6s", ((HunterRec *)&gChatBlockList)[n].name, ((HunterRec *)&gChatBlockList)[n].id);
                    y += 0x14;
                    i--;
                    n++;
                } while (i > 0);
            } else {
                Font_SetColor(0x802B7F7F);
                Font_SetPos(pos[0] + 0x1A6, 0x78);
                Text_PrintfColored(D_0080CFBC);
            }
            if (Shake_Value(0xF) == 0) {
                Font_SetColor(0x807F7F28);
                Font_SetPos(pos[0] + 0x184, 0x190);
                Text_PrintfColored(D_0080CFC4);
                if (gSubMenuWork->mutedCount >= 20) {
                    Font_SetColor(0x807F212E);
                } else {
                    Font_SetColor(0x807F7F7F);
                }
                Font_SetPos(pos[0] + 0x1F0, 0x190);
                sprintf(line, "%2d/20", gSubMenuWork->mutedCount);
                Sjis_Widen(line, num);
                Text_Draw(num);
            }
            break;
        }
        break;
    }
}

void Chat_DrawMutedTitle(s16 *pos) {
    Window_DrawText(&gMutedTitleWindow, (s16 *)pos, (u8 *)gMutedLabelTexts[0]);
    gMutedTitleIcons[0] = *pos + 0x1C1;
    Draw_Icons((Icon *) gMutedTitleIcons, 1);
}

/* unk_7727B0's .data (0x7F1F50-0x7F1FD0) */
ListPanel gSpeakerPanel = {
#include "lobby/007F1F50.inc"
};
ListPanel gSpeakerListPanel = {
#include "lobby/007F1F70.inc"
};
ListPanel gSpeakerHeadPanel = {
#include "lobby/007F1F90.inc"
};
Panel gMutedTitleWindow = {
#include "lobby/007F1FB0.inc"
};
s16 gMutedTitleIcons[4] = {
#include "lobby/007F1FC8.inc"
};
