/*
 * Copyright 1998 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * IBM RAMDAC routines.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/IBM.c,v 1.2 1998/07/25 16:57:18 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#define INIT_IBM_RAMDAC_INFO
#include "IBMPriv.h"
#include "xf86RamDacPriv.h"

#define INITIALFREQERR 100000

unsigned long
IBMramdac640CalculateMNPCForClock(
    unsigned long RefClock,	/* In 100Hz units */
    unsigned long ReqClock,	/* In 100Hz units */
    char IsPixClock,	/* boolean, is this the pixel or the sys clock */
    unsigned long MinClock,	/* Min VCO rating */
    unsigned long MaxClock,	/* Max VCO rating */
    unsigned long *rM,	/* M Out */
    unsigned long *rN,	/* N Out */
    unsigned long *rP,	/* Min P In, P Out */
    unsigned long *rC	/* C Out */
)
{
  unsigned long   M, N, P, iP = *rP;
  unsigned long   IntRef, VCO, Clock;
  long            freqErr, lowestFreqErr = INITIALFREQERR;
  unsigned long   ActualClock = 0;

  for (N = 0; N <= 63; N++)
    {
      IntRef = RefClock / (N + 1);
      if (IntRef < 10000)
	break;			/* IntRef needs to be >= 1MHz */
      for (M = 2; M <= 127; M++)
	{
	  VCO = IntRef * (M + 1);
	  if ((VCO < MinClock) || (VCO > MaxClock))
	    continue;
	  for (P = iP; P <= 4; P++)
	    {
	      if (P != 0)
		Clock = (RefClock * (M + 1)) / ((N + 1) * 2 * P);
	      else
		Clock = (RefClock * (M + 1)) / (N + 1);

	      freqErr = (Clock - ReqClock);

	      if (freqErr < 0)
		{
		  /* PixelClock gets rounded up always so monitor reports
		     correct frequency. */
		  if (IsPixClock)
		    continue;
		  freqErr = -freqErr;
		}

	      if (freqErr < lowestFreqErr)
		{
		  *rM = M;
		  *rN = N;
		  *rP = P;
		  *rC = (VCO <= 1280000 ? 1 : 2);
		  ActualClock = Clock;

		  lowestFreqErr = freqErr;
		  /* Return if we found an exact match */
		  if (freqErr == 0)
		    return (ActualClock);
		}
	    }
	}
    }

  return (ActualClock);
}

unsigned long
IBMramdac526CalculateMNPCForClock(
    unsigned long RefClock,	/* In 100Hz units */
    unsigned long ReqClock,	/* In 100Hz units */
    char IsPixClock,	/* boolean, is this the pixel or the sys clock */
    unsigned long MinClock,	/* Min VCO rating */
    unsigned long MaxClock,	/* Max VCO rating */
    unsigned long *rM,	/* M Out */
    unsigned long *rN,	/* N Out */
    unsigned long *rP,	/* Min P In, P Out */
    unsigned long *rC	/* C Out */
)
{
  unsigned long   M, N, P, iP = *rP;
  unsigned long   IntRef, VCO, Clock;
  long            freqErr, lowestFreqErr = INITIALFREQERR;
  unsigned long   ActualClock = 0;

  for (N = 0; N <= 63; N++)
    {
      IntRef = RefClock / (N + 1);
      if (IntRef < 10000)
	break;			/* IntRef needs to be >= 1MHz */
      for (M = 0; M <= 63; M++)
	{
	  VCO = IntRef * (M + 1);
	  if ((VCO < MinClock) || (VCO > MaxClock))
	    continue;
	  for (P = iP; P <= 4; P++)
	    {
	      if (P)
		Clock = (RefClock * (M + 1)) / ((N + 1) * 2 * P);
	      else
		Clock = VCO;

	      freqErr = (Clock - ReqClock);

	      if (freqErr < 0)
		{
		  /* PixelClock gets rounded up always so monitor reports
		     correct frequency. */
		  if (IsPixClock)
		    continue;
		  freqErr = -freqErr;
		}

	      if (freqErr < lowestFreqErr)
		{
		  *rM = M;
		  *rN = N;
		  *rP = P;
		  *rC = (VCO <= 1280000 ? 1 : 2);
		  ActualClock = Clock;

		  lowestFreqErr = freqErr;
		  /* Return if we found an exact match */
		  if (freqErr == 0)
		    return (ActualClock);
		}
	    }
	}
    }

  return (ActualClock);
}

