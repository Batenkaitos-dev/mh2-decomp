/*
 * The townspeople's talk: which line a talk opens with, the talk data
 * file's texts, the talk's run, and the talk scripts' tables.
 *
 * A townsperson's script is its id (NpcWork.id, gTalkScript). Talk_Start
 * picks the line the talk opens with, in two ways:
 * - the script's events (gTalkEvents[script]): the first event step that
 *   the script's check (Talk_Check) accepts gives the line;
 * - otherwise the script's selection tree (gTalkSelects[script]), three
 *   levels deep: the first entry whose village condition holds
 *   (Cond_Check), then the first whose season holds (Talk_CheckSeason),
 *   then the first whose time of day holds (Talk_CheckTimeOfDay). That entry's
 *   lines give the line of the first talk with the townsperson and the
 *   line of every later one (NpcWork.talked).
 * Talk_Step and Talk_Update then run the lines (gTalkLines[script]): their
 * yes/no questions skip ahead by the answer, and the last line may hand
 * over one of the script's gifts (gTalkGifts[script]). Talk_Say opens a
 * single line. Each line's text is in the talk data file, loaded at
 * gLbguildFree when the town's area loads: its records {script, offset}
 * lead to a table of the script's line offsets (Talk_FindScript,
 * Talk_LineTextAddr).
 *
 * The file opens with the selection tree's two tests (Talk_CheckSeason,
 * Talk_CheckTimeOfDay). The talk's end (Talk_Finish: the event step's
 * action, the gift) and the events' checks (Talk_Check and each script's
 * own) are the talk scripts' code.
 *
 * The data, by script in the game's order: the events (gTalkEvents_N),
 * the selection tree (gTalkSelects_N, its lists gTalkSelects_N_a and
 * gTalkSelects_N_a_b, named for the entry that leads to each, and the
 * lines gTalkSelects_N_a_b_c), the lines (gTalkLines_N) and the gifts
 * (gTalkGifts_N); then the four tables by script. A script with no gifts,
 * and a selection with no lines of its own, points at a record of zeros
 * in the lobby's .bss (line 0, no gift).
 */
#include "common.h"
#include "em.h"
#include "lobby_npc.h"
#include "player.h"
#include "save.h"
#include "quest.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "scene.h"
#include "chat_log.h"
#include "lobby_labels.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "pad.h"
#include "village_cond.h"
#include "area_draw.h"
#include "talk.h"
#include "font.h"
#include "gallery_objects.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "menu_steps.h"
#include "my_house.h"
#include "rand.h"
#include "talk_scripts.h"
#include "village_records.h"
#include "libc.h"
extern s32 gTalkEventFlag;
extern s32 gTownMode;
extern u16 gTalkEventStep;
extern void *gTalkScriptRec;
extern u16 gTalkScript;
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
u8 SavePouch_Add(u16 item, s16 n);
extern s32 gTalkLine;

extern TalkEvent *gTalkEvents[];
extern TalkSel *gTalkSelects[];
extern s32 gTalkNpcName;
extern TalkSel *gTalkCondList;
extern TalkSel *gTalkStateList;
extern TalkSelLines *gTalkSelLines;
extern s32 gTalkTextFrames;
extern u16 gTalkCondId;
extern u16 gTalkStateId;
extern u16 gTalkModeId;
extern s8 gTalkMode;

/* A selection list's season test: `sel` 0 is any season, 1 to 3 the season
 * SEASON_WARM, SEASON_COLD or SEASON_BREEDING (the season + 1). Returns 1
 * when the lobby's season matches. */
s32 Talk_CheckSeason(u16 sel)
{
    s32 season = LobbyWorld_GetSeason();

    switch (sel) {
    case 3:
        if (season == SEASON_BREEDING) {
            return 1;
        }
        break;
    case 2:
        if (season == SEASON_COLD) {
            return 1;
        }
        break;
    case 1:
        if (season == SEASON_WARM) {
            return 1;
        }
        break;
    case 0:
        return 1;
    }
    return 0;
}

/* A selection list's time test: `sel` 0 is any time, 1 the morning, 2 the
 * afternoon (LobbyTime_IsPm). Returns 1 when the lobby's time matches. */
s32 Talk_CheckTimeOfDay(u16 sel)
{
    s32 isPm = LobbyTime_IsPm();

    switch (sel) {
    case 2:
        if (isPm == 1) {
            return 1;
        }
        break;
    case 1:
        if (isPm == 0) {
            return 1;
        }
        break;
    case 0:
        return 1;
    }
    return 0;
}

/*
 * Starts the talk with townsperson npc: the yes/no cursor at yes, the
 * townsperson's name for the window, its record in the talk data file, and
 * the talk sound unless quiet. In town mode TOWN_MODE_TALK (and mode 5,
 * which the town's frame runs the same way) a townsperson with a gift
 * waiting opens the gift instead (NpcGift_Open); offline in a house, one
 * with a meal visit due opens the meal (MealScene_Open). The house's cook
 * in another player's house says line 29 after the meal, line 27 before.
 * Otherwise the opening line comes from the script's events, or from its
 * selection tree (the file's opening comment). Townspeople 69 to 71 open
 * with a random line of their first ten instead; their line 9 is for the
 * afternoon only, and in the morning it takes event step 99, which the
 * talk's end acts on. The first talk with a townsperson marks it talked.
 * The searches give up after TALK_EVENTS_MAX events or TALK_SEL_ENTRIES_MAX
 * entries of a list.
 */
void Talk_Start(Em *npc, s8 quiet)
{
    NpcWork *npcWork;
    TalkEvent *event;
    TalkSel *sel;
    s32 i;

    gTalkTextFrames = 0;
    gTalkMode = TALK_CHOICE_YES;
    gTalkLine = 0;
    npcWork = (NpcWork *)&npc->work;
    gTalkScriptRec = Talk_FindScript(npcWork->id);
    gTalkNpcName = (s32)npcWork->name;
    gTalkEventStep = TALK_EVENT_NONE;
    gTalkEventFlag = -1;
    gTalkScript = npcWork->id;
    if (quiet == 0) {
        LobbySe_Play(LOBBY_SE_TALK);
    }
    switch (gTownMode) {
    case TOWN_MODE_TALK:
    case 5:
        if (Village_HasGift(npcWork->id) == 1) {
            NpcGift_Open(npc);
            return;
        }
        break;
    }
    if (Game_IsOnline() == 0 && Area_IsHouse(gScene.area) == 1 && Village_HasMealVisit(npcWork->id) == 1) {
        MealScene_Open(npc);
        return;
    }
    if (npcWork->id == NPC_HOUSE_COOK && MyHouse_IsOwner() == 0) {
        if (gLobby->mealEaten != 0) {
            gTalkLine = 29;
        } else {
            gTalkLine = 27;
        }
        return;
    }
    event = gTalkEvents[npcWork->id];
    i = 0;
    while (event->step != TALK_EVENT_NONE) {
        if (Talk_Check(npcWork->id, event->step) == 1) {
            gTalkEventStep = event->step;
            gTalkLine = event->line;
            break;
        }
        if (++i > TALK_EVENTS_MAX) {
            break;
        }
        event++;
    }
    if (gTalkEventStep == TALK_EVENT_NONE) {
        sel = gTalkSelects[npcWork->id];
        i = 0;
        while (sel->next != NULL) {
            if (Cond_Check(sel->id) == 1) {
                gTalkCondId = sel->id;
                gTalkCondList = sel->next;
                break;
            }
            if (++i > TALK_SEL_ENTRIES_MAX) {
                break;
            }
            sel++;
        }
        sel = gTalkCondList;
        i = 0;
        while (sel->next != NULL) {
            if (Talk_CheckSeason(sel->id) == 1) {
                gTalkStateId = sel->id;
                gTalkStateList = sel->next;
                break;
            }
            if (++i > TALK_SEL_ENTRIES_MAX) {
                break;
            }
            sel++;
        }
        sel = gTalkStateList;
        i = 0;
        while (sel->next != NULL) {
            if (Talk_CheckTimeOfDay(sel->id) == 1) {
                gTalkModeId = sel->id;
                gTalkSelLines = sel->next;
                if (npcWork->talked == 0) {
                    gTalkLine = ((TalkSelLines *)sel->next)->firstLine;
                } else {
                    gTalkLine = ((TalkSelLines *)sel->next)->laterLine;
                }
                break;
            }
            if (++i > TALK_SEL_ENTRIES_MAX) {
                break;
            }
            sel++;
        }
        if (gTalkEventStep == TALK_EVENT_NONE) {
            switch (npcWork->id) {
            case 69:
            case 70:
            case 71:
                gTalkLine = (u16)Rand_Next(1) % 10;
                if (gTalkLine == 9) {
                    if (LobbyTime_IsPm() == 0) {
                        gTalkEventStep = 99;
                    } else {
                        gTalkLine = 0;
                    }
                }
                break;
            }
        }
        if (npcWork->talked == 0) {
            npcWork->talked++;
        }
    }
}

/* Marks every active townsperson as not talked to, so that each talk opens
 * with its first-time line again. */
void Npc_ResetTalked(void)
{
    Em *em;
    s32 i;

    i = 0;
    em = (Em *) &gEms;
    do {
        if (em->active != 0) {
            ((NpcWork *) &em->work)->talked = 0;
        }
        i = (s16)((i + 1));
        em++;
    } while (i < EM_COUNT);
}

/* Returns script id's record in the talk data file (a TalkFileScript; the
 * records end with TALK_SCRIPT_END), or NULL when the file has none. */
u8 *Talk_FindScript(u16 id)
{
    u8 *recAddr;
    s32 i;
    TalkFileScript rec;

    recAddr = gLbguildFree;
    memcpy(&rec, recAddr, sizeof(TalkFileScript));
    i = 0;
    while (rec.script != TALK_SCRIPT_END) {
        if (rec.script == id) {
            return recAddr;
        }
        i++;
        recAddr = gLbguildFree + i * 8;
        memcpy(&rec, recAddr, sizeof(TalkFileScript));
    }
    return NULL;
}

/* The talk data file's start. Talk_LineTextAddr reads the file through
 * this helper, and adds the line's offset to it before the script's: the
 * game's code computes the address in that order. */
static inline u8 *TalkFile(void)
{
    return gLbguildFree;
}

/* The address of the text of line `line` of the current script, from its
 * record (gTalkScriptRec): the record's offset leads to the script's table
 * of line offsets, each from the table's start. */
s32 Talk_LineTextAddr(u16 line)
{
    TalkFileScript rec;
    s32 off;
    s32 pos;

    memcpy(&rec, gTalkScriptRec, sizeof(TalkFileScript));
    pos = rec.offset;
    pos += line * sizeof(s32);
    memcpy(&off, gLbguildFree + pos, sizeof(s32));
    return rec.offset + ((s32)TalkFile() + off);
}

/* The records of zeros in the lobby's .bss that the tables point at: the
 * gift lists of the scripts with no gifts, and the lines of the selections
 * with none of their own (line 0). */
