#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "lobby_labels.h"

/* MemberCard_DrawRestrict, Profile_DrawRestrict, MemberCard_DrawStatus and Profile_DrawStatus pass
 * their s32 x, y unconverted; the definition's (s16, s16) adds the conversions in all four (s16
 * parameters fit the two Restrict ones only). */
/* unit's own declaration: (s16, s16) changes 4 functions */
void Font_SetPos(s32 x, s32 y);
void Text_PrintfColored(const char *fmt, ...);
s32 WordFilter_Scan(s8 *str, s32 replace);

void Icon_Draw(s32, s32, s32, s32, s32, s32); /* unit's own declaration: with the definition's void (s16 x, s16 y, s16 w, s16 h, u32 color, s32 i) (lobby/unk_790EC0.c), MemberCard_DrawStatus 17 of 36 words, Profile_DrawStatus 16 of 34 words differ */
extern u8 gMemberStatusTexts;

void Player_CalcSkills(Player *p);
void *LobbyMember_GetCard(u8 pl);

void LobbyMember_LoadEquip(u8 pl)
{
    HunterCard *d = LobbyMember_GetCard(pl);
    Player *p = &gPlayers[pl];

    p->equip[6] = d->weapon;
    p->equip[0] = d->armour[0];
    p->equip[1] = d->armour[1];
    p->equip[2] = d->armour[2];
    p->equip[3] = d->armour[3];
    p->equip[4] = d->armour[4];
    p->equip[5] = d->armour[5];
    Player_CalcSkills(p);
}

s32 Armor_GetSellPrice(EquipItem *item);

s32 Equip_GetSellPrice(EquipItem *item)
{
    s32 price;

    switch (item->type) {
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
        return Armor_GetSellPrice(item);
    case 6:
        price = gBladeData[item->id].price;
        break;
    case 7:
        price = gGunData[item->id].price;
        break;
    default:
        return 0;
    }
    return price / 2;
}

void MemberCard_DrawRestrict(HunterCard *card, s32 x, s32 y)
{
    Font_SetPos(x, y);
    Text_PrintfColored(gFriendLimitTexts[card->friendLimit]);
}

void Profile_DrawRestrict(s32 x, s32 y)
{
    Font_SetPos(x, y);
    Text_PrintfColored(gFriendLimitTexts[gPlayerText.card.friendLimit]);
}

void MemberCard_DrawStatus(HunterCard *card, s32 x, s32 y)
{
    u8 temp_v1;

    Font_SetPos(x, y);
    Text_PrintfColored(*(char **)(&gMemberStatusTexts + (card->onlineMark * 4)));
    temp_v1 = card->onlineMark;
    if (temp_v1 != 0) {
        Icon_Draw(x, y, 0x19, 0x14, -1, temp_v1 + 0x59);
    }
}


void Profile_DrawStatus(s32 x, s32 y)
{
    HunterCard *s = (HunterCard *)&gPlayerText; /* a cast keeps the address in a register */

    Font_SetPos(x, y);
    Text_PrintfColored(((char **)&gMemberStatusTexts)[s->onlineMark]);
    if (s->onlineMark != 0) {
        Icon_Draw(x, y, 0x19, 0x14, -1, s->onlineMark + 0x59);
    }
}

s8 Kbd_Run(char *text, s16 held, s16 pressed);
u8 Notice_Push(u8, s16, s16);

s32 WordFilter_Check(s32 a0, s32 a1)
{
    return WordFilter_Scan((s8 *) a0, (u8)a1);
}

extern s8 gCtypeTable[];
extern s8 gWordFilterList[];
void WordFilter_Normalize(s8 *dst, s8 *src, s32 n);
s32 WordFilter_IsWordEnd(s8 *arg0);
s32 WordFilter_ReadEntry(s8 *src, s8 *dst, s32 max);
u32 strlen(const void *s);
char *strncpy(char *dst, const char *src, u32 n);

/* The chat word filter: finds the words of the list at gWordFilterList[0] in str
 * (compared in upper case, 1/! also matching L, ( and < ignored at the start of a
 * word) where they stand alone; returns 1 when str is clean, else 0 (with
 * replace set, after overwriting each hit with asterisks). */