void
IBMramdacRestore(ScrnInfoPtr pScrn, RamDacRecPtr ramdacPtr,
				    RamDacRegRecPtr ramdacReg)
{
	int i, maxreg;

	switch (ramdacPtr->RamDacType) {
	    case IBM640_RAMDAC:
		maxreg = 0x300;
		break;
	    default:
		maxreg = 0x100;
		break;
	}

	/* Here we pass a short, so that we can evaluate a mask too */
	/* So that the mask is the high byte and the data the low byte */
	for (i=0;i<maxreg;i++) 
	    (*ramdacPtr->WriteDAC)
	        (pScrn, i, (ramdacReg->DacRegs[i] & 0xFF00) >> 8, 
						ramdacReg->DacRegs[i]);
}

void
IBMramdacSave(ScrnInfoPtr pScrn, RamDacRecPtr ramdacPtr, 
				 RamDacRegRecPtr ramdacReg)
{
	int i, maxreg;

	switch (ramdacPtr->RamDacType) {
	    case IBM640_RAMDAC:
		maxreg = 0x300;
		break;
	    default:
		maxreg = 0x100;
		break;
	}
	
	(*ramdacPtr->ReadAddress)(pScrn, 0); /* Start at index 0 */
	for (i=0;i<768;i++)
	    ramdacReg->DAC[i] = (*ramdacPtr->ReadData)(pScrn);

	for (i=0;i<maxreg;i++) 
	    ramdacReg->DacRegs[i] = (*ramdacPtr->ReadDAC)(pScrn, i);
}

int
IBMramdacProbe(ScrnInfoPtr pScrn, RamDacSupportedInfoRecPtr ramdacs/* , RamDacRecPtr ramdacPtr*/)
{
#ifdef PERSCREEN
    RamDacRecPtr ramdacPtr = RAMDACSCRPTR(pScrn);
#endif
    Bool RamDacIsSupported = FALSE;
    int IBMramdac_ID = -1;
    int i;
    unsigned char id, rev, id2, rev2;

    /* read ID and revision */
    rev = (*ramdacPtr->ReadDAC)(pScrn, IBMRGB_rev);
    id = (*ramdacPtr->ReadDAC)(pScrn, IBMRGB_id);

    /* check if ID and revision are read only */
    (*ramdacPtr->WriteDAC)(pScrn, ~rev, 0, IBMRGB_rev);
    (*ramdacPtr->WriteDAC)(pScrn, ~id, 0, IBMRGB_id);
    rev2 = (*ramdacPtr->ReadDAC)(pScrn, IBMRGB_rev);
    id2 = (*ramdacPtr->ReadDAC)(pScrn, IBMRGB_id);

    switch (id) {
	case 0x30:
		if (rev == 0xc0) IBMramdac_ID = IBM624_RAMDAC;
		if (rev == 0x80) IBMramdac_ID = IBM624DB_RAMDAC;
		break;
	case 0x12:
		if (rev == 0x1c) IBMramdac_ID = IBM640_RAMDAC;
		break;
	case 0x01:
		IBMramdac_ID = IBM525_RAMDAC;
		break;
	case 0x02:
		if (rev == 0xf0) IBMramdac_ID = IBM524_RAMDAC;
		if (rev == 0xe0) IBMramdac_ID = IBM524A_RAMDAC;
		if (rev == 0xc0) IBMramdac_ID = IBM526_RAMDAC;
		if (rev == 0x80) IBMramdac_ID = IBM526DB_RAMDAC;
		break;
    }

    if (id == 1 || id == 2) {
        if (id == id2 && rev == rev2) {		/* IBM RGB52x found */
	    /* check for 128bit VRAM -> RGB528 */
	    if (((*ramdacPtr->ReadDAC)(pScrn, IBMRGB_misc1) & 0x03) == 0x03) {
	        IBMramdac_ID = IBM528_RAMDAC;	/* 128bit DAC found */
	        if (rev == 0xe0)
		    IBMramdac_ID = IBM528A_RAMDAC;
	    }
        }
    }

    (*ramdacPtr->WriteDAC)(pScrn, rev, 0, IBMRGB_rev);
    (*ramdacPtr->WriteDAC)(pScrn, id, 0, IBMRGB_id);

    if (IBMramdac_ID == -1) {
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, 
		"Cannot determine IBM RAMDAC type, aborting\n");
	return -1;
    } else {
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, 
		"Attached RAMDAC is %s\n", IBMramdacDeviceInfo[IBMramdac_ID&0xFFFF]);
    }

    for (i=0;ramdacs[i].token != -1;i++) {
	if (ramdacs[i].token == IBMramdac_ID)
	    RamDacIsSupported = TRUE;
    }

    if (!RamDacIsSupported) {
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, 
		"This IBM RAMDAC is NOT supported by this driver, aborting\n");
	return -1;
    }
	
    return (ramdacPtr->RamDacType = IBMramdac_ID);
}

