#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "lobby_globals.h"
#include "quest.h"
#include "lobby_menu.h"
#include "lobby_vobj.h"
#include "area.h"

/* prototypes */
s32 Monster_IsAvailableOffline(u16 em);
s32 Monster_IsAvailableOnline(u16 id);
s32 Game_IsOnline(void);
/* end of prototypes */


/* Lists of 60 quest ids (0 ends a list). */
extern u16 gQuestLists[][60];
QuestInfo *Quest_GetDiscInfo(u16 id);
u16 Map_Group(u16 id);
void *memset(void *dst, s32 c, u32 n);

u16 Rand_Next(s32);
s32 QuestList_CountKind(s32 kind, s8 list);
void QuestList_DropKind(s32 kind, s32 skip, s8 list);

extern u16 gQuestDeskShown[][12];
s32 Cond_Check(u16 id);

s32 QuestList_PickShown(s8 list)
{
    s32 i;
    s32 n;
    u16 *q;
    u16 *tbl;
    u16 *shown;

    n = 0;
    shown = gQuestDeskShown[list];
    memset(shown, 0, sizeof(gQuestDeskShown[list]));
    i = 0;
    tbl = gQuestLists[list];
    q = tbl;
    for (; i < 60; i++, q++) {
        if (*q == 0) {
            break;
        }
        if (n <= 11) {
            if (Game_IsOnline() == 0) {
                if ((Rand_Next(1) & 0xFFFF) % 100 < 40 || Cond_Check(7) == 0) {
                    gQuestDeskShown[list][n++] = *q;
                }
            } else {
                if ((Rand_Next(1) & 0xFFFF) % 100 <= 39) {
                    gQuestDeskShown[list][n++] = *q;
                }
            }
        }
    }
    if (n == 0) {
        shown[0] = tbl[0];
    }
    return 0;
}

extern u16 *gQuestIdLists[];
s32 Quest_IsOffered(u16 id, s32 kind, s32 c);
s32 QuestId_IsStandard(s32 id);
void QuestList_DropRandom(s8 list);
void QuestList_Build(s32 kind)
{
    s8 list;
    s32 i;
    s32 end;
    s32 n;
    u16 *q;


    switch (kind) {
    case 0:
    case 1:
    case 3:
        list = 0;
        i = 0;
        end = 4;
        break;
    case 2:
        list = 1;
        i = 4;
        end = 8;
        break;
    }
    n = 0;
    for (; i < end; i++) {

        q = gQuestIdLists[i];
        while (*q != 0) {
            if (Quest_IsOffered(*q, kind, 0) == 1) {
                if (kind == 0 || kind == 3) {
                    if (QuestId_IsStandard(*q) == 1) {
                        gQuestLists[list][n++] = *q;
                    }
                } else if (QuestId_IsStandard(*q) == 0) {
                    gQuestLists[list][n++] = *q;
                }
            }
            if (n > 0x3B) {
                break;
            }
            q++;
        }
        if (n > 0x3B) {
            break;
        }
    }
    if (Game_IsOnline() == 1 || Cond_Check(7) == 1) {
        QuestList_DropRandom(list);
    }
    QuestList_PickShown(list);
}

s32 QuestList_CountKind(s32 kind, s8 list)
{
    s32 n;
    s32 i;
    s32 k;

    n = 0;
    for (i = 0; i < 60; i++) {
        if (gQuestLists[list][i] == 0) {
            break;
        }
        k = Map_Group(Quest_GetDiscInfo(gQuestLists[list][i])->map);
        if (k == kind) {
            n++;
        }
    }
    return n;
}

void QuestList_DropKind(s32 kind, s32 skip, s8 list)
{
    u16 drop[60];
    QuestInfo *info;
    s32 n;
    s32 i;
    s32 k;
    s32 j;
    u16 *tbl;
    u16 *d;
    u16 *last;

    memset(drop, 0, sizeof(drop));
    n = 0;
    i = 0;
    tbl = gQuestLists[list];
    for (; i < 60; i++) {
        if (tbl[i] == 0) {
            break;
        }
        info = Quest_GetDiscInfo(tbl[i]);
        k = Map_Group(info->map);
        if (k == kind) {
            if (n != skip) {
                if (info->goals[0].target == 0x35) {
                    if ((Rand_Next(1) & 0xFFFF) % 100 <= 89) {
                        drop[i] = 1;
                    }
                } else if (info->type != 6) {
                    if ((Rand_Next(1) & 0xFFFF) % 100 <= 19) {
                        drop[i] = 1;
                    }
                }
            }
            n++;
        }
    }
    i = 59;
    d = &drop[59];
    last = &tbl[59];
    for (; i >= 0; i--, d--) {
        if (*d == 1) {
            for (j = i; j < 59; j++) {
                tbl[j] = tbl[j + 1];
            }
            *last = 0;
        }
    }
}

void QuestList_DropRandom(s8 list)
{
    s32 kind;
    s32 n;

    kind = 0;
    do {
        n = QuestList_CountKind(kind, list);
        if (n != 0) {
            QuestList_DropKind(kind, (s32) (Rand_Next(1) & 0xFFFF) % n, list);
        }
        kind += 1;
    } while (kind < 0x1A);
}

