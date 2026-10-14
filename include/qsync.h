#ifndef QSYNC_H
#define QSYNC_H

#include "common.h"
#include "item.h"

/*
 * NETWORK: the game messages of the in-quest protocol (gm_sub qnet_sync.c;
 * preserve byte for byte for online play). The QSync_Send* functions build
 * one in a local buffer and post it (QNetIf_Post); QNetIf_OnMessage hands
 * each received one to its QSync_On* handler by cmd. Every message starts
 * with the same 4 bytes:
 *   +0 len    bytes sent, header included (the buffer can be longer)
 *   +1 flags  0 when built
 *   +2 kind   what the body holds (each message's kinds below; 0: nothing)
 *   +3 cmd    the message: 1-4 a player's state (the sender's index + 1),
 *             6 chat, 7 the supply box, 8 a monster's state, 10 the quest
 *             setup and requests
 * Each handler takes its message's type (QNetIf_OnMessage casts the buffer
 * by cmd) and reads the body through a pointer to it; the offsets in the
 * comments are from the message's start.
 */

/* sub_main's gQuestSync (0x6ADBC0, 0xE4 bytes; QuestSync_Clear zeroes it
 * and keeps charIndex): the quest's setup the players share (message 10
 * kind 2 sends a player's part of it, and player 0's shared values) and the
 * state of plsel's select screen (the preset hunters). The arrays of ten are
 * per player (Plsel_InitSlots fills all ten). */
typedef struct QuestSync {
    u8 unk00;
    u8 step;                    /* 0x01: plsel's sub-step (Plsel_Finish, Plsel_DebugMenu; 0 once every player is ready) */
    u8 cursor;                  /* 0x02: Plsel_DebugMenu: the monster group 0-3 it edits */
    u8 unk03;
    s16 bumpLeft;               /* 0x04: Plsel_StateChoose: the left arrow's bump frames (5); plsel_entry counts it to 60, then sends message 10 kind 8 */
    s16 bumpRight;              /* 0x06: the right arrow's */
    s16 timer;                  /* 0x08: plsel's wait frames (Plsel_WaitPlayersReady 0xE10, Plsel_Finish 0x1E) */
    u16 area;                   /* 0x0A: the start area outside a quest (gScene.area and startArea); player 0 sends it */
    u8 gender[10];              /* 0x0C: per player (gScene.playerGender; Plsel_InitSlots: odd players 1) */
    u8 unk16[10];               /* 0x16: per player: 4 (Plsel_InitSlots) */
    u8 presetEquip[10];         /* 0x20: per player (gScene.presetEquip) */
    EquipItem weapon[10];       /* 0x2A: per player (gScene.weapon; Plsel_InitSlots: gSave.equip[0], or an empty blade weapon) */
    s8 ready[10];               /* 0xA2: per player: 0 not ready, 1 ready, -1 not in the quest (Plsel_InitSlots, Plsel_WaitPlayersReady) */
    s8 startItemList[10];       /* 0xAC: per player (gScene.startItemList) */
    u16 emKind[4];              /* 0xB6: the kind of each monster group (gScene.emKind; Plsel_DebugMenu edits them) */
    u16 emCount[4];             /* 0xBE: the monsters of each group */
    u8 picks[4];                /* 0xC6: per player: the preset hunter it chose (message 10 kinds 12 and 13; bit 7: taken) */
    u8 unkCA[0xD0 - 0xCA];
    u8 charIndex;               /* 0xD0: the save's character slot (gScene.charIndex; select_chara.c sets it) */
    u8 presetSet;               /* 0xD1: the preset hunters' set (gPlselPresets; Plsel_PresetSetForStage) */
    u8 builtSet;                /* 0xD2: the set the preview players were built for (0xFF: none) */
    u8 unkD3;
    u16 yaw;                    /* 0xD4: the preview players' rotY (Plsel_StateChoose turns it) */
    u8 unkD6[0xD8 - 0xD6];
    u8 pick;                    /* 0xD8: the preset hunter under the local player's cursor (0-3; message 10 kinds 12 and 13) */
    u8 unkD9;
    u16 unkDA;                  /* 0xDA: player 0's (message 10 kind 2) */
    u16 unkDC;                  /* 0xDC: player 0's */
    s8 unkDE;                   /* 0xDE: player 0's */
    u8 unkDF;                   /* 0xDF: player 0's */
    u8 unkE0;                   /* 0xE0: player 0's */
    u8 unkE1;
    u16 pointRatePct;           /* 0xE2: the reward points' rate in percent: the taken preset's +0xC (Plsel_StateChoose); plsel unk_D67530.c applies it */
} QuestSync;
typedef char QuestSync_size_check[(sizeof(QuestSync) == 0xE4) ? 1 : -1];

