/*
 * The scene events: the camera scenes a quest plays, each one a record
 * (ScEventRec) with a check callback, which says when it starts, and a run
 * callback, which plays it one step a frame. ScEvent_Init sets up the
 * quest's events at its start (the monsters' introductions through
 * ScEvent_LoadDemoCams, the area's camera through ScEvent_LoadCamera);
 * ScEvent_Set starts a record in a slot, and ScEvent_Update runs the slots
 * each frame. The callbacks follow: ScEvent_CheckDay and
 * ScEvent_CheckAlwaysA/B check, ScEvent_RunA, ScEvent_RunVersusEnd and
 * ScEvent_RunB run. ScEvent_SetCamera points the demo camera, and
 * ScEvent_CurId and ScEvent_CurFlag1 tell the rest of the game about the
 * running event. ScEvent_StartFrom and ScEvent_SetupB play one record at
 * once, for the gallery's viewer.
 *
 * The .data is the records and gGalleryScEvents, the gallery's table of
 * them; the .rodata is ScEvent_RunA's two jump tables.
 */

#include "common.h"
#include "syssettings.h"
#include "quest.h"
#include "scevent.h"
#include "em.h"
#include "player.h"
#include "area.h"
#include "bgm.h"
#include "camera.h"
#include "fishing.h"
#include "save.h"
#include "stream.h"
#include "game_util.h"
#include "screen_fade.h"
#include "msg_event.h"

extern ScEventRec ScEvent_VersusEnd;
extern ScEventRec ScEvent_VelocidromeSighting;
extern ScEventRec ScEvent_BulldromeSighting;
extern ScEventRec ScEvent_DaimyoHermitaurSighting;
extern ScEventRec ScEvent_CongalalaSighting;
extern ScEventRec ScEvent_RathianSighting;
extern ScEventRec ScEvent_KushalaJungleSighting;
extern ScEventRec ScEvent_KushalaSnowSighting;
extern ScEventRec ScEvent_LunastraTeostraVolcano8Sighting;
extern ScEventRec ScEvent_LunastraTeostraTowerSighting;
extern ScEventRec ScEvent_GarugaSighting;
extern ScEventRec ScEvent_ChameleosSighting;
extern ScEventRec ScEvent_LunastraTeostraVolcano7Sighting;
extern ScEventRec ScEvent_BlangongaSighting;
extern ScEventRec ScEvent_ShogunCeanataurSighting;
extern ScEventRec ScEvent_KutkuSighting;
extern ScEventRec ScEvent_KhezuSighting;
extern ScEventRec ScEvent_DiablosSighting;
extern ScEventRec ScEvent_GraviosSighting;
extern ScEventRec ScEvent_MonoblosSighting;
extern ScEventRec ScEvent_RathalosSighting;
extern ScEventRec ScEvent_KirinSighting;
extern ScEventRec ScEvent_Seq1;
extern ScEventRec ScEvent_Seq5;
extern ScEventRec ScEvent_Seq3;
extern ScEventRec ScEvent_Seq3B;
extern ScEventRec ScEvent_Seq11;
extern ScEventRec ScEvent_DemoCam7;
extern ScEventRec ScEvent_DemoCam32;
extern ScEventRec ScEvent_DemoCam2;
extern ScEventRec ScEvent_DemoCam24;
extern ScEventRec ScEvent_DemoCam47;
extern ScEventRec ScEvent_DemoCam3A;
extern ScEventRec ScEvent_DemoCam37;
extern ScEventRec ScEvent_CamF;
extern ScEventRec ScEvent_Cam6;
extern ScEventRec ScEvent_CamB;
extern ScEventRec ScEvent_Cam2;
extern ScEventRec ScEvent_Cam3;
extern ScEventRec ScEvent_Cam4;
extern ScEventRec ScEvent_Cam5;
/*
 * Sets up the scene events of the quest being started: empties gSceneEvents
 * and the running flag, then runs ScEvent_LoadDemoCams and stops if that
 * started a monster's introduction event. Otherwise it loads the area camera
 * event (ScEvent_LoadCamera) and, by gScene.questId, starts the quest's own
 * event record in slot 0 with ScEvent_Set (one quest fills slot 1 too, and
 * one also sets gScene.eventRunning at once).
 */
