/*
 * main 0x182450-0x182640: Capcom's block copy, clear and fill loops.
 *
 * Twelve leaf routines, hand-written in assembly. Each is one counted loop,
 * do { ... } while (--n):
 * - the loop starts at the routine's own entry, and the count is tested
 *   only after the first pass: n must be at least 1 (n = 0 runs 2^32
 *   times);
 * - the backward branch's delay slot does the last pointer step, and the
 *   jr ra's slot is a nop;
 * - the temporaries are v0, v1 and t0-t5; no stack frame.
 *
 * The copies take (src, dst, n), the source first as in bcopy; the clears
 * and fills take (dst, n) and (dst, n, value). n counts the routine's
 * unit (a word, a quadword or a block of 16, 32 or 64 bytes), not bytes.
 * src and dst must be aligned to the routine's widest access: 4 bytes for
 * lw/sw, 8 for ld/sd (the EE raises an address error otherwise), 16 for
 * lq/sq (which ignore the low four address bits instead, so a misaligned
 * pointer silently reads or writes the quadword below it).
 *
 * Why this is assembly and not C (docs/mwcc-matching.md, "What can't come from
 * C"):
 * - the routines are packed on 8-byte boundaries (0x182470, 0x1824A8, ...).
 *   MWCC starts every function on 16 bytes, CodeWarrior asm functions too,
 *   and has no option to change it; all of Capcom's compiled functions are
 *   16-byte aligned. Written as asm functions, all twelve match, but the
 *   image grows by 0x50 bytes;
 * - MWCC's C for these loops orders the instructions differently and
 *   leaves the branch's delay slot empty.
 *
 * Built from this file (tools/configure.py prefers a committed
 * src/<module>/<piece>.s over splat's asm/). splat still writes its own
 * disassembly of the range to asm/main/mem_loops.s, and objdiff compares
 * this file's object against it.
 */

.include "macro.inc"

.set noat
.set noreorder

.section .text, "ax"

/*
 * void Mem_CopyWords(const u32 *src, u32 *dst, s32 n)
 * Copies n words. Unit 4 bytes; src and dst 4-byte aligned.
 */
glabel Mem_CopyWords
    lw      $v0, 0x0($a0)               /* w = *src */
    addiu   $a0, $a0, 0x4               /* src += 4 */
    addiu   $a2, $a2, -0x1              /* n-- */
    sw      $v0, 0x0($a1)               /* *dst = w */
    bnez    $a2, Mem_CopyWords          /* again while n != 0 */
     addiu  $a1, $a1, 0x4               /* (delay slot) dst += 4 */
    jr      $ra
     nop
endlabel Mem_CopyWords

/*
 * void Mem_Copy16ByWords(const void *src, void *dst, s32 n)
 * Copies n 16-byte blocks, four words at a time: all four loads, then all
 * four stores. Unit 16 bytes; src and dst 4-byte aligned.
 */
glabel Mem_Copy16ByWords
    lw      $v0, 0x0($a0)
    lw      $v1, 0x4($a0)
    lw      $t0, 0x8($a0)
    lw      $t1, 0xC($a0)
    sw      $v0, 0x0($a1)
    sw      $v1, 0x4($a1)
    sw      $t0, 0x8($a1)
    sw      $t1, 0xC($a1)
    addiu   $a2, $a2, -0x1              /* n-- */
    addiu   $a0, $a0, 0x10              /* src += 16 */
    bnez    $a2, Mem_Copy16ByWords      /* again while n != 0 */
     addiu  $a1, $a1, 0x10              /* (delay slot) dst += 16 */
    jr      $ra
     nop
endlabel Mem_Copy16ByWords

/*
 * void Mem_Copy16ByDwords(const void *src, void *dst, s32 n)
 * Copies n 16-byte blocks, two doublewords (64-bit ld/sd) at a time.
 * Unit 16 bytes; src and dst 8-byte aligned.
 */
