/*
 * The draw models (ModelEntry and MeshDesc, model.h): a model is made from
 * a mesh file (Model_Create, Model_Setup), its VIF packets and its parts'
 * DMA packets are built for its VU1 program (Model_BuildVifPackets,
 * Model_BuildPartPackets) with their GS texture registers (Model_SetTexRegs,
 * Gs_SetTexRegs), its data is relocated when its memory handle moves
 * (Model_RelocateAll), and it is drawn (Model_Draw) and freed
 * (Model_FreeHandle). The data: the packet step and the VIF layout of each
 * VU1 program kind, and the two GIF tag descriptions.
 */

#include "common.h"
#include "syswork.h"
#include "model.h"
#include "texture.h"
#include "hw_regs.h"
#include "gs_setup.h"
#include "gs_tex_regs.h"
#include "packet_tim2.h"
#include "vu1_packets.h"

#pragma optimization_level 4

void Mem_Free(void *handle);
void *Mem_GetAddr(void *handle);
void *Mem_Alloc(u32 size, s32 flag);
void Mem_Fill(void *dst, u8 c, s32 n);
/* unit's own declaration: declared as this file's callers use it; with the definition's void (void *dst, void *src, s32 n), Model_Create compiles differently */
void Mem_Copy(void *dst, void *src, u32 n);

extern ModelEntry gModelPool[];
/* A ModelMat per render state, RS_MATERIAL(0) to RS_MATERIAL(31). */
extern ModelMat gRsMaterials[];
extern s32 gRenderFlags2;
extern DmaQueue gDmaQueue;

/*
 * A new model from the file header hdr (a copy of the file unless flags has 4): its
 * handle (pool index + 1), or 0 when it can't be set up.
 */
s32 Model_Create(MeshDesc *hdr, u32 flags)
{
    u32 i;
    ModelEntry *entry;
    u8 *dst;

    for (i = 0; i < DRAW_MODEL_COUNT; i++) {
        if (gModelPool[i].used == 0) {
            break;
        }
    }
    entry = &gModelPool[i];
    Mem_Fill(entry, 0, sizeof(ModelEntry));
    entry->used = hdr->flags;
    entry->vtxFormat = hdr->format;
    entry->flags = flags;
    entry->unk50 = 0;
    entry->boneMask = 0;
    if (!(flags & MODEL_NO_COPY)) {
        entry->size = 48;
        entry->size += (hdr->stripsSize + 15) & ~0xF;
        entry->size += (hdr->vertsSize + 15) & ~0xF;
        entry->file = Mem_Alloc(entry->size, 3);
        dst = Mem_GetAddr(entry->file);
        Mem_Copy(dst, hdr, 36);
        dst += 48;
        Mem_Copy(dst, hdr->strips, hdr->stripsSize);
        dst += (hdr->stripsSize + 15) & ~0xF;
        Mem_Copy(dst, hdr->verts, hdr->vertsSize);
    }
    entry->renderFlags = gRenderFlags2;
    if (Model_Setup(entry, hdr) == 0) {
        Model_FreeHandle(i + 1);
        return 0;
    }
    return i + 1;
}

static void Model_CollectTextures(ModelEntry *e, s32 unused);

extern s32 gModelCount;
/*
 * Sets up model entry from its mesh hdr: picks the VU1 program, sizes and allocates its
 * data, builds the VIF and part packets and collects the textures. Returns 1, or 0 when no
 * program fits. Counts the models (gModelCount).
 */
s32 Model_Setup(ModelEntry *entry, MeshDesc *hdr)
{
    Vu1_SelectProgram(entry, hdr);
    if (entry->prog != -1) {
        Model_SetupEntry(hdr, entry, 0);
        Model_BuildVifPackets(hdr, entry);
        Model_CollectTextures(entry, 0);
        Model_BuildPartPackets(entry, entry->partCount, entry->progInfo->pktKind, entry->pktOfs + (s32)Mem_GetAddr(entry->data), entry->mat);
    } else {
        return 0;
    }
    gModelCount++;
    return 1;
}

/*
 * Rebuilds the mesh hdr from model entry's copy of the file (Model_Create): its 36-byte
 * header, with strips and verts pointed into the copy (the strips at +0x30, the vertices
 * after them, 16-aligned).
 */
void Model_ReadHeader(ModelEntry *entry, MeshDesc *hdr)
{
    MeshDesc *base;
    u8 *data;

    data = Mem_GetAddr(entry->file);
    base = (MeshDesc *)data;
    data += 48;
    Mem_Copy(hdr, base, 36);
    hdr->strips = (u16 *)data;
    data += (base->stripsSize + 15) & ~0xF;
    hdr->verts = data;
}

extern s32 gModelPktSteps[];
extern Vu1VtxLayout gVu1ProgLayouts[];

/*
 * Walks the strips of mesh hdr to size model entry's VIF data (batches cut at the program's
 * buffer (its descriptor's bufSize) and a new DMA CNT tag every 1 MB); counts the strips' vertices,
 * triangles and material runs, marks the materials used as the model's parts, and allocates
 * the data block (the VIF data, the chain table and the part packets).
 */
