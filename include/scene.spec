# Fields of SceneState (include/scene.h). Regenerate with:
#   python tools/gen_struct.py include/scene.spec include/scene.h
name SceneState
size 0x2BC
0x000 u8 scene
0x014 u16 area // the current area (compared with Em.unk680 and Player.unk680; Quest_SetStartArea)
0x020 u16 frame // +1 every field update (sub_main 0x3275F0)
0x030 u8 map // the stage map: Map_AreaIndex(map, area), Map_DayId, Tex_LoadStageFile, the per-map tables (Quest_Init: stage data word 0)
0x03E u8 playerCount // players in the quest
0x140 ItemStack{4} supply[48] // the supply box: Scene_InitSupplyBox fills the first 40 from the stage data, the field draws 40, plsel counts 48
0x200 u32 supplyTaken[2] // one bit per supply box slot taken
0x224 u8 inQuest[4] // per player: IN_QUEST_YES in the quest, IN_QUEST_LEFT left it, IN_QUEST_NO_PLAYER none (NetScene_Run, QNetIf_FindHost, Player_Init)
0x275 u8 eventRunning // a scripted event (ScEvent_*) is running: monsters, HUD and Field_UpdateAll wait
0x0D0 void * models[4] // model headers (128 bytes), copied per monster (EmSlot_SetupModel, Em +0x30F)
0x011 u8 musicState // the music's state: in the field an enum MusicState (Bgm_FieldUpdate), in the town MUSIC_TOWN_* (Bgm_TownUpdate); the hunters' actions set MUSIC_MONSTER_BATTLE with musicTimer 150
0x016 u16 startArea // the area loaded first (Quest_SetStartArea sets it with area; Quest_Setup: Area_Load)
0x02A u8 emKind[4] // per monster slot, its kind (QuestEm_SpawnArea, Quest_SpawnMonsters from gQuestSync.emKind); 0 none
0x02E u16 questId // the quest id (QuestDesk_Accept, Quest_Init; Tex_LoadStageFile(map, questId))
0x031 u8 campArea // the map's base camp: a fainted player goes back there (Player_StateThrowItem), EM_KIND_FELYNE_CART spawns there (QuestEm_SpawnArea) (Quest_Init: stage data word 1)
0x03C u8 localPlayer // the local player's index (gPlayers, per-player arrays)
0x03D u8 unk03D // set to 1 with inLobby when NetScene_Run enters the lobby from another scene (before its mode 1)
0x040 u8 questPhase // QUEST_PHASE_*: 2 the hunt, 3/4 the goal met, 5-7 lost, 8 quit (Quest_GetOutcome)
0x041 u8 inLobby // nonzero while a scene of the lobby overlay runs: the network set-up and the online town (NetScene_Run), or plsel's tour of the town. 0 in every quest, online or not: the game overlay loads at the lobby's addresses, and Player_HasSkill calls the lobby's Town_InRecordsMode while it is set. Online play is Game_IsOnline
0x12C u16 deductedItem // the item Quest_DeductReward took from the reward
0x12E s16 deductedCount // its count
0x130 u8 rewardDeducted // set by Quest_DeductReward, cleared by Quest_Init
0x208 u8 countdownOn // 1 while the countdown gQuest.countdown runs (Quest_StartCountdown, Quest_UpdateCountdown)
0x138 s16 mealStamina // the meal's stamina bonus (Meal_ApplyEffects): added to the local player's maximum stamina at the start (Player_Reset)
0x272 s8 mealAttack // the meal's attack buff (Meal_ApplyEffects); the local player's starting attackDrug
0x273 s8 mealDefense // the meal's defense buff (Meal_ApplyEffects); the local player's starting defenseDrug
0x274 s8 mealHealth // the meal's health bonus (Meal_ApplyEffects): added to the local player's maximum health at the start
0x13C u8 placed1 // the local player placed objects of kind 1 (fewer than 2 allowed, Item_CanPlaceLargeBarrelBomb); the game decrements it when one disappears (PlacedItem_Free)
0x13D u8 placed3 // the same for kind 3 (fewer than 3, Scene_CanPlaceKind3; ItemProp_Free)
0x13E u8 placed2 // the same for kind 2 (none allowed while set, Scene_CanPlaceKind2; PlacedItem_Free)
0x110 u8 itemUses[4] // per player: the items used (Stats_CountItemUse)
0x13F s8 fortStage // 0 to 10, set by the fortress monsters from their scripts (em_laoshan, em_shengaoren, em_yamatsukami, em_kushala); QuestMenu_DrawRate prints (10 - fortStage) * 10 percent
0x03F u8 hitId // the last hit id (HitId_Next increments it, skipping 0; HitId_Reset)
0x0E0 s16 modelCount[4] // per monster slot, its models (EmSlot_LoadModel loads one)
0x0E8 s16 modelFirst[4] // per monster slot, its first model slot
0x0F0 struct Model * partModels[4] // per monster slot, a second model set (EmSlot_FreeModels frees it with the first; Model, include/model.h)
0x100 s16 partCount[4] // its models, -1: none
0x108 s16 partFirst[4] // its first model slot
0x028 u16 musicTimer // the frames left in musicState's state (Bgm_FieldUpdate: it moves on at 0 or less)
0x131 u8 aimReverse // the bowgun aim reversal option: 0 none, 1 up/down, 2 up/down and left/right (Player_BowgunAim)
0x209 u8 emEventFlags // flags: bit 0 set by Shen Gaoren and Lao-Shan, bit 1 by Fatalis; area objects test them (area_objects.c, area_scenery.c)
0x20A u8 skipFlaggedTris // set by CastleGate_Create (0 in quest 3004, else 1): the area collisions skip the triangles with attribute bit 0x4000 (their masks add that bit), the kind 20 area object runs, and EMOP_IF_GATE_OPEN branches on it
0x001 u8 step // the scene's step (Scene_StartQuest, NetScene_Run, ...)
0x00D u8 padRead // the pads are read each frame while set (Pad_Update; set by the scene start, and by the lobby)
0x00E u8 menuOpen // the field or player menu is open (FieldMenu_Open, Plsel_OpenPlayerMenu); blocks the HUD input (Hud_InputBlocked)
0x20C s16 syncDelay // frames before a queued sync applies: the peers' mean delay, 1 to 30, default 8 (QSync_Begin, QSync_QueuePlayerPos)
0x26E u8 host // the host player's index (QNetIf_FindHost: the first player in the quest)
0x26F u8 prevHost // the host before (QNetIf_Update)
0x270 u8 questPaid // the quest fee was paid (QuestDesk_Accept, LobbyStep_SetOut: questPaid); plsel's rewards test it
0x2B9 u8 plselOpen // 1 while the quest preparation screen (plsel) is up (Plsel_SelectInit; cleared by Plsel_SelectTask, Plsel_StateReady): QSync_OnChat logs chat there instead of Chat_LogAddLocal
0x002 u8 subStep // the step inside a scene step (Scene_LoadQuestSounds, Scene_StartQuest); 0 on the first boot (Scene_Opening)
0x271 u8 peersLeft // a bit per peer that left (QNetIf_OnPlayerLeft)
0x003 u8 openingFirst // 1 when the opening movie plays for the first time (event flag 0x26 not set yet; Scene_Opening), cleared on entering the field
0x010 u8 musicFlags // MUSIC_FLAG_*: an area's own tune has played, the music is turned down under a tune
0x012 u8 allLocal // in a drill, every player counts as local (Player_IsLocal); set by Scene_EnterField, cleared by plsel
0x2BA u16 galexTimer // frames of the current Galex_Task step: zeroed, counted up, compared with the step limit (plsel gallery_scene.c)
0x022 u8 padIndex[2] // per pad port, its gPadState index (Pad_ReadButtons; the scene reset sets 0 and 1); the login screen reads [0] (LoginPad_Triggered); was padPort
0x025 u8 pauseState // the offline pause: 0 none, 1 requested, 2-3 paused, 4 leaving (Scene_RequestPause, Game_PauseState; the lobby keeps copies of the pause functions)
0x004 s16 timer // a scene timer (scene.c SceneView, plsel PlselScene)
0x006 s16 timer2 // a second one (SceneView)
0x042 EquipItem{12} weapon[4] // each player's weapon, from gQuestSync (SceneView in scene.c)
0x276 s8 userIds[4][8] // the players' user ids (gMyUserId: Player_SetMyUserId, and the others' from their quest sync message); drawn as the players' names, passed to Player_LoadPreset; Scene_EnterField and Galex zero them
0x032 u8 playerGender[4] // per player, from gQuestSync (Quest_InitPlayers: Player.gender)
0x0BA u8 presetEquip[4] // per player: Player.presetEquip (Quest_InitPlayers), from gQuestSync.unk20 (Scene_StartQuest)
0x0C4 s8 startItemList[4] // per player, from gQuestSync (Quest_InitPlayers: Player.startItemList)
0x008 s16 timeout // a third timer: the drill rewards list's timeout, frames (Plsel_RewardListTask)
0x00A s16 stateSyncFrame // frames since the last state sync: at 60 Scene_QuestFrameTick sends QSync_SendState(8) and restarts it; zeroed when the quest results screen starts (Plsel_ResultsUpdate)
0x26A u16 supplyWho // the player whose supply box request the host answers (QNet case 7; sent in message 10)
0x26C u16 supplySlot // the supply slot granted to supplyWho, QSYNC_SLOT_TAKEN: already taken (supplyTaken)
0x00F u8 camControl // the camera control option (option byte 4, copied with gSysSave.camControl): 0 L1/R1 change the distance, 2 reverses the turn (CamMode1 in kbd.c)
0x019 u8 charIndex // the save's character slot, copied from gQuestSync.charIndex at the scene reset (scene.c)
0x026 u8 heldMask // buttons held at a change of control, masked out until released (Pad_ReadButtons)
0x027 u8 camFollow // the camera follow option (option byte 3, copied with gSysSave.camFollow): 0 the camera swings behind the hunter (CamMode1 in kbd.c)
0x132 u8 noticeId // the last notice id, 0-127 (Notice_ResetQueue, the Notice_Push functions in area_draw.c)
0x133 u8 bannerNotice // the notice the banner shows, 255 none (NoticeBanner_Spawn*, compared with gHudState dropNotice)
0x13A s16 unk13A // a health value: the rewards REWARD_HP_BELOW need it above their threshold and gQuest.savedEmHp at or below (Quest_RollRewards); no writer seen
0x20E u16 bgmId // the tune last started on the tune channel (Jingle_Play)
