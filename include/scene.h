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
    s16 timer;                  /* 0x004: a scene timer (scene.c SceneView, plsel PlselScene) */
    s16 timer2;                 /* 0x006: a second one (SceneView) */
    s16 timeout;                /* 0x008: a third timer: the drill rewards list's timeout, frames (Plsel_RewardListTask) */
    s16 stateSyncFrame;         /* 0x00A: frames since the last state sync: at 60 Scene_QuestFrameTick sends QSync_SendState(8) and restarts it; zeroed when the quest results screen starts (Plsel_ResultsUpdate) */
    u8 unk00C[0x00D - 0x00C];
    u8 padRead;                 /* 0x00D: the pads are read each frame while set (Pad_Update; set by the scene start, and by the lobby) */
    u8 menuOpen;                /* 0x00E: the field or player menu is open (FieldMenu_Open, Plsel_OpenPlayerMenu); blocks the HUD input (Hud_InputBlocked) */
    u8 camControl;              /* 0x00F: the camera control option (option byte 4, copied with gSysSave.camControl): 0 L1/R1 change the distance, 2 reverses the turn (CamMode1 in kbd.c) */
    u8 musicFlags;              /* 0x010: MUSIC_FLAG_*: an area's own tune has played, the music is turned down under a tune */
    u8 musicState;              /* 0x011: the music's state: in the field an enum MusicState (Bgm_FieldUpdate), in the town MUSIC_TOWN_* (Bgm_TownUpdate); the hunters' actions set MUSIC_MONSTER_BATTLE with musicTimer 0x96 */
    u8 allLocal;                /* 0x012: in a drill, every player counts as local (Player_IsLocal); set by Scene_EnterField, cleared by plsel */
    u8 unk013[0x014 - 0x013];
    u16 area;                   /* 0x014: the current area (compared with Em.unk680 and Player.unk680; Quest_SetStartArea) */
    u16 startArea;              /* 0x016: the area loaded first (Quest_SetStartArea sets it with area; Quest_Setup: Area_Load) */
    u8 unk018[0x019 - 0x018];
    u8 charIndex;               /* 0x019: the save's character slot, copied from gQuestSync.charIndex at the scene reset (scene.c) */
    u8 unk01A[0x020 - 0x01A];
    u16 frame;                  /* 0x020: +1 every field update (sub_main 0x3275F0) */
    u8 padIndex[2];             /* 0x022: per pad port, its gPadState index (Pad_ReadButtons; the scene reset sets 0 and 1); the login screen reads [0] (LoginPad_Triggered); was padPort */
    u8 unk024[0x025 - 0x024];
    u8 pauseState;              /* 0x025: the offline pause: 0 none, 1 requested, 2-3 paused, 4 leaving (Scene_RequestPause, Game_PauseState; the lobby keeps copies of the pause functions) */
    u8 heldMask;                /* 0x026: buttons held at a change of control, masked out until released (Pad_ReadButtons) */
    u8 camFollow;               /* 0x027: the camera follow option (option byte 3, copied with gSysSave.camFollow): 0 the camera swings behind the hunter (CamMode1 in kbd.c) */
    u16 musicTimer;             /* 0x028: the frames left in musicState's state (Bgm_FieldUpdate: it moves on at 0 or less) */
    u8 emKind[4];               /* 0x02A: per monster slot, its kind (QuestEm_SpawnArea, Quest_SpawnMonsters from gQuestSync.emKind); 0 none */
    u16 questId;                /* 0x02E: the quest id (QuestDesk_Accept, Quest_Init; Tex_LoadStageFile(map, questId)) */
    u8 map;                     /* 0x030: the stage map: Map_AreaIndex(map, area), Map_DayId, Tex_LoadStageFile, the per-map tables (Quest_Init: stage data word 0) */
    u8 campArea;                /* 0x031: the map's base camp: a fainted player goes back there (Player_StateThrowItem), the Felyne kind 0x12 spawns there (QuestEm_SpawnArea) (Quest_Init: stage data word 1) */
    u8 playerGender[4];         /* 0x032: per player, from gQuestSync (Quest_InitPlayers: Player.gender) */
    u8 unk036[0x03C - 0x036];
    u8 localPlayer;             /* 0x03C: the local player's index (gPlayers, per-player arrays) */
    u8 unk03D;                  /* 0x03D: set to 1 with inLobby when NetScene_Run enters the lobby from another scene (before its mode 1) */
    u8 playerCount;             /* 0x03E: players in the quest */
    u8 hitId;                   /* 0x03F: the last hit id (HitId_Next increments it, skipping 0; HitId_Reset) */
    u8 questPhase;              /* 0x040: QUEST_PHASE_*: 2 the hunt, 3/4 the goal met, 5-7 lost, 8 quit (Quest_GetOutcome) */
    u8 inLobby;                 /* 0x041: nonzero while a scene of the lobby overlay runs: the network set-up and the online town (NetScene_Run), or plsel's tour of the town. 0 in every quest, online or not: the game overlay loads at the lobby's addresses, and Player_HasSkill calls the lobby's Town_InMode46 while it is set. Online play is Game_IsOnline */
    EquipItem weapon[4];        /* 0x042: each player's weapon, from gQuestSync (SceneView in scene.c) */
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
    u8 noticeId;                /* 0x132: the last notice id, 0-0x7F (Notice_ResetQueue, the Notice_Push functions in area_draw.c) */
    u8 bannerNotice;            /* 0x133: the notice the banner shows, 0xFF none (NoticeBanner_Spawn*, compared with gHudState dropNotice) */
    u8 unk134[0x138 - 0x134];
    s16 mealStamina;            /* 0x138: the meal's stamina bonus (Meal_ApplyEffects): added to the local player's maximum stamina at the start (Player_Reset) */
    s16 unk13A;                 /* 0x13A: a health value: the rewards REWARD_HP_BELOW need it above their threshold and gQuest.savedEmHp at or below (Quest_RollRewards); no writer seen */
    u8 placed1;                 /* 0x13C: the local player placed objects of kind 1 (fewer than 2 allowed, Item_CanPlaceLargeBarrelBomb); the game decrements it when one disappears (PlacedItem_Free) */
    u8 placed3;                 /* 0x13D: the same for kind 3 (fewer than 3, Scene_CanPlaceKind3; ItemProp_Free) */
    u8 placed2;                 /* 0x13E: the same for kind 2 (none allowed while set, Scene_CanPlaceKind2; PlacedItem_Free) */
    s8 fortStage;               /* 0x13F: 0 to 10, set by the fortress monsters from their scripts (em_laoshan, em_shengaoren, em_yamatsukami, em_kushala); QuestMenu_DrawRate prints (10 - fortStage) * 10 percent */
    ItemStack supply[48];       /* 0x140: the supply box: Scene_InitSupplyBox fills the first 40 from the stage data, the field draws 40, plsel counts 48 */
    u32 supplyTaken[2];         /* 0x200: one bit per supply box slot taken */
    u8 countdownOn;             /* 0x208: 1 while the countdown gQuest.countdown runs (Quest_StartCountdown, Quest_UpdateCountdown) */
    u8 emEventFlags;            /* 0x209: flags: bit 0 set by Shen Gaoren and Lao-Shan, bit 1 by Fatalis; area objects test them (area_objects.c, area_scenery.c) */
    u8 skipFlaggedTris;         /* 0x20A: set by CastleGate_Create (0 in quest 0xBBC, else 1): the area collisions skip the triangles of attribute 0x4000 (their masks add 0x4000), the kind 0x14 area object runs, and EmScript op 0x60 branches on it */
    u8 unk20B[0x20C - 0x20B];
    s16 syncDelay;              /* 0x20C: frames before a queued sync applies: the peers' mean delay, 1 to 30, default 8 (QSync_Begin, QSync_QueuePlayerPos) */
    u16 bgmId;                  /* 0x20E: the tune last started on the tune channel (Jingle_Play) */
    u8 unk210[0x224 - 0x210];
    u8 inQuest[4];              /* 0x224: per player: IN_QUEST_YES in the quest, IN_QUEST_LEFT left it, IN_QUEST_NO_PLAYER none (NetScene_Run, QNetIf_FindHost, Player_Init) */
    u8 unk228[0x26A - 0x228];
    u16 supplyWho;              /* 0x26A: the player whose supply box request the host answers (QNet case 7; sent in message 10) */
    u16 supplySlot;             /* 0x26C: the supply slot granted to supplyWho, 0xFF: already taken (supplyTaken) */
    u8 host;                    /* 0x26E: the host player's index (QNetIf_FindHost: the first player in the quest) */
    u8 prevHost;                /* 0x26F: the host before (QNetIf_Update) */
    u8 questPaid;               /* 0x270: the quest fee was paid (QuestDesk_Accept, LobbyStep_SetOut: questPaid); plsel's rewards test it */
    u8 peersLeft;               /* 0x271: a bit per peer that left (QNetIf_OnPlayerLeft) */
    s8 mealAttack;              /* 0x272: the meal's attack buff (Meal_ApplyEffects); the local player's starting attackDrug */
    s8 mealDefense;             /* 0x273: the meal's defense buff (Meal_ApplyEffects); the local player's starting defenseDrug */
    s8 mealHealth;              /* 0x274: the meal's health bonus (Meal_ApplyEffects): added to the local player's maximum health at the start */
    u8 eventRunning;            /* 0x275: a scripted event (ScEvent_*) is running: monsters, HUD and Field_UpdateAll wait */
    s8 userIds[4][8];           /* 0x276: the players' user ids (gMyUserId: Player_SetMyUserId, and the others' from their quest sync message); drawn as the players' names, passed to Player_LoadPreset; Scene_EnterField and Galex zero them */
    u8 unk296[0x2B9 - 0x296];
    u8 plselOpen;               /* 0x2B9: 1 while the quest preparation screen (plsel) is up (Plsel_SelectInit; cleared by Plsel_SelectTask, Plsel_StateReady): QSync_OnChat logs chat there instead of Chat_LogAddLocal */
    u16 galexTimer;             /* 0x2BA: frames of the current Galex_Task step: zeroed, counted up, compared with the step limit (plsel gallery_scene.c; read there as unk276[0x2BA - 0x276] before) */
} SceneState;
typedef char SceneState_size_check[(sizeof(SceneState) == 0x2BC) ? 1 : -1];