void Model_SetupEntry(MeshDesc *hdr, ModelEntry *entry, s32 unused)
{
    s8 used[32];
    s32 i;
    s32 total;
    s32 off;
    Vu1ProgInfo *info;
    u16 len;
    s32 step;
    s32 vtxBytes;
    s32 limit;
    s32 lastA;
    s32 curA;
    s32 cnt;
    u32 flags;
    s32 lastB;
    s32 curB;
    s32 count;
    u16 *stream;

    total = 0;
    info = entry->progInfo;
    off = 0;
    Mem_Fill(used, 0, 32);
    count = hdr->count;
    lastA = -1;
    stream = hdr->strips;
    curA = 0;
    lastB = lastA;
    curB = 0;
    step = 0;
    cnt = 0;
    entry->stripVtxCount = 0;
    entry->triCount = 0;
    entry->matRunCount = 0;
    limit = info->bufSize + 32;
    for (i = 0; i < count; i++) {
        len = *stream & MESH_STRIP_LEN;
        entry->stripVtxCount += len;
        entry->triCount += len - 2;
        flags = hdr->flags;
        stream++;
        if (flags & MESH_STRIP_MAT) {
            curA = *stream;
            stream++;
        }
        if (flags & MESH_STRIP_SKIN) {
            curB = *stream;
            stream++;
        }
        if (flags & MESH_INDEXED) {
            stream += len;
        }
        if (lastA != curA) {
            if (cnt != 0) {
                off += step;
                cnt = 0;
                total += off;
            }
            used[curA] = 1;
            lastA = curA;
            lastB = curB;
            off = 48;
            step = 32;
            entry->matRunCount++;
        } else if (lastB != curB) {
            if (cnt != 0) {
                off += step;
                if ((u32)(off + limit) >= 0x100000) {
                    total += off;
                    off = 16;
                }
                cnt = 0;
            }
            lastB = curB;
            off += 32;
            step = 32;
        }
        if (len > 0) {
            vtxBytes = gVu1ProgLayouts[info->layout].vtxBytes;
            do {
                if ((u32)(step + vtxBytes) >= (u32)info->bufSize) {
                    off += step;
                    if ((u32)(off + limit) >= 0x100000) {
                        total += off;
                        off = 16;
                    }
                    off += 32;
                    step = 32;
                    cnt = 0;
                }
                len--;
                cnt++;
                step += vtxBytes;
            } while (len != 0);
        }
    }
    off += step;
    total += off;
    entry->vifSize = total;
    entry->partCount = 0;
    for (i = 0; i < 32; i++) {
        if (used[i] != 0) {
            entry->mat[entry->partCount++] = i;
        }
    }
    entry->chainTblSize = (entry->partCount * 4 + 15) & ~0xF;
    entry->pktSize = (entry->partCount * gModelPktSteps[info->pktKind] + 15) & ~0xF;
    entry->pktSize += 32;
    entry->chainTblOfs = (entry->vifSize + 15) & ~0xF;
    entry->pktOfs = entry->chainTblOfs + ((entry->chainTblSize + 15) & ~0xF);
    count = ((total + 15) & ~0xF);
    count += ((entry->chainTblSize + 15) & ~0xF);
    count += ((entry->pktSize + 15) & ~0xF);
    entry->data = Mem_Alloc(count, 3);
    entry->dataAddr = Mem_GetAddr(entry->data);
}

/* The GIF tag fields per program (Vu1ProgInfo.gifInfo): PRIM, REGS and NREG. */
typedef struct GifInfo {
    u32 prim;
    u32 regs;
    u32 nreg;
} GifInfo;

extern GifInfo gVu1GifInfos[];

#define GIF_TAG(nloop, prim, nreg) ((u64)(nloop) | ((u64)1 << 15) | ((u64)1 << 46) | ((u64)(prim) << 47) | ((u64)(nreg) << 60))

extern f32 gDispTexScaleV;
extern f32 gDispTexScaleU;
/*
 * Builds model e's VIF packets from its file header hdr (flPS2ConvClayData, from its debug
 * string): per material a DMA chain of VIF unpacks of the strips' vertices (position,
 * normal, colour, texture coordinates and bone weights, as the program's bits ask), each
 * batch with its GIF tag. Batches are cut where the program's buffer (its descriptor's bufSize) would
 * overflow, and a DMA CNT tag every 1 MB. offs (at data + chainTblOfs) gets each material's
 * chain.
 */
