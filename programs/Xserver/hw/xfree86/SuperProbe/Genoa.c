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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Genoa.c,v 2.3 1994/02/28 14:09:23 dawes Exp $ */

#include "Probe.h"

static int MemProbe_Genoa __STDCARGS((int));

Chip_Descriptor Genoa_Descriptor = {
	"Genoa",
	Probe_Genoa,
	NULL,
	0,
	FALSE,
	TRUE,
	TRUE,
	MemProbe_Genoa,
};

Bool Probe_Genoa(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte addr, data[4];

	if (ReadBIOS(0x37, &addr, 1) != 1)
	{
		fprintf(stderr, "%s: Failed to read Genoa BIOS address.\n",
			MyName);
		return(FALSE);
	}
	if (ReadBIOS((unsigned)addr, data, 4) != 4)
	{
		fprintf(stderr, "%s: Failed to read Genoa BIOS signature.\n",
			MyName);
		return(FALSE);
	}
	if ((data[0] == 0x77) && (data[2] == 0x99) && (data[3] == 0x66))
	{
		/*
		 * Genoa also has ET3000 and (possibly) ET4000 based
		 * boards that match this signature.  We only match
		 * the ones with Genoa chips, and let other probe
		 * functions deal with other chipsets.
		 */
		switch (data[1])
		{
		case 0x00:
			*Chipset = CHIP_G_6200;
			result = TRUE;
			break;
		case 0x11:
			*Chipset = CHIP_G_6400;
			result = TRUE;
			break;
		case 0x22:
			*Chipset = CHIP_G_6100;
			result = TRUE;
			break;
		}
	}
	return(result);
}

static int MemProbe_Genoa(Chipset)
int Chipset;
{
	int Mem = 0;

	switch (Chipset)
	{
	case CHIP_G_6100:
	case CHIP_G_6200:
		Mem = 256;
		break;
	case CHIP_G_6400:
		Mem = 512;
		break;
	}

	return(Mem);
}
