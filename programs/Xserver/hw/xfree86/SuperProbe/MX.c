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

/* $XFree86: mit/server/ddx/x386/SuperProbe/MX.c,v 2.4 1994/03/02 08:04:56 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {SEQ_REG, SEQ_IDX};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_MX __STDCARGS((int));

Chip_Descriptor MX_Descriptor = {
	"MX",
	Probe_MX,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_MX,
};

Bool Probe_MX(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old;

	EnableIOPorts(NUMPORTS, Ports);

	old = rdinx(SEQ_IDX, 0xA7);
	wrinx(SEQ_IDX, 0xA7, 0x00);
	if (!testinx(SEQ_IDX, 0xC5))
	{
		wrinx(SEQ_IDX, 0xA7, 0x87);
		if (testinx(SEQ_IDX, 0xC5))
		{
			result = TRUE;
			if ((rdinx(SEQ_IDX, 0x26) & 0x01) == 0)
			{
				*Chipset = CHIP_MX68010;
			}
			else
			{
				*Chipset = CHIP_MX68000;
			}
		}
	}
	wrinx(SEQ_IDX, 0xA7, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

/* ARGSUSED */
static int MemProbe_MX(Chipset)
int Chipset;
{
	Byte old;
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Unlock
	 */
	old = rdinx(SEQ_IDX, 0xA7);
	wrinx(SEQ_IDX, 0xA7, 0x87);

	/*
	 * Check
	 */
	switch ((rdinx(SEQ_IDX, 0xC2) >> 2) & 0x03)
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
	}

	/*
	 * Lock
	 */
	wrinx(SEQ_IDX, 0xA7, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
