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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Trident.c,v 2.5 1994/02/28 14:09:56 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, SEQ_IDX, SEQ_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_Trident __STDCARGS((int));

Chip_Descriptor Trident_Descriptor = {
	"Trident",
	Probe_Trident,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_Trident,
};

Bool Probe_Trident(Chipset)
int *Chipset;
{
        Bool result = FALSE;
	Byte chip, old, old1, val;

	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
        EnableIOPorts(NUMPORTS, Ports);

	old = rdinx(SEQ_IDX, 0x0B);
	wrinx(SEQ_IDX, 0x0B, 0x00);
	chip = inp(SEQ_REG);
	old1 = rdinx(SEQ_IDX, 0x0E);
	outp(SEQ_REG, 0);
	val = inp(SEQ_REG);
	outp(SEQ_REG, (old1 ^ 0x02));
	wrinx(0x3C4, 0x0B, old);
	if ((val & 0x0F) == 2)
	{
		result = TRUE;
		switch (chip)
		{
		case 0x01:		/* Can't happen - no "new mode" */
			*Chipset = CHIP_TVGA8800BR;
			break;
		case 0x02:
			*Chipset = CHIP_TVGA8800CS;
			break;
		case 0x03:
			*Chipset = CHIP_TVGA8900B;
			break;
		case 0x04:
		case 0x13:
			*Chipset = CHIP_TVGA8900C;
			break;
		case 0x23:
			*Chipset = CHIP_TVGA9000;
			break;
		case 0x33:
			*Chipset = CHIP_TVGA8900CL;
			break;
		case 0x43:
			*Chipset = CHIP_TVGA9000I;
			break;
		case 0x53:
			*Chipset = CHIP_TVGA9200CX;
			break;
		case 0x63:
			*Chipset = CHIP_TVGA9100B;
			break;
		case 0x73:
		case 0xF3:
			*Chipset = CHIP_TVGA9420;
			break;
		case 0x83:
			*Chipset = CHIP_TVGA8200;
			break;
		case 0x93:
			*Chipset = CHIP_TVGA9400CX;
			break;
		case 0xA3:
			*Chipset = CHIP_TVGA9320;
			break;
		default:
			Chip_data = chip;
			*Chipset = CHIP_TVGA_UNK;
			break;
		}
	}
	else if ((chip == 1) && (testinx2(SEQ_IDX, 0x0E, 0x06)))
	{
		result = TRUE;
		*Chipset = CHIP_TVGA8800BR;
	}

        DisableIOPorts(NUMPORTS, Ports);
        return(result);
}

static int MemProbe_Trident(Chipset)
int Chipset;
{
	int Mem = 0;

        EnableIOPorts(NUMPORTS, Ports);

	switch (Chipset)
	{
	case CHIP_TVGA8800BR:
	case CHIP_TVGA8800CS:
		if (rdinx(CRTC_IDX, 0x1F) & 0x02)
		{
			Mem = 512;
		}
		else
		{
			Mem = 256;
		}
		break;
	default:
		switch (rdinx(CRTC_IDX, 0x1F) & 0x07)
		{
		case 0x00:
			Mem = 256;
			break;
		case 0x01:
			Mem = 512;
			break;
		case 0x02:
			Mem = 768;
			break;
		case 0x03:
			Mem = 1024;
			break;
		case 0x07:
			Mem = 2048;
			break;
		}
		break;
	}

        DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
