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

/* $XFree86: mit/server/ddx/x386/SuperProbe/S3.c,v 2.6 1994/02/28 14:10:50 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_S3 __STDCARGS((int));

Chip_Descriptor S3_Descriptor = {
	"S3",
	Probe_S3,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE,
	MemProbe_S3,
};

Bool Probe_S3(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Byte old, tmp, rev;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);
	old = rdinx(CRTC_IDX, 0x38);
	wrinx(CRTC_IDX, 0x38, 0x00);
	if (!testinx2(CRTC_IDX, 0x35, 0x0F))
	{
		wrinx(CRTC_IDX, 0x38, 0x48);
		if (testinx2(CRTC_IDX, 0x35, 0x0F))
		{
			result = TRUE;
			rev = rdinx(CRTC_IDX, 0x30);
			switch (rev & 0xF0)
			{
			case 0x80:
				switch (rev & 0x0F)
				{
				case 0x01:
					*Chipset = CHIP_S3_911;
					break;
				case 0x02:
					*Chipset = CHIP_S3_924;
					break;
				default:
					Chip_data = rev;
					*Chipset = CHIP_S3_UNKNOWN;
					break;
				}
				break;
			case 0xA0:
				tmp = rdinx(CRTC_IDX, 0x36);
				switch (tmp & 0x03)
				{
				case 0x00:
				case 0x01:
					/* EISA or VLB - 805 */
					switch (rev & 0x0F)
					{
					case 0x00:
						*Chipset = CHIP_S3_805B;
						break;
					case 0x01:
						Chip_data = rev;
						*Chipset = CHIP_S3_UNKNOWN;
						break;
					case 0x02:
					case 0x03:
					case 0x04:
						*Chipset = CHIP_S3_805C;
						break;
					case 0x05:
						*Chipset = CHIP_S3_805D;
						break;
					case 0x08:
						*Chipset = CHIP_S3_805I;
						break;
					default:
						/* Call >0x05 D step for now */
						*Chipset = CHIP_S3_805D;
						break;
					}
					break;
				case 0x03:
					/* ISA - 801 */
					switch (rev & 0x0F)
					{
					case 0x00:
						*Chipset = CHIP_S3_801B;
						break;
					case 0x01:
						Chip_data = rev;
						*Chipset = CHIP_S3_UNKNOWN;
						break;
					case 0x02:
					case 0x03:
					case 0x04:
						*Chipset = CHIP_S3_801C;
						break;
					case 0x05:
						*Chipset = CHIP_S3_801D;
						break;
					case 0x08:
						*Chipset = CHIP_S3_801I;
						break;
					default:
						/* Call >0x05 D step for now */
						*Chipset = CHIP_S3_801D;
						break;
					}
					break;
				default:
					Chip_data = rev;
					*Chipset = CHIP_S3_UNKNOWN;
					break;
				}
				break;
			case 0x90:
				switch (rev & 0x0F)
				{
				case 0x00:
				case 0x01:
					/*
					 * Contradictory documentation -
					 * one says 0, the other says 1.
					 */
					*Chipset = CHIP_S3_928D;
					break;
				case 0x02:
				case 0x03:
					Chip_data = rev;
					*Chipset = CHIP_S3_UNKNOWN;
					break;
				case 0x04:
				case 0x05:
					*Chipset = CHIP_S3_928E;
					break;
				default:
					/* Call >0x05 E step for now */
					*Chipset = CHIP_S3_928E;
				}
				break;
			case 0xB0:
				/*
				 * Don't know anything more about this
				 * just yet.
				 */
				*Chipset = CHIP_S3_928P;
				break;
			case 0xC0:
				*Chipset = CHIP_S3_864;
				break;
			case 0xD0:
				*Chipset = CHIP_S3_964;
				break;
			default:
				Chip_data = rev;
				*Chipset = CHIP_S3_UNKNOWN;
				break;
			}
		}
	}
	wrinx(CRTC_IDX, 0x38, old);
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

static int MemProbe_S3(Chipset)
int Chipset;
{
	Byte config, old;
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	old = rdinx(CRTC_IDX, 0x38);
	wrinx(CRTC_IDX, 0x38, 0x00);
	if (!testinx2(CRTC_IDX, 0x35, 0x0F))
	{
		wrinx(CRTC_IDX, 0x38, 0x48);
		if (testinx2(CRTC_IDX, 0x35, 0x0F))
		{
			config = rdinx(CRTC_IDX, 0x36);
			if ((config & 0x20) != 0)
			{
				Mem = 512;
			}
			else
			{
				if ((Chipset == CHIP_S3_911) || 
				    (Chipset == CHIP_S3_924))
				{
					Mem = 1024;
				}
				else
				{
					switch((config & 0xC0) >> 6)
					{
					case 0:
						Mem = 4096;
						break;
					case 1:
						Mem = 3072;
						break;
					case 2:
						Mem = 2048;
						break;
					case 3:
						Mem = 1024;
						break;
					}
				}
			}
		}
	}
	wrinx(CRTC_IDX, 0x38, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
