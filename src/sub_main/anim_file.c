/*
 * The motion file reader: AnimFile_* read a motion file's header and
 * chunks (AnimFileHeader), and Anim_LoadSet and Anim_ConvertChunk convert
 * it into an AnimSet: per bone an AnimHeader with its curves and their
 * keys (Anim_ConvertKeys*, one per key form), in the forms the Anim_*
 * evaluators read. Anim_GetStartTime and Anim_GetEndTime give a curve's
 * time range, and AnimFile_Count a motion file's entry count.
 */

#include "common.h"
#include "model.h"
#include "motion.h"
#include "heap.h"
#include "anim_file.h"
#include "engine_util.h"

extern s32 gAnimCurveBuf;          /* the base of the curves' keys */
extern LinearHeap gAnimKeyHeap;   /* the heap of the keys */
/* The 16-bit keys as the motion file stores them: KeyS2, KeyS4 and KeySM
 * (motion.h) read as unsigned halves. Anim_ConvertKeysS2/S4/SM copy them
 * through these, which the match needs: a signed half loads with lh, these
 * copies load with lhu. */
typedef struct FileKeyS2 { u16 v, t; } FileKeyS2;

typedef struct FileKeyS4 { u16 v, t, in, out; } FileKeyS4;
typedef struct FileKeySM { u32 mode; u16 v, t, in, out; } FileKeySM;
/* The motion file's header: the file itself. */
static void *AnimFile_Header(void *file)
{
    return file;
}

/* Chunk n of motion file file (the chunks follow the header, each `size` bytes long).
 * cur and chunk walk the chunks together: the compiler keeps the two in separate
 * registers, as the game's code does. */
u8 *AnimFile_GetChunk(void *file, s32 n)
{
    s32 i;
    u8 *header;
    u8 *cur;
    AnimFileChunk *chunk;

    header = AnimFile_Header(file);
    cur = header + sizeof(AnimFileHeader);
    chunk = (AnimFileChunk *)(header + sizeof(AnimFileHeader));
    for (i = 0; i < n; i++) {
        cur += chunk->size;
        chunk = (AnimFileChunk *)cur;
    }
    return (u8 *)chunk;
}

/* 1 and *loopTime = the file's loop time when it gives one, else 0 and *loopTime = 0. */
s32 AnimFile_GetLoopTime(void *file, f32 *loopTime)
{
    AnimFileHeader *header = AnimFile_Header(file);
    s32 loops;

    if (header->loops != 0) {
        loops = 1;
        *loopTime = header->loopTime;
    } else {
        *loopTime = 0.0f;
        loops = 0;
    }
    return loops;
}

/* The bytes motion file file takes once converted (Anim_LoadSet): the set, the list of
 * its animations, and per chunk an AnimHeader, its curves and their keys. */
s32 Anim_ConvertedSize(void *file)
{
    s32 i;
    s32 count;
    s32 size;
    AnimFileChunk *chunk;

    count = AnimFile_Count(file);
    size = count * sizeof(AnimHeader *) + sizeof(AnimSet);

    for (i = 0; i < count; i++) {
        chunk = (AnimFileChunk *)AnimFile_GetChunk(file, i);
        size += sizeof(AnimHeader);
        size += chunk->count * sizeof(AnimCurve);
        size += Anim_ChunkKeySize(chunk);
    }
    return size;
}

/* The bytes the keys of chunk's curve records take once converted (Anim_ConvertKeys). */
s32 Anim_ChunkKeySize(AnimFileChunk *chunk)
{
    AnimFileCurve *rec;
    s32 count;
    s32 size;
    s32 i;

    count = chunk->count;
    rec = (AnimFileCurve *)(chunk + 1);
    size = 0;

    for (i = 0; i < count; i++) {
        switch (ANIM_FILE_KEY_FORMAT(rec->info)) {
        case ANIM_KEY_F32_LINEAR:
            size += rec->count * sizeof(KeyF2);
            break;
        case ANIM_KEY_F32_CUBIC:
            size += rec->count * sizeof(KeyF4);
            break;
        case ANIM_KEY_F32_MIXED:
            size += rec->count * sizeof(KeyFM);
            break;
        case ANIM_KEY_S16_LINEAR:
            size += rec->count * sizeof(KeyS2);
            break;
        case ANIM_KEY_S16_CUBIC:
            size += rec->count * sizeof(KeyS4);
            break;
        case ANIM_KEY_S16_MIXED:
            size += rec->count * sizeof(KeySM);
            break;
        }
        rec = (AnimFileCurve *)((u8 *)rec + rec->size);
    }
    return size;
}

/*
 * Converts motion file file into buf: the set first, then the list of its animations,
 * one AnimHeader per chunk with its curves and keys, all from the heap gAnimKeyHeap over
 * buf. The set's start and end are the earliest and latest keys of all the curves. The
 * offsets in the set are made relative to buf (gAnimCurveBuf), and *out gets a copy of
 * the set with base = buf. Returns 1.
 */
