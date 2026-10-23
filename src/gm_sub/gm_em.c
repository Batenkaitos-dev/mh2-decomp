/*
 * The monsters in the quest overlay (gm_sub): each monster's task
 * (GmEm_Task: set-up, update, finish, free), the set-up and update every
 * kind shares (GmEm_Setup, GmEm_ResetState, GmEm_Update) and the dispatch to
 * the families' own Start, Init and Update functions by kind (Em.type,
 * EM_KIND_*); the sizes (GmEm_ResetSize, GmEm_ApplySize); the sounds of the
 * shared motions (GmEm_MotionSounds); and what Em_DrawParts asks before it
 * draws a mesh set: the texture scrolls of some kinds (GmEm_SetMeshSetUv)
 * and each set's materials (GmEm_PrepareMeshSet: alpha, break states,
 * colours, textures).
 *
 * The rest of the file is its .data: the per-kind monster tables the game
 * overlay's monster code reads (the families' callback tables and data),
 * generated from the game as u32 words with pointers written as C.
 */
#include "common.h"
#include "sprite.h"
#include "quest.h"
#include "gm_em.h"
#include "area_anim.h"
extern u16 gFrameTick; /* sub_main: the frame counter */
#include "scene.h"
#include "m2c_macros.h"
#include "em.h"
#include "stageobj.h"
#include "player.h"
#include "area.h"
#include "render_state.h"
#include "em_anteka.h"
#include "em_apceros.h"
#include "em_blangonga.h"
#include "em_bulldrome.h"
#include "em_cephadrome.h"
#include "em_chameleos.h"
#include "em_diablos.h"
#include "em_drome.h"
#include "em_fatalis.h"
#include "em_felyne_cart.h"
#include "em_garuga.h"
#include "em_gravios.h"
#include "em_hermitaur.h"
#include "em_khezu.h"
#include "em_kind1d.h"
#include "em_kind38.h"
#include "em_kirin.h"
#include "em_kushala.h"
#include "em_kutku.h"
#include "em_laoshan.h"
#include "em_melynx.h"
#include "em_mosswine.h"
#include "em_plesioth.h"
#include "em_rathalos.h"
#include "em_remobra.h"
#include "em_shakalaka.h"
#include "em_shengaoren.h"
#include "em_trader.h"
#include "em_velociprey.h"
#include "em_vespoid.h"
#include "em_yamatsukami.h"
#include "save.h"
#include "sound.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "game_util.h"
#include "gm_load.h"
#include "monster_effects.h"
#include "vec_math.h"
#include "em_script.h"
#include "equip_preview.h"
#include "qsync.h"

/* A texture coordinate matrix (4 x 4 f32): Mtx_Identity fills it, the
 * scroll functions set its offsets, and Gs_SetState(RS_UV_MTX) hands it to
 * the VU1 programs, which map a mesh's texture coordinates through rows 0, 1
 * and 3. Only the offsets are named: nothing here sets the rest. */
typedef struct UvMtx {
    f32 rows[12];               /* 0x00: rows 0 to 2 */
    f32 uOffset;                /* 0x30: row 3: the U offset */
    f32 vOffset;                /* 0x34: the V offset */
    f32 row3zw[2];              /* 0x38 */
} UvMtx;

struct DrawSlot;
struct TriHdr;
extern s16 gEmCombatInit[];
s32 Player_UpdateMotion(Player *);
extern void **gEmKindTables[];
extern s32 *gEmRateTbl[];
extern s32 *gEmDecay7ECRates[];
extern u32 gGmEmCallbackTbl[76];
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: that keeps the game's masking instruction */


/* .bss: kind 29's base hp row, filled at run time (gEmBaseHp's entry for
 * kind 29 points to it). */
u8 gEmBaseHp_29[112];

/* Plays the monster's sounds on frames of its current motion
 * (Em_PlaySeOnFrame(em, frame, sound)), some picked at random with
 * Sound_RandomSe; the sounds are the monster bank's. work->fxMotion keeps
 * the motion id switched on. Kinds 32, Mosswine, Bullfango and Bulldrome
 * differ in three motions. The case blocks follow the order of the game's
 * code, and each block's labels the reverse of the order it compares them. */
void GmEm_MotionSounds(Em *em, EmWork *work)
{
    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, 1):
        Em_PlaySeOnFrame(em, 16, Sound_RandomSe(0, 2, 1, 2));
        Em_PlaySeOnFrame(em, 76, Sound_RandomSe(1, 2, 2, 2));
        break;
    case EM_MOTION_ID(0, 2):
        switch (em->type) {
        case 32:
            Em_PlaySeOnFrame(em, 12, Sound_RandomSe(0, 2, 1, 2));
            Em_PlaySeOnFrame(em, 42, Sound_RandomSe(1, 2, 2, 2));
            Em_PlaySeOnFrame(em, 6, 19);
            Em_PlaySeOnFrame(em, 16, 18);
            Em_PlaySeOnFrame(em, 36, 19);
            break;
        case EM_KIND_MOSSWINE:
        case EM_KIND_BULLFANGO:
            Em_PlaySeOnFrame(em, 12, Sound_RandomSe(0, 2, 1, 2));
            Em_PlaySeOnFrame(em, 42, Sound_RandomSe(1, 2, 2, 2));
            Em_PlaySeOnFrame(em, 6, 19);
            Em_PlaySeOnFrame(em, 16, 18);
            Em_PlaySeOnFrame(em, 36, 19);
            break;
        case EM_KIND_BULLDROME:
            Em_PlaySeOnFrame(em, 12, Sound_RandomSe(0, 1, 1, 1));
            Em_PlaySeOnFrame(em, 42, Sound_RandomSe(1, 1, 2, 1));
            Em_PlaySeOnFrame(em, 2, 19);
            Em_PlaySeOnFrame(em, 16, 18);
            Em_PlaySeOnFrame(em, 36, 19);
            Em_PlaySeOnFrame(em, 42, 18);
            break;
        }
        break;
    case EM_MOTION_ID(0, 3):
        Em_PlaySeOnFrame(em, 12, Sound_RandomSe(0, 2, 1, 2));
        Em_PlaySeOnFrame(em, 38, Sound_RandomSe(1, 2, 2, 2));
        Em_PlaySeOnFrame(em, 6, 18);
        Em_PlaySeOnFrame(em, 30, 19);
        Em_PlaySeOnFrame(em, 44, 19);
        break;
    case EM_MOTION_ID(0, 4):
    case EM_MOTION_ID(0, 5):
        Em_PlaySeOnFrame(em, 6, Sound_RandomSe(0, 2, 1, 2));
        Em_PlaySeOnFrame(em, 70, Sound_RandomSe(1, 2, 2, 2));
        Em_PlaySeOnFrame(em, 22, 19);
        Em_PlaySeOnFrame(em, 56, 18);
        Em_PlaySeOnFrame(em, 100, 19);
        break;
    case EM_MOTION_ID(0, 6):
        Em_PlaySeOnFrame(em, 40, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 60, Sound_RandomSe(1, 4, 2, 4));
        Em_PlaySeOnFrame(em, 80, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 100, Sound_RandomSe(1, 4, 2, 4));
        Em_PlaySeOnFrame(em, 154, 15);
        Em_PlaySeOnFrame(em, 20, 18);
        Em_PlaySeOnFrame(em, 40, 18);
        Em_PlaySeOnFrame(em, 164, 18);
        break;
    case EM_MOTION_ID(0, 7):
        Em_PlaySeOnFrame(em, 38, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 58, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 92, Sound_RandomSe(4, 4, 5, 4));
        Em_PlaySeOnFrame(em, 114, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 164, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 180, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 210, Sound_RandomSe(4, 4, 5, 4));
        Em_PlaySeOnFrame(em, 232, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 24, 18);
        Em_PlaySeOnFrame(em, 40, 18);
        Em_PlaySeOnFrame(em, 130, 18);
        Em_PlaySeOnFrame(em, 164, 18);
        Em_PlaySeOnFrame(em, 246, 18);
        break;
    case EM_MOTION_ID(0, 8):
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 36, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 4, 19);
        Em_PlaySeOnFrame(em, 42, 19);
        Em_PlaySeOnFrame(em, 52, 18);
        break;
    case EM_MOTION_ID(0, 9):
        Em_PlaySeOnFrame(em, 6, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 70, Sound_RandomSe(0, 2, 1, 2));
        Em_PlaySeOnFrame(em, 38, 19);
        Em_PlaySeOnFrame(em, 76, 19);
        break;
    case EM_MOTION_ID(0, 10):
        Em_PlaySeOnFrame(em, 36, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 58, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 128, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 150, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 22, 18);
        Em_PlaySeOnFrame(em, 116, 19);
        break;
    case EM_MOTION_ID(0, 11):
        Em_PlaySeOnFrame(em, 22, 15);
        Em_PlaySeOnFrame(em, 10, 18);
        Em_PlaySeOnFrame(em, 82, 18);
        break;
    case EM_MOTION_ID(0, 12):
        Em_PlaySeOnFrame(em, 4, 14);
        Em_PlaySeOnFrame(em, 68, 15);
        Em_PlaySeOnFrame(em, 12, 18);
        Em_PlaySeOnFrame(em, 70, 19);
        Em_PlaySeOnFrame(em, 94, 19);
        break;
    case EM_MOTION_ID(0, 13):
        Em_PlaySeOnFrame(em, 24, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 40, 18);
        break;
    case EM_MOTION_ID(0, 14):
        Em_PlaySeOnFrame(em, 68, 13);
        Em_PlaySeOnFrame(em, 36, 20);
        Em_PlaySeOnFrame(em, 74, 16);
        break;
    case EM_MOTION_ID(0, 15):
        switch (em->type) {
        case 32:
            Em_PlaySeOnFrame(em, 20, 8);
            Em_PlaySeOnFrame(em, 24, 19);
            Em_PlaySeOnFrame(em, 36, 19);
            break;
        case EM_KIND_MOSSWINE:
        case EM_KIND_BULLFANGO:
            Em_PlaySeOnFrame(em, 10, 14);
            Em_PlaySeOnFrame(em, 24, 19);
            Em_PlaySeOnFrame(em, 36, 19);
            break;
        case EM_KIND_BULLDROME:
            Em_PlaySeOnFrame(em, 34, Sound_RandomSe(3, 2, 4, 2));
            Em_PlaySeOnFrame(em, 10, 14);
            Em_PlaySeOnFrame(em, 32, 25);
            Em_PlaySeOnFrame(em, 36, 19);
            break;
        }
        break;
    case EM_MOTION_ID(0, 16):
        Em_PlaySeOnFrame(em, 20, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 8, 19);
        Em_PlaySeOnFrame(em, 14, 19);
        Em_PlaySeOnFrame(em, 12, 13);
        Em_PlaySeOnFrame(em, 98, 18);
        break;
    case EM_MOTION_ID(0, 17):
        Em_PlaySeOnFrame(em, 32, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 70, 15);
        Em_PlaySeOnFrame(em, 200, Sound_RandomSe(4, 4, 5, 4));
        Em_PlaySeOnFrame(em, 28, 20);
        Em_PlaySeOnFrame(em, 46, 19);
        break;
    case EM_MOTION_ID(0, 18):
    case EM_MOTION_ID(0, 19):
        Em_PlaySeOnFrame(em, 14, 22);
        Em_PlaySeOnFrame(em, 100, Sound_RandomSe(1, 4, 2, 4));
        Em_PlaySeOnFrame(em, 116, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 64, 13);
        Em_PlaySeOnFrame(em, 134, 14);
        Em_PlaySeOnFrame(em, 230, 15);
        Em_PlaySeOnFrame(em, 50, 25);
        Em_PlaySeOnFrame(em, 54, 20);
        Em_PlaySeOnFrame(em, 162, 25);
        Em_PlaySeOnFrame(em, 168, 20);
        Em_PlaySeOnFrame(em, 284, 19);
        Em_PlaySeOnFrame(em, 318, 18);
        break;
    case EM_MOTION_ID(0, 21):
        Em_PlaySeOnFrame(em, 30, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 104, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 20, 18);
        Em_PlaySeOnFrame(em, 30, 18);
        Em_PlaySeOnFrame(em, 50, 18);
        Em_PlaySeOnFrame(em, 72, 18);
        Em_PlaySeOnFrame(em, 92, 18);
        Em_PlaySeOnFrame(em, 104, 18);
        break;
    case EM_MOTION_ID(0, 22):
        Em_PlaySeOnFrame(em, 28, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 82, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 8, 16);
        Em_PlaySeOnFrame(em, 6, 21);
        Em_PlaySeOnFrame(em, 54, 16);
        Em_PlaySeOnFrame(em, 52, 21);
        Em_PlaySeOnFrame(em, 84, 16);
        Em_PlaySeOnFrame(em, 82, 21);
        Em_PlaySeOnFrame(em, 114, 16);
        Em_PlaySeOnFrame(em, 112, 21);
        Em_PlaySeOnFrame(em, 140, 16);
        Em_PlaySeOnFrame(em, 170, 16);
        Em_PlaySeOnFrame(em, 200, 24);
        break;
    case EM_MOTION_ID(0, 23):
        Em_PlaySeOnFrame(em, 4, 23);
        Em_PlaySeOnFrame(em, 8, 24);
        Em_PlaySeOnFrame(em, 12, 22);
        Em_PlaySeOnFrame(em, 44, 20);
        Em_PlaySeOnFrame(em, 52, 20);
        Em_PlaySeOnFrame(em, 86, 19);
        Em_PlaySeOnFrame(em, 100, 19);
        Em_PlaySeOnFrame(em, 102, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 128, 18);
        Em_PlaySeOnFrame(em, 134, 19);
        Em_PlaySeOnFrame(em, 148, 19);
        Em_PlaySeOnFrame(em, 156, 19);
        break;
    case EM_MOTION_ID(0, 20):
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(7, 4, 8, 4));
        Em_PlaySeOnFrame(em, 26, 21);
        Em_PlaySeOnFrame(em, 60, 18);
        break;
    case EM_MOTION_ID(0, 60):
    case EM_MOTION_ID(0, 61):
        Em_PlaySeOnFrame(em, 12, Sound_RandomSe(7, 4, 8, 4));
        Em_PlaySeOnFrame(em, 96, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 38, 20);
        Em_PlaySeOnFrame(em, 42, 20);
        Em_PlaySeOnFrame(em, 84, 18);
        Em_PlaySeOnFrame(em, 108, 18);
        break;
    case EM_MOTION_ID(0, 62):
    case EM_MOTION_ID(0, 67):
        Em_PlaySeOnFrame(em, 12, Sound_RandomSe(9, 4, 10, 4));
        break;
    case EM_MOTION_ID(0, 63):
        switch (em->type) {
        case 32:
            Em_PlaySeOnFrame(em, 14, 17);
            Em_PlaySeOnFrame(em, 64, 16);
            Em_PlaySeOnFrame(em, 70, Sound_RandomSe(7, 4, 8, 4));
            break;
        case EM_KIND_MOSSWINE:
        case EM_KIND_BULLFANGO:
            Em_PlaySeOnFrame(em, 14, 17);
            Em_PlaySeOnFrame(em, 64, 16);
            Em_PlaySeOnFrame(em, 70, Sound_RandomSe(7, 4, 8, 4));
            break;
        case EM_KIND_BULLDROME:
            Em_PlaySeOnFrame(em, 16, 9);
            Em_PlaySeOnFrame(em, 14, 17);
            Em_PlaySeOnFrame(em, 64, 16);
            Em_PlaySeOnFrame(em, 70, Sound_RandomSe(7, 4, 8, 4));
            break;
        }
        break;
    case EM_MOTION_ID(0, 64):
        Em_PlaySeOnFrame(em, 8, 11);
        Em_PlaySeOnFrame(em, 8, 16);
        Em_PlaySeOnFrame(em, 30, 16);
        break;
    case EM_MOTION_ID(0, 66):
    case EM_MOTION_ID(0, 73):
        Em_PlaySeOnFrame(em, 18, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 80, 16);
        Em_PlaySeOnFrame(em, 30, 18);
        Em_PlaySeOnFrame(em, 60, 19);
        Em_PlaySeOnFrame(em, 88, 18);
        Em_PlaySeOnFrame(em, 104, 18);
        Em_PlaySeOnFrame(em, 128, 13);
        break;
    case EM_MOTION_ID(0, 72):
        Em_PlaySeOnFrame(em, 18, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 22, 16);
        Em_PlaySeOnFrame(em, 30, 18);
        Em_PlaySeOnFrame(em, 60, 19);
        Em_PlaySeOnFrame(em, 88, 18);
        Em_PlaySeOnFrame(em, 104, 18);
        Em_PlaySeOnFrame(em, 128, 13);
        break;
    case EM_MOTION_ID(0, 71):
        Em_PlaySeOnFrame(em, 6, Sound_RandomSe(9, 4, 10, 4));
        Em_PlaySeOnFrame(em, 36, 17);
        Em_PlaySeOnFrame(em, 66, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 96, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 124, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 180, Sound_RandomSe(0, 4, 1, 4));
        break;
    case EM_MOTION_ID(0, 70):
        Em_PlaySeOnFrame(em, 4, 12);
        Em_PlaySeOnFrame(em, 20, 18);
        Em_PlaySeOnFrame(em, 52, 18);
        Em_PlaySeOnFrame(em, 62, 18);
        Em_PlaySeOnFrame(em, 120, 16);
        Em_PlaySeOnFrame(em, 280, Sound_RandomSe(1, 4, 2, 4));
        break;
    }
}

/* Sets up the monster for its kind: hooks the kind's handler table
 * (gGmEmCallbackTbl) and runs its first entry, takes a draw slot when the
 * monster is visible, then calls its family's Start or Init function. */
void GmEm_SetupKind(Em *em)
{
    em->handlers = (EmVt *)gGmEmCallbackTbl[em->type];
    em->handlers->init(em);
    if (em->visible != 0) {
        em->drawSlotId = DrawSlot_Alloc();
        if (em->drawSlotId != -1) {
            em->drawSlot = DrawSlot_Get(em->drawSlotId);
            em->drawSlot->owner = em;
            em->drawSlot->draw = Em_DrawParts;
        } else {
            em->drawSlot = NULL;
        }
    }
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
        EmRathalos_Start(em);
        break;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        EmFatalis_Init(em);
        break;
    case EM_KIND_KELBI:
    case EM_KIND_ANTEKA:
        EmAnteka_Start(em);
        break;
    case EM_KIND_MOSSWINE:
        EmMosswine_Start(em);
        break;
    case EM_KIND_BULLFANGO:
        EmMosswine_Start(em);
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
        EmKutku_Start(em);
        break;
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        EmLaoshan_Init(em);
        break;
    case EM_KIND_CEPHADROME:
        EmCephadrome_Start(em);
        break;
    case 9:
        EmMelynx_Start(em);
        break;
    case EM_KIND_TRADER:
        EmTrader_Init(em);
        break;
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
        EmRathalos_Start(em);
        break;
    case 12:
    case EM_KIND_POPO:
        EmApceros_Start(em);
        break;
    case EM_KIND_GENPREY:
        EmVelociprey_Start(em);
        break;
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
        EmDiablos_Start(em);
        break;
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        EmKhezu_Start(em);
        break;
    case EM_KIND_VELOCIPREY:
    case EM_KIND_GIAPREY:
        EmVelociprey_Start(em);
        break;
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
        EmGravios_Start(em);
        break;
    case EM_KIND_FELYNE_CART:
        EmFelyneCart_Init(em);
        break;
    case EM_KIND_VESPOID:
        EmVespoid_Start(em);
        break;
    case EM_KIND_GYPCEROS:
    case EM_KIND_PURPLE_GYPCEROS:
        EmKutku_Start(em);
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        EmPlesioth_Start(em);
        break;
    case EM_KIND_BASARIOS:
        EmGravios_Start(em);
        break;
    case EM_KIND_MELYNX:
        EmMelynx_Start(em);
        break;
    case EM_KIND_HORNETAUR:
        EmVespoid_Start(em);
        break;
    case EM_KIND_APCEROS:
        EmApceros_Start(em);
        break;
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
        EmDiablos_Start(em);
        break;
    case EM_KIND_VELOCIDROME:
        EmDrome_Start(em);
        break;
    case EM_KIND_GENDROME:
        EmDrome_Start(em);
        break;
    case 29:
        EmKind1D_Init(em);
        break;
    case EM_KIND_IOPREY:
        EmVelociprey_Start(em);
        break;
    case EM_KIND_IODROME:
        EmDrome_Start(em);
        break;
    case 32:
        EmMosswine_Start(em);
        break;
    case EM_KIND_KIRIN:
        EmKirin_Init(em);
        break;
    case EM_KIND_CEPHALOS:
        EmCephadrome_Start(em);
        break;
    case EM_KIND_YIAN_GARUGA:
        EmGaruga_Start(em);
        break;
    case EM_KIND_DAIMYO_HERMITAUR:
        EmHermitaur_Start(em);
        break;
    case EM_KIND_SHEN_GAOREN:
        EmShenGaoren_Init(em);
        break;
    case EM_KIND_BLANGONGA:
        EmBlangonga_Start(em);
        break;
    case EM_KIND_CONGALALA:
        EmBlangonga_Start(em);
        break;
    case EM_KIND_RAJANG:
        EmBlangonga_Start(em);
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
        EmKushala_Start(em);
        break;
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        EmKushala_Start(em);
        break;
    case 56:
        EmKind38_Start(em);
        break;
    case EM_KIND_SHAKALAKA:
        EmShakalaka_Start(em);
        break;
    case EM_KIND_YAMA_TSUKAMI:
        EmYamaTsukami_Start(em);
        break;
    case EM_KIND_CHAMELEOS:
        EmChameleos_Start(em);
        break;
    case EM_KIND_BLANGO:
        EmBlangonga_Start(em);
        break;
    case EM_KIND_CONGA:
        EmBlangonga_Start(em);
        break;
    case EM_KIND_REMOBRA:
        EmRemobra_Start(em);
        break;
    case EM_KIND_HERMITAUR:
    case EM_KIND_CEANATAUR:
        EmHermitaur_Start(em);
        break;
    case EM_KIND_SHOGUN_CEANATAUR:
        EmHermitaur_Start(em);
        break;
    case EM_KIND_BULLDROME:
        EmBulldrome_Start(em);
        break;
    case EM_KIND_YAMA_TSUKAMI_B:
        EmYamaTsukami_Start(em);
        break;
    case 0:
        break;
    }
}

/* Resets the monster to its normal size (sizePct 100): the scale from
 * bodyScale, and the three need gauges cleared and refilled to their
 * maxima. */
void GmEm_ResetSize(Em *em)
{
    f32 scale;

    em->sizePct = 100;
    scale = em->sizePct / 100.0f;
    em->scaleX = em->bodyScale * scale;
    em->scaleY = em->bodyScale * scale;
    em->scaleZ = em->bodyScale * scale;
    Em_ClearNeed2State(em);
    Em_ClearNeed1State(em);
    Em_RefillNeed3(em);
    Em_AddNeed2(em, em->need2Max);
    Em_AddNeed1(em, em->need1Max);
    Em_AddNeed3(em, em->need3Max);
}

/* Sets the monster's scale from bodyScale and sizePct. */
void GmEm_ApplySize(Em *em)
{
    f32 scale = em->sizePct / 100.0f;

    em->scaleX = em->bodyScale * scale;
    em->scaleY = em->bodyScale * scale;
    em->scaleZ = em->bodyScale * scale;
}

/* Returns value scaled by the monster's size (sizePct). */
f32 GmEm_ScaleBySize(Em *em, f32 value)
{
    return value * (em->sizePct / 100.0f);
}

/* Resets the monster's per-quest state: clears its two spawn requests,
 * restarts its script, sets its three need gauges' maxima and starting
 * values from the per-kind rows, and clears the other fields it tracks. */
void GmEm_ResetState(Em *em)
{
    s8 i;
    s32 *need1Row;
    EmRate *need3Row;
    s32 *need2Row;

    em->hostChanged = 0;
    for (i = 0; i < 2; i++) {
        em->spawns[i].pending = 0;
        em->spawns[i].pos[0] = 0.0f;
        em->spawns[i].pos[1] = 0.0f;
        em->spawns[i].pos[2] = 0.0f;
        em->spawns[i].rot[0] = 0;
        em->spawns[i].rot[1] = 0;
        em->spawns[i].rot[2] = 0;
        em->spawns[i].stateGroup = 0;
        em->spawns[i].state = 0;
        em->spawns[i].goalKind = 0;
        em->spawns[i].goalArg = 0;
        em->spawns[i].goalNo = 0;
        em->spawns[i].area = 0;
        em->spawns[i].timer = 0;
        em->spawns[i].rand = 0;
    }
    EmScript_Init(em);
    Em_RestartScript(em);
    em->tailObj = 0;
    em->floorAttr = 1;
    em->hitPhase = 0;
    em->visible = 1;
    em->motion[0].step = 2.0f;
    em->motion[1].step = 2.0f;
    em->motion[2].step = 2.0f;
    em->motion[3].step = 2.0f;
    em->bodyScale = gEmBodyScales[em->type];
    em->scaleX = gEmBodyScales[em->type];
    em->scaleY = gEmBodyScales[em->type];
    em->scaleZ = gEmBodyScales[em->type];
    if (em->type != EM_KIND_FELYNE_CART && em->type != EM_KIND_TRADER) {
        GmEm_ApplySize(em);
    }
    if (gQuest.stage == 0) {
        em->rotX = 0.0f;
        em->rotY = 0.0f;
        em->rotZ = 0.0f;
    }
    em->alpha = 1.0f;
    em->painted = 0;
    em->motionTime = 0;
    EmMine_Remove(em);
    em->pushPriority = gEmPushPriorities[em->type];
    em->bendAngle[0] = 0;
    em->bendAngle[1] = 0;
    em->bendAngle[2] = 0;
    em->bendAngle[3] = 0;
    em->motionSpeed = 1.0f;
    em->speedBonus = 0;
    em->quakeTimer = 0;
    em->hostNo = 0;
    em->fxFlags = 0;
    em->syncPulse[0] = 0;
    em->syncPulse[1] = 0;
    em->combatTimer = 0;
    em->combat = 0;
    em->requested = 0;
    em->requestId = 0;
    em->seen = 0;
    em->noticed = 0;
    em->noticeTimer[0] = 0;
    em->noticeTimer[1] = 0;
    em->noticeTimer[2] = 0;
    em->noticeTimer[3] = 0;
    need1Row = gEmRateTbl[em->type];
    need3Row = gEmRateStatus.rate[em->type];
    need2Row = gEmDecay7ECRates[em->type];
    em->need2Max = need2Row[0];
    em->need1Max = need1Row[0];
    em->need3Max = need3Row->max;
    if (em->need2Gauge <= 0) {
        em->need2Gauge = need2Row[1];
    }
    if (em->need1Gauge <= 0) {
        em->need1Gauge = need1Row[1];
    }
    if (em->need3Gauge <= 0) {
        em->need3Gauge = need3Row->start;
    }
    em->rage = 0;
    em->rageTimer = 0;
    em->rageBuildup = 0;
    em->rageStep = 0;
    em->ragePending = 0;
    em->hooked = 0;
    em->guardTimer = 0;
    em->flashed = 0;
    em->noReact = 0;
    em->routeMissCount = 0;
    em->need2State = 0;
    em->need1State = 0;
    em->need3State = 0;
    em->noAlert = 0;
    em->alertBuildup[0] = 0;
    em->alertBuildup[1] = 0;
    em->alertBuildup[2] = 0;
    em->alertBuildup[3] = 0;
    em->sensedPlayers = 0;
    em->alertedPlayers = 0;
    em->alertLimitRow = 1;
    em->triggers = 0;
    em->noticeCount[0] = 0;
    em->noticeCount[1] = 0;
    em->noticeCount[2] = 0;
    em->noticeCount[3] = 0;
    em->tables = (void **) gEmKindTables[em->type];
    em->blindTimer = 0;
    em->part8Down = 0;
    em->partBreakCount = 0;
    em->trapHit = 0;
    em->trapTimer = 0;
    em->hideShadowHold = 0;
    em->senseDelay = 0;
    em->requestPrio = 0;
    em->lowHpNeed = 0;
    em->need4 = 0;
    em->need2 = 0;
    em->need1 = 0;
    em->need3 = 0;
    em->holdTimer = 0;
    em->holdTimer2 = 0;
    em->ceilingTimer = 0;
    em->hermitTimer = 0;
    em->windTimer = 0;
    em->moveFrozen = 0;
    Em_InitStatusThresholds(em);
    em->status2Buildup = 0;
    em->status2Decay = 0;
    em->status2Count = 0;
    em->status1Buildup = 0;
    em->status1Decay = 0;
    em->status1Count = 0;
    em->status4Buildup = 0;
    em->status4Decay = 0;
    em->status4Count = 0;
    em->status5Count = 0;
    em->poisonBuildup = 0;
    em->poisonTimer = 0;
    em->poisonCount = 0;
    em->poisonDamage = 0;
    em->status = 0;
    em->statusFlags = 0;
    em->hitReactMode = 0;
    em->blind = 0;
    em->hideShadow = 0;
    em->attackFactor = Em_LevelAttackFactor(em);
    em->damageFactor = Em_LevelDamageFactor(em);
    em->fieldCollide = 1;
    em->link = 0;
    em->enterDelay = 0;
    em->inSceneArea = 0;
    em->hitzoneSet = 0;
    em->itemUseKind = 0;
    em->versusGauge = 0;
    em->rageStarted = 0;
    em->scriptCallEntryHi = 0;
    em->scriptDelay = 0;
    em->stateArmor = 0;
    em->mineSlot2 = -1;
    em->mineTimer2 = 0;
    em->mineCount2 = 1;
    Em_ApplyAreaValues(em);
}

/* The task's set-up phase: sets up the model, resets the per-quest state,
 * motions, sight, reactions, statuses, rage and kind parameters, shows all
 * 32 meshes and runs the kind's set-up. For a large monster, the first
 * player in the quest in its area (on another side in versus) becomes its
 * host. */
void GmEm_Setup(Em *em)
{
    s32 i;

    EmSlot_SetupModel(em);
    em->taskPhase++;
    em->isEm = 1;
    em->ownTask = 0;
    em->hostNo = gScene.localPlayer;
    Versus_AssignEmTeam(em);
    GmEm_ResetState(em);
    em->motionId[0] = 0;
    em->motionId[1] = 0;
    em->motionId[2] = 0;
    em->motionId[3] = 0;
    Em_InitCollMask(em);
    Em_SelectSightRow(em, 0);
    Em_LoadReactionSet(em, 0);
    Em_InitStatusThresholds(em);
    Em_InitRage(em);
    Em_InitKindParams(em);
    for (i = 0; i < 32; i++) {
        em->meshShown[i] = 1;
    }
    em->motionLayerCount = (u8)(s32)EmKind_MotionGroupCount(em->type);
    GmEm_SetupKind(em);
    Em_InitParts(em);
    if (em->large != 0) {
        for (i = 0; i < gScene.playerCount; i++) {
            if (em->area == gPlayers[i].area && gScene.inQuest[i] == 1) {
                if (Quest_IsVersus() == 1) {
                    if (gPlayers[i].side >= 3 || Side_Match(gPlayers[i].side, *(u8 *)&em->side, 1) == 1) {
                        continue;
                    }
                    if (gScene.localPlayer == i) {
                        em->remote = 0;
                    } else {
                        em->remote = 1;
                    }
                } else {
                    if (gScene.localPlayer == i) {
                        em->remote = 0;
                    } else {
                        em->remote = 1;
                    }
                }
                em->hostNo = i;
                break;
            }
        }
    } else {
        em->remote = 0;
    }
    Player_UpdateAreaPos(em);
    Em_UpdateMatrix(em);
}

/* Runs the update function of the monster's family, by kind. */
void GmEm_UpdateKind(Em *em)
{
    switch (em->type) {
    case 0:
    case EM_KIND_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_AZURE_RATHALOS:
        EmRathalos_Update(em);
        break;
    case EM_KIND_GRAVIOS:
    case EM_KIND_BASARIOS:
    case EM_KIND_BLACK_GRAVIOS:
        EmGravios_Update(em);
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_PURPLE_GYPCEROS:
        EmKutku_Update(em);
        break;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        EmFatalis_Update(em);
        break;
    case EM_KIND_KELBI:
    case EM_KIND_ANTEKA:
        EmAnteka_Update(em);
        break;
    case EM_KIND_MOSSWINE:
    case EM_KIND_BULLFANGO:
    case 32:
        EmMosswine_Update(em);
        break;
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        EmLaoshan_Update(em);
        break;
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        EmCephadrome_Update(em);
        break;
    case 9:
    case EM_KIND_MELYNX:
        EmMelynx_Update(em);
        break;
    case EM_KIND_TRADER:
        EmTrader_Update(em);
        break;
    case 12:
    case EM_KIND_APCEROS:
    case EM_KIND_POPO:
        EmApceros_Update(em);
        break;
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        EmKhezu_Update(em);
        break;
    case EM_KIND_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_WHITE_MONOBLOS:
        EmDiablos_Update(em);
        break;
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_IOPREY:
    case EM_KIND_GIAPREY:
        EmVelociprey_Update(em);
        break;
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
        EmDrome_Update(em);
        break;
    case EM_KIND_FELYNE_CART:
        EmFelyneCart_Update(em);
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        EmPlesioth_Update(em);
        break;
    case EM_KIND_VESPOID:
    case EM_KIND_HORNETAUR:
        EmVespoid_Update(em);
        break;
    case 29:
        EmKind1D_Update(em);
        break;
    case EM_KIND_KIRIN:
        EmKirin_Update(em);
        break;
    case EM_KIND_YIAN_GARUGA:
        EmGaruga_Update(em);
        break;
    case EM_KIND_SHAKALAKA:
        EmShakalaka_Update(em);
        break;
    case EM_KIND_DAIMYO_HERMITAUR:
    case EM_KIND_HERMITAUR:
    case EM_KIND_SHOGUN_CEANATAUR:
    case EM_KIND_CEANATAUR:
        EmHermitaur_Update(em);
        break;
    case EM_KIND_BLANGONGA:
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
    case EM_KIND_BLANGO:
    case EM_KIND_CONGA:
        EmBlangonga_Update(em);
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        EmKushala_Update(em);
        break;
    case EM_KIND_SHEN_GAOREN:
        EmShenGaoren_Update(em);
        break;
    case 56:
        EmKind38_Update(em);
        break;
    case EM_KIND_YAMA_TSUKAMI:
        EmYamaTsukami_Update(em);
        break;
    case EM_KIND_CHAMELEOS:
        EmChameleos_Update(em);
        break;
    case EM_KIND_REMOBRA:
        EmRemobra_Update(em);
        break;
    case EM_KIND_BULLDROME:
        EmBulldrome_Update(em);
        break;
    case EM_KIND_YAMA_TSUKAMI_B:
        EmYamaTsukami_Update(em);
        break;
    }
}

/* The monster update step shared by all kinds (timers, flags, the kind's
 * update through GmEm_UpdateKind, the render scale). The scale's float
 * registers need the speed in a local of its own, scaled in place
 * (speed *= ...): as one expression they come out rotated. */
void GmEm_Update(Em *em)
{
    s32 noticed;
    u8 set;
    u8 nearAreaObj;
    u8 result;
    u16 dayMap;
    f32 motionStep;
    f32 speed;
    EmRageRow *rageRow;
    f32 pos[3];

    set = 0;
    Em_MigrateHost(em);
    if (Game_IsOnline() == 1 && em->large != 0) {
        QSync_UpdateEmState(em);
    }
    if (em->stateGroup == EM_GROUP_VERSUS) {
        em->noHitTimer = 2;
    }
    Player_TickTimers((Player *) em);
    Player_UpdateHitStop(em);
    if (em->inSceneArea == 0) {
        if (gScene.area == em->area) {
            em->inSceneArea = 1;
            Em_GetFloorInfoEx(em, &em->posX, (struct TriHdr *)&em->floorKind, &em->floorY, &em->surfaceY);
            if (em->stance == EM_STANCE_GROUND) {
                em->posY = em->floorY;
            }
        }
    } else if (gScene.area != em->area) {
        em->inSceneArea = 0;
    }
    if (em->moveFrozen == 0) {
        em->prevPosX = em->posX;
        em->prevPosY = em->posY;
        em->prevPosZ = em->posZ;
    }
    if (gScene.area != em->area) {
        em->enterDelay = 2;
    }
    if (em->stateGroup != EM_GROUP_DEFEAT && em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_EVENT2 && em->stateGroup != EM_GROUP_VERSUS) {
        if (em->combat == 1) {
            em->senseDelay = 0;
        }
        if (em->senseDelay != 0) {
            em->senseDelay--;
        } else {
            Em_SensePlayers(em);
            if (em->noAlert != 1) {
                Em_CheckPartBuildup(em);
            }
            Em_SeePlayers(em);
            switch (em->type) {
            case EM_KIND_RATHIAN:
            case EM_KIND_GOLD_RATHIAN:
            case EM_KIND_PINK_RATHIAN:
            case EM_KIND_CEPHADROME:
            case EM_KIND_RATHALOS:
            case EM_KIND_SILVER_RATHALOS:
            case EM_KIND_AZURE_RATHALOS:
            case EM_KIND_KHEZU:
            case EM_KIND_RED_KHEZU:
            case EM_KIND_GYPCEROS:
            case EM_KIND_PURPLE_GYPCEROS:
            case EM_KIND_DAIMYO_HERMITAUR:
            case EM_KIND_BLANGONGA:
            case EM_KIND_CONGALALA:
                if (Em_ScriptFlagsFrom(em, EM_TRIGGER_AREA_OBJ) == 0 && em->remote == 0 && em->requested == 0) {
                    nearAreaObj = Em_FindNearAreaObj(em);
                } else {
                    nearAreaObj = 0;
                }
                if (em->combat == 0 || em->type == EM_KIND_CONGALALA) {
                    if (nearAreaObj == 1 && Em_ScriptFlagsFrom(em, EM_TRIGGER_AREA_OBJ) == 0 && em->noReact == 0) {
                        em->triggers |= EM_TRIGGER_AREA_OBJ;
                    }
                }
                break;
            }
            if (em->ragePending != 0 && em->stateGroup != EM_GROUP_REACT && em->noReact == 0 && Em_ScriptFlagsFrom(em, EM_TRIGGER_RAGE) == 0) {
                Em_StartRage2(em);
            }
            Em_PickRequest(em);
            noticed = (u8)Em_UpdateNoticed(em);
            if ((em->type == 9 || em->type == EM_KIND_MOSSWINE) && em->combat == 0) {
                noticed = 0;
            }
            if (em->combat == 0 && noticed != 0 && Em_ScriptFlagsFrom(em, EM_TRIGGER_NOTICED) == 0 && em->noAlert != 1 && em->noReact == 0) {
                em->scriptPending = 1;
                em->triggers |= EM_TRIGGER_NOTICED;
                em->triggers &= ~EM_TRIGGER_AREA_OBJ;
            }
            if (em->combat == 1 && noticed != 0) {
                em->combatTimer = gEmCombatInit[em->type];
            }
            if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                if (em->targetNo != -1) {
                    Em_GetTargetPos(em, em->targetNo, pos);
                    Vec_Set(em->goal, pos[0], pos[1], pos[2]);
                }
            }
        }
    }
    if (*(s8 *)&em->syncPulse[0] > 0) {
        (*(s8 *)&em->syncPulse[0])--;
    }
    if (*(s8 *)&em->syncPulse[1] > 0) {
        (*(s8 *)&em->syncPulse[1])--;
    }
    if (*(s8 *)&em->holdTimer > 0) {
        (*(s8 *)&em->holdTimer)--;
    }
    if (*(s8 *)&em->holdTimer2 > 0) {
        (*(s8 *)&em->holdTimer2)--;
    }
    if (*(s8 *)&em->ceilingTimer > 0) {
        (*(s8 *)&em->ceilingTimer)--;
    }
    if (*(s8 *)&em->hermitTimer > 0) {
        (*(s8 *)&em->hermitTimer)--;
    }
    if (em->windTimer > 0) {
        em->windTimer--;
    }
    if (*(s8 *)&em->guardTimer != 0) {
        if (--*(s8 *)&em->guardTimer <= 0) {
            em->guardTimer = 0;
        }
    }
    if (em->rage != 0) {
        rageRow = gEmRageTbl[em->type];
        if (rageRow != NULL) {
            em->motionSpeed = rageRow->motionSpeed;
        }
        if (--em->rageTimer <= 0) {
            Em_EndRage(em);
        }
    }
    dayMap = Map_DayId(gScene.map);
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_YIAN_GARUGA:
        if (dayMap != MAP_BATTLEFIELD && dayMap != MAP_GREAT_ARENA && dayMap != MAP_ARENA) {
            result = Em_InBlockArea2(em);
            if (em->combat == 0 && result == 1 && Em_ScriptFlagsFrom(em, EM_TRIGGER_BLOCK_AREA) == 0 && em->noReact == 0) {
                em->scriptPending = 1;
                em->triggers |= EM_TRIGGER_BLOCK_AREA;
            }
        }
        break;
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_BASARIOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        break;
    }
    if (em->areaTimer1 > 0) {
        em->areaTimer1--;
    }
    if (em->combat == 1) {
        if (em->areaTimer2 > 0) {
            em->areaTimer2--;
        }
    }
    if (em->stateGroup != EM_GROUP_DEFEAT) {
        if (em->blindTimer != 0) {
            if (--em->blindTimer <= 0) {
                em->blindTimer = 0;
                if (em->status != 2 && em->status != 1) {
                    em->blind = 0;
                    if (em->remote == 0 && em->stateGroup != EM_GROUP_REACT && em->stance == EM_STANCE_GROUND) {
                        Em_ForceState19(em);
                    }
                }
            }
        } else if (em->blind != 0) {
            em->blind--;
        }
        Em_UpdateStatus(em);
    }
    if (em->wallHitDirs != 0) {
        if (em->stateGroup == EM_GROUP_FLY && em->velY >= 0.0f && em->velZ > 0.0f) {
            em->posY += 30.0f;
        }
        em->wallTouch = 2;
    } else {
        em->wallTouch = 0;
    }
    if (*(u8 *)&em->side != 0) {
        if (em->rageStarted != 0) {
            em->itemUseKind = 136;
        } else {
            result = Side_HornItemPlaying(*(u8 *)&em->side);
            if (result == 0) {
                em->itemUseKind = 0;
            } else if (!(em->itemUseKind & 0x80)) {
                em->itemUseKind = result | 0x80;
            }
        }
    }
    GmEm_UpdateKind(em);
    EmMine_UpdateTimed(em);
    em->rotX = *(u16 *)&em->rotX;
    em->rotY = *(u16 *)&em->rotY;
    em->rotZ = *(u16 *)&em->rotZ;
    Mtx_FromAnglesYXZ(&em->rotX, (f32 *) em->rotMtx);
    Em_UpdateBendTarget(em);
    Em_UpdateReactions(em);
    Em_DecayNotice(em);
    speed = em->motionSpeed;
    speed *= (f32)(em->speedBonus + 100) / 100.0f;
    motionStep = 2.0f * speed;
    em->motion[0].step = motionStep;
    em->motion[1].step = motionStep;
    em->motion[2].step = motionStep;
    em->motion[3].step = motionStep;
    if (em->moveFrozen != 0) {
        if (em->flags & EM_FLAG_NO_ROOT_MOTION) {
            set = 1;
        } else {
            Player_SetFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
    }
    Player_UpdateMotion((Player *) em);
    if (em->moveFrozen != 0 && !set) {
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
    }
    Em_CollideField(em, 0);
    Em_GetFloorInfoEx(em, &em->posX, (struct TriHdr *)&em->floorKind, &em->floorY, &em->surfaceY);
    if (*(s8 *)&em->hideShadowHold == 0) {
        if (em->shadowOff != 0 || em->stance == EM_STANCE_UNDER) {
            em->hideShadow = 1;
        } else {
            em->hideShadow = 0;
        }
    } else if (--*(s8 *)&em->hideShadowHold <= 0) {
        em->hideShadowHold = 0;
    }
    if (em->stance != EM_STANCE_AIR && em->stance != EM_STANCE_UNDER) {
        if (em->surfaceKind != 0) {
            if (em->type != EM_KIND_CEPHADROME && em->type != EM_KIND_DIABLOS && em->type != EM_KIND_MONOBLOS && (u32)(em->type - EM_KIND_BLACK_DIABLOS) > 1 && em->type != EM_KIND_CEPHALOS) {
                em->floorY = em->surfaceY;
            }
        }
        if (em->remote == 0) {
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
        } else if (gScene.area == em->area) {
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
        }
    }
    Em_UpdateLightColors(em);
}

/* The task's finish phase: moves on to the free phase, hides the monster
 * and removes its timed mines. */
void GmEm_Finish(Em *em)
{
    em->taskPhase++;
    em->visible = 0;
    EmMine_RemoveTimed(em);
}

/* The task's last phase: reports the monster hunted (QuestEm_Hunted),
 * removes its mines and frees its slot. */
void GmEm_Free(Em *em)
{
    QuestEm_Hunted(em);
    EmMine_Remove(em);
    EmSlot_Free(em);
}

/* Returns 0, hiding the monster's shadow, while a scene event that runs
 * only some small monsters plays: event 171 keeps kind 12 and Popo of
 * variant 0, event 164 keeps Blango; any other event keeps none. Else 1.
 * The gotos are the game's control flow. */
s32 GmEm_CanUpdate(Em *em)
{
    s32 eventId;
    u8 kind;

    if ((gScene.eventRunning != 0) && (em->large == 0) && (ScEvent_CurFlag1() == 1)) {
        eventId = ScEvent_CurId();
        switch (eventId) {
        case 171:
            kind = em->type;
            if ((kind == 12) || (kind == EM_KIND_POPO)) {
                if (em->variant != 0) {
                    goto block_10;
                }
                goto block_15;
            }
block_10:
            em->hideShadow = 1;
            return 0;
        case 164:
            if (em->type != EM_KIND_BLANGO) {
                em->hideShadow = 1;
                return 0;
            }
            goto block_15;
        default:
            em->hideShadow = 1;
            return 0;
        }
    } else {
block_15:
        return 1;
    }
}

/* The monster's task, each frame, by Em.taskPhase: set-up, update, finish
 * and free. Then it adds the monster's draw slot to an ordering table:
 * the Fatalis, Lao-Shan Lung, Shen Gaoren and Yama Tsukami kinds and an
 * invisible Chameleos go to the sorted one. Returns 1 once freed. */
s32 GmEm_Task(Em *em)
{
    switch (em->taskPhase) {
    case 0:
        GmEm_Setup(em);
        GmEm_RunMotionFx(em);
        Player_UpdateAreaPos(em);
        return 0;
    case 1:
        if (GmEm_CanUpdate(em) == 0) {
            return 0;
        }
        GmEm_Update(em);
        break;
    case 2:
        if (GmEm_CanUpdate(em) == 0) {
            return 0;
        }
        GmEm_Finish(em);
        break;
    case 3:
        GmEm_Free(em);
        return 1;
    }
    GmEm_RunMotionFx(em);
    if (em->visible != 0 && em->area == gScene.area) {
        em->drawSlot->pos[0] = em->posX;
        em->drawSlot->pos[1] = em->posY;
        em->drawSlot->pos[2] = em->posZ;
        if (em->drawSlot != NULL) {
            if (em->alpha == 1.0f) {
                if (em->type == EM_KIND_FATALIS || em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C || em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG ||
                    em->type == EM_KIND_SHEN_GAOREN || em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
                    OT_AddByDepth(gDrawOt32, em->drawSlot, 32, 1);
                } else if (em->type == EM_KIND_CHAMELEOS && EmChameleos_IsInvisible(em) == 1) {
                    OT_AddByDepth(gDrawOtSorted, em->drawSlot, 64, 0);
                } else {
                    OT_AddByDepth(gDrawOt32, em->drawSlot, 32, 0);
                }
            } else {
                if (em->type == EM_KIND_FATALIS || em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C || em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG ||
                    em->type == EM_KIND_SHEN_GAOREN || em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
                    OT_AddByDepth(gDrawOtSorted, em->drawSlot, 64, 1);
                } else {
                    OT_AddByDepth(gDrawOtSorted, em->drawSlot, 64, 0);
                }
            }
        }
    }
    Player_UpdateAreaPos(em);
    return 0;
}

/* The Khezu kinds: mesh set 2's texture scrolls along U (a 64-frame
 * cycle), except while the monster is defeated. */
void GmEm_KhezuScrollUv(Em *em, s32 meshSet)
{
    UvMtx uvMtx;

    Mtx_Identity((f32 *) &uvMtx);
    if (em->stateGroup != EM_GROUP_DEFEAT) {
        switch (meshSet) {
        case 2:
            uvMtx.uOffset = (gScene.frame & 0x3F) / 64.0f;
            break;
        }
    }
    Gs_SetState(RS_UV_MTX, (s32)&uvMtx);
}

/* The Plesioth kinds: mesh set 4's texture scrolls along V (a 64-frame
 * cycle). */
void GmEm_PlesiothScrollUv(Em *em, s32 meshSet)
{
    UvMtx uvMtx;

    Mtx_Identity((f32 *) &uvMtx);
    switch (meshSet) {
    case 4:
        uvMtx.vOffset = 1.0f - (gScene.frame & 0x3F) / 64.0f;
        break;
    }
    Gs_SetState(RS_UV_MTX, (s32)&uvMtx);
}

/* The Yama Tsukami kinds: mesh sets 1 and 4 scroll along U (a 128-frame
 * cycle), except while defeated. */
void GmEm_YamaTsukamiScrollUv(Em *em, s32 meshSet)
{
    UvMtx uvMtx;

    Mtx_Identity((f32 *) &uvMtx);
    if (em->stateGroup != EM_GROUP_DEFEAT) {
        switch (meshSet) {
        case 1:
        case 4:
            uvMtx.uOffset = (gScene.frame & 0x7F) / 128.0f;
            break;
        }
    }
    Gs_SetState(RS_UV_MTX, (s32)&uvMtx);
}

/* Chameleos: mesh sets 5 to 7 offset their texture by the invisibility
 * fade (none: 0.75 for set 7, else 0); set 7 is skipped once fxFlags bit 0
 * is set. The other sets are drawn only while it is visible. Returns 0 to
 * skip the set. */
s32 GmEm_ChameleosMeshUv(Em *em, s32 meshSet)
{
    UvMtx uvMtx;
    /* work + 0x4C is Em.invisFade, Chameleos' invisibility fade (0xFFFF: none),
     * read through a pointer to the work area as the game's code does: as
     * em->invisFade the registers come out in another order */
    u8 *work = &em->work;

    switch (meshSet) {
    default:
        return EmChameleos_IsInvisible(em) != 1;
    case 7:
        if (em->fxFlags & 1) {
            return 0;
        }
    case 5:
    case 6:
        Mtx_Identity((f32 *) &uvMtx);
        if (*(u16 *)(work + 76) != 0xFFFF) {
            uvMtx.uOffset = 1.0f - *(u16 *)(work + 76) / 32.0f;
        } else {
            switch (meshSet) {
            case 7:
                uvMtx.uOffset = 0.75f;
                break;
            case 5:
            case 6:
            default:
                uvMtx.uOffset = 0.0f;
                break;
            }
        }
        Gs_SetState(RS_UV_MTX, (s32)&uvMtx);
        return 1;
    }
}

/* Fatalis C: mesh set 0's texture offset pulses along U (a 128-frame
 * sine). */
void GmEm_FatalisCPulseUv(Em *em, s32 meshSet)
{
    UvMtx uvMtx;

    switch (meshSet) {
    case 0:
        Mtx_Identity((f32 *) &uvMtx);
        uvMtx.uOffset = 0.15f * (1.0f + Math_Sin(6.2831855f * ((gScene.frame & 0x7F) << 9) / 65536.0f));
        Gs_SetState(RS_UV_MTX, (s32)&uvMtx);
        break;
    }
}

/* Sets the texture matrix of the monster's mesh set meshSet, for the kinds
 * that move their textures (Em_DrawParts calls it per set). Returns 0 when
 * the set must not be drawn (Chameleos only), else 1. */
s32 GmEm_SetMeshSetUv(Em *em, s32 meshSet)
{
    switch (em->type) {
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        GmEm_KhezuScrollUv(em, meshSet);
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        GmEm_PlesiothScrollUv(em, meshSet);
        break;
    case EM_KIND_CHAMELEOS:
        return GmEm_ChameleosMeshUv(em, meshSet);
    case EM_KIND_YAMA_TSUKAMI:
    case EM_KIND_YAMA_TSUKAMI_B:
        GmEm_YamaTsukamiScrollUv(em, meshSet);
        break;
    case EM_KIND_FATALIS_C:
        GmEm_FatalisCPulseUv(em, meshSet);
        break;
    }
    return 1;
}

/* Hands material to the renderer as material slot `slot`
 * (RS_MATERIAL). owner, the monster, is unused. */
void GmEm_UploadMaterial(s32 owner, ModelMat *material, s32 slot)
{
    Gs_SetState(RS_MATERIAL(slot), (s32)material);
}

/* Shen Gaoren: colours the material by the flinch level of part `part`
 * (its work area's partFlinch, at Em +0x45E): dark red, pulsing red after a
 * first flinch, bright pulsing red after a second. */
void GmEm_ShenGaorenFlinchColor(Em *em, ModelMat *material, u16 part)
{
    f32 red;
    s8 flinch = *(s8 *)(part + (u32)em + 1118);

    if (flinch == 0) {
        red = 25.0f;
    } else if (flinch == 1) {
        red = 100.0f + 30.0f * (0.5f * (1.0f + Math_Sin(6.2831855f * (gScene.frame << 14) / 65536.0f)));
    } else {
        red = 220.0f + 35.0f * (0.5f * (1.0f + Math_Sin(6.2831855f * (gScene.frame << 14) / 65536.0f)));
    }
    material->rgb[0] = red / 255.0f;
    material->rgb[1] = 40.0f / 255.0f;
    material->rgb[2] = 55.0f / 255.0f;
}

extern s32 gTexHandles[];
extern s16 gGmEmFatalisBMatTex[];
/* The fields of the monster's work area (Em.work) GmEm_PrepareMeshSet reads,
 * as the family views name them: EmBlangongaWork's rageTimer and tint,
 * EmFatalisWork's rageTimer, EmWork's hunted and huntedB. One view of its
 * own, because reading them through those structs (casts of one pointer)
 * compiles the function differently from the game's code. */
typedef struct EmMeshWork {
    u8 unk00[56];
    s16 blangRageTimer;         /* 0x38: EmBlangongaWork.rageTimer (Rajang) */
    u8 unk3A[65 - 58];
    u8 blangTint[3];            /* 0x41: EmBlangongaWork.tint, the body colour (r, g, b) */
    u8 unk44[87 - 68];
    s8 fatalisRageTimer;        /* 0x57: EmFatalisWork.rageTimer */
    u8 unk58[94 - 88];
    u8 hunted;                  /* 0x5E: EmWork.hunted (the dromes) */
    u8 unk5F[110 - 95];
    u8 huntedB;                 /* 0x6E: EmWork.huntedB (the preys) */
} EmMeshWork;
s32 Em_PartBroken(Em *em, s32 arg1);
u8 EmChameleos_CanTurnInvisible(Em *em); /* unit's own declaration: the definition (game) returns s32 (0, 1 or 2); declared u8 here because GmEm_PrepareMeshSet's code masks the result to a byte, as a u8 return compiles */

/* Prepares mesh set `set` of the monster's model (sets: the model's
 * material sets) before it is drawn: every material of the set gets
 * Em.alpha, then per kind some are hidden (alpha 0) by the parts' break
 * states, partBreakCount, the rage or a blink cycle, or are recoloured or
 * retextured; each is handed to the renderer with GmEm_UploadMaterial.
 * Em_DrawParts calls it per shown set, the Bulldrome's draw for set 1.
 * Two forms are kept because they give the game's code: the Felyne case's
 * switch on animFrame lists `default:` first, which leaves its branches'
 * delay slots empty as in the game; and in Fatalis B's 32-material loop,
 * texIdx is set by an if/else and shadeDark is computed in the branch for
 * materials 1 and 14, which keeps the loop's two pointers and its counter
 * in the registers the game's code uses. */
void GmEm_PrepareMeshSet(Em *em, s32 set, ModelMatSet *sets)
{
    ModelMat *modelMats = em->model->mats;
    ModelMat *material;
    s32 i;
    s32 texIdx;
    s32 kind20Tex;
    s32 cond;
    s16 animFrame;
    EmMeshWork *work;
    f32 shade;
    f32 shadeDark;
    s32 j;
    u8 rageTex;

    switch (em->type) {
    /* The Rathian kinds: meshes hidden as parts 1, 2 and 6 break, and the
     * meshes shown only after the part-break reaction (partBreakCount). */
    case EM_KIND_RATHIAN: case EM_KIND_PINK_RATHIAN: case EM_KIND_GOLD_RATHIAN:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 0:
                    if (em->parts[1].state > 0) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[2].state > 0) material->alpha = 0.0f;
                    break;
                case 5:
                    material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 0:
                case 1:
                case 2:
                    break;
                case 3:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 0:
                case 1:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[2].state == 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[1].state == 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 1:
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Fatalis: the head's meshes by part 5's break stage (set 1, and their
     * broken counterparts in set 4); set 3's textures by parts[7].state. */
    case EM_KIND_FATALIS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 0:
                case 1:
                    if (em->parts[5].state >= 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state > 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 1:
                case 5:
                    if (em->parts[7].state >= 10) texIdx = 2; else texIdx = 1;
                    material->tex = gTexHandles[em->modelSlot * 20 + 163 + texIdx];
                    break;
                case 3:
                case 8:
                    if (em->parts[0].state > 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 1:
                    if (em->parts[5].state < 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state >= 2 || em->parts[5].state <= 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state < 2) material->alpha = 0.0f;
                    break;
                case 0:
                    if (em->parts[0].state <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Yian Kut-Ku kinds: whole and broken meshes by part 6 and the
     * part-break reaction. */
    case EM_KIND_YIAN_KUT_KU: case EM_KIND_BLUE_YIAN_KUT_KU:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 5:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 6:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 0:
                    break;
                case 1:
                    material->alpha = 0.0f;
                    break;
                case 2:
                case 3:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 0:
                    material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Lao-Shan Lung kinds: whole meshes (sets 2 and 4) and their broken
     * counterparts (set 5) by parts 0, 4, 5 and 6. */
    case EM_KIND_LAO_SHAN_LUNG: case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 2:
                switch (i) {
                case 0:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[0].state >= 2) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[0].state > 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->parts[4].state >= 3) material->alpha = 0.0f;
                    break;
                }
                break;
            case 5:
                switch (i) {
                case 0:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[5].state < 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[0].state < 2) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[0].state <= 0) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[4].state < 3) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Cephadrome kinds: Cephadrome tints every material sand-brown;
     * meshes by part 0 and the part-break reaction. */
    case EM_KIND_CEPHADROME: case EM_KIND_CEPHALOS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            if (em->type == EM_KIND_CEPHADROME) {
                material->rgb[0] = 101.0f / 255.0f;
                material->rgb[1] = 96.0f / 255.0f;
                material->rgb[2] = 65.0f / 255.0f;
            }
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 2:
                    material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 5:
                case 6:
                    if (em->type == EM_KIND_CEPHADROME && em->parts[0].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 2:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->parts[0].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Felyne kinds (9, the cart, Melynx): a 10-step cycle picks the face
     * mesh (a blink). Em_DrawParts draws these kinds with their own function. */
    case 9: case EM_KIND_FELYNE_CART: case EM_KIND_MELYNX:
        animFrame = (gFrameTick >> 1) % 10;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
            case 1:
                switch (animFrame) {
                default:
                    switch (i) {
                    case 2:
                    case 3:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 8:
                    switch (i) {
                    case 1:
                    case 3:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 9:
                    switch (i) {
                    case 1:
                    case 2:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Rathalos kinds: as the Rathian kinds, by parts 1, 2 and 6 and the
     * part-break reaction. */
    case EM_KIND_RATHALOS: case EM_KIND_SILVER_RATHALOS: case EM_KIND_AZURE_RATHALOS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 3:
                    if (em->parts[1].state > 0) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[2].state > 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 0:
                case 4:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 1:
                    material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->parts[1].state == 0) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[2].state == 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 4:
                    material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 1:
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The raptors and their leaders: the eyes blink on a 98-frame cycle (one
     * of materials 0-3 shown), closed once hunted; material 9 only in two motions. */
    case EM_KIND_GENPREY: case EM_KIND_VELOCIPREY: case EM_KIND_VELOCIDROME: case EM_KIND_GENDROME: case EM_KIND_IOPREY: case EM_KIND_IODROME: case EM_KIND_GIAPREY:
        work = (EmMeshWork *)&em->work;
        switch (em->type) {
        case 27:
        case 28:
        case 31:
            cond = work->hunted;
            break;
        default:
            cond = work->huntedB;
            break;
        }
        if (cond != 0) animFrame = 0; else animFrame = gScene.frame % 98;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            if (em->type == EM_KIND_VELOCIPREY || em->type == EM_KIND_GIAPREY || em->type == EM_KIND_GENPREY || em->type == EM_KIND_IOPREY) {
                switch (i) {
                case 5:
                    material->alpha = 0.0f;
                    break;
                }
            } else {
                switch (i) {
                case 4:
                    material->alpha = 0.0f;
                    break;
                }
            }
            switch (animFrame) {
            case 0:
            case 1:
            case 2:
            case 3:
                switch (i) {
                case 0:
                case 1:
                case 3:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
            case 5:
                switch (i) {
                case 0:
                case 2:
                case 3:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 6:
            case 7:
                switch (i) {
                case 0:
                case 1:
                case 2:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            default:
                switch (i) {
                case 1:
                case 2:
                case 3:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            }
            switch (i) {
            case 9:
                if (em->motionId[0] != EM_MOTION_ID(0, 40) && em->motionId[0] != EM_MOTION_ID(0, 45)) material->alpha = 0.0f;
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Diablos kinds: meshes by part 6 and the part-break reaction. */
    case EM_KIND_DIABLOS: case EM_KIND_BLACK_DIABLOS:
        if (em->motionId[0] == EM_MOTION_ID(0, 113)) EmMotion_ReachedFrame(em, 0, 10.0f);
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 3:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[6].state > 0) material->alpha = 0.0f;
                    break;
                case 6:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 4:
                    material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 7:
                    if (em->parts[6].state <= 0) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Khezu kinds: meshes by parts 0 and 6 and the part-break reaction. */
    case EM_KIND_KHEZU: case EM_KIND_RED_KHEZU:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 1:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 3:
                    material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->parts[0].state < 2) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Gravios kinds: meshes by part 6 and the part-break reaction. */
    case EM_KIND_GRAVIOS: case EM_KIND_BLACK_GRAVIOS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 3:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 4:
                    if (em->parts[6].state > 1) material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->parts[6].state > 0) material->alpha = 0.0f;
                    break;
                case 6:
                    material->alpha = 0.0f;
                    break;
                case 7:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 0:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[6].state == 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Vespoid and Hornetaur: the wings, one of materials 2-4 on a 9-frame
     * cycle; materials 1 and 5-7 flicker in the flying motions. */
    case EM_KIND_VESPOID: case EM_KIND_HORNETAUR:
        animFrame = gScene.frame % 9;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (i) {
            case 0:
                break;
            case 2:
            case 3:
            case 4:
                switch (animFrame) {
                case 0:
                case 1:
                    switch (i) {
                    case 3:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 3:
                case 4:
                case 5:
                case 6:
                    switch (i) {
                    case 2:
                    case 3:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                default:
                    switch (i) {
                    case 2:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                }
                break;
            case 1:
            case 5:
            case 6:
            case 7:
                switch (em->motionId[0]) {
                case 1001:
                case 1002:
                case 1003:
                case 1006:
                case 1007:
                case 1010:
                case 1011:
                case 1012:
                    if (em->motionId[0] == EM_MOTION_ID(0, 12) && EmMotion_ReachedFrame(em, 0, 72.0f) == 0) {
                        switch (i) {
                        case 6:
                        case 7:
                            material->alpha = 0.0f;
                            break;
                        }
                    } else if (gScene.frame & 1) {
                        switch (i) {
                        case 7:
                            material->alpha = 0.0f;
                            break;
                        }
                    } else {
                        switch (i) {
                        case 1:
                        case 5:
                        case 6:
                            material->alpha = 0.0f;
                            break;
                        }
                    }
                    break;
                default:
                    switch (i) {
                    case 6:
                    case 7:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Plesioth kinds: meshes by part 0 and the part-break reaction. */
    case EM_KIND_PLESIOTH: case EM_KIND_GREEN_PLESIOTH:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 2:
                    material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 5:
                case 6:
                    if (em->parts[0].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 2:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 5:
                switch (i) {
                case 0:
                case 1:
                    if (em->parts[0].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Basarios: meshes by part 6 and the part-break reaction. */
    case EM_KIND_BASARIOS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 2:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 4:
                    if (em->parts[6].state > 0) material->alpha = 0.0f;
                    break;
                case 5:
                    material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 0:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[6].state <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Shakalaka: the mask meshes (picked by variant) hidden from frame 222
     * of its death motion. */
    case EM_KIND_SHAKALAKA:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 0:
                case 2:
                case 3:
                    if (em->motionId[0] == EM_MOTION_ID(0, 68) && EmMotion_ReachedFrame(em, 0, 222.0f) != 0) material->alpha = 0.0f;
                    if (em->variant == 3) {
                        if (i == 2) material->alpha = 0.0f;
                    } else {
                        if (i == 3) material->alpha = 0.0f;
                    }
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Monoblos kinds: material 0 tinted red by the rage fade (Em.rageFade,
     * counted to 60 while enraged); meshes by part 6 and the part-break reaction. */
    case EM_KIND_MONOBLOS: case EM_KIND_WHITE_MONOBLOS:
        shade = 1.0f - 0.172549009f * ((s8) em->rageFade / 60.0f);
        shadeDark = 1.0f - 205.0f / 255.0f * ((s8) em->rageFade / 60.0f);
        if (em->motionId[0] == EM_MOTION_ID(0, 113)) EmMotion_ReachedFrame(em, 0, 10.0f);
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 0:
                    material->rgb[0] = shade;
                    material->rgb[1] = shadeDark;
                    material->rgb[2] = shadeDark;
                    break;
                case 4:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 7:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 4:
                    material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Fatalis B: set 0 first recolours its 32 materials (dimmed, then lit
     * again) and switches them to the rage textures (+8) as the rage comes and
     * goes; then the meshes as Fatalis'. */
    case EM_KIND_FATALIS_B:
        cond = em->rage;
        work = (EmMeshWork *)&em->work;
        if (cond != 0) {
            if (em->rageStep == 0) {
                if (work->fatalisRageTimer < 30) {
                    rageTex = 0;
                } else {
                    rageTex = 1;
                }
            } else {
                rageTex = 1;
            }
        } else if (work->fatalisRageTimer == -1) {
            rageTex = 0;
        } else if (work->fatalisRageTimer < 30) {
            rageTex = 1;
        } else {
            rageTex = 0;
        }
        if (set == 0) {
            if (work->fatalisRageTimer > 0) {
                if (cond != 0) {
                    if (work->fatalisRageTimer < 30) {
                        shade = 1.1f - 0.9f * (work->fatalisRageTimer / 30.0f);
                    } else {
                        shade = 0.1f + 0.9f * ((work->fatalisRageTimer - 30) / 10.0f);
                    }
                } else {
                    if (work->fatalisRageTimer < 30) {
                        shade = 1.1f - 0.9f * (work->fatalisRageTimer / 30.0f);
                    } else {
                        shade = 0.1f + 0.9f * ((work->fatalisRageTimer - 30) / 20.0f);
                    }
                }
            } else {
                shade = 1.0f;
            }
            for (j = 0; j < 32; j++) {
                texIdx = gGmEmFatalisBMatTex[j];
                if (rageTex && texIdx < 8) texIdx += 8;
                if (j == 1 || j == 14) {
                    shadeDark = 0.7f * shade;
                    modelMats[j].rgb[0] = shadeDark;
                    modelMats[j].rgb[1] = shadeDark;
                    modelMats[j].rgb[2] = shadeDark;
                } else {
                    modelMats[j].rgb[0] = shade;
                    modelMats[j].rgb[1] = shade;
                    modelMats[j].rgb[2] = shade;
                }
                modelMats[j].tex = gTexHandles[em->modelSlot * 20 + 163 + texIdx];
            }
        }
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 0:
                case 1:
                    if (em->parts[5].state >= 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state > 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 1:
                case 5:
                    if (em->parts[7].state >= 10) texIdx = 2; else texIdx = 1;
                    if (rageTex) texIdx += 8;
                    material->tex = gTexHandles[em->modelSlot * 20 + 163 + texIdx];
                    break;
                case 3:
                case 8:
                    if (em->parts[0].state > 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 1:
                    if (em->parts[5].state < 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state >= 2 || em->parts[5].state <= 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state < 2) material->alpha = 0.0f;
                    break;
                case 0:
                    if (em->parts[0].state <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Fatalis C: as Fatalis, and set 6 (the rage glow) lit by the rage timer. */
    case EM_KIND_FATALIS_C:
        work = (EmMeshWork *)&em->work;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 0:
                case 1:
                    if (em->parts[5].state >= 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state > 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 1:
                case 5:
                    if (em->parts[7].state >= 10) texIdx = 2; else texIdx = 1;
                    material->tex = gTexHandles[em->modelSlot * 20 + 163 + texIdx];
                    break;
                case 3:
                case 8:
                    if (em->parts[0].state > 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 1:
                    if (em->parts[5].state < 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state >= 2 || em->parts[5].state <= 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state < 2) material->alpha = 0.0f;
                    break;
                case 0:
                    if (em->parts[0].state <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 6:
                if (em->rage != 0) {
                    switch (em->rageStep) {
                    case 0:
                        if (work->fatalisRageTimer < 0) {
                            shade = 0.0f;
                        } else {
                            shade = work->fatalisRageTimer / 60.0f;
                        }
                        break;
                    case 1:
                    default:
                        shade = 1.0f;
                        break;
                    }
                } else {
                    switch (em->rageStep) {
                    case 0:
                        if (work->fatalisRageTimer < 0) {
                            shade = 0.0f;
                        } else {
                            shade = 1.0f - work->fatalisRageTimer / 60.0f;
                        }
                        break;
                    case 1:
                    default:
                        shade = 1.0f;
                        break;
                    }
                }
                material->rgb[0] = shade;
                material->rgb[1] = shade;
                material->rgb[2] = shade;
                switch (i) {
                case 1:
                    if (em->parts[5].state >= 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state < 3) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Kind 0x20: every material gets the slot's texture 1. */
    case 32:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            if (i == 0) {
                kind20Tex = 1;
                material->tex = gTexHandles[em->modelSlot * 20 + 163 + kind20Tex];
            }
            material->alpha = em->alpha;
            material->tex = gTexHandles[em->modelSlot * 20 + 163 + kind20Tex];
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Yian Garuga: whole meshes and their broken counterparts by parts 0, 1,
     * 2 and 6, and the part-break reaction. */
    case EM_KIND_YIAN_GARUGA:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 5:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 6:
                    material->alpha = 0.0f;
                    break;
                case 7:
                    if (em->parts[1].state >= 2) material->alpha = 0.0f;
                    break;
                case 8:
                    if (em->parts[2].state >= 2) material->alpha = 0.0f;
                    break;
                case 9:
                    if (em->parts[0].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 0:
                    material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[6].state >= 3) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 0:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[1].state < 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[6].state < 3) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[2].state < 2) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[0].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Blangonga kinds: material 0 coloured by the work area's tint;
     * meshes by part 2 and the part-break reaction; a day mesh pair while
     * defeated. */
    case EM_KIND_BLANGONGA: case EM_KIND_BLANGO:
        work = (EmMeshWork *)&em->work;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            if (em->type == EM_KIND_BLANGONGA) {
                switch (set) {
                case 0:
                    switch (i) {
                    case 0:
                        Color_RgbToFloat(material->rgb, 0xFF000000 | (work->blangTint[0] << 16) | (work->blangTint[1] << 8) | work->blangTint[2]);
                        break;
                    case 4:
                        if (em->partBreakCount > 0) material->alpha = 0.0f;
                        break;
                    case 5:
                        if (Em_PartBroken(em, 2) == 1) material->alpha = 0.0f;
                        break;
                    }
                    break;
                }
            }
            switch (set) {
            case 0:
                switch (i) {
                case 0:
                    Color_RgbToFloat(material->rgb, 0xFF000000 | (work->blangTint[0] << 16) | (work->blangTint[1] << 8) | work->blangTint[2]);
                    break;
                case 1:
                    if (Area_IsDay(gScene.area) == 0 || em->stateGroup != EM_GROUP_DEFEAT) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 0:
                    if (Area_IsDay(gScene.area) == 1 || em->stateGroup == EM_GROUP_DEFEAT) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Congalala kinds: meshes by parts 0, 2 and 3; the carried item's and
     * the food's meshes (sets 6-8); the tint and the day pair as Blangonga. */
    case EM_KIND_CONGALALA: case EM_KIND_CONGA:
        work = (EmMeshWork *)&em->work;
        rageTex = EmBlangonga_EatPropMode(em);
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            if (em->type == EM_KIND_CONGALALA) {
                switch (set) {
                case 0:
                    switch (i) {
                    case 0:
                        if (em->parts[0].state >= 2) material->alpha = 0.0f;
                        break;
                    case 3:
                        if (em->parts[2].state >= 2) material->alpha = 0.0f;
                        break;
                    case 4:
                        if (em->parts[3].state >= 2) material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 1:
                    switch (i) {
                    case 0:
                    case 1:
                        if (em->parts[0].state < 2) material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 2:
                    material->alpha = 0.0f;
                    break;
                case 4:
                    material->alpha = 0.0f;
                    break;
                case 6:
                    if (EmBlangonga_CarriedShown(em, 0) == 0 && EmBlangonga_CarriedShown(em, 1) == 0 && EmBlangonga_CarriedShown(em, 2) == 0 && rageTex != 1) material->alpha = 0.0f;
                    break;
                case 7:
                    if (EmBlangonga_CarriedShown(em, 0) == 1 || EmBlangonga_CarriedShown(em, 1) == 1 || EmBlangonga_CarriedShown(em, 2) == 1 || rageTex == 1) material->alpha = 0.0f;
                    break;
                case 8:
                    switch (i) {
                    case 0:
                        if (EmBlangonga_CarriedShown(em, 0) == 0 || rageTex != 0) material->alpha = 0.0f;
                        break;
                    case 1:
                        if (EmBlangonga_CarriedShown(em, 1) == 0 && rageTex != 1) material->alpha = 0.0f;
                        break;
                    case 2:
                        if (EmBlangonga_CarriedShown(em, 2) == 0 || rageTex != 0) material->alpha = 0.0f;
                        break;
                    }
                    break;
                }
            } else {
                switch (set) {
                case 0:
                    switch (i) {
                    case 0:
                    case 2:
                    case 3:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 5:
                case 6:
                case 7:
                case 8:
                    material->alpha = 0.0f;
                    break;
                }
            }
            switch (set) {
            case 0:
                switch (i) {
                case 1:
                    Color_RgbToFloat(material->rgb, 0xFF000000 | (work->blangTint[0] << 16) | (work->blangTint[1] << 8) | work->blangTint[2]);
                    break;
                case 7:
                    if (Area_IsDay(gScene.area) == 0 || em->stateGroup != EM_GROUP_DEFEAT) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                if (Area_IsDay(gScene.area) == 1 || em->stateGroup == EM_GROUP_DEFEAT) material->alpha = 0.0f;
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Rajang: the rage glow (sets 3-5) faded in by the rage timer (/ 15);
     * meshes by part 0 and the part-break reaction. */
    case EM_KIND_RAJANG:
        work = (EmMeshWork *)&em->work;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 5:
                    if (em->parts[0].state > 0) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[0].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                if (em->partBreakCount > 0) material->alpha = 0.0f;
                break;
            case 3:
                switch (i) {
                case 0:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->partBreakCount <= 0) {
                        material->alpha = 0.0f;
                    } else {
                        material->alpha *= work->blangRageTimer / 15.0f;
                    }
                    break;
                }
                break;
            case 4:
                material->alpha *= work->blangRageTimer / 15.0f;
                break;
            case 5:
                switch (i) {
                case 0:
                    material->alpha *= work->blangRageTimer / 15.0f;
                    break;
                case 1:
                    if (EmBlangonga_CarriedShown(em, 6) == 1) {
                        material->alpha *= work->blangRageTimer / 15.0f;
                    } else {
                        material->alpha = 0.0f;
                    }
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Shen Gaoren: set 0's materials coloured by the flinch level of parts 5,
     * 6, 3 and 4 (GmEm_ShenGaorenFlinchColor); set 4 by part 2's stage. */
    case EM_KIND_SHEN_GAOREN:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 0:
                    GmEm_ShenGaorenFlinchColor(em, material, 5);
                    break;
                case 1:
                    GmEm_ShenGaorenFlinchColor(em, material, 6);
                    break;
                case 2:
                    GmEm_ShenGaorenFlinchColor(em, material, 3);
                    break;
                case 3:
                    GmEm_ShenGaorenFlinchColor(em, material, 4);
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    break;
                case 1:
                    if (em->parts[2].state >= 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[2].state < 2) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[2].state >= 4) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[2].state < 4) material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->parts[2].state >= 4) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[2].state < 4) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Daimyo Hermitaur: whole and broken pairs by parts 5 and 6; the
     * shell (set 5) by the part-break count. */
    case EM_KIND_DAIMYO_HERMITAUR:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 1:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[5].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 5:
                switch (i) {
                case 1:
                    if (em->partBreakCount > 0) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->partBreakCount >= 2) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->partBreakCount < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Shogun Ceanataur: calm and enraged meshes, broken meshes by parts 5
     * and 6; the shell (set 3) picked by variant, hidden once part 2 breaks. */
    case EM_KIND_SHOGUN_CEANATAUR:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 1:
                case 4:
                    if (em->rage == 1) material->alpha = 0.0f;
                    if (i == 1) goto part6;
                    goto part5;
                case 0:
                case 3:
                    if (em->rage == 0) material->alpha = 0.0f;
                    if (i != 0) goto part5;
                case 2:
                part6:
                    if (em->parts[6].state > 0) material->alpha = 0.0f;
                    break;
                case 5:
                part5:
                    if (em->parts[5].state > 0) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[6].state <= 0) material->alpha = 0.0f;
                    break;
                case 7:
                    if (em->parts[5].state <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                if (i != em->variant) {
                    material->alpha = 0.0f;
                } else if (em->parts[2].state >= 2) {
                    material->alpha = 0.0f;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Lunastra and Teostra: whole and broken meshes by Em_PartBroken(0) and
     * (1), and the part-break reaction. */
    case EM_KIND_LUNASTRA: case EM_KIND_TEOSTRA:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 3:
                    if (Em_PartBroken(em, 0) == 1) material->alpha = 0.0f;
                    break;
                case 4:
                    if (Em_PartBroken(em, 0) != 1) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 4:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 5:
                case 6:
                    if (Em_PartBroken(em, 1) == 1) material->alpha = 0.0f;
                    break;
                case 7:
                    if (Em_PartBroken(em, 1) != 1) material->alpha = 0.0f;
                    break;
                case 8:
                case 9:
                    if (Em_PartBroken(em, 1) == 1) material->alpha = 0.0f;
                    break;
                case 10:
                    if (Em_PartBroken(em, 1) != 1) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Kushala Daora kinds: whole and broken meshes by Em_PartBroken(0)
     * and (1), and the part-break reaction. */
    case EM_KIND_KUSHALA_DAORA: case EM_KIND_RUSTED_KUSHALA_DAORA:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 2:
                    if (Em_PartBroken(em, 0) == 1) material->alpha = 0.0f;
                    break;
                case 3:
                    if (Em_PartBroken(em, 0) != 1) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 4:
                    if (Em_PartBroken(em, 1) == 1) material->alpha = 0.0f;
                    break;
                case 5:
                    if (Em_PartBroken(em, 1) != 1) material->alpha = 0.0f;
                    break;
                case 6:
                    if (Em_PartBroken(em, 1) == 1) material->alpha = 0.0f;
                    break;
                case 7:
                    if (Em_PartBroken(em, 1) != 1) material->alpha = 0.0f;
                    break;
                case 8:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Chameleos: meshes by part 0 and the part-break reaction; set 8's
     * material 1 only while it can turn invisible. */
    case EM_KIND_CHAMELEOS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 1:
                    if (Em_PartBroken(em, 0) == 1) material->alpha = 0.0f;
                    break;
                case 2:
                    if (Em_PartBroken(em, 0) == 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 8:
                switch (i) {
                case 0:
                    if (Em_PartBroken(em, 0) == 1) material->alpha = 0.0f;
                    break;
                case 1:
                    if (EmChameleos_CanTurnInvisible(em) == 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Yama Tsukami: set 10's meshes dropped as part 6's state grows. */
    case EM_KIND_YAMA_TSUKAMI:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 10:
                switch (i) {
                case 0:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 1:
                case 2:
                    if (em->parts[6].state >= 6) material->alpha = 0.0f;
                    break;
                case 3:
                case 4:
                    if (em->parts[6].state >= 4) material->alpha = 0.0f;
                    break;
                case 5:
                case 6:
                    if (em->parts[6].state >= 8) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The other kinds: the alpha only. */
    default:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    }
}

/* Bulldrome, with its current motion: while it is enraged, every 45 frames
 * of its idle and moving motions puts a rage aura (monster effect 13,
 * 0.4 times its scale) on it. */
void GmEm_SpawnBulldromeRageAura(Em *em, s16 motion)
{
    switch (motion) {
    case EM_MOTION_ID(0, 1):
    case EM_MOTION_ID(0, 2):
    case EM_MOTION_ID(0, 3):
    case EM_MOTION_ID(0, 4):
    case EM_MOTION_ID(0, 5):
    case EM_MOTION_ID(0, 7):
    case EM_MOTION_ID(0, 10):
    case EM_MOTION_ID(0, 11):
    case EM_MOTION_ID(0, 14):
    case EM_MOTION_ID(0, 17):
        if (em->rage != 0 && gScene.frame % 45 == 0) {
            MonsterFx_CreateOnEm(em, 13, 0.4f * em->scaleX);
        }
        break;
    }
}

/* The file's .data: the per-kind monster tables, read by the game
 * overlay (gEmLowHpRatios, gEmScaleRows, gEmBaseHp, gEmVersusHpFactors, gEmSizeLevelFactors there) and by
 * GmEm_SetupKind (gGmEmCallbackTbl), GmEm_PrepareMeshSet (gGmEmFatalisBMatTex) and gm_load.c
 * (the lists of gGmKindFileLists), and the game's per-kind area tables
 * (Em.tables: gEmKindTables_<Kind>, gm_em.h), area value lists, goal points
 * and motion rows. Generated by tools/gen_data.py as u32 words with pointers
 * as C; typed as their readers read them,
 * the rest still words. */
extern u32 gGmEmCallbackTbl_0[];

EmPartHpRow gEmPartHp_Rathian = {
#include "gm_sub/0090ACC0.inc"
};
EmPartHpRow gEmPartHp_Fatalis = {
#include "gm_sub/0090ACE0.inc"
};
EmPartHpRow gEmPartHp_Kelbi = {
#include "gm_sub/0090AD00.inc"
};
EmPartHpRow gEmPartHp_Mosswine = {
#include "gm_sub/0090AD20.inc"
};
EmPartHpRow gEmPartHp_Bullfango = {
#include "gm_sub/0090AD40.inc"
};
EmPartHpRow gEmPartHp_YianKutKu = {
#include "gm_sub/0090AD60.inc"
};
EmPartHpRow gEmPartHp_LaoShanLung = {
#include "gm_sub/0090AD80.inc"
};
EmPartHpRow gEmPartHp_Cephadrome = {
#include "gm_sub/0090ADA0.inc"
};
EmPartHpRow gEmPartHpTables_9 = {
#include "gm_sub/0090ADC0.inc"
};
EmPartHpRow gEmPartHp_Rathalos = {
#include "gm_sub/0090ADE0.inc"
};
EmPartHpRow gEmPartHpTables_12 = {
#include "gm_sub/0090AE00.inc"
};
EmPartHpRow gEmPartHp_Genprey = {
#include "gm_sub/0090AE20.inc"
};
EmPartHpRow gEmPartHp_Diablos = {
#include "gm_sub/0090AE40.inc"
};
EmPartHpRow gEmPartHp_Khezu = {
#include "gm_sub/0090AE60.inc"
};
EmPartHpRow gEmPartHp_Velociprey = {
#include "gm_sub/0090AE80.inc"
};
EmPartHpRow gEmPartHp_Gravios = {
#include "gm_sub/0090AEA0.inc"
};
EmPartHpRow gEmPartHp_Vespoid = {
#include "gm_sub/0090AEC0.inc"
};
EmPartHpRow gEmPartHp_Gypceros = {
#include "gm_sub/0090AEE0.inc"
};
EmPartHpRow gEmPartHp_Plesioth = {
#include "gm_sub/0090AF00.inc"
};
EmPartHpRow gEmPartHp_Basarios = {
#include "gm_sub/0090AF20.inc"
};
EmPartHpRow gEmPartHp_Melynx = {
#include "gm_sub/0090AF40.inc"
};
EmPartHpRow gEmPartHp_Hornetaur = {
#include "gm_sub/0090AF60.inc"
};
EmPartHpRow gEmPartHp_Apceros = {
#include "gm_sub/0090AF80.inc"
};
EmPartHpRow gEmPartHp_Monoblos = {
#include "gm_sub/0090AFA0.inc"
};
EmPartHpRow gEmPartHp_Velocidrome = {
#include "gm_sub/0090AFC0.inc"
};
EmPartHpRow gEmPartHp_Gendrome = {
#include "gm_sub/0090AFE0.inc"
};
EmPartHpRow gEmPartHpTables_29 = {
#include "gm_sub/0090B000.inc"
};
EmPartHpRow gEmPartHp_Ioprey = {
#include "gm_sub/0090B020.inc"
};
EmPartHpRow gEmPartHp_Iodrome = {
#include "gm_sub/0090B040.inc"
};
EmPartHpRow gEmPartHp_Kirin = {
#include "gm_sub/0090B060.inc"
};
EmPartHpRow gEmPartHp_Cephalos = {
#include "gm_sub/0090B080.inc"
};
EmPartHpRow gEmPartHp_Giaprey = {
#include "gm_sub/0090B0A0.inc"
};
EmPartHpRow gEmPartHp_YianGaruga = {
#include "gm_sub/0090B0C0.inc"
};
EmPartHpRow gEmPartHp_DaimyoHermitaur = {
#include "gm_sub/0090B0E0.inc"
};
EmPartHpRow gEmPartHp_Blangonga = {
#include "gm_sub/0090B100.inc"
};
EmPartHpRow gEmPartHp_Congalala = {
#include "gm_sub/0090B120.inc"
};
EmPartHpRow gEmPartHp_Rajang = {
#include "gm_sub/0090B140.inc"
};
EmPartHpRow gEmPartHp_KushalaDaora = {
#include "gm_sub/0090B160.inc"
};
EmPartHpRow gEmPartHp_ShenGaoren = {
#include "gm_sub/0090B180.inc"
};
EmPartHpRow gEmPartHpTables_56 = {
#include "gm_sub/0090B1A0.inc"
};
EmPartHpRow gEmPartHp_Shakalaka = {
#include "gm_sub/0090B1C0.inc"
};
EmPartHpRow gEmPartHp_YamaTsukami = {
#include "gm_sub/0090B1E0.inc"
};
EmPartHpRow gEmPartHp_Chameleos = {
#include "gm_sub/0090B200.inc"
};
EmPartHpRow gEmPartHp_RustedKushalaDaora = {
#include "gm_sub/0090B220.inc"
};
EmPartHpRow gEmPartHp_Blango = {
#include "gm_sub/0090B240.inc"
};
EmPartHpRow gEmPartHp_Conga = {
#include "gm_sub/0090B260.inc"
};
EmPartHpRow gEmPartHp_Remobra = {
#include "gm_sub/0090B280.inc"
};
EmPartHpRow gEmPartHp_Lunastra = {
#include "gm_sub/0090B2A0.inc"
};
EmPartHpRow gEmPartHp_Teostra = {
#include "gm_sub/0090B2C0.inc"
};
EmPartHpRow gEmPartHp_Hermitaur = {
#include "gm_sub/0090B2E0.inc"
};
EmPartHpRow gEmPartHp_ShogunCeanataur = {
#include "gm_sub/0090B300.inc"
};
EmPartHpRow gEmPartHp_Bulldrome = {
#include "gm_sub/0090B320.inc"
};
EmPartHpRow gEmPartHp_Anteka = {
#include "gm_sub/0090B340.inc"
};
EmPartHpRow gEmPartHp_Popo = {
#include "gm_sub/0090B360.inc"
};
EmPartHpRow gEmPartHp_FatalisC = {
#include "gm_sub/0090B380.inc"
};
EmPartHpRow gEmPartHp_Ceanataur = {
#include "gm_sub/0090B3A0.inc"
};
s16 *gEmPartHpTables[EM_KIND_SLOTS] = {
#include "gm_sub/0090B3C0.inc"
};
EmHitzone gEmHitzones_Trader_0[7] = {
#include "gm_sub/0090B4F0.inc"
};
EmHitzone *gEmHitzones_Trader[2] = {
    gEmHitzones_Trader_0, NULL,
};
EmHitzone gEmHitzones_Rathian_0[7] = {
#include "gm_sub/0090B540.inc"
};
EmHitzone *gEmHitzones_Rathian[2] = {
    gEmHitzones_Rathian_0, NULL,
};
EmHitzone gEmHitzones_Fatalis_0[7] = {
#include "gm_sub/0090B590.inc"
};
EmHitzone *gEmHitzones_Fatalis[2] = {
    gEmHitzones_Fatalis_0, NULL,
};
EmHitzone gEmHitzones_Kelbi_0[7] = {
#include "gm_sub/0090B5E0.inc"
};
EmHitzone *gEmHitzones_Kelbi[2] = {
    gEmHitzones_Kelbi_0, NULL,
};
EmHitzone gEmHitzones_Mosswine_0[7] = {
#include "gm_sub/0090B630.inc"
};
EmHitzone *gEmHitzones_Mosswine[2] = {
    gEmHitzones_Mosswine_0, NULL,
};
EmHitzone gEmHitzones_Bullfango_0[7] = {
#include "gm_sub/0090B680.inc"
};
EmHitzone *gEmHitzones_Bullfango[2] = {
    gEmHitzones_Bullfango_0, NULL,
};
EmHitzone gEmHitzones_YianKutKu_0[7] = {
#include "gm_sub/0090B6D0.inc"
};
EmHitzone *gEmHitzones_YianKutKu[2] = {
    gEmHitzones_YianKutKu_0, NULL,
};
EmHitzone gEmHitzones_LaoShanLung_0[7] = {
#include "gm_sub/0090B720.inc"
};
EmHitzone *gEmHitzones_LaoShanLung[2] = {
    gEmHitzones_LaoShanLung_0, NULL,
};
EmHitzone gEmHitzones_Cephadrome_0[7] = {
#include "gm_sub/0090B770.inc"
};
EmHitzone *gEmHitzones_Cephadrome[2] = {
    gEmHitzones_Cephadrome_0, NULL,
};
EmHitzone gEmHitzoneTables_9_0[7] = {
#include "gm_sub/0090B7C0.inc"
};
EmHitzone *gEmHitzoneTables_9[2] = {
    gEmHitzoneTables_9_0, NULL,
};
EmHitzone gEmHitzones_Rathalos_0[7] = {
#include "gm_sub/0090B810.inc"
};
EmHitzone *gEmHitzones_Rathalos[2] = {
    gEmHitzones_Rathalos_0, NULL,
};
EmHitzone gEmHitzoneTables_12_0[7] = {
#include "gm_sub/0090B860.inc"
};
EmHitzone *gEmHitzoneTables_12[2] = {
    gEmHitzoneTables_12_0, NULL,
};
EmHitzone gEmHitzones_Genprey_0[7] = {
#include "gm_sub/0090B8B0.inc"
};
EmHitzone *gEmHitzones_Genprey[2] = {
    gEmHitzones_Genprey_0, NULL,
};
EmHitzone gEmHitzones_Diablos_0[7] = {
#include "gm_sub/0090B900.inc"
};
EmHitzone *gEmHitzones_Diablos[2] = {
    gEmHitzones_Diablos_0, NULL,
};
EmHitzone gEmHitzones_Khezu_0[7] = {
#include "gm_sub/0090B950.inc"
};
EmHitzone *gEmHitzones_Khezu[2] = {
    gEmHitzones_Khezu_0, NULL,
};
EmHitzone gEmHitzones_Velociprey_0[7] = {
#include "gm_sub/0090B9A0.inc"
};
EmHitzone *gEmHitzones_Velociprey[2] = {
    gEmHitzones_Velociprey_0, NULL,
};
EmHitzone gEmHitzones_Gravios_0[8] = {
#include "gm_sub/0090B9F0.inc"
};
EmHitzone gEmHitzones_Gravios_1[7] = {
#include "gm_sub/0090BA40.inc"
};
EmHitzone *gEmHitzones_Gravios[2] = {
    gEmHitzones_Gravios_0, gEmHitzones_Gravios_1,
};
EmHitzone gEmHitzones_Vespoid_0[7] = {
#include "gm_sub/0090BA90.inc"
};
EmHitzone *gEmHitzones_Vespoid[2] = {
    gEmHitzones_Vespoid_0, NULL,
};
EmHitzone gEmHitzones_Gypceros_0[7] = {
#include "gm_sub/0090BAE0.inc"
};
EmHitzone *gEmHitzones_Gypceros[2] = {
    gEmHitzones_Gypceros_0, NULL,
};
EmHitzone gEmHitzones_Plesioth_0[7] = {
#include "gm_sub/0090BB30.inc"
};
EmHitzone *gEmHitzones_Plesioth[2] = {
    gEmHitzones_Plesioth_0, NULL,
};
EmHitzone gEmHitzones_Basarios_0[7] = {
#include "gm_sub/0090BB80.inc"
};
EmHitzone *gEmHitzones_Basarios[2] = {
    gEmHitzones_Basarios_0, NULL,
};
EmHitzone gEmHitzones_Melynx_0[7] = {
#include "gm_sub/0090BBD0.inc"
};
EmHitzone *gEmHitzones_Melynx[2] = {
    gEmHitzones_Melynx_0, NULL,
};
EmHitzone gEmHitzones_Hornetaur_0[7] = {
#include "gm_sub/0090BC20.inc"
};
EmHitzone *gEmHitzones_Hornetaur[2] = {
    gEmHitzones_Hornetaur_0, NULL,
};
EmHitzone gEmHitzones_Apceros_0[7] = {
#include "gm_sub/0090BC70.inc"
};
EmHitzone *gEmHitzones_Apceros[2] = {
    gEmHitzones_Apceros_0, NULL,
};
EmHitzone gEmHitzones_Monoblos_0[7] = {
#include "gm_sub/0090BCC0.inc"
};
EmHitzone *gEmHitzones_Monoblos[2] = {
    gEmHitzones_Monoblos_0, NULL,
};
EmHitzone gEmHitzones_Velocidrome_0[7] = {
#include "gm_sub/0090BD10.inc"
};
EmHitzone *gEmHitzones_Velocidrome[2] = {
    gEmHitzones_Velocidrome_0, NULL,
};
EmHitzone gEmHitzones_Gendrome_0[7] = {
#include "gm_sub/0090BD60.inc"
};
EmHitzone *gEmHitzones_Gendrome[2] = {
    gEmHitzones_Gendrome_0, NULL,
};
EmHitzone gEmHitzoneTables_29_0[7] = {
#include "gm_sub/0090BDB0.inc"
};
EmHitzone *gEmHitzoneTables_29[2] = {
    gEmHitzoneTables_29_0, NULL,
};
EmHitzone gEmHitzones_Ioprey_0[7] = {
#include "gm_sub/0090BE00.inc"
};
EmHitzone *gEmHitzones_Ioprey[2] = {
    gEmHitzones_Ioprey_0, NULL,
};
EmHitzone gEmHitzones_Iodrome_0[7] = {
#include "gm_sub/0090BE50.inc"
};
EmHitzone *gEmHitzones_Iodrome[2] = {
    gEmHitzones_Iodrome_0, NULL,
};
EmHitzone gEmHitzones_Kirin_0[7] = {
#include "gm_sub/0090BEA0.inc"
};
EmHitzone *gEmHitzones_Kirin[2] = {
    gEmHitzones_Kirin_0, NULL,
};
EmHitzone gEmHitzones_Cephalos_0[7] = {
#include "gm_sub/0090BEF0.inc"
};
EmHitzone *gEmHitzones_Cephalos[2] = {
    gEmHitzones_Cephalos_0, NULL,
};
EmHitzone gEmHitzones_Giaprey_0[7] = {
#include "gm_sub/0090BF40.inc"
};
EmHitzone *gEmHitzones_Giaprey[2] = {
    gEmHitzones_Giaprey_0, NULL,
};
EmHitzone gEmHitzones_FatalisB_0[8] = {
#include "gm_sub/0090BF90.inc"
};
EmHitzone gEmHitzones_FatalisB_1[7] = {
#include "gm_sub/0090BFE0.inc"
};
EmHitzone *gEmHitzones_FatalisB[2] = {
    gEmHitzones_FatalisB_0, gEmHitzones_FatalisB_1,
};
EmHitzone gEmHitzones_PinkRathian_0[7] = {
#include "gm_sub/0090C030.inc"
};
EmHitzone *gEmHitzones_PinkRathian[2] = {
    gEmHitzones_PinkRathian_0, NULL,
};
EmHitzone gEmHitzones_YianGaruga_0[7] = {
#include "gm_sub/0090C080.inc"
};
EmHitzone *gEmHitzones_YianGaruga[2] = {
    gEmHitzones_YianGaruga_0, NULL,
};
EmHitzone gEmHitzones_SilverRathalos_0[7] = {
#include "gm_sub/0090C0D0.inc"
};
EmHitzone *gEmHitzones_SilverRathalos[2] = {
    gEmHitzones_SilverRathalos_0, NULL,
};
EmHitzone gEmHitzones_GoldRathian_0[7] = {
#include "gm_sub/0090C120.inc"
};
EmHitzone *gEmHitzones_GoldRathian[2] = {
    gEmHitzones_GoldRathian_0, NULL,
};
EmHitzone gEmHitzones_RedKhezu_0[7] = {
#include "gm_sub/0090C170.inc"
};
EmHitzone *gEmHitzones_RedKhezu[2] = {
    gEmHitzones_RedKhezu_0, NULL,
};
EmHitzone gEmHitzones_BlackGravios_0[8] = {
#include "gm_sub/0090C1C0.inc"
};
EmHitzone gEmHitzones_BlackGravios_1[7] = {
#include "gm_sub/0090C210.inc"
};
EmHitzone *gEmHitzones_BlackGravios[2] = {
    gEmHitzones_BlackGravios_0, gEmHitzones_BlackGravios_1,
};
EmHitzone gEmHitzones_DaimyoHermitaur_0[8] = {
#include "gm_sub/0090C260.inc"
};
EmHitzone gEmHitzones_DaimyoHermitaur_1[8] = {
#include "gm_sub/0090C2B0.inc"
};
EmHitzone gEmHitzones_DaimyoHermitaur_2[7] = {
#include "gm_sub/0090C300.inc"
};
EmHitzone *gEmHitzones_DaimyoHermitaur[3] = {
    gEmHitzones_DaimyoHermitaur_0, gEmHitzones_DaimyoHermitaur_1, gEmHitzones_DaimyoHermitaur_2,
};
EmHitzone gEmHitzones_AzureRathalos_0[7] = {
#include "gm_sub/0090C360.inc"
};
EmHitzone *gEmHitzones_AzureRathalos[2] = {
    gEmHitzones_AzureRathalos_0, NULL,
};
EmHitzone gEmHitzones_Blangonga_0[7] = {
#include "gm_sub/0090C3B0.inc"
};
EmHitzone *gEmHitzones_Blangonga[2] = {
    gEmHitzones_Blangonga_0, NULL,
};
EmHitzone gEmHitzones_Congalala_0[8] = {
#include "gm_sub/0090C400.inc"
};
EmHitzone gEmHitzones_Congalala_1[7] = {
#include "gm_sub/0090C450.inc"
};
EmHitzone *gEmHitzones_Congalala[2] = {
    gEmHitzones_Congalala_0, gEmHitzones_Congalala_1,
};
EmHitzone gEmHitzones_Rajang_0[7] = {
#include "gm_sub/0090C4A0.inc"
};
EmHitzone *gEmHitzones_Rajang[2] = {
    gEmHitzones_Rajang_0, NULL,
};
EmHitzone gEmHitzones_KushalaDaora_0[8] = {
#include "gm_sub/0090C4F0.inc"
};
EmHitzone gEmHitzones_KushalaDaora_1[7] = {
#include "gm_sub/0090C540.inc"
};
EmHitzone *gEmHitzones_KushalaDaora[2] = {
    gEmHitzones_KushalaDaora_0, gEmHitzones_KushalaDaora_1,
};
EmHitzone gEmHitzones_ShenGaoren_0[7] = {
#include "gm_sub/0090C590.inc"
};
EmHitzone *gEmHitzones_ShenGaoren[2] = {
    gEmHitzones_ShenGaoren_0, NULL,
};
EmHitzone gEmHitzoneTables_56_0[7] = {
#include "gm_sub/0090C5E0.inc"
};
EmHitzone *gEmHitzoneTables_56[2] = {
    gEmHitzoneTables_56_0, NULL,
};
EmHitzone gEmHitzones_Shakalaka_0[7] = {
#include "gm_sub/0090C630.inc"
};
EmHitzone *gEmHitzones_Shakalaka[2] = {
    gEmHitzones_Shakalaka_0, NULL,
};
EmHitzone gEmHitzones_YamaTsukami_0[7] = {
#include "gm_sub/0090C680.inc"
};
EmHitzone *gEmHitzones_YamaTsukami[2] = {
    gEmHitzones_YamaTsukami_0, NULL,
};
EmHitzone gEmHitzones_Chameleos_0[8] = {
#include "gm_sub/0090C6D0.inc"
};
EmHitzone gEmHitzones_Chameleos_1[7] = {
#include "gm_sub/0090C720.inc"
};
EmHitzone *gEmHitzones_Chameleos[2] = {
    gEmHitzones_Chameleos_0, gEmHitzones_Chameleos_1,
};
EmHitzone gEmHitzones_Blango_0[7] = {
#include "gm_sub/0090C770.inc"
};
EmHitzone *gEmHitzones_Blango[2] = {
    gEmHitzones_Blango_0, NULL,
};
EmHitzone gEmHitzones_Conga_0[7] = {
#include "gm_sub/0090C7C0.inc"
};
EmHitzone *gEmHitzones_Conga[2] = {
    gEmHitzones_Conga_0, NULL,
};
EmHitzone gEmHitzones_Remobra_0[7] = {
#include "gm_sub/0090C810.inc"
};
EmHitzone *gEmHitzones_Remobra[2] = {
    gEmHitzones_Remobra_0, NULL,
};
EmHitzone gEmHitzones_Lunastra_0[8] = {
#include "gm_sub/0090C860.inc"
};
EmHitzone gEmHitzones_Lunastra_1[7] = {
#include "gm_sub/0090C8B0.inc"
};
EmHitzone *gEmHitzones_Lunastra[2] = {
    gEmHitzones_Lunastra_0, gEmHitzones_Lunastra_1,
};
EmHitzone gEmHitzones_Hermitaur_0[7] = {
#include "gm_sub/0090C900.inc"
};
EmHitzone *gEmHitzones_Hermitaur[2] = {
    gEmHitzones_Hermitaur_0, NULL,
};
EmHitzone gEmHitzones_ShogunCeanataur_0[8] = {
#include "gm_sub/0090C950.inc"
};
EmHitzone gEmHitzones_ShogunCeanataur_1[7] = {
#include "gm_sub/0090C9A0.inc"
};
EmHitzone *gEmHitzones_ShogunCeanataur[2] = {
    gEmHitzones_ShogunCeanataur_0, gEmHitzones_ShogunCeanataur_1,
};
EmHitzone gEmHitzones_Bulldrome_0[7] = {
#include "gm_sub/0090C9F0.inc"
};
EmHitzone *gEmHitzones_Bulldrome[2] = {
    gEmHitzones_Bulldrome_0, NULL,
};
EmHitzone gEmHitzones_Anteka_0[7] = {
#include "gm_sub/0090CA40.inc"
};
EmHitzone *gEmHitzones_Anteka[2] = {
    gEmHitzones_Anteka_0, NULL,
};
EmHitzone gEmHitzones_Popo_0[7] = {
#include "gm_sub/0090CA90.inc"
};
EmHitzone *gEmHitzones_Popo[2] = {
    gEmHitzones_Popo_0, NULL,
};
EmHitzone gEmHitzones_FatalisC_0[8] = {
#include "gm_sub/0090CAE0.inc"
};
EmHitzone gEmHitzones_FatalisC_1[7] = {
#include "gm_sub/0090CB30.inc"
};
EmHitzone *gEmHitzones_FatalisC[2] = {
    gEmHitzones_FatalisC_0, gEmHitzones_FatalisC_1,
};
EmHitzone gEmHitzones_Ceanataur_0[7] = {
#include "gm_sub/0090CB80.inc"
};
EmHitzone *gEmHitzones_Ceanataur[2] = {
#include "gm_sub/0090CBC8.inc"
};
EmHitzone **gEmHitzoneTables[EM_KIND_SLOTS] = {
#include "gm_sub/0090CBD0.inc"
};
f32 gEmLowHpRatios[EM_KIND_SLOTS] = {
#include "gm_sub/0090CD00.inc"
};
EmScaleRow gEmScaleRows_Kelbi[26] = {
#include "gm_sub/0090CE30.inc"
};
EmScaleRow gEmScaleRows_Rathian[26] = {
#include "gm_sub/0090CF70.inc"
};
EmScaleRow gEmScaleRows_Fatalis[26] = {
#include "gm_sub/0090D0B0.inc"
};
EmScaleRow gEmScaleRows_LaoShanLung[26] = {
#include "gm_sub/0090D1F0.inc"
};
EmScaleRow gEmScaleRows_YianGaruga[26] = {
#include "gm_sub/0090D330.inc"
};
EmScaleRow *gEmScaleRows[EM_KIND_CEANATAUR + 1] = {
#include "gm_sub/0090D470.inc"
};
/*
 * The level tables (QuestEm_RollLevel in sub_main's quest.c, Galex_RollEmLevel in plsel's gallery_scene.c):
 * gEmLevelTables' rows are (weight, level) s8 pairs ended by a weight of -1, picked by
 * gEmLevelTableIdx[kind][daytime]; gEmLevelSeasonAdd and gEmSizeSeasonAdd are by kind and season.
 */
s8 gEmLevelTables_0[8] = {
#include "gm_sub/0090D598.inc"
};
s8 gEmLevelTables_1[16] = {
#include "gm_sub/0090D5A0.inc"
};
s8 gEmLevelTables_2[8] = {
#include "gm_sub/0090D5B0.inc"
};
s8 *gEmLevelTables[3] = {
#include "gm_sub/0090D5B8.inc"
};
s8 gEmLevelTableIdx[EM_KIND_CEANATAUR + 1][2] = {
#include "gm_sub/0090D5D0.inc"
};
s8 gEmLevelSeasonAdd[EM_KIND_CEANATAUR + 1][3] = {
#include "gm_sub/0090D670.inc"
};
u16 gEmSizeSeasonAdd[EM_KIND_CEANATAUR + 1][3] = {
#include "gm_sub/0090D750.inc"
};
s16 gEmBaseHp_Rathian[EM_HP_VARIANTS] = {
#include "gm_sub/0090D910.inc"
};
s16 gEmBaseHp_Fatalis[EM_HP_VARIANTS] = {
#include "gm_sub/0090D918.inc"
};
s16 gEmBaseHp_Kelbi[EM_HP_VARIANTS] = {
#include "gm_sub/0090D920.inc"
};
s16 gEmBaseHp_Mosswine[EM_HP_VARIANTS] = {
#include "gm_sub/0090D928.inc"
};
s16 gEmBaseHp_Bullfango[EM_HP_VARIANTS] = {
#include "gm_sub/0090D930.inc"
};
s16 gEmBaseHp_YianKutKu[EM_HP_VARIANTS] = {
#include "gm_sub/0090D938.inc"
};
s16 gEmBaseHp_LaoShanLung[EM_HP_VARIANTS] = {
#include "gm_sub/0090D940.inc"
};
s16 gEmBaseHp_Cephadrome[EM_HP_VARIANTS] = {
#include "gm_sub/0090D948.inc"
};
s16 gEmBaseHp_9[EM_HP_VARIANTS] = {
#include "gm_sub/0090D950.inc"
};
s16 gEmBaseHp_Trader[EM_HP_VARIANTS] = {
#include "gm_sub/0090D958.inc"
};
s16 gEmBaseHp_Rathalos[EM_HP_VARIANTS] = {
#include "gm_sub/0090D960.inc"
};
s16 gEmBaseHp_12[EM_HP_VARIANTS] = {
#include "gm_sub/0090D968.inc"
};
s16 gEmBaseHp_Genprey[EM_HP_VARIANTS] = {
#include "gm_sub/0090D970.inc"
};
s16 gEmBaseHp_Diablos[EM_HP_VARIANTS] = {
#include "gm_sub/0090D978.inc"
};
s16 gEmBaseHp_Khezu[EM_HP_VARIANTS] = {
#include "gm_sub/0090D980.inc"
};
s16 gEmBaseHp_Velociprey[EM_HP_VARIANTS] = {
#include "gm_sub/0090D988.inc"
};
s16 gEmBaseHp_Gravios[EM_HP_VARIANTS] = {
#include "gm_sub/0090D990.inc"
};
s16 gEmBaseHp_FelyneCart[EM_HP_VARIANTS] = {
#include "gm_sub/0090D998.inc"
};
s16 gEmBaseHp_Vespoid[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9A0.inc"
};
s16 gEmBaseHp_Gypceros[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9A8.inc"
};
s16 gEmBaseHp_Plesioth[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9B0.inc"
};
s16 gEmBaseHp_Basarios[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9B8.inc"
};
s16 gEmBaseHp_Melynx[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9C0.inc"
};
s16 gEmBaseHp_Hornetaur[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9C8.inc"
};
s16 gEmBaseHp_Apceros[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9D0.inc"
};
s16 gEmBaseHp_Monoblos[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9D8.inc"
};
s16 gEmBaseHp_Velocidrome[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9E0.inc"
};
s16 gEmBaseHp_Gendrome[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9E8.inc"
};
s16 gEmBaseHp_Ioprey[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9F0.inc"
};
s16 gEmBaseHp_Iodrome[EM_HP_VARIANTS] = {
#include "gm_sub/0090D9F8.inc"
};
s16 gEmBaseHp_32[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA00.inc"
};
s16 gEmBaseHp_Kirin[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA08.inc"
};
s16 gEmBaseHp_Cephalos[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA10.inc"
};
s16 gEmBaseHp_Giaprey[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA18.inc"
};
s16 gEmBaseHp_FatalisB[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA20.inc"
};
s16 gEmBaseHp_PinkRathian[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA28.inc"
};
s16 gEmBaseHp_BlueYianKutKu[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA30.inc"
};
s16 gEmBaseHp_PurpleGypceros[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA38.inc"
};
s16 gEmBaseHp_YianGaruga[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA40.inc"
};
s16 gEmBaseHp_SilverRathalos[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA48.inc"
};
s16 gEmBaseHp_GoldRathian[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA50.inc"
};
s16 gEmBaseHp_BlackDiablos[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA58.inc"
};
s16 gEmBaseHp_WhiteMonoblos[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA60.inc"
};
s16 gEmBaseHp_RedKhezu[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA68.inc"
};
s16 gEmBaseHp_GreenPlesioth[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA70.inc"
};
s16 gEmBaseHp_BlackGravios[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA78.inc"
};
s16 gEmBaseHp_DaimyoHermitaur[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA80.inc"
};
s16 gEmBaseHp_AzureRathalos[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA88.inc"
};
s16 gEmBaseHp_AshenLaoShanLung[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA90.inc"
};
s16 gEmBaseHp_Blangonga[EM_HP_VARIANTS] = {
#include "gm_sub/0090DA98.inc"
};
s16 gEmBaseHp_Congalala[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAA0.inc"
};
s16 gEmBaseHp_Rajang[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAA8.inc"
};
s16 gEmBaseHp_KushalaDaora[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAB0.inc"
};
s16 gEmBaseHp_ShenGaoren[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAB8.inc"
};
s16 gEmBaseHp_56[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAC0.inc"
};
s16 gEmBaseHp_Shakalaka[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAC8.inc"
};
s16 gEmBaseHp_YamaTsukami[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAD0.inc"
};
s16 gEmBaseHp_Chameleos[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAD8.inc"
};
s16 gEmBaseHp_Blango[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAE0.inc"
};
s16 gEmBaseHp_Conga[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAE8.inc"
};
s16 gEmBaseHp_Remobra[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAF0.inc"
};
s16 gEmBaseHp_Lunastra[EM_HP_VARIANTS] = {
#include "gm_sub/0090DAF8.inc"
};
s16 gEmBaseHp_Hermitaur[EM_HP_VARIANTS] = {
#include "gm_sub/0090DB00.inc"
};
s16 gEmBaseHp_ShogunCeanataur[EM_HP_VARIANTS] = {
#include "gm_sub/0090DB08.inc"
};
s16 gEmBaseHp_Bulldrome[EM_HP_VARIANTS] = {
#include "gm_sub/0090DB10.inc"
};
s16 gEmBaseHp_Anteka[EM_HP_VARIANTS] = {
#include "gm_sub/0090DB18.inc"
};
s16 gEmBaseHp_Popo[EM_HP_VARIANTS] = {
#include "gm_sub/0090DB20.inc"
};
s16 gEmBaseHp_Ceanataur[EM_HP_VARIANTS] = {
#include "gm_sub/0090DB28.inc"
};
s16 *gEmBaseHp[EM_KIND_CEANATAUR + 1] = {
#include "gm_sub/0090DB30.inc"
};
f32 gEmVersusHpFactors_0[2] = {
#include "gm_sub/0090DC58.inc"
};
f32 gEmVersusHpFactors_1[2] = {
#include "gm_sub/0090DC60.inc"
};
f32 gEmVersusHpFactors_2[2] = {
#include "gm_sub/0090DC68.inc"
};
f32 gEmVersusHpFactors_3[2] = {
#include "gm_sub/0090DC70.inc"
};
f32 gEmVersusHpFactors_4[2] = {
#include "gm_sub/0090DC78.inc"
};
f32 gEmVersusHpFactors_5[2] = {
#include "gm_sub/0090DC80.inc"
};
f32 gEmVersusHpFactors_6[2] = {
#include "gm_sub/0090DC88.inc"
};
f32 gEmVersusHpFactors_7[2] = {
#include "gm_sub/0090DC90.inc"
};
f32 gEmVersusHpFactors_8[2] = {
#include "gm_sub/0090DC98.inc"
};
f32 gEmVersusHpFactors_9[2] = {
#include "gm_sub/0090DCA0.inc"
};
f32 gEmVersusHpFactors_10[2] = {
#include "gm_sub/0090DCA8.inc"
};
f32 gEmVersusHpFactors_11[2] = {
#include "gm_sub/0090DCB0.inc"
};
f32 gEmVersusHpFactors_12[2] = {
#include "gm_sub/0090DCB8.inc"
};
f32 gEmVersusHpFactors_13[2] = {
#include "gm_sub/0090DCC0.inc"
};
f32 gEmVersusHpFactors_14[2] = {
#include "gm_sub/0090DCC8.inc"
};
f32 gEmVersusHpFactors_15[2] = {
#include "gm_sub/0090DCD0.inc"
};
f32 gEmVersusHpFactors_16[2] = {
#include "gm_sub/0090DCD8.inc"
};
f32 *gEmVersusHpFactors[EM_VERSUS_HP_FACTORS] = {
#include "gm_sub/0090DCE0.inc"
};
f32 gEmSizeLevelFactors[EM_SIZE_LEVEL_FACTORS] = {
#include "gm_sub/0090DD50.inc"
};
u8 gEmMotionGroupCounts[80] = {
#include "gm_sub/0090DDC0.inc"
};
u32 gGmEmCallbackTbl[76] = {
    (u32)gGmEmCallbackTbl_0, (u32)EmRathalos_Callbacks, (u32)EmFatalis_Callbacks, (u32)EmAnteka_Callbacks,
    (u32)EmMosswine_Callbacks, (u32)EmMosswine_Callbacks, (u32)EmKutku_Callbacks, (u32)EmLaoshan_Callbacks,
    (u32)EmCephadrome_Callbacks, (u32)EmMelynx_Callbacks, (u32)EmTrader_Callbacks, (u32)EmRathalos_Callbacks,
    (u32)EmApceros_Callbacks, (u32)EmVelociprey_Callbacks, (u32)EmDiablos_Callbacks, (u32)EmKhezu_Callbacks,
    (u32)EmVelociprey_Callbacks, (u32)EmGravios_Callbacks, (u32)EmFelyneCart_Callbacks, (u32)EmVespoid_Callbacks,
    (u32)EmKutku_Callbacks, (u32)EmPlesioth_Callbacks, (u32)EmGravios_Callbacks, (u32)EmMelynx_Callbacks,
    (u32)EmVespoid_Callbacks, (u32)EmApceros_Callbacks, (u32)EmDiablos_Callbacks, (u32)EmDrome_Callbacks,
    (u32)EmDrome_Callbacks, (u32)EmKind1D_Callbacks, (u32)EmVelociprey_Callbacks, (u32)EmDrome_Callbacks,
    (u32)EmMosswine_Callbacks, (u32)EmKirin_Callbacks, (u32)EmCephadrome_Callbacks, (u32)EmVelociprey_Callbacks,
    (u32)EmFatalis_Callbacks, (u32)EmRathalos_Callbacks, (u32)EmKutku_Callbacks, (u32)EmKutku_Callbacks,
    (u32)EmGaruga_Callbacks, (u32)EmRathalos_Callbacks, (u32)EmRathalos_Callbacks, (u32)EmDiablos_Callbacks,
    (u32)EmDiablos_Callbacks, (u32)EmKhezu_Callbacks, (u32)EmPlesioth_Callbacks, (u32)EmGravios_Callbacks,
    (u32)EmHermitaur_Callbacks, (u32)EmRathalos_Callbacks, (u32)EmLaoshan_Callbacks, (u32)EmBlangonga_Callbacks,
    (u32)EmBlangonga_Callbacks, (u32)EmBlangonga_Callbacks, (u32)EmKushala_Callbacks, (u32)EmShenGaoren_Callbacks,
    (u32)EmKind38_Callbacks, (u32)EmShakalaka_Callbacks, (u32)EmYamaTsukami_Callbacks, (u32)EmChameleos_Callbacks,
    (u32)EmKushala_Callbacks, (u32)EmBlangonga_Callbacks, (u32)EmBlangonga_Callbacks, (u32)EmRemobra_Callbacks,
    (u32)EmKushala_Callbacks, (u32)EmKushala_Callbacks, (u32)EmHermitaur_Callbacks, (u32)EmHermitaur_Callbacks,
    (u32)EmBulldrome_Callbacks, (u32)EmAnteka_Callbacks, (u32)EmApceros_Callbacks, (u32)EmFatalis_Callbacks,
    (u32)EmYamaTsukami_Callbacks, (u32)EmHermitaur_Callbacks, 0, 0,
};
/* The texture offsets of the 32 meshes of EM_KIND_FATALIS_B (GmEm_PrepareMeshSet). */
s16 gGmEmFatalisBMatTex[40] = {
#include "gm_sub/0090DF40.inc"
};
u32 gGmEmCallbackTbl_0[8] = {
    (u32)GmEm_SpawnShadow, (u32)GmEm_SpawnShadow, (u32)GmEm_Nop, (u32)GmEm_Nop,
    (u32)GmEm_ReturnZero, 0, 0, 0,
};
f32 gEmAreaPoints_Rathian_1_0[10][3] = {
#include "gm_sub/0090DFB0.inc"
};
f32 gEmAreaPoints_Rathian_1_1[4][3] = {
#include "gm_sub/0090E030.inc"
};
f32 gEmAreaPoints_Rathian_1_2[5][3] = {
#include "gm_sub/0090E060.inc"
};
f32 gEmAreaPoints_Rathian_1_3[9][3] = {
#include "gm_sub/0090E0A0.inc"
};
f32 gEmAreaPoints_Rathian_1_4[16][3] = {
#include "gm_sub/0090E110.inc"
};
f32 gEmAreaPoints_Rathian_1_5[8][3] = {
#include "gm_sub/0090E1D0.inc"
};
f32 gEmAreaPoints_Rathian_1_6[6][3] = {
#include "gm_sub/0090E230.inc"
};
f32 gEmAreaPoints_Rathian_1_7[4][3] = {
#include "gm_sub/0090E280.inc"
};
f32 gEmAreaPoints_Rathian_1_8[5][3] = {
#include "gm_sub/0090E2B0.inc"
};
f32 gEmAreaPoints_Rathian_1_9[4][3] = {
#include "gm_sub/0090E2F0.inc"
};
f32 gEmAreaPoints_Rathian_1_10[2][3] = {
#include "gm_sub/0090E320.inc"
};
f32 gEmAreaPoints_Rathian_1_11[2][3] = {
#include "gm_sub/0090E340.inc"
};
f32 gEmAreaPoints_Rathian_1_12[2][3] = {
#include "gm_sub/0090E360.inc"
};
f32 gEmAreaPoints_Rathian_1_13[2][3] = {
#include "gm_sub/0090E380.inc"
};
f32 gEmAreaPoints_Rathian_1_14[2][3] = {
#include "gm_sub/0090E3A0.inc"
};
f32 gEmAreaPoints_Rathian_1_15[2][3] = {
#include "gm_sub/0090E3C0.inc"
};
f32 gEmAreaPoints_Rathian_1_16[2][3] = {
#include "gm_sub/0090E3E0.inc"
};
f32 gEmAreaPoints_Rathian_1_17[2][3] = {
#include "gm_sub/0090E400.inc"
};
f32 gEmAreaPoints_Rathian_1_18[4][3] = {
#include "gm_sub/0090E420.inc"
};
f32 gEmAreaPoints_Rathian_1_19[4][3] = {
#include "gm_sub/0090E450.inc"
};
f32 gEmAreaPoints_Rathian_1_20[4][3] = {
#include "gm_sub/0090E480.inc"
};
f32 gEmAreaPoints_Rathian_1_21[8][3] = {
#include "gm_sub/0090E4B0.inc"
};
f32 gEmAreaPoints_Rathian_1_22[4][3] = {
#include "gm_sub/0090E510.inc"
};
f32 gEmAreaPoints_Rathian_1_23[2][3] = {
#include "gm_sub/0090E540.inc"
};
f32 gEmAreaPoints_Rathian_1_24[6][3] = {
#include "gm_sub/0090E560.inc"
};
f32 gEmAreaPoints_Rathian_1_25[4][3] = {
#include "gm_sub/0090E5B0.inc"
};
f32 gEmAreaPoints_Rathian_1_26[6][3] = {
#include "gm_sub/0090E5E0.inc"
};
f32 gEmAreaPoints_Rathian_1_27[4][3] = {
#include "gm_sub/0090E630.inc"
};
f32 gEmAreaPoints_Rathian_1_28[2][3] = {
#include "gm_sub/0090E660.inc"
};
f32 gEmAreaPoints_Rathian_1_29[4][3] = {
#include "gm_sub/0090E680.inc"
};
f32 gEmAreaPoints_Rathian_1_30[4][3] = {
#include "gm_sub/0090E6B0.inc"
};
f32 gEmAreaPoints_Rathian_1_31[4][3] = {
#include "gm_sub/0090E6E0.inc"
};
f32 gEmAreaPoints_Rathian_1_32[4][3] = {
#include "gm_sub/0090E710.inc"
};
f32 gEmAreaPoints_Rathian_1_33[4][3] = {
#include "gm_sub/0090E740.inc"
};
f32 gEmAreaPoints_Rathian_1_34[4][3] = {
#include "gm_sub/0090E770.inc"
};
f32 gEmAreaPoints_Rathian_1_35[6][3] = {
#include "gm_sub/0090E7A0.inc"
};
f32 gEmAreaPoints_Rathian_1_36[5][3] = {
#include "gm_sub/0090E7F0.inc"
};
f32 gEmAreaPoints_Rathian_1_37[5][3] = {
#include "gm_sub/0090E830.inc"
};
f32 gEmAreaPoints_Rathian_1_38[4][3] = {
#include "gm_sub/0090E870.inc"
};
EmAreaPoints gEmAreaPoints_Rathian_1[40] = {
#include "gm_sub/0090E8A0.inc"
};
f32 gEmAreaPoints_Rathian_3_0[1][3] = {
#include "gm_sub/0090E9E0.inc"
};
f32 gEmAreaPoints_Rathian_3_1[1][3] = {
#include "gm_sub/0090E9F0.inc"
};
f32 gEmAreaPoints_Rathian_3_2[1][3] = {
#include "gm_sub/0090EA00.inc"
};
f32 gEmAreaPoints_Rathian_3_3[1][3] = {
#include "gm_sub/0090EA10.inc"
};
f32 gEmAreaPoints_Rathian_3_4[1][3] = {
#include "gm_sub/0090EA20.inc"
};
f32 gEmAreaPoints_Rathian_3_5[1][3] = {
#include "gm_sub/0090EA30.inc"
};
f32 gEmAreaPoints_Rathian_3_6[1][3] = {
#include "gm_sub/0090EA40.inc"
};
f32 gEmAreaPoints_Rathian_3_7[1][3] = {
#include "gm_sub/0090EA50.inc"
};
f32 gEmAreaPoints_Rathian_3_8[1][3] = {
#include "gm_sub/0090EA60.inc"
};
f32 gEmAreaPoints_Rathian_3_9[1][3] = {
#include "gm_sub/0090EA70.inc"
};
f32 gEmAreaPoints_Rathian_3_10[1][3] = {
#include "gm_sub/0090EA80.inc"
};
f32 gEmAreaPoints_Rathian_3_11[1][3] = {
#include "gm_sub/0090EA90.inc"
};
f32 gEmAreaPoints_Rathian_3_12[1][3] = {
#include "gm_sub/0090EAA0.inc"
};
f32 gEmAreaPoints_Rathian_3_13[1][3] = {
#include "gm_sub/0090EAB0.inc"
};
f32 gEmAreaPoints_Rathian_3_14[1][3] = {
#include "gm_sub/0090EAC0.inc"
};
f32 gEmAreaPoints_Rathian_3_15[1][3] = {
#include "gm_sub/0090EAD0.inc"
};
f32 gEmAreaPoints_Rathian_3_16[1][3] = {
#include "gm_sub/0090EAE0.inc"
};
f32 gEmAreaPoints_Rathian_3_17[1][3] = {
#include "gm_sub/0090EAF0.inc"
};
f32 gEmAreaPoints_Rathian_3_18[1][3] = {
#include "gm_sub/0090EB00.inc"
};
f32 gEmAreaPoints_Rathian_3_19[1][3] = {
#include "gm_sub/0090EB10.inc"
};
f32 gEmAreaPoints_Rathian_3_20[1][3] = {
#include "gm_sub/0090EB20.inc"
};
f32 gEmAreaPoints_Rathian_3_21[1][3] = {
#include "gm_sub/0090EB30.inc"
};
f32 gEmAreaPoints_Rathian_3_22[1][3] = {
#include "gm_sub/0090EB40.inc"
};
f32 gEmAreaPoints_Rathian_3_23[1][3] = {
#include "gm_sub/0090EB50.inc"
};
f32 gEmAreaPoints_Rathian_3_24[1][3] = {
#include "gm_sub/0090EB60.inc"
};
f32 gEmAreaPoints_Rathian_3_25[1][3] = {
#include "gm_sub/0090EB70.inc"
};
f32 gEmAreaPoints_Rathian_3_26[1][3] = {
#include "gm_sub/0090EB80.inc"
};
f32 gEmAreaPoints_Rathian_3_27[1][3] = {
#include "gm_sub/0090EB90.inc"
};
f32 gEmAreaPoints_Rathian_3_28[1][3] = {
#include "gm_sub/0090EBA0.inc"
};
f32 gEmAreaPoints_Rathian_3_29[1][3] = {
#include "gm_sub/0090EBB0.inc"
};
f32 gEmAreaPoints_Rathian_3_30[1][3] = {
#include "gm_sub/0090EBC0.inc"
};
f32 gEmAreaPoints_Rathian_3_31[1][3] = {
#include "gm_sub/0090EBD0.inc"
};
f32 gEmAreaPoints_Rathian_3_32[1][3] = {
#include "gm_sub/0090EBE0.inc"
};
f32 gEmAreaPoints_Rathian_3_33[1][3] = {
#include "gm_sub/0090EBF0.inc"
};
f32 gEmAreaPoints_Rathian_3_34[1][3] = {
#include "gm_sub/0090EC00.inc"
};
f32 gEmAreaPoints_Rathian_3_35[1][3] = {
#include "gm_sub/0090EC10.inc"
};
f32 gEmAreaPoints_Rathian_3_36[1][3] = {
#include "gm_sub/0090EC20.inc"
};
EmAreaPoints gEmAreaPoints_Rathian_3[38] = {
#include "gm_sub/0090EC30.inc"
};
f32 gEmAreaPoints_Rathian_4_0[1][3] = {
#include "gm_sub/0090ED60.inc"
};
f32 gEmAreaPoints_Rathian_4_1[1][3] = {
#include "gm_sub/0090ED70.inc"
};
f32 gEmAreaPoints_Rathian_4_2[1][3] = {
#include "gm_sub/0090ED80.inc"
};
f32 gEmAreaPoints_Rathian_4_3[1][3] = {
#include "gm_sub/0090ED90.inc"
};
f32 gEmAreaPoints_Rathian_4_4[1][3] = {
#include "gm_sub/0090EDA0.inc"
};
f32 gEmAreaPoints_Rathian_4_5[1][3] = {
#include "gm_sub/0090EDB0.inc"
};
f32 gEmAreaPoints_Rathian_4_6[1][3] = {
#include "gm_sub/0090EDC0.inc"
};
f32 gEmAreaPoints_Rathian_4_7[1][3] = {
#include "gm_sub/0090EDD0.inc"
};
f32 gEmAreaPoints_Rathian_4_8[1][3] = {
#include "gm_sub/0090EDE0.inc"
};
f32 gEmAreaPoints_Rathian_4_9[1][3] = {
#include "gm_sub/0090EDF0.inc"
};
f32 gEmAreaPoints_Rathian_4_10[1][3] = {
#include "gm_sub/0090EE00.inc"
};
f32 gEmAreaPoints_Rathian_4_11[1][3] = {
#include "gm_sub/0090EE10.inc"
};
f32 gEmAreaPoints_Rathian_4_12[1][3] = {
#include "gm_sub/0090EE20.inc"
};
f32 gEmAreaPoints_Rathian_4_13[1][3] = {
#include "gm_sub/0090EE30.inc"
};
f32 gEmAreaPoints_Rathian_4_14[1][3] = {
#include "gm_sub/0090EE40.inc"
};
f32 gEmAreaPoints_Rathian_4_15[1][3] = {
#include "gm_sub/0090EE50.inc"
};
f32 gEmAreaPoints_Rathian_4_16[1][3] = {
#include "gm_sub/0090EE60.inc"
};
f32 gEmAreaPoints_Rathian_4_17[1][3] = {
#include "gm_sub/0090EE70.inc"
};
f32 gEmAreaPoints_Rathian_4_18[1][3] = {
#include "gm_sub/0090EE80.inc"
};
f32 gEmAreaPoints_Rathian_4_19[1][3] = {
#include "gm_sub/0090EE90.inc"
};
f32 gEmAreaPoints_Rathian_4_20[1][3] = {
#include "gm_sub/0090EEA0.inc"
};
f32 gEmAreaPoints_Rathian_4_21[1][3] = {
#include "gm_sub/0090EEB0.inc"
};
f32 gEmAreaPoints_Rathian_4_22[1][3] = {
#include "gm_sub/0090EEC0.inc"
};
f32 gEmAreaPoints_Rathian_4_23[1][3] = {
#include "gm_sub/0090EED0.inc"
};
f32 gEmAreaPoints_Rathian_4_24[1][3] = {
#include "gm_sub/0090EEE0.inc"
};
f32 gEmAreaPoints_Rathian_4_25[1][3] = {
#include "gm_sub/0090EEF0.inc"
};
f32 gEmAreaPoints_Rathian_4_26[1][3] = {
#include "gm_sub/0090EF00.inc"
};
f32 gEmAreaPoints_Rathian_4_27[1][3] = {
#include "gm_sub/0090EF10.inc"
};
EmAreaPoints gEmAreaPoints_Rathian_4[29] = {
#include "gm_sub/0090EF20.inc"
};
f32 gEmAreaPoints_Rathian_5_0[1][3] = {
#include "gm_sub/0090F008.inc"
};
f32 gEmAreaPoints_Rathian_5_1[1][3] = {
#include "gm_sub/0090F018.inc"
};
f32 gEmAreaPoints_Rathian_5_2[1][3] = {
#include "gm_sub/0090F028.inc"
};
f32 gEmAreaPoints_Rathian_5_3[1][3] = {
#include "gm_sub/0090F038.inc"
};
f32 gEmAreaPoints_Rathian_5_4[1][3] = {
#include "gm_sub/0090F048.inc"
};
f32 gEmAreaPoints_Rathian_5_5[1][3] = {
#include "gm_sub/0090F058.inc"
};
f32 gEmAreaPoints_Rathian_5_6[1][3] = {
#include "gm_sub/0090F068.inc"
};
f32 gEmAreaPoints_Rathian_5_7[1][3] = {
#include "gm_sub/0090F078.inc"
};
f32 gEmAreaPoints_Rathian_5_8[1][3] = {
#include "gm_sub/0090F088.inc"
};
f32 gEmAreaPoints_Rathian_5_9[1][3] = {
#include "gm_sub/0090F098.inc"
};
f32 gEmAreaPoints_Rathian_5_10[1][3] = {
#include "gm_sub/0090F0A8.inc"
};
f32 gEmAreaPoints_Rathian_5_11[1][3] = {
#include "gm_sub/0090F0B8.inc"
};
extern u32 gEmKindTables_68_0[];
extern u32 gEmKindTables_68_2[];
extern u32 gEmRouteSets[];
EmAreaPoints gEmAreaPoints_Rathian_5[13] = {
#include "gm_sub/0090F0D0.inc"
};
void *gEmKindTables_Rathian[6] = {
#include "gm_sub/0090F140.inc"
};
EmAreaValue gEmAreaTimer1Values_Rathian[1] = {
#include "gm_sub/0090F158.inc"
};
EmAreaValue gEmAreaTimer2Values_Rathian[22] = {
#include "gm_sub/0090F160.inc"
};
s16 gEmIdSets_Rathian_0_1_0[8] = {
#include "gm_sub/0090F1C0.inc"
};
s16 gEmIdSets_Rathian_0_1_1[8] = {
#include "gm_sub/0090F1D0.inc"
};
s16 gEmIdSets_Rathian_0_1_2[8] = {
#include "gm_sub/0090F1E0.inc"
};
s16 gEmIdSets_Rathian_0_1_3[8] = {
#include "gm_sub/0090F1F0.inc"
};
s16 gEmIdSets_Rathian_0_1_4[8] = {
#include "gm_sub/0090F200.inc"
};
s16 gEmIdSets_Rathian_0_1_5[8] = {
#include "gm_sub/0090F210.inc"
};
s16 gEmIdSets_Rathian_0_1_6[8] = {
#include "gm_sub/0090F220.inc"
};
EmNextAreaEntry gEmIdSets_Rathian_0_1[8] = {
#include "gm_sub/0090F230.inc"
};
s16 gEmIdSets_Rathian_0_0_0[8] = {
#include "gm_sub/0090F270.inc"
};
s16 gEmIdSets_Rathian_0_0_1[8] = {
#include "gm_sub/0090F280.inc"
};
s16 gEmIdSets_Rathian_0_0_2[8] = {
#include "gm_sub/0090F290.inc"
};
EmNextAreaEntry gEmIdSets_Rathian_0_0[4] = {
#include "gm_sub/0090F2A0.inc"
};
EmNextAreaSet gEmIdSets_Rathian_0[2] = {
#include "gm_sub/0090F2C0.inc"
};
s16 gEmIdSets_Rathian_3_0_0[8] = {
#include "gm_sub/0090F2D0.inc"
};
s16 gEmIdSets_Rathian_3_0_1[8] = {
#include "gm_sub/0090F2E0.inc"
};
s16 gEmIdSets_Rathian_3_0_2[8] = {
#include "gm_sub/0090F2F0.inc"
};
s16 gEmIdSets_Rathian_3_0_3[8] = {
#include "gm_sub/0090F300.inc"
};
s16 gEmIdSets_Rathian_3_0_4[8] = {
#include "gm_sub/0090F310.inc"
};
s16 gEmIdSets_Rathian_3_0_5[8] = {
#include "gm_sub/0090F320.inc"
};
s16 gEmIdSets_Rathian_3_0_6[8] = {
#include "gm_sub/0090F330.inc"
};
EmNextAreaEntry gEmIdSets_Rathian_3_0[8] = {
#include "gm_sub/0090F340.inc"
};
EmNextAreaSet gEmIdSets_Rathian_3[1] = {
#include "gm_sub/0090F380.inc"
};
s16 gEmIdSets_Rathian_4_0_0[8] = {
#include "gm_sub/0090F390.inc"
};
s16 gEmIdSets_Rathian_4_0_1[8] = {
#include "gm_sub/0090F3A0.inc"
};
s16 gEmIdSets_Rathian_4_0_2[8] = {
#include "gm_sub/0090F3B0.inc"
};
s16 gEmIdSets_Rathian_4_0_3[8] = {
#include "gm_sub/0090F3C0.inc"
};
s16 gEmIdSets_Rathian_4_0_4[8] = {
#include "gm_sub/0090F3D0.inc"
};
s16 gEmIdSets_Rathian_4_0_5[8] = {
#include "gm_sub/0090F3E0.inc"
};
s16 gEmIdSets_Rathian_4_0_6[8] = {
#include "gm_sub/0090F3F0.inc"
};
EmNextAreaEntry gEmIdSets_Rathian_4_0[8] = {
#include "gm_sub/0090F400.inc"
};
EmNextAreaSet gEmIdSets_Rathian_4[1] = {
#include "gm_sub/0090F440.inc"
};
s16 gEmIdSets_Rathian_6_0_0[8] = {
#include "gm_sub/0090F450.inc"
};
s16 gEmIdSets_Rathian_6_0_1[8] = {
#include "gm_sub/0090F460.inc"
};
s16 gEmIdSets_Rathian_6_0_2[8] = {
#include "gm_sub/0090F470.inc"
};
s16 gEmIdSets_Rathian_6_0_3[8] = {
#include "gm_sub/0090F480.inc"
};
s16 gEmIdSets_Rathian_6_0_4[8] = {
#include "gm_sub/0090F490.inc"
};
s16 gEmIdSets_Rathian_6_0_5[8] = {
#include "gm_sub/0090F4A0.inc"
};
s16 gEmIdSets_Rathian_6_0_6[8] = {
#include "gm_sub/0090F4B0.inc"
};
s16 gEmIdSets_Rathian_6_0_7[8] = {
#include "gm_sub/0090F4C0.inc"
};
s16 gEmIdSets_Rathian_6_0_8[8] = {
#include "gm_sub/0090F4D0.inc"
};
s16 gEmIdSets_Rathian_6_0_9[8] = {
#include "gm_sub/0090F4E0.inc"
};
EmNextAreaEntry gEmIdSets_Rathian_6_0[11] = {
#include "gm_sub/0090F4F0.inc"
};
EmNextAreaSet gEmIdSets_Rathian_6[1] = {
#include "gm_sub/0090F548.inc"
};
s16 gEmIdSets_Rathian_11_0_0[8] = {
#include "gm_sub/0090F550.inc"
};
s16 gEmIdSets_Rathian_11_0_1[8] = {
#include "gm_sub/0090F560.inc"
};
s16 gEmIdSets_Rathian_11_0_2[8] = {
#include "gm_sub/0090F570.inc"
};
s16 gEmIdSets_Rathian_11_0_3[8] = {
#include "gm_sub/0090F580.inc"
};
s16 gEmIdSets_Rathian_11_0_4[8] = {
#include "gm_sub/0090F590.inc"
};
s16 gEmIdSets_Rathian_11_0_5[8] = {
#include "gm_sub/0090F5A0.inc"
};
EmNextAreaEntry gEmIdSets_Rathian_11_0[7] = {
#include "gm_sub/0090F5B0.inc"
};
EmNextAreaSet gEmIdSets_Rathian_11[1] = {
#include "gm_sub/0090F5E8.inc"
};
s16 gEmIdSets_Rathian_13_0_0[8] = {
#include "gm_sub/0090F5F0.inc"
};
s16 gEmIdSets_Rathian_13_0_1[8] = {
#include "gm_sub/0090F600.inc"
};
s16 gEmIdSets_Rathian_13_0_2[8] = {
#include "gm_sub/0090F610.inc"
};
s16 gEmIdSets_Rathian_13_0_3[8] = {
#include "gm_sub/0090F620.inc"
};
s16 gEmIdSets_Rathian_13_0_4[8] = {
#include "gm_sub/0090F630.inc"
};
s16 gEmIdSets_Rathian_13_0_5[8] = {
#include "gm_sub/0090F640.inc"
};
EmNextAreaEntry gEmIdSets_Rathian_13_0[7] = {
#include "gm_sub/0090F650.inc"
};
EmNextAreaSet gEmIdSets_Rathian_13[1] = {
#include "gm_sub/0090F688.inc"
};
EmNextAreaSet *gEmIdSets_Rathian[MAP_COUNT] = {
#include "gm_sub/0090F690.inc"
};
s16 gEmIdSets_Rathalos_0_1_0[8] = {
#include "gm_sub/0090F700.inc"
};
s16 gEmIdSets_Rathalos_0_1_1[8] = {
#include "gm_sub/0090F710.inc"
};
s16 gEmIdSets_Rathalos_0_1_2[8] = {
#include "gm_sub/0090F720.inc"
};
s16 gEmIdSets_Rathalos_0_1_3[8] = {
#include "gm_sub/0090F730.inc"
};
s16 gEmIdSets_Rathalos_0_1_4[8] = {
#include "gm_sub/0090F740.inc"
};
s16 gEmIdSets_Rathalos_0_1_5[8] = {
#include "gm_sub/0090F750.inc"
};
s16 gEmIdSets_Rathalos_0_1_6[8] = {
#include "gm_sub/0090F760.inc"
};
EmNextAreaEntry gEmIdSets_Rathalos_0_1[8] = {
#include "gm_sub/0090F770.inc"
};
s16 gEmIdSets_Rathalos_0_0_0[8] = {
#include "gm_sub/0090F7B0.inc"
};
s16 gEmIdSets_Rathalos_0_0_1[8] = {
#include "gm_sub/0090F7C0.inc"
};
s16 gEmIdSets_Rathalos_0_0_2[8] = {
#include "gm_sub/0090F7D0.inc"
};
s16 gEmIdSets_Rathalos_0_0_3[8] = {
#include "gm_sub/0090F7E0.inc"
};
s16 gEmIdSets_Rathalos_0_0_4[8] = {
#include "gm_sub/0090F7F0.inc"
};
EmNextAreaEntry gEmIdSets_Rathalos_0_0[6] = {
#include "gm_sub/0090F800.inc"
};
EmNextAreaSet gEmIdSets_Rathalos_0[2] = {
#include "gm_sub/0090F830.inc"
};
s16 gEmIdSets_Rathalos_4_0_0[8] = {
#include "gm_sub/0090F840.inc"
};
s16 gEmIdSets_Rathalos_4_0_1[8] = {
#include "gm_sub/0090F850.inc"
};
s16 gEmIdSets_Rathalos_4_0_2[8] = {
#include "gm_sub/0090F860.inc"
};
s16 gEmIdSets_Rathalos_4_0_3[8] = {
#include "gm_sub/0090F870.inc"
};
EmNextAreaEntry gEmIdSets_Rathalos_4_0[5] = {
#include "gm_sub/0090F880.inc"
};
EmNextAreaSet gEmIdSets_Rathalos_4[1] = {
#include "gm_sub/0090F8A8.inc"
};
s16 gEmIdSets_Rathalos_5_0_0[8] = {
#include "gm_sub/0090F8B0.inc"
};
s16 gEmIdSets_Rathalos_5_0_1[8] = {
#include "gm_sub/0090F8C0.inc"
};
s16 gEmIdSets_Rathalos_5_0_2[8] = {
#include "gm_sub/0090F8D0.inc"
};
s16 gEmIdSets_Rathalos_5_0_3[8] = {
#include "gm_sub/0090F8E0.inc"
};
EmNextAreaEntry gEmIdSets_Rathalos_5_0[5] = {
#include "gm_sub/0090F8F0.inc"
};
EmNextAreaSet gEmIdSets_Rathalos_5[1] = {
#include "gm_sub/0090F918.inc"
};
EmNextAreaSet *gEmIdSets_Rathalos[MAP_COUNT] = {
#include "gm_sub/0090F920.inc"
};
EmMotionRow gEmMotionRows_Rathian[101] = {
#include "gm_sub/0090F990.inc"
};
f32 gEmAreaPoints_Fatalis_1_0[6][3] = {
#include "gm_sub/0090FE50.inc"
};
f32 gEmAreaPoints_Fatalis_1_1[1][3] = {
#include "gm_sub/0090FE98.inc"
};
f32 gEmAreaPoints_Fatalis_1_3[5][3] = {
#include "gm_sub/0090FEB0.inc"
};
EmAreaPoints gEmAreaPoints_Fatalis_1[5] = {
#include "gm_sub/0090FEF0.inc"
};
f32 gEmAreaPoints_Fatalis_3_0[1][3] = {
#include "gm_sub/0090FF18.inc"
};
f32 gEmAreaPoints_Fatalis_3_1[1][3] = {
#include "gm_sub/0090FF28.inc"
};
EmAreaPoints gEmAreaPoints_Fatalis_3[3] = {
#include "gm_sub/0090FF40.inc"
};
f32 gEmAreaPoints_Fatalis_4_0[1][3] = {
#include "gm_sub/0090FF58.inc"
};
f32 gEmAreaPoints_Fatalis_4_1[1][3] = {
#include "gm_sub/0090FF68.inc"
};
EmAreaPoints gEmAreaPoints_Fatalis_4[3] = {
#include "gm_sub/0090FF80.inc"
};
f32 gEmAreaPoints_Fatalis_5_0[1][3] = {
#include "gm_sub/0090FF98.inc"
};
f32 gEmAreaPoints_Fatalis_5_1[1][3] = {
#include "gm_sub/0090FFA8.inc"
};
EmAreaPoints gEmAreaPoints_Fatalis_5[3] = {
#include "gm_sub/0090FFC0.inc"
};
void *gEmKindTables_Fatalis[6] = {
#include "gm_sub/0090FFE0.inc"
};
EmAreaValue gEmAreaTimer1Values_Fatalis[1] = {
#include "gm_sub/0090FFF8.inc"
};
EmAreaValue gEmAreaTimer2Values_Fatalis[1] = {
#include "gm_sub/00910000.inc"
};
EmMotionRow gEmMotionRows_Fatalis[101] = {
#include "gm_sub/00910010.inc"
};
f32 gEmAreaPoints_Kelbi_1_0[6][3] = {
#include "gm_sub/009104D0.inc"
};
f32 gEmAreaPoints_Kelbi_1_1[8][3] = {
#include "gm_sub/00910520.inc"
};
f32 gEmAreaPoints_Kelbi_1_2[5][3] = {
#include "gm_sub/00910580.inc"
};
f32 gEmAreaPoints_Kelbi_1_3[5][3] = {
#include "gm_sub/009105C0.inc"
};
f32 gEmAreaPoints_Kelbi_1_4[8][3] = {
#include "gm_sub/00910600.inc"
};
f32 gEmAreaPoints_Kelbi_1_5[6][3] = {
#include "gm_sub/00910660.inc"
};
f32 gEmAreaPoints_Kelbi_1_6[8][3] = {
#include "gm_sub/009106B0.inc"
};
f32 gEmAreaPoints_Kelbi_1_7[5][3] = {
#include "gm_sub/00910710.inc"
};
f32 gEmAreaPoints_Kelbi_1_8[8][3] = {
#include "gm_sub/00910750.inc"
};
f32 gEmAreaPoints_Kelbi_1_9[6][3] = {
#include "gm_sub/009107B0.inc"
};
f32 gEmAreaPoints_Kelbi_1_10[8][3] = {
#include "gm_sub/00910800.inc"
};
f32 gEmAreaPoints_Kelbi_1_11[8][3] = {
#include "gm_sub/00910860.inc"
};
f32 gEmAreaPoints_Kelbi_1_12[6][3] = {
#include "gm_sub/009108C0.inc"
};
EmAreaPoints gEmAreaPoints_Kelbi_1[14] = {
#include "gm_sub/00910910.inc"
};
f32 gEmAreaPoints_Kelbi_3_0[1][3] = {
#include "gm_sub/00910980.inc"
};
EmAreaPoints gEmAreaPoints_Kelbi_3[2] = {
#include "gm_sub/00910990.inc"
};
f32 gEmAreaPoints_Kelbi_4_0[1][3] = {
#include "gm_sub/009109A0.inc"
};
EmAreaPoints gEmAreaPoints_Kelbi_4[2] = {
#include "gm_sub/009109B0.inc"
};
f32 gEmAreaPoints_Kelbi_5_0[1][3] = {
#include "gm_sub/009109C0.inc"
};
EmAreaPoints gEmAreaPoints_Kelbi_5[2] = {
#include "gm_sub/009109D0.inc"
};
void *gEmKindTables_Kelbi[7] = {
#include "gm_sub/009109E0.inc"
};
EmMotionRow gEmMotionRows_Kelbi[101] = {
#include "gm_sub/00910A00.inc"
};
f32 gEmAreaPoints_Mosswine_1_0[4][3] = {
#include "gm_sub/00910EC0.inc"
};
f32 gEmAreaPoints_Mosswine_1_1[4][3] = {
#include "gm_sub/00910EF0.inc"
};
f32 gEmAreaPoints_Mosswine_1_2[8][3] = {
#include "gm_sub/00910F20.inc"
};
f32 gEmAreaPoints_Mosswine_1_3[18][3] = {
#include "gm_sub/00910F80.inc"
};
f32 gEmAreaPoints_Mosswine_1_4[6][3] = {
#include "gm_sub/00911060.inc"
};
f32 gEmAreaPoints_Mosswine_1_5[8][3] = {
#include "gm_sub/009110B0.inc"
};
f32 gEmAreaPoints_Mosswine_1_6[13][3] = {
#include "gm_sub/00911110.inc"
};
f32 gEmAreaPoints_Mosswine_1_7[8][3] = {
#include "gm_sub/009111B0.inc"
};
f32 gEmAreaPoints_Mosswine_1_8[8][3] = {
#include "gm_sub/00911210.inc"
};
f32 gEmAreaPoints_Mosswine_1_9[8][3] = {
#include "gm_sub/00911270.inc"
};
EmAreaPoints gEmAreaPoints_Mosswine_1[11] = {
#include "gm_sub/009112D0.inc"
};
EmAreaPoints gEmAreaPoints_Mosswine_3[1] = {
#include "gm_sub/00911328.inc"
};
EmAreaPoints gEmAreaPoints_Mosswine_4[1] = {
#include "gm_sub/00911330.inc"
};
EmAreaPoints gEmAreaPoints_Mosswine_5[1] = {
#include "gm_sub/00911338.inc"
};
EmRoutePt gEmRouteSets_Mosswine_0_0[6] = {
#include "gm_sub/00911340.inc"
};
EmRoute gEmRouteSets_Mosswine_0[1] = {
#include "gm_sub/009113C0.inc"
};
EmRouteSet gEmRouteSets_Mosswine[2] = {
#include "gm_sub/009113E0.inc"
};
void *gEmKindTables_Mosswine[7] = {
#include "gm_sub/009113F0.inc"
};
f32 gEmGoalPoints_Mosswine[5][3] = {
#include "gm_sub/00911410.inc"
};
EmMotionRow gEmMotionRows_Mosswine[101] = {
#include "gm_sub/00911450.inc"
};
EmMotionRow gEmMotionRows_YianKutKu[110] = {
#include "gm_sub/00911910.inc"
};
f32 gEmAreaPoints_LaoShanLung_1_0[5][3] = {
#include "gm_sub/00911E40.inc"
};
f32 gEmAreaPoints_LaoShanLung_1_1[8][3] = {
#include "gm_sub/00911E80.inc"
};
f32 gEmAreaPoints_LaoShanLung_1_2[4][3] = {
#include "gm_sub/00911EE0.inc"
};
f32 gEmAreaPoints_LaoShanLung_1_3[2][3] = {
#include "gm_sub/00911F10.inc"
};
f32 gEmAreaPoints_LaoShanLung_1_4[5][3] = {
#include "gm_sub/00911F30.inc"
};
EmAreaPoints gEmAreaPoints_LaoShanLung_1[6] = {
#include "gm_sub/00911F70.inc"
};
f32 gEmAreaPoints_LaoShanLung_3_0[1][3] = {
#include "gm_sub/00911FA0.inc"
};
EmAreaPoints gEmAreaPoints_LaoShanLung_3[2] = {
#include "gm_sub/00911FB0.inc"
};
f32 gEmAreaPoints_LaoShanLung_4_0[1][3] = {
#include "gm_sub/00911FC0.inc"
};
EmAreaPoints gEmAreaPoints_LaoShanLung_4[2] = {
#include "gm_sub/00911FD0.inc"
};
f32 gEmAreaPoints_LaoShanLung_5_0[1][3] = {
#include "gm_sub/00911FE0.inc"
};
EmAreaPoints gEmAreaPoints_LaoShanLung_5[2] = {
#include "gm_sub/00911FF0.inc"
};
EmRoutePt gEmRouteSets_LaoShanLung_0_0[1] = {
#include "gm_sub/00912000.inc"
};
EmRoute gEmRouteSets_LaoShanLung_0[1] = {
#include "gm_sub/00912020.inc"
};
EmRoutePt gEmRouteSets_LaoShanLung_1_0[1] = {
#include "gm_sub/00912040.inc"
};
EmRoute gEmRouteSets_LaoShanLung_1[1] = {
#include "gm_sub/00912060.inc"
};
EmRoutePt gEmRouteSets_LaoShanLung_2_0[1] = {
#include "gm_sub/00912080.inc"
};
EmRoute gEmRouteSets_LaoShanLung_2[1] = {
#include "gm_sub/009120A0.inc"
};
EmRoutePt gEmRouteSets_LaoShanLung_3_0[1] = {
#include "gm_sub/009120C0.inc"
};
EmRoute gEmRouteSets_LaoShanLung_3[1] = {
#include "gm_sub/009120E0.inc"
};
EmRoutePt gEmRouteSets_LaoShanLung_4_0[1] = {
#include "gm_sub/00912100.inc"
};
EmRoute gEmRouteSets_LaoShanLung_4[1] = {
#include "gm_sub/00912120.inc"
};
EmRouteSet gEmRouteSets_LaoShanLung[6] = {
#include "gm_sub/00912140.inc"
};
void *gEmKindTables_LaoShanLung[7] = {
#include "gm_sub/00912170.inc"
};
EmMotionRow gEmMotionRows_LaoShanLung[101] = {
#include "gm_sub/00912190.inc"
};
EmMotionRow gEmMotionTransRows_Rathian[2] = {
#include "gm_sub/00912650.inc"
};
EmMotionRow gEmMotionAltRows_Rathian[12] = {
#include "gm_sub/00912670.inc"
};
f32 gEmAreaPoints_Cephadrome_1_0[4][3] = {
#include "gm_sub/00912700.inc"
};
f32 gEmAreaPoints_Cephadrome_1_1[4][3] = {
#include "gm_sub/00912730.inc"
};
f32 gEmAreaPoints_Cephadrome_1_2[4][3] = {
#include "gm_sub/00912760.inc"
};
EmAreaPoints gEmAreaPoints_Cephadrome_1[4] = {
#include "gm_sub/00912790.inc"
};
f32 gEmAreaPoints_Cephadrome_2_0[6][3] = {
#include "gm_sub/009127B0.inc"
};
f32 gEmAreaPoints_Cephalos_2_0[4][3] = {
#include "gm_sub/00912800.inc"
};
f32 gEmAreaPoints_Cephadrome_2_1[4][3] = {
#include "gm_sub/00912830.inc"
};
f32 gEmAreaPoints_Cephadrome_2_2[6][3] = {
#include "gm_sub/00912860.inc"
};
f32 gEmAreaPoints_Cephalos_2_2[4][3] = {
#include "gm_sub/009128B0.inc"
};
f32 gEmAreaPoints_Cephadrome_2_3[4][3] = {
#include "gm_sub/009128E0.inc"
};
EmAreaPoints gEmAreaPoints_Cephadrome_2[6] = {
#include "gm_sub/00912910.inc"
};
EmAreaPoints gEmAreaPoints_Cephalos_2[5] = {
#include "gm_sub/00912940.inc"
};
EmAreaPoints gEmAreaPoints_Cephadrome_3[1] = {
#include "gm_sub/00912968.inc"
};
EmAreaPoints gEmAreaPoints_Cephadrome_4[1] = {
#include "gm_sub/00912970.inc"
};
EmAreaPoints gEmAreaPoints_Cephadrome_5[1] = {
#include "gm_sub/00912978.inc"
};
EmRoutePt gEmRouteSets_Cephadrome_0_0[2] = {
#include "gm_sub/00912980.inc"
};
EmRoute gEmRouteSets_Cephadrome_0[1] = {
#include "gm_sub/009129B0.inc"
};
EmRoutePt gEmRouteSets_Cephadrome_1_0[3] = {
#include "gm_sub/009129D0.inc"
};
EmRoute gEmRouteSets_Cephadrome_1[1] = {
#include "gm_sub/00912A10.inc"
};
EmRoutePt gEmRouteSets_Cephadrome_2_0[2] = {
#include "gm_sub/00912A30.inc"
};
EmRoutePt gEmRouteSets_Cephadrome_2_1[2] = {
#include "gm_sub/00912A60.inc"
};
EmRoute gEmRouteSets_Cephadrome_2[2] = {
#include "gm_sub/00912A90.inc"
};
EmRoutePt gEmRouteSets_Cephadrome_3_0[1] = {
#include "gm_sub/00912AC0.inc"
};
EmRoutePt gEmRouteSets_Cephadrome_3_1[1] = {
#include "gm_sub/00912AE0.inc"
};
EmRoute gEmRouteSets_Cephadrome_3[2] = {
#include "gm_sub/00912B00.inc"
};
EmRoutePt gEmRouteSets_Cephadrome_4_0[1] = {
#include "gm_sub/00912B30.inc"
};
EmRoutePt gEmRouteSets_Cephadrome_4_1[1] = {
#include "gm_sub/00912B50.inc"
};
EmRoute gEmRouteSets_Cephadrome_4[2] = {
#include "gm_sub/00912B70.inc"
};
EmRouteSet gEmRouteSets_Cephadrome[6] = {
#include "gm_sub/00912BA0.inc"
};
void *gEmKindTables_Cephadrome[7] = {
#include "gm_sub/00912BD0.inc"
};
void *gEmKindTables_Cephalos[7] = {
#include "gm_sub/00912BF0.inc"
};
EmAreaValue gEmAreaTimer1Values_Cephadrome[1] = {
#include "gm_sub/00912C10.inc"
};
EmAreaValue gEmAreaTimer2Values_Cephadrome[4] = {
#include "gm_sub/00912C20.inc"
};
EmMotionRow gEmMotionRows_Cephadrome[141] = {
#include "gm_sub/00912C30.inc"
};
f32 gEmAreaPoints_Melynx_1_0[7][3] = {
#include "gm_sub/009132D0.inc"
};
f32 gEmAreaPoints_Melynx_1_1[1][3] = {
#include "gm_sub/00913328.inc"
};
f32 gEmAreaPoints_Melynx_1_2[1][3] = {
#include "gm_sub/00913338.inc"
};
f32 gEmAreaPoints_Melynx_1_3[2][3] = {
#include "gm_sub/00913350.inc"
};
f32 gEmAreaPoints_Melynx_1_4[2][3] = {
#include "gm_sub/00913370.inc"
};
f32 gEmAreaPoints_Melynx_1_5[2][3] = {
#include "gm_sub/00913390.inc"
};
f32 gEmAreaPoints_Melynx_1_6[5][3] = {
#include "gm_sub/009133B0.inc"
};
f32 gEmAreaPoints_Melynx_1_7[4][3] = {
#include "gm_sub/009133F0.inc"
};
f32 gEmAreaPoints_Melynx_1_8[1][3] = {
#include "gm_sub/00913420.inc"
};
f32 gEmAreaPoints_Melynx_1_9[2][3] = {
#include "gm_sub/00913430.inc"
};
f32 gEmAreaPoints_Melynx_1_10[6][3] = {
#include "gm_sub/00913450.inc"
};
EmAreaPoints gEmAreaPoints_Melynx_1[12] = {
#include "gm_sub/009134A0.inc"
};
f32 gEmAreaPoints_Melynx_3_0[1][3] = {
#include "gm_sub/00913500.inc"
};
f32 gEmAreaPoints_Melynx_3_1[1][3] = {
#include "gm_sub/00913510.inc"
};
f32 gEmAreaPoints_Melynx_3_2[1][3] = {
#include "gm_sub/00913520.inc"
};
EmAreaPoints gEmAreaPoints_Melynx_3[4] = {
#include "gm_sub/00913530.inc"
};
f32 gEmAreaPoints_Melynx_4_0[1][3] = {
#include "gm_sub/00913550.inc"
};
f32 gEmAreaPoints_Melynx_4_1[1][3] = {
#include "gm_sub/00913560.inc"
};
f32 gEmAreaPoints_Melynx_4_2[1][3] = {
#include "gm_sub/00913570.inc"
};
f32 gEmAreaPoints_Melynx_4_3[1][3] = {
#include "gm_sub/00913580.inc"
};
f32 gEmAreaPoints_Melynx_4_4[1][3] = {
#include "gm_sub/00913590.inc"
};
f32 gEmAreaPoints_Melynx_4_5[1][3] = {
#include "gm_sub/009135A0.inc"
};
f32 gEmAreaPoints_Melynx_4_6[1][3] = {
#include "gm_sub/009135B0.inc"
};
EmAreaPoints gEmAreaPoints_Melynx_4[8] = {
#include "gm_sub/009135C0.inc"
};
f32 gEmAreaPoints_Melynx_5_0[1][3] = {
#include "gm_sub/00913600.inc"
};
f32 gEmAreaPoints_Melynx_5_1[1][3] = {
#include "gm_sub/00913610.inc"
};
f32 gEmAreaPoints_Melynx_5_2[1][3] = {
#include "gm_sub/00913620.inc"
};
f32 gEmAreaPoints_Melynx_5_3[1][3] = {
#include "gm_sub/00913630.inc"
};
f32 gEmAreaPoints_Melynx_5_4[1][3] = {
#include "gm_sub/00913640.inc"
};
f32 gEmAreaPoints_Melynx_5_5[1][3] = {
#include "gm_sub/00913650.inc"
};
f32 gEmAreaPoints_Melynx_5_6[1][3] = {
#include "gm_sub/00913660.inc"
};
EmAreaPoints gEmAreaPoints_Melynx_5[8] = {
#include "gm_sub/00913670.inc"
};
void *gEmKindTables_Melynx[7] = {
#include "gm_sub/009136B0.inc"
};
EmMotionRow gEmMotionRows_Melynx[101] = {
#include "gm_sub/009136D0.inc"
};
f32 gEmGoalPoints_Trader[2][3] = {
#include "gm_sub/00913B90.inc"
};
EmMotionRow gEmMotionRows_Trader[101] = {
#include "gm_sub/00913BB0.inc"
};
f32 gEmAreaPoints_Apceros_1_0[5][3] = {
#include "gm_sub/00914070.inc"
};
f32 gEmAreaPoints_Apceros_1_1[5][3] = {
#include "gm_sub/009140B0.inc"
};
EmAreaPoints gEmAreaPoints_Apceros_1[3] = {
#include "gm_sub/009140F0.inc"
};
EmAreaPoints gEmAreaPoints_Apceros_3[1] = {
#include "gm_sub/00914108.inc"
};
EmAreaPoints gEmAreaPoints_Apceros_4[1] = {
#include "gm_sub/00914110.inc"
};
EmAreaPoints gEmAreaPoints_Apceros_5[1] = {
#include "gm_sub/00914118.inc"
};
void *gEmKindTables_Apceros[7] = {
#include "gm_sub/00914120.inc"
};
f32 gEmGoalPoints_Apceros[30][3] = {
#include "gm_sub/00914140.inc"
};
EmMotionRow gEmMotionRows_Apceros[101] = {
#include "gm_sub/009142B0.inc"
};
f32 gEmAreaPoints_Diablos_1_0[4][3] = {
#include "gm_sub/00914770.inc"
};
f32 gEmAreaPoints_Diablos_1_1[4][3] = {
#include "gm_sub/009147A0.inc"
};
f32 gEmAreaPoints_Diablos_1_2[2][3] = {
#include "gm_sub/009147D0.inc"
};
f32 gEmAreaPoints_Diablos_1_3[4][3] = {
#include "gm_sub/009147F0.inc"
};
f32 gEmAreaPoints_Diablos_1_4[4][3] = {
#include "gm_sub/00914820.inc"
};
f32 gEmAreaPoints_Diablos_1_5[4][3] = {
#include "gm_sub/00914850.inc"
};
f32 gEmAreaPoints_Diablos_1_6[4][3] = {
#include "gm_sub/00914880.inc"
};
EmAreaPoints gEmAreaPoints_Diablos_1[8] = {
#include "gm_sub/009148B0.inc"
};
EmAreaPoints gEmAreaPoints_Diablos_3[1] = {
#include "gm_sub/009148F0.inc"
};
EmAreaPoints gEmAreaPoints_Diablos_4[1] = {
#include "gm_sub/009148F8.inc"
};
EmAreaPoints gEmAreaPoints_Diablos_5[2] = {
#include "gm_sub/00914900.inc"
};
EmRoutePt gEmRouteSets_Diablos_0_0[2] = {
#include "gm_sub/00914910.inc"
};
EmRoutePt gEmRouteSets_Diablos_0_1[2] = {
#include "gm_sub/00914940.inc"
};
EmRoute gEmRouteSets_Diablos_0[2] = {
#include "gm_sub/00914970.inc"
};
EmRoutePt gEmRouteSets_Diablos_1_0[3] = {
#include "gm_sub/009149A0.inc"
};
EmRoutePt gEmRouteSets_Diablos_1_1[3] = {
#include "gm_sub/009149E0.inc"
};
EmRoutePt gEmRouteSets_Diablos_1_2[3] = {
#include "gm_sub/00914A20.inc"
};
EmRoutePt gEmRouteSets_Diablos_1_3[3] = {
#include "gm_sub/00914A60.inc"
};
EmRoute gEmRouteSets_Diablos_1[4] = {
#include "gm_sub/00914AA0.inc"
};
EmRoutePt gEmRouteSets_Diablos_2_0[2] = {
#include "gm_sub/00914B00.inc"
};
EmRoutePt gEmRouteSets_Diablos_2_1[2] = {
#include "gm_sub/00914B30.inc"
};
EmRoute gEmRouteSets_Diablos_2[2] = {
#include "gm_sub/00914B60.inc"
};
EmRoutePt gEmRouteSets_Diablos_3_0[2] = {
#include "gm_sub/00914B90.inc"
};
EmRoutePt gEmRouteSets_Diablos_3_1[2] = {
#include "gm_sub/00914BC0.inc"
};
EmRoutePt gEmRouteSets_Diablos_3_2[2] = {
#include "gm_sub/00914BF0.inc"
};
EmRoutePt gEmRouteSets_Diablos_3_3[2] = {
#include "gm_sub/00914C20.inc"
};
EmRoute gEmRouteSets_Diablos_3[4] = {
#include "gm_sub/00914C50.inc"
};
EmRoutePt gEmRouteSets_Diablos_4_0[2] = {
#include "gm_sub/00914CB0.inc"
};
EmRoutePt gEmRouteSets_Diablos_4_1[2] = {
#include "gm_sub/00914CE0.inc"
};
EmRoute gEmRouteSets_Diablos_4[2] = {
#include "gm_sub/00914D10.inc"
};
EmRoutePt gEmRouteSets_Diablos_5_0[2] = {
#include "gm_sub/00914D40.inc"
};
EmRoutePt gEmRouteSets_Diablos_5_1[2] = {
#include "gm_sub/00914D70.inc"
};
EmRoute gEmRouteSets_Diablos_5[2] = {
#include "gm_sub/00914DA0.inc"
};
EmRouteSet gEmRouteSets_Diablos[13] = {
#include "gm_sub/00914DD0.inc"
};
void *gEmKindTables_Diablos[7] = {
#include "gm_sub/00914E40.inc"
};
EmAreaValue gEmAreaTimer1Values_Diablos[1] = {
#include "gm_sub/00914E60.inc"
};
EmAreaValue gEmAreaTimer2Values_Diablos[7] = {
#include "gm_sub/00914E70.inc"
};
EmMotionRow gEmMotionRows_Diablos[141] = {
#include "gm_sub/00914E90.inc"
};
f32 gEmAreaPoints_Khezu_1_0[5][3] = {
#include "gm_sub/00915530.inc"
};
f32 gEmAreaPoints_Khezu_1_1[4][3] = {
#include "gm_sub/00915570.inc"
};
f32 gEmAreaPoints_Khezu_1_2[4][3] = {
#include "gm_sub/009155A0.inc"
};
f32 gEmAreaPoints_Khezu_1_3[4][3] = {
#include "gm_sub/009155D0.inc"
};
f32 gEmAreaPoints_Khezu_1_4[4][3] = {
#include "gm_sub/00915600.inc"
};
f32 gEmAreaPoints_Khezu_1_5[5][3] = {
#include "gm_sub/00915630.inc"
};
f32 gEmAreaPoints_Khezu_1_6[6][3] = {
#include "gm_sub/00915670.inc"
};
f32 gEmAreaPoints_Khezu_1_7[6][3] = {
#include "gm_sub/009156C0.inc"
};
f32 gEmAreaPoints_Khezu_1_8[5][3] = {
#include "gm_sub/00915710.inc"
};
f32 gEmAreaPoints_Khezu_1_9[4][3] = {
#include "gm_sub/00915750.inc"
};
f32 gEmAreaPoints_Khezu_1_10[8][3] = {
#include "gm_sub/00915780.inc"
};
f32 gEmAreaPoints_Khezu_1_11[2][3] = {
#include "gm_sub/009157E0.inc"
};
EmAreaPoints gEmAreaPoints_Khezu_1[13] = {
#include "gm_sub/00915800.inc"
};
f32 gEmAreaPoints_Khezu_3_0[1][3] = {
#include "gm_sub/00915868.inc"
};
f32 gEmAreaPoints_Khezu_3_1[1][3] = {
#include "gm_sub/00915878.inc"
};
f32 gEmAreaPoints_Khezu_3_2[1][3] = {
#include "gm_sub/00915888.inc"
};
f32 gEmAreaPoints_Khezu_3_3[1][3] = {
#include "gm_sub/00915898.inc"
};
f32 gEmAreaPoints_Khezu_3_4[1][3] = {
#include "gm_sub/009158A8.inc"
};
f32 gEmAreaPoints_Khezu_3_5[1][3] = {
#include "gm_sub/009158B8.inc"
};
EmAreaPoints gEmAreaPoints_Khezu_3[7] = {
#include "gm_sub/009158D0.inc"
};
f32 gEmAreaPoints_Khezu_4_0[1][3] = {
#include "gm_sub/00915908.inc"
};
EmAreaPoints gEmAreaPoints_Khezu_4[2] = {
#include "gm_sub/00915920.inc"
};
EmAreaPoints gEmAreaPoints_Khezu_5[2] = {
#include "gm_sub/00915930.inc"
};
EmRoutePt gEmRouteSets_Khezu_0_0[3] = {
#include "gm_sub/00915940.inc"
};
EmRoute gEmRouteSets_Khezu_0[1] = {
#include "gm_sub/00915980.inc"
};
EmRoutePt gEmRouteSets_Khezu_1_0[2] = {
#include "gm_sub/009159A0.inc"
};
EmRoute gEmRouteSets_Khezu_1[1] = {
#include "gm_sub/009159D0.inc"
};
EmRoutePt gEmRouteSets_Khezu_2_0[3] = {
#include "gm_sub/009159F0.inc"
};
EmRoute gEmRouteSets_Khezu_2[1] = {
#include "gm_sub/00915A30.inc"
};
EmRouteSet gEmRouteSets_Khezu[4] = {
#include "gm_sub/00915A50.inc"
};
void *gEmKindTables_Khezu[7] = {
#include "gm_sub/00915A70.inc"
};
EmAreaValue gEmAreaTimer1Values_Khezu[1] = {
#include "gm_sub/00915A90.inc"
};
EmAreaValue gEmAreaTimer2Values_Khezu[12] = {
#include "gm_sub/00915AA0.inc"
};
s16 gEmIdSets_Khezu_0_0_0[8] = {
#include "gm_sub/00915AD0.inc"
};
s16 gEmIdSets_Khezu_0_0_1[8] = {
#include "gm_sub/00915AE0.inc"
};
s16 gEmIdSets_Khezu_0_0_2[8] = {
#include "gm_sub/00915AF0.inc"
};
s16 gEmIdSets_Khezu_0_0_3[8] = {
#include "gm_sub/00915B00.inc"
};
s16 gEmIdSets_Khezu_0_0_4[8] = {
#include "gm_sub/00915B10.inc"
};
s16 gEmIdSets_Khezu_0_0_5[8] = {
#include "gm_sub/00915B20.inc"
};
EmNextAreaEntry gEmIdSets_Khezu_0_0[7] = {
#include "gm_sub/00915B30.inc"
};
EmNextAreaSet gEmIdSets_Khezu_0[1] = {
#include "gm_sub/00915B68.inc"
};
s16 gEmIdSets_Khezu_11_0_0[8] = {
#include "gm_sub/00915B70.inc"
};
s16 gEmIdSets_Khezu_11_0_1[8] = {
#include "gm_sub/00915B80.inc"
};
s16 gEmIdSets_Khezu_11_0_2[8] = {
#include "gm_sub/00915B90.inc"
};
s16 gEmIdSets_Khezu_11_0_3[8] = {
#include "gm_sub/00915BA0.inc"
};
s16 gEmIdSets_Khezu_11_0_4[8] = {
#include "gm_sub/00915BB0.inc"
};
s16 gEmIdSets_Khezu_11_0_5[8] = {
#include "gm_sub/00915BC0.inc"
};
s16 gEmIdSets_Khezu_11_0_6[8] = {
#include "gm_sub/00915BD0.inc"
};
EmNextAreaEntry gEmIdSets_Khezu_11_0[8] = {
#include "gm_sub/00915BE0.inc"
};
EmNextAreaSet gEmIdSets_Khezu_11[1] = {
#include "gm_sub/00915C20.inc"
};
EmNextAreaSet *gEmIdSets_Khezu[MAP_COUNT] = {
#include "gm_sub/00915C30.inc"
};
EmMotionRow gEmMotionRows_Khezu[141] = {
#include "gm_sub/00915CA0.inc"
};
EmMotionRow gEmMotionRows_Genprey[101] = {
#include "gm_sub/00916340.inc"
};
f32 gEmAreaPoints_Gravios_1_0[8][3] = {
#include "gm_sub/00916800.inc"
};
f32 gEmAreaPoints_Gravios_1_1[5][3] = {
#include "gm_sub/00916860.inc"
};
f32 gEmAreaPoints_Gravios_1_2[4][3] = {
#include "gm_sub/009168A0.inc"
};
f32 gEmAreaPoints_Gravios_1_3[5][3] = {
#include "gm_sub/009168D0.inc"
};
f32 gEmAreaPoints_Gravios_1_4[5][3] = {
#include "gm_sub/00916910.inc"
};
f32 gEmAreaPoints_Gravios_1_5[4][3] = {
#include "gm_sub/00916950.inc"
};
f32 gEmAreaPoints_Gravios_1_6[2][3] = {
#include "gm_sub/00916980.inc"
};
f32 gEmAreaPoints_Gravios_1_7[4][3] = {
#include "gm_sub/009169A0.inc"
};
f32 gEmAreaPoints_Gravios_1_8[8][3] = {
#include "gm_sub/009169D0.inc"
};
f32 gEmAreaPoints_Gravios_1_9[4][3] = {
#include "gm_sub/00916A30.inc"
};
EmAreaPoints gEmAreaPoints_Gravios_1[11] = {
#include "gm_sub/00916A60.inc"
};
EmAreaPoints gEmAreaPoints_Gravios_3[1] = {
#include "gm_sub/00916AB8.inc"
};
EmAreaPoints gEmAreaPoints_Gravios_4[1] = {
#include "gm_sub/00916AC0.inc"
};
EmAreaPoints gEmAreaPoints_Gravios_5[1] = {
#include "gm_sub/00916AC8.inc"
};
EmRoutePt gEmRouteSets_Gravios_0_0[3] = {
#include "gm_sub/00916AD0.inc"
};
EmRoutePt gEmRouteSets_Gravios_0_1[3] = {
#include "gm_sub/00916B10.inc"
};
EmRoute gEmRouteSets_Gravios_0[2] = {
#include "gm_sub/00916B50.inc"
};
EmRoutePt gEmRouteSets_Gravios_1_0[3] = {
#include "gm_sub/00916B80.inc"
};
EmRoutePt gEmRouteSets_Gravios_1_1[3] = {
#include "gm_sub/00916BC0.inc"
};
EmRoute gEmRouteSets_Gravios_1[2] = {
#include "gm_sub/00916C00.inc"
};
EmRoutePt gEmRouteSets_Gravios_2_0[2] = {
#include "gm_sub/00916C30.inc"
};
EmRoutePt gEmRouteSets_Gravios_2_1[2] = {
#include "gm_sub/00916C60.inc"
};
EmRoute gEmRouteSets_Gravios_2[2] = {
#include "gm_sub/00916C90.inc"
};
EmRoutePt gEmRouteSets_Gravios_3_0[2] = {
#include "gm_sub/00916CC0.inc"
};
EmRoutePt gEmRouteSets_Gravios_3_1[2] = {
#include "gm_sub/00916CF0.inc"
};
EmRoute gEmRouteSets_Gravios_3[2] = {
#include "gm_sub/00916D20.inc"
};
EmRoutePt gEmRouteSets_Gravios_4_0[3] = {
#include "gm_sub/00916D50.inc"
};
EmRoutePt gEmRouteSets_Gravios_4_1[3] = {
#include "gm_sub/00916D90.inc"
};
EmRoute gEmRouteSets_Gravios_4[2] = {
#include "gm_sub/00916DD0.inc"
};
EmRoutePt gEmRouteSets_Gravios_5_0[3] = {
#include "gm_sub/00916E00.inc"
};
EmRoutePt gEmRouteSets_Gravios_5_1[3] = {
#include "gm_sub/00916E40.inc"
};
EmRoute gEmRouteSets_Gravios_5[2] = {
#include "gm_sub/00916E80.inc"
};
EmRoutePt gEmRouteSets_Gravios_6_0[2] = {
#include "gm_sub/00916EB0.inc"
};
EmRoutePt gEmRouteSets_Gravios_6_1[2] = {
#include "gm_sub/00916EE0.inc"
};
EmRoute gEmRouteSets_Gravios_6[2] = {
#include "gm_sub/00916F10.inc"
};
EmRoutePt gEmRouteSets_Gravios_7_0[4] = {
#include "gm_sub/00916F40.inc"
};
EmRoutePt gEmRouteSets_Gravios_7_1[4] = {
#include "gm_sub/00916F90.inc"
};
EmRoute gEmRouteSets_Gravios_7[2] = {
#include "gm_sub/00916FE0.inc"
};
EmRoutePt gEmRouteSets_Gravios_8_0[2] = {
#include "gm_sub/00917010.inc"
};
EmRoutePt gEmRouteSets_Gravios_8_1[2] = {
#include "gm_sub/00917040.inc"
};
EmRoute gEmRouteSets_Gravios_8[2] = {
#include "gm_sub/00917070.inc"
};
EmRouteSet gEmRouteSets_Gravios[10] = {
#include "gm_sub/009170A0.inc"
};
void *gEmKindTables_Gravios[7] = {
#include "gm_sub/009170F0.inc"
};
EmAreaValue gEmAreaTimer1Values_Gravios[1] = {
#include "gm_sub/00917110.inc"
};
EmAreaValue gEmAreaTimer2Values_Gravios[5] = {
#include "gm_sub/00917120.inc"
};
EmMotionRow gEmMotionRows_Gravios[141] = {
#include "gm_sub/00917140.inc"
};
EmMotionRow gEmMotionRows_FelyneCart[101] = {
#include "gm_sub/009177E0.inc"
};
EmMotionRow gEmMotionRows_Vespoid[101] = {
#include "gm_sub/00917CA0.inc"
};
f32 gEmAreaPoints_YianKutKu_1_0[4][3] = {
#include "gm_sub/00918160.inc"
};
f32 gEmAreaPoints_YianKutKu_1_1[4][3] = {
#include "gm_sub/00918190.inc"
};
f32 gEmAreaPoints_YianKutKu_1_2[6][3] = {
#include "gm_sub/009181C0.inc"
};
f32 gEmAreaPoints_YianKutKu_1_3[2][3] = {
#include "gm_sub/00918210.inc"
};
f32 gEmAreaPoints_YianKutKu_1_4[8][3] = {
#include "gm_sub/00918230.inc"
};
f32 gEmAreaPoints_YianKutKu_1_5[2][3] = {
#include "gm_sub/00918290.inc"
};
f32 gEmAreaPoints_YianKutKu_1_6[6][3] = {
#include "gm_sub/009182B0.inc"
};
f32 gEmAreaPoints_YianKutKu_1_7[4][3] = {
#include "gm_sub/00918300.inc"
};
f32 gEmAreaPoints_YianKutKu_1_8[4][3] = {
#include "gm_sub/00918330.inc"
};
f32 gEmAreaPoints_YianKutKu_1_9[4][3] = {
#include "gm_sub/00918360.inc"
};
f32 gEmAreaPoints_YianKutKu_1_10[2][3] = {
#include "gm_sub/00918390.inc"
};
f32 gEmAreaPoints_YianKutKu_1_11[2][3] = {
#include "gm_sub/009183B0.inc"
};
f32 gEmAreaPoints_YianKutKu_1_12[8][3] = {
#include "gm_sub/009183D0.inc"
};
f32 gEmAreaPoints_YianKutKu_1_13[2][3] = {
#include "gm_sub/00918430.inc"
};
EmAreaPoints gEmAreaPoints_YianKutKu_1[16] = {
#include "gm_sub/00918450.inc"
};
f32 gEmAreaPoints_YianKutKu_3_0[2][3] = {
#include "gm_sub/009184D0.inc"
};
f32 gEmAreaPoints_YianKutKu_3_1[2][3] = {
#include "gm_sub/009184F0.inc"
};
f32 gEmAreaPoints_YianKutKu_3_2[2][3] = {
#include "gm_sub/00918510.inc"
};
f32 gEmAreaPoints_YianKutKu_3_3[2][3] = {
#include "gm_sub/00918530.inc"
};
f32 gEmAreaPoints_YianKutKu_3_4[2][3] = {
#include "gm_sub/00918550.inc"
};
f32 gEmAreaPoints_YianKutKu_3_5[2][3] = {
#include "gm_sub/00918570.inc"
};
f32 gEmAreaPoints_YianKutKu_3_6[2][3] = {
#include "gm_sub/00918590.inc"
};
f32 gEmAreaPoints_YianKutKu_3_7[2][3] = {
#include "gm_sub/009185B0.inc"
};
f32 gEmAreaPoints_YianKutKu_3_8[2][3] = {
#include "gm_sub/009185D0.inc"
};
f32 gEmAreaPoints_YianKutKu_3_9[2][3] = {
#include "gm_sub/009185F0.inc"
};
f32 gEmAreaPoints_YianKutKu_3_10[2][3] = {
#include "gm_sub/00918610.inc"
};
f32 gEmAreaPoints_YianKutKu_3_17[2][3] = {
#include "gm_sub/00918630.inc"
};
f32 gEmAreaPoints_YianKutKu_3_18[2][3] = {
#include "gm_sub/00918650.inc"
};
f32 gEmAreaPoints_YianKutKu_3_19[2][3] = {
#include "gm_sub/00918670.inc"
};
f32 gEmAreaPoints_YianKutKu_3_20[2][3] = {
#include "gm_sub/00918690.inc"
};
f32 gEmAreaPoints_YianKutKu_3_21[2][3] = {
#include "gm_sub/009186B0.inc"
};
f32 gEmAreaPoints_YianKutKu_3_22[2][3] = {
#include "gm_sub/009186D0.inc"
};
f32 gEmAreaPoints_YianKutKu_3_23[2][3] = {
#include "gm_sub/009186F0.inc"
};
EmAreaPoints gEmAreaPoints_YianKutKu_3[32] = {
#include "gm_sub/00918710.inc"
};
f32 gEmAreaPoints_YianKutKu_4_0[1][3] = {
#include "gm_sub/00918810.inc"
};
f32 gEmAreaPoints_YianKutKu_4_1[1][3] = {
#include "gm_sub/00918820.inc"
};
f32 gEmAreaPoints_YianKutKu_4_2[1][3] = {
#include "gm_sub/00918830.inc"
};
f32 gEmAreaPoints_YianKutKu_4_3[1][3] = {
#include "gm_sub/00918840.inc"
};
f32 gEmAreaPoints_YianKutKu_4_4[1][3] = {
#include "gm_sub/00918850.inc"
};
f32 gEmAreaPoints_YianKutKu_4_5[1][3] = {
#include "gm_sub/00918860.inc"
};
f32 gEmAreaPoints_YianKutKu_4_6[1][3] = {
#include "gm_sub/00918870.inc"
};
f32 gEmAreaPoints_YianKutKu_4_7[1][3] = {
#include "gm_sub/00918880.inc"
};
f32 gEmAreaPoints_YianKutKu_4_8[1][3] = {
#include "gm_sub/00918890.inc"
};
f32 gEmAreaPoints_YianKutKu_4_9[1][3] = {
#include "gm_sub/009188A0.inc"
};
f32 gEmAreaPoints_YianKutKu_4_10[1][3] = {
#include "gm_sub/009188B0.inc"
};
f32 gEmAreaPoints_YianKutKu_4_11[1][3] = {
#include "gm_sub/009188C0.inc"
};
EmAreaPoints gEmAreaPoints_YianKutKu_4[13] = {
#include "gm_sub/009188D0.inc"
};
f32 gEmAreaPoints_YianKutKu_5_0[1][3] = {
#include "gm_sub/00918938.inc"
};
f32 gEmAreaPoints_YianKutKu_5_1[1][3] = {
#include "gm_sub/00918948.inc"
};
f32 gEmAreaPoints_YianKutKu_5_2[1][3] = {
#include "gm_sub/00918958.inc"
};
f32 gEmAreaPoints_YianKutKu_5_3[1][3] = {
#include "gm_sub/00918968.inc"
};
f32 gEmAreaPoints_YianKutKu_5_4[1][3] = {
#include "gm_sub/00918978.inc"
};
EmAreaPoints gEmAreaPoints_YianKutKu_5[6] = {
#include "gm_sub/00918990.inc"
};
void *gEmKindTables_YianKutKu[7] = {
#include "gm_sub/009189C0.inc"
};
s16 gEmIdSets_YianKutKu_0_0_0[8] = {
#include "gm_sub/009189E0.inc"
};
s16 gEmIdSets_YianKutKu_0_0_1[8] = {
#include "gm_sub/009189F0.inc"
};
s16 gEmIdSets_YianKutKu_0_0_2[8] = {
#include "gm_sub/00918A00.inc"
};
s16 gEmIdSets_YianKutKu_0_0_3[8] = {
#include "gm_sub/00918A10.inc"
};
s16 gEmIdSets_YianKutKu_0_0_4[8] = {
#include "gm_sub/00918A20.inc"
};
s16 gEmIdSets_YianKutKu_0_0_5[8] = {
#include "gm_sub/00918A30.inc"
};
EmNextAreaEntry gEmIdSets_YianKutKu_0_0[7] = {
#include "gm_sub/00918A40.inc"
};
EmNextAreaSet gEmIdSets_YianKutKu_0[1] = {
#include "gm_sub/00918A78.inc"
};
s16 gEmIdSets_YianKutKu_4_0_0[8] = {
#include "gm_sub/00918A80.inc"
};
s16 gEmIdSets_YianKutKu_4_0_1[8] = {
#include "gm_sub/00918A90.inc"
};
s16 gEmIdSets_YianKutKu_4_0_2[8] = {
#include "gm_sub/00918AA0.inc"
};
s16 gEmIdSets_YianKutKu_4_0_3[8] = {
#include "gm_sub/00918AB0.inc"
};
s16 gEmIdSets_YianKutKu_4_0_4[8] = {
#include "gm_sub/00918AC0.inc"
};
s16 gEmIdSets_YianKutKu_4_0_5[8] = {
#include "gm_sub/00918AD0.inc"
};
s16 gEmIdSets_YianKutKu_4_0_6[8] = {
#include "gm_sub/00918AE0.inc"
};
EmNextAreaEntry gEmIdSets_YianKutKu_4_0[8] = {
#include "gm_sub/00918AF0.inc"
};
EmNextAreaSet gEmIdSets_YianKutKu_4[1] = {
#include "gm_sub/00918B30.inc"
};
s16 gEmIdSets_YianKutKu_6_0_0[8] = {
#include "gm_sub/00918B40.inc"
};
s16 gEmIdSets_YianKutKu_6_0_1[8] = {
#include "gm_sub/00918B50.inc"
};
s16 gEmIdSets_YianKutKu_6_0_2[8] = {
#include "gm_sub/00918B60.inc"
};
s16 gEmIdSets_YianKutKu_6_0_3[8] = {
#include "gm_sub/00918B70.inc"
};
s16 gEmIdSets_YianKutKu_6_0_4[8] = {
#include "gm_sub/00918B80.inc"
};
s16 gEmIdSets_YianKutKu_6_0_5[8] = {
#include "gm_sub/00918B90.inc"
};
s16 gEmIdSets_YianKutKu_6_0_6[8] = {
#include "gm_sub/00918BA0.inc"
};
s16 gEmIdSets_YianKutKu_6_0_7[8] = {
#include "gm_sub/00918BB0.inc"
};
s16 gEmIdSets_YianKutKu_6_0_8[8] = {
#include "gm_sub/00918BC0.inc"
};
s16 gEmIdSets_YianKutKu_6_0_9[8] = {
#include "gm_sub/00918BD0.inc"
};
EmNextAreaEntry gEmIdSets_YianKutKu_6_0[11] = {
#include "gm_sub/00918BE0.inc"
};
EmNextAreaSet gEmIdSets_YianKutKu_6[1] = {
#include "gm_sub/00918C38.inc"
};
EmNextAreaSet *gEmIdSets_Gypceros[MAP_COUNT] = {
#include "gm_sub/00918C40.inc"
};
EmNextAreaSet *gEmIdSets_YianKutKu[MAP_COUNT] = {
#include "gm_sub/00918CB0.inc"
};
EmAreaValue gEmAreaTimer1Values_YianKutKu[1] = {
#include "gm_sub/00918D18.inc"
};
EmAreaValue gEmAreaTimer2Values_YianKutKu[7] = {
#include "gm_sub/00918D20.inc"
};
EmAreaValue gEmAreaTimer2Values_Gypceros[1] = {
#include "gm_sub/00918D40.inc"
};
EmMotionRow gEmMotionRows_Gypceros[112] = {
#include "gm_sub/00918D50.inc"
};
f32 gEmAreaPoints_Plesioth_1_0[9][3] = {
#include "gm_sub/00919290.inc"
};
f32 gEmAreaPoints_Plesioth_1_1[9][3] = {
#include "gm_sub/00919300.inc"
};
f32 gEmAreaPoints_Plesioth_1_2[10][3] = {
#include "gm_sub/00919370.inc"
};
f32 gEmAreaPoints_Plesioth_1_3[10][3] = {
#include "gm_sub/009193F0.inc"
};
EmAreaPoints gEmAreaPoints_Plesioth_1[6] = {
#include "gm_sub/00919470.inc"
};
f32 gEmAreaPoints_Plesioth_2_0[4][3] = {
#include "gm_sub/009194A0.inc"
};
f32 gEmAreaPoints_Plesioth_2_1[4][3] = {
#include "gm_sub/009194D0.inc"
};
f32 gEmAreaPoints_Plesioth_2_2[4][3] = {
#include "gm_sub/00919500.inc"
};
f32 gEmAreaPoints_Plesioth_2_3[4][3] = {
#include "gm_sub/00919530.inc"
};
EmAreaPoints gEmAreaPoints_Plesioth_2[5] = {
#include "gm_sub/00919560.inc"
};
f32 gEmAreaPoints_Plesioth_3_0[1][3] = {
#include "gm_sub/00919588.inc"
};
EmAreaPoints gEmAreaPoints_Plesioth_3[2] = {
#include "gm_sub/009195A0.inc"
};
f32 gEmAreaPoints_Plesioth_4_0[1][3] = {
#include "gm_sub/009195B0.inc"
};
EmAreaPoints gEmAreaPoints_Plesioth_4[2] = {
#include "gm_sub/009195C0.inc"
};
f32 gEmAreaPoints_Plesioth_5_0[1][3] = {
#include "gm_sub/009195D0.inc"
};
EmAreaPoints gEmAreaPoints_Plesioth_5[2] = {
#include "gm_sub/009195E0.inc"
};
EmRoutePt gEmRouteSets_Plesioth_0_0[1] = {
#include "gm_sub/009195F0.inc"
};
EmRoute gEmRouteSets_Plesioth_0[1] = {
#include "gm_sub/00919610.inc"
};
EmRoutePt gEmRouteSets_Plesioth_1_0[1] = {
#include "gm_sub/00919630.inc"
};
EmRoute gEmRouteSets_Plesioth_1[1] = {
#include "gm_sub/00919650.inc"
};
EmRoutePt gEmRouteSets_Plesioth_2_0[3] = {
#include "gm_sub/00919670.inc"
};
EmRoute gEmRouteSets_Plesioth_2[1] = {
#include "gm_sub/009196B0.inc"
};
EmRoutePt gEmRouteSets_Plesioth_3_0[3] = {
#include "gm_sub/009196D0.inc"
};
EmRoute gEmRouteSets_Plesioth_3[1] = {
#include "gm_sub/00919710.inc"
};
EmRouteSet gEmRouteSets_Plesioth[5] = {
#include "gm_sub/00919730.inc"
};
void *gEmKindTables_Plesioth[7] = {
#include "gm_sub/00919760.inc"
};
f32 gEmGoalPoints_Plesioth[1][3] = {
#include "gm_sub/00919780.inc"
};
EmAreaValue gEmAreaTimer1Values_Plesioth[1] = {
#include "gm_sub/00919790.inc"
};
EmAreaValue gEmAreaTimer2Values_Plesioth[3] = {
#include "gm_sub/00919798.inc"
};
EmMotionRow gEmMotionRows_Plesioth[141] = {
#include "gm_sub/009197B0.inc"
};
f32 gEmAreaPoints_Genprey_1_0[4][3] = {
#include "gm_sub/00919E50.inc"
};
f32 gEmAreaPoints_Genprey_1_1[5][3] = {
#include "gm_sub/00919E80.inc"
};
f32 gEmAreaPoints_Genprey_1_2[4][3] = {
#include "gm_sub/00919EC0.inc"
};
f32 gEmAreaPoints_Genprey_1_3[5][3] = {
#include "gm_sub/00919EF0.inc"
};
f32 gEmAreaPoints_Genprey_1_4[4][3] = {
#include "gm_sub/00919F30.inc"
};
f32 gEmAreaPoints_Genprey_1_5[6][3] = {
#include "gm_sub/00919F60.inc"
};
f32 gEmAreaPoints_Genprey_1_6[5][3] = {
#include "gm_sub/00919FB0.inc"
};
f32 gEmAreaPoints_Genprey_1_7[6][3] = {
#include "gm_sub/00919FF0.inc"
};
f32 gEmAreaPoints_Genprey_1_8[5][3] = {
#include "gm_sub/0091A040.inc"
};
f32 gEmAreaPoints_Genprey_1_9[4][3] = {
#include "gm_sub/0091A080.inc"
};
f32 gEmAreaPoints_Genprey_1_10[6][3] = {
#include "gm_sub/0091A0B0.inc"
};
f32 gEmAreaPoints_Genprey_1_11[4][3] = {
#include "gm_sub/0091A100.inc"
};
f32 gEmAreaPoints_Genprey_1_12[2][3] = {
#include "gm_sub/0091A130.inc"
};
f32 gEmAreaPoints_Genprey_1_13[8][3] = {
#include "gm_sub/0091A150.inc"
};
f32 gEmAreaPoints_Genprey_1_14[2][3] = {
#include "gm_sub/0091A1B0.inc"
};
f32 gEmAreaPoints_Genprey_1_15[2][3] = {
#include "gm_sub/0091A1D0.inc"
};
f32 gEmAreaPoints_Genprey_1_16[4][3] = {
#include "gm_sub/0091A1F0.inc"
};
f32 gEmAreaPoints_Genprey_1_17[5][3] = {
#include "gm_sub/0091A220.inc"
};
f32 gEmAreaPoints_Genprey_1_18[6][3] = {
#include "gm_sub/0091A260.inc"
};
f32 gEmAreaPoints_Genprey_1_19[5][3] = {
#include "gm_sub/0091A2B0.inc"
};
f32 gEmAreaPoints_Genprey_1_20[5][3] = {
#include "gm_sub/0091A2F0.inc"
};
f32 gEmAreaPoints_Genprey_1_21[6][3] = {
#include "gm_sub/0091A330.inc"
};
f32 gEmAreaPoints_Genprey_1_22[6][3] = {
#include "gm_sub/0091A380.inc"
};
f32 gEmAreaPoints_Genprey_1_23[6][3] = {
#include "gm_sub/0091A3D0.inc"
};
f32 gEmAreaPoints_Genprey_1_24[4][3] = {
#include "gm_sub/0091A420.inc"
};
f32 gEmAreaPoints_Genprey_1_25[5][3] = {
#include "gm_sub/0091A450.inc"
};
f32 gEmAreaPoints_Genprey_1_26[5][3] = {
#include "gm_sub/0091A490.inc"
};
f32 gEmAreaPoints_Genprey_1_27[8][3] = {
#include "gm_sub/0091A4D0.inc"
};
f32 gEmAreaPoints_Genprey_1_28[8][3] = {
#include "gm_sub/0091A530.inc"
};
f32 gEmAreaPoints_Genprey_1_29[6][3] = {
#include "gm_sub/0091A590.inc"
};
EmAreaPoints gEmAreaPoints_Genprey_1[31] = {
#include "gm_sub/0091A5E0.inc"
};
EmAreaPoints gEmAreaPoints_Genprey_3[1] = {
#include "gm_sub/0091A6D8.inc"
};
EmAreaPoints gEmAreaPoints_Genprey_4[1] = {
#include "gm_sub/0091A6E0.inc"
};
EmAreaPoints gEmAreaPoints_Genprey_5[1] = {
#include "gm_sub/0091A6E8.inc"
};
EmRoutePt gEmRouteSets_Genprey_0_0[10] = {
#include "gm_sub/0091A6F0.inc"
};
EmRoutePt gEmRouteSets_Genprey_0_1[6] = {
#include "gm_sub/0091A7C0.inc"
};
EmRoute gEmRouteSets_Genprey_0[2] = {
#include "gm_sub/0091A840.inc"
};
EmRoutePt gEmRouteSets_Genprey_1_0[5] = {
#include "gm_sub/0091A870.inc"
};
EmRoute gEmRouteSets_Genprey_1[1] = {
#include "gm_sub/0091A8E0.inc"
};
EmRoutePt gEmRouteSets_Genprey_2_0[7] = {
#include "gm_sub/0091A900.inc"
};
EmRoute gEmRouteSets_Genprey_2[1] = {
#include "gm_sub/0091A990.inc"
};
EmRoutePt gEmRouteSets_Genprey_3_0[5] = {
#include "gm_sub/0091A9B0.inc"
};
EmRoutePt gEmRouteSets_Genprey_3_1[4] = {
#include "gm_sub/0091AA20.inc"
};
EmRoute gEmRouteSets_Genprey_3[2] = {
#include "gm_sub/0091AA70.inc"
};
EmRoutePt gEmRouteSets_Genprey_4_0[4] = {
#include "gm_sub/0091AAA0.inc"
};
EmRoutePt gEmRouteSets_Genprey_4_1[3] = {
#include "gm_sub/0091AAF0.inc"
};
EmRoute gEmRouteSets_Genprey_4[2] = {
#include "gm_sub/0091AB30.inc"
};
EmRoutePt gEmRouteSets_Genprey_5_0[4] = {
#include "gm_sub/0091AB60.inc"
};
EmRoute gEmRouteSets_Genprey_5[1] = {
#include "gm_sub/0091ABB0.inc"
};
EmRoutePt gEmRouteSets_Genprey_6_0[5] = {
#include "gm_sub/0091ABD0.inc"
};
EmRoutePt gEmRouteSets_Genprey_6_1[5] = {
#include "gm_sub/0091AC40.inc"
};
EmRoute gEmRouteSets_Genprey_6[2] = {
#include "gm_sub/0091ACB0.inc"
};
EmRoutePt gEmRouteSets_Genprey_7_0[4] = {
#include "gm_sub/0091ACE0.inc"
};
EmRoutePt gEmRouteSets_Genprey_7_1[4] = {
#include "gm_sub/0091AD30.inc"
};
EmRoute gEmRouteSets_Genprey_7[2] = {
#include "gm_sub/0091AD80.inc"
};
EmRoutePt gEmRouteSets_Genprey_8_0[4] = {
#include "gm_sub/0091ADB0.inc"
};
EmRoutePt gEmRouteSets_Genprey_8_1[4] = {
#include "gm_sub/0091AE00.inc"
};
EmRoute gEmRouteSets_Genprey_8[2] = {
#include "gm_sub/0091AE50.inc"
};
EmRoutePt gEmRouteSets_Genprey_9_0[4] = {
#include "gm_sub/0091AE80.inc"
};
EmRoute gEmRouteSets_Genprey_9[1] = {
#include "gm_sub/0091AED0.inc"
};
EmRoutePt gEmRouteSets_Genprey_10_0[2] = {
#include "gm_sub/0091AEF0.inc"
};
EmRoute gEmRouteSets_Genprey_10[1] = {
#include "gm_sub/0091AF20.inc"
};
EmRoutePt gEmRouteSets_Genprey_11_0[9] = {
#include "gm_sub/0091AF40.inc"
};
EmRoute gEmRouteSets_Genprey_11[1] = {
#include "gm_sub/0091B000.inc"
};
EmRoutePt gEmRouteSets_Genprey_12_0[4] = {
#include "gm_sub/0091B020.inc"
};
EmRoute gEmRouteSets_Genprey_12[1] = {
#include "gm_sub/0091B070.inc"
};
EmRoutePt gEmRouteSets_Genprey_13_0[3] = {
#include "gm_sub/0091B090.inc"
};
EmRoute gEmRouteSets_Genprey_13[1] = {
#include "gm_sub/0091B0D0.inc"
};
EmRoutePt gEmRouteSets_Genprey_14_0[4] = {
#include "gm_sub/0091B0F0.inc"
};
EmRoute gEmRouteSets_Genprey_14[1] = {
#include "gm_sub/0091B140.inc"
};
EmRoutePt gEmRouteSets_Genprey_15_0[4] = {
#include "gm_sub/0091B160.inc"
};
EmRoutePt gEmRouteSets_Genprey_15_1[4] = {
#include "gm_sub/0091B1B0.inc"
};
EmRoute gEmRouteSets_Genprey_15[2] = {
#include "gm_sub/0091B200.inc"
};
EmRoutePt gEmRouteSets_Genprey_16_0[4] = {
#include "gm_sub/0091B230.inc"
};
EmRoute gEmRouteSets_Genprey_16[1] = {
#include "gm_sub/0091B280.inc"
};
EmRoutePt gEmRouteSets_Genprey_17_0[3] = {
#include "gm_sub/0091B2A0.inc"
};
EmRoutePt gEmRouteSets_Genprey_17_1[3] = {
#include "gm_sub/0091B2E0.inc"
};
EmRoute gEmRouteSets_Genprey_17[2] = {
#include "gm_sub/0091B320.inc"
};
EmRoutePt gEmRouteSets_Genprey_18_0[5] = {
#include "gm_sub/0091B350.inc"
};
EmRoute gEmRouteSets_Genprey_18[1] = {
#include "gm_sub/0091B3C0.inc"
};
EmRoutePt gEmRouteSets_Genprey_19_0[3] = {
#include "gm_sub/0091B3E0.inc"
};
EmRoutePt gEmRouteSets_Genprey_19_1[3] = {
#include "gm_sub/0091B420.inc"
};
EmRoute gEmRouteSets_Genprey_19[2] = {
#include "gm_sub/0091B460.inc"
};
EmRoutePt gEmRouteSets_Genprey_20_0[4] = {
#include "gm_sub/0091B490.inc"
};
EmRoutePt gEmRouteSets_Genprey_20_1[4] = {
#include "gm_sub/0091B4E0.inc"
};
EmRoute gEmRouteSets_Genprey_20[2] = {
#include "gm_sub/0091B530.inc"
};
EmRoutePt gEmRouteSets_Genprey_21_0[4] = {
#include "gm_sub/0091B560.inc"
};
EmRoutePt gEmRouteSets_Genprey_21_1[4] = {
#include "gm_sub/0091B5B0.inc"
};
EmRoute gEmRouteSets_Genprey_21[2] = {
#include "gm_sub/0091B600.inc"
};
EmRoutePt gEmRouteSets_Genprey_22_0[2] = {
#include "gm_sub/0091B630.inc"
};
EmRoutePt gEmRouteSets_Genprey_22_1[2] = {
#include "gm_sub/0091B660.inc"
};
EmRoute gEmRouteSets_Genprey_22[2] = {
#include "gm_sub/0091B690.inc"
};
EmRoutePt gEmRouteSets_Genprey_23_0[3] = {
#include "gm_sub/0091B6C0.inc"
};
EmRoutePt gEmRouteSets_Genprey_23_1[4] = {
#include "gm_sub/0091B700.inc"
};
EmRoute gEmRouteSets_Genprey_23[2] = {
#include "gm_sub/0091B750.inc"
};
EmRoutePt gEmRouteSets_Genprey_24_0[3] = {
#include "gm_sub/0091B780.inc"
};
EmRoutePt gEmRouteSets_Genprey_24_1[4] = {
#include "gm_sub/0091B7C0.inc"
};
EmRoute gEmRouteSets_Genprey_24[2] = {
#include "gm_sub/0091B810.inc"
};
EmRoutePt gEmRouteSets_Genprey_25_0[5] = {
#include "gm_sub/0091B840.inc"
};
EmRoutePt gEmRouteSets_Genprey_25_1[4] = {
#include "gm_sub/0091B8B0.inc"
};
EmRoute gEmRouteSets_Genprey_25[2] = {
#include "gm_sub/0091B900.inc"
};
EmRoutePt gEmRouteSets_Genprey_26_0[4] = {
#include "gm_sub/0091B930.inc"
};
EmRoutePt gEmRouteSets_Genprey_26_1[4] = {
#include "gm_sub/0091B980.inc"
};
EmRoute gEmRouteSets_Genprey_26[2] = {
#include "gm_sub/0091B9D0.inc"
};
EmRoutePt gEmRouteSets_Genprey_27_0[4] = {
#include "gm_sub/0091BA00.inc"
};
EmRoutePt gEmRouteSets_Genprey_27_1[4] = {
#include "gm_sub/0091BA50.inc"
};
EmRoute gEmRouteSets_Genprey_27[2] = {
#include "gm_sub/0091BAA0.inc"
};
EmRouteSet gEmRouteSets_Genprey[29] = {
#include "gm_sub/0091BAD0.inc"
};
void *gEmKindTables_Genprey[7] = {
#include "gm_sub/0091BBC0.inc"
};
EmAreaValue gEmAreaTimer1Values_Velocidrome[1] = {
#include "gm_sub/0091BBE0.inc"
};
EmAreaValue gEmAreaTimer2Values_Velocidrome[18] = {
#include "gm_sub/0091BBF0.inc"
};
f32 gEmAreaPoints_29_0_0[1][3] = {
#include "gm_sub/0091BC38.inc"
};
f32 gEmAreaPoints_29_0_1[1][3] = {
#include "gm_sub/0091BC48.inc"
};
f32 gEmAreaPoints_29_0_2[1][3] = {
#include "gm_sub/0091BC58.inc"
};
f32 gEmAreaPoints_29_0_3[1][3] = {
#include "gm_sub/0091BC68.inc"
};
f32 gEmAreaPoints_29_0_4[1][3] = {
#include "gm_sub/0091BC78.inc"
};
f32 gEmAreaPoints_29_0_5[1][3] = {
#include "gm_sub/0091BC88.inc"
};
f32 gEmAreaPoints_29_0_6[1][3] = {
#include "gm_sub/0091BC98.inc"
};
f32 gEmAreaPoints_29_0_7[1][3] = {
#include "gm_sub/0091BCA8.inc"
};
f32 gEmAreaPoints_29_0_8[1][3] = {
#include "gm_sub/0091BCB8.inc"
};
f32 gEmAreaPoints_29_0_9[1][3] = {
#include "gm_sub/0091BCC8.inc"
};
f32 gEmAreaPoints_29_0_10[1][3] = {
#include "gm_sub/0091BCD8.inc"
};
f32 gEmAreaPoints_29_0_11[1][3] = {
#include "gm_sub/0091BCE8.inc"
};
f32 gEmAreaPoints_29_0_12[1][3] = {
#include "gm_sub/0091BCF8.inc"
};
f32 gEmAreaPoints_29_0_13[1][3] = {
#include "gm_sub/0091BD08.inc"
};
f32 gEmAreaPoints_29_0_14[1][3] = {
#include "gm_sub/0091BD18.inc"
};
EmAreaPoints gEmAreaPoints_29_0[16] = {
#include "gm_sub/0091BD30.inc"
};
f32 gEmAreaPoints_29_1_0[7][3] = {
#include "gm_sub/0091BDB0.inc"
};
f32 gEmAreaPoints_29_1_1[1][3] = {
#include "gm_sub/0091BE08.inc"
};
f32 gEmAreaPoints_29_1_4[1][3] = {
#include "gm_sub/0091BE18.inc"
};
f32 gEmAreaPoints_29_1_5[2][3] = {
#include "gm_sub/0091BE30.inc"
};
f32 gEmAreaPoints_29_1_6[2][3] = {
#include "gm_sub/0091BE50.inc"
};
f32 gEmAreaPoints_29_1_8[2][3] = {
#include "gm_sub/0091BE70.inc"
};
f32 gEmAreaPoints_29_1_11[4][3] = {
#include "gm_sub/0091BE90.inc"
};
f32 gEmAreaPoints_29_1_14[1][3] = {
#include "gm_sub/0091BEC0.inc"
};
f32 gEmAreaPoints_29_1_16[4][3] = {
#include "gm_sub/0091BED0.inc"
};
f32 gEmAreaPoints_29_1_17[6][3] = {
#include "gm_sub/0091BF00.inc"
};
f32 gEmAreaPoints_29_1_18[2][3] = {
#include "gm_sub/0091BF50.inc"
};
EmAreaPoints gEmAreaPoints_29_1[20] = {
#include "gm_sub/0091BF70.inc"
};
f32 gEmAreaPoints_29_2_0[4][3] = {
#include "gm_sub/0091C010.inc"
};
f32 gEmAreaPoints_29_2_1[4][3] = {
#include "gm_sub/0091C040.inc"
};
f32 gEmAreaPoints_29_2_4[4][3] = {
#include "gm_sub/0091C070.inc"
};
f32 gEmAreaPoints_29_2_7[4][3] = {
#include "gm_sub/0091C0A0.inc"
};
f32 gEmAreaPoints_29_2_11[4][3] = {
#include "gm_sub/0091C0D0.inc"
};
f32 gEmAreaPoints_29_2_14[4][3] = {
#include "gm_sub/0091C100.inc"
};
EmAreaPoints gEmAreaPoints_29_2[17] = {
#include "gm_sub/0091C130.inc"
};
f32 gEmAreaPoints_29_3_0[1][3] = {
#include "gm_sub/0091C1B8.inc"
};
f32 gEmAreaPoints_29_3_1[1][3] = {
#include "gm_sub/0091C1C8.inc"
};
f32 gEmAreaPoints_29_3_2[1][3] = {
#include "gm_sub/0091C1D8.inc"
};
EmAreaPoints gEmAreaPoints_29_3[4] = {
#include "gm_sub/0091C1F0.inc"
};
f32 gEmAreaPoints_29_4_0[1][3] = {
#include "gm_sub/0091C210.inc"
};
f32 gEmAreaPoints_29_4_1[1][3] = {
#include "gm_sub/0091C220.inc"
};
f32 gEmAreaPoints_29_4_2[1][3] = {
#include "gm_sub/0091C230.inc"
};
f32 gEmAreaPoints_29_4_3[1][3] = {
#include "gm_sub/0091C240.inc"
};
f32 gEmAreaPoints_29_4_4[1][3] = {
#include "gm_sub/0091C250.inc"
};
f32 gEmAreaPoints_29_4_5[1][3] = {
#include "gm_sub/0091C260.inc"
};
f32 gEmAreaPoints_29_4_6[1][3] = {
#include "gm_sub/0091C270.inc"
};
EmAreaPoints gEmAreaPoints_29_4[8] = {
#include "gm_sub/0091C280.inc"
};
f32 gEmAreaPoints_29_5_0[1][3] = {
#include "gm_sub/0091C2C0.inc"
};
f32 gEmAreaPoints_29_5_1[1][3] = {
#include "gm_sub/0091C2D0.inc"
};
f32 gEmAreaPoints_29_5_2[1][3] = {
#include "gm_sub/0091C2E0.inc"
};
f32 gEmAreaPoints_29_5_3[1][3] = {
#include "gm_sub/0091C2F0.inc"
};
f32 gEmAreaPoints_29_5_4[1][3] = {
#include "gm_sub/0091C300.inc"
};
f32 gEmAreaPoints_29_5_5[1][3] = {
#include "gm_sub/0091C310.inc"
};
f32 gEmAreaPoints_29_5_6[1][3] = {
#include "gm_sub/0091C320.inc"
};
EmAreaPoints gEmAreaPoints_29_5[8] = {
#include "gm_sub/0091C330.inc"
};
EmRoutePt gEmRouteSets_29_0_0[6] = {
#include "gm_sub/0091C370.inc"
};
EmRoute gEmRouteSets_29_0[1] = {
#include "gm_sub/0091C3F0.inc"
};
EmRouteSet gEmRouteSets_29[2] = {
#include "gm_sub/0091C410.inc"
};
void *gEmKindTables_29[7] = {
#include "gm_sub/0091C420.inc"
};
f32 gEmGoalPoints_29[5][3] = {
#include "gm_sub/0091C440.inc"
};
EmMotionRow gEmMotionRows_29[101] = {
#include "gm_sub/0091C480.inc"
};
f32 gEmAreaPoints_Kirin_1_0[4][3] = {
#include "gm_sub/0091C940.inc"
};
f32 gEmAreaPoints_Kirin_1_1[6][3] = {
#include "gm_sub/0091C970.inc"
};
f32 gEmAreaPoints_Kirin_1_2[8][3] = {
#include "gm_sub/0091C9C0.inc"
};
f32 gEmAreaPoints_Kirin_1_3[8][3] = {
#include "gm_sub/0091CA20.inc"
};
f32 gEmAreaPoints_Kirin_1_4[4][3] = {
#include "gm_sub/0091CA80.inc"
};
f32 gEmAreaPoints_Kirin_1_5[4][3] = {
#include "gm_sub/0091CAB0.inc"
};
f32 gEmAreaPoints_Kirin_1_6[6][3] = {
#include "gm_sub/0091CAE0.inc"
};
f32 gEmAreaPoints_Kirin_1_7[6][3] = {
#include "gm_sub/0091CB30.inc"
};
f32 gEmAreaPoints_Kirin_1_8[4][3] = {
#include "gm_sub/0091CB80.inc"
};
EmAreaPoints gEmAreaPoints_Kirin_1[10] = {
#include "gm_sub/0091CBB0.inc"
};
f32 gEmAreaPoints_Kirin_3_0[1][3] = {
#include "gm_sub/0091CC00.inc"
};
EmAreaPoints gEmAreaPoints_Kirin_3[2] = {
#include "gm_sub/0091CC10.inc"
};
f32 gEmAreaPoints_Kirin_4_0[1][3] = {
#include "gm_sub/0091CC20.inc"
};
EmAreaPoints gEmAreaPoints_Kirin_4[2] = {
#include "gm_sub/0091CC30.inc"
};
f32 gEmAreaPoints_Kirin_5_0[1][3] = {
#include "gm_sub/0091CC40.inc"
};
EmAreaPoints gEmAreaPoints_Kirin_5[2] = {
#include "gm_sub/0091CC50.inc"
};
EmRoutePt gEmRouteSets_Kirin_0_0[3] = {
#include "gm_sub/0091CC60.inc"
};
EmRoutePt gEmRouteSets_Kirin_0_1[3] = {
#include "gm_sub/0091CCA0.inc"
};
EmRoute gEmRouteSets_Kirin_0[2] = {
#include "gm_sub/0091CCE0.inc"
};
EmRoutePt gEmRouteSets_Kirin_1_0[3] = {
#include "gm_sub/0091CD10.inc"
};
EmRoutePt gEmRouteSets_Kirin_1_1[3] = {
#include "gm_sub/0091CD50.inc"
};
EmRoute gEmRouteSets_Kirin_1[2] = {
#include "gm_sub/0091CD90.inc"
};
EmRoutePt gEmRouteSets_Kirin_2_0[4] = {
#include "gm_sub/0091CDC0.inc"
};
EmRoutePt gEmRouteSets_Kirin_2_1[4] = {
#include "gm_sub/0091CE10.inc"
};
EmRoute gEmRouteSets_Kirin_2[2] = {
#include "gm_sub/0091CE60.inc"
};
EmRoutePt gEmRouteSets_Kirin_3_0[2] = {
#include "gm_sub/0091CE90.inc"
};
EmRoute gEmRouteSets_Kirin_3[1] = {
#include "gm_sub/0091CEC0.inc"
};
EmRoutePt gEmRouteSets_Kirin_4_0[2] = {
#include "gm_sub/0091CEE0.inc"
};
EmRoutePt gEmRouteSets_Kirin_4_1[2] = {
#include "gm_sub/0091CF10.inc"
};
EmRoute gEmRouteSets_Kirin_4[2] = {
#include "gm_sub/0091CF40.inc"
};
EmRoutePt gEmRouteSets_Kirin_5_0[2] = {
#include "gm_sub/0091CF70.inc"
};
EmRoutePt gEmRouteSets_Kirin_5_1[2] = {
#include "gm_sub/0091CFA0.inc"
};
EmRoute gEmRouteSets_Kirin_5[2] = {
#include "gm_sub/0091CFD0.inc"
};
EmRoutePt gEmRouteSets_Kirin_6_0[2] = {
#include "gm_sub/0091D000.inc"
};
EmRoutePt gEmRouteSets_Kirin_6_1[2] = {
#include "gm_sub/0091D030.inc"
};
EmRoute gEmRouteSets_Kirin_6[2] = {
#include "gm_sub/0091D060.inc"
};
EmRouteSet gEmRouteSets_Kirin[8] = {
#include "gm_sub/0091D090.inc"
};
void *gEmKindTables_Kirin[7] = {
#include "gm_sub/0091D0D0.inc"
};
EmAreaValue gEmAreaTimer1Values_Kirin[1] = {
#include "gm_sub/0091D0F0.inc"
};
EmAreaValue gEmAreaTimer2Values_Kirin[5] = {
#include "gm_sub/0091D100.inc"
};
EmMotionRow gEmMotionRows_FatalisB[101] = {
#include "gm_sub/0091D120.inc"
};
f32 gEmAreaPoints_YianGaruga_1_0[12][3] = {
#include "gm_sub/0091D5E0.inc"
};
f32 gEmAreaPoints_YianGaruga_1_1[8][3] = {
#include "gm_sub/0091D670.inc"
};
f32 gEmAreaPoints_YianGaruga_1_2[16][3] = {
#include "gm_sub/0091D6D0.inc"
};
f32 gEmAreaPoints_YianGaruga_1_3[16][3] = {
#include "gm_sub/0091D790.inc"
};
f32 gEmAreaPoints_YianGaruga_1_4[14][3] = {
#include "gm_sub/0091D850.inc"
};
f32 gEmAreaPoints_YianGaruga_1_5[6][3] = {
#include "gm_sub/0091D900.inc"
};
f32 gEmAreaPoints_YianGaruga_1_6[4][3] = {
#include "gm_sub/0091D950.inc"
};
f32 gEmAreaPoints_YianGaruga_1_7[4][3] = {
#include "gm_sub/0091D980.inc"
};
f32 gEmAreaPoints_YianGaruga_1_8[4][3] = {
#include "gm_sub/0091D9B0.inc"
};
f32 gEmAreaPoints_YianGaruga_1_9[8][3] = {
#include "gm_sub/0091D9E0.inc"
};
f32 gEmAreaPoints_YianGaruga_1_10[4][3] = {
#include "gm_sub/0091DA40.inc"
};
f32 gEmAreaPoints_YianGaruga_1_11[2][3] = {
#include "gm_sub/0091DA70.inc"
};
f32 gEmAreaPoints_YianGaruga_1_12[6][3] = {
#include "gm_sub/0091DA90.inc"
};
f32 gEmAreaPoints_YianGaruga_1_13[4][3] = {
#include "gm_sub/0091DAE0.inc"
};
f32 gEmAreaPoints_YianGaruga_1_14[5][3] = {
#include "gm_sub/0091DB10.inc"
};
f32 gEmAreaPoints_YianGaruga_1_15[4][3] = {
#include "gm_sub/0091DB50.inc"
};
f32 gEmAreaPoints_YianGaruga_1_16[1][3] = {
#include "gm_sub/0091DB80.inc"
};
EmAreaPoints gEmAreaPoints_YianGaruga_1[28] = {
#include "gm_sub/0091DB90.inc"
};
f32 gEmAreaPoints_YianGaruga_3_0[2][3] = {
#include "gm_sub/0091DC70.inc"
};
f32 gEmAreaPoints_YianGaruga_3_1[2][3] = {
#include "gm_sub/0091DC90.inc"
};
f32 gEmAreaPoints_YianGaruga_3_2[2][3] = {
#include "gm_sub/0091DCB0.inc"
};
f32 gEmAreaPoints_YianGaruga_3_3[2][3] = {
#include "gm_sub/0091DCD0.inc"
};
f32 gEmAreaPoints_YianGaruga_3_4[2][3] = {
#include "gm_sub/0091DCF0.inc"
};
f32 gEmAreaPoints_YianGaruga_3_5[1][3] = {
#include "gm_sub/0091DD08.inc"
};
f32 gEmAreaPoints_YianGaruga_3_6[1][3] = {
#include "gm_sub/0091DD18.inc"
};
f32 gEmAreaPoints_YianGaruga_3_7[1][3] = {
#include "gm_sub/0091DD28.inc"
};
f32 gEmAreaPoints_YianGaruga_3_8[1][3] = {
#include "gm_sub/0091DD38.inc"
};
f32 gEmAreaPoints_YianGaruga_3_9[1][3] = {
#include "gm_sub/0091DD48.inc"
};
f32 gEmAreaPoints_YianGaruga_3_10[1][3] = {
#include "gm_sub/0091DD58.inc"
};
f32 gEmAreaPoints_YianGaruga_3_11[1][3] = {
#include "gm_sub/0091DD68.inc"
};
f32 gEmAreaPoints_YianGaruga_3_12[1][3] = {
#include "gm_sub/0091DD78.inc"
};
f32 gEmAreaPoints_YianGaruga_3_13[1][3] = {
#include "gm_sub/0091DD88.inc"
};
f32 gEmAreaPoints_YianGaruga_3_14[1][3] = {
#include "gm_sub/0091DD98.inc"
};
EmAreaPoints gEmAreaPoints_YianGaruga_3[25] = {
#include "gm_sub/0091DDB0.inc"
};
f32 gEmAreaPoints_YianGaruga_4_0[1][3] = {
#include "gm_sub/0091DE78.inc"
};
f32 gEmAreaPoints_YianGaruga_4_1[1][3] = {
#include "gm_sub/0091DE88.inc"
};
f32 gEmAreaPoints_YianGaruga_4_2[1][3] = {
#include "gm_sub/0091DE98.inc"
};
f32 gEmAreaPoints_YianGaruga_4_3[1][3] = {
#include "gm_sub/0091DEA8.inc"
};
f32 gEmAreaPoints_YianGaruga_4_4[1][3] = {
#include "gm_sub/0091DEB8.inc"
};
EmAreaPoints gEmAreaPoints_YianGaruga_4[6] = {
#include "gm_sub/0091DED0.inc"
};
f32 gEmAreaPoints_YianGaruga_5_0[1][3] = {
#include "gm_sub/0091DF00.inc"
};
f32 gEmAreaPoints_YianGaruga_5_1[1][3] = {
#include "gm_sub/0091DF10.inc"
};
f32 gEmAreaPoints_YianGaruga_5_2[1][3] = {
#include "gm_sub/0091DF20.inc"
};
f32 gEmAreaPoints_YianGaruga_5_3[1][3] = {
#include "gm_sub/0091DF30.inc"
};
f32 gEmAreaPoints_YianGaruga_5_4[1][3] = {
#include "gm_sub/0091DF40.inc"
};
EmAreaPoints gEmAreaPoints_YianGaruga_5[6] = {
#include "gm_sub/0091DF50.inc"
};
void *gEmKindTables_YianGaruga[7] = {
#include "gm_sub/0091DF80.inc"
};
s16 gEmIdSets_YianGaruga_0_0_0[8] = {
#include "gm_sub/0091DFA0.inc"
};
s16 gEmIdSets_YianGaruga_0_0_1[8] = {
#include "gm_sub/0091DFB0.inc"
};
s16 gEmIdSets_YianGaruga_0_0_2[8] = {
#include "gm_sub/0091DFC0.inc"
};
s16 gEmIdSets_YianGaruga_0_0_3[8] = {
#include "gm_sub/0091DFD0.inc"
};
s16 gEmIdSets_YianGaruga_0_0_4[8] = {
#include "gm_sub/0091DFE0.inc"
};
s16 gEmIdSets_YianGaruga_0_0_5[8] = {
#include "gm_sub/0091DFF0.inc"
};
EmNextAreaEntry gEmIdSets_YianGaruga_0_0[7] = {
#include "gm_sub/0091E000.inc"
};
EmNextAreaSet gEmIdSets_YianGaruga_0[1] = {
#include "gm_sub/0091E038.inc"
};
s16 gEmIdSets_YianGaruga_6_0_0[8] = {
#include "gm_sub/0091E040.inc"
};
s16 gEmIdSets_YianGaruga_6_0_1[8] = {
#include "gm_sub/0091E050.inc"
};
s16 gEmIdSets_YianGaruga_6_0_2[8] = {
#include "gm_sub/0091E060.inc"
};
s16 gEmIdSets_YianGaruga_6_0_3[8] = {
#include "gm_sub/0091E070.inc"
};
s16 gEmIdSets_YianGaruga_6_0_4[8] = {
#include "gm_sub/0091E080.inc"
};
s16 gEmIdSets_YianGaruga_6_0_5[8] = {
#include "gm_sub/0091E090.inc"
};
s16 gEmIdSets_YianGaruga_6_0_6[8] = {
#include "gm_sub/0091E0A0.inc"
};
s16 gEmIdSets_YianGaruga_6_0_7[8] = {
#include "gm_sub/0091E0B0.inc"
};
s16 gEmIdSets_YianGaruga_6_0_8[8] = {
#include "gm_sub/0091E0C0.inc"
};
s16 gEmIdSets_YianGaruga_6_0_9[8] = {
#include "gm_sub/0091E0D0.inc"
};
EmNextAreaEntry gEmIdSets_YianGaruga_6_0[11] = {
#include "gm_sub/0091E0E0.inc"
};
EmNextAreaSet gEmIdSets_YianGaruga_6[1] = {
#include "gm_sub/0091E138.inc"
};
EmNextAreaSet *gEmIdSets_YianGaruga[MAP_COUNT] = {
#include "gm_sub/0091E140.inc"
};
EmAreaValue gEmAreaTimer1Values_YianGaruga[1] = {
#include "gm_sub/0091E1A8.inc"
};
EmAreaValue gEmAreaTimer2Values_YianGaruga[2] = {
#include "gm_sub/0091E1B0.inc"
};
EmMotionRow gEmMotionRows_YianGaruga[112] = {
#include "gm_sub/0091E1C0.inc"
};
EmMotionRow gEmMotionRows_RedKhezu[141] = {
#include "gm_sub/0091E700.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_0[4][3] = {
#include "gm_sub/0091EDA0.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_1[4][3] = {
#include "gm_sub/0091EDD0.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_2[8][3] = {
#include "gm_sub/0091EE00.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_3[4][3] = {
#include "gm_sub/0091EE60.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_4[4][3] = {
#include "gm_sub/0091EE90.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_5[4][3] = {
#include "gm_sub/0091EEC0.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_6[4][3] = {
#include "gm_sub/0091EEF0.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_7[4][3] = {
#include "gm_sub/0091EF20.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_8[4][3] = {
#include "gm_sub/0091EF50.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_9[4][3] = {
#include "gm_sub/0091EF80.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_10[5][3] = {
#include "gm_sub/0091EFB0.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_11[5][3] = {
#include "gm_sub/0091EFF0.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_12[4][3] = {
#include "gm_sub/0091F030.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_13[4][3] = {
#include "gm_sub/0091F060.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_14[5][3] = {
#include "gm_sub/0091F090.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_15[5][3] = {
#include "gm_sub/0091F0D0.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_16[4][3] = {
#include "gm_sub/0091F110.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_17[4][3] = {
#include "gm_sub/0091F140.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_18[5][3] = {
#include "gm_sub/0091F170.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_19[4][3] = {
#include "gm_sub/0091F1B0.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_20[4][3] = {
#include "gm_sub/0091F1E0.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_21[4][3] = {
#include "gm_sub/0091F210.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_22[6][3] = {
#include "gm_sub/0091F240.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_23[6][3] = {
#include "gm_sub/0091F290.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_1_24[4][3] = {
#include "gm_sub/0091F2E0.inc"
};
EmAreaPoints gEmAreaPoints_DaimyoHermitaur_1[26] = {
#include "gm_sub/0091F310.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_3_0[1][3] = {
#include "gm_sub/0091F3E0.inc"
};
EmAreaPoints gEmAreaPoints_DaimyoHermitaur_3[2] = {
#include "gm_sub/0091F3F0.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_4_0[1][3] = {
#include "gm_sub/0091F400.inc"
};
EmAreaPoints gEmAreaPoints_DaimyoHermitaur_4[2] = {
#include "gm_sub/0091F410.inc"
};
f32 gEmAreaPoints_DaimyoHermitaur_5_0[1][3] = {
#include "gm_sub/0091F420.inc"
};
EmAreaPoints gEmAreaPoints_DaimyoHermitaur_5[2] = {
#include "gm_sub/0091F430.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_5_0[3] = {
#include "gm_sub/0091F440.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_5[1] = {
#include "gm_sub/0091F480.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_2_2[4] = {
#include "gm_sub/0091F4A0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_2_0[4] = {
#include "gm_sub/0091F4F0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_2_1[3] = {
#include "gm_sub/0091F540.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_2[3] = {
#include "gm_sub/0091F580.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_3_0[5] = {
#include "gm_sub/0091F5D0.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_3[1] = {
#include "gm_sub/0091F640.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_4_0[5] = {
#include "gm_sub/0091F660.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_4_1[5] = {
#include "gm_sub/0091F6D0.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_4[2] = {
#include "gm_sub/0091F740.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_0_0[4] = {
#include "gm_sub/0091F770.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_0_1[4] = {
#include "gm_sub/0091F7C0.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_0[2] = {
#include "gm_sub/0091F810.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_1_0[4] = {
#include "gm_sub/0091F840.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_1[1] = {
#include "gm_sub/0091F890.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_6_0[2] = {
#include "gm_sub/0091F8B0.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_6[1] = {
#include "gm_sub/0091F8E0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_7_0[3] = {
#include "gm_sub/0091F900.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_7_1[3] = {
#include "gm_sub/0091F940.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_7_2[3] = {
#include "gm_sub/0091F980.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_7[3] = {
#include "gm_sub/0091F9C0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_8_0[5] = {
#include "gm_sub/0091FA10.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_8_1[5] = {
#include "gm_sub/0091FA80.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_8[2] = {
#include "gm_sub/0091FAF0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_9_0[4] = {
#include "gm_sub/0091FB20.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_9[1] = {
#include "gm_sub/0091FB70.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_10_0[3] = {
#include "gm_sub/0091FB90.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_10_1[3] = {
#include "gm_sub/0091FBD0.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_10[2] = {
#include "gm_sub/0091FC10.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_11_0[4] = {
#include "gm_sub/0091FC40.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_11[1] = {
#include "gm_sub/0091FC90.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_12_0[3] = {
#include "gm_sub/0091FCB0.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_12[1] = {
#include "gm_sub/0091FCF0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_13_0[3] = {
#include "gm_sub/0091FD10.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_13_1[3] = {
#include "gm_sub/0091FD50.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_13[2] = {
#include "gm_sub/0091FD90.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_14_0[4] = {
#include "gm_sub/0091FDC0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_14_1[4] = {
#include "gm_sub/0091FE10.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_14[2] = {
#include "gm_sub/0091FE60.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_15_0[4] = {
#include "gm_sub/0091FE90.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_15_1[4] = {
#include "gm_sub/0091FEE0.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_15[2] = {
#include "gm_sub/0091FF30.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_16_0[3] = {
#include "gm_sub/0091FF60.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_16_1[3] = {
#include "gm_sub/0091FFA0.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_16[2] = {
#include "gm_sub/0091FFE0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_17_0[3] = {
#include "gm_sub/00920010.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_17_1[3] = {
#include "gm_sub/00920050.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_17[2] = {
#include "gm_sub/00920090.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_18_0[3] = {
#include "gm_sub/009200C0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_18_1[3] = {
#include "gm_sub/00920100.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_18[2] = {
#include "gm_sub/00920140.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_19_0[4] = {
#include "gm_sub/00920170.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_19[1] = {
#include "gm_sub/009201C0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_20_0[3] = {
#include "gm_sub/009201E0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_20_1[4] = {
#include "gm_sub/00920220.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_20[2] = {
#include "gm_sub/00920270.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_21_0[4] = {
#include "gm_sub/009202A0.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_21[1] = {
#include "gm_sub/009202F0.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_22_0[6] = {
#include "gm_sub/00920310.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_22_1[5] = {
#include "gm_sub/00920390.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_22[2] = {
#include "gm_sub/00920400.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_23_0[4] = {
#include "gm_sub/00920430.inc"
};
EmRoutePt gEmRouteSets_DaimyoHermitaur_23_1[4] = {
#include "gm_sub/00920480.inc"
};
EmRoute gEmRouteSets_DaimyoHermitaur_23[2] = {
#include "gm_sub/009204D0.inc"
};
EmRouteSet gEmRouteSets_DaimyoHermitaur[25] = {
#include "gm_sub/00920500.inc"
};
void *gEmKindTables_DaimyoHermitaur[7] = {
#include "gm_sub/009205D0.inc"
};
EmAreaValue gEmAreaTimer1Values_DaimyoHermitaur[1] = {
#include "gm_sub/009205F0.inc"
};
EmAreaValue gEmAreaTimer2Values_DaimyoHermitaur[12] = {
#include "gm_sub/00920600.inc"
};
EmMotionRow gEmMotionRows_DaimyoHermitaur[101] = {
#include "gm_sub/00920630.inc"
};
f32 gEmAreaPoints_Blangonga_1_0[4][3] = {
#include "gm_sub/00920AF0.inc"
};
f32 gEmAreaPoints_Blangonga_1_1[5][3] = {
#include "gm_sub/00920B20.inc"
};
f32 gEmAreaPoints_Blangonga_1_2[6][3] = {
#include "gm_sub/00920B60.inc"
};
f32 gEmAreaPoints_Blangonga_1_3[8][3] = {
#include "gm_sub/00920BB0.inc"
};
f32 gEmAreaPoints_Blangonga_1_4[5][3] = {
#include "gm_sub/00920C10.inc"
};
f32 gEmAreaPoints_Blangonga_1_5[5][3] = {
#include "gm_sub/00920C50.inc"
};
f32 gEmAreaPoints_Blangonga_1_6[6][3] = {
#include "gm_sub/00920C90.inc"
};
f32 gEmAreaPoints_Blangonga_1_7[5][3] = {
#include "gm_sub/00920CE0.inc"
};
f32 gEmAreaPoints_Blangonga_1_8[5][3] = {
#include "gm_sub/00920D20.inc"
};
f32 gEmAreaPoints_Blangonga_1_9[10][3] = {
#include "gm_sub/00920D60.inc"
};
f32 gEmAreaPoints_Blangonga_1_10[4][3] = {
#include "gm_sub/00920DE0.inc"
};
f32 gEmAreaPoints_Blangonga_1_11[5][3] = {
#include "gm_sub/00920E10.inc"
};
f32 gEmAreaPoints_Blangonga_1_12[4][3] = {
#include "gm_sub/00920E50.inc"
};
f32 gEmAreaPoints_Blangonga_1_13[6][3] = {
#include "gm_sub/00920E80.inc"
};
f32 gEmAreaPoints_Blangonga_1_14[8][3] = {
#include "gm_sub/00920ED0.inc"
};
f32 gEmAreaPoints_Blangonga_1_15[4][3] = {
#include "gm_sub/00920F30.inc"
};
f32 gEmAreaPoints_Blangonga_1_16[4][3] = {
#include "gm_sub/00920F60.inc"
};
f32 gEmAreaPoints_Blangonga_1_17[8][3] = {
#include "gm_sub/00920F90.inc"
};
f32 gEmAreaPoints_Blangonga_1_18[4][3] = {
#include "gm_sub/00920FF0.inc"
};
f32 gEmAreaPoints_Blangonga_1_19[4][3] = {
#include "gm_sub/00921020.inc"
};
f32 gEmAreaPoints_Blangonga_1_20[4][3] = {
#include "gm_sub/00921050.inc"
};
f32 gEmAreaPoints_Blangonga_1_21[5][3] = {
#include "gm_sub/00921080.inc"
};
f32 gEmAreaPoints_Blangonga_1_22[4][3] = {
#include "gm_sub/009210C0.inc"
};
EmAreaPoints gEmAreaPoints_Blangonga_1[24] = {
#include "gm_sub/009210F0.inc"
};
EmAreaPoints gEmAreaPoints_Blangonga_3[1] = {
#include "gm_sub/009211B0.inc"
};
EmAreaPoints gEmAreaPoints_Blangonga_4[1] = {
#include "gm_sub/009211B8.inc"
};
EmAreaPoints gEmAreaPoints_Blangonga_5[2] = {
#include "gm_sub/009211C0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_0_0[3] = {
#include "gm_sub/009211D0.inc"
};
EmRoute gEmRouteSets_Blangonga_0[1] = {
#include "gm_sub/00921210.inc"
};
EmRoutePt gEmRouteSets_Blangonga_1_0[2] = {
#include "gm_sub/00921230.inc"
};
EmRoutePt gEmRouteSets_Blangonga_1_1[3] = {
#include "gm_sub/00921260.inc"
};
EmRoute gEmRouteSets_Blangonga_1[3] = {
#include "gm_sub/009212A0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_2_0[3] = {
#include "gm_sub/009212F0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_2_1[3] = {
#include "gm_sub/00921330.inc"
};
EmRoute gEmRouteSets_Blangonga_2[2] = {
#include "gm_sub/00921370.inc"
};
EmRoutePt gEmRouteSets_Blangonga_3_0[3] = {
#include "gm_sub/009213A0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_3_2[3] = {
#include "gm_sub/009213E0.inc"
};
EmRoute gEmRouteSets_Blangonga_3[3] = {
#include "gm_sub/00921420.inc"
};
EmRoutePt gEmRouteSets_Blangonga_4_0[3] = {
#include "gm_sub/00921470.inc"
};
EmRoutePt gEmRouteSets_Blangonga_4_1[3] = {
#include "gm_sub/009214B0.inc"
};
EmRoute gEmRouteSets_Blangonga_4[3] = {
#include "gm_sub/009214F0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_5_0[4] = {
#include "gm_sub/00921540.inc"
};
EmRoute gEmRouteSets_Blangonga_5[2] = {
#include "gm_sub/00921590.inc"
};
EmRoutePt gEmRouteSets_Blangonga_6_0[2] = {
#include "gm_sub/009215C0.inc"
};
EmRoute gEmRouteSets_Blangonga_6[1] = {
#include "gm_sub/009215F0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_7_0[2] = {
#include "gm_sub/00921610.inc"
};
EmRoute gEmRouteSets_Blangonga_7[1] = {
#include "gm_sub/00921640.inc"
};
EmRoutePt gEmRouteSets_Blangonga_8_0[2] = {
#include "gm_sub/00921660.inc"
};
EmRoutePt gEmRouteSets_Blangonga_8_1[2] = {
#include "gm_sub/00921690.inc"
};
EmRoute gEmRouteSets_Blangonga_8[2] = {
#include "gm_sub/009216C0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_9_0[2] = {
#include "gm_sub/009216F0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_9_1[2] = {
#include "gm_sub/00921720.inc"
};
EmRoute gEmRouteSets_Blangonga_9[2] = {
#include "gm_sub/00921750.inc"
};
EmRoutePt gEmRouteSets_Blangonga_10_0[2] = {
#include "gm_sub/00921780.inc"
};
EmRoutePt gEmRouteSets_Blangonga_10_1[2] = {
#include "gm_sub/009217B0.inc"
};
EmRoute gEmRouteSets_Blangonga_10[2] = {
#include "gm_sub/009217E0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_11_0[4] = {
#include "gm_sub/00921810.inc"
};
EmRoute gEmRouteSets_Blangonga_11[2] = {
#include "gm_sub/00921860.inc"
};
EmRoutePt gEmRouteSets_Blangonga_12_0[1] = {
#include "gm_sub/00921890.inc"
};
EmRoutePt gEmRouteSets_Blangonga_12_1[2] = {
#include "gm_sub/009218B0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_12_2[2] = {
#include "gm_sub/009218E0.inc"
};
EmRoute gEmRouteSets_Blangonga_12[3] = {
#include "gm_sub/00921910.inc"
};
EmRoutePt gEmRouteSets_Blangonga_13_0[3] = {
#include "gm_sub/00921960.inc"
};
EmRoute gEmRouteSets_Blangonga_13[1] = {
#include "gm_sub/009219A0.inc"
};
EmRoutePt gEmRouteSets_Blangonga_14_0[3] = {
#include "gm_sub/009219C0.inc"
};
EmRoute gEmRouteSets_Blangonga_14[1] = {
#include "gm_sub/00921A00.inc"
};
EmRouteSet gEmRouteSets_Blangonga[16] = {
#include "gm_sub/00921A20.inc"
};
void *gEmKindTables_Blangonga[7] = {
#include "gm_sub/00921AA0.inc"
};
EmAreaValue gEmAreaTimer1Values_Blangonga[1] = {
#include "gm_sub/00921AC0.inc"
};
EmAreaValue gEmAreaTimer2Values_Blangonga[18] = {
#include "gm_sub/00921AD0.inc"
};
/* Blangonga's next-area lists (em.h's EmNextAreaSet; Em_NextAreas): gEmIdSets_Blangonga by
 * map, its sets, their entries and the entries' area lists. */
s16 gEmIdSets_Blangonga_4_0_0[8] = {
#include "gm_sub/00921B20.inc"
};
s16 gEmIdSets_Blangonga_4_0_1[8] = {
#include "gm_sub/00921B30.inc"
};
s16 gEmIdSets_Blangonga_4_0_2[8] = {
#include "gm_sub/00921B40.inc"
};
s16 gEmIdSets_Blangonga_4_0_3[8] = {
#include "gm_sub/00921B50.inc"
};
s16 gEmIdSets_Blangonga_4_0_4[8] = {
#include "gm_sub/00921B60.inc"
};
s16 gEmIdSets_Blangonga_4_0_5[8] = {
#include "gm_sub/00921B70.inc"
};
EmNextAreaEntry gEmIdSets_Blangonga_4_0[7] = {
#include "gm_sub/00921B80.inc"
};
EmNextAreaSet gEmIdSets_Blangonga_4[1] = {
#include "gm_sub/00921BB8.inc"
};
s16 gEmIdSets_Blangonga_5_0_0[8] = {
#include "gm_sub/00921BC0.inc"
};
s16 gEmIdSets_Blangonga_5_0_1[8] = {
#include "gm_sub/00921BD0.inc"
};
s16 gEmIdSets_Blangonga_5_0_2[8] = {
#include "gm_sub/00921BE0.inc"
};
s16 gEmIdSets_Blangonga_5_0_3[8] = {
#include "gm_sub/00921BF0.inc"
};
s16 gEmIdSets_Blangonga_5_0_4[8] = {
#include "gm_sub/00921C00.inc"
};
EmNextAreaEntry gEmIdSets_Blangonga_5_0[6] = {
#include "gm_sub/00921C10.inc"
};
EmNextAreaSet gEmIdSets_Blangonga_5[1] = {
#include "gm_sub/00921C40.inc"
};
s16 gEmIdSets_Blangonga_6_0_0[8] = {
#include "gm_sub/00921C50.inc"
};
s16 gEmIdSets_Blangonga_6_0_1[8] = {
#include "gm_sub/00921C60.inc"
};
s16 gEmIdSets_Blangonga_6_0_2[8] = {
#include "gm_sub/00921C70.inc"
};
s16 gEmIdSets_Blangonga_6_0_3[8] = {
#include "gm_sub/00921C80.inc"
};
s16 gEmIdSets_Blangonga_6_0_4[8] = {
#include "gm_sub/00921C90.inc"
};
s16 gEmIdSets_Blangonga_6_0_5[8] = {
#include "gm_sub/00921CA0.inc"
};
EmNextAreaEntry gEmIdSets_Blangonga_6_0[7] = {
#include "gm_sub/00921CB0.inc"
};
EmNextAreaSet gEmIdSets_Blangonga_6[1] = {
#include "gm_sub/00921CE8.inc"
};
s16 gEmIdSets_Blangonga_0_0_0[8] = {
#include "gm_sub/00921CF0.inc"
};
s16 gEmIdSets_Blangonga_0_0_1[8] = {
#include "gm_sub/00921D00.inc"
};
s16 gEmIdSets_Blangonga_0_0_2[8] = {
#include "gm_sub/00921D10.inc"
};
s16 gEmIdSets_Blangonga_0_0_3[8] = {
#include "gm_sub/00921D20.inc"
};
s16 gEmIdSets_Blangonga_0_0_4[8] = {
#include "gm_sub/00921D30.inc"
};
s16 gEmIdSets_Blangonga_0_0_5[8] = {
#include "gm_sub/00921D40.inc"
};
s16 gEmIdSets_Blangonga_0_0_6[8] = {
#include "gm_sub/00921D50.inc"
};
EmNextAreaEntry gEmIdSets_Blangonga_0_0[8] = {
#include "gm_sub/00921D60.inc"
};
EmNextAreaSet gEmIdSets_Blangonga_0[1] = {
#include "gm_sub/00921DA0.inc"
};
EmNextAreaSet *gEmIdSets_Blangonga[MAP_COUNT] = {
#include "gm_sub/00921DB0.inc"
};
EmMotionRow gEmMotionRows_Blangonga[101] = {
#include "gm_sub/00921E20.inc"
};
EmMotionRow gEmMotionRows_Congalala[101] = {
#include "gm_sub/009222E0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_0[4][3] = {
#include "gm_sub/009227A0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_1[1][3] = {
#include "gm_sub/009227D0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_2[4][3] = {
#include "gm_sub/009227E0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_3[4][3] = {
#include "gm_sub/00922810.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_4[2][3] = {
#include "gm_sub/00922840.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_5[6][3] = {
#include "gm_sub/00922860.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_6[2][3] = {
#include "gm_sub/009228B0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_7[5][3] = {
#include "gm_sub/009228D0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_8[2][3] = {
#include "gm_sub/00922910.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_9[1][3] = {
#include "gm_sub/00922928.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_10[4][3] = {
#include "gm_sub/00922940.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_11[4][3] = {
#include "gm_sub/00922970.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_12[4][3] = {
#include "gm_sub/009229A0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_13[6][3] = {
#include "gm_sub/009229D0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_14[2][3] = {
#include "gm_sub/00922A20.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_15[4][3] = {
#include "gm_sub/00922A40.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_16[8][3] = {
#include "gm_sub/00922A70.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_17[6][3] = {
#include "gm_sub/00922AD0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_18[4][3] = {
#include "gm_sub/00922B20.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_19[4][3] = {
#include "gm_sub/00922B50.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_20[4][3] = {
#include "gm_sub/00922B80.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_21[2][3] = {
#include "gm_sub/00922BB0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_22[4][3] = {
#include "gm_sub/00922BD0.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_23[4][3] = {
#include "gm_sub/00922C00.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_24[4][3] = {
#include "gm_sub/00922C30.inc"
};
f32 gEmAreaPoints_KushalaDaora_1_25[4][3] = {
#include "gm_sub/00922C60.inc"
};
EmAreaPoints gEmAreaPoints_KushalaDaora_1[27] = {
#include "gm_sub/00922C90.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_0[1][3] = {
#include "gm_sub/00922D68.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_1[1][3] = {
#include "gm_sub/00922D78.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_2[1][3] = {
#include "gm_sub/00922D88.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_3[1][3] = {
#include "gm_sub/00922D98.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_4[1][3] = {
#include "gm_sub/00922DA8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_5[1][3] = {
#include "gm_sub/00922DB8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_6[1][3] = {
#include "gm_sub/00922DC8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_7[1][3] = {
#include "gm_sub/00922DD8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_8[1][3] = {
#include "gm_sub/00922DE8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_9[1][3] = {
#include "gm_sub/00922DF8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_19[1][3] = {
#include "gm_sub/00922E08.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_20[1][3] = {
#include "gm_sub/00922E18.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_21[1][3] = {
#include "gm_sub/00922E28.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_22[1][3] = {
#include "gm_sub/00922E38.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_23[1][3] = {
#include "gm_sub/00922E48.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_24[1][3] = {
#include "gm_sub/00922E58.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_31[1][3] = {
#include "gm_sub/00922E68.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_32[1][3] = {
#include "gm_sub/00922E78.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_33[1][3] = {
#include "gm_sub/00922E88.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_34[1][3] = {
#include "gm_sub/00922E98.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_35[1][3] = {
#include "gm_sub/00922EA8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_36[1][3] = {
#include "gm_sub/00922EB8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_37[1][3] = {
#include "gm_sub/00922EC8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_38[1][3] = {
#include "gm_sub/00922ED8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_39[1][3] = {
#include "gm_sub/00922EE8.inc"
};
f32 gEmAreaPoints_KushalaDaora_3_40[1][3] = {
#include "gm_sub/00922EF8.inc"
};
EmAreaPoints gEmAreaPoints_KushalaDaora_3[42] = {
#include "gm_sub/00922F10.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_0[1][3] = {
#include "gm_sub/00923060.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_1[1][3] = {
#include "gm_sub/00923070.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_2[1][3] = {
#include "gm_sub/00923080.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_3[1][3] = {
#include "gm_sub/00923090.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_4[1][3] = {
#include "gm_sub/009230A0.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_5[1][3] = {
#include "gm_sub/009230B0.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_6[1][3] = {
#include "gm_sub/009230C0.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_7[1][3] = {
#include "gm_sub/009230D0.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_8[1][3] = {
#include "gm_sub/009230E0.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_9[1][3] = {
#include "gm_sub/009230F0.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_10[1][3] = {
#include "gm_sub/00923100.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_11[1][3] = {
#include "gm_sub/00923110.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_12[1][3] = {
#include "gm_sub/00923120.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_13[1][3] = {
#include "gm_sub/00923130.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_14[1][3] = {
#include "gm_sub/00923140.inc"
};
f32 gEmAreaPoints_KushalaDaora_4_15[1][3] = {
#include "gm_sub/00923150.inc"
};
EmAreaPoints gEmAreaPoints_KushalaDaora_4[17] = {
#include "gm_sub/00923160.inc"
};
f32 gEmAreaPoints_KushalaDaora_5_0[1][3] = {
#include "gm_sub/009231E8.inc"
};
f32 gEmAreaPoints_KushalaDaora_5_1[1][3] = {
#include "gm_sub/009231F8.inc"
};
f32 gEmAreaPoints_KushalaDaora_5_2[1][3] = {
#include "gm_sub/00923208.inc"
};
f32 gEmAreaPoints_KushalaDaora_5_3[1][3] = {
#include "gm_sub/00923218.inc"
};
f32 gEmAreaPoints_KushalaDaora_5_4[1][3] = {
#include "gm_sub/00923228.inc"
};
f32 gEmAreaPoints_KushalaDaora_5_5[1][3] = {
#include "gm_sub/00923238.inc"
};
EmAreaPoints gEmAreaPoints_KushalaDaora_5[7] = {
#include "gm_sub/00923250.inc"
};
void *gEmKindTables_KushalaDaora[6] = {
#include "gm_sub/00923290.inc"
};
EmAreaValue gEmAreaTimer1Values_KushalaDaora[1] = {
#include "gm_sub/009232A8.inc"
};
EmAreaValue gEmAreaTimer2Values_KushalaDaora[13] = {
#include "gm_sub/009232B0.inc"
};
/* KushalaDaora's next-area lists (em.h's EmNextAreaSet; Em_NextAreas): gEmIdSets_KushalaDaora by
 * map, its sets, their entries and the entries' area lists. */
s16 gEmIdSets_KushalaDaora_0_0_0[8] = {
#include "gm_sub/009232F0.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_1[8] = {
#include "gm_sub/00923300.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_2[8] = {
#include "gm_sub/00923310.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_3[8] = {
#include "gm_sub/00923320.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_4[8] = {
#include "gm_sub/00923330.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_5[8] = {
#include "gm_sub/00923340.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_6[8] = {
#include "gm_sub/00923350.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_7[8] = {
#include "gm_sub/00923360.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_8[8] = {
#include "gm_sub/00923370.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_9[8] = {
#include "gm_sub/00923380.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_10[8] = {
#include "gm_sub/00923390.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_11[8] = {
#include "gm_sub/009233A0.inc"
};
s16 gEmIdSets_KushalaDaora_0_0_12[8] = {
#include "gm_sub/009233B0.inc"
};
EmNextAreaEntry gEmIdSets_KushalaDaora_0_0[14] = {
#include "gm_sub/009233C0.inc"
};
EmNextAreaSet gEmIdSets_KushalaDaora_0[1] = {
#include "gm_sub/00923430.inc"
};
s16 gEmIdSets_KushalaDaora_2_1_0[8] = {
#include "gm_sub/00923440.inc"
};
s16 gEmIdSets_KushalaDaora_2_1_1[8] = {
#include "gm_sub/00923450.inc"
};
s16 gEmIdSets_KushalaDaora_2_1_2[8] = {
#include "gm_sub/00923460.inc"
};
s16 gEmIdSets_KushalaDaora_2_1_3[8] = {
#include "gm_sub/00923470.inc"
};
s16 gEmIdSets_KushalaDaora_2_1_4[8] = {
#include "gm_sub/00923480.inc"
};
s16 gEmIdSets_KushalaDaora_2_1_5[8] = {
#include "gm_sub/00923490.inc"
};
s16 gEmIdSets_KushalaDaora_2_1_6[8] = {
#include "gm_sub/009234A0.inc"
};
EmNextAreaEntry gEmIdSets_KushalaDaora_2_1[8] = {
#include "gm_sub/009234B0.inc"
};
s16 gEmIdSets_KushalaDaora_2_0_0[8] = {
#include "gm_sub/009234F0.inc"
};
s16 gEmIdSets_KushalaDaora_2_0_1[8] = {
#include "gm_sub/00923500.inc"
};
s16 gEmIdSets_KushalaDaora_2_0_2[8] = {
#include "gm_sub/00923510.inc"
};
EmNextAreaEntry gEmIdSets_KushalaDaora_2_0[4] = {
#include "gm_sub/00923520.inc"
};
EmNextAreaSet gEmIdSets_KushalaDaora_2[2] = {
#include "gm_sub/00923540.inc"
};
s16 gEmIdSets_KushalaDaora_3_0_0[8] = {
#include "gm_sub/00923550.inc"
};
s16 gEmIdSets_KushalaDaora_3_0_1[8] = {
#include "gm_sub/00923560.inc"
};
s16 gEmIdSets_KushalaDaora_3_0_2[8] = {
#include "gm_sub/00923570.inc"
};
s16 gEmIdSets_KushalaDaora_3_0_3[8] = {
#include "gm_sub/00923580.inc"
};
s16 gEmIdSets_KushalaDaora_3_0_4[8] = {
#include "gm_sub/00923590.inc"
};
s16 gEmIdSets_KushalaDaora_3_0_5[8] = {
#include "gm_sub/009235A0.inc"
};
s16 gEmIdSets_KushalaDaora_3_0_6[8] = {
#include "gm_sub/009235B0.inc"
};
s16 gEmIdSets_KushalaDaora_3_0_7[8] = {
#include "gm_sub/009235C0.inc"
};
EmNextAreaEntry gEmIdSets_KushalaDaora_3_0[9] = {
#include "gm_sub/009235D0.inc"
};
EmNextAreaSet gEmIdSets_KushalaDaora_3[1] = {
#include "gm_sub/00923618.inc"
};
s16 gEmIdSets_KushalaDaora_4_0_0[8] = {
#include "gm_sub/00923620.inc"
};
s16 gEmIdSets_KushalaDaora_4_0_1[8] = {
#include "gm_sub/00923630.inc"
};
s16 gEmIdSets_KushalaDaora_4_0_2[8] = {
#include "gm_sub/00923640.inc"
};
s16 gEmIdSets_KushalaDaora_4_0_3[8] = {
#include "gm_sub/00923650.inc"
};
EmNextAreaEntry gEmIdSets_KushalaDaora_4_0[5] = {
#include "gm_sub/00923660.inc"
};
s16 gEmIdSets_KushalaDaora_4_1_0[8] = {
#include "gm_sub/00923690.inc"
};
s16 gEmIdSets_KushalaDaora_4_1_1[8] = {
#include "gm_sub/009236A0.inc"
};
s16 gEmIdSets_KushalaDaora_4_1_2[8] = {
#include "gm_sub/009236B0.inc"
};
s16 gEmIdSets_KushalaDaora_4_1_3[8] = {
#include "gm_sub/009236C0.inc"
};
s16 gEmIdSets_KushalaDaora_4_1_4[8] = {
#include "gm_sub/009236D0.inc"
};
s16 gEmIdSets_KushalaDaora_4_1_5[8] = {
#include "gm_sub/009236E0.inc"
};
s16 gEmIdSets_KushalaDaora_4_1_6[8] = {
#include "gm_sub/009236F0.inc"
};
EmNextAreaEntry gEmIdSets_KushalaDaora_4_1[8] = {
#include "gm_sub/00923700.inc"
};
EmNextAreaSet gEmIdSets_KushalaDaora_4[2] = {
#include "gm_sub/00923740.inc"
};
s16 gEmIdSets_KushalaDaora_6_0_0[8] = {
#include "gm_sub/00923750.inc"
};
s16 gEmIdSets_KushalaDaora_6_0_1[8] = {
#include "gm_sub/00923760.inc"
};
s16 gEmIdSets_KushalaDaora_6_0_2[8] = {
#include "gm_sub/00923770.inc"
};
s16 gEmIdSets_KushalaDaora_6_0_3[8] = {
#include "gm_sub/00923780.inc"
};
s16 gEmIdSets_KushalaDaora_6_0_4[8] = {
#include "gm_sub/00923790.inc"
};
s16 gEmIdSets_KushalaDaora_6_0_5[8] = {
#include "gm_sub/009237A0.inc"
};
s16 gEmIdSets_KushalaDaora_6_0_6[8] = {
#include "gm_sub/009237B0.inc"
};
s16 gEmIdSets_KushalaDaora_6_0_7[8] = {
#include "gm_sub/009237C0.inc"
};
s16 gEmIdSets_KushalaDaora_6_0_8[8] = {
#include "gm_sub/009237D0.inc"
};
s16 gEmIdSets_KushalaDaora_6_0_9[8] = {
#include "gm_sub/009237E0.inc"
};
EmNextAreaEntry gEmIdSets_KushalaDaora_6_0[11] = {
#include "gm_sub/009237F0.inc"
};
EmNextAreaSet gEmIdSets_KushalaDaora_6[1] = {
#include "gm_sub/00923848.inc"
};
s16 gEmIdSets_KushalaDaora_11_0_0[8] = {
#include "gm_sub/00923850.inc"
};
s16 gEmIdSets_KushalaDaora_11_0_1[8] = {
#include "gm_sub/00923860.inc"
};
s16 gEmIdSets_KushalaDaora_11_0_2[8] = {
#include "gm_sub/00923870.inc"
};
s16 gEmIdSets_KushalaDaora_11_0_3[8] = {
#include "gm_sub/00923880.inc"
};
s16 gEmIdSets_KushalaDaora_11_0_4[8] = {
#include "gm_sub/00923890.inc"
};
s16 gEmIdSets_KushalaDaora_11_0_5[8] = {
#include "gm_sub/009238A0.inc"
};
EmNextAreaEntry gEmIdSets_KushalaDaora_11_0[7] = {
#include "gm_sub/009238B0.inc"
};
EmNextAreaSet gEmIdSets_KushalaDaora_11[1] = {
#include "gm_sub/009238E8.inc"
};
s16 gEmIdSets_KushalaDaora_12_0_0[8] = {
#include "gm_sub/009238F0.inc"
};
s16 gEmIdSets_KushalaDaora_12_0_1[8] = {
#include "gm_sub/00923900.inc"
};
EmNextAreaBlock3 gEmIdSets_KushalaDaora[1] = {
#include "gm_sub/00923910.inc"
};
EmMotionRow gEmMotionRows_KushalaDaora[101] = {
#include "gm_sub/009239A0.inc"
};
f32 gEmAreaPoints_ShenGaoren_1_0[8][3] = {
#include "gm_sub/00923E60.inc"
};
f32 gEmAreaPoints_ShenGaoren_1_1[8][3] = {
#include "gm_sub/00923EC0.inc"
};
f32 gEmAreaPoints_ShenGaoren_1_2[6][3] = {
#include "gm_sub/00923F20.inc"
};
f32 gEmAreaPoints_ShenGaoren_1_3[8][3] = {
#include "gm_sub/00923F70.inc"
};
f32 gEmAreaPoints_ShenGaoren_1_4[9][3] = {
#include "gm_sub/00923FD0.inc"
};
f32 gEmAreaPoints_ShenGaoren_1_5[9][3] = {
#include "gm_sub/00924040.inc"
};
EmAreaPoints gEmAreaPoints_ShenGaoren_1[7] = {
#include "gm_sub/009240B0.inc"
};
f32 gEmAreaPoints_ShenGaoren_3_0[1][3] = {
#include "gm_sub/009240E8.inc"
};
EmAreaPoints gEmAreaPoints_ShenGaoren_3[2] = {
#include "gm_sub/00924100.inc"
};
f32 gEmAreaPoints_ShenGaoren_4_0[1][3] = {
#include "gm_sub/00924110.inc"
};
EmAreaPoints gEmAreaPoints_ShenGaoren_4[2] = {
#include "gm_sub/00924120.inc"
};
f32 gEmAreaPoints_ShenGaoren_5_0[1][3] = {
#include "gm_sub/00924130.inc"
};
EmAreaPoints gEmAreaPoints_ShenGaoren_5[2] = {
#include "gm_sub/00924140.inc"
};
EmRoutePt gEmRouteSets_ShenGaoren_0_0[1] = {
#include "gm_sub/00924150.inc"
};
EmRoute gEmRouteSets_ShenGaoren_0[1] = {
#include "gm_sub/00924170.inc"
};
EmRoutePt gEmRouteSets_ShenGaoren_1_0[1] = {
#include "gm_sub/00924190.inc"
};
EmRoute gEmRouteSets_ShenGaoren_1[1] = {
#include "gm_sub/009241B0.inc"
};
EmRoutePt gEmRouteSets_ShenGaoren_2_0[1] = {
#include "gm_sub/009241D0.inc"
};
EmRoute gEmRouteSets_ShenGaoren_2[1] = {
#include "gm_sub/009241F0.inc"
};
EmRoutePt gEmRouteSets_ShenGaoren_3_0[1] = {
#include "gm_sub/00924210.inc"
};
EmRoute gEmRouteSets_ShenGaoren_3[1] = {
#include "gm_sub/00924230.inc"
};
EmRoutePt gEmRouteSets_ShenGaoren_4_0[1] = {
#include "gm_sub/00924250.inc"
};
EmRoute gEmRouteSets_ShenGaoren_4[1] = {
#include "gm_sub/00924270.inc"
};
EmRouteSet gEmRouteSets_ShenGaoren[6] = {
#include "gm_sub/00924290.inc"
};
void *gEmKindTables_ShenGaoren[7] = {
#include "gm_sub/009242C0.inc"
};
EmMotionRow gEmMotionRows_ShenGaoren[101] = {
#include "gm_sub/009242E0.inc"
};
EmMotionRow gEmMotionRows_Shakalaka[101] = {
#include "gm_sub/009247A0.inc"
};
f32 gEmAreaPoints_YamaTsukami_1_0[16][3] = {
#include "gm_sub/00924C60.inc"
};
f32 gEmAreaPoints_YamaTsukami_1_1[2][3] = {
#include "gm_sub/00924D20.inc"
};
f32 gEmAreaPoints_YamaTsukami_1_2[13][3] = {
#include "gm_sub/00924D40.inc"
};
f32 gEmAreaPoints_YamaTsukami_1_3[2][3] = {
#include "gm_sub/00924DE0.inc"
};
f32 gEmAreaPoints_YamaTsukami_1_4[2][3] = {
#include "gm_sub/00924E00.inc"
};
f32 gEmAreaPoints_YamaTsukami_1_5[14][3] = {
#include "gm_sub/00924E20.inc"
};
f32 gEmAreaPoints_YamaTsukami_1_6[6][3] = {
#include "gm_sub/00924ED0.inc"
};
EmAreaPoints gEmAreaPoints_YamaTsukami_1[8] = {
#include "gm_sub/00924F20.inc"
};
f32 gEmAreaPoints_YamaTsukami_3_0[1][3] = {
#include "gm_sub/00924F60.inc"
};
EmAreaPoints gEmAreaPoints_YamaTsukami_3[2] = {
#include "gm_sub/00924F70.inc"
};
f32 gEmAreaPoints_YamaTsukami_4_0[1][3] = {
#include "gm_sub/00924F80.inc"
};
EmAreaPoints gEmAreaPoints_YamaTsukami_4[2] = {
#include "gm_sub/00924F90.inc"
};
f32 gEmAreaPoints_YamaTsukami_5_0[1][3] = {
#include "gm_sub/00924FA0.inc"
};
EmAreaPoints gEmAreaPoints_YamaTsukami_5[2] = {
#include "gm_sub/00924FB0.inc"
};
EmRoutePt gEmRouteSets_YamaTsukami_0_0[1] = {
#include "gm_sub/00924FC0.inc"
};
EmRoute gEmRouteSets_YamaTsukami_0[1] = {
#include "gm_sub/00924FE0.inc"
};
EmRoutePt gEmRouteSets_YamaTsukami_1_0[1] = {
#include "gm_sub/00925000.inc"
};
EmRoute gEmRouteSets_YamaTsukami_1[1] = {
#include "gm_sub/00925020.inc"
};
EmRoutePt gEmRouteSets_YamaTsukami_2_0[1] = {
#include "gm_sub/00925040.inc"
};
EmRoute gEmRouteSets_YamaTsukami_2[1] = {
#include "gm_sub/00925060.inc"
};
EmRoutePt gEmRouteSets_YamaTsukami_3_0[1] = {
#include "gm_sub/00925080.inc"
};
EmRoute gEmRouteSets_YamaTsukami_3[1] = {
#include "gm_sub/009250A0.inc"
};
EmRoutePt gEmRouteSets_YamaTsukami_4_0[1] = {
#include "gm_sub/009250C0.inc"
};
EmRoute gEmRouteSets_YamaTsukami_4[1] = {
#include "gm_sub/009250E0.inc"
};
EmRouteSet gEmRouteSets_YamaTsukami[6] = {
#include "gm_sub/00925100.inc"
};
void *gEmKindTables_YamaTsukami[7] = {
#include "gm_sub/00925130.inc"
};
EmMotionRow gEmMotionRows_YamaTsukami[101] = {
#include "gm_sub/00925150.inc"
};
f32 gEmAreaPoints_Chameleos_1_0[4][3] = {
#include "gm_sub/00925610.inc"
};
f32 gEmAreaPoints_Chameleos_1_1[4][3] = {
#include "gm_sub/00925640.inc"
};
f32 gEmAreaPoints_Chameleos_1_2[5][3] = {
#include "gm_sub/00925670.inc"
};
f32 gEmAreaPoints_Chameleos_1_3[6][3] = {
#include "gm_sub/009256B0.inc"
};
f32 gEmAreaPoints_Chameleos_1_4[4][3] = {
#include "gm_sub/00925700.inc"
};
f32 gEmAreaPoints_Chameleos_1_5[4][3] = {
#include "gm_sub/00925730.inc"
};
f32 gEmAreaPoints_Chameleos_1_6[2][3] = {
#include "gm_sub/00925760.inc"
};
f32 gEmAreaPoints_Chameleos_1_7[4][3] = {
#include "gm_sub/00925780.inc"
};
f32 gEmAreaPoints_Chameleos_1_8[4][3] = {
#include "gm_sub/009257B0.inc"
};
f32 gEmAreaPoints_Chameleos_1_9[4][3] = {
#include "gm_sub/009257E0.inc"
};
f32 gEmAreaPoints_Chameleos_1_10[4][3] = {
#include "gm_sub/00925810.inc"
};
f32 gEmAreaPoints_Chameleos_1_11[4][3] = {
#include "gm_sub/00925840.inc"
};
f32 gEmAreaPoints_Chameleos_1_12[8][3] = {
#include "gm_sub/00925870.inc"
};
f32 gEmAreaPoints_Chameleos_1_13[4][3] = {
#include "gm_sub/009258D0.inc"
};
f32 gEmAreaPoints_Chameleos_1_14[4][3] = {
#include "gm_sub/00925900.inc"
};
f32 gEmAreaPoints_Chameleos_1_15[4][3] = {
#include "gm_sub/00925930.inc"
};
f32 gEmAreaPoints_Chameleos_1_16[4][3] = {
#include "gm_sub/00925960.inc"
};
f32 gEmAreaPoints_Chameleos_1_17[4][3] = {
#include "gm_sub/00925990.inc"
};
f32 gEmAreaPoints_Chameleos_1_18[4][3] = {
#include "gm_sub/009259C0.inc"
};
EmAreaPoints gEmAreaPoints_Chameleos_1[20] = {
#include "gm_sub/009259F0.inc"
};
f32 gEmAreaPoints_Chameleos_3_0[1][3] = {
#include "gm_sub/00925A90.inc"
};
f32 gEmAreaPoints_Chameleos_3_1[1][3] = {
#include "gm_sub/00925AA0.inc"
};
f32 gEmAreaPoints_Chameleos_3_2[1][3] = {
#include "gm_sub/00925AB0.inc"
};
f32 gEmAreaPoints_Chameleos_3_3[1][3] = {
#include "gm_sub/00925AC0.inc"
};
f32 gEmAreaPoints_Chameleos_3_4[1][3] = {
#include "gm_sub/00925AD0.inc"
};
f32 gEmAreaPoints_Chameleos_3_5[1][3] = {
#include "gm_sub/00925AE0.inc"
};
f32 gEmAreaPoints_Chameleos_3_12[1][3] = {
#include "gm_sub/00925AF0.inc"
};
f32 gEmAreaPoints_Chameleos_3_13[1][3] = {
#include "gm_sub/00925B00.inc"
};
f32 gEmAreaPoints_Chameleos_3_14[1][3] = {
#include "gm_sub/00925B10.inc"
};
f32 gEmAreaPoints_Chameleos_3_15[1][3] = {
#include "gm_sub/00925B20.inc"
};
f32 gEmAreaPoints_Chameleos_3_16[1][3] = {
#include "gm_sub/00925B30.inc"
};
f32 gEmAreaPoints_Chameleos_3_17[1][3] = {
#include "gm_sub/00925B40.inc"
};
f32 gEmAreaPoints_Chameleos_3_18[1][3] = {
#include "gm_sub/00925B50.inc"
};
f32 gEmAreaPoints_Chameleos_3_6[1][3] = {
#include "gm_sub/00925B60.inc"
};
f32 gEmAreaPoints_Chameleos_3_7[1][3] = {
#include "gm_sub/00925B70.inc"
};
f32 gEmAreaPoints_Chameleos_3_8[1][3] = {
#include "gm_sub/00925B80.inc"
};
f32 gEmAreaPoints_Chameleos_3_9[1][3] = {
#include "gm_sub/00925B90.inc"
};
f32 gEmAreaPoints_Chameleos_3_10[1][3] = {
#include "gm_sub/00925BA0.inc"
};
f32 gEmAreaPoints_Chameleos_3_11[1][3] = {
#include "gm_sub/00925BB0.inc"
};
EmAreaPoints gEmAreaPoints_Chameleos_3[20] = {
#include "gm_sub/00925BC0.inc"
};
f32 gEmAreaPoints_Chameleos_4_0[1][3] = {
#include "gm_sub/00925C60.inc"
};
f32 gEmAreaPoints_Chameleos_4_1[1][3] = {
#include "gm_sub/00925C70.inc"
};
f32 gEmAreaPoints_Chameleos_4_2[1][3] = {
#include "gm_sub/00925C80.inc"
};
f32 gEmAreaPoints_Chameleos_4_3[1][3] = {
#include "gm_sub/00925C90.inc"
};
f32 gEmAreaPoints_Chameleos_4_4[1][3] = {
#include "gm_sub/00925CA0.inc"
};
f32 gEmAreaPoints_Chameleos_4_5[1][3] = {
#include "gm_sub/00925CB0.inc"
};
EmAreaPoints gEmAreaPoints_Chameleos_4[7] = {
#include "gm_sub/00925CC0.inc"
};
f32 gEmAreaPoints_Chameleos_5_0[1][3] = {
#include "gm_sub/00925CF8.inc"
};
f32 gEmAreaPoints_Chameleos_5_1[1][3] = {
#include "gm_sub/00925D08.inc"
};
f32 gEmAreaPoints_Chameleos_5_2[1][3] = {
#include "gm_sub/00925D18.inc"
};
f32 gEmAreaPoints_Chameleos_5_3[1][3] = {
#include "gm_sub/00925D28.inc"
};
f32 gEmAreaPoints_Chameleos_5_4[1][3] = {
#include "gm_sub/00925D38.inc"
};
f32 gEmAreaPoints_Chameleos_5_5[1][3] = {
#include "gm_sub/00925D48.inc"
};
f32 gEmAreaPoints_Chameleos_5_6[1][3] = {
#include "gm_sub/00925D58.inc"
};
f32 gEmAreaPoints_Chameleos_5_7[1][3] = {
#include "gm_sub/00925D68.inc"
};
f32 gEmAreaPoints_Chameleos_5_8[1][3] = {
#include "gm_sub/00925D78.inc"
};
f32 gEmAreaPoints_Chameleos_5_9[1][3] = {
#include "gm_sub/00925D88.inc"
};
f32 gEmAreaPoints_Chameleos_5_10[1][3] = {
#include "gm_sub/00925D98.inc"
};
f32 gEmAreaPoints_Chameleos_5_11[1][3] = {
#include "gm_sub/00925DA8.inc"
};
f32 gEmAreaPoints_Chameleos_5_12[1][3] = {
#include "gm_sub/00925DB8.inc"
};
f32 gEmAreaPoints_Chameleos_5_13[1][3] = {
#include "gm_sub/00925DC8.inc"
};
f32 gEmAreaPoints_Chameleos_5_14[1][3] = {
#include "gm_sub/00925DD8.inc"
};
f32 gEmAreaPoints_Chameleos_5_15[1][3] = {
#include "gm_sub/00925DE8.inc"
};
f32 gEmAreaPoints_Chameleos_5_16[1][3] = {
#include "gm_sub/00925DF8.inc"
};
f32 gEmAreaPoints_Chameleos_5_17[1][3] = {
#include "gm_sub/00925E08.inc"
};
EmAreaPoints gEmAreaPoints_Chameleos_5[19] = {
#include "gm_sub/00925E20.inc"
};
void *gEmKindTables_Chameleos[6] = {
#include "gm_sub/00925EC0.inc"
};
EmAreaValue gEmAreaTimer1Values_Chameleos[1] = {
#include "gm_sub/00925ED8.inc"
};
EmAreaValue gEmAreaTimer2Values_Chameleos[14] = {
#include "gm_sub/00925EE0.inc"
};
/* Chameleos's next-area lists (em.h's EmNextAreaSet; Em_NextAreas): gEmIdSets_Chameleos by
 * map, its sets, their entries and the entries' area lists. */
s16 gEmIdSets_Chameleos_0_0_0[8] = {
#include "gm_sub/00925F20.inc"
};
s16 gEmIdSets_Chameleos_0_0_1[8] = {
#include "gm_sub/00925F30.inc"
};
s16 gEmIdSets_Chameleos_0_0_2[8] = {
#include "gm_sub/00925F40.inc"
};
s16 gEmIdSets_Chameleos_0_0_3[8] = {
#include "gm_sub/00925F50.inc"
};
s16 gEmIdSets_Chameleos_0_0_4[8] = {
#include "gm_sub/00925F60.inc"
};
s16 gEmIdSets_Chameleos_0_0_5[8] = {
#include "gm_sub/00925F70.inc"
};
s16 gEmIdSets_Chameleos_0_0_6[8] = {
#include "gm_sub/00925F80.inc"
};
EmNextAreaEntry gEmIdSets_Chameleos_0_0[8] = {
#include "gm_sub/00925F90.inc"
};
EmNextAreaSet gEmIdSets_Chameleos_0[1] = {
#include "gm_sub/00925FD0.inc"
};
s16 gEmIdSets_Chameleos_4_0_0[8] = {
#include "gm_sub/00925FE0.inc"
};
s16 gEmIdSets_Chameleos_4_0_1[8] = {
#include "gm_sub/00925FF0.inc"
};
s16 gEmIdSets_Chameleos_4_0_2[8] = {
#include "gm_sub/00926000.inc"
};
s16 gEmIdSets_Chameleos_4_0_3[8] = {
#include "gm_sub/00926010.inc"
};
s16 gEmIdSets_Chameleos_4_0_4[8] = {
#include "gm_sub/00926020.inc"
};
s16 gEmIdSets_Chameleos_4_0_5[8] = {
#include "gm_sub/00926030.inc"
};
s16 gEmIdSets_Chameleos_4_0_6[8] = {
#include "gm_sub/00926040.inc"
};
s16 gEmIdSets_Chameleos_4_0_7[8] = {
#include "gm_sub/00926050.inc"
};
EmNextAreaEntry gEmIdSets_Chameleos_4_0[9] = {
#include "gm_sub/00926060.inc"
};
EmNextAreaSet gEmIdSets_Chameleos_4[1] = {
#include "gm_sub/009260A8.inc"
};
s16 gEmIdSets_Chameleos_6_0_0[8] = {
#include "gm_sub/009260B0.inc"
};
s16 gEmIdSets_Chameleos_6_0_1[8] = {
#include "gm_sub/009260C0.inc"
};
s16 gEmIdSets_Chameleos_6_0_2[8] = {
#include "gm_sub/009260D0.inc"
};
s16 gEmIdSets_Chameleos_6_0_3[8] = {
#include "gm_sub/009260E0.inc"
};
s16 gEmIdSets_Chameleos_6_0_4[8] = {
#include "gm_sub/009260F0.inc"
};
s16 gEmIdSets_Chameleos_6_0_5[8] = {
#include "gm_sub/00926100.inc"
};
s16 gEmIdSets_Chameleos_6_0_6[8] = {
#include "gm_sub/00926110.inc"
};
s16 gEmIdSets_Chameleos_6_0_7[8] = {
#include "gm_sub/00926120.inc"
};
s16 gEmIdSets_Chameleos_6_0_8[8] = {
#include "gm_sub/00926130.inc"
};
EmNextAreaEntry gEmIdSets_Chameleos_6_0[10] = {
#include "gm_sub/00926140.inc"
};
EmNextAreaSet gEmIdSets_Chameleos_6[1] = {
#include "gm_sub/00926190.inc"
};
EmNextAreaSet *gEmIdSets_Chameleos[MAP_COUNT] = {
#include "gm_sub/009261A0.inc"
};
EmMotionRow gEmMotionRows_Chameleos[101] = {
#include "gm_sub/00926210.inc"
};
f32 gEmAreaPoints_Lunastra_1_0[5][3] = {
#include "gm_sub/009266D0.inc"
};
f32 gEmAreaPoints_Lunastra_1_1[4][3] = {
#include "gm_sub/00926710.inc"
};
f32 gEmAreaPoints_Lunastra_1_2[4][3] = {
#include "gm_sub/00926740.inc"
};
f32 gEmAreaPoints_Lunastra_1_3[5][3] = {
#include "gm_sub/00926770.inc"
};
f32 gEmAreaPoints_Lunastra_1_4[4][3] = {
#include "gm_sub/009267B0.inc"
};
f32 gEmAreaPoints_Lunastra_1_5[8][3] = {
#include "gm_sub/009267E0.inc"
};
f32 gEmAreaPoints_Lunastra_1_6[5][3] = {
#include "gm_sub/00926840.inc"
};
f32 gEmAreaPoints_Lunastra_1_7[4][3] = {
#include "gm_sub/00926880.inc"
};
f32 gEmAreaPoints_Lunastra_1_8[4][3] = {
#include "gm_sub/009268B0.inc"
};
f32 gEmAreaPoints_Lunastra_1_9[4][3] = {
#include "gm_sub/009268E0.inc"
};
f32 gEmAreaPoints_Lunastra_1_10[2][3] = {
#include "gm_sub/00926910.inc"
};
f32 gEmAreaPoints_Lunastra_1_11[4][3] = {
#include "gm_sub/00926930.inc"
};
f32 gEmAreaPoints_Lunastra_1_12[4][3] = {
#include "gm_sub/00926960.inc"
};
f32 gEmAreaPoints_Lunastra_1_13[4][3] = {
#include "gm_sub/00926990.inc"
};
f32 gEmAreaPoints_Lunastra_1_14[4][3] = {
#include "gm_sub/009269C0.inc"
};
f32 gEmAreaPoints_Lunastra_1_15[4][3] = {
#include "gm_sub/009269F0.inc"
};
f32 gEmAreaPoints_Lunastra_1_16[4][3] = {
#include "gm_sub/00926A20.inc"
};
f32 gEmAreaPoints_Lunastra_1_17[4][3] = {
#include "gm_sub/00926A50.inc"
};
f32 gEmAreaPoints_Lunastra_1_18[4][3] = {
#include "gm_sub/00926A80.inc"
};
f32 gEmAreaPoints_Lunastra_1_19[4][3] = {
#include "gm_sub/00926AB0.inc"
};
f32 gEmAreaPoints_Lunastra_1_20[2][3] = {
#include "gm_sub/00926AE0.inc"
};
f32 gEmAreaPoints_Lunastra_1_21[8][3] = {
#include "gm_sub/00926B00.inc"
};
EmAreaPoints gEmAreaPoints_Lunastra_1[23] = {
#include "gm_sub/00926B60.inc"
};
f32 gEmAreaPoints_Lunastra_3_0[1][3] = {
#include "gm_sub/00926C18.inc"
};
f32 gEmAreaPoints_Lunastra_3_1[1][3] = {
#include "gm_sub/00926C28.inc"
};
f32 gEmAreaPoints_Lunastra_3_2[1][3] = {
#include "gm_sub/00926C38.inc"
};
f32 gEmAreaPoints_Lunastra_3_3[1][3] = {
#include "gm_sub/00926C48.inc"
};
f32 gEmAreaPoints_Lunastra_3_4[1][3] = {
#include "gm_sub/00926C58.inc"
};
f32 gEmAreaPoints_Lunastra_3_5[1][3] = {
#include "gm_sub/00926C68.inc"
};
f32 gEmAreaPoints_Lunastra_3_6[1][3] = {
#include "gm_sub/00926C78.inc"
};
f32 gEmAreaPoints_Lunastra_3_7[1][3] = {
#include "gm_sub/00926C88.inc"
};
f32 gEmAreaPoints_Lunastra_3_8[1][3] = {
#include "gm_sub/00926C98.inc"
};
f32 gEmAreaPoints_Lunastra_3_9[1][3] = {
#include "gm_sub/00926CA8.inc"
};
f32 gEmAreaPoints_Lunastra_3_10[1][3] = {
#include "gm_sub/00926CB8.inc"
};
f32 gEmAreaPoints_Lunastra_3_11[1][3] = {
#include "gm_sub/00926CC8.inc"
};
f32 gEmAreaPoints_Lunastra_3_12[1][3] = {
#include "gm_sub/00926CD8.inc"
};
f32 gEmAreaPoints_Lunastra_3_13[1][3] = {
#include "gm_sub/00926CE8.inc"
};
f32 gEmAreaPoints_Lunastra_3_14[1][3] = {
#include "gm_sub/00926CF8.inc"
};
f32 gEmAreaPoints_Lunastra_3_15[1][3] = {
#include "gm_sub/00926D08.inc"
};
f32 gEmAreaPoints_Lunastra_3_16[1][3] = {
#include "gm_sub/00926D18.inc"
};
f32 gEmAreaPoints_Lunastra_3_17[1][3] = {
#include "gm_sub/00926D28.inc"
};
f32 gEmAreaPoints_Lunastra_3_18[1][3] = {
#include "gm_sub/00926D38.inc"
};
f32 gEmAreaPoints_Lunastra_3_19[1][3] = {
#include "gm_sub/00926D48.inc"
};
f32 gEmAreaPoints_Lunastra_3_20[1][3] = {
#include "gm_sub/00926D58.inc"
};
f32 gEmAreaPoints_Lunastra_3_21[1][3] = {
#include "gm_sub/00926D68.inc"
};
EmAreaPoints gEmAreaPoints_Lunastra_3[23] = {
#include "gm_sub/00926D80.inc"
};
f32 gEmAreaPoints_Lunastra_4_0[1][3] = {
#include "gm_sub/00926E38.inc"
};
f32 gEmAreaPoints_Lunastra_4_1[1][3] = {
#include "gm_sub/00926E48.inc"
};
f32 gEmAreaPoints_Lunastra_4_2[1][3] = {
#include "gm_sub/00926E58.inc"
};
f32 gEmAreaPoints_Lunastra_4_3[1][3] = {
#include "gm_sub/00926E68.inc"
};
f32 gEmAreaPoints_Lunastra_4_4[1][3] = {
#include "gm_sub/00926E78.inc"
};
f32 gEmAreaPoints_Lunastra_4_5[1][3] = {
#include "gm_sub/00926E88.inc"
};
EmAreaPoints gEmAreaPoints_Lunastra_4[7] = {
#include "gm_sub/00926EA0.inc"
};
f32 gEmAreaPoints_Lunastra_5_0[1][3] = {
#include "gm_sub/00926ED8.inc"
};
f32 gEmAreaPoints_Lunastra_5_1[1][3] = {
#include "gm_sub/00926EE8.inc"
};
f32 gEmAreaPoints_Lunastra_5_2[1][3] = {
#include "gm_sub/00926EF8.inc"
};
f32 gEmAreaPoints_Lunastra_5_3[1][3] = {
#include "gm_sub/00926F08.inc"
};
f32 gEmAreaPoints_Lunastra_5_4[1][3] = {
#include "gm_sub/00926F18.inc"
};
f32 gEmAreaPoints_Lunastra_5_5[1][3] = {
#include "gm_sub/00926F28.inc"
};
EmAreaPoints gEmAreaPoints_Lunastra_5[7] = {
#include "gm_sub/00926F40.inc"
};
void *gEmKindTables_Lunastra[6] = {
#include "gm_sub/00926F80.inc"
};
EmAreaValue gEmAreaTimer1Values_Lunastra[1] = {
#include "gm_sub/00926F98.inc"
};
EmAreaValue gEmAreaTimer2Values_Lunastra[9] = {
#include "gm_sub/00926FA0.inc"
};
/* Lunastra's next-area lists (em.h's EmNextAreaSet; Em_NextAreas): gEmIdSets_Lunastra by
 * map, its sets, their entries and the entries' area lists. */
s16 gEmIdSets_Lunastra_3_0_0[8] = {
#include "gm_sub/00926FD0.inc"
};
s16 gEmIdSets_Lunastra_3_0_1[8] = {
#include "gm_sub/00926FE0.inc"
};
s16 gEmIdSets_Lunastra_3_0_2[8] = {
#include "gm_sub/00926FF0.inc"
};
s16 gEmIdSets_Lunastra_3_0_3[8] = {
#include "gm_sub/00927000.inc"
};
s16 gEmIdSets_Lunastra_3_0_4[8] = {
#include "gm_sub/00927010.inc"
};
s16 gEmIdSets_Lunastra_3_0_5[8] = {
#include "gm_sub/00927020.inc"
};
s16 gEmIdSets_Lunastra_3_0_6[8] = {
#include "gm_sub/00927030.inc"
};
s16 gEmIdSets_Lunastra_3_0_7[8] = {
#include "gm_sub/00927040.inc"
};
EmNextAreaEntry gEmIdSets_Lunastra_3_0[9] = {
#include "gm_sub/00927050.inc"
};
EmNextAreaSet gEmIdSets_Lunastra_3[1] = {
#include "gm_sub/00927098.inc"
};
s16 gEmIdSets_Lunastra_4_0_0[8] = {
#include "gm_sub/009270A0.inc"
};
s16 gEmIdSets_Lunastra_4_0_1[8] = {
#include "gm_sub/009270B0.inc"
};
s16 gEmIdSets_Lunastra_4_0_2[8] = {
#include "gm_sub/009270C0.inc"
};
s16 gEmIdSets_Lunastra_4_0_3[8] = {
#include "gm_sub/009270D0.inc"
};
s16 gEmIdSets_Lunastra_4_0_4[8] = {
#include "gm_sub/009270E0.inc"
};
s16 gEmIdSets_Lunastra_4_0_5[8] = {
#include "gm_sub/009270F0.inc"
};
EmNextAreaEntry gEmIdSets_Lunastra_4_0[7] = {
#include "gm_sub/00927100.inc"
};
EmNextAreaSet gEmIdSets_Lunastra_4[1] = {
#include "gm_sub/00927138.inc"
};
s16 gEmIdSets_Lunastra_0_0_0[8] = {
#include "gm_sub/00927140.inc"
};
s16 gEmIdSets_Lunastra_0_0_1[8] = {
#include "gm_sub/00927150.inc"
};
s16 gEmIdSets_Lunastra_0_0_2[8] = {
#include "gm_sub/00927160.inc"
};
s16 gEmIdSets_Lunastra_0_0_3[8] = {
#include "gm_sub/00927170.inc"
};
s16 gEmIdSets_Lunastra_0_0_4[8] = {
#include "gm_sub/00927180.inc"
};
s16 gEmIdSets_Lunastra_0_0_5[8] = {
#include "gm_sub/00927190.inc"
};
s16 gEmIdSets_Lunastra_0_0_6[8] = {
#include "gm_sub/009271A0.inc"
};
EmNextAreaEntry gEmIdSets_Lunastra_0_0[8] = {
#include "gm_sub/009271B0.inc"
};
EmNextAreaSet gEmIdSets_Lunastra_0[1] = {
#include "gm_sub/009271F0.inc"
};
s16 gEmIdSets_Lunastra_12_0_0[8] = {
#include "gm_sub/00927200.inc"
};
s16 gEmIdSets_Lunastra_12_0_1[8] = {
#include "gm_sub/00927210.inc"
};
EmNextAreaEntry gEmIdSets_Lunastra_12_0[3] = {
#include "gm_sub/00927220.inc"
};
EmNextAreaSet gEmIdSets_Lunastra_12[1] = {
#include "gm_sub/00927238.inc"
};
s16 gEmIdSets_Lunastra_15_0_0[8] = {
#include "gm_sub/00927240.inc"
};
s16 gEmIdSets_Lunastra_15_0_1[8] = {
#include "gm_sub/00927250.inc"
};
s16 gEmIdSets_Lunastra_15_0_2[8] = {
#include "gm_sub/00927260.inc"
};
s16 gEmIdSets_Lunastra_15_0_3[8] = {
#include "gm_sub/00927270.inc"
};
s16 gEmIdSets_Lunastra_15_0_4[8] = {
#include "gm_sub/00927280.inc"
};
s16 gEmIdSets_Lunastra_15_0_5[8] = {
#include "gm_sub/00927290.inc"
};
EmNextAreaBlock7 gEmIdSets_Lunastra[1] = {
#include "gm_sub/009272A0.inc"
};
f32 gEmAreaPoints_YamaTsukamiB_1_0[6][3] = {
#include "gm_sub/00927350.inc"
};
EmAreaPoints gEmAreaPoints_YamaTsukamiB_1[3] = {
#include "gm_sub/009273A0.inc"
};
EmAreaPoints gEmAreaPoints_YamaTsukamiB_3[1] = {
#include "gm_sub/009273B8.inc"
};
EmAreaPoints gEmAreaPoints_YamaTsukamiB_4[1] = {
#include "gm_sub/009273C0.inc"
};
EmAreaPoints gEmAreaPoints_YamaTsukamiB_5[1] = {
#include "gm_sub/009273C8.inc"
};
EmRouteSet gEmRouteSets_YamaTsukamiB[1] = {
#include "gm_sub/009273D0.inc"
};
void *gEmKindTables_YamaTsukamiB[7] = {
#include "gm_sub/009273E0.inc"
};
/*
 * The areas' collision and light rows, which sub_main's area tables (area_update.c) hold as
 * numbers, by area: gAreaKindFlags (AreaKindRow by triangle kind), gAreaLightZones,
 * gAreaLightZones2 and gAreaLightZonesAlt (AreaLightZone lists, entry 0 unused: the zone
 * numbers start at 1), gAreaLightColors (32-byte entries, two rows of four floats); read by
 * area_collision.c (Obj_UpdateLightColors and the floor kinds' readers). Each row is named
 * after the table and the first area whose entry points to it.
 */
AreaKindRow gAreaKindFlags_7[2] = {
#include "gm_sub/00927400.inc"
};
AreaKindRow gAreaKindFlags_9[2] = {
#include "gm_sub/00927410.inc"
};
AreaKindRow gAreaKindFlags_10[2] = {
#include "gm_sub/00927420.inc"
};
AreaKindRow gAreaKindFlags_13[2] = {
#include "gm_sub/00927430.inc"
};
AreaKindRow gAreaKindFlags_20[2] = {
#include "gm_sub/00927440.inc"
};
AreaKindRow gAreaKindFlags_21[2] = {
#include "gm_sub/00927450.inc"
};
AreaKindRow gAreaKindFlags_32[2] = {
#include "gm_sub/00927460.inc"
};
AreaKindRow gAreaKindFlags_33[2] = {
#include "gm_sub/00927470.inc"
};
AreaKindRow gAreaKindFlags_34[4] = {
#include "gm_sub/00927480.inc"
};
AreaKindRow gAreaKindFlags_35[2] = {
#include "gm_sub/009274A0.inc"
};
AreaKindRow gAreaKindFlags_36[2] = {
#include "gm_sub/009274B0.inc"
};
AreaKindRow gAreaKindFlags_37[2] = {
#include "gm_sub/009274C0.inc"
};
AreaKindRow gAreaKindFlags_38[2] = {
#include "gm_sub/009274D0.inc"
};
AreaKindRow gAreaKindFlags_39[4] = {
#include "gm_sub/009274E0.inc"
};
AreaKindRow gAreaKindFlags_40[2] = {
#include "gm_sub/00927500.inc"
};
AreaKindRow gAreaKindFlags_41[2] = {
#include "gm_sub/00927510.inc"
};
AreaKindRow gAreaKindFlags_42[6] = {
#include "gm_sub/00927520.inc"
};
AreaKindRow gAreaKindFlags_50[4] = {
#include "gm_sub/00927550.inc"
};
AreaKindRow gAreaKindFlags_59[4] = {
#include "gm_sub/00927570.inc"
};
AreaKindRow gAreaKindFlags_60[4] = {
#include "gm_sub/00927590.inc"
};
AreaKindRow gAreaKindFlags_63[4] = {
#include "gm_sub/009275B0.inc"
};
AreaKindRow gAreaKindFlags_64[4] = {
#include "gm_sub/009275D0.inc"
};
AreaKindRow gAreaKindFlags_72[4] = {
#include "gm_sub/009275F0.inc"
};
AreaKindRow gAreaKindFlags_27[2] = {
#include "gm_sub/00927610.inc"
};
AreaKindRow gAreaKindFlags_75[2] = {
#include "gm_sub/00927620.inc"
};
AreaKindRow gAreaKindFlags_88[2] = {
#include "gm_sub/00927630.inc"
};
AreaKindRow gAreaKindFlags_46[2] = {
#include "gm_sub/00927640.inc"
};
AreaKindRow gAreaKindFlags_57[4] = {
#include "gm_sub/00927650.inc"
};
AreaKindRow gAreaKindFlags_66[2] = {
#include "gm_sub/00927670.inc"
};
AreaKindRow gAreaKindFlags_92[2] = {
#include "gm_sub/00927680.inc"
};
AreaKindRow gAreaKindFlags_93[2] = {
#include "gm_sub/00927690.inc"
};
AreaKindRow gAreaKindFlags_94[2] = {
#include "gm_sub/009276A0.inc"
};
AreaKindRow gAreaKindFlags_96[2] = {
#include "gm_sub/009276B0.inc"
};
AreaKindRow gAreaKindFlags_97[4] = {
#include "gm_sub/009276C0.inc"
};
AreaKindRow gAreaKindFlags_98[2] = {
#include "gm_sub/009276E0.inc"
};
AreaKindRow gAreaKindFlags_99[4] = {
#include "gm_sub/009276F0.inc"
};
AreaKindRow gAreaKindFlags_100[4] = {
#include "gm_sub/00927710.inc"
};
AreaKindRow gAreaKindFlags_1[4] = {
#include "gm_sub/00927730.inc"
};
AreaKindRow gAreaKindFlags_2[2] = {
#include "gm_sub/00927750.inc"
};
AreaKindRow gAreaKindFlags_3[2] = {
#include "gm_sub/00927760.inc"
};
AreaKindRow gAreaKindFlags_4[4] = {
#include "gm_sub/00927770.inc"
};
AreaKindRow gAreaKindFlags_5[4] = {
#include "gm_sub/00927790.inc"
};
AreaKindRow gAreaKindFlags_18[2] = {
#include "gm_sub/009277B0.inc"
};
AreaKindRow gAreaKindFlags_19[2] = {
#include "gm_sub/009277C0.inc"
};
AreaKindRow gAreaKindFlags_22[4] = {
#include "gm_sub/009277D0.inc"
};
AreaKindRow gAreaKindFlags_23[2] = {
#include "gm_sub/009277F0.inc"
};
AreaKindRow gAreaKindFlags_26[2] = {
#include "gm_sub/00927800.inc"
};
AreaKindRow gAreaKindFlags_120[4] = {
#include "gm_sub/00927810.inc"
};
AreaKindRow gAreaKindFlags_121[2] = {
#include "gm_sub/00927830.inc"
};
AreaKindRow gAreaKindFlags_122[2] = {
#include "gm_sub/00927840.inc"
};
AreaKindRow gAreaKindFlags_123[2] = {
#include "gm_sub/00927850.inc"
};
AreaKindRow gAreaKindFlags_124[2] = {
#include "gm_sub/00927860.inc"
};
AreaKindRow gAreaKindFlags_83[2] = {
#include "gm_sub/00927870.inc"
};
AreaKindRow gAreaKindFlags_126[2] = {
#include "gm_sub/00927880.inc"
};
AreaKindRow gAreaKindFlags_84[2] = {
#include "gm_sub/00927890.inc"
};
AreaKindRow gAreaKindFlags_129[2] = {
#include "gm_sub/009278A0.inc"
};
AreaKindRow gAreaKindFlags_130[4] = {
#include "gm_sub/009278B0.inc"
};
AreaKindRow gAreaKindFlags_45[2] = {
#include "gm_sub/009278D0.inc"
};
AreaKindRow gAreaKindFlags_47[4] = {
#include "gm_sub/009278E0.inc"
};
AreaKindRow gAreaKindFlags_48[6] = {
#include "gm_sub/00927900.inc"
};
AreaKindRow gAreaKindFlags_49[4] = {
#include "gm_sub/00927930.inc"
};
AreaKindRow gAreaKindFlags_144[4] = {
#include "gm_sub/00927950.inc"
};
AreaKindRow gAreaKindFlags_51[4] = {
#include "gm_sub/00927970.inc"
};
AreaKindRow gAreaKindFlags_52[4] = {
#include "gm_sub/00927990.inc"
};
AreaKindRow gAreaKindFlags_53[2] = {
#include "gm_sub/009279B0.inc"
};
AreaKindRow gAreaKindFlags_54[2] = {
#include "gm_sub/009279C0.inc"
};
AreaKindRow gAreaKindFlags_55[4] = {
#include "gm_sub/009279D0.inc"
};
AreaKindRow gAreaKindFlags_56[2] = {
#include "gm_sub/009279F0.inc"
};
AreaKindRow gAreaKindFlags_16[2] = {
#include "gm_sub/00927A00.inc"
};
AreaKindRow gAreaKindFlags_44[2] = {
#include "gm_sub/00927A10.inc"
};
AreaKindRow gAreaKindFlags_67[4] = {
#include "gm_sub/00927A20.inc"
};
AreaKindRow gAreaKindFlags_71[2] = {
#include "gm_sub/00927A40.inc"
};
AreaKindRow gAreaKindFlags_158[4] = {
#include "gm_sub/00927A50.inc"
};
AreaKindRow gAreaKindFlags_159[2] = {
#include "gm_sub/00927A70.inc"
};
AreaKindRow gAreaKindFlags_58[4] = {
#include "gm_sub/00927A80.inc"
};
AreaKindRow gAreaKindFlags_162[4] = {
#include "gm_sub/00927AA0.inc"
};
AreaKindRow gAreaKindFlags_163[4] = {
#include "gm_sub/00927AC0.inc"
};
AreaKindRow gAreaKindFlags_62[2] = {
#include "gm_sub/00927AE0.inc"
};
AreaKindRow gAreaKindFlags_166[4] = {
#include "gm_sub/00927AF0.inc"
};
AreaKindRow gAreaKindFlags_167[2] = {
#include "gm_sub/00927B10.inc"
};
AreaKindRow gAreaKindFlags_169[2] = {
#include "gm_sub/00927B20.inc"
};
AreaKindRow gAreaKindFlags_177[2] = {
#include "gm_sub/00927B30.inc"
};
AreaKindRow gAreaKindFlags_178[2] = {
#include "gm_sub/00927B40.inc"
};
AreaLightZone gAreaLightZones_7[2] = {
#include "gm_sub/00927B50.inc"
};
f32 gAreaLightColors_7[3][2][4] = {
#include "gm_sub/00927B80.inc"
};
AreaLightZone gAreaLightZones_10[2] = {
#include "gm_sub/00927BE0.inc"
};
f32 gAreaLightColors_10[3][2][4] = {
#include "gm_sub/00927C10.inc"
};
AreaLightZone gAreaLightZones_15[2] = {
#include "gm_sub/00927C70.inc"
};
f32 gAreaLightColors_15[3][2][4] = {
#include "gm_sub/00927CA0.inc"
};
AreaLightZone gAreaLightZones_20[2] = {
#include "gm_sub/00927D00.inc"
};
f32 gAreaLightColors_20[3][2][4] = {
#include "gm_sub/00927D30.inc"
};
AreaLightZone gAreaLightZones_21[2] = {
#include "gm_sub/00927D90.inc"
};
f32 gAreaLightColors_21[3][2][4] = {
#include "gm_sub/00927DC0.inc"
};
AreaLightZone gAreaLightZones_32[2] = {
#include "gm_sub/00927E20.inc"
};
f32 gAreaLightColors_32[3][2][4] = {
#include "gm_sub/00927E50.inc"
};
AreaLightZone gAreaLightZones_33[2] = {
#include "gm_sub/00927EB0.inc"
};
f32 gAreaLightColors_33[3][2][4] = {
#include "gm_sub/00927EE0.inc"
};
AreaLightZone gAreaLightZones2_34[2] = {
#include "gm_sub/00927F40.inc"
};
f32 gAreaLightColors_34[3][2][4] = {
#include "gm_sub/00927F70.inc"
};
AreaLightZone gAreaLightZones2_35[2] = {
#include "gm_sub/00927FD0.inc"
};
f32 gAreaLightColors_35[3][2][4] = {
#include "gm_sub/00928000.inc"
};
AreaLightZone gAreaLightZones_36[2] = {
#include "gm_sub/00928060.inc"
};
f32 gAreaLightColors_36[3][2][4] = {
#include "gm_sub/00928090.inc"
};
AreaLightZone gAreaLightZones2_37[2] = {
#include "gm_sub/009280F0.inc"
};
f32 gAreaLightColors_37[3][2][4] = {
#include "gm_sub/00928120.inc"
};
AreaLightZone gAreaLightZones_38[2] = {
#include "gm_sub/00928180.inc"
};
f32 gAreaLightColors_38[3][2][4] = {
#include "gm_sub/009281B0.inc"
};
AreaLightZone gAreaLightZones2_39[2] = {
#include "gm_sub/00928210.inc"
};
f32 gAreaLightColors_39[3][2][4] = {
#include "gm_sub/00928240.inc"
};
AreaLightZone gAreaLightZones2_40[2] = {
#include "gm_sub/009282A0.inc"
};
f32 gAreaLightColors_40[3][2][4] = {
#include "gm_sub/009282D0.inc"
};
AreaLightZone gAreaLightZones_41[2] = {
#include "gm_sub/00928330.inc"
};
f32 gAreaLightColors_41[3][2][4] = {
#include "gm_sub/00928360.inc"
};
AreaLightZone gAreaLightZones_42[3] = {
#include "gm_sub/009283C0.inc"
};
f32 gAreaLightColors_42[5][2][4] = {
#include "gm_sub/00928410.inc"
};
AreaLightZone gAreaLightZones_46[2] = {
#include "gm_sub/009284B0.inc"
};
f32 gAreaLightColors_46[3][2][4] = {
#include "gm_sub/009284E0.inc"
};
AreaLightZone gAreaLightZones_50[2] = {
#include "gm_sub/00928540.inc"
};
f32 gAreaLightColors_50[3][2][4] = {
#include "gm_sub/00928570.inc"
};
AreaLightZone gAreaLightZones_57[2] = {
#include "gm_sub/009285D0.inc"
};
f32 gAreaLightColors_57[3][2][4] = {
#include "gm_sub/00928600.inc"
};
AreaLightZone gAreaLightZones_59[2] = {
#include "gm_sub/00928660.inc"
};
f32 gAreaLightColors_59[3][2][4] = {
#include "gm_sub/00928690.inc"
};
AreaLightZone gAreaLightZones_66[2] = {
#include "gm_sub/009286F0.inc"
};
f32 gAreaLightColors_66[3][2][4] = {
#include "gm_sub/00928720.inc"
};
AreaLightZone gAreaLightZones_27[2] = {
#include "gm_sub/00928780.inc"
};
f32 gAreaLightColors_27[3][2][4] = {
#include "gm_sub/009287B0.inc"
};
AreaLightZone gAreaLightZones2_88[2] = {
#include "gm_sub/00928810.inc"
};
f32 gAreaLightColors_88[3][2][4] = {
#include "gm_sub/00928840.inc"
};
AreaLightZone gAreaLightZones_89[2] = {
#include "gm_sub/009288A0.inc"
};
f32 gAreaLightColors_89[3][2][4] = {
#include "gm_sub/009288D0.inc"
};
AreaLightZone gAreaLightZones_90[2] = {
#include "gm_sub/00928930.inc"
};
f32 gAreaLightColors_90[3][2][4] = {
#include "gm_sub/00928960.inc"
};
AreaLightZone gAreaLightZones_91[2] = {
#include "gm_sub/009289C0.inc"
};
f32 gAreaLightColors_91[3][2][4] = {
#include "gm_sub/009289F0.inc"
};
AreaLightZone gAreaLightZones_92[2] = {
#include "gm_sub/00928A50.inc"
};
f32 gAreaLightColors_92[3][2][4] = {
#include "gm_sub/00928A80.inc"
};
AreaLightZone gAreaLightZones_93[2] = {
#include "gm_sub/00928AE0.inc"
};
f32 gAreaLightColors_93[3][2][4] = {
#include "gm_sub/00928B10.inc"
};
AreaLightZone gAreaLightZones_94[2] = {
#include "gm_sub/00928B70.inc"
};
f32 gAreaLightColors_94[3][2][4] = {
#include "gm_sub/00928BA0.inc"
};
AreaLightZone gAreaLightZonesAlt_1[2] = {
#include "gm_sub/00928C00.inc"
};
f32 gAreaLightColors_97[3][2][4] = {
#include "gm_sub/00928C30.inc"
};
AreaLightZone gAreaLightZonesAlt_2[2] = {
#include "gm_sub/00928C90.inc"
};
f32 gAreaLightColors_98[3][2][4] = {
#include "gm_sub/00928CC0.inc"
};
AreaLightZone gAreaLightZonesAlt_3[2] = {
#include "gm_sub/00928D20.inc"
};
f32 gAreaLightColors_99[3][2][4] = {
#include "gm_sub/00928D50.inc"
};
AreaLightZone gAreaLightZonesAlt_4[2] = {
#include "gm_sub/00928DB0.inc"
};
f32 gAreaLightColors_100[3][2][4] = {
#include "gm_sub/00928DE0.inc"
};
AreaLightZone gAreaLightZones_1[2] = {
#include "gm_sub/00928E40.inc"
};
f32 gAreaLightColors_1[3][2][4] = {
#include "gm_sub/00928E70.inc"
};
AreaLightZone gAreaLightZones_2[2] = {
#include "gm_sub/00928ED0.inc"
};
f32 gAreaLightColors_2[3][2][4] = {
#include "gm_sub/00928F00.inc"
};
AreaLightZone gAreaLightZones_3[2] = {
#include "gm_sub/00928F60.inc"
};
f32 gAreaLightColors_3[2][2][4] = {
#include "gm_sub/00928F90.inc"
};
AreaLightZone gAreaLightZones_4[2] = {
#include "gm_sub/00928FD0.inc"
};
f32 gAreaLightColors_4[3][2][4] = {
#include "gm_sub/00929000.inc"
};
AreaLightZone gAreaLightZones_5[2] = {
#include "gm_sub/00929060.inc"
};
f32 gAreaLightColors_5[3][2][4] = {
#include "gm_sub/00929090.inc"
};
AreaLightZone gAreaLightZones_18[2] = {
#include "gm_sub/009290F0.inc"
};
f32 gAreaLightColors_18[3][2][4] = {
#include "gm_sub/00929120.inc"
};
AreaLightZone gAreaLightZones_19[2] = {
#include "gm_sub/00929180.inc"
};
f32 gAreaLightColors_19[3][2][4] = {
#include "gm_sub/009291B0.inc"
};
AreaLightZone gAreaLightZones_22[2] = {
#include "gm_sub/00929210.inc"
};
f32 gAreaLightColors_22[3][2][4] = {
#include "gm_sub/00929240.inc"
};
AreaLightZone gAreaLightZones_23[2] = {
#include "gm_sub/009292A0.inc"
};
f32 gAreaLightColors_23[3][2][4] = {
#include "gm_sub/009292D0.inc"
};
AreaLightZone gAreaLightZones_26[2] = {
#include "gm_sub/00929330.inc"
};
f32 gAreaLightColors_26[3][2][4] = {
#include "gm_sub/00929360.inc"
};
AreaLightZone gAreaLightZonesAlt_24[2] = {
#include "gm_sub/009293C0.inc"
};
f32 gAreaLightColors_120[3][2][4] = {
#include "gm_sub/009293F0.inc"
};
AreaLightZone gAreaLightZonesAlt_25[2] = {
#include "gm_sub/00929450.inc"
};
f32 gAreaLightColors_121[3][2][4] = {
#include "gm_sub/00929480.inc"
};
AreaLightZone gAreaLightZonesAlt_26[2] = {
#include "gm_sub/009294E0.inc"
};
f32 gAreaLightColors_122[3][2][4] = {
#include "gm_sub/00929510.inc"
};
AreaLightZone gAreaLightZonesAlt_27[2] = {
#include "gm_sub/00929570.inc"
};
f32 gAreaLightColors_123[3][2][4] = {
#include "gm_sub/009295A0.inc"
};
AreaLightZone gAreaLightZonesAlt_28[2] = {
#include "gm_sub/00929600.inc"
};
f32 gAreaLightColors_124[3][2][4] = {
#include "gm_sub/00929630.inc"
};
AreaLightZone gAreaLightZones_83[2] = {
#include "gm_sub/00929690.inc"
};
f32 gAreaLightColors_83[3][2][4] = {
#include "gm_sub/009296C0.inc"
};
AreaLightZone gAreaLightZonesAlt_30[2] = {
#include "gm_sub/00929720.inc"
};
f32 gAreaLightColors_126[3][2][4] = {
#include "gm_sub/00929750.inc"
};
AreaLightZone gAreaLightZones_84[2] = {
#include "gm_sub/009297B0.inc"
};
f32 gAreaLightColors_84[3][2][4] = {
#include "gm_sub/009297E0.inc"
};
AreaLightZone gAreaLightZonesAlt_33[2] = {
#include "gm_sub/00929840.inc"
};
f32 gAreaLightColors_129[3][2][4] = {
#include "gm_sub/00929870.inc"
};
AreaLightZone gAreaLightZonesAlt_34[2] = {
#include "gm_sub/009298D0.inc"
};
f32 gAreaLightColors_130[3][2][4] = {
#include "gm_sub/00929900.inc"
};
AreaLightZone gAreaLightZones_45[2] = {
#include "gm_sub/00929960.inc"
};
f32 gAreaLightColors_45[3][2][4] = {
#include "gm_sub/00929990.inc"
};
AreaLightZone gAreaLightZones_47[2] = {
#include "gm_sub/009299F0.inc"
};
f32 gAreaLightColors_47[3][2][4] = {
#include "gm_sub/00929A20.inc"
};
AreaLightZone gAreaLightZones_48[2] = {
#include "gm_sub/00929A80.inc"
};
f32 gAreaLightColors_48[3][2][4] = {
#include "gm_sub/00929AB0.inc"
};
AreaLightZone gAreaLightZones_49[2] = {
#include "gm_sub/00929B10.inc"
};
f32 gAreaLightColors_49[3][2][4] = {
#include "gm_sub/00929B40.inc"
};
AreaLightZone gAreaLightZonesAlt_48[2] = {
#include "gm_sub/00929BA0.inc"
};
f32 gAreaLightColors_144[3][2][4] = {
#include "gm_sub/00929BD0.inc"
};
AreaLightZone gAreaLightZones_51[2] = {
#include "gm_sub/00929C30.inc"
};
f32 gAreaLightColors_51[3][2][4] = {
#include "gm_sub/00929C60.inc"
};
AreaLightZone gAreaLightZones_53[2] = {
#include "gm_sub/00929CC0.inc"
};
f32 gAreaLightColors_53[3][2][4] = {
#include "gm_sub/00929CF0.inc"
};
AreaLightZone gAreaLightZones_54[2] = {
#include "gm_sub/00929D50.inc"
};
f32 gAreaLightColors_54[3][2][4] = {
#include "gm_sub/00929D80.inc"
};
AreaLightZone gAreaLightZones_55[2] = {
#include "gm_sub/00929DE0.inc"
};
f32 gAreaLightColors_55[3][2][4] = {
#include "gm_sub/00929E10.inc"
};
AreaLightZone gAreaLightZones_56[2] = {
#include "gm_sub/00929E70.inc"
};
f32 gAreaLightColors_56[3][2][4] = {
#include "gm_sub/00929EA0.inc"
};
AreaLightZone gAreaLightZones_44[2] = {
#include "gm_sub/00929F00.inc"
};
f32 gAreaLightColors_44[3][2][4] = {
#include "gm_sub/00929F30.inc"
};
AreaLightZone gAreaLightZones_67[2] = {
#include "gm_sub/00929F90.inc"
};
f32 gAreaLightColors_67[3][2][4] = {
#include "gm_sub/00929FC0.inc"
};
AreaLightZone gAreaLightZonesAlt_63[2] = {
#include "gm_sub/0092A020.inc"
};
f32 gAreaLightColors_159[3][2][4] = {
#include "gm_sub/0092A050.inc"
};
AreaLightZone gAreaLightZones_58[2] = {
#include "gm_sub/0092A0B0.inc"
};
f32 gAreaLightColors_58[3][2][4] = {
#include "gm_sub/0092A0E0.inc"
};
AreaLightZone gAreaLightZonesAlt_66[2] = {
#include "gm_sub/0092A140.inc"
};
f32 gAreaLightColors_162[3][2][4] = {
#include "gm_sub/0092A170.inc"
};
AreaLightZone gAreaLightZonesAlt_73[2] = {
#include "gm_sub/0092A1D0.inc"
};
f32 gAreaLightColors_169[3][2][4] = {
#include "gm_sub/0092A200.inc"
};
AreaLightZone gAreaLightZonesAlt_81[2] = {
#include "gm_sub/0092A260.inc"
};
f32 gAreaLightColors_177[3][2][4] = {
#include "gm_sub/0092A290.inc"
};
AreaLightZone gAreaLightZonesAlt_82[2] = {
#include "gm_sub/0092A2F0.inc"
};
f32 gAreaLightColors_178[3][2][4] = {
#include "gm_sub/0092A320.inc"
};
Vec3f gPlayerResetPos[26] = {
#include "gm_sub/0092A380.inc"
};
s32 gPlayerResetRotY[26] = {
#include "gm_sub/0092A4C0.inc"
};
f32 gAmbSePosSwampNight7[2][3] = {
#include "gm_sub/0092A530.inc"
};
s32 gAmbSeIdsSwampNight7[2] = {
#include "gm_sub/0092A548.inc"
};
s16 gAmbSePeriodsSwampNight7[2] = {
#include "gm_sub/0092A550.inc"
};
s16 gAmbSeKindsSwampNight7[2] = {
#include "gm_sub/0092A558.inc"
};
f32 gAmbSePosSwampNight9[3][3] = {
#include "gm_sub/0092A560.inc"
};
s32 gAmbSeIdsSwampNight9[3] = {
#include "gm_sub/0092A588.inc"
};
s16 gAmbSePeriodsSwampNight9[3] = {
#include "gm_sub/0092A598.inc"
};
s16 gAmbSeKindsSwampNight9[3] = {
#include "gm_sub/0092A5A0.inc"
};
f32 gAmbSePosJungle0[2][3] = {
#include "gm_sub/0092A5B0.inc"
};
s32 gAmbSeIdsJungle0[2] = {
#include "gm_sub/0092A5C8.inc"
};
s16 gAmbSePeriodsJungle0[2] = {
#include "gm_sub/0092A5D0.inc"
};
s16 gAmbSeKindsJungle0[2] = {
#include "gm_sub/0092A5D8.inc"
};
f32 gAmbSePosJungle1[1][3] = {
#include "gm_sub/0092A5E0.inc"
};
s32 gAmbSeIdsJungle1[1] = {
#include "gm_sub/0092A5F0.inc"
};
s16 gAmbSePeriodsJungle1[1] = {
#include "gm_sub/0092A5F8.inc"
};
s16 gAmbSeKindsJungle1[1] = {
#include "gm_sub/0092A600.inc"
};
f32 gAmbSePosJungle4[1][3] = {
#include "gm_sub/0092A608.inc"
};
s32 gAmbSeIdsJungle4[1] = {
#include "gm_sub/0092A618.inc"
};
s16 gAmbSePeriodsJungle4[1] = {
#include "gm_sub/0092A620.inc"
};
s16 gAmbSeKindsJungle4[2] = {
#include "gm_sub/0092A628.inc"
};
f32 gAmbSePosJungle7[2][3] = {
#include "gm_sub/0092A630.inc"
};
s32 gAmbSeIdsJungle7[2] = {
#include "gm_sub/0092A648.inc"
};
s16 gAmbSePeriodsJungle7[2] = {
#include "gm_sub/0092A650.inc"
};
s16 gAmbSeKindsJungle7[2] = {
#include "gm_sub/0092A658.inc"
};
f32 gAmbSePosJungle8[1][3] = {
#include "gm_sub/0092A660.inc"
};
s32 gAmbSeIdsJungle8[1] = {
#include "gm_sub/0092A670.inc"
};
s16 gAmbSePeriodsJungle8[1] = {
#include "gm_sub/0092A678.inc"
};
s16 gAmbSeKindsJungle8[1] = {
#include "gm_sub/0092A680.inc"
};
f32 gAmbSePosTower0[1][3] = {
#include "gm_sub/0092A688.inc"
};
s32 gAmbSeIdsTower0[1] = {
#include "gm_sub/0092A698.inc"
};
s16 gAmbSePeriodsTower0[1] = {
#include "gm_sub/0092A6A0.inc"
};
s16 gAmbSeKindsTower0[1] = {
#include "gm_sub/0092A6A8.inc"
};
f32 gAmbSePosTower1[1][3] = {
#include "gm_sub/0092A6B0.inc"
};
s32 gAmbSeIdsTower1[1] = {
#include "gm_sub/0092A6C0.inc"
};
s16 gAmbSePeriodsTower1[1] = {
#include "gm_sub/0092A6C8.inc"
};
s16 gAmbSeKindsTower1[1] = {
#include "gm_sub/0092A6D0.inc"
};
f32 gAmbSePosVolcano7[1][3] = {
#include "gm_sub/0092A6D8.inc"
};
s32 gAmbSeIdsVolcano7[1] = {
#include "gm_sub/0092A6E8.inc"
};
s16 gAmbSePeriodsVolcano7[1] = {
#include "gm_sub/0092A6F0.inc"
};
s16 gAmbSeKindsVolcano7[1] = {
#include "gm_sub/0092A6F8.inc"
};
f32 gAmbSePosVolcano8[1][3] = {
#include "gm_sub/0092A700.inc"
};
s32 gAmbSeIdsVolcano8[1] = {
#include "gm_sub/0092A710.inc"
};
s16 gAmbSePeriodsVolcano8[1] = {
#include "gm_sub/0092A718.inc"
};
s16 gAmbSeKindsVolcano8[1] = {
#include "gm_sub/0092A720.inc"
};
AnimRot D_0092A730[1] = {
#include "gm_sub/0092A730.inc"
};
AnimRot D_0092A740[3] = {
#include "gm_sub/0092A740.inc"
};
AnimRot D_0092A770[4] = {
#include "gm_sub/0092A770.inc"
};
AnimRot D_0092A7B0[2] = {
#include "gm_sub/0092A7B0.inc"
};
AnimRot D_0092A7D0[2] = {
#include "gm_sub/0092A7D0.inc"
};
AnimRot D_0092A7F0[2] = {
#include "gm_sub/0092A7F0.inc"
};
AnimRot D_0092A810[3] = {
#include "gm_sub/0092A810.inc"
};
AnimRot D_0092A840[2] = {
#include "gm_sub/0092A840.inc"
};
AnimRot D_0092A860[2] = {
#include "gm_sub/0092A860.inc"
};
AnimRot D_0092A880[3] = {
#include "gm_sub/0092A880.inc"
};
AnimRot D_0092A8B0[6] = {
#include "gm_sub/0092A8B0.inc"
};
AnimRot D_0092A910[3] = {
#include "gm_sub/0092A910.inc"
};
AnimRot D_0092A940[1] = {
#include "gm_sub/0092A940.inc"
};
AnimRot D_0092A950[5] = {
#include "gm_sub/0092A950.inc"
};
f32 D_0092A9A0[4][2] = {
#include "gm_sub/0092A9A0.inc"
};
f32 D_0092A9C0[4][2] = {
#include "gm_sub/0092A9C0.inc"
};
AnimRot D_0092A9E0[5] = {
#include "gm_sub/0092A9E0.inc"
};
AnimRot D_0092AA30[5] = {
#include "gm_sub/0092AA30.inc"
};
f32 D_0092AA80[2][2] = {
#include "gm_sub/0092AA80.inc"
};
f32 D_0092AA90[3][2] = {
#include "gm_sub/0092AA90.inc"
};
AnimRot D_0092AAB0[4] = {
#include "gm_sub/0092AAB0.inc"
};
AnimRot D_0092AAF0[2] = {
#include "gm_sub/0092AAF0.inc"
};
AnimRot D_0092AB10[2] = {
#include "gm_sub/0092AB10.inc"
};
AnimRot D_0092AB30[2] = {
#include "gm_sub/0092AB30.inc"
};
AnimRot D_0092AB50[4] = {
#include "gm_sub/0092AB50.inc"
};
AnimRot D_0092AB90[4] = {
#include "gm_sub/0092AB90.inc"
};
AnimRot D_0092ABD0[2] = {
#include "gm_sub/0092ABD0.inc"
};
AnimRot D_0092ABF0[1] = {
#include "gm_sub/0092ABF0.inc"
};
AnimRot D_0092AC00[1] = {
#include "gm_sub/0092AC00.inc"
};
AnimRot D_0092AC10[1] = {
#include "gm_sub/0092AC10.inc"
};
AnimRot D_0092AC20[1] = {
#include "gm_sub/0092AC20.inc"
};
AnimRot D_0092AC30[1] = {
#include "gm_sub/0092AC30.inc"
};
AnimRot D_0092AC40[6] = {
#include "gm_sub/0092AC40.inc"
};
AnimRot D_0092ACA0[5] = {
#include "gm_sub/0092ACA0.inc"
};
AnimRot D_0092ACF0[4] = {
#include "gm_sub/0092ACF0.inc"
};
AnimRot D_0092AD30[3] = {
#include "gm_sub/0092AD30.inc"
};
s16 D_0092AD60[119][16] = {
#include "gm_sub/0092AD60.inc"
};
GatherEntry gGatherItemTables_0[8] = {
#include "gm_sub/0092BC40.inc"
};
GatherEntry gGatherItemTables_1[8] = {
#include "gm_sub/0092BC60.inc"
};
GatherEntry gGatherItemTables_2[4] = {
#include "gm_sub/0092BC80.inc"
};
GatherEntry gGatherItemTables_3[4] = {
#include "gm_sub/0092BC90.inc"
};
GatherEntry gGatherItemTables_4[8] = {
#include "gm_sub/0092BCA0.inc"
};
GatherEntry gGatherItemTables_5[12] = {
#include "gm_sub/0092BCC0.inc"
};
GatherEntry gGatherItemTables_7[16] = {
#include "gm_sub/0092BCF0.inc"
};
GatherEntry gGatherItemTables_10[8] = {
#include "gm_sub/0092BD30.inc"
};
GatherEntry gGatherItemTables_11[12] = {
#include "gm_sub/0092BD50.inc"
};
GatherEntry gGatherItemTables_13[8] = {
#include "gm_sub/0092BD80.inc"
};
GatherEntry gGatherItemTables_14[40] = {
#include "gm_sub/0092BDA0.inc"
};
GatherEntry gGatherItemTables_23[8] = {
#include "gm_sub/0092BE40.inc"
};
GatherEntry gGatherItemTables_24[10] = {
#include "gm_sub/0092BE60.inc"
};
GatherEntry gGatherItemTables_26[3] = {
#include "gm_sub/0092BE88.inc"
};
GatherEntry gGatherItemTables_27[8] = {
#include "gm_sub/0092BEA0.inc"
};
GatherEntry gGatherItemTables_28[12] = {
#include "gm_sub/0092BEC0.inc"
};
GatherEntry gGatherItemTables_30[12] = {
#include "gm_sub/0092BEF0.inc"
};
GatherEntry gGatherItemTables_32[6] = {
#include "gm_sub/0092BF20.inc"
};
GatherEntry gGatherItemTables_33[3] = {
#include "gm_sub/0092BF38.inc"
};
GatherEntry gGatherItemTables_34[3] = {
#include "gm_sub/0092BF48.inc"
};
GatherEntry gGatherItemTables_35[8] = {
#include "gm_sub/0092BF60.inc"
};
GatherEntry gGatherItemTables_36[12] = {
#include "gm_sub/0092BF80.inc"
};
GatherEntry gGatherItemTables_38[6] = {
#include "gm_sub/0092BFB0.inc"
};
GatherEntry gGatherItemTables_39[3] = {
#include "gm_sub/0092BFC8.inc"
};
GatherEntry gGatherItemTables_40[3] = {
#include "gm_sub/0092BFD8.inc"
};
GatherEntry gGatherItemTables_41[8] = {
#include "gm_sub/0092BFF0.inc"
};
GatherEntry gGatherItemTables_42[12] = {
#include "gm_sub/0092C010.inc"
};
GatherEntry gGatherItemTables_44[8] = {
#include "gm_sub/0092C040.inc"
};
GatherEntry gGatherItemTables_45[8] = {
#include "gm_sub/0092C060.inc"
};
GatherEntry gGatherItemTables_46[12] = {
#include "gm_sub/0092C080.inc"
};
GatherEntry gGatherItemTables_48[8] = {
#include "gm_sub/0092C0B0.inc"
};
GatherEntry gGatherItemTables_49[8] = {
#include "gm_sub/0092C0D0.inc"
};
GatherEntry gGatherItemTables_50[8] = {
#include "gm_sub/0092C0F0.inc"
};
GatherEntry gGatherItemTables_51[4] = {
#include "gm_sub/0092C110.inc"
};
GatherEntry gGatherItemTables_52[12] = {
#include "gm_sub/0092C120.inc"
};
GatherEntry gGatherItemTables_54[12] = {
#include "gm_sub/0092C150.inc"
};
GatherEntry gGatherItemTables_56[16] = {
#include "gm_sub/0092C180.inc"
};
GatherEntry gGatherItemTables_59[8] = {
#include "gm_sub/0092C1C0.inc"
};
GatherEntry gGatherItemTables_60[6] = {
#include "gm_sub/0092C1E0.inc"
};
GatherEntry gGatherItemTables_61[2] = {
#include "gm_sub/0092C1F8.inc"
};
GatherEntry gGatherItemTables_62[8] = {
#include "gm_sub/0092C200.inc"
};
GatherEntry gGatherItemTables_63[6] = {
#include "gm_sub/0092C220.inc"
};
GatherEntry gGatherItemTables_64[2] = {
#include "gm_sub/0092C238.inc"
};
GatherEntry gGatherItemTables_65[4] = {
#include "gm_sub/0092C240.inc"
};
GatherEntry gGatherItemTables_66[4] = {
#include "gm_sub/0092C250.inc"
};
GatherEntry gGatherItemTables_67[4] = {
#include "gm_sub/0092C260.inc"
};
GatherEntry gGatherItemTables_68[6] = {
#include "gm_sub/0092C270.inc"
};
GatherEntry gGatherItemTables_69[3] = {
#include "gm_sub/0092C288.inc"
};
GatherEntry gGatherItemTables_70[3] = {
#include "gm_sub/0092C298.inc"
};
GatherEntry gGatherItemTables_71[12] = {
#include "gm_sub/0092C2B0.inc"
};
GatherEntry gGatherItemTables_73[12] = {
#include "gm_sub/0092C2E0.inc"
};
GatherEntry gGatherItemTables_75[10] = {
#include "gm_sub/0092C310.inc"
};
GatherEntry gGatherItemTables_77[3] = {
#include "gm_sub/0092C338.inc"
};
GatherEntry gGatherItemTables_78[6] = {
#include "gm_sub/0092C350.inc"
};
GatherEntry gGatherItemTables_79[3] = {
#include "gm_sub/0092C368.inc"
};
GatherEntry gGatherItemTables_80[8] = {
#include "gm_sub/0092C380.inc"
};
GatherEntry gGatherItemTables_81[12] = {
#include "gm_sub/0092C3A0.inc"
};
GatherEntry gGatherItemTables_83[8] = {
#include "gm_sub/0092C3D0.inc"
};
GatherEntry gGatherItemTables_84[8] = {
#include "gm_sub/0092C3F0.inc"
};
GatherEntry gGatherItemTables_85[28] = {
#include "gm_sub/0092C410.inc"
};
GatherEntry gGatherItemTables_91[8] = {
#include "gm_sub/0092C480.inc"
};
GatherEntry gGatherItemTables_92[28] = {
#include "gm_sub/0092C4A0.inc"
};
GatherEntry gGatherItemTables_98[16] = {
#include "gm_sub/0092C510.inc"
};
GatherEntry gGatherItemTables_101[6] = {
#include "gm_sub/0092C550.inc"
};
GatherEntry gGatherItemTables_102[3] = {
#include "gm_sub/0092C568.inc"
};
GatherEntry gGatherItemTables_103[3] = {
#include "gm_sub/0092C578.inc"
};
GatherEntry gGatherItemTables_104[24] = {
#include "gm_sub/0092C590.inc"
};
GatherEntry gGatherItemTables_109[8] = {
#include "gm_sub/0092C5F0.inc"
};
GatherEntry gGatherItemTables_110[10] = {
#include "gm_sub/0092C610.inc"
};
GatherEntry gGatherItemTables_112[3] = {
#include "gm_sub/0092C638.inc"
};
GatherEntry gGatherItemTables_113[20] = {
#include "gm_sub/0092C650.inc"
};
GatherEntry gGatherItemTables_117[6] = {
#include "gm_sub/0092C6A0.inc"
};
GatherEntry gGatherItemTables_118[3] = {
#include "gm_sub/0092C6B8.inc"
};
GatherEntry gGatherItemTables_119[8] = {
#include "gm_sub/0092C6D0.inc"
};
GatherEntry gGatherItemTables_120[12] = {
#include "gm_sub/0092C6F0.inc"
};
GatherEntry gGatherItemTables_122[8] = {
#include "gm_sub/0092C720.inc"
};
GatherEntry gGatherItemTables_123[8] = {
#include "gm_sub/0092C740.inc"
};
GatherEntry gGatherItemTables_124[8] = {
#include "gm_sub/0092C760.inc"
};
GatherEntry gGatherItemTables_125[8] = {
#include "gm_sub/0092C780.inc"
};
GatherEntry gGatherItemTables_126[4] = {
#include "gm_sub/0092C7A0.inc"
};
GatherEntry gGatherItemTables_127[4] = {
#include "gm_sub/0092C7B0.inc"
};
GatherEntry gGatherItemTables_128[8] = {
#include "gm_sub/0092C7C0.inc"
};
GatherEntry gGatherItemTables_129[12] = {
#include "gm_sub/0092C7E0.inc"
};
GatherEntry gGatherItemTables_131[8] = {
#include "gm_sub/0092C810.inc"
};
GatherEntry gGatherItemTables_132[4] = {
#include "gm_sub/0092C830.inc"
};
GatherEntry gGatherItemTables_133[8] = {
#include "gm_sub/0092C840.inc"
};
GatherEntry gGatherItemTables_134[8] = {
#include "gm_sub/0092C860.inc"
};
GatherEntry gGatherItemTables_135[8] = {
#include "gm_sub/0092C880.inc"
};
GatherEntry gGatherItemTables_136[4] = {
#include "gm_sub/0092C8A0.inc"
};
GatherEntry gGatherItemTables_137[8] = {
#include "gm_sub/0092C8B0.inc"
};
GatherEntry gGatherItemTables_138[8] = {
#include "gm_sub/0092C8D0.inc"
};
GatherEntry gGatherItemTables_139[4] = {
#include "gm_sub/0092C8F0.inc"
};
GatherEntry gGatherItemTables_140[4] = {
#include "gm_sub/0092C900.inc"
};
GatherEntry gGatherItemTables_141[8] = {
#include "gm_sub/0092C910.inc"
};
GatherEntry gGatherItemTables_142[10] = {
#include "gm_sub/0092C930.inc"
};
GatherEntry gGatherItemTables_144[3] = {
#include "gm_sub/0092C958.inc"
};
GatherEntry gGatherItemTables_145[12] = {
#include "gm_sub/0092C970.inc"
};
GatherEntry gGatherItemTables_147[8] = {
#include "gm_sub/0092C9A0.inc"
};
GatherEntry gGatherItemTables_148[8] = {
#include "gm_sub/0092C9C0.inc"
};
GatherEntry gGatherItemTables_149[6] = {
#include "gm_sub/0092C9E0.inc"
};
GatherEntry gGatherItemTables_150[3] = {
#include "gm_sub/0092C9F8.inc"
};
GatherEntry gGatherItemTables_151[8] = {
#include "gm_sub/0092CA10.inc"
};
GatherEntry gGatherItemTables_152[8] = {
#include "gm_sub/0092CA30.inc"
};
GatherEntry gGatherItemTables_153[4] = {
#include "gm_sub/0092CA50.inc"
};
GatherEntry gGatherItemTables_154[12] = {
#include "gm_sub/0092CA60.inc"
};
GatherEntry gGatherItemTables_156[8] = {
#include "gm_sub/0092CA90.inc"
};
GatherEntry gGatherItemTables_157[6] = {
#include "gm_sub/0092CAB0.inc"
};
GatherEntry gGatherItemTables_158[3] = {
#include "gm_sub/0092CAC8.inc"
};
GatherEntry gGatherItemTables_159[8] = {
#include "gm_sub/0092CAE0.inc"
};
GatherEntry gGatherItemTables_160[12] = {
#include "gm_sub/0092CB00.inc"
};
GatherEntry gGatherItemTables_162[6] = {
#include "gm_sub/0092CB30.inc"
};
GatherEntry gGatherItemTables_163[3] = {
#include "gm_sub/0092CB48.inc"
};
GatherEntry gGatherItemTables_164[2] = {
#include "gm_sub/0092CB58.inc"
};
GatherEntry gGatherItemTables_165[8] = {
#include "gm_sub/0092CB60.inc"
};
GatherEntry gGatherItemTables_166[8] = {
#include "gm_sub/0092CB80.inc"
};
GatherEntry gGatherItemTables_167[8] = {
#include "gm_sub/0092CBA0.inc"
};
GatherEntry gGatherItemTables_168[8] = {
#include "gm_sub/0092CBC0.inc"
};
GatherEntry gGatherItemTables_169[8] = {
#include "gm_sub/0092CBE0.inc"
};
GatherEntry gGatherItemTables_170[12] = {
#include "gm_sub/0092CC00.inc"
};
GatherEntry gGatherItemTables_172[8] = {
#include "gm_sub/0092CC30.inc"
};
GatherEntry gGatherItemTables_173[8] = {
#include "gm_sub/0092CC50.inc"
};
GatherEntry gGatherItemTables_174[8] = {
#include "gm_sub/0092CC70.inc"
};
GatherEntry gGatherItemTables_175[4] = {
#include "gm_sub/0092CC90.inc"
};
GatherEntry gGatherItemTables_176[8] = {
#include "gm_sub/0092CCA0.inc"
};
GatherEntry gGatherItemTables_177[4] = {
#include "gm_sub/0092CCC0.inc"
};
GatherEntry gGatherItemTables_178[8] = {
#include "gm_sub/0092CCD0.inc"
};
GatherEntry gGatherItemTables_179[4] = {
#include "gm_sub/0092CCF0.inc"
};
GatherEntry gGatherItemTables_180[6] = {
#include "gm_sub/0092CD00.inc"
};
GatherEntry gGatherItemTables_181[3] = {
#include "gm_sub/0092CD18.inc"
};
GatherEntry gGatherItemTables_182[2] = {
#include "gm_sub/0092CD28.inc"
};
GatherEntry gGatherItemTables_183[8] = {
#include "gm_sub/0092CD30.inc"
};
GatherEntry gGatherItemTables_184[12] = {
#include "gm_sub/0092CD50.inc"
};
GatherEntry gGatherItemTables_186[8] = {
#include "gm_sub/0092CD80.inc"
};
GatherEntry gGatherItemTables_187[12] = {
#include "gm_sub/0092CDA0.inc"
};
GatherEntry gGatherItemTables_189[10] = {
#include "gm_sub/0092CDD0.inc"
};
GatherEntry gGatherItemTables_191[3] = {
#include "gm_sub/0092CDF8.inc"
};
GatherEntry gGatherItemTables_192[6] = {
#include "gm_sub/0092CE10.inc"
};
GatherEntry gGatherItemTables_193[3] = {
#include "gm_sub/0092CE28.inc"
};
GatherEntry gGatherItemTables_194[2] = {
#include "gm_sub/0092CE38.inc"
};
GatherEntry gGatherItemTables_195[12] = {
#include "gm_sub/0092CE40.inc"
};
GatherEntry gGatherItemTables_197[12] = {
#include "gm_sub/0092CE70.inc"
};
GatherEntry gGatherItemTables_199[8] = {
#include "gm_sub/0092CEA0.inc"
};
GatherEntry gGatherItemTables_200[6] = {
#include "gm_sub/0092CEC0.inc"
};
GatherEntry gGatherItemTables_201[3] = {
#include "gm_sub/0092CED8.inc"
};
GatherEntry gGatherItemTables_202[6] = {
#include "gm_sub/0092CEF0.inc"
};
GatherEntry gGatherItemTables_203[3] = {
#include "gm_sub/0092CF08.inc"
};
GatherEntry gGatherItemTables_204[8] = {
#include "gm_sub/0092CF20.inc"
};
GatherEntry gGatherItemTables_205[8] = {
#include "gm_sub/0092CF40.inc"
};
GatherEntry gGatherItemTables_206[4] = {
#include "gm_sub/0092CF60.inc"
};
GatherEntry gGatherItemTables_207[8] = {
#include "gm_sub/0092CF70.inc"
};
GatherEntry gGatherItemTables_208[8] = {
#include "gm_sub/0092CF90.inc"
};
GatherEntry gGatherItemTables_209[12] = {
#include "gm_sub/0092CFB0.inc"
};
GatherEntry gGatherItemTables_211[20] = {
#include "gm_sub/0092CFE0.inc"
};
GatherEntry gGatherItemTables_215[8] = {
#include "gm_sub/0092D030.inc"
};
GatherEntry gGatherItemTables_216[24] = {
#include "gm_sub/0092D050.inc"
};
GatherEntry gGatherItemTables_221[8] = {
#include "gm_sub/0092D0B0.inc"
};
GatherEntry gGatherItemTables_222[8] = {
#include "gm_sub/0092D0D0.inc"
};
GatherEntry gGatherItemTables_223[16] = {
#include "gm_sub/0092D0F0.inc"
};
GatherEntry gGatherItemTables_226[8] = {
#include "gm_sub/0092D130.inc"
};
GatherEntry gGatherItemTables_227[4] = {
#include "gm_sub/0092D150.inc"
};
GatherEntry gGatherItemTables_228[4] = {
#include "gm_sub/0092D160.inc"
};
GatherEntry gGatherItemTables_229[24] = {
#include "gm_sub/0092D170.inc"
};
GatherEntry gGatherItemTables_234[8] = {
#include "gm_sub/0092D1D0.inc"
};
GatherEntry gGatherItemTables_235[8] = {
#include "gm_sub/0092D1F0.inc"
};
GatherEntry gGatherItemTables_236[6] = {
#include "gm_sub/0092D210.inc"
};
GatherEntry gGatherItemTables_237[3] = {
#include "gm_sub/0092D228.inc"
};
GatherEntry gGatherItemTables_238[8] = {
#include "gm_sub/0092D240.inc"
};
GatherEntry gGatherItemTables_239[12] = {
#include "gm_sub/0092D260.inc"
};
GatherEntry gGatherItemTables_241[8] = {
#include "gm_sub/0092D290.inc"
};
GatherEntry gGatherItemTables_242[8] = {
#include "gm_sub/0092D2B0.inc"
};
GatherEntry gGatherItemTables_243[4] = {
#include "gm_sub/0092D2D0.inc"
};
GatherEntry gGatherItemTables_244[8] = {
#include "gm_sub/0092D2E0.inc"
};
GatherEntry gGatherItemTables_245[12] = {
#include "gm_sub/0092D300.inc"
};
GatherEntry gGatherItemTables_247[8] = {
#include "gm_sub/0092D330.inc"
};
GatherEntry gGatherItemTables_248[8] = {
#include "gm_sub/0092D350.inc"
};
GatherEntry gGatherItemTables_249[24] = {
#include "gm_sub/0092D370.inc"
};
GatherEntry gGatherItemTables_254[12] = {
#include "gm_sub/0092D3D0.inc"
};
GatherEntry gGatherItemTables_256[20] = {
#include "gm_sub/0092D400.inc"
};
GatherEntry gGatherItemTables_260[12] = {
#include "gm_sub/0092D450.inc"
};
GatherEntry gGatherItemTables_262[8] = {
#include "gm_sub/0092D480.inc"
};
GatherEntry gGatherItemTables_263[8] = {
#include "gm_sub/0092D4A0.inc"
};
GatherEntry gGatherItemTables_264[12] = {
#include "gm_sub/0092D4C0.inc"
};
GatherEntry gGatherItemTables_267[10] = {
#include "gm_sub/0092D4F0.inc"
};
GatherEntry gGatherItemTables_269[3] = {
#include "gm_sub/0092D518.inc"
};
GatherEntry gGatherItemTables_270[16] = {
#include "gm_sub/0092D530.inc"
};
GatherEntry gGatherItemTables_273[8] = {
#include "gm_sub/0092D570.inc"
};
GatherEntry gGatherItemTables_274[8] = {
#include "gm_sub/0092D590.inc"
};
GatherEntry gGatherItemTables_275[8] = {
#include "gm_sub/0092D5B0.inc"
};
GatherEntry gGatherItemTables_277[12] = {
#include "gm_sub/0092D5D0.inc"
};
GatherEntry gGatherItemTables_279[8] = {
#include "gm_sub/0092D600.inc"
};
GatherEntry gGatherItemTables_280[4] = {
#include "gm_sub/0092D620.inc"
};
GatherEntry gGatherItemTables_281[8] = {
#include "gm_sub/0092D630.inc"
};
GatherEntry gGatherItemTables_282[8] = {
#include "gm_sub/0092D650.inc"
};
GatherEntry gGatherItemTables_283[2] = {
#include "gm_sub/0092D670.inc"
};
GatherEntry gGatherItemTables_284[2] = {
#include "gm_sub/0092D678.inc"
};
GatherEntry gGatherItemTables_285[4] = {
#include "gm_sub/0092D680.inc"
};
GatherEntry gGatherItemTables_286[8] = {
#include "gm_sub/0092D690.inc"
};
GatherEntry gGatherItemTables_287[8] = {
#include "gm_sub/0092D6B0.inc"
};
GatherEntry gGatherItemTables_288[8] = {
#include "gm_sub/0092D6D0.inc"
};
GatherEntry gGatherItemTables_289[4] = {
#include "gm_sub/0092D6F0.inc"
};
GatherEntry gGatherItemTables_290[8] = {
#include "gm_sub/0092D700.inc"
};
GatherEntry gGatherItemTables_291[12] = {
#include "gm_sub/0092D720.inc"
};
GatherEntry gGatherItemTables_293[8] = {
#include "gm_sub/0092D750.inc"
};
GatherEntry gGatherItemTables_294[16] = {
#include "gm_sub/0092D770.inc"
};
GatherEntry gGatherItemTables_297[8] = {
#include "gm_sub/0092D7B0.inc"
};
GatherEntry gGatherItemTables_298[8] = {
#include "gm_sub/0092D7D0.inc"
};
GatherEntry gGatherItemTables_299[8] = {
#include "gm_sub/0092D7F0.inc"
};
GatherEntry gGatherItemTables_300[4] = {
#include "gm_sub/0092D810.inc"
};
GatherEntry gGatherItemTables_301[6] = {
#include "gm_sub/0092D820.inc"
};
GatherEntry gGatherItemTables_302[2] = {
#include "gm_sub/0092D838.inc"
};
GatherEntry gGatherItemTables_303[6] = {
#include "gm_sub/0092D840.inc"
};
GatherEntry gGatherItemTables_304[2] = {
#include "gm_sub/0092D858.inc"
};
GatherEntry gGatherItemTables_305[6] = {
#include "gm_sub/0092D860.inc"
};
GatherEntry gGatherItemTables_306[3] = {
#include "gm_sub/0092D878.inc"
};
GatherEntry gGatherItemTables_307[2] = {
#include "gm_sub/0092D888.inc"
};
GatherEntry gGatherItemTables_308[8] = {
#include "gm_sub/0092D890.inc"
};
GatherEntry gGatherItemTables_309[6] = {
#include "gm_sub/0092D8B0.inc"
};
GatherEntry gGatherItemTables_310[2] = {
#include "gm_sub/0092D8C8.inc"
};
GatherEntry gGatherItemTables_311[8] = {
#include "gm_sub/0092D8D0.inc"
};
GatherEntry gGatherItemTables_312[6] = {
#include "gm_sub/0092D8F0.inc"
};
GatherEntry gGatherItemTables_313[2] = {
#include "gm_sub/0092D908.inc"
};
GatherEntry gGatherItemTables_314[4] = {
#include "gm_sub/0092D910.inc"
};
GatherEntry gGatherItemTables_315[4] = {
#include "gm_sub/0092D920.inc"
};
GatherEntry gGatherItemTables_316[4] = {
#include "gm_sub/0092D930.inc"
};
GatherEntry gGatherItemTables_317[6] = {
#include "gm_sub/0092D940.inc"
};
GatherEntry gGatherItemTables_318[3] = {
#include "gm_sub/0092D958.inc"
};
GatherEntry gGatherItemTables_319[2] = {
#include "gm_sub/0092D968.inc"
};
GatherEntry gGatherItemTables_320[6] = {
#include "gm_sub/0092D970.inc"
};
GatherEntry gGatherItemTables_321[2] = {
#include "gm_sub/0092D988.inc"
};
GatherEntry gGatherItemTables_322[6] = {
#include "gm_sub/0092D990.inc"
};
GatherEntry gGatherItemTables_323[2] = {
#include "gm_sub/0092D9A8.inc"
};
GatherEntry gGatherItemTables_324[10] = {
#include "gm_sub/0092D9B0.inc"
};
GatherEntry gGatherItemTables_326[2] = {
#include "gm_sub/0092D9D8.inc"
};
GatherEntry gGatherItemTables_327[6] = {
#include "gm_sub/0092D9E0.inc"
};
GatherEntry gGatherItemTables_328[3] = {
#include "gm_sub/0092D9F8.inc"
};
GatherEntry gGatherItemTables_329[8] = {
#include "gm_sub/0092DA10.inc"
};
GatherEntry gGatherItemTables_330[6] = {
#include "gm_sub/0092DA30.inc"
};
GatherEntry gGatherItemTables_331[2] = {
#include "gm_sub/0092DA48.inc"
};
GatherEntry gGatherItemTables_332[12] = {
#include "gm_sub/0092DA50.inc"
};
GatherEntry gGatherItemTables_334[12] = {
#include "gm_sub/0092DA80.inc"
};
GatherEntry gGatherItemTables_336[20] = {
#include "gm_sub/0092DAB0.inc"
};
GatherEntry gGatherItemTables_340[8] = {
#include "gm_sub/0092DB00.inc"
};
GatherEntry gGatherItemTables_341[6] = {
#include "gm_sub/0092DB20.inc"
};
GatherEntry gGatherItemTables_342[3] = {
#include "gm_sub/0092DB38.inc"
};
GatherEntry gGatherItemTables_343[2] = {
#include "gm_sub/0092DB48.inc"
};
GatherEntry gGatherItemTables_344[4] = {
#include "gm_sub/0092DB50.inc"
};
GatherEntry gGatherItemTables_345[4] = {
#include "gm_sub/0092DB60.inc"
};
GatherEntry gGatherItemTables_346[4] = {
#include "gm_sub/0092DB70.inc"
};
GatherEntry gGatherItemTables_347[10] = {
#include "gm_sub/0092DB80.inc"
};
GatherEntry gGatherItemTables_349[2] = {
#include "gm_sub/0092DBA8.inc"
};
GatherEntry gGatherItemTables_350[4] = {
#include "gm_sub/0092DBB0.inc"
};
GatherEntry gGatherItemTables_351[4] = {
#include "gm_sub/0092DBC0.inc"
};
GatherEntry gGatherItemTables_352[4] = {
#include "gm_sub/0092DBD0.inc"
};
GatherEntry gGatherItemTables_353[6] = {
#include "gm_sub/0092DBE0.inc"
};
GatherEntry gGatherItemTables_354[3] = {
#include "gm_sub/0092DBF8.inc"
};
GatherEntry gGatherItemTables_355[2] = {
#include "gm_sub/0092DC08.inc"
};
GatherEntry gGatherItemTables_356[4] = {
#include "gm_sub/0092DC10.inc"
};
GatherEntry gGatherItemTables_357[4] = {
#include "gm_sub/0092DC20.inc"
};
GatherEntry gGatherItemTables_358[4] = {
#include "gm_sub/0092DC30.inc"
};
GatherEntry gGatherItemTables_359[10] = {
#include "gm_sub/0092DC40.inc"
};
GatherEntry gGatherItemTables_361[2] = {
#include "gm_sub/0092DC68.inc"
};
GatherEntry gGatherItemTables_362[10] = {
#include "gm_sub/0092DC70.inc"
};
GatherEntry gGatherItemTables_364[2] = {
#include "gm_sub/0092DC98.inc"
};
GatherEntry gGatherItemTables_365[8] = {
#include "gm_sub/0092DCA0.inc"
};
GatherEntry gGatherItemTables_366[6] = {
#include "gm_sub/0092DCC0.inc"
};
GatherEntry gGatherItemTables_367[3] = {
#include "gm_sub/0092DCD8.inc"
};
GatherEntry gGatherItemTables_368[16] = {
#include "gm_sub/0092DCF0.inc"
};
GatherEntry gGatherItemTables_371[8] = {
#include "gm_sub/0092DD30.inc"
};
GatherEntry gGatherItemTables_372[10] = {
#include "gm_sub/0092DD50.inc"
};
GatherEntry gGatherItemTables_374[2] = {
#include "gm_sub/0092DD78.inc"
};
GatherEntry gGatherItemTables_375[4] = {
#include "gm_sub/0092DD80.inc"
};
GatherEntry gGatherItemTables_376[4] = {
#include "gm_sub/0092DD90.inc"
};
GatherEntry *gGatherItemTables[GATHER_ITEM_TABLES] = {
#include "gm_sub/0092DDA0.inc"
};
EmMineEntry gEmMinePoints_Rathian[2] = {
#include "gm_sub/0092E390.inc"
};
EmMineEntry gEmMinePoints_Kelbi[1] = {
#include "gm_sub/0092E3B0.inc"
};
EmMineEntry gEmMinePoints_Mosswine[1] = {
#include "gm_sub/0092E3C0.inc"
};
EmMineEntry gEmMinePoints_Bullfango[1] = {
#include "gm_sub/0092E3D0.inc"
};
EmMineEntry gEmMinePoints_YianKutKu[1] = {
#include "gm_sub/0092E3E0.inc"
};
EmMineEntry gEmMinePoints_Cephadrome[1] = {
#include "gm_sub/0092E3F0.inc"
};
EmMineEntry gEmMinePoints_Rathalos[2] = {
#include "gm_sub/0092E400.inc"
};
EmMineEntry gEmMinePoints_12[2] = {
#include "gm_sub/0092E420.inc"
};
EmMineEntry gEmMinePoints_Genprey[1] = {
#include "gm_sub/0092E440.inc"
};
EmMineEntry gEmMinePoints_Diablos[2] = {
#include "gm_sub/0092E450.inc"
};
EmMineEntry gEmMinePoints_Khezu[1] = {
#include "gm_sub/0092E470.inc"
};
EmMineEntry gEmMinePoints_Velociprey[1] = {
#include "gm_sub/0092E480.inc"
};
EmMineEntry gEmMinePoints_Gravios[2] = {
#include "gm_sub/0092E490.inc"
};
EmMineEntry gEmMinePoints_Vespoid[1] = {
#include "gm_sub/0092E4B0.inc"
};
EmMineEntry gEmMinePoints_Gypceros[1] = {
#include "gm_sub/0092E4C0.inc"
};
EmMineEntry gEmMinePoints_Plesioth[1] = {
#include "gm_sub/0092E4D0.inc"
};
EmMineEntry gEmMinePoints_Basarios[2] = {
#include "gm_sub/0092E4E0.inc"
};
EmMineEntry gEmMinePoints_Melynx[1] = {
#include "gm_sub/0092E500.inc"
};
EmMineEntry gEmMinePoints_Hornetaur[1] = {
#include "gm_sub/0092E510.inc"
};
EmMineEntry gEmMinePoints_Apceros[1] = {
#include "gm_sub/0092E520.inc"
};
EmMineEntry gEmMinePoints_Monoblos[2] = {
#include "gm_sub/0092E530.inc"
};
EmMineEntry gEmMinePoints_Velocidrome[1] = {
#include "gm_sub/0092E550.inc"
};
EmMineEntry gEmMinePoints_Gendrome[1] = {
#include "gm_sub/0092E560.inc"
};
EmMineEntry gEmMinePoints_Ioprey[1] = {
#include "gm_sub/0092E570.inc"
};
EmMineEntry gEmMinePoints_Iodrome[1] = {
#include "gm_sub/0092E580.inc"
};
EmMineEntry gEmMinePoints_Kirin[1] = {
#include "gm_sub/0092E590.inc"
};
EmMineEntry gEmMinePoints_Cephalos[1] = {
#include "gm_sub/0092E5A0.inc"
};
EmMineEntry gEmMinePoints_Giaprey[1] = {
#include "gm_sub/0092E5B0.inc"
};
EmMineEntry gEmMinePoints_PinkRathian[2] = {
#include "gm_sub/0092E5C0.inc"
};
EmMineEntry gEmMinePoints_BlueYianKutKu[1] = {
#include "gm_sub/0092E5E0.inc"
};
EmMineEntry gEmMinePoints_PurpleGypceros[1] = {
#include "gm_sub/0092E5F0.inc"
};
EmMineEntry gEmMinePoints_YianGaruga[2] = {
#include "gm_sub/0092E600.inc"
};
EmMineEntry gEmMinePoints_SilverRathalos[2] = {
#include "gm_sub/0092E620.inc"
};
EmMineEntry gEmMinePoints_GoldRathian[2] = {
#include "gm_sub/0092E640.inc"
};
EmMineEntry gEmMinePoints_BlackDiablos[2] = {
#include "gm_sub/0092E660.inc"
};
EmMineEntry gEmMinePoints_WhiteMonoblos[2] = {
#include "gm_sub/0092E680.inc"
};
EmMineEntry gEmMinePoints_RedKhezu[1] = {
#include "gm_sub/0092E6A0.inc"
};
EmMineEntry gEmMinePoints_GreenPlesioth[1] = {
#include "gm_sub/0092E6B0.inc"
};
EmMineEntry gEmMinePoints_BlackGravios[2] = {
#include "gm_sub/0092E6C0.inc"
};
EmMineEntry gEmMinePoints_DaimyoHermitaur[1] = {
#include "gm_sub/0092E6E0.inc"
};
EmMineEntry gEmMinePoints_AzureRathalos[2] = {
#include "gm_sub/0092E6F0.inc"
};
EmMineEntry gEmMinePoints_Blangonga[1] = {
#include "gm_sub/0092E710.inc"
};
EmMineEntry gEmMinePoints_Congalala[1] = {
#include "gm_sub/0092E720.inc"
};
EmMineEntry gEmMinePoints_Rajang[1] = {
#include "gm_sub/0092E730.inc"
};
EmMineEntry gEmMinePoints_KushalaDaora[2] = {
#include "gm_sub/0092E740.inc"
};
EmMineEntry gEmMinePoints_ShenGaoren[1] = {
#include "gm_sub/0092E760.inc"
};
EmMineEntry gEmMinePoints_Shakalaka[2] = {
#include "gm_sub/0092E770.inc"
};
EmMineEntry gEmMinePoints_YamaTsukami[1] = {
#include "gm_sub/0092E790.inc"
};
EmMineEntry gEmMinePoints_Chameleos[2] = {
#include "gm_sub/0092E7A0.inc"
};
EmMineEntry gEmMinePoints_RustedKushalaDaora[2] = {
#include "gm_sub/0092E7C0.inc"
};
EmMineEntry gEmMinePoints_Blango[1] = {
#include "gm_sub/0092E7E0.inc"
};
EmMineEntry gEmMinePoints_Conga[1] = {
#include "gm_sub/0092E7F0.inc"
};
EmMineEntry gEmMinePoints_Remobra[1] = {
#include "gm_sub/0092E800.inc"
};
EmMineEntry gEmMinePoints_Lunastra[2] = {
#include "gm_sub/0092E810.inc"
};
EmMineEntry gEmMinePoints_Teostra[2] = {
#include "gm_sub/0092E830.inc"
};
EmMineEntry gEmMinePoints_Hermitaur[1] = {
#include "gm_sub/0092E850.inc"
};
EmMineEntry gEmMinePoints_ShogunCeanataur[1] = {
#include "gm_sub/0092E860.inc"
};
EmMineEntry gEmMinePoints_Bulldrome[1] = {
#include "gm_sub/0092E870.inc"
};
EmMineEntry gEmMinePoints_Anteka[1] = {
#include "gm_sub/0092E880.inc"
};
EmMineEntry gEmMinePoints_Popo[1] = {
#include "gm_sub/0092E890.inc"
};
EmMineEntry gEmMinePoints_Ceanataur[1] = {
#include "gm_sub/0092E8A0.inc"
};
EmMineEntry *gEmMinePoints[EM_KIND_CEANATAUR + 1] = {
#include "gm_sub/0092E8B0.inc"
};
EmMineEntry2 gEmMinePoints2_Rathian[1] = {
#include "gm_sub/0092E9D8.inc"
};
EmMineEntry2 gEmMinePoints2_YianKutKu[1] = {
#include "gm_sub/0092E9E0.inc"
};
EmMineEntry2 gEmMinePoints2_Cephadrome[1] = {
#include "gm_sub/0092E9E8.inc"
};
EmMineEntry2 gEmMinePoints2_Rathalos[1] = {
#include "gm_sub/0092E9F0.inc"
};
EmMineEntry2 gEmMinePoints2_Diablos[1] = {
#include "gm_sub/0092E9F8.inc"
};
EmMineEntry2 gEmMinePoints2_Khezu[1] = {
#include "gm_sub/0092EA00.inc"
};
EmMineEntry2 gEmMinePoints2_Gravios[1] = {
#include "gm_sub/0092EA08.inc"
};
EmMineEntry2 gEmMinePoints2_Gypceros[1] = {
#include "gm_sub/0092EA10.inc"
};
EmMineEntry2 gEmMinePoints2_Plesioth[1] = {
#include "gm_sub/0092EA18.inc"
};
EmMineEntry2 gEmMinePoints2_Basarios[1] = {
#include "gm_sub/0092EA20.inc"
};
EmMineEntry2 gEmMinePoints2_Melynx[1] = {
#include "gm_sub/0092EA28.inc"
};
EmMineEntry2 gEmMinePoints2_Monoblos[1] = {
#include "gm_sub/0092EA30.inc"
};
EmMineEntry2 gEmMinePoints2_PinkRathian[1] = {
#include "gm_sub/0092EA38.inc"
};
EmMineEntry2 gEmMinePoints2_BlueYianKutKu[1] = {
#include "gm_sub/0092EA40.inc"
};
EmMineEntry2 gEmMinePoints2_PurpleGypceros[1] = {
#include "gm_sub/0092EA48.inc"
};
EmMineEntry2 gEmMinePoints2_YianGaruga[1] = {
#include "gm_sub/0092EA50.inc"
};
EmMineEntry2 gEmMinePoints2_SilverRathalos[1] = {
#include "gm_sub/0092EA58.inc"
};
EmMineEntry2 gEmMinePoints2_GoldRathian[1] = {
#include "gm_sub/0092EA60.inc"
};
EmMineEntry2 gEmMinePoints2_BlackDiablos[1] = {
#include "gm_sub/0092EA68.inc"
};
EmMineEntry2 gEmMinePoints2_WhiteMonoblos[1] = {
#include "gm_sub/0092EA70.inc"
};
EmMineEntry2 gEmMinePoints2_RedKhezu[1] = {
#include "gm_sub/0092EA78.inc"
};
EmMineEntry2 gEmMinePoints2_GreenPlesioth[1] = {
#include "gm_sub/0092EA80.inc"
};
EmMineEntry2 gEmMinePoints2_BlackGravios[1] = {
#include "gm_sub/0092EA88.inc"
};
EmMineEntry2 gEmMinePoints2_DaimyoHermitaur[1] = {
#include "gm_sub/0092EA90.inc"
};
EmMineEntry2 gEmMinePoints2_AzureRathalos[1] = {
#include "gm_sub/0092EA98.inc"
};
EmMineEntry2 gEmMinePoints2_Blangonga[2] = {
#include "gm_sub/0092EAA0.inc"
};
EmMineEntry2 gEmMinePoints2_Congalala[3] = {
#include "gm_sub/0092EAB0.inc"
};
EmMineEntry2 gEmMinePoints2_KushalaDaora[1] = {
#include "gm_sub/0092EAC8.inc"
};
EmMineEntry2 gEmMinePoints2_56[1] = {
#include "gm_sub/0092EAD0.inc"
};
EmMineEntry2 gEmMinePoints2_YamaTsukami[1] = {
#include "gm_sub/0092EAD8.inc"
};
EmMineEntry2 gEmMinePoints2_Chameleos[1] = {
#include "gm_sub/0092EAE0.inc"
};
EmMineEntry2 gEmMinePoints2_RustedKushalaDaora[1] = {
#include "gm_sub/0092EAE8.inc"
};
EmMineEntry2 gEmMinePoints2_Lunastra[1] = {
#include "gm_sub/0092EAF0.inc"
};
EmMineEntry2 gEmMinePoints2_Teostra[1] = {
#include "gm_sub/0092EAF8.inc"
};
EmMineEntry2 gEmMinePoints2_ShogunCeanataur[2] = {
#include "gm_sub/0092EB00.inc"
};
EmMineEntry2 *gEmMinePoints2[EM_KIND_SLOTS] = {
#include "gm_sub/0092EB10.inc"
};
u16 gRewardItemTables_0[16] = {
#include "gm_sub/0092EC40.inc"
};
u16 gRewardItemTables_1[8] = {
#include "gm_sub/0092EC60.inc"
};
u16 gRewardItemTables_2[8] = {
#include "gm_sub/0092EC70.inc"
};
u16 gRewardItemTables_3[4] = {
#include "gm_sub/0092EC80.inc"
};
u16 gRewardItemTables_4[4] = {
#include "gm_sub/0092EC88.inc"
};
u16 gRewardItemTables_5[4] = {
#include "gm_sub/0092EC90.inc"
};
u16 gRewardItemTables_6[4] = {
#include "gm_sub/0092EC98.inc"
};
u16 gRewardItemTables_7[8] = {
#include "gm_sub/0092ECA0.inc"
};
u16 gRewardItemTables_8[16] = {
#include "gm_sub/0092ECB0.inc"
};
u16 gRewardItemTables_9[8] = {
#include "gm_sub/0092ECD0.inc"
};
u16 gRewardItemTables_10[8] = {
#include "gm_sub/0092ECE0.inc"
};
u16 gRewardItemTables_11[24] = {
#include "gm_sub/0092ECF0.inc"
};
u16 gRewardItemTables_12[40] = {
#include "gm_sub/0092ED20.inc"
};
u16 gRewardItemTables_15[24] = {
#include "gm_sub/0092ED70.inc"
};
u16 gRewardItemTables_17[32] = {
#include "gm_sub/0092EDA0.inc"
};
u16 gRewardItemTables_20[24] = {
#include "gm_sub/0092EDE0.inc"
};
u16 gRewardItemTables_22[16] = {
#include "gm_sub/0092EE10.inc"
};
u16 gRewardItemTables_23[8] = {
#include "gm_sub/0092EE30.inc"
};
u16 gRewardItemTables_24[16] = {
#include "gm_sub/0092EE40.inc"
};
u16 gRewardItemTables_25[8] = {
#include "gm_sub/0092EE60.inc"
};
u16 gRewardItemTables_26[24] = {
#include "gm_sub/0092EE70.inc"
};
u16 gRewardItemTables_28[24] = {
#include "gm_sub/0092EEA0.inc"
};
u16 gRewardItemTables_30[24] = {
#include "gm_sub/0092EED0.inc"
};
u16 gRewardItemTables_32[12] = {
#include "gm_sub/0092EF00.inc"
};
u16 gRewardItemTables_33[4] = {
#include "gm_sub/0092EF18.inc"
};
u16 gRewardItemTables_34[16] = {
#include "gm_sub/0092EF20.inc"
};
u16 gRewardItemTables_35[16] = {
#include "gm_sub/0092EF40.inc"
};
u16 gRewardItemTables_36[20] = {
#include "gm_sub/0092EF60.inc"
};
u16 gRewardItemTables_38[4] = {
#include "gm_sub/0092EF88.inc"
};
u16 gRewardItemTables_39[4] = {
#include "gm_sub/0092EF90.inc"
};
u16 gRewardItemTables_40[4] = {
#include "gm_sub/0092EF98.inc"
};
u16 gRewardItemTables_41[16] = {
#include "gm_sub/0092EFA0.inc"
};
u16 gRewardItemTables_43[16] = {
#include "gm_sub/0092EFC0.inc"
};
u16 gRewardItemTables_44[8] = {
#include "gm_sub/0092EFE0.inc"
};
u16 gRewardItemTables_45[8] = {
#include "gm_sub/0092EFF0.inc"
};
u16 gRewardItemTables_46[16] = {
#include "gm_sub/0092F000.inc"
};
u16 gRewardItemTables_47[8] = {
#include "gm_sub/0092F020.inc"
};
u16 gRewardItemTables_48[24] = {
#include "gm_sub/0092F030.inc"
};
u16 gRewardItemTables_50[48] = {
#include "gm_sub/0092F060.inc"
};
u16 gRewardItemTables_55[32] = {
#include "gm_sub/0092F0C0.inc"
};
u16 gRewardItemTables_58[16] = {
#include "gm_sub/0092F100.inc"
};
u16 gRewardItemTables_59[8] = {
#include "gm_sub/0092F120.inc"
};
u16 gRewardItemTables_60[8] = {
#include "gm_sub/0092F130.inc"
};
u16 gRewardItemTables_61[16] = {
#include "gm_sub/0092F140.inc"
};
u16 gRewardItemTables_62[16] = {
#include "gm_sub/0092F160.inc"
};
u16 gRewardItemTables_63[16] = {
#include "gm_sub/0092F180.inc"
};
u16 gRewardItemTables_64[8] = {
#include "gm_sub/0092F1A0.inc"
};
u16 gRewardItemTables_65[16] = {
#include "gm_sub/0092F1B0.inc"
};
u16 gRewardItemTables_66[8] = {
#include "gm_sub/0092F1D0.inc"
};
u16 gRewardItemTables_67[24] = {
#include "gm_sub/0092F1E0.inc"
};
u16 gRewardItemTables_69[16] = {
#include "gm_sub/0092F210.inc"
};
u16 gRewardItemTables_70[8] = {
#include "gm_sub/0092F230.inc"
};
u16 gRewardItemTables_71[16] = {
#include "gm_sub/0092F240.inc"
};
u16 gRewardItemTables_72[16] = {
#include "gm_sub/0092F260.inc"
};
u16 gRewardItemTables_73[8] = {
#include "gm_sub/0092F280.inc"
};
u16 gRewardItemTables_74[40] = {
#include "gm_sub/0092F290.inc"
};
u16 gRewardItemTables_78[24] = {
#include "gm_sub/0092F2E0.inc"
};
u16 gRewardItemTables_79[32] = {
#include "gm_sub/0092F310.inc"
};
u16 gRewardItemTables_80[16] = {
#include "gm_sub/0092F350.inc"
};
u16 gRewardItemTables_81[16] = {
#include "gm_sub/0092F370.inc"
};
u16 gRewardItemTables_82[32] = {
#include "gm_sub/0092F390.inc"
};
u16 gRewardItemTables_85[32] = {
#include "gm_sub/0092F3D0.inc"
};
u16 gRewardItemTables_88[16] = {
#include "gm_sub/0092F410.inc"
};
u16 gRewardItemTables_89[24] = {
#include "gm_sub/0092F430.inc"
};
u16 gRewardItemTables_92[16] = {
#include "gm_sub/0092F460.inc"
};
u16 gRewardItemTables_93[48] = {
#include "gm_sub/0092F480.inc"
};
u16 gRewardItemTables_98[48] = {
#include "gm_sub/0092F4E0.inc"
};
u16 gRewardItemTables_103[16] = {
#include "gm_sub/0092F540.inc"
};
u16 gRewardItemTables_104[40] = {
#include "gm_sub/0092F560.inc"
};
u16 gRewardItemTables_108[12] = {
#include "gm_sub/0092F5B0.inc"
};
u16 gRewardItemTables_109[4] = {
#include "gm_sub/0092F5C8.inc"
};
u16 gRewardItemTables_110[4] = {
#include "gm_sub/0092F5D0.inc"
};
u16 gRewardItemTables_111[4] = {
#include "gm_sub/0092F5D8.inc"
};
u16 gRewardItemTables_112[16] = {
#include "gm_sub/0092F5E0.inc"
};
u16 gRewardItemTables_114[16] = {
#include "gm_sub/0092F600.inc"
};
u16 gRewardItemTables_115[8] = {
#include "gm_sub/0092F620.inc"
};
u16 gRewardItemTables_116[8] = {
#include "gm_sub/0092F630.inc"
};
u16 gRewardItemTables_117[4] = {
#include "gm_sub/0092F640.inc"
};
u16 gRewardItemTables_118[4] = {
#include "gm_sub/0092F648.inc"
};
u16 gRewardItemTables_119[4] = {
#include "gm_sub/0092F650.inc"
};
u16 gRewardItemTables_120[4] = {
#include "gm_sub/0092F658.inc"
};
u16 gRewardItemTables_121[8] = {
#include "gm_sub/0092F660.inc"
};
u16 gRewardItemTables_122[16] = {
#include "gm_sub/0092F670.inc"
};
u16 gRewardItemTables_123[24] = {
#include "gm_sub/0092F690.inc"
};
u16 gRewardItemTables_125[24] = {
#include "gm_sub/0092F6C0.inc"
};
u16 gRewardItemTables_126[32] = {
#include "gm_sub/0092F6F0.inc"
};
u16 gRewardItemTables_127[16] = {
#include "gm_sub/0092F730.inc"
};
u16 gRewardItemTables_128[16] = {
#include "gm_sub/0092F750.inc"
};
u16 gRewardItemTables_129[16] = {
#include "gm_sub/0092F770.inc"
};
u16 gRewardItemTables_130[16] = {
#include "gm_sub/0092F790.inc"
};
u16 gRewardItemTables_131[16] = {
#include "gm_sub/0092F7B0.inc"
};
u16 gRewardItemTables_132[24] = {
#include "gm_sub/0092F7D0.inc"
};
u16 gRewardItemTables_134[24] = {
#include "gm_sub/0092F800.inc"
};
u16 gRewardItemTables_136[16] = {
#include "gm_sub/0092F830.inc"
};
u16 gRewardItemTables_137[16] = {
#include "gm_sub/0092F850.inc"
};
u16 gRewardItemTables_138[16] = {
#include "gm_sub/0092F870.inc"
};
u16 gRewardItemTables_139[16] = {
#include "gm_sub/0092F890.inc"
};
u16 gRewardItemTables_140[16] = {
#include "gm_sub/0092F8B0.inc"
};
u16 gRewardItemTables_141[16] = {
#include "gm_sub/0092F8D0.inc"
};
u16 gRewardItemTables_142[16] = {
#include "gm_sub/0092F8F0.inc"
};
u16 gRewardItemTables_143[16] = {
#include "gm_sub/0092F910.inc"
};
u16 gRewardItemTables_144[16] = {
#include "gm_sub/0092F930.inc"
};
u16 gRewardItemTables_145[16] = {
#include "gm_sub/0092F950.inc"
};
u16 gRewardItemTables_146[24] = {
#include "gm_sub/0092F970.inc"
};
u16 gRewardItemTables_148[16] = {
#include "gm_sub/0092F9A0.inc"
};
u16 gRewardItemTables_149[16] = {
#include "gm_sub/0092F9C0.inc"
};
u16 gRewardItemTables_150[20] = {
#include "gm_sub/0092F9E0.inc"
};
u16 gRewardItemTables_152[4] = {
#include "gm_sub/0092FA08.inc"
};
u16 gRewardItemTables_153[4] = {
#include "gm_sub/0092FA10.inc"
};
u16 gRewardItemTables_154[4] = {
#include "gm_sub/0092FA18.inc"
};
u16 gRewardItemTables_155[16] = {
#include "gm_sub/0092FA20.inc"
};
u16 gRewardItemTables_157[16] = {
#include "gm_sub/0092FA40.inc"
};
u16 gRewardItemTables_158[8] = {
#include "gm_sub/0092FA60.inc"
};
u16 gRewardItemTables_159[8] = {
#include "gm_sub/0092FA70.inc"
};
u16 gRewardItemTables_160[16] = {
#include "gm_sub/0092FA80.inc"
};
u16 gRewardItemTables_161[8] = {
#include "gm_sub/0092FAA0.inc"
};
u16 gRewardItemTables_162[16] = {
#include "gm_sub/0092FAB0.inc"
};
u16 gRewardItemTables_163[16] = {
#include "gm_sub/0092FAD0.inc"
};
u16 gRewardItemTables_164[48] = {
#include "gm_sub/0092FAF0.inc"
};
u16 gRewardItemTables_169[32] = {
#include "gm_sub/0092FB50.inc"
};
u16 gRewardItemTables_172[16] = {
#include "gm_sub/0092FB90.inc"
};
u16 gRewardItemTables_173[8] = {
#include "gm_sub/0092FBB0.inc"
};
u16 gRewardItemTables_174[8] = {
#include "gm_sub/0092FBC0.inc"
};
u16 gRewardItemTables_175[16] = {
#include "gm_sub/0092FBD0.inc"
};
u16 gRewardItemTables_176[16] = {
#include "gm_sub/0092FBF0.inc"
};
u16 gRewardItemTables_177[12] = {
#include "gm_sub/0092FC10.inc"
};
u16 gRewardItemTables_178[4] = {
#include "gm_sub/0092FC28.inc"
};
u16 gRewardItemTables_179[16] = {
#include "gm_sub/0092FC30.inc"
};
u16 gRewardItemTables_180[16] = {
#include "gm_sub/0092FC50.inc"
};
u16 gRewardItemTables_181[24] = {
#include "gm_sub/0092FC70.inc"
};
u16 gRewardItemTables_183[16] = {
#include "gm_sub/0092FCA0.inc"
};
u16 gRewardItemTables_184[16] = {
#include "gm_sub/0092FCC0.inc"
};
u16 gRewardItemTables_185[16] = {
#include "gm_sub/0092FCE0.inc"
};
u16 gRewardItemTables_186[16] = {
#include "gm_sub/0092FD00.inc"
};
u16 gRewardItemTables_187[8] = {
#include "gm_sub/0092FD20.inc"
};
u16 gRewardItemTables_188[16] = {
#include "gm_sub/0092FD30.inc"
};
u16 gRewardItemTables_189[16] = {
#include "gm_sub/0092FD50.inc"
};
u16 gRewardItemTables_190[8] = {
#include "gm_sub/0092FD70.inc"
};
u16 gRewardItemTables_191[8] = {
#include "gm_sub/0092FD80.inc"
};
u16 gRewardItemTables_192[24] = {
#include "gm_sub/0092FD90.inc"
};
u16 gRewardItemTables_193[32] = {
#include "gm_sub/0092FDC0.inc"
};
u16 gRewardItemTables_194[16] = {
#include "gm_sub/0092FE00.inc"
};
u16 gRewardItemTables_195[16] = {
#include "gm_sub/0092FE20.inc"
};
u16 gRewardItemTables_196[16] = {
#include "gm_sub/0092FE40.inc"
};
u16 gRewardItemTables_197[8] = {
#include "gm_sub/0092FE60.inc"
};
u16 gRewardItemTables_198[16] = {
#include "gm_sub/0092FE70.inc"
};
u16 gRewardItemTables_199[16] = {
#include "gm_sub/0092FE90.inc"
};
u16 gRewardItemTables_200[24] = {
#include "gm_sub/0092FEB0.inc"
};
u16 gRewardItemTables_202[16] = {
#include "gm_sub/0092FEE0.inc"
};
u16 gRewardItemTables_203[16] = {
#include "gm_sub/0092FF00.inc"
};
u16 gRewardItemTables_205[16] = {
#include "gm_sub/0092FF20.inc"
};
u16 gRewardItemTables_206[16] = {
#include "gm_sub/0092FF40.inc"
};
u16 gRewardItemTables_207[32] = {
#include "gm_sub/0092FF60.inc"
};
u16 gRewardItemTables_210[24] = {
#include "gm_sub/0092FFA0.inc"
};
u16 gRewardItemTables_212[16] = {
#include "gm_sub/0092FFD0.inc"
};
u16 gRewardItemTables_213[24] = {
#include "gm_sub/0092FFF0.inc"
};
u16 gRewardItemTables_215[16] = {
#include "gm_sub/00930020.inc"
};
u16 gRewardItemTables_216[16] = {
#include "gm_sub/00930040.inc"
};
u16 gRewardItemTables_217[16] = {
#include "gm_sub/00930060.inc"
};
u16 gRewardItemTables_218[32] = {
#include "gm_sub/00930080.inc"
};
u16 gRewardItemTables_221[16] = {
#include "gm_sub/009300C0.inc"
};
u16 gRewardItemTables_222[16] = {
#include "gm_sub/009300E0.inc"
};
u16 gRewardItemTables_223[4] = {
#include "gm_sub/00930100.inc"
};
u16 gRewardItemTables_224[4] = {
#include "gm_sub/00930108.inc"
};
u16 gRewardItemTables_225[4] = {
#include "gm_sub/00930110.inc"
};
u16 gRewardItemTables_226[4] = {
#include "gm_sub/00930118.inc"
};
u16 gRewardItemTables_227[8] = {
#include "gm_sub/00930120.inc"
};
u16 gRewardItemTables_228[8] = {
#include "gm_sub/00930130.inc"
};
u16 gRewardItemTables_229[8] = {
#include "gm_sub/00930140.inc"
};
u16 gRewardItemTables_230[12] = {
#include "gm_sub/00930150.inc"
};
u16 gRewardItemTables_231[4] = {
#include "gm_sub/00930168.inc"
};
u16 gRewardItemTables_232[4] = {
#include "gm_sub/00930170.inc"
};
u16 gRewardItemTables_233[4] = {
#include "gm_sub/00930178.inc"
};
u16 gRewardItemTables_234[4] = {
#include "gm_sub/00930180.inc"
};
u16 gRewardItemTables_235[4] = {
#include "gm_sub/00930188.inc"
};
u16 gRewardItemTables_236[8] = {
#include "gm_sub/00930190.inc"
};
u16 gRewardItemTables_237[12] = {
#include "gm_sub/009301A0.inc"
};
u16 gRewardItemTables_238[4] = {
#include "gm_sub/009301B8.inc"
};
u16 gRewardItemTables_239[24] = {
#include "gm_sub/009301C0.inc"
};
u16 gRewardItemTables_240[48] = {
#include "gm_sub/009301F0.inc"
};
u16 gRewardItemTables_244[16] = {
#include "gm_sub/00930250.inc"
};
u16 gRewardItemTables_245[40] = {
#include "gm_sub/00930270.inc"
};
u16 gRewardItemTables_249[24] = {
#include "gm_sub/009302C0.inc"
};
u16 gRewardItemTables_251[16] = {
#include "gm_sub/009302F0.inc"
};
u16 gRewardItemTables_252[32] = {
#include "gm_sub/00930310.inc"
};
u16 gRewardItemTables_255[16] = {
#include "gm_sub/00930350.inc"
};
u16 gRewardItemTables_256[24] = {
#include "gm_sub/00930370.inc"
};
u16 gRewardItemTables_258[24] = {
#include "gm_sub/009303A0.inc"
};
u16 gRewardItemTables_260[32] = {
#include "gm_sub/009303D0.inc"
};
u16 gRewardItemTables_263[16] = {
#include "gm_sub/00930410.inc"
};
u16 gRewardItemTables_264[20] = {
#include "gm_sub/00930430.inc"
};
u16 gRewardItemTables_266[4] = {
#include "gm_sub/00930458.inc"
};
u16 gRewardItemTables_267[4] = {
#include "gm_sub/00930460.inc"
};
u16 gRewardItemTables_268[4] = {
#include "gm_sub/00930468.inc"
};
u16 gRewardItemTables_269[4] = {
#include "gm_sub/00930470.inc"
};
u16 gRewardItemTables_270[4] = {
#include "gm_sub/00930478.inc"
};
u16 gRewardItemTables_271[32] = {
#include "gm_sub/00930480.inc"
};
u16 gRewardItemTables_274[24] = {
#include "gm_sub/009304C0.inc"
};
u16 gRewardItemTables_276[16] = {
#include "gm_sub/009304F0.inc"
};
u16 gRewardItemTables_277[16] = {
#include "gm_sub/00930510.inc"
};
u16 gRewardItemTables_278[48] = {
#include "gm_sub/00930530.inc"
};
u16 gRewardItemTables_283[16] = {
#include "gm_sub/00930590.inc"
};
u16 gRewardItemTables_284[24] = {
#include "gm_sub/009305B0.inc"
};
u16 gRewardItemTables_286[16] = {
#include "gm_sub/009305E0.inc"
};
u16 gRewardItemTables_287[32] = {
#include "gm_sub/00930600.inc"
};
u16 gRewardItemTables_290[16] = {
#include "gm_sub/00930640.inc"
};
u16 gRewardItemTables_291[12] = {
#include "gm_sub/00930660.inc"
};
u16 gRewardItemTables_292[4] = {
#include "gm_sub/00930678.inc"
};
u16 gRewardItemTables_293[24] = {
#include "gm_sub/00930680.inc"
};
u16 gRewardItemTables_295[24] = {
#include "gm_sub/009306B0.inc"
};
u16 gRewardItemTables_297[16] = {
#include "gm_sub/009306E0.inc"
};
u16 gRewardItemTables_298[24] = {
#include "gm_sub/00930700.inc"
};
u16 gRewardItemTables_300[16] = {
#include "gm_sub/00930730.inc"
};
u16 gRewardItemTables_301[16] = {
#include "gm_sub/00930750.inc"
};
u16 gRewardItemTables_302[16] = {
#include "gm_sub/00930770.inc"
};
u16 gRewardItemTables_303[20] = {
#include "gm_sub/00930790.inc"
};
u16 gRewardItemTables_305[4] = {
#include "gm_sub/009307B8.inc"
};
u16 gRewardItemTables_306[24] = {
#include "gm_sub/009307C0.inc"
};
u16 gRewardItemTables_307[48] = {
#include "gm_sub/009307F0.inc"
};
u16 gRewardItemTables_311[16] = {
#include "gm_sub/00930850.inc"
};
u16 gRewardItemTables_312[24] = {
#include "gm_sub/00930870.inc"
};
u16 gRewardItemTables_314[16] = {
#include "gm_sub/009308A0.inc"
};
u16 gRewardItemTables_315[16] = {
#include "gm_sub/009308C0.inc"
};
u16 gRewardItemTables_316[32] = {
#include "gm_sub/009308E0.inc"
};
u16 gRewardItemTables_319[24] = {
#include "gm_sub/00930920.inc"
};
u16 gRewardItemTables_321[24] = {
#include "gm_sub/00930950.inc"
};
u16 gRewardItemTables_323[24] = {
#include "gm_sub/00930980.inc"
};
u16 gRewardItemTables_325[24] = {
#include "gm_sub/009309B0.inc"
};
u16 gRewardItemTables_327[24] = {
#include "gm_sub/009309E0.inc"
};
u16 gRewardItemTables_329[32] = {
#include "gm_sub/00930A10.inc"
};
u16 gRewardItemTables_332[16] = {
#include "gm_sub/00930A50.inc"
};
u16 gRewardItemTables_333[16] = {
#include "gm_sub/00930A70.inc"
};
u16 gRewardItemTables_334[16] = {
#include "gm_sub/00930A90.inc"
};
u16 gRewardItemTables_335[20] = {
#include "gm_sub/00930AB0.inc"
};
u16 gRewardItemTables_337[4] = {
#include "gm_sub/00930AD8.inc"
};
u16 gRewardItemTables_338[4] = {
#include "gm_sub/00930AE0.inc"
};
u16 gRewardItemTables_339[4] = {
#include "gm_sub/00930AE8.inc"
};
u16 gRewardItemTables_340[8] = {
#include "gm_sub/00930AF0.inc"
};
u16 *gRewardItemTables[REWARD_ITEM_TABLES] = {
#include "gm_sub/00930B00.inc"
};
PartReward gPartRewardRecs[PART_REWARD_RECS] = {
#include "gm_sub/00931060.inc"
};
u16 gStartAreaTables_0_0[4] = {
#include "gm_sub/009313F8.inc"
};
u16 *gStartAreaTables_0[2] = {
    gStartAreaTables_0_0, gStartAreaTables_0_0,
};
u16 gStartAreaTables_1_0[4] = {
#include "gm_sub/00931408.inc"
};
u16 *gStartAreaTables_1[4] = {
    gStartAreaTables_1_0, gStartAreaTables_1_0, NULL, NULL,
};
u16 gStartAreaTables_2_0[12] = {
#include "gm_sub/00931420.inc"
};
u16 *gStartAreaTables_2[2] = {
    gStartAreaTables_2_0, gStartAreaTables_2_0,
};
u16 gStartAreaTables_3_0[12] = {
#include "gm_sub/00931440.inc"
};
u16 *gStartAreaTables_3[2] = {
    gStartAreaTables_3_0, gStartAreaTables_3_0,
};
u16 gStartAreaTables_4_0[12] = {
#include "gm_sub/00931460.inc"
};
u16 *gStartAreaTables_4[2] = {
    gStartAreaTables_4_0, gStartAreaTables_4_0,
};
u16 gStartAreaTables_5_0[12] = {
#include "gm_sub/00931480.inc"
};
u16 *gStartAreaTables_5[2] = {
    gStartAreaTables_5_0, gStartAreaTables_5_0,
};
u16 gStartAreaTables_6_0[12] = {
#include "gm_sub/009314A0.inc"
};
u16 *gStartAreaTables_6[2] = {
    gStartAreaTables_6_0, gStartAreaTables_6_0,
};
u16 gStartAreaTables_7_0[4] = {
#include "gm_sub/009314C0.inc"
};
u16 *gStartAreaTables_7[2] = {
    gStartAreaTables_7_0, gStartAreaTables_7_0,
};
u16 gStartAreaTables_8_0[8] = {
#include "gm_sub/009314D0.inc"
};
u16 *gStartAreaTables_8[2] = {
    gStartAreaTables_8_0, gStartAreaTables_8_0,
};
u16 gStartAreaTables_9_0[6] = {
#include "gm_sub/009314E8.inc"
};
u16 *gStartAreaTables_9[2] = {
    gStartAreaTables_9_0, gStartAreaTables_9_0,
};
u16 gStartAreaTables_10_0[8] = {
#include "gm_sub/00931500.inc"
};
u16 *gStartAreaTables_10[4] = {
    gStartAreaTables_10_0, gStartAreaTables_10_0, NULL, NULL,
};
u16 gStartAreaTables_11_0[12] = {
#include "gm_sub/00931520.inc"
};
u16 *gStartAreaTables_11[2] = {
    gStartAreaTables_11_0, gStartAreaTables_11_0,
};
u16 gStartAreaTables_12_0[4] = {
#include "gm_sub/00931540.inc"
};
u16 *gStartAreaTables_12[2] = {
    gStartAreaTables_12_0, gStartAreaTables_12_0,
};
u16 gStartAreaTables_13_0[12] = {
#include "gm_sub/00931550.inc"
};
u16 *gStartAreaTables_13[2] = {
    gStartAreaTables_13_0, gStartAreaTables_13_0,
};
u16 gStartAreaTables_14_0[4] = {
#include "gm_sub/00931570.inc"
};
u16 *gStartAreaTables_14[2] = {
    gStartAreaTables_14_0, gStartAreaTables_14_0,
};
u16 gStartAreaTables_15_0[12] = {
#include "gm_sub/00931580.inc"
};
u16 *gStartAreaTables_15[2] = {
    gStartAreaTables_15_0, gStartAreaTables_15_0,
};
u16 gStartAreaTables_16_0[12] = {
#include "gm_sub/009315A0.inc"
};
u16 *gStartAreaTables_16[2] = {
    gStartAreaTables_16_0, gStartAreaTables_16_0,
};
u16 gStartAreaTables_17_0[12] = {
#include "gm_sub/009315C0.inc"
};
u16 *gStartAreaTables_17[2] = {
    gStartAreaTables_17_0, gStartAreaTables_17_0,
};
u16 gStartAreaTables_18_0[12] = {
#include "gm_sub/009315E0.inc"
};
u16 *gStartAreaTables_18[2] = {
    gStartAreaTables_18_0, gStartAreaTables_18_0,
};
u16 gStartAreaTables_19_0[12] = {
#include "gm_sub/00931600.inc"
};
u16 *gStartAreaTables_19[2] = {
    gStartAreaTables_19_0, gStartAreaTables_19_0,
};
u16 gStartAreaTables_20_0[12] = {
#include "gm_sub/00931620.inc"
};
u16 *gStartAreaTables_20[2] = {
    gStartAreaTables_20_0, gStartAreaTables_20_0,
};
u16 gStartAreaTables_21_0[12] = {
#include "gm_sub/00931640.inc"
};
u16 *gStartAreaTables_21[2] = {
    gStartAreaTables_21_0, gStartAreaTables_21_0,
};
u16 gStartAreaTables_22_0[4] = {
#include "gm_sub/00931660.inc"
};
u16 *gStartAreaTables_22[2] = {
    gStartAreaTables_22_0, gStartAreaTables_22_0,
};
u16 gStartAreaTables_23_0[4] = {
#include "gm_sub/00931670.inc"
};
u16 *gStartAreaTables_23[2] = {
    gStartAreaTables_23_0, gStartAreaTables_23_0,
};
u16 gStartAreaTables_24_0[8] = {
#include "gm_sub/00931680.inc"
};
u16 *gStartAreaTables_24[2] = {
#include "gm_sub/00931690.inc"
};
u16 gStartAreaTables_25_0[6] = {
#include "gm_sub/00931698.inc"
};
u16 *gStartAreaTables_25[2] = {
#include "gm_sub/009316A8.inc"
};
u16 **gStartAreaTables[28] = {
#include "gm_sub/009316B0.inc"
};
/*
 * The default stage's data, used with no stage file: sub_main's gDefaultStageHeader holds
 * their addresses as numbers (emSpawns: gDefaultEmSpawns; stageEntry: gDefaultStageEntry;
 * startAreas, stageInfo, supply, info), as gAreaTriKindFlags (sub_main's area_update.c) does
 * the wall kind rows after them (gAreaTriKindFlags_N: its entry N, an area).
 */
QuestEm gDefaultEmSpawnEms_0[1] = {
#include "gm_sub/00931720.inc"
};
s32 gDefaultEmSpawnIds_0[4] = {
#include "gm_sub/00931760.inc"
};
QuestEm gDefaultEmSpawnEms_1[2] = {
#include "gm_sub/00931770.inc"
};
s32 gDefaultEmSpawnIds_1[4] = {
#include "gm_sub/009317F0.inc"
};
DefaultStageEntry gDefaultEmSpawns_0[3] = {
#include "gm_sub/00931800.inc"
};
DefaultStageEntry *gDefaultEmSpawns[4] = {
#include "gm_sub/00931830.inc"
};
s32 gDefaultStageEmKinds[9] = {
#include "gm_sub/00931840.inc"
};
DefaultStageEntry gDefaultStageEntry[2] = {
#include "gm_sub/00931870.inc"
};
StartArea gDefaultStartAreas[4] = {
#include "gm_sub/00931890.inc"
};
u32 gDefaultStageInfo[2] = {
#include "gm_sub/009318D0.inc"
};
ItemStack gDefaultSupply[4] = {
#include "gm_sub/009318E0.inc"
};
QuestInfo gDefaultQuestInfo[1] = {
#include "gm_sub/009318F0.inc"
};
AreaWallKindRow gAreaTriKindFlags_11[2] = {
#include "gm_sub/00931940.inc"
};
AreaWallKindRow gAreaTriKindFlags_12[2] = {
#include "gm_sub/00931950.inc"
};
AreaWallKindRow gAreaTriKindFlags_17[2] = {
#include "gm_sub/00931960.inc"
};
AreaWallKindRow gAreaTriKindFlags_20[2] = {
#include "gm_sub/00931970.inc"
};
AreaWallKindRow gAreaTriKindFlags_25[2] = {
#include "gm_sub/00931980.inc"
};
AreaWallKindRow gAreaTriKindFlags_28[2] = {
#include "gm_sub/00931990.inc"
};
AreaWallKindRow gAreaTriKindFlags_30[2] = {
#include "gm_sub/009319A0.inc"
};
AreaWallKindRow gAreaTriKindFlags_32[2] = {
#include "gm_sub/009319B0.inc"
};
AreaWallKindRow gAreaTriKindFlags_38[2] = {
#include "gm_sub/009319C0.inc"
};
AreaWallKindRow gAreaTriKindFlags_50[2] = {
#include "gm_sub/009319D0.inc"
};
AreaWallKindRow gAreaTriKindFlags_57[2] = {
#include "gm_sub/009319E0.inc"
};
AreaWallKindRow gAreaTriKindFlags_6[2] = {
#include "gm_sub/009319F0.inc"
};
AreaWallKindRow gAreaTriKindFlags_97[2] = {
#include "gm_sub/00931A00.inc"
};
AreaWallKindRow gAreaTriKindFlags_1[2] = {
#include "gm_sub/00931A10.inc"
};
AreaWallKindRow gAreaTriKindFlags_2[2] = {
#include "gm_sub/00931A20.inc"
};
AreaWallKindRow gAreaTriKindFlags_83[2] = {
#include "gm_sub/00931A30.inc"
};
AreaWallKindRow gAreaTriKindFlags_139[2] = {
#include "gm_sub/00931A40.inc"
};
AreaWallKindRow gAreaTriKindFlags_48[2] = {
#include "gm_sub/00931A50.inc"
};
AreaWallKindRow gAreaTriKindFlags_49[2] = {
#include "gm_sub/00931A60.inc"
};
AreaWallKindRow gAreaTriKindFlags_144[2] = {
#include "gm_sub/00931A70.inc"
};
AreaWallKindRow gAreaTriKindFlags_55[2] = {
#include "gm_sub/00931A80.inc"
};
AreaWallKindRow gAreaTriKindFlags_176[2] = {
#include "gm_sub/00931A90.inc"
};
s32 gGmKindFileLists_0[76] = {
#include "gm_sub/00931AA0.inc"
};
s32 gGmKindFileLists_2[76] = {
#include "gm_sub/00931BD0.inc"
};
s32 gGmKindFileLists_3[76] = {
#include "gm_sub/00931D00.inc"
};
s32 gGmKindFileLists_4[76] = {
#include "gm_sub/00931E30.inc"
};
s32 gGmKindFileLists_5[76] = {
#include "gm_sub/00931F60.inc"
};
s32 gGmKindFileLists_6[76] = {
#include "gm_sub/00932090.inc"
};
s32 gGmKindFileLists_7[76] = {
#include "gm_sub/009321C0.inc"
};
s32 gGmKindFileLists_8[76] = {
#include "gm_sub/009322F0.inc"
};
s32 gGmKindFileLists_9[76] = {
#include "gm_sub/00932420.inc"
};
s32 gGmKindFileLists_10[76] = {
#include "gm_sub/00932550.inc"
};
s32 gGmKindFileLists_11[76] = {
#include "gm_sub/00932680.inc"
};
s32 gGmKindFileLists_12[76] = {
#include "gm_sub/009327B0.inc"
};
s32 gGmKindFileLists_13[76] = {
#include "gm_sub/009328E0.inc"
};
s32 gGmKindFileLists_14[76] = {
#include "gm_sub/00932A10.inc"
};
s32 gGmKindFileLists_15[76] = {
#include "gm_sub/00932B40.inc"
};
