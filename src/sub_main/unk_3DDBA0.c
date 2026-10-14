/*
 * Memory card saving and loading. The save menu steps (SaveMenu_SaveSystem ...)
 * start card operations: the card task gCardTask (0xF4 bytes) runs one
 * step function of gCardSteps per frame (CardTask_Update), each driving the
 * libmc calls (sceMcInit ... sceMcSetFileInfo) through small sub-state
 * machines, and the menu polls the result (CardTask_Result, -1 while busy).
 * The system data (gSysSave) and the character data (SaveData slots at
 * gCardSlots) are copied to and from the card buffer here.
 *
 * The file ends with monster helpers: the nearest player in range
 * (Em_FindTargetPlayer), the breakable parts' hp (Em_SetPartScaled ...) and the
 * bone accessors (Em_GetBonePos ...).
 */
#include "common.h"
#include "syssettings.h"
#include "em.h"
#include "save.h"
#include "player.h"
#include "objhead.h"
#include "card.h"
#include "syssave.h"
#include "net_menu.h"


/* The date of a save slot (8 bytes). */
typedef struct SaveDate {
    s32 a;
    s32 b;
} SaveDate;

/* The save/load menu (the first argument of SaveMenu_CopyCharacter ...). */
typedef struct SaveMenu {
    u8 pad00[0x18];
    s32 unk18;          /* 0x18: card slot */
    u8 pad1C[0x40 - 0x1C];
    s32 unk40[8];       /* 0x40: result per slot */
    s32 unk60;          /* 0x60: date of the loaded data */
    s32 unk64;
    SaveDate dates[4];  /* 0x68: per save slot */
    s32 unk88;          /* 0x88 */
    u8 pad8C[0x90 - 0x8C];
    u8 *unk90;          /* 0x90: save buffer */
    u8 *unk94;          /* 0x94 */
    u8 pad98[0x9C - 0x98];
    u8 unk9C[4];        /* 0x9C: by slot + 1; 1: loaded, 0x80: broken */
    u8 padA0[0xA2 - 0xA0];
    u8 slot;            /* 0xA2: save slot */
    u8 step;            /* 0xA3 */
    u8 padA4[0xA8 - 0xA4];
    s32 unkA8;          /* 0xA8: date of the saved data */
    s32 unkAC;
} SaveMenu;

/* A breakable part of a monster: Em.parts[9] from +0x2C4 (include/em.h). */
#define PART_ACTIVE(em, i) ((em)->parts[i].active)
#define PART_INDEX(em, i) ((em)->parts[i].index)
#define PART_UNK2(em, i) ((em)->parts[i].unk2)
#define PART_UNK3(em, i) ((em)->parts[i].unk3)
#define PART_HP(em, i) ((em)->parts[i].hp)
#define PART_STATE(em, i) ((em)->parts[i].state)

extern s32 D_006AF014;
extern s32 gCardFileKind;
extern u8 D_006AF040;
extern s32 gCardLocked;
extern u8 D_006AF02D[3];
extern s16 *gEmPartHpTables[];
extern u8 gCardDirEntry[];
extern u8 D_006AF0E0[];
extern char D_006AF060[];
extern s32 D_006AF0D0;
extern u8 D_00468B40[];
extern u16 D_00468B54;
extern u8 gOldGameKind;
extern SaveData *gCardSlots;

