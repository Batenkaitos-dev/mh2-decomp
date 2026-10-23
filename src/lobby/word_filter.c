/*
 * The chat word filter, and two answers for sub_main's quest page.
 *
 * - The word filter: WordFilter_Scan looks for the words of
 *   gWordFilterList in a line, compared in upper case with look-alike
 *   characters folded (WordFilter_Normalize), where they stand alone
 *   (WordFilter_IsWordEnd), and says whether the line is clean or stars
 *   the hits out. Kbd_RunFiltered runs the chat keyboard through it and
 *   shows a notice when a line typed held a filtered word.
 * - Two answers for sub_main's quest terms page in the lobby:
 *   ArenaCup_DrawTermChoice prints the arena cup's choice in place of the
 *   time limit, and LobbyQuest_IsMustOrder says whether the quest taken
 *   must be ordered (QUEST_FLAG_MUST_ORDER).
 *
 * gWordFilterList is a list of 16-byte entries: a word's characters in the
 * first 14 bytes and its length in the last; a longer word goes on in the
 * entries after it, whose last byte is -1; an entry with a zero first byte
 * ends the list. The file's data is that list alone.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "chat_log.h"
#include "libc_ctype.h"
#include "quest.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "lobby_labels.h"
#include "area_draw.h"
#include "text.h"
#include "word_filter.h"

s8 Kbd_Run(char *text, s16 held, s16 pressed);

/* Runs the word filter on text (WordFilter_Scan), starring the hits out
 * when replace's low byte is set; 1 if the text was clean. */
s32 WordFilter_Check(s32 text, s32 replace)
{
    return WordFilter_Scan((s8 *) text, (u8)replace);
}

/* gWordFilterList's entries: a word's characters, then its length in the
 * entry's last byte; a longer word goes on in the entries after it. */
#define WORD_FILTER_ENTRY_SIZE 16   /* an entry's bytes */
#define WORD_FILTER_ENTRY_CHARS 14  /* the word's characters an entry holds */
#define WORD_FILTER_LEN 15          /* the byte that holds the word's length, in its first entry */
#define WORD_FILTER_MORE (-1)       /* that byte in an entry the word goes on into */
#define WORD_FILTER_LIST_SIZE 10944 /* gWordFilterList's bytes: 684 entries */

extern s8 gCtypeTable[];
extern s8 gWordFilterList[];
u32 strlen(const void *s);
char *strncpy(char *dst, const char *src, u32 n);

/* The chat word filter: finds the words of the list at gWordFilterList[0] in str
 * (compared in upper case, 1/! also matching L, ( and < ignored at the start of a
 * word) where they stand alone; returns 1 when str is clean, else 0 (with
 * replace set, after overwriting each hit with asterisks). */
