/*
 * (c) Copyright 1993,1994 by David Wexelblat <dwex@xfree86.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * DAVID WEXELBLAT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of David Wexelblat shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from David Wexelblat.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/SuperProbe/ATI.c,v 3.0 1994/05/14 06:50:37 dawes Exp $ */

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
