/*
 * Memory card saving and loading, under the card menus (NetMenu, gCardMenu).
 *
 * The save menu functions (SaveMenu_SaveSystem, SaveMenu_LoadCharacter ...) each run over
 * several frames (NetMenu.saveStep): they copy the system data (gSysSave, as a SysFileBody)
 * or a character (a SaveData of gCardSlots) to or from the card image and start the card
 * task, then poll it. A save on the card is a directory of files: icon.sys and the icons,
 * the system file, the three character files and the patch (the card operation's files,
 * gCardOps, CARD_OP_FILE_*); each data file starts with a head and a date stamp
 * (CARD_FILE_*).
 *
 * The card task (gCardTask, a CardTask) runs one step of gCardSteps per frame
 * (CardTask_Update, CARD_STEP_*): poll the ports, read, find, save, format or list. Each
 * step calls the card calls (Card_GetInfo, Card_ReadFile, Card_WriteFile ...), which drive
 * one libmc command at a time (sceMcOpen, sceMcRead ... then sceMcSync) and return
 * CARD_CALL_* or CARD_STATE_*; the step's result for the menus is CARD_OK, CARD_BUSY or a
 * CARD_ERR_* (CardTask_Result). The calls and steps share their failure and retry tails through
 * gotos, which give the game's code layout.
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
#include "scene.h"


extern s32 gCardFileKind;
extern s32 gCardLocked;
extern s16 *gEmPartHpTables[];
extern McDirEntry gCardDirEntry;
extern char gCardDeletePath[];
extern McDirEntry gCardFileAttr;
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
s32 sceMcRead(s32 fd, u8 *buf, s32 size);
s32 sceMcWrite(s32 fd, u8 *buf, s32 size);
s32 sceMcSetFileInfo(s32 port, s32 slot, char *name, McDirEntry *info, s32 flags);
s32 Card_GetDir(CardTask *t);
s32 Card_GetDirEntry(CardTask *t);
s32 Card_ReadFile(CardTask *t);
s32 Card_MkDir(CardTask *t);
s32 Card_CreateFile(CardTask *t);
s32 Card_WriteFile(CardTask *t);
s32 Card_SetFileInfo(CardTask *t);
s32 Card_Delete(CardTask *t);
void CardOp_MakeIconSys(CardTask *t);
char *strcat(char *dst, const char *src);
void CardTask_End(void);
void CardTask_EndIfDone(CardTask *t);
s32 Card_Format(CardTask *t);
s32 Card_DeleteDir(CardTask *t);
extern char gCardDirListFmt[];
extern char gCardPathFmt[];
extern char gCardNameFmt[];
static s32 Card_StubFail(void);
static void Em_ScalePartHp(Em *em, s32 i, s16 hp);
s32 sprintf(char *buf, char *fmt, ...);
void McDate_SetWeekday(McDateTime *date);
void Card_GetDate(void *dst);
static void CardState_SetFlags(s32 a);
s32 SaveBuf_Offset(s32 addr, s32 kind, s32 n);
void SaveMenu_CopySystem(NetMenu *m, u8 *data, s32 save);
void SaveMenu_CopyCharacter(NetMenu *m, s32 data, s32 save, s32 slot);
void CardTask_StartRead(s32 port, u8 *buf);
void CardTask_StartFind(s32 port, u8 *buf, s32 readFound);
void CardTask_StartSave(s32 port, u8 *buf);
void CardTask_SelectFile(s32 file, s32 clear);
s32 CardTask_Result(void);
struct NetMenu;
s32 SaveMenu_ReadBad(struct NetMenu *m);
s32 SaveMenu_SlotUsed(s32 i);
void SaveMenu_ClearCharacter(struct NetMenu *m);
s32 SaveFile_IsCharUsed(NetMenu *m, s32 data, s32 save, s32 slot);
void SaveExtra_CopyFile(s32 a, s32 addr);
void Phrase_TruncateNames(void);
void Options_Apply(void);
void SaveFile_Encrypt(u8 *data);
s32 sceMcGetDir(s32 port, s32 slot, char *name, u32 mode, s32 max, McDirEntry *table);
s32 sceMcMkdir(s32 a, s32 b, char *name);
u16 strlen(char *s);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
EmBone *Em_GetBone(void *obj, s16 bone);
f32 *Em_BoneMatrix(void *obj, s16 bone);

/* The card operations' titles and file names, read-only. */
#pragma rodata on
char gCardSaveTitle1[0x18] = {
#include "sub_main/004B0570.inc"
};
char gCardSaveTitle2[13] = {
#include "sub_main/004B0588.inc"
};
char gCardIconSysName[0xC] = {
#include "sub_main/004B0598.inc"
};
char gCardSaveIconName[0xC] = {
#include "sub_main/004B05A8.inc"
};
char gCardSystemFileName[0x10] = {
#include "sub_main/004B05C0.inc"
};
char gCardCharFileName0[0x10] = {
#include "sub_main/004B05D0.inc"
};
char gCardCharFileName1[0x10] = {
#include "sub_main/004B05E0.inc"
};
char gCardCharFileName2[0x10] = {
#include "sub_main/004B05F0.inc"
};
char gCardPatchFileName[0x10] = {
#include "sub_main/004B0600.inc"
};
char gNetFileTitle1[0x10] = {
#include "sub_main/004B0610.inc"
};
char gNetFileTitle2[0x10] = {
#include "sub_main/004B0620.inc"
};
char gNetFileIcon1[0x10] = {
#include "sub_main/004B0630.inc"
};
char gNetFileIcon2[0x10] = {
#include "sub_main/004B0640.inc"
};
char gNetFileIcon3[0x10] = {
#include "sub_main/004B0650.inc"
};
char gNetFileName[0x10] = {
#include "sub_main/004B0660.inc"
};
/* The empty name of the earlier games' operations' unused files, then at +8 the first game's
 * directory name (gOldMhDirName): one array, as in the game. As an array of its own the name
 * would be aligned to 16 bytes, and the data after it would move. */
char gCardNoName[32] = {
#include "sub_main/004B0670.inc"
};
char gOldMhgDirName[0x10] = {
#include "sub_main/004B0690.inc"
};
#pragma rodata off

/* Returns 0. The boot seeds the random numbers with it (Rand_Seed) once the card load is done. */
u8 Card_GetSeed(void)
{
    return 0;
}

/*
 * Sets the file kind (gCardFileKind) and the card menu's files to write
 * (CARD_FILE_* bits). Static: SaveMenu_LoadSystem keeps its mode in an
 * argument register across the call, as the game's code does, and the
 * compiler does that only for a static function. The card menus call it too: the two files are
 * probably one compilation unit (their data are adjacent), and the build
 * makes the symbol global for their calls.
 */
static void CardState_SetFlags(s32 files)
{
    gCardMenu.filesToWrite = files;
    gCardFileKind = files;
}

/* Stores the files to write (CARD_FILE_* bits) in the card menu's filesToWrite. */
void CardState_SetFlag(s32 files)
{
    gCardMenu.filesToWrite = files;
}

/* Returns the address of a file in the card image at addr: the system file, character file n or
 * the patch file (kind, CARD_IMAGE_FILE_*). Each file takes its size rounded up to 64 bytes, as
 * CardTask_StartSave lays them out. */
