/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "window.h"
#include "chat_log.h"
#include "em.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "plsel.h"
#include "lobby_globals.h"
#include "syssave.h"
#include "area.h"

/* prototypes */
void Gfx_DrawGauge(s32 x, s32 y, s32 w, s32 h, s32 flags); /* unit's own declaration: with the definition's void (s16 x, s16 y, s16 w, s16 h, s32 flags) (sub_main/unk_363550.c), VillageClock_DrawChatMark 17 of 58 words differ */
s32 Town_IsMode(s32 arg0);
void func_00792A50(s32 arg0, s32 arg1);
void Quest_SetInfo(QuestInfo *q);
void *LobbyMember_GetCard(u8 pl);
void Tex_Prepare(s32 n, s32 tex);
void Gfx_SetBlend(s32 a, s32 b);
void Tex_Bind(s32);
/* end of prototypes */

extern s32 gTownMode;
void Chat_Send6714(s32 arg0, M2C_UNK arg1);

s32 Town_IsMode(s32 mode) {
    return gTownMode == mode;
}

void Chat_SendText(s32, void *text, s32); /* unit's own declaration: with the definition's void (s32 unused, void *text, u8 color) (lobby/unk_79EA30.c), Chat_SendToTargets 3 of 76 words differ */
char *strcpy(char *dst, const char *src);
extern u8 gNetUsers;
extern s8 gChatTargets;
extern s8 D_008C0C68;
extern s8 D_008C0C70;
extern s8 D_008C0C78;
extern s8 D_008C0C80;
extern s8 D_008C0C88;
extern s8 D_008C0C90;

extern u16 gChatRecipients;
void Chat_SendToTargets(s32 arg0, void *text, s32 color)
{
    s32 i;
    s32 bit;
    u8 *user;
    s8 *dst;

    if (gChatToAll != 0) {
        gLobby->unk3967 = 0;
    } else {
        bit = 1;
        i = 0;
        user = &gNetUsers;
        dst = &gChatTargets;
        gLobby->unk3967 = 1;
        gChatTargets = 0;
        D_008C0C68 = 0;
        D_008C0C70 = 0;
        D_008C0C78 = 0;
        D_008C0C80 = 0;
        D_008C0C88 = 0;
        D_008C0C90 = 0;
        do {
            if ((i != gScene.localPlayer) && (gChatRecipients & bit)) {
                strcpy((char *)dst, (char *)user);
                dst += 8;
            }
            i += 1;
            user += 0x11C;
            bit += bit;
        } while (i < 0xA);
    }
    Chat_SendText(arg0, text, color);
}

/* A tail call: passes its own a0/a1 on to Chat_Send6714 (no direct caller in the code; reached through a pointer). */
void func_00792A50(s32 arg0, s32 arg1) {
    Chat_Send6714(arg0, arg1);
}

void VillageClock_DrawChatMark(s32 x, s32 y)
{
    s32 flags;

    flags = 0;
    if ((gScene.online == 0) || (gScene.area != AREA_HUB_0x83)) {
        if (gChatSendTo == 0) {
            if (gChatWinOpen != 0) {
                flags = 5;
            }
        } else if ((gSysSave.options.chatBeep == 0) && (gChatLog.win.open != 0)) {
            flags = 4;
        }
        if (flags != 0) {
            Gfx_SetBlend(4, 5);
            Tex_Prepare(1, 0x155);
            Tex_Bind(0x155);
            Gfx_DrawGauge(x, y, 0x14, 0x12, flags);
        }
    }
}

extern char gMyUserId[];
s32 strcmp(char *a, char *b);

/* the bits (1 << member index in gNetUsers) of the party players listed
   in gLobby->players, but for the one with the id at gMyUserId (the
   local user, it seems) */
u16 QuestParty_UserMask(void)
{
    u32 j;
    u32 i;
    u16 mask;
    s32 off;
    NetUser *u;

    if (gLobby->questTaken == 0) {
        return 0;
    }
    mask = 0;
    for (i = 0, off = 0; i < 4; i++, off += 0x11C) {
        u8 *base = (u8 *)gLobby + off;
        if (*(s8 *)(base + 0x5A6) != 0 && strcmp((char *)base + 0x5A6, gMyUserId) != 0) {
            for (j = 0, u = (NetUser *)&gNetUsers; j < 10; j++, u++) {
                if (strcmp((char *)gLobby + off + 0x5A6, u->id) == 0) {
                    mask += (u16)(1 << j);
                    break;
                }
            }
        }
    }
    return mask;
}

void Quest_SetInfo(QuestInfo *q) {
    gQuestInfo = q;
    gQuestTimeLeft = q->timeLimit;
    gQuestReward = q->reward;
}

