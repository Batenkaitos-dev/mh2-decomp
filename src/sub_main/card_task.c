/*
 * The memory card's input and output, under the card menus (NetMenu, gCardMenu), whose save
 * and load steps (SaveMenu_SaveSystem, SaveMenu_LoadCharacter ...) start the card task and
 * poll it. A save on the card is a directory of files: icon.sys and the icons,
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
#include "text.h"
#include "chat_phrases.h"
#include "libmc.h"

extern s32 gCardFileKind;
extern s32 gCardLocked;
extern McDirEntry gCardDirEntry;
extern char gCardDeletePath[];
extern McDirEntry gCardFileAttr;
extern u8 gOldGameKind;
extern SaveData *gCardSlots;

void *memset(void *p, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
void func_161EC0(void);
void func_161ED8(void);
char *strcpy(char *dst, const char *src);
s32 sceMcSetFileInfo(s32 port, s32 slot, char *name, McDirEntry *info, s32 flags);
char *strcat(char *dst, const char *src);
static s32 Card_StubFail(void);
s32 sprintf(char *buf, char *fmt, ...);
static void CardState_SetFlags(s32 a);
struct NetMenu;
s32 sceMcGetDir(s32 port, s32 slot, char *name, u32 mode, s32 max, McDirEntry *table);
u16 strlen(char *s);

/* The card operations' titles and file names, read-only. */
#pragma rodata on
char gCardSaveTitle1[24] = {
#include "sub_main/004B0570.inc"
};
char gCardSaveTitle2[13] = {
#include "sub_main/004B0588.inc"
};
char gCardIconSysName[12] = {
#include "sub_main/004B0598.inc"
};
char gCardSaveIconName[12] = {
#include "sub_main/004B05A8.inc"
};
char gCardSystemFileName[16] = {
#include "sub_main/004B05C0.inc"
};
char gCardCharFileName0[16] = {
#include "sub_main/004B05D0.inc"
};
char gCardCharFileName1[16] = {
#include "sub_main/004B05E0.inc"
};
char gCardCharFileName2[16] = {
#include "sub_main/004B05F0.inc"
};
char gCardPatchFileName[16] = {
#include "sub_main/004B0600.inc"
};
char gNetFileTitle1[16] = {
#include "sub_main/004B0610.inc"
};
char gNetFileTitle2[16] = {
#include "sub_main/004B0620.inc"
};
char gNetFileIcon1[16] = {
#include "sub_main/004B0630.inc"
};
char gNetFileIcon2[16] = {
#include "sub_main/004B0640.inc"
};
char gNetFileIcon3[16] = {
#include "sub_main/004B0650.inc"
};
char gNetFileName[16] = {
#include "sub_main/004B0660.inc"
};
/* The empty name of the earlier games' operations' unused files, then at +8 the first game's
 * directory name (gOldMhDirName): one array, as in the game. As an array of its own the name
 * would be aligned to 16 bytes, and the data after it would move. */
char gCardNoName[32] = {
#include "sub_main/004B0670.inc"
};
char gOldMhgDirName[16] = {
#include "sub_main/004B0690.inc"
};
#pragma rodata off

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
#pragma rodata on
char gCardDirListFmt[5] = {
#include "sub_main/004B06A0.inc"
};
char gCardPathFmt[6] = {
#include "sub_main/004B06A8.inc"
};
char gCardNameFmt[3] = {
#include "sub_main/004B06B0.inc"
};
#pragma rodata off

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

/* Lists the task's directory (sceMcGetDir, up to 256 entries into buf). Returns CARD_CALL_DONE
 * with the count in entryNo, CARD_CALL_NO_DIR when there is no such directory, CARD_CALL_FAILED,
 * or CARD_CALL_BUSY while busy. */
s32 Card_GetDir(CardTask *task)
{
    switch (task->subStep) {
    case 0:
        if (Card_Sync(task) < 0) {
            break;
        }
        if (sceMcGetDir(task->port, 0, task->name, 0, 256, (McDirEntry *)task->buf) < 0) {
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
