/*
 * (c) Copyright 1993,1994 by Robin Cutshaw <robin@xfree86.org>
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
 * Except as contained in this notice, the name of Robin Cutshaw shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Robin Cutshaw.
 *
 */

/* $XFree86$ */

#include "Probe.h"

static Word Ports[] = {0xCF8, 0xCFA, 0x0000, 0x0000 };

#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_I128 __STDCARGS((int));
static int I128Mem = 0;

Chip_Descriptor I128_Descriptor = {
	"I128",
	Probe_I128,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE,
	MemProbe_I128,
};

Bool Probe_I128(Chipset)
int *Chipset;
{
	Bool result = FALSE;
	Word ioaddr, vendor, device, iobase;
	Long id, vendordevice;

	EnableIOPorts(NUMPORTS, Ports);

	outp(0xCF8, 0x80);
	outp(0xCFA, 0x00);

	DisableIOPorts(NUMPORTS, Ports);

	for (ioaddr = 0xC000; ioaddr < 0xD000; ioaddr += 0x0100) {
	    Ports[2] = ioaddr;
	    EnableIOPorts(NUMPORTS, Ports);

	    vendordevice = inpl(ioaddr);
            vendor = (unsigned short )(vendordevice & 0xFFFF);
            device = (unsigned short )((vendordevice >> 16) & 0xFFFF);

#ifdef DEBUG
	printf("pci slot at 0x%04x, vendor 0x%04x board 0x%04x\n",
	    ioaddr, vendor, board);
#endif

	    if ((vendor != 0x105D) || (device != 0x2309)) {
	        DisableIOPorts(NUMPORTS, Ports);
	        continue;
	    }

            iobase = inpl(ioaddr + 0x24) & 0xFFFFFF00;

	    DisableIOPorts(NUMPORTS, Ports);
	    Ports[3] = iobase;
	    EnableIOPorts(NUMPORTS, Ports);

	    id = inpl(iobase + 0x18) & 0x7FFFFFFF;
	    switch (id & 0xC0) {
	        case 0x00:
	            I128Mem =  4096;
	            break;
	        case 0x40:
	            I128Mem =  8192;
	            break;
	        case 0x80:
	            I128Mem =  16384;
	            break;
	        case 0xC0:
	            I128Mem =  32768;
	            break;
	    }

	    *Chipset = CHIP_I128;
	    result = TRUE;
	    DisableIOPorts(NUMPORTS, Ports);
	    Ports[3] = 0x0000;
	    break;
	}

	Ports[2] = Ports[3] = 0x0000;
	EnableIOPorts(NUMPORTS, Ports);
	outp(0xCF8, 0x00);
	DisableIOPorts(NUMPORTS, Ports);

	return(result);
}

static int MemProbe_I128(Chipset)
int Chipset;
{
	return(I128Mem);
}
