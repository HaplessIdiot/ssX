/*
 * (c) Copyright 1993,1994 by David Wexelblat <dwex@xfree86.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * DAVID WEXELBLAT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of David Wexelblat shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from David Wexelblat.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/SuperProbe/ATIMach.c,v 3.0 1994/05/14 06:50:38 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {ROM_ADDR_1,DESTX_DIASTP,READ_SRC_X,
		       CONFIG_STATUS_1,MISC_OPTIONS};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_ATIMach __STDCARGS((int));

Chip_Descriptor ATIMach_Descriptor = {
	"ATI_Mach",
	Probe_ATIMach,
	Ports,
	NUMPORTS,
	TRUE,
	FALSE,
	FALSE,
	MemProbe_ATIMach,
};

#define WaitIdleEmpty() { int i; \
			  for (i=0; i < 100000; i++) \
				if (!(inpw(GP_STAT) & (GPBUSY | 1))) \
					break; \
			}

Bool Probe_ATIMach(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Word tmp;
	int chip = -1;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Check for 8514/A registers first.  Don't read BIOS, or an
	 * attached 8514 Ultra won't be detected (the slave SVGA's BIOS
	 * is in the normal SVGA place).
	 */
	tmp = inpw(ROM_ADDR_1);
	outpw(ROM_ADDR_1, 0x5555);
	WaitIdleEmpty();
	if (inpw(ROM_ADDR_1) == 0x5555)
	{
		outpw(ROM_ADDR_1, 0x2A2A);
		WaitIdleEmpty();
		if (inpw(ROM_ADDR_1) == 0x2A2A)
		{
			result = TRUE;
		}
	}
	outpw(ROM_ADDR_1, tmp);
	if (result)
	{
		/*
		 * Accelerator is really present; now figure
		 * out which one.
		 */
		outpw(DESTX_DIASTP, 0xAAAA);
		WaitIdleEmpty();
		if (inpw(READ_SRC_X) != 0x02AA)
		{
			chip = CHIP_MACH8;
		}
		else
		{
			chip = CHIP_MACH32;
		}
		outpw(DESTX_DIASTP, 0x5555);
		WaitIdleEmpty();
		if (inpw(READ_SRC_X) != 0x0555)
		{
			if (chip != CHIP_MACH8)
			{
				/*
				 * Something bizarre is happening.
				 */
				chip = -1;
				result = FALSE;
			}
		}
		else
		{
			if (chip != CHIP_MACH32)
			{
				/*
				 * Something bizarre is happening.
				 */
				chip = -1;
				result = FALSE;
			}
		}
	}

	if (chip != -1)
	{
		*Chipset = chip;
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

static int MemProbe_ATIMach(Chipset)
int Chipset;
{
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);
	if (Chipset == CHIP_MACH8)
	{
		if (inpw(CONFIG_STATUS_1) & 0x0020)
		{
			Mem = 1024;
		}
		else
		{
			Mem = 512;
		}
	}
	else
	{
		switch ((inpw(MISC_OPTIONS) & 0x000C) >> 2)
		{
		case 0x00:
			Mem = 512;
			break;
		case 0x01:
			Mem = 1024;
			break;
		case 0x02:
			Mem = 2048;
			break;
		case 0x03:
			Mem = 4096;
			break;
		}
	}
	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