void Model_BuildVifPackets(MeshDesc *mesh, ModelEntry *entry)
{
    Vu1ProgInfo *info = entry->progInfo;
    s32 bits = gVu1ProgLayouts[info->layout].attrs;
    s32 boneIdx[4] = {0};
    s32 boneWt[4] = {0};
    u32 starts[32];
    u32 base;
    u32 unpackPos;
    u32 tag;
    s32 cnt;
    s32 lastA;
    s32 curA;
    s32 lastB;
    s32 curB;
    u32 step;
    u32 vtxBytes;
    s32 count;
    u32 raw;
    s32 *offs;
    u32 prim;
    u32 regs;
    u32 nreg;
    s32 toggle;
    u32 limit;
    s32 i;
    s32 j;
    s32 k;
    u32 out;
    u16 *stream;
    u32 *walk;
    u32 *w;
    f32 *weightOut;
    u32 *lastQw;
    MeshVtxCol *colVtx;
    MeshVtxTex *texVtx;
    MeshVtxSkin *skinVtx;

    for (i = 0; i < 32; i++) {
        starts[i] = 0;
    }
    prim = gVu1GifInfos[info->gifInfo].prim;
    regs = gVu1GifInfos[info->gifInfo].regs;
    nreg = gVu1GifInfos[info->gifInfo].nreg;
    if ((entry->renderFlags & 0x18000) == 0x8000) {
        prim |= 0x20;
    }
    base = (u32)Mem_GetAddr(entry->data);
    out = base;
    stream = mesh->strips;
    count = mesh->count;
    limit = info->bufSize + 32;
    lastA = -1;
    lastB = lastA;
    curA = 0;
    curB = 0;
    cnt = 0;
    offs = (s32 *)(base + entry->chainTblOfs);
    for (i = 0; i < count; i++) {
        raw = *stream;
        toggle = 0;
        stream++;
        if (mesh->flags & MESH_STRIP_MAT) {
            curA = *stream;
            stream++;
        }
        if (mesh->flags & MESH_STRIP_SKIN) {
            curB = *stream;
            stream++;
        }
        if (lastA != curA) {
            if (cnt != 0) {
                unpackPos = (u32)Vif_UnpackV4((u128 *)unpackPos, step >> 4, 0);
                switch (lastB) {
                case 0:
                    ((u32 *)unpackPos)[0] = 1;
                    break;
                case 1:
                    ((u32 *)unpackPos)[0] = 2;
                    break;
                }
                ((u32 *)unpackPos)[1] = ((nreg << 4) * (cnt + 2) >> 4) + 1;
                ((u64 *)unpackPos)[1] = 0;
                ((u64 *)unpackPos)[2] = GIF_TAG(cnt, prim, nreg);
                ((u32 *)unpackPos)[6] = regs;
                ((u32 *)unpackPos)[7] = 0;
                out = (u32)Vif_MsCnt((u128 *)out);
                if (tag == starts[lastA]) {
                    DmaTag_Ret((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
                    offs[lastA] = tag - base;
                } else {
                    walk = (u32 *)starts[lastA];
                    do {
                        switch (walk[0] & DMA_TAG_ID_MASK) {
                        case DMA_TAG_CNT:
                        case DMA_TAG_NEXT:
                            walk = (u32 *)((u8 *)walk + ((walk[0] & 0xFFFF) + 1) * 16);
                            break;
                        case DMA_TAG_RET:
                            DmaTag_Next((u128 *)walk, walk[0] & 0xFFFF, tag, 0, 0);
                            walk = NULL;
                            break;
                        }
                    } while (walk != NULL);
                    DmaTag_Ret((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
                }
                cnt = 0;
            }
            tag = out;
            lastA = curA;
            lastB = curB;
            if (starts[curA] == 0) {
                starts[curA] = out;
            }
            unpackPos = out + 16;
            out += 64;
            step = 32;
        } else if (lastB != curB) {
            if (cnt != 0) {
                w = (u32 *)Vif_UnpackV4((u128 *)unpackPos, step >> 4, 0);
                switch (lastB) {
                case 0:
                    w[0] = 1;
                    break;
                case 1:
                    w[0] = 2;
                    break;
                }
                w[1] = ((nreg << 4) * (cnt + 2) >> 4) + 1;
                ((u64 *)w)[1] = 0;
                ((u64 *)w)[2] = GIF_TAG(cnt, prim, nreg);
                w[6] = regs;
                w[7] = 0;
                out = (u32)Vif_MsCnt((u128 *)out);
                if (out - tag + limit >= 0x100000) {
                    DmaTag_Cnt((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
                    tag = out;
                    out += 16;
                }
                cnt = 0;
            }
            unpackPos = out;
            out += 48;
            lastB = curB;
            step = 32;
        }
        for (j = 0; j < (raw & MESH_STRIP_LEN); j++) {
            vtxBytes = gVu1ProgLayouts[info->layout].vtxBytes;
            if (step + vtxBytes >= (u32)info->bufSize) {
                w = (u32 *)Vif_UnpackV4((u128 *)unpackPos, step >> 4, 0);
                switch (lastB) {
                case 0:
                    w[0] = 1;
                    break;
                case 1:
                    w[0] = 2;
                    break;
                }
                w[1] = ((nreg << 4) * (cnt + 2) >> 4) + 1;
                ((u64 *)w)[1] = 0;
                ((u64 *)w)[2] = GIF_TAG(cnt, prim, nreg);
                w[6] = regs;
                w[7] = 0;
                out = (u32)Vif_MsCnt((u128 *)out);
                if (out - tag + limit >= 0x100000) {
                    DmaTag_Cnt((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
                    tag = out;
                    out += 16;
                }
                unpackPos = out;
                step = 32;
                out += 48;
                cnt = 0;
            }
            step += vtxBytes;
            cnt++;
            switch (mesh->format) {
            case 37:
                if (mesh->flags & MESH_INDEXED) {
                    colVtx = &((MeshVtxCol *)mesh->verts)[*stream];
                    stream++;
                } else {
                    colVtx++;
                }
                if (bits & 1) {
                    ((f32 *)out)[0] = colVtx->pos[0];
                    ((f32 *)out)[1] = colVtx->pos[1];
                    ((f32 *)out)[2] = colVtx->pos[2];
                    if (j < 2) {
                        ((u32 *)out)[3] = 32768;
                    } else {
                        ((u32 *)out)[3] = 0;
                    }
                    out += 16;
                }
                if (bits & 2) {
                    ((f32 *)out)[0] = colVtx->nrm[0];
                    ((f32 *)out)[1] = colVtx->nrm[1];
                    ((f32 *)out)[2] = colVtx->nrm[2];
                    if (toggle == 0) {
                        ((f32 *)out)[3] = 1.0f;
                    } else {
                        ((f32 *)out)[3] = -1.0f;
                    }
                    out += 16;
                }
                if (bits & 4) {
                    if (!(entry->renderFlags & 0xF00)) {
                        ((f32 *)out)[0] = (colVtx->col >> 16) & 0xFF;
                        ((f32 *)out)[1] = (colVtx->col >> 8) & 0xFF;
                        ((f32 *)out)[2] = colVtx->col & 0xFF;
                    } else {
                        ((f32 *)out)[0] = Gs_AlphaFromByte((colVtx->col >> 16) & 0xFF, 1);
                        ((f32 *)out)[1] = Gs_AlphaFromByte((colVtx->col >> 8) & 0xFF, 1);
                        ((f32 *)out)[2] = Gs_AlphaFromByte(colVtx->col & 0xFF, 1);
                    }
                    ((f32 *)out)[3] = Gs_AlphaFromByte((colVtx->col >> 24) & 0xFF, 0);
                    out += 16;
                }
                break;
            case 53:
                if (mesh->flags & MESH_INDEXED) {
                    texVtx = &((MeshVtxTex *)mesh->verts)[*stream];
                    stream++;
                } else {
                    texVtx++;
                }
                if (bits & 1) {
                    ((f32 *)out)[0] = texVtx->pos[0];
                    ((f32 *)out)[1] = texVtx->pos[1];
                    ((f32 *)out)[2] = texVtx->pos[2];
                    if (j < 2) {
                        ((u32 *)out)[3] = 32768;
                    } else {
                        ((u32 *)out)[3] = 0;
                    }
                    out += 16;
                }
                if (bits & 2) {
                    ((f32 *)out)[0] = texVtx->nrm[0];
                    ((f32 *)out)[1] = texVtx->nrm[1];
                    ((f32 *)out)[2] = texVtx->nrm[2];
                    if (toggle == 0) {
                        ((f32 *)out)[3] = 1.0f;
                    } else {
                        ((f32 *)out)[3] = -1.0f;
                    }
                    out += 16;
                }
                if (bits & 4) {
                    if (!(entry->renderFlags & 0xF00)) {
                        ((f32 *)out)[0] = Gs_AlphaFromByte((texVtx->col >> 16) & 0xFF, 0);
                        ((f32 *)out)[1] = Gs_AlphaFromByte((texVtx->col >> 8) & 0xFF, 0);
                        ((f32 *)out)[2] = Gs_AlphaFromByte(texVtx->col & 0xFF, 0);
                    } else {
                        ((f32 *)out)[0] = Gs_AlphaFromByte((texVtx->col >> 16) & 0xFF, 1);
                        ((f32 *)out)[1] = Gs_AlphaFromByte((texVtx->col >> 8) & 0xFF, 1);
                        ((f32 *)out)[2] = Gs_AlphaFromByte(texVtx->col & 0xFF, 1);
                    }
                    ((f32 *)out)[3] = Gs_AlphaFromByte((texVtx->col >> 24) & 0xFF, 0);
                    out += 16;
                }
                if (bits & 8) {
                    lastQw = (u32 *)out;
                    ((f32 *)out)[0] = texVtx->uv[0];
                    ((f32 *)out)[1] = texVtx->uv[1];
                    ((f32 *)out)[2] = 1.0f;
                    ((u32 *)out)[3] = 0;
                    out += 16;
                    if (entry->flags & MODEL_SCALE_UV) {
                        ((f32 *)lastQw)[0] = ((f32 *)lastQw)[0] * gDispTexScaleU;
                        ((f32 *)lastQw)[1] = ((f32 *)lastQw)[1] * gDispTexScaleV;
                    }
                }
                break;
            case 309:
                if (mesh->flags & MESH_INDEXED) {
                    skinVtx = &((MeshVtxSkin *)mesh->verts)[*stream];
                    stream++;
                } else {
                    skinVtx++;
                }
                if (bits & 1) {
                    ((f32 *)out)[0] = skinVtx->pos[0];
                    ((f32 *)out)[1] = skinVtx->pos[1];
                    ((f32 *)out)[2] = skinVtx->pos[2];
                    if (j < 2) {
                        ((u32 *)out)[3] = 32768;
                    } else {
                        ((u32 *)out)[3] = 0;
                    }
                    out += 16;
                }
                if (bits & 2) {
                    ((f32 *)out)[0] = skinVtx->nrm[0];
                    ((f32 *)out)[1] = skinVtx->nrm[1];
                    ((f32 *)out)[2] = skinVtx->nrm[2];
                    if (toggle == 0) {
                        ((f32 *)out)[3] = 1.0f;
                    } else {
                        ((f32 *)out)[3] = -1.0f;
                    }
                    out += 16;
                }
                if (bits & 4) {
                    if (!(entry->renderFlags & 0xF00)) {
                        ((f32 *)out)[0] = Gs_AlphaFromByte((skinVtx->col >> 16) & 0xFF, 0);
                        ((f32 *)out)[1] = Gs_AlphaFromByte((skinVtx->col >> 8) & 0xFF, 0);
                        ((f32 *)out)[2] = Gs_AlphaFromByte(skinVtx->col & 0xFF, 0);
                    } else {
                        ((f32 *)out)[0] = Gs_AlphaFromByte((skinVtx->col >> 16) & 0xFF, 1);
                        ((f32 *)out)[1] = Gs_AlphaFromByte((skinVtx->col >> 8) & 0xFF, 1);
                        ((f32 *)out)[2] = Gs_AlphaFromByte(skinVtx->col & 0xFF, 1);
                    }
                    ((f32 *)out)[3] = Gs_AlphaFromByte((skinVtx->col >> 24) & 0xFF, 0);
                    out += 16;
                }
                if (bits & 8) {
                    lastQw = (u32 *)out;
                    ((f32 *)out)[0] = skinVtx->uv[0];
                    ((f32 *)out)[1] = skinVtx->uv[1];
                    ((f32 *)out)[2] = 1.0f;
                    ((u32 *)out)[3] = 0;
                    out += 16;
                    if (entry->flags & MODEL_SCALE_UV) {
                        ((f32 *)lastQw)[0] = ((f32 *)lastQw)[0] * gDispTexScaleU;
                        ((f32 *)lastQw)[1] = ((f32 *)lastQw)[1] * gDispTexScaleV;
                    }
                }
                if ((bits & 0x10) && (bits & 0x20)) {
                    lastQw = (u32 *)out;
                    weightOut = (f32 *)(out + 16);
                    out += 32;
                    boneIdx[0] = (skinVtx->bones >> 24) & 0xFF;
                    boneIdx[1] = (skinVtx->bones >> 16) & 0xFF;
                    boneIdx[2] = (skinVtx->bones >> 8) & 0xFF;
                    boneIdx[3] = skinVtx->bones & 0xFF;
                    boneWt[0] = (skinVtx->weights >> 24) & 0xFF;
                    boneWt[1] = (skinVtx->weights >> 16) & 0xFF;
                    boneWt[2] = (skinVtx->weights >> 8) & 0xFF;
                    boneWt[3] = skinVtx->weights & 0xFF;
                    *(u128 *)lastQw = 0;
                    *(u128 *)weightOut = 0;
                    switch (lastB) {
                    case 0:
                        lastQw[0] = (u32)&(info->bones)->base[boneIdx[0]];
                        entry->boneMask |= 1 << boneIdx[0];
                        lastQw[1] = (u32)&(info->bones)->base[(info->bones)->off] + boneIdx[0] * (info->bones)->size;
                        weightOut[0] = 100.0f;
                        break;
                    case 1:
                        for (k = 0; k < 4; k++) {
                            lastQw[k] = (u32)&(info->bones)->base[boneIdx[k]];
                            entry->boneMask |= 1 << boneIdx[k];
                            weightOut[k] = boneWt[k] / 255.0f;
                        }
                        break;
                    }
                }
                break;
            }
            toggle ^= 1;
        }
    }
    if (cnt != 0) {
        w = (u32 *)Vif_UnpackV4((u128 *)unpackPos, step >> 4, 0);
        switch (lastB) {
        case 0:
            w[0] = 1;
            break;
        case 1:
            w[0] = 2;
            break;
        }
        w[1] = ((nreg << 4) * (cnt + 2) >> 4) + 1;
        ((u64 *)w)[1] = 0;
        ((u64 *)w)[2] = GIF_TAG(cnt, prim, nreg);
        w[6] = regs;
        w[7] = 0;
        out = (u32)Vif_MsCnt((u128 *)out);
    }
    if (tag == starts[lastA]) {
        DmaTag_Ret((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
        offs[lastA] = tag - base;
    } else {
        walk = (u32 *)starts[lastA];
        do {
            switch (walk[0] & DMA_TAG_ID_MASK) {
            case DMA_TAG_CNT:
            case DMA_TAG_NEXT:
                walk = (u32 *)((u8 *)walk + ((walk[0] & 0xFFFF) + 1) * 16);
                break;
            case DMA_TAG_RET:
                DmaTag_Next((u128 *)walk, walk[0] & 0xFFFF, tag, 0, 0);
                walk = NULL;
                break;
            }
        } while (walk != NULL);
        DmaTag_Ret((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
    }
    if (entry->vifSize != out - base) {
        Debug_Printf("A = %x B = %x, %x @flPS2ConvClayData", base, out, entry->vifSize);
    }
}

/* A 0..255 alpha as the GS's 0..128 (mode 0) or as 0..1 (mode 1). */
f32 Gs_AlphaFromByte(u32 byteVal, s32 mode)
{
    f32 result = 0.0f;

    if (mode == 0) {
        if (byteVal == 255) {
            result = 128.0f;
        } else if (byteVal != 0) {
            byteVal >>= 1;
            if (byteVal == 0) {
                byteVal = 1;
            }
            result = byteVal;
        }
    } else {
        result = byteVal / 255.0f;
        if (result > 1.0f) {
            result = 1.0f;
        }
        if (result < 0.0f) {
            result = 0.0f;
        }
    }
    return result;
}

/* The DMA packets of the n parts mat[] of model e (Model_BuildPartPacket, a step of gModelPktSteps[prog]
 * bytes each) from dst, closed by an END tag. */
void Model_BuildPartPackets(ModelEntry *entry, s32 n, s32 prog, s32 dst, u8 *mat)
{
    s32 i;
    s32 step;
    s32 *parts;
    u8 *base;

    step = gModelPktSteps[prog];
    base = entry->dataAddr;
    parts = (s32 *)(base + entry->chainTblOfs);
    for (i = 0; i < n; i++) {
        Model_BuildPartPacket(base + parts[mat[i]], (u32 *)dst, prog, mat[i]);
        dst += step;
    }
    DmaTag_End((u128 *)dst, 1, 1, 0);
    ((u32 *)dst)[2] = 0;
    ((u32 *)dst)[3] = 0;
    ((u32 *)dst)[4] = 0;
    ((u32 *)dst)[5] = 0;
    ((u32 *)dst)[6] = 0;
    ((u32 *)dst)[7] = 0;
}

void Gs_SetTexRegs2(u32 tex, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2,
                   u32 tex2, u64 *tex1b, u64 *tex0b, u64 *clampb, u64 *mip1b, u64 *mip2b);

extern u32 D_0020E134;
/* The DMA packet of one model part at dst for VU1 program kind prog: a CALL of the part's
 * data, an UNPACK of the header, a GIF tag, the texture registers of material mat
 * (Gs_SetTexRegs, or fixed ones for kinds 1 and 4), the material's colours and an MSCNT. */
void Model_BuildPartPacket(u8 *part, u32 *dst, s32 prog, s32 mat)
{
    ModelMat *material = &gRsMaterials[mat];
    u8 *pkt;
    s32 lod;
    s32 isPow2;

    switch (prog) {
    case 0:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 12, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 16), 10, 0);
        *(u128 *)(pkt + 32) = 0;
        *(u64 *)(pkt + 48) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 56) = 14;
        *(u32 *)(pkt + 60) = 0;
        Gs_SetTexRegs(material->tex, (u64 *)(pkt + 64), (u64 *)(pkt + 80), (u64 *)(pkt + 96),
                      (u64 *)(pkt + 112), (u64 *)(pkt + 128), (u64 *)(pkt + 144));
        *(f32 *)(pkt + 160) = material->rgb[0];
        *(f32 *)(pkt + 164) = material->rgb[1];
        *(f32 *)(pkt + 168) = material->rgb[2];
        *(f32 *)(pkt + 172) = 128.0f * material->alpha;
        *(f32 *)(pkt + 176) = material->unk24[0];
        *(f32 *)(pkt + 180) = material->unk24[1];
        *(f32 *)(pkt + 184) = material->unk24[2];
        *(f32 *)(pkt + 188) = material->unk24[3];
        Vif_MsCnt((u128 *)(pkt + 192));
        break;
    case 1:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 12, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 16), 10, 0);
        *(u128 *)(pkt + 32) = 0;
        *(u64 *)(pkt + 48) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 56) = 14;
        *(u32 *)(pkt + 60) = 0;
        *(u64 *)(pkt + 64) = 0;
        *(u64 *)(pkt + 72) = 59;
        *(u64 *)(pkt + 80) = 0;
        *(u64 *)(pkt + 88) = 20;
        *(u64 *)(pkt + 96) = 0;
        *(u64 *)(pkt + 104) = 6;
        *(u64 *)(pkt + 112) = 0;
        *(u64 *)(pkt + 120) = 8;
        *(u64 *)(pkt + 128) = 0;
        *(u64 *)(pkt + 136) = 52;
        *(u64 *)(pkt + 144) = 0;
        *(u64 *)(pkt + 152) = 54;
        *(f32 *)(pkt + 160) = material->rgb[0];
        *(f32 *)(pkt + 164) = material->rgb[1];
        *(f32 *)(pkt + 168) = material->rgb[2];
        *(f32 *)(pkt + 172) = 128.0f * material->alpha;
        *(f32 *)(pkt + 176) = material->unk24[0];
        *(f32 *)(pkt + 180) = material->unk24[1];
        *(f32 *)(pkt + 184) = material->unk24[2];
        *(f32 *)(pkt + 188) = material->unk24[3];
        Vif_MsCnt((u128 *)(pkt + 192));
        break;
    case 2:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 14, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 16), 12, 0);
        *(u128 *)(pkt + 32) = 0;
        *(u64 *)(pkt + 48) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 56) = 14;
        *(u32 *)(pkt + 60) = 0;
        Gs_SetTexRegs(material->tex, (u64 *)(pkt + 64), (u64 *)(pkt + 80), (u64 *)(pkt + 96),
                      (u64 *)(pkt + 112), (u64 *)(pkt + 128), (u64 *)(pkt + 144));
        *(f32 *)(pkt + 160) = material->rgb[0];
        *(f32 *)(pkt + 164) = material->rgb[1];
        *(f32 *)(pkt + 168) = material->rgb[2];
        *(f32 *)(pkt + 172) = 128.0f * material->alpha;
        *(f32 *)(pkt + 176) = material->unk24[0];
        *(f32 *)(pkt + 180) = material->unk24[1];
        *(f32 *)(pkt + 184) = material->unk24[2];
        *(f32 *)(pkt + 188) = material->unk24[3];
        *(f32 *)(pkt + 192) = material->unk14[0];
        *(f32 *)(pkt + 196) = material->unk14[1];
        *(f32 *)(pkt + 200) = material->unk14[2];
        *(f32 *)(pkt + 204) = material->unk14[3];
        switch (material->size) {
        case 0:
        case 1:
        case 2:
            isPow2 = lod = 1;
            break;
        case 4:
            lod = 2;
            isPow2 = 1;
            break;
        case 8:
            lod = 3;
            isPow2 = 1;
            break;
        case 16:
            lod = 4;
            isPow2 = 1;
            break;
        case 32:
            lod = 5;
            isPow2 = 1;
            break;
        case 64:
            lod = 6;
            isPow2 = 1;
            break;
        case 128:
            lod = 7;
            isPow2 = 1;
            break;
        case 256:
            lod = 8;
            isPow2 = 1;
            break;
        default:
            lod = material->size - 1;
            isPow2 = 0;
            break;
        }
        *(s32 *)(pkt + 208) = lod;
        *(s32 *)(pkt + 212) = isPow2;
        *(s32 *)(pkt + 216) = 0;
        *(s32 *)(pkt + 220) = 0;
        Vif_MsCnt((u128 *)(pkt + 224));
        break;
    case 3:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 10, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 16), 8, 0);
        *(u128 *)(pkt + 32) = 0;
        *(u64 *)(pkt + 48) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 56) = 14;
        *(u32 *)(pkt + 60) = 0;
        Gs_SetTexRegs(material->tex, (u64 *)(pkt + 64), (u64 *)(pkt + 80), (u64 *)(pkt + 96),
                      (u64 *)(pkt + 112), (u64 *)(pkt + 128), (u64 *)(pkt + 144));
        Vif_MsCnt((u128 *)(pkt + 160));
        break;
    case 4:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 11, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 16), 9, 0);
        *(u128 *)(pkt + 32) = 0;
        *(u64 *)(pkt + 48) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 56) = 14;
        *(u32 *)(pkt + 60) = 0;
        *(u64 *)(pkt + 64) = 0;
        *(u64 *)(pkt + 72) = 59;
        *(u64 *)(pkt + 80) = 0;
        *(u64 *)(pkt + 88) = 20;
        *(u64 *)(pkt + 96) = 0;
        *(u64 *)(pkt + 104) = 6;
        *(u64 *)(pkt + 112) = 0;
        *(u64 *)(pkt + 120) = 8;
        *(u64 *)(pkt + 128) = 0;
        *(u64 *)(pkt + 136) = 52;
        *(u64 *)(pkt + 144) = 0;
        *(u64 *)(pkt + 152) = 54;
        *(u128 *)(pkt + 160) = 0;
        Vif_MsCnt((u128 *)(pkt + 176));
        break;
    case 5:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 16, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 16), 14, 0);
        *(u128 *)(pkt + 32) = 0;
        *(u64 *)(pkt + 48) = 0x800B | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 56) = 14;
        *(u32 *)(pkt + 60) = 0;
        Gs_SetTexRegs2(material->tex, (u64 *)(pkt + 64), (u64 *)(pkt + 80), (u64 *)(pkt + 96),
                       (u64 *)(pkt + 112), (u64 *)(pkt + 128), (u64 *)(pkt + 144),
                       D_0020E134, (u64 *)(pkt + 160), (u64 *)(pkt + 176),
                       (u64 *)(pkt + 192), (u64 *)(pkt + 208), (u64 *)(pkt + 224));
        *(f32 *)(pkt + 240) = material->rgb[0];
        *(f32 *)(pkt + 244) = material->rgb[1];
        *(f32 *)(pkt + 248) = material->rgb[2];
        *(f32 *)(pkt + 252) = 128.0f * material->alpha;
        Vif_MsCnt((u128 *)(pkt + 256));
        break;
    }
}

