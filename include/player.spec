# Fields of Player (include/player.h). Regenerate with:
#   python tools/gen_struct.py include/player.spec include/player.h
name Player
size 0x970
0x000 u8 active // nonzero while the slot is in use
0x002 u8 weaponType // the weapon type: 0 great sword, 1 heavy bowgun, 2 hammer, 3 lance, 4 sword and shield, 5 light bowgun, 6 dual swords, 7 long sword, 8 hunting horn, 9 gunlance, 10 bow (21 coating slots)
0x003 u8 isGunner // 1: a gunner (bowgun: ammo, no sharpness)
0x004 u8 unk004 // with step, stateVar0, stateVar1: cleared together (Player_ClearActionState)
0x008 s32 timer // as Em.timer: per-state counter (a "hit once" flag in weapon_states.c, a countdown from 10 in the lobby states)
0x00C u16 index // the slot in gPlayers (== gScene.localPlayer, gNetUsers[index]; as Em.index)
0x014 u8 stateGroup // the state group (as Em.stateGroup; Player_RunState switches on it)
0x00E u16 targetYaw // heading to turn to (as Em.targetYaw): set from rotY when a hit turns the player (Player_ReactToHit)
0x0AC f32 pos[3] // position (as Em)
0x2B4 u16 unk2B4
0x2B8 u8 unk2B8
0x2BC u8 motionStarted[4] // per motion layer: 0 after Player_SetLayerMotion; Player_Update then begins the layer motion (Player_BeginLayerMotion) and sets it
0x36C PlayerInput{36} input // the pad input (Player_CopyPadInput)
0x390 u8 action // the posture, PL_POSTURE_* (Player_SetPosture sets it)
0x391 u8 unk391
0x392 u8 unk392
0x397 u8 padSwapLR // bit 0: replayed pad input swaps the d-pad's left and right (PAD_LEFT, PAD_RIGHT; Pad_ReadButtons)
0x398 u32 flags // flag bits: Player_SetFlag/ClearFlag/TestFlag without bit 31 (stateFlags with it); the quest's player resets keep 0x100100 or 0x80100
0x39C u32 stateFlags // flags (Player_SetFlag, Player_ClearFlag, Player_TestFlag), cleared on every state change (Player_SetState)
0x3A0 u16 unk3A0
0x3A4 s32 motionTime // the motion time (Player_UpdateMotionTime adds len - blend, Player_TickTimers 2; wraps at 100000)
0x3B0 f32 emDist // the distance to a monster in its area: Field_PushApart writes it for each in turn, so the last one in gEms stays
0x3B8 f32 vel[3] // velocity
0x3C4 f32 accel[3] // acceleration
0x3D4 u8 hitDelay // frames before the player's own hit window opens (as ObjAttach.hitDelay): Player_UpdateHitTimers counts it down in hitPhase 1, a motion blend adds to it (player_stats.c)
0x3D5 u8 hitFrames // frames the hit window stays open (as ObjAttach.hitFrames), counted down in hitPhase 2 (Player_UpdateHitTimers)
0x3F8 u8 unk3F8
0x40E u16 noHitTimer // frames the player cannot be hit (as Em.noHitTimer): Player_SetEvadeFrames sets 6 (+2/+4 with the evasion skills), Player_Reset 30; counted down by Player_TickTimers; Player_UpdateEnvironment skips the area damage while > 0
0x417 u8 unk417
0x430 u16 unk430
0x434 u8 unk434
0x4BA u16 unk4BA
0x4C1 u8 unk4C1
0x4C4 u8 unk4C4
0x533 u8 carry // the carried item: its kind less one in the high nibble (PL_CARRY_KIND_MASK, from Item_CarryKind), the hold count in the low (PL_CARRY_HOLD_MASK, Player_UpdateCarryKind)
0x680 u16 area // the area the player is in (as Em.unk680; gathering compares it)
0x682 u8 areaMoveState // PL_AREA_MOVE_*: how Player_Reset places the hunter in the area it moves to (nextArea)
0x684 u16 nextArea // the area the player goes to (Town_EnterArea, AreaMenu_GoTo; as Em.nextArea)
0x7D0 struct ActionSpot * spot // the action spot the player stands at (Player_FindActionSpot; in the town, the one the action button uses)
0x7D4 s16 weaponTimer // great sword: the charge (frames); gunlance: the wyvern fire cooldown; bow: frames since the last shot (to 1000)
0x7D8 s8 chargeLevel // great sword: the charge level 0-3 (GreatSword_ChargeLevel)
0x7E4 u8 itemSelect // 1 while the item selector is held open (Player_UpdateItemSelect); only buttons 0x90 get through
0x7E8 f32 targetPos[3] // the point the monsters aim at (Em_GetTargetPos)
0x81A u8 supplyBoxOpen // Hud_SupplyBoxOpen, gHudState->supply; the supply box menu clears it on cancel (pouch.c)
0x848 u8 freeState // set to 1 each frame by the idle and move states, cleared at the frame start (Player_Update): items can be used (Player_IsFree, the pouch menu)
0x8B1 u8 deliveryMenu // the delivery menu: 255, 1, 0 (Player_RunDeliveryMenu, Player_StateBedMenu); blocks the pad input
0x8D5 u8 deliveryOpen // Hud_DeliveryOpen, gHudState->unk23; Quest_DeliveryMenu clears it when closing (pouch.c)
0x01C u8 roundsLoaded // bowgun: rounds loaded
0x01D u8 clipSize // bowgun: clip size
0x2C2 s16 hp // health (as Em.hp)
0x432 s16 noFlinchTimer // the attack states set it (60, 40, 2...), Player_TickTimers counts it down; while nonzero Player_IgnoresFlinch (pouch.c) returns 1 and Player_ReactToHit (player_hit.c) skips the flinch
0x534 u8 ammoType // kind of the loaded ammo (ItemData.ammoKind)
0x694 s16 stamina
0x69C u16 lean // the body lean while turning, a 16-bit turn kept within 2560 of upright either way (Player_UpdateAutoTurn, Actor_TurnToTarget); Player_TwistBoneToLook rolls a bone by it
0x6DC s16 hpRecoverable // the red part of the health bar
0x6DE s16 hpMax // 1..150
0x754 f32 moveTo[3] // a point to move to (Player_StartMoveTo, Player_MoveToSpawn, Contest_TakeSeat, HairMenu_MovePlayer)
0x760 f32 moveToStep[3] // added to pos each frame while moveToFrames counts (Player_UpdateMoveTo, pouch.c)
0x76C s16 moveToFrames // frames to reach moveTo (Player_StartMoveTo, Player_UpdateMoveTo)
0x76E s16 lookYaw // the head turn toward the nearest monster (Player_FindLookTarget; pouch.c reads it signed; the lobby unwinds it by ANGLE_TURN / 64 a frame through u16, TownPc_UnwindAngle76E)
0x775 u8 coatingSlot // bow: the pouch slot of the loaded coating, 0xFF: none (sub_main Bow_SetCoatingSlot; a shot uses one up)
0x77C ItemStack{4} pouch[20] // the item pouch
0x7D6 s16 sharpness
0x7DA s16 staminaMax // 75..450
0x7DC s16 sharpnessMax
0x7DF u8 sharpnessLevel // the colour (Player_GetSharpnessLevel)
0x7E0 u16 pouchCursor // the selected pouch slot
0x7E6 u16 ammoSlot // bowgun: the pouch slot of the loaded ammo, 0xFF: none
0x814 s16 ammoCount // bowgun: rounds of the loaded ammo left, 0xFF: unlimited
0x818 s16 staminaTimer // frames until the maximum stamina drops (10800)
0x81B u8 menuCursor // menu cursor (the supply box and delivery menus)
0x81C u8 actRequest // the gesture requested (Player_RequestGesture); Player_StartGesture starts state 6/gGestureStates[it]
0x826 s16 savedAmmoCount // ammoCount saved with savedCoatingSlot (Player_BowgunAmmoSwitch, Bow_StateFire)
0x829 u8 savedRoundsLoaded // roundsLoaded kept over the reload and the weapon change (restored from it), as savedClipSize and savedAmmoCount
0x82A u8 savedClipSize // clipSize saved with savedCoatingSlot (Player_BowgunAmmoSwitch, Bow_StateFire)
0x849 u8 menuPage // menu page
0x84B u8 menuDelay // menu input delay
0x872 u16 hotDrinkTimer // frames left of item ITEM_ID_HOT_DRINK's effect (18000; Item_Use); nonzero: no cold in zones 2 and 4 (Player_UpdateHunger, Hud_StaminaColor)
0x877 u8 supplyTaken // nonzero once the supply box was taken from (SupplyBox_Take, QSync_OnSupply)
0x88E u16 supplyBoxTimer // 900 frames after a supply box take (SupplyBox_Take, SupplyBox_Update)
0x899 s8 demonMode // dual swords: 1 in the demon mode (DualSwords_StateDemonToggle; PlayerAttack_Setup)
0x89A s16 noFlinchItemTimer // item ITEM_ID_DEFENSE_BURST sets it with defenseBuffTimer (Item_Use); counted down; tested with noFlinchTimer and demonMode (Player_IgnoresFlinch)
0x8B2 u8 yesNoCursor // the delivery menu: the yes or no cursor (gYesNoTexts[it], DeliveryMenu.yesNo; Player_RunDeliveryMenu, Quest_DeliveryMenu)
0x8FC s16 hpMaxBase // hpMax without the song effect 3: set from hpMax, raised by Player_AddMaxHp unless temporary (to 150); restored to hpMax when effect 3 ends (Player_UpdateStatus)
0x8A4 u32 styleColor // gPlayerColorWords[style] (Player_SetColorWord)
0x8CB u8 style // the style (save symbolColor; Player_SetColorWord); 255 none; the lobby draws its icon (11 cycles colours)
0x005 u8 step // as Em.step: the step within the state (counted up by the states), cleared on every state change (Player_ClearActionState)
0x006 u8 stateVar0 // as Em.stateVar0: per-state scratch byte (a counter to 5 in PL_ACT_WALL_HOLD), cleared on every state change
0x007 u8 stateVar1 // as Em.stateVar1: per-state scratch byte (a "hit once" flag with hitPending; a countdown in the lobby states)
0x010 u8 isEm // 0 for a hunter: copied to the effects' isEm (Explosion_Create); the Melynx steal checks it is 0
0x012 u8 weaponOut // nonzero while the weapon is drawn (set per weapon type by state 0/04; the idle input branches on it)
0x015 u8 state // the state within the group (Player_IsState; the group dispatchers switch on it)
0x016 u8 prevStateGroup // the previous stateGroup
0x017 u8 prevState // the previous state
0x01E u8 ownTask // as Em.ownTask: 0 for the players (Player_InitForQuest); an Em run by another runner (the lobby NPCs) has 1
0x01F u8 playerMotions // read when the object is an Em: with ownTask set, nonzero takes the player motion sets of slot index, not the monster ones (Player_SetPartAnim, Player_BeginLayerMotion)
0x29C u16 motionId[4] // motion layers (Player_SetLayerMotion): the motion id (as Em.motionId)
0x2A4 s16 motionFrame[4] // motion layers: the start frame (as Em.motionFrame)
0x2AC s16 motionBlend[4] // motion layers: the blend frames, halved (as Em.motionBlend)
0x3A2 u16 stateRand // random value drawn on each state change
0x588 u8 rideState // 0 none, 1 standing on the monster rideEm, 3 landed on it from a jump, 2 leaving (Em_CarryRiders sets 1/3, the states set 2/0)
0x653 u8 stateRequest // 1: a state change was requested (Player_RequestState, Player_RequestStateSynced; Player_Update)
0x66C u8 partAnimOn[4] // per part: a motion override is set (Player_SetAimLayers, Player_ClearPartOverrides)
0x670 u16 partAnimMotion[4] // per part: the override motion (Player_SetPartAnim)
0x678 u16 partAnimWeight[4] // per part: the override weight, 0..100 (from aimWeight)
0x821 s8 aimWeight // bowgun: -100..100, the weight of the second motion layers (Bowgun_SetAimLayers)
0x8B4 f32 motionSpeed // motion speed factor (1.0; 0.5..1.54 per state: Player_SetState and the weapon states); motionStep0/1 are 2.0 times it
0x537 u8 spotCheckDelay // nonzero: no action spot check (Player_CheckActionSpot sets 20; Player_UpdateStatus counts it down)
0x538 u16 startAngle // its heading: gStartAngle, the action spot angle + ANGLE_TURN / 4
0x688 f32 startPos[3] // where the player starts in the area: gStartPos, the action spot target (Player_InitAll, Player_CheckActionSpot)
0x020 f32 rotMtx[16] // the rotation matrix from rot (Mtx_FromAnglesXYZ, Player_Reset; as Em.rotMtx)
0x0A0 s32 rot[3] // rotation angles as integers, ANGLE_TURN per turn; rot[1] the heading (as Em.rotX..rotZ)
0x2C0 u16 motionLayerCount // number of motion layers
0x4B8 u16 statusIcons // the HUD's status icons, PL_STATUS_ICON_* bits (Player_UpdateStatusIcons)
0x628 s8 attackDrug // attack bonus of the drugs (Item_Use: at least 3 or 5), kept in gScene.mealAttack (Player_RecomputeStats)
0x629 s8 attackBuff // the timed attack buff (10, 25...; Item_Use, Item_UseTeamPowder)
0x62C s8 defenseDrug // defense bonus of the drugs (Item_Use: at least 6 or 10), kept in gScene.mealDefense
0x62D s8 defenseBuff // the timed defense buff (20, 40, 50)
0x630 u16 attack // attack power, 1..800
0x632 u16 defense // defense
0x706 s16 poison // poison left: counts down every poisonTimer frames (60 with SKILL_POISON_HALF, 15 with SKILL_POISON_DOUBLE, else 30); an area of kind 1 keeps it at least 10 (Player_UpdateEnvironment; as Em +0x706)
0x70A s16 poisonTimer // frames to the next poison tick
0x724 f32 attackFactor // attack factor: attack / 100
0x728 f32 defenseFactor // defense factor: defense / 2, at least 1
0x771 u8 unk771
0x868 u8 skills[5] // skill ids (PL_SKILL_SLOTS)
0x870 u16 coolDrinkTimer // frames left of item ITEM_ID_COOL_DRINK's effect (18000; Item_Use); nonzero: Hud_HealthColor and Player_UpdateEnvironment skip the heat
0x874 u16 regenTimer // frames left of items ITEM_ID_REGEN and ITEM_ID_FOOD_REGEN_SHORT to ITEM_ID_FOOD_REGEN_LONG (Item_Use, 3600..14400): doubles the health recovery step (Player_UpdateStatus); Notice 22 at the end; status icon PL_STATUS_ICON_REGEN
0x878 f32 resist[5] // element resistances (fire, water, thunder, ice, dragon), -100..100
0x894 u8 skillGroups[5] // the skill group of each slot
0x89D s8 skillPoints[5]
0x8C9 u8 teamSkill // the team combo skill: Player_CheckTeamCombo sets it when the four hunters' armour styles make one of gArmorCombos; Player_HasSkill and Player_GetSkill count the local player's for every hunter, in any quest (gScene.inLobby 0), not in the lobby
0x8CA u8 teamSkillGroup // its skill group
0x8CE s16 status6Timer // status 6 frames left: 1800 at a hit (900 with SKILL_STATUS6_HALF, 3600 with SKILL_STATUS6_DOUBLE; Player_ReactToHit, Item_Use, ObjAttach_DamagePlayer), counted down (Player_UpdateStatus), cleared with the other statuses (Player_Reset, Player_StateBedLieDown); an icon (Player_UpdateStatusIcons)
0x8D0 s16 status7Hit // the hit status 7 amount: > 0 starts status7Timer unless SKILL_STATUS7_IMMUNE (Player_ReactToHit)
0x8D2 s16 status7Timer // status 7 frames left: 1800 at a hit or a poisoning item (900 with SKILL_STATUS7_HALF; Player_ReactToHit, Item_Use), counted down (Player_UpdateStatus), cleared with the other statuses; an icon (Player_UpdateStatusIcons)
0x8D6 u8 songEffect[12] // the level of each effect kind, PL_SONG_* (Player_HasSongEffect, Player_ClearSongEffects; only the hunting horn's songs set them)
0x8E2 s16 songEffectTimer[12] // per kind: frames left (1800, 2700, 3600; counted down by Player_UpdateStatus)
0x312 EquipItem{12} equip[7] // 0, 2-5: armour; 6: the weapon (type 7: a bowgun); 3: the torso
0x011 u8 gender // the hunter gender (from the save data)
0x30C u16 weaponKind // the weapon kind (EquipItem kind, Equip_GetKind)
0x30E u8 hairStyle // the hair style (HairMenu_Apply, HunterCard_Apply, Player_SetupFromSave)
0x409 u8 side // side: players with the same bit 0 are on the same side
0x580 u32 hairColor // the hair colour (HairMenu_Apply, HunterCard_Build, Player_SetupFromSave)
0x598 s16 swordGaugeTimer // long sword: frames left after the gauge fills (900; LongSword_AddGauge)
0x62A s16 attackBuffTimer // duration of attackBuff
0x6AF u8 aimLockTimer // bowgun and bow: 5 or 10 frames after an aim change (Bow_UpdateAim); no shot while nonzero
0x6B0 u8 aimMode // bowgun and bow: the aim mode (toggled; gScene.aimReverse applies in it)
0x774 u8 weaponFlags // flags of the weapon type (WeaponType_Flags(weaponType): 1, 2, 4; Player_SkillApplies)
0x776 s16 swordGauge // long sword: the spirit gauge 0..100 (LongSword_AddGauge, LongSword_GaugeAtLeast, Hud_DrawWeaponBox)
0x824 s16 staminaLockTimer // frames left of an item effect that keeps the stamina (Item_Use; Player_StaminaLocked)
0x82B u8 voice // the voice: save data +0x20D8 and the hunter card (Player_SetupFromSave); gVoiceBanks[gender * 16 + it] (lobby), Chara_SetVoiceMotion
0x846 s16 aimPitch // the aim pitch, 16-bit turn: the first-person camera (camera mode 1, Mtx_FromAngles16Pos) and Hud_DrawObjMarker use it with the heading rot[1]; up/down input adds +-ANGLE_TURN / 64 clamped to +-ANGLE_TURN / 8 (45 deg; Player_AddAimPitch, gScene.aimReverse flips it); Player_AddAimPitchPct sets it from aimPitchPct
0x8A2 u8 attackPercent // the attack power in percent (great sword charge levels: 100..130)
0x8A3 u8 attackPercentBase // the weapon type's attack percent (gWeaponClassAttack[weaponType], Player_ResetAttackPercent)
0x8BC u8 charType // the character's TYPE, 0-3 (SaveData.charType, HunterCard.charType, the preset characters): with gender it picks the row of gLookPartMap0/3/4 (gender * 16 + it * 4)
0x8C7 u8 bladeSpecial // the weapon blade data value unk13 (Equip_GetBladeSpecial of equip[6]): hunting horn: its note set (gBladeSpecialMarks[x * 3 + i]); gunlance: the shelling type (x % 3) and level (x / 3)
0x8FF u8 inPoisonArea // 1 while in an area of kind 1 without SKILL_POISON_IMMUNE, which keeps poison at least 10 (Player_UpdateEnvironment clears it each frame); Player_InPoisonArea tests it offline
0x84C ItemStack{4} delivered[4] // items delivered for the quest (Quest_CountDelivered)
0x82C s8 name[18] // the hunter name (SaveData name, Player_LoadName)
0x778 struct Em * cart // the Felyne cart (an Em of kind EM_KIND_FELYNE_CART, its cartPlayer this player) that carries the player away (Player_StateCart); cleared when it leaves (QuestEm_LeaveArea)
0x7E2 u16 useItem // the item being used: the selected pouch item (Player_UseItem, Item_Use compare it with item ids); copied to gSpecialMinePoint (SpecialMine_Create)
0x001 u8 visible // nonzero while drawn: checked with active before the draw calls (DrawCb_CharMake, Hud_DrawPlayerNames); Player_Reset sets 1 (as Em.visible)
0x62E u16 defenseBuffTimer // duration of defenseBuff (Player_ClearBuffs, Item_Use)
0x6F6 s16 status5Buildup // the status 5 buildup: status5Hit added per hit (Player_ReactToHit), full at 50 (Player_Status5Full); counted down after status5Delay (Player_UpdateStatus)
0x6F8 s16 status5Delay // frames before status5Buildup decays: 150 at a hit (Player_ReactToHit), counted down (Player_UpdateStatus)
0x6FE s16 status2Buildup // the status 2 buildup: status2Hit added per hit (Player_ReactToHit); > 0 picks the status state (Player_PickStatusState)
0x710 s16 status4Buildup // the status 4 buildup: status4Hit added per hit (Player_ReactToHit); > 0 picks the status state (Player_PickStatusState)
0x744 s16 mashBuildup // the buildup toward the mash status: mashBuildupHit added per hit (Player_ReactToHit); Player_HasMashBuildup, Player_InMashStatus
0x746 s16 mashLeft // the status of state 2/0x1E: pressed buttons take 10 (SKILL_MASH_FAST twice, SKILL_MASH_SLOW half: mashTimer); at 0 PL_REACT_MASH_FREE; synced in qnet_sync message 3
0x88C s16 detectTimer // frames left of an item effect (Item_Use) that Player_DetectLevel and the HUD glow test
0x890 s16 recoverTimer // state 4/x: 150 frames (skill 1 halves, skill 3 doubles), mashing takes 1 more each 3 frames; the hunter gets up at 0
0x8BA s16 actDetectTimer // 300 frames from the gesture state 6/2 (Player_StateGestureScout): as detectTimer (Player_DetectLevel, Map_MonsterLocated, AreaFlyer_Run)
0x772 u8 inBattle // nonzero while a monster fights it (Player_UpdateInBattle; Bgm_IsBattle, Hud_DrawPlayerStatus)
0x86F u8 teamItemMask // set to 15 when a team item (ITEM_ID_TEAM_HEAL to ITEM_ID_TEAM_DEFENSE) is used (Item_UseTeamPowder); the bit of each player reached is cleared (player_motion_fx.c)
0x83E s16 gotItem // the item to give the player (Pouch_GiveItem: Item_Use, Player_GatherAtSpot, Contest_GivePrize, PoogieMenu_Update)
0x661 u8 floorArg // byte 1 of the floor triangle (as Em.floorArg): passed with the voice SEs (bank 7, PlayerSe_VoiceAtFrame)
0x698 u32 wallHitDirs // as Em.wallHitDirs: Em_CollideField also runs for the players (Field_Update); Motion_VoiceAtFrame tests 0xE0000007 (a wall ahead) to pick the voice from gEmWallContacts
0x738 u8 pushed // as Em.pushed (same offset): Field_PushApart sets it on players too; Field_UpdateAll reads it as ((Em *) pl)->pushed
0x739 u8 weaponSeMute // nonzero: no weapon SEs (PlayerSe_WeaponAtFrame); the weapons hit checks set 2, Player_UpdateStatus counts it down
0x30F u8 emModelSlot // read when the object is an Em: Em.modelSlot, the monster motion sets it takes (Player_SetPartAnim, Player_BeginLayerMotion)
0x310 u8 unk310
0x311 s8 unk311
0x3D0 struct PlayerVt * handlers // the player handlers, gPlayerHandlers (Player_Init)
0x40A u8 hitStopAdd // the hit stop of the last hit (the attack object +0x70, ObjAttach_DamageEm/DamagePlayer), added to hitStop (Player_UpdateHitStop)
0x40B u8 hitStop // counts down; nonzero stops the timers of Player_TickTimers
0x410 u16 noPushTimer // frames without body pushing (as Em.noPushTimer): Field_PushApart skips the player while nonzero; counted down by Player_TickTimers; states set 2/10
0x52C struct DrawSlot * drawSlot // its draw slot: an entry of the pool at 0x6A5AD0 (DrawSlot_Get), drawn by DrawCb_LocalPlayer (Player_InitAll)
0x530 s16 drawSlotId // its index, -1: none
0x532 u8 painted // the Em paint mark read through Player (as Em.painted): Player_TickPaint runs only for isEm objects; Player_Reset sets 255 (players: never); Map_MonsterLocated tests it
0x53A s16 paintTimer // frames until painted is cleared (as Em.paintTimer; Player_TickPaint)
0x540 u32 lightFadeColors[3] // per light of light set 1: the colour it fades from, 0xFFRRGGBB (LightSet_FadeColors moves it a fifth of the way to the set's colour each draw and keeps the result; 0: the set's own colour). Player_ClearLightColors clears it; the character maker sets 0x01000000 (from black)
0x664 void * lightColors // the hunter's own colours of lights 0 and 1 (LightSet_SetColors reads it as rows of 4 floats); NULL: the area's
0x594 s16 hitSlowTimer // from the attack object +0x70 (ObjAttach_DamageEm): while > 0 the motion runs at 0.2 (Player_UpdateMotion) and some attacks wait
0x59A u8 presetEquip // nonzero: preset equipment, from the tables at 0x44AED0 (Player_InitForQuest)
0x722 u8 hideShadow // as Em.hideShadow: nonzero: Shadow_Draw (the shadow) skips the object; counted down (Player_TickTimers, TownPc_TickTimers)
0x817 u8 startItemList // 1 + the start item list (Pouch_SetStartItems, Quest_InitPlayers)
0x0B8 f32 scale[3] // set to 1.0 by Player_Reset (as Em.scaleX..scaleZ)
0x7E5 u8 questStartDone // 1 once Player_StateQuestStart (PL_EVENT_QUEST_START) ran; Player_Reset clears it. In questPhase 4, Player_IdleActions and Player_CrouchInput ask for the quest's start pose while it is 0
0x8FE u8 scoutSpotted // set once by Player_StateGestureScout, with actDetectTimer 300, when sub_main's Area_GetFlyerPos point is within about 33 degrees of the facing; Player_Reset clears it (seen; what it spots is not settled)
0x876 u8 faints // times fainted in the quest, up to 99 (Player_StateFaint; QuestMenu_DrawFaints)
0x81F u8 tradeCount // the trader kind 0x0A: +1 per trade up to 7 (7 at once from 4 at random), decremented by the trader's later steps
0x8FA s16 tradeCooldown // 9000 frames set at a trade when 0 (EmTrader_RunTalk), counted down by Player_UpdateStatus; the trader clears it
0x8B3 u8 unk8B3
0x866 u8 syncTimer // frames until the player state is sent again (QSync_SendPlayer sets it; counted down before each send)
0x437 u8 fieldCollide // 1: the player collides with the walls (as Em.fieldCollide): Player_RunState and TownPc_Spawn set 1; Player_StateBedLieDown..35, TownPc_State19 and Contest_Open clear it
0x438 PlayerWork{20} work // the work area of Player_MotionFxStep, the motions' sounds and effects (player_motion_fx.c)
0x7CC s16 unk7CC
0x7CE s16 unk7CE
0x574 f32 floorY // the floor height under the player (Player_Update; Player_Reset sets pos[1]; as Em.floorY)
0x770 u8 lookFound // 1: Player_FindLookTarget found a monster in range
0x420 u8 * hitTimers // the hit window timers in use: &hitDelay (Player_Reset), or the hitDelay of its attack object that hit (ObjAttach_LinkTarget)
0x536 u8 ammoMenuOpen // 1 while the ammo menu is open (Player_BowgunAmmoSwitch)
0x58B u8 lookVariant // the look variant (0..3) from the colour (Player_BuildPartIds: gFaceBodyTypeMale/gFaceBodyTypeFemale by gender): picks the head model (Look_MapPartId k) and the texture set (+ gender * 4)
0x58C s16 unk58C
0x58E s16 unk58E
0x6E4 f32 alpha // set to 1.0 (Player_Reset, Chara_SetupNewPlayer; as Em.alpha)
0x6EC void * contact // a Player or an Em: the other party of the last hit (ObjAttach_DamagePlayer), as Em.contact
0x6F0 struct ObjAttach * contactAttach // that hit's attack object
0x740 f32 fadeAlpha // the local player's draw alpha in the field: eased a tenth of the way to CamCtl_GetPlayerFade's target each frame, 1.0 without one (Player_DrawLocal; Player_Reset sets 1.0)
0x81D u8 weaponShown // nonzero: the weapon model is drawn (Player_DrawLocal, Player_Draw call Player_DrawWeapon); Player_Reset and the hunter setups set 1, plsel's preset players 0, the town hunters while not free (TownPc_IsFree)
0x4C8 s8 meshShown[32] // one per mesh of model, 1: drawn (as Em.meshShown): Player_InitRecords sets all 32; Player_DrawLocal and Player_Draw skip the others
0x683 u8 nearCamera // 1: another player's hunter within 200 of the camera (Player_Update): drawn at alpha 0.2 in the sorted list (Player_Draw); 0 for the local player
0x860 u8 offerRequest // the pouch menu's offer sets it with offerTo/offerItem/offerCount; the idle states then enter PL_ACT_OFFER_ITEM
0x81E u8 tradeMsgTimer // EM_KIND_TRADER sets 5, 10, 60 (EmTrader_RunTalk, EmTrader_ShowMessage); Player_UpdateStatus counts it down
0x892 u8 offerNoticeTimer // 90 frames after the pouch-full notice for an offered item (no new take while nonzero)
0x845 u8 bowgunReady // 1 while the bowgun attack input runs with rounds loaded (Bowgun_AttackInput), cleared at each frame start (Player_Update) and by Player_Reset; Player_IsAiming tests it
0x89C u8 prevFreeState // freeState of the last frame (Player_Update): the guard SKILL_AUTO_GUARD and the heavy bowgun shield guard only then
0x84A u8 pouchHudFlags // pouch HUD flags: 2 next, 4 previous item (Player_UpdateItemSelect), 1, 8, 16 (ItemHud_Update, Hud_DrawPouchBar)
0x596 u8 pushPriority // compared between two bodies to pick the one pushed (PlayerEm_PushApart, EmEm_PushApart); Player_Reset sets 1
0x597 u8 nearLargeMonster // 1 while a large monster (pushPriority 3 or more) is near: within 300 horizontally, 500 for one above 3, and less than 300 below the player (Player_UpdateNearMonster)
0x6AC s16 dashDelay // frames before R1 with the stick dashes again (states PL_ACT_RUN_AFTER_DASH, PL_REACT_MASH_RUN, PL_CARRY_WALK): the states set 30, counted down (Player_UpdateStatus, TownPc_TickTimers)
0x7DE u8 itemUseDelay // counted down (Player_UpdateStatus); the item button (PAD_SQUARE) does nothing while nonzero (idle and move states)
0x816 u8 ammoMenuLock // counted down to 0 by Player_UpdateStatus; Player_BowgunAmmoSwitch does nothing while nonzero. Player_Reset zeroes it; no C code sets it nonzero
0x864 s16 envTimer // frames to the next environment effect (30..180, Player_UpdateEnvironment)
0x669 u8 floorHazard // byte 7 of the floor kind's gAreaKindFlags row. Player_UpdateEnvironment: 1 poison (at least 10 unless SKILL_POISON_IMMUNE), 2 damage every 8 frames
0x7D9 s8 fishTimer // fishing: frames (Lure_Update, Fish_StepBite), counted down by Player_UpdateStatus
0x0D0 struct PlRec * partRecs[32] // the 32 records of 176 bytes in the 5632-byte block at 0x5DE5D0 (Player_InitPartTable, CamScript_AtPoint)
0x154 MotionLayer{80} motion[4] // motion layers (include/motion.h; as Em.motion): Player_SetLayerMotion starts one, Player_UpdateMotion steps it
0x4C6 u16 unk4C6
0x4E8 s16 modelCount // model slots used by the body (as Em.modelCount; Player_LoadPartModels, Player_FreeModels)
0x4EA s16 modelFirst // the body's first model slot (as Em.modelFirst)
0x4FC struct Model * partModels[6] // parts 1-5 (the equipment): the model (Model_Get of its first slot; Player_LoadPartModels); [0] unused, the body's is model
0x514 s16 partModelCount[6] // parts 1-5: model slots used
0x520 s16 partModelFirst[6] // parts 1-5: the first model slot
0x4EC struct Model * model // the skeleton (as Em.model): select_chara.c sets it to Model_Get(gAreaSubModelId[slot])
0x4F4 struct Model * weaponModel // the model: a 128-byte header of the pool at 0x6AE000 (Player_LoadWeaponModel)
0x4F8 s16 weaponModelSlotCount // model slots used by the weapon model (Player_AllocWeaponModel)
0x4FA s16 weaponModelSlot // the first model slot of the weapon model (Player_AllocWeaponModel, Player_FreeWeaponModel)
0x8A8 u8 lookPartId[6] // per armour part: the model part id Look_MapPartId picked (Player_LoadLookModels), PL_LOOK_PART_NONE: none (Player_InitPartTable)
0x8AE u16 weaponModelKind // the kind of the loaded model, PL_WEAPON_MODEL_NONE: none (Player_LoadWeaponModel)
0x8B0 u8 headAltModel // 1: the head armour has no model for the gender and part slot 6 is used (Armor_LacksSexModel; Player_LoadLookModels, Player_QueueLookFiles)
0x6A0 f32 mapPos[3] // the position with the area record offsets, y at the middle of its range, for the map (Player_UpdateAreaPos; Map_DrawArea)
0x8C6 s8 aimPitchPct // the bow's aim pitch, -100..100 percent: Player_AddAimPitchPct turns it into aimPitch (45 deg up, 35 deg down); the arrow's rotX (attack_families shot spawn); synced online (qnet_sync k1.unk22)
0x3E7 u8 nextRow // the attack row started when the player's own hit window closes, 0: none (Player_UpdateHitTimers runs Player_StartNextHit with it); as ObjAttach.nextRow
0x3EF u8 hitAttrs // attribute bits of the attacks taken, ORed in from the attack object +0x65 (ObjAttach_DamagePlayer): 0x80 cannot be guarded without SKILL_GUARD_PLUS20 or with the heavy bowgun (Player_CanGuardHit)
0x3F0 u16 hitAngle // direction of the last hit taken (ObjAttach_SetHitHeading; Player_CanGuardHit; as Em.hitAngle)
0x3F2 s16 hitForce // the force of a hit taken while guarding (Player_GuardHit)
0x3F4 u16 lastAttackKind // the kind of the attack taken (the attack object +0x63; Player_GuardHit compares 4, 15, 21)
0x8BD u8 hornNoteSlot // hunting horn: the slot of the last note in hornNotes (0..7)
0x8BE u8 hornNotes[8] // hunting horn: the last 8 notes played (HuntingHorn_AddNote)
0x8C8 u8 hornSong // hunting horn: the song the last notes matched (HuntingHorn_CheckSongs)
0x662 u8 floorAttr // the floor triangle attribute (Em.floorAttr, a u16 there): EmScript_IfScriptPlayerFloorAttr compares the byte, EmScript_PickNoticedPlayerSameFloor reads the u16 (1..3, 4..10)
0x73A u8 unk73A_combat // set by Em_SetCombat; cleared by Player_UpdateInBattle when the monster (its bit in Em +0x73A) resets
0x018 u8 hitKind // a hit to react to (SwordShield_CheckHit and the other weapons' copies): 1 a state change, 2 and 3 a flinch
0x019 u8 hitPending // nonzero: hitKind is to be handled
0x589 u8 rideEm // the slotNo of the monster the player stands on (Em_CarryRiders, Player_CheckRideSlip)
0x59C f32 rideDelta[3] // how far the monster carried the player this frame (Em_CarryRiders; Player_CheckRideSlip compares its length)
0x5A8 EquipFxSwing{32} equipFx[4] // the swinging parts of the equipment (Player_RunEquipEffects, Effect_RunList); cleared by Player_ClearFxBlock
0x773 u8 prevInBattle // the previous inBattle (Player_UpdateInBattle)
0x3AC s32 targetAngle // the heading from target, plus a quarter turn (Player_UpdateInBattle)
0x3B4 void * target // the Em it deals with: the monster in battle with it that set inBattle (Player_UpdateInBattle), the one it hit (ObjAttach_DamagePlayer), in the lobby the townsperson it talks to
0x8B8 s8 mashTimer // a countdown between the button presses that shake off a stun (Player_UpdateMashEscape)
0x8B9 u8 savedCoatingSlot // bow: a copy of coatingSlot (taken at a shot, with savedAmmoCount, unk829 and savedClipSize, and by the ammo menu Player_BowgunAmmoSwitch, which restores it)
0x822 s16 cookTimer // the cooking timer (frames on the spit, Player_StateSpitRoast)
0x828 u8 cookResult // the cooking result: 0 raw, 1 rare, 2 done, 3 burnt, 4 gourmet (Player_StateSpitRoast)
0x85C u16 offerItem // an item offered to another player (Player_TakeOfferedItem)
0x85E s16 offerCount // its count
0x861 u8 offerTo // the index of the player it is offered to
0x862 u8 offerState // PL_OFFER_*: the answer to an item offer (the giver holds the asker's index for a moment; QSYNC_PL_ITEM_ASK and QSYNC_PL_ITEM_ANSWER)
0x7F4 PlayerPend{16} pend[2] // positions received over the network, applied when their timers run out (QSync_QueuePlayerPos, QSync_UpdatePlayerPos)
0x893 s8 fallGrabCount // the ledge grabs out of a fall: Player_StateLedgeHang counts PL_ACT_LEDGE_GRAB after PL_ACT_FALL; Player_Update zeroes it in any other state; Player_StateLeapOrFall allows the grab while it is under 21
0x535 u8 savedAmmoType // ammoType saved while the ammo menu is open (Player_BowgunAmmoSwitch)
0x668 u8 mustCrouch // byte 2 of the floor kind's gAreaKindFlags row. While set, moving is PL_ACT_CROUCH_WALK (no walk, run, roll, standing up or weapon draw)
0x863 u8 offerStarted // 1 in the frame an offer starts (offerRequest taken, PL_ACT_OFFER_ITEM): the pouch menu closes (PouchMenu_Update); cleared by Player_Update
0x366 u8 armourKind[6] // per armour piece a kind: sub_main copies it from the lobby record colour[] and the model setup (Look_MapPartId) reads it; main armour rattle sound picks from it
0x3F9 u8 hitIds[16] // the ids of the attacks that hit it last (as Em.hitIds)
0x412 u8 hitIdNext // the next slot of hitIds (as Em.hitIdNext)
0x844 u8 drunk // lobby: 1 while drunk: set by the tenth drink at a seat online (TownPc_CountDrink) and by the drink contest; the town's idle, walk and run take their drunk forms; sent in the state messages; talks and menus clear it
0x867 u8 drinkCount // lobby: the drinks taken at a seat while not drunk (TownPc_CountDrink); at 10 (online) drunk is set; cleared with drunk
0x6B2 s16 hitDamage[8] // the damage of the hit taken in [0] (ObjAttach_DamagePlayer; Player_ReactToHit); as Em.hitPartDamage, one per part; cleared by Hit_ClearStatusAll
0x6FA s16 status5Hit // the hit status 5 amount, added to status5Buildup (Player_ReactToHit; as Em.status5Hit)
0x700 s16 status2Hit // the hit status 2 amount, added to status2Buildup (Player_ReactToHit; as Em.status2Hit)
0x708 s16 poisonHit // the hit poison amount, passed to Player_AddPoison (Player_ReactToHit; as Em.poisonHit)
0x712 s16 status4Hit // the hit status 4 amount, added to status4Buildup (Player_ReactToHit; as Em.status4Hit)
0x748 s16 mashBuildupHit // the hit amount added to mashBuildup (Player_ReactToHit)
0x8CC s16 status6Hit // the hit status 6 amount: > 0 starts status6Timer unless SKILL_STATUS6_IMMUNE (Player_ReactToHit)
0x74A s16 tradeGiveCount // the trader kind 0x0A: the count taken with tradeGiveItem, -1 or -99 (Pouch_Add; EmTrader_RunTalk)
0x74C u8 contactHit // as Em.contactHit: 1 once one of its attacks with ATKF_REPORT_CONTACT hits (ObjAttach_DamagePlayer, ObjAttach_DamageEm); each attack's Create clears it (PlayerAttack_Create)
0x395 u8 hitReceived // nonzero: a hit taken, to react to (Player_ReactToHit; as Em.hitReceived)
0x8D4 u8 status8Hit // the hit status 8 amount: > 0 lowers staminaMax unless SKILL_STATUS7_IMMUNE (Player_ReactToHit)
0x416 u8 fxMade // 1 once the state made its effect (WeaponTrail_Create) at its frame: LongSword_StateSlash, LongSword_StateSpirit1 (as Em.unk416)
0x060 f32 worldMtx[4][4] // as Em.worldMtx (the hunters states turn vectors with it, Vec_Rotate)
0x294 u16 yawCenter // as Em.yawCenter: the heading targetYaw is kept within a quarter turn of (Player_UpdateLean, Player_StateMountedGunAim)
0x29A s16 msgTimer // as Em.msgTimer (cleared by Player_StateFishReel)
0x568 f32 prevPos[3] // the position before the move of this frame (as Em.prevPosX..prevPosZ; TownPc_Update copies pos)
0x660 u8 floorKind // the floor triangle under the player (as Em.floorKind; TownPc_Update passes it to Em_GetFloorInfo as a TriHdr)
0x730 f32 surfaceY // as Em.surfaceY (TownPc_Update passes it to Em_GetFloorInfo)
0x842 u16 baited // the lure keeps its bait: 1 from the cast (Player_StateFishCast, the town's cast); a fish goes for the lure only while it is set (Fish_StepSwim) and a nibble clears it one time in four (Fish_StepBite); with it the action button picks PL_ACT_FISH_REEL_BAITED over PL_ACT_FISH_REEL_EMPTY (in the town PL_TOWN_FISH_REEL_BAITED over PL_TOWN_FISH_REEL_EMPTY)
0x3EC u8 netActive // the lobby: set when a town message moves or places the player (TownMsg_OnActor, TownMsg_OnMove, TownMsg_OnArea)
0x840 u16 gotItemId // the item id that goes with gotItem (Pouch_GiveItem in TownPc_State63 and the fishing states; the lure's bite code switches on it)
0x634 u16 tradeGetItem // the trader kind 0x0A: the item it gives the player (Pouch_Add with 1, Notice_Push; EmTrader_RunTalk)
0x636 u16 tradeGiveItem // the trader kind 0x0A: the item the player hands over (Pouch_Add with tradeGiveCount; cleared from gQuest.unk158 when it matches)
0x58A u8 rideBone // the bone of the monster rideEm the player stands on (Em_CarryRiders: Em_PosInFrame with it)
0x86E u8 questStatus // the lobby: low bits the quest category (QuestFlags_GetCategory), QUEST_STATUS_PAID accepted and paid, QUEST_STATUS_TAKEN accepted (QuestDesk_Accept), QUEST_STATUS_SET_OUT set out (Town_UpdateMode); cleared by QuestDesk_Drop; on the hunter card (HunterCard_Build)
# Round 31 session 31 (types): the hit fields (sub_main hit_detection.c)
0x414 u8 hitPhase // the player's hit window: 0 off, 1 waiting hitDelay frames, 2 open for hitFrames (Player_UpdateHitTimers; as ObjAttach.hitPhase)
0x424 f32 hitPos[3] // where the last hit taken landed (ObjAttach_DamagePlayer)
0x4BC struct ObjAttach * hitAttach // the attack object of the last hit taken (ObjAttach_DamagePlayer; as Em.hitAttach)
0x6D4 u8 partFlags[8] // as Em.partFlags: the mark bits of the hits taken; ObjAttach_DamagePlayer ORs the attack's hitMarks into [0]; cleared by Hit_ClearStatusAll
0x71A s16 status1Hit // the hit status 1 amount (Em_ApplyHitStatus; as Em.status1Hit)
0x396 u8 flinchPart // as Em.flinchPart: set to 0 at a hit (ObjAttach_DamagePlayer)