void ScEvent_Init(void)
{
    ScEventList *list = &gSceneEvents;

    gScene.eventRunning = 0;
    gSceneEvents.active = 0;
    *(s32 *) &gSceneEvents.ev[2] = 0;
    *(s32 *) &gSceneEvents.ev[1] = 0;
    *(s32 *) &gSceneEvents.ev[0] = 0;
    gSceneEvents.ended = 0;
    if (ScEvent_LoadDemoCams(list) & 0xFFFF) {
        return;
    }
    Dmcam_Init(gSysSettings.readBuf, 1);
    ScEvent_LoadCamera(list);
    switch (gScene.questId) {
    case 25001:
        ScEvent_Set(&list->ev[0], &ScEvent_VersusEnd.head);
        gScene.eventRunning = 1;
        break;
    case QUEST_ID_VELOCIDROME_SCENE_1:
    case QUEST_ID_VELOCIDROME_SCENE_2:
    case QUEST_ID_VELOCIDROME_SCENE_3:
        ScEvent_Set(&list->ev[0], &ScEvent_VelocidromeSighting.head);
        break;
    case QUEST_ID_BULLDROME_SCENE_1:
    case QUEST_ID_BULLDROME_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_BulldromeSighting.head);
        break;
    case QUEST_ID_DAIMYO_HERMITAUR_SCENE_1:
    case QUEST_ID_DAIMYO_HERMITAUR_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_DaimyoHermitaurSighting.head);
        break;
    case QUEST_ID_CONGALALA_SCENE_1:
    case QUEST_ID_CONGALALA_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_CongalalaSighting.head);
        break;
    case QUEST_ID_RATHIAN_SCENE_1:
    case QUEST_ID_RATHIAN_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_RathianSighting.head);
        break;
    case QUEST_ID_KUSHALA_JUNGLE_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_KushalaJungleSighting.head);
        break;
    case QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_LunastraTeostraTowerSighting.head);
        break;
    case QUEST_ID_KUSHALA_SNOW_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_KushalaSnowSighting.head);
        break;
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_LunastraTeostraVolcano8Sighting.head);
        break;
    case QUEST_ID_GARUGA_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_GarugaSighting.head);
        break;
    case QUEST_ID_CHAMELEOS_SCENE_1:
    case QUEST_ID_CHAMELEOS_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_ChameleosSighting.head);
        break;
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_1:
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_LunastraTeostraVolcano7Sighting.head);
        break;
    case QUEST_ID_BLANGONGA_SCENE_1:
    case QUEST_ID_BLANGONGA_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_BlangongaSighting.head);
        break;
    case QUEST_ID_SHOGUN_CEANATAUR_SCENE_1:
    case QUEST_ID_SHOGUN_CEANATAUR_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_ShogunCeanataurSighting.head);
        break;
    case QUEST_ID_KUTKU_SCENE_1:
    case QUEST_ID_KUTKU_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_KutkuSighting.head);
        break;
    case QUEST_ID_KHEZU_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_KhezuSighting.head);
        break;
    case QUEST_ID_DIABLOS_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_DiablosSighting.head);
        break;
    case QUEST_ID_GRAVIOS_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_GraviosSighting.head);
        break;
    case QUEST_ID_MONOBLOS_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_MonoblosSighting.head);
        break;
    case QUEST_ID_RATHALOS_SCENE_1:
    case QUEST_ID_RATHALOS_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_RathalosSighting.head);
        break;
    case QUEST_ID_KIRIN_SCENE_1:
    case QUEST_ID_KIRIN_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_KirinSighting.head);
        break;
    case 1:
        ScEvent_Set(&list->ev[0], &ScEvent_Seq1.head);
        break;
    case 3:
        ScEvent_Set(&list->ev[0], &ScEvent_Seq3.head);
        ScEvent_Set(&list->ev[1], &ScEvent_Seq3B.head);
        break;
    case 5:
        ScEvent_Set(&list->ev[0], &ScEvent_Seq5.head);
        break;
    case 17:
        ScEvent_Set(&list->ev[0], &ScEvent_Seq11.head);
        break;
    }
}