s32 SaveBuf_Offset(s32 addr, s32 kind, s32 n)
{
    switch (kind) {
    case CARD_IMAGE_FILE_SYSTEM:
        break;
    case CARD_IMAGE_FILE_CHAR:
        addr += CARD_IMAGE_SYSTEM_SIZE;
        addr += n * CARD_IMAGE_CHAR_SIZE;
        break;
    case CARD_IMAGE_FILE_PATCH:
        addr += CARD_IMAGE_SYSTEM_SIZE + CARD_CHAR_SLOTS * CARD_IMAGE_CHAR_SIZE;
        break;
    }
    return addr;
}

/* With save 0, copies a system file's body (SysFileBody) into gSysSave, with the earlier game's
 * kind when the file has one, and keeps the file's date stamp; then rebuilds the phrase names and
 * applies the options. Otherwise stamps the date, writes the same fields into a cleared body and
 * encrypts the file. */
void SaveMenu_CopySystem(NetMenu *menu, u8 *data, s32 save)
{
    s32 *stamp = (s32 *)(data + CARD_FILE_STAMP);
    SysFileBody *body = (SysFileBody *)((u8 *)stamp + (CARD_FILE_BODY - CARD_FILE_STAMP));
    SysSave *sys = &gSysSave;

    if (save == 0) {
        memcpy(&sys->options, &body->options, sizeof(SysOptions));
        memcpy(sys->netOptions, body->netOptions, sizeof(sys->netOptions));
        sys->bonusUnlocked = body->bonusUnlocked;
        sys->lastSlot = body->lastSlot;
        sys->charSaved = body->charSaved;
        sys->oldGameKind = body->oldGameKind;
        if (body->oldGameKind != OLD_GAME_NONE) {
            gOldGameKind = body->oldGameKind;
        }
        memcpy(sys->eventFlags, body->eventFlags, sizeof(sys->eventFlags));
        memcpy(sys->savedFlags, body->savedFlags, sizeof(sys->savedFlags));
        memcpy(sys->phrases, body->phrases, sizeof(sys->phrases));
        menu->fileDates[0][0] = stamp[0];
        menu->fileDates[0][1] = stamp[1];
        Phrase_TruncateNames();
        Options_Apply();
        return;
    }
    Card_GetDate(stamp);
    menu->readDate[0] = stamp[0];
    menu->readDate[1] = stamp[1];
    memset(body, 0, CARD_FILE_SYSTEM_BODY_SIZE);
    memcpy(&body->options, &sys->options, sizeof(SysOptions));
    memcpy(body->netOptions, sys->netOptions, sizeof(sys->netOptions));
    body->bonusUnlocked = sys->bonusUnlocked;
    body->lastSlot = sys->lastSlot;
    body->charSaved = sys->charSaved;
    body->oldGameKind = gOldGameKind;
    memcpy(body->eventFlags, sys->eventFlags, sizeof(sys->eventFlags));
    memcpy(body->savedFlags, sys->savedFlags, sizeof(sys->savedFlags));
    memcpy(body->phrases, sys->phrases, sizeof(sys->phrases));
    SaveFile_Encrypt(data);
}

/* Saves the system file in two steps: builds it in the card image and starts the card save of its
 * file, then polls. Returns SAVE_STEP_DONE when saved (the new stamp is kept), SAVE_STEP_FAILED
 * on an error, SAVE_STEP_RUNNING while running. */
s32 SaveMenu_SaveSystem(NetMenu *menu)
{
    s32 result;

    switch (menu->saveStep) {
    case 0:
        CardState_SetFlags(CARD_FILE_SYSTEM);
        SaveMenu_CopySystem(menu, (u8 *)SaveBuf_Offset(menu->imageBuf, CARD_IMAGE_FILE_SYSTEM, 0), 1);
        CardTask_StartSave(menu->slot, (u8 *)menu->imageBuf);
        CardTask_SelectFile(CARD_OP_FILE_SYSTEM, 0);
        menu->saveStep++;
        break;
    case 1:
        result = CardTask_Result();
        menu->result[menu->slot] = result;
        switch (result) {
        case CARD_OK:
            menu->fileDates[0][0] = menu->readDate[0];
            menu->fileDates[0][1] = menu->readDate[1];
            return SAVE_STEP_DONE;
        case CARD_BUSY:
            break;
        case CARD_ERR_NO_FILE:
        case CARD_ERR_NO_SPACE:
        case CARD_ERR_NO_CARD:
        default:
            return SAVE_STEP_FAILED;
        }
        break;
    }
    return SAVE_STEP_RUNNING;
}

/* Loads the system file in two steps: mode 0 reads it at once, other modes clear the buffer and
 * find it first. Returns SAVE_STEP_DONE when loaded, SAVE_STEP_FAILED on an error (bad data
 * gives CARD_ERR_BAD_DATA), SAVE_STEP_RUNNING while running. */
s32 SaveMenu_LoadSystem(NetMenu *menu, s32 mode)
{
    s32 result;

    switch (menu->saveStep) {
    case 0:
        CardState_SetFlags(CARD_FILE_SYSTEM);
        if (mode == 0) {
            CardTask_StartRead(menu->slot, menu->readBuf);
        } else {
            CardTask_StartFind(menu->slot, menu->readBuf, 1);
        }
        CardTask_SelectFile(CARD_OP_FILE_SYSTEM, 0);
        menu->saveStep++;
        break;
    case 1:
        result = CardTask_Result();
        menu->result[menu->slot] = result;
        switch (result) {
        case CARD_OK:
            if (SaveMenu_ReadBad(menu) != 0) {
                menu->result[menu->slot] = CARD_ERR_BAD_DATA;
                return SAVE_STEP_FAILED;
            }
            SaveMenu_CopySystem(menu, menu->readBuf, 0);
            return SAVE_STEP_DONE;
        case CARD_BUSY:
            break;
        default:
            return SAVE_STEP_FAILED;
        }
        break;
    }
    return SAVE_STEP_RUNNING;
}

/* Saves the patch file in two steps: copies it into the card image (SaveExtra_CopyFile) and saves
 * its file. Returns SAVE_STEP_DONE, SAVE_STEP_FAILED or SAVE_STEP_RUNNING. */
s32 SaveMenu_SaveExtra(NetMenu *menu)
{
    s32 result;

    switch (menu->saveStep) {
    case 0:
        CardState_SetFlags(CARD_FILE_PATCH);
        SaveExtra_CopyFile(1, SaveBuf_Offset(menu->imageBuf, CARD_IMAGE_FILE_PATCH, 0));
        CardTask_StartSave(menu->slot, (u8 *)menu->imageBuf);
        CardTask_SelectFile(CARD_OP_FILE_PATCH, 0);
        menu->saveStep++;
        break;
    case 1:
        result = CardTask_Result();
        menu->result[menu->slot] = result;
        switch (result) {
        case CARD_OK:
            menu->fileDates[0][0] = menu->readDate[0];
            menu->fileDates[0][1] = menu->readDate[1];
            return SAVE_STEP_DONE;
        case CARD_BUSY:
            break;
        case CARD_ERR_NO_FILE:
        case CARD_ERR_NO_SPACE:
        case CARD_ERR_NO_CARD:
        default:
            return SAVE_STEP_FAILED;
        }
        break;
    }
    return SAVE_STEP_RUNNING;
}

/* Copies the loaded character (gSysSave.save) into save slot slot of gCardSlots. */
void Save_CopyToSlot(s32 slot)
{
    SaveData *dst;

    dst = gCardSlots;
    dst += slot;
    *dst = gSysSave.save;
}

/* With save 0, copies a character file's body into save slot slot and keeps its date stamp.
 * Otherwise stamps the date, copies the slot into a cleared body and encrypts the file. */