void *memset(void *p, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
void func_161EC0(void);
void func_161ED8(void);
s32 sceMcInit(void);
s32 sceMcSync(s32 port, s32 *a, s32 *b);
char *strcpy(char *dst, const char *src);
void Mtx_GetTrans(f32 *out, f32 *m);
f32 Math_Absf(f32 x);
f32 Math_Sqrtf(f32 x);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Card_Lock(void);
void Card_Unlock(void);
s32 Card_GetInfo(CardTask *t);
s32 Card_Sync(CardTask *t);
s32 sceMcDelete(s32 a, s32 b, char *name);
s32 sceMcFormat(s32 a, s32 b);
s32 sceMcGetInfo(s32 port, s32 slot, s32 *type, s32 *free, s32 *format);
s32 sceMcOpen(s32 port, s32 slot, char *name, s32 mode);
s32 sceMcClose(s32 fd);
s32 sceMcWrite(s32 fd, u8 *buf, s32 size);
s32 sceMcRead(s32 fd, u8 *buf, s32 size);
s32 sceMcSetFileInfo(s32 port, s32 slot, char *name, u8 *info, s32 flags);
s32 Card_GetDir(CardTask *t);
s32 Card_GetDirEntry(CardTask *t);
s32 Card_WriteFile(CardTask *t);
s32 Card_MkDir(CardTask *t);
s32 Card_CreateFile(CardTask *t);
s32 Card_ReadFile(CardTask *t);
s32 Card_SetFileInfo(CardTask *t);
s32 Card_Delete(CardTask *t);
void CardOp_MakeIconSys(CardTask *t);
char *strcat(char *dst, const char *src);
void CardTask_End(void);
void CardTask_EndIfDone(CardTask *t);
s32 Card_Format(CardTask *t);
s32 Card_DeleteDir(CardTask *t);
extern char D_004B06A0[];
extern char D_004B06A8[];
extern char D_004B06B0[];
static s32 func_003DF2E0(void);
static void Em_ScalePartHp(Em *em, s32 i, s16 hp);
s32 sprintf(char *buf, char *fmt, ...);
void McDate_SetWeekday(McDateTime *d);
void Card_GetDate(void *dst);
static void CardState_SetFlags(s32 a);
s32 SaveBuf_Offset(s32 addr, s32 kind, s32 n);
void SaveMenu_CopySystem(SaveMenu *m, u8 *data, s32 save);
void SaveMenu_CopyCharacter(SaveMenu *m, s32 data, s32 save, s32 slot);
void CardTask_StartRead(s32 a, u8 *buf);
void CardTask_StartReadClear(s32 a, u8 *buf, s32 c);
void CardTask_StartSave(s32 a, u8 *buf);
void CardTask_SelectFile(s32 a, s32 b);
s32 CardTask_Result(void);
struct NetMenu;
s32 SaveMenu_ReadBad(struct NetMenu *m);
s32 SaveMenu_SlotUsed(s32 i);
void SaveMenu_ClearCharacter(struct NetMenu *m);
s32 SaveBuf_FirstByte(SaveMenu *m, s32 data, s32 save, s32 slot);
void SaveExtra_CopyFile(s32 a, s32 addr);
void Phrase_TruncateNames(void);
void Options_Apply(void);
void SaveFile_Encrypt(u8 *data);
s32 sceMcGetDir(s32 port, s32 slot, char *name, u32 mode, s32 max, u8 *table);
s32 sceMcMkdir(s32 a, s32 b, char *name);
u16 strlen(char *s);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
EmBone *Em_GetBone(void *obj, s16 bone);
f32 *Em_BoneMatrix(void *obj, s16 bone);

/* unk_3DDBA0's .rodata (0x4B0570-0x4B06A0) */
#pragma rodata on
char D_004B0570[0x18] = {
#include "sub_main/004B0570.inc"
};
char D_004B0588[13] = {
#include "sub_main/004B0588.inc"
};
char D_004B0598[0xC] = {
#include "sub_main/004B0598.inc"
};
char D_004B05A8[0xC] = {
#include "sub_main/004B05A8.inc"
};
char D_004B05C0[0x10] = {
#include "sub_main/004B05C0.inc"
};
char D_004B05D0[0x10] = {
#include "sub_main/004B05D0.inc"
};
char D_004B05E0[0x10] = {
#include "sub_main/004B05E0.inc"
};
char D_004B05F0[0x10] = {
#include "sub_main/004B05F0.inc"
};
char D_004B0600[0x10] = {
#include "sub_main/004B0600.inc"
};
char D_004B0610[0x10] = {
#include "sub_main/004B0610.inc"
};
char D_004B0620[0x10] = {
#include "sub_main/004B0620.inc"
};
char D_004B0630[0x10] = {
#include "sub_main/004B0630.inc"
};
char D_004B0640[0x10] = {
#include "sub_main/004B0640.inc"
};
char D_004B0650[0x10] = {
#include "sub_main/004B0650.inc"
};
char D_004B0660[0x10] = {
#include "sub_main/004B0660.inc"
};
/* "" (the code points to it for no file name), then D_004B0678 */
char D_004B0670[32] = {
#include "sub_main/004B0670.inc"
};
char D_004B0690[0x10] = {
#include "sub_main/004B0690.inc"
};
#pragma rodata off

s32 func_003DDBA0(void)
{
    return 0;
}

/*
 * Static: SaveMenu_LoadSystem keeps its mode in a1 across the call. The
 * network menu (unk_3D54B0.c) calls it too; the two files are probably one
 * translation unit (their .rodata and .data are adjacent), and mwcc_cc.py
 * makes the symbol global for that file's calls.
 */
static void CardState_SetFlags(s32 a)
{
    D_006AF014 = a;
    gCardFileKind = a;
}

void CardState_SetFlag(s32 a)
{
    D_006AF014 = a;
}

s32 SaveBuf_Offset(s32 addr, s32 kind, s32 n)
{
    switch (kind) {
    case 0:
        break;
    case 1:
        addr += 0x580;
        addr += n * 0x8080;
        break;
    case 2:
        addr += 0x18700;
        break;
    }
    return addr;
}

void SaveMenu_CopySystem(SaveMenu *m, u8 *data, s32 save)
{
    s32 *hdr = (s32 *)(data + 0x40);
    u8 *body = (u8 *)hdr + 0x10;
    u8 *sys = (u8 *)&gSysSave;

    if (save == 0) {
        memcpy(sys, body, 0x10);
        memcpy(sys + 0x10, body + 0x10, 0x10);
        *(s32 *)(sys + 0x458) = *(s32 *)(body + 0x20);
        sys[0x45C] = body[0x24];
        sys[0x45D] = body[0x25];
        sys[0x3C5E] = body[0x2A];
        if (body[0x2A] != 0) {
            gOldGameKind = body[0x2A];
        }
        memcpy(sys + 0x3C64, body + 0x30, 0x10);
        memcpy(sys + 0x45E, body + 0x26, 4);
        memcpy(sys + 0x20, body + 0x40, 0x438);
        m->unk60 = hdr[0];
        m->unk64 = hdr[1];
        Phrase_TruncateNames();
        Options_Apply();
        return;
    }
    Card_GetDate(hdr);
    m->unkA8 = hdr[0];
    m->unkAC = hdr[1];
    memset(body, 0, 0x500);
    memcpy(body, sys, 0x10);
    memcpy(body + 0x10, sys + 0x10, 0x10);
    *(s32 *)(body + 0x20) = *(s32 *)(sys + 0x458);
    body[0x24] = sys[0x45C];
    body[0x25] = sys[0x45D];
    body[0x2A] = gOldGameKind;
    memcpy(body + 0x30, sys + 0x3C64, 0x10);
    memcpy(body + 0x26, sys + 0x45E, 4);
    memcpy(body + 0x40, sys + 0x20, 0x438);
    SaveFile_Encrypt(data);
}

s32 SaveMenu_SaveSystem(SaveMenu *m)
{
    s32 r;

    switch (m->step) {
    case 0:
        CardState_SetFlags(1);
        SaveMenu_CopySystem(m, (u8 *)SaveBuf_Offset((s32)m->unk90, 0, 0), 1);
        CardTask_StartSave(m->unk18, m->unk90);
        CardTask_SelectFile(4, 0);
        m->step++;
        break;
    case 1:
        r = CardTask_Result();
        m->unk40[m->unk18] = r;
        switch (r) {
        case 0:
            m->unk60 = m->unkA8;
            m->unk64 = m->unkAC;
            return 1;
        case -1:
            break;
        case -0xFD:
        case -0xFC:
        case -0xFF:
        default:
            return -1;
        }
        break;
    }
    return 0;
}

s32 SaveMenu_LoadSystem(SaveMenu *m, s32 mode)
{
    s32 r;

    switch (m->step) {
    case 0:
        CardState_SetFlags(1);
        if (mode == 0) {
            CardTask_StartRead(m->unk18, m->unk94);
        } else {
            CardTask_StartReadClear(m->unk18, m->unk94, 1);
        }
        CardTask_SelectFile(4, 0);
        m->step++;
        break;
    case 1:
        r = CardTask_Result();
        m->unk40[m->unk18] = r;
        switch (r) {
        case 0:
            if (SaveMenu_ReadBad((struct NetMenu *)m) != 0) {
                m->unk40[m->unk18] = -0xFB;
                return -1;
            }
            SaveMenu_CopySystem(m, m->unk94, 0);
            return 1;
        case -1:
            break;
        default:
            return -1;
        }
        break;
    }
    return 0;
}

s32 SaveMenu_SaveExtra(SaveMenu *m)
{
    s32 r;

    switch (m->step) {
    case 0:
        CardState_SetFlags(0x10);
        SaveExtra_CopyFile(1, SaveBuf_Offset((s32)m->unk90, 2, 0));
        CardTask_StartSave(m->unk18, m->unk90);
        CardTask_SelectFile(8, 0);
        m->step++;
        break;
    case 1:
        r = CardTask_Result();
        m->unk40[m->unk18] = r;
        switch (r) {
        case 0:
            m->unk60 = m->unkA8;
            m->unk64 = m->unkAC;
            return 1;
        case -1:
            break;
        case -0xFD:
        case -0xFC:
        case -0xFF:
        default:
            return -1;
        }
        break;
    }
    return 0;
}

void Save_CopyToSlot(s32 slot)
{
    SaveData *p;

    p = gCardSlots;
    p += slot;
    *p = gSysSave.save;
}

void SaveMenu_CopyCharacter(SaveMenu *m, s32 data, s32 save, s32 slot)
{
    s32 *hdr = (s32 *)(data + 0x40);
    u8 *body = (u8 *)hdr + 0x10;
    SaveData *s;

    s = gCardSlots;
    s += slot;

    if (save == 0) {
        memcpy(s, body, 0x37F8);
        m->unkA8 = hdr[0];
        m->unkAC = hdr[1];
        return;
    }
    Card_GetDate(hdr);
    m->unkA8 = hdr[0];
    m->unkAC = hdr[1];
    memset(body, 0, 0x8000);
    memcpy(body, s, 0x37F8);
    SaveFile_Encrypt((u8 *)data);
}

s32 SaveMenu_SaveCharacter(SaveMenu *m)
{
    s32 r;
    s32 slot;

    switch (m->step) {
    case 0:
        m->unk88 = 2 << m->slot;
        slot = m->slot;
        SaveMenu_CopyCharacter(m, SaveBuf_Offset((s32)m->unk90, 1, slot), 1, slot);
        CardTask_StartSave(m->unk18, m->unk90);
        CardTask_SelectFile(slot + 5, 0);
        m->step++;
        break;
    case 1:
        r = CardTask_Result();
        m->unk40[m->unk18] = r;
        switch (r) {
        case 0:
            m->dates[m->slot].a = m->unkA8;
            m->dates[m->slot].b = m->unkAC;
            return 1;
        case -1:
            break;
        case -0xFD:
        case -0xFC:
        case -0xFF:
        default:
            return -1;
        }
        break;
    }
    return 0;
}

s32 SaveMenu_LoadCharacter(SaveMenu *m)
{
    s32 r;
    s32 slot;

    switch (m->step) {
    case 0:
        m->unk88 = 2 << m->slot;
        slot = m->slot;
        CardTask_StartRead(m->unk18, m->unk94);
        CardTask_SelectFile(slot + 5, 0);
        m->step++;
        break;
    case 1:
        r = CardTask_Result();
        m->unk40[m->unk18] = r;
        switch (r) {
        case 0:
            slot = m->slot + 1;
            if (SaveMenu_ReadBad((struct NetMenu *)m) != 0) {
                m->unk40[m->unk18] = -0xFB;
                m->unk9C[slot] = 0x80;
                SaveMenu_ClearCharacter((struct NetMenu *)m);
                return -1;
            }
            m->unk9C[slot] = 1;
            SaveMenu_CopyCharacter(m, (s32)m->unk94, 0, m->slot);
            return 1;
        case -1:
            break;
        case -0xFD:
        case -0xFC:
        case -0xFF:
        default:
            return -1;
        }
        break;
    }
    return 0;
}

s32 SaveBuf_FirstByte(SaveMenu *m, s32 data, s32 save, s32 slot)
{
    return ((u8 *)data)[0x50];
}

s32 SaveMenu_CheckCharacter(SaveMenu *m)
{
    s32 r;
    s32 slot;

    switch (m->step) {
    case 0:
        m->unk88 = 2 << m->slot;
        slot = m->slot;
        CardTask_StartRead(m->unk18, m->unk94);
        CardTask_SelectFile(slot + 5, 0);
        m->step++;
        break;
    case 1:
        r = CardTask_Result();
        m->unk40[m->unk18] = r;
        switch (r) {
        case 0:
            slot = m->slot + 1;
            if (SaveMenu_ReadBad((struct NetMenu *)m) != 0) {
                m->unk40[m->unk18] = -0xFB;
                m->unk9C[slot] = 0x80;
                SaveMenu_ClearCharacter((struct NetMenu *)m);
                return -1;
            }
            if (SaveBuf_FirstByte(m, (s32)m->unk94, 0, m->slot) == 1) {
                m->unk9C[slot] = 1;
            } else {
                m->unk9C[slot] = 0;
            }
            return 1;
        case -1:
            break;
        case -0xFD:
        case -0xFC:
        case -0xFF:
        default:
            return -1;
        }
        break;
    }
    return 0;
}

s32 Card_IsFlagE055(void)
{
    return gSysSettings.systemSaved == 1;
}

s32 Card_IsFlagF040(void)
{
    return D_006AF040 == 1;
}

s32 Card_SlotReady(s32 i)
{
    u8 *c = (u8 *)&gCardMenu;   /* c[i + 0x9D] is gCardMenu.charRead[i]: the field form changes the code */

    if (D_006AF02D[i] & 0x80) {
        return 0;
    }
    if (c[i + 0x9D] != 0) {
        return 1;
    }
    return 0;
}

s32 Card_ReadyCount(void)
{
    s32 n = 0;
    s32 i;
    u8 *c = (u8 *)&gCardMenu;   /* c[i + 0x9D] is gCardMenu.charRead[i]: the field form changes the code */

    for (i = 0; i < 3; i++) {
        if (c[i + 0x9D] == 1) {
            n++;
        }
    }
    return n;
}

void Card_Lock(void)
{
    if (gCardLocked == 0) {
        func_161EC0();
        gCardLocked = 1;
    }
}

void Card_Unlock(void)
{
    if (gCardLocked != 0) {
        func_161ED8();
        gCardLocked = 0;
    }
}

void Card_Init(void)
{
    memset(&gCardTask, 0, 0xF4);
    gCardTask.portCount = 2;
    while (sceMcInit() < 0) {
    }
}

s32 Card_Sync(CardTask *t)
{
    t->syncCmd = 0;
    t->syncResult = 0;
    return -(sceMcSync(1, &t->syncCmd, &t->syncResult) == 0);
}

s32 Card_GetInfo(CardTask *t)
{
    s32 v;

    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        t->subStep++;
        t->retries = 3;
    case 1:
        if (sceMcGetInfo(t->port, 0, &t->type, &t->free, &t->format) < 0) {
            if (--t->retries > 0) {
                break;
            }
        none:
            t->subStep = 0;
            t->cardState[t->port] = 0;
            t->cardFree[t->port] = 0;
            return 0;
        }
        t->subStep++;
        break;
    case 2:
        if (Card_Sync(t) < 0) {
            break;
        }
        switch (t->syncResult) {
        case 0:
            v = 1;
        check:
            if (t->type != 2) {
                goto none;
            }
            if (t->format == 0) {
                goto unformatted;
            }
            t->cardState[t->port] = v;
            t->cardFree[t->port] = t->free;
            break;
        case -1:
            t->changed |= 1 << t->port;
            v = 2;
            goto check;
        case -2:
            if (t->type != 2) {
                goto none;
            }
            t->changed |= 1 << t->port;
        unformatted:
            if (--t->retries > 0) {
            retry:
                t->subStep = 1;
                return -1;
            }
            t->cardState[t->port] = 3;
            t->cardFree[t->port] = 8000;
            break;
        default:
            if (--t->retries > 0) {
                goto retry;
            }
            goto none;
        }
        t->subStep = 0;
        return t->cardState[t->port];
    }
    return -1;
}

