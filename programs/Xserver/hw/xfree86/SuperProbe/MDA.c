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

/* $XFree86: mit/server/ddx/x386/SuperProbe/MDA.c,v 2.0 1994/02/28 14:09:28 dawes Exp $ */

#include "Probe.h"

/*
 * Assume that the CRT Controller is at 0x3B4 (either no other adapter,
 * or an EGA/CGA is installed in Color mode).  Look for the cursor
 * postition register.
 */
static Word Ports[] = {0x3B4, 0x3B5};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Bool Probe_MDA(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte tmp;

	EnableIOPorts(NUMPORTS, Ports);
	tmp = rdinx(0x3B4, 0x0F);
	if (testinx(0x3B4, 0x0F))
	{
		result = TRUE;
		*Chipset = CHIP_MDA;
	}
	wrinx(0x3B4, 0x0F, tmp);
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
