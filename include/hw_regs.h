#ifndef HW_REGS_H
#define HW_REGS_H

/* The EE's and the GS's hardware registers the game's code touches, by their
 * public names (the EE and GS user's manuals). Each is declared with the
 * width the code accesses it with, and the code reaches it only through
 * HW_RD32, HW_WR32, HW_RD64, HW_WR64 and HW_WR128 (below). Our own header:
 * the addresses are the hardware's, and on the PS2 the code compiles exactly
 * as with the raw casts.
 *
 * Each register is an lvalue (`T0_MODE`), which the HW_* macros read and
 * assign; NAME_ADDR is its address. */

/* EE timers */
#define T0_COUNT_ADDR 0x10000000
#define T0_MODE_ADDR 0x10000010
#define T1_COUNT_ADDR 0x10000800
#define T1_MODE_ADDR 0x10000810
/* Tn_MODE's fields (the EE user's manual): CLKS, the clock counted, and
 * CUE, which starts the count */
#define T_MODE_CLKS_HBLANK 0x03     /* count H-blanks */
#define T_MODE_CUE 0x80

/* VIF1 */
#define VIF1_STAT_ADDR 0x10003C00
#define VIF1_FIFO_ADDR 0x10005000

/* DMA channels: each channel's registers start with its CHCR */
#define D0_CHCR_ADDR 0x10008000 /* VIF0 */
#define D1_CHCR_ADDR 0x10009000 /* VIF1 */
#define D2_CHCR_ADDR 0x1000A000 /* GIF */
#define D3_CHCR_ADDR 0x1000B000 /* fromIPU */
#define D4_CHCR_ADDR 0x1000B400 /* toIPU */
#define D5_CHCR_ADDR 0x1000C000 /* SIF0 */
#define D6_CHCR_ADDR 0x1000C400 /* SIF1 */
#define D7_CHCR_ADDR 0x1000C800 /* SIF2 */
#define D8_CHCR_ADDR 0x1000D000 /* fromSPR */
#define D9_CHCR_ADDR 0x1000D400 /* toSPR */

/* GS privileged registers */
#define GS_PMODE_ADDR 0x12000000
#define GS_SMODE2_ADDR 0x12000020
#define GS_DISPFB1_ADDR 0x12000070
#define GS_DISPLAY1_ADDR 0x12000080
#define GS_DISPFB2_ADDR 0x12000090
#define GS_DISPLAY2_ADDR 0x120000A0
#define GS_EXTWRITE_ADDR 0x120000D0
#define GS_CSR_ADDR 0x12001000
#define GS_IMR_ADDR 0x12001010
#define GS_BUSDIR_ADDR 0x12001040


#define T0_COUNT (*(volatile u32 *)T0_COUNT_ADDR)
#define T0_MODE (*(volatile u32 *)T0_MODE_ADDR)
#define T1_COUNT (*(volatile u32 *)T1_COUNT_ADDR)
#define T1_MODE (*(volatile u32 *)T1_MODE_ADDR)
#define VIF1_STAT (*(volatile u32 *)VIF1_STAT_ADDR)
#define VIF1_FIFO (*(volatile u128 *)VIF1_FIFO_ADDR)
#define D0_CHCR (*(volatile u32 *)D0_CHCR_ADDR)
#define D1_CHCR (*(volatile u32 *)D1_CHCR_ADDR)
#define D2_CHCR (*(volatile u32 *)D2_CHCR_ADDR)
#define D3_CHCR (*(volatile u32 *)D3_CHCR_ADDR)
#define D4_CHCR (*(volatile u32 *)D4_CHCR_ADDR)
#define D5_CHCR (*(volatile u32 *)D5_CHCR_ADDR)
#define D6_CHCR (*(volatile u32 *)D6_CHCR_ADDR)
#define D7_CHCR (*(volatile u32 *)D7_CHCR_ADDR)
#define D8_CHCR (*(volatile u32 *)D8_CHCR_ADDR)
#define D9_CHCR (*(volatile u32 *)D9_CHCR_ADDR)
#define GS_PMODE (*(volatile u64 *)GS_PMODE_ADDR)
#define GS_SMODE2 (*(volatile u64 *)GS_SMODE2_ADDR)
#define GS_DISPFB1 (*(volatile u64 *)GS_DISPFB1_ADDR)
#define GS_DISPLAY1 (*(volatile u64 *)GS_DISPLAY1_ADDR)
#define GS_DISPFB2 (*(volatile u64 *)GS_DISPFB2_ADDR)
#define GS_DISPLAY2 (*(volatile u64 *)GS_DISPLAY2_ADDR)
#define GS_EXTWRITE (*(volatile u64 *)GS_EXTWRITE_ADDR)
#define GS_CSR (*(volatile u64 *)GS_CSR_ADDR)
#define GS_IMR (*(volatile u64 *)GS_IMR_ADDR)
#define GS_BUSDIR (*(volatile u64 *)GS_BUSDIR_ADDR)