extern TalkGift gTalkGifts_2[];
extern TalkGift gTalkGifts_6[];
extern TalkGift gTalkGifts_9[];
extern TalkGift gTalkGifts_15[];
extern TalkGift gTalkGifts_19[];
extern TalkGift gTalkGifts_21[];
extern TalkGift gTalkGifts_22[];
extern TalkGift gTalkGifts_23[];
extern TalkGift gTalkGifts_27[];
extern TalkGift gTalkGifts_28[];
extern TalkSelLines gTalkSelects_29_0_0_0;
extern TalkGift gTalkGifts_29[];
extern TalkGift gTalkGifts_30[];
extern TalkGift gTalkGifts_31[];
extern TalkSelLines gTalkSelects_32_2_0_0;
extern TalkGift gTalkGifts_32[];
extern TalkGift gTalkGifts_33[];
extern TalkSelLines gTalkSelects_34_0_0_0;
extern TalkGift gTalkGifts_34[];
extern TalkSelLines gTalkSelects_35_2_0_0;
extern TalkGift gTalkGifts_35[];
extern TalkSelLines gTalkSelects_36_2_0_1;
extern TalkGift gTalkGifts_36[];
extern TalkGift gTalkGifts_37[];
extern TalkGift gTalkGifts_38[];
extern TalkSelLines gTalkSelects_39_2_0_1;
extern TalkGift gTalkGifts_39[];
extern TalkGift gTalkGifts_40[];
extern TalkGift gTalkGifts_41[];
extern TalkGift gTalkGifts_42[];
extern TalkSelLines gTalkSelects_43_2_0_0;
extern TalkGift gTalkGifts_43[];
extern TalkGift gTalkGifts_47[];
extern TalkGift gTalkGifts_48[];
extern TalkGift gTalkGifts_49[];
extern TalkSelLines gTalkSelects_50_2_2_1;
extern TalkGift gTalkGifts_50[];
extern TalkGift gTalkGifts_51[];
extern TalkGift gTalkGifts_52[];
extern TalkGift gTalkGifts_53[];
extern TalkGift gTalkGifts_54[];
extern TalkGift gTalkGifts_56[];
extern TalkGift gTalkGifts_57[];
extern TalkGift gTalkGifts_58[];
extern TalkGift gTalkGifts_59[];
extern TalkGift gTalkGifts_60[];
extern TalkSelLines gTalkSelects_61_2_2_0;
extern TalkGift gTalkGifts_61[];
extern TalkGift gTalkGifts_62[];
extern TalkGift gTalkGifts_63[];
extern TalkGift gTalkGifts_66[];
extern TalkSelLines gTalkSelects_67_0_0_0;
extern TalkGift gTalkGifts_67[];
extern TalkSelLines gTalkSelects_68_0_0_0;
extern TalkGift gTalkGifts_68[];
extern TalkSelLines gTalkSelects_69_0_0_0;
extern TalkGift gTalkGifts_69[];
extern TalkSelLines gTalkSelects_70_0_0_0;
extern TalkGift gTalkGifts_70[];
extern TalkSelLines gTalkSelects_71_0_0_0;
extern TalkGift gTalkGifts_71[];
extern TalkSelLines gTalkSelects_72_0_0_0;
extern TalkGift gTalkGifts_72[];
extern TalkGift gTalkGifts_74[];
extern TalkGift gTalkGifts_75[];
extern TalkGift gTalkGifts_76[];
extern TalkSelLines gTalkSelects_77_1_0_0;
extern TalkGift gTalkGifts_79[];
extern TalkGift gTalkGifts_80[];
extern TalkSelLines gTalkSelects_81_1_0_0;
extern TalkGift gTalkGifts_81[];
extern TalkSelLines gTalkSelects_82_1_0_0;
extern TalkGift gTalkGifts_82[];
extern TalkGift gTalkGifts_83[];
extern TalkGift gTalkGifts_84[];
extern TalkGift gTalkGifts_85[];
extern TalkGift gTalkGifts_86[];
extern TalkSelLines gTalkSelects_87_2_0_1;
extern TalkGift gTalkGifts_87[];
extern TalkSelLines gTalkSelects_88_0_0_0;
extern TalkGift gTalkGifts_88[];
extern TalkSelLines gTalkSelects_89_0_0_0;
extern TalkGift gTalkGifts_89[];
extern TalkSelLines gTalkSelects_90_0_0_0;
extern TalkGift gTalkGifts_90[];
extern TalkSelLines gTalkSelects_91_0_0_0;
extern TalkGift gTalkGifts_91[];
extern TalkSelLines gTalkSelects_92_0_0_0;
extern TalkGift gTalkGifts_92[];
extern TalkGift gTalkGifts_93[];
extern TalkGift gTalkGifts_94[];
extern TalkGift gTalkGifts_95[];
extern TalkGift gTalkGifts_96[];
extern TalkGift gTalkGifts_97[];
extern TalkGift gTalkGifts_98[];
extern TalkGift gTalkGifts_99[];
extern TalkGift gTalkGifts_100[];
extern TalkGift gTalkGifts_101[];
extern TalkGift gTalkGifts_102[];
extern TalkGift gTalkGifts_103[];
extern TalkGift gTalkGifts_104[];
extern TalkGift gTalkGifts_106[];
extern TalkGift gTalkGifts_107[];
extern TalkGift gTalkGifts_108[];
extern TalkSelLines gTalkSelects_109_0_0_0;
extern TalkGift gTalkGifts_109[];
extern TalkGift gTalkGifts_110[];
extern TalkGift gTalkGifts_111[];
extern TalkGift gTalkGifts_112[];
extern TalkSelLines gTalkSelects_114_0_0_0;
extern TalkGift gTalkGifts_114[];
TalkEvent gTalkEvents_1[22] = {
#include "lobby/0080DD50.inc"
};
TalkSelLines gTalkSelects_1_0_0_0 = {
#include "lobby/0080DDA8.inc"
};
TalkSelLines gTalkSelects_1_0_0_1 = {
#include "lobby/0080DDB0.inc"
};
TalkSelLines gTalkSelects_1_1_0_0 = {
#include "lobby/0080DDB8.inc"
};
TalkSelLines gTalkSelects_1_2_0_0 = {
#include "lobby/0080DDC0.inc"
};
TalkSelLines gTalkSelects_1_2_1_0 = {
#include "lobby/0080DDC8.inc"
};
TalkSelLines gTalkSelects_1_2_2_0 = {
#include "lobby/0080DDD0.inc"
};
TalkSelLines gTalkSelects_1_3_0_0 = {
#include "lobby/0080DDD8.inc"
};
TalkSelLines gTalkSelects_1_3_1_0 = {
#include "lobby/0080DDE0.inc"
};
TalkSelLines gTalkSelects_1_3_2_0 = {
#include "lobby/0080DDE8.inc"
};
TalkSelLines gTalkSelects_1_4_0_0 = {
#include "lobby/0080DDF0.inc"
};
TalkSelLines gTalkSelects_1_4_0_1 = {
#include "lobby/0080DDF8.inc"
};
TalkSelLines gTalkSelects_1_4_1_0 = {
#include "lobby/0080DE00.inc"
};
TalkSelLines gTalkSelects_1_4_1_1 = {
#include "lobby/0080DE08.inc"
};
TalkSelLines gTalkSelects_1_4_2_0 = {
#include "lobby/0080DE10.inc"
};
TalkSelLines gTalkSelects_1_4_2_1 = {
#include "lobby/0080DE18.inc"
};
TalkSelLines gTalkSelects_1_5_0_0 = {
#include "lobby/0080DE20.inc"
};
TalkSelLines gTalkSelects_1_6_0_0 = {
#include "lobby/0080DE28.inc"
};
TalkSelLines gTalkSelects_1_7_0_0 = {
#include "lobby/0080DE30.inc"
};
TalkSelLines gTalkSelects_1_7_0_1 = {
#include "lobby/0080DE38.inc"
};
TalkSelLines gTalkSelects_1_8_0_0 = {
#include "lobby/0080DE40.inc"
};
TalkSelLines gTalkSelects_1_8_0_1 = {
#include "lobby/0080DE48.inc"
};
TalkSelLines gTalkSelects_1_8_1_0 = {
#include "lobby/0080DE50.inc"
};
TalkSelLines gTalkSelects_1_8_1_1 = {
#include "lobby/0080DE58.inc"
};
TalkSelLines gTalkSelects_1_8_2_0 = {
#include "lobby/0080DE60.inc"
};
TalkSelLines gTalkSelects_1_8_2_1 = {
#include "lobby/0080DE68.inc"
};
TalkSelLines gTalkSelects_1_9_0_0 = {
#include "lobby/0080DE70.inc"
};
TalkSelLines gTalkSelects_1_10_0_0 = {
#include "lobby/0080DE78.inc"
};
TalkSelLines gTalkSelects_1_10_0_1 = {
#include "lobby/0080DE80.inc"
};
TalkSelLines gTalkSelects_1_11_0_0 = {
#include "lobby/0080DE88.inc"
};
TalkSelLines gTalkSelects_1_11_0_1 = {
#include "lobby/0080DE90.inc"
};
TalkSelLines gTalkSelects_1_12_0_0 = {
#include "lobby/0080DE98.inc"
};
TalkSelLines gTalkSelects_1_12_0_1 = {
#include "lobby/0080DEA0.inc"
};
TalkSelLines gTalkSelects_1_13_0_0 = {
#include "lobby/0080DEA8.inc"
};
TalkSelLines gTalkSelects_1_13_0_1 = {
#include "lobby/0080DEB0.inc"
};
TalkSelLines gTalkSelects_1_13_1_0 = {
#include "lobby/0080DEB8.inc"
};
TalkSelLines gTalkSelects_1_13_1_1 = {
#include "lobby/0080DEC0.inc"
};
TalkSelLines gTalkSelects_1_13_2_0 = {
#include "lobby/0080DEC8.inc"
};
TalkSelLines gTalkSelects_1_13_2_1 = {
#include "lobby/0080DED0.inc"
};
TalkSelLines gTalkSelects_1_14_0_0 = {
#include "lobby/0080DED8.inc"
};
TalkSelLines gTalkSelects_1_14_1_0 = {
#include "lobby/0080DEE0.inc"
};
TalkSelLines gTalkSelects_1_14_2_0 = {
#include "lobby/0080DEE8.inc"
};
TalkSelLines gTalkSelects_1_15_0_0 = {
#include "lobby/0080DEF0.inc"
};
TalkSelLines gTalkSelects_1_15_0_1 = {
#include "lobby/0080DEF8.inc"
};
TalkSelLines gTalkSelects_1_15_1_0 = {
#include "lobby/0080DF00.inc"
};
TalkSelLines gTalkSelects_1_15_1_1 = {
#include "lobby/0080DF08.inc"
};
TalkSelLines gTalkSelects_1_15_2_0 = {
#include "lobby/0080DF10.inc"
};
TalkSelLines gTalkSelects_1_15_2_1 = {
#include "lobby/0080DF18.inc"
};
TalkSelLines gTalkSelects_1_16_0_0 = {
#include "lobby/0080DF20.inc"
};
TalkSelLines gTalkSelects_1_16_1_0 = {
#include "lobby/0080DF28.inc"
};
TalkSelLines gTalkSelects_1_16_2_0 = {
#include "lobby/0080DF30.inc"
};
TalkSelLines gTalkSelects_1_17_0_0 = {
#include "lobby/0080DF38.inc"
};
TalkSelLines gTalkSelects_1_17_1_0 = {
#include "lobby/0080DF40.inc"
};
TalkSelLines gTalkSelects_1_17_2_0 = {
#include "lobby/0080DF48.inc"
};
TalkSelLines gTalkSelects_1_18_0_0 = {
#include "lobby/0080DF50.inc"
};
TalkSelLines gTalkSelects_1_18_1_0 = {
#include "lobby/0080DF58.inc"
};
TalkSelLines gTalkSelects_1_18_2_0 = {
#include "lobby/0080DF60.inc"
};
TalkSelLines gTalkSelects_1_19_0_0 = {
#include "lobby/0080DF68.inc"
};
TalkSelLines gTalkSelects_1_19_0_1 = {
#include "lobby/0080DF70.inc"
};
TalkSelLines gTalkSelects_1_19_1_0 = {
#include "lobby/0080DF78.inc"
};
TalkSelLines gTalkSelects_1_19_1_1 = {
#include "lobby/0080DF80.inc"
};
TalkSelLines gTalkSelects_1_19_2_0 = {
#include "lobby/0080DF88.inc"
};
TalkSelLines gTalkSelects_1_19_2_1 = {
#include "lobby/0080DF90.inc"
};
TalkSelLines gTalkSelects_1_20_0_0 = {
#include "lobby/0080DF98.inc"
};
TalkSelLines gTalkSelects_1_20_0_1 = {
#include "lobby/0080DFA0.inc"
};
TalkSelLines gTalkSelects_1_21_0_0 = {
#include "lobby/0080DFA8.inc"
};
TalkSel gTalkSelects_1_21_0[1] = {
    {0, &gTalkSelects_1_21_0_0},
};
TalkSel gTalkSelects_1_21[1] = {
    {0, gTalkSelects_1_21_0},
};
TalkSel gTalkSelects_1_20_0[2] = {
#include "lobby/0080DFC0.inc"
};
TalkSel gTalkSelects_1_20[2] = {
    {0, gTalkSelects_1_20_0},
    {0, NULL},
};
TalkSel gTalkSelects_1_19_2[2] = {
#include "lobby/0080DFE0.inc"
};
TalkSel gTalkSelects_1_19_1[2] = {
#include "lobby/0080DFF0.inc"
};
TalkSel gTalkSelects_1_19_0[2] = {
#include "lobby/0080E000.inc"
};
TalkSel gTalkSelects_1_19[3] = {
#include "lobby/0080E010.inc"
};
TalkSel gTalkSelects_1_18_2[1] = {
    {0, &gTalkSelects_1_18_2_0},
};
TalkSel gTalkSelects_1_18_1[1] = {
    {0, &gTalkSelects_1_18_1_0},
};
TalkSel gTalkSelects_1_18_0[1] = {
    {0, &gTalkSelects_1_18_0_0},
};
TalkSel gTalkSelects_1_18[3] = {
#include "lobby/0080E040.inc"
};
TalkSel gTalkSelects_1_17_2[1] = {
    {0, &gTalkSelects_1_17_2_0},
};
TalkSel gTalkSelects_1_17_1[1] = {
    {0, &gTalkSelects_1_17_1_0},
};
TalkSel gTalkSelects_1_17_0[1] = {
    {0, &gTalkSelects_1_17_0_0},
};
TalkSel gTalkSelects_1_17[3] = {
#include "lobby/0080E070.inc"
};
TalkSel gTalkSelects_1_16_2[1] = {
    {0, &gTalkSelects_1_16_2_0},
};
TalkSel gTalkSelects_1_16_1[1] = {
    {0, &gTalkSelects_1_16_1_0},
};
TalkSel gTalkSelects_1_16_0[1] = {
    {0, &gTalkSelects_1_16_0_0},
};
TalkSel gTalkSelects_1_16[4] = {
#include "lobby/0080E0A0.inc"
};
TalkSel gTalkSelects_1_15_2[2] = {
#include "lobby/0080E0C0.inc"
};
TalkSel gTalkSelects_1_15_1[2] = {
#include "lobby/0080E0D0.inc"
};
TalkSel gTalkSelects_1_15_0[2] = {
#include "lobby/0080E0E0.inc"
};
TalkSel gTalkSelects_1_15[3] = {
#include "lobby/0080E0F0.inc"
};
TalkSel gTalkSelects_1_14_2[1] = {
    {0, &gTalkSelects_1_14_2_0},
};
TalkSel gTalkSelects_1_14_1[1] = {
    {0, &gTalkSelects_1_14_1_0},
};
TalkSel gTalkSelects_1_14_0[1] = {
    {0, &gTalkSelects_1_14_0_0},
};
TalkSel gTalkSelects_1_14[4] = {
#include "lobby/0080E120.inc"
};
TalkSel gTalkSelects_1_13_2[2] = {
#include "lobby/0080E140.inc"
};
TalkSel gTalkSelects_1_13_1[2] = {
#include "lobby/0080E150.inc"
};
TalkSel gTalkSelects_1_13_0[2] = {
#include "lobby/0080E160.inc"
};
TalkSel gTalkSelects_1_13[4] = {
#include "lobby/0080E170.inc"
};
TalkSel gTalkSelects_1_12_0[2] = {
#include "lobby/0080E190.inc"
};
TalkSel gTalkSelects_1_12[2] = {
    {0, gTalkSelects_1_12_0},
    {0, NULL},
};
TalkSel gTalkSelects_1_11_0[2] = {
#include "lobby/0080E1B0.inc"
};
TalkSel gTalkSelects_1_11[2] = {
    {0, gTalkSelects_1_11_0},
    {0, NULL},
};
TalkSel gTalkSelects_1_10_0[2] = {
#include "lobby/0080E1D0.inc"
};
TalkSel gTalkSelects_1_10[1] = {
    {0, gTalkSelects_1_10_0},
};
TalkSel gTalkSelects_1_9_0[1] = {
    {0, &gTalkSelects_1_9_0_0},
};
TalkSel gTalkSelects_1_9[2] = {
    {0, gTalkSelects_1_9_0},
    {0, NULL},
};
TalkSel gTalkSelects_1_8_2[2] = {
#include "lobby/0080E200.inc"
};
TalkSel gTalkSelects_1_8_1[2] = {
#include "lobby/0080E210.inc"
};
TalkSel gTalkSelects_1_8_0[2] = {
#include "lobby/0080E220.inc"
};
TalkSel gTalkSelects_1_8[4] = {
#include "lobby/0080E230.inc"
};
TalkSel gTalkSelects_1_7_0[2] = {
#include "lobby/0080E250.inc"
};
TalkSel gTalkSelects_1_7[1] = {
    {0, gTalkSelects_1_7_0},
};
TalkSel gTalkSelects_1_6_0[1] = {
    {0, &gTalkSelects_1_6_0_0},
};
TalkSel gTalkSelects_1_6[1] = {
    {0, gTalkSelects_1_6_0},
};
TalkSel gTalkSelects_1_5_0[1] = {
    {0, &gTalkSelects_1_5_0_0},
};
TalkSel gTalkSelects_1_5[2] = {
    {0, gTalkSelects_1_5_0},
    {0, NULL},
};
TalkSel gTalkSelects_1_4_2[2] = {
#include "lobby/0080E290.inc"
};
TalkSel gTalkSelects_1_4_1[2] = {
#include "lobby/0080E2A0.inc"
};
TalkSel gTalkSelects_1_4_0[2] = {
#include "lobby/0080E2B0.inc"
};
TalkSel gTalkSelects_1_4[3] = {
#include "lobby/0080E2C0.inc"
};
TalkSel gTalkSelects_1_3_2[1] = {
    {0, &gTalkSelects_1_3_2_0},
};
TalkSel gTalkSelects_1_3_1[1] = {
    {0, &gTalkSelects_1_3_1_0},
};
TalkSel gTalkSelects_1_3_0[1] = {
    {0, &gTalkSelects_1_3_0_0},
};
TalkSel gTalkSelects_1_3[3] = {
#include "lobby/0080E2F0.inc"
};
TalkSel gTalkSelects_1_2_2[1] = {
    {0, &gTalkSelects_1_2_2_0},
};
TalkSel gTalkSelects_1_2_1[1] = {
    {0, &gTalkSelects_1_2_1_0},
};
TalkSel gTalkSelects_1_2_0[1] = {
    {0, &gTalkSelects_1_2_0_0},
};
TalkSel gTalkSelects_1_2[3] = {
#include "lobby/0080E320.inc"
};
TalkSel gTalkSelects_1_1_0[1] = {
    {0, &gTalkSelects_1_1_0_0},
};
TalkSel gTalkSelects_1_1[2] = {
    {0, gTalkSelects_1_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_1_0_0[2] = {
#include "lobby/0080E350.inc"
};
TalkSel gTalkSelects_1_0[2] = {
    {0, gTalkSelects_1_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_1[22] = {
#include "lobby/0080E370.inc"
};
TalkLine gTalkLines_1[620] = {
#include "lobby/0080E420.inc"
};
TalkGift gTalkGifts_1[12] = {
#include "lobby/0080EDD0.inc"
};
TalkEvent gTalkEvents_2[10] = {
#include "lobby/0080EE30.inc"
};
TalkSelLines gTalkSelects_2_0_0_0 = {
#include "lobby/0080EE58.inc"
};
TalkSelLines gTalkSelects_2_0_0_1 = {
#include "lobby/0080EE60.inc"
};
TalkSelLines gTalkSelects_2_0_1_0 = {
#include "lobby/0080EE68.inc"
};
TalkSelLines gTalkSelects_2_0_1_1 = {
#include "lobby/0080EE70.inc"
};
TalkSelLines gTalkSelects_2_0_2_0 = {
#include "lobby/0080EE78.inc"
};
TalkSelLines gTalkSelects_2_0_2_1 = {
#include "lobby/0080EE80.inc"
};
TalkSelLines gTalkSelects_2_1_0_0 = {
#include "lobby/0080EE88.inc"
};
TalkSelLines gTalkSelects_2_1_0_1 = {
#include "lobby/0080EE90.inc"
};
TalkSelLines gTalkSelects_2_1_1_0 = {
#include "lobby/0080EE98.inc"
};
TalkSelLines gTalkSelects_2_1_1_1 = {
#include "lobby/0080EEA0.inc"
};
TalkSelLines gTalkSelects_2_1_2_0 = {
#include "lobby/0080EEA8.inc"
};
TalkSelLines gTalkSelects_2_1_2_1 = {
#include "lobby/0080EEB0.inc"
};
TalkSelLines gTalkSelects_2_2_0_0 = {
#include "lobby/0080EEB8.inc"
};
TalkSelLines gTalkSelects_2_2_0_1 = {
#include "lobby/0080EEC0.inc"
};
TalkSelLines gTalkSelects_2_2_1_0 = {
#include "lobby/0080EEC8.inc"
};
TalkSelLines gTalkSelects_2_2_1_1 = {
#include "lobby/0080EED0.inc"
};
TalkSelLines gTalkSelects_2_2_2_0 = {
#include "lobby/0080EED8.inc"
};
TalkSelLines gTalkSelects_2_2_2_1 = {
#include "lobby/0080EEE0.inc"
};
TalkSelLines gTalkSelects_2_3_0_0 = {
#include "lobby/0080EEE8.inc"
};
TalkSelLines gTalkSelects_2_3_0_1 = {
#include "lobby/0080EEF0.inc"
};
TalkSelLines gTalkSelects_2_3_1_0 = {
#include "lobby/0080EEF8.inc"
};
TalkSelLines gTalkSelects_2_3_1_1 = {
#include "lobby/0080EF00.inc"
};
TalkSelLines gTalkSelects_2_3_2_0 = {
#include "lobby/0080EF08.inc"
};
TalkSelLines gTalkSelects_2_3_2_1 = {
#include "lobby/0080EF10.inc"
};
TalkSelLines gTalkSelects_2_4_0_0 = {
#include "lobby/0080EF18.inc"
};
TalkSelLines gTalkSelects_2_4_0_1 = {
#include "lobby/0080EF20.inc"
};
TalkSelLines gTalkSelects_2_4_1_0 = {
#include "lobby/0080EF28.inc"
};
TalkSelLines gTalkSelects_2_4_1_1 = {
#include "lobby/0080EF30.inc"
};
TalkSelLines gTalkSelects_2_4_2_0 = {
#include "lobby/0080EF38.inc"
};
TalkSelLines gTalkSelects_2_4_2_1 = {
#include "lobby/0080EF40.inc"
};
TalkSelLines gTalkSelects_2_5_0_0 = {
#include "lobby/0080EF48.inc"
};
TalkSelLines gTalkSelects_2_5_0_1 = {
#include "lobby/0080EF50.inc"
};
TalkSelLines gTalkSelects_2_5_1_0 = {
#include "lobby/0080EF58.inc"
};
TalkSelLines gTalkSelects_2_5_1_1 = {
#include "lobby/0080EF60.inc"
};
TalkSelLines gTalkSelects_2_5_2_0 = {
#include "lobby/0080EF68.inc"
};
TalkSelLines gTalkSelects_2_5_2_1 = {
#include "lobby/0080EF70.inc"
};
TalkSelLines gTalkSelects_2_6_0_0 = {
#include "lobby/0080EF78.inc"
};
TalkSelLines gTalkSelects_2_6_0_1 = {
#include "lobby/0080EF80.inc"
};
TalkSelLines gTalkSelects_2_7_0_0 = {
#include "lobby/0080EF88.inc"
};
TalkSelLines gTalkSelects_2_7_0_1 = {
#include "lobby/0080EF90.inc"
};
TalkSelLines gTalkSelects_2_8_0_0 = {
#include "lobby/0080EF98.inc"
};
TalkSel gTalkSelects_2_8_0[1] = {
    {0, &gTalkSelects_2_8_0_0},
};
TalkSel gTalkSelects_2_8[1] = {
    {0, gTalkSelects_2_8_0},
};
TalkSel gTalkSelects_2_7_0[2] = {
#include "lobby/0080EFB0.inc"
};
TalkSel gTalkSelects_2_7[2] = {
    {0, gTalkSelects_2_7_0},
    {0, NULL},
};
TalkSel gTalkSelects_2_6_0[2] = {
#include "lobby/0080EFD0.inc"
};
TalkSel gTalkSelects_2_6[2] = {
    {0, gTalkSelects_2_6_0},
    {0, NULL},
};
TalkSel gTalkSelects_2_5_2[2] = {
#include "lobby/0080EFF0.inc"
};
TalkSel gTalkSelects_2_5_1[2] = {
#include "lobby/0080F000.inc"
};
TalkSel gTalkSelects_2_5_0[2] = {
#include "lobby/0080F010.inc"
};
TalkSel gTalkSelects_2_5[4] = {
#include "lobby/0080F020.inc"
};
TalkSel gTalkSelects_2_4_2[2] = {
#include "lobby/0080F040.inc"
};
TalkSel gTalkSelects_2_4_1[2] = {
#include "lobby/0080F050.inc"
};
TalkSel gTalkSelects_2_4_0[2] = {
#include "lobby/0080F060.inc"
};
TalkSel gTalkSelects_2_4[4] = {
#include "lobby/0080F070.inc"
};
TalkSel gTalkSelects_2_3_2[2] = {
#include "lobby/0080F090.inc"
};
TalkSel gTalkSelects_2_3_1[2] = {
#include "lobby/0080F0A0.inc"
};
TalkSel gTalkSelects_2_3_0[2] = {
#include "lobby/0080F0B0.inc"
};
TalkSel gTalkSelects_2_3[4] = {
#include "lobby/0080F0C0.inc"
};
TalkSel gTalkSelects_2_2_2[2] = {
#include "lobby/0080F0E0.inc"
};
TalkSel gTalkSelects_2_2_1[2] = {
#include "lobby/0080F0F0.inc"
};
TalkSel gTalkSelects_2_2_0[2] = {
#include "lobby/0080F100.inc"
};
TalkSel gTalkSelects_2_2[4] = {
#include "lobby/0080F110.inc"
};
TalkSel gTalkSelects_2_1_2[2] = {
#include "lobby/0080F130.inc"
};
TalkSel gTalkSelects_2_1_1[2] = {
#include "lobby/0080F140.inc"
};
TalkSel gTalkSelects_2_1_0[2] = {
#include "lobby/0080F150.inc"
};
TalkSel gTalkSelects_2_1[4] = {
#include "lobby/0080F160.inc"
};
TalkSel gTalkSelects_2_0_2[2] = {
#include "lobby/0080F180.inc"
};
TalkSel gTalkSelects_2_0_1[2] = {
#include "lobby/0080F190.inc"
};
TalkSel gTalkSelects_2_0_0[2] = {
#include "lobby/0080F1A0.inc"
};
TalkSel gTalkSelects_2_0[4] = {
#include "lobby/0080F1B0.inc"
};
TalkSel gTalkSelects_2[10] = {
#include "lobby/0080F1D0.inc"
};
TalkLine gTalkLines_2[132] = {
#include "lobby/0080F220.inc"
};
TalkEvent gTalkEvents_3[10] = {
#include "lobby/0080F430.inc"
};
TalkSelLines gTalkSelects_3_0_0_0 = {
#include "lobby/0080F458.inc"
};
TalkSelLines gTalkSelects_3_0_0_1 = {
#include "lobby/0080F460.inc"
};
TalkSelLines gTalkSelects_3_0_1_0 = {
#include "lobby/0080F468.inc"
};
TalkSelLines gTalkSelects_3_0_1_1 = {
#include "lobby/0080F470.inc"
};
TalkSelLines gTalkSelects_3_0_2_0 = {
#include "lobby/0080F478.inc"
};
TalkSelLines gTalkSelects_3_0_2_1 = {
#include "lobby/0080F480.inc"
};
TalkSelLines gTalkSelects_3_1_0_0 = {
#include "lobby/0080F488.inc"
};
TalkSelLines gTalkSelects_3_1_0_1 = {
#include "lobby/0080F490.inc"
};
TalkSelLines gTalkSelects_3_1_1_0 = {
#include "lobby/0080F498.inc"
};
TalkSelLines gTalkSelects_3_1_1_1 = {
#include "lobby/0080F4A0.inc"
};
TalkSelLines gTalkSelects_3_1_2_0 = {
#include "lobby/0080F4A8.inc"
};
TalkSelLines gTalkSelects_3_1_2_1 = {
#include "lobby/0080F4B0.inc"
};
TalkSelLines gTalkSelects_3_2_0_0 = {
#include "lobby/0080F4B8.inc"
};
TalkSelLines gTalkSelects_3_2_0_1 = {
#include "lobby/0080F4C0.inc"
};
TalkSelLines gTalkSelects_3_3_0_0 = {
#include "lobby/0080F4C8.inc"
};
TalkSelLines gTalkSelects_3_3_0_1 = {
#include "lobby/0080F4D0.inc"
};
TalkSelLines gTalkSelects_3_3_1_0 = {
#include "lobby/0080F4D8.inc"
};
TalkSelLines gTalkSelects_3_3_1_1 = {
#include "lobby/0080F4E0.inc"
};
TalkSelLines gTalkSelects_3_3_2_0 = {
#include "lobby/0080F4E8.inc"
};
TalkSelLines gTalkSelects_3_3_2_1 = {
#include "lobby/0080F4F0.inc"
};
TalkSelLines gTalkSelects_3_4_0_0 = {
#include "lobby/0080F4F8.inc"
};
TalkSelLines gTalkSelects_3_4_0_1 = {
#include "lobby/0080F500.inc"
};
TalkSelLines gTalkSelects_3_4_1_0 = {
#include "lobby/0080F508.inc"
};
TalkSelLines gTalkSelects_3_4_1_1 = {
#include "lobby/0080F510.inc"
};
TalkSelLines gTalkSelects_3_4_2_0 = {
#include "lobby/0080F518.inc"
};
TalkSelLines gTalkSelects_3_4_2_1 = {
#include "lobby/0080F520.inc"
};
TalkSelLines gTalkSelects_3_5_0_0 = {
#include "lobby/0080F528.inc"
};
TalkSelLines gTalkSelects_3_5_0_1 = {
#include "lobby/0080F530.inc"
};
TalkSelLines gTalkSelects_3_6_0_0 = {
#include "lobby/0080F538.inc"
};
TalkSelLines gTalkSelects_3_6_0_1 = {
#include "lobby/0080F540.inc"
};
TalkSelLines gTalkSelects_3_7_0_0 = {
#include "lobby/0080F548.inc"
};
TalkSel gTalkSelects_3_7_0[1] = {
    {0, &gTalkSelects_3_7_0_0},
};
TalkSel gTalkSelects_3_7[1] = {
    {0, gTalkSelects_3_7_0},
};
TalkSel gTalkSelects_3_6_0[2] = {
#include "lobby/0080F560.inc"
};
TalkSel gTalkSelects_3_6[2] = {
    {0, gTalkSelects_3_6_0},
    {0, NULL},
};
TalkSel gTalkSelects_3_5_0[2] = {
#include "lobby/0080F580.inc"
};
TalkSel gTalkSelects_3_5[2] = {
    {0, gTalkSelects_3_5_0},
    {0, NULL},
};
TalkSel gTalkSelects_3_4_2[2] = {
#include "lobby/0080F5A0.inc"
};
TalkSel gTalkSelects_3_4_1[2] = {
#include "lobby/0080F5B0.inc"
};
TalkSel gTalkSelects_3_4_0[2] = {
#include "lobby/0080F5C0.inc"
};
TalkSel gTalkSelects_3_4[4] = {
#include "lobby/0080F5D0.inc"
};
TalkSel gTalkSelects_3_3_2[2] = {
#include "lobby/0080F5F0.inc"
};
TalkSel gTalkSelects_3_3_1[2] = {
#include "lobby/0080F600.inc"
};
TalkSel gTalkSelects_3_3_0[2] = {
#include "lobby/0080F610.inc"
};
TalkSel gTalkSelects_3_3[4] = {
#include "lobby/0080F620.inc"
};
TalkSel gTalkSelects_3_2_0[2] = {
#include "lobby/0080F640.inc"
};
TalkSel gTalkSelects_3_2[2] = {
    {0, gTalkSelects_3_2_0},
    {0, NULL},
};
TalkSel gTalkSelects_3_1_2[2] = {
#include "lobby/0080F660.inc"
};
TalkSel gTalkSelects_3_1_1[2] = {
#include "lobby/0080F670.inc"
};
TalkSel gTalkSelects_3_1_0[2] = {
#include "lobby/0080F680.inc"
};
TalkSel gTalkSelects_3_1[4] = {
#include "lobby/0080F690.inc"
};
TalkSel gTalkSelects_3_0_2[2] = {
#include "lobby/0080F6B0.inc"
};
TalkSel gTalkSelects_3_0_1[2] = {
#include "lobby/0080F6C0.inc"
};
TalkSel gTalkSelects_3_0_0[2] = {
#include "lobby/0080F6D0.inc"
};
TalkSel gTalkSelects_3_0[4] = {
#include "lobby/0080F6E0.inc"
};
TalkSel gTalkSelects_3[8] = {
#include "lobby/0080F700.inc"
};
TalkLine gTalkLines_3[144] = {
#include "lobby/0080F740.inc"
};
TalkGift gTalkGifts_3[4] = {
#include "lobby/0080F980.inc"
};
TalkEvent gTalkEvents_4[6] = {
#include "lobby/0080F9A0.inc"
};
TalkSelLines gTalkSelects_4_0_0_0 = {
#include "lobby/0080F9B8.inc"
};
TalkSelLines gTalkSelects_4_0_0_1 = {
#include "lobby/0080F9C0.inc"
};
TalkSelLines gTalkSelects_4_0_1_0 = {
#include "lobby/0080F9C8.inc"
};
TalkSelLines gTalkSelects_4_0_1_1 = {
#include "lobby/0080F9D0.inc"
};
TalkSelLines gTalkSelects_4_0_2_0 = {
#include "lobby/0080F9D8.inc"
};
TalkSelLines gTalkSelects_4_0_2_1 = {
#include "lobby/0080F9E0.inc"
};
TalkSelLines gTalkSelects_4_1_0_0 = {
#include "lobby/0080F9E8.inc"
};
TalkSelLines gTalkSelects_4_1_0_1 = {
#include "lobby/0080F9F0.inc"
};
TalkSelLines gTalkSelects_4_2_0_0 = {
#include "lobby/0080F9F8.inc"
};
TalkSelLines gTalkSelects_4_2_0_1 = {
#include "lobby/0080FA00.inc"
};
TalkSelLines gTalkSelects_4_3_0_0 = {
#include "lobby/0080FA08.inc"
};
TalkSelLines gTalkSelects_4_3_1_0 = {
#include "lobby/0080FA10.inc"
};
TalkSelLines gTalkSelects_4_3_2_0 = {
#include "lobby/0080FA18.inc"
};
TalkSelLines gTalkSelects_4_4_0_0 = {
#include "lobby/0080FA20.inc"
};
TalkSelLines gTalkSelects_4_4_0_1 = {
#include "lobby/0080FA28.inc"
};
TalkSelLines gTalkSelects_4_5_0_0 = {
#include "lobby/0080FA30.inc"
};
TalkSelLines gTalkSelects_4_5_1_0 = {
#include "lobby/0080FA38.inc"
};
TalkSelLines gTalkSelects_4_5_2_0 = {
#include "lobby/0080FA40.inc"
};
TalkSelLines gTalkSelects_4_6_0_0 = {
#include "lobby/0080FA48.inc"
};
TalkSelLines gTalkSelects_4_7_0_0 = {
#include "lobby/0080FA50.inc"
};
TalkSelLines gTalkSelects_4_7_0_1 = {
#include "lobby/0080FA58.inc"
};
TalkSelLines gTalkSelects_4_8_0_0 = {
#include "lobby/0080FA60.inc"
};
TalkSelLines gTalkSelects_4_8_0_1 = {
#include "lobby/0080FA68.inc"
};
TalkSelLines gTalkSelects_4_9_0_0 = {
#include "lobby/0080FA70.inc"
};
TalkSelLines gTalkSelects_4_9_0_1 = {
#include "lobby/0080FA78.inc"
};
TalkSelLines gTalkSelects_4_10_0_0 = {
#include "lobby/0080FA80.inc"
};
TalkSelLines gTalkSelects_4_10_0_1 = {
#include "lobby/0080FA88.inc"
};
TalkSelLines gTalkSelects_4_11_0_0 = {
#include "lobby/0080FA90.inc"
};
TalkSelLines gTalkSelects_4_11_0_1 = {
#include "lobby/0080FA98.inc"
};
TalkSelLines gTalkSelects_4_11_1_0 = {
#include "lobby/0080FAA0.inc"
};
TalkSelLines gTalkSelects_4_11_1_1 = {
#include "lobby/0080FAA8.inc"
};
TalkSelLines gTalkSelects_4_11_2_0 = {
#include "lobby/0080FAB0.inc"
};
TalkSelLines gTalkSelects_4_11_2_1 = {
#include "lobby/0080FAB8.inc"
};
TalkSelLines gTalkSelects_4_12_0_0 = {
#include "lobby/0080FAC0.inc"
};
TalkSelLines gTalkSelects_4_12_0_1 = {
#include "lobby/0080FAC8.inc"
};
TalkSel gTalkSelects_4_12_0[2] = {
#include "lobby/0080FAD0.inc"
};
TalkSel gTalkSelects_4_12[2] = {
    {0, gTalkSelects_4_12_0},
    {0, NULL},
};
TalkSel gTalkSelects_4_11_2[2] = {
#include "lobby/0080FAF0.inc"
};
TalkSel gTalkSelects_4_11_1[2] = {
#include "lobby/0080FB00.inc"
};
TalkSel gTalkSelects_4_11_0[2] = {
#include "lobby/0080FB10.inc"
};
TalkSel gTalkSelects_4_11[4] = {
#include "lobby/0080FB20.inc"
};
TalkSel gTalkSelects_4_10_0[2] = {
#include "lobby/0080FB40.inc"
};
TalkSel gTalkSelects_4_10[2] = {
    {0, gTalkSelects_4_10_0},
    {0, NULL},
};
TalkSel gTalkSelects_4_9_0[2] = {
#include "lobby/0080FB60.inc"
};
TalkSel gTalkSelects_4_9[2] = {
    {0, gTalkSelects_4_9_0},
    {0, NULL},
};
TalkSel gTalkSelects_4_8_0[2] = {
#include "lobby/0080FB80.inc"
};
TalkSel gTalkSelects_4_8[2] = {
    {0, gTalkSelects_4_8_0},
    {0, NULL},
};
TalkSel gTalkSelects_4_7_0[2] = {
#include "lobby/0080FBA0.inc"
};
TalkSel gTalkSelects_4_7[1] = {
    {0, gTalkSelects_4_7_0},
};
TalkSel gTalkSelects_4_6_0[1] = {
    {0, &gTalkSelects_4_6_0_0},
};
TalkSel gTalkSelects_4_6[1] = {
    {0, gTalkSelects_4_6_0},
};
TalkSel gTalkSelects_4_5_2[1] = {
    {0, &gTalkSelects_4_5_2_0},
};
TalkSel gTalkSelects_4_5_1[1] = {
    {0, &gTalkSelects_4_5_1_0},
};
TalkSel gTalkSelects_4_5_0[1] = {
    {0, &gTalkSelects_4_5_0_0},
};
TalkSel gTalkSelects_4_5[4] = {
#include "lobby/0080FBE0.inc"
};
TalkSel gTalkSelects_4_4_0[2] = {
#include "lobby/0080FC00.inc"
};
TalkSel gTalkSelects_4_4[1] = {
    {0, gTalkSelects_4_4_0},
};
TalkSel gTalkSelects_4_3_2[1] = {
    {0, &gTalkSelects_4_3_2_0},
};
TalkSel gTalkSelects_4_3_1[1] = {
    {0, &gTalkSelects_4_3_1_0},
};
TalkSel gTalkSelects_4_3_0[1] = {
    {0, &gTalkSelects_4_3_0_0},
};
TalkSel gTalkSelects_4_3[4] = {
#include "lobby/0080FC30.inc"
};
TalkSel gTalkSelects_4_2_0[2] = {
#include "lobby/0080FC50.inc"
};
TalkSel gTalkSelects_4_2[2] = {
    {0, gTalkSelects_4_2_0},
    {0, NULL},
};
TalkSel gTalkSelects_4_1_0[2] = {
#include "lobby/0080FC70.inc"
};
TalkSel gTalkSelects_4_1[2] = {
    {0, gTalkSelects_4_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_4_0_2[2] = {
#include "lobby/0080FC90.inc"
};
TalkSel gTalkSelects_4_0_1[2] = {
#include "lobby/0080FCA0.inc"
};
TalkSel gTalkSelects_4_0_0[2] = {
#include "lobby/0080FCB0.inc"
};
TalkSel gTalkSelects_4_0[4] = {
#include "lobby/0080FCC0.inc"
};
TalkSel gTalkSelects_4[14] = {
#include "lobby/0080FCE0.inc"
};
TalkLine gTalkLines_4[252] = {
#include "lobby/0080FD50.inc"
};
TalkGift gTalkGifts_4[2] = {
#include "lobby/00810140.inc"
};
TalkEvent gTalkEvents_5[16] = {
#include "lobby/00810150.inc"
};
TalkSelLines gTalkSelects_5_0_0_0 = {
#include "lobby/00810190.inc"
};
TalkSelLines gTalkSelects_5_0_0_1 = {
#include "lobby/00810198.inc"
};
TalkSelLines gTalkSelects_5_0_1_0 = {
#include "lobby/008101A0.inc"
};
TalkSelLines gTalkSelects_5_0_1_1 = {
#include "lobby/008101A8.inc"
};
TalkSelLines gTalkSelects_5_0_2_0 = {
#include "lobby/008101B0.inc"
};
TalkSelLines gTalkSelects_5_0_2_1 = {
#include "lobby/008101B8.inc"
};
TalkSelLines gTalkSelects_5_1_0_0 = {
#include "lobby/008101C0.inc"
};
TalkSelLines gTalkSelects_5_1_0_1 = {
#include "lobby/008101C8.inc"
};
TalkSelLines gTalkSelects_5_2_0_0 = {
#include "lobby/008101D0.inc"
};
TalkSelLines gTalkSelects_5_2_0_1 = {
#include "lobby/008101D8.inc"
};
TalkSelLines gTalkSelects_5_3_0_0 = {
#include "lobby/008101E0.inc"
};
TalkSelLines gTalkSelects_5_3_0_1 = {
#include "lobby/008101E8.inc"
};
TalkSelLines gTalkSelects_5_4_0_0 = {
#include "lobby/008101F0.inc"
};
TalkSelLines gTalkSelects_5_4_0_1 = {
#include "lobby/008101F8.inc"
};
TalkSelLines gTalkSelects_5_5_0_0 = {
#include "lobby/00810200.inc"
};
TalkSelLines gTalkSelects_5_5_0_1 = {
#include "lobby/00810208.inc"
};
TalkSelLines gTalkSelects_5_6_0_0 = {
#include "lobby/00810210.inc"
};
TalkSelLines gTalkSelects_5_6_0_1 = {
#include "lobby/00810218.inc"
};
TalkSelLines gTalkSelects_5_7_0_0 = {
#include "lobby/00810220.inc"
};
TalkSelLines gTalkSelects_5_7_0_1 = {
#include "lobby/00810228.inc"
};
TalkSelLines gTalkSelects_5_8_0_0 = {
#include "lobby/00810230.inc"
};
TalkSelLines gTalkSelects_5_8_0_1 = {
#include "lobby/00810238.inc"
};
TalkSelLines gTalkSelects_5_9_0_0 = {
#include "lobby/00810240.inc"
};
TalkSelLines gTalkSelects_5_9_0_1 = {
#include "lobby/00810248.inc"
};
TalkSelLines gTalkSelects_5_10_0_0 = {
#include "lobby/00810250.inc"
};
TalkSelLines gTalkSelects_5_10_0_1 = {
#include "lobby/00810258.inc"
};
TalkSelLines gTalkSelects_5_10_1_0 = {
#include "lobby/00810260.inc"
};
TalkSelLines gTalkSelects_5_10_1_1 = {
#include "lobby/00810268.inc"
};
TalkSelLines gTalkSelects_5_10_2_0 = {
#include "lobby/00810270.inc"
};
TalkSelLines gTalkSelects_5_10_2_1 = {
#include "lobby/00810278.inc"
};
TalkSelLines gTalkSelects_5_11_0_0 = {
#include "lobby/00810280.inc"
};
TalkSelLines gTalkSelects_5_11_0_1 = {
#include "lobby/00810288.inc"
};
TalkSelLines gTalkSelects_5_12_0_0 = {
#include "lobby/00810290.inc"
};
TalkSelLines gTalkSelects_5_12_0_1 = {
#include "lobby/00810298.inc"
};
TalkSelLines gTalkSelects_5_13_0_0 = {
#include "lobby/008102A0.inc"
};
TalkSel gTalkSelects_5_13_0[1] = {
    {0, &gTalkSelects_5_13_0_0},
};
TalkSel gTalkSelects_5_13[2] = {
    {0, gTalkSelects_5_13_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_12_0[2] = {
#include "lobby/008102C0.inc"
};
TalkSel gTalkSelects_5_12[2] = {
    {0, gTalkSelects_5_12_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_11_0[2] = {
#include "lobby/008102E0.inc"
};
TalkSel gTalkSelects_5_11[2] = {
    {0, gTalkSelects_5_11_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_10_2[2] = {
#include "lobby/00810300.inc"
};
TalkSel gTalkSelects_5_10_1[2] = {
#include "lobby/00810310.inc"
};
TalkSel gTalkSelects_5_10_0[2] = {
#include "lobby/00810320.inc"
};
TalkSel gTalkSelects_5_10[4] = {
#include "lobby/00810330.inc"
};
TalkSel gTalkSelects_5_9_0[2] = {
#include "lobby/00810350.inc"
};
TalkSel gTalkSelects_5_9[2] = {
    {0, gTalkSelects_5_9_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_8_0[2] = {
#include "lobby/00810370.inc"
};
TalkSel gTalkSelects_5_8[2] = {
    {0, gTalkSelects_5_8_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_7_0[2] = {
#include "lobby/00810390.inc"
};
TalkSel gTalkSelects_5_7[2] = {
    {0, gTalkSelects_5_7_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_6_0[2] = {
#include "lobby/008103B0.inc"
};
TalkSel gTalkSelects_5_6[2] = {
    {0, gTalkSelects_5_6_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_5_0[2] = {
#include "lobby/008103D0.inc"
};
TalkSel gTalkSelects_5_5[2] = {
    {0, gTalkSelects_5_5_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_4_0[2] = {
#include "lobby/008103F0.inc"
};
TalkSel gTalkSelects_5_4[2] = {
    {0, gTalkSelects_5_4_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_3_0[2] = {
#include "lobby/00810410.inc"
};
TalkSel gTalkSelects_5_3[2] = {
    {0, gTalkSelects_5_3_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_2_0[2] = {
#include "lobby/00810430.inc"
};
TalkSel gTalkSelects_5_2[2] = {
    {0, gTalkSelects_5_2_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_1_0[2] = {
#include "lobby/00810450.inc"
};
TalkSel gTalkSelects_5_1[2] = {
    {0, gTalkSelects_5_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_5_0_2[2] = {
#include "lobby/00810470.inc"
};
TalkSel gTalkSelects_5_0_1[2] = {
#include "lobby/00810480.inc"
};
TalkSel gTalkSelects_5_0_0[2] = {
#include "lobby/00810490.inc"
};
TalkSel gTalkSelects_5_0[4] = {
#include "lobby/008104A0.inc"
};
TalkSel gTalkSelects_5[14] = {
#include "lobby/008104C0.inc"
};
TalkLine gTalkLines_5[336] = {
#include "lobby/00810530.inc"
};
TalkGift gTalkGifts_5[6] = {
#include "lobby/00810A70.inc"
};
TalkEvent gTalkEvents_6[2] = {
#include "lobby/00810AA0.inc"
};
TalkSelLines gTalkSelects_6_0_0_0 = {
#include "lobby/00810AA8.inc"
};
TalkSelLines gTalkSelects_6_0_1_0 = {
#include "lobby/00810AB0.inc"
};
TalkSelLines gTalkSelects_6_0_2_0 = {
#include "lobby/00810AB8.inc"
};
TalkSelLines gTalkSelects_6_1_0_0 = {
#include "lobby/00810AC0.inc"
};
TalkSelLines gTalkSelects_6_1_1_0 = {
#include "lobby/00810AC8.inc"
};
TalkSelLines gTalkSelects_6_1_2_0 = {
#include "lobby/00810AD0.inc"
};
TalkSelLines gTalkSelects_6_2_0_0 = {
#include "lobby/00810AD8.inc"
};
TalkSelLines gTalkSelects_6_3_0_0 = {
#include "lobby/00810AE0.inc"
};
TalkSelLines gTalkSelects_6_3_1_0 = {
#include "lobby/00810AE8.inc"
};
TalkSelLines gTalkSelects_6_3_2_0 = {
#include "lobby/00810AF0.inc"
};
TalkSel gTalkSelects_6_3_2[1] = {
    {0, &gTalkSelects_6_3_2_0},
};
TalkSel gTalkSelects_6_3_1[1] = {
    {0, &gTalkSelects_6_3_1_0},
};
TalkSel gTalkSelects_6_3_0[1] = {
    {0, &gTalkSelects_6_3_0_0},
};
TalkSel gTalkSelects_6_3[3] = {
#include "lobby/00810B10.inc"
};
TalkSel gTalkSelects_6_2_0[1] = {
    {0, &gTalkSelects_6_2_0_0},
};
TalkSel gTalkSelects_6_2[1] = {
    {0, gTalkSelects_6_2_0},
};
TalkSel gTalkSelects_6_1_2[1] = {
    {0, &gTalkSelects_6_1_2_0},
};
TalkSel gTalkSelects_6_1_1[1] = {
    {0, &gTalkSelects_6_1_1_0},
};
TalkSel gTalkSelects_6_1_0[1] = {
    {0, &gTalkSelects_6_1_0_0},
};
TalkSel gTalkSelects_6_1[3] = {
#include "lobby/00810B50.inc"
};
TalkSel gTalkSelects_6_0_2[1] = {
    {0, &gTalkSelects_6_0_2_0},
};
TalkSel gTalkSelects_6_0_1[1] = {
    {0, &gTalkSelects_6_0_1_0},
};
TalkSel gTalkSelects_6_0_0[1] = {
    {0, &gTalkSelects_6_0_0_0},
};
TalkSel gTalkSelects_6_0[4] = {
#include "lobby/00810B80.inc"
};
TalkSel gTalkSelects_6[4] = {
#include "lobby/00810BA0.inc"
};
TalkLine gTalkLines_6[52] = {
#include "lobby/00810BC0.inc"
};
TalkEvent gTalkEvents_7[12] = {
#include "lobby/00810C90.inc"
};
TalkSelLines gTalkSelects_7_0_0_0 = {
#include "lobby/00810CC0.inc"
};
TalkSelLines gTalkSelects_7_0_0_1 = {
#include "lobby/00810CC8.inc"
};
TalkSelLines gTalkSelects_7_0_1_0 = {
#include "lobby/00810CD0.inc"
};
TalkSelLines gTalkSelects_7_0_1_1 = {
#include "lobby/00810CD8.inc"
};
TalkSelLines gTalkSelects_7_0_2_0 = {
#include "lobby/00810CE0.inc"
};
TalkSelLines gTalkSelects_7_0_2_1 = {
#include "lobby/00810CE8.inc"
};
TalkSelLines gTalkSelects_7_1_0_0 = {
#include "lobby/00810CF0.inc"
};
TalkSelLines gTalkSelects_7_1_0_1 = {
#include "lobby/00810CF8.inc"
};
TalkSelLines gTalkSelects_7_2_0_0 = {
#include "lobby/00810D00.inc"
};
TalkSelLines gTalkSelects_7_2_0_1 = {
#include "lobby/00810D08.inc"
};
TalkSelLines gTalkSelects_7_3_0_0 = {
#include "lobby/00810D10.inc"
};
TalkSelLines gTalkSelects_7_3_0_1 = {
#include "lobby/00810D18.inc"
};
TalkSelLines gTalkSelects_7_3_1_0 = {
#include "lobby/00810D20.inc"
};
TalkSelLines gTalkSelects_7_3_1_1 = {
#include "lobby/00810D28.inc"
};
TalkSelLines gTalkSelects_7_3_2_0 = {
#include "lobby/00810D30.inc"
};
TalkSelLines gTalkSelects_7_3_2_1 = {
#include "lobby/00810D38.inc"
};
TalkSelLines gTalkSelects_7_4_0_0 = {
#include "lobby/00810D40.inc"
};
TalkSelLines gTalkSelects_7_4_0_1 = {
#include "lobby/00810D48.inc"
};
TalkSelLines gTalkSelects_7_5_0_0 = {
#include "lobby/00810D50.inc"
};
TalkSelLines gTalkSelects_7_5_0_1 = {
#include "lobby/00810D58.inc"
};
TalkSelLines gTalkSelects_7_6_0_0 = {
#include "lobby/00810D60.inc"
};
TalkSelLines gTalkSelects_7_6_0_1 = {
#include "lobby/00810D68.inc"
};
TalkSelLines gTalkSelects_7_6_1_0 = {
#include "lobby/00810D70.inc"
};
TalkSelLines gTalkSelects_7_6_1_1 = {
#include "lobby/00810D78.inc"
};
TalkSelLines gTalkSelects_7_6_2_0 = {
#include "lobby/00810D80.inc"
};
TalkSelLines gTalkSelects_7_6_2_1 = {
#include "lobby/00810D88.inc"
};
TalkSelLines gTalkSelects_7_7_0_0 = {
#include "lobby/00810D90.inc"
};
TalkSelLines gTalkSelects_7_7_0_1 = {
#include "lobby/00810D98.inc"
};
TalkSelLines gTalkSelects_7_7_1_0 = {
#include "lobby/00810DA0.inc"
};
TalkSelLines gTalkSelects_7_7_1_1 = {
#include "lobby/00810DA8.inc"
};
TalkSelLines gTalkSelects_7_7_2_0 = {
#include "lobby/00810DB0.inc"
};
TalkSelLines gTalkSelects_7_7_2_1 = {
#include "lobby/00810DB8.inc"
};
TalkSelLines gTalkSelects_7_8_0_0 = {
#include "lobby/00810DC0.inc"
};
TalkSel gTalkSelects_7_8_0[1] = {
    {0, &gTalkSelects_7_8_0_0},
};
TalkSel gTalkSelects_7_8[2] = {
    {0, gTalkSelects_7_8_0},
    {0, NULL},
};
TalkSel gTalkSelects_7_7_2[2] = {
#include "lobby/00810DE0.inc"
};
TalkSel gTalkSelects_7_7_1[2] = {
#include "lobby/00810DF0.inc"
};
TalkSel gTalkSelects_7_7_0[2] = {
#include "lobby/00810E00.inc"
};
TalkSel gTalkSelects_7_7[4] = {
#include "lobby/00810E10.inc"
};
TalkSel gTalkSelects_7_6_2[2] = {
#include "lobby/00810E30.inc"
};
TalkSel gTalkSelects_7_6_1[2] = {
#include "lobby/00810E40.inc"
};
TalkSel gTalkSelects_7_6_0[2] = {
#include "lobby/00810E50.inc"
};
TalkSel gTalkSelects_7_6[4] = {
#include "lobby/00810E60.inc"
};
TalkSel gTalkSelects_7_5_0[2] = {
#include "lobby/00810E80.inc"
};
TalkSel gTalkSelects_7_5[2] = {
    {0, gTalkSelects_7_5_0},
    {0, NULL},
};
TalkSel gTalkSelects_7_4_0[2] = {
#include "lobby/00810EA0.inc"
};
TalkSel gTalkSelects_7_4[2] = {
    {0, gTalkSelects_7_4_0},
    {0, NULL},
};
TalkSel gTalkSelects_7_3_2[2] = {
#include "lobby/00810EC0.inc"
};
TalkSel gTalkSelects_7_3_1[2] = {
#include "lobby/00810ED0.inc"
};
TalkSel gTalkSelects_7_3_0[2] = {
#include "lobby/00810EE0.inc"
};
TalkSel gTalkSelects_7_3[4] = {
#include "lobby/00810EF0.inc"
};
TalkSel gTalkSelects_7_2_0[2] = {
#include "lobby/00810F10.inc"
};
TalkSel gTalkSelects_7_2[2] = {
    {0, gTalkSelects_7_2_0},
    {0, NULL},
};
TalkSel gTalkSelects_7_1_0[2] = {
#include "lobby/00810F30.inc"
};
TalkSel gTalkSelects_7_1[2] = {
    {0, gTalkSelects_7_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_7_0_2[2] = {
#include "lobby/00810F50.inc"
};
TalkSel gTalkSelects_7_0_1[2] = {
#include "lobby/00810F60.inc"
};
TalkSel gTalkSelects_7_0_0[2] = {
#include "lobby/00810F70.inc"
};
TalkSel gTalkSelects_7_0[4] = {
#include "lobby/00810F80.inc"
};
TalkSel gTalkSelects_7[10] = {
#include "lobby/00810FA0.inc"
};
TalkLine gTalkLines_7[88] = {
#include "lobby/00810FF0.inc"
};
TalkGift gTalkGifts_7[2] = {
#include "lobby/00811150.inc"
};
TalkEvent gTalkEvents_8[12] = {
#include "lobby/00811160.inc"
};
TalkSelLines gTalkSelects_8_0_0_0 = {
#include "lobby/00811190.inc"
};
TalkSelLines gTalkSelects_8_0_0_1 = {
#include "lobby/00811198.inc"
};
TalkSelLines gTalkSelects_8_0_1_0 = {
#include "lobby/008111A0.inc"
};
TalkSelLines gTalkSelects_8_0_1_1 = {
#include "lobby/008111A8.inc"
};
TalkSelLines gTalkSelects_8_0_2_0 = {
#include "lobby/008111B0.inc"
};
TalkSelLines gTalkSelects_8_0_2_1 = {
#include "lobby/008111B8.inc"
};
TalkSelLines gTalkSelects_8_1_0_0 = {
#include "lobby/008111C0.inc"
};
TalkSelLines gTalkSelects_8_1_0_1 = {
#include "lobby/008111C8.inc"
};
TalkSelLines gTalkSelects_8_1_1_0 = {
#include "lobby/008111D0.inc"
};
TalkSelLines gTalkSelects_8_1_1_1 = {
#include "lobby/008111D8.inc"
};
TalkSelLines gTalkSelects_8_1_2_0 = {
#include "lobby/008111E0.inc"
};
TalkSelLines gTalkSelects_8_1_2_1 = {
#include "lobby/008111E8.inc"
};
TalkSelLines gTalkSelects_8_2_0_0 = {
#include "lobby/008111F0.inc"
};
TalkSelLines gTalkSelects_8_3_0_0 = {
#include "lobby/008111F8.inc"
};
TalkSelLines gTalkSelects_8_3_0_1 = {
#include "lobby/00811200.inc"
};
TalkSelLines gTalkSelects_8_3_1_0 = {
#include "lobby/00811208.inc"
};
TalkSelLines gTalkSelects_8_3_1_1 = {
#include "lobby/00811210.inc"
};
TalkSelLines gTalkSelects_8_3_2_0 = {
#include "lobby/00811218.inc"
};
TalkSelLines gTalkSelects_8_3_2_1 = {
#include "lobby/00811220.inc"
};
TalkSelLines gTalkSelects_8_4_0_0 = {
#include "lobby/00811228.inc"
};
TalkSelLines gTalkSelects_8_4_0_1 = {
#include "lobby/00811230.inc"
};
TalkSelLines gTalkSelects_8_4_1_0 = {
#include "lobby/00811238.inc"
};
TalkSelLines gTalkSelects_8_4_1_1 = {
#include "lobby/00811240.inc"
};
TalkSelLines gTalkSelects_8_4_2_0 = {
#include "lobby/00811248.inc"
};
TalkSelLines gTalkSelects_8_4_2_1 = {
#include "lobby/00811250.inc"
};
TalkSelLines gTalkSelects_8_5_0_0 = {
#include "lobby/00811258.inc"
};
TalkSelLines gTalkSelects_8_5_0_1 = {
#include "lobby/00811260.inc"
};
TalkSelLines gTalkSelects_8_5_1_0 = {
#include "lobby/00811268.inc"
};
TalkSelLines gTalkSelects_8_5_1_1 = {
#include "lobby/00811270.inc"
};
TalkSelLines gTalkSelects_8_5_2_0 = {
#include "lobby/00811278.inc"
};
TalkSelLines gTalkSelects_8_5_2_1 = {
#include "lobby/00811280.inc"
};
TalkSelLines gTalkSelects_8_6_0_0 = {
#include "lobby/00811288.inc"
};
TalkSelLines gTalkSelects_8_6_0_1 = {
#include "lobby/00811290.inc"
};
TalkSelLines gTalkSelects_8_6_1_0 = {
#include "lobby/00811298.inc"
};
TalkSelLines gTalkSelects_8_6_1_1 = {
#include "lobby/008112A0.inc"
};
TalkSelLines gTalkSelects_8_6_2_0 = {
#include "lobby/008112A8.inc"
};
TalkSelLines gTalkSelects_8_6_2_1 = {
#include "lobby/008112B0.inc"
};
TalkSelLines gTalkSelects_8_7_0_0 = {
#include "lobby/008112B8.inc"
};
TalkSelLines gTalkSelects_8_8_0_0 = {
#include "lobby/008112C0.inc"
};
TalkSel gTalkSelects_8_8_0[1] = {
    {0, &gTalkSelects_8_8_0_0},
};
TalkSel gTalkSelects_8_8[1] = {
    {0, gTalkSelects_8_8_0},
};
TalkSel gTalkSelects_8_7_0[1] = {
    {0, &gTalkSelects_8_7_0_0},
};
TalkSel gTalkSelects_8_7[2] = {
    {0, gTalkSelects_8_7_0},
    {0, NULL},
};
TalkSel gTalkSelects_8_6_2[2] = {
#include "lobby/008112F0.inc"
};
TalkSel gTalkSelects_8_6_1[2] = {
#include "lobby/00811300.inc"
};
TalkSel gTalkSelects_8_6_0[2] = {
#include "lobby/00811310.inc"
};
TalkSel gTalkSelects_8_6[4] = {
#include "lobby/00811320.inc"
};
TalkSel gTalkSelects_8_5_2[2] = {
#include "lobby/00811340.inc"
};
TalkSel gTalkSelects_8_5_1[2] = {
#include "lobby/00811350.inc"
};
TalkSel gTalkSelects_8_5_0[2] = {
#include "lobby/00811360.inc"
};
TalkSel gTalkSelects_8_5[4] = {
#include "lobby/00811370.inc"
};
TalkSel gTalkSelects_8_4_2[2] = {
#include "lobby/00811390.inc"
};
TalkSel gTalkSelects_8_4_1[2] = {
#include "lobby/008113A0.inc"
};
TalkSel gTalkSelects_8_4_0[2] = {
#include "lobby/008113B0.inc"
};
TalkSel gTalkSelects_8_4[4] = {
#include "lobby/008113C0.inc"
};
TalkSel gTalkSelects_8_3_2[2] = {
#include "lobby/008113E0.inc"
};
TalkSel gTalkSelects_8_3_1[2] = {
#include "lobby/008113F0.inc"
};
TalkSel gTalkSelects_8_3_0[2] = {
#include "lobby/00811400.inc"
};
TalkSel gTalkSelects_8_3[3] = {
#include "lobby/00811410.inc"
};
TalkSel gTalkSelects_8_2_0[1] = {
    {0, &gTalkSelects_8_2_0_0},
};
TalkSel gTalkSelects_8_2[2] = {
    {0, gTalkSelects_8_2_0},
    {0, NULL},
};
TalkSel gTalkSelects_8_1_2[2] = {
#include "lobby/00811440.inc"
};
TalkSel gTalkSelects_8_1_1[2] = {
#include "lobby/00811450.inc"
};
TalkSel gTalkSelects_8_1_0[2] = {
#include "lobby/00811460.inc"
};
TalkSel gTalkSelects_8_1[4] = {
#include "lobby/00811470.inc"
};
TalkSel gTalkSelects_8_0_2[2] = {
#include "lobby/00811490.inc"
};
TalkSel gTalkSelects_8_0_1[2] = {
#include "lobby/008114A0.inc"
};
TalkSel gTalkSelects_8_0_0[2] = {
#include "lobby/008114B0.inc"
};
TalkSel gTalkSelects_8_0[4] = {
#include "lobby/008114C0.inc"
};
TalkSel gTalkSelects_8[10] = {
#include "lobby/008114E0.inc"
};
TalkLine gTalkLines_8[116] = {
#include "lobby/00811530.inc"
};
TalkGift gTalkGifts_8[3] = {
#include "lobby/00811700.inc"
};
TalkEvent gTalkEvents_9[2] = {
#include "lobby/00811718.inc"
};
TalkSelLines gTalkSelects_9_0_0_0 = {
#include "lobby/00811720.inc"
};
TalkSelLines gTalkSelects_9_0_1_0 = {
#include "lobby/00811728.inc"
};
TalkSelLines gTalkSelects_9_0_2_0 = {
#include "lobby/00811730.inc"
};
TalkSelLines gTalkSelects_9_1_0_0 = {
#include "lobby/00811738.inc"
};
TalkSelLines gTalkSelects_9_1_1_0 = {
#include "lobby/00811740.inc"
};
TalkSelLines gTalkSelects_9_1_2_0 = {
#include "lobby/00811748.inc"
};
TalkSelLines gTalkSelects_9_2_0_0 = {
#include "lobby/00811750.inc"
};
TalkSelLines gTalkSelects_9_2_1_0 = {
#include "lobby/00811758.inc"
};
TalkSelLines gTalkSelects_9_2_2_0 = {
#include "lobby/00811760.inc"
};
TalkSelLines gTalkSelects_9_3_0_0 = {
#include "lobby/00811768.inc"
};
TalkSelLines gTalkSelects_9_3_1_0 = {
#include "lobby/00811770.inc"
};
TalkSelLines gTalkSelects_9_3_2_0 = {
#include "lobby/00811778.inc"
};
TalkSelLines gTalkSelects_9_4_0_0 = {
#include "lobby/00811780.inc"
};
TalkSelLines gTalkSelects_9_4_1_0 = {
#include "lobby/00811788.inc"
};
TalkSelLines gTalkSelects_9_4_2_0 = {
#include "lobby/00811790.inc"
};
TalkSelLines gTalkSelects_9_5_0_0 = {
#include "lobby/00811798.inc"
};
TalkSelLines gTalkSelects_9_5_0_1 = {
#include "lobby/008117A0.inc"
};
TalkSelLines gTalkSelects_9_5_1_0 = {
#include "lobby/008117A8.inc"
};
TalkSelLines gTalkSelects_9_5_1_1 = {
#include "lobby/008117B0.inc"
};
TalkSelLines gTalkSelects_9_5_2_0 = {
#include "lobby/008117B8.inc"
};
TalkSelLines gTalkSelects_9_5_2_1 = {
#include "lobby/008117C0.inc"
};
TalkSelLines gTalkSelects_9_6_0_0 = {
#include "lobby/008117C8.inc"
};
TalkSelLines gTalkSelects_9_6_0_1 = {
#include "lobby/008117D0.inc"
};
TalkSelLines gTalkSelects_9_7_0_0 = {
#include "lobby/008117D8.inc"
};
TalkSelLines gTalkSelects_9_7_0_1 = {
#include "lobby/008117E0.inc"
};
TalkSelLines gTalkSelects_9_8_0_0 = {
#include "lobby/008117E8.inc"
};
TalkSelLines gTalkSelects_9_8_0_1 = {
#include "lobby/008117F0.inc"
};
TalkSel gTalkSelects_9_8_0[2] = {
#include "lobby/00811800.inc"
};
TalkSel gTalkSelects_9_8[2] = {
    {0, gTalkSelects_9_8_0},
    {0, NULL},
};
TalkSel gTalkSelects_9_7_0[2] = {
#include "lobby/00811820.inc"
};
TalkSel gTalkSelects_9_7[2] = {
    {0, gTalkSelects_9_7_0},
    {0, NULL},
};
TalkSel gTalkSelects_9_6_0[2] = {
#include "lobby/00811840.inc"
};
TalkSel gTalkSelects_9_6[2] = {
    {0, gTalkSelects_9_6_0},
    {0, NULL},
};
TalkSel gTalkSelects_9_5_2[2] = {
#include "lobby/00811860.inc"
};
TalkSel gTalkSelects_9_5_1[2] = {
#include "lobby/00811870.inc"
};
TalkSel gTalkSelects_9_5_0[2] = {
#include "lobby/00811880.inc"
};
TalkSel gTalkSelects_9_5[3] = {
#include "lobby/00811890.inc"
};
TalkSel gTalkSelects_9_4_2[1] = {
    {0, &gTalkSelects_9_4_2_0},
};
TalkSel gTalkSelects_9_4_1[1] = {
    {0, &gTalkSelects_9_4_1_0},
};
TalkSel gTalkSelects_9_4_0[1] = {
    {0, &gTalkSelects_9_4_0_0},
};
TalkSel gTalkSelects_9_4[3] = {
#include "lobby/008118C0.inc"
};
TalkSel gTalkSelects_9_3_2[1] = {
    {0, &gTalkSelects_9_3_2_0},
};
TalkSel gTalkSelects_9_3_1[1] = {
    {0, &gTalkSelects_9_3_1_0},
};
TalkSel gTalkSelects_9_3_0[1] = {
    {0, &gTalkSelects_9_3_0_0},
};
TalkSel gTalkSelects_9_3[3] = {
#include "lobby/008118F0.inc"
};
TalkSel gTalkSelects_9_2_2[1] = {
    {0, &gTalkSelects_9_2_2_0},
};
TalkSel gTalkSelects_9_2_1[1] = {
    {0, &gTalkSelects_9_2_1_0},
};
TalkSel gTalkSelects_9_2_0[1] = {
    {0, &gTalkSelects_9_2_0_0},
};
TalkSel gTalkSelects_9_2[3] = {
#include "lobby/00811920.inc"
};
TalkSel gTalkSelects_9_1_2[1] = {
    {0, &gTalkSelects_9_1_2_0},
};
TalkSel gTalkSelects_9_1_1[1] = {
    {0, &gTalkSelects_9_1_1_0},
};
TalkSel gTalkSelects_9_1_0[1] = {
    {0, &gTalkSelects_9_1_0_0},
};
TalkSel gTalkSelects_9_1[3] = {
#include "lobby/00811950.inc"
};
TalkSel gTalkSelects_9_0_2[1] = {
    {0, &gTalkSelects_9_0_2_0},
};
TalkSel gTalkSelects_9_0_1[1] = {
    {0, &gTalkSelects_9_0_1_0},
};
TalkSel gTalkSelects_9_0_0[1] = {
    {0, &gTalkSelects_9_0_0_0},
};
TalkSel gTalkSelects_9_0[4] = {
#include "lobby/00811980.inc"
};
TalkSel gTalkSelects_9[10] = {
#include "lobby/008119A0.inc"
};
TalkLine gTalkLines_9[164] = {
#include "lobby/008119F0.inc"
};
TalkEvent gTalkEvents_10[4] = {
#include "lobby/00811C80.inc"
};
TalkSelLines gTalkSelects_10_0_0_0 = {
#include "lobby/00811C90.inc"
};
TalkSelLines gTalkSelects_10_0_0_1 = {
#include "lobby/00811C98.inc"
};
TalkSelLines gTalkSelects_10_0_1_0 = {
#include "lobby/00811CA0.inc"
};
TalkSelLines gTalkSelects_10_0_1_1 = {
#include "lobby/00811CA8.inc"
};
TalkSelLines gTalkSelects_10_0_2_0 = {
#include "lobby/00811CB0.inc"
};
TalkSelLines gTalkSelects_10_0_2_1 = {
#include "lobby/00811CB8.inc"
};
TalkSel gTalkSelects_10_0_2[2] = {
#include "lobby/00811CC0.inc"
};
TalkSel gTalkSelects_10_0_1[2] = {
#include "lobby/00811CD0.inc"
};
TalkSel gTalkSelects_10_0_0[2] = {
#include "lobby/00811CE0.inc"
};
TalkSel gTalkSelects_10_0[3] = {
#include "lobby/00811CF0.inc"
};
TalkSel gTalkSelects_10[1] = {
    {0, gTalkSelects_10_0},
};
TalkLine gTalkLines_10[28] = {
#include "lobby/00811D10.inc"
};
TalkGift gTalkGifts_10[2] = {
#include "lobby/00811D80.inc"
};
TalkEvent gTalkEvents_11[6] = {
#include "lobby/00811D90.inc"
};
TalkSelLines gTalkSelects_11_0_0_0 = {
#include "lobby/00811DA8.inc"
};
TalkSelLines gTalkSelects_11_0_0_1 = {
#include "lobby/00811DB0.inc"
};
TalkSelLines gTalkSelects_11_0_1_0 = {
#include "lobby/00811DB8.inc"
};
TalkSelLines gTalkSelects_11_0_1_1 = {
#include "lobby/00811DC0.inc"
};
TalkSelLines gTalkSelects_11_0_2_0 = {
#include "lobby/00811DC8.inc"
};
TalkSelLines gTalkSelects_11_0_2_1 = {
#include "lobby/00811DD0.inc"
};
TalkSel gTalkSelects_11_0_2[2] = {
#include "lobby/00811DE0.inc"
};
TalkSel gTalkSelects_11_0_1[2] = {
#include "lobby/00811DF0.inc"
};
TalkSel gTalkSelects_11_0_0[2] = {
#include "lobby/00811E00.inc"
};
TalkSel gTalkSelects_11_0[3] = {
#include "lobby/00811E10.inc"
};
TalkSel gTalkSelects_11[1] = {
    {0, gTalkSelects_11_0},
};
TalkLine gTalkLines_11[32] = {
#include "lobby/00811E30.inc"
};
TalkGift gTalkGifts_11[2] = {
#include "lobby/00811EB0.inc"
};
TalkEvent gTalkEvents_12[4] = {
#include "lobby/00811EC0.inc"
};
TalkSelLines gTalkSelects_12_0_0_0 = {
#include "lobby/00811ED0.inc"
};
TalkSelLines gTalkSelects_12_0_0_1 = {
#include "lobby/00811ED8.inc"
};
TalkSelLines gTalkSelects_12_1_0_0 = {
#include "lobby/00811EE0.inc"
};
TalkSelLines gTalkSelects_12_1_0_1 = {
#include "lobby/00811EE8.inc"
};
TalkSel gTalkSelects_12_1_0[2] = {
#include "lobby/00811EF0.inc"
};
TalkSel gTalkSelects_12_1[2] = {
    {0, gTalkSelects_12_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_12_0_0[2] = {
#include "lobby/00811F10.inc"
};
TalkSel gTalkSelects_12_0[2] = {
    {0, gTalkSelects_12_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_12[2] = {
#include "lobby/00811F30.inc"
};
TalkLine gTalkLines_12[32] = {
#include "lobby/00811F40.inc"
};
TalkGift gTalkGifts_12[2] = {
#include "lobby/00811FC0.inc"
};
TalkEvent gTalkEvents_13[4] = {
#include "lobby/00811FD0.inc"
};
TalkSelLines gTalkSelects_13_0_0_0 = {
#include "lobby/00811FE0.inc"
};
TalkSelLines gTalkSelects_13_0_0_1 = {
#include "lobby/00811FE8.inc"
};
TalkSelLines gTalkSelects_13_0_1_0 = {
#include "lobby/00811FF0.inc"
};
TalkSelLines gTalkSelects_13_0_1_1 = {
#include "lobby/00811FF8.inc"
};
TalkSelLines gTalkSelects_13_0_2_0 = {
#include "lobby/00812000.inc"
};
TalkSelLines gTalkSelects_13_0_2_1 = {
#include "lobby/00812008.inc"
};
TalkSel gTalkSelects_13_0_2[2] = {
#include "lobby/00812010.inc"
};
TalkSel gTalkSelects_13_0_1[2] = {
#include "lobby/00812020.inc"
};
TalkSel gTalkSelects_13_0_0[2] = {
#include "lobby/00812030.inc"
};
TalkSel gTalkSelects_13_0[3] = {
#include "lobby/00812040.inc"
};
TalkSel gTalkSelects_13[1] = {
    {0, gTalkSelects_13_0},
};
TalkLine gTalkLines_13[28] = {
#include "lobby/00812060.inc"
};
TalkGift gTalkGifts_13[2] = {
#include "lobby/008120D0.inc"
};
TalkEvent gTalkEvents_14[18] = {
#include "lobby/008120E0.inc"
};
TalkSelLines gTalkSelects_14_0_0_0 = {
#include "lobby/00812128.inc"
};
TalkSelLines gTalkSelects_14_0_0_1 = {
#include "lobby/00812130.inc"
};
TalkSelLines gTalkSelects_14_0_1_0 = {
#include "lobby/00812138.inc"
};
TalkSelLines gTalkSelects_14_0_1_1 = {
#include "lobby/00812140.inc"
};
TalkSelLines gTalkSelects_14_0_2_0 = {
#include "lobby/00812148.inc"
};
TalkSelLines gTalkSelects_14_0_2_1 = {
#include "lobby/00812150.inc"
};
TalkSelLines gTalkSelects_14_1_0_0 = {
#include "lobby/00812158.inc"
};
TalkSelLines gTalkSelects_14_1_0_1 = {
#include "lobby/00812160.inc"
};
TalkSelLines gTalkSelects_14_1_1_0 = {
#include "lobby/00812168.inc"
};
TalkSelLines gTalkSelects_14_1_1_1 = {
#include "lobby/00812170.inc"
};
TalkSelLines gTalkSelects_14_1_2_0 = {
#include "lobby/00812178.inc"
};
TalkSelLines gTalkSelects_14_1_2_1 = {
#include "lobby/00812180.inc"
};
TalkSelLines gTalkSelects_14_2_0_0 = {
#include "lobby/00812188.inc"
};
TalkSelLines gTalkSelects_14_2_0_1 = {
#include "lobby/00812190.inc"
};
TalkSelLines gTalkSelects_14_2_1_0 = {
#include "lobby/00812198.inc"
};
TalkSelLines gTalkSelects_14_2_1_1 = {
#include "lobby/008121A0.inc"
};
TalkSelLines gTalkSelects_14_2_2_0 = {
#include "lobby/008121A8.inc"
};
TalkSelLines gTalkSelects_14_2_2_1 = {
#include "lobby/008121B0.inc"
};
TalkSelLines gTalkSelects_14_3_0_0 = {
#include "lobby/008121B8.inc"
};
TalkSelLines gTalkSelects_14_3_0_1 = {
#include "lobby/008121C0.inc"
};
TalkSelLines gTalkSelects_14_3_1_0 = {
#include "lobby/008121C8.inc"
};
TalkSelLines gTalkSelects_14_3_1_1 = {
#include "lobby/008121D0.inc"
};
TalkSelLines gTalkSelects_14_3_2_0 = {
#include "lobby/008121D8.inc"
};
TalkSelLines gTalkSelects_14_3_2_1 = {
#include "lobby/008121E0.inc"
};
TalkSelLines gTalkSelects_14_4_0_0 = {
#include "lobby/008121E8.inc"
};
TalkSelLines gTalkSelects_14_4_0_1 = {
#include "lobby/008121F0.inc"
};
TalkSelLines gTalkSelects_14_4_1_0 = {
#include "lobby/008121F8.inc"
};
TalkSelLines gTalkSelects_14_4_1_1 = {
#include "lobby/00812200.inc"
};
TalkSelLines gTalkSelects_14_4_2_0 = {
#include "lobby/00812208.inc"
};
TalkSelLines gTalkSelects_14_4_2_1 = {
#include "lobby/00812210.inc"
};
TalkSelLines gTalkSelects_14_5_0_0 = {
#include "lobby/00812218.inc"
};
TalkSelLines gTalkSelects_14_5_0_1 = {
#include "lobby/00812220.inc"
};
TalkSelLines gTalkSelects_14_6_0_0 = {
#include "lobby/00812228.inc"
};
TalkSelLines gTalkSelects_14_7_0_0 = {
#include "lobby/00812230.inc"
};
TalkSel gTalkSelects_14_7_0[1] = {
    {0, &gTalkSelects_14_7_0_0},
};
TalkSel gTalkSelects_14_7[1] = {
    {0, gTalkSelects_14_7_0},
};
TalkSel gTalkSelects_14_6_0[1] = {
    {0, &gTalkSelects_14_6_0_0},
};
TalkSel gTalkSelects_14_6[2] = {
    {0, gTalkSelects_14_6_0},
    {0, NULL},
};
TalkSel gTalkSelects_14_5_0[2] = {
#include "lobby/00812260.inc"
};
TalkSel gTalkSelects_14_5[2] = {
    {0, gTalkSelects_14_5_0},
    {0, NULL},
};
TalkSel gTalkSelects_14_4_2[2] = {
#include "lobby/00812280.inc"
};
TalkSel gTalkSelects_14_4_1[2] = {
#include "lobby/00812290.inc"
};
TalkSel gTalkSelects_14_4_0[2] = {
#include "lobby/008122A0.inc"
};
TalkSel gTalkSelects_14_4[4] = {
#include "lobby/008122B0.inc"
};
TalkSel gTalkSelects_14_3_2[2] = {
#include "lobby/008122D0.inc"
};
TalkSel gTalkSelects_14_3_1[2] = {
#include "lobby/008122E0.inc"
};
TalkSel gTalkSelects_14_3_0[2] = {
#include "lobby/008122F0.inc"
};
TalkSel gTalkSelects_14_3[4] = {
#include "lobby/00812300.inc"
};
TalkSel gTalkSelects_14_2_2[2] = {
#include "lobby/00812320.inc"
};
TalkSel gTalkSelects_14_2_1[2] = {
#include "lobby/00812330.inc"
};
TalkSel gTalkSelects_14_2_0[2] = {
#include "lobby/00812340.inc"
};
TalkSel gTalkSelects_14_2[4] = {
#include "lobby/00812350.inc"
};
TalkSel gTalkSelects_14_1_2[2] = {
#include "lobby/00812370.inc"
};
TalkSel gTalkSelects_14_1_1[2] = {
#include "lobby/00812380.inc"
};
TalkSel gTalkSelects_14_1_0[2] = {
#include "lobby/00812390.inc"
};
TalkSel gTalkSelects_14_1[4] = {
#include "lobby/008123A0.inc"
};
TalkSel gTalkSelects_14_0_2[2] = {
#include "lobby/008123C0.inc"
};
TalkSel gTalkSelects_14_0_1[2] = {
#include "lobby/008123D0.inc"
};
TalkSel gTalkSelects_14_0_0[2] = {
#include "lobby/008123E0.inc"
};
TalkSel gTalkSelects_14_0[4] = {
#include "lobby/008123F0.inc"
};
TalkSel gTalkSelects_14[8] = {
#include "lobby/00812410.inc"
};
TalkLine gTalkLines_14[160] = {
#include "lobby/00812450.inc"
};
TalkGift gTalkGifts_14[2] = {
#include "lobby/008126D0.inc"
};
TalkEvent gTalkEvents_15[4] = {
#include "lobby/008126E0.inc"
};
TalkSelLines gTalkSelects_15_0_0_0 = {
#include "lobby/008126F0.inc"
};
TalkSel gTalkSelects_15_0_0[1] = {
    {0, &gTalkSelects_15_0_0_0},
};
TalkSel gTalkSelects_15_0[1] = {
    {0, gTalkSelects_15_0_0},
};
TalkSel gTalkSelects_15[1] = {
    {0, gTalkSelects_15_0},
};
TalkLine gTalkLines_15[28] = {
#include "lobby/00812710.inc"
};
TalkEvent gTalkEvents_16[12] = {
#include "lobby/00812780.inc"
};
TalkSelLines gTalkSelects_16_0_0_0 = {
#include "lobby/008127B0.inc"
};
TalkSelLines gTalkSelects_16_0_0_1 = {
#include "lobby/008127B8.inc"
};
TalkSelLines gTalkSelects_16_0_1_0 = {
#include "lobby/008127C0.inc"
};
TalkSelLines gTalkSelects_16_0_1_1 = {
#include "lobby/008127C8.inc"
};
TalkSelLines gTalkSelects_16_0_2_0 = {
#include "lobby/008127D0.inc"
};
TalkSelLines gTalkSelects_16_0_2_1 = {
#include "lobby/008127D8.inc"
};
TalkSelLines gTalkSelects_16_1_0_0 = {
#include "lobby/008127E0.inc"
};
TalkSelLines gTalkSelects_16_1_0_1 = {
#include "lobby/008127E8.inc"
};
TalkSelLines gTalkSelects_16_2_0_0 = {
#include "lobby/008127F0.inc"
};
TalkSelLines gTalkSelects_16_2_0_1 = {
#include "lobby/008127F8.inc"
};
TalkSelLines gTalkSelects_16_3_0_0 = {
#include "lobby/00812800.inc"
};
TalkSelLines gTalkSelects_16_4_0_0 = {
#include "lobby/00812808.inc"
};
TalkSelLines gTalkSelects_16_4_0_1 = {
#include "lobby/00812810.inc"
};
TalkSelLines gTalkSelects_16_5_0_0 = {
#include "lobby/00812818.inc"
};
TalkSelLines gTalkSelects_16_5_0_1 = {
#include "lobby/00812820.inc"
};
TalkSelLines gTalkSelects_16_6_0_0 = {
#include "lobby/00812828.inc"
};
TalkSelLines gTalkSelects_16_6_1_0 = {
#include "lobby/00812830.inc"
};
TalkSelLines gTalkSelects_16_6_2_0 = {
#include "lobby/00812838.inc"
};
TalkSelLines gTalkSelects_16_7_0_0 = {
#include "lobby/00812840.inc"
};
TalkSelLines gTalkSelects_16_7_0_1 = {
#include "lobby/00812848.inc"
};
TalkSelLines gTalkSelects_16_8_0_0 = {
#include "lobby/00812850.inc"
};
TalkSelLines gTalkSelects_16_8_0_1 = {
#include "lobby/00812858.inc"
};
TalkSel gTalkSelects_16_8_0[2] = {
#include "lobby/00812860.inc"
};
TalkSel gTalkSelects_16_8[2] = {
    {0, gTalkSelects_16_8_0},
    {0, NULL},
};
TalkSel gTalkSelects_16_7_0[2] = {
#include "lobby/00812880.inc"
};
TalkSel gTalkSelects_16_7[1] = {
    {0, gTalkSelects_16_7_0},
};
TalkSel gTalkSelects_16_6_2[1] = {
    {0, &gTalkSelects_16_6_2_0},
};
TalkSel gTalkSelects_16_6_1[1] = {
    {0, &gTalkSelects_16_6_1_0},
};
TalkSel gTalkSelects_16_6_0[1] = {
    {0, &gTalkSelects_16_6_0_0},
};
TalkSel gTalkSelects_16_6[4] = {
#include "lobby/008128B0.inc"
};
TalkSel gTalkSelects_16_5_0[2] = {
#include "lobby/008128D0.inc"
};
TalkSel gTalkSelects_16_5[2] = {
    {0, gTalkSelects_16_5_0},
    {0, NULL},
};
TalkSel gTalkSelects_16_4_0[2] = {
#include "lobby/008128F0.inc"
};
TalkSel gTalkSelects_16_4[1] = {
    {0, gTalkSelects_16_4_0},
};
TalkSel gTalkSelects_16_3_0[1] = {
    {0, &gTalkSelects_16_3_0_0},
};
TalkSel gTalkSelects_16_3[2] = {
    {0, gTalkSelects_16_3_0},
    {0, NULL},
};
TalkSel gTalkSelects_16_2_0[2] = {
#include "lobby/00812920.inc"
};
TalkSel gTalkSelects_16_2[2] = {
    {0, gTalkSelects_16_2_0},
    {0, NULL},
};
TalkSel gTalkSelects_16_1_0[2] = {
#include "lobby/00812940.inc"
};
TalkSel gTalkSelects_16_1[2] = {
    {0, gTalkSelects_16_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_16_0_2[2] = {
#include "lobby/00812960.inc"
};
TalkSel gTalkSelects_16_0_1[2] = {
#include "lobby/00812970.inc"
};
TalkSel gTalkSelects_16_0_0[2] = {
#include "lobby/00812980.inc"
};
TalkSel gTalkSelects_16_0[4] = {
#include "lobby/00812990.inc"
};
TalkSel gTalkSelects_16[10] = {
#include "lobby/008129B0.inc"
};
TalkLine gTalkLines_16[240] = {
#include "lobby/00812A00.inc"
};
TalkGift gTalkGifts_16[4] = {
#include "lobby/00812DC0.inc"
};
TalkEvent gTalkEvents_18[10] = {
#include "lobby/00812DE0.inc"
};
TalkSelLines gTalkSelects_18_0_0_0 = {
#include "lobby/00812E08.inc"
};
TalkSelLines gTalkSelects_18_1_0_0 = {
#include "lobby/00812E10.inc"
};
TalkSelLines gTalkSelects_18_2_0_0 = {
#include "lobby/00812E18.inc"
};
TalkSel gTalkSelects_18_2_0[1] = {
    {0, &gTalkSelects_18_2_0_0},
};
TalkSel gTalkSelects_18_2[1] = {
    {0, gTalkSelects_18_2_0},
};
TalkSel gTalkSelects_18_1_0[1] = {
    {0, &gTalkSelects_18_1_0_0},
};
TalkSel gTalkSelects_18_1[1] = {
    {0, gTalkSelects_18_1_0},
};
TalkSel gTalkSelects_18_0_0[1] = {
    {0, &gTalkSelects_18_0_0_0},
};
TalkSel gTalkSelects_18_0[1] = {
    {0, gTalkSelects_18_0_0},
};
TalkSel gTalkSelects_18[4] = {
#include "lobby/00812E50.inc"
};
TalkLine gTalkLines_18[44] = {
#include "lobby/00812E70.inc"
};
TalkGift gTalkGifts_18[2] = {
#include "lobby/00812F20.inc"
};
TalkEvent gTalkEvents_19[8] = {
#include "lobby/00812F30.inc"
};
TalkSelLines gTalkSelects_19_0_0_0 = {
#include "lobby/00812F50.inc"
};
TalkSelLines gTalkSelects_19_1_0_0 = {
#include "lobby/00812F58.inc"
};
TalkSelLines gTalkSelects_19_2_0_0 = {
#include "lobby/00812F60.inc"
};
TalkSel gTalkSelects_19_2_0[1] = {
    {0, &gTalkSelects_19_2_0_0},
};
TalkSel gTalkSelects_19_2[1] = {
    {0, gTalkSelects_19_2_0},
};
TalkSel gTalkSelects_19_1_0[1] = {
    {0, &gTalkSelects_19_1_0_0},
};
TalkSel gTalkSelects_19_1[1] = {
    {0, gTalkSelects_19_1_0},
};
TalkSel gTalkSelects_19_0_0[1] = {
    {0, &gTalkSelects_19_0_0_0},
};
TalkSel gTalkSelects_19_0[2] = {
    {0, gTalkSelects_19_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_19[4] = {
#include "lobby/00812FA0.inc"
};
TalkLine gTalkLines_19[48] = {
#include "lobby/00812FC0.inc"
};
TalkEvent gTalkEvents_21[6] = {
#include "lobby/00813080.inc"
};
TalkSelLines gTalkSelects_21_0_0_0 = {
#include "lobby/00813098.inc"
};
TalkSelLines gTalkSelects_21_0_0_1 = {
#include "lobby/008130A0.inc"
};
TalkSelLines gTalkSelects_21_0_1_0 = {
#include "lobby/008130A8.inc"
};
TalkSelLines gTalkSelects_21_0_1_1 = {
#include "lobby/008130B0.inc"
};
TalkSelLines gTalkSelects_21_0_2_0 = {
#include "lobby/008130B8.inc"
};
TalkSelLines gTalkSelects_21_0_2_1 = {
#include "lobby/008130C0.inc"
};
TalkSel gTalkSelects_21_0_2[2] = {
#include "lobby/008130D0.inc"
};
TalkSel gTalkSelects_21_0_1[2] = {
#include "lobby/008130E0.inc"
};
TalkSel gTalkSelects_21_0_0[2] = {
#include "lobby/008130F0.inc"
};
TalkSel gTalkSelects_21_0[3] = {
#include "lobby/00813100.inc"
};
TalkSel gTalkSelects_21[1] = {
    {0, gTalkSelects_21_0},
};
TalkLine gTalkLines_21[66] = {
#include "lobby/00813120.inc"
};
TalkEvent gTalkEvents_22[2] = {
#include "lobby/00813228.inc"
};
TalkSelLines gTalkSelects_22_0_0_0 = {
#include "lobby/00813230.inc"
};
TalkSelLines gTalkSelects_22_0_0_1 = {
#include "lobby/00813238.inc"
};
TalkSelLines gTalkSelects_22_0_1_0 = {
#include "lobby/00813240.inc"
};
TalkSelLines gTalkSelects_22_0_1_1 = {
#include "lobby/00813248.inc"
};
TalkSelLines gTalkSelects_22_0_2_0 = {
#include "lobby/00813250.inc"
};
TalkSelLines gTalkSelects_22_0_2_1 = {
#include "lobby/00813258.inc"
};
TalkSel gTalkSelects_22_0_2[2] = {
#include "lobby/00813260.inc"
};
TalkSel gTalkSelects_22_0_1[2] = {
#include "lobby/00813270.inc"
};
TalkSel gTalkSelects_22_0_0[2] = {
#include "lobby/00813280.inc"
};
TalkSel gTalkSelects_22_0[3] = {
#include "lobby/00813290.inc"
};
TalkSel gTalkSelects_22[1] = {
    {0, gTalkSelects_22_0},
};
TalkLine gTalkLines_22[32] = {
#include "lobby/008132B0.inc"
};
TalkEvent gTalkEvents_23[2] = {
#include "lobby/00813330.inc"
};
TalkSelLines gTalkSelects_23_0_0_0 = {
#include "lobby/00813338.inc"
};
TalkSelLines gTalkSelects_23_0_0_1 = {
#include "lobby/00813340.inc"
};
TalkSelLines gTalkSelects_23_0_1_0 = {
#include "lobby/00813348.inc"
};
TalkSelLines gTalkSelects_23_0_1_1 = {
#include "lobby/00813350.inc"
};
TalkSelLines gTalkSelects_23_0_2_0 = {
#include "lobby/00813358.inc"
};
TalkSelLines gTalkSelects_23_0_2_1 = {
#include "lobby/00813360.inc"
};
TalkSel gTalkSelects_23_0_2[2] = {
#include "lobby/00813370.inc"
};
TalkSel gTalkSelects_23_0_1[2] = {
#include "lobby/00813380.inc"
};
TalkSel gTalkSelects_23_0_0[2] = {
#include "lobby/00813390.inc"
};
TalkSel gTalkSelects_23_0[3] = {
#include "lobby/008133A0.inc"
};
TalkSel gTalkSelects_23[1] = {
    {0, gTalkSelects_23_0},
};
TalkLine gTalkLines_23[42] = {
#include "lobby/008133C0.inc"
};
TalkEvent gTalkEvents_24[3] = {
#include "lobby/00813468.inc"
};
TalkSelLines gTalkSelects_24_0_0_0 = {
#include "lobby/00813478.inc"
};
TalkSelLines gTalkSelects_24_0_0_1 = {
#include "lobby/00813480.inc"
};
TalkSelLines gTalkSelects_24_0_1_0 = {
#include "lobby/00813488.inc"
};
TalkSelLines gTalkSelects_24_0_1_1 = {
#include "lobby/00813490.inc"
};
TalkSelLines gTalkSelects_24_0_2_0 = {
#include "lobby/00813498.inc"
};
TalkSelLines gTalkSelects_24_0_2_1 = {
#include "lobby/008134A0.inc"
};
TalkSelLines gTalkSelects_24_1_0_0 = {
#include "lobby/008134A8.inc"
};
TalkSelLines gTalkSelects_24_2_0_0 = {
#include "lobby/008134B0.inc"
};
TalkSelLines gTalkSelects_24_2_0_1 = {
#include "lobby/008134B8.inc"
};
TalkSelLines gTalkSelects_24_3_0_0 = {
#include "lobby/008134C0.inc"
};
TalkSelLines gTalkSelects_24_3_1_0 = {
#include "lobby/008134C8.inc"
};
TalkSelLines gTalkSelects_24_3_2_0 = {
#include "lobby/008134D0.inc"
};
TalkSelLines gTalkSelects_24_4_0_0 = {
#include "lobby/008134D8.inc"
};
TalkSelLines gTalkSelects_24_4_0_1 = {
#include "lobby/008134E0.inc"
};
TalkSelLines gTalkSelects_24_4_1_0 = {
#include "lobby/008134E8.inc"
};
TalkSelLines gTalkSelects_24_4_1_1 = {
#include "lobby/008134F0.inc"
};
TalkSelLines gTalkSelects_24_4_2_0 = {
#include "lobby/008134F8.inc"
};
TalkSelLines gTalkSelects_24_4_2_1 = {
#include "lobby/00813500.inc"
};
TalkSelLines gTalkSelects_24_5_0_0 = {
#include "lobby/00813508.inc"
};
TalkSelLines gTalkSelects_24_5_0_1 = {
#include "lobby/00813510.inc"
};
TalkSelLines gTalkSelects_24_5_1_0 = {
#include "lobby/00813518.inc"
};
TalkSelLines gTalkSelects_24_5_1_1 = {
#include "lobby/00813520.inc"
};
TalkSelLines gTalkSelects_24_5_2_0 = {
#include "lobby/00813528.inc"
};
TalkSelLines gTalkSelects_24_5_2_1 = {
#include "lobby/00813530.inc"
};
TalkSel gTalkSelects_24_5_2[2] = {
#include "lobby/00813540.inc"
};
TalkSel gTalkSelects_24_5_1[2] = {
#include "lobby/00813550.inc"
};
TalkSel gTalkSelects_24_5_0[2] = {
#include "lobby/00813560.inc"
};
TalkSel gTalkSelects_24_5[4] = {
#include "lobby/00813570.inc"
};
TalkSel gTalkSelects_24_4_2[2] = {
#include "lobby/00813590.inc"
};
TalkSel gTalkSelects_24_4_1[2] = {
#include "lobby/008135A0.inc"
};
TalkSel gTalkSelects_24_4_0[2] = {
#include "lobby/008135B0.inc"
};
TalkSel gTalkSelects_24_4[3] = {
#include "lobby/008135C0.inc"
};
TalkSel gTalkSelects_24_3_2[1] = {
    {0, &gTalkSelects_24_3_2_0},
};
TalkSel gTalkSelects_24_3_1[1] = {
    {0, &gTalkSelects_24_3_1_0},
};
TalkSel gTalkSelects_24_3_0[1] = {
    {0, &gTalkSelects_24_3_0_0},
};
TalkSel gTalkSelects_24_3[4] = {
#include "lobby/008135F0.inc"
};
TalkSel gTalkSelects_24_2_0[2] = {
#include "lobby/00813610.inc"
};
TalkSel gTalkSelects_24_2[1] = {
    {0, gTalkSelects_24_2_0},
};
TalkSel gTalkSelects_24_1_0[1] = {
    {0, &gTalkSelects_24_1_0_0},
};
TalkSel gTalkSelects_24_1[2] = {
    {0, gTalkSelects_24_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_24_0_2[2] = {
#include "lobby/00813640.inc"
};
TalkSel gTalkSelects_24_0_1[2] = {
#include "lobby/00813650.inc"
};
TalkSel gTalkSelects_24_0_0[2] = {
#include "lobby/00813660.inc"
};
TalkSel gTalkSelects_24_0[4] = {
#include "lobby/00813670.inc"
};
TalkSel gTalkSelects_24[6] = {
#include "lobby/00813690.inc"
};
TalkLine gTalkLines_24[176] = {
#include "lobby/008136C0.inc"
};
TalkGift gTalkGifts_24[2] = {
#include "lobby/00813980.inc"
};
TalkEvent gTalkEvents_25[4] = {
#include "lobby/00813990.inc"
};
TalkSelLines gTalkSelects_25_0_0_0 = {
#include "lobby/008139A0.inc"
};
TalkSelLines gTalkSelects_25_0_0_1 = {
#include "lobby/008139A8.inc"
};
TalkSelLines gTalkSelects_25_1_0_0 = {
#include "lobby/008139B0.inc"
};
TalkSelLines gTalkSelects_25_1_0_1 = {
#include "lobby/008139B8.inc"
};
TalkSelLines gTalkSelects_25_2_0_0 = {
#include "lobby/008139C0.inc"
};
TalkSelLines gTalkSelects_25_3_0_0 = {
#include "lobby/008139C8.inc"
};
TalkSelLines gTalkSelects_25_3_0_1 = {
#include "lobby/008139D0.inc"
};
TalkSelLines gTalkSelects_25_4_0_0 = {
#include "lobby/008139D8.inc"
};
TalkSel gTalkSelects_25_4_0[1] = {
    {0, &gTalkSelects_25_4_0_0},
};
TalkSel gTalkSelects_25_4[1] = {
    {0, gTalkSelects_25_4_0},
};
TalkSel gTalkSelects_25_3_0[2] = {
#include "lobby/008139F0.inc"
};
TalkSel gTalkSelects_25_3[1] = {
    {0, gTalkSelects_25_3_0},
};
TalkSel gTalkSelects_25_2_0[1] = {
    {0, &gTalkSelects_25_2_0_0},
};
TalkSel gTalkSelects_25_2[2] = {
    {0, gTalkSelects_25_2_0},
    {0, NULL},
};
TalkSel gTalkSelects_25_1_0[2] = {
#include "lobby/00813A20.inc"
};
TalkSel gTalkSelects_25_1[2] = {
    {0, gTalkSelects_25_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_25_0_0[2] = {
#include "lobby/00813A40.inc"
};
TalkSel gTalkSelects_25_0[2] = {
    {0, gTalkSelects_25_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_25[6] = {
#include "lobby/00813A60.inc"
};
TalkLine gTalkLines_25[60] = {
#include "lobby/00813A90.inc"
};
TalkGift gTalkGifts_25[2] = {
#include "lobby/00813B80.inc"
};
TalkEvent gTalkEvents_26[6] = {
#include "lobby/00813B90.inc"
};
TalkSelLines gTalkSelects_26_0_0_0 = {
#include "lobby/00813BA8.inc"
};
TalkSelLines gTalkSelects_26_0_0_1 = {
#include "lobby/00813BB0.inc"
};
TalkSelLines gTalkSelects_26_1_0_0 = {
#include "lobby/00813BB8.inc"
};
TalkSelLines gTalkSelects_26_2_0_0 = {
#include "lobby/00813BC0.inc"
};
TalkSelLines gTalkSelects_26_2_0_1 = {
#include "lobby/00813BC8.inc"
};
TalkSelLines gTalkSelects_26_3_0_0 = {
#include "lobby/00813BD0.inc"
};
TalkSelLines gTalkSelects_26_4_0_0 = {
#include "lobby/00813BD8.inc"
};
TalkSelLines gTalkSelects_26_4_0_1 = {
#include "lobby/00813BE0.inc"
};
TalkSelLines gTalkSelects_26_5_0_0 = {
#include "lobby/00813BE8.inc"
};
TalkSelLines gTalkSelects_26_5_1_0 = {
#include "lobby/00813BF0.inc"
};
TalkSelLines gTalkSelects_26_5_2_0 = {
#include "lobby/00813BF8.inc"
};
TalkSelLines gTalkSelects_26_6_0_0 = {
#include "lobby/00813C00.inc"
};
TalkSelLines gTalkSelects_26_6_0_1 = {
#include "lobby/00813C08.inc"
};
TalkSel gTalkSelects_26_6_0[2] = {
#include "lobby/00813C10.inc"
};
TalkSel gTalkSelects_26_6[1] = {
    {0, gTalkSelects_26_6_0},
};
TalkSel gTalkSelects_26_5_2[1] = {
    {0, &gTalkSelects_26_5_2_0},
};
TalkSel gTalkSelects_26_5_1[1] = {
    {0, &gTalkSelects_26_5_1_0},
};
TalkSel gTalkSelects_26_5_0[1] = {
    {0, &gTalkSelects_26_5_0_0},
};
TalkSel gTalkSelects_26_5[4] = {
#include "lobby/00813C40.inc"
};
TalkSel gTalkSelects_26_4_0[2] = {
#include "lobby/00813C60.inc"
};
TalkSel gTalkSelects_26_4[1] = {
    {0, gTalkSelects_26_4_0},
};
TalkSel gTalkSelects_26_3_0[1] = {
    {0, &gTalkSelects_26_3_0_0},
};
TalkSel gTalkSelects_26_3[2] = {
    {0, gTalkSelects_26_3_0},
    {0, NULL},
};
TalkSel gTalkSelects_26_2_0[2] = {
#include "lobby/00813C90.inc"
};
TalkSel gTalkSelects_26_2[1] = {
    {0, gTalkSelects_26_2_0},
};
TalkSel gTalkSelects_26_1_0[1] = {
    {0, &gTalkSelects_26_1_0_0},
};
TalkSel gTalkSelects_26_1[2] = {
    {0, gTalkSelects_26_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_26_0_0[2] = {
#include "lobby/00813CC0.inc"
};
TalkSel gTalkSelects_26_0[2] = {
    {0, gTalkSelects_26_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_26[8] = {
#include "lobby/00813CE0.inc"
};
TalkLine gTalkLines_26[92] = {
#include "lobby/00813D20.inc"
};
TalkGift gTalkGifts_26[2] = {
#include "lobby/00813E90.inc"
};
TalkEvent gTalkEvents_27[2] = {
#include "lobby/00813EA0.inc"
};
TalkSelLines gTalkSelects_27_0_0_0 = {
#include "lobby/00813EA8.inc"
};
TalkSelLines gTalkSelects_27_0_0_1 = {
#include "lobby/00813EB0.inc"
};
TalkSelLines gTalkSelects_27_0_1_0 = {
#include "lobby/00813EB8.inc"
};
TalkSelLines gTalkSelects_27_0_1_1 = {
#include "lobby/00813EC0.inc"
};
TalkSelLines gTalkSelects_27_0_2_0 = {
#include "lobby/00813EC8.inc"
};
TalkSelLines gTalkSelects_27_0_2_1 = {
#include "lobby/00813ED0.inc"
};
TalkSel gTalkSelects_27_0_2[2] = {
#include "lobby/00813EE0.inc"
};
TalkSel gTalkSelects_27_0_1[2] = {
#include "lobby/00813EF0.inc"
};
TalkSel gTalkSelects_27_0_0[2] = {
#include "lobby/00813F00.inc"
};
TalkSel gTalkSelects_27_0[3] = {
#include "lobby/00813F10.inc"
};
TalkSel gTalkSelects_27[1] = {
    {0, gTalkSelects_27_0},
};
TalkLine gTalkLines_27[42] = {
#include "lobby/00813F30.inc"
};
TalkEvent gTalkEvents_28[3] = {
#include "lobby/00813FD8.inc"
};
TalkSelLines gTalkSelects_28_0_0_0 = {
#include "lobby/00813FE8.inc"
};
TalkSelLines gTalkSelects_28_1_0_0 = {
#include "lobby/00813FF0.inc"
};
TalkSelLines gTalkSelects_28_2_0_0 = {
#include "lobby/00813FF8.inc"
};
TalkSelLines gTalkSelects_28_2_0_1 = {
#include "lobby/00814000.inc"
};
TalkSel gTalkSelects_28_2_0[2] = {
#include "lobby/00814010.inc"
};
TalkSel gTalkSelects_28_2[1] = {
    {0, gTalkSelects_28_2_0},
};
TalkSel gTalkSelects_28_1_0[1] = {
    {0, &gTalkSelects_28_1_0_0},
};
TalkSel gTalkSelects_28_1[1] = {
    {0, gTalkSelects_28_1_0},
};
TalkSel gTalkSelects_28_0_0[1] = {
    {0, &gTalkSelects_28_0_0_0},
};
TalkSel gTalkSelects_28_0[2] = {
    {0, gTalkSelects_28_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_28[4] = {
#include "lobby/00814050.inc"
};
TalkLine gTalkLines_28[174] = {
#include "lobby/00814070.inc"
};
TalkEvent gTalkEvents_29[2] = {
#include "lobby/00814328.inc"
};
TalkSel gTalkSelects_29_0_0[1] = {
    {0, &gTalkSelects_29_0_0_0},
};
TalkSel gTalkSelects_29_0[1] = {
    {0, gTalkSelects_29_0_0},
};
TalkSel gTalkSelects_29[1] = {
    {0, gTalkSelects_29_0},
};
TalkLine gTalkLines_29[2] = {
#include "lobby/00814348.inc"
};
TalkEvent gTalkEvents_30[6] = {
#include "lobby/00814350.inc"
};
TalkSelLines gTalkSelects_30_0_0_0 = {
#include "lobby/00814368.inc"
};
TalkSelLines gTalkSelects_30_1_0_0 = {
#include "lobby/00814370.inc"
};
TalkSelLines gTalkSelects_30_2_0_0 = {
#include "lobby/00814378.inc"
};
TalkSelLines gTalkSelects_30_3_0_0 = {
#include "lobby/00814380.inc"
};
TalkSel gTalkSelects_30_3_0[1] = {
    {0, &gTalkSelects_30_3_0_0},
};
TalkSel gTalkSelects_30_3[1] = {
    {0, gTalkSelects_30_3_0},
};
TalkSel gTalkSelects_30_2_0[1] = {
    {0, &gTalkSelects_30_2_0_0},
};
TalkSel gTalkSelects_30_2[1] = {
    {0, gTalkSelects_30_2_0},
};
TalkSel gTalkSelects_30_1_0[1] = {
    {0, &gTalkSelects_30_1_0_0},
};
TalkSel gTalkSelects_30_1[1] = {
    {0, gTalkSelects_30_1_0},
};
TalkSel gTalkSelects_30_0_0[1] = {
    {0, &gTalkSelects_30_0_0_0},
};
TalkSel gTalkSelects_30_0[2] = {
    {0, gTalkSelects_30_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_30[4] = {
#include "lobby/008143D0.inc"
};
TalkLine gTalkLines_30[36] = {
#include "lobby/008143F0.inc"
};
TalkEvent gTalkEvents_31[2] = {
#include "lobby/00814480.inc"
};
TalkSelLines gTalkSelects_31_0_0_0 = {
#include "lobby/00814488.inc"
};
TalkSelLines gTalkSelects_31_1_0_0 = {
#include "lobby/00814490.inc"
};
TalkSelLines gTalkSelects_31_2_0_0 = {
#include "lobby/00814498.inc"
};
TalkSelLines gTalkSelects_31_2_1_0 = {
#include "lobby/008144A0.inc"
};
TalkSelLines gTalkSelects_31_2_2_0 = {
#include "lobby/008144A8.inc"
};
TalkSel gTalkSelects_31_2_2[1] = {
    {0, &gTalkSelects_31_2_2_0},
};
TalkSel gTalkSelects_31_2_1[1] = {
    {0, &gTalkSelects_31_2_1_0},
};
TalkSel gTalkSelects_31_2_0[2] = {
    {0, &gTalkSelects_31_2_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_31_2[3] = {
#include "lobby/008144D0.inc"
};
TalkSel gTalkSelects_31_1_0[1] = {
    {0, &gTalkSelects_31_1_0_0},
};
TalkSel gTalkSelects_31_1[1] = {
    {0, gTalkSelects_31_1_0},
};
TalkSel gTalkSelects_31_0_0[1] = {
    {0, &gTalkSelects_31_0_0_0},
};
TalkSel gTalkSelects_31_0[2] = {
    {0, gTalkSelects_31_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_31[4] = {
#include "lobby/00814510.inc"
};
TalkLine gTalkLines_31[30] = {
#include "lobby/00814530.inc"
};
TalkEvent gTalkEvents_32[2] = {
#include "lobby/008145A8.inc"
};
TalkSelLines gTalkSelects_32_0_0_0 = {
#include "lobby/008145B0.inc"
};
TalkSelLines gTalkSelects_32_1_0_0 = {
#include "lobby/008145B8.inc"
};
TalkSel gTalkSelects_32_2_0[1] = {
    {0, &gTalkSelects_32_2_0_0},
};
TalkSel gTalkSelects_32_2[1] = {
    {0, gTalkSelects_32_2_0},
};
TalkSel gTalkSelects_32_1_0[1] = {
    {0, &gTalkSelects_32_1_0_0},
};
TalkSel gTalkSelects_32_1[1] = {
    {0, gTalkSelects_32_1_0},
};
TalkSel gTalkSelects_32_0_0[1] = {
    {0, &gTalkSelects_32_0_0_0},
};
TalkSel gTalkSelects_32_0[1] = {
    {0, gTalkSelects_32_0_0},
};
TalkSel gTalkSelects_32[4] = {
#include "lobby/008145F0.inc"
};
TalkLine gTalkLines_32[24] = {
#include "lobby/00814610.inc"
};
TalkEvent gTalkEvents_33[2] = {
#include "lobby/00814670.inc"
};
TalkSelLines gTalkSelects_33_0_0_0 = {
#include "lobby/00814678.inc"
};
TalkSelLines gTalkSelects_33_1_0_0 = {
#include "lobby/00814680.inc"
};
TalkSelLines gTalkSelects_33_2_0_0 = {
#include "lobby/00814688.inc"
};
TalkSel gTalkSelects_33_2_0[1] = {
    {0, &gTalkSelects_33_2_0_0},
};
TalkSel gTalkSelects_33_2[1] = {
    {0, gTalkSelects_33_2_0},
};
TalkSel gTalkSelects_33_1_0[1] = {
    {0, &gTalkSelects_33_1_0_0},
};
TalkSel gTalkSelects_33_1[1] = {
    {0, gTalkSelects_33_1_0},
};
TalkSel gTalkSelects_33_0_0[1] = {
    {0, &gTalkSelects_33_0_0_0},
};
TalkSel gTalkSelects_33_0[1] = {
    {0, gTalkSelects_33_0_0},
};
TalkSel gTalkSelects_33[4] = {
#include "lobby/008146C0.inc"
};
TalkLine gTalkLines_33[18] = {
#include "lobby/008146E0.inc"
};
TalkEvent gTalkEvents_34[2] = {
#include "lobby/00814728.inc"
};
TalkSel gTalkSelects_34_0_0[1] = {
    {0, &gTalkSelects_34_0_0_0},
};
TalkSel gTalkSelects_34_0[1] = {
    {0, gTalkSelects_34_0_0},
};
TalkSel gTalkSelects_34[1] = {
    {0, gTalkSelects_34_0},
};
TalkLine gTalkLines_34[2] = {
#include "lobby/00814748.inc"
};
TalkEvent gTalkEvents_35[4] = {
#include "lobby/00814750.inc"
};
TalkSelLines gTalkSelects_35_0_0_0 = {
#include "lobby/00814760.inc"
};
TalkSelLines gTalkSelects_35_1_0_0 = {
#include "lobby/00814768.inc"
};
TalkSel gTalkSelects_35_2_0[1] = {
    {0, &gTalkSelects_35_2_0_0},
};
TalkSel gTalkSelects_35_2[1] = {
    {0, gTalkSelects_35_2_0},
};
TalkSel gTalkSelects_35_1_0[1] = {
    {0, &gTalkSelects_35_1_0_0},
};
TalkSel gTalkSelects_35_1[1] = {
    {0, gTalkSelects_35_1_0},
};
TalkSel gTalkSelects_35_0_0[1] = {
    {0, &gTalkSelects_35_0_0_0},
};
TalkSel gTalkSelects_35_0[1] = {
    {0, gTalkSelects_35_0_0},
};
TalkSel gTalkSelects_35[4] = {
#include "lobby/008147A0.inc"
};
TalkLine gTalkLines_35[18] = {
#include "lobby/008147C0.inc"
};
TalkEvent gTalkEvents_36[2] = {
#include "lobby/00814808.inc"
};
TalkSelLines gTalkSelects_36_0_0_0 = {
#include "lobby/00814810.inc"
};
TalkSelLines gTalkSelects_36_1_0_0 = {
#include "lobby/00814818.inc"
};
TalkSelLines gTalkSelects_36_2_0_0 = {
#include "lobby/00814820.inc"
};
TalkSel gTalkSelects_36_2_0[2] = {
#include "lobby/00814830.inc"
};
TalkSel gTalkSelects_36_2[1] = {
    {0, gTalkSelects_36_2_0},
};
TalkSel gTalkSelects_36_1_0[1] = {
    {0, &gTalkSelects_36_1_0_0},
};
TalkSel gTalkSelects_36_1[1] = {
    {0, gTalkSelects_36_1_0},
};
TalkSel gTalkSelects_36_0_0[1] = {
    {0, &gTalkSelects_36_0_0_0},
};
TalkSel gTalkSelects_36_0[2] = {
    {0, gTalkSelects_36_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_36[4] = {
#include "lobby/00814870.inc"
};
TalkLine gTalkLines_36[20] = {
#include "lobby/00814890.inc"
};
TalkEvent gTalkEvents_37[2] = {
#include "lobby/008148E0.inc"
};
TalkSelLines gTalkSelects_37_0_0_0 = {
#include "lobby/008148E8.inc"
};
TalkSelLines gTalkSelects_37_1_0_0 = {
#include "lobby/008148F0.inc"
};
TalkSelLines gTalkSelects_37_2_0_0 = {
#include "lobby/008148F8.inc"
};
TalkSel gTalkSelects_37_2_0[1] = {
    {0, &gTalkSelects_37_2_0_0},
};
TalkSel gTalkSelects_37_2[1] = {
    {0, gTalkSelects_37_2_0},
};
TalkSel gTalkSelects_37_1_0[1] = {
    {0, &gTalkSelects_37_1_0_0},
};
TalkSel gTalkSelects_37_1[1] = {
    {0, gTalkSelects_37_1_0},
};
TalkSel gTalkSelects_37_0_0[1] = {
    {0, &gTalkSelects_37_0_0_0},
};
TalkSel gTalkSelects_37_0[1] = {
    {0, gTalkSelects_37_0_0},
};
TalkSel gTalkSelects_37[4] = {
#include "lobby/00814930.inc"
};
TalkLine gTalkLines_37[20] = {
#include "lobby/00814950.inc"
};
TalkEvent gTalkEvents_38[2] = {
#include "lobby/008149A0.inc"
};
TalkSelLines gTalkSelects_38_0_0_0 = {
#include "lobby/008149A8.inc"
};
TalkSelLines gTalkSelects_38_1_0_0 = {
#include "lobby/008149B0.inc"
};
TalkSelLines gTalkSelects_38_2_0_0 = {
#include "lobby/008149B8.inc"
};
TalkSel gTalkSelects_38_2_0[1] = {
    {0, &gTalkSelects_38_2_0_0},
};
TalkSel gTalkSelects_38_2[1] = {
    {0, gTalkSelects_38_2_0},
};
TalkSel gTalkSelects_38_1_0[1] = {
    {0, &gTalkSelects_38_1_0_0},
};
TalkSel gTalkSelects_38_1[1] = {
    {0, gTalkSelects_38_1_0},
};
TalkSel gTalkSelects_38_0_0[1] = {
    {0, &gTalkSelects_38_0_0_0},
};
TalkSel gTalkSelects_38_0[1] = {
    {0, gTalkSelects_38_0_0},
};
TalkSel gTalkSelects_38[4] = {
#include "lobby/008149F0.inc"
};
TalkLine gTalkLines_38[20] = {
#include "lobby/00814A10.inc"
};
TalkEvent gTalkEvents_39[2] = {
#include "lobby/00814A60.inc"
};
TalkSelLines gTalkSelects_39_0_0_0 = {
#include "lobby/00814A68.inc"
};
TalkSelLines gTalkSelects_39_1_0_0 = {
#include "lobby/00814A70.inc"
};
TalkSelLines gTalkSelects_39_2_0_0 = {
#include "lobby/00814A78.inc"
};
TalkSel gTalkSelects_39_2_0[2] = {
#include "lobby/00814A80.inc"
};
TalkSel gTalkSelects_39_2[1] = {
    {0, gTalkSelects_39_2_0},
};
TalkSel gTalkSelects_39_1_0[1] = {
    {0, &gTalkSelects_39_1_0_0},
};
TalkSel gTalkSelects_39_1[1] = {
    {0, gTalkSelects_39_1_0},
};
TalkSel gTalkSelects_39_0_0[1] = {
    {0, &gTalkSelects_39_0_0_0},
};
TalkSel gTalkSelects_39_0[2] = {
    {0, gTalkSelects_39_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_39[4] = {
#include "lobby/00814AC0.inc"
};
TalkLine gTalkLines_39[24] = {
#include "lobby/00814AE0.inc"
};
TalkEvent gTalkEvents_40[6] = {
#include "lobby/00814B40.inc"
};
TalkSelLines gTalkSelects_40_0_0_0 = {
#include "lobby/00814B58.inc"
};
TalkSelLines gTalkSelects_40_1_0_0 = {
#include "lobby/00814B60.inc"
};
TalkSelLines gTalkSelects_40_2_0_0 = {
#include "lobby/00814B68.inc"
};
TalkSel gTalkSelects_40_2_0[1] = {
    {0, &gTalkSelects_40_2_0_0},
};
TalkSel gTalkSelects_40_2[1] = {
    {0, gTalkSelects_40_2_0},
};
TalkSel gTalkSelects_40_1_0[1] = {
    {0, &gTalkSelects_40_1_0_0},
};
TalkSel gTalkSelects_40_1[1] = {
    {0, gTalkSelects_40_1_0},
};
TalkSel gTalkSelects_40_0_0[1] = {
    {0, &gTalkSelects_40_0_0_0},
};
TalkSel gTalkSelects_40_0[1] = {
    {0, gTalkSelects_40_0_0},
};
TalkSel gTalkSelects_40[4] = {
#include "lobby/00814BA0.inc"
};
TalkLine gTalkLines_40[20] = {
#include "lobby/00814BC0.inc"
};
TalkEvent gTalkEvents_41[2] = {
#include "lobby/00814C10.inc"
};
TalkSelLines gTalkSelects_41_0_0_0 = {
#include "lobby/00814C18.inc"
};
TalkSelLines gTalkSelects_41_1_0_0 = {
#include "lobby/00814C20.inc"
};
TalkSelLines gTalkSelects_41_2_0_0 = {
#include "lobby/00814C28.inc"
};
TalkSel gTalkSelects_41_2_0[1] = {
    {0, &gTalkSelects_41_2_0_0},
};
TalkSel gTalkSelects_41_2[1] = {
    {0, gTalkSelects_41_2_0},
};
TalkSel gTalkSelects_41_1_0[1] = {
    {0, &gTalkSelects_41_1_0_0},
};
TalkSel gTalkSelects_41_1[1] = {
    {0, gTalkSelects_41_1_0},
};
TalkSel gTalkSelects_41_0_0[1] = {
    {0, &gTalkSelects_41_0_0_0},
};
TalkSel gTalkSelects_41_0[1] = {
    {0, gTalkSelects_41_0_0},
};
TalkSel gTalkSelects_41[4] = {
#include "lobby/00814C60.inc"
};
TalkLine gTalkLines_41[24] = {
#include "lobby/00814C80.inc"
};
TalkEvent gTalkEvents_42[2] = {
#include "lobby/00814CE0.inc"
};
TalkSelLines gTalkSelects_42_0_0_0 = {
#include "lobby/00814CE8.inc"
};
TalkSelLines gTalkSelects_42_1_0_0 = {
#include "lobby/00814CF0.inc"
};
TalkSelLines gTalkSelects_42_1_1_0 = {
#include "lobby/00814CF8.inc"
};
TalkSelLines gTalkSelects_42_1_2_0 = {
#include "lobby/00814D00.inc"
};
TalkSelLines gTalkSelects_42_2_0_0 = {
#include "lobby/00814D08.inc"
};
TalkSelLines gTalkSelects_42_2_1_0 = {
#include "lobby/00814D10.inc"
};
TalkSelLines gTalkSelects_42_2_2_0 = {
#include "lobby/00814D18.inc"
};
TalkSel gTalkSelects_42_2_2[1] = {
    {0, &gTalkSelects_42_2_2_0},
};
TalkSel gTalkSelects_42_2_1[1] = {
    {0, &gTalkSelects_42_2_1_0},
};
TalkSel gTalkSelects_42_2_0[2] = {
    {0, &gTalkSelects_42_2_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_42_2[3] = {
#include "lobby/00814D40.inc"
};
TalkSel gTalkSelects_42_1_2[1] = {
    {0, &gTalkSelects_42_1_2_0},
};
TalkSel gTalkSelects_42_1_1[1] = {
    {0, &gTalkSelects_42_1_1_0},
};
TalkSel gTalkSelects_42_1_0[1] = {
    {0, &gTalkSelects_42_1_0_0},
};
TalkSel gTalkSelects_42_1[3] = {
#include "lobby/00814D70.inc"
};
TalkSel gTalkSelects_42_0_0[1] = {
    {0, &gTalkSelects_42_0_0_0},
};
TalkSel gTalkSelects_42_0[2] = {
    {0, gTalkSelects_42_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_42[4] = {
#include "lobby/00814DA0.inc"
};
TalkLine gTalkLines_42[40] = {
#include "lobby/00814DC0.inc"
};
TalkEvent gTalkEvents_43[2] = {
#include "lobby/00814E60.inc"
};
TalkSelLines gTalkSelects_43_0_0_0 = {
#include "lobby/00814E68.inc"
};
TalkSelLines gTalkSelects_43_1_0_0 = {
#include "lobby/00814E70.inc"
};
TalkSel gTalkSelects_43_2_0[1] = {
    {0, &gTalkSelects_43_2_0_0},
};
TalkSel gTalkSelects_43_2[1] = {
    {0, gTalkSelects_43_2_0},
};
TalkSel gTalkSelects_43_1_0[1] = {
    {0, &gTalkSelects_43_1_0_0},
};
TalkSel gTalkSelects_43_1[1] = {
    {0, gTalkSelects_43_1_0},
};
TalkSel gTalkSelects_43_0_0[1] = {
    {0, &gTalkSelects_43_0_0_0},
};
TalkSel gTalkSelects_43_0[2] = {
    {0, gTalkSelects_43_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_43[4] = {
#include "lobby/00814EB0.inc"
};
TalkLine gTalkLines_43[16] = {
#include "lobby/00814ED0.inc"
};
TalkEvent gTalkEvents_44[6] = {
#include "lobby/00814F10.inc"
};
TalkSelLines gTalkSelects_44_0_0_0 = {
#include "lobby/00814F28.inc"
};
TalkSelLines gTalkSelects_44_1_0_0 = {
#include "lobby/00814F30.inc"
};
TalkSelLines gTalkSelects_44_2_0_0 = {
#include "lobby/00814F38.inc"
};
TalkSel gTalkSelects_44_2_0[1] = {
    {0, &gTalkSelects_44_2_0_0},
};
TalkSel gTalkSelects_44_2[1] = {
    {0, gTalkSelects_44_2_0},
};
TalkSel gTalkSelects_44_1_0[1] = {
    {0, &gTalkSelects_44_1_0_0},
};
TalkSel gTalkSelects_44_1[1] = {
    {0, gTalkSelects_44_1_0},
};
TalkSel gTalkSelects_44_0_0[1] = {
    {0, &gTalkSelects_44_0_0_0},
};
TalkSel gTalkSelects_44_0[1] = {
    {0, gTalkSelects_44_0_0},
};
TalkSel gTalkSelects_44[4] = {
#include "lobby/00814F70.inc"
};
TalkLine gTalkLines_44[16] = {
#include "lobby/00814F90.inc"
};
TalkGift gTalkGifts_44[4] = {
#include "lobby/00814FD0.inc"
};
TalkEvent gTalkEvents_45[6] = {
#include "lobby/00814FF0.inc"
};
TalkSelLines gTalkSelects_45_0_0_0 = {
#include "lobby/00815008.inc"
};
TalkSelLines gTalkSelects_45_0_0_1 = {
#include "lobby/00815010.inc"
};
TalkSelLines gTalkSelects_45_0_1_0 = {
#include "lobby/00815018.inc"
};
TalkSelLines gTalkSelects_45_0_1_1 = {
#include "lobby/00815020.inc"
};
TalkSelLines gTalkSelects_45_0_2_0 = {
#include "lobby/00815028.inc"
};
TalkSelLines gTalkSelects_45_0_2_1 = {
#include "lobby/00815030.inc"
};
TalkSelLines gTalkSelects_45_1_0_0 = {
#include "lobby/00815038.inc"
};
TalkSelLines gTalkSelects_45_1_0_1 = {
#include "lobby/00815040.inc"
};
TalkSelLines gTalkSelects_45_1_1_0 = {
#include "lobby/00815048.inc"
};
TalkSelLines gTalkSelects_45_1_1_1 = {
#include "lobby/00815050.inc"
};
TalkSelLines gTalkSelects_45_1_2_0 = {
#include "lobby/00815058.inc"
};
TalkSelLines gTalkSelects_45_1_2_1 = {
#include "lobby/00815060.inc"
};
TalkSelLines gTalkSelects_45_2_0_0 = {
#include "lobby/00815068.inc"
};
TalkSelLines gTalkSelects_45_2_0_1 = {
#include "lobby/00815070.inc"
};
TalkSelLines gTalkSelects_45_2_1_0 = {
#include "lobby/00815078.inc"
};
TalkSelLines gTalkSelects_45_2_1_1 = {
#include "lobby/00815080.inc"
};
TalkSelLines gTalkSelects_45_2_2_0 = {
#include "lobby/00815088.inc"
};
TalkSelLines gTalkSelects_45_2_2_1 = {
#include "lobby/00815090.inc"
};
TalkSel gTalkSelects_45_2_2[2] = {
#include "lobby/008150A0.inc"
};
TalkSel gTalkSelects_45_2_1[2] = {
#include "lobby/008150B0.inc"
};
TalkSel gTalkSelects_45_2_0[2] = {
#include "lobby/008150C0.inc"
};
TalkSel gTalkSelects_45_2[4] = {
#include "lobby/008150D0.inc"
};
TalkSel gTalkSelects_45_1_2[2] = {
#include "lobby/008150F0.inc"
};
TalkSel gTalkSelects_45_1_1[2] = {
#include "lobby/00815100.inc"
};
TalkSel gTalkSelects_45_1_0[2] = {
#include "lobby/00815110.inc"
};
TalkSel gTalkSelects_45_1[4] = {
#include "lobby/00815120.inc"
};
TalkSel gTalkSelects_45_0_2[2] = {
#include "lobby/00815140.inc"
};
TalkSel gTalkSelects_45_0_1[2] = {
#include "lobby/00815150.inc"
};
TalkSel gTalkSelects_45_0_0[2] = {
#include "lobby/00815160.inc"
};
TalkSel gTalkSelects_45_0[4] = {
#include "lobby/00815170.inc"
};
TalkSel gTalkSelects_45[4] = {
#include "lobby/00815190.inc"
};
TalkLine gTalkLines_45[28] = {
#include "lobby/008151B0.inc"
};
TalkGift gTalkGifts_45[2] = {
#include "lobby/00815220.inc"
};
TalkEvent gTalkEvents_46[6] = {
#include "lobby/00815230.inc"
};
TalkSelLines gTalkSelects_46_0_0_0 = {
#include "lobby/00815248.inc"
};
TalkSelLines gTalkSelects_46_0_0_1 = {
#include "lobby/00815250.inc"
};
TalkSelLines gTalkSelects_46_1_0_0 = {
#include "lobby/00815258.inc"
};
TalkSelLines gTalkSelects_46_1_0_1 = {
#include "lobby/00815260.inc"
};
TalkSelLines gTalkSelects_46_2_0_0 = {
#include "lobby/00815268.inc"
};
TalkSelLines gTalkSelects_46_2_0_1 = {
#include "lobby/00815270.inc"
};
TalkSel gTalkSelects_46_2_0[2] = {
#include "lobby/00815280.inc"
};
TalkSel gTalkSelects_46_2[2] = {
    {0, gTalkSelects_46_2_0},
    {0, NULL},
};
TalkSel gTalkSelects_46_1_0[2] = {
#include "lobby/008152A0.inc"
};
TalkSel gTalkSelects_46_1[2] = {
    {0, gTalkSelects_46_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_46_0_0[2] = {
#include "lobby/008152C0.inc"
};
TalkSel gTalkSelects_46_0[2] = {
    {0, gTalkSelects_46_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_46[4] = {
#include "lobby/008152E0.inc"
};
TalkLine gTalkLines_46[16] = {
#include "lobby/00815300.inc"
};
TalkGift gTalkGifts_46[2] = {
#include "lobby/00815340.inc"
};
TalkEvent gTalkEvents_47[4] = {
#include "lobby/00815350.inc"
};
TalkSelLines gTalkSelects_47_0_0_0 = {
#include "lobby/00815360.inc"
};
TalkSelLines gTalkSelects_47_1_0_0 = {
#include "lobby/00815368.inc"
};
TalkSelLines gTalkSelects_47_2_0_0 = {
#include "lobby/00815370.inc"
};
TalkSel gTalkSelects_47_2_0[1] = {
    {0, &gTalkSelects_47_2_0_0},
};
TalkSel gTalkSelects_47_2[1] = {
    {0, gTalkSelects_47_2_0},
};
TalkSel gTalkSelects_47_1_0[1] = {
    {0, &gTalkSelects_47_1_0_0},
};
TalkSel gTalkSelects_47_1[1] = {
    {0, gTalkSelects_47_1_0},
};
TalkSel gTalkSelects_47_0_0[1] = {
    {0, &gTalkSelects_47_0_0_0},
};
TalkSel gTalkSelects_47_0[2] = {
    {0, gTalkSelects_47_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_47[4] = {
#include "lobby/008153B0.inc"
};
TalkLine gTalkLines_47[12] = {
#include "lobby/008153D0.inc"
};
TalkEvent gTalkEvents_48[2] = {
#include "lobby/00815400.inc"
};
TalkSelLines gTalkSelects_48_0_0_0 = {
#include "lobby/00815408.inc"
};
TalkSelLines gTalkSelects_48_0_0_1 = {
#include "lobby/00815410.inc"
};
TalkSelLines gTalkSelects_48_0_1_0 = {
#include "lobby/00815418.inc"
};
TalkSelLines gTalkSelects_48_0_1_1 = {
#include "lobby/00815420.inc"
};
TalkSelLines gTalkSelects_48_0_2_0 = {
#include "lobby/00815428.inc"
};
TalkSelLines gTalkSelects_48_0_2_1 = {
#include "lobby/00815430.inc"
};
TalkSelLines gTalkSelects_48_1_0_0 = {
#include "lobby/00815438.inc"
};
TalkSelLines gTalkSelects_48_1_0_1 = {
#include "lobby/00815440.inc"
};
TalkSelLines gTalkSelects_48_1_1_0 = {
#include "lobby/00815448.inc"
};
TalkSelLines gTalkSelects_48_1_1_1 = {
#include "lobby/00815450.inc"
};
TalkSelLines gTalkSelects_48_1_2_0 = {
#include "lobby/00815458.inc"
};
TalkSelLines gTalkSelects_48_1_2_1 = {
#include "lobby/00815460.inc"
};
TalkSelLines gTalkSelects_48_2_0_0 = {
#include "lobby/00815468.inc"
};
TalkSelLines gTalkSelects_48_2_0_1 = {
#include "lobby/00815470.inc"
};
TalkSelLines gTalkSelects_48_2_1_0 = {
#include "lobby/00815478.inc"
};
TalkSelLines gTalkSelects_48_2_1_1 = {
#include "lobby/00815480.inc"
};
TalkSelLines gTalkSelects_48_2_2_0 = {
#include "lobby/00815488.inc"
};
TalkSelLines gTalkSelects_48_2_2_1 = {
#include "lobby/00815490.inc"
};
TalkSel gTalkSelects_48_2_2[2] = {
#include "lobby/008154A0.inc"
};
TalkSel gTalkSelects_48_2_1[2] = {
#include "lobby/008154B0.inc"
};
TalkSel gTalkSelects_48_2_0[2] = {
#include "lobby/008154C0.inc"
};
TalkSel gTalkSelects_48_2[4] = {
#include "lobby/008154D0.inc"
};
TalkSel gTalkSelects_48_1_2[2] = {
#include "lobby/008154F0.inc"
};
TalkSel gTalkSelects_48_1_1[2] = {
#include "lobby/00815500.inc"
};
TalkSel gTalkSelects_48_1_0[2] = {
#include "lobby/00815510.inc"
};
TalkSel gTalkSelects_48_1[4] = {
#include "lobby/00815520.inc"
};
TalkSel gTalkSelects_48_0_2[2] = {
#include "lobby/00815540.inc"
};
TalkSel gTalkSelects_48_0_1[2] = {
#include "lobby/00815550.inc"
};
TalkSel gTalkSelects_48_0_0[2] = {
#include "lobby/00815560.inc"
};
TalkSel gTalkSelects_48_0[4] = {
#include "lobby/00815570.inc"
};
TalkSel gTalkSelects_48[4] = {
#include "lobby/00815590.inc"
};
TalkLine gTalkLines_48[78] = {
#include "lobby/008155B0.inc"
};
TalkEvent gTalkEvents_49[2] = {
#include "lobby/008156E8.inc"
};
TalkSelLines gTalkSelects_49_0_0_0 = {
#include "lobby/008156F0.inc"
};
TalkSelLines gTalkSelects_49_1_0_0 = {
#include "lobby/008156F8.inc"
};
TalkSelLines gTalkSelects_49_2_0_0 = {
#include "lobby/00815700.inc"
};
TalkSel gTalkSelects_49_2_0[1] = {
    {0, &gTalkSelects_49_2_0_0},
};
TalkSel gTalkSelects_49_2[1] = {
    {0, gTalkSelects_49_2_0},
};
TalkSel gTalkSelects_49_1_0[1] = {
    {0, &gTalkSelects_49_1_0_0},
};
TalkSel gTalkSelects_49_1[1] = {
    {0, gTalkSelects_49_1_0},
};
TalkSel gTalkSelects_49_0_0[1] = {
    {0, &gTalkSelects_49_0_0_0},
};
TalkSel gTalkSelects_49_0[2] = {
    {0, gTalkSelects_49_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_49[4] = {
#include "lobby/00815740.inc"
};
TalkLine gTalkLines_49[22] = {
#include "lobby/00815760.inc"
};
TalkEvent gTalkEvents_50[2] = {
#include "lobby/008157B8.inc"
};
TalkSelLines gTalkSelects_50_0_0_0 = {
#include "lobby/008157C0.inc"
};
TalkSelLines gTalkSelects_50_0_0_1 = {
#include "lobby/008157C8.inc"
};
TalkSelLines gTalkSelects_50_1_0_0 = {
#include "lobby/008157D0.inc"
};
TalkSelLines gTalkSelects_50_1_0_1 = {
#include "lobby/008157D8.inc"
};
TalkSelLines gTalkSelects_50_1_1_0 = {
#include "lobby/008157E0.inc"
};
TalkSelLines gTalkSelects_50_1_1_1 = {
#include "lobby/008157E8.inc"
};
TalkSelLines gTalkSelects_50_1_2_0 = {
#include "lobby/008157F0.inc"
};
TalkSelLines gTalkSelects_50_1_2_1 = {
#include "lobby/008157F8.inc"
};
TalkSelLines gTalkSelects_50_2_0_0 = {
#include "lobby/00815800.inc"
};
TalkSelLines gTalkSelects_50_2_0_1 = {
#include "lobby/00815808.inc"
};
TalkSelLines gTalkSelects_50_2_1_0 = {
#include "lobby/00815810.inc"
};
TalkSelLines gTalkSelects_50_2_1_1 = {
#include "lobby/00815818.inc"
};
TalkSelLines gTalkSelects_50_2_2_0 = {
#include "lobby/00815820.inc"
};
TalkSel gTalkSelects_50_2_2[2] = {
#include "lobby/00815830.inc"
};
TalkSel gTalkSelects_50_2_1[2] = {
#include "lobby/00815840.inc"
};
TalkSel gTalkSelects_50_2_0[2] = {
#include "lobby/00815850.inc"
};
TalkSel gTalkSelects_50_2[4] = {
#include "lobby/00815860.inc"
};
TalkSel gTalkSelects_50_1_2[2] = {
#include "lobby/00815880.inc"
};
TalkSel gTalkSelects_50_1_1[2] = {
#include "lobby/00815890.inc"
};
TalkSel gTalkSelects_50_1_0[2] = {
#include "lobby/008158A0.inc"
};
TalkSel gTalkSelects_50_1[4] = {
#include "lobby/008158B0.inc"
};
TalkSel gTalkSelects_50_0_0[2] = {
#include "lobby/008158D0.inc"
};
TalkSel gTalkSelects_50_0[2] = {
    {0, gTalkSelects_50_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_50[4] = {
#include "lobby/008158F0.inc"
};
TalkLine gTalkLines_50[42] = {
#include "lobby/00815910.inc"
};
TalkEvent gTalkEvents_51[2] = {
#include "lobby/008159B8.inc"
};
TalkSelLines gTalkSelects_51_0_0_0 = {
#include "lobby/008159C0.inc"
};
TalkSelLines gTalkSelects_51_1_0_0 = {
#include "lobby/008159C8.inc"
};
TalkSelLines gTalkSelects_51_2_0_0 = {
#include "lobby/008159D0.inc"
};
TalkSel gTalkSelects_51_2_0[1] = {
    {0, &gTalkSelects_51_2_0_0},
};
TalkSel gTalkSelects_51_2[1] = {
    {0, gTalkSelects_51_2_0},
};
TalkSel gTalkSelects_51_1_0[1] = {
    {0, &gTalkSelects_51_1_0_0},
};
TalkSel gTalkSelects_51_1[1] = {
    {0, gTalkSelects_51_1_0},
};
TalkSel gTalkSelects_51_0_0[1] = {
    {0, &gTalkSelects_51_0_0_0},
};
TalkSel gTalkSelects_51_0[2] = {
    {0, gTalkSelects_51_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_51[4] = {
#include "lobby/00815A10.inc"
};
TalkLine gTalkLines_51[12] = {
#include "lobby/00815A30.inc"
};
TalkEvent gTalkEvents_52[2] = {
#include "lobby/00815A60.inc"
};
TalkSelLines gTalkSelects_52_0_0_0 = {
#include "lobby/00815A68.inc"
};
TalkSel gTalkSelects_52_0_0[1] = {
    {0, &gTalkSelects_52_0_0_0},
};
TalkSel gTalkSelects_52_0[1] = {
    {0, gTalkSelects_52_0_0},
};
TalkSel gTalkSelects_52[2] = {
    {0, gTalkSelects_52_0},
    {0, NULL},
};
TalkLine gTalkLines_52[6] = {
#include "lobby/00815A90.inc"
};
TalkEvent gTalkEvents_53[2] = {
#include "lobby/00815AA8.inc"
};
TalkSelLines gTalkSelects_53_0_0_0 = {
#include "lobby/00815AB0.inc"
};
TalkSel gTalkSelects_53_0_0[1] = {
    {0, &gTalkSelects_53_0_0_0},
};
TalkSel gTalkSelects_53_0[1] = {
    {0, gTalkSelects_53_0_0},
};
TalkSel gTalkSelects_53[1] = {
    {0, gTalkSelects_53_0},
};
TalkLine gTalkLines_53[6] = {
#include "lobby/00815AD0.inc"
};
TalkEvent gTalkEvents_54[2] = {
#include "lobby/00815AE8.inc"
};
TalkSelLines gTalkSelects_54_0_0_0 = {
#include "lobby/00815AF0.inc"
};
TalkSel gTalkSelects_54_0_0[1] = {
    {0, &gTalkSelects_54_0_0_0},
};
TalkSel gTalkSelects_54_0[1] = {
    {0, gTalkSelects_54_0_0},
};
TalkSel gTalkSelects_54[1] = {
    {0, gTalkSelects_54_0},
};
TalkLine gTalkLines_54[8] = {
#include "lobby/00815B10.inc"
};
TalkEvent gTalkEvents_55[8] = {
#include "lobby/00815B30.inc"
};
TalkSelLines gTalkSelects_55_0_0_0 = {
#include "lobby/00815B50.inc"
};
TalkSelLines gTalkSelects_55_1_0_0 = {
#include "lobby/00815B58.inc"
};
TalkSelLines gTalkSelects_55_2_0_0 = {
#include "lobby/00815B60.inc"
};
TalkSelLines gTalkSelects_55_3_0_0 = {
#include "lobby/00815B68.inc"
};
TalkSelLines gTalkSelects_55_3_0_1 = {
#include "lobby/00815B70.inc"
};
TalkSelLines gTalkSelects_55_4_0_0 = {
#include "lobby/00815B78.inc"
};
TalkSel gTalkSelects_55_4_0[1] = {
    {0, &gTalkSelects_55_4_0_0},
};
TalkSel gTalkSelects_55_4[1] = {
    {0, gTalkSelects_55_4_0},
};
TalkSel gTalkSelects_55_3_0[2] = {
#include "lobby/00815B90.inc"
};
TalkSel gTalkSelects_55_3[1] = {
    {0, gTalkSelects_55_3_0},
};
TalkSel gTalkSelects_55_2_0[1] = {
    {0, &gTalkSelects_55_2_0_0},
};
TalkSel gTalkSelects_55_2[1] = {
    {0, gTalkSelects_55_2_0},
};
TalkSel gTalkSelects_55_1_0[1] = {
    {0, &gTalkSelects_55_1_0_0},
};
TalkSel gTalkSelects_55_1[1] = {
    {0, gTalkSelects_55_1_0},
};
TalkSel gTalkSelects_55_0_0[1] = {
    {0, &gTalkSelects_55_0_0_0},
};
TalkSel gTalkSelects_55_0[2] = {
    {0, gTalkSelects_55_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_55[6] = {
#include "lobby/00815BE0.inc"
};
TalkLine gTalkLines_55[56] = {
#include "lobby/00815C10.inc"
};
TalkGift gTalkGifts_55[4] = {
#include "lobby/00815CF0.inc"
};
TalkEvent gTalkEvents_56[6] = {
#include "lobby/00815D10.inc"
};
TalkSelLines gTalkSelects_56_0_0_0 = {
#include "lobby/00815D28.inc"
};
TalkSelLines gTalkSelects_56_1_0_0 = {
#include "lobby/00815D30.inc"
};
TalkSelLines gTalkSelects_56_2_0_0 = {
#include "lobby/00815D38.inc"
};
TalkSelLines gTalkSelects_56_3_0_0 = {
#include "lobby/00815D40.inc"
};
TalkSel gTalkSelects_56_3_0[1] = {
    {0, &gTalkSelects_56_3_0_0},
};
TalkSel gTalkSelects_56_3[1] = {
    {0, gTalkSelects_56_3_0},
};
TalkSel gTalkSelects_56_2_0[1] = {
    {0, &gTalkSelects_56_2_0_0},
};
TalkSel gTalkSelects_56_2[1] = {
    {0, gTalkSelects_56_2_0},
};
TalkSel gTalkSelects_56_1_0[1] = {
    {0, &gTalkSelects_56_1_0_0},
};
TalkSel gTalkSelects_56_1[1] = {
    {0, gTalkSelects_56_1_0},
};
TalkSel gTalkSelects_56_0_0[1] = {
    {0, &gTalkSelects_56_0_0_0},
};
TalkSel gTalkSelects_56_0[2] = {
    {0, gTalkSelects_56_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_56[4] = {
#include "lobby/00815D90.inc"
};
TalkLine gTalkLines_56[56] = {
#include "lobby/00815DB0.inc"
};
TalkEvent gTalkEvents_57[8] = {
#include "lobby/00815E90.inc"
};
TalkSelLines gTalkSelects_57_0_0_0 = {
#include "lobby/00815EB0.inc"
};
TalkSelLines gTalkSelects_57_0_1_0 = {
#include "lobby/00815EB8.inc"
};
TalkSelLines gTalkSelects_57_0_2_0 = {
#include "lobby/00815EC0.inc"
};
TalkSelLines gTalkSelects_57_1_0_0 = {
#include "lobby/00815EC8.inc"
};
TalkSelLines gTalkSelects_57_1_1_0 = {
#include "lobby/00815ED0.inc"
};
TalkSelLines gTalkSelects_57_1_2_0 = {
#include "lobby/00815ED8.inc"
};
TalkSelLines gTalkSelects_57_2_0_0 = {
#include "lobby/00815EE0.inc"
};
TalkSelLines gTalkSelects_57_2_1_0 = {
#include "lobby/00815EE8.inc"
};
TalkSelLines gTalkSelects_57_2_2_0 = {
#include "lobby/00815EF0.inc"
};
TalkSel gTalkSelects_57_2_2[1] = {
    {0, &gTalkSelects_57_2_2_0},
};
TalkSel gTalkSelects_57_2_1[1] = {
    {0, &gTalkSelects_57_2_1_0},
};
TalkSel gTalkSelects_57_2_0[1] = {
    {0, &gTalkSelects_57_2_0_0},
};
TalkSel gTalkSelects_57_2[3] = {
#include "lobby/00815F10.inc"
};
TalkSel gTalkSelects_57_1_2[1] = {
    {0, &gTalkSelects_57_1_2_0},
};
TalkSel gTalkSelects_57_1_1[1] = {
    {0, &gTalkSelects_57_1_1_0},
};
TalkSel gTalkSelects_57_1_0[1] = {
    {0, &gTalkSelects_57_1_0_0},
};
TalkSel gTalkSelects_57_1[3] = {
#include "lobby/00815F40.inc"
};
TalkSel gTalkSelects_57_0_2[1] = {
    {0, &gTalkSelects_57_0_2_0},
};
TalkSel gTalkSelects_57_0_1[1] = {
    {0, &gTalkSelects_57_0_1_0},
};
TalkSel gTalkSelects_57_0_0[1] = {
    {0, &gTalkSelects_57_0_0_0},
};
TalkSel gTalkSelects_57_0[4] = {
#include "lobby/00815F70.inc"
};
TalkSel gTalkSelects_57[4] = {
#include "lobby/00815F90.inc"
};
TalkLine gTalkLines_57[30] = {
#include "lobby/00815FB0.inc"
};
TalkEvent gTalkEvents_58[2] = {
#include "lobby/00816028.inc"
};
TalkSelLines gTalkSelects_58_0_0_0 = {
#include "lobby/00816030.inc"
};
TalkSelLines gTalkSelects_58_1_0_0 = {
#include "lobby/00816038.inc"
};
TalkSelLines gTalkSelects_58_2_0_0 = {
#include "lobby/00816040.inc"
};
TalkSel gTalkSelects_58_2_0[1] = {
    {0, &gTalkSelects_58_2_0_0},
};
TalkSel gTalkSelects_58_2[1] = {
    {0, gTalkSelects_58_2_0},
};
TalkSel gTalkSelects_58_1_0[1] = {
    {0, &gTalkSelects_58_1_0_0},
};
TalkSel gTalkSelects_58_1[1] = {
    {0, gTalkSelects_58_1_0},
};
TalkSel gTalkSelects_58_0_0[1] = {
    {0, &gTalkSelects_58_0_0_0},
};
TalkSel gTalkSelects_58_0[2] = {
    {0, gTalkSelects_58_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_58[4] = {
#include "lobby/00816080.inc"
};
TalkLine gTalkLines_58[18] = {
#include "lobby/008160A0.inc"
};
TalkEvent gTalkEvents_59[3] = {
#include "lobby/008160E8.inc"
};
TalkSelLines gTalkSelects_59_0_0_0 = {
#include "lobby/008160F8.inc"
};
TalkSelLines gTalkSelects_59_1_0_0 = {
#include "lobby/00816100.inc"
};
TalkSelLines gTalkSelects_59_2_0_0 = {
#include "lobby/00816108.inc"
};
TalkSelLines gTalkSelects_59_2_0_1 = {
#include "lobby/00816110.inc"
};
TalkSelLines gTalkSelects_59_2_1_0 = {
#include "lobby/00816118.inc"
};
TalkSelLines gTalkSelects_59_2_1_1 = {
#include "lobby/00816120.inc"
};
TalkSelLines gTalkSelects_59_2_2_0 = {
#include "lobby/00816128.inc"
};
TalkSelLines gTalkSelects_59_2_2_1 = {
#include "lobby/00816130.inc"
};
TalkSel gTalkSelects_59_2_2[2] = {
#include "lobby/00816140.inc"
};
TalkSel gTalkSelects_59_2_1[2] = {
#include "lobby/00816150.inc"
};
TalkSel gTalkSelects_59_2_0[2] = {
#include "lobby/00816160.inc"
};
TalkSel gTalkSelects_59_2[3] = {
#include "lobby/00816170.inc"
};
TalkSel gTalkSelects_59_1_0[1] = {
    {0, &gTalkSelects_59_1_0_0},
};
TalkSel gTalkSelects_59_1[1] = {
    {0, gTalkSelects_59_1_0},
};
TalkSel gTalkSelects_59_0_0[1] = {
    {0, &gTalkSelects_59_0_0_0},
};
TalkSel gTalkSelects_59_0[2] = {
    {0, gTalkSelects_59_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_59[4] = {
#include "lobby/008161B0.inc"
};
TalkLine gTalkLines_59[48] = {
#include "lobby/008161D0.inc"
};
TalkEvent gTalkEvents_60[4] = {
#include "lobby/00816290.inc"
};
TalkSelLines gTalkSelects_60_0_0_0 = {
#include "lobby/008162A0.inc"
};
TalkSelLines gTalkSelects_60_1_0_0 = {
#include "lobby/008162A8.inc"
};
TalkSelLines gTalkSelects_60_2_0_0 = {
#include "lobby/008162B0.inc"
};
TalkSelLines gTalkSelects_60_2_0_1 = {
#include "lobby/008162B8.inc"
};
TalkSelLines gTalkSelects_60_2_1_0 = {
#include "lobby/008162C0.inc"
};
TalkSelLines gTalkSelects_60_2_1_1 = {
#include "lobby/008162C8.inc"
};
TalkSelLines gTalkSelects_60_2_2_0 = {
#include "lobby/008162D0.inc"
};
TalkSelLines gTalkSelects_60_2_2_1 = {
#include "lobby/008162D8.inc"
};
TalkSel gTalkSelects_60_2_2[2] = {
#include "lobby/008162E0.inc"
};
TalkSel gTalkSelects_60_2_1[2] = {
#include "lobby/008162F0.inc"
};
TalkSel gTalkSelects_60_2_0[2] = {
#include "lobby/00816300.inc"
};
TalkSel gTalkSelects_60_2[3] = {
#include "lobby/00816310.inc"
};
TalkSel gTalkSelects_60_1_0[1] = {
    {0, &gTalkSelects_60_1_0_0},
};
TalkSel gTalkSelects_60_1[1] = {
    {0, gTalkSelects_60_1_0},
};
TalkSel gTalkSelects_60_0_0[1] = {
    {0, &gTalkSelects_60_0_0_0},
};
TalkSel gTalkSelects_60_0[2] = {
    {0, gTalkSelects_60_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_60[4] = {
#include "lobby/00816350.inc"
};
TalkLine gTalkLines_60[46] = {
#include "lobby/00816370.inc"
};
TalkEvent gTalkEvents_61[2] = {
#include "lobby/00816428.inc"
};
TalkSelLines gTalkSelects_61_0_0_0 = {
#include "lobby/00816430.inc"
};
TalkSelLines gTalkSelects_61_1_0_0 = {
#include "lobby/00816438.inc"
};
TalkSelLines gTalkSelects_61_2_0_0 = {
#include "lobby/00816440.inc"
};
TalkSelLines gTalkSelects_61_2_1_0 = {
#include "lobby/00816448.inc"
};
TalkSel gTalkSelects_61_2_2[1] = {
    {0, &gTalkSelects_61_2_2_0},
};
TalkSel gTalkSelects_61_2_1[1] = {
    {0, &gTalkSelects_61_2_1_0},
};
TalkSel gTalkSelects_61_2_0[2] = {
    {0, &gTalkSelects_61_2_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_61_2[3] = {
#include "lobby/00816470.inc"
};
TalkSel gTalkSelects_61_1_0[1] = {
    {0, &gTalkSelects_61_1_0_0},
};
TalkSel gTalkSelects_61_1[1] = {
    {0, gTalkSelects_61_1_0},
};
TalkSel gTalkSelects_61_0_0[1] = {
    {0, &gTalkSelects_61_0_0_0},
};
TalkSel gTalkSelects_61_0[2] = {
    {0, gTalkSelects_61_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_61[4] = {
#include "lobby/008164B0.inc"
};
TalkLine gTalkLines_61[36] = {
#include "lobby/008164D0.inc"
};
TalkEvent gTalkEvents_62[2] = {
#include "lobby/00816560.inc"
};
TalkSelLines gTalkSelects_62_0_0_0 = {
#include "lobby/00816568.inc"
};
TalkSelLines gTalkSelects_62_1_0_0 = {
#include "lobby/00816570.inc"
};
TalkSelLines gTalkSelects_62_2_0_0 = {
#include "lobby/00816578.inc"
};
TalkSelLines gTalkSelects_62_2_0_1 = {
#include "lobby/00816580.inc"
};
TalkSel gTalkSelects_62_2_0[2] = {
#include "lobby/00816590.inc"
};
TalkSel gTalkSelects_62_2[1] = {
    {0, gTalkSelects_62_2_0},
};
TalkSel gTalkSelects_62_1_0[1] = {
    {0, &gTalkSelects_62_1_0_0},
};
TalkSel gTalkSelects_62_1[1] = {
    {0, gTalkSelects_62_1_0},
};
TalkSel gTalkSelects_62_0_0[1] = {
    {0, &gTalkSelects_62_0_0_0},
};
TalkSel gTalkSelects_62_0[2] = {
    {0, gTalkSelects_62_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_62[4] = {
#include "lobby/008165D0.inc"
};
TalkLine gTalkLines_62[26] = {
#include "lobby/008165F0.inc"
};
TalkEvent gTalkEvents_63[2] = {
#include "lobby/00816658.inc"
};
TalkSelLines gTalkSelects_63_0_0_0 = {
#include "lobby/00816660.inc"
};
TalkSel gTalkSelects_63_0_0[1] = {
    {0, &gTalkSelects_63_0_0_0},
};
TalkSel gTalkSelects_63_0[1] = {
    {0, gTalkSelects_63_0_0},
};
TalkSel gTalkSelects_63[1] = {
    {0, gTalkSelects_63_0},
};
TalkLine gTalkLines_63[8] = {
#include "lobby/00816680.inc"
};
TalkEvent gTalkEvents_64[14] = {
#include "lobby/008166A0.inc"
};
TalkSelLines gTalkSelects_64_0_0_0 = {
#include "lobby/008166D8.inc"
};
TalkSelLines gTalkSelects_64_1_0_0 = {
#include "lobby/008166E0.inc"
};
TalkSelLines gTalkSelects_64_2_0_0 = {
#include "lobby/008166E8.inc"
};
TalkSelLines gTalkSelects_64_3_0_0 = {
#include "lobby/008166F0.inc"
};
TalkSelLines gTalkSelects_64_4_0_0 = {
#include "lobby/008166F8.inc"
};
TalkSel gTalkSelects_64_4_0[1] = {
    {0, &gTalkSelects_64_4_0_0},
};
TalkSel gTalkSelects_64_4[1] = {
    {0, gTalkSelects_64_4_0},
};
TalkSel gTalkSelects_64_3_0[1] = {
    {0, &gTalkSelects_64_3_0_0},
};
TalkSel gTalkSelects_64_3[1] = {
    {0, gTalkSelects_64_3_0},
};
TalkSel gTalkSelects_64_2_0[1] = {
    {0, &gTalkSelects_64_2_0_0},
};
TalkSel gTalkSelects_64_2[1] = {
    {0, gTalkSelects_64_2_0},
};
TalkSel gTalkSelects_64_1_0[1] = {
    {0, &gTalkSelects_64_1_0_0},
};
TalkSel gTalkSelects_64_1[1] = {
    {0, gTalkSelects_64_1_0},
};
TalkSel gTalkSelects_64_0_0[1] = {
    {0, &gTalkSelects_64_0_0_0},
};
TalkSel gTalkSelects_64_0[1] = {
    {0, gTalkSelects_64_0_0},
};
TalkSel gTalkSelects_64[6] = {
#include "lobby/00816750.inc"
};
TalkLine gTalkLines_64[108] = {
#include "lobby/00816780.inc"
};
TalkGift gTalkGifts_64[6] = {
#include "lobby/00816930.inc"
};
TalkEvent gTalkEvents_65[6] = {
#include "lobby/00816960.inc"
};
TalkSelLines gTalkSelects_65_0_0_0 = {
#include "lobby/00816978.inc"
};
TalkSelLines gTalkSelects_65_0_0_1 = {
#include "lobby/00816980.inc"
};
TalkSel gTalkSelects_65_0_0[2] = {
#include "lobby/00816990.inc"
};
TalkSel gTalkSelects_65_0[1] = {
    {0, gTalkSelects_65_0_0},
};
TalkSel gTalkSelects_65[1] = {
    {0, gTalkSelects_65_0},
};
TalkLine gTalkLines_65[40] = {
#include "lobby/008169B0.inc"
};
TalkGift gTalkGifts_65[4] = {
#include "lobby/00816A50.inc"
};
TalkEvent gTalkEvents_66[4] = {
#include "lobby/00816A70.inc"
};
TalkSelLines gTalkSelects_66_0_0_0 = {
#include "lobby/00816A80.inc"
};
TalkSel gTalkSelects_66_0_0[1] = {
    {0, &gTalkSelects_66_0_0_0},
};
TalkSel gTalkSelects_66_0[1] = {
    {0, gTalkSelects_66_0_0},
};
TalkSel gTalkSelects_66[1] = {
    {0, gTalkSelects_66_0},
};
TalkLine gTalkLines_66[10] = {
#include "lobby/00816AA0.inc"
};
TalkEvent gTalkEvents_67[2] = {
#include "lobby/00816AC8.inc"
};
TalkSel gTalkSelects_67_0_0[1] = {
    {0, &gTalkSelects_67_0_0_0},
};
TalkSel gTalkSelects_67_0[1] = {
    {0, gTalkSelects_67_0_0},
};
TalkSel gTalkSelects_67[1] = {
    {0, gTalkSelects_67_0},
};
TalkLine gTalkLines_67[2] = {
#include "lobby/00816AE8.inc"
};
TalkEvent gTalkEvents_68[2] = {
#include "lobby/00816AF0.inc"
};
TalkSel gTalkSelects_68_0_0[1] = {
    {0, &gTalkSelects_68_0_0_0},
};
TalkSel gTalkSelects_68_0[1] = {
    {0, gTalkSelects_68_0_0},
};
TalkSel gTalkSelects_68[1] = {
    {0, gTalkSelects_68_0},
};
TalkLine gTalkLines_68[2] = {
#include "lobby/00816B10.inc"
};
TalkEvent gTalkEvents_69[2] = {
#include "lobby/00816B18.inc"
};
TalkSel gTalkSelects_69_0_0[1] = {
    {0, &gTalkSelects_69_0_0_0},
};
TalkSel gTalkSelects_69_0[1] = {
    {0, gTalkSelects_69_0_0},
};
TalkSel gTalkSelects_69[2] = {
    {0, gTalkSelects_69_0},
    {0, NULL},
};
TalkLine gTalkLines_69[12] = {
#include "lobby/00816B40.inc"
};
TalkEvent gTalkEvents_70[2] = {
#include "lobby/00816B70.inc"
};
TalkSel gTalkSelects_70_0_0[1] = {
    {0, &gTalkSelects_70_0_0_0},
};
TalkSel gTalkSelects_70_0[1] = {
    {0, gTalkSelects_70_0_0},
};
TalkSel gTalkSelects_70[1] = {
    {0, gTalkSelects_70_0},
};
TalkLine gTalkLines_70[12] = {
#include "lobby/00816B90.inc"
};
TalkEvent gTalkEvents_71[2] = {
#include "lobby/00816BC0.inc"
};
TalkSel gTalkSelects_71_0_0[1] = {
    {0, &gTalkSelects_71_0_0_0},
};
TalkSel gTalkSelects_71_0[1] = {
    {0, gTalkSelects_71_0_0},
};
TalkSel gTalkSelects_71[1] = {
    {0, gTalkSelects_71_0},
};
TalkLine gTalkLines_71[12] = {
#include "lobby/00816BE0.inc"
};
TalkEvent gTalkEvents_72[2] = {
#include "lobby/00816C10.inc"
};
TalkSel gTalkSelects_72_0_0[1] = {
    {0, &gTalkSelects_72_0_0_0},
};
TalkSel gTalkSelects_72_0[1] = {
    {0, gTalkSelects_72_0_0},
};
TalkSel gTalkSelects_72[1] = {
    {0, gTalkSelects_72_0},
};
TalkLine gTalkLines_72[4] = {
#include "lobby/00816C30.inc"
};
TalkEvent gTalkEvents_73[6] = {
#include "lobby/00816C40.inc"
};
TalkSelLines gTalkSelects_73_0_0_0 = {
#include "lobby/00816C58.inc"
};
TalkSelLines gTalkSelects_73_1_0_0 = {
#include "lobby/00816C60.inc"
};
TalkSelLines gTalkSelects_73_2_0_0 = {
#include "lobby/00816C68.inc"
};
TalkSel gTalkSelects_73_2_0[1] = {
    {0, &gTalkSelects_73_2_0_0},
};
TalkSel gTalkSelects_73_2[1] = {
    {0, gTalkSelects_73_2_0},
};
TalkSel gTalkSelects_73_1_0[1] = {
    {0, &gTalkSelects_73_1_0_0},
};
TalkSel gTalkSelects_73_1[1] = {
    {0, gTalkSelects_73_1_0},
};
TalkSel gTalkSelects_73_0_0[1] = {
    {0, &gTalkSelects_73_0_0_0},
};
TalkSel gTalkSelects_73_0[1] = {
    {0, gTalkSelects_73_0_0},
};
TalkSel gTalkSelects_73[4] = {
#include "lobby/00816CA0.inc"
};
TalkLine gTalkLines_73[28] = {
#include "lobby/00816CC0.inc"
};
TalkGift gTalkGifts_73[2] = {
#include "lobby/00816D30.inc"
};
TalkEvent gTalkEvents_74[2] = {
#include "lobby/00816D40.inc"
};
TalkSelLines gTalkSelects_74_0_0_0 = {
#include "lobby/00816D48.inc"
};
TalkSelLines gTalkSelects_74_1_0_0 = {
#include "lobby/00816D50.inc"
};
TalkSelLines gTalkSelects_74_2_0_0 = {
#include "lobby/00816D58.inc"
};
TalkSel gTalkSelects_74_2_0[1] = {
    {0, &gTalkSelects_74_2_0_0},
};
TalkSel gTalkSelects_74_2[1] = {
    {0, gTalkSelects_74_2_0},
};
TalkSel gTalkSelects_74_1_0[1] = {
    {0, &gTalkSelects_74_1_0_0},
};
TalkSel gTalkSelects_74_1[1] = {
    {0, gTalkSelects_74_1_0},
};
TalkSel gTalkSelects_74_0_0[1] = {
    {0, &gTalkSelects_74_0_0_0},
};
TalkSel gTalkSelects_74_0[1] = {
    {0, gTalkSelects_74_0_0},
};
TalkSel gTalkSelects_74[4] = {
#include "lobby/00816D90.inc"
};
TalkLine gTalkLines_74[18] = {
#include "lobby/00816DB0.inc"
};
TalkEvent gTalkEvents_75[2] = {
#include "lobby/00816DF8.inc"
};
TalkSelLines gTalkSelects_75_0_0_0 = {
#include "lobby/00816E00.inc"
};
TalkSelLines gTalkSelects_75_1_0_0 = {
#include "lobby/00816E08.inc"
};
TalkSelLines gTalkSelects_75_2_0_0 = {
#include "lobby/00816E10.inc"
};
TalkSel gTalkSelects_75_2_0[1] = {
    {0, &gTalkSelects_75_2_0_0},
};
TalkSel gTalkSelects_75_2[1] = {
    {0, gTalkSelects_75_2_0},
};
TalkSel gTalkSelects_75_1_0[1] = {
    {0, &gTalkSelects_75_1_0_0},
};
TalkSel gTalkSelects_75_1[1] = {
    {0, gTalkSelects_75_1_0},
};
TalkSel gTalkSelects_75_0_0[1] = {
    {0, &gTalkSelects_75_0_0_0},
};
TalkSel gTalkSelects_75_0[2] = {
    {0, gTalkSelects_75_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_75[4] = {
#include "lobby/00816E50.inc"
};
TalkLine gTalkLines_75[18] = {
#include "lobby/00816E70.inc"
};
TalkEvent gTalkEvents_76[2] = {
#include "lobby/00816EB8.inc"
};
TalkSelLines gTalkSelects_76_0_0_0 = {
#include "lobby/00816EC0.inc"
};
TalkSelLines gTalkSelects_76_1_0_0 = {
#include "lobby/00816EC8.inc"
};
TalkSelLines gTalkSelects_76_2_0_0 = {
#include "lobby/00816ED0.inc"
};
TalkSel gTalkSelects_76_2_0[1] = {
    {0, &gTalkSelects_76_2_0_0},
};
TalkSel gTalkSelects_76_2[1] = {
    {0, gTalkSelects_76_2_0},
};
TalkSel gTalkSelects_76_1_0[1] = {
    {0, &gTalkSelects_76_1_0_0},
};
TalkSel gTalkSelects_76_1[1] = {
    {0, gTalkSelects_76_1_0},
};
TalkSel gTalkSelects_76_0_0[1] = {
    {0, &gTalkSelects_76_0_0_0},
};
TalkSel gTalkSelects_76_0[2] = {
    {0, gTalkSelects_76_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_76[4] = {
#include "lobby/00816F10.inc"
};
TalkLine gTalkLines_76[28] = {
#include "lobby/00816F30.inc"
};
TalkEvent gTalkEvents_77[6] = {
#include "lobby/00816FA0.inc"
};
TalkSelLines gTalkSelects_77_0_0_0 = {
#include "lobby/00816FB8.inc"
};
TalkSel gTalkSelects_77_1_0[1] = {
    {0, &gTalkSelects_77_1_0_0},
};
TalkSel gTalkSelects_77_1[1] = {
    {0, gTalkSelects_77_1_0},
};
TalkSel gTalkSelects_77_0_0[1] = {
    {0, &gTalkSelects_77_0_0_0},
};
TalkSel gTalkSelects_77_0[1] = {
    {0, gTalkSelects_77_0_0},
};
TalkSel gTalkSelects_77[2] = {
#include "lobby/00816FE0.inc"
};
TalkLine gTalkLines_77[36] = {
#include "lobby/00816FF0.inc"
};
TalkGift gTalkGifts_77[2] = {
#include "lobby/00817080.inc"
};
TalkEvent gTalkEvents_78[12] = {
#include "lobby/00817090.inc"
};
TalkSelLines gTalkSelects_78_0_0_0 = {
#include "lobby/008170C0.inc"
};
TalkSelLines gTalkSelects_78_1_0_0 = {
#include "lobby/008170C8.inc"
};
TalkSel gTalkSelects_78_1_0[1] = {
    {0, &gTalkSelects_78_1_0_0},
};
TalkSel gTalkSelects_78_1[1] = {
    {0, gTalkSelects_78_1_0},
};
TalkSel gTalkSelects_78_0_0[1] = {
    {0, &gTalkSelects_78_0_0_0},
};
TalkSel gTalkSelects_78_0[1] = {
    {0, gTalkSelects_78_0_0},
};
TalkSel gTalkSelects_78[2] = {
#include "lobby/008170F0.inc"
};
TalkLine gTalkLines_78[80] = {
#include "lobby/00817100.inc"
};
TalkGift gTalkGifts_78[6] = {
#include "lobby/00817240.inc"
};
TalkEvent gTalkEvents_79[6] = {
#include "lobby/00817270.inc"
};
TalkSelLines gTalkSelects_79_0_0_0 = {
#include "lobby/00817288.inc"
};
TalkSelLines gTalkSelects_79_1_0_0 = {
#include "lobby/00817290.inc"
};
TalkSel gTalkSelects_79_1_0[1] = {
    {0, &gTalkSelects_79_1_0_0},
};
TalkSel gTalkSelects_79_1[1] = {
    {0, gTalkSelects_79_1_0},
};
TalkSel gTalkSelects_79_0_0[1] = {
    {0, &gTalkSelects_79_0_0_0},
};
TalkSel gTalkSelects_79_0[2] = {
    {0, gTalkSelects_79_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_79[2] = {
#include "lobby/008172C0.inc"
};
TalkLine gTalkLines_79[36] = {
#include "lobby/008172D0.inc"
};
TalkEvent gTalkEvents_80[4] = {
#include "lobby/00817360.inc"
};
TalkSelLines gTalkSelects_80_0_0_0 = {
#include "lobby/00817370.inc"
};
TalkSelLines gTalkSelects_80_1_0_0 = {
#include "lobby/00817378.inc"
};
TalkSel gTalkSelects_80_1_0[1] = {
    {0, &gTalkSelects_80_1_0_0},
};
TalkSel gTalkSelects_80_1[1] = {
    {0, gTalkSelects_80_1_0},
};
TalkSel gTalkSelects_80_0_0[1] = {
    {0, &gTalkSelects_80_0_0_0},
};
TalkSel gTalkSelects_80_0[1] = {
    {0, gTalkSelects_80_0_0},
};
TalkSel gTalkSelects_80[2] = {
#include "lobby/008173A0.inc"
};
TalkLine gTalkLines_80[10] = {
#include "lobby/008173B0.inc"
};
TalkEvent gTalkEvents_81[2] = {
#include "lobby/008173D8.inc"
};
TalkSelLines gTalkSelects_81_0_0_0 = {
#include "lobby/008173E0.inc"
};
TalkSel gTalkSelects_81_1_0[1] = {
    {0, &gTalkSelects_81_1_0_0},
};
TalkSel gTalkSelects_81_1[1] = {
    {0, gTalkSelects_81_1_0},
};
TalkSel gTalkSelects_81_0_0[1] = {
    {0, &gTalkSelects_81_0_0_0},
};
TalkSel gTalkSelects_81_0[2] = {
    {0, gTalkSelects_81_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_81[2] = {
#include "lobby/00817410.inc"
};
TalkLine gTalkLines_81[14] = {
#include "lobby/00817420.inc"
};
TalkEvent gTalkEvents_82[2] = {
#include "lobby/00817458.inc"
};
TalkSelLines gTalkSelects_82_0_0_0 = {
#include "lobby/00817460.inc"
};
TalkSel gTalkSelects_82_1_0[1] = {
    {0, &gTalkSelects_82_1_0_0},
};
TalkSel gTalkSelects_82_1[1] = {
    {0, gTalkSelects_82_1_0},
};
TalkSel gTalkSelects_82_0_0[1] = {
    {0, &gTalkSelects_82_0_0_0},
};
TalkSel gTalkSelects_82_0[2] = {
    {0, gTalkSelects_82_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_82[2] = {
#include "lobby/00817490.inc"
};
TalkLine gTalkLines_82[14] = {
#include "lobby/008174A0.inc"
};
TalkEvent gTalkEvents_83[2] = {
#include "lobby/008174D8.inc"
};
TalkSelLines gTalkSelects_83_0_0_0 = {
#include "lobby/008174E0.inc"
};
TalkSelLines gTalkSelects_83_1_0_0 = {
#include "lobby/008174E8.inc"
};
TalkSel gTalkSelects_83_1_0[1] = {
    {0, &gTalkSelects_83_1_0_0},
};
TalkSel gTalkSelects_83_1[1] = {
    {0, gTalkSelects_83_1_0},
};
TalkSel gTalkSelects_83_0_0[1] = {
    {0, &gTalkSelects_83_0_0_0},
};
TalkSel gTalkSelects_83_0[1] = {
    {0, gTalkSelects_83_0_0},
};
TalkSel gTalkSelects_83[2] = {
#include "lobby/00817510.inc"
};
TalkLine gTalkLines_83[12] = {
#include "lobby/00817520.inc"
};
TalkEvent gTalkEvents_84[2] = {
#include "lobby/00817550.inc"
};
TalkSelLines gTalkSelects_84_0_0_0 = {
#include "lobby/00817558.inc"
};
TalkSelLines gTalkSelects_84_1_0_0 = {
#include "lobby/00817560.inc"
};
TalkSel gTalkSelects_84_1_0[1] = {
    {0, &gTalkSelects_84_1_0_0},
};
TalkSel gTalkSelects_84_1[1] = {
    {0, gTalkSelects_84_1_0},
};
TalkSel gTalkSelects_84_0_0[1] = {
    {0, &gTalkSelects_84_0_0_0},
};
TalkSel gTalkSelects_84_0[2] = {
    {0, gTalkSelects_84_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_84[2] = {
#include "lobby/00817590.inc"
};
TalkLine gTalkLines_84[20] = {
#include "lobby/008175A0.inc"
};
TalkEvent gTalkEvents_85[4] = {
#include "lobby/008175F0.inc"
};
TalkSelLines gTalkSelects_85_0_0_0 = {
#include "lobby/00817600.inc"
};
TalkSelLines gTalkSelects_85_1_0_0 = {
#include "lobby/00817608.inc"
};
TalkSelLines gTalkSelects_85_2_0_0 = {
#include "lobby/00817610.inc"
};
TalkSel gTalkSelects_85_2_0[1] = {
    {0, &gTalkSelects_85_2_0_0},
};
TalkSel gTalkSelects_85_2[1] = {
    {0, gTalkSelects_85_2_0},
};
TalkSel gTalkSelects_85_1_0[1] = {
    {0, &gTalkSelects_85_1_0_0},
};
TalkSel gTalkSelects_85_1[1] = {
    {0, gTalkSelects_85_1_0},
};
TalkSel gTalkSelects_85_0_0[1] = {
    {0, &gTalkSelects_85_0_0_0},
};
TalkSel gTalkSelects_85_0[2] = {
    {0, gTalkSelects_85_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_85[4] = {
#include "lobby/00817650.inc"
};
TalkLine gTalkLines_85[12] = {
#include "lobby/00817670.inc"
};
TalkEvent gTalkEvents_86[8] = {
#include "lobby/008176A0.inc"
};
TalkSelLines gTalkSelects_86_0_0_0 = {
#include "lobby/008176C0.inc"
};
TalkSelLines gTalkSelects_86_1_0_0 = {
#include "lobby/008176C8.inc"
};
TalkSelLines gTalkSelects_86_2_0_0 = {
#include "lobby/008176D0.inc"
};
TalkSel gTalkSelects_86_2_0[1] = {
    {0, &gTalkSelects_86_2_0_0},
};
TalkSel gTalkSelects_86_2[1] = {
    {0, gTalkSelects_86_2_0},
};
TalkSel gTalkSelects_86_1_0[1] = {
    {0, &gTalkSelects_86_1_0_0},
};
TalkSel gTalkSelects_86_1[1] = {
    {0, gTalkSelects_86_1_0},
};
TalkSel gTalkSelects_86_0_0[1] = {
    {0, &gTalkSelects_86_0_0_0},
};
TalkSel gTalkSelects_86_0[2] = {
    {0, gTalkSelects_86_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_86[4] = {
#include "lobby/00817710.inc"
};
TalkLine gTalkLines_86[18] = {
#include "lobby/00817730.inc"
};
TalkEvent gTalkEvents_87[2] = {
#include "lobby/00817778.inc"
};
TalkSelLines gTalkSelects_87_0_0_0 = {
#include "lobby/00817780.inc"
};
TalkSelLines gTalkSelects_87_0_0_1 = {
#include "lobby/00817788.inc"
};
TalkSelLines gTalkSelects_87_1_0_0 = {
#include "lobby/00817790.inc"
};
TalkSelLines gTalkSelects_87_1_1_0 = {
#include "lobby/00817798.inc"
};
TalkSelLines gTalkSelects_87_1_2_0 = {
#include "lobby/008177A0.inc"
};
TalkSelLines gTalkSelects_87_2_0_0 = {
#include "lobby/008177A8.inc"
};
TalkSel gTalkSelects_87_2_0[2] = {
#include "lobby/008177B0.inc"
};
TalkSel gTalkSelects_87_2[1] = {
    {0, gTalkSelects_87_2_0},
};
TalkSel gTalkSelects_87_1_2[1] = {
    {0, &gTalkSelects_87_1_2_0},
};
TalkSel gTalkSelects_87_1_1[1] = {
    {0, &gTalkSelects_87_1_1_0},
};
TalkSel gTalkSelects_87_1_0[1] = {
    {0, &gTalkSelects_87_1_0_0},
};
TalkSel gTalkSelects_87_1[4] = {
#include "lobby/008177E0.inc"
};
TalkSel gTalkSelects_87_0_0[2] = {
#include "lobby/00817800.inc"
};
TalkSel gTalkSelects_87_0[2] = {
    {0, gTalkSelects_87_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_87[4] = {
#include "lobby/00817820.inc"
};
TalkLine gTalkLines_87[52] = {
#include "lobby/00817840.inc"
};
TalkEvent gTalkEvents_88[4] = {
#include "lobby/00817910.inc"
};
TalkSel gTalkSelects_88_0_0[1] = {
    {0, &gTalkSelects_88_0_0_0},
};
TalkSel gTalkSelects_88_0[1] = {
    {0, gTalkSelects_88_0_0},
};
TalkSel gTalkSelects_88[2] = {
    {0, gTalkSelects_88_0},
    {0, NULL},
};
TalkLine gTalkLines_88[6] = {
#include "lobby/00817940.inc"
};
TalkEvent gTalkEvents_89[2] = {
#include "lobby/00817958.inc"
};
TalkSel gTalkSelects_89_0_0[1] = {
    {0, &gTalkSelects_89_0_0_0},
};
TalkSel gTalkSelects_89_0[1] = {
    {0, gTalkSelects_89_0_0},
};
TalkSel gTalkSelects_89[1] = {
    {0, gTalkSelects_89_0},
};
TalkLine gTalkLines_89[2] = {
#include "lobby/00817978.inc"
};
TalkEvent gTalkEvents_90[2] = {
#include "lobby/00817980.inc"
};
TalkSel gTalkSelects_90_0_0[1] = {
    {0, &gTalkSelects_90_0_0_0},
};
TalkSel gTalkSelects_90_0[1] = {
    {0, gTalkSelects_90_0_0},
};
TalkSel gTalkSelects_90[1] = {
    {0, gTalkSelects_90_0},
};
TalkLine gTalkLines_90[2] = {
#include "lobby/008179A0.inc"
};
TalkEvent gTalkEvents_91[2] = {
#include "lobby/008179A8.inc"
};
TalkSel gTalkSelects_91_0_0[1] = {
    {0, &gTalkSelects_91_0_0_0},
};
TalkSel gTalkSelects_91_0[1] = {
    {0, gTalkSelects_91_0_0},
};
TalkSel gTalkSelects_91[1] = {
    {0, gTalkSelects_91_0},
};
TalkLine gTalkLines_91[2] = {
#include "lobby/008179C8.inc"
};
TalkEvent gTalkEvents_92[2] = {
#include "lobby/008179D0.inc"
};
TalkSel gTalkSelects_92_0_0[1] = {
    {0, &gTalkSelects_92_0_0_0},
};
TalkSel gTalkSelects_92_0[1] = {
    {0, gTalkSelects_92_0_0},
};
TalkSel gTalkSelects_92[1] = {
    {0, gTalkSelects_92_0},
};
TalkLine gTalkLines_92[2] = {
#include "lobby/008179F0.inc"
};
TalkEvent gTalkEvents_93[2] = {
#include "lobby/008179F8.inc"
};
TalkSelLines gTalkSelects_93_0_0_0 = {
#include "lobby/00817A00.inc"
};
TalkSelLines gTalkSelects_93_0_0_1 = {
#include "lobby/00817A08.inc"
};
TalkSelLines gTalkSelects_93_0_1_0 = {
#include "lobby/00817A10.inc"
};
TalkSelLines gTalkSelects_93_0_1_1 = {
#include "lobby/00817A18.inc"
};
TalkSelLines gTalkSelects_93_0_2_0 = {
#include "lobby/00817A20.inc"
};
TalkSelLines gTalkSelects_93_0_2_1 = {
#include "lobby/00817A28.inc"
};
TalkSel gTalkSelects_93_0_2[2] = {
#include "lobby/00817A30.inc"
};
TalkSel gTalkSelects_93_0_1[2] = {
#include "lobby/00817A40.inc"
};
TalkSel gTalkSelects_93_0_0[2] = {
#include "lobby/00817A50.inc"
};
TalkSel gTalkSelects_93_0[3] = {
#include "lobby/00817A60.inc"
};
TalkSel gTalkSelects_93[1] = {
    {0, gTalkSelects_93_0},
};
TalkLine gTalkLines_93[8] = {
#include "lobby/00817A80.inc"
};
TalkEvent gTalkEvents_94[2] = {
#include "lobby/00817AA0.inc"
};
TalkSelLines gTalkSelects_94_0_0_0 = {
#include "lobby/00817AA8.inc"
};
TalkSelLines gTalkSelects_94_0_0_1 = {
#include "lobby/00817AB0.inc"
};
TalkSelLines gTalkSelects_94_0_1_0 = {
#include "lobby/00817AB8.inc"
};
TalkSelLines gTalkSelects_94_0_1_1 = {
#include "lobby/00817AC0.inc"
};
TalkSelLines gTalkSelects_94_0_2_0 = {
#include "lobby/00817AC8.inc"
};
TalkSelLines gTalkSelects_94_0_2_1 = {
#include "lobby/00817AD0.inc"
};
TalkSel gTalkSelects_94_0_2[2] = {
#include "lobby/00817AE0.inc"
};
TalkSel gTalkSelects_94_0_1[2] = {
#include "lobby/00817AF0.inc"
};
TalkSel gTalkSelects_94_0_0[2] = {
#include "lobby/00817B00.inc"
};
TalkSel gTalkSelects_94_0[3] = {
#include "lobby/00817B10.inc"
};
TalkSel gTalkSelects_94[1] = {
    {0, gTalkSelects_94_0},
};
TalkLine gTalkLines_94[8] = {
#include "lobby/00817B30.inc"
};
TalkEvent gTalkEvents_95[2] = {
#include "lobby/00817B50.inc"
};
TalkSelLines gTalkSelects_95_0_0_0 = {
#include "lobby/00817B58.inc"
};
TalkSelLines gTalkSelects_95_0_0_1 = {
#include "lobby/00817B60.inc"
};
TalkSelLines gTalkSelects_95_0_1_0 = {
#include "lobby/00817B68.inc"
};
TalkSelLines gTalkSelects_95_0_1_1 = {
#include "lobby/00817B70.inc"
};
TalkSelLines gTalkSelects_95_0_2_0 = {
#include "lobby/00817B78.inc"
};
TalkSelLines gTalkSelects_95_0_2_1 = {
#include "lobby/00817B80.inc"
};
TalkSel gTalkSelects_95_0_2[2] = {
#include "lobby/00817B90.inc"
};
TalkSel gTalkSelects_95_0_1[2] = {
#include "lobby/00817BA0.inc"
};
TalkSel gTalkSelects_95_0_0[2] = {
#include "lobby/00817BB0.inc"
};
TalkSel gTalkSelects_95_0[3] = {
#include "lobby/00817BC0.inc"
};
TalkSel gTalkSelects_95[1] = {
    {0, gTalkSelects_95_0},
};
TalkLine gTalkLines_95[30] = {
#include "lobby/00817BE0.inc"
};
TalkEvent gTalkEvents_96[2] = {
#include "lobby/00817C58.inc"
};
TalkSelLines gTalkSelects_96_0_0_0 = {
#include "lobby/00817C60.inc"
};
TalkSelLines gTalkSelects_96_0_1_0 = {
#include "lobby/00817C68.inc"
};
TalkSelLines gTalkSelects_96_0_2_0 = {
#include "lobby/00817C70.inc"
};
TalkSel gTalkSelects_96_0_2[1] = {
    {0, &gTalkSelects_96_0_2_0},
};
TalkSel gTalkSelects_96_0_1[1] = {
    {0, &gTalkSelects_96_0_1_0},
};
TalkSel gTalkSelects_96_0_0[1] = {
    {0, &gTalkSelects_96_0_0_0},
};
TalkSel gTalkSelects_96_0[3] = {
#include "lobby/00817C90.inc"
};
TalkSel gTalkSelects_96[1] = {
    {0, gTalkSelects_96_0},
};
TalkLine gTalkLines_96[16] = {
#include "lobby/00817CB0.inc"
};
TalkEvent gTalkEvents_97[2] = {
#include "lobby/00817CF0.inc"
};
TalkSelLines gTalkSelects_97_0_0_0 = {
#include "lobby/00817CF8.inc"
};
TalkSelLines gTalkSelects_97_0_1_0 = {
#include "lobby/00817D00.inc"
};
TalkSelLines gTalkSelects_97_0_2_0 = {
#include "lobby/00817D08.inc"
};
TalkSel gTalkSelects_97_0_2[1] = {
    {0, &gTalkSelects_97_0_2_0},
};
TalkSel gTalkSelects_97_0_1[1] = {
    {0, &gTalkSelects_97_0_1_0},
};
TalkSel gTalkSelects_97_0_0[2] = {
    {0, &gTalkSelects_97_0_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_97_0[3] = {
#include "lobby/00817D30.inc"
};
TalkSel gTalkSelects_97[1] = {
    {0, gTalkSelects_97_0},
};
TalkLine gTalkLines_97[16] = {
#include "lobby/00817D50.inc"
};
TalkEvent gTalkEvents_98[2] = {
#include "lobby/00817D90.inc"
};
TalkSelLines gTalkSelects_98_0_0_0 = {
#include "lobby/00817D98.inc"
};
TalkSelLines gTalkSelects_98_0_0_1 = {
#include "lobby/00817DA0.inc"
};
TalkSelLines gTalkSelects_98_0_1_0 = {
#include "lobby/00817DA8.inc"
};
TalkSelLines gTalkSelects_98_0_1_1 = {
#include "lobby/00817DB0.inc"
};
TalkSelLines gTalkSelects_98_0_2_0 = {
#include "lobby/00817DB8.inc"
};
TalkSelLines gTalkSelects_98_0_2_1 = {
#include "lobby/00817DC0.inc"
};
TalkSel gTalkSelects_98_0_2[2] = {
#include "lobby/00817DD0.inc"
};
TalkSel gTalkSelects_98_0_1[2] = {
#include "lobby/00817DE0.inc"
};
TalkSel gTalkSelects_98_0_0[2] = {
#include "lobby/00817DF0.inc"
};
TalkSel gTalkSelects_98_0[3] = {
#include "lobby/00817E00.inc"
};
TalkSel gTalkSelects_98[1] = {
    {0, gTalkSelects_98_0},
};
TalkLine gTalkLines_98[28] = {
#include "lobby/00817E20.inc"
};
TalkEvent gTalkEvents_99[2] = {
#include "lobby/00817E90.inc"
};
TalkSelLines gTalkSelects_99_0_0_0 = {
#include "lobby/00817E98.inc"
};
TalkSel gTalkSelects_99_0_0[1] = {
    {0, &gTalkSelects_99_0_0_0},
};
TalkSel gTalkSelects_99_0[1] = {
    {0, gTalkSelects_99_0_0},
};
TalkSel gTalkSelects_99[2] = {
    {0, gTalkSelects_99_0},
    {0, NULL},
};
TalkLine gTalkLines_99[12] = {
#include "lobby/00817EC0.inc"
};
TalkEvent gTalkEvents_100[2] = {
#include "lobby/00817EF0.inc"
};
TalkSelLines gTalkSelects_100_0_0_0 = {
#include "lobby/00817EF8.inc"
};
TalkSelLines gTalkSelects_100_0_0_1 = {
#include "lobby/00817F00.inc"
};
TalkSel gTalkSelects_100_0_0[2] = {
#include "lobby/00817F10.inc"
};
TalkSel gTalkSelects_100_0[1] = {
    {0, gTalkSelects_100_0_0},
};
TalkSel gTalkSelects_100[1] = {
    {0, gTalkSelects_100_0},
};
TalkLine gTalkLines_100[12] = {
#include "lobby/00817F30.inc"
};
TalkEvent gTalkEvents_101[2] = {
#include "lobby/00817F60.inc"
};
TalkSelLines gTalkSelects_101_0_0_0 = {
#include "lobby/00817F68.inc"
};
TalkSelLines gTalkSelects_101_0_0_1 = {
#include "lobby/00817F70.inc"
};
TalkSelLines gTalkSelects_101_0_1_0 = {
#include "lobby/00817F78.inc"
};
TalkSelLines gTalkSelects_101_0_1_1 = {
#include "lobby/00817F80.inc"
};
TalkSelLines gTalkSelects_101_0_2_0 = {
#include "lobby/00817F88.inc"
};
TalkSelLines gTalkSelects_101_0_2_1 = {
#include "lobby/00817F90.inc"
};
TalkSel gTalkSelects_101_0_2[2] = {
#include "lobby/00817FA0.inc"
};
TalkSel gTalkSelects_101_0_1[2] = {
#include "lobby/00817FB0.inc"
};
TalkSel gTalkSelects_101_0_0[2] = {
#include "lobby/00817FC0.inc"
};
TalkSel gTalkSelects_101_0[3] = {
#include "lobby/00817FD0.inc"
};
TalkSel gTalkSelects_101[1] = {
    {0, gTalkSelects_101_0},
};
TalkLine gTalkLines_101[40] = {
#include "lobby/00817FF0.inc"
};
TalkEvent gTalkEvents_102[2] = {
#include "lobby/00818090.inc"
};
TalkSelLines gTalkSelects_102_0_0_0 = {
#include "lobby/00818098.inc"
};
TalkSelLines gTalkSelects_102_0_0_1 = {
#include "lobby/008180A0.inc"
};
TalkSelLines gTalkSelects_102_0_1_0 = {
#include "lobby/008180A8.inc"
};
TalkSelLines gTalkSelects_102_0_1_1 = {
#include "lobby/008180B0.inc"
};
TalkSelLines gTalkSelects_102_0_2_0 = {
#include "lobby/008180B8.inc"
};
TalkSelLines gTalkSelects_102_0_2_1 = {
#include "lobby/008180C0.inc"
};
TalkSel gTalkSelects_102_0_2[2] = {
#include "lobby/008180D0.inc"
};
TalkSel gTalkSelects_102_0_1[2] = {
#include "lobby/008180E0.inc"
};
TalkSel gTalkSelects_102_0_0[2] = {
#include "lobby/008180F0.inc"
};
TalkSel gTalkSelects_102_0[3] = {
#include "lobby/00818100.inc"
};
TalkSel gTalkSelects_102[1] = {
    {0, gTalkSelects_102_0},
};
TalkLine gTalkLines_102[42] = {
#include "lobby/00818120.inc"
};
TalkEvent gTalkEvents_103[2] = {
#include "lobby/008181C8.inc"
};
TalkSelLines gTalkSelects_103_0_0_0 = {
#include "lobby/008181D0.inc"
};
TalkSelLines gTalkSelects_103_0_0_1 = {
#include "lobby/008181D8.inc"
};
TalkSelLines gTalkSelects_103_0_1_0 = {
#include "lobby/008181E0.inc"
};
TalkSelLines gTalkSelects_103_0_1_1 = {
#include "lobby/008181E8.inc"
};
TalkSelLines gTalkSelects_103_0_2_0 = {
#include "lobby/008181F0.inc"
};
TalkSelLines gTalkSelects_103_0_2_1 = {
#include "lobby/008181F8.inc"
};
TalkSelLines gTalkSelects_103_1_0_0 = {
#include "lobby/00818200.inc"
};
TalkSelLines gTalkSelects_103_1_0_1 = {
#include "lobby/00818208.inc"
};
TalkSelLines gTalkSelects_103_2_0_0 = {
#include "lobby/00818210.inc"
};
TalkSelLines gTalkSelects_103_2_0_1 = {
#include "lobby/00818218.inc"
};
TalkSel gTalkSelects_103_2_0[2] = {
#include "lobby/00818220.inc"
};
TalkSel gTalkSelects_103_2[2] = {
    {0, gTalkSelects_103_2_0},
    {0, NULL},
};
TalkSel gTalkSelects_103_1_0[2] = {
#include "lobby/00818240.inc"
};
TalkSel gTalkSelects_103_1[2] = {
    {0, gTalkSelects_103_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_103_0_2[2] = {
#include "lobby/00818260.inc"
};
TalkSel gTalkSelects_103_0_1[2] = {
#include "lobby/00818270.inc"
};
TalkSel gTalkSelects_103_0_0[2] = {
#include "lobby/00818280.inc"
};
TalkSel gTalkSelects_103_0[4] = {
#include "lobby/00818290.inc"
};
TalkSel gTalkSelects_103[4] = {
#include "lobby/008182B0.inc"
};
TalkLine gTalkLines_103[62] = {
#include "lobby/008182D0.inc"
};
TalkEvent gTalkEvents_104[2] = {
#include "lobby/008183C8.inc"
};
TalkSelLines gTalkSelects_104_0_0_0 = {
#include "lobby/008183D0.inc"
};
TalkSelLines gTalkSelects_104_0_0_1 = {
#include "lobby/008183D8.inc"
};
TalkSelLines gTalkSelects_104_0_1_0 = {
#include "lobby/008183E0.inc"
};
TalkSelLines gTalkSelects_104_0_1_1 = {
#include "lobby/008183E8.inc"
};
TalkSelLines gTalkSelects_104_0_2_0 = {
#include "lobby/008183F0.inc"
};
TalkSelLines gTalkSelects_104_0_2_1 = {
#include "lobby/008183F8.inc"
};
TalkSel gTalkSelects_104_0_2[2] = {
#include "lobby/00818400.inc"
};
TalkSel gTalkSelects_104_0_1[2] = {
#include "lobby/00818410.inc"
};
TalkSel gTalkSelects_104_0_0[2] = {
#include "lobby/00818420.inc"
};
TalkSel gTalkSelects_104_0[3] = {
#include "lobby/00818430.inc"
};
TalkSel gTalkSelects_104[1] = {
    {0, gTalkSelects_104_0},
};
TalkLine gTalkLines_104[8] = {
#include "lobby/00818450.inc"
};
TalkEvent gTalkEvents_105[8] = {
#include "lobby/00818470.inc"
};
TalkSelLines gTalkSelects_105_0_0_0 = {
#include "lobby/00818490.inc"
};
TalkSelLines gTalkSelects_105_0_0_1 = {
#include "lobby/00818498.inc"
};
TalkSelLines gTalkSelects_105_0_1_0 = {
#include "lobby/008184A0.inc"
};
TalkSelLines gTalkSelects_105_0_1_1 = {
#include "lobby/008184A8.inc"
};
TalkSelLines gTalkSelects_105_0_2_0 = {
#include "lobby/008184B0.inc"
};
TalkSelLines gTalkSelects_105_0_2_1 = {
#include "lobby/008184B8.inc"
};
TalkSelLines gTalkSelects_105_1_0_0 = {
#include "lobby/008184C0.inc"
};
TalkSelLines gTalkSelects_105_1_0_1 = {
#include "lobby/008184C8.inc"
};
TalkSelLines gTalkSelects_105_2_0_0 = {
#include "lobby/008184D0.inc"
};
TalkSelLines gTalkSelects_105_3_0_0 = {
#include "lobby/008184D8.inc"
};
TalkSelLines gTalkSelects_105_4_0_0 = {
#include "lobby/008184E0.inc"
};
TalkSelLines gTalkSelects_105_4_0_1 = {
#include "lobby/008184E8.inc"
};
TalkSel gTalkSelects_105_4_0[2] = {
#include "lobby/008184F0.inc"
};
TalkSel gTalkSelects_105_4[1] = {
    {0, gTalkSelects_105_4_0},
};
TalkSel gTalkSelects_105_3_0[1] = {
    {0, &gTalkSelects_105_3_0_0},
};
TalkSel gTalkSelects_105_3[1] = {
    {0, gTalkSelects_105_3_0},
};
TalkSel gTalkSelects_105_2_0[1] = {
    {0, &gTalkSelects_105_2_0_0},
};
TalkSel gTalkSelects_105_2[2] = {
    {0, gTalkSelects_105_2_0},
    {0, NULL},
};
TalkSel gTalkSelects_105_1_0[2] = {
#include "lobby/00818530.inc"
};
TalkSel gTalkSelects_105_1[2] = {
    {0, gTalkSelects_105_1_0},
    {0, NULL},
};
TalkSel gTalkSelects_105_0_2[2] = {
#include "lobby/00818550.inc"
};
TalkSel gTalkSelects_105_0_1[2] = {
#include "lobby/00818560.inc"
};
TalkSel gTalkSelects_105_0_0[2] = {
#include "lobby/00818570.inc"
};
TalkSel gTalkSelects_105_0[4] = {
#include "lobby/00818580.inc"
};
TalkSel gTalkSelects_105[6] = {
#include "lobby/008185A0.inc"
};
TalkLine gTalkLines_105[104] = {
#include "lobby/008185D0.inc"
};
TalkGift gTalkGifts_105[4] = {
#include "lobby/00818770.inc"
};
TalkEvent gTalkEvents_106[2] = {
#include "lobby/00818790.inc"
};
TalkSelLines gTalkSelects_106_0_0_0 = {
#include "lobby/00818798.inc"
};
TalkSelLines gTalkSelects_106_0_1_0 = {
#include "lobby/008187A0.inc"
};
TalkSelLines gTalkSelects_106_0_2_0 = {
#include "lobby/008187A8.inc"
};
TalkSel gTalkSelects_106_0_2[1] = {
    {0, &gTalkSelects_106_0_2_0},
};
TalkSel gTalkSelects_106_0_1[1] = {
    {0, &gTalkSelects_106_0_1_0},
};
TalkSel gTalkSelects_106_0_0[2] = {
    {0, &gTalkSelects_106_0_0_0},
    {0, NULL},
};
TalkSel gTalkSelects_106_0[3] = {
#include "lobby/008187D0.inc"
};
TalkSel gTalkSelects_106[1] = {
    {0, gTalkSelects_106_0},
};
TalkLine gTalkLines_106[6] = {
#include "lobby/008187F0.inc"
};
TalkEvent gTalkEvents_107[3] = {
#include "lobby/00818808.inc"
};
TalkSelLines gTalkSelects_107_0_0_0 = {
#include "lobby/00818818.inc"
};
TalkSelLines gTalkSelects_107_0_0_1 = {
#include "lobby/00818820.inc"
};
TalkSelLines gTalkSelects_107_0_1_0 = {
#include "lobby/00818828.inc"
};
TalkSelLines gTalkSelects_107_0_1_1 = {
#include "lobby/00818830.inc"
};
TalkSelLines gTalkSelects_107_0_2_0 = {
#include "lobby/00818838.inc"
};
TalkSelLines gTalkSelects_107_0_2_1 = {
#include "lobby/00818840.inc"
};
TalkSel gTalkSelects_107_0_2[2] = {
#include "lobby/00818850.inc"
};
TalkSel gTalkSelects_107_0_1[2] = {
#include "lobby/00818860.inc"
};
TalkSel gTalkSelects_107_0_0[2] = {
#include "lobby/00818870.inc"
};
TalkSel gTalkSelects_107_0[3] = {
#include "lobby/00818880.inc"
};
TalkSel gTalkSelects_107[1] = {
    {0, gTalkSelects_107_0},
};
TalkLine gTalkLines_107[44] = {
#include "lobby/008188A0.inc"
};
TalkEvent gTalkEvents_108[2] = {
#include "lobby/00818950.inc"
};
TalkSelLines gTalkSelects_108_0_0_0 = {
#include "lobby/00818958.inc"
};
TalkSelLines gTalkSelects_108_0_0_1 = {
#include "lobby/00818960.inc"
};
TalkSel gTalkSelects_108_0_0[2] = {
#include "lobby/00818970.inc"
};
TalkSel gTalkSelects_108_0[1] = {
    {0, gTalkSelects_108_0_0},
};
TalkSel gTalkSelects_108[1] = {
    {0, gTalkSelects_108_0},
};
TalkLine gTalkLines_108[20] = {
#include "lobby/00818990.inc"
};
TalkEvent gTalkEvents_109[2] = {
#include "lobby/008189E0.inc"
};
TalkSel gTalkSelects_109_0_0[1] = {
    {0, &gTalkSelects_109_0_0_0},
};
TalkSel gTalkSelects_109_0[1] = {
    {0, gTalkSelects_109_0_0},
};
TalkSel gTalkSelects_109[1] = {
    {0, gTalkSelects_109_0},
};
TalkLine gTalkLines_109[2] = {
#include "lobby/00818A00.inc"
};
TalkEvent gTalkEvents_110[2] = {
#include "lobby/00818A08.inc"
};
TalkSelLines gTalkSelects_110_0_0_0 = {
#include "lobby/00818A10.inc"
};
TalkSelLines gTalkSelects_110_0_1_0 = {
#include "lobby/00818A18.inc"
};
TalkSelLines gTalkSelects_110_0_2_0 = {
#include "lobby/00818A20.inc"
};
TalkSel gTalkSelects_110_0_2[1] = {
    {0, &gTalkSelects_110_0_2_0},
};
TalkSel gTalkSelects_110_0_1[1] = {
    {0, &gTalkSelects_110_0_1_0},
};
TalkSel gTalkSelects_110_0_0[1] = {
    {0, &gTalkSelects_110_0_0_0},
};
TalkSel gTalkSelects_110_0[3] = {
#include "lobby/00818A40.inc"
};
TalkSel gTalkSelects_110[1] = {
    {0, gTalkSelects_110_0},
};
TalkLine gTalkLines_110[16] = {
#include "lobby/00818A60.inc"
};
TalkEvent gTalkEvents_111[2] = {
#include "lobby/00818AA0.inc"
};
TalkSelLines gTalkSelects_111_0_0_0 = {
#include "lobby/00818AA8.inc"
};
TalkSel gTalkSelects_111_0_0[1] = {
    {0, &gTalkSelects_111_0_0_0},
};
TalkSel gTalkSelects_111_0[1] = {
    {0, gTalkSelects_111_0_0},
};
TalkSel gTalkSelects_111[2] = {
    {0, gTalkSelects_111_0},
    {0, NULL},
};
TalkLine gTalkLines_111[8] = {
#include "lobby/00818AD0.inc"
};
TalkEvent gTalkEvents_112[2] = {
#include "lobby/00818AF0.inc"
};
TalkSelLines gTalkSelects_112_0_0_0 = {
#include "lobby/00818AF8.inc"
};
TalkSel gTalkSelects_112_0_0[1] = {
    {0, &gTalkSelects_112_0_0_0},
};
TalkSel gTalkSelects_112_0[1] = {
    {0, gTalkSelects_112_0_0},
};
TalkSel gTalkSelects_112[2] = {
    {0, gTalkSelects_112_0},
    {0, NULL},
};
TalkLine gTalkLines_112[8] = {
#include "lobby/00818B20.inc"
};
TalkEvent gTalkEvents_113[4] = {
#include "lobby/00818B40.inc"
};
TalkSelLines gTalkSelects_113_0_0_0 = {
#include "lobby/00818B50.inc"
};
TalkSelLines gTalkSelects_113_0_0_1 = {
#include "lobby/00818B58.inc"
};
TalkSel gTalkSelects_113_0_0[2] = {
#include "lobby/00818B60.inc"
};
TalkSel gTalkSelects_113_0[1] = {
    {0, gTalkSelects_113_0_0},
};
TalkSel gTalkSelects_113[1] = {
    {0, gTalkSelects_113_0},
};
TalkLine gTalkLines_113[52] = {
#include "lobby/00818B80.inc"
};
TalkGift gTalkGifts_113[3] = {
#include "lobby/00818C50.inc"
};
TalkEvent gTalkEvents_114[2] = {
#include "lobby/00818C68.inc"
};
TalkSel gTalkSelects_114_0_0[1] = {
    {0, &gTalkSelects_114_0_0_0},
};
TalkSel gTalkSelects_114_0[1] = {
    {0, gTalkSelects_114_0_0},
};
TalkSel gTalkSelects_114[1] = {
    {0, gTalkSelects_114_0},
};
TalkLine gTalkLines_114[2] = {
#include "lobby/00818C88.inc"
};
TalkSel *gTalkSelects[116] = {
    NULL, gTalkSelects_1, gTalkSelects_2, gTalkSelects_3,
    gTalkSelects_4, gTalkSelects_5, gTalkSelects_6, gTalkSelects_7,
    gTalkSelects_8, gTalkSelects_9, gTalkSelects_10, gTalkSelects_11,
    gTalkSelects_12, gTalkSelects_13, gTalkSelects_14, gTalkSelects_15,
    gTalkSelects_16, NULL, gTalkSelects_18, gTalkSelects_19,
    NULL, gTalkSelects_21, gTalkSelects_22, gTalkSelects_23,
    gTalkSelects_24, gTalkSelects_25, gTalkSelects_26, gTalkSelects_27,
    gTalkSelects_28, gTalkSelects_29, gTalkSelects_30, gTalkSelects_31,
    gTalkSelects_32, gTalkSelects_33, gTalkSelects_34, gTalkSelects_35,
    gTalkSelects_36, gTalkSelects_37, gTalkSelects_38, gTalkSelects_39,
    gTalkSelects_40, gTalkSelects_41, gTalkSelects_42, gTalkSelects_43,
    gTalkSelects_44, gTalkSelects_45, gTalkSelects_46, gTalkSelects_47,
    gTalkSelects_48, gTalkSelects_49, gTalkSelects_50, gTalkSelects_51,
    gTalkSelects_52, gTalkSelects_53, gTalkSelects_54, gTalkSelects_55,
    gTalkSelects_56, gTalkSelects_57, gTalkSelects_58, gTalkSelects_59,
    gTalkSelects_60, gTalkSelects_61, gTalkSelects_62, gTalkSelects_63,
    gTalkSelects_64, gTalkSelects_65, gTalkSelects_66, gTalkSelects_67,
    gTalkSelects_68, gTalkSelects_69, gTalkSelects_70, gTalkSelects_71,
    gTalkSelects_72, gTalkSelects_73, gTalkSelects_74, gTalkSelects_75,
    gTalkSelects_76, gTalkSelects_77, gTalkSelects_78, gTalkSelects_79,
    gTalkSelects_80, gTalkSelects_81, gTalkSelects_82, gTalkSelects_83,
    gTalkSelects_84, gTalkSelects_85, gTalkSelects_86, gTalkSelects_87,
    gTalkSelects_88, gTalkSelects_89, gTalkSelects_90, gTalkSelects_91,
    gTalkSelects_92, gTalkSelects_93, gTalkSelects_94, gTalkSelects_95,
    gTalkSelects_96, gTalkSelects_97, gTalkSelects_98, gTalkSelects_99,
    gTalkSelects_100, gTalkSelects_101, gTalkSelects_102, gTalkSelects_103,
    gTalkSelects_104, gTalkSelects_105, gTalkSelects_106, gTalkSelects_107,
    gTalkSelects_108, gTalkSelects_109, gTalkSelects_110, gTalkSelects_111,
    gTalkSelects_112, gTalkSelects_113, gTalkSelects_114, NULL,
};
TalkLine *gTalkLines[116] = {
    NULL, gTalkLines_1, gTalkLines_2, gTalkLines_3,
    gTalkLines_4, gTalkLines_5, gTalkLines_6, gTalkLines_7,
    gTalkLines_8, gTalkLines_9, gTalkLines_10, gTalkLines_11,
    gTalkLines_12, gTalkLines_13, gTalkLines_14, gTalkLines_15,
    gTalkLines_16, NULL, gTalkLines_18, gTalkLines_19,
    NULL, gTalkLines_21, gTalkLines_22, gTalkLines_23,
    gTalkLines_24, gTalkLines_25, gTalkLines_26, gTalkLines_27,
    gTalkLines_28, gTalkLines_29, gTalkLines_30, gTalkLines_31,
    gTalkLines_32, gTalkLines_33, gTalkLines_34, gTalkLines_35,
    gTalkLines_36, gTalkLines_37, gTalkLines_38, gTalkLines_39,
    gTalkLines_40, gTalkLines_41, gTalkLines_42, gTalkLines_43,
    gTalkLines_44, gTalkLines_45, gTalkLines_46, gTalkLines_47,
    gTalkLines_48, gTalkLines_49, gTalkLines_50, gTalkLines_51,
    gTalkLines_52, gTalkLines_53, gTalkLines_54, gTalkLines_55,
    gTalkLines_56, gTalkLines_57, gTalkLines_58, gTalkLines_59,
    gTalkLines_60, gTalkLines_61, gTalkLines_62, gTalkLines_63,
    gTalkLines_64, gTalkLines_65, gTalkLines_66, gTalkLines_67,
    gTalkLines_68, gTalkLines_69, gTalkLines_70, gTalkLines_71,
    gTalkLines_72, gTalkLines_73, gTalkLines_74, gTalkLines_75,
    gTalkLines_76, gTalkLines_77, gTalkLines_78, gTalkLines_79,
    gTalkLines_80, gTalkLines_81, gTalkLines_82, gTalkLines_83,
    gTalkLines_84, gTalkLines_85, gTalkLines_86, gTalkLines_87,
    gTalkLines_88, gTalkLines_89, gTalkLines_90, gTalkLines_91,
    gTalkLines_92, gTalkLines_93, gTalkLines_94, gTalkLines_95,
    gTalkLines_96, gTalkLines_97, gTalkLines_98, gTalkLines_99,
    gTalkLines_100, gTalkLines_101, gTalkLines_102, gTalkLines_103,
    gTalkLines_104, gTalkLines_105, gTalkLines_106, gTalkLines_107,
    gTalkLines_108, gTalkLines_109, gTalkLines_110, gTalkLines_111,
    gTalkLines_112, gTalkLines_113, gTalkLines_114, NULL,
};
TalkEvent *gTalkEvents[116] = {
    NULL, gTalkEvents_1, gTalkEvents_2, gTalkEvents_3,
    gTalkEvents_4, gTalkEvents_5, gTalkEvents_6, gTalkEvents_7,
    gTalkEvents_8, gTalkEvents_9, gTalkEvents_10, gTalkEvents_11,
    gTalkEvents_12, gTalkEvents_13, gTalkEvents_14, gTalkEvents_15,
    gTalkEvents_16, NULL, gTalkEvents_18, gTalkEvents_19,
    NULL, gTalkEvents_21, gTalkEvents_22, gTalkEvents_23,
    gTalkEvents_24, gTalkEvents_25, gTalkEvents_26, gTalkEvents_27,
    gTalkEvents_28, gTalkEvents_29, gTalkEvents_30, gTalkEvents_31,
    gTalkEvents_32, gTalkEvents_33, gTalkEvents_34, gTalkEvents_35,
    gTalkEvents_36, gTalkEvents_37, gTalkEvents_38, gTalkEvents_39,
    gTalkEvents_40, gTalkEvents_41, gTalkEvents_42, gTalkEvents_43,
    gTalkEvents_44, gTalkEvents_45, gTalkEvents_46, gTalkEvents_47,
    gTalkEvents_48, gTalkEvents_49, gTalkEvents_50, gTalkEvents_51,
    gTalkEvents_52, gTalkEvents_53, gTalkEvents_54, gTalkEvents_55,
    gTalkEvents_56, gTalkEvents_57, gTalkEvents_58, gTalkEvents_59,
    gTalkEvents_60, gTalkEvents_61, gTalkEvents_62, gTalkEvents_63,
    gTalkEvents_64, gTalkEvents_65, gTalkEvents_66, gTalkEvents_67,
    gTalkEvents_68, gTalkEvents_69, gTalkEvents_70, gTalkEvents_71,
    gTalkEvents_72, gTalkEvents_73, gTalkEvents_74, gTalkEvents_75,
    gTalkEvents_76, gTalkEvents_77, gTalkEvents_78, gTalkEvents_79,
    gTalkEvents_80, gTalkEvents_81, gTalkEvents_82, gTalkEvents_83,
    gTalkEvents_84, gTalkEvents_85, gTalkEvents_86, gTalkEvents_87,
    gTalkEvents_88, gTalkEvents_89, gTalkEvents_90, gTalkEvents_91,
    gTalkEvents_92, gTalkEvents_93, gTalkEvents_94, gTalkEvents_95,
    gTalkEvents_96, gTalkEvents_97, gTalkEvents_98, gTalkEvents_99,
    gTalkEvents_100, gTalkEvents_101, gTalkEvents_102, gTalkEvents_103,
    gTalkEvents_104, gTalkEvents_105, gTalkEvents_106, gTalkEvents_107,
    gTalkEvents_108, gTalkEvents_109, gTalkEvents_110, gTalkEvents_111,
    gTalkEvents_112, gTalkEvents_113, gTalkEvents_114, NULL,
};
TalkGift *gTalkGifts[116] = {
    NULL, gTalkGifts_1, gTalkGifts_2, gTalkGifts_3,
    gTalkGifts_4, gTalkGifts_5, gTalkGifts_6, gTalkGifts_7,
    gTalkGifts_8, gTalkGifts_9, gTalkGifts_10, gTalkGifts_11,
    gTalkGifts_12, gTalkGifts_13, gTalkGifts_14, gTalkGifts_15,
    gTalkGifts_16, NULL, gTalkGifts_18, gTalkGifts_19,
    NULL, gTalkGifts_21, gTalkGifts_22, gTalkGifts_23,
    gTalkGifts_24, gTalkGifts_25, gTalkGifts_26, gTalkGifts_27,
    gTalkGifts_28, gTalkGifts_29, gTalkGifts_30, gTalkGifts_31,
    gTalkGifts_32, gTalkGifts_33, gTalkGifts_34, gTalkGifts_35,
    gTalkGifts_36, gTalkGifts_37, gTalkGifts_38, gTalkGifts_39,
    gTalkGifts_40, gTalkGifts_41, gTalkGifts_42, gTalkGifts_43,
    gTalkGifts_44, gTalkGifts_45, gTalkGifts_46, gTalkGifts_47,
    gTalkGifts_48, gTalkGifts_49, gTalkGifts_50, gTalkGifts_51,
    gTalkGifts_52, gTalkGifts_53, gTalkGifts_54, gTalkGifts_55,
    gTalkGifts_56, gTalkGifts_57, gTalkGifts_58, gTalkGifts_59,
    gTalkGifts_60, gTalkGifts_61, gTalkGifts_62, gTalkGifts_63,
    gTalkGifts_64, gTalkGifts_65, gTalkGifts_66, gTalkGifts_67,
    gTalkGifts_68, gTalkGifts_69, gTalkGifts_70, gTalkGifts_71,
    gTalkGifts_72, gTalkGifts_73, gTalkGifts_74, gTalkGifts_75,
    gTalkGifts_76, gTalkGifts_77, gTalkGifts_78, gTalkGifts_79,
    gTalkGifts_80, gTalkGifts_81, gTalkGifts_82, gTalkGifts_83,
    gTalkGifts_84, gTalkGifts_85, gTalkGifts_86, gTalkGifts_87,
    gTalkGifts_88, gTalkGifts_89, gTalkGifts_90, gTalkGifts_91,
    gTalkGifts_92, gTalkGifts_93, gTalkGifts_94, gTalkGifts_95,
    gTalkGifts_96, gTalkGifts_97, gTalkGifts_98, gTalkGifts_99,
    gTalkGifts_100, gTalkGifts_101, gTalkGifts_102, gTalkGifts_103,
    gTalkGifts_104, gTalkGifts_105, gTalkGifts_106, gTalkGifts_107,
    gTalkGifts_108, gTalkGifts_109, gTalkGifts_110, gTalkGifts_111,
    gTalkGifts_112, gTalkGifts_113, gTalkGifts_114, NULL,
};

extern s32 gTalkLineText;

/* The text of a talk line (Talk_LineTextAddr). */
char *Talk_LineText(u16 line)
{
    return (char *)Talk_LineTextAddr(line);
}


/* The kind of the current talk line (TALK_LINE_*). The callers pass the
 * line (u16), which it does not use: it reads gTalkLine. The pointer is
 * stepped to the line first (talks += gTalkLine), the order of the game's
 * address arithmetic. */
s32 Talk_LineKind(u16 unused)
{
    TalkLine *talks;

    talks = gTalkLines[gTalkScript];
    talks += gTalkLine;
    return talks->kind;
}

s32 Notice_OpenTextN(char *text, s32 width, s32 mode, s32 sel, s32 arg4); /* unit's own declaration: declared here with an s32 frame count, as the callers here pass it; with the definition's u32, Talk_RunText and Talk_Step compile two instructions differently each */

/* One frame of the current talk line's text, with no input (the line's
 * own text, or Talk_LineTextAddr's; gTalkTextFrames counts its frames, the
 * line's kind and the yes/no cursor gTalkMode choose the window).
 * Returns MENU_RUNNING while the text is still being shown, MENU_DONE once
 * it is all shown. */
s32 Talk_RunText(void)
{
    s32 text;

    LobbyPad_Get(0);
    if (gTalkTextFrames >= 0) {
        gTalkTextFrames++;
    }
    text = gTalkLineText != 0 ? gTalkLineText : Talk_LineTextAddr((u16)gTalkLine);
    if (Notice_OpenTextN((char *)text, gTalkTextFrames, Talk_LineKind(gTalkLine), gTalkMode, gTalkNpcName) == 0) {
        return MENU_DONE;
    }
    return MENU_RUNNING;
}


/*
 * One frame of the talk with the buttons. While the line's text is still
 * being shown, circle or cross shows it all at once (gTalkTextFrames -1).
 * Once it is shown:
 * - a TALK_LINE_NEXT line goes on to the next line with circle or cross;
 * - a TALK_LINE_ASK line moves the yes/no cursor (gTalkMode) with left and
 *   right; circle takes the cursor's answer, and cross answers no (the
 *   first press only moves the cursor to no). The answer skips ahead by
 *   the line's yesSkip or noSkip; with no skip the talk goes on to the
 *   next line, or ends when endOnAnswer is 1;
 * - any other line ends the talk.
 * Returns MENU_RUNNING while the talk goes on, MENU_DONE at its end.
 */
s32 Talk_Step(s8 endOnAnswer)
{
    u16 pad = LobbyPad_Get(0);
    s32 text;
    s32 kind;
    TalkLine *talks;
    s32 skip;

    if (gTalkTextFrames >= 0) {
        gTalkTextFrames++;
    }
    text = gTalkLineText != 0 ? gTalkLineText : Talk_LineTextAddr((u16)gTalkLine);
    kind = Talk_LineKind(gTalkLine);
    talks = gTalkLines[gTalkScript];
    talks += gTalkLine;
    if (Notice_OpenTextN((char *)text, gTalkTextFrames, kind, gTalkMode, gTalkNpcName) == 0) {
        if (pad & PAD_CIRCLE) {
            if (kind == TALK_LINE_NEXT) {
                gTalkTextFrames = 0;
                gTalkMode = TALK_CHOICE_YES;
                gTalkLine++;
            } else if (kind == TALK_LINE_ASK) {
                if (gTalkMode != TALK_CHOICE_NO) {
                    LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
                    if ((skip = talks->yesSkip) != 0) {
                        gTalkTextFrames = 0;
                        gTalkMode = TALK_CHOICE_YES;
                        gTalkLine += skip;
                    } else if (endOnAnswer == 1) {
                        gTalkTextFrames = 0;
                        return MENU_DONE;
                    } else {
                        gTalkTextFrames = 0;
                        gTalkMode = TALK_CHOICE_YES;
                        gTalkLine++;
                    }
                } else {
                    LobbySe_Play(LOBBY_SE_CANCEL);
                    if ((skip = talks->noSkip) != 0) {
                        gTalkTextFrames = 0;
                        gTalkMode = TALK_CHOICE_YES;
                        gTalkLine += skip;
                    } else if (endOnAnswer == 1) {
                        gTalkTextFrames = 0;
                        return MENU_DONE;
                    } else {
                        gTalkTextFrames = 0;
                        gTalkMode = TALK_CHOICE_YES;
                        gTalkLine++;
                    }
                }
            } else {
                return MENU_DONE;
            }
        } else if (pad & PAD_CROSS) {
            if (kind == TALK_LINE_NEXT) {
                gTalkTextFrames = 0;
                gTalkMode = TALK_CHOICE_YES;
                gTalkLine++;
            } else if (kind == TALK_LINE_ASK) {
                LobbySe_Play(LOBBY_SE_CANCEL);
                if (gTalkMode != TALK_CHOICE_NO) {
                    gTalkMode = TALK_CHOICE_NO;
                } else if ((skip = talks->noSkip) != 0) {
                    gTalkTextFrames = 0;
                    gTalkMode = TALK_CHOICE_YES;
                    gTalkLine += skip;
                } else if (endOnAnswer == 1) {
                    gTalkTextFrames = 0;
                    return MENU_DONE;
                } else {
                    gTalkTextFrames = 0;
                    gTalkMode = TALK_CHOICE_YES;
                    gTalkLine++;
                }
            } else {
                gTalkTextFrames = 0;
                return MENU_DONE;
            }
        } else if (kind == TALK_LINE_ASK) {
            if (pad & PAD_LEFT) {
                if (gTalkMode != TALK_CHOICE_YES) {
                    LobbySe_Play(LOBBY_SE_SCROLL);
                    gTalkMode = TALK_CHOICE_YES;
                }
            } else if (pad & PAD_RIGHT) {
                if (gTalkMode != TALK_CHOICE_NO) {
                    LobbySe_Play(LOBBY_SE_SCROLL);
                    gTalkMode = TALK_CHOICE_NO;
                }
            }
        }
    } else if (pad & (PAD_CIRCLE | PAD_CROSS)) {
        gTalkTextFrames = -1;
    }
    return MENU_RUNNING;
}


/* Talk_Step, ending on an answer. At the talk's end: MENU_CANCEL when the
 * last answer was no; otherwise the last line's gift is handed over (money,
 * with its notice; an item into the pouch, or a notice that it is full; a
 * piece of equipment into the box, or a notice that it is full) and
 * MENU_DONE. MENU_RUNNING while the talk goes on. */
s32 Talk_Update(void)
{
    TalkGift *given;
    TalkLine *talks;
    u8 giftIndex;
    s16 count;
    char msg[32];
    char wide[32];

    if (Talk_Step(1) == MENU_DONE) {
        if (gTalkMode == TALK_CHOICE_NO) {
            return MENU_CANCEL;
        }
        talks = gTalkLines[gTalkScript];
        talks += gTalkLine;
        giftIndex = talks->gift;
        if (giftIndex != TALK_GIFT_NONE) {
            given = gTalkGifts[gTalkScript];
            given += giftIndex;
            switch (given->kind) {
            case TALK_GIFT_MONEY:
                Save_AddMoney(given->a.value);
                LobbySe_Play(LOBBY_SE_MONEY);
                sprintf(msg, gTownLabels[TOWN_LABEL_ZENNY_GOT_FMT], given->a.value);
                Sjis_Widen(msg, wide);
                Notice_PushText(wide);
                break;
            case TALK_GIFT_ITEM:
                count = given->b.count;
                if (SavePouch_Space(given->a.item) >= count) {
                    SavePouch_Add(given->a.item, count);
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, given->a.value);
                } else {
                    Menu_PushNotice(LOBBY_NOTICE_POUCH_FULL);
                }
                break;
            case TALK_GIFT_EQUIP:
                if ((u16)Box_FindFreeSlot(&gSave) != BOX_NONE) {
                    Box_AddItem(&gSave, given->a.type, given->b.id, 0);
                    Menu_PushNotice(LOBBY_NOTICE_EQUIP_GOT);
                } else {
                    Menu_PushNotice(LOBBY_NOTICE_BOX_FULL);
                }
                break;
            }
        }
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/* Starts the talk's line `line`, the yes/no cursor at `choice`
 * (TALK_CHOICE_*), with the line's own text. */
void Talk_Say(s32 line, s8 choice)
{
    gTalkLine = line;
    gTalkMode = choice;
    gTalkLineText = 0;
    gTalkTextFrames = 0;
}