extern ListPanel gMemberCardPanels[];
extern Panel gMemberCardWindows[];
extern u8 gMemberCardTabTexts[];
extern char *gStatusTitleTexts[];
extern char *D_0080D1DC;
extern char *gStatusBoxTitles[];
extern char *D_447754[][2];
extern char *gSymbolColorNames[];
extern char *gSymbolColorTexts[];
extern char *D_0080C07C;
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Menu_DrawFrame(void *b, s16 *pos);
void Window_DrawText(void *p, s16 *pos, u8 *text);
s32 sprintf(char *buf, char *fmt, ...);
void Text_SetColor(s16 color);
void Font_SetPos(s16 x, s16 y);
void Text_Draw(char *s);
void Text_PrintfColored(char *fmt, ...);
void StatusPage_DrawWeaponClass(Player *pl);
void PersonalStatus_DrawEquipment(Player *pl);
void Player_DrawSkillsPlain(Player *a, s32 b, s16 c);
void Sjis_Widen(char *src, char *dst);
struct V00793350_0;
void MemberCard_DrawStatus(HunterCard *card, s32 x, s32 y);
void MemberCard_DrawRestrict(HunterCard *card, s32 x, s32 y);
void *LobbyArea_Name(u16 id);
void Text_DrawWrapped33(s16 x, s16 y, s16 lineh, char *s);

/* A member's card, page 0 (name, id, rank...) or 1 (equipment). tab first
   holds p->active (without that, page, tab and card take s1, not s0). */
s32 MemberCard_Draw(u8 pl, u8 page)
{
    char num[16];
    char buf[64];
    Player *p;
    s32 flag;
    HunterCard *card;
    s32 tab;

    flag = page & 0x80;
    p = &gPlayers[pl];
    if (flag) {
        page &= 0xF;
    }
    tab = p->active;
    if (tab == 0) {
        ListPanel_Draw(&gMemberCardPanels[page], 0, gStatusTitleTexts[0], -1);
        return -1;
    }
    sprintf(buf, D_0080D1DC, page + 1);
    tab = page;
    ListPanel_Draw(&gMemberCardPanels[tab], 0, buf, -1);
    if (!flag) {
        Menu_DrawFrame(gMemberCardPanels, 0);
    }
    Window_DrawText(&gMemberCardWindows[tab], (s16 *)0, (u8 *)gStatusBoxTitles[gMemberCardTabTexts[tab]]);
    card = LobbyMember_GetCard(p->index);
    switch (page) {
    case 0:
        Text_SetColor(0);
        Font_SetPos(0x17A, 0x52);
        Text_Draw(gLobby->members[p->index].name);
        Font_SetPos(0x17A, 0x66);
        Text_Draw(gLobby->members[p->index].id);
        Font_SetPos(0x17A, 0x7A);
        Text_PrintfColored(D_447754[card->title][0]);
        Font_SetPos(0x17A, 0x8E);
        StatusPage_DrawWeaponClass(p);
        Font_SetPos(0x132, 0xA2);
        if (card->symbolColor != 0xFF) {
            Text_PrintfColored("~C05%s~C00%s", gSymbolColorTexts[0], gSymbolColorNames[card->symbolColor]);
        }
        Font_SetPos(0x17A, 0xB6);
        if (card->hr != 0) {
            Text_PrintfColored(gMemberCardNumFmt, card->hr);
        } else {
            Text_Draw(D_0080C07C);
        }
        Font_SetPos(0x19E, 0xCA);
        sprintf(num, "%4d", card->versusWins);
        Sjis_Widen(num, buf);
        Text_PrintfColored(buf);
        MemberCard_DrawStatus(card, 0x18C, 0xDE);
        MemberCard_DrawRestrict(card, 0x1D4, 0xF2);
        Font_SetPos(0x19E, 0x106);
        Text_PrintfColored(LobbyArea_Name(p->area));
        Text_DrawWrapped33(0x132, 0x13A, 0x14, (char *)card->comment);
        break;
    case 1:
        PersonalStatus_DrawEquipment(p);
        Player_DrawSkillsPlain(p, 0x132, 0x126);
        break;
    }
    return 0;
}

extern Panel gFriendRegWindow;   /* read as a Window (Window_DrawTextB2); 0x18 bytes */
extern Icon gFriendRegIcons[];
extern char *gFriendRegTexts[];
void Window_DrawTextB2(Window *w, u8 *text);
void Draw_Icons(Icon *icons, u8 count);

void MemberCard_DrawFriendReg(s16 y, u8 i)
{
    gFriendRegWindow.y = y;
    Window_DrawTextB2((Window *)&gFriendRegWindow, (u8 *) gFriendRegTexts[i]);
    if (i == 0) {
        gFriendRegIcons[0].y = y - 2;
        Draw_Icons(gFriendRegIcons, 1);
    }
}

/* the text of member pl's NetUser in gLobby->unkA16: its card */
void *LobbyMember_GetCard(u8 pl)
{
    return gLobby->members[pl].text;
}

/* unk_792900's .data (0x7FF280-0x7FF320) */
extern char *gMemberCardPanels_0[];
ListPanel gMemberCardPanels[2] = {
#include "lobby/007FF280.inc"
};
Panel gMemberCardWindows[2] = {
#include "lobby/007FF2C0.inc"
};
u8 gMemberCardTabTexts[16] = {
#include "lobby/007FF2F0.inc"
};
Panel gFriendRegWindow = {
#include "lobby/007FF300.inc"
};
Icon gFriendRegIcons[1] = {
#include "lobby/007FF318.inc"
};
