# Fields of ObjAttach (include/objattach.h), an object of sub_main's second object pool (0x40 of 0xD4
# bytes at 0x5B70D0, ObjAttach_Alloc/Free in sub_main bgm.c), attached to a
# player or a monster: effects and the like. Regenerate with:
#   python tools/gen_struct.py include/objattach.spec include/objattach.h
name ObjAttach
size 0xD4
0x000 u8 active // nonzero while the object is in use
0x001 u8 visible
0x002 u8 kind // the family, enum ObjAttachKind, set by the creator (0x21: EmEffectAttack_Init)
0x003 u8 type // the variant within the kind (the creator's argument; the updates switch on it)
0x004 u8 state // the object's state, enum ObjAttachState (each family's Update switches on it)
0x005 u8 step // as Em.step: the step within the update (switch on it, counted up; 0 at set up)
0x006 u8 stateVar0 // as Em.stateVar0: per-kind scratch byte (switch on it, counters, 0xFF none)
0x007 u8 stateVar1 // as Em.stateVar1: a second per-kind scratch byte (0xFF/0xFE marks, counters)
0x009 u8 keep // not freed by ObjAttach_FreeUnkept
0x00A u8 idx // the index of the player or monster it follows (gEms, gPlayers)
0x00B u8 hitPhase // the hit window: 0 off, 1 waiting hitDelay frames, 2 open for hitFrames (ObjAttach_UpdateTimers; ObjAttach_LoadOwnedRow restarts it); cleared when the owner motion changes (LaoShanAttack_Run, RathalosAttack_Run, FatalisAttack_Run end the object at 0)
0x00C struct ObjAttach * prev
0x010 struct ObjAttach * next
0x014 ObjAttachFn{4} draw // drawn every frame while it, its owner and the object are visible (ObjAttach_DrawAll); NULL: none
0x018 u8 * buf // its blocks of the object buffer
0x01C u8 blk
0x01D u8 nblk
0x020 ObjAttachFn{4} update // run every frame once delay is 0 (ObjAttach_UpdateAll)
0x024 s32 rotX // its rotation (angles; the creators copy the owner's, Em +0xA0/+0xA4/+0xA8)
0x028 s32 rotY
0x030 f32 pos[3] // the position
0x07A u8 isEm // 0: follows a player, else a monster
0x07B u8 delay // frames before update runs
0x07C s16 ownerMotion // the owner's motionId[0] when the object was made (LaoShanAttack_Run's family and the others that end with it); the hit window closes when the owner's motion differs; some kinds count frames in it (EmRangedAttack: the creator's bone, moved to its buffer at set up)
0x07E s16 aux // per-kind value: a bone of the owner (Em_GetBone, f06_BoneLaunchPos), a monster kind (EmSlot_FindKind), a frame counter
0x094 void * owner // the player or monster it belongs to (EmEffectAttack_Init)
0x0BC struct DrawSlot * drawSlotBuf // the draw slot of drawSlot (DrawSlot_Get/DrawSlotB_Get(drawSlot)), NULL: none; drawn at its pos (EmImpactAttack_Run)
0x0CC u16 yaw // its yaw (the owner's rotation Y, Em/Player +0xA4, when made; +0x8000 turns it round)
0x0D1 u8 side // the owner side (Em/Player side, PlayerAttack_Create, Explosion_Create; copied to spawned objects): ObjAttach_DamagePlayer Side_Match(t->side, it)
0x061 u8 hitFrames // frames the hit window stays open (99: long; FatalisAttack 0x3C): ObjAttach_UpdateTimers counts it in hitPhase 2
0x062 u8 power // the attack power: ObjAttach_DamageEm (dmgf = it), ObjAttach_DamagePlayer (it * atk); BowgunShot_CalcPower, BowArrow_ApplyPower set it
0x06A u8 hitListNo // the index into the kind tables hitList is taken from (hitList = BlangongaAttack_HitShapes[it] and the like, Setup functions)
0x088 struct HitShape * hitList // the attack hit shapes (HitShape, include/capsule.h), a list ended by bone -1, read with pos/pos2 (ObjAttach_HitObjects, ObjAttach_HitTarget: no hit while NULL); PlacedItem_Shapes[type] and the kind tables
0x08C struct HitShape * hurtList // the shapes other objects hit (HitShape, include/capsule.h) (ObjAttach_HitObjects: elist = e->hurtList, skipped while NULL)
0x0C0 s16 drawSlot // the draw slot, -1: none (DrawSlot_Alloc/DrawSlotB_Alloc in the Setup functions)
0x0CE u16 area // the area it is in (the owner's +0x680); compared with the current area, gScene.unk014
0x02C s32 rotZ
0x060 u8 hitDelay // frames before the hit window opens: the Setup functions compute it from the owner motion frames (PlayerAttack_Setup: k - motionFrame / 2 - 1); ObjAttach_UpdateTimers counts it down in hitPhase 1
0x0C8 f32 hitScale // the hit capsule scale: Capsule_FromPoints(pos, pos2, hitList, .., it) (1.0 for the object hit)
0x0B4 u8 wasHit // 1 once another object hit it (ObjAttach_HitObjects sets it and skips it after); PlacedItem_Run tests it
0x048 f32 velX // velocity x: ItemAttack/BowgunShot/BowArrow add it to pos; Vec_RotateY(&velX) and Vec_DirToAngles use velX..velZ as a vector
0x06C u16 elementFlags // the element bits: 0x10 fire, 0x20 water, 0x40 thunder, 0x80 ice, 0x200 dragon take elementPower (ObjAttach_DamageEm); the status bits 1 status 2, 2 poison, 4 status 4, 8 status 1, 0x100 mash buildup add elementPower to the target's hit amounts, 0x400, 0x1000 and 0x800 set status 6, 7 and 8 (Em_ApplyHitStatus); ObjAttach_HitEffects picks the hit effect from them; BowArrow_ApplyCoating sets them
0x06E u8 elementPower // the element value for elementFlags (ObjAttach_DamageEm: fire = it, ...)
0x06F u8 hitFx // low 6 bits: the hit effect kind (HitSplash_CreateByHitFx, HitSpark_Spawn in ObjAttach_HitEffects); 0x80 a critical hit, 0x40 a critical miss (the debug prints of ObjAttach_DamageEm)
0x03C f32 pos2[3] // the second point of the hit capsule: Capsule_FromPoints(pos, pos2, hitList, .., hitScale) (ObjAttach_HitObjects, ObjAttach_HitTarget); PlayerAttack_PlaceOnWeapon sets it from a bone matrix, ItemAttack_Setup from pos
0x0B8 u16 randBits // random bits from the owner stateRand (Em/Player), read by shifts (BowgunShot, BowArrow, TeostraBlastRing)
0x080 s16 unk080
0x0B5 u8 hitId // the attack id: an object is not hit twice by one id (compared with hitIds in ObjAttach_HitAll; stored there by the damage functions)
0x06B u8 hitSe // the hit sound kind: 0xFF none; on a player 1 SE 0x20, others 0x21 (ObjAttach_DamagePlayer); on a monster by kind (ObjAttach_DamageEm)
0x04C f32 velY // velocity y (see velX)
0x050 f32 velZ // velocity z (see velX)
0x058 f32 gravity // added to velY each frame (BowgunShot, EmEffectAttack: EmEffectAttack_Gravities[type], -1.0 / -1.5)
0x008 u8 ownerIndex // the owner's index (ObjHead.index), set with the attack row (ObjAttach_LoadOwnedRow); 0xFF: an attack with no owner (ObjAttach_LoadRow)
0x01E u8 didHit // 1 once it hit a monster or a player (ObjAttach_DamageEm, ObjAttach_DamagePlayer); cleared before each hit pass; PlacedItem and PitfallTrap test it
0x09C struct Em * targetEm // the monster it locked on or stuck to (found in range in attack_families.c), NULL: none
0x0A0 struct HitShape * targetPart // the hit shape of targetEm it hit (ObjAttach_DamageEm): its bone and position
0x063 u8 attackKind // the attack kind, copied to the target lastAttackKind (ObjAttach_DamageEm/DamagePlayer): 7 paints (Em.painted), 8 and 0x10 special cases, 0xA hits only state-3 players (ObjAttach_HitTarget); ObjAttach_HitObjects/HitAll switch on it
0x054 f32 homingTurn // homing: the turn toward the target, grown each frame (attack_families.c); zeroed at creation by the other kinds
0x05C f32 homingSpeed // homing: added to the speed, grown each frame (attack_families.c); zeroed at creation by the other kinds
0x0D2 u8 powerPct // nonzero: power is scaled by this percentage on a hit (ObjAttach_DamageEm, ObjAttach_DamagePlayer); the Explosion family sets 0xFA
0x0D3 u8 hitSpan // LaoShanAttack type 0x47 (Lao-Shan's): twice its hitFrames (LaoShanAttack_CreateSpan's last argument; LaoShanAttack_Setup halves it); 0 from the other LaoShanAttack creators
0x065 u8 attrs // attribute bits ORed into the target hitAttrs (ObjAttach_DamageEm, ObjAttach_DamagePlayer): 0x80 cannot be guarded; 1 tested by ObjAttach_HitAll, 0x10 by ObjAttach_DamagePlayer
0x070 u8 hitStop // the hit stop: the hitter hitStopAdd (times 3 in some cases) and hitSlowTimer, the target hitStopAdd (ObjAttach_DamageEm, ObjAttach_DamagePlayer)
0x071 u8 swordGaugeAdd // long sword: the spirit gauge gained by the hit, times gSwordGaugeHitFactors[hitKind] (LongSword_AddGauge in ObjAttach_DamageEm)
0x073 u8 nextRow // the attack row loaded when the hit window closes, 0: none (ObjAttach_UpdateTimers runs ObjAttach_LoadOwnedRow with it and reopens the window); AttackRow's nextRow
0x075 u8 mvFramesA // frames of the first motion-value phase, counted down while the window is open (0xFF: held); ObjAttach_DamageEm picks motion-value kind 0 at 0xFF, 1 while it runs
0x076 u8 mvFramesB // frames of the second phase, counted down once mvFramesA is 0; ObjAttach_DamageEm picks kind 2 while it runs (3 on a shape of zone 1)
0x074 u8 stun // the stun amount: the target status5Hit (ObjAttach_DamageEm), stun (ObjAttach_DamagePlayer)
# Round 31 session 31 (types): the hit fields (sub_main hit_detection.c)
0x066 s8 hitYawOfs // degrees added to the object's yaw for the target's hitAngle, unless attrs 0x10 (ObjAttach_SetHitHeading)
0x068 u8 hitForce // copied to the target's hitForce (ObjAttach_DamagePlayer, ObjAttach_DamageEm)
0x069 u8 hitMarks // mark bits ORed into the target's partFlags: 1 a critical hit (with flag 0x40), 2, 8; on a player 4 fire, 0x10 water, 0x40 thunder, 0x20 ice, 0x80 dragon pick the resistance (ObjAttach_DamagePlayer)
0x072 u8 multiHit // low 4 bits: the hits left; high 4 bits: the delay before the next (ObjAttach_NextMultiHit reopens the hit window with a new hitId)
0x0D0 u8 hitKind // the hitter's hitKind at the hit (ObjAttach_DamageEm), 0 on a player (ObjAttach_DamagePlayer); ObjAttach_HitEffects picks the effect by it
0x0C4 void * hitBy // set when another object hit it: gPlayers or gEms by the hitter's isEm, at its own idx (the game's indexing), NULL when the hitter's ownerIndex is 0xFF (ObjAttach_HitObjects)
0x078 u16 attackFlags // ATKF_* bits, set by the Setup functions (Player_TestSetFlags) and tested by the hit and damage functions (Player_TestFlags); cleared by ObjAttach_Free
0x090 void * attackRows // the family's attack rows (the address of its RowsPtr: ObjAttach_StartOwnedAttack stores it; the row picked by the creator's type is copied over 0x060-0x077)
