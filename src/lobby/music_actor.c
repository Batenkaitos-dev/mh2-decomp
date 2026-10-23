/*
 * The townsperson the town's music moves (the music actor), and the record
 * book's page texts.
 *
 * While the lobby's music show plays, the music player (LobbyBgm_StepPlay)
 * moves one townsperson, the one of look 78 that Npc_Init hands to
 * LobbyBgm_SetActor. Each entry of each track has a motion script,
 * gMusicActorScripts[track][entry]: a list of MusicActorKey (a frame, a
 * motion with its blend and start frames, a speed), ended by frame -1.
 * MusicActor_StartScript starts an entry's script when the music reaches
 * that entry, MusicActor_Update runs it a frame at a time (the motions, the
 * floor, the swing chains, the draw slot) and MusicActor_Draw draws the
 * actor once the music is past its opening. gMusicActorScript holds the
 * script running and its frame.
 *
 * The rest of the data is the record book's texts, which the record book's
 * draws reach through their tables: the horn score pages, the guide's
 * pages, its entry names and category names. They follow the actor's
 * scripts in the overlay's data, so they are defined here.
 */
#include "common.h"
#include "em.h"
#include "sprite.h"
#include "player.h"
#include "render_state.h"
#include "area.h"
#include "bgm.h"
#include "place_rules.h"
#include "card.h"
#include "light.h"
#include "text.h"
#include "music_actor.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "lobby_world.h"
#include "town_players.h"
#include "vec_math.h"

extern MusicActorScript gMusicActorScript;

struct EmBone;
extern MusicActorKey **gMusicActorScripts[];
struct TriHdr;

/* starts the actor's motion script (set, idx) at frame 0 */
void MusicActor_StartScript(Em *actor, s16 track, s16 entry)
{
    gMusicActorScript.entry = entry;
    gMusicActorScript.track = track;
    gMusicActorScript.keys = MusicActor_GetScript(gMusicActorScript.track, gMusicActorScript.entry);
    gMusicActorScript.frame = 0;
    MusicActor_RunScript(actor, gMusicActorScript.frame);
}

/* The music actor's frame, run by the music player while the music plays:
 * advances its script a frame (starting the motion a key gives), keeps it
 * on the floor, runs its timers, hit stop and lights, sets its matrix, runs
 * the swing chains of its fixed effect list (from bone 26's matrix times
 * bone 23's, pushed by bone 9) and queues its draw slot, drawn by
 * MusicActor_Draw. Debug switches 16 and 36, off in the retail game, skip
 * the floor and the draw. */
void MusicActor_Update(Em *actor)
{
    f32 swingBase[16];
    Model *mdl;
    f32 *boneMtx;

    mdl = actor->model;
    actor->drawSlot->draw = MusicActor_Draw;
    actor->prevPosX = actor->posX;
    actor->prevPosY = actor->posY;
    actor->prevPosZ = actor->posZ;
    gMusicActorScript.frame++;
    MusicActor_RunScript(actor, gMusicActorScript.frame);
    if (Debug_IsOn(16) == 0) {
        actor->floorY = AreaGrid2_GetFloorY(&actor->posX);
        Em_GetFloorInfo(actor, &actor->posX, (struct TriHdr *) &actor->floorKind, &actor->floorY, (s32 *) &actor->surfaceY);
    }
    Player_TickTimers((Player *) actor);
    Player_UpdateHitStop((Player *) actor);
    LightSet_UpdateAll();
    MusicActor_SetMatrix(actor);
    boneMtx = Em_BoneMatrix(actor, 26);
    Mtx_Mul(swingBase, boneMtx, (f32 *)(Em_GetBone(actor, 23)));
    Effect_RunFixedList(((Player *) actor)->equipFx, (EmBone *) swingBase, (EmBone *) (Em_GetBone(actor, 9)), &mdl->bones[27]);
    actor->drawSlot->pos[0] = actor->posX;
    actor->drawSlot->pos[1] = actor->posY;
    actor->drawSlot->pos[2] = actor->posZ;
    if (Debug_IsOn(36) == 0) {
        OT_AddByDepth(gDrawOt32, actor->drawSlot, 32, 0);
    }
}

/* starts the script's key for frame, if one has that frame (only the first
 * such key), then runs the actor's motions (Actor_StartMotions); returns 0 */
s32 MusicActor_RunScript(Em *actor, s32 frame)
{
    MusicActorKey *key;

    key = gMusicActorScript.keys;
    while (key->frame != -1) {
        if (key->frame == (s16)frame) {
            if (key->speed > 0.0f) {
                actor->motion[0].step = 2.0f * key->speed;
                actor->motion[1].step = 2.0f * key->speed;
            }
            Actor_SetMotion(actor, key->motion, key->blend, key->startFrame);
            break;
        }
        key++;
    }

    Actor_StartMotions((Player *)actor);
    return 0;
}