s32 WordFilter_Scan(s8 *str, s32 replace)
{
    s8 *list;
    s8 *w;
    s32 ok;
    s32 len;
    s32 i;
    s32 j;
    s32 k;
    s32 n;
    s32 hit;
    s8 c2;
    s8 up[0x50];
    s8 word[0x50];

    ok = 1;
    list = gWordFilterList;
    len = strlen(str);
    WordFilter_Normalize(up, str, len);
    i = 0;
    while (up[i] != 0) {
        hit = 0;
        for (w = list; *w != 0; w += 0x10) {
            n = WordFilter_ReadEntry(w, word, len);
            if (n == -1) {
                continue;
            }
            k = 0;
            j = i;
            while (!word[k] == 0 && str[j] != 0) {
                if (gCtypeTable[up[j] + 1] & 7) {
                    if (up[j] == word[k]) {
                        goto match;
                    }
                    if ((c2 = str[j]) == '1' || c2 == '!') {
                        if (word[k] != 'L') {
                            goto done;
                        }
                        goto match;
                    } else if (c2 == '(' || c2 == '<') {
                        if (k != 0) {
                            goto done;
                        }
                        goto next;
                    } else {
                        goto done;
                    }
                match:
                    k++;
                    if (k == n) {
                        hit = 1;
                        j++;
                        break;
                    }
                }
            next:
                j++;
            }
        done:
            if (hit == 1) {
                if ((gCtypeTable[str[j] + 1] & 7) && WordFilter_IsWordEnd(&up[j]) == 0) {
                    hit = 0;
                    continue;
                }
                if (replace == 0) {
                    return 0;
                }
                for (hit = i; hit < j; hit++) {
                    up[hit] = '*';
                }
                strncpy((char *)str + i, (char *)up + i, j - i);
                i = j;
                ok = 0;
                break;
            }
        }
        while (up[i] != 0) {
            if (hit != 0) {
                break;
            }
            if (!(gCtypeTable[str[i++] + 1] & 7) && (gCtypeTable[up[i] + 1] & 7)) {
                break;
            }
        }
    }
    return ok;
}

extern s8 gCtypeTable[];

void WordFilter_Normalize(s8 *dst, s8 *src, s32 n)
{
    s32 i;

    for (i = 0; i <= n; i++) {
        dst[i] = src[i];
        if (gCtypeTable[src[i] + 1] & 2) {
            dst[i] -= 0x20;
        }
        if (dst[i] == '@') {
            dst[i] = 'A';
        }
        if (dst[i] == '$') {
            dst[i] = 'S';
        }
        if (dst[i] == '5') {
            dst[i] = 'S';
        }
        if (dst[i] == '<') {
            dst[i] = 'C';
        }
        if (dst[i] == '(') {
            dst[i] = 'C';
        }
        if (dst[i] == '!') {
            dst[i] = 'I';
        }
        if (dst[i] == '1') {
            dst[i] = 'I';
        }
        if (dst[i] == '2') {
            dst[i] = 'Z';
        }
        if (dst[i] == '0') {
            dst[i] = 'O';
        }
    }
}

s32 WordFilter_InList(s8 *, s8 *);
extern s8 gWordFilterList[];
extern s8 D_1BD421;

s32 WordFilter_IsWordEnd(s8 *name)
{
    s8 c;

    c = *name;
    if (c == 0) {
        return 1;
    }
    if (!(*(&D_1BD421 + c) & 7)) {
        return 1;
    }
    if (WordFilter_InList((s8 *) name, (s8 *) gWordFilterList)) {
        return 1;
    }
    return 0;
}

s32 WordFilter_ReadEntry(s8 *src, s8 *dst, s32 max);
u32 strlen(const void *s);
s32 strncmp(const void *a, const void *b, s32 n);

s32 WordFilter_InList(s8 *name, s8 *list)
{
    s8 buf[0x60];
    s32 len = strlen(name);
    s32 n;

    while (*list != 0) {
        n = WordFilter_ReadEntry(list, buf, len);
        if (n != -1 && strncmp(name, buf, n) == 0) {
            return 1;
        }
        list += 0x10;
    }
    return 0;
}

s32 WordFilter_ReadEntry(s8 *src, s8 *dst, s32 max)
{
    s8 len = src[0xF];
    s32 n;
    s32 left;
    s32 i;
    s32 j;
    s32 off;

    if (max < len) {
        return -1;
    }
    n = 0;
    left = len;
    if (src[0x1F] == -1) {
        off = 0;
        do {
            for (j = 0; j < 14; j++) {
                dst[off + j] = src[j];
            }
            src += 0x10;
            off += 14;
            n++;
            left -= 14;
        } while (src[0x1F] == -1);
    }
    for (i = 0; i < left; i++) {
        dst[n * 14 + i] = src[i];
    }
    *(dst + i + n * 14) = 0;
    return len;
}

s8 Kbd_RunFiltered(char *text, s16 held, s16 pressed)
{
    s8 n = Kbd_Run(text, held, pressed);

    if (n != 0 && n > 0) {
        if (WordFilter_Check((s32)text, 1) == 0) {
            Notice_Push(0, 0x1C, 0);
        }
    }
    return n;
}

extern LobbyRecs gLobbyRecs;

void func_00793BE0(void)
{
    Text_PrintfColored(gLobbyRecs.rec[1].subName[gLobbyRecs.rec[1].cur]);
}

/* unk_793070's .data (0x7FF320-0x801DE0) */
s8 gWordFilterList[3328] = {
#include "lobby/007FF320.inc"
};
u32 D_00800020[8] = {
#include "lobby/00800020.inc"
};
u32 D_00800040[12] = {
#include "lobby/00800040.inc"
};
u32 D_00800070[4] = {
#include "lobby/00800070.inc"
};
u32 D_00800080[4] = {
#include "lobby/00800080.inc"
};
u32 D_00800090[12] = {
#include "lobby/00800090.inc"
};
u32 D_008000C0[8] = {
#include "lobby/008000C0.inc"
};
u32 D_008000E0[8] = {
#include "lobby/008000E0.inc"
};
u32 D_00800100[1848] = {
#include "lobby/00800100.inc"
};
