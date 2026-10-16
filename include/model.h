#ifndef MODEL_H
#define MODEL_H

#include "common.h"

/* A bone of a model's skeleton (0x190 bytes): the pool Bone_Get hands out (Model +0x24),
 * a tree of siblings and children (sub_main unk_2EF020.c: Bone_UpdateWorld,
 * Bone_StoreMatrices). Its matrices are f32[16] (row vectors, the translation at
 * [12..14]: Mtx_GetTrans). Named from the monsters' code, which reads em->model->bones[i]. */
typedef struct EmBone {
    f32 world[16];              /* 0x000: local times the parent's world (Bone_UpdateWorld) */
    f32 local[16];              /* 0x040: from the pose at 0x104 (Mtx_FromTransform) */
    f32 skin[16];               /* 0x080: the matrix slot is skin times world (Bone_StoreMatrices) */
    u8 padC0[2];
    s16 count;                  /* 0x0C2: in a model's first bone, the bones of the tree (Model_Setup's loop, unk_376C00.c) */
    s16 index;                  /* 0x0C4: a slot of the matrix table gMtxSlots */
    u16 id;                     /* 0x0C6: the part it belongs to */
    struct EmBone *parent;      /* 0x0C8 */
    struct EmBone *sibling;     /* 0x0CC */
    struct EmBone *child;       /* 0x0D0 */
    s32 anim;                   /* 0x0D4: a handle (Anim_SetData) */
    s32 animOfs;                /* 0x0D8: an offset into it */
    f32 base[10];               /* 0x0DC: the rest pose: [0..2] scale, [3..5] rotation, [6..8] translation */
    f32 pose[10];               /* 0x104: the current pose */
    u8 pad12C[0x15C - 0x12C];
    s16 keyHint[12];            /* 0x15C: per pose value, the key found last (Anim_EvalCurve) */
    s32 unk174;                 /* 0x174 */
    s32 unk178;                 /* 0x178 */
    s32 unk17C;                 /* 0x17C */
    s16 siblingIdx;             /* 0x180: -1 none */
    s16 childIdx;               /* 0x182: -1 none */
    u8 pad184[0x190 - 0x184];
} EmBone;
typedef char EmBone_size_check[(sizeof(EmBone) == 0x190) ? 1 : -1];

/* A material of a model (0x4C bytes; Model +0x10, Model_LoadMaterials; ModelFile_ReadMat
 * fills it from the model file's material record). Gs_SetState's registers 0x3A-0x59 copy one
 * to gRsMaterials (main unk_183D20.c), which Model_BuildPartPacket (sub_main unk_2E3940.c)
 * puts in a part's VU1 packet. */
typedef struct ModelMat {
    f32 unk00;
    f32 rgb[3];         /* 0x04: GmEm_PartColor's out[1..3]; the file's 0x10 */
    f32 alpha;          /* 0x10: 0 hides it (gm_sub GmEm_SetMeshAlphas: from Em +0x6E4); times 128 in the packet */
    f32 unk14[4];       /* 0x14: the file's 0x20; in the packet only for VU1 program kind 2, with size */
    f32 unk24[4];       /* 0x24: the file's 0x00; in the packet after the colour */
    s32 unk34[4];       /* 0x34: 0 */
    s32 tex;            /* 0x44: a texture handle (gTexHandles; ModelFile_ReadMat stores the file's id, Model_LoadMaterials the handle), 0 none */
    s32 size;           /* 0x48: the file's 0x30 (an f32) as an integer: 0-2, 4 .. 0x100 (a power of two), else a level + 1 (Model_BuildPartPacket) */
} ModelMat;
typedef char ModelMat_size_check[(sizeof(ModelMat) == 0x4C) ? 1 : -1];

/* A material set of a model (0x8C bytes; Model +0x30, the pool gMatSets, MatSet_Get):
 * a part of the model file with the materials it draws with (the stage objects draw
 * gCommonModels[0]->matSets[41], [43], [58], [88], [89], [91], [118], [150]...). */
typedef struct ModelMatSet {
    s32 id;             /* 0x00: its draw model (Model_Create's result; Model_Draw), -1: none */
    s32 count;          /* 0x04: materials used */
    s32 mat[32];        /* 0x08: their indices in the model's materials */
    s32 flags;          /* 0x88: draw state flags (DrawStates_Apply, Draw_ApplyStates) */
} ModelMatSet;