/* the motion script of entry idx of track set's music */
MusicActorKey *MusicActor_GetScript(s32 track, s32 entry)
{
    return gMusicActorScripts[(s16)track][(s16)entry];
}

/* the music actor's matrix from its angles (rotX), position (posX) and
 * scale (scaleX), stored in its worldMtx and passed down its model's bones */
void MusicActor_SetMatrix(Em *actor)
{
    f32 mtx[16];
    f32 scaleMtx[16];
    Model *mdl;

    mdl = actor->model;
    Mtx_Scale(scaleMtx, actor->scaleX, actor->scaleY, actor->scaleZ);
    Mtx_FromAnglesYXZ(&actor->rotX, mtx);
    Mtx_SetTrans(mtx, actor->posX, actor->posY, actor->posZ);
    Mtx_Concat3x3(mtx, scaleMtx);
    Mtx_Copy((f32 *) actor->worldMtx, mtx);
    Bone_UpdateWorldScaledUpToRoot(mdl->bones, mtx, 1);
}

/* The music actor's draw callback. It draws the actor the music player
 * holds (LobbyBgm_GetActor), not the slot's object, and only once the
 * music is past its opening: the music player sets that flag when the
 * music's position reaches 100 and clears it when it ends. It builds the
 * matrix as MusicActor_SetMatrix does, then draws each material set of
 * the actor's model with its materials. */
void MusicActor_Draw(DrawSlot *slot /* unused: the draw-slot walk passes it */)
{
    f32 angles[3];
    f32 mtx[16];
    f32 identity[16];
    f32 scaleMtx[16];
    Em *actor;
    Model *mdl;
    ModelMatSet *matSet;
    ModelMat *mats;
    s32 i;
    s32 setCount;
    s32 j;

    actor = LobbyBgm_GetActor();
    mdl = actor->model;
    actor->visible = LobbyBgm_IsActorShown(); /* the music player's "show the actor" flag */
    if (actor->active == 0 || actor->visible == 0) {
        return;
    }
    LightSet_SetColors(actor, 1);
    LightSet_FadeColors(actor);
    Vec_AnglesToRad(&actor->rotX, angles); /* angles is not used after: the game makes the call */
    Mtx_Scale(scaleMtx, actor->scaleX, actor->scaleY, actor->scaleZ);
    Mtx_FromAnglesYXZ(&actor->rotX, mtx);
    Mtx_SetTrans(mtx, actor->posX, actor->posY, actor->posZ);
    Mtx_Concat3x3(mtx, scaleMtx);
    Mtx_Copy((f32 *) actor->worldMtx, mtx);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    matSet = mdl->matSets;
    setCount = mdl->matSetCount;
    mats = mdl->mats;
    Bone_UpdateWorldScaledRoot(&mdl->bones[26], (f32 *) &mdl->bones[23]);
    Bone_StoreMatrices(mdl->bones);
    Mtx_Identity(identity);
    Gs_SetState(RS_UV_MTX, (s32)identity);
    Tex_Prepare(10, 163);
    for (i = 0; i < setCount; i++, matSet++) {
        Gs_SetState(RS_COLOR2, -1);
        if (matSet->id != -1) {
            for (j = 0; j < matSet->count; j++) {
                Gs_SetState((u8)RS_MATERIAL(j), (s32)&mats[matSet->mat[j]]);
            }
            Draw_ApplyStates(matSet->flags);
            Model_Draw(matSet->id, 0);
        }
    }
    Draw_SetDefaultState();
    Gs_SetState(RS_ALPHA_REF, 0);
}

/* The file's data: the music actor's motion scripts, then the record
 * book's page texts. A script is a list of MusicActorKey, ended by a key
 * of frame -1; gMusicActorScripts_T lists track T's scripts by entry. */