void SaveMenu_CopyCharacter(NetMenu *menu, s32 data, s32 save, s32 slot)
{
    s32 *stamp = (s32 *)(data + CARD_FILE_STAMP);
    SaveData *body = (SaveData *)((u8 *)stamp + (CARD_FILE_BODY - CARD_FILE_STAMP));
    SaveData *chara;

    chara = gCardSlots;
    chara += slot;

    if (save == 0) {
        memcpy(chara, body, sizeof(SaveData));
        menu->readDate[0] = stamp[0];
        menu->readDate[1] = stamp[1];
        return;
    }
    Card_GetDate(stamp);
    menu->readDate[0] = stamp[0];
    menu->readDate[1] = stamp[1];
    memset(body, 0, CARD_FILE_CHAR_BODY_SIZE);
    memcpy(body, chara, sizeof(SaveData));
    SaveFile_Encrypt((u8 *)data);
}

/* Saves the character file of the menu's slot (charIndex) in two steps. Returns SAVE_STEP_DONE
 * when saved (its stamp becomes the slot's), SAVE_STEP_FAILED or SAVE_STEP_RUNNING. */
s32 SaveMenu_SaveCharacter(NetMenu *menu)
{
    s32 result;
    s32 slot;

    switch (menu->saveStep) {
    case 0:
        menu->fileKind = CARD_FILE_CHAR(menu->charIndex);
        slot = menu->charIndex;
        SaveMenu_CopyCharacter(menu, SaveBuf_Offset(menu->imageBuf, CARD_IMAGE_FILE_CHAR, slot), 1, slot);
        CardTask_StartSave(menu->slot, (u8 *)menu->imageBuf);
        CardTask_SelectFile(CARD_OP_FILE_CHAR(slot), 0);
        menu->saveStep++;
        break;
    case 1:
        result = CardTask_Result();
        menu->result[menu->slot] = result;
        switch (result) {
        case CARD_OK:
            menu->fileDates[menu->charIndex + 1][0] = menu->readDate[0];
            menu->fileDates[menu->charIndex + 1][1] = menu->readDate[1];
            return SAVE_STEP_DONE;
        case CARD_BUSY:
            break;
        case CARD_ERR_NO_FILE:
        case CARD_ERR_NO_SPACE:
        case CARD_ERR_NO_CARD:
        default:
            return SAVE_STEP_FAILED;
        }
        break;
    }
    return SAVE_STEP_RUNNING;
}

/* Reads the character file of the menu's slot (charIndex) into its save slot in two steps, and
 * marks the slot CHAR_READ_OK, or CHAR_READ_BAD with the slot cleared. Returns SAVE_STEP_DONE,
 * SAVE_STEP_FAILED or SAVE_STEP_RUNNING. charRead is written through slot + 1, which the
 * function keeps in a register as the game's code does. */
s32 SaveMenu_LoadCharacter(NetMenu *menu)
{
    s32 result;
    s32 slot;

    switch (menu->saveStep) {
    case 0:
        menu->fileKind = CARD_FILE_CHAR(menu->charIndex);
        slot = menu->charIndex;
        CardTask_StartRead(menu->slot, menu->readBuf);
        CardTask_SelectFile(CARD_OP_FILE_CHAR(slot), 0);
        menu->saveStep++;
        break;
    case 1:
        result = CardTask_Result();
        menu->result[menu->slot] = result;
        switch (result) {
        case CARD_OK:
            slot = menu->charIndex + 1;
            if (SaveMenu_ReadBad(menu) != 0) {
                menu->result[menu->slot] = CARD_ERR_BAD_DATA;
                menu->charRead[slot - 1] = CHAR_READ_BAD;
                SaveMenu_ClearCharacter(menu);
                return SAVE_STEP_FAILED;
            }
            menu->charRead[slot - 1] = CHAR_READ_OK;
            SaveMenu_CopyCharacter(menu, (s32)menu->readBuf, 0, menu->charIndex);
            return SAVE_STEP_DONE;
        case CARD_BUSY:
            break;
        case CARD_ERR_NO_FILE:
        case CARD_ERR_NO_SPACE:
        case CARD_ERR_NO_CARD:
        default:
            return SAVE_STEP_FAILED;
        }
        break;
    }
    return SAVE_STEP_RUNNING;
}

/* Returns SaveData.used of the character file in data: nonzero when the slot holds a hunter. The
 * other parameters are unused. */
s32 SaveFile_IsCharUsed(NetMenu *menu, s32 data, s32 save, s32 slot)
{
    return ((SaveData *)(data + CARD_FILE_BODY))->used;
}

/* Like SaveMenu_LoadCharacter, but only records whether the file holds a hunter (charRead
 * CHAR_READ_OK or CHAR_READ_NONE, CHAR_READ_BAD when bad). Returns SAVE_STEP_DONE,
 * SAVE_STEP_FAILED or SAVE_STEP_RUNNING. */
s32 SaveMenu_CheckCharacter(NetMenu *menu)
{
    s32 result;
    s32 slot;

    switch (menu->saveStep) {
    case 0:
        menu->fileKind = CARD_FILE_CHAR(menu->charIndex);
        slot = menu->charIndex;
        CardTask_StartRead(menu->slot, menu->readBuf);
        CardTask_SelectFile(CARD_OP_FILE_CHAR(slot), 0);
        menu->saveStep++;
        break;
    case 1:
        result = CardTask_Result();
        menu->result[menu->slot] = result;
        switch (result) {
        case CARD_OK:
            slot = menu->charIndex + 1;
            if (SaveMenu_ReadBad(menu) != 0) {
                menu->result[menu->slot] = CARD_ERR_BAD_DATA;
                menu->charRead[slot - 1] = CHAR_READ_BAD;
                SaveMenu_ClearCharacter(menu);
                return SAVE_STEP_FAILED;
            }
            if (SaveFile_IsCharUsed(menu, (s32)menu->readBuf, 0, menu->charIndex) == 1) {
                menu->charRead[slot - 1] = CHAR_READ_OK;
            } else {
                menu->charRead[slot - 1] = CHAR_READ_NONE;
            }
            return SAVE_STEP_DONE;
        case CARD_BUSY:
            break;
        case CARD_ERR_NO_FILE:
        case CARD_ERR_NO_SPACE:
        case CARD_ERR_NO_CARD:
        default:
            return SAVE_STEP_FAILED;
        }
        break;
    }
    return SAVE_STEP_RUNNING;
}

/* 1 once the system data has been saved (gSysSettings.systemSaved), else 0. */
s32 Card_IsSystemSaved(void)
{
    return gSysSettings.systemSaved == 1;
}

/* 1 when the card menu has loaded its texture (gCardMenu.menuTexOn), else 0. */
s32 CardMenu_IsTexOn(void)
{
    return gCardMenu.menuTexOn == 1;
}

/* 1 when character slot i was read from the card without bad data, else 0. */
s32 Card_SlotReady(s32 i)
{
    /* menuBytes[i + 0x9D] is gCardMenu.charRead[i] (charRead is at 0x9D), read as a byte of the
     * menu: written as the field, the compiler forms the address another way. */
    u8 *menuBytes = (u8 *)&gCardMenu;

    if (gCardMenu.charRead[i] & CHAR_READ_BAD) {
        return 0;
    }
    if (menuBytes[i + 0x9D] != 0) {
        return 1;
    }
    return 0;
}