glabel Mem_Copy16ByDwords
    ld      $v0, 0x0($a0)
    ld      $v1, 0x8($a0)
    sd      $v0, 0x0($a1)
    sd      $v1, 0x8($a1)
    addiu   $a2, $a2, -0x1              /* n-- */
    addiu   $a0, $a0, 0x10              /* src += 16 */
    bnez    $a2, Mem_Copy16ByDwords     /* again while n != 0 */
     addiu  $a1, $a1, 0x10              /* (delay slot) dst += 16 */
    jr      $ra
     nop
endlabel Mem_Copy16ByDwords

/*
 * void Mem_CopyQwords(const void *src, void *dst, s32 n)
 * Copies n quadwords, one 128-bit lq/sq at a time (the EE's general
 * registers are 128 bits wide). Unit 16 bytes; src and dst 16-byte
 * aligned. The game's GS packet templates (copied to the scratchpad) and
 * DMA packets go through it.
 */
glabel Mem_CopyQwords
    addiu   $a2, $a2, -0x1              /* n-- */
    lq      $v0, 0x0($a0)               /* q = *src */
    addiu   $a0, $a0, 0x10              /* src += 16 */
    sq      $v0, 0x0($a1)               /* *dst = q */
    bnez    $a2, Mem_CopyQwords         /* again while n != 0 */
     addiu  $a1, $a1, 0x10              /* (delay slot) dst += 16 */
    jr      $ra
     nop
endlabel Mem_CopyQwords

/*
 * void Mem_Copy32ByWords(const void *src, void *dst, s32 n)
 * Copies n 32-byte blocks, eight words at a time. Unit 32 bytes; src and
 * dst 4-byte aligned.
 */
glabel Mem_Copy32ByWords
    lw      $v0, 0x0($a0)
    lw      $v1, 0x4($a0)
    lw      $t0, 0x8($a0)
    lw      $t1, 0xC($a0)
    lw      $t2, 0x10($a0)
    lw      $t3, 0x14($a0)
    lw      $t4, 0x18($a0)
    lw      $t5, 0x1C($a0)
    sw      $v0, 0x0($a1)
    sw      $v1, 0x4($a1)
    sw      $t0, 0x8($a1)
    sw      $t1, 0xC($a1)
    sw      $t2, 0x10($a1)
    sw      $t3, 0x14($a1)
    sw      $t4, 0x18($a1)
    sw      $t5, 0x1C($a1)
    addiu   $a2, $a2, -0x1              /* n-- */
    addiu   $a0, $a0, 0x20              /* src += 32 */
    bnez    $a2, Mem_Copy32ByWords      /* again while n != 0 */
     addiu  $a1, $a1, 0x20              /* (delay slot) dst += 32 */
    jr      $ra
     nop
endlabel Mem_Copy32ByWords

/*
 * void Mem_Copy32ByDwords(const void *src, void *dst, s32 n)
 * Copies n 32-byte blocks, four doublewords at a time. Unit 32 bytes; src
 * and dst 8-byte aligned.
 */
glabel Mem_Copy32ByDwords
    ld      $v0, 0x0($a0)
    ld      $v1, 0x8($a0)
    ld      $t0, 0x10($a0)
    ld      $t1, 0x18($a0)
    sd      $v0, 0x0($a1)
    sd      $v1, 0x8($a1)
    sd      $t0, 0x10($a1)
    sd      $t1, 0x18($a1)
    addiu   $a2, $a2, -0x1              /* n-- */
    addiu   $a0, $a0, 0x20              /* src += 32 */
    bnez    $a2, Mem_Copy32ByDwords     /* again while n != 0 */
     addiu  $a1, $a1, 0x20              /* (delay slot) dst += 32 */
    jr      $ra
     nop
endlabel Mem_Copy32ByDwords

/*
 * void Mem_Copy32ByQwords(const void *src, void *dst, s32 n)
 * Copies n 32-byte blocks, two quadwords at a time. Unit 32 bytes; src and
 * dst 16-byte aligned.
 */