/*
 * A model (0x80 bytes): an entry of the pool at *gModels (0x80 entries,
 * Model_Get, Model_Alloc; unk_339EC0.c). Set up by Model_SetupWithPack,
 * Model_SetupWithPackRaw, Model_SetupWithPackEx (unk_335040.c) and Model_SetupTexPack,
 * Model_SetupTexPackB (unk_339EC0.c). The monsters' and the hunters' skeleton
 * (Em.model, Player.model at 0x4EC) and the stage objects' common model sets
 * (gCommonModels) are models too. unk_376C00.c has another view
 * of it (ModelHeader).
 */
typedef struct Model {
    u8 active;          /* 0x00 */
    u8 unk01;           /* 0x01 */
    u8 unk02[0x04 - 0x02];
    u8 *file;          /* 0x04: the model file (Model_SetupWithPack and the like; Model_InitMatSets reads it) */
    s16 texCount;       /* 0x08: the textures of its pack (Tex_LoadPack's result: Tex_RegisterPack's count; *pack for a loaded pack) */
    s16 texSlot;        /* 0x0A: the texture slot its pack was loaded to (Model_SetupWithPack and the like: the slot passed to Tex_LoadPack) */
    s16 matCount;       /* 0x0C: materials: slots of the 0x50-byte pool (Model_LoadMaterials, Material_Release) */
    s16 matFirst;       /* 0x0E: its first material slot */
    ModelMat *mats;     /* 0x10: matCount materials (Model_LoadMaterials) */
    u8 unk14[0x20 - 0x14];
    s16 boneCount;      /* 0x20: bone slots (0x190-byte pool) */
    s16 boneFirst;      /* 0x22: first bone slot */
    EmBone *bones;      /* 0x24 */
    EmBone *bones2;     /* 0x28 */
    s16 matSetCount;    /* 0x2C: the model file part count (ModelFile_PartCount): the material set slots it takes (Model_InitMatSets, Model_InitMatSetsSlot) */
    s16 matSetFirst;    /* 0x2E: its first slot of the 0x8C-byte pool (gMatSets; MatSet_Release) */
    ModelMatSet *matSets; /* 0x30: matSetCount material sets */
    s32 partCount;      /* 0x34 */
    u8 unk38[0x44 - 0x38];
    EmBone *partBones[4];   /* 0x44: each part's first bone in bones */
    EmBone *partBones2[4];  /* 0x54: the same in bones2 (kind 2) */
    s32 partSkels[4];   /* 0x64: each part's skeleton copy (Skeleton_CopyData; Bone_InitTreeRoot, Model_FreePartCopies) */
    u8 boneKind;        /* 0x74: the kind of its bone setup (unk_335040.c; 2: bones2 too, unk_376C00.c) */
    u8 unk75;           /* 0x75 */
    u8 unk76[0x80 - 0x76];
} Model;
typedef char Model_size_check[(sizeof(Model) == 0x80) ? 1 : -1];

/*
 * A mesh: one part of a model file built for drawing (0x24 bytes; Mesh_BuildRigid and
 * Mesh_BuildSkinned from ModelFile_LoadMesh, sub_main unk_2F6540.c; flPS2ConvClayData, from
 * Model_BuildVifPackets' debug string). Model_Create copies it with its strips and vertices
 * into one block (the strips at +0x30, the vertices after them, 16-aligned) and Model_ReadHeader
 * reads it back from there; Model_SetupEntry and Model_BuildVifPackets build the model's VIF
 * packets from it (sub_main unk_2E3940.c).
 */
typedef struct MeshDesc {
    u32 flags;          /* 0x00: 0x1000 indexed vertices, 0x100000 / 0x200000 per-strip material ids */
    s32 count;          /* 0x04: strips (ModelChunk5_TotalItems) */
    u16 *strips;        /* 0x08: per strip its length (low 14 bits), then the ids and indexes */
    s32 format;         /* 0x0C: vertex format 0x25, 0x35 (| 0x10: texture coordinates) or 0x135 */
    u8 *verts;          /* 0x10: 0x24 bytes each (Mesh_BuildRigid), 0x2C (Mesh_BuildSkinned) */
    s32 vertCount;      /* 0x14: ModelChunk7_Count */
    s32 vertsSize;      /* 0x18: bytes at verts */
    s32 stripsSize;     /* 0x1C: bytes at strips */
    s16 matCount;       /* 0x20: ModelFile_PartMatCount */
} MeshDesc;
typedef char MeshDesc_size_check[(sizeof(MeshDesc) == 0x24) ? 1 : -1];

