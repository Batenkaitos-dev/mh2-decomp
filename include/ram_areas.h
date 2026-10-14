#ifndef RAM_AREAS_H
#define RAM_AREAS_H

/* Fixed areas of the EE's RAM outside every module, which the code
 * addresses directly. The code takes their addresses as constants (`lui;
 * ori`), which a linked symbol can't give (`lui; addiu`), so they are named
 * here rather than in config/symbols. */

/* The patch area (0xAA6780-0xAC67C0): a downloaded patch file is copied
 * here (Patch_InstallFile): its data (0x10000 bytes), the records
 * PatchExecCS applies (magic, count, records; 0x10000 bytes) and its header
 * (0x40 bytes: the version text, "1000" at +0xA, the active flag at +0x14). */
#define PATCH_DATA ((u8 *)0xAA6780)
#define PATCH_RECS ((u8 *)0xAB6780)
#define PATCH_HEADER ((u8 *)0xAC6780)

/* The IOP module pack, read here before it is sent to the IOP (iop.c). */
#define IOP_MODPACK_BUF ((u8 *)0x1000000)

/* data.bin's table of contents, read at boot (partition 5; unk_192EF0.c). */
#define DATA_TOC_BUF ((u8 *)0x800000)

/* The memory layout Mem_InitLayout (sub_main unk_2E3940.c) sets in
 * gSysSettings: values, not objects. */
#define MEM_AREAS_ADDR 0xAC8000    /* gSysSettings.unk54 and unk60: the start of the areas below (0x55E000 bytes) */
#define WORK_AREA_ADDR 0xC0A200    /* gSysSettings.workArea: the overlays' shared work area (0x18000 bytes) */
#define ANIM_AREA_ADDR 0xC2C200    /* gSysSettings.unk74: animation sets are loaded here */
#define LOAD_BUF_ADDR 0xD88000     /* gSysSettings.loadBuf (gLoadBuf): overlay files and sound banks are read here */

#endif