s32 Card_GetDirEntry(CardTask *t)
{
    s32 r;

    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (sceMcGetDir(t->port, 0, t->name, 0, 1, gCardDirEntry) < 0) {
            return 1;
        }
        t->subStep++;
        break;
    case 1:
        if (Card_Sync(t) < 0) {
            break;
        }
        t->subStep = 0;
        r = t->syncResult;
        if (r == 1) {
            return 0;
        }
        if (r == -5 || r < -10) {
            return 2;
        }
        if (r == -6) {
            return 3;
        }
        return 1;
    }
    return -1;
}

s32 Card_WriteFile(CardTask *t)
{
    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (sceMcOpen(t->port, 0, t->name, 1) < 0) {
        fail:
            t->subStep = 0;
            return 1;
        }
        t->subStep++;
        break;
    case 1:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->syncResult < 0) {
            goto fail;
        }
        t->fd = t->syncResult;
        if (sceMcWrite(t->fd, t->buf, t->size) < 0) {
            goto fail;
        }
        t->subStep++;
        break;
    case 2:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->syncResult < 0) {
            goto fail;
        }
        if (sceMcClose(t->fd) < 0) {
            goto fail;
        }
        t->subStep++;
        break;
    case 3:
        if (Card_Sync(t) < 0) {
            break;
        }
        t->subStep = 0;
        return t->syncResult < 0;
    }
    return -1;
}

