#ifndef MODEL_H
#define MODEL_H

#include "common.h"

/* A bone of a model's skeleton (0x190 bytes): the pool Bone_Get hands out (Model +0x24),
 * a tree of siblings and children (Bone_UpdateWorld,
 * Bone_StoreMatrices). Its matrices are f32[16] (row vectors, the translation at
 * [12..14]: Mtx_GetTrans). Named from the monsters' code, which reads em->model->bones[i]. */
typedef struct EmBone {
    f32 world[16];              /* 0x000: local times the parent's world (Bone_UpdateWorld) */
    f32 local[16];              /* 0x040: from the pose at 0x104 (Mtx_FromTransform) */
    f32 skin[16];               /* 0x080: the matrix slot is skin times world (Bone_StoreMatrices) */
    u8 padC0[2];
    s16 count;                  /* 0x0C2: in a model's first bone, the bones of the tree (Model_Setup's loop) */
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
 * to gRsMaterials, which Model_BuildPartPacket puts in a part's VU1 packet. */
typedef struct ModelMat {
    f32 unk00;
    f32 rgb[3];         /* 0x04: GmEm_ShenGaorenFlinchColor's out[1..3]; the file's 0x10 */
    f32 alpha;          /* 0x10: 0 hides it (gm_sub GmEm_PrepareMeshSet: from Em +0x6E4); times 128 in the packet */
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
 * Model_Get, Model_Alloc). Set up by Model_SetupWithPack,
 * Model_SetupWithPackRaw, Model_SetupWithPackEx and Model_SetupTexPack,
 * Model_SetupTexPackB. The monsters' and the hunters' skeleton
 * (Em.model, Player.model at 0x4EC) and the stage objects' common model sets
 * (gCommonModels) are models too.
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
    u8 boneKind;        /* 0x74: the kind of its bone setup (2: bones2 too) */
    u8 isCopy;          /* 0x75: nonzero: only its bones are its own, so Model_Free releases only them (Model_FreeCopy); the set-ups clear it */
    u8 unk76[0x80 - 0x76];
} Model;
typedef char Model_size_check[(sizeof(Model) == 0x80) ? 1 : -1];

/*
 * A mesh: one part of a model file built for drawing (0x24 bytes; Mesh_BuildRigid and
 * Mesh_BuildSkinned from ModelFile_LoadMesh; flPS2ConvClayData, from
 * Model_BuildVifPackets' debug string). Model_Create copies it with its strips and vertices
 * into one block (the strips at +0x30, the vertices after them, 16-aligned) and Model_ReadHeader
 * reads it back from there; Model_SetupEntry and Model_BuildVifPackets build the model's VIF
 * packets from it.
 */
typedef struct MeshDesc {
    u32 flags;          /* 0x00: MESH_* */
    s32 count;          /* 0x04: strips (ModelPart_StripCount) */
    u16 *strips;        /* 0x08: per strip its length (MESH_STRIP_LEN), then the ids and indexes MESH_* says */
    s32 format;         /* 0x0C: MESH_FORMAT_*: the vertices' layout */
    u8 *verts;          /* 0x10: MeshVtxCol, MeshVtxTex or MeshVtxSkin by the format */
    s32 vertCount;      /* 0x14: ModelPart_VertCount */
    s32 vertsSize;      /* 0x18: bytes at verts */
    s32 stripsSize;     /* 0x1C: bytes at strips */
    s16 matCount;       /* 0x20: ModelFile_PartMatCount */
} MeshDesc;

/* MeshDesc.flags: what follows each strip's length in strips. */
#define MESH_INDEXED 0x1000         /* the strip's vertex indexes */
#define MESH_BUILT 0x20000          /* set by both builders; nothing seen reads it */
#define MESH_STRIP_MAT 0x100000     /* a material id (of the model's parts) */
#define MESH_STRIP_SKIN 0x200000    /* a skin kind, MESH_SKIN_* */
#define MESH_STRIP_LEN 0x3FFF       /* the mask of a strip's first word: its vertices */
#define MESH_STRIP_FLAGGED 0x8000   /* ORed into the length when the file's strip has ModelStrip.flag set */
#define MESH_STRIP_UNFLAGGED 0x4000 /* ORed in when not (the draw code masks both off: MESH_STRIP_LEN) */
#define MESH_SKIN_ONE_BONE 0        /* the strip's vertices follow one bone at full weight */
#define MESH_SKIN_FOUR_BONES 1      /* they follow four weighted bones */
typedef char MeshDesc_size_check[(sizeof(MeshDesc) == 0x24) ? 1 : -1];