/* The number of character slots read from the card without bad data. */
s32 Card_ReadyCount(void)
{
    s32 n = 0;
    s32 i;
    /* menuBytes[i + 0x9D] is gCardMenu.charRead[i] (charRead is at 0x9D), read as a byte of the
     * menu: written as the field, the compiler forms the address another way. */
    u8 *menuBytes = (u8 *)&gCardMenu;

    for (i = 0; i < CARD_CHAR_SLOTS; i++) {
        if (menuBytes[i + 0x9D] == CHAR_READ_OK) {
            n++;
        }
    }
    return n;
}

/* Takes the sound library's server lock once (gCardLocked), so that the card calls don't run beside
 * it. */
void Card_Lock(void)
{
    if (gCardLocked == 0) {
        func_161EC0();
        gCardLocked = 1;
    }
}

/* Releases the lock Card_Lock took. */
void Card_Unlock(void)
{
    if (gCardLocked != 0) {
        func_161ED8();
        gCardLocked = 0;
    }
}

/* Clears gCardTask, sets the two ports and calls sceMcInit until it succeeds. */
void Card_Init(void)
{
    memset(&gCardTask, 0, sizeof(CardTask));
    gCardTask.portCount = CARD_PORTS;
    while (sceMcInit() < 0) {
    }
}

/* Polls sceMcSync without waiting and keeps the finished command and its result. Returns -1 while a
 * command runs, else 0. */
s32 Card_Sync(CardTask *task)
{
    task->syncCmd = 0;
    task->syncResult = 0;
    return -(sceMcSync(1, &task->syncCmd, &task->syncResult) == 0);
}

/* Calls sceMcGetInfo on the task's port, with 3 retries, and sets cardState and cardFree. Returns
 * the state (CARD_STATE_*), or CARD_CALL_BUSY while busy. */
s32 Card_GetInfo(CardTask *task)
{
    s32 state;

    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        task->subStep++;
        task->retries = 3;
    case 1:
        if (sceMcGetInfo(task->port, 0, &task->type, &task->free, &task->format) < 0) {
            if (--task->retries > 0) {
                break;
            }
        none:
            task->subStep = 0;
            task->cardState[task->port] = CARD_STATE_NONE;
            task->cardFree[task->port] = 0;
            return CARD_STATE_NONE;
        }
        task->subStep++;
        break;
    case 2:
        if (Card_Sync(task) < 0) {
            break;
        }
        switch (task->syncResult) {
        case 0:
            state = CARD_STATE_SAME;
        check:
            if (task->type != MC_TYPE_PS2) {
                goto none;
            }
            if (task->format == 0) {
                goto unformatted;
            }
            task->cardState[task->port] = state;
            task->cardFree[task->port] = task->free;
            break;
        case -1:
            task->changed |= 1 << task->port;
            state = CARD_STATE_NEW;
            goto check;
        case -2:
            if (task->type != MC_TYPE_PS2) {
                goto none;
            }
            task->changed |= 1 << task->port;
        unformatted:
            if (--task->retries > 0) {
            retry:
                task->subStep = 1;
                return CARD_CALL_BUSY;
            }
            task->cardState[task->port] = CARD_STATE_UNFORMATTED;
            task->cardFree[task->port] = 8000; /* clusters, a whole card */
            break;
        default:
            if (--task->retries > 0) {
                goto retry;
            }
            goto none;
        }
        task->subStep = 0;
        return task->cardState[task->port];
    }
    return CARD_CALL_BUSY;
}

/* Calls sceMcGetDir on the task's name for one entry (gCardDirEntry). Returns CARD_ENTRY_*, or
 * CARD_CALL_BUSY while busy. */
s32 Card_GetDirEntry(CardTask *task)
{
    s32 result;

    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (sceMcGetDir(task->port, 0, task->name, 0, 1, &gCardDirEntry) < 0) {
            return CARD_ENTRY_MISSING;
        }
        task->subStep++;
        break;
    case 1:
        if (Card_Sync(task) < 0) {
            break;
        }
        task->subStep = 0;
        result = task->syncResult;
        if (result == 1) {
            return CARD_ENTRY_FOUND;
        }
        if (result == -5 || result < -10) {
            return CARD_ENTRY_NO_CARD;
        }
        if (result == -6) {
            return CARD_ENTRY_FAILED;
        }
        return CARD_ENTRY_MISSING;
    }
    return CARD_CALL_BUSY;
}

/* Opens the task's file to read (sceMcOpen), reads size bytes into buf (sceMcRead) and closes it.
 * Returns CARD_CALL_DONE, CARD_CALL_FAILED, or CARD_CALL_BUSY while busy (as the calls below). */
s32 Card_ReadFile(CardTask *task)
{
    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (sceMcOpen(task->port, 0, task->name, MC_OPEN_READ) < 0) {
        fail:
            task->subStep = 0;
            return CARD_CALL_FAILED;
        }
        task->subStep++;
        break;
    case 1:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->syncResult < 0) {
            goto fail;
        }
        task->fd = task->syncResult;
        if (sceMcRead(task->fd, task->buf, task->size) < 0) {
            goto fail;
        }
        task->subStep++;
        break;
    case 2:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->syncResult < 0) {
            goto fail;
        }
        if (sceMcClose(task->fd) < 0) {
            goto fail;
        }
        task->subStep++;
        break;
    case 3:
        if (Card_Sync(task) < 0) {
            break;
        }
        task->subStep = 0;
        return task->syncResult < 0;
    }
    return CARD_CALL_BUSY;
}

/* Makes the task's directory (sceMcMkdir); one that is already there counts as made. */
s32 Card_MkDir(CardTask *task)
{
    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (sceMcMkdir(task->port, 0, task->name) < 0) {
            return CARD_CALL_FAILED;
        }
        task->subStep++;
        break;
    case 1:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->syncResult == -4) {
            task->syncResult = 0;
        }
        task->subStep = 0;
        return task->syncResult < 0;
    }
    return CARD_CALL_BUSY;
}

/* Creates the task's file (sceMcOpen with create) and closes the task's fd. */
s32 Card_CreateFile(CardTask *task)
{
    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (sceMcOpen(task->port, 0, task->name, MC_OPEN_CREATE) < 0) {
        fail:
            task->subStep = 0;
            return CARD_CALL_FAILED;
        }
        task->subStep++;
        break;
    case 1:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->syncResult < 0) {
            goto fail;
        }
        if (sceMcClose(task->fd) < 0) {
            goto fail;
        }
        task->subStep++;
        break;
    case 2:
        if (Card_Sync(task) < 0) {
            break;
        }
        task->subStep = 0;
        return task->syncResult < 0;
    }
    return CARD_CALL_BUSY;
}

/* Opens the task's file to write (sceMcOpen), writes size bytes from buf (sceMcWrite) and closes
 * it. */
s32 Card_WriteFile(CardTask *task)
{
    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (sceMcOpen(task->port, 0, task->name, MC_OPEN_WRITE) < 0) {
        fail:
            task->subStep = 0;
            return CARD_CALL_FAILED;
        }
        task->subStep++;
        break;
    case 1:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->syncResult < 0) {
            goto fail;
        }
        task->fd = task->syncResult;
        if (sceMcWrite(task->fd, task->buf, task->size) < 0) {
            goto fail;
        }
        task->subStep++;
        break;
    case 2:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->syncResult < 0) {
            goto fail;
        }
        if (sceMcClose(task->fd) < 0) {
            goto fail;
        }
        task->subStep++;
        break;
    case 3:
        if (Card_Sync(task) < 0) {
            break;
        }
        task->subStep = 0;
        return task->syncResult < 0;
    }
    return CARD_CALL_BUSY;
}

