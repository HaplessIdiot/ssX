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

/* $XFree86: mit/server/ddx/x386/SuperProbe/HMC.c,v 2.0 1994/02/28 14:09:25 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {SEQ_IDX, SEQ_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_HMC __STDCARGS((int));

Chip_Descriptor HMC_Descriptor = {
	"HMC",
	Probe_HMC,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_HMC,
};

Bool Probe_HMC(Chipset)
int *Chipset;
{
	Bool result = FALSE;

	EnableIOPorts(NUMPORTS, Ports);

	if (testinx(SEQ_IDX, 0xE7) && testinx(SEQ_IDX, 0xEE))
	{
		result = TRUE;
		*Chipset = CHIP_HM86304;
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

/* ARGSUSED */
static int MemProbe_HMC(Chipset)
int Chipset;
{
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	if (rdinx(SEQ_IDX, 0xE7) & 0x10)
	{
		Mem = 512;
	}
	else
	{
		Mem = 256;
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