void Gs_SetTexRegs2(u32 tex, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2,
                   u32 tex2, u64 *tex1b, u64 *tex0b, u64 *clampb, u64 *mip1b, u64 *mip2b);

/* The GS texture registers in the packets of n parts from pkt (textures tex[]), for VU1
 * program kinds 0, 2, 3 (one context) and 5 (two). */
void Model_SetTexRegs(s32 n, s32 prog, u8 *pkt, s32 *tex)
{
    s32 step = gModelPktSteps[prog];
    s32 i;

    switch (prog) {
    case 0:
        for (i = 0; i < n; i++) {
            Gs_SetTexRegs(*tex, (u64 *)(pkt + 64), (u64 *)(pkt + 80), (u64 *)(pkt + 96),
                          (u64 *)(pkt + 112), (u64 *)(pkt + 128), (u64 *)(pkt + 144));
            pkt += step;
            tex++;
        }
        break;
    case 2:
        for (i = 0; i < n; i++) {
            Gs_SetTexRegs(*tex, (u64 *)(pkt + 64), (u64 *)(pkt + 80), (u64 *)(pkt + 96),
                          (u64 *)(pkt + 112), (u64 *)(pkt + 128), (u64 *)(pkt + 144));
            pkt += step;
            tex++;
        }
        break;
    case 3:
        for (i = 0; i < n; i++) {
            Gs_SetTexRegs(*tex, (u64 *)(pkt + 64), (u64 *)(pkt + 80), (u64 *)(pkt + 96),
                          (u64 *)(pkt + 112), (u64 *)(pkt + 128), (u64 *)(pkt + 144));
            pkt += step;
            tex++;
        }
        break;
    case 5:
        for (i = 0; i < n; i++) {
            Gs_SetTexRegs2(*tex, (u64 *)(pkt + 64), (u64 *)(pkt + 80), (u64 *)(pkt + 96),
                          (u64 *)(pkt + 112), (u64 *)(pkt + 128), (u64 *)(pkt + 144),
                          D_0020E134, (u64 *)(pkt + 160), (u64 *)(pkt + 176),
                          (u64 *)(pkt + 192), (u64 *)(pkt + 208), (u64 *)(pkt + 224));
            pkt += step;
            tex++;
        }
        break;
    case 1:
    case 4:
        break;
    }
}

