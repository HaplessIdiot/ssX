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

/* $XFree86: mit/server/ddx/x386/SuperProbe/WD.c,v 2.6 1994/04/15 05:09:51 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, GRC_IDX, GRC_REG, SEQ_IDX, SEQ_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_WD __STDCARGS((int));

Chip_Descriptor WD_Descriptor = {
	"WD",
	Probe_WD,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_WD,
};

Bool Probe_WD(Chipset)
int *Chipset;
{
	Byte old, old1, old2, old3;
	Word ver;
	Bool result = FALSE;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);

	old = rdinx(GRC_IDX, 0x0F);
	setinx(GRC_IDX, 0x0F, 0x17);	/* Lock registers */
	if (!testinx2(GRC_IDX, 0x09, 0x7F))
	{
		wrinx(GRC_IDX, 0x0F, 0x05);	/* Unlock them again */
		if (testinx2(GRC_IDX, 0x09, 0x7F))
		{
			result = TRUE;
			old2 = rdinx(CRTC_IDX, 0x29);
			/* Unlock WD90Cxx regs */
			modinx(CRTC_IDX, 0x29, 0x8F, 0x85);
			if (!testinx(CRTC_IDX, 0x2B))
			{
				*Chipset = CHIP_WD_PVGA1;
			}
			else
			{
				old1 = rdinx(SEQ_IDX, 0x06);
				wrinx(SEQ_IDX, 0x06, 0x48);
				if (!testinx2(SEQ_IDX, 0x07, 0xF0))
				{
					*Chipset = CHIP_WD_90C00;
				}
				else if (!testinx(SEQ_IDX, 0x10))
				{
					if (testinx2(CRTC_IDX, 0x31, 0x68))
					{
						*Chipset = CHIP_WD_90C22;
					}
					else if (testinx2(CRTC_IDX, 0x31, 0x90))
					{
						*Chipset = CHIP_WD_90C20A;
					}
					else
					{
						*Chipset = CHIP_WD_90C20;
					}
					wrinx(CRTC_IDX, 0x34, 0xA6);
					if (rdinx(CRTC_IDX, 0x32) & 0x20)
					{
						wrinx(CRTC_IDX, 0x34, 0x00);
					}
				}
				else if (testinx2(SEQ_IDX, 0x14, 0x0F))
				{
					old3 = rdinx(CRTC_IDX, 0x34);
					wrinx(CRTC_IDX, 0x34, 0x00);
					ver = ((rdinx(CRTC_IDX, 0x36) << 8) |
					       (rdinx(CRTC_IDX, 0x37)));
					switch (ver)
					{
					case 0x3234:
						*Chipset = CHIP_WD_90C24;
						break;
					case 0x3236:
						*Chipset = CHIP_WD_90C26;
						break;
					case 0x3237:	/* Guess */
						*Chipset = CHIP_WD_90C27;
						break;
					case 0x3330:
						*Chipset = CHIP_WD_90C30;
						break;
					case 0x3331:
						*Chipset = CHIP_WD_90C31;
						break;
					case 0x3333:
						*Chipset = CHIP_WD_90C33;
						break;
					default:
						Chip_data = ver & 0xFF;
						*Chipset = CHIP_WD_UNK;
						break;
					}
					wrinx(CRTC_IDX, 0x34, old3);
				}
				else if (!testinx2(0x3C4, 0x10, 0x04))
				{
					*Chipset = CHIP_WD_90C10;
				}
				else
				{
					*Chipset = CHIP_WD_90C11;
				}
				wrinx(0x3C4, 0x06, old1);
			}
			wrinx(CRTC_IDX, 0x29, old2);
		}
	}
	wrinx(0x3CE, 0x0F, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

static int MemProbe_WD(Chipset)
int Chipset;
{
	Byte old;
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Unlock
	 */
	old = rdinx(GRC_IDX, 0x0F);
	wrinx(GRC_IDX, 0x0F, 0x05);

	/*
	 * Check
	 */
	switch (rdinx(GRC_IDX, 0x0B) >> 6)
	{
	case 0x00:
	case 0x01:
		Mem = 256;
		break;
	case 0x02:
		Mem = 512;
		break;
	case 0x03:
		Mem = 1024;
		break;
	}

	if ((Chipset >= CHIP_WD_90C33) &&
	    (rdinx(CRTC_IDX, 0x3E) & 0x80))
	{
		Mem = 2048;
	}

	/*
	 * Lock
	 */
	wrinx(GRC_IDX, 0x0F, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