char *Quest_GetText(u16 id, s32 k);
void Text_PrintfColored(char *fmt, ...);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
extern u8 gMenuWork[];
extern u32 *gDecoFrameColors[];
extern char *gInfoIndexTexts[];
extern char *D_0080CD2C;
extern char *gHuntInfoHeadTexts[];
void Font_SetSpacing(u8 a, u8 b);
void Menu_DrawDecoFrame(s16 x, s16 y0, s16 w0, s16 h, s8 kind, s8 b);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);
void QuestDesk_DrawFieldInfo(s16 x, s16 y)
{
    u32 *p;
    s16 ty = y;
    u32 cArrow;
    u32 cShadow;
    u32 cItem;
    u32 cText;
    s16 cx;
    QuestDesk *m = (QuestDesk *)gMenuWork;
    s16 ly;
    s32 i;
    s16 tx;
    char *s;
    s16 k;

    Menu_DrawDecoFrame(x, y, 0x140, 0x168, 3, m->season);
    p = gDecoFrameColors[m->season];
    cArrow = *(p += 2);
    cShadow = *++p;
    cText = *++p;
    cItem = *++p;
    Font_SetColor(0x807F7F7F);
    cx = x + 0xA0;
    ly = (s16)y + 0x64;
    Font_SetSpacing(0x16, 0x16);
    s = D_0080CD2C;
    Font_SetPos(Text_CenterX(s, cx, 0x16), ty += 0x50);
    Text_PrintfColored(s);
    s = gHuntInfoHeadTexts[0];
    Font_SetPos(Text_CenterX(s, cx, 0x16), ty + 0x78);
    Text_PrintfColored(s);
    Font_SetSpacing(0x14, 0x14);
    ly += 0xA;
    for (i = 0; i < m->count; i++) {
        k = m->kinds[i];
        s = gInfoIndexTexts[k];
        if (k > 1) {
            tx = Text_CenterX(s, cx, 0x14);
            if (i == m->kindSel) {
                Menu_DrawArrowsRev(tx, ly, 0x14, s, cArrow);
                Text_DrawShadowRgb(tx, ly, cText, cShadow, s);
            } else {
                Font_SetColor(cItem);
                Font_SetPos(tx, ly);
                Text_PrintfColored(s);
            }
            ly += 0x18;
        }
    }
    ly = ty + 0x96;
    for (i = 0; i < m->count; i++) {
        k = m->kinds[i];
        s = gInfoIndexTexts[k];
        if (k < 2) {
            tx = Text_CenterX(s, cx, 0x14);
            if (i == m->kindSel) {
                Menu_DrawArrowsRev(tx, ly, 0x14, s, cArrow);
                Text_DrawShadowRgb(tx, ly, cText, cShadow, s);
            } else {
                Font_SetColor(cItem);
                Font_SetPos(tx, ly);
                Text_PrintfColored(s);
            }
            ly += 0x18;
        }
    }
}

extern char *D_0080C14C;
extern char *gEmClassNames[];
extern s16 gQuestMapGroups[];
extern char *gStageNames[];

void QuestDesk_DrawIndexList(s16 x, s16 y)
{
    QuestDesk *m = (QuestDesk *)gMenuWork;
    s32 end;
    u32 cArrow;
    u32 cShadow;
    s32 i;
    char *s;
    u32 cItem;
    u32 *p;
    s16 tx;
    s32 first;
    u32 cText;

    Menu_DrawDecoFrame(x, y, 0x140, 0x168, 3, m->season);
    p = gDecoFrameColors[m->season];
    cArrow = *(p += 2);
    cShadow = *++p;
    cText = *++p;
    cItem = *++p;
    Font_SetColor(0x807F7F7F);
    x = x + 0xA0;
    Font_SetSpacing(0x16, 0x16);
    s = gInfoIndexTexts[m->kinds[m->kindSel]];
    Font_SetPos(Text_CenterX(s, x, 0x16) - 0x16, y + 0x3C);
    Text_PrintfColored(D_0080C14C, s);
    if (m->kinds[m->kindSel] == 4) {
        first = 1;
        end = m->unk72 + 1;
    } else {
        first = 0;
        end = m->unk72;
    }
    Font_SetSpacing(0x14, 0x14);
    if (m->kinds[m->kindSel] == 4) {
        y += 0x84;
    } else {
        y += 0x6E;
    }
    for (i = first; i < end; i++) {
        if (m->kinds[m->kindSel] == 4) {
            s = gEmClassNames[i];
        } else {
            s = gStageNames[gQuestMapGroups[i]];
        }
        tx = Text_CenterX(s, x, 0x14);
        if (m->list == i - first) {
            Menu_DrawArrowsRev(tx, y, 0x14, s, cArrow);
            Text_DrawShadowRgb(tx, y, cText, cShadow, s);
        } else {
            Font_SetColor(cItem);
            Font_SetPos(tx, y);
            Text_PrintfColored(s);
        }
        y += 0x18;
    }
}

char *strcat(char *dst, const char *src);
void Quest_DrawSummary(s16 x, s16 y, QuestInfo *q);
extern char D_00838B30[];
extern char *gStarMarkTexts[];
extern char *gQuestKindNames[];
extern char *gMainTargetTexts[];
extern char *D_0080CD64;
extern char *D_0080CD4C;
extern char *gSubTargetBTexts[];
extern char *D_0080CD54;
void Quest_DrawOverview(s16 x, s16 y, QuestInfo *q)
{
    s32 i;
    s32 id;
    s16 y0;
    char buf[0x40];

    if ((q->flags & 0x400000) != 0 && Game_IsOnline() == 1) {
        Quest_DrawSummary(x, y, q);
        return;
    }
    id = q->id;
    y0 = y;
    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(D_00838B30, Quest_GetText(id, 0));
    Font_SetPos(x, y += 0x3C);
    Text_PrintfColored(D_00838B30, Quest_GetText(id, 1));
    Font_SetPos(x + 0x48, y += 0x18);
    buf[0] = 0;
    for (i = 0; i < q->stars; i++) {
        strcat(buf, gStarMarkTexts[0]);
    }
    Text_PrintfColored(D_00838B30, buf);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(D_00838B30, Quest_GetText(id, 2));
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(D_00838B30, Quest_GetText(id, 3));
    Font_SetPos(x, y + 0x2A);
    Text_PrintfColored(D_00838B30, gQuestKindNames[q->terms]);
    Font_SetColor(0x80301010);
    x = x - 0xA;
    y = y0 + 0x14;
    y += 0x2A;
    Font_SetPos(x, y);
    Text_PrintfColored(D_00838B30, gMainTargetTexts[0]);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(D_00838B30, D_0080CD64);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_00838B30, D_0080CD4C);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(D_00838B30, gSubTargetBTexts[0]);
    Font_SetPos(x, y + 0x2A);
    Text_PrintfColored(D_00838B30, D_0080CD54);
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_71C390", D_00838B30);