glabel Mem_Copy32ByQwords
    lq      $v0, 0x0($a0)
    lq      $v1, 0x10($a0)
    sq      $v0, 0x0($a1)
    sq      $v1, 0x10($a1)
    addiu   $a2, $a2, -0x1              /* n-- */
    addiu   $a0, $a0, 0x20              /* src += 32 */
    bnez    $a2, Mem_Copy32ByQwords     /* again while n != 0 */
     addiu  $a1, $a1, 0x20              /* (delay slot) dst += 32 */
    jr      $ra
     nop
endlabel Mem_Copy32ByQwords

/*
 * void Mem_Copy64ByQwords(const void *src, void *dst, s32 n)
 * Copies n 64-byte blocks, four quadwords at a time. Unit 64 bytes (the
 * size of a 4x4 float matrix); src and dst 16-byte aligned.
 */
glabel Mem_Copy64ByQwords
    lq      $v0, 0x0($a0)
    lq      $v1, 0x10($a0)
    lq      $t0, 0x20($a0)
    lq      $t1, 0x30($a0)
    sq      $v0, 0x0($a1)
    sq      $v1, 0x10($a1)
    sq      $t0, 0x20($a1)
    sq      $t1, 0x30($a1)
    addiu   $a2, $a2, -0x1              /* n-- */
    addiu   $a0, $a0, 0x40              /* src += 64 */
    bnez    $a2, Mem_Copy64ByQwords     /* again while n != 0 */
     addiu  $a1, $a1, 0x40              /* (delay slot) dst += 64 */
    jr      $ra
     nop
endlabel Mem_Copy64ByQwords

/*
 * void Mem_ClearWords(u32 *dst, s32 n)
 * Zeroes n words. Unit 4 bytes; dst 4-byte aligned.
 */
glabel Mem_ClearWords
    addiu   $a1, $a1, -0x1              /* n-- */
    sw      $zero, 0x0($a0)             /* *dst = 0 */
    bnez    $a1, Mem_ClearWords         /* again while n != 0 */
     addiu  $a0, $a0, 0x4               /* (delay slot) dst += 4 */
    jr      $ra
     nop
endlabel Mem_ClearWords

/*
 * void Mem_ClearQwords(void *dst, s32 n)
 * Zeroes n quadwords: sq of the zero register stores all 128 bits. Unit
 * 16 bytes; dst 16-byte aligned.
 */
glabel Mem_ClearQwords
    addiu   $a1, $a1, -0x1              /* n-- */
    sq      $zero, 0x0($a0)             /* *dst = 0 (16 bytes) */
    bnez    $a1, Mem_ClearQwords        /* again while n != 0 */
     addiu  $a0, $a0, 0x10              /* (delay slot) dst += 16 */
    jr      $ra
     nop
endlabel Mem_ClearQwords

/*
 * void Mem_FillWords(u32 *dst, s32 n, u32 value)
 * Stores value in n words. Unit 4 bytes; dst 4-byte aligned.
 */
glabel Mem_FillWords
    addiu   $a1, $a1, -0x1              /* n-- */
    sw      $a2, 0x0($a0)               /* *dst = value */
    bnez    $a1, Mem_FillWords          /* again while n != 0 */
     addiu  $a0, $a0, 0x4               /* (delay slot) dst += 4 */
    jr      $ra
     nop
endlabel Mem_FillWords

/*
 * void Mem_FillQwords(void *dst, s32 n, u128 value)
 * Stores the whole 128-bit a2 in n quadwords: the EE passes a 128-bit
 * argument in one general register. Unit 16 bytes; dst 16-byte aligned.
 */
glabel Mem_FillQwords
    addiu   $a1, $a1, -0x1              /* n-- */
    sq      $a2, 0x0($a0)               /* *dst = value (16 bytes) */
    bnez    $a1, Mem_FillQwords         /* again while n != 0 */
     addiu  $a0, $a0, 0x10              /* (delay slot) dst += 16 */
    jr      $ra
     nop
endlabel Mem_FillQwords