void
IBMramdacSetBpp(ScrnInfoPtr pScrn, RamDacRegRecPtr ramdacReg)
{
    /* We need to deal with Direct Colour visuals for 8bpp and other
     * good stuff for colours */
    switch (pScrn->bitsPerPixel) {
	case 32:
	    ramdacReg->DacRegs[IBMRGB_pix_fmt] = PIXEL_FORMAT_32BPP;
	    ramdacReg->DacRegs[IBMRGB_32bpp] = B32_DCOL_DIRECT;
	    ramdacReg->DacRegs[IBMRGB_24bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_16bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_8bpp] = 0;
	    break;
	case 24:
	    ramdacReg->DacRegs[IBMRGB_pix_fmt] = PIXEL_FORMAT_24BPP;
	    ramdacReg->DacRegs[IBMRGB_32bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_24bpp] = B24_DCOL_DIRECT;
	    ramdacReg->DacRegs[IBMRGB_16bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_8bpp] = 0;
	    break;
	case 16:
	    ramdacReg->DacRegs[IBMRGB_pix_fmt] = PIXEL_FORMAT_16BPP;
	    ramdacReg->DacRegs[IBMRGB_32bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_24bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_16bpp] = B16_DCOL_DIRECT | B16_LINEAR |
					       B16_CONTIGUOUS | B16_565;
	    ramdacReg->DacRegs[IBMRGB_8bpp] = 0;
	    break;
	case 15:
	    ramdacReg->DacRegs[IBMRGB_pix_fmt] = PIXEL_FORMAT_16BPP;
	    ramdacReg->DacRegs[IBMRGB_32bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_24bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_16bpp] = B16_DCOL_DIRECT | B16_LINEAR |
					       B16_CONTIGUOUS | B16_555;
	    ramdacReg->DacRegs[IBMRGB_8bpp] = 0;
	    break;
	case 8:
	    ramdacReg->DacRegs[IBMRGB_pix_fmt] = PIXEL_FORMAT_8BPP;
	    ramdacReg->DacRegs[IBMRGB_32bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_24bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_16bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_8bpp] = B8_DCOL_INDIRECT;
	    break;
	case 4:
	    ramdacReg->DacRegs[IBMRGB_pix_fmt] = PIXEL_FORMAT_4BPP;
	    ramdacReg->DacRegs[IBMRGB_32bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_24bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_16bpp] = 0;
	    ramdacReg->DacRegs[IBMRGB_8bpp] = 0;
    }
}
