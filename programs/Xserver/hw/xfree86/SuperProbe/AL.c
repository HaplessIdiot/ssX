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

/* $XFree86: mit/server/ddx/x386/SuperProbe/AL.c,v 2.5 1994/04/15 05:09:35 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_AL __STDCARGS((int));

Chip_Descriptor AL_Descriptor = {
	"AL",
	Probe_AL,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE,
	MemProbe_AL,
};

Bool Probe_AL(Chipset)
int *Chipset;
{
	Byte old;
	Bool result = FALSE;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);

	old = rdinx(CRTC_IDX, 0x1A);
	clrinx(CRTC_IDX, 0x1A, 0x10);
	if (!testinx2(CRTC_IDX, 0x19, 0xCF))
	{
		setinx(CRTC_IDX, 0x1A, 0x10);
		if ((testinx2(CRTC_IDX, 0x19, 0xCF) && 
		    (testinx2(CRTC_IDX, 0x1A, 0x3F))))
		{
			result = TRUE;
			switch (rdinx(CRTC_IDX, 0x1A) >> 6)
			{
			case 0x03:
				*Chipset = CHIP_AL2101;
				break;
			case 0x02:
				*Chipset = CHIP_AL2228;
				break;
			default:
				Chip_data = rdinx(CRTC_IDX, 0x1A);
				*Chipset = CHIP_AL_UNKNOWN;
				break;
			}
		}
	}
	wrinx(CRTC_IDX, 0x1A, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

/* ARGSUSED */
static int MemProbe_AL(Chipset)
int Chipset;
{
	Byte old;
	int Mem;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Unlock
	 */
	old = rdinx(CRTC_IDX, 0x1A);
	setinx(CRTC_IDX, 0x1A, 0x10);

	/*
	 * Check
	 */
	switch (rdinx(CRTC_IDX, 0x1E) & 0x03)
	{
	case 0x00:
		Mem = 256;
		break;
	case 0x01:
		Mem = 512;
		break;
	case 0x02:
		Mem = 1024;
		break;
	case 0x03:
		Mem = 2048;
		break;
	}

	/*
	 * Lock
	 */
	wrinx(CRTC_IDX, 0x1A, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