s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
u16 Dojo_GetParMinutes(u16 id);
void Quest_DrawContract(s16 x, s16 y, QuestInfo *q);
extern char *D_0080C08C;
extern char *D_0080CD7C;
extern char *D_0080CD5C;
extern char *gTimeAttackTexts[];
extern char *gContractFeeTexts[];
extern char *gMainPointsTexts[];
extern char *D_0080CD34;
extern char *gSubAPointsTexts[];
extern char *D_0080CD3C;
extern char *gSubBPointsTexts[];
extern char *D_0080CD44;
extern char *gClearCondTexts[];
extern char *D_0080CD6C;
void Quest_DrawTerms(s16 x, s16 y, QuestInfo *q)
{
    s32 id;
    s16 y0;
    char wide[0x40];
    char buf[0x40];

    if ((q->flags & 0x400000) != 0 && Game_IsOnline() == 1) {
        Quest_DrawContract(x, y, q);
        return;
    }
    id = q->id;
    y0 = y;
    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(D_00838B30, Quest_GetText(id, 0));
    Font_SetPos(x + 0x7E, y += 0x2A);
    sprintf(buf, "%d%s", q->timeLimit / 60 / 30, D_0080C08C);
    Sjis_Widen(buf, wide);
    Text_PrintfColored(D_00838B30, wide);
    Font_SetPos(x + 0x7E, y += 0x18);
    if ((q->flags & 0x200000) != 0) {
        sprintf(buf, D_0080CD7C, Dojo_GetParMinutes(id));
    } else {
        sprintf(buf, "%dz", q->fee);
    }
    Sjis_Widen(buf, wide);
    Text_PrintfColored(D_00838B30, wide);
    Font_SetPos(x + 0x7E, y += 0x18);
    if ((q->flags & 0x200000) != 0) {
        sprintf(buf, "%dP", q->reward);
    } else {
        sprintf(buf, "%dz", q->reward);
    }
    Sjis_Widen(buf, wide);
    Text_PrintfColored(D_00838B30, wide);
    Font_SetPos(x + 0x7E, y += 0x18);
    if ((q->flags & 0x200000) != 0) {
        sprintf(buf, "%dP", q->rewardSubA);
    } else {
        sprintf(buf, "%dz", q->rewardSubA);
    }
    Sjis_Widen(buf, wide);
    Text_PrintfColored(D_00838B30, wide);
    Font_SetPos(x + 0x7E, y += 0x18);
    if ((q->flags & 0x200000) != 0) {
        sprintf(buf, "%dP", q->rewardSubB);
    } else {
        sprintf(buf, "%dz", q->rewardSubB);
    }
    Sjis_Widen(buf, wide);
    Text_PrintfColored(D_00838B30, wide);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(D_00838B30, Quest_GetText(id, 4));
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(D_00838B30, Quest_GetText(id, 5));
    Font_SetColor(0x80301010);
    x = x - 0xA;
    y = y0 + 0x14;
    y += 0x2A;
    Font_SetPos(x, y);
    Text_PrintfColored(D_00838B30, D_0080CD5C);
    Font_SetPos(x, y += 0x18);
    if ((q->flags & 0x200000) != 0) {
        Text_PrintfColored(D_00838B30, gTimeAttackTexts[0]);
    } else {
        Text_PrintfColored(D_00838B30, gContractFeeTexts[0]);
    }
    Font_SetPos(x, y += 0x18);
    if ((q->flags & 0x200000) != 0) {
        Text_PrintfColored(D_00838B30, gMainPointsTexts[0]);
    } else {
        Text_PrintfColored(D_00838B30, D_0080CD34);
    }
    Font_SetPos(x, y += 0x18);
    if ((q->flags & 0x200000) != 0) {
        Text_PrintfColored(D_00838B30, gSubAPointsTexts[0]);
    } else {
        Text_PrintfColored(D_00838B30, D_0080CD3C);
    }
    Font_SetPos(x, y += 0x18);
    if ((q->flags & 0x200000) != 0) {
        Text_PrintfColored(D_00838B30, gSubBPointsTexts[0]);
    } else {
        Text_PrintfColored(D_00838B30, D_0080CD44);
    }
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_00838B30, gClearCondTexts[0]);
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(D_00838B30, D_0080CD6C);
}