s32 Card_MkDir(CardTask *t)
{
    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (sceMcMkdir(t->port, 0, t->name) < 0) {
            return 1;
        }
        t->subStep++;
        break;
    case 1:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->syncResult == -4) {
            t->syncResult = 0;
        }
        t->subStep = 0;
        return t->syncResult < 0;
    }
    return -1;
}

s32 Card_CreateFile(CardTask *t)
{
    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (sceMcOpen(t->port, 0, t->name, 0x200) < 0) {
        fail:
            t->subStep = 0;
            return 1;
        }
        t->subStep++;
        break;
    case 1:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->syncResult < 0) {
            goto fail;
        }
        if (sceMcClose(t->fd) < 0) {
            goto fail;
        }
        t->subStep++;
        break;
    case 2:
        if (Card_Sync(t) < 0) {
            break;
        }
        t->subStep = 0;
        return t->syncResult < 0;
    }
    return -1;
}

s32 Card_ReadFile(CardTask *t)
{
    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (sceMcOpen(t->port, 0, t->name, 2) < 0) {
        fail:
            t->subStep = 0;
            return 1;
        }
        t->subStep++;
        break;
    case 1:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->syncResult < 0) {
            goto fail;
        }
        t->fd = t->syncResult;
        if (sceMcRead(t->fd, t->buf, t->size) < 0) {
            goto fail;
        }
        t->subStep++;
        break;
    case 2:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->syncResult < 0) {
            goto fail;
        }
        if (sceMcClose(t->fd) < 0) {
            goto fail;
        }
        t->subStep++;
        break;
    case 3:
        if (Card_Sync(t) < 0) {
            break;
        }
        t->subStep = 0;
        return t->syncResult < 0;
    }
    return -1;
}

