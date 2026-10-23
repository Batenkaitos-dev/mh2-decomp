/*
 * The quests' lookups: a quest's QuestInfo by id, from the disc's lists by
 * id group (gGuildQuestLists) or from the three downloaded quest files
 * (gQuestFiles, ids from QUEST_ID_DOWNLOADED); the quest taken
 * (Quest_CurrentInfo); a quest's texts, stars and list flags. The quest
 * desk, the quest counter and the quest room read quests through them.
 * The file has no data of its own.
 */

#include "common.h"
#include "lobby_menu.h"
#include "quest.h"
#include "lobby_labels.h"
#include "text.h"
#include "quest_order_terms.h"
#include "game_util.h"

extern u32 gStageFile;
extern u16 gLobbyQuestId;
extern QuestFile gQuestFiles[3];
extern u16 gQuestGroupLastIds[];
extern QuestInfo **gGuildQuestLists[];
/* The QuestInfo of quest `id` on the disc (a downloaded id goes to
 * Quest_FindDownloaded): entry id % 100 of the list of the first id group
 * whose last id (gQuestGroupLastIds) is at least id. Quest_SetCurrent records
 * the entry. */
QuestInfo *Quest_GetDiscInfo(u16 id)
{
    s32 i;
    u16 *groupLast = gQuestGroupLastIds;
    QuestInfo **entry;

    if (id >= QUEST_ID_DOWNLOADED) {
        return Quest_FindDownloaded(id);
    }
    for (i = 0; *groupLast != 0; groupLast++, i++) {
        if (*groupLast >= id) {
            break;
        }
    }
    entry = gGuildQuestLists[i];
    entry += id % 100;
    Quest_SetCurrent(id, entry, 1);
    return *entry;
}

/* The QuestInfo of quest `id`, downloaded or on the disc, without
 * Quest_GetInfo's checks (a downloaded one may be NULL or another quest's). */
QuestInfo *Quest_GetInfoAny(u16 id)
{
    if (id >= QUEST_ID_DOWNLOADED) {
        return Quest_FindDownloaded(id);
    }
    return Quest_GetDiscInfo(id);
}

/* The QuestInfo of downloaded quest `id` in the three quest files
 * (gQuestFiles), or NULL. A file's first word is its QuestInfo's offset. */
QuestInfo *Quest_FindDownloaded(u16 id)
{
    s8 i = 0;
    QuestFile *file = gQuestFiles;

    for (; i < 3; i++, file++) {
        QuestInfo *questInfo = (QuestInfo *)(file->info + (u32)file);

        if (questInfo->id == id) {
            return questInfo;
        }
    }
    return NULL;
}

/* The QuestInfo of the stage file loaded (gStageFile: its first word is the
 * QuestInfo's offset in the file). */
QuestInfo *Quest_CurrentInfo(void)
{
    return (QuestInfo *)(((StageHeader *)gStageFile)->info + gStageFile);
}

/* The slot (0-2) of gQuestFiles that holds downloaded quest `id`, or 3 when
 * none does. */
s8 Quest_DownloadedSlot(u16 id)
{
    s8 i = 0;
    QuestFile *file = gQuestFiles;

    for (; i < 3; i++, file++) {
        QuestInfo *questInfo = (QuestInfo *)(file->info + (u32)file);

        if (questInfo->id == id) {
            return i;
        }
    }
    return i;
}

/* Text `which` of quest `id` (QuestInfo.texts). A downloaded quest's texts
 * are offsets in its file; they are read only while the downloaded quests
 * are ready (LobbyState.dlQuestsReady) and the file holds that quest.
 * Otherwise, and for a missing quest, the unknown quest's text
 * (gQuestCounterTexts[QUEST_COUNTER_TEXT_UNKNOWN_QUEST]). */
char *Quest_GetText(u16 id, s32 which)
{
    QuestInfo *info = Quest_GetInfoAny(id);

    if (id >= QUEST_ID_DOWNLOADED) {
        s8 slot;
        u32 fileBase;

        if (info == NULL) {
            return gQuestCounterTexts[QUEST_COUNTER_TEXT_UNKNOWN_QUEST];
        }
        if (id != info->id || gLobby->dlQuestsReady == 0) {
            return gQuestCounterTexts[QUEST_COUNTER_TEXT_UNKNOWN_QUEST];
        }
        slot = Quest_DownloadedSlot(id);
        fileBase = (u32)&gQuestFiles[slot];
        {
            s32 off = ((s32 *)((u32)info->texts + fileBase))[which];

            return (char *)(off + fileBase);
        }
    }
    if (info == NULL) {
        return gQuestCounterTexts[QUEST_COUNTER_TEXT_UNKNOWN_QUEST];
    }
    return ((char **)info->texts)[which];
}

/* Text `which` of the quest selected in the lobby (gLobbyQuestId); the quest
 * code reads its texts through it. */
char *Quest_SelectedText(s32 which)
{
    QuestInfo *info;

    if (gLobbyQuestId >= QUEST_ID_DOWNLOADED) {
        return Quest_GetText(gLobbyQuestId, which);
    }
    info = Quest_GetDiscInfo(gLobbyQuestId);
    return ((char **)info->texts)[which];
}

/* The QuestInfo of quest `id`, or NULL: a downloaded quest's only while the
 * downloaded quests are ready and its file holds that id. */
QuestInfo *Quest_GetInfo(u16 id)
{
    QuestInfo *info = NULL;

    if (id >= QUEST_ID_DOWNLOADED) {
        if (gLobby->dlQuestsReady == 1) {
            info = Quest_FindDownloaded(id);
            if (info != NULL && info->id != id) {
                info = NULL;
            }
        }
    } else {
        info = Quest_GetDiscInfo(id);
    }
    return info;
}

/* The marks the quest list shows for quest `id`: QuestInfo.flags bits 0-2
 * for a disc quest, 8 for the quests Quest_IsSpecialId lists, 0x10 for the
 * other downloaded quests; 0 for a missing quest. */
u8 Quest_GetListFlags(u16 id)
{
    QuestInfo *info = Quest_GetInfo(id);
    u8 listFlags;

    if (info == NULL) {
        return 0;
    }
    if (id >= QUEST_ID_DOWNLOADED) {
        listFlags = 0;
    } else {
        listFlags = info->flags & 7;
    }
    if (Quest_IsSpecialId(id) == 1) {
        listFlags |= 8;
    } else if (id >= QUEST_ID_DOWNLOADED) {
        listFlags |= 0x10;
    }
    return listFlags;
}

/* The difficulty stars of quest `id` (QuestInfo.stars bits 0-3); quests 3001
 * and 3002 show 4 and 6 stars whatever their info holds. 0 for a missing
 * quest. */
u8 Quest_GetStars(u16 id)
{
    QuestInfo *info = Quest_GetInfo(id);
    u8 starCount;

    if (info == NULL) {
        return 0;
    }
    switch (id) {
    case 3001:
        starCount = 4;
        break;
    case 3002:
        starCount = 6;
        break;
    default:
        starCount = info->stars & 0xF;
        break;
    }
    return starCount;
}
