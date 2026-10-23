#ifndef RAM_AREAS_H
#define RAM_AREAS_H

/* Fixed areas of the EE's RAM outside every module, which the code
 * addresses directly. The code takes their addresses as constants (`lui;
 * ori`), which a linked symbol can't give (`lui; addiu`), so they are named
 * here rather than in config/symbols. */

/* The system heap's area: 15 MB of RAM that Sys_InitWork gives the linear
 * heap gSysHeap, from which the block heap takes 14 MB. */
#define SYS_HEAP_AREA ((void *)0x1068000)
#define SYS_HEAP_AREA_SIZE 0xF00000

/* The patch area (PATCH_DATA to the end of PATCH_HEADER): a downloaded patch file is copied
 * here (Patch_InstallFile): its data (65536 bytes), the records
 * PatchExecCS applies (magic, count, records; 65536 bytes) and its header
 * (64 bytes: the version text at +0xA, the active flag at +0x14). */
#define PATCH_DATA ((u8 *)0xAA6780)
#define PATCH_RECS ((u8 *)0xAB6780)
#define PATCH_HEADER ((u8 *)0xAC6780)

/* The IOP module pack, read here before it is sent to the IOP (iop.c). */
#define IOP_MODPACK_BUF ((u8 *)0x1000000)

/* data.bin's table of contents, read at boot (partition 5). */
#define DATA_TOC_BUF ((u8 *)0x800000)

/* The memory layout Mem_InitLayout sets in
 * gSysSettings: values, not objects. */
#define MEM_AREAS_ADDR 0xAC8000    /* gSysSettings.unk54 and unk60: the start of the areas below (0x55E000 bytes) */
#define WORK_AREA_ADDR 0xC0A200    /* gSysSettings.workArea: the overlays' shared work area (98304 bytes) */
#define ANIM_AREA_ADDR 0xC2C200    /* gSysSettings.unk74: animation sets are loaded here */
#define LOAD_BUF_ADDR 0xD88000     /* gSysSettings.loadBuf (gLoadBuf): overlay files and sound banks are read here */
#define MEM_AREAS_SIZE 0x55E000    /* bytes Mem_InitLayout clears from MEM_AREAS_ADDR */
#define BONE_AREA_ADDR 0xB09200    /* gSysSettings.bones (gBones): the bone pool */
#define MODEL_AREA_ADDR 0xBD1200   /* gSysSettings.models (gModels): the model headers */
#define PRIM_AREA_ADDR 0xBD5200    /* gSysSettings.prims (gPrims) */
#define EXTRA_BUF_ADDR 0xBDA200    /* gSysSettings.extraBuf: the extra save file */
#define LOBBY_WORK_ADDR 0xBEE200   /* gSysSettings.lobbyWorkBuf (gLobbyWorkBuf) */
#define STAGE_FILE_ADDR 0xC22200   /* gSysSettings.stageFile (gStageFile) */
#define CAM_DATA_ADDR 0xC2A200     /* gSysSettings.camData: the area's camera data */
#define SAVE_BUF_ADDR 0xD60200     /* gSysSettings.saveBuf (gSaveBuf); plsel.bin is loaded at the same address */
#define READ_BUF_ADDR 0xF08000     /* gSysSettings.readBuf (gReadBuf): LoadCompressedFile's compressed input */

#endif