/*
 * Starts the event record rec in slot: returns -1 and does nothing when the
 * record's doneFlag is nonzero and already set in the save
 * (Save_TestEventFlag). Otherwise loads the record's camera (Dmcam_Load),
 * switches the slot on with the record's check and run callbacks and a
 * pointer to the record, and returns 0.
 */
s32 ScEvent_Set(ScEvent *slot, ScEventHead *rec)
{
    u16 n;

    n = rec->doneFlag;
    if ((n != 0) && (Save_TestEventFlag((s32) n) == 1)) {
        return -1;
    }
    Dmcam_Load((s32) rec->camera, 0);
    slot->on = 1;
    slot->check = rec->check;
    slot->run = rec->run;
    slot->data = rec;
    return 0;
}

/*
 * Prepares the monsters' introduction events of the stage: clears seven save
 * event flags and the demo cameras, then walks the stage's monster kinds
 * (Quest_GetStageEmKinds). The first of EM_KIND_LAO_SHAN_LUNG,
 * EM_KIND_ASHEN_LAO_SHAN_LUNG, EM_KIND_FATALIS, EM_KIND_FATALIS_B,
 * EM_KIND_FATALIS_C, EM_KIND_SHEN_GAOREN or EM_KIND_YAMA_TSUKAMI puts its
 * record (ScEvent_DemoCam*) in slot 0 of list, loads that kind's demo cameras
 * and is returned. The Kushala kinds and Chameleos, and Lunastra and Teostra,
 * only load a camera and the walk goes on. Returns 0 when no introduction was
 * started.
 */
s32 ScEvent_LoadDemoCams(ScEventList *list)
{
    u8 ids[4];
    s32 i;
    s32 n;

    Save_ClearEventFlag(EVENT_FLAG_LAO_SHAN_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_ASHEN_LAO_SHAN_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_FATALIS_CASTLE_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_FATALIS_BATTLEFIELD_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_FATALIS_TOWER_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_SHEN_GAOREN_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_YAMA_TSUKAMI_TOWER_SCENE);
    Dmcam_Init(gSysSettings.readBuf, 0);
    n = Quest_GetStageEmKinds(ids);
    for (i = 0; i < n; i++) {
        switch (ids[i]) {
        case 7:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam7.head);
            Dmcam_Load(124, 0);
            Dmcam_Load(134, 0);
            return 7;
        case 50:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam32.head);
            Dmcam_Load(124, 0);
            Dmcam_Load(134, 0);
            return 50;
        case 2:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam2.head);
            Dmcam_Load(131, 0);
            Dmcam_Load(137, 0);
            Dmcam_Load(138, 0);
            return 2;
        case 36:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam24.head);
            Dmcam_Load(131, 0);
            Dmcam_Load(137, 0);
            Dmcam_Load(138, 0);
            return 36;
        case 71:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam47.head);
            Dmcam_Load(131, 0);
            Dmcam_Load(137, 0);
            Dmcam_Load(138, 0);
            return 71;
        case 55:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam37.head);
            Dmcam_Load(126, 0);
            Dmcam_Load(135, 0);
            return 55;
        case 58:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam3A.head);
            Dmcam_Load(136, 0);
            return 58;
        case 54:
        case 60:
        case 59:
            Dmcam_Load(127, 0);
            break;
        case 64:
        case 65:
            Dmcam_Load(128, 0);
            break;
        }
    }
    return 0;
}

