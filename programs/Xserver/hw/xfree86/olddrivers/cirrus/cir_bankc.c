/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir_bankc.c,v 1.3 1998/01/24 16:57:56 hohndel Exp $ */


#include "compiler.h"

/*
 * Low level VGA bankswitching routines.
 *
 * Comments from Intel assembly code:
 * 
 * We always have 256 granules, each either 4KB if total memory is 1MB,
 * or 16KB if total memory is 2 (-4MB?). Segments addresses are expressed
 * in granules, and the granule address is set in AH. Segment numbers
 * passed to these routines are expressed in multiples of 32KB. Thus
 * we must convert this number to a granule, and then shift it into
 * AH.
 *
 * Not sure if these portable funcs will work or not since their callers
 * expected to deliver the segment number in %eax (appears to be first
 * (function argument). Probably doesn't matter anyway since non-intel
 * machines are mostly interested in linear mode cirrus chips which do
 * not use the back switching routines.  However, these will suffice
 * to resolve the undefined symbols in the driver ...
 */

#define GRX	0x3ce
#define cirrus1MBSHIFT 11	/* 8 + (lg(32KB) - lg(4KB))     */
#define cirrus2MBSHIFT 9	/* 8 + (lg(32KB) - lg(16KB))    */

void
cirrusSetRead(int seg)
{
	outw(GRX, (seg << cirrus1MBSHIFT) | 9);
}

void
cirrusSetWrite(int seg)
{
	outw(GRX, (seg << cirrus1MBSHIFT) | 0x0a);
}

void
cirrusSetReadWrite(int seg)
{
	outw(GRX, (seg << cirrus1MBSHIFT) | 0x0a);
}


void
cirrusSetRead2MB(int seg)
{
	outw(GRX, (seg << cirrus2MBSHIFT) | 9);
}

void
cirrusSetWrite2MB(int seg)
{
	outw(GRX, (seg << cirrus2MBSHIFT) | 0x0a);
}

void
cirrusSetReadWrite2MB(int seg)
{
	outw(GRX, (seg << cirrus2MBSHIFT) | 0x0a);
}