struct QuestView;
void Quest_DrawDojoDetails(s16 x, s16 y, struct QuestView *q);
extern char *gStageNames[];
extern char *gDestinationTexts[];
extern char *gClientLabelTexts[];
extern char *D_0080CD74;
void Quest_DrawDetails(s16 x, s16 y, QuestInfo *q)
{
    s32 id = q->id;
    s16 y0 = y;

    if ((q->flags & 0x200000) != 0) {
        Quest_DrawDojoDetails(x, y, (struct QuestView *)q);
        return;
    }
    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(D_00838B30, Quest_GetText(id, 0));
    Font_SetPos(x + 0x5A, y += 0x34);
    Text_PrintfColored(D_00838B30, Quest_GetText(id, 6));
    Font_SetPos(x + 0x5A, y += 0x18);
    Text_PrintfColored(gStageNames[Map_Group(q->map)]);
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(D_00838B30, Quest_GetText(id, 7));
    Font_SetColor(0x80301010);
    x = x - 0xA;
    y = y0 + 0x14;
    y += 0x34;
    Font_SetPos(x, y);
    Text_PrintfColored(D_00838B30, gClientLabelTexts[0]);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_00838B30, gDestinationTexts[0]);
    Font_SetPos(x, y + 0x2A);
    Text_PrintfColored(D_00838B30, D_0080CD74);
}

void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
extern char *gClearMarkTexts[];
void Quest_DrawClearMark(s16 x, s16 y)
{
    Font_SetSpacing(0x12, 0x12);
    Text_DrawShadow(x + 0xC8, y + 0x132, 1, 7, gClearMarkTexts[0]);
}

void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 kind, s32 b);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl);
void QuestBoard_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
s32 Save_IsQuestCleared(u16 id);
extern u8 gMapGroupMapIds[][2];
extern u16 gQuestStampRand;
void QuestBoard_Draw(s16 x, s16 y)
{
    QuestDesk *m = (QuestDesk *)gMenuWork;
    QuestInfo *q;
    s8 *f;
    s16 *o;
    s32 i;
    s32 j;
    u16 id;
    u8 c;
    s32 tx;
    s16 offs[12];

    f = m->unk48;
    q = Quest_GetDiscInfo(m->ids[m->list][m->sel]);
    if (m->unk6D == 1 || f[1] == 1) {
        return;
    }
    if (m->quest != NULL) {
        q = m->quest;
    }
    memset(offs, 0, sizeof(offs));
    for (i = 0; i < 12; i++) {
        id = m->ids[m->list][i];
        if (id == 0) {
            break;
        }
        offs[i] = Map_Group(Quest_GetDiscInfo(id)->map);
        if (i == m->sel) {
            offs[i] = -offs[i];
        }
    }
    c = gMapGroupMapIds[Map_Group(q->map)][0];
    if ((q->flags & 0x200000) != 0 || (q->flags & 0x4000000) != 0) {
        Menu_DrawQuestSheet(x, y, 0x140, 0x168, offs, m->sel, c, -1);
    } else {
        Menu_DrawQuestSheet(x, y, 0x140, 0x168, offs, m->sel, c, m->season);
    }
    Font_SetColor(0x80101010);
    if (m->unk77 == 0) {
        Menu_DrawPageNum(0x21C, y += 5, m->page, 3, 1);
    } else {
        y += 5;
    }
    Font_SetSpacing(0x12, 0x12);
    switch (m->page) {
    case 0:
        if (m->mode == 0xC) {
            q = m->quest;
        }
        Quest_DrawOverview((s16)(x + 0x28) + 0xA, y + 4, q);
        break;
    case 1:
        if (m->mode == 0xC) {
            q = m->quest;
        }
        Quest_DrawTerms((s16)(x + 0x28) + 0xA, y + 4, q);
        break;
    case 2:
        if (m->mode == 0xC) {
            q = m->quest;
        }
        Quest_DrawDetails((s16)(x + 0x28) + 0xA, y + 4, q);
        break;
    }
    if (Save_IsQuestCleared(q->id) != 0) {
        Quest_DrawClearMark(x + 0x28, y);
    }
    if (m->quest != NULL) {
        switch (m->mode) {
        case 8:
            Menu_DrawStamp(x + 0x28, y, 0x140, 0x168, gQuestStampRand, m->unk44);
            break;
        case 12:
        case 9:
        case 10:
        case 11:
            if (m->unk68 > 0) {
                tx = x + 0x28;
                QuestBoard_DrawStamp(tx, y, 0x140, 0x168, gQuestStampRand, m->unk3C);
                Menu_DrawStamp(tx, y, 0x140, 0x168, gQuestStampRand, m->unk44);
                break;
            }
            Menu_DrawStamp(x + 0x28, y, 0x140, 0x168, gQuestStampRand, m->unk3C);
            break;
        }
    }
}

