# Fields of Em (include/em.h). Regenerate with: python tools/gen_struct.py include/em.spec include/em.h
name Em
size 0x9D0
0x000 u8 active // nonzero while the slot is in use
0x002 u8 type // monster kind: index into the per-kind tables
0x014 u8 stateGroup // state group, set with the state (sub_main 0x3AB7F0)
0x0B8 f32 scaleX // scale (scaleX..scaleZ; EM_BASESCALE times the size)
0x390 u8 stance // 0: on the ground, 1: family-specific (Kushala kinds hovering, Lao-Shan, Shen Gaoren), 2: in the air (flying, leaping out of the sand), 3: dead (QuestEm_Hunted), 4: under the sand or water (Diablos, Cephadrome, Plesioth)
0x662 u16 floorAttr // the u16 at +2 of the floor triangle (TriHdr.unk02, Em_GetFloorInfo): EmScript_Op5F and Op62 test 1..3, EmBlangonga_ScriptQuery 1, func_00708640 nonzero without bit 0x8000; GmEm_ResetState sets 1 (Player +0x662: compared by EmScript_Op5A)
0x680 u16 area // the area (map zone) the monster is in (gScene.unk014 is the current one)
0x6AE u8 hitReactMode // how Em_UpdateDamage reacts to a hit: 0 normal flinch, 2 no flinch (reaction 0xA; Em_BeginStatus1/2, captured states set it every frame through Em_SetHitReactMode), 3 reaction 0xD (EmFatalis_KnockDown), 4 kept through the hit (Em_BeginStatus4/5); Em_ClearHitReactMode resets it each frame; EmScript_Op0A sets, Op32/Op33 test it
0x6F6 s16 status5Buildup // raised by Em_AddStatus5; reaching status5Threshold sets statusFlags 0x10
0x6F8 s16 status5Decay // frames to the next decay of status5Buildup
0x6FC s16 status2Threshold
0x6FE s16 status2Buildup // raised by Em_AddStatus2; reaching status2Threshold sets statusFlags 2
0x702 s16 status2Decay // frames to the next decay of status2Buildup
0x704 s16 poisonThreshold // from D_008BCCB0 (per kind); poisonBuildup reaching it poisons the monster
0x70E s16 status4Threshold
0x710 s16 status4Buildup // raised by Em_AddStatus4; reaching status4Threshold sets statusFlags 8
0x714 s16 status4Decay // frames to the next decay of status4Buildup
0x716 s16 status1Threshold
0x718 s16 status1Buildup // raised by Em_AddStatus1; reaching status1Threshold sets statusFlags 1; decays by the kind's decayAmount every decayInterval frames
0x71C s16 status1Decay // frames to the next decay of status1Buildup (Em_UpdateStatus)
0x71E u8 status // current status (1, 2, 4, 5), 0 when none; 5 is the stun (dizzy: the families stun states make StageObj_Stun); poison is statusFlags 4
0x71F u8 statusFlags // 1: status 1, 2: status 2, 8: status 4, 0x10: status 5
0x73A u8 combatPlayers // bit per player (1 << index) the monster fights: Player_UpdateInBattle sets them, Em_SetCombat and Hud_DrawPlayerStatus test them, Em_UpdateDamage clears them; 0xF (all) for Fatalis and Shen Gaoren
0x73B u8 status2Count
0x73D u8 status4Count
0x73E u8 status1Count
0x7A2 u8 scriptRestart // 1: EmScript_Restart restarts the script (clears scriptPending, scriptFlags); set by Em_SetRequest, Em_RestartScript, func_00708640/func_00708750 (with trigger 0x80); EmScript_Init clears it
0x7D4 s16 status5Threshold
0x7D7 u8 status5Count
0x7DC u8 combat // 1 while the monster fights the hunters (Em_SetCombat); back to 0 it forgets them (seen bits, alertBuildup, unk86C, unk7E4 cleared)
0x7DD u8 requested // 1 while a request (Em_SetRequest) is pending: the families' state choice leaves it alone
0x7DE u8 requestId // the pending request: an id of the kind's gEmKindInfo entry (Em_PickRequest)
0x7EC s32 need2Gauge // gauge of need 2 (request gEmKindInfo.need2Request): Em_DecayNeed2 lowers it (D_008BC690 rates, out of and in combat) and sets need2 at 0; Kut-Ku State0_0E refills it by 0xDE a frame up to need2Max; EmScript_Op21 compares it with 30% of the maximum; quest entry +0x10
0x7F0 s32 need1Gauge // gauge of need 1 (named after gEmKindInfo.need1Request, the request it makes): Em_DecayNeed1 lowers it by the kind rate (D_008BC520) and sets need1 at 0; refilled to need1Max by eating-like states (Kut-Ku State0_1A, with +5% hp); func_00702830 looks for an action spot under the kind threshold; from the quest entry (+0xC)
0x7F4 s32 need3Gauge // gauge of need 3 (request gEmKindInfo.need3Request): Em_DecayNeed3 lowers it and sets need3 at 0; refilled to need3Max by Em_RefillNeed3, Em_BeginStatus1/2, Em_ResetNotice; quest entry +0x14
0x7F8 s32 need2Max // maximum of need2Gauge (need2Gauge): Em_AddNeed2 caps at it
0x7FC s32 need1Max // maximum of need1Gauge (need1Gauge): Em_AddNeed1 caps at it, refills set it; Em_CheckRageByHp takes the percentage of it (kind 0x34)
0x800 s32 need3Max // maximum of need3Gauge (need3Gauge): Em_Add7F4 caps at it, the refills set it
0x804 s16 rageThreshold // the kind's EmRageRow threshold (gEmRageTbl[type]), -1 without one (Em_InitRage); Em_CheckRageByHp raises rageBuildup to it; the families pass it to Em_CheckRageByHp
0x806 s16 rageBuildup // raised by Em_CheckRageByHp (damage times a factor picked by the hp percent, doubled for some kinds with part 0 broken); reaching rageThreshold sets ragePending; Em_InitRage, Em_EndRage clear it
0x808 s16 rageTimer // frames left of the rage, counted down by GmEm_Update (Hermitaur sets 0x708)
0x80A u8 rage // nonzero while enraged: the kind's rage motion speed (GmEm_Update), Blangonga tints red (EmBlangonga_UpdateTint); func_00709A40 ends it
0x80B u8 rageStep // step of the rage timers (EmBlangonga_UpdateRageTimer, EmFatalis_UpdateRageTimer, EmDiablos_Update switch on it and step it); GmEm_SetMeshAlphas draws by it; Em_InitRage, GmEm_ResetState clear it
0x80C u8 ragePending // 1 when rageBuildup reached rageThreshold (Em_CheckRageByHp); GmEm_Update then starts the rage (func_703600) outside group 4 and noReact; cleared by Em_StartRage2, Em_EndRage
0x80E s8 enterDelay // frames: GmEm_Update sets 2 while the monster is outside the scene's area; in it, func_00708640 and func_00708750 count it down and raise no trigger 0x80 until it is 0; GmEm_ResetState clears it
0x80F u8 guardTimer // frames the hits cannot kill (hp kept at 1, also against poison), flinch or add status buildup (Em_UpdateDamage, Em_PickDamagedPart, Em_UpdateStatus), and Em_CheckFlash ignores flashes; counted down by GmEm_Update; states set 2 or 5, Em_BeginStatus2 30
0x810 u8 flashed // 1 when a flash object is in the sight cone (Em_CheckFlash, unk_6FF630.c); cleared when Em_ApplyFlash applies it
0x811 u8 noReact // 1 while a status (Em_BeginStatus1/2/4/5 .. EndStatus) or a special state runs: GmEm_Update raises no triggers or rage actions, Em_CheckPartBuildup and func_00708640 do nothing; GmEm_ResetState clears it
0x814 u8 need2State // 0 until need2Gauge first runs out (Em_DecayNeed2 sets 1 with need2); EmScript_Op0C sets, Op2B compares, Op0D/Op1E and Em_ClearNeed2State clear it
0x815 u8 need1State // 0 until need1Gauge first runs out (Em_DecayNeed1 sets 1 with need1); Em_ClearNeed1State, EmScript_Op1E clear it
0x816 u8 noAlert // set by EmScript_Op26 (the script byte), 0 at reset: while 1, GmEm_Update raises no sight trigger (0x20) out of combat and the kind Update functions (Blangonga, Cephadrome, Kushala, Khezu) start no rage; func_00708640 does nothing unless 0
0x817 u8 remote // online: 1 when another player's console runs the monster (hostNo), 0 when this one does (EmSlot_Alloc, GmEm_Setup); most AI code runs only when 0
0x846 s16 aimPitch // as Player.aimPitch (same offset): Player_AddLean846 (player code typed Em) adds to it, clamped to +-0x2000
0x848 s32 alertBuildup[4] // one per player, out of combat: func_00703D60 adds the kind table values for the player action (Em_PlayerActionKind), range and sector while the player is near (Em_SensePlayers); Em_CheckPartBuildup raises trigger 0x10 and alertedPlayers bit at the kind limit D_008BB660 and clears it; Em_SetCombat clears all
0x86B u8 triggers // script triggers pending, one bit each (0x02..0x80): EmScript_PickTrigger starts the script of the first
0x86C s32 noticeCount[4] // one per player: notice count (Em_AddNotice; at 9000 the player is noticed)
0x8B2 u16 collMask // per-kind collision mask gEmCollMasks[type] (Em_InitCollMask): func_00330EB0 (wall collision) and func_32E9D0 (path checks of func_00702830, EmScript_Op3F) use it; Apceros State1_05 adds 0x80
0x958 struct Em * link // the monster of the linked kind (Em_FindLink)
0x964 u8 questKey // byte +0x18 of the quest monster entry (QuestEm_Spawn): key of the id sets (Em_IdSetLookup, Em_IdSetHas match it or -1); EmScript_Op57 compares it; Rathalos Update tests 1
0x966 u8 large // per-kind flag gEmIsLarge[type] (Em_InitKindParams), nonzero for the large monsters: synced online (GmEm_Setup, GmEm_Update, Em_SyncState, Em_IsSyncFree), death handled by the area owner (Em_UpdateDamage, Em_UpdateStatus), target point Player.targetPos (Em_GetTargetPos); EmScript_Op23 kind 0x4A matches any of them
0x967 u8 requestPrio // priority of the pending request (0x80 low HP ... 0x40 idle); a new one needs at least this
0x968 u8 lowHpNeed // 1 when hp fell under the kind's ratio (Em_CheckLowHp): Em_PickRequest makes it a request of priority 0x80
0x96A u8 need1 // 1 when need1Gauge ran out (Em_DecayNeed1): Em_PickRequest makes request gEmKindInfo.need1Request at priority 0x60; Em_ClearNeed1State, EmScript_Op1E clear it
0x96B u8 need2 // 1 when need2Gauge ran out (Em_DecayNeed2): Em_PickRequest makes request gEmKindInfo.need2Request at priority 0x50; Em_ClearNeed2State, EmScript_Op1E clear it
0x96C u8 need3 // 1 when need3Gauge ran out (Em_DecayNeed3, kinds with gEmKindInfo.need3Request): Em_PickRequest makes request need3Request at priority 0x40 out of combat; the status begin/end functions, Em_RefillNeed3, EmScript_Op1E clear it
0x96D u8 showOnMap // per kind (D_008BF2D0[type], Em_InitKindParams): nonzero: Map_DrawArea draws the monster, Hud_DrawPlayerStatus marks its target player
0x970 u8 startsBattle // per kind (D_008BF970[type], Em_InitKindParams): nonzero: Player_UpdateInBattle counts the visible monster in the player area as a battle
0x015 u8 state // current state: the family's state dispatcher switches on it
0x0A0 s32 rotX // rotation angles as integers, 0x10000 per turn (rotX..rotZ are read as a vector: Mtx_FromAnglesYXZ)
0x0A4 s32 rotY // heading: rotation about y, 0x10000 per turn (Math_AngleXZ to the goal)
0x0A8 s32 rotZ // rotation about z, 0x10000 per turn
0x0AC f32 posX
0x0B0 f32 posY
0x0B4 f32 posZ
0x29C u16 motionId[4] // motion id per layer: 1000 + 200 * layer + the row's motion (Em_SetMotion, Em_SetLayerMotion)
0x2A4 u16 motionFrame[4] // start frame per layer (Em_SetMotion)
0x2AC s16 motionBlend[4] // blend frames per layer, halved (Em_SetMotion)
0x2C0 u16 motionLayerCount // as Player.motionLayerCount: number of motion layers in use (Em_SetMotion, Em_SetMotionRow loop over them; EmKind_MotionGroupCount(type) in GmEm_Setup, 1 or 2 in Em_SetupKind)
0x2C2 s16 hp
0x409 s8 side // as Player.side (same offset): copied into the attacks (Eff*_Create o->unk0D1), Side_Match against players (EmScript_Op52, GmEm_Setup), versus team (Versus_Team: size and rage scales); the carving player is gPlayers[side - 1] (State8_00)
0x40E s16 noHitTimer // frames the monster cannot be hit: ObjAttach_HitPlayer skips it while nonzero; counted down by Player_TickTimers (GmEm_Update); states set 5/10/2, GmEm_Update 2 in group 8 (carving); Em_CheckFlash ignores flashes while > 0; players: Player_SetEvadeFrames
0x410 s16 noPushTimer // frames without body pushing: Field_PushApart skips the object while nonzero; counted down by Player_TickTimers; states set 5/2/10
0x418 s16 rowBlend // blend frames of the motion row (+7 of the row: Em_SetMotion, Em_SetMotionRow store it); Em_SetMotion uses it instead of the given blend when nonzero
0x473 u8 unk473
0x4EC struct Model * model // skeleton: bones and their matrices
0x568 f32 prevPosX // position before this frame's move (prevPosX..prevPosZ, read as a vector: Vec_Copy): GmEm_Update and Em_UpdateState store posX..posZ, Player_Update the player position; func_00330EB0 sweeps the wall collision from it; func_00708850 puts the monster back there
0x570 f32 prevPosZ // z of the position before this frame's move (see prevPosX)
0x59B s8 targetNo // index in gPlayers of the target player, -1: none
0x6DE s16 hpMax
0x70C s16 poisonDamage // parameter +8 of the kind, set when poisoned (Em_AddPoison)
0x724 f32 attackFactor // as Player.attackFactor (same offset): the attack factor ObjAttach_DamagePlayer reads; the kind's rage row +8 times Em_RageScale1 while enraged, Em_RageScale1 (gEmScaleRows row, versus team scale) otherwise
0x728 f32 damageFactor // multiplies the part damage the monster takes (Em_UpdateDamage, Em_PickDamagedPart); the rage row +0xC times Em_RageScale2 while enraged, Em_RageScale2 otherwise (Player has defenseFactor at this offset)
0x779 u8 goalKind // what the goal is (Em_ComputeGoal): 1 a player, 2 a point of the area, 3 another area, 5 the kind's point, 6 relative to the heading, 8 objectPos, 9 a route point
0x77A s16 goalArg // first argument of goalKind (table, route or player)
0x77C s16 goalNo // second argument of goalKind (point or player index)
0x78D u8 scriptPending // 1: run the script once (Em_UpdatePendingScript calls EmScript_Run when scriptDelay is down to 0, then clears it); set by Init and the EndState functions, 0 by ChangeState and captured/hunted states
0x7C3 u8 syncRequest // a request to send the monster state (QSync_SendEm sends it and clears it): 1 by Em_RequestSync, 2 by it or EmScript_OpFF unless 1 is pending; Em_RestartScript runs only when 0 on the host
0x7CC struct StageObj * k09Obj // its stage object of kind 9 (StageObj_K09_Create); Em_ResetToBasePose places it (StageObj_K09_Place), GmEm_ResetState clears it
0x7DA s16 combatTimer // frames of combat left without a seen player (func_00708990 counts it down; at 0 the monster calms down or leaves)
0x7DF u8 blind // nonzero: Em_SeePlayers sees nobody; counted down each frame (GmEm_Update), many states set 2 every frame
0x7E3 u8 noticed // players the monster has noticed, one bit each (Em_AddNotice, Em_UpdateNoticed)
0x884 f32 motionSpeed
0x888 f32 goal[3] // the point the monster moves or turns to, computed from goalKind (Em_ComputeGoal)
0x89E s16 areaTimer1 // a countdown set from the area values (Em_SetAreaValue89E), counted down by GmEm_Update; EmScript_Op29 tests it > 0 (twin of areaTimer2)
0x8A0 s16 areaTimer2 // a countdown set from the area values (Em_SetAreaValue8A0; Hermitaur 0x708 in Setup and Update), counted down by GmEm_Update; EmScript_Op2A tests it > 0
0x8A2 s16 blindTimer // frames of flash blindness: Em_StartBlind sets D_008BEAC0[type] (and makes the stars, StageObj_Stun_CreateEm), Em_ApplyFlash copies it into timer; StageObj_Stun_Run ends at <= 0; Em_ClearPoison and GmEm_ResetState clear it; Kushala wind stops while > 0
0x96E u8 holdTimer // nonzero: the monster is held (Em_IsHeld); Em_SetCaptured(em, 0) and the trapped group-4 states set 5 every frame, Kut-Ku SetGroup4State 50; counted down by GmEm_Update; the trap Eff12_Run catches a monster with it > 0 within 700 and lets go at 0; the families pick other reactions while it is set
0x975 u8 moveFrozen // 1 when func_00708850 undid the move (in combat, no hunter in its area, the state not the given one: back to prevPosX/prevPosZ); GmEm_Update then keeps prevPos and runs the motion with flag 0x20000; GmEm_ResetState, func_00708850, Khezu and Plesioth Update clear it
0x989 u8 hitzoneSet // which of the kind's damage tables ObjAttach_DamageEm uses (D_90CBD0[type][hitzoneSet]): 0 normal, 1 while Chameleos is invisible, the Kushala wind blows, after a part break with flag 1 (Em_SpawnPartEffects), Hermitaur 2 in State3_04; ChangeState and GmEm_ResetState clear it
0x98A s8 level // strength level from the quest monster entry (+0x36, QuestEm_Spawn; versus: Versus_AssignEmTeam): row of the size, hp and rage scale tables (Em_ComputeSize, Em_RageScale1/2), part hp scaling for 11..17 (Em_ScalePartHp); QuestEm_Appear raises it up to 15; EmScript_Op71 compares it
0x98C s16 trapTimer // frames the trapped states (State4_0D, 4_11, 4_17 ...) last, counted down there: Em_SetGoal12Dist sets 0x340, 0x208 or 0x138 when enraged (0x9C kind 0x35); goalKind 12 carries it as goalNo (/ 4, Em_SetGoal12, Em_ComputeGoal2)
0x994 s16 scriptDelay // frames before the pending script runs: Em_UpdatePendingScript counts it down, then runs the script if scriptPending (scriptPending); set by EmScript_Op48, cleared by EmScript_Restart
0x9A1 u8 rageStarted // 1 from Em_StartPendingRage to Em_EndRage; GmEm_Update then sets itemUseKind to 0x88
0x9A2 u8 stateArmor // 1 while the state is in a move the monster does not break off: set by the family states, cleared each frame by Em_ClearHitReactMode (with hitReactMode); ObjAttach_HitAll ignores attack kind 0x13 then, Blangonga ignores the flash (EmBlangonga_IgnoresFlash), Hermitaur takes no group-4 reaction (EmHermitaur_AllowHit8); the ScriptQuery functions report it
0x9A3 u8 holdTimer2 // the second hold (Em_IsHeld): Em_SetCaptured(em, 1) and Em_SetHold2 set 5; counted down by GmEm_Update; the trap Eff10_RunTrap catches a monster with it > 0 within 500 and lets go at 0; cleared by the SetGroup0State functions
0x020 f32 rotMtx[4][4] // rotation matrix built from rotX..rotZ (Mtx_FromAnglesYXZ)
0x2C4 EmPart{8} parts[9] // 9 parts (Em_InitParts): active +0, index +1, hp +4, state +6
0x004 u8 taskPhase // GmEm_Task: 0 setup, 1 update, 2 finish, 3 free
0x008 s32 timer // counts down in many states (`if (--em->timer <= 0)`)
0x3A2 u16 stateRand // Rand_Next(1) drawn when a state starts; states pick variants from it
0x6E0 u8 syncKind // kind of the last synced state change (Em_SyncState, large monsters), sent online (QSync_SendEm msg 0x16, QSync_QueueEmState)
0x6E1 u8 syncArg // second argument of the last synced state change (Em_SyncState), sent online (msg 0x17)
0x6E3 u8 syncGoalKind // curGoalKind at the last synced state change (Em_SyncState), sent online
0x6F4 u8 syncGoalArg // curGoalArg at the last synced state change (Em_SyncState), sent online
0x6F5 u8 syncGoalNo // curGoalNo at the last synced state change (Em_SyncState), sent online
0x78F u8 scriptFlags // the running script thread: 0x20, 0x10 trigger scripts, 1 a loop (EmScript_Op0F); EmScript_GetPos picks the resume position by it
0x7E2 u8 hostNo // the player whose console runs the monster (online; GmEm_Setup, func_00707060)
0x7E4 s16 noticeTimer[4] // one per player: frames the player stays noticed (from Em_NoticeTime)
0x813 u8 need3State // 0 until need3Gauge first runs out (Em_DecayNeed3 sets 1 with need3); EmScript_Op1E clears it
0x858 u16 playerAngle[4] // u16[4], one per player: direction from the monster to the player (Em_SensePlayers: Math_AngleXZ, as v->dir); Em_AngleSector finds the sense sector it falls in
0x860 u16 sectorHalfAngle[4] // u16[4]: half widths of the 4 sense sectors around the heading, from the kind's sense row D_008BB150[type] (func_00704060, with the ranges at 0x838..0x844); Em_AngleSector
0x869 u8 alertedPlayers // one bit per player whose alertBuildup (alertBuildup) reached the kind limit (Em_CheckPartBuildup, with trigger 0x10); EmScript_Op11 tests the bits
0x8A8 u16 syncRand // a random value drawn at each synced state change (Em_SyncState: Rand_Next(0)) and sent with it online (QSync_SendEm, QSync_OnEm, QSync_QueueEmState)
0x8B4 EmSpawn{76} spawns[2]
0x94C f32 objectPos[3] // f32[3]: position of the object (bait, meat...) Em_SenseObjects found within its radius; the goal of goalKind 8 (Em_ComputeGoal, Em_ComputeGoal2)
0x95D u8 hostTimer // 60 when this machine takes the monster over as host, counted down each frame (Em_MigrateHost)
0x965 s8 senseDelay // while nonzero (counted down each frame), GmEm_Update skips sensing the players (Em_SensePlayers, Em_SeePlayers); states set 5
0x969 u8 need4 // 1: Em_PickRequest makes request gEmKindInfo.need4Request at priority 0x70 (then clears it); set by func_007065C0 and EmScript_Op30, cleared by EmScript_Op1E
0x997 u8 curGoalKind // the goalKind the goal was last computed from (Em_ComputeGoal)
0x998 u16 curGoalArg // the goalArg the goal was last computed from
0x99A u16 curGoalNo // the goalNo the goal was last computed from
0x87C s8 loopPoint // the area point the script loop goes to (EmScript_Op10: goalKind 2, goalArg 1, goalNo = it), -1: none; picked at random, from a table or in turn (EmScript_Op10, EmScript_OpFF); EmScript_Init, EmScript_Restart, Em_TypeSetup set -1
0x87D s8 loopCount // passes of the script loop (EmScript_Op0F: at most the area entry's point count); EmScript_OpFF loops while the pass count (+0x87E) is below it; -1: none
0x3D0 struct EmVt * handlers // the kind's handler table (EmVt): init, update, ...; Em_InitHandlers sets the default one
0x984 u16 sizePct // size in percent (quest entry +0x30, QuestEm_Spawn; 100 by default; versus team): quest size records (QuestEm_Hunted, QuestEm_Captured: gQuest.sizeMax/sizeMin), QuestEm_Appear grows it by 5, the mine radius and collision shapes scale with it
0x9A4 u16 partsBroken // broken parts, one bit each (Em_BreakPart; Em_PartBroken tests them)
0x9A6 u16 partsToBreak // parts to break, one bit each (Em_FlagPartBreak; Em_ApplyPartBreaks breaks the new ones)
0x00C u16 index // slot number in the monster array (EmSlot_Alloc; o->idx)
0x01B u8 variant // the variant from the quest monster entry (QuestEm_Spawn): size row for Em_ComputeSize and D_008C5370 (Vespoid), row of D_008C4140 (Drome kind 1D), Melynx 0xC/0x80, Hermitaur's shell (level % 3)
0x30F u8 modelSlot // the scene's monster model slot (Quest_SpawnMonsters, QuestEm_Spawn: type = gScene.emKind[slot]); its textures are gTexHandles[slot * 20 + 0xA3 ...] (GmEm_SetMeshAlphas, StageObj_K09_Draw)
0x8AF u8 spawnParam // byte +6 of the quest monster entry (QuestEm_Spawn): Velociprey/Anteka/Melynx Init switch on it, Shakalaka picks 2 or Rand_Next(1) & 1 and reads D_008CA310[it] (a distance), Kind38 bit 1, EmScript_Op75 compares it
0x96F u8 questIndex // index of the quest monster entry it came from (+0x2C, QuestEm_Spawn); Quest_OnRemoteKill matches it with the entry index
0x7E0 u8 seen // players in sight this frame, one bit each (Em_SeePlayers); Em_UpdateNoticed makes them noticed
0x7E1 s8 mineSlot // mine point on the carcass (EmMine_Create), -1 when none
0x7CA u8 mineBone // bone the carcass mine point follows, 0: the position (EmMine_Create)
0x9A8 s8 mineSlot2 // second mine point (EmMine_CreateTimed), -1 when none
0x9A9 s8 mineCount2 // gathers for the second mine point
0x9AA s16 mineTimer2 // frames left for the second mine point
0x574 f32 floorY // height of the floor under the monster (Em_GetFloorY, Em_GetFloorInfoEx)
0x584 f32 bodyScale // scale of the wall collision shapes (func_00330EB0) and of the carcass mine radius (EmMine_Create); D_8B1620[type] (GmEm_ResetState), the scale for the small kinds (Vespoid, Anteka, Apceros, Drome kind 1D Init)
0x001 u8 visible // nonzero while the monster is in the field (drawn; cleared when the carcass goes)
0x9AD s8 windTimer // 2 while the Kushala wind blows (EmKushala_Update with EmKushala_WindActive), counted down by GmEm_Update; Player_NearEm40 looks for a monster near the player with it set
0x9A0 u8 versusGauge // versus gauge 0..90, 0xFF: none: raised by Versus_AddGauge, lowered by 30 by Versus_MonsterDown; Em_StartPendingRage makes a rage of gauge * 30 + 450 frames from it and clears it; synced (QSync_PackEmState)
0x720 u16 partMask // parts whose hp scales with level, one bit each (Em_SetPartScaled, Em_ScalePartHp)
0x8A7 u8 partDamageMode // from the per-kind part table after the 9 hp values (Em_InitParts)
0x3A8 s32 targetAngle // angle to the target player - 0x4000 (Em_FindTargetPlayer)
0x3B0 f32 targetDist // distance to the target player (Em_FindTargetPlayer)
0x3B4 void * target // the nearest player in range, NULL when none (Em_FindTargetPlayer)
0x005 u8 step // step within the current state (0 on entry)
0x154 MotionLayer{80} motion[4] // motion layers (include/motion.h; as motionId[4]: Em_SetupKind sets the fourth one's step)
0x3F8 u8 lookAtTarget // nonzero: Em_UpdateBendTarget bends the body toward the target player (toward lookPoint when lookAtPoint is set), else straight; 1 while Em_TurnToTargetMotion and some group-3 states run, cleared on entry of almost every state
0x3B8 f32 velX // velocity x, in the monster frame (EmKind38_MoveByVelocity adds accel to it, rotates it by the heading and adds it to the position)
0x3BC f32 velY // vertical velocity (the curves of the families UpdateCurves)
0x3C0 f32 velZ // forward speed (the curves of the families UpdateCurves)
0x3C4 f32 accelX // acceleration x, added to velX each frame
0x3C8 f32 accelY // acceleration y: gravity in the jump and fall states (-10.0f, -2.0f...)
0x3CC f32 accelZ // acceleration z
0x43E s16 unk43E
0x458 s32 unk458
0x468 s32 unk468
0x46C s32 unk46C
0x474 s32 unk474
0x478 s32 unk478
0x47C s32 unk47C
0x684 u16 nextArea // the area the monster moves to (goalKind 3; GmEm and Em_PickRoutePoint move it there)
0x6E4 f32 alpha // draw opacity, 1.0: opaque (fades in and out; GmEm_Task sorts by it)
0x986 s16 destArea // the area the monster travels to, 0xFFFF: none: set by EmScript_Op18/Op1A (Area_IdForCurrentTime), goalArg of Em_ComputeGoal; the State2_08/2_0D travel states take it as nextArea, State2_09 clears it on arrival; Em_PickRoutePoint defaults it to nextArea
0x98E u16 aimAngle // angle the monster turns to, 0x10000 per turn (Math_AngleXZ to the goal; many states make it relative to rotY)
0x990 f32 goalDist // distance left to the goal (Vec_DistanceXZ, then counted down by velZ)
0x99C s32 flightTurn // turn left to make in flight, 0x10000 per turn: Em_UpdateFlight takes it from aimAngle and turns rotY toward it by a step each frame until 0
0x006 u8 stateVar0 // per-state scratch byte (flag or counter: "once" flags with Player_PassedFrameScaled, hit counts, table indexes); cleared with the step on every state change
0x007 u8 stateVar1 // second per-state scratch byte like stateVar0 (flags, counters, 0xFF: none, a motion layer); cleared with the step on every state change
0x00E u16 targetYaw // heading to turn to, 0x10000 per turn: EmKind0A_TurnToYaw steps rotY to it; set from rotY, Vec_Heading(...) - 0x4000 or an action spot angle (func_0070E6C0); the player code (Player_BowgunAim, Bow_UpdateAim) moves it by 0x240 steps
0x011 u8 gender // as Player.gender (same offset): Anteka picks it at random or from the variant (EmAnteka_Init) and uses other motions when set (EmAnteka_SetMotion); Kirin and Hermitaur Init clear it
0x013 u8 slotNo // the slot number again, as a byte (EmSlot_Alloc)
0x016 u8 prevStateGroup // the state group before the last change
0x017 u8 prevState // the state before the last change
0x019 u8 hitPending // as Player.hitPending (same offset, same writer): set when one of its attacks hits (ObjAttach_DamagePlayer); the attack creators (Eff00..Eff06_Create, EffEm07_CreateAt) clear it
0x0BC f32 scaleY
0x0C0 f32 scaleZ
0x396 u8 flinchPart // the part whose hp ran out on the last hit (the most damaged one); the families pick the flinch from it
0x3A4 s32 motionTime // as Player.motionTime (same offset): Player_TickTimers adds 2 a frame, wraps 100000 to 10000; states reset it to 0 and compare it (> 18, >= 240, == 300, == 600)
0x3F0 u16 hitAngle // direction of the last hit taken, 0x10000 per turn (ObjAttach_SetHitHeading); states face it with rotY = hitAngle + 0x8000
0x437 u8 fieldCollide // 1: func_00330EB0 collides the monster with the walls (spheres per kind) and fills wallHitDirs; 0 in hunted, captured and some entrance states; 1 from Em_SetupKind, GmEm_ResetState, the EndState functions
0x438 u8 work // first byte of the state functions' work area (EmWork, em_work.spec); the families also read its bytes as Em fields
0x439 s8 fortStage // the fort monsters: the hits on the fort (their views' fortHits), copied to gScene.fortStage; sent in message 8 (QSyncEmData state1, QSync_PackEmState); EmWork byte 1
0x43A s16 syncTimer // the fort monsters (their views' syncTimer, work +2): frames to the next state message; QSync_PackEmState restarts it at 150 when it sends one
0x43C u8 unk43C
0x43D u8 unk43D
0x440 u8 unk440
0x441 u8 minePoint // Gypceros: the mine point it leaves (MinePoint_AddUntimed; EmKutku_AddMinePoint)
0x442 s16 unk442
0x444 s16 unk444
0x446 s16 unk446
0x448 s16 unk448
0x44A u8 unk44A
0x44B u8 unk44B
0x44C s32 unk44C
0x450 u8 unk450
0x451 u8 unk451
0x452 u8 unk452
0x453 u8 unk453 // Gravios (0x1A, 0x2C): GmEm_SetPartFx fades the part colours by it (read as s8, / 60); EmGravios_PlaySeOnFrame tests 1
0x454 u8 unk454
0x45C s32 unk45C
0x460 f32 unk460
0x464 s32 unk464
0x480 u16 unk480
0x482 u8 unk482
0x483 u8 npcAct // lobby NPCs (an Em): the NPC's act (Npc_IsFree, Npc_IsAct51to55, Npc_IsAct4For50)
0x484 u16 unk484
0x486 u8 unk486
0x487 u8 unk487
0x488 s16 unk488
0x48A s16 unk48A
0x48C s16 unk48C
0x48E s16 unk48E
0x490 s16 unk490
0x492 s16 unk492
0x494 s16 unk494
0x496 u8 unk496
0x4C8 u8 meshShown[32] // one per model mesh, 1: drawn (in the EmWork area: work + 0x90): Em_SetupKind fills it from the variant tables gEmVariantShownMeshes/gEmVariantMeshes, GmEm_Setup sets all 32, Em_UpdateState cycles the variants; Diablos/Garuga/Gravios/Rathalos State4_04 clear element 1 at frame 6
0x52C struct DrawSlot * drawSlot // its draw slot (DrawSlot_Get), positioned at the Em every frame and put into an ordering table (gm_sub func_00905930, the lobby's townspeople)
0x530 s16 drawSlotId // an entry of the 0x6A5AD0 pool, -1: none (DrawSlot_Alloc; gm_sub func_00903EE0)
0x532 u8 painted // 1 while marked by a paint hit: ObjAttach_DamageEm sets it (attack kind 7) with paintTimer = 0x4650 frames; Player_TickTimers-side code (unk_399F70) counts paintTimer down and clears it (0xFF: never); Map_MonsterLocated shows the monster while set; synced online (QSync flag 0x10)
0x53A s16 paintTimer // frames the paint mark lasts: 0x4650 (ObjAttach_DamageEm, QSync_OnEm), counted down while painted is set, which clears at 0
0x59A u8 cartPlayer // the Felyne cart (kind 0x12): the player it carries (Player_StateCart sets it; EmKind12_State1_01 waits while that player's stateGroup is 4)
0x636 u16 msgItem // the item EmKind0A_SetMessage prints in its message (gItemNames), 0 at EmKind0A_Init
0x651 u8 lookAtPoint // 1: Em_UpdateBendTarget bends toward lookPoint instead of the target player (Rathalos State3_0C)
0x653 u8 stateRequest // as Player.stateRequest: 1 after Em_SetState (Player_ForceState): the family Update runs the new state a second time in the same frame, then clears it
0x654 f32 lookPoint[3] // f32[3]: the point bent toward when lookAtPoint is set (Em_NearestActionToBone with bone 0x23; Rathalos State3_0C)
0x698 s32 wallHitDirs // directions of the walls touched this frame, one bit per 1/32 turn relative to the heading (func_00330EB0: 1 << ((h - rotY) >> 11)); states test 0xF000000F (ahead) to stop a charge
0x6D4 u8 partFlags[9] // one per part (as parts[k]): bit 2 lets the part break (EmBlangonga_BreakParts, Velociprey), 0x20 (Kushala BreakParts below 60% hp, func_0083B8F0)
0x722 u8 hideShadow // nonzero: Effect01_Draw (the flat model drawn under the object, probably the shadow) skips it; GmEm_Update sets it from unk723 or stance 4 (under sand or water) unless hideShadowHold holds the value states set (2); counted down by Player_TickTimers
0x72C s32 surfaceDepth // an f32 kept in an s32 (read through casts): how far below surfaceY a swimmer stays
0x730 f32 surfaceY // height of the special floor found by Em_GetFloorInfoEx (water or sand); swimmers keep posY at surfaceY - surfaceDepth
0x735 u8 surfaceKind // nonzero when the floor under the monster is a special surface (byte 6 of the floor material row D_0043FB70[area][kind]: sand or water; Em_GetFloorY, Em_GetFloorInfoEx); GmEm_Update then puts floorY at surfaceY for most kinds; Diablos and Cephadrome dive only where it is set
0x74C u8 attackHit800 // set to 1 when one of its attacks with flag 0x800 hits (ObjAttach_DamagePlayer); the attack creators clear it; Melynx steal states (State1_04, State3_01) test it with contact
0x7A1 u8 areaSeqActive // 1 while the area sequence of EmScript_Op17/Op18 runs (Op18 sets it, OpFF clears it at the end); the SetGroup2State functions then aim the takeoff and landing at point 0 of table 3
0x7D3 s8 quakeTimer // set to 5 while the giant kinds stomp (Lao-Shan State0_02, Shen Gaoren and Yama Tsukami Update), counted down in their Update; Player_IdleActions knocks a player over (state 2/7) near a giant with it set
0x882 u16 prevArea // the area the monster came from, 0xFFFF: none: Em_EnterNextArea sets it to area before moving to nextArea; Em_PickRoutePoint skips routes back to it; EmScript_Op3D/Op3E test it; sent online (QSync_SendEm)
0x8AC s8 hit8Count // times the group-4 state 4 reaction ran (the ChangeState functions add 1 entering it, with unk89C |= 1); the AllowHit8 functions allow it while 0 (Hermitaur < 2; Kushala at 30% and 40% hp, Init sets 1 or 2); read as signed; GmEm_SetMeshAlphas hides parts while <= 0
0x8AD u8 unk8AD
0x8AE u8 speedBonus // percent added to the speed: GmEm_Update scales the motion speed by (speedBonus + 100) / 100, Em_TurnStep the turn step; walking states (group 1) set 10..200; Em_SetState and GmEm_ResetState clear it
0x8B0 u8 hideShadowHold // frames GmEm_Update leaves hideShadow as the state set it (states set 2 with hideShadow = 2); counted down there
0x8B1 u8 wallHitSpecial // 1 when a wall touched this frame has flag b[1] in its material row D_004407F0 (func_003294A0, which fills wallHitDirs too); Diablos func_007AC3D0 tests it with wallHitDirs 0xC0000003 at frame 60 (Player has its input lock here)
0x973 u8 ceilingTimer // nonzero while on the ceiling: the ceiling states (Khezu State2_18..2_22, Hermitaur 0x43 State2_04..2_08) set 5 every frame; counted down by GmEm_Update; a flinch then makes Khezu fall (state 4/1); EmScript_Op59 branches on it
0x977 u8 hasCeiling // 1 when there is a ceiling over the monster (AreaGrid2_GetCeiling, height in surfaceY; 1 with 1500 in the special areas): Khezu and Hermitaur kind 0x43 (EmKhezu_Update, EmHermitaur_Update); ChangeState and ScriptQuery test it
0x98B u8 hasGoal // 1: the state walks to goal (Em_StepToGoal, ends near it, goalDist from Vec_DistanceXZ), 0: no goal, the state runs on timer instead; set by the SetGroup1State/SetGroup3State functions from curGoalKind or targetNo
0x9AC u8 hermitTimer // frames (read as s8): EmHermitaur_State3_04 sets 10, GmEm_Update counts it down; while nonzero EmHermitaur_Update goes to state 4/06 (not in the air, no stateArmor) and kind 0x30 plays SE 0x2D; GmEm_ResetState clears it
0x003 u8 subType // a sub-kind: effect and stage objects typed Em index their per-kind count tables with it (EmFxSeg_End, AreaObj_K02_End, EffKushala29_End); func_0070DBB0 tests 1
0x01E u8 unk01E // cleared by GmEm_Setup (Player.unk01E, ObjHead.unk01E)
0x4B8 u16 statusIcons // as Player.statusIcons: StageObj_K05_* and EmFxSeg_Move read bit 0x8000 of the player they are given
0x776 u8 scriptCallEntry // entry of the EmScript_Op82 jump (script table k + 0xF); em_bulldrome.c reads 0x776..0x777 as one s16
0x777 u8 scriptCallEntryHi // cleared when the Em is set up (gm_em.c); the high byte of em_bulldrome.c s16 read
0x294 u16 yawCenter // the heading targetYaw is kept within +-0x4000 of (Player_UpdateLean, motion 0x19E); Player_StateMountedGunAim sets it from the action spot angle (kind 6) or targetYaw
0x296 s16 tradeStep // step of EmKind0A_Trade (switch), 0 when EmKind0A_ShowMessage starts a message
0x298 u16 msgId // the trader kind 0x0A: the message shown, a row of gEmTraderMsgs (EmKind0A_ShowMessage, EmKind0A_Trade)
0x29A s16 msgTimer // the trader kind 0x0A: frames since msgId was shown (EmKind0A_Trade counts to 999)
0x456 u8 unk456
0x498 s32 unk498
0x4A6 u8 unk4A6
0x56C f32 prevPosY // y of the position before this frame's move (see prevPosX)
0x628 s8 tutorialSlot // lobby NPCs in the village tutorial: the bit of gTutorial.mask that is this NPC (Tutorial_RunNpc; -1 none): its point on the route set, and the kind ThrownObj_Spawn gets in Npc51 state 3
0x634 s16 unk634
0x688 f32 startPos[3] // where the monster starts (as Player.startPos): EmKind0A_Init copies posX..posZ
0x7D0 u8 hookedBy // the player whose bait Plesioth took (targetNo; EmPlesioth_State2_11/2_12), compared with the player index in Player_StateFishReel
0x80D u8 hooked // 1 while Plesioth has taken the bait of a player in state 0/0x53 (the line: Lure_PlayerHasMonsterBait): EmPlesioth_State2_11 sets it with hookedBy = targetNo; Player_StateFishReel finds the monster on its line by it; EmScript_Op47, State4_0F clear it
0x398 u32 flags // as Player.flags: flag bits (Player_SetFlag/ClearFlag/TestFlag without bit 31; stateFlags at 0x39C with it); GmEm_Update and gm_sub func_00904CF0 set 0x20000
0x723 u8 shadowOff // nonzero: GmEm_Update sets hideShadow (as under the sand, stance 4) unless hideShadowHold runs
0x736 s16 unk736 // 2 while Em.wallHitDirs is set (gm_sub func_00904CF0)
0x7CB u8 inSceneArea // 1 while the monster is in the scene's current area (GmEm_Update sets and clears it)
0x89D u8 itemUseKind // 0x80 | Side_ItemUseKind(side) (GmEm_Update), 0x88 in versus quests (gm_sub func_00904CF0); EmScript_Op73 and EmScript_Op74 read the kind and keep 0x80
0x754 u8 *** script // the monster's script set (GmLoad_GetEmData): tables of script pointers by kind: [0] the main scripts (scriptNo, Em_StartScript), [1] the subscripts (EmScript_Op81), [2] to [14] the loop, trigger and area tables (unk_733B90.c), [0xF + k] EmScript_Op82's jump tables
0x774 u8 scriptNo // the script to run next (EmScript_Start)
0x778 u8 scriptCallDepth // cleared when a script starts; EmScript_Op81 sets 1 (saving the return at unk758 + 4 from 0), EmScript_Op82 sets 2 (saving it at unk758 + 8 from 1)
0x7C4 u8 * scriptPos // the current script command (Em_StartScript)
0x3F4 u16 lastAttackKind // as Player.lastAttackKind: the kind of the attack of the last hit (ObjAttach_DamageEm: attack object +0x63); Em_UpdateDamage and Em_PickStatus test 5, 6, 9, 0x13
0x470 u8 unk470
0x471 u8 unk471
0x472 u8 unk472
0x706 s16 poisonBuildup // raised by Em_AddPoison; reaching poisonThreshold poisons the monster (statusFlags 4)
0x70A s16 poisonTimer // set from parameter +2 of the kind while the buildup rises, 0 once poisoned
0x880 s8 areaSeqLen // length of the area sequence of EmScript_Op17, -1: none; EmScript_Op18 and OpFF end the sequence when areaSeqPos reaches it
0x881 s8 areaSeqPos // position in the area sequence (EmScript_Op18 reads the next area from table 14 at it and makes it destArea), -1: none
0x6EC struct Player * contact // the player of the last contact: the one it hit or that hit it (ObjAttach_DamagePlayer, with the attack at +0x6F0); Kut-Ku and Chameleos steal from it; in the lobby the player talking to the townsperson
0x6F0 struct ObjAttach * contactAttach // the attack of that contact (ObjAttach_DamagePlayer); Kut-Ku State3_0B tests its kind 0x11
0x7BC u8 * routeResume // the script position after the route command (EmScript_Op3F); EmScript_OpFF resumes there at the end of the route and clears it
0x7C0 u8 routePointScript // the current route point's script table entry (point +0x12; EmScript_RoutePointScript picks the commands to run there: EmScript_Op3F, EmScript_OpFF)
0x7C1 u8 routeNo // route index (EmScript_Op3F; goalArg of the route goal, Em_ComputeGoal copies curGoalArg back); EmScript_OpFF walks it
0x7C2 u8 routePoint // point index in the route (EmScript_Op3F, EmScript_OpFF steps it down to 0; goalNo of the route goal)
0x894 void ** tables // the per-kind area tables (D_8BB280[type]): [0] points, [6] route sets; func_00707D70 finds the entry of an area, func_00707E40 its points
0x010 u8 isEm // 1 for a monster (EmSlot_Alloc; as Player.isEm, 0 for a hunter)
0x3F9 u8 hitIds[16] // the ids of the attacks that hit it last (a ring, hitIdNext the next slot; ObjAttach skips an attack whose id is in it)
0x412 u8 hitIdNext // the next slot of hitIds, 0..15 (cleared by EmSlot_Alloc)
0x414 u8 unk414
0x416 u8 unk416
0x417 u8 unk417
0x4E8 s16 modelCount // model slots used (Model_Alloc; EmSlot_SetupModel, EmSlot_Free)
0x4EA s16 modelFirst // the first model slot
0x6A0 f32 mapPos[3] // as Player.mapPos: the position for the map (Map draws the icons from it)
0x012 u8 weaponOut // as Player.weaponOut (same offset): only player code typed Em uses it (the weapon states set 1, Player_State* 0)
0x018 u8 hitKind // as Player.hitKind (same offset): only player code typed Em uses it (the weapons' CheckHit functions)
0x40A u8 hitStopAdd // as Player.hitStopAdd (same offset): only player code typed Em uses it (Lance, Gunlance, Bow CheckHit)
0x40B u8 hitStop // as Player.hitStop (same offset): only player code typed Em uses it
0x533 u8 carry // as Player.carry (same offset): only player code typed Em uses it
0x534 u8 ammoType // as Player.ammoType (same offset): only player code typed Em uses it (func_0070DBB0)
0x535 u8 savedAmmoType // as Player.savedAmmoType (same offset): only player code typed Em uses it
0x536 u8 ammoMenuOpen // as Player.ammoMenuOpen (same offset): only player code typed Em uses it
0x588 u8 rideState // as Player.rideState (same offset): Em_GetFloorInfo tests it; the player states typed Em set it
0x596 u8 pushPriority // as Player.pushPriority (same offset): compared between two bodies (PlayerEm_PushApart, EmEm_PushApart); Player_UpdateNearMonster reads the monster's
0x638 void * sightTable // the kind's sight and sense ranges (an EmSightRange: Em_SeePlayers; f32 [6] the action reach of Em_NearestActionToBone, [7] the sense reach of Em_SensePlayers)
0x660 u8 floorKind // the floor triangle under the monster (TriHdr, unk_328CD0.c): Em_GetFloorInfo and Em_GetFloorInfoEx take 0x660..0x663 as their info: kind (TriHdr.kind)
0x661 u8 floorArg // byte 1 of that floor triangle (TriHdr.unk01; Player +0x661 goes with the voice SEs)
0x682 u8 areaMoveState // as Player.areaMoveState (same offset): only player code typed Em uses it
0x6B0 u8 aimMode // as Player.aimMode (same offset): only player code typed Em uses it
0x6B1 u8 unk_InitFlag // every family Init (or Reset) sets 1 (Cephadrome and Yama Tsukami also 0); no reader found in C
0x734 u8 floorUnderSurface // 1 when Em_GetFloorInfoEx found the floor below the surface depth (on: low && floor < depth), else 0; Plesioth (EmPlesioth_OverDeepWater) tests it
0x746 s16 mashLeft // as Player.mashLeft (same offset): only player code typed Em uses it
0x75C u8 * scriptCallReturn // the script position after an EmScript_Op81 call (Op81 saves p + 1)
0x760 u8 * scriptJumpReturn // the script position EmScript_Op82 saves before its jump
0x76C f32 nearRadius // Velocidrome and Velociprey: a radius: the player distance (playerDist[curGoalNo]) is compared with it, and EmList_FindKindNear looks for its kind within it
0x772 u8 inBattle // as Player.inBattle (same offset): Player_EnteredBattle compares it with prevInBattle
0x773 u8 prevInBattle // as Player.prevInBattle (same offset)
0x775 u8 scriptCallNo // the subscript EmScript_Op81 calls (script[1][scriptCallNo]); EmScript_Init clears it
0x77E u8 scriptCaseCount // EmScript_Op83: the number of cases it skips through
0x77F u8 routedStateArg // third argument of script command 5 (Em_ChangeStateRouted, EmScript_Run); func_00733A30 switches on it
0x784 u8 loopMode // the script loop kind (EmScript_Op0F byte 0: 0, 1, 2 loop over the area points; Op10 and OpFF switch on it)
0x785 u8 loopPointTable // the point list of the loop (EmScript_Op0F; EmScript_LoopPointList, read by Op10 and OpFF)
0x786 u8 loopScript // the script table entry run at each loop point (EmScript_Op0F; EmScript_LoopScript)
0x787 u8 loopEndScript // the script table entry run when the loop ends (EmScript_Op0F; EmScript_OpFF, EmScript_LoopScript)
0x78C u8 scriptCombat // combat when the EmScript_Op0B block started (Op0B, OpFF after Em_SetCombat)
0x78E u8 scriptVar // a script value: EmScript_Op0C sets it, Op0D clears it, Op2B compares it
0x798 u8 scriptPlayer // the player the script picked (s8, -1: none; low nibble a gPlayers index): EmScript_Op45/Op52/Op53/Op5F pick it, Op46 and Op49 use it, Em_ComputeGoal makes it the goal in combat
0x799 u8 subScriptNo // the subscript EmScript_Call runs (EmScript_SubScript)
0x79A u8 areaSeqTable // the area list of the area sequence (EmScript_Op17 byte 0; Op18 reads table 14 at areaSeqPos)
0x79B u8 areaSeqScript // the script table entry of the area sequence (EmScript_Op17; Op18, EmScript_AreaSeqScript)
0x79C u8 * areaSeqNext // the script position EmScript_Op18 saves
0x7A0 u8 areaSeqMode // EmScript_Op17 byte 1: Op18 and OpFF test it for 0
0x7C8 s16 scriptOffset // EmScript_Run: the start of the script minus scriptPos when it stops
0x7D1 u8 hitPartMask // parts whose hit effects this hit spawned, one bit each (Em_UpdateDamage clears it, sets bits, passes it to Em_SpawnPartEffects)
0x7D6 u8 turnStep // step of Em_TurnToTargetMotion (Em_SetTargetAngle clears it)
0x7D8 s8 syncPulse[2] // set to 2 by EmScript_Op11 ([0]) and Op12 ([1]), or by the sync message bits 4 and 8 (QSync_OnEm); GmEm_Update counts them down; QSync_SendEm sends them while nonzero. Player code reads Player.chargeLevel/fishTimer here
0x812 u8 routeMissCount // frames the route point was not reached (EmScript_OpFF: at 10 it gives up the point); EmScript_Op3F clears it
0x818 f32 playerDist[4] // per player, the distance from the monster (Em_SensePlayers, EmSenseView.dist); Velocidrome and Velociprey compare it with nearRadius
0x828 f32 playerDistXZ[4] // per player, the same in the ground plane (EmSenseView.distXZ); Rathalos and Garuga read the target's
0x868 u8 sensedPlayers // players within the sense reach, one bit each (Em_SensePlayers sets and clears them; Em_DecayNotice, Em_CheckPartBuildup, EmScript_Op01 test them)
0x86A u8 alertLimitRow // the row of gEmPartBuildupLimits[type] the alertBuildup limit comes from (func_00703D60, Em_CheckPartBuildup)
0x87E s8 loopIndex // the script loop's pass count (EmScript_Op0F clears it, OpFF steps it and compares it with loopCount)
0x87F s8 areaPointCount // the number of points of the area entry (EmScript_Op0F e[1]); EmScript_Op10 and OpFF bound loopPoint by it
0x89C u8 fxFlags // bit 1: set once a state's motion effect ran (Player_PassedFrameScaled ... |= 1) and by Em_ResetToBasePose; GmEm_Fx3B (mode 7) and StageObj_K09_Run test it; the ChangeState functions keep only bit 1; Hermitaur uses bits 2, 4, Shen Gaoren 8
0x899 s8 demonMode // as Player.demonMode (same offset): only player code typed Em uses it (DualSwords_ComboInput)
0x8A4 u8 actionGoal[3] // the action spot func_00702830 found: kind, goalArg, goalNo, 0xFF: none (Em_StartGoalAction, EmScript_Op2D copy [1], [2] to the goal)
0x95E u8 scriptAbort // nonzero: EmScript_Run stops and restarts the script (Em_RestartScript); EmScript_SetVar95E sets it (2 after 1000 commands in a frame)
0x95F u8 routeResult // EmScript_Op3F: 0 found, 2 no route point near, 7 a route without points
0x974 u8 routeHeight // EmScript_Op3F byte 2: a height in tenths (h = 10.0f * it)
# Round 25 (types h1): pads split from their indexed uses and the file-local views of Em
0x060 f32 worldMtx[4][4] // the world matrix: scale, rotation, position (Em_UpdateWorldMatrix, Em_Draw: Mtx_Copy into it); Vec_Rotate turns by it (EmKind0A_State0_02, Player_StateLedgeHang/15/1E/44); the effects typed Em keep a counter in its byte 1 (Eff00_Update, Eff06_Fall)
0x2B8 u8 unk2B8 // as Player.unk2B8 (same offset): Player_UpdateAutoTurn clears it
0x2BC u8 motionStarted[4] // as Player.motionStarted (same offset): the lobby townspeople (Npc_UpdateMotion) begin the layer motion (Player_BeginLayerMotion) while it is 0, then set it
0x310 u8 unk310 // as Player.unk310 (same offset): Player_RunWeaponState clears it
0x395 u8 hitReceived // nonzero when the monster took a hit: Em_UpdateDamage handles the hit (or reports the death when 0 and hp <= 0), Player_ReactToHit reacts to it
0x3EF u8 hitAttrs // as Player.hitAttrs (same offset): the attack attribute bits ORed in by ObjAttach_DamageEm; bit 0x80 makes Player_ReactToHit halve the damage
0x436 u8 unk436 // Quest_SpawnMonsters sets 1 for its sixth spawn (j == 5), 0 for the other model-slot-3 ones
0x564 u8 collideFlags // Em_CollideField clears bit 0 first
0x590 u16 turnTarget // the angle Em_TurnToward turns to (EmTurnView of unk_701810.c)
0x592 u16 turnHeading // the current heading of Em_TurnToward (EmTurnView)
0x63C u32 bendAngle[4] // the four bend angles Em_BendStep turns toward an angle (|angle| / 2, / 4, / 8, / 16, each capped by the kind's gEmBendOrigins row; EmBendView); GmEm_ResetState clears them
0x64C u16 turnSpeed // Em_TurnToward's speed (EmTurnView)
0x64E u16 turnHold // EmTurnView: hold frames of Em_TurnToward
0x650 u8 turnPhase // EmTurnView: the phase of Em_TurnToward
0x652 u8 motionRemap // nonzero: Em_SetMotion takes the row's remap row of the alternate table once, then clears it
0x668 u8 unk668 // as Player.unk668 (same offset): Player_StateCrouchWalk/3D test it with the dodge button
0x69C u16 unk69C // as Player.unk69C (same offset): Player_UpdateAutoTurn's turn angle (moved by 0x300 a frame, kept in 0xA00..0xF600)
0x6B2 s16 hitPartDamage[9] // the hit's damage per part (EmDamageView of unk_70A290.c): Em_PickDamagedPart and Em_UpdateDamage scale it by damageFactor into parts[i].hp; [8] takes the damage of the part partFlags names
0x6C4 s16 reactValue[4] // one per player: Em_UpdateReactions runs the reaction of a player whose value changed (EmReactView)
0x6CC s16 reactValueLast[4] // the last seen copies of reactValue (EmReactView)
0x6FA s16 status5Hit // the hit's status 5 amount: Em_PickDamagedPart passes it to Em_AddStatus5 (EmDamageView)
0x700 s16 status2Hit // the hit's status 2 amount (Em_AddStatus2; EmDamageView)
0x708 s16 poisonHit // the hit's poison amount (Em_AddPoison; EmDamageView)
0x712 s16 status4Hit // the hit's status 4 amount (Em_AddStatus4; EmDamageView)
0x71A s16 status1Hit // the hit's status 1 amount (Em_AddStatus1; EmDamageView)
0x738 u8 pushed // set on the monsters and players the push-apart moved (Field_PushApart and the pair pushes of unk_3AC9C0.c); Field_UpdateAll collides those with the field again, then clears it
0x73C u8 poisonCount // times poisoned: Em_UpdateStatus raises poisonThreshold each time up to 4 (as status2Count, status4Count)
0x758 u8 * mainScriptPos // the main script's position (Em_StartScript's result; EmScript_GetPos/SetPos with no scriptFlags bit 0x01, 0x10, 0x20)
0x764 f32 reactDist[2] // the reaction set's distance limits 0 and 1 (Em_LoadReactionSet, script command 0x83); limit 2 is nearRadius, limit 3 at +0x770 lies under the Player fields inBattle, prevInBattle
0x780 u8 * loopScriptPos // the position of the loop script EmScript_Op0F starts (scriptFlags 0x01; EmScript_GetPos/SetPos)
0x788 u8 * trigger10Pos // the script of trigger 0x10 (EmScript_PickTrigger; scriptFlags 0x10)
0x790 u8 * trigger20Pos // the script of trigger 0x20 (EmScript_PickTrigger; scriptFlags 0x20)
0x794 u8 * subScriptReturn // the position after an EmScript_Call (it saves p + 1; EmScript_OpFF command 3 returns there)
0x7A3 s8 repeatCount // the repeat count of EmScript_Op24; EmScript_Op25 clears it
0x7A8 s32 unk7A8 // only cleared: by EmScript_Init and EmScript_PickTrigger
0x7AC u8 * repeatPos // the command a repeat returns to (EmScript_Op24)
0x7B0 u8 * trigger02Pos // the script of trigger 2 (EmScript_PickTrigger; scriptFlags 2)
0x7B4 u8 * trigger04Pos // the script of trigger 4 (EmScript_PickTrigger; scriptFlags 4)
0x7B8 u8 * trigger08Pos // the script of trigger 8 (EmScript_PickTrigger; scriptFlags 8)
0x838 f32 reactRange[3] // the reaction set's ranges (Em_LoadReactionSet; Em_GetReaction kinds 3 to 6, EmSenseView); the fourth at +0x844 shares bytes with the Player field aimPitch
0x8AB u8 part8Down // Em_UpdateDamage (reaction 9): part 8's hp ran out and the kind's handler took it: reaction 0xB; GmEm_ResetState clears it
0x95C u8 spawnQueued // QSync_QueueEmState sets it when the local machine queues the monster's sync state into a spawn
0x971 u8 hostChanged // set when func_00707060 hands the monster to another player (hostNo, remote, QSync_SendEm kind 4); GmEm_ResetState clears it
0x972 u8 reactionSet // the reaction set Em_LoadReactionSet loaded
0x996 u8 targetFound // Em_TargetPlayerInRange: 1 when a player was in range (curGoalKind 1), 0 otherwise
0x3EB u8 unk3EB // lobby Npc4F_State58 sets 1 while the player is in gNpc4FArea rising or above the NPC, Npc4F_State5D tests it
# Round 31 session 31 (types): the hit fields (sub_main unk_3BE060.c)
0x3F2 s16 hitForce // the attack's hitForce at the last hit (ObjAttach_DamageEm; as Player.hitForce)
0x4BC struct ObjAttach * hitAttach // the attack object of the last hit taken (ObjAttach_DamageEm; as Player.hitAttach)
0x748 s16 mashBuildupHit // as Player.mashBuildupHit: the hit amount Em_ApplyHitStatus adds (flag 0x100); cleared by Hit_ClearStatusAll
