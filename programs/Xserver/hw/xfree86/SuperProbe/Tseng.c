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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Tseng.c,v 2.8 1994/05/03 08:09:44 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, 0x000, 0x3BF, 0x3CB, 0x3CD, 
		       ATR_IDX, ATR_REG_R, SEQ_IDX, SEQ_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_Tseng __STDCARGS((int));

Chip_Descriptor Tseng_Descriptor = {
	"Tseng",
	Probe_Tseng,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_Tseng,
};

Bool Probe_Tseng(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old, old1, ver;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	Ports[2] = inp(MISC_OUT_R) & 0x01 ? 0x3D8 : 0x3B8;
	EnableIOPorts(NUMPORTS, Ports);

	old = inp(0x3BF);
	old1 = inp(Ports[2]);
	outp(0x3BF, 0x03);
	outp(Ports[2], 0xA0);
	if (tstrg(0x3CD, 0x3F)) 
	{
		result = TRUE;
		if (testinx2(CRTC_IDX, 0x33, 0x0F))
		{
			if (tstrg(0x3CB, 0x33))
			{
				ver = rdinx(0x217A, 0xEC);
				switch (ver >> 4)
				{
				case 0x00:
					*Chipset = CHIP_ET4000W32;
					break;
				case 0x01:
					*Chipset = CHIP_ET4000W32I;
					break;
				case 0x02:
					*Chipset = CHIP_ET4KW32P_A;
					break;
				case 0x05:
					*Chipset = CHIP_ET4KW32P_O;
					break;
				default:
					Chip_data = ver >> 4;
					*Chipset = CHIP_TSENG_UNK;
					break;
				}
			}
			else
			{
				*Chipset = CHIP_ET4000;
			}
		}
		else
		{
			*Chipset = CHIP_ET3000;
		}
	}
	outp(Ports[2], old1);
	outp(0x3BF, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

static int MemProbe_Tseng(Chipset)
int Chipset;
{
	Byte Save[2];
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	/* 
	 * Unlock 
	 */
	Save[0] = inp(0x3BF);
	Save[1] = inp(vgaIOBase + 8);
	outp(0x3BF, 0x03);
	outp(vgaIOBase + 8, 0xA0);

	/*
	 * Check
	 */
	switch (Chipset)
	{
	case CHIP_ET3000:
		Mem = 512;
		break;
	case CHIP_ET4000:
		switch (rdinx(CRTC_IDX, 0x37) & 0x0B)
		{
		case 0x03:
		case 0x05:
			Mem = 256;
			break;
		case 0x0A:
			Mem = 512;
			break;
		case 0x0B:
			Mem = 1024;
			break;
		}
		break;
	case CHIP_ET4000W32:
	case CHIP_ET4000W32I:
	case CHIP_ET4KW32P_A:
	case CHIP_ET4KW32P_O:
		switch (rdinx(CRTC_IDX, 0x37) & 0x09)
		{
		case 0x00:
			Mem = 2048;
			break;
		case 0x01:
			Mem = 4096;
			break;
		case 0x08:
			Mem = 512;
			break;
		case 0x09:
			Mem = 1024;
			break;
		}
		if ((Chipset != CHIP_ET4000W32) &&
		    (rdinx(CRTC_IDX, 0x32) & 0x80))
		{
			Mem *= 2;
		}
		break;
	}

	/* 
	 * Lock 
	 */
	outp(vgaIOBase + 8, Save[1]);
	outp(0x3BF, Save[0]);
	DisableIOPorts(NUMPORTS, Ports);

	return(Mem);
}
