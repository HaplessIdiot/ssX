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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Primus.c,v 2.3 1994/02/28 14:09:45 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {GRC_IDX, GRC_REG, 0x3D6, 0x3D7};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Primus_Descriptor = {
	"Primus",
	Probe_Primus,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	NULL,
};

Bool Probe_Primus(Chipset)
int *Chipset;
{
	Bool result = FALSE;

	EnableIOPorts(NUMPORTS, Ports);

	if (testinx2(GRC_IDX, 0x3D, 0x3F) &&
	    tstrg(0x3D6, 0x1F) &&
	    tstrg(0x3D7, 0x1F))
	{
		result = TRUE;
		*Chipset = CHIP_P2000;
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