/*
 * In an offline game, reinitializes the demo cameras (Dmcam_Init) and, by the
 * area's base id (Map_DayId of gScene.map), puts the area's camera event
 * record (ScEvent_Cam2 to ScEvent_CamF) into slot 2 of list; areas without one
 * get none. Always returns 0.
 */
s32 ScEvent_LoadCamera(ScEventList *list)
{
    s32 base;

    if (Game_IsOnline() != 1) {
        Dmcam_Init(gSysSettings.readBuf, 1);
        base = Map_DayId((u16) gScene.map) & 0xFFFF;
        switch (base) {
        case 2:
            ScEvent_Set(&list->ev[2], &ScEvent_Cam2.head);
            break;
        case 3:
            ScEvent_Set(&list->ev[2], &ScEvent_Cam3.head);
            break;
        case 4:
            ScEvent_Set(&list->ev[2], &ScEvent_Cam4.head);
            break;
        case 5:
            ScEvent_Set(&list->ev[2], &ScEvent_Cam5.head);
            break;
        case 6:
            ScEvent_Set(&list->ev[2], &ScEvent_Cam6.head);
            break;
        case 11:
            ScEvent_Set(&list->ev[2], &ScEvent_CamB.head);
            break;
        case 13:
        case 14:
        case 15:
            ScEvent_Set(&list->ev[2], &ScEvent_CamF.head);
            break;
        }
    }
    return 0;
}

//---
void ScEvent_Update(void)
{
    ScEventList *w = &gSceneEvents;
    s32 i;
    ScEvent *slot;

    gSceneEvents.ended = 0;
    switch (gSceneEvents.active) {
    case 1:
        if (w->ev[w->cur].run(&w->ev[w->cur]) == 0) {
            break;
        }
        gScene.eventRunning = 0;
        w->active = 0;
    case 0:
        for (slot = w->ev, i = 0; i < 3; i++, slot++) {
            if (slot->on != 0 && slot->check(slot) > 0) {
                w->active = 1;
                w->cur = i;
                slot->run(slot);
                return;
            }
        }
        MsgEvent_OpenPopup();
        break;
    }
}

/*
 * Check callback of an event tied to an area: if the record's doneFlag is
 * already set in the save it clears the slot and returns -1 (drop the event);
 * otherwise returns 1 (start it) when the record's area is the scene's
 * current area by day id (Area_DayId(gScene.area)), else 0.
 */
s32 ScEvent_CheckDay(ScEvent *event)
{
    if (Save_TestEventFlag(event->data->doneFlag) == 1) {
        *(s32 *) event = 0;
        return -1;
    }
    return event->data->area == Area_DayId(gScene.area);
}

/*
 * Run callback of an event record, one step of event->step per call; returns 1
 * when the event is over, else 0. Step 0 sets gScene.eventRunning, points the
 * camera (ScEvent_SetCamera), sets ev->timer and pauses the stream
 * (Stream_Pause) by the record's music value (the timer for 2 to 10, the
 * pause for 1 to 3 and 8 to 10), and plays the record's day or night jingle
 * if it has one. Step 1 counts the timer down and at 0 plays the music for
 * the value; when the camera request ends (CamReq_State() <= 0) it clears
 * eventRunning, sets the record's doneFlag in the save, restores the music
 * (1: Bgm_EnterArea; 2, 3, 8 to 10: gScene.musicState 5, musicTimer 150),
 * cancels the camera and sets gSceneEvents.ended. Step 2 clears the slot's
 * first word and returns 1.
 */