s32 Card_SetFileInfo(CardTask *t)
{
    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->unkC8 == 0) {
            return 0;
        }
        if (sceMcGetDir(t->port, 0, t->name, 0, 1, D_00468B40) < 0) {
            return 1;
        }
        t->subStep++;
        break;
    case 1:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->syncResult < 0) {
            return 1;
        }
        D_00468B54 |= 8;
        if (sceMcSetFileInfo(t->port, 0, t->name, D_00468B40, 4) < 0) {
            t->subStep = 0;
            return 1;
        }
        t->subStep++;
        break;
    case 2:
        if (Card_Sync(t) < 0) {
            break;
        }
        t->subStep = 0;
        return t->syncResult < 0;
    }
    return -1;
}

s32 Card_Delete(CardTask *t)
{
    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (sceMcDelete(t->port, 0, t->name) < 0) {
            return 1;
        }
        t->subStep++;
        break;
    case 1:
        if (Card_Sync(t) < 0) {
            break;
        }
        t->subStep = 0;
        return t->syncResult < 0;
    }
    return -1;
}

s32 Card_Format(CardTask *t)
{
    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (sceMcFormat(t->port, 0) < 0) {
            return 1;
        }
        t->subStep++;
        break;
    case 1:
        if (Card_Sync(t) < 0) {
            break;
        }
        t->subStep = 0;
        return t->syncResult < 0;
    }
    return -1;
}

static s32 func_003DF2E0(void)
{
    return 1;
}

/* Shared with CardTask_SelectFile, still assembly. */
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3DDBA0", D_004B06A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3DDBA0", D_004B06A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3DDBA0", D_004B06B0);

s32 Card_DeleteDir(CardTask *t)
{
    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->unkC0 == 0) {
            goto dir;
        }
        t->entryNo = 0;
    next:
        sprintf(D_006AF060, D_004B06A0, t->name);
        t->subStep = 1;
    case 1:
        if (sceMcGetDir(t->port, 0, D_006AF060, t->entryNo, 1, gCardDirEntry) < 0) {
        fail:
            t->subStep = 0;
            return 1;
        }
        t->subStep++;
        break;
    case 2:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->syncResult < 0) {
            goto fail;
        }
        if (t->syncResult == 1) {
            t->entryNo++;
            if (D_006AF0E0[0] == '.') {
                goto next;
            }
            sprintf(D_006AF060, D_004B06A8, t->name, (char *)D_006AF0E0);
            t->unkC4 = 0;
        } else {
        dir:
            sprintf(D_006AF060, D_004B06B0, t->name);
            t->unkC4 = 1;
        }
        if (sceMcDelete(t->port, 0, D_006AF060) < 0) {
            goto fail;
        }
        t->subStep = 3;
        break;
    case 3:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (t->syncResult < 0) {
            goto fail;
        }
        t->subStep = 0;
        if (t->unkC4 == 0) {
            goto next;
        }
        return 0;
    }
    return -1;
}

s32 Card_GetDir(CardTask *t)
{
    switch (t->subStep) {
    case 0:
        if (Card_Sync(t) < 0) {
            break;
        }
        if (sceMcGetDir(t->port, 0, t->name, 0, 0x100, t->buf) < 0) {
            return 1;
        }
        t->subStep++;
        break;
    case 1:
        if (Card_Sync(t) < 0) {
            break;
        }
        t->subStep = 0;
        if (t->syncResult < 0) {
            t->entryNo = 0;
            return 2;
        }
        t->entryNo = t->syncResult;
        return 0;
    }
    return -1;
}

void CardTask_Begin(s32 i)
{
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.state = 0;
    gCardTask.op = i;
    strcpy(gCardTask.dir, gCardOps[i]->files[4].name);
}

void CardTask_Update(void)
{
    Card_Lock();
    gCardSteps[gCardTask.state](&gCardTask);
    Card_Unlock();
}

void CardTask_End(void)
{
    gCardTask.state = 0;
    gCardTask.unkE0 = 0;
}

void func_003DF660(void)
{
}

void CardTask_EndIfDone(CardTask *t)
{
    if (t->result != -1) {
        CardTask_End();
    }
}

void CardTask_StartCheck(void)
{
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.state = 1;
    gCardTask.result = -1;
    gCardTask.port = 0;
}

void CardTask_NextSlot(CardTask *t)
{
    if (Card_GetInfo(t) >= 0) {
        if (++t->port >= t->portCount) {
            t->port = 0;
        }
    }
}

void CardTask_StartRead(s32 a, u8 *buf)
{
    CardOp *op = gCardOps[gCardTask.op];

    gCardTask.state = 2;
    gCardTask.result = -1;
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.port = a;
    gCardTask.buf = buf;
    sprintf(gCardTask.name, D_004B06A8, gCardTask.dir, gCardTask.dir);
    gCardTask.size = op->files[4].size;
}