/* MeshDesc.format: the three vertex layouts the builders make and
 * Model_BuildVifPackets reads. */
#define MESH_FORMAT_COLOR 0x25      /* MeshVtxCol: position, normal, colour */
#define MESH_FORMAT_UV 0x10         /* with MESH_FORMAT_COLOR: MeshVtxTex, texture coordinates added */
#define MESH_FORMAT_SKIN 0x100      /* with both: MeshVtxSkin, bones and weights added */

/* The mesh keys Vu1_SelectProgram picks a VU1 program by: ModelEntry.used
 * (the MeshDesc flags) | ModelEntry.vtxFormat, for the meshes the builders make with
 * texture coordinates. */
#define MESH_KEY_RIGID (MESH_BUILT | MESH_INDEXED | MESH_FORMAT_COLOR | MESH_FORMAT_UV)
#define MESH_KEY_RIGID_MATS (MESH_KEY_RIGID | MESH_STRIP_MAT)
#define MESH_KEY_SKINNED (MESH_KEY_RIGID | MESH_STRIP_SKIN | MESH_FORMAT_SKIN)
#define MESH_KEY_SKINNED_MATS (MESH_KEY_SKINNED | MESH_STRIP_MAT)

/* A vertex of MESH_FORMAT_COLOR. The colour is a, r, g, b from the top byte down. */
typedef struct MeshVtxCol {
    f32 pos[3];                 /* 0x00 */
    f32 nrm[3];                 /* 0x0C */
    u32 col;                    /* 0x18 */
} MeshVtxCol;

/* A vertex of MESH_FORMAT_COLOR | MESH_FORMAT_UV. */
typedef struct MeshVtxTex {
    f32 pos[3];                 /* 0x00 */
    f32 nrm[3];                 /* 0x0C */
    u32 col;                    /* 0x18 */
    f32 uv[2];                  /* 0x1C */
} MeshVtxTex;

/* A vertex of all three: four bone indexes and their weights (0-0xFF, summing to 0xFF),
 * the first of each in the top byte. */
typedef struct MeshVtxSkin {
    f32 pos[3];                 /* 0x00 */
    f32 nrm[3];                 /* 0x0C */
    u32 col;                    /* 0x18 */
    f32 uv[2];                  /* 0x1C */
    u32 bones;                  /* 0x24 */
    u32 weights;                /* 0x28 */
} MeshVtxSkin;

/* Angles: the game turns a model by 16-bit angles, 0x10000 to a full turn
 * (Em.rot, the bones' rotations). Math_AngleToRad multiplies by
 * ANGLE16_TO_RAD (2 pi / 0x10000); RAD_TO_ANGLE16 is its inverse. */
#define ANGLE16_TO_RAD 0.0000958738f
#define RAD_TO_ANGLE16 10430.378f

/* The pools' sizes, in entries (Model_ResetAll frees them all): the models
 * (*gModels), the materials, the bones (EmBone), the material sets
 * (ModelMatSet) and the draw models. */
#define MODEL_POOL_COUNT 0x80
#define MATERIAL_POOL_COUNT 0x400
#define BONE_POOL_COUNT 0x800
#define MATSET_POOL_COUNT 0x180

/* Places in the model load buffer (gModelLoadBuf): a model
 * file at its start, its skeleton file (the pack's part 1) after it; a
 * stage's second file (FilePair) holds the alternative skeleton
 * (Model_InitSkeleton's alt). */
#define MODEL_LOAD_SKEL_OFFSET 0x128000
#define MODEL_LOAD_ALT_SKEL_OFFSET 0x298000

Model *Model_Get(s32 i);

/* The current area's four model sets (Area_LoadSubModels sets them from Model_Get; the
 * area props and objects draw [0]). */
extern Model *gAreaSubModels[4];

/* The five common model sets (Quest_LoadCommonModels sets [0], [1], [2], [4] from
 * Model_Get; Game_ResetSubsystems clears them): [0] the one the stage objects, effects and village
 * objects draw from, [2] the bow's aim arc (kind 0x2B). */
extern Model *gCommonModels[5];

/* gCommonModels read through a pointer cast, as the stage objects' functions that use these
 * read it: their load keeps the declaration's place among the locals, where a plain
 * gCommonModels[0] is a copy of a global, which the compiler would put after them. */
#define gStageModelSet (*(Model **)&gCommonModels)
#define gStageModelSets ((Model **)&gCommonModels)

