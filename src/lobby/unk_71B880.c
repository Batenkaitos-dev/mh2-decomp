#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "save.h"
#include "lobby_globals.h"
#include "quest.h"


QuestInfo *Quest_GetDiscInfo(u16 id);
s32 Quest_CheckOrderTerms(u8, u8);
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeason(void);
s32 Quest_MatchesSeasonTime(u16 id, s32 a, s32 b);
s32 Game_IsOnline(void);
s32 Cond_Check(u16 id);
s32 Save_TestEventFlag(s32 flag);
/* unit's own declaration: with the definition's u16 (u16 area) (sub_main/unk_376C00.c), Quest_IsOffered: 2 of 224 words differ */
s32 Area_MapGroup(u16 id);
s32 Monster_IsAvailable(u16 id);
s32 ElderForecast_QuestAppears(u16 id, s32 kind);

s32 MapGroup_IsOpen(u16 kind)
{
    if (Game_IsOnline() == 0) {
        switch (kind) {
        case 6:
            return 1;
        case 3:
            if (Cond_Check(7) == 1) {
                return 1;
            }
            break;
        case 4:
            if (Cond_Check(0xA) == 1) {
                return 1;
            }
            break;
        case 0xB:
            if (Cond_Check(0x10) == 1) {
                return 1;
            }
            break;
        case 5:
            if (Cond_Check(0x15) == 1) {
                return 1;
            }
            break;
        case 0xD:
        case 0xE:
        case 0xF:
            if (Cond_Check(0x15) == 1) {
                return 1;
            }
            break;
        case 2:
            if (Cond_Check(0x4F) == 1 && gSave.unk36E8 != 0) {
                return 1;
            }
            break;
        }
    } else {
        switch (kind) {
        case 6:
        case 3:
        case 4:
            return 1;
        case 0xB:
            if (gSave.hr >= 5) {
                return 1;
            }
            break;
        case 5:
            if (gSave.hr >= 0xB) {
                return 1;
            }
            break;
        case 0xD:
        case 0xE:
        case 0xF:
            if (gSave.hr >= 0x16) {
                return 1;
            }
            break;
        case 2:
            if (Save_TestEventFlag(0x122) == 1) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

s32 Quest_MatchesSeasonTime(u16 id, s32 a, s32 b)
{
    QuestInfo *q = Quest_GetDiscInfo(id);
    u32 mask;

    if (q == NULL) {
        return 0;
    }
    switch (a) {
    case 0:
        mask = 0x20000;
        break;
    case 1:
        mask = 0x40000;
        break;
    case 2:
        mask = 0x10000;
        break;
    }
    if (!(q->flags & mask)) {
        return 0;
    }
    switch (b) {
    case 0:
        mask = 0x80000;
        break;
    case 1:
        mask = 0x100000;
        break;
    }
    return (q->flags & mask) != 0;
}

s32 Quest_CheckOrder(u16 id)
{
    s32 season;
    s32 pm;
    QuestInfo *q;

    q = Quest_GetDiscInfo(id);
    season = LobbyWorld_GetSeason() & 0xFF;
    pm = LobbyTime_IsPm() & 0xFF;
    if (gLobby->questPaid != 0) {
        if (Quest_CheckOrderTerms(q->terms, 0) == 0) {
            return 2;
        }
        goto block_7;
    }
    if (Quest_CheckOrderTerms(q->terms, 1) == 0) {
        return 2;
    }
block_7:
    if (id >= 50000) {
        return 0;
    }
    return Quest_MatchesSeasonTime(id, season & 0xFF, pm & 0xFF) != 1;
}

s32 Quest_IsOffered(u16 id, s32 kind, s32 all)
{
    QuestInfo *q = Quest_GetDiscInfo(id);
    u8 lv = LobbyWorld_GetSeason();
    u8 b = LobbyTime_IsPm();

    if (q == NULL) {
        return 0;
    }
    if (q->flags & 0x200000) {
        return 0;
    }
    if (all == 0 && (q->flags & 0x400000)) {
        return 0;
    }
    if (q->flags & 0x4000000) {
        return 0;
    }
    if (Quest_MatchesSeasonTime(id, lv, b) == 0) {
        return 0;
    }
    if (Game_IsOnline() == 0) {
        if (MapGroup_IsOpen(Area_MapGroup(q->map)) == 0) {
            return 0;
        }
        switch (id) {
        case 1001:
            break;
        case 1002:
        case 1003:
        case 1004:
        case 1005:
            if (Cond_Check(3) == 0) {
                return 0;
            }
            break;
        case 1006:
        case 1008:
        case 1021:
        case 1022:
        case 1023:
        case 1024:
            if (Cond_Check(7) == 0) {
                return 0;
            }
            break;
        case 1011:
        case 1012:
        case 1026:
        case 1038:
        case 1039:
        case 1040:
        case 1042:
        case 1044:
            if (Cond_Check(0xA) == 0) {
                return 0;
            }
            break;
        case 1014:
        case 1016:
        case 1056:
        case 1059:
        case 1060:
            if (Cond_Check(0x10) == 0) {
                return 0;
            }
            break;
        case 1017:
        case 1018:
        case 1030:
        case 1032:
        case 1033:
        case 1063:
        case 1064:
        case 1065:
        case 1066:
        case 1067:
        case 1068:
        case 1070:
        case 1071:
            if (Cond_Check(0x15) == 0) {
                return 0;
            }
            break;
        case 1036:
        case 1037:
        case 1061:
        case 1062:
        case 1074:
        case 1075:
        case 1084:
        case 1085:
            if (Cond_Check(0x1A) == 0) {
                return 0;
            }
            break;
        case 1034:
        case 1035:
        case 1054:
        case 1055:
        case 1072:
        case 1073:
            if (Cond_Check(0x1B) == 0) {
                return 0;
            }
            break;
        case 1009:
            if (Save_TestEventFlag(0x9A) == 0) {
                return 0;
            }
            break;
        case 1049:
        case 1050:
            if (Save_TestEventFlag(0x9B) == 0) {
                return 0;
            }
            break;
        case 1047:
        case 1048:
            if (Save_TestEventFlag(0xA5) == 0) {
                return 0;
            }
            break;
        case 1081:
        case 1082:
            if (Save_TestEventFlag(0xAC) == 0) {
                return 0;
            }
            break;
        case 1046:
            if (Save_TestEventFlag(0x9C) == 0) {
                return 0;
            }
            break;
        case 1045:
            if (Save_TestEventFlag(0xA7) == 0) {
                return 0;
            }
            break;
        case 1078:
            if (Save_TestEventFlag(0xAB) == 0) {
                return 0;
            }
            break;
        default:
            if ((q->goals[0].flags & 5) && Monster_IsAvailable(q->goals[0].target) == 0) {
                return 0;
            }
            break;
        case 1086:
        case 1087:
            break;
        }
    } else {
        if (MapGroup_IsOpen(Area_MapGroup(q->map)) == 0) {
            return 0;
        }
        switch (id) {
        case 5001:
        case 5002:
        case 5003:
        case 5004:
        case 5005:
        case 5006:
        case 5007:
        case 5008:
            return 0;
        default:
            if ((q->goals[0].flags & 5) && Monster_IsAvailable(q->goals[0].target) == 0) {
                return 0;
            }
            break;
        }
    }
    if ((q->goals[0].flags & 5) && q->type == 6 && ElderForecast_QuestAppears(id, kind) == 0) {
        return 0;
    }
    return 1;
}
