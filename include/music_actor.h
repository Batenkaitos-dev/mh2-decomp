#ifndef MUSIC_ACTOR_H
#define MUSIC_ACTOR_H

#include "common.h"
#include "sprite.h"
#include "em.h"

/*
 * The music actor, the townsperson the town's music show moves:
 * MusicActor_StartScript starts an entry's motion script (a list of
 * MusicActorKey), MusicActor_Update runs it a frame at a time and
 * MusicActor_Draw draws the actor. Also the record book's page texts (the
 * horn scores, the guide's entries and categories), which the record book's
 * draws read.
 */

/* a key of the music actor's motion script (12 bytes): at its frame of the
 * script the actor starts motion, with blend frames and a start frame
 * (Actor_SetMotion), and, when speed is positive, both motion layers'
 * steps become twice speed. A frame of -1 ends the list. */
typedef struct MusicActorKey {
    /* 0x0 */ s16 frame;            /* -1 ends the script */
    /* 0x2 */ s16 motion;
    /* 0x4 */ s16 startFrame;       /* the motion's start frame */
    /* 0x6 */ s16 blend;            /* the motion's blend frames */
    /* 0x8 */ f32 speed;
} MusicActorKey;

/* the motion script of the actor MusicActor_Update updates (gMusicActorScript):
 * MusicActor_GetScript(track, entry) finds it in gMusicActorScripts, MusicActor_RunScript starts the
 * entry of the current frame */
typedef struct MusicActorScript {
    /* 0x0 */ s16 track;            /* gLobbyWorld.track */
    /* 0x2 */ s16 entry;            /* the track's entry playing */
    /* 0x4 */ s32 frame;
    /* 0x8 */ MusicActorKey *keys;
} MusicActorScript;

void MusicActor_StartScript(Em *actor, s16 track, s16 entry);
void MusicActor_Update(Em *actor);
s32 MusicActor_RunScript(Em *actor, s32 frame);
MusicActorKey *MusicActor_GetScript(s32 track, s32 entry);
void MusicActor_SetMatrix(Em *actor);
void MusicActor_Draw(DrawSlot *slot);
extern char gHornScorePage0[272];
extern char gHornScorePage1[208];
extern char gHornScorePage2[384];
extern char gHornScorePage3[272];
extern char gHornScorePage4[464];
extern char gOldHornScorePage0[208];
extern char gOldHornScorePage1[240];
extern char gOldHornScorePage2[368];
extern char gOldHornScorePage3[544];
extern char gRecordGuideEntryName0_0[32];
extern char gRecordGuideEntryName0_1[32];
extern char gRecordGuideEntryName0_2[32];
extern char gRecordGuideEntryName0_3[32];
extern char gRecordGuideEntryName0_4[32];
extern char gRecordGuideEntryName0_5[32];
extern char gRecordGuideEntryName0_6[32];
extern char gRecordGuideEntryName0_7[32];
extern char gRecordGuideEntryName0_8[32];
extern char gRecordGuideEntryName0_9[32];
extern char gRecordGuideEntryName0_10[32];
extern char gRecordGuideEntryName0_11[32];
extern char gRecordGuideEntryName0_12[32];
extern char gRecordGuideEntryName0_13[32];
extern char gRecordGuideEntryName0_14[32];
extern char gRecordGuideEntryName1_0[32];
extern char gRecordGuideEntryName1_1[32];
extern char gRecordGuideEntryName1_2[32];
extern char gRecordGuideEntryName1_3[32];
extern char gRecordGuideEntryName1_4[32];
extern char gRecordGuideEntryName1_5[32];
extern char gRecordGuideEntryName1_6[32];
extern char gRecordGuideEntryName1_7[32];
extern char gRecordGuideEntryName1_8[32];
extern char gRecordGuideEntryName1_9[32];
extern char gRecordGuideEntryName2_0[32];
extern char gRecordGuideEntryName2_1[32];
extern char gRecordGuideEntryName2_2[32];
extern char gRecordGuideEntryName2_3[32];
extern char gRecordGuideEntryName2_4[32];
extern char gRecordGuideEntryName2_5[32];
extern char gRecordGuideEntryName2_6[32];
extern char gRecordGuideEntryName2_7[32];
extern char gRecordGuideEntryName2_8[32];
extern char gRecordGuideEntryName2_9[32];
extern char gRecordGuideEntryName3_0[32];
extern char gRecordGuideEntryName3_1[32];
extern char gRecordGuideEntryName3_2[32];
extern char gRecordGuideEntryName3_4[32];
extern char gRecordGuideEntryName3_6[32];
extern char gRecordGuideEntryName3_8[32];
extern char gRecordGuideEntryName3_10[32];
extern char gRecordGuideEntryName3_12[32];
extern char gRecordGuideEntryName3_14[32];
extern char gRecordGuideEntryName3_16[32];
extern char gRecordGuideEntryName3_18[32];
extern char gRecordGuideEntryName3_20[32];
extern char gRecordGuideEntryName3_22[32];
extern char gRecordGuideCatName0[64];
extern char gRecordGuideCatName1[64];
extern char gRecordGuideCatName2[64];
extern char gRecordGuideCatName3[48];

#endif
