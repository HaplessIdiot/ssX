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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Video7.c,v 2.4 1994/02/28 14:10:03 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, SEQ_IDX, SEQ_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Video7_Descriptor = {
	"Video7",
	Probe_Video7,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	NULL,
};

Bool Probe_Video7(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old, old1;
	Word id;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);

	old = rdinx(0x3C4, 0x06);
	wrinx(0x3C4, 0x06, 0xEA);		/* enable extensions */
	old1 = rdinx(CRTC_IDX, 0x0C);
	wrinx(CRTC_IDX, 0x0C, 0x55);
	id = rdinx(CRTC_IDX, 0x1F);
	wrinx(CRTC_IDX, 0x0C, old1);
	if (id == (0x55 ^ 0xEA))
	{
		/*
		 * It's Video7
		 */
		result = TRUE;
		id = (rdinx(SEQ_IDX, 0x8F) << 8) | rdinx(0x3C4, 0x8E);
		if ((id >= 0x7000) && (id <= 0x70FF))
		{
			*Chipset = CHIP_V7_FWRITE;
		}
		else if ((id >= 0x7140) && (id <= 0x714F))
		{
			*Chipset = CHIP_V7_1024i;
		}
		else if (id == 0x7151)
		{
			*Chipset = CHIP_V7_VRAM2_B;
		}
		else if (id == 0x7152)
		{
			*Chipset = CHIP_V7_VRAM2_C;
		}
		else if (id == 0x7760)
		{
			*Chipset = CHIP_HT216BC;
		}
		else if (id == 0x7763)
		{
			*Chipset = CHIP_HT216D;
		}
		else if (id == 0x7764)
		{
			*Chipset = CHIP_HT216E;
		}
		else if (id == 0x7765)
		{
			*Chipset = CHIP_HT216F;
		}
		else if ((id >= 0x8000) && (id < 0xFFFF))
		{
			*Chipset = CHIP_V7_VEGA;
		}
		else
		{
			Chip_data = id;
			*Chipset = CHIP_V7_UNKNOWN;
		}
	}
	wrinx(0x3C4, 6, old);		/* disable extensions */
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