void CardTask_WriteStep(CardTask *t)
{
    switch (t->step) {
    case 0:
        switch (Card_GetInfo(t)) {
        case 0:
            t->result = -0xFF;
            break;
        case 3:
            t->result = -0xFE;
            break;
        case 1:
        case 2:
            t->step++;
            break;
        }
        break;
    case 1:
        switch (Card_GetDirEntry(t)) {
        case 0:
            t->step++;
            break;
        case 1:
            t->step = 0;
            t->result = -0xFD;
            break;
        case 2:
            t->step = 0;
            t->result = -0xFF;
            break;
        case 3:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    case 2:
        switch (Card_WriteFile(t)) {
        case 0:
            t->step = 0;
            t->result = 0;
            break;
        case 1:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    }
    CardTask_EndIfDone(t);
}

void CardTask_StartReadClear(s32 a, u8 *buf, s32 c)
{
    CardOp *op = gCardOps[gCardTask.op];

    gCardTask.state = 3;
    gCardTask.result = -1;
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.port = a;
    gCardTask.buf = buf;
    gCardTask.unkC4 = c;
    sprintf(gCardTask.name, D_004B06A8, gCardTask.dir, gCardTask.dir);
    gCardTask.size = op->files[4].size;
    memset(gCardTask.buf, 0, gCardTask.size);
}

void CardTask_SpaceStep(CardTask *t)
{
    CardOp *op = gCardOps[t->op];

    switch (t->step) {
    case 0:
        switch (Card_GetInfo(t)) {
        case 0:
            t->result = -0xFF;
            break;
        case 3:
            t->result = -0xFE;
            break;
        case 1:
        case 2:
            t->step++;
            break;
        }
        break;
    case 1:
        switch (Card_GetDirEntry(t)) {
        case 0:
            if (t->unkC4 == 0) {
                t->step = 0;
                t->result = 0;
            } else {
                t->step++;
            }
            break;
        case 1:
            t->step += 2;
            strcpy(t->name, t->dir);
            t->size = 0;
            break;
        case 2:
            t->step = 0;
            t->result = -0xFF;
            break;
        case 3:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    case 2:
        switch (Card_WriteFile(t)) {
        case 0:
            t->step = 0;
            t->result = 0;
            break;
        case 1:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    case 3:
        switch (Card_GetDirEntry(t)) {
        case 0:
            t->step = 0;
            t->result = 0;
            break;
        case 1:
            t->step = 0;
            if (t->cardFree[t->port] >= op->blocks) {
                t->result = -0xFD;
            } else {
                t->result = -0xFC;
            }
            break;
        case 2:
            t->step = 0;
            t->result = -0xFF;
            break;
        case 3:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    }
    CardTask_EndIfDone(t);
}

void CardTask_StartSave(s32 a, u8 *buf)
{
    CardOp *op = gCardOps[gCardTask.op];
    s32 i;
    CardTask *t = &gCardTask;

    gCardTask.result = -1;
    gCardTask.port = a;
    gCardTask.state = 4;
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.fileNo = 0;
    gCardTask.mask = 0;
    gCardTask.buf = buf;
    for (i = 4; i < 9; i++) {
        op->files[i].ptr = buf;
        buf += (op->files[i].size + 63) / 64 * 64;
        if (i != 4) {
            t->mask |= 1 << i;
        }
    }
    t->unkE0 = 1;
}

void CardTask_SaveStep(CardTask *t)
{
    CardOp *op = gCardOps[t->op];

    switch (t->step) {
    case 0:
        switch (Card_GetInfo(t)) {
        case 0:
            t->result = -0xFF;
            break;
        case 3:
            t->result = -0xFE;
            break;
        case 1:
        case 2:
            t->step++;
            CardOp_MakeIconSys(t);
            sprintf(t->name, D_004B06B0, t->dir);
            t->unkC8 = op->unk9C;
            break;
        }
        break;
    case 1:
        switch (Card_MkDir(t)) {
        case 0:
            t->step++;
            t->fileNo = 0;
            break;
        case 1:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    case 2:
        switch (Card_SetFileInfo(t)) {
        case 0:
            t->step++;
            break;
        case 1:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    case 3:
    file:
        if ((t->unkB8 = op->files[t->fileNo].flag) == 0) {
            goto next;
        }
        sprintf(t->name, "%s/", t->dir);
        if (t->fileNo == 4) {
            strcat(t->name, t->dir);
        } else {
            strcat(t->name, op->files[t->fileNo].name);
        }
        t->buf = op->files[t->fileNo].ptr;
        t->size = op->files[t->fileNo].size;
        t->step++;
    case 4:
        switch (Card_GetDirEntry(t)) {
        case 0:
            if (t->mask & (1 << t->fileNo)) {
                goto next;
            }
            if (D_006AF0D0 != t->size) {
                t->step++;
                break;
            }
            if (t->unkB8 == 1) {
                goto next;
            }
            t->step += 3;
            break;
        case 1:
            t->step += 2;
            break;
        case 2:
            t->step = 0;
            t->result = -0xFF;
            break;
        case 3:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    case 5:
        switch (Card_Delete(t)) {
        case 0:
            t->step++;
            break;
        case 1:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    case 6:
        switch (Card_CreateFile(t)) {
        case 0:
            t->step++;
            break;
        case 1:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    case 7:
        switch (Card_ReadFile(t)) {
        case 0:
        next:
            if (++t->fileNo < 9) {
                t->step = 3;
                goto file;
            }
            t->step = 0;
            t->result = 0;
            break;
        case 1:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    }
    CardTask_EndIfDone(t);
}

/* FAKEMATCH: file is a copy of the parameter (same type): with the parameter used directly the two
 * parameters swap saved registers (s2/s1) and the file >= 4 test is scheduled later. The copy is
 * numbered as a local, which gives the original's order (the permuter found it as a copy used in the
 * two tests). */
void CardTask_SelectFile(s32 file_in, s32 clear)
{
    s32 file = file_in;
    CardTask *t = &gCardTask;
    CardOp *op = gCardOps[gCardTask.op];
    s32 i;

    if (file >= 4) {
        sprintf(t->name, D_004B06A8, t->dir, op->files[file].name);
        t->size = op->files[file].size;
        t->mask = 0;
        for (i = 4; i < 9; i++) {
            if (file != i) {
                t->mask |= 1 << i;
            }
        }
        if (clear) {
            memset(t->buf, 0, t->size);
        }
    }
}

void CardTask_StartDelete(s32 a)
{
    gCardTask.port = a;
    gCardTask.step = 0;
    gCardTask.state = 5;
    gCardTask.result = -1;
    gCardTask.unkE0 = 1;
    gCardTask.subStep = 0;
}

void CardTask_FormatStep(CardTask *t)
{
    switch (t->step) {
    case 0:
        switch (Card_Format(t)) {
        case 0:
            t->result = 0;
            break;
        case 1:
            t->result = -0x100;
            break;
        }
        break;
    }
    CardTask_EndIfDone(t);
}

void CardTask_NopStep(CardTask *t)
{
    switch (t->step) {
    case 0:
        switch (func_003DF2E0()) {
        case 0:
            t->result = 0;
            break;
        case 1:
            t->result = -0x100;
            break;
        }
        break;
    }
    CardTask_EndIfDone(t);
}

void CardTask_UpdateStep(CardTask *t)
{
    switch (t->step) {
    case 0:
        switch (Card_GetInfo(t)) {
        case 0:
            t->result = -0xFF;
            break;
        case 3:
            t->result = -0xFE;
            break;
        case 1:
        case 2:
            t->step++;
            break;
        }
        break;
    case 1:
        switch (Card_GetDirEntry(t)) {
        case 0:
            t->step++;
            break;
        case 1:
            t->step = 0;
            t->result = -0xFD;
            break;
        case 2:
            t->step = 0;
            t->result = -0xFF;
            break;
        case 3:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    case 2:
        switch (Card_DeleteDir(t)) {
        case 0:
            t->step = 0;
            t->result = 0;
            break;
        case 1:
            t->step = 0;
            t->result = -0x100;
            break;
        }
        break;
    }
    CardTask_EndIfDone(t);
}

void CardTask_DeleteStep(CardTask *t)
{
    switch (t->step) {
    case 0:
        switch (Card_DeleteDir(t)) {
        case 0:
            t->result = 0;
            break;
        case 1:
            t->result = -0x100;
            break;
        }
        break;
    }
    CardTask_EndIfDone(t);
}

void CardTask_StartList(s32 a, char *b, u8 *c)
{
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.port = a;
    gCardTask.state = 9;
    gCardTask.result = -1;
    sprintf(gCardTask.name, D_004B06A0, b);
    gCardTask.buf = c;
}

void CardTask_ListStep(CardTask *t)
{
    switch (t->step) {
    case 0:
        switch (Card_GetInfo(t)) {
        case 0:
            t->result = -0xFF;
            break;
        case 3:
            t->result = -0xFE;
            break;
        case 1:
        case 2:
            t->step++;
            break;
        }
        break;
    case 1:
        switch (Card_GetDir(t)) {
        case 0:
            t->step = 0;
            t->result = t->entryNo;
            break;
        case 1:
            t->step = 0;
            t->result = -0x100;
            break;
        case 2:
            t->step = 0;
            t->result = -0xFD;
            break;
        }
        break;
    }
    CardTask_EndIfDone(t);
}

s32 CardTask_Result(void)
{
    return gCardTask.result;
}

s32 Card_PortState(s32 i)
{
    return gCardTask.cardState[i];
}

void Card_ClearChanged(void)
{
    gCardTask.changed = 0;
}

s32 Card_IsChanged(s32 i)
{
    return gCardTask.changed & (1 << i);
}

s32 CardOp_SetFiles(s32 *data)
{
    u8 *base;
    s32 total;
    CardOp *op;
    s32 i;
    s32 cnt;

    op = gCardOps[gCardTask.op];
    if (data != NULL) {
        base = (u8 *)data;
        cnt = data[1];
        data += 2;
        for (i = 0; i < 4; i++) {
            if (cnt <= 0) {
                break;
            }
            if (op->files[i].flag != 0) {
                cnt--;
                op->files[i].ptr = (u8 *)data[0];
                op->files[i].ptr += (s32)base;
                op->files[i].size = data[1];
                data += 2;
            }
        }
    }
    cnt = 0;
    total = 0;
    for (i = 0; i < 9; i++) {
        if (op->files[i].flag != 0) {
            total += (op->files[i].size + 1023) / 1024;
            cnt++;
        }
    }
    total += (cnt + 1) / 2;
    return op->blocks = total + 2;
}

void Card_GetDate(void *dst)
{
    McDateTime *buf;

    McDate_SetWeekday(buf = &D_006AF0C8);
    memcpy(dst, buf, 8);
}

void McDate_SetWeekday(McDateTime *d)
{
    s32 m = d->month;
    s32 y = d->year;

    if (m < 3) {
        y--;
        m += 12;
    }
    d->wday = (d->day + (y + y / 4 - y / 100 + y / 400 + (13 * m + 8) / 5)) % 7;
}

void CardOp_MakeIconSys(CardTask *t)
{
    u8 *icon;
    CardOp *op = gCardOps[t->op];

    if (op->files[4].flag != 0) {
        icon = op->files[0].ptr;
        *(u16 *)(icon + 6) = strlen(op->title1);
        sprintf((char *)(icon + 0xC0), "%s%s", op->title1, op->title2);
        strcpy((char *)(icon + 0x104), op->files[1].name);
        strcpy((char *)(icon + 0x144), op->files[2].name);
        strcpy((char *)(icon + 0x184), op->files[3].name);
    }
}

s8 Em_FindTargetPlayer(Em *em, u16 range, f32 maxDist, f32 maxHeight)
{
    f32 dy;
    f32 bestDist;
    f32 x;
    f32 y;
    f32 z;
    f32 d;
    s32 i;
    s32 best;
    Player *p;
    u16 ang;

    best = -1;
    p = gPlayers;
    em->target = NULL;
    bestDist = 0.0f;
    em->targetNo = -1;
    em->targetDist = 0.0f;
    x = em->posX;
    y = em->posY;
    z = em->posZ;
    for (i = 0; i < 4; i++, p++) {
        if (em->area == p->area) {
            dy = p->pos[1] - y;
            if (maxHeight == 0.0f || Math_Absf(dy) <= maxHeight) {
                f32 dx = p->pos[0] - x;
                f32 dz = p->pos[2] - z;

                d = Math_Sqrtf(dx * dx + dz * dz);
                if (maxDist == 0.0f || d <= maxDist) {
                    dy = Math_Sqrtf(d * d + dy * dy);
                    ang = Math_Heading(x, z, p->pos[0], p->pos[2]) - 0x4000;
                    if ((u16)(ang + range - em->rotY) <= range * 2) {
                        if (bestDist == 0.0f) {
                            bestDist = dy;
                        } else if (dy <= bestDist) {
                            bestDist = dy;
                        } else {
                            continue;
                        }
                        best = i;
                    }
                }
            }
        }
    }
    if (best != -1) {
        em->target = &gPlayers[best];
        em->targetNo = best;
        em->targetDist = bestDist;
        em->targetAngle = (u16)(Math_Heading(x, z, ((Player *)em->target)->pos[0], ((Player *)em->target)->pos[2]) - 0x4000);
    }
    return (u8)best;
}

void Em_SetPartScaled(Em *em, s32 bit)
{
    em->partMask |= 1 << bit;
}

/* Adds to part i's HP its table value hp times 0.33 + 0.08 * (unk98A - 11) / 3, when unk98A is 11 to 17. */
void Em_ScalePartHp(Em *em, s32 i, s16 hp)
{
    if (em->partMask & (1 << i)) {
        if (em->level >= 11 && em->level < 18) {
            hp = hp * (0.33f + 0.08f * ((em->level - 11) / 3.0f));
            PART_HP(em, i) += hp;
        }
    }
}

/* Sets up the 9 body parts from the hit-point table of the monster type (0 or less:
 * no part), then the value after them. Em_ScalePartHp is a known (static) callee. */
void Em_InitParts(Em *em)
{
    s32 i;
    s16 *tbl = gEmPartHpTables[em->type];

    for (i = 0; i < 9; tbl++, i++) {
        if (*tbl > 0) {
            PART_ACTIVE(em, i) = 1;
            PART_INDEX(em, i) = i;
            PART_UNK2(em, i) = 0;
            PART_UNK3(em, i) = 0;
            PART_HP(em, i) = *tbl;
            Em_ScalePartHp(em, i, *tbl);
            PART_STATE(em, i) = 0;
        } else {
            PART_ACTIVE(em, i) = 0;
        }
    }
    em->partDamageMode = *tbl;
}

#pragma optimization_level 4
/* Matches with Em_ScalePartHp static and the table pointer stepped (tbl += i). */
void Em_ResetPartHp(Em *em, u32 i)
{
    s16 *tbl;

    if (i >= 9 || PART_ACTIVE(em, i) == 0) {
        return;
    }
    tbl = gEmPartHpTables[em->type];
    tbl += i;
    PART_HP(em, i) = *tbl;
    Em_ScalePartHp(em, i, *tbl);
}
#pragma optimization_level reset

void Em_GetBonePos(void *obj, s16 bone, f32 *out)
{
    ObjHead *o = obj;

    Mtx_GetTrans(out, o->model->bones[bone].world);
}

void Em_BonePos(void *obj, s16 bone, f32 *out)
{
    Em_GetBonePos(obj, bone, out);
}

EmBone *Em_GetBone(void *obj, s16 bone)
{
    ObjHead *o = obj;

    return &o->model->bones[bone];
}

EmBone *Em_Bone(void *obj, s16 bone)
{
    return Em_GetBone(obj, bone);
}

f32 *Em_BoneMatrix(void *obj, s16 bone)
{
    ObjHead *o = obj;

    return o->model->bones[bone].local;
}

f32 *Em_BoneMatrixB(void *obj, s16 bone)
{
    return Em_BoneMatrix(obj, bone);
}

/* unk_3DDBA0's .data (0x468850-0x468BA8) */
extern char D_004B0678[];
CardOp gCardOps_0 = {
#include "sub_main/00468850.inc"
};
CardOp gCardOps_1 = {
#include "sub_main/00468900.inc"
};
CardOp gCardOps_2 = {
#include "sub_main/004689B0.inc"
};
CardOp gCardOps_3 = {
#include "sub_main/00468A60.inc"
};
CardOp *gCardOps[12] = {
    &gCardOps_0, &gCardOps_1, &gCardOps_2, &gCardOps_3,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u8 D_00468B40[64] = {
#include "sub_main/00468B40.inc"
};
void (*gCardSteps[10])(CardTask *t) = {
    (void (*)(CardTask *t))func_003DF660, CardTask_NextSlot, CardTask_WriteStep, CardTask_SpaceStep,
    CardTask_SaveStep, CardTask_FormatStep, CardTask_NopStep, CardTask_UpdateStep,
    CardTask_DeleteStep, CardTask_ListStep,
};
