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

#endif