extern SceneState gScene;

/* gScene.scene: what Scene_QuestTask (scene.c) runs each frame. */
#define SCENE_ENTER         0   /* Scene_EnterField: starts the quest preparation, then SCENE_QUEST_START */
#define SCENE_QUEST_START   1   /* Scene_QuestStartTask, by QSTART_* step */
#define SCENE_FIELD         2   /* Scene_UpdateQuest: the quest in the field */
#define SCENE_QUEST_END     3   /* Scene_ResultFade: the quest's end on screen */
#define SCENE_IDLE          4   /* nothing */
#define SCENE_RESULTS       5   /* Scene_QuestFrameTick: the results (overlay 0xB) */
#define SCENE_EXIT          6   /* Quest_Teardown */
#define SCENE_OPENING       7   /* Scene_Opening: the opening movie and the boot */

/* gScene.step in SCENE_QUEST_START (Scene_QuestStartTask). */
#define QSTART_INIT         0   /* Scene_StartQuest */
#define QSTART_PLAY         1   /* Scene_StartQuestPlay */
#define QSTART_SOUNDS       2   /* Scene_LoadQuestSounds */
#define QSTART_FIELD        3   /* Scene_StartField */
#define QSTART_AREA         4   /* Scene_EnterArea */

/* gScene.pauseState: the offline pause (Scene_RequestPause; the capture in frame_draw.c). */
#define PAUSE_NONE          0
#define PAUSE_REQUESTED     1   /* the screen is captured at the frame's end (Pause_CaptureScreen) */
#define PAUSE_CAPTURED      2   /* the capture is made opaque and shown (Pause_DrawCapture) */
#define PAUSE_SHOWN         3
#define PAUSE_LEAVING       4   /* Scene_FieldFrame clears it on the next frame */