s32 ScEvent_RunA(ScEvent *event)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    ScEventList *list = &gSceneEvents;

    switch (event->step) {
    case 0:
        gScene.eventRunning = 1;
        ScEvent_SetCamera(event->data->camera, event->data->camGroup);
        switch (event->data->music) {
        case 4:
            event->timer = 205;
            break;
        case 5:
            event->timer = 192;
            break;
        case 6:
            event->timer = 239;
            break;
        case 7:
            event->timer = 196;
            break;
        case 8:
            event->timer = 4;
            Stream_Pause(0, 1);
            break;
        case 9:
            event->timer = 350;
            Stream_Pause(0, 1);
            break;
        case 10:
            event->timer = 848;
            Stream_Pause(0, 1);
            break;
        case 3:
            event->timer = 270;
            Stream_Pause(0, 1);
            break;
        case 2:
            event->timer = 350;
        case 1:
            Stream_Pause(0, 1);
            break;
        case 0:
            break;
        }
        if (event->data->jingleDay != 0) {
            if (Area_IsDay(gScene.area) == 1) {
                Jingle_Play(pl, event->data->jingleDay);
            } else {
                Jingle_Play(pl, event->data->jingleNight);
            }
        }
        event->step++;
        break;
    case 1:
        if (event->timer != 0) {
            event->timer--;
            if (event->timer == 0) {
                switch (event->data->music) {
                case 0:
                case 1:
                    break;
                case 4:
                    Jingle_Play(pl, 221);
                    break;
                case 5:
                    Jingle_Play(pl, 221);
                    break;
                case 6:
                    Jingle_Play(pl, 221);
                    break;
                case 7:
                    Jingle_Play(pl, 221);
                    break;
                case 2:
                    Bgm_Play(188);
                    break;
                case 3:
                    Bgm_Play(194);
                    break;
                case 8:
                    Bgm_Play(206);
                    break;
                case 9:
                    Bgm_Play(208);
                    break;
                case 10:
                    Bgm_Play(194);
                    break;
                }
            }
        }
        if (CamReq_State() <= 0) {
            event->step++;
            gScene.eventRunning = 0;
            Save_SetEventFlag(event->data->doneFlag);
            switch (event->data->music) {
            case 1:
                Bgm_EnterArea(gScene.area);
                break;
            case 0:
                break;
            case 2:
            case 3:
            case 8:
            case 9:
            case 10:
                gScene.musicState = 5;
                gScene.musicTimer = 150;
                break;
            }
            CamReq_Cancel();
            list->ended = 1;
        }
        break;
    case 2:
        *(s32 *) event = 0;
        return 1;
    }
    return 0;
}

s32 ScEvent_CheckAlwaysA(ScEvent *event)
{
    return 1;
}

/*
 * Run callback of the ending event of a versus match, one step of event->step per
 * call; returns 1 when it is over, else 0. Every frame it sets
 * gQuest.inputLock to 90 and noHitTimer to 2 on every player. Step 0 sets
 * gScene.eventRunning; step 1 waits for a defeat bit in gQuest.versusDefeat
 * (mask 3) and starts a fade (FadeA_Start); step 2 waits for
 * Versus_BothDefeated(), then clears eventRunning and spawns an effect
 * (Effect2C_Spawn); step 3 cancels the camera request, sets
 * gSceneEvents.ended, clears the slot and returns 1.
 */
s32 ScEvent_RunVersusEnd(ScEvent *event)
{
    ScEventList *list = &gSceneEvents;
    s32 i;

    gQuest.inputLock = 90;
    for (i = 0; i < gScene.playerCount; i++) {
        gPlayers[i].noHitTimer = 2;
    }
    switch (event->step) {
    case 0:
        gScene.eventRunning = 1;
        event->step++;
        break;
    case 1:
        if (gQuest.versusDefeat & 3) {
            event->step++;
            FadeA_Start(6);
        }
        break;
    case 2:
        if (Versus_BothDefeated() == 1) {
            event->step++;
            gScene.eventRunning = 0;
            Effect2C_Spawn(90);
        }
        break;
    case 3:
        CamReq_Cancel();
        list->ended = 1;
        *(s32 *) event = 0;
        return 1;
    }
    return 0;
}