void Menu_DrawArrows(s16 x, s16 y, s16 size, char *text, s32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
extern s8 gQuestMinMembers;
extern s8 gRoomPassOn;
extern char gRoomPassword[];
extern char *gMemberCountTexts[];
extern char *gNoneSetTexts[];
extern char *D_0080CD94;
extern char *gPlayerCountTexts[];
extern char *D_0080CD9C;
extern char *gCondDecideTexts[];
void QuestDesk_DrawRecruitForm(s16 x, s16 y)
{
    QuestDesk *m = (QuestDesk *)gMenuWork;
    s16 ty = y;
    s32 t2;
    s16 bx;
    s32 tx;
    s32 ux;
    s32 sx;

    Menu_DrawQuestSheet(x, y, 0x140, 0x168, NULL, 1, -1, -1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    y += 0x28;
    sx = x;
    tx = sx + 0xD8;
    Font_SetPos(tx, y += 0x2A);
    Text_PrintfColored(D_00838B30, gMemberCountTexts[gQuestMinMembers]);
    if (m->sel == 0) {
        Menu_DrawArrows(tx, y, 0x12, gMemberCountTexts[gQuestMinMembers], 0xFF106060);
    }
    Font_SetPos(tx, y += 0x2A);
    Text_PrintfColored(gNoneSetTexts[gRoomPassOn]);
    if (m->sel == 1) {
        Menu_DrawArrows(tx, y, 0x12, gNoneSetTexts[gRoomPassOn], 0xFF106060);
    }
    Font_SetPos(Text_CenterX(gRoomPassword, 0x1B8, 0x12), y += 0x2A);
    Text_PrintfColored(D_00838B30, gRoomPassword);
    if (m->sel == 2) {
        ux = sx + 0x64;
        Menu_DrawArrowsRev(ux, y, 0x12, "             ", 0xFF106060);
        Gfx_DrawRectFrame(ux, y - 1, 0x78, 0x14, 0xFF106060);
        Gfx_DrawRectFrame(sx + 0x63, y - 2, 0x7A, 0x16, 0xFF106060);
    } else {
        Gfx_DrawRectFrame(sx + 0x64, y - 1, 0x78, 0x14, 0xFF606060);
        Gfx_DrawRectFrame(sx + 0x63, y - 2, 0x7A, 0x16, 0xFF606060);
    }
    Font_SetColor(0x80301010);
    t2 = ty + 0x28;
    y = t2;
    bx = sx + 0x28;
    Font_SetPos(Text_CenterX(D_0080CD94, 0x1B8, 0x12), t2);
    Text_PrintfColored(D_00838B30, D_0080CD94);
    Font_SetPos(bx, y += 0x2A);
    Text_PrintfColored(D_00838B30, gPlayerCountTexts[0]);
    Font_SetPos(bx, y += 0x2A);
    Text_PrintfColored(D_00838B30, D_0080CD9C);
    bx = Text_CenterX(gCondDecideTexts[0], 0x1B8, 0x12);
    Font_SetPos(bx, y += 0x78);
    Text_PrintfColored(D_00838B30, gCondDecideTexts[0]);
    if (m->sel == 3) {
        Menu_DrawArrowsRev(bx, y, 0x12, gCondDecideTexts[0], 0xFF106060);
    }
}

s32 Monster_IsHunted(u16 em);
s32 ElderForecast_IsUnlocked(void);
s32 Monster_IsAvailableOffline(u16 em)
{
    switch (em) {
    case 0x1B:
    case 0x44:
        return 1;
    case 0x6:
    case 0x30:
    case 0x1C:
    case 0x8:
        return Cond_Check(7U);
    case 0x34:
    case 0x15:
    case 0x1F:
    case 0x14:
        return Cond_Check(0xAU);
    case 0x1:
    case 0x33:
    case 0xF:
        return Cond_Check(0x10U);
    case 0xE:
    case 0x43:
    case 0x16:
    case 0x11:
        return Cond_Check(0x15U);
    case 0x1A:
        return Cond_Check(0x52U);
    case 0xB:
        if ((Cond_Check(0x4FU) == 1) && (gSave.unk36E8 != 0)) {
            return 1;
        }
        break;
    case 0x36:
        return Cond_Check(0xDU);
    case 0x40:
    case 0x21:
        return Cond_Check(0x1AU);
    case 0x41:
        return Cond_Check(0x1BU);
    case 0x3B:
        return Cond_Check(0x54U);
    case 0x28:
        return 0;
    case 0x26:
        return Monster_IsHunted(6U);
    case 0x27:
        return Monster_IsHunted(0x14U);
    case 0x2E:
        return Monster_IsHunted(0x15U);
    case 0x2D:
        return Monster_IsHunted(0xFU);
    case 0x25:
        return Monster_IsHunted(1U);
    case 0x2B:
        return Monster_IsHunted(0xEU);
    case 0x2F:
        return Monster_IsHunted(0x11U);
    case 0x31:
        return Monster_IsHunted(0xBU);
    case 0x2C:
        return Monster_IsHunted(0x1AU);
    case 0x35:
        return 0;
    default:
        return ElderForecast_IsUnlocked();
    }
    return 0;
}

s32 Save_TestEventFlag(s32 flag);
s32 Monster_IsAvailableOnline(u16 id)
{
    switch (id) {
    case 0x1B:
    case 0x1C:
    case 0x1F:
    case 0x44:
    case 0x6:
    case 0x14:
    case 0x8:
    case 0x30:
    case 0x34:
        return 1;
    case 0x16:
    case 0x1:
    case 0xF:
    case 0x33:
        if (Cond_Check(0x5CU) == 0) {
            return 0;
        }
        break;
    case 0x15:
    case 0xE:
    case 0x11:
    case 0x43:
        if (Cond_Check(0x5FU) == 0) {
            return 0;
        }
        break;
    case 0x35:
        if (Cond_Check(0x64U) == 0) {
            return 0;
        }
        break;
    case 0x3A:
        return Cond_Check(0x67U);
    case 0x28:
        return 0;
    case 0x26:
        if (gSave.hunted[6] == 0) {
            return 0;
        }
        break;
    case 0x27:
        if (gSave.hunted[0x14] == 0) {
            return 0;
        }
        break;
    case 0x2E:
        if ((gSave.hunted[0x15] == 0) || (Cond_Check(0x5FU) == 0)) {
            return 0;
        }
        break;
    case 0x2D:
        if ((gSave.hunted[0xF] == 0) || (Cond_Check(0x5CU) == 0)) {
            return 0;
        }
        break;
    case 0x25:
        if ((gSave.hunted[1] == 0) || (Cond_Check(0x5CU) == 0)) {
            return 0;
        }
        break;
    case 0x2B:
        if ((gSave.hunted[0xE] == 0) || (Cond_Check(0x5FU) == 0)) {
            return 0;
        }
        break;
    case 0x2F:
        if ((gSave.hunted[0x11] == 0) || (Cond_Check(0x5FU) == 0)) {
            return 0;
        }
        break;
    case 0x31:
        if ((gSave.hunted[0xB] == 0) || (Save_TestEventFlag(0x122) == 0)) {
            return 0;
        }
        break;
    case 0xB:
        if (Save_TestEventFlag(0x122) == 0) {
            return 0;
        }
        break;
    case 0x2C:
    case 0x1A:
        return 0;
    }
    return 1;
}

s32 Monster_IsAvailable(u16 id)
{
    if (Game_IsOnline() == 1) {
        return Monster_IsAvailableOnline(id);
    }
    return Monster_IsAvailableOffline(id);
}

void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void ElderForecast_DrawPanel(u16 *ids, s16 x, s16 y, s32 list, s32 b);
void QuestDesk_DrawElderInfo(s16 x, s16 y)
{
    QuestDesk *m = (QuestDesk *)gMenuWork;
    s32 ty;
    s32 tx;
    s32 top;
    s16 cy;
    s32 bot;
    s16 offs[2];

    memset(offs, 0, 4);
    ty = y;
    offs[0] = -99;
    Menu_DrawQuestSheet(x, ty + 10, 0x140, 0x168, offs, 0, -1, -2);
    tx = x;
    cy = ty + 0xB4;
    top = cy - 0x78;
    Icon_Draw(tx + 10, top, 0x100, 0x20, 0x80202020, 0x1B);
    bot = cy + 0x78;
    Icon_Draw(tx + 10, bot, 0x100, 0x20, 0x80202020, 0x1B);
    Icon_Draw(tx + 0x10A, top, 0x2E, 0x20, 0x80202020, 0x1C);
    Icon_Draw(tx + 0x10A, bot, 0x2E, 0x20, 0x80202020, 0x1C);
    Font_SetSpacing(0x12, 0x12);
    ElderForecast_DrawPanel(m->forecast, tx + 0x14, ty + 0x1E, m->season, m->pm);
}

extern char *gSeasonNames[];
extern char *gDaytimeNames[];
extern char *D_0080C054;
extern char *D_0080CDA4;
extern char *gMonsterEcologyTexts[];
extern char *D_0080CDAC;
extern char *gMonsterNames[];
extern char *gItemNames[];
extern u16 **gFieldGuideTables[];
extern u16 *D_007ED8E8[];
extern u16 *gFieldGuideLists[];
s32 Monster_IsAvailable(u16 id);

/* The half flag m->pm is i's later web (game s7); the monster loop counts with j, whose
 * first web is the centre line's x (sx + 0xA0); the drops have their own pointer q; top is kept
 * in the parameter y (`y = cy;`). lx and tx are reused too: first the title's centred x and the
 * title's x (lx - 0x12), later the list x and the label x. See
 * docs/notes/nm/QuestDesk_DrawFieldGuide.md. */
void QuestDesk_DrawFieldGuide(s16 x, s16 y)
{
    s32 tx;
    u16 *p;
    u16 *q;
    s32 i;
    char buf[0x40];
    QuestDesk *m;
    s16 cy2;
    s32 kind;
    s16 lx;
    s16 cy;
    s32 sx;
    s32 j;
    s16 offs[7];
    s8 idx;

    m = (QuestDesk *)gMenuWork;
    memset(offs, 0, 0xE);
    for (i = 0; i < m->unk62; i++) {
        offs[i] = gQuestMapGroups[i];
        if (offs[i] == 0xD) {
            offs[i] = 2;
        }
        if (i == m->sel) {
            offs[i] = -offs[i];
        }
    }
    kind = gQuestMapGroups[m->sel];
    if (kind == -99) {
        Menu_DrawQuestSheet(x, y + 10, 0x140, 0x168, offs, m->sel, -1, -2);
    } else {
        if (kind == 0xD) {
            kind = 2;
        }
        Menu_DrawQuestSheet(x, y, 0x140, 0x168, offs, m->sel, gMapGroupMapIds[kind][0], m->season);
    }
    Font_SetSpacing(0x12, 0x12);
    cy = y + 0x28;
    Font_SetColor(0x80301010);
    i = m->pm;
    idx = m->season;
    sprintf(buf, "%s%s%s", gSeasonNames[idx], D_0080C054, gDaytimeNames[i]);
    sx = x;
    j = sx + 0xA0;
    lx = Text_CenterX(buf, j, 0x12);
    tx = lx - 0x12;
    Font_SetPos(tx, cy);
    Text_PrintfColored(D_0080C14C, buf);
    sprintf(buf, "%s%s", gStageNames[kind], D_0080CDA4);
    Font_SetPos(Text_CenterX(buf, j, 0x12), cy += 0x14);
    Text_PrintfColored(buf);
    Font_SetColor(0x80301010);
    tx = sx + 0x28;
    Font_SetPos(tx, cy += 0x1E);
    Text_PrintfColored(gMonsterEcologyTexts[0]);
    Font_SetColor(0x80101010);
    lx = tx;
    p = gFieldGuideTables[kind][idx];
    y = cy;
    for (j = 0; j < 7; j++) {
        if (p[1] != 0) {
            if (((p[0] != 0xFFFF && Cond_Check(p[0]) == 1) || Game_IsOnline() == 0) && Monster_IsAvailable(p[1]) == 1) {
                Font_SetPos(lx, cy += 0x14);
                Text_PrintfColored(gMonsterNames[p[1]]);
            }
            p += 2;
        }
    }
    cy2 = y + 0x8C;
    Font_SetColor(0x80301010);
    Font_SetPos(tx, cy2 += 0x1E);
    Text_PrintfColored(D_0080CDAC);
    if (gScene.area == AREA_HUB_0x88) {
        q = D_007ED8E8[m->sel];
    } else {
        q = gFieldGuideLists[m->sel];
    }
    q += m->season * 6;
    if (i == 1) {
        q += 3;
    }
    Font_SetColor(0x80101010);
    Font_SetPos(lx, cy2 += 0x14);
    Text_PrintfColored(gItemNames[*q]);
    q++;
    Font_SetPos(lx, cy2 += 0x14);
    Text_PrintfColored(gItemNames[*q]);
    q++;
    Font_SetPos(lx, cy2 + 0x14);
    Text_PrintfColored(gItemNames[*q]);
}

extern u8 gMenuWork[];
void Text_SetSize(s32 size);
void Menu_DrawDecoBox(s16 x, s16 y, s16 w, s16 h, s8 kind);
void QuestBoard_Draw(s16 x, s16 y);
void QuestDesk_DrawRecruitForm(s16 x, s16 y);
void QuestDesk_DrawFieldGuide(s16 x, s16 y);
void QuestDesk_DrawElderInfo(s16 x, s16 y);
void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
extern char *gQuestKindTexts[];

void QuestDesk_Draw(u8 *task)
{
    QuestDesk *m = (QuestDesk *)gMenuWork;
    s8 *p = m->unk48;

    Text_SetSize(*(s32 *)(task + 0x18));
    switch (m->mode) {
    case 1:
    case 2:
    case 3:
        Menu_DrawDecoBox(m->panelX, 0x3A, 0x140, 0x168, 3);
        break;
    case 4:
    case 0xF:
        switch (m->kinds[m->kindSel]) {
        case 4:
        case 3:
        case 2:
            QuestBoard_Draw(m->boardX + 0x118 + m->boardOfs,
                          m->boardY + 0x3A + m->boardOfs);
            break;
        case 0:
            QuestDesk_DrawFieldGuide(0x118, 0x3A);
            break;
        case 1:
            QuestDesk_DrawElderInfo(0x118, 0x3A);
            break;
        }
        break;
    case 5:
    case 6:
    case 8:
    case 9:
    case 0xC:
        QuestBoard_Draw(m->boardX + 0x118 + m->boardOfs,
                      m->boardY + 0x3A + m->boardOfs);
        break;
    case 7:
        if (m->unk6B == 0) {
            QuestDesk_DrawRecruitForm(0x118, 0x3A);
            break;
        }
        return;
    case 0xA:
    case 0xB:
        if (m->quest != NULL) {
            QuestBoard_Draw(0x118, 0x3A);
            break;
        }
        return;
    case 0xD:
        QuestDesk_DrawFieldGuide(0x118, 0x3A);
        break;
    case 0xE:
        QuestDesk_DrawElderInfo(0x118, 0x3A);
        break;
    default:
        return;
    }
    if (m->unk6D != 1 && (s8)p[1] != 1) {
        Font_SetSpacing(0x16, 0x16);
        Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFFB0B000, gQuestKindTexts[0], 2, 1);
        Menu_DrawMoney(0x1E0, 0x10, 0);
    }
}

s32 LobbyWorld_GetSeason(void);
void QuestDesk_DrawFieldInfo(s16 x, s16 y);
void QuestDesk_DrawIndexList(s16 x, s16 y);
s32 QuestDesk_IsWanted(u8 *p);
void PriceInfo_DrawDetails(s16 x, s16 y, void *list, u8 mode0, s8 n);
void QuestDesk_Draw2(u8 *task)
{
    QuestDesk *m = (QuestDesk *)gMenuWork;
    u8 *t = (u8 *)&gTownQuestLists;
    u8 *list;
    s32 k = LobbyWorld_GetSeason();
    s16 x;

    Text_SetSize(*(s32 *)(task + 0x18));
    switch (m->mode) {
    case 1:
    case 2:
        QuestDesk_DrawFieldInfo(m->panelX, 0x3A);
        break;
    case 3:
    case 4:
    case 0xF:
        x = m->panelX;
        switch (m->kinds[m->kindSel]) {
        case 4:
        case 3:
            QuestDesk_DrawIndexList(x, 0x3A);
            break;
        default:
            QuestDesk_DrawFieldInfo(x, 0x3A);
            break;
        }
        break;
    case 5:
        switch (m->unk6B) {
        case 1:
        case 2:
        case 3:
            if ((u8 *)m->quest != NULL && QuestDesk_IsWanted((u8 *)m->quest) != 0) {
                if (gScene.area == AREA_HUB_0x88) {
                    list = t + k * 0x18 + 0x4C;
                } else if (gScene.area == AREA_HUB_0x86 || Game_IsOnline() == 0) {
                    list = t + k * 0x18 + 4;
                }
                /* the game reads a register it never set: list is s1, still the caller's, online outside AREA_HUB_0x86 and AREA_HUB_0x88 */
                PriceInfo_DrawDetails(m->panelX, 0x44, list, 0, 0);
            }
            break;
        }
        break;
    }
}

extern u16 gLobbyQuestId;
s32 ArenaCup_QuestTaken(void)
{
    if (gLobby->questTaken == 0) {
        return 0;
    }
    if (gLobbyQuestId == 25001) {
        return 1;
    }
    return 0;
}

s32 Equip_GunHasNoAmmo(Player *pl, SaveData *save);
s32 Menu_PushNoticeOnce(s32);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void ArenaCup_Draw(MenuTask *t);
void ArenaCup_Draw2(MenuTask *t);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;
void ArenaCup_Open(u8 *npc)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;

    gTownMenuOpen = 1;
    gTownMode = 0x34;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(ArenaCup_Draw, ArenaCup_Draw2, 0, 0);
    m->season = LobbyWorld_GetSeason();
    if (gSave.arenaEms[0].emKind != 0) {
        m->monsterCount++;
    }
    if (gSave.arenaEms[1].emKind != 0) {
        m->monsterCount++;
    }
    if (gSave.arenaEms[2].emKind != 0) {
        m->monsterCount++;
    }
    if (Equip_GunHasNoAmmo(&gPlayers[gScene.localPlayer], &gSave) == 1) {
        Menu_PushNoticeOnce(0x13);
    }
    gMenuInputLock = 6;
}

