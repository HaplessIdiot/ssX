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

/* $XFree86: mit/server/ddx/x386/SuperProbe/NCR.c,v 2.5 1994/04/15 05:09:42 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {SEQ_IDX, SEQ_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor NCR_Descriptor = {
	"NCR",
	Probe_NCR,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	NULL,
};

Bool Probe_NCR(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old, tmp;

	EnableIOPorts(NUMPORTS, Ports);

	if (testinx2(SEQ_IDX, 0x05, 0x05))
	{
		old = rdinx(SEQ_IDX, 0x05);
		wrinx(0x3C4, 0x05, 0x00);
		if (!testinx2(SEQ_IDX, 0x10, 0xFF))
		{
			wrinx(SEQ_IDX, 0x05, 0x01);
			if (testinx2(SEQ_IDX, 0x10, 0xFF))
			{
				result = TRUE;
				tmp = rdinx(SEQ_IDX, 0x08) >> 4;
				if (tmp == 0)
				{
					*Chipset = CHIP_NCR77C22;
				}
				else if (tmp == 1)
				{
					*Chipset = CHIP_NCR77C21;
				}
				else if (tmp == 2)
				{
					if ((rdinx(SEQ_IDX, 0x08) & 0x0F) >= 8)
					{
						*Chipset = CHIP_NCR77C22EP;
					}
					else
					{
						*Chipset = CHIP_NCR77C22E;
					}
				}
				else if (tmp == 3)
				{
					*Chipset = CHIP_NCR77C32B;
				}
				else
				{
					Chip_data = tmp;
					*Chipset = CHIP_NCR_UNK;
				}
			}
		}
		wrinx(SEQ_IDX, 0x05, old);
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