extern QuestSync gQuestSync;

/* Messages 1-4, the body: a player's state (QSync_SendPlayer sends the
 * local player's, QSync_OnPlayer applies it to the sender's Player). */
typedef union QSyncPlayerData {
    struct {                /* kinds 1 and 9 (len 0x2F): the whole state */
        u16 icons;          /* 0x04: Player.statusIcons */
        u16 mode;           /* 0x06: 4: place the player at pos (else it walks there), 8: turn with unkA4 */
        f32 pos[3];         /* 0x08 */
        u16 angle;          /* 0x14 */
        u8 state;           /* 0x16 */
        u8 sub;             /* 0x17 */
        u16 area;           /* 0x18 */
        u16 stateRand;      /* 0x1A: Player.stateRand */
        s16 hp;             /* 0x1C */
        s16 stamina;        /* 0x1E */
        u8 chargeLevel;     /* 0x20: Player.chargeLevel */
        u8 carry;           /* 0x21: Player.carry */
        u8 sharpOrPitch;    /* 0x22: Player.sharpnessLevel (kind 1), Player.aimPitchPct (kind 9, the gunners) */
        s8 aimWeight;       /* 0x23: Player.aimWeight */
        s16 staminaMax;     /* 0x24 */
        s16 hpMax;          /* 0x26 */
        u16 useItem;        /* 0x28: Player.useItem */
        u16 attack;         /* 0x2A: Player.attack */
        u8 visible;         /* 0x2C: Player.visible */
        u8 weaponOut;       /* 0x2D: Player.weaponOut */
        u8 ammoType;        /* 0x2E */
    } k1;
    struct {                /* kinds 2 and 10 (len 0x1E, 0x1F): position and health */
        f32 pos[3];         /* 0x04 */
        u16 angle;          /* 0x10 */
        s16 hp;             /* 0x12 */
        s16 stamina;        /* 0x14 */
        u16 icons;          /* 0x16 */
        s16 hpMax;          /* 0x18 */
        s16 staminaMax;     /* 0x1A */
        u16 area;           /* 0x1C */
        u8 hornSong;        /* 0x1E: Player.hornSong (kind 10) */
    } k2;
    struct {                /* kinds 3-5 (len 0x17): at an action spot */
        f32 pos[3];         /* 0x04: Player.startPos */
        u16 angle;          /* 0x10: Player.startAngle (also the receiver's targetYaw) */
        u16 area;           /* 0x12: Player.nextArea (the receiver's area) */
        s16 mashLeft;       /* 0x14: Player.mashLeft */
        u8 carry;           /* 0x16: Player.carry */
    } k3;
    struct {                /* kind 6 (len 0x28): the state while handing over an item */
        u16 icons;          /* 0x04 */
        u16 mode;           /* 0x06: as k1.mode */
        f32 pos[3];         /* 0x08 */
        u16 angle;          /* 0x14 */
        u8 state;           /* 0x16 */
        u8 sub;             /* 0x17 */
        u16 unk18;
        u16 stateRand;      /* 0x1A: Player.stateRand */
        u8 tradeWith;       /* 0x1C */
        u8 unk1D;
        u16 tradeItem;      /* 0x1E */
        u16 tradeCount;     /* 0x20 */
        u16 useItem;        /* 0x22: Player.useItem */
        s16 hpMax;          /* 0x24 */
        u8 weaponOut;       /* 0x26: Player.weaponOut */
        u8 ammoType;        /* 0x27 */
    } k6;
    struct {                /* kinds 7 (ask for an item) and 8 (the answer), len 0xC */
        u8 result;          /* 0x04: kind 8: 1: given, 2: refused */
        u8 unk5;
        u8 asker;           /* 0x06: the player who asks for the item */
        u8 giver;           /* 0x07: the player asked */
        u16 item;           /* 0x08 */
        s16 count;          /* 0x0A */
    } k7;
    u8 raw[0x3C];
} QSyncPlayerData;
typedef char QSyncPlayerData_size_check[(sizeof(QSyncPlayerData) == 0x3C) ? 1 : -1];

