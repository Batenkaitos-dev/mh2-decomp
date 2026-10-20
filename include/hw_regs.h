#ifndef HW_REGS_H
#define HW_REGS_H

/* The EE's and the GS's hardware registers the game's code touches, by
 * their public names (the EE and GS user's manuals), as lvalues: `T0_MODE =
 * 0x83;`, `GS_IMR = ~0x200LL;`. Each register is declared with the width
 * the code accesses it with. Our own header: the addresses are the
 * hardware's, the code compiles exactly as with the raw casts. */

/* EE timers */
#define T0_COUNT (*(volatile u32 *)0x10000000)
#define T0_MODE (*(volatile u32 *)0x10000010)
#define T1_COUNT (*(volatile u32 *)0x10000800)
#define T1_MODE (*(volatile u32 *)0x10000810)
/* Tn_MODE's fields (the EE user's manual): CLKS, the clock counted, and
 * CUE, which starts the count */
#define T_MODE_CLKS_HBLANK 0x03     /* count H-blanks */
#define T_MODE_CUE 0x80

/* VIF1 */
#define VIF1_STAT (*(volatile u32 *)0x10003C00)
#define VIF1_FIFO (*(volatile u128 *)0x10005000)

/* DMA channels: each channel's registers start with its CHCR */
#define D0_CHCR (*(volatile u32 *)0x10008000) /* VIF0 */
#define D1_CHCR (*(volatile u32 *)0x10009000) /* VIF1 */
#define D2_CHCR (*(volatile u32 *)0x1000A000) /* GIF */
#define D3_CHCR (*(volatile u32 *)0x1000B000) /* fromIPU */
#define D4_CHCR (*(volatile u32 *)0x1000B400) /* toIPU */
#define D5_CHCR (*(volatile u32 *)0x1000C000) /* SIF0 */
#define D6_CHCR (*(volatile u32 *)0x1000C400) /* SIF1 */
#define D7_CHCR (*(volatile u32 *)0x1000C800) /* SIF2 */
#define D8_CHCR (*(volatile u32 *)0x1000D000) /* fromSPR */
#define D9_CHCR (*(volatile u32 *)0x1000D400) /* toSPR */

/* GS privileged registers */
#define GS_PMODE (*(volatile u64 *)0x12000000)
#define GS_SMODE2 (*(volatile u64 *)0x12000020)
#define GS_DISPFB1 (*(volatile u64 *)0x12000070)
#define GS_DISPLAY1 (*(volatile u64 *)0x12000080)
#define GS_DISPFB2 (*(volatile u64 *)0x12000090)
#define GS_DISPLAY2 (*(volatile u64 *)0x120000A0)
#define GS_EXTWRITE (*(volatile u64 *)0x120000D0)
#define GS_CSR (*(volatile u64 *)0x12001000)
#define GS_IMR (*(volatile u64 *)0x12001010)
#define GS_BUSDIR (*(volatile u64 *)0x12001040)

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
#define GS_REG_TEST_1 0x47
#define GS_REG_TEST_2 0x48
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

#endif