s32 Anim_LoadSet(AnimSet *out, u8 *buf, void *file)
{
    s32 size;
    s32 i;
    s32 count;
    AnimSet *set;
    AnimHeader **list;
    f32 min;
    f32 max;
    f32 time;
    f32 loopTime;

    size = Anim_ConvertedSize(file);
    gAnimCurveBuf = (s32)buf;
    Anim_SetKeyBase(buf);
    LinearHeap_Init(&gAnimKeyHeap, buf, size, 1);
    set = LinearHeap_Alloc(&gAnimKeyHeap, sizeof(AnimSet), 0);
    count = AnimFile_Count(file);
    set->flags = 0;
    set->count = count;
    set->anims = (s32)LinearHeap_Alloc(&gAnimKeyHeap, count * sizeof(AnimHeader *), 0);
    max = 0.0f;
    set->base = 0;
    min = 1e10f;
    set->size = size;
    list = (AnimHeader **)set->anims;
    for (i = 0; i < count; i++, list++) {
        *list = LinearHeap_Alloc(&gAnimKeyHeap, sizeof(AnimHeader), 0);
        Anim_LoadChunk(*list, file, i);
        if ((time = Anim_GetEndTime(*list)) > max) {
            max = time;
        }
        if ((time = Anim_GetStartTime(*list)) < min) {
            min = time;
        }
    }
    set->start = min;
    set->end = max;
    if (AnimFile_GetLoopTime(file, &loopTime)) {
        set->flags |= ANIM_SET_LOOPS;
        set->loop = loopTime;
    } else {
        set->flags &= ~ANIM_SET_LOOPS;
        set->loop = 0.0f;
    }
    for (i = 0; i < count; i++) {
        ((s32 *)set->anims)[i] -= gAnimCurveBuf;
    }
    set->anims -= gAnimCurveBuf;
    *out = *set;
    out->base = (s32)buf;
    return 1;
}

/* Fills animation anim from chunk i of motion file file, its type from the file's
 * header; nothing when there is no such chunk. */
void Anim_LoadChunk(AnimHeader *anim, void *file, s32 i)
{
    u8 *chunk;
    AnimFileHeader *header;

    if (AnimFile_Count(file) < i) {
        return;
    }
    if ((chunk = AnimFile_GetChunk(file, i)) == NULL) {
        return;
    }
    header = AnimFile_Header(file);
    if (header == NULL) {
        return;
    }
    switch (header->type) {
    case ANIM_FILE_FLOAT:
        anim->flags = ANIM_TYPE_FLOAT;
        break;
    case ANIM_FILE_FIXED:
        anim->flags = ANIM_TYPE_FIXED;
        break;
    }
    switch (anim->flags & ANIM_TYPE_MASK) {
    case ANIM_TYPE_FLOAT:
    case ANIM_TYPE_FIXED:
        Anim_ConvertChunk(file, anim, (AnimFileChunk *)chunk, 0);
        break;
    }
}

/* Fills animation anim from chunk of motion file file: one AnimCurve per curve record,
 * the used ones (ANIM_FILE_CURVE_USED) converted (Anim_ConvertKeys), each driving the
 * pose value n given by its record's bit 1 << n. The others keep no keys. */
void Anim_ConvertChunk(void *file, AnimHeader *anim, AnimFileChunk *chunk, s32 unused)
{
    AnimCurve *curve;
    AnimFileCurve *rec;
    s32 count;
    s32 i;
    u32 info;

    count = chunk->count;
    anim->count = count;
    curve = LinearHeap_Alloc(&gAnimKeyHeap, count * sizeof(AnimCurve), 0);
    anim->curves = (s32)curve;
    for (i = 0; i < count; i++) {
        curve[i].type = ANIM_KEY_NONE;
        curve[i].count = 0;
    }
    rec = (AnimFileCurve *)(chunk + 1);
    for (i = 0; i < count; i++, curve++) {
        info = rec->info;
        if (info & ANIM_FILE_CURVE_USED) {
            Anim_ConvertKeys(rec, curve);
            switch (info & ANIM_FILE_POSE_BITS) {
            case 1 << 0:
                curve->idx = 0;
                break;
            case 1 << 1:
                curve->idx = 1;
                break;
            case 1 << 2:
                curve->idx = 2;
                break;
            case 1 << 3:
                curve->idx = 3;
                break;
            case 1 << 4:
                curve->idx = 4;
                break;
            case 1 << 5:
                curve->idx = 5;
                break;
            case 1 << 6:
                curve->idx = 6;
                break;
            case 1 << 7:
                curve->idx = 7;
                break;
            case 1 << 8:
                curve->idx = 8;
                break;
            }
        }
        rec = (AnimFileCurve *)((u8 *)rec + rec->size);
    }
    anim->curves -= gAnimCurveBuf;
}