extern u32 gRenderFlags;
/* The GS texture registers (A+D pairs: TEXA, TEX1_1, TEX0_1, CLAMP_1, MIPTBP1_1, MIPTBP2_1)
 * for texture tex (Gs_BuildTexRegs, main), or cleared when tex is 0. */
void Gs_SetTexRegs(u32 tex, u64 *texa, u64 *tex1Reg, u64 *tex0Reg, u64 *clamp, u64 *miptbp1Reg, u64 *miptbp2Reg)
{
    if (tex != 0) {
        Gs_BuildTexRegs(tex, texa, tex1Reg, tex0Reg, clamp, miptbp1Reg, miptbp2Reg, gRenderFlags);
        texa[1] = GS_REG_TEXA;
        tex1Reg[1] = GS_REG_TEX1_1;
        tex0Reg[1] = GS_REG_TEX0_1;
        clamp[1] = GS_REG_CLAMP_1;
        miptbp1Reg[1] = GS_REG_MIPTBP1_1;
        miptbp2Reg[1] = GS_REG_MIPTBP2_1;
    } else {
        texa[0] = 0;
        texa[1] = GS_REG_TEXA;
        tex1Reg[0] = 0;
        tex1Reg[1] = GS_REG_TEX1_1;
        tex0Reg[0] = 0;
        tex0Reg[1] = GS_REG_TEX0_1;
        clamp[0] = 0;
        clamp[1] = GS_REG_CLAMP_1;
        miptbp1Reg[0] = 0;
        miptbp1Reg[1] = GS_REG_MIPTBP1_1;
        miptbp2Reg[0] = 0;
        miptbp2Reg[1] = GS_REG_MIPTBP2_1;
    }
}