/*
 * Requests the demo camera req (CamReq_Set). With kind 0 the camera has no
 * target; otherwise it follows the first active monster whose group's first
 * kind (MonsterGroup_First of its type) is kind, and no request is made when
 * there is none.
 */
void ScEvent_SetCamera(u8 req, u8 kind)
{
    Em *em;
    s32 i;

    if (kind == 0) {
        CamReq_Set(req, 0);
        return;
    }
    em = gEms;
    for (i = 20; i != 0; i--) {
        if (em->active != 0 && kind == (u8) MonsterGroup_First(em->type)) {
            CamReq_Set(req, em);
            return;
        }
        em++;
    }
}

/*
 * Returns the doneFlag (the event flag id) of the record of the event now
 * running in gSceneEvents, or -1 when none runs.
 */
s32 ScEvent_CurId(void)
{
    ScEventList *list = &gSceneEvents;
    ScEvent *event;

    if (list->active == 0) {
        return -1;
    }
    event = &list->ev[list->cur];
    return event->data->doneFlag;
}

/*
 * Returns 1 when an event is running and its record's flags bit 0 is set (the
 * monsters hold still while it runs), else 0.
 */
s32 ScEvent_CurFlag1(void)
{
    ScEventList *list = &gSceneEvents;
    ScEvent *event;

    if (list->active != 0) {
        event = &list->ev[list->cur];
        return (event->data->flags & 1) != 0;
    }
    return 0;
}

/*
 * Starts the record rec at once in slot 0: empties gSceneEvents and clears
 * gScene.eventRunning, reinitializes the demo cameras (Dmcam_Init), loads the
 * record's camera and switches the slot on with the record's callbacks.
 * Unlike ScEvent_Set it does not look at doneFlag; plsel starts a record it
 * picked this way.
 */
void ScEvent_StartFrom(ScEventHead *rec)
{
    ScEvent *slot;

    gScene.eventRunning = 0;
    gSceneEvents.active = 0;
    *(s32 *) &gSceneEvents.ev[2] = 0;
    *(s32 *) &gSceneEvents.ev[1] = 0;
    *(s32 *) &gSceneEvents.ev[0] = 0;
    Dmcam_Init(gSysSettings.readBuf, 0);
    slot = &gSceneEvents.ev[0];
    Dmcam_Load(rec->camera, 0);
    slot->on = 1;
    slot->check = rec->check;
    slot->run = rec->run;
    slot->data = rec;
}

s32 ScEvent_CheckAlwaysB(ScEvent *event)
{
    return 1;
}

/*
 * Run callback of a record whose callbacks ScEvent_SetupB set (plsel's
 * viewer), one step of event->step per call; returns 1 when the event is over,
 * else 0. Step 0 is ScEvent_RunA's, with its own waits by the music value.
 * Step 1 counts the timer down and plays the music at 0; when the camera
 * request ends it clears eventRunning, cancels the camera and sets
 * gSceneEvents.ended, but sets no save flag and restores no music. Step 2
 * clears the slot's first word and returns 1.
 */
