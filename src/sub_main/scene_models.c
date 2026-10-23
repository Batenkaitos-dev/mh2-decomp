/*
 * The models of the scene's monsters and of the stage, loaded into the model pool's slots
 * and freed again.
 *
 * - The monster slots (SceneState's per-slot model fields): EmSlot_LoadModels loads the
 *   quest's monster in a slot, with its textures, its motions and, for some kinds, a second
 *   model set; EmSlot_LoadModel loads the lobby's creatures; EmSlot_FreeModels and
 *   EmSlot_Clear free them.
 * - The area's models (AreaState) and its sub-models (gAreaSubModelIds: the hub areas'
 *   extra models, Area_LoadSubModels), and the character maker's parts model, one per
 *   gender, in the first sub-model's place.
 *
 * A model is taken with Model_Alloc and Model_Reserve, set up from its pack with the
 * Model_Setup* functions, and freed slot by slot with Model_Free and Model_Release.
 */
#include "common.h"
#include "syssettings.h"
#include "scene.h"
#include "model.h"
#include "area.h"
#include "text.h"
#include "lobby_world.h"
#include "game_util.h"

extern u8 gModelUsed[];
extern s32 gModelLoadBuf;
extern s32 gAreaSubModelId;
extern s32 gAreaSubModelIds[4];     /* four more model slots, -1 when unused */
extern s32 gEmSlotTexFiles[];
extern s32 gEmSlotAnimFiles[];
extern s32 gEmSlotTexPacks[];
extern s32 gEmSlotModelPacks[];
extern s32 gEmPartModelPackFiles[];
/* Per monster slot, the kind ([0]) and the variant ([1]) of the model it holds, -1: none. */
extern s32 gEmSlotModelIds[][2];
/* The variants' column of gEmSlotModelIds (gEmSlotModelIds[0][1], a name the linker defines):
 * EmSlot_LoadModel reads the variants through it, and through gEmSlotModelIds[slot][1] it
 * compiles differently */
extern s32 gEmSlotModelVars[][2];

void Load_EmSlotModelPack(s32 kind);
void Load_EmModelPack(s32 kind);
void Load_EmPartModelPack(s32 kind);
/* unit's own declaration: the texture slot is declared s16, as the callers pass it; with the definition's s32 the calls in EmSlot_LoadModels and Area_LoadSubModels compile differently */
Model *Model_SetupWithPack(s16 idx, s32 modelFile, s16 texSlot, s32 packId, s32 setupFlags, s32 skelKind);
/* unit's own declaration: the texture slot is declared s16, as the callers pass it; with the definition's s32 the call in EmSlot_LoadModel compiles differently */
Model *Model_SetupTexPackB(s16 model, s32 work, s16 texSlot, s32 packId, s32 setupFlags);
/* unit's own declaration: the texture slot is declared s16, as EmSlot_LoadModels passes it; with the definition's s32 the call compiles differently */
Model *Model_SetupTexPackEx(s16 idx, s32 work, s16 texSlot, s32 packId, s32 setupFlags, u8 slot);

/*
 * Loads the models of the quest's monster in slot `slot` (its kind in gScene.emKind, none
 * when 0): its model with its textures and motions (texture slots 20 per monster slot from
 * 163), and when the kind has one (gEmPartModelPackFiles not -1) a second model set (texture
 * slot 16 further on); else the set's count is -1.
 * FAKEMATCH (docs/notes/hard-matches/EmSlot_LoadModels.md): written so that the compiler keeps the
 * values in the registers the game's code uses.
 * - Each path computes its own byte offset o = slot * sizeof(s16) and writes the halfword
 *   arrays through it.
 * - The kind is reached through the integer sum slot + &gScene, plus emKind's (offset 0x2A):
 *   the game's code adds them in that order.
 * - kind points first at the whole emKind array, for the test alone, and only then at the
 *   slot's entry. With one pointer to emKind[slot], the compiler gives it another register.
 */
void EmSlot_LoadModels(s32 slot)
{
    s32 work;
    s32 model;
    s32 o;
    u8 *kind;
    SceneState *scene;

    scene = &gScene;
    kind = scene->emKind;
    if (kind[slot] == 0) {
        o = slot * sizeof(s16);
        *(s16 *)((u8 *)scene->modelFirst + o) = 0;
        *(s16 *)((u8 *)scene->modelCount + o) = 0;
        scene->models[slot] = NULL;
        return;
    }
    kind = (u8 *)(slot + (s32)scene) + 42; /* &gScene.emKind[slot] */
    Load_EmModelPack(*kind);
    work = gModelLoadBuf;
    model = Model_Alloc(1);
    if (model < 0) {
        return;
    }
    o = slot * sizeof(s16);
    *(s16 *)((u8 *)scene->modelFirst + o) = model;
    *(s16 *)((u8 *)scene->modelCount + o) = 1;
    scene->models[slot] = Model_Get(model);
    Model_Reserve(model, 1);
    Model_SetupTexPackEx(model, work, slot * 20 + 163, gEmSlotTexPacks[*kind], 2304, slot);
    Anim_LoadEm(slot, *kind);
    if (gEmPartModelPackFiles[*kind] != -1) {
        Load_EmPartModelPack(*kind);
        work = gModelLoadBuf;
        model = Model_Alloc(1);
        if (model < 0) {
            *(s16 *)((u8 *)scene->partFirst + o) = 0;
            *(s16 *)((u8 *)scene->partCount + o) = -1;
            scene->partModels[slot] = NULL;
            return;
        }
        *(s16 *)((u8 *)scene->partFirst + o) = model;
        *(s16 *)((u8 *)scene->partCount + o) = 1;
        scene->partModels[slot] = Model_Get(model);
        Model_Reserve(model, 1);
        Model_SetupWithPack(model, work, slot * 20 + 179, gEmSlotModelPacks[*kind], 2304, 0);
        return;
    }
    *(s16 *)((u8 *)scene->partFirst + o) = 0;
    *(s16 *)((u8 *)scene->partCount + o) = -1;
    scene->partModels[slot] = NULL;
}