/* Converts curve record rec's keys into curve, by the record's key format. */
void Anim_ConvertKeys(AnimFileCurve *rec, AnimCurve *curve)
{
    u32 format;

    format = ANIM_FILE_KEY_FORMAT((u32)rec->info);
    switch (format) {
    case ANIM_KEY_F32_LINEAR:
        Anim_ConvertKeysF2(rec, curve);
        break;
    case ANIM_KEY_F32_CUBIC:
        Anim_ConvertKeysF4(rec, curve);
        break;
    case ANIM_KEY_F32_MIXED:
        Anim_ConvertKeysFM(rec, curve);
        break;
    case ANIM_KEY_S16_LINEAR:
        Anim_ConvertKeysS2(rec, curve);
        break;
    case ANIM_KEY_S16_CUBIC:
        Anim_ConvertKeysS4(rec, curve);
        break;
    case ANIM_KEY_S16_MIXED:
        Anim_ConvertKeysSM(rec, curve);
        break;
    }
}

/* Curve curve, ANIM_KEY_F32_LINEAR, from record rec's keys (KeyF2), copied to gAnimKeyHeap. */
void Anim_ConvertKeysF2(AnimFileCurve *rec, AnimCurve *curve)
{
    KeyF2 *src = (KeyF2 *)(rec + 1);
    KeyF2 *dst;
    s32 i;

    curve->type = ANIM_KEY_F32_LINEAR;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeyF2), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->v = src->v;
        dst->t = src->t;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* Curve curve, ANIM_KEY_F32_CUBIC, from record rec's keys (KeyF4), copied to gAnimKeyHeap. */
void Anim_ConvertKeysF4(AnimFileCurve *rec, AnimCurve *curve)
{
    KeyF4 *src = (KeyF4 *)(rec + 1);
    KeyF4 *dst;
    s32 i;

    curve->type = ANIM_KEY_F32_CUBIC;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeyF4), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->v = src->v;
        dst->t = src->t;
        dst->in = src->in;
        dst->out = src->out;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* Curve curve, ANIM_KEY_F32_MIXED, from record rec's keys (KeyFM), copied to gAnimKeyHeap. */
void Anim_ConvertKeysFM(AnimFileCurve *rec, AnimCurve *curve)
{
    KeyFM *src = (KeyFM *)(rec + 1);
    KeyFM *dst;
    s32 i;

    curve->type = ANIM_KEY_F32_MIXED;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeyFM), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->mode = src->mode;
        dst->v = src->v;
        dst->t = src->t;
        dst->in = src->in;
        dst->out = src->out;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* Curve curve, ANIM_KEY_S16_LINEAR, from record rec's keys (KeyS2), copied to gAnimKeyHeap. */
void Anim_ConvertKeysS2(AnimFileCurve *rec, AnimCurve *curve)
{
    FileKeyS2 *src = (FileKeyS2 *)(rec + 1);
    KeyS2 *dst;
    s32 i;

    curve->type = ANIM_KEY_S16_LINEAR;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeyS2), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->v = src->v;
        dst->t = src->t;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* Curve curve, ANIM_KEY_S16_CUBIC, from record rec's keys (KeyS4), copied to gAnimKeyHeap. */
void Anim_ConvertKeysS4(AnimFileCurve *rec, AnimCurve *curve)
{
    FileKeyS4 *src = (FileKeyS4 *)(rec + 1);
    KeyS4 *dst;
    s32 i;

    curve->type = ANIM_KEY_S16_CUBIC;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeyS4), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->v = src->v;
        dst->t = src->t;
        dst->in = src->in;
        dst->out = src->out;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* Curve curve, ANIM_KEY_S16_MIXED, from record rec's keys (KeySM), copied to gAnimKeyHeap. */
void Anim_ConvertKeysSM(AnimFileCurve *rec, AnimCurve *curve)
{
    FileKeySM *src = (FileKeySM *)(rec + 1);
    KeySM *dst;
    s32 i;

    curve->type = ANIM_KEY_S16_MIXED;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeySM), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->mode = src->mode;
        dst->v = src->v;
        dst->t = src->t;
        dst->in = src->in;
        dst->out = src->out;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* The latest end time (AnimCurve_GetEndTime) of animation anim's curves, at least 0. */
f32 Anim_GetEndTime(AnimHeader *anim)
{
    f32 max = 0.0f;
    s32 i;
    f32 time;

    for (i = 0; i < anim->count; i++) {
        time = AnimCurve_GetEndTime(&((AnimCurve *)(anim->curves + gAnimCurveBuf))[i]);
        if (time > max) {
            max = time;
        }
    }
    return max;
}

/* The earliest start time (AnimCurve_StartTime) of animation anim's curves (1e10 when none). */
f32 Anim_GetStartTime(AnimHeader *anim)
{
    f32 min = 1e10f;
    s32 i;
    f32 time;

    for (i = 0; i < anim->count; i++) {
        time = AnimCurve_StartTime(&((AnimCurve *)(anim->curves + gAnimCurveBuf))[i]);
        if (time < min) {
            min = time;
        }
    }
    return min;
}

/* The entries of an animation file: its header starts like a skeleton
 * file's, flags then a count. */
s32 AnimFile_Count(void *file)
{
    return ((SkelFileHeader *)AnimFile_Header(file))->count;
}