/* The save's play time stops at 9999:59:59 (PlayTime_Tick: seconds). */
#define PLAYTIME_MAX        35999999

#define SCENE_PLAYERS       4   /* the quest's player slots (SceneState.inQuest, gQuest.hunterRank, the start areas) */
#define SCENE_EM_KINDS      4   /* SceneState.emKind: the stage's monster slots */

/* SceneState.inQuest's values, per player slot (set by the lobby's network
 * code; the monsters only target and follow players IN_QUEST_YES). */
#define IN_QUEST_LEFT       0       /* the player left the quest */
#define IN_QUEST_YES        1       /* the player is in the quest */
#define IN_QUEST_NO_PLAYER  0xFF    /* no player in the slot */

/*
 * The screen fades (scene.c): two players, gFadeA and gFadeB, each
 * running one key of gFadeKeys at a time (FadeA_Start(id), ids from 1). A key
 * is a coloured rectangle whose alpha (256 opaque) moves from alphaFrom to
 * alphaTo by alphaStep a frame, after `delay` frames; the fade then holds the
 * key, or ends after a key marked last.
 */
typedef struct FadeKey {
    u8 last;            /* 0x00: nonzero: the fade ends after this key */
    u8 color;           /* 0x01: an index into gFadeAColors / gFadeBColors (0 black, 7 white) */
    u8 unk02[2];        /* 0x02: the same two bytes in every key; nothing reads them */
    f32 x;              /* 0x04: the rectangle's corner */
    f32 y;              /* 0x08 */
    f32 z;              /* 0x0C */
    s16 w;              /* 0x10 */
    s16 h;              /* 0x12 */
    s16 alphaFrom;      /* 0x14 */
    s16 alphaTo;        /* 0x16 */
    s16 alphaStep;      /* 0x18: added each frame */
    s16 delay;          /* 0x1A: frames before the alpha moves */
} FadeKey;