/* When copyProtect is set, reads the file's entry (sceMcGetDir), sets its no-copy attribute and
 * writes it back (sceMcSetFileInfo). Without copyProtect it is done at once. */
s32 Card_SetFileInfo(CardTask *task)
{
    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->copyProtect == 0) {
            return CARD_CALL_DONE;
        }
        if (sceMcGetDir(task->port, 0, task->name, 0, 1, &gCardFileAttr) < 0) {
            return CARD_CALL_FAILED;
        }
        task->subStep++;
        break;
    case 1:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->syncResult < 0) {
            return CARD_CALL_FAILED;
        }
        gCardFileAttr.attr |= MC_ATTR_NO_COPY;
        if (sceMcSetFileInfo(task->port, 0, task->name, &gCardFileAttr, MC_INFO_ATTR) < 0) {
            task->subStep = 0;
            return CARD_CALL_FAILED;
        }
        task->subStep++;
        break;
    case 2:
        if (Card_Sync(task) < 0) {
            break;
        }
        task->subStep = 0;
        return task->syncResult < 0;
    }
    return CARD_CALL_BUSY;
}

/* Deletes the task's file (sceMcDelete). */
s32 Card_Delete(CardTask *task)
{
    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (sceMcDelete(task->port, 0, task->name) < 0) {
            return CARD_CALL_FAILED;
        }
        task->subStep++;
        break;
    case 1:
        if (Card_Sync(task) < 0) {
            break;
        }
        task->subStep = 0;
        return task->syncResult < 0;
    }
    return CARD_CALL_BUSY;
}

/* Formats the card in the task's port (sceMcFormat). */
s32 Card_Format(CardTask *task)
{
    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (sceMcFormat(task->port, 0) < 0) {
            return CARD_CALL_FAILED;
        }
        task->subStep++;
        break;
    case 1:
        if (Card_Sync(task) < 0) {
            break;
        }
        task->subStep = 0;
        return task->syncResult < 0;
    }
    return CARD_CALL_BUSY;
}

/* Returns CARD_CALL_FAILED: a card call that always fails. */
static s32 Card_StubFail(void)
{
    return CARD_CALL_FAILED;
}

/* The path formats: a directory's entries, a file in a directory, a name alone. */
INCLUDE_RODATA("asm/sub_main/nonmatchings/card_task", gCardDirListFmt);
INCLUDE_RODATA("asm/sub_main/nonmatchings/card_task", gCardPathFmt);
INCLUDE_RODATA("asm/sub_main/nonmatchings/card_task", gCardNameFmt);

/* Deletes the task's directory: with deleteFiles, first each of its files but the "." entries
 * (sceMcGetDir and sceMcDelete one by one), then the directory itself. */
s32 Card_DeleteDir(CardTask *task)
{
    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->deleteFiles == 0) {
            goto dir;
        }
        task->entryNo = 0;
    next:
        sprintf(gCardDeletePath, gCardDirListFmt, task->name);
        task->subStep = 1;
    case 1:
        if (sceMcGetDir(task->port, 0, gCardDeletePath, task->entryNo, 1, &gCardDirEntry) < 0) {
        fail:
            task->subStep = 0;
            return CARD_CALL_FAILED;
        }
        task->subStep++;
        break;
    case 2:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->syncResult < 0) {
            goto fail;
        }
        if (task->syncResult == 1) {
            task->entryNo++;
            if (gCardDirEntry.name[0] == '.') {
                goto next;
            }
            sprintf(gCardDeletePath, gCardPathFmt, task->name, (char *)gCardDirEntry.name);
            task->readFound = 0;
        } else {
        dir:
            sprintf(gCardDeletePath, gCardNameFmt, task->name);
            task->readFound = 1;
        }
        if (sceMcDelete(task->port, 0, gCardDeletePath) < 0) {
            goto fail;
        }
        task->subStep = 3;
        break;
    case 3:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (task->syncResult < 0) {
            goto fail;
        }
        task->subStep = 0;
        if (task->readFound == 0) {
            goto next;
        }
        return CARD_CALL_DONE;
    }
    return CARD_CALL_BUSY;
}

/* Lists the task's directory (sceMcGetDir, up to 0x100 entries into buf). Returns CARD_CALL_DONE
 * with the count in entryNo, CARD_CALL_NO_DIR when there is no such directory, CARD_CALL_FAILED,
 * or CARD_CALL_BUSY while busy. */
s32 Card_GetDir(CardTask *task)
{
    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (sceMcGetDir(task->port, 0, task->name, 0, 0x100, (McDirEntry *)task->buf) < 0) {
            return CARD_CALL_FAILED;
        }
        task->subStep++;
        break;
    case 1:
        if (Card_Sync(task) < 0) {
            break;
        }
        task->subStep = 0;
        if (task->syncResult < 0) {
            task->entryNo = 0;
            return CARD_CALL_NO_DIR;
        }
        task->entryNo = task->syncResult;
        return CARD_CALL_DONE;
    }
    return CARD_CALL_BUSY;
}

/* Selects card operation i (CARD_OP_*), makes the task idle and takes the operation's system file
 * name as the directory's name. */
void CardTask_Begin(s32 i)
{
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.state = CARD_STEP_IDLE;
    gCardTask.op = i;
    strcpy(gCardTask.dir, gCardOps[i]->files[CARD_OP_FILE_SYSTEM].name);
}

/* Runs the current step, gCardSteps[state], once under the card lock. */
void CardTask_Update(void)
{
    Card_Lock();
    gCardSteps[gCardTask.state](&gCardTask);
    Card_Unlock();
}

/* Makes the task idle and clears writing. */
void CardTask_End(void)
{
    gCardTask.state = CARD_STEP_IDLE;
    gCardTask.writing = 0;
}

/* The idle step (gCardSteps[0]): does nothing. */
void CardTask_IdleStep(void)
{
}

/* Ends the task once its result is no longer CARD_BUSY. */
void CardTask_EndIfDone(CardTask *task)
{
    if (task->result != CARD_BUSY) {
        CardTask_End();
    }
}

/* Starts polling the ports from port 0 (CardTask_PollStep). The result stays CARD_BUSY: the callers
 * read Card_PortState. */
void CardTask_StartPoll(void)
{
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.state = CARD_STEP_POLL;
    gCardTask.result = CARD_BUSY;
    gCardTask.port = 0;
}

/* Polls Card_GetInfo on the current port and moves to the next port when it finishes. Never ends.
 */
void CardTask_PollStep(CardTask *task)
{
    if (Card_GetInfo(task) >= 0) {
        if (++task->port >= task->portCount) {
            task->port = 0;
        }
    }
}

/* Starts reading the operation's system file in port into buf (CardTask_ReadStep);
 * CardTask_SelectFile may then pick another file. */
void CardTask_StartRead(s32 port, u8 *buf)
{
    CardOp *cardOp = gCardOps[gCardTask.op];

    gCardTask.state = CARD_STEP_READ;
    gCardTask.result = CARD_BUSY;
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.port = port;
    gCardTask.buf = buf;
    sprintf(gCardTask.name, gCardPathFmt, gCardTask.dir, gCardTask.dir);
    gCardTask.size = cardOp->files[CARD_OP_FILE_SYSTEM].size;
}

/* Gets the card's state, finds the file and reads it. Result: CARD_OK, CARD_ERR_NO_CARD,
 * CARD_ERR_UNFORMATTED, CARD_ERR_NO_FILE or CARD_ERR_FAILED. */
