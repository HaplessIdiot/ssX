/*
 * Copyright 1993,1994 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: mit/server/ddx/x386/SuperProbe/ATIMach.c,v 2.8 1994/04/15 05:09:36 dawes Exp $ */

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