/* As Gs_SetTexRegs for both drawing contexts: tex for context 1, tex2 for context 2 (whose
 * MIPTBP registers get GS_REG_MIPTBP1_1 and GS_REG_MIPTBP2_1 when it has a texture, GS_REG_MIPTBP1_2 and GS_REG_MIPTBP2_2 when not). */
void Gs_SetTexRegs2(u32 tex, u64 *texa, u64 *tex1Reg, u64 *tex0Reg, u64 *clamp, u64 *miptbp1Reg, u64 *miptbp2Reg,
                   u32 tex2, u64 *tex1Reg2, u64 *tex0Reg2, u64 *clamp2, u64 *miptbp1Reg2, u64 *miptbp2Reg2)
{
    if (tex != 0) {
        Gs_BuildTexRegs(tex, texa, tex1Reg, tex0Reg, clamp, miptbp1Reg, miptbp2Reg, gRenderFlags);
        texa[1] = GS_REG_TEXA;
        tex1Reg[1] = GS_REG_TEX1_1;
        tex0Reg[1] = GS_REG_TEX0_1;
        clamp[1] = GS_REG_CLAMP_1;
        miptbp1Reg[1] = GS_REG_MIPTBP1_1;
        miptbp2Reg[1] = GS_REG_MIPTBP2_1;
    } else {
        texa[0] = 0;
        texa[1] = GS_REG_TEXA;
        tex1Reg[0] = 0;
        tex1Reg[1] = GS_REG_TEX1_1;
        tex0Reg[0] = 0;
        tex0Reg[1] = GS_REG_TEX0_1;
        clamp[0] = 0;
        clamp[1] = GS_REG_CLAMP_1;
        miptbp1Reg[0] = 0;
        miptbp1Reg[1] = GS_REG_MIPTBP1_1;
        miptbp2Reg[0] = 0;
        miptbp2Reg[1] = GS_REG_MIPTBP2_1;
    }
    if (tex2 != 0) {
        Gs_BuildTexRegs(tex2, texa, tex1Reg2, tex0Reg2, clamp2, miptbp1Reg2, miptbp2Reg2, gRenderFlags);
        texa[1] = GS_REG_TEXA;
        tex1Reg2[1] = GS_REG_TEX1_2;
        tex0Reg2[1] = GS_REG_TEX0_2;
        clamp2[1] = GS_REG_CLAMP_2;
        miptbp1Reg2[1] = GS_REG_MIPTBP1_1;
        miptbp2Reg2[1] = GS_REG_MIPTBP2_1;
    } else {
        texa[0] = 0;
        texa[1] = GS_REG_TEXA;
        tex1Reg2[0] = 0;
        tex1Reg2[1] = GS_REG_TEX1_2;
        tex0Reg2[0] = 0;
        tex0Reg2[1] = GS_REG_TEX0_2;
        clamp2[0] = 0;
        clamp2[1] = GS_REG_CLAMP_2;
        miptbp1Reg2[0] = 0;
        miptbp1Reg2[1] = GS_REG_MIPTBP1_2;
        miptbp2Reg2[0] = 0;
        miptbp2Reg2[1] = GS_REG_MIPTBP2_2;
    }
}