/* unk_71C390's .data (0x7ED460-0x7ED950) */
extern u32 D_008951F8[];
extern u32 D_00895200[];
extern u32 D_00895208[];
extern u32 D_00895210[];
extern u32 D_00895218[];
u32 gFieldGuideTables_6_0[8] = {
#include "lobby/007ED460.inc"
};
u32 gFieldGuideTables_6_1[8] = {
#include "lobby/007ED480.inc"
};
u32 gFieldGuideTables_6_2[6] = {
#include "lobby/007ED4A0.inc"
};
u16 *gFieldGuideTables_6[3] = {
    (u16 *)gFieldGuideTables_6_0, (u16 *)gFieldGuideTables_6_1, (u16 *)gFieldGuideTables_6_2,
};
u32 gFieldGuideTables_3_1[8] = {
#include "lobby/007ED4D0.inc"
};
u32 gFieldGuideTables_3_2[8] = {
#include "lobby/007ED4F0.inc"
};
u16 *gFieldGuideTables_3[4] = {
    (u16 *)D_008951F8, (u16 *)gFieldGuideTables_3_1, (u16 *)gFieldGuideTables_3_2, NULL,
};
u32 gFieldGuideTables_4_0[8] = {
#include "lobby/007ED520.inc"
};
u32 gFieldGuideTables_4_1[8] = {
#include "lobby/007ED540.inc"
};
u32 gFieldGuideTables_4_2[6] = {
#include "lobby/007ED560.inc"
};
u16 *gFieldGuideTables_4[3] = {
    (u16 *)gFieldGuideTables_4_0, (u16 *)gFieldGuideTables_4_1, (u16 *)gFieldGuideTables_4_2,
};
u32 gFieldGuideTables_11_0[4] = {
#include "lobby/007ED590.inc"
};
u32 gFieldGuideTables_11_2[4] = {
#include "lobby/007ED5A0.inc"
};
u16 *gFieldGuideTables_11[4] = {
    (u16 *)gFieldGuideTables_11_0, (u16 *)D_00895200, (u16 *)gFieldGuideTables_11_2, NULL,
};
u32 gFieldGuideTables_5_0[6] = {
#include "lobby/007ED5C0.inc"
};
u32 gFieldGuideTables_5_1[3] = {
#include "lobby/007ED5D8.inc"
};
u32 gFieldGuideTables_5_2[3] = {
#include "lobby/007ED5E8.inc"
};
u16 *gFieldGuideTables_5[3] = {
    (u16 *)gFieldGuideTables_5_0, (u16 *)gFieldGuideTables_5_1, (u16 *)gFieldGuideTables_5_2,
};
u16 *gFieldGuideTables_13[3] = {
    (u16 *)D_00895208, (u16 *)D_00895210, (u16 *)D_00895218,
};
u32 gFieldGuideTables_2_0[2] = {
#include "lobby/007ED618.inc"
};
u32 gFieldGuideTables_2_1[2] = {
#include "lobby/007ED620.inc"
};
u32 gFieldGuideTables_2_2[3] = {
#include "lobby/007ED628.inc"
};
u16 *gFieldGuideTables_2[3] = {
    (u16 *)gFieldGuideTables_2_0, (u16 *)gFieldGuideTables_2_1, (u16 *)gFieldGuideTables_2_2,
};
u16* *gFieldGuideTables[16] = {
    NULL, NULL, gFieldGuideTables_2, gFieldGuideTables_3,
    gFieldGuideTables_4, gFieldGuideTables_5, gFieldGuideTables_6, NULL,
    NULL, NULL, NULL, gFieldGuideTables_11,
    NULL, gFieldGuideTables_13, NULL, NULL,
};
u16 gFieldGuideLists_0[24] = {
#include "lobby/007ED690.inc"
};
u16 gFieldGuideLists_1[24] = {
#include "lobby/007ED6C0.inc"
};
u16 gFieldGuideLists_2[24] = {
#include "lobby/007ED6F0.inc"
};
u16 gFieldGuideLists_3[24] = {
#include "lobby/007ED720.inc"
};
u16 gFieldGuideLists_4[24] = {
#include "lobby/007ED750.inc"
};
u16 gFieldGuideLists_5[24] = {
#include "lobby/007ED780.inc"
};
u16 gFieldGuideLists_6[24] = {
#include "lobby/007ED7B0.inc"
};
u16 gFieldGuideLists_7[24] = {
#include "lobby/007ED7E0.inc"
};
u16 gFieldGuideLists_8[24] = {
#include "lobby/007ED810.inc"
};
u16 gFieldGuideLists_9[24] = {
#include "lobby/007ED840.inc"
};
u16 gFieldGuideLists_10[24] = {
#include "lobby/007ED870.inc"
};
u16 gFieldGuideLists_11[24] = {
#include "lobby/007ED8A0.inc"
};
u16 *gFieldGuideLists[12] = {
    gFieldGuideLists_0, gFieldGuideLists_1, gFieldGuideLists_2, gFieldGuideLists_3,
    gFieldGuideLists_4, gFieldGuideLists_5, gFieldGuideLists_6, gFieldGuideLists_7,
    gFieldGuideLists_8, gFieldGuideLists_9, gFieldGuideLists_10, gFieldGuideLists_11,
};
s16 gQuestMapGroups[16] = {
#include "lobby/007ED900.inc"
};
u8 gMapGroupMapIds[24][2] = {
#include "lobby/007ED920.inc"
};