/* A fade player (gFadeA, gFadeB). */
typedef struct FadeState {
    u8 active;          /* 0x00: FADE_STATE_* */
    u8 starting;        /* 0x01: set by FadeX_Start, cleared when the alpha reaches the key's end */
    u8 unk02[2];
    s16 id;             /* 0x04: the key asked for (from 1), 0 none */
    s16 cur;            /* 0x06: the key running */
    s16 alpha;          /* 0x08 */
    s16 prevAlpha;      /* 0x0A: the alpha of the frame before */
    FadeKey *key;       /* 0x0C: the key running, NULL none */
    s16 delay;          /* 0x10: frames left before the alpha moves */
} FadeState;

/* FadeState.active, and what FadeA_GetState and FadeB_GetState return. */
#define FADE_STATE_IDLE     0   /* no fade */
#define FADE_STATE_RUNNING  1   /* the alpha moving */
#define FADE_STATE_HELD     2   /* holding the key's end */

/* The rectangle FadeA_Draw gives Prim_FillRectF: two corners (x, y, z, 1)
 * and a colour. */
typedef struct FadeQuad {
    f32 p0[4];
    f32 p1[4];
    u32 color;
} FadeQuad;

/*
 * The keys of gFadeKeys (FadeA_Start, FadeB_Start), from the table: "out"
 * keys go to opaque and hold, "in" keys go back to clear and end the fade.
 * The frame counts are the ramp's length.
 */
#define FADE_OUT_BLACK          1   /* 16 frames */
#define FADE_IN_BLACK           2   /* 16 frames */
#define FADE_OUT_WHITE          3   /* 16 frames */
#define FADE_IN_WHITE           4   /* 16 frames */
#define FADE_OUT_BLACK_SLOW     5   /* 32 frames */
#define FADE_IN_BLACK_SLOW      6   /* 32 frames */
#define FADE_OUT_HALF_BLACK     7   /* to half opaque, 16 frames */
#define FADE_IN_HALF_BLACK      8   /* from half opaque, 16 frames */
#define FADE_OUT_WHITE_SLOW     9   /* 128 frames */
#define FADE_BLACK              10  /* opaque black at once, held */
#define FADE_OUT_BLACK_FAST     11  /* 8 frames */
#define FADE_IN_BLACK_FAST      12  /* 8 frames */
#define FADE_IN_WHITE_FAST      13  /* 4 frames */
#define FADE_IN_BAND            14  /* a black band 32 pixels high at y 246, 16 frames */
#define FADE_IN_BLACK_DELAYED   15  /* 16 frames, after 4 */

#endif /* SCENE_H */
