/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/CirrusClk.c,v 3.0 1994/10/20 06:09:41 dawes Exp $ */

/*
 * Programming of the built-in Cirrus clock generator.
 * Harm Hanemaayer <hhanemaa@cs.ruu.nl>
 */
 
#include "compiler.h"
#include "misc.h"
#define NO_OSLIB_PROTOTYPES
#include "xf86_OSlib.h"

/* CLOCK_FACTOR is double the osc freq in kHz (osc = 14.31818 MHz) */
#define CLOCK_FACTOR 28636

/* clock in kHz is (numer * CLOCK_FACTOR / (denom & 0x3E)) >> (denom & 1) */
#define CLOCKVAL(n, d) \
     ((((n) & 0x7F) * CLOCK_FACTOR / ((d) & 0x3E)) >> ((d) & 1))

/*
 * This function returns the 7-bit numerator and 6-bit denominator/post-scalar
 * value that corresponds to the closest clock found. If the MCLK is very close
 * to the requested frequency, it sets a flag so that the MCLK can be used
 * as VCLK on chips that support it.
 */

int CirrusFindClock(freq, num_out, den_out, usemclk_out)
	int freq;
	int *num_out;
	int *den_out;
	int *usemclk_out;
{
	int n;
	int num, den;
	int ffreq, mindiff;
	int mclk;

	mindiff = freq; 
	for (n = 0x10; n < 0x7f; n++) {
		int d;
		for (d = 0x14; d < 0x3f; d++) {
			int c, diff;
			c = CLOCKVAL(n, d);
			diff = abs(c - freq);
			if (diff < mindiff) {
				mindiff = diff;
				num = n;
				den = d;
				ffreq = c;
			}
		}
	}

	*num_out = num;
	*den_out = den;

	/* Calculate the MCLK. */
	outb(0x3c4, 0x0f);
	mclk = 14318 * (inb(0x3c5) & 0x3f) / 8;
	/*
	 * Favour MCLK as VLCK if it matches as good as the found clock,
	 * or if it is within 0.2 MHz of the request clock. A VCLK close
	 * to MCLK can cause instability.
	 */
	if (abs(mclk - freq) <= mindiff + 10 || abs(mclk - freq) <= 200)
		*usemclk_out = TRUE;
	else
		*usemclk_out = FALSE;

	return 0;
}


int CirrusSetClock(freq)
	int freq;
{
	int num, den, usemclk;
	unsigned char tmp;

	CirrusFindClock(freq, &num, &den, usemclk);

	/*
	 * The 'Use MCLK as VCLK' flag is ignored.
	 * This function is only called during start-up if the
	 * "probe_clocks" option is specified.
	 */

	/* Set VLCK3. */
	outb(0x3c4, 0x0e);
	tmp = inb(0x3c5);
	outb(0x3c5, (tmp & 0x80) | num);
	outb(0x3c4, 0x1e);
	tmp = inb(0x3c5);
	outb(0x3c5, (tmp & 0xc0) | den);
	return 0;
}
