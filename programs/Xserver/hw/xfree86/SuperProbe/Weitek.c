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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Weitek.c,v 2.1 1994/03/07 13:56:09 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3CD, SEQ_IDX, SEQ_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Weitek_Descriptor = {
	"Weitek",
	Probe_Weitek,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	NULL,
};

static Byte WeitekEnable(Flag)
Byte Flag;
{
	Byte new, old, x;

	old = rdinx(SEQ_IDX, 0x11);
	for (x=0; x < 10; x++)
	{
		;
	}
	outp(SEQ_REG, old);
	for (x=0; x < 10; x++)
	{
		;
	}
	outp(SEQ_REG, old);
	for (x=0; x < 10; x++)
	{
		;
	}
	new = inp(SEQ_REG);
	for (x=0; x < 10; x++)
	{
		;
	}
	outp(SEQ_REG, (new & 0x9F) | Flag);

	return(old);
}

Bool Probe_Weitek(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old, ver;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Disable extensions
	 */
	old = WeitekEnable(0x60);
	if (!testinx(SEQ_IDX, 0x12))
	{
		/*
		 * Enable extensions
		 */
		WeitekEnable(0);
		if (testinx(SEQ_IDX, 0x12))
		{
			if (tstrg(0x3CD, 0xFF))
			{
				result = TRUE;
				ver = rdinx(SEQ_IDX, 0x07) >> 5;
				switch (ver)
				{
				case 0x00:	/* guess */
					*Chipset = CHIP_WEIT_5086;
					break;
				case 0x01:
					*Chipset = CHIP_WEIT_5186;
					break;
				case 0x02:
					*Chipset = CHIP_WEIT_5286;
					break;
				default:
					Chip_data = ver;
					*Chipset = CHIP_WEIT_UNK;
					break;
				}
			}
		}
	}

	wrinx(SEQ_IDX, 0x12, old);
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