/* The textures of e: partTex[i] = the texture of part i's material, tex[] the distinct ones. */
static void Model_CollectTextures(ModelEntry *entry, s32 unused)
{
    s32 i;
    s32 j;
    ModelMat *material;

    entry->texCount = 0;
    for (i = 0; i < entry->partCount; i++) {
        material = &gRsMaterials[entry->mat[i]];
        entry->partTex[i] = 0;
        if (material->tex != 0) {
            entry->partTex[i] = material->tex;
            for (j = 0; j < entry->texCount; j++) {
                if (entry->tex[j] == material->tex) {
                    goto next;
                }
            }
            entry->tex[entry->texCount++] = material->tex;
        }
    next:;
    }
}

/* Model_Relocate for every model in use that has its data. */
void Model_RelocateAll(void)
{
    s32 i;
    ModelEntry *entry;

    i = 0;
    entry = gModelPool;
    do {
        if (entry->used != 0 && entry->data != NULL) {
            Model_Relocate(entry);
        }
        i++;
        entry++;
    } while (i < 384);
}

/*
 * After model e's data moved (its handle compacted): the new address, each part's DMA
 * chain start, and the NEXT tags inside the chains relocated.
 */
void Model_Relocate(ModelEntry *entry)
{
    u8 *old = entry->dataAddr;
    u8 *base = Mem_GetAddr(entry->data);
    u32 i;
    s32 *ofs;
    u8 *partPkt;
    u8 *tag;
    u32 w;
    u8 *walk;

    entry->dataAddr = base;
    ofs = (s32 *)(base + entry->chainTblOfs);
    partPkt = base + entry->pktOfs;
    for (i = 0; i < entry->partCount; i++, ofs++) {
        tag = base + *ofs;
        *(u8 **)(partPkt + 4) = tag;
        walk = tag;
        partPkt += (*(u16 *)partPkt + 1) * 16;
        do {
            w = *(u32 *)walk;
            switch (w & DMA_TAG_ID_MASK) {
            case DMA_TAG_CNT:
                walk += ((w & 0xFFFF) + 1) * 16;
                break;
            case DMA_TAG_NEXT:
                walk = *(u8 **)(walk + 4) = *(u8 **)(walk + 4) - (s32)old + (s32)base;
                break;
            case DMA_TAG_RET:
                tag = NULL;
                break;
            }
        } while (tag != NULL);
    }
}