/* The pools' sizes, in entries (Model_ResetAll frees them all): the models
 * (*gModels), the materials, the bones (EmBone), the material sets
 * (ModelMatSet) and the draw models. */
#define MODEL_POOL_COUNT 0x80
#define MATERIAL_POOL_COUNT 0x400
#define BONE_POOL_COUNT 0x800
#define MATSET_POOL_COUNT 0x180

/* Places in the model load buffer (gModelLoadBuf, unk_335040.c): a model
 * file at its start, its skeleton file (the pack's part 1) after it; a
 * stage's second file (FilePair) holds the alternative skeleton
 * (Model_InitSkeleton's alt). */
#define MODEL_LOAD_SKEL_OFFSET 0x128000
#define MODEL_LOAD_ALT_SKEL_OFFSET 0x298000

Model *Model_Get(s32 i);

/* sub_main 0x5DA070: the current area's four model sets (unk_33B080.c sets them from Model_Get; the
 * area props and objects draw [0]). */
extern Model *gAreaSubModels[4];

/* sub_main 0x5DA030: the five common model sets (unk_3240E0.c sets [0], [1], [2], [4] from
 * Model_Get; unk_368F10.c clears them): [0] the one the stage objects, effects and village
 * objects draw from, [2] the bow's aim arc (kind 0x2B). */
extern Model *gCommonModels[5];

/* gCommonModels read through a pointer cast, as the stage objects' functions that use these
 * read it: their load keeps the declaration's place among the locals, where a plain
 * gCommonModels[0] is a copy of a global numbered after them (docs/mwcc-matching.md, "Saved
 * registers in another order", 2). */
#define gStageModelSet (*(Model **)&gCommonModels)
#define gStageModelSets ((Model **)&gCommonModels)

/* sub_main 0x441170: per area (gScene.area), 16 material set indices into gAreaSubModels[0]'s
 * matSets, -1: none (unk_37B570.c; most rows are gm_sub's D_0092AD60): [0] the set
 * Player_GetAreaAmbModel checks and Eff12 draws (three from it), [1] AmbientSound_Draw's,
 * [2..9] AreaObj_K09_DrawType1..8's. */
extern s16 *gAreaAmbModels[200];

/*
 * A skeleton file (read by unk_2F61D0.c; unk_2F6540.c builds skeletons
 * from it): a SkelFileHeader, then `count` chunks, each a SkelChunk
 * header and its data from +0xC. The parts chunk (type
 * SKEL_CHUNK_PARTS) lists each part's root node; every other chunk holds
 * one node, a SkelFileNode. A part's nodes form a tree through child and
 * sibling (-1: none).
 */
#define SKEL_FILE_HAS_PARTS 0x80000000  /* SkelFileHeader.flags: one chunk is the parts chunk, not a bone */
#define SKEL_CHUNK_PARTS 0              /* SkelChunk.type of the parts chunk */

typedef struct SkelFileHeader {
    u32 flags;                  /* 0x0: SKEL_FILE_HAS_PARTS */
    u32 count;                  /* 0x4: the chunks */
    u32 unk8;
} SkelFileHeader;

typedef struct SkelChunk {
    u8 type;                    /* 0x0: SKEL_CHUNK_PARTS, or a node's */
    u8 pad1[3];
    u32 count;                  /* 0x4: the parts chunk's: the parts (s32 root nodes from +0xC) */
    u32 size;                   /* 0x8: the whole chunk's: the next chunk follows */
} SkelChunk;

