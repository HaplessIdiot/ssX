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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Compaq.c,v 2.6 1994/03/02 08:04:55 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {GRC_IDX, GRC_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_Compaq __STDCARGS((int));

Chip_Descriptor Compaq_Descriptor = {
	"Compaq",
	Probe_Compaq,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE,
	MemProbe_Compaq,
};

Bool Probe_Compaq(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old, ver;

	EnableIOPorts(NUMPORTS, Ports);

	old = rdinx(GRC_IDX, 0x0F);
	wrinx(GRC_IDX, 0x0F, 0x00);
	if (!testinx(GRC_IDX, 0x45))
	{
		wrinx(GRC_IDX, 0x0F, 0x05);
		if (testinx(GRC_IDX, 0x45))
		{
			result = TRUE;
			ver = rdinx(GRC_IDX, 0x0C) >> 3;
			switch (ver)
			{
			case 0x03:
				*Chipset = CHIP_CPQ_IVGS;
				break;
			case 0x05:
				*Chipset = CHIP_CPQ_AVGA;
				break;
			case 0x06:
				*Chipset = CHIP_CPQ_Q1024;
				break;
			case 0x0E:
				if (rdinx(GRC_IDX, 0x56) & 0x04)
				{
					*Chipset = CHIP_CPQ_Q1280;
				}
				else
				{
					*Chipset = CHIP_CPQ_Q1024;
				}
				break;
			case 0x10:
				*Chipset = CHIP_CPQ_AVGA_P;
				break;
			default:
				Chip_data = ver >> 3;
				*Chipset = CHIP_CPQ_UNK;
				break;
			}
		}
	}
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

static int MemProbe_Compaq(Chipset)
int Chipset;
{
	Byte old, temp;
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Unlock.
	 */
	old = rdinx(GRC_IDX, 0x0F);
	wrinx(GRC_IDX, 0x0F, 0x05);

	/*
	 * Check
	 */
	switch (Chipset)
	{
	case CHIP_CPQ_IVGS:
		Mem = 256;
		break;
	case CHIP_CPQ_AVGA:
	case CHIP_CPQ_AVGA_P:
		Mem = 512;
		break;
	case CHIP_CPQ_Q1024:
	case CHIP_CPQ_Q1280:
		temp = rdinx(GRC_IDX, 0x0F);
		wrinx(GRC_IDX, 0x0F, 0x05);
		switch (rdinx(GRC_IDX, 0x54))
		{
		case 0x00:
			Mem = 1024;
			break;
		case 0x02:
			Mem = 512;
			break;
		case 0x04:
			Mem = 1024;
			break;
		case 0x08:
			Mem = 2048;
			break;
		}
		wrinx(GRC_IDX, 0x0F, temp);
		break;
	}

	/*
	 * Lock 
	 */
	wrinx(GRC_IDX, 0x0F, 0x05);

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
