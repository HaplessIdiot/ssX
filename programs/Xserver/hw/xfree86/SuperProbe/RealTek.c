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

/* $XFree86: mit/server/ddx/x386/SuperProbe/RealTek.c,v 2.3 1994/02/28 14:09:53 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, 0x3D6, 0x3D7};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_RealTek __STDCARGS((int));

Chip_Descriptor RealTek_Descriptor = {
	"RealTek",
	Probe_RealTek,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_RealTek,
};

#ifdef __STDC__
Bool Probe_RealTek(int *Chipset)
#else
Bool Probe_RealTek(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte ver;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);

	if (testinx2(CRTC_IDX, 0x1F, 0x3F) &&
	    tstrg(0x3D6, 0x0F) &&
	    tstrg(0x3D7, 0x0F))
	{
		result = TRUE;
		ver = rdinx(CRTC_IDX, 0x1A) >> 6;
		switch (ver)
		{
		case 0x00:
			*Chipset = CHIP_RT_3103;
			break;
		case 0x01:
			*Chipset = CHIP_RT_3105;
			break;
		case 0x02:
			*Chipset = CHIP_RT_3106;
			break;
		default:
			Chip_data = ver;
			*Chipset = CHIP_RT_UNK;
			break;
		}
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

static int MemProbe_RealTek(Chipset)
int Chipset;
{
	int Mem = 0;

	EnableIOPorts(NUMPORTS, Ports);

	switch (Chipset)
	{
	case CHIP_RT_3103:
		switch (rdinx(CRTC_IDX, 0x1E) & 0x03)
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
		break;
	case CHIP_RT_3105:
	case CHIP_RT_3106:
		switch (rdinx(CRTC_IDX, 0x1E) & 0x03)
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

	DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
}
