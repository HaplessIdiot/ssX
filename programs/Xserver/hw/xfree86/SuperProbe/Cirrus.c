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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Cirrus.c,v 2.8 1994/04/15 05:09:39 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, SEQ_IDX, SEQ_REG, GRC_IDX, GRC_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Cirrus_Descriptor = {
	"Cirrus",
	Probe_Cirrus,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	NULL,
};

static int MemProbe_Cirrus54 __STDCARGS((int));

Chip_Descriptor Cirrus54_Descriptor = {
	"Cirrus54",
	Probe_Cirrus54,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_Cirrus54,
};

static int MemProbe_Cirrus64 __STDCARGS((int));

Chip_Descriptor Cirrus64_Descriptor = {
	"Cirrus64",
	Probe_Cirrus64,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_Cirrus64,
};

#define CLASS_OLD	0
#define CLASS_54XX	1
#define CLASS_64XX	2

#ifdef __STDC__
static Bool Probe_Cirrus_Class(int *, int);
#else
static Bool Probe_Cirrus_Class();
#endif

Bool Probe_Cirrus(Chipset)
int *Chipset;
{
	return(Probe_Cirrus_Class(Chipset, CLASS_OLD));
}

Bool Probe_Cirrus54(Chipset)
int *Chipset;
{
	return(Probe_Cirrus_Class(Chipset, CLASS_54XX));
}

Bool Probe_Cirrus64(Chipset)
int *Chipset;
{
	return(Probe_Cirrus_Class(Chipset, CLASS_64XX));
}

