/* $XFree86: $ */


#include "compiler.h"

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

