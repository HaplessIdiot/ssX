/*
 * (c) Copyright 1996 Alan Hourihane <alanh@fairlite.demon.co.uk>
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
 * ALAN HOURIHANE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Alan Hourihane shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Alan Hourihane.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/SuperProbe/SiS.c,v 3.3 1997/02/25 14:20:02 hohndel Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, SEQ_IDX, SEQ_REG, 0x3c4, 0x3c5};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_SiS __STDCARGS((int));

Chip_Descriptor SiS_Descriptor = {
	"SiS",
	Probe_SiS,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_SiS,
};

Bool Probe_SiS(Chipset)
int *Chipset;
{
	int i = 0;

	if (!NoPCI)
	{
	    while ((pcrp = pci_devp[i]) != (struct pci_config_reg *)NULL) {
		if (pcrp->_vendor == PCI_VENDOR_SIS)
		{
			switch (pcrp->_device)
			{
			case PCI_CHIP_SG86C201:
				*Chipset = CHIP_SIS86C201;
				break;
			case PCI_CHIP_SG86C202:
				*Chipset = CHIP_SIS86C202;
				break;
			case PCI_CHIP_SG86C205:
				*Chipset = CHIP_SIS86C205;
				break;
			case PCI_CHIP_SG86C215:
				*Chipset = CHIP_SIS86C215;
				break;
			case PCI_CHIP_SG86C225:
				*Chipset = CHIP_SIS86C225;
				break;
			case PCI_CHIP_SIS5597:
				*Chipset = CHIP_SIS5597;
				break;
			case PCI_CHIP_SIS530:
				*Chipset = CHIP_SIS530;
				break;
			case PCI_CHIP_SIS6326:
				*Chipset = CHIP_SIS6326;
				break;
			default:
				*Chipset = CHIP_SIS_UNK;
				break;
			}
			PCIProbed = TRUE;
			return(TRUE);
		}
		i++;
	    }
	}

        return(FALSE);
}

static int MemProbe_SiS(Chipset)
int Chipset;
{
	int Mem = 0;
	int temp;

        EnableIOPorts(NUMPORTS, Ports);
	outpw(0x3C4, 0x8605);	/* Unlock extended registers */ 

	switch (Chipset)
	{
	case CHIP_SIS86C201:
	case CHIP_SIS86C202:
	case CHIP_SIS86C205:
	case CHIP_SIS86C215:
	case CHIP_SIS86C225:
		switch (rdinx(0x3C4, 0xF) & 0x03)
		{
		case 0x00:
			Mem = 1024;
			break;
		case 0x01:
			Mem = 2048;
			break;
		case 0x02:
			Mem = 4096;
			break;
		}
		break;
	case CHIP_SIS5597:
	  temp = rdinx(0x3C4, 0x2F);
	  temp &= 7;
	  temp++;
	  Mem = temp * 256;
	  if ((rdinx(0x3C4, 0x2F)>>1)&3)
	    Mem *= 2;
	  break;
	case CHIP_SIS530:
	case CHIP_SIS6326:
	  temp = rdinx(0x3C4, 0x0C);
	  temp >>=1;
	  switch (temp & 0x0B)
	    {
	    case 0:
	      Mem = 1024;
	      break;
	    case 1:
	      Mem = 2048;
	      break;
	    case 2:
	      Mem = 4096;
	      break;
	    case 3:
	      Mem = 1024;
	      break;
	    case 8:
	      Mem = 0;
	      break;
	    case 9:
	      Mem = 2048;
	      break;
	    case 0xA:
	      Mem = 4096;
	      break;
	    case 0xB:
	      Mem = 8192;
	      break;
	    }
	}

	outpw(0x3C4, 0x0005);	/* Lock extended registers */ 
        DisableIOPorts(NUMPORTS, Ports);
	return(Mem);
    }