s32 ScEvent_RunB(ScEvent *event)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    ScEventList *list = &gSceneEvents;

    switch (event->step) {
    case 0:
        gScene.eventRunning = 1;
        ScEvent_SetCamera(event->data->camera, event->data->camGroup);
        switch (event->data->music) {
        case 8:
            event->timer = 4;
            Stream_Pause(0, 1);
            break;
        case 9:
            event->timer = 350;
            Stream_Pause(0, 1);
            break;
        case 10:
            event->timer = 900;
            Stream_Pause(0, 1);
            break;
        case 3:
            event->timer = 270;
            Stream_Pause(0, 1);
            break;
        case 2:
            event->timer = 350;
        case 1:
            Stream_Pause(0, 1);
            break;
        case 0:
            break;
        }
        if (event->data->jingleDay != 0) {
            if (Area_IsDay(gScene.area) == 1) {
                Jingle_Play(pl, event->data->jingleDay);
            } else {
                Jingle_Play(pl, event->data->jingleNight);
            }
        }
        event->step++;
        break;
    case 1:
        if (event->timer != 0) {
            event->timer--;
            if (event->timer == 0) {
                switch (event->data->music) {
                case 2:
                    Bgm_Play(188);
                    break;
                case 3:
                    Bgm_Play(194);
                    break;
                case 8:
                    Bgm_Play(206);
                    break;
                case 9:
                    Bgm_Play(208);
                    break;
                case 10:
                    Bgm_Play(194);
                    break;
                }
            }
        }
        if (CamReq_State() <= 0) {
            event->step++;
            gScene.eventRunning = 0;
            CamReq_Cancel();
            list->ended = 1;
        }
        break;
    case 2:
        *(s32 *) event = 0;
        return 1;
    }
    return 0;
}

void ScEvent_SetupB(ScEventHead *rec)
{
    rec->check = ScEvent_CheckAlwaysB;
    rec->run = ScEvent_RunB;
}

/* This file's initialised data. */
/*
 * The scene events' records (ScEventRec). ScEvent_VersusEnd is the arena
 * cup's versus quest's: no area, done flag or camera; ScEvent_CheckAlwaysA
 * starts it and ScEvent_RunVersusEnd runs the end of the match. The
 * <Monster>Sighting records are the scenes a story quest starts with
 * (ScEvent_Init picks one by gScene.questId; plsel's gallery replays them
 * through gGalleryScEvents). Each plays once per save, the first time the
 * hunter enters the record's area (ScEvent_CheckDay), and then sets its
 * doneFlag. ScEvent_RunA shows the record's camera following the first
 * monster of camGroup, with its jingle and music, while the monster's
 * Update holds it in EM_GROUP_EVENT.
 */