static Bool Probe_Cirrus_Class(Chipset, Class)
int *Chipset;
int Class;
{
	Bool result = FALSE;
	Byte old, old1, Ver;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);

	if (Class == CLASS_OLD)
	{
		old = rdinx(CRTC_IDX, 0x0C);
		old1 = rdinx(SEQ_IDX, 0x06);
		wrinx(CRTC_IDX,0x0C,0);
		Ver = rdinx(CRTC_IDX, 0x1F);
		wrinx(SEQ_IDX, 0x06, (Ver >> 4) | (Ver << 4));
		if (inp(SEQ_REG) == 0)
		{
			outp(SEQ_REG, Ver);
			if (inp(SEQ_REG) == 1)
			{
				result = TRUE;
				switch (Ver)
				{
				case 0xEC:
					*Chipset = CHIP_CL510;
					break;
				case 0xCA:
					*Chipset = CHIP_CL610;
					break;
				case 0xEA:
					*Chipset = CHIP_CLV7;
					break;
				default:
					Chip_data = Ver;
					*Chipset = CHIP_CL_UNKNOWN;
					break;
				}
			}
		}
		wrinx(SEQ_IDX, 0x06, old1);
		wrinx(CRTC_IDX, 0x0C, old);
	}
	else if (Class == CLASS_54XX)
	{
		/*
		 * Cirrus 542x, 543x, 62x5 chips
	 	 */
		old = rdinx(SEQ_IDX, 0x06);
		wrinx(SEQ_IDX, 0x06, 0x00);
		if (rdinx(SEQ_IDX, 0x06) == 0x0F)
		{
			wrinx(SEQ_IDX, 0x06, 0x12);
			if ((rdinx(SEQ_IDX, 0x06) == 0x12) &&
		    	    (testinx2(SEQ_IDX, 0x1E, 0x3F)))
			{
				result = TRUE;
				Ver = rdinx(CRTC_IDX, 0x27);
				if (testinx(GRC_IDX, 0x09))
				{
					/* 542x */
					switch ((Ver & 0xFC) >> 2)
					{
					case 0x06:
						*Chipset = CHIP_CLAVGA2;
						break;
					case 0x22:
						switch (Ver & 0x03)
						{
						case 0x00:
							*Chipset=CHIP_CL5402;
							break;
						case 0x01:
							*Chipset=CHIP_CL5402R1;
							break;
						case 0x02:
							*Chipset=CHIP_CL5420;
							break;
						case 0x03:
							*Chipset=CHIP_CL5420R1;
							break;
						}
						break;
					case 0x23:
						*Chipset = CHIP_CL5422;
						break;
					case 0x25:
						*Chipset = CHIP_CL5424;
						break;
					case 0x24:
						*Chipset = CHIP_CL5426;
						break;
					case 0x26:
						*Chipset = CHIP_CL5428;
						break;
					case 0x27:
						*Chipset = CHIP_CL5429;
						break;
					case 0x29:
						*Chipset = CHIP_CL543X;
						break;
					default:
						Chip_data = Ver;
						*Chipset = CHIP_CL_UNKNOWN;
						break;
					}
				}
				else if (testinx(SEQ_IDX, 0x19))
				{
					/* 62x5 */
					switch ((Ver & 0xC0) >> 6)
					{
					case 0x00:
						*Chipset = CHIP_CL6205;
						break;
					case 0x01:
						*Chipset = CHIP_CL6235;
						break;
					case 0x02:
						*Chipset = CHIP_CL6215;
						break;
					case 0x03:
						*Chipset = CHIP_CL6225;
						break;
					}
				}
				else
				{
					*Chipset = CHIP_CLAVGA2;
				}
			}
		}
		wrinx(SEQ_IDX, 0x06, old);
	}
	else
	{
		/*
		 * The Cirrus 64xx chips.
	 	 */
		old = rdinx(GRC_IDX, 0x0A);
		wrinx(GRC_IDX, 0x0A, 0xCE);
		if (rdinx(GRC_IDX, 0x0A) == 0x00)
		{
			wrinx(GRC_IDX, 0x0A, 0xEC);
			if (rdinx(GRC_IDX, 0x0A) == 0x01)
			{
				result = TRUE;
				Ver = rdinx(GRC_IDX, 0xAA);
				switch ((Ver & 0xF0) >> 4)
				{
				case 0x04:
					*Chipset = CHIP_CL6440;
					break;
				case 0x05:
					*Chipset = CHIP_CL6412;
					break;
				case 0x06:
					*Chipset = CHIP_CL5410;
					break;
				case 0x07:
					if (testinx2(0x3CE, 0x87, 0x90))
					{
						*Chipset = CHIP_CL6420B;
					}
					else
					{
						*Chipset = CHIP_CL6420A;
					}
					break;
				case 0x08:
					*Chipset = CHIP_CL6410;	
					break;
				default:
					Chip_data = Ver;
					*Chipset = CHIP_CL_UNKNOWN;
					break;
				}
			}
		}
		wrinx(GRC_IDX, 0x0A, old);
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

static int MemProbe_Cirrus54(Chipset)
int Chipset;
{
	Byte old;
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Unlock
	 */
	old = rdinx(SEQ_IDX, 0x06);
	wrinx(SEQ_IDX, 0x06, 0x12);

	/*
	 * Check
	 */
	switch (Chipset)
	{
	case CHIP_CL6205:
	case CHIP_CL6215:
	case CHIP_CL6225:
	case CHIP_CL6235:
		Mem = 512;
		break;
	default:
		switch ((rdinx(SEQ_IDX, 0x0A) & 0x18) >> 3)
		{
		case 0x00:
			Mem = 256;
			break;
		case 0x01:
			Mem = 512;
			break;
		case 0x02:
			Mem = 1024;
			break;
		case 0x03:
			Mem = 2048;
			break;
		}
		break;
	}

	/*
	 * Lock
	 */
	wrinx(SEQ_IDX, 0x06, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}

/* ARGSUSED */
static int MemProbe_Cirrus64(Chipset)
int Chipset;
{
	Byte old;
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Unlock
	 */
	old = rdinx(GRC_IDX, 0x0A);
	wrinx(GRC_IDX, 0x0A, 0xEC);

	/*
	 * Check
	 */
	switch (rdinx(GRC_IDX, 0xBB) >> 6)
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
	}

	/*
	 * Lock
	 */
	wrinx(GRC_IDX, 0x0A, old);

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