s32 WordFilter_Scan(s8 *str, s32 replace)
{
    s8 *list;
    s8 *entry;
    s32 clean;
    s32 len;
    s32 i;
    s32 j;
    s32 k;
    s32 wordLen;
    s32 hit;
    s8 rawChar;
    s8 upper[80];
    s8 word[80];

    clean = 1;
    list = gWordFilterList;
    len = strlen(str);
    WordFilter_Normalize(upper, str, len);
    i = 0;
    while (upper[i] != 0) {
        hit = 0;
        for (entry = list; *entry != 0; entry += WORD_FILTER_ENTRY_SIZE) {
            wordLen = WordFilter_ReadEntry(entry, word, len);
            if (wordLen == -1) {
                continue;
            }
            k = 0;
            j = i;
            while (!word[k] == 0 && str[j] != 0) {
                if (gCtypeTable[upper[j] + 1] & CTYPE_ALNUM) {
                    if (upper[j] == word[k]) {
                        goto match;
                    }
                    if ((rawChar = str[j]) == '1' || rawChar == '!') {
                        if (word[k] != 'L') {
                            goto done;
                        }
                        goto match;
                    } else if (rawChar == '(' || rawChar == '<') {
                        if (k != 0) {
                            goto done;
                        }
                        goto next;
                    } else {
                        goto done;
                    }
                match:
                    k++;
                    if (k == wordLen) {
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
                if ((gCtypeTable[str[j] + 1] & CTYPE_ALNUM) && WordFilter_IsWordEnd(&upper[j]) == 0) {
                    hit = 0;
                    continue;
                }
                if (replace == 0) {
                    return 0;
                }
                for (hit = i; hit < j; hit++) {
                    upper[hit] = '*';
                }
                strncpy((char *)str + i, (char *)upper + i, j - i);
                i = j;
                clean = 0;
                break;
            }
        }
        while (upper[i] != 0) {
            if (hit != 0) {
                break;
            }
            if (!(gCtypeTable[str[i++] + 1] & CTYPE_ALNUM) && (gCtypeTable[upper[i] + 1] & CTYPE_ALNUM)) {
                break;
            }
        }
    }
    return clean;
}


/* Copies the n + 1 bytes of src (n characters and the terminator) to dst
 * in upper case, the look-alike characters folded to the letters they
 * stand for: @ to A, $ and 5 to S, < and ( to C, ! and 1 to I, 2 to Z,
 * 0 to O. */
void WordFilter_Normalize(s8 *dst, s8 *src, s32 n)
{
    s32 i;

    for (i = 0; i <= n; i++) {
        dst[i] = src[i];
        if (gCtypeTable[src[i] + 1] & CTYPE_LOWER) {
            dst[i] -= 32;
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

/* Whether a hit that ends just before name stands alone: name is at the
 * line's end or at a character that is neither a letter nor a digit, or a
 * word of the list starts there (WordFilter_InList). 1 if so. */
s32 WordFilter_IsWordEnd(s8 *name)
{
    s8 first;

    first = *name;
    if (first == 0) {
        return 1;
    }
    if (!((gCtypeTable + 1)[first] & CTYPE_ALNUM)) {
        return 1;
    }
    if (WordFilter_InList((s8 *) name, (s8 *) gWordFilterList)) {
        return 1;
    }
    return 0;
}

s32 strncmp(const void *a, const void *b, s32 n);

/* Whether name starts with one of list's words (each read with
 * WordFilter_ReadEntry; those longer than name are skipped). 1 if so. */
s32 WordFilter_InList(s8 *name, s8 *list)
{
    s8 buf[96];
    s32 len = strlen(name);
    s32 wordLen;

    while (*list != 0) {
        wordLen = WordFilter_ReadEntry(list, buf, len);
        if (wordLen != -1 && strncmp(name, buf, wordLen) == 0) {
            return 1;
        }
        list += WORD_FILTER_ENTRY_SIZE;
    }
    return 0;
}

/* Reads the word of list entry src into dst: its characters, 14 from each
 * entry it goes on into (the next entry's last byte -1), then a
 * terminator. Returns its length, or -1 with nothing read when it is
 * longer than max. */
s32 WordFilter_ReadEntry(s8 *src, s8 *dst, s32 max)
{
    s8 len = src[WORD_FILTER_LEN];
    s32 extra;
    s32 left;
    s32 i;
    s32 j;
    s32 dstOff;

    if (max < len) {
        return -1;
    }
    extra = 0;
    left = len;
    if (src[WORD_FILTER_ENTRY_SIZE + WORD_FILTER_LEN] == WORD_FILTER_MORE) {
        dstOff = 0;
        do {
            for (j = 0; j < WORD_FILTER_ENTRY_CHARS; j++) {
                dst[dstOff + j] = src[j];
            }
            src += WORD_FILTER_ENTRY_SIZE;
            dstOff += WORD_FILTER_ENTRY_CHARS;
            extra++;
            left -= WORD_FILTER_ENTRY_CHARS;
        } while (src[WORD_FILTER_ENTRY_SIZE + WORD_FILTER_LEN] == WORD_FILTER_MORE);
    }
    for (i = 0; i < left; i++) {
        dst[extra * WORD_FILTER_ENTRY_CHARS + i] = src[i];
    }
    *(dst + i + extra * WORD_FILTER_ENTRY_CHARS) = 0;
    return len;
}

/* Runs the chat keyboard (Kbd_Run) on text with the pad's held and pressed
 * buttons. When a line is entered (a result above 0) and the word filter
 * finds a word in it, the hits are starred out and the notice
 * NOTICE_MSG_WORD_FILTERED is shown. Returns Kbd_Run's result. */
s8 Kbd_RunFiltered(char *text, s16 held, s16 pressed)
{
    s8 result = Kbd_Run(text, held, pressed);

    if (result != 0 && result > 0) {
        if (WordFilter_Check((s32)text, 1) == 0) {
            Notice_Push(NOTICE_KIND_MESSAGE, NOTICE_MSG_WORD_FILTERED, 0);
        }
    }
    return result;
}

extern LobbyRecs gLobbyRecs;

/* Prints the name of the value chosen for the arena cup room's first
 * record (gLobbyRecs.rec[1]): sub_main's quest terms page prints it for an
 * arena cup quest where other quests show their time limit. */
void ArenaCup_DrawTermChoice(void)
{
    Text_PrintfColored(gLobbyRecs.rec[1].subName[gLobbyRecs.rec[1].cur]);
}

extern u16 gLobbyQuestId;

/* Whether the lobby's quest (gLobbyQuestId) must be ordered
 * (QUEST_FLAG_MUST_ORDER): sub_main's quest terms page asks it in the
 * lobby, as Quest_IsMustOrder answers it for the field's quest. */
s32 LobbyQuest_IsMustOrder(void)
{
    return (Quest_GetInfoAny(gLobbyQuestId)->flags & QUEST_FLAG_MUST_ORDER) != 0;
}

/* The file's data: the word filter's list. */
s8 gWordFilterList[WORD_FILTER_LIST_SIZE] = {
#include "lobby/007FF320.inc"
};