/* Per area (gScene.area), 16 material set indices into gAreaSubModels[0]'s
 * matSets, -1: none (most rows are a table of gm_sub's): [0] the set
 * Player_GetAreaPitModel checks and PitfallTrap draws (three from it), [1] AreaAtmos_Draw's,
 * [2..9] the draws of AmbientLife's types 1 to 8 (AmbientLife_DrawFallingLeaf ...). */
extern s16 *gAreaFxModels[200];

/*
 * A skeleton file (read by the skeleton loader; SkelFile_BuildPart builds
 * skeletons from it): a SkelFileHeader, then `count` chunks, each a SkelChunk
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

/*
 * A model file (ModelFile_LoadMesh and its readers) is a tree of chunks in the same
 * container as a skeleton file: each chunk is a 0xC-byte header, s32 type,
 * u32 count (its child chunks, or its records) and u32 size (the whole
 * chunk: the next one follows), then its data. The file is the root chunk.
 * The chunk types:
 */
#define MODEL_CHUNK_ANY (-1)            /* ModelChunk_Get: a chunk of any type */
#define MODEL_CHUNK_PARTS 2             /* the file's parts; each child's type is its kind */
#define MODEL_PART_MESH 4               /* the kind of a mesh part, the only kind built */
#define MODEL_CHUNK_STRIPS 5            /* a part's strip groups (each a chunk of strips) */
#define MODEL_CHUNK_MATERIALS 9         /* the file's materials */
#define MODEL_CHUNK_TEXTURES 0xA        /* the file's textures: each child's first word is its id */
#define MODEL_CHUNK_PART_MATS 0x50000   /* a part's materials, s32 indexes into the file's */
#define MODEL_CHUNK_STRIP_MATS 0x60000  /* a material per strip (MESH_STRIP_MAT) */
#define MODEL_CHUNK_POSITIONS 0x70000   /* the vertices' positions, 3 f32 each */
#define MODEL_CHUNK_NORMALS 0x80000     /* the vertices' normals, 3 f32 each */
#define MODEL_CHUNK_UVS 0xA0000         /* texture coordinates, 2 f32 each */
#define MODEL_CHUNK_COLORS 0xB0000      /* colours, 4 f32 each */
#define MODEL_CHUNK_WEIGHTS 0xC0000     /* per vertex, a count and its bone weights: a skinned part */
#define MODEL_CHUNK_DRAW_STATES 0xF0000 /* a part's draw states (0x48 bytes) */
#define MODEL_CHUNK_MTX_LIST 0x100000   /* a part's matrix list */
#define MODEL_CHUNK_STRIPS_RIGID 0x30000 /* a strip group whose strips follow one bone (MESH_SKIN_ONE_BONE) */
#define MODEL_CHUNK_STRIPS_SKINNED 0x40000 /* a strip group of four weighted bones (MESH_SKIN_FOUR_BONES) */

/* A model file chunk's header. */
typedef struct ModelChunk {
    s32 type;                   /* 0x0: MODEL_CHUNK_*, or a part's kind (MODEL_PART_MESH ...) */
    u32 count;                  /* 0x4: its child chunks, or its records */
    u32 size;                   /* 0x8: the whole chunk's bytes, this header included */
} ModelChunk;

/* The chunk's first child chunk, and its data when it holds records. */
#define MODEL_CHUNK_FIRST(c) ((c) + 1)
#define MODEL_CHUNK_DATA(c) ((void *)((c) + 1))
/* The chunk after c, in the same parent. */
#define MODEL_CHUNK_NEXT(c) ((ModelChunk *)((u8 *)(c) + (c)->size))

/* A strip in a strip group (MODEL_CHUNK_STRIPS_*): this word, then
 * `length` u32 vertex indexes. */
typedef struct ModelStrip {
    u32 length : 31;
    u32 flag : 1;               /* the mesh keeps it as MESH_STRIP_FLAGGED (meaning unknown) */
} ModelStrip;
#define MODEL_STRIP_FLAG 0x80000000 /* ModelStrip.flag, as a mask of the word */

/* The records of the vertex chunks: positions and normals (ModelVec3), texture
 * coordinates (ModelVec2), colours (ModelVec4: r, g, b, a, each 0-255) and,
 * per vertex in MODEL_CHUNK_WEIGHTS, an s32 count and that many ModelWeights. */
typedef struct ModelVec2 {
    f32 v[2];
} ModelVec2;
typedef struct ModelVec3 {
    f32 v[3];
} ModelVec3;
typedef struct ModelVec4 {
    f32 v[4];
} ModelVec4;
typedef struct ModelWeight {
    u8 bone;                    /* 0x0: the bone's index */
    u8 pad1[3];
    f32 percent;                /* 0x4: the weight, 0-100 */
} ModelWeight;

