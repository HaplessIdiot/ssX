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

/* $XFree86: mit/server/ddx/x386/SuperProbe/ATI.c,v 2.7 1994/03/07 13:56:04 dawes Exp $ */

#include "Probe.h"

static int MemProbe_ATI __STDCARGS((int));

Chip_Descriptor ATI_Descriptor = {
	"ATI",
	Probe_ATI,
	NULL,
	0,
	FALSE,
	TRUE,
	TRUE,
	MemProbe_ATI,
};

#ifdef __STDC__
Bool Probe_ATI(int *Chipset)
#else
Bool Probe_ATI(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte bios[10];
	Byte *signature = (Byte *)"761295520";
	Word GUP;

	if (ReadBIOS(0x31, bios, 9) != 9)
	{
		fprintf(stderr, "%s: Failed to read ATI signature\n", MyName);
		return(FALSE);
	}
	if (memcmp(bios, signature, 9) == 0)
	{
		if (ReadBIOS(0x40, bios, 4) != 4)
		{
			fprintf(stderr, "%s: Failed to read ATI BIOS data\n",
				MyName);
			return(FALSE);
		}
		if ((bios[0] == '3') && (bios[1] == '1'))
		{
			result = TRUE;
			switch (bios[3])
			{
			case '1':
				*Chipset = CHIP_ATI18800;
				break;
			case '2':
				*Chipset = CHIP_ATI18800_1;
				break;
			case '3':
				*Chipset = CHIP_ATI28800_2;
				break;
			case '4':
				*Chipset = CHIP_ATI28800_4;
				break;
			case '5':
				*Chipset = CHIP_ATI28800_5;
				break;
			case 'a':
			case 'b':
			case 'c':
				/*
				 * 68800-??
				 *
				 * Chipset ID encoded as:
				 *	Bits 0-4 - low letter
				 *	Bits 5-9 - high letter
				 *  Add 0x41 to each to get ascii letter.
				 *
				 * Started at 68800-6, so 0 shows up for the
				 * 68800-3.
				 */
				GUP = inpw(CHIP_ID);
				switch (GUP & 0x03FF)
				{
				case 0x0000:
					*Chipset = CHIP_ATI68800_3;
					break;
				case 0x02F7:	/* XX */
					*Chipset = CHIP_ATI68800_6;
					break;
				case 0x0177:	/* LX */
					*Chipset = CHIP_ATI68800LX;
					break;
				case 0x0017:	/* AX */
					*Chipset = CHIP_ATI68800AX;
					break;
				default:
					Chip_data = ((GUP >> 5) & 0x1F) + 0x41;
					*Chipset = CHIP_ATI_UNK;
					break;
				}
				break;
			default:
				Chip_data = bios[3];
				*Chipset = CHIP_ATI_UNK;
				break;
			}
		}
	}
	return(result);
}

static int MemProbe_ATI(Chipset)
int Chipset;
{
	Word Ports[3] = {0x000, 0x000, MISC_OPTIONS};
	Byte bios[20];
	int Mem = 0;

	if (ReadBIOS(0x40, bios, 20) != 20)
	{
		fprintf(stderr, "%s: Failed to read ATI BIOS data\n", MyName);
		return(0);
	}
	Ports[0] = *((Word *)bios + 0x08);
	Ports[1] = Ports[0] + 1;
	EnableIOPorts(3, Ports);

	switch (Chipset)
	{
	case CHIP_ATI18800:
	case CHIP_ATI18800_1:
		if (rdinx(Ports[0], 0xBB) & 0x20)
		{
			Mem = 512;
		}
		else
		{
			Mem = 256;
		}
		break;
	case CHIP_ATI28800_2:
		if (rdinx(Ports[0], 0xB0) & 0x10)
		{
			Mem = 512;
		}
		else
		{
			Mem = 256;
		}
		break;
	case CHIP_ATI28800_4:
	case CHIP_ATI28800_5:
		switch (rdinx(Ports[0], 0xB0) & 0x18)
		{
		case 0x00:
			Mem = 256;
			break;
		case 0x10:
			Mem = 512;
			break;
		case 0x08:
		case 0x18:
			Mem = 1024;
			break;
		}
		break;
	default:
		/* GUP */
		switch ((inpw(MISC_OPTIONS) & 0x000C) >> 2)
		{
		case 0x00:
			Mem = 512;
			break;
		case 0x01:
			Mem = 1024;
			break;
		case 0x02:
			Mem = 2048;
			break;
		case 0x03:
			Mem = 4096;
			break;
		}
		break;
	}

	DisableIOPorts(3, Ports);
	return(Mem);

}
