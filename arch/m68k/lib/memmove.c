/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#include <linux/module.h>
#include <linux/string.h>

void *memmove(void *dest, const void *src, size_t n)
{
	void *xdest = dest;
	size_t temp;

	if (!n)
		return xdest;

	if (dest < src) {
		if ((long)dest & 1) {
			char *cdest = dest;
			const char *csrc = src;
			*cdest++ = *csrc++;
			dest = cdest;
			src = csrc;
			n--;
		}
#if defined(CONFIG_M68000)
		if ((long)src & 1) {
			char *cdest = dest;
			const char *csrc = src;
			for (; n; n--)
				*cdest++ = *csrc++;
			return xdest;
		}
#endif
		if (n > 2 && (long)dest & 2) {
			short *sdest = dest;
			const short *ssrc = src;
			*sdest++ = *ssrc++;
			dest = sdest;
			src = ssrc;
			n -= 2;
		}
		temp = n >> 2;
		if (temp) {
			long *ldest = dest;
			const long *lsrc = src;
#if defined(CONFIG_COLDFIRE)
			temp--;
			do
				*ldest++ = *lsrc++;
			while (temp--);
#else
			size_t temp1;
			asm volatile (
				"	movel %2,%3\n"
				"	andw  #7,%3\n"
				"	lsrl  #3,%2\n"
				"	negw  %3\n"
#if defined(CONFIG_M68000)
				/*
				 * 68000 has no scaled index mode; each
				 * movel below is one word, so double the
				 * index by hand.
				 */
				"	addw  %3,%3\n"
				"	jmp   %%pc@(1f,%3:w)\n"
#else
				"	jmp   %%pc@(1f,%3:w:2)\n"
#endif
				"4:	movel %0@+,%1@+\n"
				"	movel %0@+,%1@+\n"
				"	movel %0@+,%1@+\n"
				"	movel %0@+,%1@+\n"
				"	movel %0@+,%1@+\n"
				"	movel %0@+,%1@+\n"
				"	movel %0@+,%1@+\n"
				"	movel %0@+,%1@+\n"
				"1:	dbra  %2,4b\n"
				"	clrw  %2\n"
				"	subql #1,%2\n"
				"	jpl   4b"
				: "=a" (lsrc), "=a" (ldest), "=d" (temp),
				  "=&d" (temp1)
				: "0" (lsrc), "1" (ldest), "2" (temp));
#endif
			dest = ldest;
			src = lsrc;
		}
		if (n & 2) {
			short *sdest = dest;
			const short *ssrc = src;
			*sdest++ = *ssrc++;
			dest = sdest;
			src = ssrc;
		}
		if (n & 1) {
			char *cdest = dest;
			const char *csrc = src;
			*cdest = *csrc;
		}
	} else {
		dest = (char *)dest + n;
		src = (const char *)src + n;
		if ((long)dest & 1) {
			char *cdest = dest;
			const char *csrc = src;
			*--cdest = *--csrc;
			dest = cdest;
			src = csrc;
			n--;
		}
#if defined(CONFIG_M68000)
		if ((long)src & 1) {
			char *cdest = dest;
			const char *csrc = src;
			for (; n; n--)
				*--cdest = *--csrc;
			return xdest;
		}
#endif
		if (n > 2 && (long)dest & 2) {
			short *sdest = dest;
			const short *ssrc = src;
			*--sdest = *--ssrc;
			dest = sdest;
			src = ssrc;
			n -= 2;
		}
		temp = n >> 2;
		if (temp) {
			long *ldest = dest;
			const long *lsrc = src;
#if defined(CONFIG_COLDFIRE)
			temp--;
			do
				*--ldest = *--lsrc;
			while (temp--);
#else
			size_t temp1;
			asm volatile (
				"	movel %2,%3\n"
				"	andw  #7,%3\n"
				"	lsrl  #3,%2\n"
				"	negw  %3\n"
#if defined(CONFIG_M68000)
				/*
				 * 68000 has no scaled index mode; each
				 * movel below is one word, so double the
				 * index by hand.
				 */
				"	addw  %3,%3\n"
				"	jmp   %%pc@(1f,%3:w)\n"
#else
				"	jmp   %%pc@(1f,%3:w:2)\n"
#endif
				"4:	movel %0@-,%1@-\n"
				"	movel %0@-,%1@-\n"
				"	movel %0@-,%1@-\n"
				"	movel %0@-,%1@-\n"
				"	movel %0@-,%1@-\n"
				"	movel %0@-,%1@-\n"
				"	movel %0@-,%1@-\n"
				"	movel %0@-,%1@-\n"
				"1:	dbra  %2,4b\n"
				"	clrw  %2\n"
				"	subql #1,%2\n"
				"	jpl   4b"
				: "=a" (lsrc), "=a" (ldest), "=d" (temp),
				  "=&d" (temp1)
				: "0" (lsrc), "1" (ldest), "2" (temp));
#endif
			dest = ldest;
			src = lsrc;
		}
		if (n & 2) {
			short *sdest = dest;
			const short *ssrc = src;
			*--sdest = *--ssrc;
			dest = sdest;
			src = ssrc;
		}
		if (n & 1) {
			char *cdest = dest;
			const char *csrc = src;
			*--cdest = *--csrc;
		}
	}
	return xdest;
}
EXPORT_SYMBOL(memmove);