/* A material record (MODEL_CHUNK_MATERIALS; ModelFile_ReadMat copies it to a ModelMat). */
typedef struct ModelFileMat {
    f32 unk00[4];               /* 0x000: to ModelMat.unk24 */
    f32 rgb[3];                 /* 0x010 */
    f32 alpha;                  /* 0x01C */
    f32 unk20[4];               /* 0x020: to ModelMat.unk14 */
    f32 size;                   /* 0x030: to ModelMat.size, as an integer */
    s32 hasTexture;             /* 0x034: nonzero: texIdx is read */
    u8 pad038[0x100 - 0x038];
    u32 texIdx;                 /* 0x100: a child of MODEL_CHUNK_TEXTURES, whose first word is the texture's id */
} ModelFileMat;

/* A part's draw states (MODEL_CHUNK_DRAW_STATES, 0x48 bytes; ModelFile_GetDrawStates
 * copies them out). Each field indexes a table of values for one render state
 * (gDrawState15Values ...) or gives bits of a material set's packed flags
 * (DrawStates_Apply). */
typedef struct DrawStates {
    s32 version;    /* 0x00: its high half must be 1 (ModelFile_GetDrawStates returns -2 otherwise) */
    s32 s15;        /* 0x04: gDrawState15Values, state 0x15 */
    s32 s02;        /* 0x08: gDrawState02Values, state 2 */
    s32 s00;        /* 0x0C: gDrawState00Values, state 0 */
    s32 s0C;        /* 0x10: gDrawState0CValues, state 0xC; flag bit 15 */
    s32 s01;        /* 0x14: gDrawState01Values, state 1 */
    s32 unk18;
    s32 s62;        /* 0x1C: gDrawState62Values, state 0x62 */
    s32 s12;        /* 0x20: gDrawState12Values, state 0x12 */
    s32 s66;        /* 0x24: gDrawState66Values, state 0x66 (1: also 0x67); flag bit 1 */
    s32 s5D;        /* 0x28: 5 or 6 sets state 0x5D to that << 24, else 0 */
    s32 f2;         /* 0x2C: flag bits 2-5 */
    s32 f6;         /* 0x30: flag bits 6-9 */
    s32 f10;        /* 0x34: flag bits 10-11 */
    s32 unk38[2];
    s32 f12;        /* 0x40: flag bit 12 */
    s32 f13;        /* 0x44: flag bits 13-14 */
} DrawStates;
#define DRAW_STATES_VERSION_MASK 0xFFFF0000
#define DRAW_STATES_VERSION 0x10000 /* DrawStates.version & DRAW_STATES_VERSION_MASK */

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
 * A skeleton part as built (SkelFile_BuildPart, from a
 * skeleton file): this header, then `count` BoneDefs, SkelFile_PartSize
 * bytes in all. It is built in a buffer (gLoadBuf), and the header is
 * copied to the caller's, whose `block` then points at the buffer;
 * Skeleton_CopyData copies the block into a memory handle,
 * from which Bone_InitTree sets up a model's bones.
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

/* For the skinned VU1 programs: where the bone matrices go in VU1 memory. */
typedef struct Vu1BoneLayout {
    s32 *base;                  /* bone matrix 0; &base[n] is bone n's (4 quadwords each) */
    s32 off;                    /* the normal matrices' start, in those units */
    s32 size;                   /* quadwords per normal matrix */
} Vu1BoneLayout;

struct ModelEntry;

/*
 * A VU1 program's descriptor (12 words, gVu1ProgInfo00 ... for the 60 programs;
 * Vu1_SelectProgram picks one for a model). Model_Draw sends the program's code when
 * the program changes, then runs the packet builder, which uploads the program's
 * constants and starts it.
 */
typedef struct Vu1ProgInfo {
    s32 upload;                 /* 0x00: the DMA chain that loads the program's code, which Model_Draw CALLs */
    s32 start;                  /* 0x04: where the program starts, in bytes (0 in every descriptor) */
    s32 codeStart;              /* 0x08: where its code goes in VU1 memory, in bytes (0 in every descriptor) */
    s32 codeEnd;                /* 0x0C: the end of its code: (codeEnd - codeStart) >> 3 is gSysWork.vu1Size */
    u8 *(*build)(struct ModelEntry *entry, u32 start, u8 *next); /* 0x10: the packet builder (Vu1Pkt_*): start in doublewords, the chain to go on with */
    s32 pktKind;                /* 0x14: the part packets' kind (gModelPktSteps; Model_BuildPartPackets, Model_SetTexRegs) */
    s32 layout;                 /* 0x18: the vertex layout (gVu1ProgLayouts) */
    s32 gifInfo;                /* 0x1C: the GIF tag fields (gVu1GifInfos) */
    s32 bufSize;                /* 0x20: the batch buffer, in bytes: the VIF data is cut into batches that fit */
    f32 colorScale;             /* 0x24: 128.0 in every descriptor: the GS's 1.0, by which the builders scale the colours */
    Vu1BoneLayout *bones;       /* 0x28: the skinned programs' bone matrices, else NULL */
    s32 pad2C;
} Vu1ProgInfo;
typedef char Vu1ProgInfo_size_check[(sizeof(Vu1ProgInfo) == 0x30) ? 1 : -1];