/* Reads and writes of a register of the width in the macro's name. */
#define HW_RD32(reg) (reg)
#define HW_WR32(reg, value) ((reg) = (value))
#define HW_RD64(reg) (reg)
#define HW_WR64(reg, value) ((reg) = (value))
#define HW_WR128(reg, value) ((reg) = (value))


/* The scratchpad RAM (16 KB) */
#define SCRATCHPAD ((u8 *)0x70000000)

/* GS registers, by their addresses in an A+D packet (the GS's register numbers). */
#define GS_REG_PRIM 0x00
#define GS_REG_RGBAQ 0x01
#define GS_REG_ST 0x02
#define GS_REG_UV 0x03
#define GS_REG_XYZF2 0x04
#define GS_REG_XYZ2 0x05
#define GS_REG_TEX0_1 0x06
#define GS_REG_TEX0_2 0x07
#define GS_REG_CLAMP_1 0x08
#define GS_REG_CLAMP_2 0x09
#define GS_REG_FOG 0x0A
#define GS_REG_TEX1_1 0x14
#define GS_REG_TEX1_2 0x15
#define GS_REG_XYOFFSET_1 0x18
#define GS_REG_XYOFFSET_2 0x19
#define GS_REG_PRMODECONT 0x1A
#define GS_REG_MIPTBP1_1 0x34
#define GS_REG_MIPTBP1_2 0x35
#define GS_REG_MIPTBP2_1 0x36
#define GS_REG_MIPTBP2_2 0x37
#define GS_REG_TEXA 0x3B
#define GS_REG_FOGCOL 0x3D
#define GS_REG_TEXFLUSH 0x3F
#define GS_REG_SCISSOR_1 0x40
#define GS_REG_SCISSOR_2 0x41
#define GS_REG_ALPHA_1 0x42
#define GS_REG_ALPHA_2 0x43
#define GS_REG_DIMX 0x44
#define GS_REG_DTHE 0x45
#define GS_REG_COLCLAMP 0x46
#define GS_REG_TEST_1 0x47
#define GS_REG_TEST_2 0x48
#define GS_REG_PABE 0x49
#define GS_REG_FBA_1 0x4A
#define GS_REG_FBA_2 0x4B
#define GS_REG_FRAME_1 0x4C
#define GS_REG_FRAME_2 0x4D
#define GS_REG_ZBUF_1 0x4E
#define GS_REG_ZBUF_2 0x4F
#define GS_REG_BITBLTBUF 0x50       /* an image transfer's VRAM buffer */
#define GS_REG_TRXPOS 0x51          /* its position */
#define GS_REG_TRXREG 0x52          /* its size */
#define GS_REG_TRXDIR 0x53          /* its direction: 0 starts a transfer to VRAM */

/* The GIF tag's register descriptor A+D: each qword is a value and the
 * number of the GS register it goes to. */
#define GIF_REG_AD 0xE

/* One A+D register write: the value, then the register's number. */
typedef struct GsAd {
    u64 data;
    u64 reg;
} GsAd;

/* A GIF tag's fields (its low doubleword): NLOOP is the low 15 bits. */
#define GIF_TAG_EOP 0x8000              /* the packet's last tag */
#define GIF_TAG_NREG(n) ((u64)(n) << 60) /* n register descriptors */

/* The GS's TEX1 fields (the texture's LOD and sampling). */
#define TEX1_MXL(n) ((u64)(n) << 2)     /* the last mipmap level */
#define TEX1_MMAG_LINEAR 0x20           /* magnified: bilinear */
#define TEX1_MMIN(m) ((m) << 6)         /* minified: TEX1_MIN_* */
#define TEX1_L(l) ((u64)(l) << 19)
#define TEX1_K(k) ((u64)(((k) << 4) & 0xFFF) << 32)  /* K in 16ths */
#define TEX1_MIN_LINEAR 1
#define TEX1_MIN_NEAREST_MIP_NEAREST 2
#define TEX1_MIN_NEAREST_MIP_LINEAR 3
#define TEX1_MIN_LINEAR_MIP_NEAREST 4
#define TEX1_MIN_LINEAR_MIP_LINEAR 5

/* TEST's fields: the alpha test (ATE, ATST, AREF) and the depth test
 * (ZTE, ZTST). */
#define GS_TEST_ATE 1
#define GS_TEST_ZTE 0x10000
#define GS_ATST_NEVER 0
#define GS_ATST_ALWAYS 1
#define GS_ATST_LESS 2
#define GS_ATST_LEQUAL 3
#define GS_ATST_EQUAL 4
#define GS_ATST_GEQUAL 5
#define GS_ATST_GREATER 6
#define GS_ATST_NOTEQUAL 7
#define GS_ZTST_NEVER 0
#define GS_ZTST_ALWAYS 1
#define GS_ZTST_GEQUAL 2
#define GS_ZTST_GREATER 3

/* PRIM's primitive types. */
#define GS_PRIM_SPRITE 6

#endif
