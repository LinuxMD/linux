/* libgcc1 routines for 68000 w/o floating-point hardware.
   Copyright (C) 1994, 1996, 1997, 1998 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

In addition to the permissions in the GNU General Public License, the
Free Software Foundation gives you unlimited permission to link the
compiled version of this file with other programs, and to distribute
those programs without any restriction coming from the use of this
file.  (The General Public License restrictions do apply in other
respects; for example, they cover modification of the file, and
distribution when not linked into another program.)

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details. */

/* As a special exception, if you link this library with files
   compiled with GCC to produce an executable, this does not cause
   the resulting executable to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.  */

/* Use this one for any 680x0; assumes no floating point hardware.
   The trailing " '" appearing on some lines is for ANSI preprocessors.  Yuk.
   Some of this code comes from MINIX, via the folks at ericsson.
   D. V. Henkel-Wallace (gumby@cygnus.com) Fete Bastille, 1992
*/
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
#define d5 REG (d5)
#define d6 REG (d6)
#define d7 REG (d7)
#define a0 REG (a0)
#define a1 REG (a1)
#define a2 REG (a2)
#define a3 REG (a3)
#define a4 REG (a4)
#define a5 REG (a5)
#define a6 REG (a6)
#define fp REG (fp)
#define sp REG (sp)

	.text
	.proc
	.globl	SYM (__udivsi3)
	.globl	SYM (__udivmodsi4_reg)

#if !(defined(__mcf5200__) || defined(__mcoldfire__))

/* Stack-args ABI entry, unchanged for gcc.  As a side effect the
   remainder is now returned in d1 (a call-clobbered register).  */
SYM (__udivsi3):
	movel	sp@(8), d1	/* d1 = divisor */
	movel	sp@(4), d0	/* d0 = dividend */

/* Register-args entry for the in-kernel div/mod helpers:
   input:   d0 = dividend, d1 = divisor
   output:  d0 = quotient, d1 = remainder
   clobbers a0/a1, preserves d2-d7/a2-a6 (divsi3 relies on d2).  */
SYM (__udivmodsi4_reg):
	cmpl	IMM (0x10000), d1 /* divisor >= 2 ^ 16 ?   */
	jcc	L3		/* then try next algorithm */
	cmpl	IMM (0x10000), d0 /* dividend >= 2 ^ 16 ?   */
	jcc	L1

	divu	d1, d0		/* 16/16 bit: single divide; d0 = rem:quot */
	moveq	IMM (0), d1
	swap	d0		/* d0 = quot:rem */
	movew	d0, d1		/* d1 = remainder */
	clrw	d0
	swap	d0		/* d0 = quotient */
	rts

L1:	movew	d0, a0		/* 32/16 bit: save low word of dividend */
	clrw	d0
	swap	d0		/* d0 = high word of dividend */
	divu	d1, d0		/* high quotient in lower word */
	movew	d0, a1		/* save high quotient */
	movew	a0, d0		/* get low dividend, keep high rest */
	divu	d1, d0		/* d0 = rem:low quotient */
	moveq	IMM (0), d1
	swap	d0		/* d0 = low quot:rem */
	movew	d0, d1		/* d1 = remainder */
	movew	a1, d0		/* d0 = low quot:high quot */
	swap	d0		/* d0 = quotient */
	rts

L3:	movel	d0, a0		/* save dividend for correction/remainder */
	movel	d1, a1		/* save divisor for remainder */
	movel	d2, sp@-
	movel	d1, d2		/* use d2 as divisor backup */
L4:	lsrl	IMM (1), d1	/* shift divisor */
	lsrl	IMM (1), d0	/* shift dividend */
	cmpl	IMM (0x10000), d1 /* still divisor >= 2 ^ 16 ?  */
	jcc	L4
	divu	d1, d0		/* now we have 16 bit divisor */
	andl	IMM (0xffff), d0 /* mask out divisor, ignore remainder */

/* Multiply the 16 bit tentative quotient with the 32 bit divisor.  Because of
   the operand ranges, this might give a 33 bit product.  If this product is
   greater than the dividend, the tentative quotient was too large.  Unlike
   the original code, keep low32(quotient * divisor) in d1 on every exit:
   the final product fits in 32 bits, so mod 2^32 arithmetic is exact and
   the remainder is dividend - quotient * divisor.  */
	movel	d2, d1
	mulu	d0, d1		/* low part, 32 bits */
	swap	d2
	mulu	d0, d2		/* high part, at most 17 bits */
	swap	d2		/* align high part with low part */
	tstw	d2		/* high part 17 bits? */
	jeq	L5
	clrw	d2		/* drop bit 32 of the product */
	addl	d2, d1		/* d1 = low32(quotient * divisor) */
	jra	L6		/* 17 bits: quotient was too large */
L5:	addl	d2, d1		/* add parts */
	jcs	L6		/* if sum is 33 bits, quotient was too large */
	cmpl	a0, d1		/* compare the sum with the dividend */
	jls	L7		/* if sum <= dividend, quotient was correct */
L6:	subql	IMM (1), d0	/* adjust quotient */
	subl	a1, d1		/* adjust product */
L7:	negl	d1
	addl	a0, d1		/* d1 = dividend - quotient * divisor */
	movel	sp@+, d2
	rts

#else /* __mcf5200__ || __mcoldfire__ */

/* ColdFire implementation of non-restoring division algorithm from
   Hennessy & Patterson, Appendix A. */
SYM (__udivsi3):
	movel	sp@(8), d1	/* d1 = divisor */
	movel	sp@(4), d0	/* d0 = dividend */

/* Register-args entry, same convention as above.  The remainder is the
   final partial remainder (p) of the loop.  */
SYM (__udivmodsi4_reg):
	link	a6,IMM (-12)
	moveml	d2-d4,sp@
	clrl	d2		| clear p
	moveq	IMM (31),d4
L1:	addl	d0,d0		| shift reg pair (p,a) one bit left
	addxl	d2,d2
	movl	d2,d3		| subtract b from p, store in tmp.
	subl	d1,d3
	jcs	L2		| if no carry,
	bset	IMM (0),d0	| set the low order bit of a to 1,
	movl	d3,d2		| and store tmp in p.
L2:	subql	IMM (1),d4
	jcc	L1
	movel	d2,d1		| remainder in d1
	moveml	sp@,d2-d4	| restore data registers
	unlk	a6		| and return
	rts
#endif /* __mcf5200__ || __mcoldfire__ */
	EXPORT_SYMBOL(__udivsi3)
