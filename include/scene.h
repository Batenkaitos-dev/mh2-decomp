#ifndef SCENE_H
#define SCENE_H

#include "common.h"
#include "item.h"

/*
 * The scene state in sub_main's data at 0x6ADCB0 (gScene, 0x2BC bytes);
 * its first byte selects the scene. The struct is generated from
 * include/scene.spec (tools/gen_struct.py): add fields there.
 *
 * Read its fields as members of gScene: MWCC reloads a struct member
 * after every store through a pointer, as the game does, but assumes that
 * such stores leave a plain scalar global alone, which changes loops.
 */
typedef struct SceneState {
    u8 scene;                   /* 0x000 */
    u8 step;                    /* 0x001: the scene's step (Scene_StartQuest, NetScene_Run, ...) */
    u8 subStep;                 /* 0x002: the step inside a scene step (Scene_LoadQuestSounds, Scene_StartQuest); 0 on the first boot (Scene_Opening) */
    u8 openingFirst;            /* 0x003: 1 when the opening movie plays for the first time (event flag 0x26 not set yet; Scene_Opening), cleared on entering the field */
    s16 timer;                  /* 0x004: a scene timer (unk_3240E0.c SceneView, plsel PlselScene) */
    s16 timer2;                 /* 0x006: a second one (SceneView) */
    u8 unk008[0x00A - 0x008];
    s16 stateSyncFrame;         /* 0x00A: frames since the last state sync: at 60 Scene_QuestFrameTick sends QSync_SendState(8) and restarts it; zeroed when the quest results screen starts (Plsel_ResultsUpdate) */
    u8 unk00C[0x00D - 0x00C];
    u8 padRead;                 /* 0x00D: the pads are read each frame while set (Pad_Update; set by the scene start, and by the lobby) */
    u8 menuOpen;                /* 0x00E: the field or player menu is open (FieldMenu_Open, Plsel_OpenPlayerMenu); blocks the HUD input (Hud_InputBlocked) */
    u8 camControl;              /* 0x00F: the camera control option (option byte 4, copied with D_006A9E59): 0 L1/R1 change the distance, 2 reverses the turn (CamMode1 in unk_314E00.c) */
    u8 musicFlags;              /* 0x010: bit 0, bit 1: the music faded (the BGM tasks in unk_363550.c; was gSceneMusicFlags) */
    u8 musicState;              /* 0x011: the BGM state machine step (unk_363550.c; the game sets 5 with musicTimer 0x96; was gSceneMusicState) */
    u8 allLocal;                /* 0x012: in a drill, every player counts as local (Player_IsLocal); set by Scene_EnterField, cleared by plsel */
    u8 unk013[0x014 - 0x013];
    u16 area;                   /* 0x014: the current area (compared with Em.unk680 and Player.unk680; Quest_SetStartArea) */
    u16 startArea;              /* 0x016: the area loaded first (Quest_SetStartArea sets it with area; Quest_Setup: Area_Load) */
    u8 unk018[0x019 - 0x018];
    u8 charIndex;               /* 0x019: the save's character slot, copied from gQuestSync.charIndex at the scene reset (unk_3240E0.c) */
    u8 unk01A[0x020 - 0x01A];
    u16 frame;                  /* 0x020: +1 every field update (sub_main 0x3275F0) */
    u8 padIndex[2];             /* 0x022: per pad port, its gPadState index (Pad_ReadButtons; the scene reset sets 0 and 1); the login screen reads [0] (LoginPad_Triggered); was padPort */
    u8 unk024[0x025 - 0x024];
    u8 pauseState;              /* 0x025: the offline pause: 0 none, 1 requested, 2-3 paused, 4 leaving (Scene_RequestPause, Game_PauseState; the lobby's copies in unk_7C8FA0.c); was gPauseState, dayState */
    u8 heldMask;                /* 0x026: buttons held at a change of control, masked out until released (Pad_ReadButtons) */
    u8 camFollow;               /* 0x027: the camera follow option (option byte 3, copied with D_006A9E5A): 0 the camera swings behind the hunter (CamMode1 in unk_314E00.c) */
    u16 musicTimer;             /* 0x028: the BGM state machine's frame timer (unk_363550.c, compared with <= 0); the game sets 0x96 with musicState 5 (unk_70A290.c) */
    u8 emKind[4];               /* 0x02A: per monster slot, its kind (QuestEm_SpawnArea, Quest_SpawnMonsters from gQuestSync.emKind); 0 none */
    u16 questId;                /* 0x02E: the quest id (QuestDesk_Accept, Quest_Init; Tex_LoadStageFile(map, questId)) */
    u8 map;                     /* 0x030: the stage map: Map_AreaIndex(map, area), Area_BaseId, Tex_LoadStageFile, the per-map tables (Quest_Init: stage data word 0) */
    u8 campArea;                /* 0x031: the map's base camp: a fainted player goes back there (Player_State0_0B), the Felyne kind 0x12 spawns there (QuestEm_SpawnArea) (Quest_Init: stage data word 1) */
    u8 playerGender[4];         /* 0x032: per player, from gQuestSync (Quest_InitPlayers: Player.gender) */
    u8 unk036[0x03C - 0x036];
    u8 localPlayer;             /* 0x03C: the local player's index (gPlayers, per-player arrays) */
    u8 unk03D;                  /* 0x03D: set to 1 with online when the lobby starts an online quest as a guest (lobby unk_7C8FA0.c, NetScene_SetMode 1) */
    u8 playerCount;             /* 0x03E: players in the quest */
    u8 hitId;                   /* 0x03F: the last hit id (HitId_Next increments it, skipping 0; HitId_Reset; D_006ADCEF) */
    u8 questPhase;              /* 0x040: 0-2 before the start, 3/4 running, 5-7 ending, 8 (Quest_IsRunning) */
    u8 online;                  /* 0x041: nonzero online (the lobby overlay is loaded; the alias D_006ADCF1) */
    EquipItem weapon[4];        /* 0x042: each player's weapon, from gQuestSync (SceneView in unk_3240E0.c) */
    u8 unk072[0x0BA - 0x072];
    u8 presetEquip[4];          /* 0x0BA: per player: Player.presetEquip (Quest_InitPlayers), from gQuestSync.unk20 (Scene_StartQuest) */
    u8 unk0BE[0x0C4 - 0x0BE];
    s8 startItemList[4];        /* 0x0C4: per player, from gQuestSync (Quest_InitPlayers: Player.startItemList) */
    u8 unk0C8[0x0D0 - 0x0C8];
    void *models[4];            /* 0x0D0: model headers (0x80 bytes), copied per monster (EmSlot_SetupModel, Em +0x30F) */
    s16 modelCount[4];          /* 0x0E0: per monster slot, its models (EmSlot_LoadModel loads one) */
    s16 modelFirst[4];          /* 0x0E8: per monster slot, its first model slot */
    struct Model *partModels[4];/* 0x0F0: per monster slot, a second model set (EmSlot_FreeModels frees it with the first; Model, include/model.h) */
    s16 partCount[4];           /* 0x100: its models, -1: none */
    s16 partFirst[4];           /* 0x108: its first model slot */
    u8 itemUses[4];             /* 0x110: per player: the items used (Stats_CountItemUse) */
    u8 unk114[0x12C - 0x114];
    u16 deductedItem;           /* 0x12C: the item Quest_DeductReward took from the reward */
    s16 deductedCount;          /* 0x12E: its count */
    u8 rewardDeducted;          /* 0x130: set by Quest_DeductReward, cleared by Quest_Init */
    u8 aimReverse;              /* 0x131: the bowgun aim reversal option: 0 none, 1 up/down, 2 up/down and left/right (Player_BowgunAim) */
    u8 noticeId;                /* 0x132: the last notice id, 0-0x7F (Notice_ResetQueue, the Notice_Push functions in unk_37B570.c) */
    u8 bannerNotice;            /* 0x133: the notice the banner shows, 0xFF none (NoticeBanner_Spawn*, compared with gHudState dropNotice) */
    u8 unk134[0x138 - 0x134];
    s16 mealStamina;            /* 0x138: the meal's stamina bonus (Meal_ApplyEffects): added to the local player's maximum stamina at the start (Player_Reset) */
    s16 unk13A;                 /* 0x13A: compared with 0x1900-0x6400 for the quest goal kind 0xB (unk_3A1070.c) */
    u8 placed1;                 /* 0x13C: the local player placed objects of kind 1 (fewer than 2 allowed, Item_CanPlaceLargeBarrelBomb); the game decrements it when one disappears (game func_006D4920) */
    u8 placed3;                 /* 0x13D: the same for kind 3 (fewer than 3, Scene_FewPlaced3_Unk; game func_00868B00) */
    u8 placed2;                 /* 0x13E: the same for kind 2 (none allowed while set, Scene_CanPlaceKind2; game func_006D4920) */
    s8 fortStage;               /* 0x13F: 0 to 10, set by the fortress monsters from their scripts (em_laoshan, em_shengaoren, em_yamatsukami, em_kushala); QuestMenu_DrawRate prints (10 - fortStage) * 10 percent */
    ItemStack supply[48];       /* 0x140: the supply box, from the stage data (Scene_InitSupplyBox clears 48, the field draws 40, plsel counts 48) */
    u32 supplyTaken[2];         /* 0x200: one bit per supply box slot taken */
    u8 countdownOn;             /* 0x208: 1 while the countdown gQuest.countdown runs (Quest_StartCountdown, Quest_UpdateCountdown) */
    u8 emEventFlags;            /* 0x209: flags: bit 0 set by Shen Gaoren and Lao-Shan, bit 1 by Fatalis; area objects test them (unk_6EC680.c, unk_6F8A10.c) */
    u8 unk20A;                  /* 0x20A: 0 for quest 0xBBC, else 1 (AreaObj_K14_Create); the kind 0x14 object runs while set */
    u8 unk20B[0x20C - 0x20B];
    s16 syncDelay;              /* 0x20C: frames before a queued sync applies: the peers' mean delay, 1 to 30, default 8 (QSync_Begin, QSync_QueuePlayerPos) */
    u16 bgmId;                  /* 0x20E: the BGM playing (the BGM functions in unk_363550.c) */
    u8 unk210[0x224 - 0x210];
    u8 inQuest[4];              /* 0x224: per player: 1 in the quest, 0 left, 0xFF no player (NetScene_Run, QNetIf_FindHost, Player_Init) */
    u8 unk228[0x26A - 0x228];
    u16 supplyWho;              /* 0x26A: the player whose supply box request the host answers (QNet case 7; sent in message 10) */
    u16 supplySlot;             /* 0x26C: the supply slot granted to supplyWho, 0xFF: already taken (supplyTaken) */
    u8 host;                    /* 0x26E: the host player's index (QNetIf_FindHost: the first player in the quest) */
    u8 prevHost;                /* 0x26F: the host before (QNetIf_Update) */
    u8 questPaid;               /* 0x270: the quest fee was paid (QuestDesk_Accept, LobbyStep_6504: questPaid); plsel's rewards test it */
    u8 peersLeft;               /* 0x271: a bit per peer that left (QNetIf_OnPlayerLeft) */
    s8 mealAttack;              /* 0x272: the meal's attack buff (Meal_ApplyEffects); the local player's starting attackDrug */
    s8 mealDefense;             /* 0x273: the meal's defense buff (Meal_ApplyEffects); the local player's starting defenseDrug */
    s8 mealHealth;              /* 0x274: the meal's health bonus (Meal_ApplyEffects): added to the local player's maximum health at the start */
    u8 eventRunning;            /* 0x275: a scripted event (ScEvent_*) is running: monsters, HUD and Field_UpdateAll wait */
    s8 preset[4][8];            /* 0x276: a preset hunter id per player (SceneView, Player_LoadPreset); Scene_EnterField and Galex zero it as unk276[i * 8 + k] */
    u8 unk296[0x2B9 - 0x296];
    u8 plselOpen;               /* 0x2B9: 1 while the quest preparation screen (plsel) is up (Plsel_SelectInit; cleared by Plsel_SelectTask, Plsel_StateReady): QSync_OnChat logs chat there instead of Chat_LogAddLocal */
    u16 galexTimer;             /* 0x2BA: frames of the current Galex_Task step: zeroed, counted up, compared with the step limit (plsel unk_D74450.c; read there as unk276[0x2BA - 0x276] before) */
} SceneState;
typedef char SceneState_size_check[(sizeof(SceneState) == 0x2BC) ? 1 : -1];

extern SceneState gScene;

#endif /* SCENE_H */
