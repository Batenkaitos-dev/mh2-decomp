#ifndef SCEVENT_H
#define SCEVENT_H

#include "common.h"

struct ScEvent;

/* A scene event's callback: check (may the event start: > 0 starts it, < 0 drops it) or run
 * (one step of the event: nonzero when it is over). */
typedef s32 (*ScEventFn)(struct ScEvent *e);

/* The head of a scene event's record: the game overlay's records (ScEventRec: ScEvent_Cam5, the
 * ScEvent_Seq* and ScEvent_DemoCam* tables of game unk_70A290.c, listed in gGalleryScEvents)
 * start with it. ScEvent_Set and ScEvent_StartFrom read the camera and the two callbacks and keep
 * a pointer to the record as the running event's data (ScEvent.data); ScEvent_SetupB sets the
 * callbacks to the B pair. plsel (unk_D74450.c) copies a record's head into a local (its GalView). */
typedef struct ScEventHead {
    u16 area;                   /* 0x00: the area the event plays in (ScEvent_CheckDay: by Area_DayId) */
    u16 doneFlag;               /* 0x02: an event flag: when it is set, ScEvent_Set does not start the event (0: none); ScEvent_RunA sets it at the end; ScEvent_CurId returns it */
    u8 camera;                  /* 0x04: the Dmcam id (Dmcam_Load); ScEvent_SetCamera's first argument */
    u8 camGroup;                /* 0x05: ScEvent_SetCamera's second argument: the camera follows the first monster of this group (MonsterGroup_First; 0: none) */
    u16 jingleDay;              /* 0x06: the jingle played as the event starts in the day (Jingle_Play; 0: none) */
    u16 jingleNight;            /* 0x08: the jingle played at night */
    u8 music;                   /* 0x0A: the run callbacks switch on it: the stream pause, the wait before Bgm_Play and the music after the event */
    u8 unk0B;
    u8 flags;                   /* 0x0C: bit 0: the monsters hold still while the event runs (ScEvent_CurFlag1, GmEm_CanUpdate) */
    u8 unk0D[3];
    ScEventFn check;            /* 0x10: may the event start (ScEvent_CheckAlwaysA/B, ScEvent_CheckDay) */
    ScEventFn run;              /* 0x14: its steps (ScEvent_RunA, ScEvent_RunB, ScEvent_RunVersusEnd) */
} ScEventHead; /* 0x18 */

typedef char ScEventHead_size_check[(sizeof(ScEventHead) == 0x18) ? 1 : -1];

/* A scene event's record in the game overlay's data (0x20 bytes). */
typedef struct ScEventRec {
    ScEventHead head;
    s32 unk18[2];               /* 0 in every record; no code reads them */
} ScEventRec; /* 0x20 */

typedef char ScEventRec_size_check[(sizeof(ScEventRec) == 0x20) ? 1 : -1];

/* One of the three scene events of the area (gSceneEvents.ev). The callbacks clear its first
 * word (on, step and timer) with one store when the event is over. */
typedef struct ScEvent {
    u8 on;                      /* 0x00: set by ScEvent_Set and ScEvent_StartFrom */
    u8 step;                    /* 0x01: the run callback's step */
    s16 timer;                  /* 0x02: the run callbacks' wait before Bgm_Play (by the record's music) */
    ScEventFn check;            /* 0x04: the record's check */
    ScEventFn run;              /* 0x08: the record's run */
    ScEventHead *data;          /* 0x0C: the record */
} ScEvent; /* 0x10 */

typedef char ScEvent_size_check[(sizeof(ScEvent) == 0x10) ? 1 : -1];

/* gSceneEvents (game): the area's scene events. */
typedef struct ScEventList {
    u8 active;                  /* 0x00: an event runs (ScEvent_Update) */
    u8 cur;                     /* 0x01: the event that runs */
    u8 ended;                   /* 0x02: an event ended this frame: the run callbacks set it at their end, ScEvent_Update clears it (Player_InSceneEvent) */
    u8 unk03;
    ScEvent ev[3];              /* 0x04 */
} ScEventList; /* 0x34 */

typedef char ScEventList_size_check[(sizeof(ScEventList) == 0x34) ? 1 : -1];

extern ScEventList gSceneEvents;

#endif