/* Loads the model of monster kind `kind`, variant `var`, into monster slot `slot`, unless the
 * slot holds it already (gEmSlotModelIds): the slot's old model is freed first, and its old
 * motions unless it is slot 0. The lobby loads its creatures this way. */
void EmSlot_LoadModel(s32 kind, s32 var, s32 slot)
{
    SceneState *scene;
    s32 *cur;
    s32 *curVar;
    s32 work;
    s32 model;

    cur = &gEmSlotModelIds[slot][0];
    scene = &gScene;
    if (kind != *cur || var != gEmSlotModelVars[slot][0]) {
        if (*cur != -1) {
            EmSlot_Clear(slot);
        }
        curVar = &gEmSlotModelVars[slot][0];
        if (*curVar != -1 && slot != 0) {
            Motion_FreeEmSlot(slot);
        }
        *cur = kind;
        *curVar = var;
        Load_EmSlotModelPack(kind);
        work = (*(s32 *)&gSysSettings.dataBuf);
        model = Model_Alloc(1);
        if (model >= 0) {
            scene->modelFirst[slot] = model;
            scene->modelCount[slot] = 1;
            scene->models[slot] = Model_Get(model);
            Model_Reserve(model, 1);
            Model_SetupTexPackB(model, work, slot * 20 + 163, gEmSlotTexFiles[kind], 2304);
            if (gEmSlotAnimFiles[kind] != 0) {
                Anim_LoadEmSlot(kind, var, slot);
            }
        }
    }
}

/* Forgets which model monster slot `slot` holds (gEmSlotModelIds), so that EmSlot_LoadModel
 * loads it again: its kind (which 0), its variant (1) or both (2). */
void EmSlot_ForgetModel(s16 slot, s8 which)
{
    switch (which) {
    case 0:
        gEmSlotModelIds[slot][0] = -1;
        break;
    case 1:
        gEmSlotModelIds[slot][1] = -1;
        break;
    case 2:
        gEmSlotModelIds[slot][0] = -1;
        gEmSlotModelIds[slot][1] = -1;
        break;
    }
}

/* Forgets monster slot i's kind and frees its models (modelFirst[i], modelCount[i] of them).
 * The bound first + n is written in the loop's test, not kept in a local: the compiler hoists
 * it out of the loop itself, and gives it the register the game's code does. */
void EmSlot_Clear(s16 i)
{
    s16 j, first, n;

    EmSlot_ForgetModel(i, 0);
    first = gScene.modelFirst[i];
    n = gScene.modelCount[i];
    for (j = first; j < first + n; j++) {
        Model_Free(j);
        if (gModelUsed[j] != 0) {
            Model_Release(j, 1);
        }
    }
}

/*
 * Frees the models of the quest's monster in slot i and its second model set, marks both
 * empty (count -1), frees its motions and clears its kind.
 * FAKEMATCH (docs/notes/hard-matches/EmSlot_FreeModels.md): the game's code computes i and i * 2 in
 * an order that plain C doesn't give, and three forms here reproduce it:
 * - the (s16) cast on the index of the first count read;
 * - n set from i with an explicit cast, and the last store indexed with i itself;
 * - the empty test of first, which the compiler removes but which still splits the block
 *   after the first loop in two.
 */
void EmSlot_FreeModels(s16 i)
{
    SceneState *scene = &gScene;
    s16 j, first, cnt;
    s32 n = (s32)i;

    first = gScene.modelFirst[n];
    cnt = gScene.modelCount[(s16)n];
    for (j = first; j < first + cnt; j++) {
        Model_Free(j);
        if (gModelUsed[j] != 0) {
            Model_Release(j, 1);
        }
    }
    first = scene->partFirst[n];
    if (first < 0 || first >= 128) {} /* the empty test the match needs (above) */
    cnt = scene->partCount[n];
    if (cnt != -1) {
        for (j = first; j < first + cnt; j++) {
            Model_Free(j);
            if (gModelUsed[j] != 0) {
                Model_Release(j, 1);
            }
        }
    }
    scene->modelFirst[n] = 0;
    scene->modelCount[n] = -1;
    scene->models[n] = NULL;
    scene->partFirst[n] = 0;
    scene->partCount[n] = -1;
    scene->partModels[n] = NULL;
    Motion_FreeEmSlot(i);
    scene->emKind[i] = 0;
}

