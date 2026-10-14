/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_vobj.h"
#include "quest.h"

u8 MonsterGroup_First(u8 group);
s32 Wanted_IsSet(s16 *p);

s32 Wanted_MatchesQuest(QuestGoal *goal, TownEntry *e) {
    s32 group;

    if (e->v[0] == 3) {
        return 0;
    }
    if (Wanted_IsSet(e->v) == 0) {
        return 0;
    }
    if (!(goal->flags & 1)) {
        return 0;
    }
    group = MonsterGroup_First(goal->target) & 0xFF;
    return group == (MonsterGroup_First(*(u8 *)&e->v[1]) & 0xFF); /* the low byte of v[1] (lbu) */
}
