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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Ahead.c,v 2.6 1994/04/15 05:09:37 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {GRC_IDX, GRC_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_Ahead __STDCARGS((int));

Chip_Descriptor Ahead_Descriptor = {
	"Ahead",
	Probe_Ahead,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_Ahead,
};

Bool Probe_Ahead(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old, tmp;

	EnableIOPorts(NUMPORTS, Ports);
	old = rdinx(GRC_IDX, 0x0F);
	wrinx(GRC_IDX, 0x0F, 0);
	if (!testinx2(GRC_IDX, 0x0C, 0xFB))
	{
		wrinx(GRC_IDX, 0x0F, 0x20);
		if (testinx2(GRC_IDX, 0x0C, 0xFB))
		{
			result = TRUE;
			tmp = rdinx(GRC_IDX, 0x0F) & 0x0F;
			switch (tmp)
			{
			case 0x00:
				*Chipset = CHIP_AHEAD_A;
				break;
			case 0x01:
				*Chipset = CHIP_AHEAD_B;
				break;
			default:
				Chip_data = tmp;
				*Chipset = CHIP_AHEAD_UNK;
				break;
			}
		}
	}
	wrinx(GRC_IDX, 0x0F, old);
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

/* ARGSUSED */
static int MemProbe_Ahead(Chipset)
int Chipset;
{
	Byte old;
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Unlock
	 */
	old = rdinx(GRC_IDX, 0x0F);
	setinx(GRC_IDX, 0x0F, 0x20);

	/*
	 * Check
	 */
	switch (rdinx(GRC_IDX, 0x1F) & 0x03)
	{
	case 0x00:
		Mem = 256;
		break;
	case 0x01:
		Mem = 512;
		break;
	case 0x03:
		Mem = 1024;
		break;
	}

	/*
	 * Lock
	 */
	wrinx(GRC_IDX, 0x0F, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