ScEventRec ScEvent_VersusEnd = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, ScEvent_CheckAlwaysA, ScEvent_RunVersusEnd },
};
ScEventRec ScEvent_DemoCam7 = {
#include "game/008C3520.inc"
};
ScEventRec ScEvent_DemoCam32 = {
#include "game/008C3540.inc"
};
ScEventRec ScEvent_DemoCam2 = {
#include "game/008C3560.inc"
};
ScEventRec ScEvent_DemoCam24 = {
#include "game/008C3580.inc"
};
ScEventRec ScEvent_DemoCam47 = {
#include "game/008C35A0.inc"
};
ScEventRec ScEvent_DemoCam3A = {
#include "game/008C35C0.inc"
};
ScEventRec ScEvent_DemoCam37 = {
#include "game/008C35E0.inc"
};
ScEventRec ScEvent_VelocidromeSighting = {
#include "game/008C3600.inc"
};
ScEventRec ScEvent_BulldromeSighting = {
#include "game/008C3620.inc"
};
ScEventRec ScEvent_DaimyoHermitaurSighting = {
#include "game/008C3640.inc"
};
ScEventRec ScEvent_CongalalaSighting = {
#include "game/008C3660.inc"
};
ScEventRec ScEvent_RathianSighting = {
#include "game/008C3680.inc"
};
ScEventRec ScEvent_KushalaJungleSighting = {
#include "game/008C36A0.inc"
};
ScEventRec ScEvent_KushalaSnowSighting = {
#include "game/008C36C0.inc"
};
ScEventRec ScEvent_LunastraTeostraVolcano8Sighting = {
#include "game/008C36E0.inc"
};
ScEventRec ScEvent_LunastraTeostraTowerSighting = {
#include "game/008C3700.inc"
};
ScEventRec ScEvent_GarugaSighting = {
#include "game/008C3720.inc"
};
ScEventRec ScEvent_ChameleosSighting = {
#include "game/008C3740.inc"
};
ScEventRec ScEvent_LunastraTeostraVolcano7Sighting = {
#include "game/008C3760.inc"
};
ScEventRec ScEvent_BlangongaSighting = {
#include "game/008C3780.inc"
};
ScEventRec ScEvent_ShogunCeanataurSighting = {
#include "game/008C37A0.inc"
};
ScEventRec ScEvent_KutkuSighting = {
#include "game/008C37C0.inc"
};
ScEventRec ScEvent_KhezuSighting = {
#include "game/008C37E0.inc"
};
ScEventRec ScEvent_DiablosSighting = {
#include "game/008C3800.inc"
};
ScEventRec ScEvent_GraviosSighting = {
#include "game/008C3820.inc"
};
ScEventRec ScEvent_MonoblosSighting = {
#include "game/008C3840.inc"
};
ScEventRec ScEvent_RathalosSighting = {
#include "game/008C3860.inc"
};
ScEventRec ScEvent_KirinSighting = {
#include "game/008C3880.inc"
};
ScEventRec ScEvent_Seq1 = {
#include "game/008C38A0.inc"
};
ScEventRec ScEvent_Seq5 = {
#include "game/008C38C0.inc"
};
ScEventRec ScEvent_Seq3 = {
#include "game/008C38E0.inc"
};
ScEventRec ScEvent_Seq3B = {
#include "game/008C3900.inc"
};
ScEventRec ScEvent_Seq11 = {
#include "game/008C3920.inc"
};
ScEventRec ScEvent_CamF = {
#include "game/008C3940.inc"
};
ScEventRec ScEvent_Cam6 = {
#include "game/008C3960.inc"
};
ScEventRec ScEvent_CamB = {
#include "game/008C3980.inc"
};
ScEventRec ScEvent_Cam2 = {
#include "game/008C39A0.inc"
};
ScEventRec ScEvent_Cam3 = {
#include "game/008C39C0.inc"
};
ScEventRec ScEvent_Cam4 = {
#include "game/008C39E0.inc"
};
ScEventRec ScEvent_Cam5 = {
#include "game/008C3A00.inc"
};
/* Read by plsel's gallery as a table of GalView pointers by gallery entry (Galex_QuestTask). */
ScEventHead *gGalleryScEvents[48] = {
    &ScEvent_BulldromeSighting.head, &ScEvent_VelocidromeSighting.head, &ScEvent_KutkuSighting.head, &ScEvent_DaimyoHermitaurSighting.head,
    0, &ScEvent_CongalalaSighting.head, 0, &ScEvent_RathianSighting.head,
    &ScEvent_KushalaJungleSighting.head, &ScEvent_KhezuSighting.head, &ScEvent_BlangongaSighting.head, &ScEvent_ShogunCeanataurSighting.head,
    &ScEvent_DiablosSighting.head, &ScEvent_KushalaSnowSighting.head, 0, &ScEvent_LunastraTeostraVolcano8Sighting.head,
    &ScEvent_LunastraTeostraVolcano7Sighting.head, &ScEvent_GraviosSighting.head, &ScEvent_LunastraTeostraTowerSighting.head, &ScEvent_KirinSighting.head,
    &ScEvent_RathalosSighting.head, &ScEvent_MonoblosSighting.head, &ScEvent_ChameleosSighting.head, &ScEvent_GarugaSighting.head,
    &ScEvent_DemoCam7.head, &ScEvent_DemoCam37.head, &ScEvent_DemoCam2.head, &ScEvent_DemoCam24.head,
    &ScEvent_DemoCam47.head, &ScEvent_DemoCam3A.head, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, &ScEvent_Cam6.head,
    &ScEvent_Cam3.head, &ScEvent_Cam4.head, &ScEvent_CamB.head, &ScEvent_Cam5.head,
    &ScEvent_CamF.head, &ScEvent_Cam2.head, 0, 0,
};
