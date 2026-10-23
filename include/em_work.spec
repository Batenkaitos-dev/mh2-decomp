# Fields of EmWork (include/em.h), the work area passed to the state functions.
# Regenerate with: python tools/gen_struct.py include/em_work.spec include/em.h
name EmWork
size 0x100
0x006 s16 fxMotion // the motion id the MotionEffects and MotionSounds functions last saw (motionId[0] stored and compared; switched on); the flying families (Rathalos, Kutku, Garuga, Gravios, Diablos) keep frames since landing here instead (150 set by the landing states, counted down by the update)
0x030 s32 moveAngX // angles X, Y, Z for Em_MoveRotated, Em_MoveRotatedAccel, Em_GetPitchToGoal/Target (&moveAngX: Mtx_FromAnglesYXZ); set from rotX
0x034 s32 moveAngY // set from rotY or Math_AngleXZ to the goal
0x038 s32 moveAngZ // 0
0x03C s32 pitchStep // added to or taken from rotX each frame
0x040 s32 turnSpeed // Em_TurnToGoal speed (256)
0x044 s32 rollSpeed // flying families: rotZ is stepped back to 0 by it (256 at init); Velocidrome and Velociprey State3: an f32 velZ kept while a wall stops the jump
0x050 s16 unk050
0x000 u8 fxStep // UpdateMotionEffects: 0, then 1 (runs MotionEffects): Kirin, Anteka, kind 29 (the families with a view keep their own, at +0 or elsewhere)
0x002 s16 sndMotion // the motion id the MotionEffects functions keep (motionId[0], stored when it changes) and the MotionSounds copies (Kut-Ku, Garuga, Rathalos, Gravios, Diablos, Khezu, Chameleos, Fatalis) switch on; Rathalos and Fatalis write it through their views (EmFlyerWork.sndMotion, EmFatalisWork.fxMotion); Yama Tsukami keeps a timer here (EmYamaTsukamiWork)
0x004 s16 prevMotion // a motion id kept to see it change (motionId[0] stored and compared; switched on)
0x008 s32 accelAngX // angles X, Y, Z for Em_MoveRotatedAccel (&accelAngX; set from rotX, rotY and 0)
0x00C s32 accelAngY // set from rotY, or rotY plus or minus a quarter turn, plus half a turn
0x010 s32 accelAngZ // 0; some states keep an f32 distance here
0x014 u16 goalYaw // Math_AngleXZ to the goal; copied to rotY
0x017 u8 unk017
0x018 u8 flyGuard // flying families: 1 in states of group 2 (flight; Diablos underground): the update then keeps guardTimer at 2 (read as s8 w[0x18]); Velocidrome and Velociprey keep their curve frame here (EmDromeWork, EmVelocipreyWork)
0x019 u8 unk019
0x01A u8 countdown // s8 counts down to 0 (--*(s8 *)&countdown <= 0)
0x01B u8 unk01B
0x01C s32 moveAngBX // angles X, Y, Z for Em_MoveRotated and Em_GetPitchToGoal (the second set; see moveAngX)
0x020 s32 moveAngBY // set from rotY
0x024 s32 moveAngBZ // 0; some states keep a position (Vec_Set) from here
0x028 s32 moveAngCX // Chameleos: angles X, Y, Z for Em_MoveRotated and Em_GetPitchToGoal (the third set: X, Y here, Z at moveAngX); Rathalos keeps an f32 target height here
0x02C s32 moveAngCY // Chameleos: set from rotY or Math_AngleXZ to the goal
0x048 s32 unk048
0x04C s32 unk04C
0x054 f32 startX // Velocidrome State1_01: the position the walk started from (X, Z) and the frames walked
0x058 f32 startZ
0x05C s16 walkFrames // compared with 60 when the goal is reached near the start
0x064 f32 startXB // Velociprey State1_01: as startX, startZ, walkFrames
0x068 f32 startZB
0x06C s16 walkFramesB
0x06E u8 huntedB // Velociprey: as hunted
0x052 s16 wallTimer // Rathalos swoops: frames (30) after a wall hit in which the pitch is not aimed at the target
0x05E u8 hunted // Velocidrome: 1 once a hunted state (group 5) ran; cleared at init