/* Messages 1-4 (cmd: the sender's index + 1; kinds 1-10), 0x40 bytes. */
typedef struct QSyncPlayerMsg {
    u8 len;                     /* 0x00 */
    u8 flags;                   /* 0x01 */
    u8 kind;                    /* 0x02: 1-10 */
    u8 cmd;                     /* 0x03: the sender's index + 1 */
    QSyncPlayerData u;          /* 0x04 */
} QSyncPlayerMsg;
typedef char QSyncPlayerMsg_size_check[(sizeof(QSyncPlayerMsg) == 0x40) ? 1 : -1];

/* Message 6, the body: a chat text (QSync_SendChat sends it, QSync_OnChat
 * shows it to the players in `to`). */
typedef struct QSyncChat {
    u8 text[0x40];              /* 0x04: Shift-JIS, 0-terminated */
    u8 n;                       /* 0x44: the text's length */
    u8 who;                     /* 0x45: the sender */
    u8 to;                      /* 0x46: the players it is for (one bit each) */
    u8 unk47;
} QSyncChat;
typedef char QSyncChat_size_check[(sizeof(QSyncChat) == 0x44) ? 1 : -1];

/* Message 6, chat: kind 1, len 0x48. */
typedef struct QSyncChatMsg {
    u8 len;                     /* 0x00: 0x48 */
    u8 flags;                   /* 0x01 */
    u8 kind;                    /* 0x02: 1 */
    u8 cmd;                     /* 0x03: 6 */
    QSyncChat c;                /* 0x04 */
} QSyncChatMsg;
typedef char QSyncChatMsg_size_check[(sizeof(QSyncChatMsg) == 0x48) ? 1 : -1];

/* The supply box request and answer: message 7's body (kind 1 asks the
 * host, kind 2 answers) and message 10's for its kinds 7 and 10. */
typedef struct QSyncSupply {
    u8 who;                     /* 0x04: the sender */
    u8 answer;                  /* 0x05: 0 in the request, 1 in the answer */
    u8 taker;                   /* 0x06: the answer: gScene.supplyWho, the player who asked (it gets the item) */
    u8 slot;                    /* 0x07: the request: the slot (Player.menuCursor); the answer: gScene.supplySlot (0xFF: taken already) */
    s32 taken[2];               /* 0x08: the answer: gScene.supplyTaken (ORed into the receiver's) */
} QSyncSupply;
typedef char QSyncSupply_size_check[(sizeof(QSyncSupply) == 0xC) ? 1 : -1];

/* Message 7, the supply box (QSync_SendSupply builds it, QSync_OnSupply
 * handles it): kinds 1 and 2, len 0x10, in a 0x20-byte buffer. */
typedef struct QSyncSupplyMsg {
    u8 len;                     /* 0x00: 0x10 */
    u8 flags;                   /* 0x01 */
    u8 kind;                    /* 0x02: 1 or 2 */
    u8 cmd;                     /* 0x03: 7 */
    QSyncSupply s;              /* 0x04 */
    u8 pad10[0x10];             /* 0x10: never sent */
} QSyncSupplyMsg;
typedef char QSyncSupplyMsg_size_check[(sizeof(QSyncSupplyMsg) == 0x20) ? 1 : -1];

/* The parts of a monster in message 8 (QSync_PackEmParts, QSync_UnpackEmParts). */
typedef struct QSyncEmParts {
    u8 state[8];                /* 0x00: Em.parts[i].state */
    s16 hp[9];                  /* 0x08: Em.parts[i].hp */
} QSyncEmParts;
typedef char QSyncEmParts_size_check[(sizeof(QSyncEmParts) == 0x1A) ? 1 : -1];

/* Message 8, the body: a monster's state (QSync_SendEm sends it from the
 * console that runs the monster, QSync_OnEm applies it to gEms[index]).
 * status: bit 0 poisoned (Em.statusFlags bit 2), 1 flashed (Em.blindTimer),
 * 2 and 3 Em.syncPulse[0] and [1], 4 painted, 5 enraged, 6 and 7 a script
 * restart (Em.syncRequest 1, with Em_TypeSetup, and 2). state0 and state1
 * are QSync_PackEmState's: Em.versusGauge in a versus quest, else the
 * work area's first two bytes for the fort monsters (types 7, 0x32, 0x37,
 * 0x3A, 0x36, 0x3C, 0x40, 0x41: Em.work and fortStage), its bytes 0x44 and
 * 0x45 for types 0xF and 0x2D. */
