/*
 * Copyright 1994 by David Wexelblat <dwex@goblin.org>
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

/* $XFree86: mit/server/ddx/x386/SuperProbe/UMC.c,v 2.1 1994/03/02 08:04:58 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3BF, SEQ_IDX, SEQ_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_UMC __STDCARGS((int));

Chip_Descriptor UMC_Descriptor = {
	"UMC",
	Probe_UMC,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_UMC,
};

Bool Probe_UMC(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old;

	EnableIOPorts(NUMPORTS, Ports);

	old = inp(0x3BF);
	outp(0x3BF, 0x03);
	if (!testinx(SEQ_IDX, 0x06))
	{
		outp(0x3BF, 0xAC);
		if (testinx(SEQ_IDX, 0x06))
		{
			result = TRUE;
			*Chipset = CHIP_UMC_408;
		}
	}
	outp(0x3BF, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

/* ARGSUSED */
static int MemProbe_UMC(Chipset)
int Chipset;
{
	Byte old;
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Unlock
	 */
	old = inp(0x3BF);
	outp(0x3BF, 0xAC);

	/*
	 * Check
	 */
	switch(rdinx(SEQ_IDX, 0x07) >> 6)
	{
	case 0x00:
		Mem = 256;
		break;
	case 0x01:
		Mem = 512;
		break;
	case 0x02:
	case 0x03:
		Mem = 1024;
		break;
	}

	/*
	 * Lock
	 */
	outp(0x3BF, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
