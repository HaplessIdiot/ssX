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

/* $XFree86: mit/server/ddx/x386/SuperProbe/8514.c,v 2.3 1994/02/28 14:09:09 dawes Exp $ */

#include "Probe.h"

/*
 * Check for basic 8514 functionality.  8514 extended functionality will
 * be checked for elsewhere.
 */

static Word Ports[] = {SUBSYS_CNTL,ERR_TERM};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_8514 __STDCARGS((int));

Chip_Descriptor IBM8514_Descriptor = {
	"8514/A",
	Probe_8514,
	Ports,
	NUMPORTS,
	TRUE,
	FALSE,
	FALSE,
	MemProbe_8514,
};

Bool Probe_8514(Chipset)
int *Chipset;
{
	Bool result = FALSE;

	EnableIOPorts(NUMPORTS, Ports);

	/* 
	 * Reset the 8514/A, and disable all interrupts. 
	 */
	outpw(SUBSYS_CNTL, GPCTRL_RESET|CHPTEST_NORMAL);
	outpw(SUBSYS_CNTL, GPCTRL_ENAB|CHPTEST_NORMAL);

	/*
	 * Check to see if an 8514/A is actually installed by writing
	 * to the ERR_TERM register and reading back.  The 0x5A5A value
	 * is entirely arbitrary.
	 */
	outpw(ERR_TERM, 0x5A5A);
	if (inpw(ERR_TERM) == 0x5A5A)
	{
		/* 
		 * Let's make certain.
		 */
		outpw(ERR_TERM, 0x5555);
		if (inpw(ERR_TERM) == 0x5555)
		{
			if (tstrg(EXT_CONF_3, 0xF0))
			{
				*Chipset = CHIP_CT480;
			}
			else
			{
				*Chipset = CHIP_8514;
			}
			result = TRUE;
		}
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

/* ARGSUSED */
static int MemProbe_8514(Chipset)
int Chipset;
{
	int Mem;

	EnableIOPorts(NUMPORTS, Ports);

	if (inpw(SUBSYS_CNTL) & 0x0080)
	{
		Mem = 1024;
	}
	else
	{
		Mem = 512;
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