typedef union QSyncEmData {
    struct {                /* kinds 1 (len 0x48) and 2 (len 0x46, no parts): the state */
        f32 pos[3];         /* 0x04 */
        s16 rot[3];         /* 0x10 */
        u8 stateGroup;      /* 0x16: kind 1: Em.syncKind, kind 2: Em.stateGroup */
        u8 state;           /* 0x17: kind 1: Em.syncArg, kind 2: Em.state */
        u8 index;           /* 0x18: the monster's index in gEms */
        u8 goalNo;          /* 0x19: Em.syncGoalNo (the receiver's curGoalNo) */
        u16 goalKind;       /* 0x1A: Em.syncGoalKind */
        u16 goalArg;        /* 0x1C: Em.syncGoalArg */
        s16 timer;          /* 0x1E: Em.timer */
        u8 area;            /* 0x20: the area's index in the map (Map_AreaIndex) */
        u8 combat;          /* 0x21: Em.combat */
        u16 rand;           /* 0x22: Em.syncRand (the receiver's stateRand) */
        s16 hp;             /* 0x24 */
        u8 status;          /* 0x26: the bits above */
        u8 state0;          /* 0x27 */
        s8 state1;          /* 0x28 */
        u8 from;            /* 0x29: the sender (applied only while gScene.inQuest[from] is 1) */
        u8 prevArea;        /* 0x2A: Em.prevArea's index in the map, 0xFF: none */
        u8 flags;           /* 0x2B: bit 1: Em.blind */
        u16 partsBroken;    /* 0x2C: Em.partsBroken (Em_BreakParts) */
        QSyncEmParts parts; /* 0x2E: kind 1 */
    } k1;
    struct {                /* kind 3 (len 0x46): the state, from Em_SyncState's values */
        f32 pos[3];         /* 0x04 */
        s16 rot[3];         /* 0x10 */
        u8 stateGroup;      /* 0x16: Em.syncKind */
        u8 state;           /* 0x17: Em.syncArg */
        u8 status;          /* 0x18 */
        u8 index;           /* 0x19 */
        s16 hp;             /* 0x1A */
        u16 rand;           /* 0x1C: Em.syncRand (the receiver's stateRand and syncRand) */
        s16 timer;          /* 0x1E */
        u16 goalKind;       /* 0x20: Em.syncGoalKind */
        u16 goalArg;        /* 0x22: Em.syncGoalArg */
        u8 goalNo;          /* 0x24: Em.syncGoalNo */
        u8 combat;          /* 0x25 */
        u8 state0;          /* 0x26 */
        s8 state1;          /* 0x27 */
        u8 from;            /* 0x28 */
        u8 flags;           /* 0x29 */
        u16 partsBroken;    /* 0x2A */
        QSyncEmParts parts; /* 0x2C */
    } k3;
    struct {                /* kind 4 (len 0x46): the monster handed to another console */
        f32 pos[3];         /* 0x04: not applied to types 2, 0x24, 0x47 */
        s16 rot[3];         /* 0x10 */
        s16 hp;             /* 0x16: not applied when syncKind is 5 */
        u8 hostNo;          /* 0x18: Em.hostNo, the console that runs it now */
        u8 index;           /* 0x19 */
        u8 area;            /* 0x1A */
        u8 painted;         /* 0x1B: Em.painted */
        s16 paintTimer;     /* 0x1C: Em.paintTimer */
        u8 prevArea;        /* 0x1E */
        u8 goalNo;          /* 0x1F: Em.goalNo (the receiver's goalNo and curGoalNo) */
        u16 goalKind;       /* 0x20: Em.goalKind */
        u16 goalArg;        /* 0x22: Em.goalArg */
        u8 state0;          /* 0x24 */
        s8 state1;          /* 0x25 */
        u8 flags;           /* 0x26 */
        u8 syncKind;        /* 0x27: Em.syncKind */
        s16 poisonDamage;   /* 0x28: Em.poisonDamage */
        u16 partsBroken;    /* 0x2A */
        QSyncEmParts parts; /* 0x2C */
    } k4;
} QSyncEmData;
typedef char QSyncEmData_size_check[(sizeof(QSyncEmData) == 0x44) ? 1 : -1];