/* Frees the area's models (AreaState.modelSlot on, modelCount of them) and its sub-models
 * (gAreaSubModelIds). */
void Area_FreeModels(void)
{
    s16 i;
    s16 k;
    s16 id;
    AreaState *area;

    i = gAreaState.modelSlot;
    area = &gAreaState;
    if (i < i + gAreaState.modelCount) {
        do {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
            i++;
        } while (i < area->modelSlot + area->modelCount);
    }
    for (k = 0; k < 4; k++) {
        if (gAreaSubModelIds[k] != -1) {
            id = gAreaSubModelIds[k];
            Model_Free(id);
            if (gModelUsed[id] != 0) {
                Model_Release(id, 1);
            }
        }
    }
}

u8 Village_FacilityLevel(s32 facility);
void Load_AreaSubModelPack(s32 id, s32 sub);
extern s32 gAreaSubModelFiles[];
extern s32 gAreaSubModelTexFiles[];
extern s32 gOnlineVillageSubModelTexFiles[];
extern s32 gOnlineVillageSubModelTexFileSeason1;
extern s32 gArea89SubModelTexFiles[];
extern s16 gAreaSubModelKinds[4];

/* Loads area `id`'s sub-models (Area_SubModelCount of them, at most four) into
 * gAreaSubModelIds and gAreaSubModels; unused entries get -1 and NULL.
 * - Sub-model 0 is the area's own (none when gAreaSubModelFiles has -1), with its texture
 *   from gAreaSubModelTexFiles; AREA_ONLINE_VILLAGE in the lobby takes the texture of the season
 *   (LobbyWorld_GetSeason).
 * - Sub-models 1-3 exist only in AREA_HUB_0x89: the textures of each facility's level
 *   (Village_FacilityLevel). */
void Area_LoadSubModels(u16 id)
{
    s32 n;
    s32 model;
    s32 i;
    s32 tex;

    n = Area_SubModelCount(id);
    for (i = 0; i < 4; i++) {
        if (i < n) {
            if (i == 0) {
                if (gAreaSubModelFiles[id] == -1) {
                    gAreaSubModelIds[i] = -1;
                    gAreaSubModels[i] = NULL;
                    continue;
                }
                switch (id) {
                case AREA_ONLINE_VILLAGE:
                    if (gScene.inLobby != 0) {
                        switch (LobbyWorld_GetSeason()) {
                        case 2:
                        default:
                            tex = gOnlineVillageSubModelTexFiles[0];
                            break;
                        case 0:
                            tex = gOnlineVillageSubModelTexFiles[1];
                            break;
                        case 1:
                            tex = gOnlineVillageSubModelTexFileSeason1;
                            break;
                        }
                    } else {
                        tex = gOnlineVillageSubModelTexFiles[0];
                    }
                    break;
                default:
                    tex = gAreaSubModelTexFiles[id];
                    break;
                }
            } else {
                switch (id) {
                case AREA_HUB_0x89:
                    tex = gArea89SubModelTexFiles[(i - 1) * 3 + (u8)Village_FacilityLevel(i)];
                    break;
                default:
                    gAreaSubModelIds[i] = -1;
                    gAreaSubModels[i] = NULL;
                    continue;
                }
            }
            Load_AreaSubModelPack(id, i);
            model = Model_Alloc(1);
            if (model < 0) {
                return;
            }
            gAreaSubModelIds[i] = model;
            gAreaSubModels[i] = Model_Get(model);
            Model_Reserve(model, 1);
            Model_SetupWithPack(model, (*(s32 *)&gSysSettings.modelPack), gAreaSubModelKinds[i], tex, 0, 0);
        } else {
            gAreaSubModelIds[i] = -1;
            gAreaSubModels[i] = NULL;
        }
    }
}

extern s32 gSubModelPairTexFiles[];
extern void *gAreaSubModel;

/* Loads the character maker's parts model for the gender (its file pair, Stage_LoadFilePair)
 * into gAreaSubModelId and gAreaSubModel: one material set per part. */
void CharMake_LoadPartsModel(s32 gender)
{
    s32 buf;
    s32 model;

    Stage_LoadFilePair(gender);
    buf = (*(s32 *)&gSysSettings.dataBuf);
    model = Model_Alloc(1);
    if (model >= 0) {
        gAreaSubModelId = model;
        gAreaSubModel = Model_Get(model);
        Model_Reserve(model, 1);
        Model_SetupWithPackRaw(model, buf, 13, gSubModelPairTexFiles[gender], 2304, 2);
    }
}

/* Frees the character maker's parts model. */
void CharMake_FreePartsModel(void)
{
    s32 id;

    id = gAreaSubModelId;
    Model_Free(id);
    if (gModelUsed[id] != 0) {
        Model_Release(id, 1);
    }
}