MusicActorKey gMusicActorScripts_0_0[5] = {
#include "lobby/0081BCA0.inc"
};
MusicActorKey gMusicActorScripts_0_1[4] = {
#include "lobby/0081BCE0.inc"
};
MusicActorKey gMusicActorScripts_0_2[4] = {
#include "lobby/0081BD10.inc"
};
MusicActorKey gMusicActorScripts_0_3[5] = {
#include "lobby/0081BD40.inc"
};
MusicActorKey gMusicActorScripts_0_4[5] = {
#include "lobby/0081BD80.inc"
};
MusicActorKey gMusicActorScripts_0_5[3] = {
#include "lobby/0081BDC0.inc"
};
MusicActorKey gMusicActorScripts_0_6[3] = {
#include "lobby/0081BDF0.inc"
};
MusicActorKey gMusicActorScripts_0_7[6] = {
#include "lobby/0081BE20.inc"
};
MusicActorKey *gMusicActorScripts_0[12] = {
    gMusicActorScripts_0_0, gMusicActorScripts_0_1, gMusicActorScripts_0_2, gMusicActorScripts_0_3,
    gMusicActorScripts_0_4, gMusicActorScripts_0_5, gMusicActorScripts_0_6, gMusicActorScripts_0_7,
    NULL, NULL, NULL, NULL,
};
MusicActorKey gMusicActorScripts_1_0[5] = {
#include "lobby/0081BEA0.inc"
};
MusicActorKey gMusicActorScripts_1_1[2] = {
#include "lobby/0081BEE0.inc"
};
MusicActorKey gMusicActorScripts_1_2[6] = {
#include "lobby/0081BF00.inc"
};
MusicActorKey gMusicActorScripts_1_3[5] = {
#include "lobby/0081BF50.inc"
};
MusicActorKey gMusicActorScripts_1_4[5] = {
#include "lobby/0081BF90.inc"
};
MusicActorKey gMusicActorScripts_1_5[2] = {
#include "lobby/0081BFD0.inc"
};
MusicActorKey gMusicActorScripts_1_6[3] = {
#include "lobby/0081BFF0.inc"
};
MusicActorKey gMusicActorScripts_1_7[3] = {
#include "lobby/0081C020.inc"
};
MusicActorKey gMusicActorScripts_1_8[4] = {
#include "lobby/0081C050.inc"
};
MusicActorKey *gMusicActorScripts_1[12] = {
    gMusicActorScripts_1_0, gMusicActorScripts_1_1, gMusicActorScripts_1_2, gMusicActorScripts_1_3,
    gMusicActorScripts_1_4, gMusicActorScripts_1_5, gMusicActorScripts_1_6, gMusicActorScripts_1_7,
    gMusicActorScripts_1_8, NULL, NULL, NULL,
};
MusicActorKey gMusicActorScripts_2_0[4] = {
#include "lobby/0081C0B0.inc"
};
MusicActorKey gMusicActorScripts_2_1[2] = {
#include "lobby/0081C0E0.inc"
};
MusicActorKey gMusicActorScripts_2_2[5] = {
#include "lobby/0081C100.inc"
};
MusicActorKey gMusicActorScripts_2_3[3] = {
#include "lobby/0081C140.inc"
};
MusicActorKey gMusicActorScripts_2_4[3] = {
#include "lobby/0081C170.inc"
};
MusicActorKey gMusicActorScripts_2_5[3] = {
#include "lobby/0081C1A0.inc"
};
MusicActorKey gMusicActorScripts_2_6[3] = {
#include "lobby/0081C1D0.inc"
};
MusicActorKey gMusicActorScripts_2_7[6] = {
#include "lobby/0081C200.inc"
};
MusicActorKey *gMusicActorScripts_2[12] = {
    gMusicActorScripts_2_0, gMusicActorScripts_2_1, gMusicActorScripts_2_2, gMusicActorScripts_2_3,
    gMusicActorScripts_2_4, gMusicActorScripts_2_5, gMusicActorScripts_2_6, gMusicActorScripts_2_7,
    NULL, NULL, NULL, NULL,
};
MusicActorKey gMusicActorScripts_3_0[4] = {
#include "lobby/0081C280.inc"
};
MusicActorKey gMusicActorScripts_3_1[4] = {
#include "lobby/0081C2B0.inc"
};
MusicActorKey gMusicActorScripts_3_2[4] = {
#include "lobby/0081C2E0.inc"
};
MusicActorKey gMusicActorScripts_3_3[4] = {
#include "lobby/0081C310.inc"
};
MusicActorKey gMusicActorScripts_3_4[5] = {
#include "lobby/0081C340.inc"
};
MusicActorKey gMusicActorScripts_3_5[3] = {
#include "lobby/0081C380.inc"
};
MusicActorKey gMusicActorScripts_3_6[3] = {
#include "lobby/0081C3B0.inc"
};
MusicActorKey gMusicActorScripts_3_7[2] = {
#include "lobby/0081C3E0.inc"
};
MusicActorKey gMusicActorScripts_3_8[5] = {
#include "lobby/0081C400.inc"
};
MusicActorKey gMusicActorScripts_3_9[3] = {
#include "lobby/0081C440.inc"
};
MusicActorKey gMusicActorScripts_3_10[3] = {
#include "lobby/0081C470.inc"
};
MusicActorKey gMusicActorScripts_3_11[3] = {
#include "lobby/0081C4A0.inc"
};
MusicActorKey gMusicActorScripts_3_12[5] = {
#include "lobby/0081C4D0.inc"
};
MusicActorKey gMusicActorScripts_3_13[4] = {
#include "lobby/0081C510.inc"
};
MusicActorKey *gMusicActorScripts_3[16] = {
    gMusicActorScripts_3_0, gMusicActorScripts_3_1, gMusicActorScripts_3_2, gMusicActorScripts_3_3,
    gMusicActorScripts_3_4, gMusicActorScripts_3_5, gMusicActorScripts_3_6, gMusicActorScripts_3_7,
    gMusicActorScripts_3_8, gMusicActorScripts_3_9, gMusicActorScripts_3_10, gMusicActorScripts_3_11,
    gMusicActorScripts_3_12, gMusicActorScripts_3_13, NULL, NULL,
};
/* the motion scripts by track (LobbyBgm_Play's id), then by entry; tracks
 * 0-3 have scripts */
