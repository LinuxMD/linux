/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 64 x 64 -> 64 bit multiply for 68000, replacing the generic C
   lib/muldi3.c, which gcc compiles into six __mulsi3 libcalls on
   CPUs without a 32-bit multiply instruction.

   result = low64(u * v)
	  = (u.low * v.low) + ((u.high * v.low + u.low * v.high) << 32)

   The 32 x 32 -> 64 core is the classic four-mulu construction from
   gcc's longlong.h.  Cross products are skipped when the respective
   high long is zero, and operands that both fit in 16 bits take a
   single mulu.  */

#include <linux/export.h>

/* These are predefined by new versions of GNU cpp.  */

#ifndef __USER_LABEL_PREFIX__
#define __USER_LABEL_PREFIX__ _
#endif

#ifndef __REGISTER_PREFIX__
#define __REGISTER_PREFIX__
#endif

#ifndef __IMMEDIATE_PREFIX__
#define __IMMEDIATE_PREFIX__ #
#endif

/* ANSI concatenation macros.  */

#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

/* Use the right prefix for global labels.  */

#define SYM(x) CONCAT1 (__USER_LABEL_PREFIX__, x)

/* Use the right prefix for registers.  */

#define REG(x) CONCAT1 (__REGISTER_PREFIX__, x)

/* Use the right prefix for immediate values.  */

#define IMM(x) CONCAT1 (__IMMEDIATE_PREFIX__, x)

#define d0 REG (d0)
#define d1 REG (d1)
#define d2 REG (d2)
#define d3 REG (d3)
#define d4 REG (d4)
#define sp REG (sp)

	.text
	.proc
	.globl	SYM (__muldi3)

/* Args (big endian): u.high sp@(4), u.low sp@(8), v.high sp@(12),
   v.low sp@(16).  Result in d0 (high) : d1 (low).  */

SYM (__muldi3):
	movel	sp@(4), d1	/* u.high | v.high */
	orl	sp@(12), d1
	jne	L2
	movel	sp@(8), d0	/* both high longs zero: 32 x 32 */
	movel	sp@(16), d1
	cmpl	IMM (0x10000), d0
	jcc	L1
	cmpl	IMM (0x10000), d1
	jcc	L1
	mulu	d1, d0		/* 16 x 16: a single mulu */
	movel	d0, d1
	moveq	IMM (0), d0
	rts

L1:	moveml	d2-d4, sp@-	/* 32 x 32 -> 64, no cross products */
	jbsr	L5
	moveml	sp@+, d2-d4
	rts

L2:	moveml	d2-d4, sp@-	/* full 64 x 64 */
	movel	sp@(20), d0	/* u.low */
	movel	sp@(28), d1	/* v.low */
	jbsr	L5		/* d0:d1 = u.low * v.low */
	tstl	sp@(16)		/* u.high == 0: skip first cross product */
	jeq	L3
	movew	sp@(18), d2	/* u.high.lo16 */
	movew	d2, d3
	muluw	sp@(30), d2	/* u.high.lo16 * v.low.lo16 */
	muluw	sp@(28), d3	/* u.high.lo16 * v.low.hi16 */
	movew	sp@(16), d4	/* u.high.hi16 */
	muluw	sp@(30), d4	/* u.high.hi16 * v.low.lo16 */
	addw	d4, d3		/* only bits 15..0 survive the << 16 */
	swap	d3
	clrw	d3
	addl	d3, d2		/* d2 = low32(u.high * v.low) */
	addl	d2, d0
L3:	tstl	sp@(24)		/* v.high == 0: skip second cross product */
	jeq	L4
	movew	sp@(26), d2	/* v.high.lo16 */
	movew	d2, d3
	muluw	sp@(22), d2	/* v.high.lo16 * u.low.lo16 */
	muluw	sp@(20), d3	/* v.high.lo16 * u.low.hi16 */
	movew	sp@(24), d4	/* v.high.hi16 */
	muluw	sp@(22), d4	/* v.high.hi16 * u.low.lo16 */
	addw	d4, d3
	swap	d3
	clrw	d3
	addl	d3, d2		/* d2 = low32(u.low * v.high) */
	addl	d2, d0
L4:	moveml	sp@+, d2-d4
	rts

/* d0:d1 = d0 * d1 (32 x 32 -> 64), clobbers d2-d4.  With
   d0 = A:B and d1 = C:D in 16-bit halves the product is
   A*C << 32 | (A*D + B*C) << 16 | B*D.  */

L5:	movel	d0, d2
	swap	d0		/* d0.w = A */
	movel	d1, d3
	swap	d1		/* d1.w = C */
	movew	d2, d4		/* d4.w = B */
	mulu	d3, d4		/* d4 = B*D */
	mulu	d1, d2		/* d2 = B*C */
	mulu	d0, d3		/* d3 = A*D */
	mulu	d0, d1		/* d1 = A*C */
	movel	d4, d0
	clrw	d0
	swap	d0		/* d0 = hi16(B*D) */
	addl	d0, d2		/* mid = B*C + hi16(B*D), cannot carry */
	addl	d3, d2		/* mid += A*D, can carry */
	jcc	L6
	addl	IMM (0x10000), d1 /* propagate carry into A*C */
L6:	movel	d2, d0
	clrw	d0
	swap	d0
	addl	d1, d0		/* d0 = A*C + hi16(mid): high 32 bits */
	swap	d2
	clrw	d2
	movew	d4, d2		/* d2 = lo16(mid):lo16(B*D) */
	movel	d2, d1		/* d1: low 32 bits */
	rts

	EXPORT_SYMBOL(__muldi3)
