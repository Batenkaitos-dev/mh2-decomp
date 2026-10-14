#ifndef LBGUILD_H
#define LBGUILD_H

/*
 * lbguild.bin: the guild's quests, an overlay with no code that the lobby
 * loads at 0x8FAE00. It is an asset: its bytes are not in the repository.
 * config/assets/lbguild/lbguild.txt describes its layout, and the build
 * writes its C from the user's copy (tools/asset_c.py; CONTRIBUTING,
 * "Described assets").
 *
 * Layout: 328 quests, each a GuildQuest record, its eight Shift-JIS texts
 * and the table of them (GuildQuest.texts); then the quest lists (GuildQuest
 * pointers, each list starting with the same quest, most ending with NULL);
 * last, the table of the 14 lists at 0x924240, which the lobby reads as
 * gGuildQuestLists (Quest_GetDiscInfo).
 */
#include "common.h"
#include "quest.h"

/*
 * A quest of the guild's lists (0x50 bytes). The first 0x2C bytes are laid
 * out as a stage file's QuestInfo (quest.h): e.g. timeLimit 90000 frames
 * (50 minutes), texts the quest's eight texts (title, goal, ...).
 */
typedef struct GuildQuest {
    u32 flags;                  /* 0x00 */
    u16 stars;                  /* 0x04: the difficulty */
    u16 unk06;
    s32 unk08;
    s32 reward;                 /* 0x0C */
    s32 penalty;                /* 0x10 */
    s32 unk14;
    s32 unk18;
    s32 timeLimit;              /* 0x1C: frames */
    u32 unk20;
    char **texts;               /* 0x24: 8 texts */
    u16 unk28;
    u16 id;                     /* 0x2A */
    QuestGoal goals[3];         /* 0x2C */
    u32 unk44;
    u32 unk48;
    u32 unk4C;
} GuildQuest;

#endif /* LBGUILD_H */