void CardTask_ReadStep(CardTask *task)
{
    switch (task->step) {
    case 0:
        switch (Card_GetInfo(task)) {
        case CARD_STATE_NONE:
            task->result = CARD_ERR_NO_CARD;
            break;
        case CARD_STATE_UNFORMATTED:
            task->result = CARD_ERR_UNFORMATTED;
            break;
        case CARD_STATE_SAME:
        case CARD_STATE_NEW:
            task->step++;
            break;
        }
        break;
    case 1:
        switch (Card_GetDirEntry(task)) {
        case CARD_ENTRY_FOUND:
            task->step++;
            break;
        case CARD_ENTRY_MISSING:
            task->step = 0;
            task->result = CARD_ERR_NO_FILE;
            break;
        case CARD_ENTRY_NO_CARD:
            task->step = 0;
            task->result = CARD_ERR_NO_CARD;
            break;
        case CARD_ENTRY_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    case 2:
        switch (Card_ReadFile(task)) {
        case CARD_CALL_DONE:
            task->step = 0;
            task->result = CARD_OK;
            break;
        case CARD_CALL_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    }
    CardTask_EndIfDone(task);
}

/* Clears buf for the system file's size and starts CardTask_FindStep on port; with readFound, a
 * file found is read too. */
void CardTask_StartFind(s32 port, u8 *buf, s32 readFound)
{
    CardOp *cardOp = gCardOps[gCardTask.op];

    gCardTask.state = CARD_STEP_FIND;
    gCardTask.result = CARD_BUSY;
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.port = port;
    gCardTask.buf = buf;
    gCardTask.readFound = readFound;
    sprintf(gCardTask.name, gCardPathFmt, gCardTask.dir, gCardTask.dir);
    gCardTask.size = cardOp->files[CARD_OP_FILE_SYSTEM].size;
    memset(gCardTask.buf, 0, gCardTask.size);
}

/* Looks for the file: found, CARD_OK (after reading it with readFound). Missing: when its directory
 * exists, CARD_OK; else CARD_ERR_NO_FILE when the card has the operation's blocks free,
 * CARD_ERR_NO_SPACE when not. */
void CardTask_FindStep(CardTask *task)
{
    CardOp *cardOp = gCardOps[task->op];

    switch (task->step) {
    case 0:
        switch (Card_GetInfo(task)) {
        case CARD_STATE_NONE:
            task->result = CARD_ERR_NO_CARD;
            break;
        case CARD_STATE_UNFORMATTED:
            task->result = CARD_ERR_UNFORMATTED;
            break;
        case CARD_STATE_SAME:
        case CARD_STATE_NEW:
            task->step++;
            break;
        }
        break;
    case 1:
        switch (Card_GetDirEntry(task)) {
        case CARD_ENTRY_FOUND:
            if (task->readFound == 0) {
                task->step = 0;
                task->result = CARD_OK;
            } else {
                task->step++;
            }
            break;
        case CARD_ENTRY_MISSING:
            task->step += 2;
            strcpy(task->name, task->dir);
            task->size = 0;
            break;
        case CARD_ENTRY_NO_CARD:
            task->step = 0;
            task->result = CARD_ERR_NO_CARD;
            break;
        case CARD_ENTRY_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    case 2:
        switch (Card_ReadFile(task)) {
        case CARD_CALL_DONE:
            task->step = 0;
            task->result = CARD_OK;
            break;
        case CARD_CALL_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    case 3:
        switch (Card_GetDirEntry(task)) {
        case CARD_ENTRY_FOUND:
            task->step = 0;
            task->result = CARD_OK;
            break;
        case CARD_ENTRY_MISSING:
            task->step = 0;
            if (task->cardFree[task->port] >= cardOp->blocks) {
                task->result = CARD_ERR_NO_FILE;
            } else {
                task->result = CARD_ERR_NO_SPACE;
            }
            break;
        case CARD_ENTRY_NO_CARD:
            task->step = 0;
            task->result = CARD_ERR_NO_CARD;
            break;
        case CARD_ENTRY_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    }
    CardTask_EndIfDone(task);
}

/* Lays the operation's data files out one after another in buf (each rounded up to 64 bytes), marks
 * all but the system file as not to write, and starts CardTask_SaveStep on port. */
void CardTask_StartSave(s32 port, u8 *buf)
{
    CardOp *cardOp = gCardOps[gCardTask.op];
    s32 i;
    CardTask *task = &gCardTask;

    gCardTask.result = CARD_BUSY;
    gCardTask.port = port;
    gCardTask.state = CARD_STEP_SAVE;
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.fileNo = 0;
    gCardTask.mask = 0;
    gCardTask.buf = buf;
    for (i = CARD_OP_FILE_SYSTEM; i < CARD_OP_FILES; i++) {
        cardOp->files[i].ptr = buf;
        buf += (cardOp->files[i].size + 63) / 64 * 64;
        if (i != CARD_OP_FILE_SYSTEM) {
            task->mask |= 1 << i;
        }
    }
    task->writing = 1;
}

/* Writes the save: icon.sys, the directory and its copy protection, then the files 0-8. Skips the
 * masked and unused ones, deletes and recreates a file whose size changed, and keeps an existing
 * file of the same size whose fileFlag is 1. Result: CARD_OK or an error. */
void CardTask_SaveStep(CardTask *task)
{
    CardOp *cardOp = gCardOps[task->op];

    switch (task->step) {
    case 0:
        switch (Card_GetInfo(task)) {
        case CARD_STATE_NONE:
            task->result = CARD_ERR_NO_CARD;
            break;
        case CARD_STATE_UNFORMATTED:
            task->result = CARD_ERR_UNFORMATTED;
            break;
        case CARD_STATE_SAME:
        case CARD_STATE_NEW:
            task->step++;
            CardOp_MakeIconSys(task);
            sprintf(task->name, gCardNameFmt, task->dir);
            task->copyProtect = cardOp->copyProtect;
            break;
        }
        break;
    case 1:
        switch (Card_MkDir(task)) {
        case CARD_CALL_DONE:
            task->step++;
            task->fileNo = 0;
            break;
        case CARD_CALL_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    case 2:
        switch (Card_SetFileInfo(task)) {
        case CARD_CALL_DONE:
            task->step++;
            break;
        case CARD_CALL_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    case 3:
    file:
        if ((task->fileFlag = cardOp->files[task->fileNo].flag) == 0) {
            goto next;
        }
        sprintf(task->name, "%s/", task->dir);
        if (task->fileNo == CARD_OP_FILE_SYSTEM) {
            strcat(task->name, task->dir);
        } else {
            strcat(task->name, cardOp->files[task->fileNo].name);
        }
        task->buf = cardOp->files[task->fileNo].ptr;
        task->size = cardOp->files[task->fileNo].size;
        task->step++;
    case 4:
        switch (Card_GetDirEntry(task)) {
        case CARD_ENTRY_FOUND:
            if (task->mask & (1 << task->fileNo)) {
                goto next;
            }
            if (gCardDirEntry.size != task->size) {
                task->step++;
                break;
            }
            if (task->fileFlag == CARD_OPFILE_KEEP) {
                goto next;
            }
            task->step += 3;
            break;
        case CARD_ENTRY_MISSING:
            task->step += 2;
            break;
        case CARD_ENTRY_NO_CARD:
            task->step = 0;
            task->result = CARD_ERR_NO_CARD;
            break;
        case CARD_ENTRY_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    case 5:
        switch (Card_Delete(task)) {
        case CARD_CALL_DONE:
            task->step++;
            break;
        case CARD_CALL_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    case 6:
        switch (Card_CreateFile(task)) {
        case CARD_CALL_DONE:
            task->step++;
            break;
        case CARD_CALL_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    case 7:
        switch (Card_WriteFile(task)) {
        case CARD_CALL_DONE:
        next:
            if (++task->fileNo < CARD_OP_FILES) {
                task->step = 3;
                goto file;
            }
            task->step = 0;
            task->result = CARD_OK;
            break;
        case CARD_CALL_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    }
    CardTask_EndIfDone(task);
}

/* FAKEMATCH: file is a copy of the parameter, of the same type: with the parameter used directly,
 * the compiler swaps the registers of the two parameters and moves the file >= CARD_OP_FILE_SYSTEM test later. */
void CardTask_SelectFile(s32 fileArg, s32 clear)
{
    s32 file = fileArg;
    CardTask *task = &gCardTask;
    CardOp *cardOp = gCardOps[gCardTask.op];
    s32 i;

    if (file >= CARD_OP_FILE_SYSTEM) {
        sprintf(task->name, gCardPathFmt, task->dir, cardOp->files[file].name);
        task->size = cardOp->files[file].size;
        task->mask = 0;
        for (i = CARD_OP_FILE_SYSTEM; i < CARD_OP_FILES; i++) {
            if (file != i) {
                task->mask |= 1 << i;
            }
        }
        if (clear) {
            memset(task->buf, 0, task->size);
        }
    }
}

/* Starts formatting the card in port (CardTask_FormatStep). */
void CardTask_StartFormat(s32 port)
{
    gCardTask.port = port;
    gCardTask.step = 0;
    gCardTask.state = CARD_STEP_FORMAT;
    gCardTask.result = CARD_BUSY;
    gCardTask.writing = 1;
    gCardTask.subStep = 0;
}

/* Formats the card. Result: CARD_OK or CARD_ERR_FAILED. */
void CardTask_FormatStep(CardTask *task)
{
    switch (task->step) {
    case 0:
        switch (Card_Format(task)) {
        case CARD_CALL_DONE:
            task->result = CARD_OK;
            break;
        case CARD_CALL_FAILED:
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    }
    CardTask_EndIfDone(task);
}

/* A step nothing starts: fails at once with CARD_ERR_FAILED. */
void CardTask_StubStep(CardTask *task)
{
    switch (task->step) {
    case 0:
        switch (Card_StubFail()) {
        case 0:
            task->result = CARD_OK;
            break;
        case 1:
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    }
    CardTask_EndIfDone(task);
}

/* A step nothing starts: gets the card's state, finds the task's entry and deletes it with
 * Card_DeleteDir. Result: CARD_OK, CARD_ERR_NO_CARD, CARD_ERR_UNFORMATTED, CARD_ERR_NO_FILE or
 * CARD_ERR_FAILED. */
void CardTask_DeleteSaveStep(CardTask *task)
{
    switch (task->step) {
    case 0:
        switch (Card_GetInfo(task)) {
        case CARD_STATE_NONE:
            task->result = CARD_ERR_NO_CARD;
            break;
        case CARD_STATE_UNFORMATTED:
            task->result = CARD_ERR_UNFORMATTED;
            break;
        case CARD_STATE_SAME:
        case CARD_STATE_NEW:
            task->step++;
            break;
        }
        break;
    case 1:
        switch (Card_GetDirEntry(task)) {
        case CARD_ENTRY_FOUND:
            task->step++;
            break;
        case CARD_ENTRY_MISSING:
            task->step = 0;
            task->result = CARD_ERR_NO_FILE;
            break;
        case CARD_ENTRY_NO_CARD:
            task->step = 0;
            task->result = CARD_ERR_NO_CARD;
            break;
        case CARD_ENTRY_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    case 2:
        switch (Card_DeleteDir(task)) {
        case CARD_CALL_DONE:
            task->step = 0;
            task->result = CARD_OK;
            break;
        case CARD_CALL_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    }
    CardTask_EndIfDone(task);
}

/* A step nothing starts: deletes the task's directory with Card_DeleteDir, with no check first.
 * Result: CARD_OK or CARD_ERR_FAILED. */
void CardTask_DeleteDirStep(CardTask *task)
{
    switch (task->step) {
    case 0:
        switch (Card_DeleteDir(task)) {
        case CARD_CALL_DONE:
            task->result = CARD_OK;
            break;
        case CARD_CALL_FAILED:
            task->result = CARD_ERR_FAILED;
            break;
        }
        break;
    }
    CardTask_EndIfDone(task);
}

/* Starts listing the files of directory dir on port into table (CardTask_ListStep). */
void CardTask_StartList(s32 port, char *dir, u8 *table)
{
    gCardTask.step = 0;
    gCardTask.subStep = 0;
    gCardTask.port = port;
    gCardTask.state = CARD_STEP_LIST;
    gCardTask.result = CARD_BUSY;
    sprintf(gCardTask.name, gCardDirListFmt, dir);
    gCardTask.buf = table;
}

/* Gets the card's state, then lists the directory. Result: the number of entries, CARD_ERR_NO_FILE
 * when the directory is missing, or CARD_ERR_FAILED. */
void CardTask_ListStep(CardTask *task)
{
    switch (task->step) {
    case 0:
        switch (Card_GetInfo(task)) {
        case CARD_STATE_NONE:
            task->result = CARD_ERR_NO_CARD;
            break;
        case CARD_STATE_UNFORMATTED:
            task->result = CARD_ERR_UNFORMATTED;
            break;
        case CARD_STATE_SAME:
        case CARD_STATE_NEW:
            task->step++;
            break;
        }
        break;
    case 1:
        switch (Card_GetDir(task)) {
        case CARD_CALL_DONE:
            task->step = 0;
            task->result = task->entryNo;
            break;
        case CARD_CALL_FAILED:
            task->step = 0;
            task->result = CARD_ERR_FAILED;
            break;
        case CARD_CALL_NO_DIR:
            task->step = 0;
            task->result = CARD_ERR_NO_FILE;
            break;
        }
        break;
    }
    CardTask_EndIfDone(task);
}

/* The task's result (CARD_BUSY while it runs). */
s32 CardTask_Result(void)
{
    return gCardTask.result;
}

/* The card state of port i, as Card_GetInfo returns it. */
s32 Card_PortState(s32 i)
{
    return gCardTask.cardState[i];
}

/* Clears the ports' card-changed bits. */
void Card_ClearChanged(void)
{
    gCardTask.changed = 0;
}

/* Nonzero when the card in port i changed. */
s32 Card_IsChanged(s32 i)
{
    return gCardTask.changed & (1 << i);
}

/* With data, points the operation's icon files at the parts of a packed icon file (offset and size
 * pairs). Then counts the card blocks the save needs (a block per KB of each file, plus one per two
 * files, plus 2), and stores and returns it. */
s32 CardOp_SetFiles(s32 *data)
{
    u8 *base;
    s32 total;
    CardOp *cardOp;
    s32 i;
    s32 cnt;

    cardOp = gCardOps[gCardTask.op];
    if (data != NULL) {
        base = (u8 *)data;
        cnt = data[1];
        data += 2;
        for (i = 0; i < CARD_OP_FILE_SYSTEM; i++) {
            if (cnt <= 0) {
                break;
            }
            if (cardOp->files[i].flag != 0) {
                cnt--;
                cardOp->files[i].ptr = (u8 *)data[0];
                cardOp->files[i].ptr += (s32)base;
                cardOp->files[i].size = data[1];
                data += 2;
            }
        }
    }
    cnt = 0;
    total = 0;
    for (i = 0; i < CARD_OP_FILES; i++) {
        if (cardOp->files[i].flag != 0) {
            total += (cardOp->files[i].size + 1023) / 1024;
            cnt++;
        }
    }
    total += (cnt + 1) / 2;
    return cardOp->blocks = total + 2;
}

/* Sets the weekday of the last directory entry's modified date and copies that date (8 bytes) to
 * dst: the file's date stamp. */
void Card_GetDate(void *dst)
{
    McDateTime *buf;

    McDate_SetWeekday(buf = &gCardDirEntry.modified);
    memcpy(dst, buf, sizeof(McDateTime));
}

/* Computes the weekday of the date (0 Sunday) into its wday. */
void McDate_SetWeekday(McDateTime *date)
{
    s32 month = date->month;
    s32 year = date->year;

    if (month < 3) {
        year--;
        month += 12;
    }
    date->wday = (date->day + (year + year / 4 - year / 100 + year / 400 + (13 * month + 8) / 5)) % 7;
}

/* When the operation has a system file, fills its icon.sys (files[0]): where the first title breaks
 * the line, the two titles joined, and the three icon file names. */
void CardOp_MakeIconSys(CardTask *task)
{
    McIconSys *icon;
    CardOp *cardOp = gCardOps[task->op];

    if (cardOp->files[CARD_OP_FILE_SYSTEM].flag != 0) {
        icon = (McIconSys *)cardOp->files[0].ptr;
        icon->titleBreak = strlen(cardOp->title1);
        sprintf(icon->title, "%s%s", cardOp->title1, cardOp->title2);
        strcpy(icon->icons[0], cardOp->files[1].name);
        strcpy(icon->icons[1], cardOp->files[2].name);
        strcpy(icon->icons[2], cardOp->files[3].name);
    }
}

/* Picks the nearest of the players in the monster's area, within maxHeight and maxDist (0: any) and
 * within range of its facing. Sets target, targetNo, targetDist and targetAngle; returns the
 * player's index, or -1. */
s8 Em_FindTargetPlayer(Em *em, u16 range, f32 maxDist, f32 maxHeight)
{
    f32 dy;
    f32 bestDist;
    f32 x;
    f32 y;
    f32 z;
    f32 flatDist;
    s32 i;
    s32 best;
    Player *player;
    u16 ang;

    best = -1;
    player = gPlayers;
    em->target = NULL;
    bestDist = 0.0f;
    em->targetNo = -1;
    em->targetDist = 0.0f;
    x = em->posX;
    y = em->posY;
    z = em->posZ;
    for (i = 0; i < SCENE_PLAYERS; i++, player++) {
        if (em->area == player->area) {
            dy = player->pos[1] - y;
            if (maxHeight == 0.0f || Math_Absf(dy) <= maxHeight) {
                f32 dx = player->pos[0] - x;
                f32 dz = player->pos[2] - z;

                flatDist = Math_Sqrtf(dx * dx + dz * dz);
                if (maxDist == 0.0f || flatDist <= maxDist) {
                    dy = Math_Sqrtf(flatDist * flatDist + dy * dy);
                    ang = Math_Heading(x, z, player->pos[0], player->pos[2]) - 0x4000;
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

/* Sets bit `bit` of partMask, which lets Em_ScalePartHp scale that part's hp. */
void Em_SetPartScaled(Em *em, s32 bit)
{
    em->partMask |= 1 << bit;
}

/* Adds to part i's hp its table value baseHp times 0.33 + 0.08 * (level - 11) / 3, when the
 * part's partMask bit is set and the monster's level is 11 to 17. Static (declared so above): the
 * callers keep their values in the registers the game's code uses only for a static callee. */
void Em_ScalePartHp(Em *em, s32 i, s16 baseHp)
{
    if (em->partMask & (1 << i)) {
        if (em->level >= 11 && em->level < 18) {
            baseHp = baseHp * (0.33f + 0.08f * ((em->level - 11) / 3.0f));
            em->parts[i].hp += baseHp;
        }
    }
}

/* Sets up the 9 breakable parts from the hp table of the monster's type (gEmPartHpTables; 0 or
 * less: no part), and partDamageMode from the value after them. */
void Em_InitParts(Em *em)
{
    s32 i;
    s16 *tbl = gEmPartHpTables[em->type];

    for (i = 0; i < 9; tbl++, i++) {
        if (*tbl > 0) {
            em->parts[i].active = 1;
            em->parts[i].index = i;
            em->parts[i].unk2 = 0;
            em->parts[i].unk3 = 0;
            em->parts[i].hp = *tbl;
            Em_ScalePartHp(em, i, *tbl);
            em->parts[i].state = 0;
        } else {
            em->parts[i].active = 0;
        }
    }
    em->partDamageMode = *tbl;
}

#pragma optimization_level 4
/* Resets part i's hp to its table value, scaled as Em_InitParts does, when i is a part in use.
 * Built at optimization level 4, as the game's code was: at the file's level the compiler orders
 * the code differently. The table pointer is stepped (tbl += i) rather than indexed for the same
 * reason. */
void Em_ResetPartHp(Em *em, u32 i)
{
    s16 *tbl;

    if (i >= 9 || em->parts[i].active == 0) {
        return;
    }
    tbl = gEmPartHpTables[em->type];
    tbl += i;
    em->parts[i].hp = *tbl;
    Em_ScalePartHp(em, i, *tbl);
}
#pragma optimization_level reset

/* Copies the position of bone `bone`'s world matrix to out. */
void Em_GetBonePos(void *obj, s16 bone, f32 *out)
{
    ObjHead *head = obj;

    Mtx_GetTrans(out, head->model->bones[bone].world);
}

/* Em_GetBonePos again, under a second name. */
void Em_BonePos(void *obj, s16 bone, f32 *out)
{
    Em_GetBonePos(obj, bone, out);
}

/* Bone `bone` of the object's model. */
EmBone *Em_GetBone(void *obj, s16 bone)
{
    ObjHead *head = obj;

    return &head->model->bones[bone];
}

/* Em_GetBone again, under a second name. */
EmBone *Em_Bone(void *obj, s16 bone)
{
    return Em_GetBone(obj, bone);
}

/* The local matrix of bone `bone`. */
f32 *Em_BoneMatrix(void *obj, s16 bone)
{
    ObjHead *head = obj;

    return head->model->bones[bone].local;
}

/* Em_BoneMatrix again, under a second name. */
f32 *Em_BoneMatrixB(void *obj, s16 bone)
{
    return Em_BoneMatrix(obj, bone);
}

/* The card operations (gCardOps), the card steps (gCardSteps) and the directory entry template. */
extern char gOldMhDirName[];
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
McDirEntry gCardFileAttr = {
#include "sub_main/00468B40.inc"
};
void (*gCardSteps[CARD_STEP_COUNT])(CardTask *t) = {
    (void (*)(CardTask *t))CardTask_IdleStep, CardTask_PollStep, CardTask_ReadStep, CardTask_FindStep,
    CardTask_SaveStep, CardTask_FormatStep, CardTask_StubStep, CardTask_DeleteSaveStep,
    CardTask_DeleteDirStep, CardTask_ListStep,
};