/* Message 8, a monster's state (kinds 1-4), 0x48 bytes. */
typedef struct QSyncEmMsg {
    u8 len;                     /* 0x00: 0x48 (kind 1), 0x46 */
    u8 flags;                   /* 0x01 */
    u8 kind;                    /* 0x02: 1-4 */
    u8 cmd;                     /* 0x03: 8 */
    QSyncEmData u;              /* 0x04 */
} QSyncEmMsg;
typedef char QSyncEmMsg_size_check[(sizeof(QSyncEmMsg) == 0x48) ? 1 : -1];

/* Message 10, the body (QSync_SendState builds it for the local player,
 * QSync_OnState applies it). */
typedef union QSyncStateData {
    struct {                /* kind 1 (len 8): from the host */
        u8 count;           /* 0x04: gScene.playerCount */
    } k1;
    struct {                /* kind 2 (len 0x22): the sender's part of gQuestSync, and player 0's shared values */
        u8 who;             /* 0x04 */
        u8 presetEquip;     /* 0x05: gQuestSync.presetEquip[who] */
        u8 weaponType;      /* 0x06: gQuestSync.weapon[who].type */
        u8 weaponLevel;     /* 0x07: gQuestSync.weapon[who].level */
        u16 weaponId;       /* 0x08: gQuestSync.weapon[who].id */
        u16 area;           /* 0x0A: gQuestSync.area (player 0's) */
        u8 ready;           /* 0x0C: gQuestSync.ready[who] */
        u8 gender;          /* 0x0D: gQuestSync.gender[who] */
        u16 stageId;        /* 0x0E: NetQuest_SetStage's id at 0x6ADB48 (player 0's) */
        u8 pick;            /* 0x10: gQuestSync.picks[who] */
        u8 unk11;           /* 0x11: gQuestSync.unkDE (player 0's) */
        u16 unk12;          /* 0x12: gQuestSync.unkDA (player 0's) */
        u16 unk14;          /* 0x14: gQuestSync.unkDC (player 0's) */
        u8 unk16;           /* 0x16: gQuestSync.unkE0 (player 0's) */
        u8 unk17;           /* 0x17: gQuestSync.unkDF (player 0's) */
        u8 startItemList;   /* 0x18: gQuestSync.startItemList[who] */
        u8 unk19;
        s8 preset[8];       /* 0x1A: the sender's preset hunters (into gScene.preset[who]) */
    } k2;
    struct {                /* kind 6 (len 0xE): a quest event, gQuest's 10-byte net message (Quest_HandleNetMsg) */
        u8 from;            /* 0x04: Quest.msgFrom */
        u8 type;            /* 0x05: Quest.msgType */
        u16 arg0;           /* 0x06: Quest.msgArg0 */
        s16 arg1;           /* 0x08: Quest.msgArg1 */
        s16 arg2;           /* 0x0A: Quest.msgArg2 */
        s16 area;           /* 0x0C: Quest.msgArea */
    } k6;
    QSyncSupply supply;     /* kinds 7 (to the host) and 10 (its answer), len 0x10: as message 7 */
    struct {                /* kinds 8, 9, 12 and 13 (len 8) */
        u8 who;             /* 0x04: the sender (kind 9: Hud_StartMapPing's player) */
        u8 pick;            /* 0x05: kinds 12 and 13: the sender's choice (gQuestSync +0xD8; kind 12 with bit 7 set) */
    } k12;
} QSyncStateData;
typedef char QSyncStateData_size_check[(sizeof(QSyncStateData) == 0x20) ? 1 : -1];

/* Message 10 (kinds 1, 2, 6-10, 12, 13), 0x24 bytes (len at most 0x22). */
typedef struct QSyncStateMsg {
    u8 len;                     /* 0x00 */
    u8 flags;                   /* 0x01 */
    u8 kind;                    /* 0x02 */
    u8 cmd;                     /* 0x03: 10 */
    QSyncStateData u;           /* 0x04 */
} QSyncStateMsg;
typedef char QSyncStateMsg_size_check[(sizeof(QSyncStateMsg) == 0x24) ? 1 : -1];

#endif /* QSYNC_H */
