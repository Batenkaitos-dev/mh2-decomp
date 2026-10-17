# The work area of the player select overlay (0x3B88 bytes, cleared by
# Plsel_ClearWork), reached through the pointer at 0x6AE018 (gPlselWork in the
# overlay's C). Regenerate with: python tools/gen_struct.py include/plsel.spec include/plsel.h
name PlselWork
size 0x3B88
0x0000 s32 unk0000
0x0004 s32 moneyTotal // the money paid (all four sums; Save_AddMoney); t[4]
0x0008 s32 questMoney // the quest reward: fees and goal rewards (t[1], t[20])
0x000C s32 monsterMoney // the monsters' trade price total (t[2], t[11])
0x0010 s32 extraMoney // added to the total; not set here
0x0014 s32 itemMoney // the items sold (t[3], t[24])
0x0018 s32 questMoneyCut // 1: the goal reward differs from the quest's, or the 60% cut online
0x001C s32 monsterMoneyCut // 1: online with others, or the 60% cut (monsterMoney * 60 / 100)
0x0020 s32 mustOrder // 1: a must-order quest (the sub goals fold into the main one)
0x0024 s32 mainDone // the main goal is done (QuestGoal_IsDone(0))
0x0028 s32 subADone // sub goal A is done
0x002C s32 subBDone // sub goal B is done
0x0030 u16 recordGroups[0x4A] // monster groups with a new record (Plsel_AddRecordGroup)
0x00C4 u16 largestNew[0x4A] // per group: 1 or 2 when the largest record is new
0x0158 u16 smallestNew[0x4A] // the same for the smallest
0x01EC s32 recordGroupCount // entries of recordGroups
0x01F0 s32 recordGroupCur // the recordGroups entry shown
0x01F4 u16 newRecord // 1: some size record is new
0x01F6 u16 oldLargest // the shown group's largest record before (a size percent)
0x01F8 u16 oldSmallest // its smallest record before
0x01FA u16 newLargest // its largest size of the quest (0: none)
0x01FC u16 newSmallest // its smallest size of the quest
0x0200 f32 oldLargestCm // oldLargest in cm (Plsel_BuildResultText)
0x0204 f32 oldSmallestCm // oldSmallest in cm
0x0208 f32 newLargestCm // newLargest in cm (t[29])
0x020C f32 newSmallestCm // newSmallest in cm (t[30])
0x0210 u16 shownGroup // the monster group shown
0x0212 s8 unk0212
0x0213 s8 unk0213
0x033C u16 largestBefore[0x4A] // largest record per group before the quest
0x03D0 u16 smallestBefore[0x4A] // smallest record per group before the quest
0x0464 u16 huntedByGroup[0x4A] // monsters hunted in the quest, by kind group (gQuest.hunted)
0x04F8 u16 capturedByGroup[0x4A] // monsters captured, by kind group (gQuest.captured)
0x058C u16 huntedLeft[0x4A] // copies of huntedByGroup, used up by the hunt goals' bonuses
0x0620 u16 capturedLeft[0x4A] // copies of capturedByGroup, the same
0x06B4 s32 page // the results page (Plsel_BuildResultText's kind); the menus' cursor
0x06B8 s32 pageCount // the results pages
0x06BC s32 timeout // frames until the screen moves on by itself (0xE10: a minute)
0x06C0 s8 closeState // 0 open, 1 close asked, 2 closing (PlselRes_Close)
0x06C1 s8 closeTimer // frames left of the closing
0x06C2 s8 unk06C2
0x06C3 s8 menuStep // the record slot menu's step (Plsel_SaveRecordMenu)
0x06C4 s8 recordSlot // the record slot chosen
0x06C5 s8 help1 // help line 1 (0: none; gArenaEmPenHelp)
0x06C6 s8 help2 // help line 2
0x06C8 s32 recordCount // arena records saved (gSave.arenaEms[].unkA != 0, at most 3)
0x06CC s32 friendChanged // the arena monster's friendliness changed (t[39] lit)
0x06D0 s32 strengthChanged // its strength changed (t[40] lit)
0x06D4 s32 sizeChanged // its size changed (t[41] lit)
0x06D8 ArenaEm{28} arenaBefore // the arena monster before the fight (t[37])
0x06F4 s32 fee // the contract fee back (t[15])
0x06F8 s32 wtdFee // the WTD contract fee (t[16])
0x06FC f32 feeRate // fee multiplier (2.0 or 1.0; str[1] shown when not 1)
0x0700 f32 wtdFeeRate // wtdFee multiplier (2.0 when wtdDouble)
0x0704 s32 mainMoney // the main goal's reward (t[17]; t[52] must-order)
0x0708 s32 subAMoney // sub goal A's reward (t[18])
0x070C s32 subBMoney // sub goal B's reward (t[19])
0x0710 s32 deaths // the local player's deaths (t[47])
0x0714 s32 hrpTotal // hunter rank points in all (t[48]; Hr_AddPoints)
0x0718 s32 hrpDeaths // the deaths' points (deaths * -20)
0x071C s32 hrpMain // the main goal's points (t[44]; t[53] must-order; t[51] failed)
0x0720 s32 hrpSubA // sub goal A's points (t[45])
0x0724 s32 hrpSubB // sub goal B's points (t[46])
0x072C s32 rankBefore // hunter rank before (Save_RankFromPoints)
0x0730 s32 rankAfter // hunter rank after (t[50])
0x0734 s32 rankChange // 1: rank up, 2: down
0x0738 s32 hrPoints // gSave.hrPoints after (t[49])
0x073C s32 hrpResult // Hr_AddPoints' result (which note to show)
0x0748 s32 unk0748
0x074C s32 scroll // the first line shown on the scrolling pages
0x0750 s32 lineCount // text lines written
0x0754 s32 firstLine // first item line shown (page 3, seven lines)
0x0758 char lines[80][0x4C] // text lines (Plsel_BuildResultText)
0x1F18 PlselList{6756} rewList // the rewards list
0x397C s32 noticeCount // entries of notices
0x3980 s32 noticePage // the notices page shown (three a page)
0x3984 u16 notices[0x39] // notices to show: unlocks (Plsel_QueueFlagUnlocks), 0x3nn, 0x400, 0x500
0x39F8 s32 itemLines // item lines (Plsel_TallyRewards)
0x39FC s32 awardCount // entries of awards (at most 0x20)
0x3A00 u16 awards[0x20] // awards to show (Plsel_QueueAward)
0x3A40 u16 questLargest[0x50] // largest size of the quest, per kind then per group
0x3AE0 u16 questSmallest[0x50] // smallest size of the quest
0x3B80 u8 lowRankCut // 1: rank over 30 on a quest under 5 stars: 60% of the points and money
0x3B81 u8 wtdDouble // 1: the WTD fee counts double (Plsel_SumRecordRewards)
0x3B82 u8 unk3B82
0x3B83 u8 unk3B83
0x3B84 s32 pointRate // the points' percent (Plsel_ResetPointRate)