/*
 * An entry of the draw model pool gModelPool (0x180 entries of 0x180 bytes;
 * a draw model's handle is its index + 1): a model file part set up for
 * drawing (Model_Create, Model_Setup), with its VU1 program
 * and its materials' textures.
 */
typedef struct ModelEntry {
    u32 flags;                  /* 0x00: MODEL_* (Model_Create's flags) */
    s32 used;                   /* 0x04: the file header's flags (MeshDesc.flags), 0 free */
    s32 vtxFormat;              /* 0x08: the file header's vertex format; with used, the key of Vu1_SelectProgram */
    s32 renderFlags;            /* 0x0C: gRenderFlags2 when the data was built; Model_Draw rebuilds it when they differ */
    s32 size;                   /* 0x10: of the copy at 0x14 */
    void *file;                 /* 0x14: a handle, the copied file */
    s32 vifSize;                /* 0x18: bytes of VIF data (Model_SetupEntry; Model_BuildVifPackets checks it) */
    void *data;                 /* 0x1C: a handle: the VIF data, the chain table, the part packets */
    u8 *dataAddr;               /* 0x20 */
    s32 pktOfs;                 /* 0x24: the part packets' offset in data */
    s32 pktSize;                /* 0x28 */
    s32 chainTblOfs;            /* 0x2C: the chain table's offset in data (vifSize rounded up to 16): per material, its VIF chain's offset */
    s32 chainTblSize;           /* 0x30: the chain table's bytes (partCount words, rounded up to 16) */
    s32 partCount;              /* 0x34 */
    s32 texCount;               /* 0x38 */
    s32 triCount;               /* 0x3C: triangles of the strips */
    s32 stripVtxCount;          /* 0x40: vertices of the strips */
    s32 matRunCount;            /* 0x44: changes of material between strips */
    s32 prog;                   /* 0x48: VU1 program (Vu1_SelectProgram), -1 none */
    Vu1ProgInfo *progInfo;      /* 0x4C: its program's descriptor */
    s32 unk50;                  /* 0x50: only cleared (Model_Create) */
    s32 boneMask;               /* 0x54: a bit per bone the skinned strips use (Model_BuildVifPackets); the VU1 packets copy those matrix slots */
    u8 pad58[0x60 - 0x58];
    u8 mat[0x20];               /* 0x60: per part, a material of gRsMaterials */
    s32 partTex[0x20];          /* 0x80: per part, its material's texture */
    s32 tex[0x20];              /* 0x100: the distinct textures */
} ModelEntry;
typedef char ModelEntry_size_check[(sizeof(ModelEntry) == 0x180) ? 1 : -1];

#define DRAW_MODEL_COUNT 0x180      /* gModelPool's entries */

/* ModelEntry.flags, Model_Create's flags (gMatSetModelKinds gives
 * them per kind of model). */
#define MODEL_PREBUILT 0x2          /* the part packets are built into data (pktOfs) once, not each frame */
#define MODEL_NO_COPY 0x4           /* no copy of the file is kept, so the data is never rebuilt */
#define MODEL_SCALE_UV 0x20         /* texture coordinates scaled by gDispTexScaleU and gDispTexScaleV */

/* A 4x4 matrix as one object, so that it copies as a whole: a slot of the
 * matrix table gMtxSlots (MtxSlot_Store, MtxSlot_Load; EmBone.index). */
typedef struct MtxSlot {
    f32 m[4][4];
} MtxSlot;

/* The slots of gMtxSlots the camera keeps. */
#define MTX_SLOT_PROJ 0x20  /* the camera's perspective projection (Camera_SetPerspective) */
#define MTX_SLOT_VIEW 0x21  /* a saved copy of the view matrix gViewMatrix (MtxSlot_Store, MtxSlot_Load) */

#endif /* MODEL_H */