void Mem_Compact(void);
void Mem_CopyQwords(void *src, void *dst, u32 qwc);

/*
 * Draws model h: sets its data up again when the frame changed, builds or copies its
 * packets, makes its textures resident, runs its VU1 program's packet builder
 * (the descriptor's build) and queues the chains (with a CALL of the program when it changed).
 */
s32 Model_Draw(u32 handle, s32 flags)
{
    ModelEntry *entry = &gModelPool[handle - 1];
    MeshDesc hdr;
    u8 *top;
    u32 *progCall;
    u32 *last;
    u8 *end;
    u8 *pkt;
    u8 *data = entry->dataAddr;

    if (gSysWork.noDraw != 0) {
        return 1;
    }
    if (!(entry->flags & MODEL_NO_COPY)) {
        if (entry->renderFlags != gRenderFlags2) {
            Dma_FlushQueue();
            entry->renderFlags = gRenderFlags2;
            if (entry->data != NULL) {
                Mem_Free(entry->data);
            }
            Model_ReadHeader(entry, &hdr);
            if (Model_Setup(entry, &hdr) == 0) {
                return 0;
            }
            Mem_Compact();
            pkt = entry->dataAddr + entry->pktOfs;
        } else if (entry->flags & MODEL_PREBUILT) {
            pkt = data + entry->pktOfs;
        } else {
            pkt = Packet_Alloc(entry->pktSize, 16);
            Model_BuildPartPackets(entry, entry->partCount, entry->progInfo->pktKind, (s32)pkt, entry->mat);
            Model_CollectTextures(entry, 0);
        }
        if (entry->texCount != 0) {
            Tex_MakeResident(entry->texCount, (u32 *)entry->tex);
            Model_SetTexRegs(entry->partCount, entry->progInfo->pktKind, pkt, entry->partTex);
        }
    } else if (entry->flags & MODEL_PREBUILT) {
        pkt = Packet_Alloc(entry->pktSize, 16);
        Mem_CopyQwords(data + entry->pktOfs, pkt, (u32)entry->pktSize >> 4);
        if (entry->texCount != 0) {
            Tex_MakeResident(entry->texCount, (u32 *)entry->tex);
            Model_SetTexRegs(entry->partCount, entry->progInfo->pktKind, pkt, entry->partTex);
        }
    } else {
        pkt = Packet_Alloc(entry->pktSize, 16);
        Model_CollectTextures(entry, 0);
        if (entry->texCount != 0) {
            Tex_MakeResident(entry->texCount, (u32 *)entry->tex);
        }
        Model_BuildPartPackets(entry, entry->partCount, entry->progInfo->pktKind, (s32)pkt, entry->mat);
    }
    end = pkt + entry->partCount * gModelPktSteps[entry->progInfo->pktKind];
    gSysWork.vu1Size = (u32)(entry->progInfo->codeEnd - entry->progInfo->codeStart) >> 3;
    top = entry->progInfo->build(entry, (u32)(entry->progInfo->start - entry->progInfo->codeStart) >> 3, pkt);
    if (gSysWork.vu1Prog != entry->prog) {
        gSysWork.vu1Prog = entry->prog;
        progCall = Packet_Alloc(64, 16);
        progCall[0] = DMA_TAG_CALL | 1;
        progCall[1] = entry->progInfo->upload;
        *(u64 *)&progCall[2] = 0;
        last = progCall + 8;
        last[0] = DMA_TAG_IRQ | DMA_TAG_END | 1;
        last[1] = 0;
        *(u64 *)&last[2] = 0;
        *(u128 *)&progCall[12] = 0;
        *(u128 *)&progCall[4] = 0;
        DmaQueue_AddChain(0, (u32)progCall & 0x0FFFFFFF, last, &gDmaQueue);
    }
    DmaQueue_AddChain(0, ((u32)top & 0x0FFFFFFF) | 0x50000000, (u32 *)end, &gDmaQueue);
    return 1;
}

/* Frees model h (its handles and pool entry); 0 when h is not a model in use. */
s32 Model_FreeHandle(u32 handle)
{
    ModelEntry *entry = &gModelPool[handle - 1];

    if (handle == 0) {
        return 0;
    }
    if (handle > 384) {
        return 0;
    }
    if (entry->used == 0) {
        return 0;
    }
    Dma_FlushQueue();
    entry->used = 0;
    if (entry->file != NULL) {
        Mem_Free(entry->file);
    }
    if (entry->data != NULL) {
        Mem_Free(entry->data);
    }
    Mem_Fill(entry, 0, sizeof(ModelEntry));
    gModelCount--;
    return 1;
}

/* This file's initialised data. */
s32 gModelPktSteps[8] = {
#include "sub_main/004024B0.inc"
};
Vu1VtxLayout gVu1ProgLayouts[12] = {
#include "sub_main/004024D0.inc"
};
GifInfo gVu1GifInfos[2] = {
#include "sub_main/00402530.inc"
};
