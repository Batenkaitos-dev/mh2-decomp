#ifndef MSG_EVENT_H
#define MSG_EVENT_H

/*
 * The quest's advice messages (the game overlay's MsgEvent_* functions).
 * A message event is a moment of play that may show an advice message:
 * MsgEvent_Check(kind) picks the message for the quest being played, once
 * per save (each message has an event flag), and the popup shows it. The
 * field menu lists the quest set's messages already seen.
 * The kinds are read from MsgEvent_Check's switch and from the code that
 * calls it with each one.
 */
enum MsgEventKind {
    MSGEV_QUEST_START = 0,          /* the quest starts (Quest_Init) */
    MSGEV_SUPPLY_BOX = 1,           /* the local player stands at the supply box */
    MSGEV_SUPPLY_BOX_CLOSED = 2,    /* the player closes the supply box (SupplyBox_Update) */
    MSGEV_DELIVERY_BOX = 3,         /* the local player at the delivery box */
    MSGEV_BED = 4,                  /* the local player at the bed */
    MSGEV_JUNGLE_1 = 5,             /* entering the jungle's area 1, day or night */
    MSGEV_JUNGLE_2 = 6,             /* entering the jungle's area 2 */
    MSGEV_JUNGLE_3 = 7,             /* entering the jungle's area 3 */
    MSGEV_JUNGLE_4 = 8,             /* entering the jungle's area 4 */
    MSGEV_JUNGLE_8 = 9,             /* entering the jungle's area 8 */
    MSGEV_JUNGLE_6 = 10,            /* entering the jungle's area 6 */
    MSGEV_FORTRESS_0 = 11,          /* entering the fortress's area 0 */
    MSGEV_APCEROS_HUNTED = 12,      /* an Apceros is hunted */
    MSGEV_MINING = 13,              /* the player gathers at a mining point */
    MSGEV_CAMP = 14,                /* the local player in the base camp (Quest_CheckCampAdvice) */
    MSGEV_GOAL_DELIVERED = 15,      /* a quest goal is met by a delivery */
    MSGEV_CARRY_ITEM = 16,          /* a gathered item of a carried kind goes to the pouch */
    MSGEV_GOAL_KILLED = 17,         /* a quest goal is met by a kill */
    MSGEV_GOAL_CAPTURED = 18        /* a quest goal is met by a capture */
};

/* Message n of the advice messages has the event flag MSGEV_SEEN_FLAG_BASE + n,
 * set once it has been shown. */
#define MSGEV_SEEN_FLAG_BASE 200

void Quest_CheckCampAdvice(void);
void MsgEvent_Init(void);
void MsgEvent_OpenPopup(void);
s32 MsgEvent_OpenList(void);
s32 MsgEvent_ListInput(u16 pad);
void MsgEvent_DrawPopup(void);
void MsgEvent_DrawList(void);
void MsgEvent_DrawMessage(u8 inList, s32 windowOfs);
void MsgEvent_DrawPageCounter(s16 x, s16 y);
void MsgEvent_DrawIcon(s32 unused, s32 windowOfs);
void MsgEvent_CopyQuestTitle(s8 *buf);
void MsgEvent_Draw(void);
s32 MsgEvent_Check(u16 kind);
s32 MsgEvent_CheckQuest(s32 flag, s32 questId, s8 msgNo);
s32 MsgEvent_CheckSupplyBoxClosed(void);
u8 MsgEvent_CountInSet(u8 msgSet);
s32 MsgEvent_IsSeen(u8 msgSet, u8 msgNo);
char *MsgEvent_Title(u8 msgSet, u8 msgNo);
char *MsgEvent_Line(u8 msgSet, u8 msgNo, u8 pageNo);
u8 MsgEvent_PageCount(u8 msgSet, u8 msgNo);

#endif