/* A node of a skeleton file (SkelFile_GetNode). */
typedef struct SkelFileNode {
    s32 index;                  /* 0x00 */
    s32 unk4;
    s32 child;                  /* 0x08 */
    s32 sibling;                /* 0x0C */
    f32 scale[4];               /* 0x10 */
    f32 rot[4];                 /* 0x20 */
    f32 trans[4];               /* 0x30 */
    s32 unkA;                   /* 0x40 */
    s32 id;                     /* 0x44 */
} SkelFileNode;

/*
 * A skeleton part as built (SkelFile_BuildPart, unk_2F6540.c, from a
 * skeleton file): this header, then `count` BoneDefs, SkelFile_PartSize
 * bytes in all. It is built in a buffer (gLoadBuf), and the header is
 * copied to the caller's, whose `block` then points at the buffer;
 * Skeleton_CopyData (unk_2E3940.c) copies the block into a memory handle,
 * from which Bone_InitTree (unk_2EF020.c) sets up a model's bones.
 */
typedef struct SkelHeader {
    s16 unk0;                   /* 0x0: not set by SkelFile_BuildPart */
    s16 count;                  /* 0x2: the bones */
    s32 headerSize;             /* 0x4: 0x10: the bones follow */
    struct SkelHeader *block;   /* 0x8: in the caller's copy, the built block; NULL in the block */
    s32 size;                   /* 0xC: the block's bytes */
} SkelHeader;

/* A bone of a built skeleton (0x40 bytes): a skeleton file's node with its
 * links made relative to the part (-1: none). Bone_ReadDef reads it into an
 * EmBone. */
typedef struct BoneDef {
    s16 index;                  /* 0x00 */
    u16 id;                     /* 0x02: the part it belongs to (EmBone.id) */
    u8 pad04[2];
    s16 child;                  /* 0x06 */
    s16 sibling;                /* 0x08 */
    s16 unkA;                   /* 0x0A: the node's unkA; Bone_ReadDef keeps it in EmBone.unk174 */
    u8 pad0C[4];
    f32 scale[4];               /* 0x10 */
    f32 rot[4];                 /* 0x20 */
    f32 trans[4];               /* 0x30 */
} BoneDef;

/*
 * An entry of the draw model pool gModelPool (0x180 entries of 0x180 bytes;
 * a draw model's handle is its index + 1): a model file part set up for
 * drawing (Model_Create, Model_Setup; unk_2E3940.c), with its VU1 program
 * and its materials' textures.
 */
typedef struct ModelEntry {
    u32 flags;                  /* 0x00: 2 and 4 select how Model_Draw builds its packets */
    s32 used;                   /* 0x04: from the file header, 0 free */
    s32 unk08;                  /* 0x08 */
    s32 frame;                  /* 0x0C: the frame its data was last set up */
    s32 size;                   /* 0x10: of the copy at 0x14 */
    void *file;                 /* 0x14: a handle, the copied file */
    s32 unk18;                  /* 0x18 */
    void *data;                 /* 0x1C: a handle */
    u8 *dataAddr;               /* 0x20 */
    s32 unk24;                  /* 0x24 */
    s32 pktSize;                /* 0x28 */
    s32 unk2C;                  /* 0x2C */
    s32 unk30;                  /* 0x30 */
    s32 partCount;              /* 0x34 */
    s32 texCount;               /* 0x38 */
    s32 unk3C;                  /* 0x3C */
    s32 unk40;                  /* 0x40 */
    s32 unk44;                  /* 0x44 */
    s32 prog;                   /* 0x48: VU1 program (Vu1_SelectProgram), -1 none */
    s32 *progInfo;              /* 0x4C */
    s32 unk50;                  /* 0x50 */
    s32 unk54;                  /* 0x54 */
    u8 pad58[0x60 - 0x58];
    u8 mat[0x20];               /* 0x60: per part, a material of gRsMaterials */
    s32 partTex[0x20];          /* 0x80: per part, its material's texture */
    s32 tex[0x20];              /* 0x100: the distinct textures */
} ModelEntry;
typedef char ModelEntry_size_check[(sizeof(ModelEntry) == 0x180) ? 1 : -1];

/* A 4x4 matrix as one object, so that it copies as a whole: a slot of the
 * matrix table gMtxSlots (MtxSlot_Store, MtxSlot_Load; EmBone.index). */
typedef struct Mtx44Copy {
    f32 m[4][4];
} Mtx44Copy;

#endif /* MODEL_H */
