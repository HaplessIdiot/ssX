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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Yamaha.c,v 2.3 1994/02/28 14:10:07 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3D4, 0x3D5};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Yamaha_Descriptor = {
	"Yamaha",
	Probe_Yamaha,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	NULL,
};

Bool Probe_Yamaha(Chipset)
int *Chipset;
{
	Bool result = FALSE;

	EnableIOPorts(NUMPORTS, Ports);
	if (testinx2(0x3D4, 0x7C, 0x7C))
	{
		result = TRUE;
		*Chipset = CHIP_YAMAHA6388;
	}
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