MusicActorKey* *gMusicActorScripts[32] = {
    gMusicActorScripts_0, gMusicActorScripts_1, gMusicActorScripts_2, gMusicActorScripts_3,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
/* the record book's texts (records_draw.c's page tables):
 * the horn score pages, then the older score pages */
char gHornScorePage0[272] = {
#include "lobby/0081C600.inc"
};
char gHornScorePage1[208] = {
#include "lobby/0081C710.inc"
};
char gHornScorePage2[384] = {
#include "lobby/0081C7E0.inc"
};
char gHornScorePage3[272] = {
#include "lobby/0081C960.inc"
};
char gHornScorePage4[464] = {
#include "lobby/0081CA70.inc"
};
char gOldHornScorePage0[208] = {
#include "lobby/0081CC40.inc"
};
char gOldHornScorePage1[240] = {
#include "lobby/0081CD10.inc"
};
char gOldHornScorePage2[368] = {
#include "lobby/0081CE00.inc"
};
char gOldHornScorePage3[544] = {
#include "lobby/0081CF70.inc"
};
/* the guide's pages by category, entry and page */
char gRecordGuidePage0_0_0[128] = {
#include "lobby/0081D190.inc"
};
char gRecordGuidePage0_0_1[240] = {
#include "lobby/0081D210.inc"
};
char gRecordGuidePage0_0_2[208] = {
#include "lobby/0081D300.inc"
};
char gRecordGuidePage0_1_0[192] = {
#include "lobby/0081D3D0.inc"
};
char gRecordGuidePage0_1_1[176] = {
#include "lobby/0081D490.inc"
};
char gRecordGuidePage0_2_0[208] = {
#include "lobby/0081D540.inc"
};
char gRecordGuidePage0_2_1[224] = {
#include "lobby/0081D610.inc"
};
char gRecordGuidePage0_3_0[240] = {
#include "lobby/0081D6F0.inc"
};
char gRecordGuidePage0_3_1[192] = {
#include "lobby/0081D7E0.inc"
};
char gRecordGuidePage0_4_0[224] = {
#include "lobby/0081D8A0.inc"
};
char gRecordGuidePage0_4_1[240] = {
#include "lobby/0081D980.inc"
};
char gRecordGuidePage0_4_2[176] = {
#include "lobby/0081DA70.inc"
};
char gRecordGuidePage0_5_0[192] = {
#include "lobby/0081DB20.inc"
};
char gRecordGuidePage0_5_1[192] = {
#include "lobby/0081DBE0.inc"
};
char gRecordGuidePage0_6_0[208] = {
#include "lobby/0081DCA0.inc"
};
char gRecordGuidePage0_6_1[160] = {
#include "lobby/0081DD70.inc"
};
char gRecordGuidePage0_7_0[224] = {
#include "lobby/0081DE10.inc"
};
char gRecordGuidePage0_7_1[208] = {
#include "lobby/0081DEF0.inc"
};
char gRecordGuidePage0_7_2[208] = {
#include "lobby/0081DFC0.inc"
};
char gRecordGuidePage0_7_3[224] = {
#include "lobby/0081E090.inc"
};
char gRecordGuidePage0_8_0[240] = {
#include "lobby/0081E170.inc"
};
char gRecordGuidePage0_8_1[176] = {
#include "lobby/0081E260.inc"
};
char gRecordGuidePage0_9_0[144] = {
#include "lobby/0081E310.inc"
};
char gRecordGuidePage0_9_1[192] = {
#include "lobby/0081E3A0.inc"
};
char gRecordGuidePage0_9_2[160] = {
#include "lobby/0081E460.inc"
};
char gRecordGuidePage0_9_3[208] = {
#include "lobby/0081E500.inc"
};
char gRecordGuidePage0_10_0[208] = {
#include "lobby/0081E5D0.inc"
};
char gRecordGuidePage0_10_1[192] = {
#include "lobby/0081E6A0.inc"
};
char gRecordGuidePage0_11_0[256] = {
#include "lobby/0081E760.inc"
};
char gRecordGuidePage0_11_1[208] = {
#include "lobby/0081E860.inc"
};
char gRecordGuidePage0_11_2[192] = {
#include "lobby/0081E930.inc"
};
char gRecordGuidePage0_12_0[240] = {
#include "lobby/0081E9F0.inc"
};
char gRecordGuidePage0_12_1[256] = {
#include "lobby/0081EAE0.inc"
};
char gRecordGuidePage0_12_2[192] = {
#include "lobby/0081EBE0.inc"
};
char gRecordGuidePage0_13_0[224] = {
#include "lobby/0081ECA0.inc"
};
char gRecordGuidePage0_13_1[160] = {
#include "lobby/0081ED80.inc"
};
char gRecordGuidePage0_14_0[224] = {
#include "lobby/0081EE20.inc"
};
char gRecordGuidePage0_14_1[240] = {
#include "lobby/0081EF00.inc"
};
char gRecordGuidePage1_0_0[160] = {
#include "lobby/0081EFF0.inc"
};
char gRecordGuidePage1_0_1[128] = {
#include "lobby/0081F090.inc"
};
char gRecordGuidePage1_1_0[176] = {
#include "lobby/0081F110.inc"
};
char gRecordGuidePage1_1_1[224] = {
#include "lobby/0081F1C0.inc"
};
char gRecordGuidePage1_1_2[176] = {
#include "lobby/0081F2A0.inc"
};
char gRecordGuidePage1_1_3[160] = {
#include "lobby/0081F350.inc"
};
char gRecordGuidePage1_2_0[192] = {
#include "lobby/0081F3F0.inc"
};
char gRecordGuidePage1_2_1[160] = {
#include "lobby/0081F4B0.inc"
};
char gRecordGuidePage1_3_0[224] = {
#include "lobby/0081F550.inc"
};
char gRecordGuidePage1_3_1[192] = {
#include "lobby/0081F630.inc"
};
char gRecordGuidePage1_4_0[240] = {
#include "lobby/0081F6F0.inc"
};
char gRecordGuidePage1_4_1[192] = {
#include "lobby/0081F7E0.inc"
};
char gRecordGuidePage1_4_2[176] = {
#include "lobby/0081F8A0.inc"
};
char gRecordGuidePage1_5_0[272] = {
#include "lobby/0081F950.inc"
};
char gRecordGuidePage1_5_1[256] = {
#include "lobby/0081FA60.inc"
};
char gRecordGuidePage1_6_0[256] = {
#include "lobby/0081FB60.inc"
};
char gRecordGuidePage1_6_1[224] = {
#include "lobby/0081FC60.inc"
};
char gRecordGuidePage1_7_0[240] = {
#include "lobby/0081FD40.inc"
};
char gRecordGuidePage1_7_1[160] = {
#include "lobby/0081FE30.inc"
};
char gRecordGuidePage1_8_0[240] = {
#include "lobby/0081FED0.inc"
};
char gRecordGuidePage1_9_0[224] = {
#include "lobby/0081FFC0.inc"
};
char gRecordGuidePage1_9_1[224] = {
#include "lobby/008200A0.inc"
};
char gRecordGuidePage1_9_2[208] = {
#include "lobby/00820180.inc"
};
char gRecordGuidePage1_9_3[224] = {
#include "lobby/00820250.inc"
};
char gRecordGuidePage2_0_0[256] = {
#include "lobby/00820330.inc"
};
char gRecordGuidePage2_1_0[240] = {
#include "lobby/00820430.inc"
};
char gRecordGuidePage2_1_1[160] = {
#include "lobby/00820520.inc"
};
char gRecordGuidePage2_2_0[192] = {
#include "lobby/008205C0.inc"
};
char gRecordGuidePage2_2_1[224] = {
#include "lobby/00820680.inc"
};
char gRecordGuidePage2_2_2[272] = {
#include "lobby/00820760.inc"
};
char gRecordGuidePage2_3_0[224] = {
#include "lobby/00820870.inc"
};
char gRecordGuidePage2_3_1[176] = {
#include "lobby/00820950.inc"
};
char gRecordGuidePage2_3_2[208] = {
#include "lobby/00820A00.inc"
};
char gRecordGuidePage2_3_3[240] = {
#include "lobby/00820AD0.inc"
};
char gRecordGuidePage2_3_4[256] = {
#include "lobby/00820BC0.inc"
};
char gRecordGuidePage2_4_0[240] = {
#include "lobby/00820CC0.inc"
};
char gRecordGuidePage2_4_1[192] = {
#include "lobby/00820DB0.inc"
};
char gRecordGuidePage2_4_2[240] = {
#include "lobby/00820E70.inc"
};
char gRecordGuidePage2_5_0[224] = {
#include "lobby/00820F60.inc"
};
char gRecordGuidePage2_5_1[256] = {
#include "lobby/00821040.inc"
};
char gRecordGuidePage2_6_0[208] = {
#include "lobby/00821140.inc"
};
char gRecordGuidePage2_6_1[208] = {
#include "lobby/00821210.inc"
};
char gRecordGuidePage2_6_2[208] = {
#include "lobby/008212E0.inc"
};
char gRecordGuidePage2_7_0[240] = {
#include "lobby/008213B0.inc"
};
char gRecordGuidePage2_7_1[176] = {
#include "lobby/008214A0.inc"
};
char gRecordGuidePage2_8_0[192] = {
#include "lobby/00821550.inc"
};
char gRecordGuidePage2_8_1[224] = {
#include "lobby/00821610.inc"
};
char gRecordGuidePage2_9_0[176] = {
#include "lobby/008216F0.inc"
};
char gRecordGuidePage2_9_1[160] = {
#include "lobby/008217A0.inc"
};
char gRecordGuidePage3_0_0[192] = {
#include "lobby/00821840.inc"
};
char gRecordGuidePage3_0_1[176] = {
#include "lobby/00821900.inc"
};
char gRecordGuidePage3_0_2[192] = {
#include "lobby/008219B0.inc"
};
char gRecordGuidePage3_0_3[176] = {
#include "lobby/00821A70.inc"
};
char gRecordGuidePage3_1_0[176] = {
#include "lobby/00821B20.inc"
};
char gRecordGuidePage3_1_1[208] = {
#include "lobby/00821BD0.inc"
};
char gRecordGuidePage3_1_2[192] = {
#include "lobby/00821CA0.inc"
};
char gRecordGuidePage3_2_0[176] = {
#include "lobby/00821D60.inc"
};
char gRecordGuidePage3_2_1[192] = {
#include "lobby/00821E10.inc"
};
char gRecordGuidePage3_2_2[176] = {
#include "lobby/00821ED0.inc"
};
char gRecordGuidePage3_2_3[160] = {
#include "lobby/00821F80.inc"
};
char gRecordGuidePage3_3_0[160] = {
#include "lobby/00822020.inc"
};
char gRecordGuidePage3_3_1[144] = {
#include "lobby/008220C0.inc"
};
char gRecordGuidePage3_3_2[224] = {
#include "lobby/00822150.inc"
};
char gRecordGuidePage3_4_0[192] = {
#include "lobby/00822230.inc"
};
char gRecordGuidePage3_4_1[240] = {
#include "lobby/008222F0.inc"
};
char gRecordGuidePage3_4_2[224] = {
#include "lobby/008223E0.inc"
};
char gRecordGuidePage3_5_0[176] = {
#include "lobby/008224C0.inc"
};
char gRecordGuidePage3_5_1[208] = {
#include "lobby/00822570.inc"
};
char gRecordGuidePage3_5_2[208] = {
#include "lobby/00822640.inc"
};
char gRecordGuidePage3_5_3[192] = {
#include "lobby/00822710.inc"
};
char gRecordGuidePage3_5_4[224] = {
#include "lobby/008227D0.inc"
};
char gRecordGuidePage3_6_0[176] = {
#include "lobby/008228B0.inc"
};
char gRecordGuidePage3_6_1[192] = {
#include "lobby/00822960.inc"
};
char gRecordGuidePage3_7_0[208] = {
#include "lobby/00822A20.inc"
};
char gRecordGuidePage3_7_1[160] = {
#include "lobby/00822AF0.inc"
};
char gRecordGuidePage3_7_2[176] = {
#include "lobby/00822B90.inc"
};
char gRecordGuidePage3_7_3[176] = {
#include "lobby/00822C40.inc"
};
char gRecordGuidePage3_8_0[160] = {
#include "lobby/00822CF0.inc"
};
char gRecordGuidePage3_8_1[192] = {
#include "lobby/00822D90.inc"
};
char gRecordGuidePage3_8_2[192] = {
#include "lobby/00822E50.inc"
};
char gRecordGuidePage3_8_3[192] = {
#include "lobby/00822F10.inc"
};
char gRecordGuidePage3_9_0[144] = {
#include "lobby/00822FD0.inc"
};
char gRecordGuidePage3_9_1[240] = {
#include "lobby/00823060.inc"
};
char gRecordGuidePage3_9_2[208] = {
#include "lobby/00823150.inc"
};
char gRecordGuidePage3_10_0[160] = {
#include "lobby/00823220.inc"
};
char gRecordGuidePage3_10_1[176] = {
#include "lobby/008232C0.inc"
};
char gRecordGuidePage3_10_2[176] = {
#include "lobby/00823370.inc"
};
char gRecordGuidePage3_11_0[176] = {
#include "lobby/00823420.inc"
};
char gRecordGuidePage3_11_1[208] = {
#include "lobby/008234D0.inc"
};
char gRecordGuidePage3_11_2[224] = {
#include "lobby/008235A0.inc"
};
char gRecordGuidePage3_12_0[208] = {
#include "lobby/00823680.inc"
};
char gRecordGuidePage3_12_1[192] = {
#include "lobby/00823750.inc"
};
char gRecordGuidePage3_12_2[208] = {
#include "lobby/00823810.inc"
};
char gRecordGuidePage3_13_0[176] = {
#include "lobby/008238E0.inc"
};
char gRecordGuidePage3_13_1[208] = {
#include "lobby/00823990.inc"
};
char gRecordGuidePage3_13_2[160] = {
#include "lobby/00823A60.inc"
};
char gRecordGuidePage3_13_3[160] = {
#include "lobby/00823B00.inc"
};
char gRecordGuidePage3_14_0[192] = {
#include "lobby/00823BA0.inc"
};
char gRecordGuidePage3_14_1[192] = {
#include "lobby/00823C60.inc"
};
char gRecordGuidePage3_14_2[208] = {
#include "lobby/00823D20.inc"
};
char gRecordGuidePage3_14_3[208] = {
#include "lobby/00823DF0.inc"
};
char gRecordGuidePage3_15_0[160] = {
#include "lobby/00823EC0.inc"
};
char gRecordGuidePage3_15_1[224] = {
#include "lobby/00823F60.inc"
};
char gRecordGuidePage3_15_2[160] = {
#include "lobby/00824040.inc"
};
char gRecordGuidePage3_15_3[208] = {
#include "lobby/008240E0.inc"
};
char gRecordGuidePage3_15_4[240] = {
#include "lobby/008241B0.inc"
};
char gRecordGuidePage3_16_0[288] = {
#include "lobby/008242A0.inc"
};
char gRecordGuidePage3_16_1[144] = {
#include "lobby/008243C0.inc"
};
char gRecordGuidePage3_16_2[208] = {
#include "lobby/00824450.inc"
};
char gRecordGuidePage3_16_3[224] = {
#include "lobby/00824520.inc"
};
char gRecordGuidePage3_17_0[176] = {
#include "lobby/00824600.inc"
};
char gRecordGuidePage3_17_1[160] = {
#include "lobby/008246B0.inc"
};
char gRecordGuidePage3_17_2[224] = {
#include "lobby/00824750.inc"
};
char gRecordGuidePage3_17_3[160] = {
#include "lobby/00824830.inc"
};
char gRecordGuidePage3_18_0[288] = {
#include "lobby/008248D0.inc"
};
char gRecordGuidePage3_18_1[128] = {
#include "lobby/008249F0.inc"
};
char gRecordGuidePage3_18_2[224] = {
#include "lobby/00824A70.inc"
};
char gRecordGuidePage3_18_3[224] = {
#include "lobby/00824B50.inc"
};
char gRecordGuidePage3_19_0[176] = {
#include "lobby/00824C30.inc"
};
char gRecordGuidePage3_19_1[192] = {
#include "lobby/00824CE0.inc"
};
char gRecordGuidePage3_19_2[192] = {
#include "lobby/00824DA0.inc"
};
char gRecordGuidePage3_19_3[160] = {
#include "lobby/00824E60.inc"
};
char gRecordGuidePage3_20_0[208] = {
#include "lobby/00824F00.inc"
};
char gRecordGuidePage3_20_1[224] = {
#include "lobby/00824FD0.inc"
};
char gRecordGuidePage3_20_2[128] = {
#include "lobby/008250B0.inc"
};
char gRecordGuidePage3_21_0[208] = {
#include "lobby/00825130.inc"
};
char gRecordGuidePage3_21_1[160] = {
#include "lobby/00825200.inc"
};
char gRecordGuidePage3_22_0[192] = {
#include "lobby/008252A0.inc"
};
char gRecordGuidePage3_22_1[192] = {
#include "lobby/00825360.inc"
};
/* the guide's entry names by category and entry */
char gRecordGuideEntryName0_0[32] = {
#include "lobby/00825420.inc"
};
char gRecordGuideEntryName0_1[32] = {
#include "lobby/00825440.inc"
};
char gRecordGuideEntryName0_2[32] = {
#include "lobby/00825460.inc"
};
char gRecordGuideEntryName0_3[32] = {
#include "lobby/00825480.inc"
};
char gRecordGuideEntryName0_4[32] = {
#include "lobby/008254A0.inc"
};
char gRecordGuideEntryName0_5[32] = {
#include "lobby/008254C0.inc"
};
char gRecordGuideEntryName0_6[32] = {
#include "lobby/008254E0.inc"
};
char gRecordGuideEntryName0_7[32] = {
#include "lobby/00825500.inc"
};
char gRecordGuideEntryName0_8[32] = {
#include "lobby/00825520.inc"
};
char gRecordGuideEntryName0_9[32] = {
#include "lobby/00825540.inc"
};
char gRecordGuideEntryName0_10[32] = {
#include "lobby/00825560.inc"
};
char gRecordGuideEntryName0_11[32] = {
#include "lobby/00825580.inc"
};
char gRecordGuideEntryName0_12[32] = {
#include "lobby/008255A0.inc"
};
char gRecordGuideEntryName0_13[32] = {
#include "lobby/008255C0.inc"
};
char gRecordGuideEntryName0_14[32] = {
#include "lobby/008255E0.inc"
};
char gRecordGuideEntryName1_0[32] = {
#include "lobby/00825600.inc"
};
char gRecordGuideEntryName1_1[32] = {
#include "lobby/00825620.inc"
};
char gRecordGuideEntryName1_2[32] = {
#include "lobby/00825640.inc"
};
char gRecordGuideEntryName1_3[32] = {
#include "lobby/00825660.inc"
};
char gRecordGuideEntryName1_4[32] = {
#include "lobby/00825680.inc"
};
char gRecordGuideEntryName1_5[32] = {
#include "lobby/008256A0.inc"
};
char gRecordGuideEntryName1_6[32] = {
#include "lobby/008256C0.inc"
};
char gRecordGuideEntryName1_7[32] = {
#include "lobby/008256E0.inc"
};
char gRecordGuideEntryName1_8[32] = {
#include "lobby/00825700.inc"
};
char gRecordGuideEntryName1_9[32] = {
#include "lobby/00825720.inc"
};
char gRecordGuideEntryName2_0[32] = {
#include "lobby/00825740.inc"
};
char gRecordGuideEntryName2_1[32] = {
#include "lobby/00825760.inc"
};
char gRecordGuideEntryName2_2[32] = {
#include "lobby/00825780.inc"
};
char gRecordGuideEntryName2_3[32] = {
#include "lobby/008257A0.inc"
};
char gRecordGuideEntryName2_4[32] = {
#include "lobby/008257C0.inc"
};
char gRecordGuideEntryName2_5[32] = {
#include "lobby/008257E0.inc"
};
char gRecordGuideEntryName2_6[32] = {
#include "lobby/00825800.inc"
};
char gRecordGuideEntryName2_7[32] = {
#include "lobby/00825820.inc"
};
char gRecordGuideEntryName2_8[32] = {
#include "lobby/00825840.inc"
};
char gRecordGuideEntryName2_9[32] = {
#include "lobby/00825860.inc"
};
char gRecordGuideEntryName3_0[32] = {
#include "lobby/00825880.inc"
};
char gRecordGuideEntryName3_1[32] = {
#include "lobby/008258A0.inc"
};
char gRecordGuideEntryName3_2[32] = {
#include "lobby/008258C0.inc"
};
char gRecordGuideEntryName3_4[32] = {
#include "lobby/008258E0.inc"
};
char gRecordGuideEntryName3_6[32] = {
#include "lobby/00825900.inc"
};
char gRecordGuideEntryName3_8[32] = {
#include "lobby/00825920.inc"
};
char gRecordGuideEntryName3_10[32] = {
#include "lobby/00825940.inc"
};
char gRecordGuideEntryName3_12[32] = {
#include "lobby/00825960.inc"
};
char gRecordGuideEntryName3_14[32] = {
#include "lobby/00825980.inc"
};
char gRecordGuideEntryName3_16[32] = {
#include "lobby/008259A0.inc"
};
char gRecordGuideEntryName3_18[32] = {
#include "lobby/008259C0.inc"
};
char gRecordGuideEntryName3_20[32] = {
#include "lobby/008259E0.inc"
};
char gRecordGuideEntryName3_22[32] = {
#include "lobby/00825A00.inc"
};
/* the guide's category names */
char gRecordGuideCatName0[64] = {
#include "lobby/00825A20.inc"
};
char gRecordGuideCatName1[64] = {
#include "lobby/00825A60.inc"
};
char gRecordGuideCatName2[64] = {
#include "lobby/00825AA0.inc"
};
char gRecordGuideCatName3[48] = {
#include "lobby/00825AE0.inc"
};
